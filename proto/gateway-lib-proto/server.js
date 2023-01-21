const express = require("express");
const app = express();

const port = 8282;

const db = {};
const latestPacket = {};

const intFromBytes = (buf, start, len) => {
    let x = 0;
    for (let i=0; i<len; i++) {
        x <<= 8;
        x += buf[start + i];
    }
    return x;
}

// Set parser to parse as text/plain
app.use(express.text());

app.post('/test', (req, res) => {
    console.log("Got req, body: ", req.body);
    
    // Parse message
    const buf = Buffer.from(req.body, 'base64');
    const header = intFromBytes(buf, 0, 1);
    const srcId = intFromBytes(buf, 1, 2);
    const packetId = intFromBytes(buf, 3, 2);
    const hdr_len = 5;
    
    console.log("RECV msg. header: %d, srcId: %d, packetId: %d", header, srcId, packetId);
    
    // If outdated
    if (packetId <= latestPacket[srcId]) {
        const msg = `Outdated packet: ${packetId}, newest: ${latestPacket[srcId]}`;
        res.send(msg);
        console.log(msg);
        return;
    }
    latestPacket[srcId] = packetId;

    switch (header) {
        case 1:
            const temp = intFromBytes(buf, hdr_len + 0, 1);
            const humidity = intFromBytes(buf, hdr_len + 1, 1);

            // Store
            if (!(srcId in db)) db[srcId] = {};
            db[srcId].data = {packetId: packetId, temp: temp, humidity: humidity};
            console.log("Storing db[%d].data = {%d: {temp: %d, humidity: %d}}", srcId, packetId, temp, humidity);
            res.send("ACK from Node.js");
            break;
        case 2:
            let adj = [];                
            for (let i=hdr_len; i<buf.length; i+=2) {
                let peerId = intFromBytes(buf, i, 2);
                if (peerId > 0)
                    adj.push(peerId);
            }
            
            // Store
            if (!(srcId in db)) db[srcId] = {};
            db[srcId].adj = {packetId: packetId, list: adj};
            console.log("Storing db[%d].adj = {packetId: %d, list: %s}", srcId, packetId, adj.toString());
            res.send("ACK from Node.js");
            break;
        default:
            const msg = "Invalid header";
            res.send(msg);
            console.log("\x1b[1mInvalid header\x1b[0m");
    }
});

app.listen(port, () => {
    console.log("Listening on port: ", port);
});


// Log db on interval
const logDB = () => {
    console.log("db: ", db);
    console.log("adjacencies:");
    for (let [key, v] of Object.entries(db)) {
        if (v.adj)
            console.log("%d: %s", key, v.adj.list.toString());
    }
}
setInterval(logDB, 20000);
