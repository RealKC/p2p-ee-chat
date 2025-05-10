use std::{iter, net::SocketAddr, sync::Arc};

use axum::{
    extract::{
        ConnectInfo, State, WebSocketUpgrade,
        ws::{Message as WsMessage, WebSocket},
    },
    response::IntoResponse,
};
use dashmap::DashMap;
use futures::SinkExt;
use futures_channel::mpsc::{self, UnboundedSender};
use futures_util::{StreamExt, TryStreamExt, future, pin_mut};
use rand::{Rng, distr::Alphanumeric};
use tokio_tungstenite::tungstenite::Error as WsError;

type Tx = UnboundedSender<WsMessage>;
pub type Channels = DashMap<String, Tx>;

#[derive(serde::Deserialize, serde::Serialize, PartialEq, Eq, Debug)]
#[serde(rename_all = "kebab-case")]
enum MessageType {
    Offer,
    Answer,
    IceCandidate,
    Identify,
}

#[derive(serde::Deserialize, serde::Serialize)]
#[serde(rename_all = "camelCase")]
struct Message {
    r#type: MessageType,
    user_id: String,
    data: Option<String>,
}

pub async fn handle_connection(
    ws: WebSocketUpgrade,
    ConnectInfo(addr): ConnectInfo<SocketAddr>,
    State(state): State<Arc<Channels>>,
) -> impl IntoResponse {
    tracing::info!("Incoming TCP connection from {addr}");

    ws.on_upgrade(move |ws| handle_socket(state, ws, addr))
}

async fn handle_socket(state: Arc<Channels>, ws: WebSocket, addr: SocketAddr) {
    let my_id = {
        let mut rng = rand::rng();

        // These better be unique because I won't check for collisions
        iter::repeat_with(|| rng.sample(Alphanumeric))
            .map(char::from)
            .take(8)
            .collect::<String>()
    };

    let (tx, rx) = mpsc::unbounded();
    state.insert(my_id.clone(), tx);

    let (outgoing, incoming) = ws.split();

    let handle_incoming_messages = incoming.try_for_each(|msg| {
        let id = my_id.clone();
        let state = state.clone();

        async move {
            tracing::trace!("Received a message from {addr}: {msg:?}");

            if let WsMessage::Close(_) = msg {
                tracing::info!("User {id} died...");
                state.remove(&id);
                return Err(axum::Error::new(WsError::ConnectionClosed));
            }

            let Ok(text) = msg.to_text() else {
                tracing::info!("Received non-text messsage: {msg:?}");
                return Ok(());
            };

            let Ok(deserialized) = serde_json::from_str::<Message>(text) else {
                tracing::info!("Received invalid JSON: {msg:?}, text='{text}'");
                return Ok(());
            };

            if let Some(mut tx) = state.get_mut(&deserialized.user_id) {
                tracing::info!(
                    "user: {id} is sending {} a {:?}",
                    deserialized.user_id,
                    deserialized.r#type,
                );

                // Replace this user's id with the peer's id in the message so the peer knows who to reply to
                tx.send(WsMessage::Text(
                    serde_json::to_string(&Message {
                        r#type: deserialized.r#type,
                        data: deserialized.data,
                        user_id: id,
                    })
                    .unwrap()
                    .into(),
                ))
                .await
                .unwrap();
            } else {
                tracing::info!(
                    "User {id} tried messaging {}, but the latter user does not exist...",
                    deserialized.user_id
                );
            }

            Ok(())
        }
    });

    let identify_msg = serde_json::to_string(&Message {
        r#type: MessageType::Identify,
        user_id: my_id.clone(),
        data: None,
    })
    .unwrap();
    tracing::info!("generated {identify_msg}");
    let fwd = futures::stream::iter(iter::once(WsMessage::Text(identify_msg.into())))
        .chain(rx)
        .map(Ok)
        .forward(outgoing);

    pin_mut!(handle_incoming_messages, fwd);
    future::select(handle_incoming_messages, fwd).await;
}
