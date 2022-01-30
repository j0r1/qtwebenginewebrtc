#include "rtcwindow.h"
#include <QApplication>
#include <QWebEngineSettings>
#include <QMainWindow>
#include <memory>
#include <vector>
#include <cassert>

using namespace std;

// Will be executed when all local streams (webcams) have been started
void startStreams(const vector<RtcWindow*> &rtcWindows,
    vector<QString> &streamUuids,
    vector<vector<QString>> &iceCandidateBuffer)
{
    qDebug() << "Starting all streams";
    assert(rtcWindows.size() == 2);

    streamUuids[0] = rtcWindows[0]->startGenerateOffer("Remote user (1)");
    QObject::connect(rtcWindows[0], &RtcWindow::generatedOffer, [&streamUuids, &rtcWindows, &iceCandidateBuffer](const QString &streamUuid, const QString &offer) {
        
        streamUuids[1] = rtcWindows[1]->startFromOffer(offer, "Remote user (0)");

        // Check if there's something in the iceBuffer, because we didn't know the uuid yet

        for (auto &s : iceCandidateBuffer[1])
            rtcWindows[1]->addIceCandidate(streamUuids[1], s);
        iceCandidateBuffer[1].clear();
    });

    QObject::connect(rtcWindows[1], &RtcWindow::generatedAnswer, [&streamUuids,&rtcWindows](const QString &streamUuid, const QString &answer) {
        
        rtcWindows[0]->processAnswer(streamUuids[0], answer);

        
    });

    auto iceHandler = [&rtcWindows, &streamUuids, &iceCandidateBuffer](size_t idx)
    {
        return [idx,&rtcWindows, &streamUuids, &iceCandidateBuffer](const QString &streamUuid, const QString &candidate)
        {
            size_t otherIdx = 1-idx;
            if (streamUuids[otherIdx].isEmpty())
                iceCandidateBuffer[otherIdx].push_back(candidate);
            else
                rtcWindows[otherIdx]->addIceCandidate(streamUuids[otherIdx], candidate);
        };
    };

    QObject::connect(rtcWindows[0], &RtcWindow::newIceCandidate, iceHandler(0));
    QObject::connect(rtcWindows[1], &RtcWindow::newIceCandidate, iceHandler(1));
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

	// Needed?
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

	vector<unique_ptr<QMainWindow>> mainWindows;
    vector<RtcWindow*> rtcWindows;

	for (size_t i = 0 ; i < 2 ; i++)
		mainWindows.push_back(make_unique<QMainWindow>());

    vector<QString> streamUuids(mainWindows.size());
    vector<vector<QString>> iceCandidateBuffer(mainWindows.size());

    size_t count = 0;
    auto updateLocalStartedCount = [&count,&mainWindows,&rtcWindows,&streamUuids,&iceCandidateBuffer]()
    {
        count++;
        qDebug() << count;
        if (count == mainWindows.size())
            startStreams(rtcWindows, streamUuids, iceCandidateBuffer);
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
