const server = Bun.serve({
    fetch(req: Request) {
        const request = {
            method: 'POST',
            host: req.url,
            headers: req.headers,
            body: req.body,
        };
        console.log(`got\n${JSON.stringify(request)}\nsize=${JSON.stringify(request).length}`);
        return new Response('Hello World!');
    },
    port: 3000,
});

console.log(`Listening on ${server.url}`);
