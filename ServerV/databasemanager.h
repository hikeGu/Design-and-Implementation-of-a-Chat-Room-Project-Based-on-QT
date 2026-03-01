#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVector>
#include <QDateTime>

class DatabaseManager
{
public:
    DatabaseManager(const QString &dbPath = "chatserver.db");
    ~DatabaseManager();

    // 初始化数据库并创建表
    bool initialize();

    // 用户管理
    bool addUser(const QString &username, const QString &password);
    bool removeUser(const QString &username);
    bool updateUserPassword(const QString &username, const QString &newPassword);
    bool validateUser(const QString &username, const QString &password); // 新增验证方法
    QVector<QString> getAllUsers();

    // 聊天记录管理
    bool addChatRecord(const QString &sender, const QString &message, const QDateTime &timestamp = QDateTime::currentDateTime());
    bool clearChatRecords();
    bool deleteChatRecord(const QString &sender, const QString &message, const QDateTime &timestamp);
    QVector<QVector<QString>> getChatRecords(int limit = 100); // 返回 sender, message, timestamp
    QList<QStringList> getMessageHistory(int limit);

    // 文件传输记录管理
    bool addFileTransferRecord(const QString &sender, const QString &filename, qint64 size, const QDateTime &timestamp = QDateTime::currentDateTime());
    bool clearFileTransferRecords();
    bool deleteFileTransferRecord(const QString &sender, const QString &filename, const QDateTime &timestamp);
    QVector<QVector<QString>> getFileTransferRecords(int limit = 100); // 返回 sender, filename, size, timestamp

    // 检查数据库是否正常打开
    bool isOpen() const;

    QVector<QVector<QString> > getChatRecordsByKeyword(const QString &keyword, int limit);
    QVector<QVector<QString> > getChatRecordsByUser(const QString &user, int limit);
private:
    QSqlDatabase db;

    // 创建表的辅助函数
    bool createUserTable();
    bool createChatTable();
    bool createFileTransferTable();
    QString hashPassword(const QString &password); // 密码加密辅助函数

};

#endif // DATABASEMANAGER_H
