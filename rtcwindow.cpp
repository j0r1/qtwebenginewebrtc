#include "rtcwindow.h"
#include "websocketchannel.h"
#include <QFile>
#include <QDir>
#include <QWebSocketServer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QPushButton>
#include <iostream>

using namespace std;

RtcWindow::RtcWindow(QWidget *pParent)
	: QWebEngineView(pParent)
{
	auto p = page();
	QObject::connect(p, &QWebEnginePage::featurePermissionRequested, this, &RtcWindow::handleFeaturePermissionRequested);

	m_pWSChannel = new WebSocketChannel(this);
	if (!m_pWSChannel->start())
		cerr << "ERROR: Couldn't start websocket channel" << endl;

	m_pWebChannel = new QWebChannel(this);

	QObject::connect(m_pWSChannel, &WebSocketChannel::newVerifiedConnection, this, &RtcWindow::onNewVerifiedConnection);

	QWidget *pWidget = new QPushButton(nullptr);
	m_pWebChannel->registerObject("genericWidget", pWidget);

	//QFile f(":/rtcpage.html");
	QFile f("rtcpage.html");
	f.open(QIODevice::ReadOnly);
	QString code = f.readAll();
	code = code.replace("WSCONTROLLERPORT", QString::number(m_pWSChannel->getPort()));
	code = code.replace("HANDSHAKEID", "\"" + m_pWSChannel->getHandshakeIdentifier() + "\"");

	// Show the qwebchannel code, but can be loaded using qrc:// prefix in script tag
	//QFile qwebchannel(":/qtwebchannel/qwebchannel.js");
	//qwebchannel.open(QIODevice::ReadOnly);
	//cout << qwebchannel.readAll().toStdString() << endl;

	// cout << code.toStdString() << endl;

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

void RtcWindow::onNewVerifiedConnection(QWebSocket *pSocket)
{
	cerr << "Got new verified connection" << endl;
	// Do a reconnect so that this connection has needed info
	m_pWebChannel->disconnectFrom(m_pWSChannel);
	m_pWebChannel->connectTo(m_pWSChannel);
}