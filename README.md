
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
