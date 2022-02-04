#pragma once

#include <QMainWindow>
#include <QWebSocket>
#include "rtcwindow.h"

class MainWin : public QMainWindow
{
	Q_OBJECT
public:
	MainWin(const QString &serverUrl, const QString &displayName, const QString &roomName);
	~MainWin();
private slots:
	void onLocalVideoStarted();
	void onGeneratedOffer(const QString &streamUuid, const QString &offer);
	void onGeneratedAnswer(const QString &streamUuid, const QString &answer);
	void onNewIceCandidate(const QString &streamUuid, const QString &candidate);
	void onWebSocketConnected();
	void onTextMessage(const QString &msg);
private:
	void processRoomMessage(const QJsonObject &obj);
	void onPersonalMessage(const QJsonObject &obj);
	void onUserJoined(const QString &uuid);
	void onUserLeft(const QString &uuid);

	RtcWindow *m_pRtcWin;
	QWebSocket *m_pSock;
	QString m_displayName, m_wsUrl, m_roomName;
	QString m_ownUserUuid;
};
