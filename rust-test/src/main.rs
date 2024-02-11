use axum::{
    http::{HeaderMap, Method},
    response::Html,
    routing::post,
    Router,
};

#[tokio::main]
async fn main() {
    let app = Router::new().route("/", post(handler));

    let listener = tokio::net::TcpListener::bind("127.0.0.1:3000")
        .await
        .unwrap();
    println!("listening on {}", listener.local_addr().unwrap());
    axum::serve(listener, app).await.unwrap();
}

async fn handler(method: Method, headers: HeaderMap, body: String) -> Html<&'static str> {
    dbg!(&method);
    dbg!(&headers);
    dbg!(&body);
    Html("Hello, World!")
}
