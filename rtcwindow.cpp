#include "rtcwindow.h"
#include "websocketchannel.h"
#include <QFile>
#include <QDir>
#include <QWebSocketServer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QPushButton>
#include <QUuid>
#include <iostream>

using namespace std;

RtcWindow::RtcWindow(QWidget *pParent)
	: QWebEngineView(pParent)
{
	m_pWSChannel = new WebSocketChannel(this);
	if (!m_pWSChannel->start())
		qWarning() << "ERROR: Couldn't start websocket channel";

	m_pWebChannel = new QWebChannel(this);

	QObject::connect(m_pWSChannel, &WebSocketChannel::newVerifiedConnection, this, &RtcWindow::onNewVerifiedConnection);

	// For testing, need to register some kind of controller object here
	QWidget *pWidget = new QPushButton(nullptr);
	m_pWebChannel->registerObject("genericWidget", pWidget);

	m_origin = "https://qrtc." + QUuid::createUuid().toString(QUuid::WithoutBraces)+".org/";
	qDebug() << "Using origin " << m_origin;

	setNewPage();
}

RtcWindow::~RtcWindow()
{
}

void RtcWindow::setNewPage()
{
	QWebEnginePage *pPage = new QWebEnginePage(this);
	setPage(pPage); // should delete a previous one

	QObject::connect(pPage, &QWebEnginePage::featurePermissionRequested, this, &RtcWindow::handleFeaturePermissionRequested);

	//QFile f(":/rtcpage.html"); // TODO: when finalizing the page should be embedded in the resource file
	QFile f("rtcpage.html");
	f.open(QIODevice::ReadOnly);
	QString code = f.readAll();
	code = code.replace("WSCONTROLLERPORT", QString::number(m_pWSChannel->getPort()));
	code = code.replace("HANDSHAKEID", "\"" + m_pWSChannel->getHandshakeIdentifier() + "\"");

	// Show the qwebchannel code, but can be loaded using qrc:// prefix in script tag
	//QFile qwebchannel(":/qtwebchannel/qwebchannel.js");
	//qwebchannel.open(QIODevice::ReadOnly);
	//cout << qwebchannel.readAll().toStdString() << endl;

	// The second argument is needed, otherwise: TypeError: Cannot read property 'getUserMedia' of undefined
	// It should be either http://localhost, or another valid https:// name
	// We're using something based on https:// so that we can check the feature permission
	pPage->setHtml(code, m_origin);
}

void RtcWindow::handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature)
{
	if (securityOrigin != m_origin)
	{
		qWarning() << "NOT accepting feature permission for " << (int)feature << " from " << securityOrigin.toString();
		page()->setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionDeniedByUser);
	}
	else
	{
		qDebug() << "ACCEPTING feature permission for " << (int)feature << " from " << securityOrigin.toString();
		page()->setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionGrantedByUser);
	}
}

void RtcWindow::onNewVerifiedConnection(QWebSocket *pSocket)
{
	qDebug() << "Got new verified connection!";
	// Do a reconnect so that this connection has needed info
	m_pWebChannel->disconnectFrom(m_pWSChannel);
	m_pWebChannel->connectTo(m_pWSChannel);
}