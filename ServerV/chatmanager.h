#ifndef CHATMANAGER_H
#define CHATMANAGER_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include "databasemanager.h"

class ChatManager : public QDialog
{
    Q_OBJECT

public:
    ChatManager(DatabaseManager *db, QWidget *parent = nullptr);
    ~ChatManager();

private:
    void setupUI();
    void populateChatTable(const QString &userFilter = "", const QString &keywordFilter = "");

    DatabaseManager *dbManager;
    QTableWidget *chatTable;
    QPushButton *refreshButton;
    QPushButton *clearButton;
    QPushButton *deleteButton;
    QLineEdit *userSearchEdit;
    QPushButton *searchUserButton;
    QLineEdit *keywordSearchEdit;
    QPushButton *searchKeywordButton;
    QPushButton *showAllButton;

private slots:
    void refreshChatRecords();
    void clearChatRecords();
    void deleteChatRecord();
    void searchByUser();
    void searchByKeyword();
    void showAllRecords();
    void showMessageDetails(int row, int column); // 新增：显示消息详情
};

#endif // CHATMANAGER_H
