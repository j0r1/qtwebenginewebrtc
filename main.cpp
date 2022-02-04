#include "mainwin.h"
#include <QApplication>
#include <QWebEngineSettings>
#include <memory>
#include <vector>
#include <cassert>
#include <string>

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

int main_signalslot(QApplication &app, const vector<QString> &args)
{
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

int main_testroomserver(QApplication &app, const vector<QString> &args)
{
	if (args.size() != 2)
	{
		qCritical() << "Usage: qtwebrtctest 'testroomserver' wsurl num";
		return -1;
	}

	QString wsUrl(args[0]);
	int num = args[1].toInt();
	if (num < 1)
	{
		qCritical() << "Nothing to do";
		return -1;
	}
	if (num > 16)
	{
		qCritical() << "Too many instances requested";
		return -1;
	}

	vector<unique_ptr<MainWin>> mainWindows;
	for (int i = 0 ; i < num ; i++)
		mainWindows.push_back(make_unique<MainWin>(wsUrl, "Local user (" + QString::number(i) + ")", "ABCDEF"));

	int status = app.exec();
	mainWindows.clear();
	return status;
}

int main_singleinstance(QApplication &app, const vector<QString> &args)
{
	if (args.size() != 3)
	{
		qCritical() << "Usage: qtwebrtctest 'main' wsurl roomid(6 capital letters) displayname";
		return -1;
	}

	QString wsUrl(args[0]);
    QString roomId(args[1]);
    QString displayName(args[2]);
	
    MainWin win(wsUrl, displayName, roomId);
	int status = app.exec();
	return status;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);    

    // This is required to allow the videos to play immediately, otherwise an
    // error may be given that the user needs to interact with the webpage first
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
	// Needed?
    // QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    auto globalUsage = [](){
        qCritical() << R"XYZ(The first argument should specify a type:
    - testsignalslot: for a single program that uses two RtcWindow instances, 
                  connecting them directly with signals & slots
    - testroomserver: for a single program in which multiple RtcWindow
                       instances are connected through a server program
    - main: starts a single instance and connects to a server program. Start
            one or more other programs with the same server/room to have
            multiple streams
)XYZ";
        return -1;
    };

    if (argc < 2)
        return globalUsage();
    
    QString type(argv[1]);
    vector<QString> args;
    for (size_t i = 2 ; i < argc ; i++)
        args.push_back(argv[i]);

    if (type == "testsignalslot")
        return main_signalslot(app, args);
    if (type == "testroomserver")
        return main_testroomserver(app, args);
    if (type == "main")
        return main_singleinstance(app, args);

	return globalUsage();
}
