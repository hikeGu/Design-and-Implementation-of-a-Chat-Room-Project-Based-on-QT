#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer> // Added for timeout handling
#include "client.h"
#include "filetransfer.h"
#include "historywindow.h"
#include "voicechatclient.h"
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void onVoiceChatButtonClicked();
private:
    void setupUI();
    Client *client;
    QComboBox *ipCombo;
    QSpinBox *portSpin;
    QPushButton *connectButton;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;
    QPushButton *voiceButton;
    QTextEdit *chatDisplay;
    QLineEdit *messageEdit;
    QPushButton *sendButton;
    QPushButton *fileButton;
    QPushButton *historyButton;
    bool isLoggedIn = false; // Track login state
    QTimer *connectionTimer; // Timer for connection timeout
    QTimer *disconnectionTimer; // 新增断开超时计时器
    void onDisconnectionTimeout(); // 新增断开超时处理
    FileTransfer *Fwindow;
    HistoryWindow *Hwindow;
    VoiceChatClient *vclient;
    bool isInVoiceChat = false; // 语音聊天状态标志

private slots:
    void toggleConnection();
    void login();
    void registerUser();
    void sendMessage();
    void sendFile();
    void updateChatDisplay(const QString &message);
    void updateConnectionState(bool connected);
    void showHistoryWindow();
    void onConnectionTimeout(); // New slot for handling timeout

protected:
    void keyPressEvent(QKeyEvent *event) override; // 添加这一行
};

#endif // MAINWINDOW_H
