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

RtcCommunicator::RtcCommunicator(QObject *pParent) : QObject(pParent)
{
}

RtcCommunicator::~RtcCommunicator()
{
}

void RtcCommunicator::onMainProgramStarted()
{
	qDebug() << "Main program started";
	emit jsSignalMainProgramStarted();
}

void RtcCommunicator::onLocalStreamStarted()
{
	qDebug() << "Local stream started";
	emit jsSignalLocalStreamStarted();
}

void RtcCommunicator::onLocalStreamError(const QString &err)
{
	qDebug() << "Local stream error:" << err;
	emit jsSignalLocalStreamError(err);
}

void RtcCommunicator::onGeneratedOffer(const QString &streamUuid, const QString &offer)
{
	qDebug() << "Offer for" << streamUuid;
	qDebug() << offer;
	emit jsSignalGeneratedOffer(streamUuid, offer);
}

void RtcCommunicator::onGeneratedAnswer(const QString &streamUuid, const QString &answer)
{
	qDebug() << "Answer for " << streamUuid;
	qDebug() << answer;
	emit jsSignalGeneratedAnswer(streamUuid, answer);
}

void RtcCommunicator::onIceCandidate(const QString &streamUuid, const QString &candidate)
{
	qDebug() << "Ice candidate for" << streamUuid;
	qDebug() << candidate;
	emit jsSignalIceCandidate(streamUuid, candidate);
}

void RtcCommunicator::onStreamError(const QString &streamUuid, const QString &err)
{
	qDebug() << "Stream error for" << streamUuid;
	qDebug() << err;
	emit jsSignalStreamError(streamUuid, err);
}

void RtcCommunicator::onConnected(const QString &streamUuid)
{
	qDebug() << "CONNECTED:" << streamUuid;
	emit jsSignalStreamConnected(streamUuid);
}

/////////////////////////////////////

RtcPage::RtcPage(QObject *pParent, const QString &origin) 
	: QWebEnginePage(pParent), m_origin(origin)
{
}

RtcPage::~RtcPage()
{
}

void RtcPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID)
{
	QString type = "[UNKNOWN]";
	if (level == QWebEnginePage::InfoMessageLevel)
		type = "[INFO]";
	else if (level == QWebEnginePage::WarningMessageLevel)
		type = "[WARNING]";
	else if (level == QWebEnginePage::ErrorMessageLevel)
		type = "[ERROR]";

	QString out = type + " (";
	if (sourceID != m_origin)
		out += sourceID + ":";
	out += QString::number(lineNumber) + ") " + message;

	qDebug() << out.toUtf8().data();
}

/////////////////////////////////////

RtcWindow::RtcWindow(const QString &localName, QWidget *pParent)
	: QWebEngineView(pParent), m_localName(localName)
{
	m_pWSChannel = new WebSocketChannel(this);
	QObject::connect(m_pWSChannel, &WebSocketChannel::newVerifiedConnection, this, &RtcWindow::onNewVerifiedConnection);

	if (!m_pWSChannel->start())
		qWarning() << "ERROR: Couldn't start websocket channel";

	m_pComm = new RtcCommunicator(this);
	QObject::connect(m_pComm, &RtcCommunicator::jsSignalMainProgramStarted, this, &RtcWindow::onMainProgramStarted);
	QObject::connect(m_pComm, &RtcCommunicator::jsSignalLocalStreamStarted, this, &RtcWindow::onLocalStreamStarted);
	QObject::connect(m_pComm, &RtcCommunicator::jsSignalLocalStreamError, this, &RtcWindow::onLocalStreamError);
	QObject::connect(m_pComm, &RtcCommunicator::jsSignalGeneratedOffer, this, &RtcWindow::onGeneratedOffer);
	QObject::connect(m_pComm, &RtcCommunicator::jsSignalGeneratedAnswer, this, &RtcWindow::onGeneratedAnswer);
	QObject::connect(m_pComm, &RtcCommunicator::jsSignalIceCandidate, this, &RtcWindow::onIceCandidate);
	QObject::connect(m_pComm, &RtcCommunicator::jsSignalStreamError, this, &RtcWindow::onStreamError);
	QObject::connect(m_pComm, &RtcCommunicator::jsSignalStreamConnected, this, &RtcWindow::onStreamConnected);

	m_pWebChannel = new QWebChannel(this);
	m_pWebChannel->registerObject("communicator", m_pComm);

	m_origin = "https://qrtc." + QUuid::createUuid().toString(QUuid::WithoutBraces)+".org/";
	qDebug() << "Using origin " << m_origin;

	setNewPage();
}

RtcWindow::~RtcWindow()
{
}

//#define WEBPAGEINQRC

void RtcWindow::setNewPage()
{
	QWebEnginePage *pPage = new RtcPage(this, m_origin);
	setPage(pPage); // should delete a previous one

	QObject::connect(pPage, &QWebEnginePage::featurePermissionRequested, this, &RtcWindow::handleFeaturePermissionRequested);

#ifdef WEBPAGEINQRC
	QFile f(":/rtcpage.html"); // TODO: when finalizing the page should be embedded in the resource file
#else
	QFile f("rtcpage.html");
#endif
	f.open(QIODevice::ReadOnly);
	QString code = f.readAll();
	code = code.replace("WSCONTROLLERPORT", QString::number(m_pWSChannel->getPort()));
	code = code.replace("HANDSHAKEID", "\"" + m_pWSChannel->getHandshakeIdentifier() + "\"");
	
#ifndef WEBPAGEINQRC
	// For debugging, just include the rtcpage.js code in the html code
	QFile js("rtcpage.js");
	js.open(QIODevice::ReadOnly);
	QString jsCode = js.readAll();
	code = code.replace("<script type=\"text/javascript\" src=\"qrc:///rtcpage.js\"></script>",
						"<script>\n" + jsCode + "\n</script>\n");
#endif
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

void RtcWindow::onMainProgramStarted()
{
	// NOTE: In case of a reload of the page, this will be called again!
	emit m_pComm->signalStartLocalStream(m_localName);
}

QString RtcWindow::startGenerateOffer(const QString &displayName)
{
	QString uuid = QUuid::createUuid().toString();
	emit m_pComm->signalStartGenerateOffer(uuid, displayName);
	return uuid;
}

QString RtcWindow::startFromOffer(const QString &offer, const QString &displayName)
{
	QString uuid = QUuid::createUuid().toString();
	emit m_pComm->signalStartFromOffer(uuid, offer, displayName);
	return uuid;
}

void RtcWindow::addIceCandidate(const QString &streamUuid, const QString &candidate)
{
	emit m_pComm->signalAddIceCandidate(streamUuid, candidate);
}

void RtcWindow::removeStream(const QString &streamUuid)
{
	emit m_pComm->signalRemoveStream(streamUuid);
}

void RtcWindow::onLocalStreamStarted()
{
	emit localStreamStarted();
}

void RtcWindow::onLocalStreamError(const QString &err)
{
	emit localStreamError(err);
}

void RtcWindow::onGeneratedOffer(const QString &streamUuid, const QString &offer)
{
	emit generatedOffer(streamUuid, offer);
}

void RtcWindow::onGeneratedAnswer(const QString &streamUuid, const QString &answer)
{
	emit generatedAnswer(streamUuid, answer);
}

void RtcWindow::onIceCandidate(const QString &streamUuid, const QString &candidate)
{
	emit newIceCandidate(streamUuid, candidate);
}

void RtcWindow::onStreamError(const QString &streamUuid, const QString &err)
{
	emit streamError(streamUuid, err);
}

void RtcWindow::onStreamConnected(const QString &streamUuid)
{
	emit streamConnected(streamUuid);
}
