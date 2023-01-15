const express = require("express");
const app = express();

const port = 8282;

app.post('/test', (req, res) => {
    console.log("Got req: ", req);

    res.send("Hello from Node.js! Good Luck!");
});

app.listen(port, () => {
    console.log("Listening on port: ", port);
});
