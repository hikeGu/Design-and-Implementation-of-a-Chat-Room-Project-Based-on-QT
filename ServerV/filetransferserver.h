#ifndef FILETRANSFERSERVER_H
#define FILETRANSFERSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include "databasemanager.h" // 引入 DatabaseManager 以记录文件传输
#include <QFile>

class FileTransferServer : public QObject
{
    Q_OBJECT
public:
    explicit FileTransferServer(DatabaseManager *db, QObject *parent = nullptr);
    ~FileTransferServer();

    bool startServer(const QString &ip, int port = 12345); // 默认端口 12345
    void stopServer();

signals:
    void logMessage(const QString &message); // 日志信号
    void broadcastMessage(const QString &sender, const QString &message); // 广播文件上传完成

private slots:
    void handleNewConnection();
    void readClientData();
    void clientDisconnected();

private:

    void processFileRequest(QTcpSocket *client, const QString &data);
    void sendData(QTcpSocket *socket, const QString &data);

    QTcpServer *tcpServer;
    QMap<QTcpSocket*, QFile*> fileUploads; // 正在上传的文件
    QMap<QTcpSocket*, QString> clients; // 客户端socket到用户名的映射
    DatabaseManager *dbManager; // 数据库管理，用于记录文件传输
    QString fileStoragePath = "./files/"; // 文件存储目录
};

#endif // FILETRANSFERSERVER_H
