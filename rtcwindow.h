#pragma once

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebChannel>
#include <QWebSocket>

class WebSocketChannel;

class RtcCommunicator : public QObject
{
	Q_OBJECT
public:
	RtcCommunicator(QObject *pParent);
	~RtcCommunicator();
signals:
	void signalStartGenerateOffer(const QString &streamUuid);
	void signalStartFromOffer(const QString &streamUuid, const QString &offer);
	void signalAddIceCandidate(const QString &streamUuid, const QString &candidate);
	void signalRemoveStream(const QString &streamUuid);
public slots:
	void onGeneratedOffer(const QString &streamUuid, const QString &offer);
	void onGeneratedAnswer(const QString &streamUuid, const QString &answer);
	void onIceCandidate(const QString &streamUuid, const QString &candidate);
	void onStreamError(const QString &streamUuid, const QString &err);
	void onConnected(const QString &streamUuid);
};

class RtcPage : public QWebEnginePage
{
	Q_OBJECT
public:
	RtcPage(QObject *pParent, const QString &origin);
	~RtcPage();
private:
	void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) override;

	const QString m_origin;	
};

class RtcWindow : public QWebEngineView
{
	Q_OBJECT
public:
	RtcWindow(QWidget *pParent = nullptr);
	~RtcWindow();
private slots:
	void handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);
	void onNewVerifiedConnection(QWebSocket *pSocket);
private:
	void setNewPage();

	RtcCommunicator *m_pComm;

	WebSocketChannel *m_pWSChannel;
	QWebChannel *m_pWebChannel;
	QString m_origin;
};
