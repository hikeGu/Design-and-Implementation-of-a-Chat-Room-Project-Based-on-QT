#include "server.h"
#include <QDateTime>
#include <QDebug>
#include <QDataStream>
#include <QFile>
#include <QDir>
#include <QDebug>

Server::Server(DatabaseManager *db, QObject *parent)
    : QObject(parent), tcpServer(nullptr), dbManager(db)
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &Server::handleNewConnection);

    // 确保文件存储目录存在
    QDir dir;
    if (!dir.exists(fileStoragePath)) {
        dir.mkpath(fileStoragePath);
    }

    aiClient = new AIChatClient(this);
    connect(aiClient, &AIChatClient::responseReceived,
            this, &Server::handleAIResponse);
    connect(aiClient, &AIChatClient::errorOccurred,
            this, &Server::handleAIError);
}

Server::~Server()
{
    stopServer();
}

bool Server::startServer(const QString &ip, int port)
{
    if (!tcpServer->listen(QHostAddress(ip), port)) {
        emit logMessage(QString("服务启动失败: %1").arg(tcpServer->errorString()));
        return false;
    }
    emit logMessage(QString("服务已启动: IP=%1, 端口=%2").arg(ip).arg(port));
    return true;
}

void Server::stopServer()
{
    if (tcpServer->isListening()) {
        // 获取所有活动的 QTcpSocket 对象
        QList<QTcpSocket *> allClients = tcpServer->findChildren<QTcpSocket *>();

        // 断开所有客户端连接
        for (QTcpSocket *client : allClients) {
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->disconnectFromHost(); // 断开连接
            }
        }

        // 清理客户端映射表
        for (QTcpSocket *client : clients.keys()) {
            clients.remove(client); // 从映射中移除客户端
        }

        // 关闭服务器监听
        tcpServer->close();

        // 记录日志
        emit logMessage("服务已停止");
    }
}
void Server::handleNewConnection()
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::readClientData);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::clientDisconnected);

    emit logMessage(QString("新客户端连接: %1:%2")
                        .arg(clientSocket->peerAddress().toString())
                        .arg(clientSocket->peerPort()));
}


void Server::readClientData()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QDataStream in(client);
    in.setVersion(QDataStream::Qt_5_15);

    while (client->bytesAvailable() > 0) {
        QString data;
        in >> data;
        if (!data.isEmpty()) {
            processMessage(client, data);
        }
    }
}

void Server::clientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QString username = clients.value(client, "未知用户");
    clients.remove(client);
    emit logMessage(QString("客户端断开连接: %1").arg(username));
    emit userStatusChanged(username, false);
    broadcastMessage("系统", QString("%1 已离开聊天室").arg(username));

    client->deleteLater();
}

void Server::processMessage(QTcpSocket *client, const QString &data)
{
    QStringList parts = data.split("|", Qt::SkipEmptyParts);
    if (parts.isEmpty()) return;

    QString command = parts[0];
    QByteArray response;
    QDataStream out(&response, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    if (command == "LOGIN") {
        if (parts.size() < 3) {
            out << QString("LOGIN_FAIL");
            client->write(response);
            return;
        }
        QString username = parts[1];
        QString password = parts[2];

        if (validateUser(username, password)) {
            if (clients.values().contains(username)) {
                out << QString("LOGIN_FAIL");
                emit logMessage(QString("用户 %1 尝试重复登录").arg(username));
            } else {
                clients[client] = username;
                out << QString("LOGIN_OK");
                emit logMessage(QString("用户 %1 已登录").arg(username));
                emit userStatusChanged(username, true);
                broadcastMessage("系统", QString("%1 加入了聊天室").arg(username));
            }
        } else {
            out << QString("LOGIN_FAIL");
            emit logMessage(QString("用户 %1 登录失败：密码错误或用户不存在").arg(username));
        }
        client->write(response);
    }
    else if (command == "REGISTER") {
        if (parts.size() < 3) {
            out << QString("REGISTER_FAIL");
            client->write(response);
            return;
        }
        QString username = parts[1];
        QString password = parts[2];

        if (dbManager->getAllUsers().contains(username)) {
            out << QString("REGISTER_FAIL");
            emit logMessage(QString("注册失败：用户 %1 已存在").arg(username));
        } else if (dbManager->addUser(username, password)) {
            out << QString("REGISTER_OK");
            emit logMessage(QString("用户 %1 注册成功").arg(username));
        } else {
            out << QString("REGISTER_FAIL");
            emit logMessage(QString("用户 %1 注册失败：数据库错误").arg(username));
        }
        client->write(response);
    }
    else if (command == "MSG") {
        if (parts.size() < 2 || !clients.contains(client)) return;
        QString message = parts[1];
        QString sender = clients[client];
        broadcastMessage(sender, message);
        dbManager->addChatRecord(sender, message, QDateTime::currentDateTime());
    }
    else if (command == "PRIVATE") {
        if (parts.size() < 3 || !clients.contains(client)) return;
        QString receiver = parts[1];
        QString message = parts[2];
        QString sender = clients[client];
        if(receiver == "AI"){
            aiClient->sendMessage(message);
            broadcastMessage(sender, "@AI " + message);
            dbManager->addChatRecord(sender, QString("@AI %1").arg(message), QDateTime::currentDateTime());
        }else{
            sendPrivateMessage(sender, receiver, message);
            dbManager->addChatRecord(sender, QString("[私聊 %1] %2").arg(receiver, message), QDateTime::currentDateTime());
        }
    }
    else if (command == "HISTORY") {
        if (!clients.contains(client)) return;
        sendHistory(client);
        emit logMessage(QString("用户 %1 请求历史消息").arg(clients.value(client, "未知")));
    }
    else if (command == "FILE" || command == "DOWNLOAD" || command == "RFile") {
        // 引导客户端使用 FileTransferServer
        out << QString("%1_FAIL|请连接文件传输服务器 (端口 12345) 执行此操作").arg(command);
        client->write(response);
        emit logMessage(QString("用户 %1 尝试 %2，已引导至文件传输服务器").arg(clients[client], command));
    }
    else {
        emit logMessage(QString("未知命令: %1").arg(data));
    }
}

void Server::broadcastMessage(const QString &sender, const QString &message)
{
    QString formattedMessage = QString("[%1] %2: %3")
    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"), sender, message);
    emit logMessage(formattedMessage);

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << QString("MSG|%1|%2").arg(sender, message);

    for (QTcpSocket *client : clients.keys()) {
        client->write(data);
    }
}


void Server::sendPrivateMessage(const QString &sender, const QString &receiver, const QString &message)
{
    QString formattedMessage = QString("[私聊 %1] %2: %3")
                                   .arg(receiver, QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"), message);
    emit logMessage(formattedMessage);

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << QString("PRIVATE|%1|%2").arg(sender, message);

    bool sent = false;
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it.value() == receiver) {
            it.key()->write(data);
            sent = true;
            break;
        }
    }
    if (!sent) {
        emit logMessage(QString("私聊失败: 用户 %1 未在线").arg(receiver));
        QTcpSocket *senderSocket = clients.key(sender);
        if (senderSocket) {
            QByteArray errorData;
            QDataStream errorOut(&errorData, QIODevice::WriteOnly);
            errorOut.setVersion(QDataStream::Qt_5_15);
            errorOut << QString("PRIVATE|系统|用户 %1 未在线").arg(receiver);
            senderSocket->write(errorData);
        }
    }
}

bool Server::validateUser(const QString &username, const QString &password)
{
    return dbManager->validateUser(username, password); // 直接调用 DatabaseManager 的方法
}

void Server::sendHistory(QTcpSocket *client)
{
    QList<QStringList> history = dbManager->getMessageHistory(10000);
    if (history.isEmpty()) {
        sendData(client, "HISTORY_OK|");
        return;
    }

    QString historyData = "HISTORY_OK|";
    for (const QStringList &msg : history) {
        if (msg.size() >= 3) {
            QString sender = msg[0];    // sender
            QString content = msg[1];   // message
            QString time = msg[2];      // timestamp

            // 过滤掉私聊消息（以 "[私聊 " 开头）
            if (!content.startsWith("[私聊 ")) {
                historyData += QString("%1|%2|%3|").arg(sender, content, time);
            }
        }
    }

    if (historyData == "HISTORY_OK|") {
        // 如果没有非私聊消息，返回空历史
        sendData(client, "HISTORY_OK|");
    } else {
        historyData.chop(1); // 移除末尾的 "|"
        sendData(client, historyData);
    }
}




void Server::sendData(QTcpSocket *socket, const QString &data)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << data;
    socket->write(block);
}

void Server::handleAIResponse(const QString &response)
{
    qDebug() << "AI回复:" << response;
    // 在这里处理AI的回复
    broadcastMessage("AI", response);
    dbManager->addChatRecord("AI", response, QDateTime::currentDateTime());
}

void Server::handleAIError(const QString &errorMessage)
{
    qWarning() << "AI请求错误:" << errorMessage;
    // 在这里处理错误
}
