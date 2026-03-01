#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include "databasemanager.h"
#include <QFile>
#include "aichatclient.h"

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(DatabaseManager *db, QObject *parent = nullptr);
    ~Server();

    bool startServer(const QString &ip, int port);
    void stopServer();
    QMap<QTcpSocket*, QString> getClients() const { return clients; }
    void broadcastMessage(const QString &sender, const QString &message);

    void handleAIResponse(const QString &response);
    void handleAIError(const QString &errorMessage);
signals:
    void logMessage(const QString &message);
    void userStatusChanged(const QString &username, bool isOnline); // 新增信号

private slots:
    void handleNewConnection();
    void readClientData();
    void clientDisconnected();

private:
    QTcpServer *tcpServer;
    QMap<QTcpSocket*, QString> clients; // 客户端socket和用户名的映射
    QMap<QTcpSocket*, QFile*> fileUploads; // 正在上传的文件
    DatabaseManager *dbManager;
    const QString fileStoragePath = "./files/"; // 文件存储目录

    void sendPrivateMessage(const QString &sender, const QString &receiver, const QString &message);
    void processMessage(QTcpSocket *client, const QString &data);
    bool validateUser(const QString &username, const QString &password);
    void sendHistory(QTcpSocket *client); // 新增：发送历史消息
    void sendData(QTcpSocket *socket, const QString &data); // 新增：发送数据辅助函数

    AIChatClient *aiClient;
};

#endif // SERVER_H
