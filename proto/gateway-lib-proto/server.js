const express = require("express");
const app = express();

const port = 8282;

const db = {};

// Set parser to parse as text/plain
app.use(express.text());

app.post('/test', (req, res) => {
    console.log("Got req, body: ", req.body);
    
    // Parse message
    const msg = req.body
    const args = msg.split('/').map(str => parseInt(str));
    
    // Invalid format
    if (args.length < 3) {
        const msg = "Insufficient ARGS, dropping.";
        console.log(msg);
        res.send(msg);
        return;
    }
   
    let nodeId = args[0];
    let packetId = args[1];
    let temp = args[2];
    let humidity = args[3];

    // Store
    db[nodeId] = {packetId: packetId, temp: temp, humidity: humidity};
    console.log("Storing db[%d] = {%d: {temp: %d, humidity: %d}}", nodeId, packetId, temp, humidity);
    
    res.send("ACK from Node.js");
});

app.listen(port, () => {
    console.log("Listening on port: ", port);
});


// Log db on interval
const logDB = () => {
    console.log("db: ", db);
}
setInterval(logDB, 3000);
