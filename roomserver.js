const http = require("http");
const websocket = require("websocket"); // npm install websocket
const { v4: uuidv4 } = require('uuid');

const server = http.createServer((req, res) => {
    res.end("Nothing to see here");
});

const wsServer = new websocket.server({
    httpServer: server,
    autoAcceptConnections: false
});

const provisionalConnections = [ ];
const rooms = { };

function removeConnectionFrom(c, l)
{
    let idx = l.indexOf(c);
    if (idx < 0)
        throw `Specified connection ${c.getAddress()} not found in list`;

    l.splice(idx, 1);
}

const allowedCharactersInRoomId = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

function validateRoomId(cmd)
{
    if (!("roomid" in cmd))
        throw "Expecting a roomid message";

    const roomId = cmd["roomid"];
    if (roomId.length !== 6)
        throw "Expecting a string of length 6";
    for (let c of roomId)
    {
        if (allowedCharactersInRoomId.indexOf(c) < 0)
            throw `Character '${c}' is not allowed in room ID`;
    }
    return roomId;
}

class Connection
{
    constructor(conn)
    {
        this.address = conn.remoteAddress;
        this.connection = conn;
        this.roomId = null;
        this.uuid = uuidv4();

        this.send({"uuid": this.uuid}); // Make sure you know your own uuid

        conn.on("message", (msg) => this.onMessage(msg));
        conn.on("close", () => this.onClose());

        console.log(`Connection from ${this.address}, uuid is ${this.uuid}`);
    }

    getUuid() { return this.uuid; }
    getAddress() { return this.address; }

    onClose()
    {
        try 
        {
            console.log(`Removing connection from ${this.address}, uuid ${this.uuid}`);

            if (this.roomId === null)
                removeConnectionFrom(this, provisionalConnections);
            else
            {
                removeConnectionFrom(this, rooms[this.roomId]);
                for (let c of rooms[this.roomId])
                    c.send({"userleft": this.uuid});
            }
        }
        catch(err)
        {
            console.log("Error removing connection: " + err);
        }
    }

    send(dict)
    {
        let s = JSON.stringify(dict);
        this.connection.send(s);
    }

    onMessageParsed(cmd)
    {
        if (this.roomId === null)
        {
            this.roomId = validateRoomId(cmd);
            removeConnectionFrom(this, provisionalConnections);
            if (!(this.roomId in rooms))
                rooms[this.roomId] = [ this ];
            else
                rooms[this.roomId].push(this);
            
            // Announce participant to everyone, including ourselves (this is indication that we're in the room)
            for (let c of rooms[this.roomId])
                c.send({"userjoined": this.uuid});
        }
        else
        {
            // Forward message to destination
            let dst = cmd["destination"];

            let destConn = null;
            for (let c of rooms[this.roomId])
            {
                if (c.getUuid() === dst)
                {
                    destConn = c;
                    break;
                }
            }

            if (!destConn)
                console.warn(`Destination ${dst} not found in room ${this.roomId}`);
            else
                destConn.send(cmd);
        }
    }

    onMessage(msg)
    {
        if (msg.type === 'binary')
        {
            console.error("Can't handle binary data, closing connection");
            this.connection.close(); // will trigger onClose
            return;
        }

        try
        {
            let cmd = JSON.parse(msg.utf8Data);
            this.onMessageParsed(cmd);
        }
        catch(err)
        {
            console.log("Error:");
            console.log(err);
            console.log("Error processing message " + msg.utf8Data + ", closing connection");
            this.connection.close(); // will trigger onClose
        }
    }
};

wsServer.on("request", (request) => {

    let conn = request.accept(null, request.origin);
    provisionalConnections.push(new Connection(conn));
})

function main()
{
    console.log("Server started on port " + serverPort);
}

let serverPort = parseInt(process.argv[2]);
server.listen(serverPort, main);

