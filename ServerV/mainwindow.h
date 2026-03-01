#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include "databasemanager.h"
#include "server.h"
#include "usermanager.h"
#include "chatmanager.h"
#include "filetransfermanager.h"
#include "filetransferserver.h"
#include "voicechatserver.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleServer();
    void clearLogs();
    void manageUsers();
    void manageChatHistory();
    void manageFileTransfer();
    void updateRuntime();

private:
    QComboBox *ipCombo;
    QSpinBox *portSpin;
    QPushButton *toggleButton;
    QPushButton *clearLogButton;
    QPushButton *userManageButton;
    QPushButton *chatManageButton;
    QPushButton *fileManageButton;
    QTextEdit *logDisplay;
    QLabel *statusLabel;
    QLabel *runtimeLabel;
    QStatusBar *statusBar;
    QTimer *timer;
    qint64 startTime;
    bool isServerRunning;

    DatabaseManager *dbManager;
    Server *server;
    UserManager *userManager;
    ChatManager *chatManager;
    FileTransferManager *fileTransferManager;
    FileTransferServer *fileServer; // 新增文件传输服务器
    VoiceChatServer *vserver;

    void setupUI();
    void updateButtonState(bool running);
};

#endif // MAINWINDOW_H
