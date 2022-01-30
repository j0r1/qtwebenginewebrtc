
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
    // TODO: what if the local stream/webcam is specified? Ignore? Do something else?
    // It may still be used in the rest of the peerconnections!

    // Cleanup peerConnections entry
    if (uuid in peerConnections)
    {
        let pc = peerConnections[uuid];
        delete peerConnections[uuid];

        pc.onicecandidate = null;
        pc.ontrack = null;
    }

    // Remove UI part
    let div = document.getElementById(uuid);
    if (div)
        document.body.removeChild(div);
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
    
    // This should be the remote video (or audio in case that's enabled)
    pc.ontrack = (evt) => {

        if (!vid.srcObject)
            vid.srcObject =  new MediaStream();

        vid.srcObject.addTrack(evt.track);
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

    let offer = new RTCSessionDescription(JSON.parse(offerStr));
    await pc.setRemoteDescription(offer);

    let answer = await pc.createAnswer();
    await pc.setLocalDescription(answer);

    return JSON.stringify(answer);
}

async function startGenerateOfferAsync(uuid, displayName)
{
    let pc = newPeerConnectionCommon(uuid, displayName);
        
    let offer = await pc.createOffer();
    await pc.setLocalDescription(offer);

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
    .then((answer) => {
        communicator.onGeneratedAnswer(uuid, answer)
    })
    .catch((err) => {
        removeStream(uuid);
        communicator.onStreamError(uuid, "" + err);
        return;
    })
}

async function processAnswerAsync(uuid, answerStr)
{
    if (!(uuid in peerConnections)) // TODO: report this somehow?
    {
        console.warn("processAnswer: uuid " + uuid + " not found");
        return;
    }

    let answer = new RTCSessionDescription(JSON.parse(answerStr));
    let pc = peerConnections[uuid];
    await pc.setRemoteDescription(answer);  
}

function processAnswer(uuid, answerStr)
{
    processAnswerAsync(uuid, answerStr)
    .then((answer) => {
        console.log("Processed answer!"); // TODO: more feedback?    
    })
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
    let candidate = JSON.parse(candidateStr);
    if (candidate)
        pc.addIceCandidate(candidate);
}

async function startLocalStreamAsync(displayName)
{
    if (localStream)
        throw "Local stream already exists";

    localStream = await navigator.mediaDevices.getUserMedia({video:true, audio:false});

    let vid = createVideoElement(localStreamName, displayName);
    vid.srcObject = localStream;
    vid.play();
}

function startLocalStream(displayName)
{
    startLocalStreamAsync(displayName)
    .then(() => communicator.onLocalStreamStarted())
    .catch((err) => {
        removeStream(localStreamName);
        communicator.onLocalStreamError("" + err);
    })
}

async function main(comm)
{
    communicator = comm;
    
    communicator.signalStartLocalStream.connect(startLocalStream);
    communicator.signalStartGenerateOffer.connect(startGenerateOffer);
    communicator.signalProcessAnswer.connect(processAnswer);
    communicator.signalStartFromOffer.connect(startFromOffer);
    communicator.signalAddIceCandidate.connect(addIceCandidate);
    communicator.signalRemoveStream.connect(removeStream);

    communicator.onMainProgramStarted(); // Only do this _after_ the signal handlers were installed!
}