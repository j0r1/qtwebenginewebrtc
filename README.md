
The class to use is `RtcWindow` in [rtcwindow.h](rtcwindow.h). In this
widget a QWebEngineView is used to display a webpage which in turn shows
your own video as well as for a number of participants. So in your
application, you'd use a single RtcWindow instance.

With the RtcWindow, you can control a number of RTCPeerConnection instances
in the web page. Each is identified by what is called a `streamUuid` in the
RtcWindow interface. To start a video communication with someone, one
party calls the `startGenerateOffer` method, which will later trigger a
signal `generatedOffer` with the actual offer. That offer should be sent to
the other party, that calls `startFromOffer`. The answer provided in the
`generatedAnswer` signal should be sent back to the first party, that needs
to process the answer with (the aptly named) `processAnswer` method.

As soon as these `generate...` functions are called, the ICE (Interactive 
Connectivity Establishment) mechanism will start working, trying to figure
out how participants can be reached. For now, a default STUN server is
used in the web page (stun.l.google.com:19302), and no TURN surver is
specified. The candidates signalled by the `newIceCandidate` signal need
to be sent to the other party, that needs to process them with
`addIceCandidate`.

Depending on the first argument, different versions of the main program
can be executed: `testsignalslot` starts the original main program where
two RtcWindow instances are connected using signals and slots. For the
`testroomserver` and `main` versions, you need to start a helper server
that will be used to exchange offer/answer/ICE info.

This is a node.js program, to install the dependencies first run
`npm install`. Then, start the program on a specific port, e.g.

    node roomserver.js 12345

The `testroomserver` version takes a websocket URL and a number of 
instances of RtcWindow to launch, e.g.

    ./qtwebrtctest testroomserver ws://localhost:12345 3

will show three windows that will start exchanging video streams.

The `main` version just launches a single instance, you need
to supply a websocket URL again, a room name and your name. Everyone
in the same room will exchange video information with each other,
and this room name should consists of six capital letters, e.g.

    ./qtwebrtctest main ws://localhost:12345 ROOMXY John


