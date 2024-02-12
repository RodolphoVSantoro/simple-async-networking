use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{TcpListener, TcpStream},
};

async fn run() -> std::io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:3000").await?;
    println!("Listening for connections on port {}", 3000);

    while let Ok((stream, _)) = listener.accept().await {
        tokio::spawn(async move {
            let conn = handle_connection_async(stream).await;
            if let Err(e) = conn {
                println!("An error occurred while handling connection: {}", e);
            }
        });
    }

    Ok(())
}
async fn handle_connection_async(mut stream: TcpStream) -> std::io::Result<()> {
    loop {
        let buffer = &mut [0; 512];
        stream.read(buffer).await?;
        println!("Request: {}", String::from_utf8_lossy(&buffer[..]));

        let response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
        stream.write(response.as_bytes()).await?;
    }
}

#[tokio::main(flavor = "current_thread")]
async fn main(){
    run().await.expect("An error occurred while running the server");
}