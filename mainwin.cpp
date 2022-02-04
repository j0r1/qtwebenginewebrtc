#include "mainwin.h"
#include <QJsonObject>
#include <QJsonDocument>

MainWin::MainWin(const QString &serverUrl, const QString &displayName, const QString &roomName)
	: m_wsUrl(serverUrl), m_displayName(displayName), m_roomName(roomName), m_pSock(nullptr)
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
	QObject::connect(m_pSock, &QWebSocket::disconnected, this, [](){
		qDebug() << "Websocket disconnected";
	});
	QObject::connect(m_pSock, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
    	[=](QAbstractSocket::SocketError error) {
			qDebug() << "Error in websocket connection" << error;
	});

	QObject::connect(m_pSock, &QWebSocket::textMessageReceived, this, &MainWin::onTextMessage);

	m_pSock->open(m_wsUrl);
}

void MainWin::onWebSocketConnected()
{
	qDebug() << "Websocket connected, sending room to join";
	QJsonObject obj;
	obj["roomid"] = m_roomName;
	
	QJsonDocument doc(obj);
	m_pSock->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void MainWin::onTextMessage(const QString &msg)
{
	// All messages should be JSON
	QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
	if(doc.isNull() || !doc.isObject())
	{
		qDebug() << "Message is not a JSON object";
		m_pSock->close();
		return;
	}

	QJsonObject obj = doc.object();

	if (m_ownUserUuid.length() == 0) // first message should be a uuid message
	{
		if (!obj.contains("uuid"))
		{
			qDebug() << "No uuid found in message";
			m_pSock->close();
		}
		else
		{
			m_ownUserUuid = obj["uuid"].toString();
			qDebug() << "Own uuid is" << m_ownUserUuid;
		}
	}
	else
	{
		processRoomMessage(obj);
	}
}

void MainWin::processRoomMessage(const QJsonObject &obj)
{
	// 'userjoined' 'userleft' 'destination'
	if (obj.contains("destination"))
	{
		if (obj["destination"].toString() != m_ownUserUuid)
			qDebug() << "Received message for different destination";
		else
			onPersonalMessage(obj);
	}
	else if (obj.contains("userjoined"))
	{
		onUserJoined(obj["userjoined"].toString());
	}
	else if (obj.contains("userleft"))
	{
		onUserLeft(obj["userleft"].toString());
	}
	else
	{
		qDebug() << "Unexpected message from server:";
		qDebug() << obj;
	}
}

void MainWin::onPersonalMessage(const QJsonObject &obj)
{
	// TODO
}

void MainWin::onUserJoined(const QString &uuid)
{
	qDebug() << "User joined:" << uuid;
	if (m_ownUserUuid == uuid)
		qDebug() << "This is us! We're in room" << m_roomName;
	else
	{
		// TODO: start by generating offer
	}
}

void MainWin::onUserLeft(const QString &uuid)
{
	qDebug() << "User left:" << uuid;
}

void MainWin::onGeneratedOffer(const QString &streamUuid, const QString &offer)
{
	// TODO
}

void MainWin::onGeneratedAnswer(const QString &streamUuid, const QString &answer)
{
	// TODO
}

void MainWin::onNewIceCandidate(const QString &streamUuid, const QString &candidate)
{
	// TODO
}
