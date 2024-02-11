import express from 'express'
const app = express()
const port = 3000

app.post('/', (req, res) => {
    const request = {
        method: 'POST',
        host: req.hostname,
        path: req.path,
        headers: req.headers,
        body: req.body,
        protocol: req.protocol,
    };
    console.log(`got\n${request}\nsize=${JSON.stringify(request).length}`);
    res.send('Hello World!')
})

app.listen(port, () => {
    console.log(`Example app listening on port ${port}`)
})