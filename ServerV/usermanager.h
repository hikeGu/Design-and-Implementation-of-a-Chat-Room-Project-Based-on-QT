#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include "databasemanager.h"
#include "server.h"

class UserManager : public QDialog
{
    Q_OBJECT

public:
    UserManager(DatabaseManager *db, Server *server, QWidget *parent = nullptr);
    ~UserManager();

private slots:
    void addUser();
    void removeUser();
    void updatePassword();
    void refreshUserList();
    void updateUserStatus(const QString &username, bool isOnline); // 新增槽函数

private:
    void setupUI();
    void populateUserTable();

    DatabaseManager *dbManager;
    Server *server; // 您的自定义 Server 类
    QTableWidget *userTable;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *updatePasswordButton;
    QPushButton *refreshButton;
};

#endif // USERMANAGER_H
