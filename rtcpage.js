
let communicator = null;
const localStreamName = "LOCALSTREAM";

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
function createVideoElement(uuid, displayName)
{
    let div = document.createElement("div");
    div.id = uuid;

    let nameDiv = document.createElement("div");
    nameDiv.innerText = displayName;
    
    let vid = document.createElement("video");
    
    div.appendChild(nameDiv);
    div.appendChild(vid);

    document.body.appendChild(div);

    return vid;
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

function newPeerConnectionCommon(uuid, displayName)
{
    if (!localStream || localStream.getVideoTracks().length == 0)
        throw "No local video stream available";

    let pc = new RTCPeerConnection(pcConfig);
    peerConnections[uuid] = pc;

    let vid = createVideoElement(uuid, displayName);
    
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

async function startFromOfferAsync(uuid, offerStr, displayName)
{
    let pc = newPeerConnectionCommon(uuid, displayName);

    let offer = RTCSessionDescription(JSON.parse(offerStr));
    await pc.setRemoteDescription(offer);

    let answer = pc.createAnswer();
    await pc.setLocalDescription(answer);

    return JSON.stringify(answer);
}

async function startGenerateOfferAsync(uuid, displayName)
{
    let pc = newPeerConnectionCommon(uuid, displayName);
        
    let offer = await pc.createOffer();
    await pc.setLocalDescription();

    return JSON.stringify(offer);
}

function startGenerateOffer(uuid, displayName)
{
    startGenerateOfferAsync(uuid, displayName)
    .then((offer) => communicator.onGeneratedOffer(uuid, offer))
    .catch((err) => {
        removeStream(uuid);
        communicator.onStreamError(uuid, "" + err);
        return;
    })
}

function startFromOffer(uuid, offerStr, displayName)
{
    startFromOfferAsync(uuid, offerStr, displayName)
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

async function startLocalStreamAsync(displayName)
{
    if (localStream)
        throw "Local stream already exists";

    localStream = await navigator.mediaDevices.getUserMedia({video:true, audio:false});

    let vid = createVideoElement(localStreamName, displayName);
    vid.srcObject = localStream;
    vid.play();

    setTimeout(() => startGenerateOffer("testuuid", "Bob"), 1000);
}

function startLocalStream(displayName)
{
    console.log("HERE");
    startLocalStreamAsync(displayName)
    .then(() => {}) // TODO: some feedback?
    .catch((err) => {
        removeStream(localStreamName);
        communicator.onStreamError(localStreamName, "" + err);
    })
}

async function main(comm)
{
    communicator = comm;
    
    communicator.signalStartLocalStream.connect(startLocalStream);
    communicator.signalStartGenerateOffer.connect(startGenerateOffer);
    communicator.signalStartFromOffer.connect(startFromOffer);
    communicator.signalAddIceCandidate.connect(addIceCandidate);
    communicator.signalRemoveStream.connect(removeStream);

    communicator.onMainProgramStarted();
}