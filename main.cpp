#include "rtcwindow.h"
#include <QApplication>
#include <QWebEngineSettings>
#include <QMainWindow>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

	// Needed?
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

	QMainWindow  mainWin;
    auto *pWin = new RtcWindow("Me");
	mainWin.setCentralWidget(pWin);
	mainWin.show();

    return app.exec();
}
