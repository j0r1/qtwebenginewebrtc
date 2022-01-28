#pragma once

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebChannel>
#include <QWebSocket>

class WebSocketChannel;

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

	WebSocketChannel *m_pWSChannel;
	QWebChannel *m_pWebChannel;
	QString m_origin;
};
