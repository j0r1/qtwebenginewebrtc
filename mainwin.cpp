#include "mainwin.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>

MainWin::MainWin(const QString &serverUrl, const QString &displayName, const QString &roomName)
	: m_wsUrl(serverUrl), m_displayName(displayName), m_roomName(roomName), m_pSock(nullptr)
{
	setupUi(this);

	m_pRtcWin = new RtcWindow(displayName, this);
	QObject::connect(actionNext_webcam, SIGNAL(triggered()), m_pRtcWin, SLOT(toggleNextWebcam()));
	QObject::connect(actionNext_layout, SIGNAL(triggered()), m_pRtcWin, SLOT(toggleNextLayout()));

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
			QMessageBox::critical(this, "Error", "Error in connection with room server");
	});

	QObject::connect(m_pSock, &QWebSocket::textMessageReceived, this, &MainWin::onTextMessage);

	m_pSock->open(m_wsUrl);
}

void MainWin::onWebSocketConnected()
{
	// qDebug() << "Websocket connected, sending room to join";
	sendJson({ { "roomid", m_roomName}, { "displayname", m_displayName } });
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
		onUserJoined(obj["userjoined"].toString(), obj["displayname"].toString());
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
	QString senderUuid = obj["source"].toString();
	QString displayName = obj["displayname"].toString(); // Probably not the best idea to send the displayname with each message though
	if (obj.contains("offer"))
	{
		qDebug() << "Received offer";
		QString offer = obj["offer"].toString();
		QString streamUuid = m_pRtcWin->startFromOffer(offer, displayName);

		m_userToStream[senderUuid] = streamUuid;
		m_streamToUser[streamUuid] = senderUuid;

		auto it = m_userIceBuffer.find(senderUuid);
		if (it != m_userIceBuffer.end())
		{
			qDebug() << "Got buffered ice candidates, applying them";
			for (auto &cand: it->second)
				m_pRtcWin->addIceCandidate(streamUuid, cand);
			m_userIceBuffer.erase(it);
		}
	}
	else if (obj.contains("answer"))
	{
		qDebug() << "Received answer";
		QString streamUuid = m_userToStream[senderUuid];
		m_pRtcWin->processAnswer(streamUuid, obj["answer"].toString());
	}
	else if (obj.contains("ice"))
	{
		qDebug() << "Received ice candidate";
		if (m_userToStream.find(senderUuid) != m_userToStream.end())
		{
			QString streamUuid = m_userToStream[senderUuid];
			m_pRtcWin->addIceCandidate(streamUuid, obj["ice"].toString());
		}
		else
		{
			qDebug() << "Don't have user/stream mapping yet, buffering candidate";
			m_userIceBuffer[senderUuid].push_back(obj["ice"].toString());
		}
	}
	else
	{
		qDebug() << "Can't handle personal message:";
		qDebug() << obj;
	}
}

void MainWin::onUserJoined(const QString &uuid, const QString &displayName)
{
	qDebug() << "User joined:" << uuid;
	if (m_ownUserUuid == uuid)
		qDebug() << "This is us! We're in room" << m_roomName;
	else
	{
		QString streamUuid = m_pRtcWin->startGenerateOffer(displayName);
		m_userToStream[uuid] = streamUuid;
		m_streamToUser[streamUuid] = uuid;

		qDebug() << "Generating offer for" << uuid << displayName;
	}
}

void MainWin::onUserLeft(const QString &uuid)
{
	qDebug() << "User left:" << uuid;
	QString streamUuid = m_userToStream[uuid];
	m_pRtcWin->removeStream(streamUuid);

	auto it = m_streamToUser[streamUuid];
	m_streamToUser.erase(it);

	it = m_userToStream[uuid];
	m_userToStream.erase(it);
}

void MainWin::onGeneratedOffer(const QString &streamUuid, const QString &offer)
{
	// Got offer, need to send this to the new user
	QString userUuid = m_streamToUser[streamUuid];
	sendJson({ { "destination", userUuid }, { "offer", offer } });	
}

void MainWin::sendJson(const QJsonObject &obj)
{
	QJsonDocument doc(obj);
	m_pSock->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void MainWin::onGeneratedAnswer(const QString &streamUuid, const QString &answer)
{
	QString userUuid = m_streamToUser[streamUuid];
	sendJson({{"destination",  userUuid}, { "answer", answer }});
}

void MainWin::onNewIceCandidate(const QString &streamUuid, const QString &candidate)
{
	QString userUuid = m_streamToUser[streamUuid];
	sendJson({{"destination", userUuid}, {"ice", candidate}});
}
