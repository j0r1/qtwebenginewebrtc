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
	void jsSignalLocalStreamStarted();
	void jsSignalLocalStreamError(const QString &err);
	void jsSignalGeneratedOffer(const QString &streamUuid, const QString &offer);
	void jsSignalGeneratedAnswer(const QString &streamUuid, const QString &answer);
	void jsSignalIceCandidate(const QString &streamUuid, const QString &candidate);
	void jsSignalStreamError(const QString &streamUuid, const QString &err);
	void jsSignalStreamConnected(const QString &streamUuid);
public slots:
	void onMainProgramStarted();
	void onLocalStreamStarted();
	void onLocalStreamError(const QString &err);

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

	QString startGenerateOffer(const QString &displayName); // returns uuid
	QString startFromOffer(const QString &offer, const QString &displayName); // returns uuid
	void addIceCandidate(const QString &streamUuid, const QString &candidate);
	void removeStream(const QString &streamUuid);

signals:
	void localStreamStarted();
	void localStreamError(const QString &err);

	void generatedOffer(const QString &streamUuid, const QString &offer);
	void generatedAnswer(const QString &streamUuid, const QString &answer);
	void newIceCandidate(const QString &streamUuid, const QString &candidate);
	void streamError(const QString &streamUuid, const QString &err);
	void streamConnected(const QString &streamUuid);

private slots:
	void handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);
	void onNewVerifiedConnection(QWebSocket *pSocket);
	void onMainProgramStarted();
	void onLocalStreamStarted();
	void onLocalStreamError(const QString &err);

	void onGeneratedOffer(const QString &streamUuid, const QString &offer);
	void onGeneratedAnswer(const QString &streamUuid, const QString &answer);
	void onIceCandidate(const QString &streamUuid, const QString &candidate);
	void onStreamError(const QString &streamUuid, const QString &err);
	void onStreamConnected(const QString &streamUuid);
private:
	void setNewPage();

	RtcCommunicator *m_pComm;

	WebSocketChannel *m_pWSChannel;
	QWebChannel *m_pWebChannel;
	QString m_origin;
	QString m_localName;
};
