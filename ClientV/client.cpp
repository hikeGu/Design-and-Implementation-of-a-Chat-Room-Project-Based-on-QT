#include "client.h"
#include <QDataStream>
#include <QFile>

Client::Client(QObject *parent)
    : QObject(parent), socket(new QTcpSocket(this)), fileSocket(nullptr),
    currentUsername(""), currentDownloadPath(""), currentDownloadFile(nullptr), remainingBytes(0)
{
    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
}

Client::~Client()
{
    disconnectFromServer();
    if (fileSocket) {
        fileSocket->deleteLater();
    }
}

void Client::connectToServer(const QString &ip, int port)
{
    serverIp = ip;
    serverPort = port;
    socket->connectToHost(ip, port);
}

void Client::disconnectFromServer()
{
    socket->disconnectFromHost();
    if (fileSocket && fileSocket->state() == QAbstractSocket::ConnectedState) {
        fileSocket->disconnectFromHost();
    }
}

bool Client::isConnected() const
{
    return socket->state() == QAbstractSocket::ConnectedState;
}

void Client::sendLogin(const QString &username, const QString &password)
{
    currentUsername = username;
    sendData(QString("LOGIN|%1|%2").arg(username, password));
}

void Client::sendRegister(const QString &username, const QString &password)
{
    sendData(QString("REGISTER|%1|%2").arg(username, password));
}

void Client::sendMessage(const QString &message)
{
    sendData(QString("MSG|%1").arg(message));
}

void Client::sendPrivateMessage(const QString &receiver, const QString &message)
{
    sendData(QString("PRIVATE|%1|%2").arg(receiver, message));
}

void Client::sendFile(const QString &filePath, const QString &filename, qint64 size)
{
    connectToFileServer();
    if (!fileSocket || fileSocket->state() != QAbstractSocket::ConnectedState) {
        emit messageReceived("无法连接文件传输服务器！");
        return;
    }

    // 存储文件信息，等待服务端响应后再发送内容
    pendingFilePath = filePath;
    pendingFileName = filename;

    // 发送文件上传请求
    sendData(QString("FILE_UPLOAD|%1|%2|%3").arg(currentUsername, filename).arg(size), fileSocket);
}

void Client::requestFile(const QString &filename, const QString &savePath)
{
    currentDownloadPath = savePath;
    connectToFileServer();
    if (!fileSocket || fileSocket->state() != QAbstractSocket::ConnectedState) {
        emit messageReceived("无法连接文件传输服务器！");
        return;
    }
    sendData(QString("FILE_DOWNLOAD|%1|%2").arg(currentUsername, filename), fileSocket);
}

void Client::requestFileList()
{
    connectToFileServer();
    if (!fileSocket || fileSocket->state() != QAbstractSocket::ConnectedState) {
        emit messageReceived("无法连接文件传输服务器！");
        return;
    }
    sendData("FILE_LIST|", fileSocket);
}

void Client::requestHist()
{
    sendData("HISTORY|");
}

void Client::onConnected()
{
    emit connectionChanged(true);
}

void Client::onDisconnected()
{
    emit connectionChanged(false);
}

void Client::onReadyRead()
{
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_15);

    while (socket->bytesAvailable()) {
        QString data;
        in >> data;
        if (data == "LOGIN_OK") {
            emit messageReceived("登录成功！");
        } else if (data == "LOGIN_FAIL") {
            emit messageReceived("登录失败，请检查用户名和密码！");
        } else if (data == "REGISTER_OK") {
            emit messageReceived("注册成功，请登录！");
        } else if (data == "REGISTER_FAIL") {
            emit messageReceived("注册失败，用户名可能已存在！");
        } else if (data.startsWith("MSG|")) {
            QStringList parts = data.split("|");
            if (parts.size() >= 3) {
                QString sender = parts[1];
                QString message = parts[2];
                if (sender != currentUsername) {
                    emit messageReceived(QString("[%1] %2").arg(sender, message));
                }
            }
        } else if (data.startsWith("PRIVATE|")) {
            QStringList parts = data.split("|");
            if (parts.size() >= 3) {
                emit messageReceived(QString("[私聊 %1] %2").arg(parts[1], parts[2]));
            }
        } else if (data.startsWith("HISTORY_OK|")) {
            emit messageHist(data);
        } else if (data.startsWith("FILE_FAIL|") || data.startsWith("DOWNLOAD_FAIL|") || data.startsWith("RFile_FAIL|")) {
            connectToFileServer();
            QStringList parts = data.split("|");
            if (parts.size() >= 2 && parts[1].contains("请连接文件传输服务器")) {
                emit messageReceived("文件操作已引导至文件传输服务器 (端口 12345)");
            }
        }
    }
}

void Client::onFileServerConnected()
{
    emit messageReceived("已连接文件传输服务器");
}

void Client::onFileServerReadyRead()
{
    QDataStream in(fileSocket);
    in.setVersion(QDataStream::Qt_5_15);

    while (fileSocket->bytesAvailable()) {
        if (currentDownloadFile && currentDownloadFile->isOpen()) {
            qint64 bytesToRead = qMin(fileSocket->bytesAvailable(), remainingBytes);
            QByteArray chunk = fileSocket->read(bytesToRead);
            qint64 bytesWritten = currentDownloadFile->write(chunk);
            remainingBytes -= bytesWritten;

            if (remainingBytes <= 0) {
                currentDownloadFile->close();
                delete currentDownloadFile;
                currentDownloadFile = nullptr;
                emit messageReceived("文件下载完成");
            }
        } else {
            QString data;
            in >> data;
            if (data == "FILE_UPLOAD_OK") {
                emit messageReceived("文件上传完成");
                pendingFilePath.clear();
                pendingFileName.clear();
            } else if (data == "FILE_UPLOAD_FAIL") {
                emit messageReceived("文件上传失败");
                pendingFilePath.clear();
                pendingFileName.clear();
            } else if (data == "FILE_UPLOAD_READY" && !pendingFilePath.isEmpty()) {
                // 服务端准备好后发送文件内容
                sendFileContent(pendingFilePath, pendingFileName);
            } else if (data.startsWith("DOWNLOAD_OK|")) {
                QStringList parts = data.split("|");
                if (parts.size() >= 3) {
                    QString filename = parts[1];
                    qint64 size = parts[2].toLongLong();
                    currentDownloadFile = new QFile(currentDownloadPath);
                    if (currentDownloadFile->open(QIODevice::WriteOnly)) {
                        remainingBytes = size;
                        emit messageReceived(QString("开始下载文件: %1 (%2 bytes)").arg(filename).arg(size));
                    } else {
                        emit messageReceived("无法保存下载文件！");
                        delete currentDownloadFile;
                        currentDownloadFile = nullptr;
                    }
                }
            } else if (data.startsWith("DOWNLOAD_FAIL|")) {
                emit messageReceived("下载失败: " + data.section('|', 1));
            } else if (data.startsWith("LIST_OK|")) {
                emit messageFL(data.section('|', 1));
            } else if (data.startsWith("LIST_FAIL|")) {
                emit messageReceived("获取文件列表失败: " + data.section('|', 1));
            }
        }
    }
}

void Client::onFileServerDisconnected()
{
    if (currentDownloadFile) {
        currentDownloadFile->close();
        delete currentDownloadFile;
        currentDownloadFile = nullptr;
        emit messageReceived("文件传输服务器断开，下载中断");
    }
}

void Client::sendData(const QString &data, QTcpSocket *targetSocket)
{
    QTcpSocket *sock = targetSocket ? targetSocket : socket;
    if (sock->state() != QAbstractSocket::ConnectedState) {
        emit messageReceived("未连接到服务器！");
        return;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << data;
    sock->write(block);
}

void Client::sendFileContent(const QString &filePath, const QString &filename)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit messageReceived(QString("无法打开文件: %1").arg(filename));
        return;
    }

    emit messageReceived(QString("开始上传文件: %1").arg(filename));

    // 分块发送文件内容
    while (!file.atEnd()) {
        QByteArray chunk = file.read(16384); // 每次发送 16KB
        fileSocket->write(chunk);
        fileSocket->waitForBytesWritten(3000); // 等待写入完成，超时 3 秒
    }

    file.close();
    // 不在这里发出完成信号，等待服务端响应 FILE_UPLOAD_OK
}

void Client::connectToFileServer()
{
    if (!fileSocket) {
        fileSocket = new QTcpSocket(this);
        connect(fileSocket, &QTcpSocket::connected, this, &Client::onFileServerConnected);
        connect(fileSocket, &QTcpSocket::readyRead, this, &Client::onFileServerReadyRead);
        connect(fileSocket, &QTcpSocket::disconnected, this, &Client::onFileServerDisconnected);
    }
    if (fileSocket->state() != QAbstractSocket::ConnectedState) {
        fileSocket->connectToHost(serverIp, 12345);
        if (!fileSocket->waitForConnected(3000)) {
            emit messageReceived("连接文件传输服务器失败");
        } else {
            // 连接成功后发送用户名
            sendData(QString("USERNAME|%1").arg(currentUsername), fileSocket);
        }
    }
}

