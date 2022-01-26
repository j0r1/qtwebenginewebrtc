#include "rtcwindow.h"
#include <QFile>
#include <QDir>
#include <iostream>

using namespace std;

RtcWindow::RtcWindow(QWidget *pParent)
	: QWebEngineView(pParent)
{
	auto p = page();
	QObject::connect(p, &QWebEnginePage::featurePermissionRequested, this, &RtcWindow::handleFeaturePermissionRequested);

	QFile f(":/rtcpage.html");
	f.open(QIODevice::ReadOnly);
	p->setHtml(f.readAll(), QUrl("http://localhost")); // The second argument is needed, otherwise: TypeError: Cannot read property 'getUserMedia' of undefined
}

RtcWindow::~RtcWindow()
{
}

void RtcWindow::handleFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature)
{
	// TODO: this is auto accept, check origin, check feature!
	cout << "accepting feature permission for " << (int)feature << " from " << securityOrigin.toString().toStdString() << endl;
	page()->setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionGrantedByUser);
}
