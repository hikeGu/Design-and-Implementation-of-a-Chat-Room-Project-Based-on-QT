#include "voicechatserver.h"

VoiceChatServer::VoiceChatServer(QObject *parent) : QObject(parent)
{
    udpSocket = new QUdpSocket(this);
}

VoiceChatServer::~VoiceChatServer()
{
    stopServer();
    delete udpSocket;
}

void VoiceChatServer::startServer(quint16 port)
{
    if (udpSocket->bind(QHostAddress::Any, port)) {
        connect(udpSocket, &QUdpSocket::readyRead, this, &VoiceChatServer::readPendingDatagrams);
    }
}

void VoiceChatServer::stopServer()
{
    udpSocket->close();
    clients.clear();
}

void VoiceChatServer::readPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QString clientId = QString("%1:%2").arg(sender.toString()).arg(senderPort);

        if (!clients.contains(clientId)) {
            clients[clientId] = qMakePair(sender, senderPort);
            emit clientConnected(clientId);
        }

        // Forward the datagram to all other clients
        for (auto it = clients.begin(); it != clients.end(); ++it) {
            if (it.key() != clientId) {
                udpSocket->writeDatagram(datagram, it.value().first, it.value().second);
            }
        }
    }
}

void VoiceChatServer::handleClientDisconnected(const QString &clientId)
{
    clients.remove(clientId);
    emit clientDisconnected(clientId);
}
