#ifndef VOICECHATCLIENT_H
#define VOICECHATCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QAudioSource>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudioFormat>

class VoiceChatClient : public QObject
{
    Q_OBJECT

public:
    explicit VoiceChatClient(QObject *parent = nullptr);
    ~VoiceChatClient();

    void connectToServer(const QHostAddress &serverAddress, quint16 serverPort);
    void disconnectFromServer();

private slots:
    void readPendingDatagrams();
    void handleAudioInputStateChanged(QAudio::State state);
    void handleAudioOutputStateChanged(QAudio::State state);

private:
    QUdpSocket *udpSocket;
    QAudioSource *audioInput;
    QAudioSink *audioOutput;
    QIODevice *inputDevice;
    QIODevice *outputDevice;
    QHostAddress serverAddress;
    quint16 serverPort;
};

#endif // VOICECHATCLIENT_H
