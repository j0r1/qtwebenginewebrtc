#pragma once

#include <QWebEngineView>
#include <QWebEnginePage>

class RtcWindow : public QWebEngineView
{
	Q_OBJECT
public:
	RtcWindow(QWidget *pParent = nullptr);
	~RtcWindow();

private slots:
	void handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);
};
