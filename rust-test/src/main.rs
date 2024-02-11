//! Run with
//!
//! ```not_rust
//! cargo run -p example-hello-world
//! ```

use axum::{
    http::{HeaderMap, Method},
    response::Html,
    routing::post,
    Router,
};

#[tokio::main]
async fn main() {
    // build our application with a route
    let app = Router::new().route("/", post(handler));

    // run it
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
    Html("<h1>Hello, World!</h1>")
}
