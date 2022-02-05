#pragma once

#include <QMainWindow>
#include <QWebSocket>
#include "rtcwindow.h"
#include "ui_mainwin.h"
#include <map>
#include <vector>

class MainWin : public QMainWindow, public Ui_MainWin
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
	void sendJson(const QJsonObject &obj);
	void processRoomMessage(const QJsonObject &obj);
	void onPersonalMessage(const QJsonObject &obj);
	void onUserJoined(const QString &uuid, const QString &displayName);
	void onUserLeft(const QString &uuid);

	RtcWindow *m_pRtcWin;
	QWebSocket *m_pSock;
	QString m_displayName, m_wsUrl, m_roomName;
	QString m_ownUserUuid;

	std::map<QString,QString> m_userToStream;
	std::map<QString,QString> m_streamToUser;

	std::map<QString,std::vector<QString>> m_userIceBuffer;
};
