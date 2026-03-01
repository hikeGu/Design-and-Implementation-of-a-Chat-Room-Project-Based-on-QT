#include "databasemanager.h"
#include <QSqlError>
#include <QDebug>
#include <QCryptographicHash>

DatabaseManager::DatabaseManager(const QString &dbPath)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "Error: Failed to open database:" << db.lastError().text();
    }
}

DatabaseManager::~DatabaseManager()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool DatabaseManager::initialize()
{
    if (!isOpen()) {
        return false;
    }

    return createUserTable() && createChatTable() && createFileTransferTable();
}

bool DatabaseManager::createUserTable()
{
    QSqlQuery query;
    QString createTableQuery =
        "CREATE TABLE IF NOT EXISTS users ("
        "username TEXT PRIMARY KEY,"
        "password TEXT NOT NULL)";

    if (!query.exec(createTableQuery)) {
        qDebug() << "Error creating users table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createChatTable()
{
    QSqlQuery query;
    QString createTableQuery =
        "CREATE TABLE IF NOT EXISTS chat_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT NOT NULL,"
        "message TEXT NOT NULL,"
        "timestamp DATETIME NOT NULL)";

    if (!query.exec(createTableQuery)) {
        qDebug() << "Error creating chat_records table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createFileTransferTable()
{
    QSqlQuery query;
    QString createTableQuery =
        "CREATE TABLE IF NOT EXISTS file_transfers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT NOT NULL,"
        "filename TEXT NOT NULL,"
        "size INTEGER NOT NULL,"
        "timestamp DATETIME NOT NULL)";

    if (!query.exec(createTableQuery)) {
        qDebug() << "Error creating file_transfers table:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::addUser(const QString &username, const QString &password)
{
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password) VALUES (:username, :password)");
    query.bindValue(":username", username);
    query.bindValue(":password", hashPassword(password)); // 存储加密后的密码

    if (!query.exec()) {
        qDebug() << "Error adding user:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removeUser(const QString &username)
{
    QSqlQuery query;
    query.prepare("DELETE FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec()) {
        qDebug() << "Error removing user:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::updateUserPassword(const QString &username, const QString &newPassword)
{
    QSqlQuery query;
    query.prepare("UPDATE users SET password = :password WHERE username = :username");
    query.bindValue(":username", username);
    query.bindValue(":password", hashPassword(newPassword)); // 存储加密后的新密码

    if (!query.exec()) {
        qDebug() << "Error updating user password:" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<QString> DatabaseManager::getAllUsers()
{
    QVector<QString> users;
    QSqlQuery query("SELECT username FROM users");

    while (query.next()) {
        users.append(query.value(0).toString());
    }
    return users;
}

bool DatabaseManager::validateUser(const QString &username, const QString &password)
{
    QSqlQuery query;
    query.prepare("SELECT password FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        QString storedPassword = query.value(0).toString();
        QString hashedInputPassword = hashPassword(password);
        return storedPassword == hashedInputPassword; // 比较加密后的密码
    }
    return false;
}

QString DatabaseManager::hashPassword(const QString &password)
{
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool DatabaseManager::addChatRecord(const QString &sender, const QString &message, const QDateTime &timestamp)
{
    QSqlQuery query;
    query.prepare("INSERT INTO chat_records (sender, message, timestamp) VALUES (:sender, :message, :timestamp)");
    query.bindValue(":sender", sender);
    query.bindValue(":message", message);
    query.bindValue(":timestamp", timestamp.toString(Qt::ISODate));

    if (!query.exec()) {
        qDebug() << "Error adding chat record:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::clearChatRecords()
{
    QSqlQuery query("DELETE FROM chat_records");
    if (!query.exec()) {
        qDebug() << "Error clearing chat records:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::deleteChatRecord(const QString &sender, const QString &message, const QDateTime &timestamp)
{
    QSqlQuery query;
    query.prepare("DELETE FROM chat_records WHERE sender = :sender AND message = :message AND timestamp = :timestamp");
    query.bindValue(":sender", sender);
    query.bindValue(":message", message);
    query.bindValue(":timestamp", timestamp);
    return query.exec();
}


QVector<QVector<QString>> DatabaseManager::getChatRecords(int limit)
{
    QVector<QVector<QString>> records;
    QSqlQuery query;
    query.prepare("SELECT sender, message, timestamp FROM chat_records ORDER BY timestamp DESC LIMIT :limit");
    query.bindValue(":limit", limit);

    if (!query.exec()) {
        qDebug() << "Error fetching chat records:" << query.lastError().text();
        return records;
    }

    while (query.next()) {
        QVector<QString> record;
        record.append(query.value(0).toString()); // sender
        record.append(query.value(1).toString()); // message
        record.append(query.value(2).toString()); // timestamp
        records.append(record);
    }
    return records;
}

bool DatabaseManager::addFileTransferRecord(const QString &sender, const QString &filename, qint64 size, const QDateTime &timestamp)
{
    QSqlQuery query;
    query.prepare("INSERT INTO file_transfers (sender, filename, size, timestamp) VALUES (:sender, :filename, :size, :timestamp)");
    query.bindValue(":sender", sender);
    query.bindValue(":filename", filename);
    query.bindValue(":size", size);
    query.bindValue(":timestamp", timestamp.toString(Qt::ISODate));

    if (!query.exec()) {
        qDebug() << "Error adding file transfer record:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::clearFileTransferRecords()
{
    QSqlQuery query("DELETE FROM file_transfers");
    if (!query.exec()) {
        qDebug() << "Error clearing file transfer records:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::deleteFileTransferRecord(const QString &sender, const QString &filename, const QDateTime &timestamp)
{
    QSqlQuery query;
    // 准备删除语句，基于 sender、filename 和 timestamp 匹配
    query.prepare("DELETE FROM file_transfers WHERE sender = :sender AND filename = :filename AND timestamp = :timestamp");
    query.bindValue(":sender", sender);
    query.bindValue(":filename", filename);
    query.bindValue(":timestamp", timestamp.toString(Qt::ISODate)); // 转换为 ISO 格式，与存储一致

    if (!query.exec()) {
        qDebug() << "Error deleting file transfer record:" << query.lastError().text();
        return false;
    }

    // 检查是否有记录被删除
    if (query.numRowsAffected() > 0) {
        qDebug() << "File transfer record deleted successfully: sender=" << sender << ", filename=" << filename << ", timestamp=" << timestamp.toString(Qt::ISODate);
        return true;
    } else {
        qDebug() << "No matching file transfer record found for deletion: sender=" << sender << ", filename=" << filename << ", timestamp=" << timestamp.toString(Qt::ISODate);
        return false;
    }
}


QVector<QVector<QString>> DatabaseManager::getFileTransferRecords(int limit)
{
    QVector<QVector<QString>> records;
    QSqlQuery query;
    query.prepare("SELECT sender, filename, size, timestamp FROM file_transfers ORDER BY timestamp DESC LIMIT :limit");
    query.bindValue(":limit", limit);

    if (!query.exec()) {
        qDebug() << "Error fetching file transfer records:" << query.lastError().text();
        return records;
    }

    while (query.next()) {
        QVector<QString> record;
        record.append(query.value(0).toString()); // sender
        record.append(query.value(1).toString()); // filename
        record.append(query.value(2).toString()); // size
        record.append(query.value(3).toString()); // timestamp
        records.append(record);
    }
    return records;
}

bool DatabaseManager::isOpen() const
{
    return db.isOpen();
}

QList<QStringList> DatabaseManager::getMessageHistory(int limit)
{
    QList<QStringList> history;
    QSqlDatabase db = QSqlDatabase::database(); // 获取默认连接
    if (!db.isOpen()) {
        qDebug() << "Database not open!";
        return history;
    }

    QSqlQuery query;
    query.prepare("SELECT sender, message, timestamp FROM chat_records ORDER BY timestamp DESC LIMIT :limit");
    query.bindValue(":limit", limit);

    qDebug() << "Executing SQL: SELECT sender, message, timestamp FROM chat_records ORDER BY timestamp DESC LIMIT" << limit;

    if (!query.exec()) {
        qDebug() << "Error fetching message history:" << query.lastError().text();
        qDebug() << "Executed query:" << query.executedQuery();
        qDebug() << "Bound values:" << query.boundValues();
        return history;
    }

    while (query.next()) {
        QStringList record;
        record.append(query.value(0).toString()); // sender
        record.append(query.value(1).toString()); // message
        record.append(query.value(2).toString()); // timestamp
        history.append(record);
    }
    return history;
}

QVector<QVector<QString>> DatabaseManager::getChatRecordsByUser(const QString &user, int limit)
{
    QVector<QVector<QString>> records;
    QSqlQuery query;
    query.prepare("SELECT sender, message, timestamp FROM chat_records WHERE sender = :sender ORDER BY timestamp DESC LIMIT :limit");
    query.bindValue(":sender", user);
    query.bindValue(":limit", limit);
    if (query.exec()) {
        while (query.next()) {
            records.append({query.value(0).toString(), query.value(1).toString(), query.value(2).toDateTime().toString(Qt::ISODate)});
        }
    }
    return records;
}

QVector<QVector<QString>> DatabaseManager::getChatRecordsByKeyword(const QString &keyword, int limit)
{
    QVector<QVector<QString>> records;
    QSqlQuery query;
    query.prepare("SELECT sender, message, timestamp FROM chat_records WHERE message LIKE :keyword ORDER BY timestamp DESC LIMIT :limit");
    query.bindValue(":keyword", "%" + keyword + "%");
    query.bindValue(":limit", limit);
    if (query.exec()) {
        while (query.next()) {
            records.append({query.value(0).toString(), query.value(1).toString(), query.value(2).toDateTime().toString(Qt::ISODate)});
        }
    }
    return records;
}
