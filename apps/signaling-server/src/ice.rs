use axum::response::IntoResponse;

pub async fn get() -> impl IntoResponse {
    let api_key = std::env::var("METERED_API_KEY").unwrap();
    let a = reqwest::get(format!(
        "https://dumitruac.metered.live/api/v1/turn/credentials?apiKey={api_key}"
    ))
    .await
    .unwrap();

    a.text().await.unwrap()
}
