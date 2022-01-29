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
public slots:
	void onTestMessage(const QString &s);
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
