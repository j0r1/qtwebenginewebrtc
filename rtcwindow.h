#pragma once

#include <QWebEngineView>
#include <QWebEnginePage>

class WebSocketChannel;

class RtcWindow : public QWebEngineView
{
	Q_OBJECT
public:
	RtcWindow(QWidget *pParent = nullptr);
	~RtcWindow();
private slots:
	void handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);
private:
	WebSocketChannel *m_pWSChannel;
};
