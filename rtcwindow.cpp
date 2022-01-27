#include "rtcwindow.h"
#include <QFile>
#include <QDir>
#include <QWebSocketServer>
#include <QJsonObject>
#include <QJsonDocument>
#include <iostream>

using namespace std;

RtcWindow::RtcWindow(QWidget *pParent)
	: QWebEngineView(pParent)
{
	auto p = page();
	QObject::connect(p, &QWebEnginePage::featurePermissionRequested, this, &RtcWindow::handleFeaturePermissionRequested);

	int port = startWSServer();
	cout << "Port is: " << port << endl;

	//QFile f(":/rtcpage.html");
	QFile f("rtcpage.html");
	f.open(QIODevice::ReadOnly);
	QString code = f.readAll();
	code = code.replace("WSCONTROLLERPORT", QString::number(port));

	p->setHtml(code, QUrl("http://localhost")); // The second argument is needed, otherwise: TypeError: Cannot read property 'getUserMedia' of undefined
}

RtcWindow::~RtcWindow()
{
}

void RtcWindow::handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature)
{
	// TODO: this is auto accept, check origin, check feature!
	cout << "accepting feature permission for " << (int)feature << " from " << securityOrigin.toString().toStdString() << endl;
	page()->setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionGrantedByUser);
}

int RtcWindow::startWSServer()
{
	QWebSocketServer *pSrv = new QWebSocketServer("WSController", QWebSocketServer::NonSecureMode, this);
	if (!pSrv->listen(QHostAddress::LocalHost))
	{
		qDebug() << "Unable to create websocket server";
		return -1;
	}

	QObject::connect(pSrv, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
	return (int)pSrv->serverPort();
}

void RtcWindow::sendMessage(const QJsonObject &message)
{
	QJsonDocument doc(message);
	QString str(doc.toJson(QJsonDocument::Compact));
	// Send to all connections
	for (auto pConn: m_connections)
		pConn->sendTextMessage(str);
}

void RtcWindow::onNewConnection()
{
	QWebSocketServer *pSrv = qobject_cast<QWebSocketServer*>(sender());
	if (!pSrv)
		return;

	QWebSocket *pSocket = pSrv->nextPendingConnection();
	if (!pSocket)
		return;

	log("New connection");
	m_connections.insert(pSocket);
	QObject::connect(pSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
	QObject::connect(pSocket, SIGNAL(textMessageReceived(const QString&)), this, SLOT(onWebSocketMessage(const QString&)));
}

void RtcWindow::onDisconnected()
{
	QWebSocket *pSock = qobject_cast<QWebSocket *>(sender());
	if (!pSock)
		return;

	auto it = m_connections.find(pSock);
	if (it == m_connections.end())
		cerr << "Connection closing, but not found" << endl;
	else
	{
		cerr << "Closing connection" << endl;
		m_connections.erase(it);
	}

	pSock->deleteLater();
}

QJsonObject ObjectFromString(const QString& in)
{
    QJsonObject obj;

    // check validity of the document
    if(!doc.isNull())
    {
        if(doc.isObject())
            obj = doc.object();        
        else
            qDebug() << "Document is not an object" << endl;
    }
    else
        qDebug() << "Invalid JSON...\n" << in << endl;

    return obj;
}

void RtcWindow::onWebSocketMessage(const QString &message)
{
	// TODO: do some verification, check that not anyone is trying to send us something
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
	if(!doc.isNull())
	{
		if (doc.isObject())
			emit messageReceived(doc.object(), this);
		else
			cerr << "Not an object: " << message.toStdString() << endl;
	}
	else
		cerr << "Not JSON: " << message.toStdString() << endl;
}