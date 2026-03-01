#include "filetransferserver.h"
#include <QDataStream>
#include <QFile>
#include <QDir>
#include <QDateTime>

FileTransferServer::FileTransferServer(DatabaseManager *db, QObject *parent)
    : QObject(parent), tcpServer(nullptr), dbManager(db)
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &FileTransferServer::handleNewConnection);

    // 确保文件存储目录存在
    QDir dir;
    if (!dir.exists(fileStoragePath)) {
        dir.mkpath(fileStoragePath);
    }
}

FileTransferServer::~FileTransferServer()
{
    stopServer();
}

bool FileTransferServer::startServer(const QString &ip, int port)
{
    if (!tcpServer->listen(QHostAddress(ip), port)) {
        emit logMessage(QString("文件传输服务启动失败: %1").arg(tcpServer->errorString()));
        return false;
    }
    emit logMessage(QString("文件传输服务已启动: IP=%1, 端口=%2").arg(ip).arg(port));
    return true;
}

void FileTransferServer::stopServer()
{
    if (tcpServer->isListening()) {
        QList<QTcpSocket *> allClients = tcpServer->findChildren<QTcpSocket *>();
        for (QTcpSocket *client : allClients) {
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->disconnectFromHost();
            }
        }
        for (QTcpSocket *client : fileUploads.keys()) {
            QFile *file = fileUploads.take(client);
            if (file->isOpen()) file->close();
            file->remove();
            delete file;
        }
        clients.clear();
        tcpServer->close();
        emit logMessage("文件传输服务已停止");
    }
}

void FileTransferServer::handleNewConnection()
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &FileTransferServer::readClientData);
    connect(clientSocket, &QTcpSocket::disconnected, this, &FileTransferServer::clientDisconnected);

    // 初始化客户端名称为IP地址
    clients[clientSocket] = clientSocket->peerAddress().toString();

    // 发送日志信息，显示客户端名称（IP）和完整的地址:端口
    emit logMessage(QString("新客户端 %1 连接文件传输服务: %2:%3")
                        .arg(clients.value(clientSocket))  // 使用客户端名称（即IP）
                        .arg(clientSocket->peerAddress().toString())  // 显示IP地址
                        .arg(clientSocket->peerPort()));  // 显示端口
}


void FileTransferServer::readClientData()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QDataStream in(client);
    in.setVersion(QDataStream::Qt_5_15);

    while (client->bytesAvailable() > 0) {
        if (fileUploads.contains(client)) {
            // 处理文件内容上传
            QFile *file = fileUploads[client];
            qint64 bytesToRead = qMin(client->bytesAvailable(), file->property("size").toLongLong() - file->size());
            QByteArray chunk = client->read(bytesToRead);
            qint64 bytesWritten = file->write(chunk);

            if (bytesWritten == -1) {
                emit logMessage(QString("用户 %1 文件写入失败: %2").arg(clients.value(client), file->errorString()));
                file->close();
                delete fileUploads.take(client);
                sendData(client, "FILE_UPLOAD_FAIL");
                return;
            }

            if (file->size() >= file->property("size").toLongLong()) {
                file->close();
                QString filename = file->fileName().section('/', -1);
                qint64 size = file->size();
                QString sender = clients.value(client);
                dbManager->addFileTransferRecord(sender, filename, size, QDateTime::currentDateTime());
                emit logMessage(QString("用户 %1 文件上传完成: %2 (%3 bytes)").arg(sender, filename).arg(size));
                emit broadcastMessage("系统", QString("%1 上传了文件: %2").arg(sender, filename));
                delete fileUploads.take(client);
                sendData(client, "FILE_UPLOAD_OK");
            }
        } else {
            // 处理文件请求头部
            QString data;
            in >> data;
            if (!data.isEmpty()) {
                processFileRequest(client, data);
            }
        }
    }
}

void FileTransferServer::clientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QString username = clients.value(client);
    if (fileUploads.contains(client)) {
        QFile *file = fileUploads.take(client);
        if (file->isOpen()) file->close();
        file->remove();
        emit logMessage(QString("用户 %1 断开连接，文件上传中断").arg(username));
    }
    clients.remove(client);
    emit logMessage(QString("客户端 %1 断开文件传输服务: %2:%3")
                        .arg(username)
                        .arg(client->peerAddress().toString())
                        .arg(client->peerPort()));
    client->deleteLater();
}

void FileTransferServer::processFileRequest(QTcpSocket *client, const QString &data)
{
    QStringList parts = data.split("|", Qt::SkipEmptyParts);
    if (parts.isEmpty()) return;

    QString command = parts[0];
    QByteArray response;
    QDataStream out(&response, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    if (command == "USERNAME") { // 处理客户端发送的用户名
        if (parts.size() >= 2) {
            QString username = parts[1].trimmed();
            QString oldName = clients.value(client);

            // 验证用户名格式（可选）
            if (username.isEmpty() || username.length() > 20) {
                emit logMessage("客户端 " + oldName + " 提供了无效的用户名");
                return;
            }

            clients[client] = username;
            emit logMessage(QString("新客户端 %1（原IP %2）的用户名为 %3")
                                .arg(oldName)
                                .arg(oldName)
                                .arg(username));
        }
    }
    else if (command == "FILE" || command == "FILE_UPLOAD") { // 兼容原协议和新协议
        if (parts.size() < 3) {
            out << QString("FILE_UPLOAD_FAIL|参数不足");
            client->write(response);
            return;
        }
        QString username = (parts.size() >= 4) ? parts[1] : clients.value(client, "未知用户"); // 优先使用协议中的用户名
        QString filename = (command == "FILE") ? parts[1] : parts[2];
        qint64 size = (command == "FILE") ? parts[2].toLongLong() : parts[3].toLongLong();

        clients[client] = username; // 更新客户端用户名
        QFile *file = new QFile(fileStoragePath + filename);
        if (file->open(QIODevice::WriteOnly)) {
            file->setProperty("size", size);
            fileUploads[client] = file;
            out << QString("FILE_UPLOAD_READY");
            emit logMessage(QString("用户 %1 开始上传文件: %2 (%3 bytes)").arg(username, filename).arg(size));
        } else {
            out << QString("FILE_UPLOAD_FAIL|文件打开失败");
            emit logMessage(QString("用户 %1 文件 %2 打开失败").arg(username, filename));
            delete file;
        }
        client->write(response);
    }
    else if (command == "DOWNLOAD" || command == "FILE_DOWNLOAD") {
        if (parts.size() < 2) {
            out << QString("FILE_DOWNLOAD_FAIL|参数不足");
            client->write(response);
            return;
        }
        QString username = (parts.size() >= 3) ? parts[1] : clients.value(client, "未知用户");
        QString filename = (command == "DOWNLOAD") ? parts[1] : parts[2];
        clients[client] = username;

        QFile file(fileStoragePath + filename);
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            qint64 fileSize = file.size();
            QByteArray fileData = file.readAll();
            file.close();

            out << QString("DOWNLOAD_OK|%1|%2").arg(filename).arg(fileSize); // 兼容原协议
            client->write(response);
            client->write(fileData);
            emit logMessage(QString("用户 %1 下载文件: %2 (%3 bytes)").arg(username, filename).arg(fileSize));
        } else {
            out << QString("DOWNLOAD_FAIL|文件不存在");
            emit logMessage(QString("用户 %1 请求的文件 %2 未找到").arg(username, filename));
            client->write(response);
        }
    }
    else if (command == "RFile" || command == "FILE_LIST") {
        // 从数据库获取文件记录
        QVector<QVector<QString>> fileRecords = dbManager->getFileTransferRecords(1000); // 假设获取最近 1000 条记录，可调整 limit

        // 构建详细的文件列表
        QStringList entries;
        for (const QVector<QString> &record : fileRecords) {
            if (record.size() >= 4) { // 确保记录包含 sender, filename, size, timestamp
                QString entry = QString("%1|%2|%3|%4")
                                    .arg(record[0]) // sender
                                    .arg(record[1]) // filename
                                    .arg(record[2]) // size
                                    .arg(record[3]); // timestamp
                entries << entry;
            }
        }

        // 即使记录为空，也返回 LIST_OK|（空列表）
        out << QString("LIST_OK|%1").arg(entries.join(","));
        client->write(response);
        emit logMessage(QString("客户端 %1 请求文件列表，已返回 %2 条记录")
                            .arg(clients.value(client, "未知用户"))
                            .arg(entries.size()));
    }
    else {
        emit logMessage(QString("客户端 %1 发送未知文件传输命令: %2")
                            .arg(clients.value(client, "未知用户"))
                            .arg(data));
    }
}


void FileTransferServer::sendData(QTcpSocket *socket, const QString &data)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << data;
    socket->write(block);
}
