#pragma once

#include <QWebChannelAbstractTransport>
#include <map>

class QWebSocket;

class WebSocketChannel : public QWebChannelAbstractTransport
{
public:
	WebSocketChannel(QObject *pParent = nullptr);
	~WebSocketChannel();
	bool start();
	int getPort() const { return m_port; }
	QString getHandshakeIdentifier() const { return m_id; }
public slots:
	void sendMessage(const QJsonObject &message) override;
private slots:
	void onNewConnection();
	void onDisconnected();
	void onWebSocketMessage(const QString &s);
private:
	int m_port;
	QString m_id;
	std::map<QWebSocket*,bool> m_connections; // bool is to check if handshake was completed
};