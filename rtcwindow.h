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
	void signalStartLocalStream(const QString &displayName);
	void signalStartGenerateOffer(const QString &streamUuid, const QString &displayName);
	void signalStartFromOffer(const QString &streamUuid, const QString &offer, const QString &displayName);
	void signalAddIceCandidate(const QString &streamUuid, const QString &candidate);
	void signalRemoveStream(const QString &streamUuid);

	void jsSignalMainProgramStarted();
public slots:
	void onMainProgramStarted();
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
	RtcWindow(const QString &localName, QWidget *pParent = nullptr);
	~RtcWindow();
private slots:
	void handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);
	void onNewVerifiedConnection(QWebSocket *pSocket);
	void onMainProgramStarted();
private:
	void setNewPage();

	RtcCommunicator *m_pComm;

	WebSocketChannel *m_pWSChannel;
	QWebChannel *m_pWebChannel;
	QString m_origin;
	QString m_localName;
};
