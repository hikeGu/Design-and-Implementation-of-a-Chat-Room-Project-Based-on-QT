#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);
    ~Client();

    void connectToServer(const QString &ip, int port);
    void disconnectFromServer();
    bool isConnected() const;

    void sendLogin(const QString &username, const QString &password);
    void sendRegister(const QString &username, const QString &password);
    void sendMessage(const QString &message);
    void sendPrivateMessage(const QString &receiver, const QString &message);
    void sendFile(const QString &filePath, const QString &filename, qint64 size);
    void requestFile(const QString &filename, const QString &savePath);
    void requestFileList();

    void requestHist();

signals:
    void connectionChanged(bool connected);
    void messageReceived(const QString &message);
    void messageHist(const QString &message);
    void messageFL(const QString &fileList);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onFileServerConnected();
    void onFileServerReadyRead();
    void onFileServerDisconnected();

private:
    void sendData(const QString &data, QTcpSocket *targetSocket = nullptr);
    void sendFileContent(const QString &filePath, const QString &filename);
    void connectToFileServer();

    QTcpSocket *socket; // 主服务器（聊天）连接
    QTcpSocket *fileSocket; // 文件传输服务器连接
    QString currentUsername;
    QString currentDownloadPath;
    QString serverIp;
    int serverPort;
    QFile *currentDownloadFile; // 当前下载的文件
    qint64 remainingBytes; // 剩余下载字节数

    QString pendingFilePath; // 新增：存储待发送的文件路径
    QString pendingFileName; // 新增：存储待发送的文件名
};

#endif // CLIENT_H
