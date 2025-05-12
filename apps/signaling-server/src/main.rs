mod ice;
mod ws;

use std::{env, net::SocketAddr, sync::Arc};

use axum::{
    Router,
    http::{HeaderValue, Method},
    routing::{any, get},
};
use tower_http::cors::{CorsLayer, Any};
use ws::Channels;

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt::init();

    let addr = env::args()
        .nth(1)
        .unwrap_or_else(|| "0.0.0.0:8080".to_string());

    let state = Arc::new(Channels::new());

    let app = Router::new()
        .route("/ws", any(ws::handle_connection))
        .route("/ice", get(ice::get))
        .layer(
            CorsLayer::new()
                .allow_origin(Any)
                .allow_methods([Method::GET, Method::POST]),
        )
        .with_state(state);

    let listener = tokio::net::TcpListener::bind(&addr).await.unwrap();

    tracing::info!("Listening on: {}", addr);

    axum::serve(
        listener,
        app.into_make_service_with_connect_info::<SocketAddr>(),
    )
    .await
    .unwrap()
}
