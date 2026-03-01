#ifndef HISTORYWINDOW_H
#define HISTORYWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include "client.h" // 假设您的 Client 类定义在 client.h 中

class HistoryWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit HistoryWindow(Client *client, QWidget *parent = nullptr);
    ~HistoryWindow();

private:
    void setupUI();
    void requestHistory(); // 向服务端请求历史消息

    Client *client; // Client 实例，用于发送请求和接收消息
    QTextEdit *historyDisplay; // 显示历史消息的区域
    QLineEdit *userFilterEdit; // 按用户查找的输入框
    QLineEdit *keywordFilterEdit; // 按关键字查找的输入框
    QPushButton *userFilterButton; // 按用户查找按钮
    QPushButton *keywordFilterButton; // 按关键字查找按钮
    QPushButton *showAllButton; // 显示所有信息按钮
    QStringList historyMessages; // 存储所有历史消息，便于过滤

private slots:
    void appendMessage(const QString &message); // 处理接收到的消息
    void filterByUser(); // 按用户过滤
    void filterByKeyword(); // 按关键字过滤
    void showAllMessages(); // 显示所有消息
};

#endif // HISTORYWINDOW_H
