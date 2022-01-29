function startQWebChannel(wsControllerPort, wsControllerHandshakeID)
{
    return new Promise((resolve, reject) => {

        console.log("WebSocket server port is " + wsControllerPort);
        console.log("Handshake ID is " + wsControllerHandshakeID);

        let ws = new WebSocket("ws://localhost:" + wsControllerPort);
        ws.onopen = () => { 
            console.log("WebSocket opened, sending handshake ID");
            ws.send(wsControllerHandshakeID);

            console.log("Setting up QWebChannel");
            new QWebChannel(ws, (channel) => {
                // console.log("Listing objects:");
                // for (let i in channel.objects)
                //     console.log(i);

                resolve(channel.objects.communicator);
            });
        }
        ws.onclose = () => {
            console.log("WebSocket closed");
            reject("WebSocket closed");
        }
        ws.onerror = () => console.error("WebSocket error");
        // This is no longer called I think, is intercepted by QWebChannel
        ws.onmessage = (msg) => console.log("Received message: " + msg);
    })
}

async function main()
{
    let stream = await navigator.mediaDevices.getUserMedia({video:true, audio:false});
    let localVideo = document.getElementById("localvideo");
    localVideo.srcObject = stream;
    localVideo.play();
}