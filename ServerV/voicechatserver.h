#ifndef VOICECHATSERVER_H
#define VOICECHATSERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QMap>
#include <QHostAddress>

class VoiceChatServer : public QObject
{
    Q_OBJECT

public:
    explicit VoiceChatServer(QObject *parent = nullptr);
    ~VoiceChatServer();

    void startServer(quint16 port);
    void stopServer();

signals:
    void clientConnected(const QString &clientId);
    void clientDisconnected(const QString &clientId);

private slots:
    void readPendingDatagrams();
    void handleClientDisconnected(const QString &clientId);

private:
    QUdpSocket *udpSocket;
    QMap<QString, QPair<QHostAddress, quint16>> clients;
};

#endif // VOICECHATSERVER_H
