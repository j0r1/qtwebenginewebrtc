#include "rtcwindow.h"
#include <QApplication>
#include <QWebEngineSettings>
#include <QMainWindow>
#include <memory>
#include <vector>
#include <cassert>

using namespace std;

// Will be executed when all local streams (webcams) have been started
void startStreams(const vector<RtcWindow*> &rtcWindows)
{
    qDebug() << "Starting all streams";
    assert(rtcWindows.size() == 2);

    QString uuid = rtcWindows[0]->startGenerateOffer("Remote user (1)");
    QObject::connect(rtcWindows[0], &RtcWindow::generatedOffer, [&rtcWindows](const QString &streamUuid, const QString &offer) {
        
        rtcWindows[1]->startFromOffer(offer, "Remote user (0)");
    });

    QObject::connect(rtcWindows[1], &RtcWindow::generatedAnswer, [uuid,&rtcWindows](const QString &streamUuid, const QString &answer) {
        // Note that we need to use the first uuid here!
        rtcWindows[0]->processAnswer(uuid, answer);
    });

    // TODO: ice information needs to be exchanged!

}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

	// Needed?
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

	vector<unique_ptr<QMainWindow>> mainWindows;
    vector<RtcWindow*> rtcWindows;

    mainWindows.push_back(make_unique<QMainWindow>());
    mainWindows.push_back(make_unique<QMainWindow>());

    size_t count = 0;
    auto updateLocalStartedCount = [&count,&mainWindows,&rtcWindows]()
    {
        count++;
        qDebug() << count;
        if (count == mainWindows.size())
            startStreams(rtcWindows);
    };

    for (size_t i = 0 ; i < mainWindows.size() ; i++)
    {
        auto &pMainWin = mainWindows[i];
        auto *pWin = new RtcWindow("Local user (" + QString::number(i) + ")", pMainWin.get());
        rtcWindows.push_back(pWin);

        QObject::connect(pWin, &RtcWindow::localStreamStarted, [&updateLocalStartedCount](){
            qDebug() << "Local stream started!";
            updateLocalStartedCount();
        });

        QObject::connect(pWin, &RtcWindow::localStreamError, [](const QString &err){
            qDebug() << "Error starting local stream!" << err;
        });

        pMainWin->setCentralWidget(pWin);
        pMainWin->resize(400,800);
        pMainWin->show();
    }

    int status = app.exec();
    mainWindows.clear();
    return status;
}
