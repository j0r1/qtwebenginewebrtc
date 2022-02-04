#include "mainwin.h"

MainWin::MainWin(const QString &serverUrl, const QString &displayName, const QString &roomName)
	: m_wsUrl(serverUrl), m_displayName(displayName), m_pSock(nullptr)
{
	m_pRtcWin = new RtcWindow(displayName, this);
	QObject::connect(m_pRtcWin, &RtcWindow::localStreamStarted, this, &MainWin::onLocalVideoStarted);
	QObject::connect(m_pRtcWin, &RtcWindow::localStreamError, [](const QString &err)
	{
		qDebug() << "Error starting local video stream";
	});

	QObject::connect(m_pRtcWin, &RtcWindow::generatedOffer, this, &MainWin::onGeneratedOffer);
	QObject::connect(m_pRtcWin, &RtcWindow::generatedAnswer, this, &MainWin::onGeneratedAnswer);
	QObject::connect(m_pRtcWin, &RtcWindow::newIceCandidate, this, &MainWin::onNewIceCandidate);
	QObject::connect(m_pRtcWin, &RtcWindow::streamError, [](const QString &streamUuid, const QString &err){
		qDebug() << "Error in stream" << streamUuid << err;
	});
	QObject::connect(m_pRtcWin, &RtcWindow::streamConnected, [](const QString &streamUuid) {
		qDebug() << "Stream connected: " << streamUuid;
	});

	setCentralWidget(m_pRtcWin);
	resize(800, 450);
	show();
}

MainWin::~MainWin()
{
}

void MainWin::onLocalVideoStarted()
{
	m_pSock = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
	QObject::connect(m_pSock, &QWebSocket::connected, this, &MainWin::onWebSocketConnected);
	QObject::connect(m_pSock, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
    	[=](QAbstractSocket::SocketError error) {
			qDebug() << "Error in websocket connection" << error;
	});

	m_pSock->open(m_wsUrl);
}

void MainWin::onWebSocketConnected()
{
	qDebug() << "Websocket connected";
}

void MainWin::onGeneratedOffer(const QString &streamUuid, const QString &offer)
{

}

void MainWin::onGeneratedAnswer(const QString &streamUuid, const QString &answer)
{

}

void MainWin::onNewIceCandidate(const QString &streamUuid, const QString &candidate)
{

}
