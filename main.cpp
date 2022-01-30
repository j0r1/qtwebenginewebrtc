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

    QObject::connect(pWin, &RtcWindow::localStreamStarted, [](){
        qDebug() << "Local stream started!";
    });

    QObject::connect(pWin, &RtcWindow::localStreamError, [](const QString &err){
        qDebug() << "Error starting local stream!" << err;
    });

	mainWin.setCentralWidget(pWin);
	mainWin.show();

    return app.exec();
}
