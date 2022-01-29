
let communicator = null;

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

const pcConfig = {"iceServers": [{"url": "stun:stun.l.google.com:19302"}]};
let localStream = null;

// Just a very basic implementation for now
function createVideoElement(uuid)
{
    let vid = document.createElement("video");
    vid.id = uuid;
    document.body.appendChild(vid);
}

function removeStream(uuid)
{
    // Cleanup peerConnections entry
    if (uuid in peerConnections)
    {
        let pc = peerConnections[uuid];
        delete peerConnections[uuid];

        pc.onicecandidate = null;
        pc.ontrack = null;
    }

    // Remove UI part
    let vid = document.getElementById(uuid);
    document.body.removeChild(vid);
}

let peerConnections = { };

function newPeerConnectionCommon(uuid)
{
    if (!localStream || localStream.getVideoTracks().length == 0)
        throw "No local video stream available";

    let pc = new RTCPeerConnection(pcConfig);
    peerConnections[uuid] = pc;

    let vid = createVideoElement(uuid);
    
    let videoTrack = localStream.getVideoTracks()[0];
    pc.addTrack(videoTrack); // This is our own video
    
    // This should be the remote video
    pc.ontrack = (evt) => {
        let incomingStream = new MediaStream();
        incomingStream.addTrack(evt.track);

        vid.srcObject = incomingStream;
        vid.play();
    }

    pc.onicecandidate = (evt) => { 
        communicator.onIceCandidate(uuid, JSON.stringify(evt.candidate));
    }

    pc.oniceconnectionstatechange = () => {
        if (pc.iceConnectionState == "connected")
            communicator.onConnected(uuid);
    }

    return pc;
}

async function startFromOfferAsync(uuid, offerStr)
{
    let offer = RTCSessionDescription(JSON.parse(offerStr));
    let pc = newPeerConnectionCommon(uuid);

    await pc.setRemoteDescription(offer);
    let answer = pc.createAnswer();
    await pc.setLocalDescription(answer);

    return JSON.stringify(answer);
}

async function startGenerateOfferAsync(uuid)
{
    let pc = newPeerConnectionCommon(uuid);
        
    let offer = await pc.createOffer();
    await pc.setLocalDescription();

    return JSON.stringify(offer);
}

function startGenerateOffer(uuid)
{
    startGenerateOfferAsync(uuid)
    .then((offer) => communicator.onGeneratedOffer(uuid, offer))
    .catch((err) => {
        removeStream(uuid);
        communicator.onStreamError(uuid, "" + err);
        return;
    })
}

function startFromOffer(uuid, offerStr)
{
    startFromOfferAsync(uuid, offerStr)
    .then((offer) => { }) // TODO: do we need to do something here?
    .catch((err) => {
        removeStream(uuid);
        communicator.onStreamError(uuid, "" + err);
        return;
    })
}

function addIceCandidate(uuid, candidateStr)
{
    if (!(uuid in peerConnections))
    {
        console.warn("addIceCandidate: uuid " + uuid + " not found in peerconnection table");
        return;
    }

    let pc = peerConnections[uuid];
    pc.addIceCandidate(JSON.parse(candidateStr));
}

async function main(comm)
{
    communicator = comm;
    communicator.signalStartGenerateOffer.connect(startGenerateOffer);
    communicator.signalStartFromOffer.connect(startFromOffer);
    communicator.signalAddIceCandidate.connect(addIceCandidate);
    communicator.signalRemoveStream.connect(removeStream);
    
    localStream = await navigator.mediaDevices.getUserMedia({video:true, audio:false});

    let localVideo = document.getElementById("localvideo");
    localVideo.srcObject = localStream;
    localVideo.play();

    // setTimeout(() => startGenerateOffer("testuuid"), 1000);
}