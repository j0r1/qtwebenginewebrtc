#pragma once

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebChannelAbstractTransport>
#include <set>

class QWebSocket;

class RtcWindow : public QWebEngineView, public QWebChannelAbstractTransport
{
	Q_OBJECT
public:
	RtcWindow(QWidget *pParent = nullptr);
	~RtcWindow();
public slots:
	void sendMessage(const QJsonObject &message) override;
private:
	int startWSServer();
private slots:
	void handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);
	void onNewConnection();
	void onDisconnected();
	void onWebSocketMessage(const QString &s);
private:
	std::set<QWebSocket*> m_connections;
};
