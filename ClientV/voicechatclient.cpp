#include "voicechatclient.h"
#include <QAudioFormat>
#include <QMediaDevices>
#include <QDebug>

VoiceChatClient::VoiceChatClient(QObject *parent) : QObject(parent)
{
    udpSocket = new QUdpSocket(this);
    connect(udpSocket, &QUdpSocket::readyRead, this, &VoiceChatClient::readPendingDatagrams);

    // 设置音频格式
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    // 初始化音频输入
    QAudioDevice inputDeviceInfo = QMediaDevices::defaultAudioInput();
    if (!inputDeviceInfo.isFormatSupported(format)) {
        qWarning() << "Default input format not supported, trying to use the nearest.";
        format = inputDeviceInfo.preferredFormat();
    }
    audioInput = new QAudioSource(inputDeviceInfo, format);
    connect(audioInput, &QAudioSource::stateChanged, this, &VoiceChatClient::handleAudioInputStateChanged);

    // 初始化音频输出
    QAudioDevice outputDeviceInfo = QMediaDevices::defaultAudioOutput();
    if (!outputDeviceInfo.isFormatSupported(format)) {
        qWarning() << "Default output format not supported, trying to use the nearest.";
        format = outputDeviceInfo.preferredFormat();
    }
    audioOutput = new QAudioSink(outputDeviceInfo, format);
    connect(audioOutput, &QAudioSink::stateChanged, this, &VoiceChatClient::handleAudioOutputStateChanged);
}

VoiceChatClient::~VoiceChatClient()
{
    disconnectFromServer();
    delete udpSocket;
    delete audioInput;
    delete audioOutput;
}

void VoiceChatClient::connectToServer(const QHostAddress &serverAddress, quint16 serverPort)
{
    this->serverAddress = serverAddress;
    this->serverPort = serverPort;

    // 开始音频输入
    inputDevice = audioInput->start();
    connect(inputDevice, &QIODevice::readyRead, this, [this]() {
        QByteArray data = inputDevice->readAll();
        udpSocket->writeDatagram(data, this->serverAddress, this->serverPort);
    });

    // 开始音频输出
    outputDevice = audioOutput->start();
}


void VoiceChatClient::disconnectFromServer()
{
    audioInput->stop();
    audioOutput->stop();
}

void VoiceChatClient::readPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());

        // 将接收到的音频数据写入音频输出设备
        outputDevice->write(datagram);
    }
}

void VoiceChatClient::handleAudioInputStateChanged(QAudio::State state)
{
    if (state == QAudio::StoppedState && audioInput->error() != QAudio::NoError) {
        qWarning() << "Audio input error:" << audioInput->error();
    }
}

void VoiceChatClient::handleAudioOutputStateChanged(QAudio::State state)
{
    if (state == QAudio::StoppedState && audioOutput->error() != QAudio::NoError) {
        qWarning() << "Audio output error:" << audioOutput->error();
    }
}
