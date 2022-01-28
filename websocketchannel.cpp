#include "websocketchannel.h"
#include <QWebSocketServer>
#include <QWebSocket>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>

WebSocketChannel::WebSocketChannel(QObject *pParent)
	: QWebChannelAbstractTransport(pParent),
	  m_port(-1)
{
}

WebSocketChannel::~WebSocketChannel()
{
}

bool WebSocketChannel::start()
{
	QWebSocketServer *pSrv = new QWebSocketServer("WSController", QWebSocketServer::NonSecureMode, this);
	if (!pSrv->listen(QHostAddress::LocalHost))
	{
		qDebug() << "Unable to create websocket server";
		return -1;
	}

	QObject::connect(pSrv, &QWebSocketServer::newConnection, this, &WebSocketChannel::onNewConnection);

	m_port = (int)pSrv->serverPort();
	m_id = QUuid::createUuid().toString();

	return true;
}

void WebSocketChannel::sendMessage(const QJsonObject &message)
{
	QJsonDocument doc(message);
	QString str(doc.toJson(QJsonDocument::Compact));

	//qDebug() << "WebSocketChannel::sendMessage: " << str;

	// Send to all connections
	for (auto &it: m_connections)
	{
		//qDebug() << "Inspecting " << (void*)it.first;
		if (it.second) // Only send message after successful identification
		{
			//qDebug() << "Sending message!";
			it.first->sendTextMessage(str);
		}
	}
}

void WebSocketChannel::onNewConnection()
{
	QWebSocketServer *pSrv = qobject_cast<QWebSocketServer*>(sender());
	if (!pSrv)
		return;

	QWebSocket *pSocket = pSrv->nextPendingConnection();
	if (!pSocket)
		return;

	qDebug() << "New connection on websocket server";
	
	m_connections[pSocket] = false; // Not identified yet
	QObject::connect(pSocket, &QWebSocket::disconnected, this, &WebSocketChannel::onDisconnected);
	QObject::connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketChannel::onWebSocketMessage);
}

void WebSocketChannel::onDisconnected()
{
	QWebSocket *pSock = qobject_cast<QWebSocket *>(sender());
	if (!pSock)
		return;

	auto it = m_connections.find(pSock);
	if (it == m_connections.end())
		qDebug() << "Connection closing, but not found in map";
	else
	{
		qDebug() << "Closing connection, found in map";
		m_connections.erase(it);
	}

	pSock->deleteLater();
}

void WebSocketChannel::onWebSocketMessage(const QString &message)
{
	QWebSocket *pSock = qobject_cast<QWebSocket *>(sender());
	if (!pSock)
		return;

	auto it = m_connections.find(pSock);
	if (it == m_connections.end())
	{
		qDebug() << "Message received from unregistered websocket? Closing connection";
		pSock->close();
		return;
	}

	if (!it->second) // Need verification message
	{
		if (message != m_id)
		{
			qDebug() << "Received incorrect verification message, closing connection";
			pSock->close();
			return;
		}
		// Ok, correct verification message received
		it->second = true;
		emit newVerifiedConnection(pSock);
		return;
	}

	// Parse as json and emit signal
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
	if(!doc.isNull())
	{
		if (doc.isObject())
			emit messageReceived(doc.object(), this);
		else
			qDebug() << "WebSocketChannel::onWebSocketMessage: Not an object: " << message;
	}
	else
		qDebug() << "WebSocketChannel::onWebSocketMessage: Not JSON: " << message;
}
