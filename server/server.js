const WebSocket = require("ws");

const server = new WebSocket.Server({
    port: 8080
});

console.log("Chessify server started on port 8080");

server.on("connection", (socket) => {

    console.log("Client connected.");

    socket.send("Welcome to Chessify!");

    socket.on("message", (message) => {

        console.log("Received:", message.toString());

        server.clients.forEach(client => {

            if (client.readyState === WebSocket.OPEN)
            {
                client.send(message.toString());
            }

        });

    });

    socket.on("close", () => {

        console.log("Client disconnected.");

    });

});