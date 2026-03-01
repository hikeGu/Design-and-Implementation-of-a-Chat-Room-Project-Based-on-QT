#include "mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include "filetransfer.h"
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include "historywindow.h"
#include <QDebug>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    setWindowTitle("聊天室客户端");
    setMinimumSize(800, 600);
    setStyleSheet("background-color: #F7FAFC;");

    client = new Client(this);
    vclient = new VoiceChatClient(this);
    connectionTimer = new QTimer(this);
    connectionTimer->setSingleShot(true);
    disconnectionTimer = new QTimer(this);
    disconnectionTimer->setSingleShot(true);

    // 连接信号和槽
    connect(client, &Client::messageReceived, this, &MainWindow::updateChatDisplay);
    connect(client, &Client::connectionChanged, this, &MainWindow::updateConnectionState);
    connect(connectionTimer, &QTimer::timeout, this, &MainWindow::onConnectionTimeout);
    connect(disconnectionTimer, &QTimer::timeout, this, &MainWindow::onDisconnectionTimeout); // 修复：添加断开超时信号连接
    connect(voiceButton, &QPushButton::clicked, this, &MainWindow::onVoiceChatButtonClicked);

    Fwindow = nullptr;
    Hwindow = nullptr;
}

MainWindow::~MainWindow()
{
    delete client;
    delete connectionTimer;
    delete disconnectionTimer; // 修复：确保删除 disconnectionTimer
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Connection area
    QHBoxLayout *connectLayout = new QHBoxLayout;
    connectLayout->setSpacing(15);

    QLabel *connectLabel = new QLabel("连接服务端");
    connectLabel->setStyleSheet("font-size: 18px; font-weight: 600; color: #2D3748;");

    ipCombo = new QComboBox;
    ipCombo->addItems({"127.0.0.1", "192.168.1.1"});
    ipCombo->setFixedWidth(180);
    ipCombo->setEditable(true);
    ipCombo->setStyleSheet(
        "QComboBox { padding: 10px; border: none; border-radius: 10px; background-color: #EDF2F7; font-size: 14px; color: #2D3748; } "
        "QComboBox:hover { background-color: #E2E8F0; } "
        "QComboBox::drop-down { width: 30px; border: none; } "
        "QComboBox QAbstractItemView { background-color: #FFFFFF; border-radius: 8px; padding: 5px; }");

    portSpin = new QSpinBox;
    portSpin->setRange(1024, 65535);
    portSpin->setValue(8888);
    portSpin->setFixedWidth(100);
    portSpin->setStyleSheet(
        "QSpinBox { padding: 10px; border: none; border-radius: 10px; background-color: #EDF2F7; font-size: 14px; color: #2D3748; } "
        "QSpinBox:hover { background-color: #E2E8F0; } "
        "QSpinBox::up-button, QSpinBox::down-button { width: 20px; background-color: transparent; } "
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover { background-color: #CBD5E0; }");

    connectButton = new QPushButton("连接");
    connectButton->setFixedSize(100, 40);
    connectButton->setStyleSheet(
        "QPushButton { background-color: #2B6CB0; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
        "QPushButton:hover { background-color: #2C5282; } "
        "QPushButton:disabled { background-color: #A0AEC0; }");

    connectLayout->addWidget(connectLabel);
    connectLayout->addStretch();
    connectLayout->addWidget(new QLabel("IP:"));
    connectLayout->addWidget(ipCombo);
    connectLayout->addWidget(new QLabel("端口:"));
    connectLayout->addWidget(portSpin);
    connectLayout->addWidget(connectButton);
    mainLayout->addLayout(connectLayout);

    // Login/Register area
    QHBoxLayout *authLayout = new QHBoxLayout;
    authLayout->setSpacing(15);

    usernameEdit = new QLineEdit;
    usernameEdit->setPlaceholderText("用户名");
    usernameEdit->setFixedWidth(150);
    usernameEdit->setStyleSheet(
        "QLineEdit { padding: 10px; border: none; border-radius: 8px; background-color: #EDF2F7; font-size: 14px; color: #2D3748; } "
        "QLineEdit:hover { background-color: #E2E8F0; }");

    passwordEdit = new QLineEdit;
    passwordEdit->setPlaceholderText("密码");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setFixedWidth(150);
    passwordEdit->setStyleSheet(
        "QLineEdit { padding: 10px; border: none; border-radius: 8px; background-color: #EDF2F7; font-size: 14px; color: #2D3748; } "
        "QLineEdit:hover { background-color: #E2E8F0; }");

    loginButton = new QPushButton("登录");
    loginButton->setFixedSize(100, 40);
    loginButton->setEnabled(false);
    loginButton->setStyleSheet(
        "QPushButton { background-color: #38A169; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
        "QPushButton:hover { background-color: #2F855A; } "
        "QPushButton:disabled { background-color: #A0AEC0; }");

    registerButton = new QPushButton("注册");
    registerButton->setFixedSize(100, 40);
    registerButton->setEnabled(false);
    registerButton->setStyleSheet(
        "QPushButton { background-color: #805AD5; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
        "QPushButton:hover { background-color: #6B46C1; } "
        "QPushButton:disabled { background-color: #A0AEC0; }");

    authLayout->addWidget(new QLabel("用户名:"));
    authLayout->addWidget(usernameEdit);
    authLayout->addWidget(new QLabel("密码:"));
    authLayout->addWidget(passwordEdit);
    authLayout->addWidget(loginButton);
    authLayout->addWidget(registerButton);
    authLayout->addStretch();
    mainLayout->addLayout(authLayout);

    // Chat display area
    QLabel *chatLabel = new QLabel("聊天窗口");
    chatLabel->setStyleSheet("font-size: 18px; font-weight: 600; color: #2D3748;");
    mainLayout->addWidget(chatLabel);

    chatDisplay = new QTextEdit;
    chatDisplay->setReadOnly(true);
    chatDisplay->setStyleSheet(
        "QTextEdit { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px; padding: 15px; font-family: 'Courier New'; font-size: 14px; color: #2D3748; }");
    mainLayout->addWidget(chatDisplay, 1);

    // Message sending and file operation area
    QHBoxLayout *sendLayout = new QHBoxLayout;
    sendLayout->setSpacing(15);

    messageEdit = new QLineEdit;
    messageEdit->setPlaceholderText("输入消息（私聊格式：@用户名 消息 | 在线AI格式： @AI 信息)");
    messageEdit->setStyleSheet(
        "QLineEdit { padding: 10px; border: none; border-radius: 8px; background-color: #EDF2F7; font-size: 14px; color: #2D3748; } "
        "QLineEdit:hover { background-color: #E2E8F0; }");

    sendButton = new QPushButton("发送");
    sendButton->setFixedSize(100, 40);
    sendButton->setEnabled(false);
    sendButton->setStyleSheet(
        "QPushButton { background-color: #D69E2E; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
        "QPushButton:hover { background-color: #B7791F; } "
        "QPushButton:disabled { background-color: #A0AEC0; }");

    voiceButton = new QPushButton("加入语音聊天");
    voiceButton->setFixedSize(200, 40);
    voiceButton->setEnabled(false);
    voiceButton->setStyleSheet(
        "QPushButton { background-color: #2B6CB0; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
        "QPushButton:hover { background-color: #2C5282; } "
        "QPushButton:disabled { background-color: #A0AEC0; }");


    fileButton = new QPushButton("文件管理");
    fileButton->setFixedSize(100, 40);
    fileButton->setEnabled(false);
    fileButton->setStyleSheet(
        "QPushButton { background-color: #38A169; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
        "QPushButton:hover { background-color: #2F855A; } "
        "QPushButton:disabled { background-color: #A0AEC0; }");

    historyButton = new QPushButton("查看历史");
    historyButton->setFixedSize(100, 40);
    historyButton->setEnabled(false);
    historyButton->setStyleSheet(
        "QPushButton { background-color: #4A5568; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
        "QPushButton:hover { background-color: #2D3748; } "
        "QPushButton:disabled { background-color: #A0AEC0; }");

    sendLayout->addWidget(voiceButton);
    sendLayout->addWidget(fileButton);
    sendLayout->addWidget(historyButton);
    mainLayout->addLayout(sendLayout);

    QHBoxLayout *sendMessageLayout = new QHBoxLayout;
    sendLayout->setSpacing(15);
    sendMessageLayout->addWidget(messageEdit, 1);
    sendMessageLayout->addWidget(sendButton);
    sendLayout->addStretch();
    mainLayout->addLayout(sendMessageLayout);

    // Connect signals and slots
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::login);
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::registerUser);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(fileButton, &QPushButton::clicked, this, &MainWindow::sendFile);
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::showHistoryWindow);
}

void MainWindow::toggleConnection()
{
    if (client->isConnected()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "断开连接", "确定要断开与服务端的连接吗？",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            chatDisplay->append("正在断开连接...");
            connectButton->setEnabled(false);
            connectButton->setText("断开中...");

            // 启动断开超时定时器
            disconnectionTimer->start(3000);// 3秒断开超时
            // 调用断开函数
            client->disconnectFromServer();
        }
    } else {
        QString ip = ipCombo->currentText().trimmed();
        int port = portSpin->value();

        if (ip.isEmpty()) {
            QMessageBox::warning(this, "输入错误", "IP 地址不能为空！");
            return;
        }

        chatDisplay->append(QString("正在连接到 %1:%2...").arg(ip).arg(port));
        connectButton->setEnabled(false);
        connectButton->setText("连接中...");
        // 启动连接超时定时器
        connectionTimer->start(3000); // 3秒连接超时
        client->connectToServer(ip, port);
    }
}

void MainWindow::login()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text().trimmed();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "用户名和密码不能为空！");
        return;
    }
    loginButton->setEnabled(false);
    registerButton->setEnabled(false);
    chatDisplay->append(QString("正在登录 %1...").arg(username));
    client->sendLogin(username, password);
}

void MainWindow::registerUser()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text().trimmed();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "用户名和密码不能为空！");
        return;
    }
    registerButton->setEnabled(false);
    loginButton->setEnabled(false);
    chatDisplay->append(QString("正在注册 %1...").arg(username));
    client->sendRegister(username, password);
}

void MainWindow::sendMessage()
{
    QString message = messageEdit->text().trimmed();
    if (message.isEmpty()) return;

    if (message.startsWith("@")) {
        QStringList parts = message.split(" ", Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            QString receiver = parts[0].mid(1);
            QString content = parts.mid(1).join(" ");
            client->sendPrivateMessage(receiver, content);
            if(receiver == "AI"){
                chatDisplay->append(QString("我: %1").arg(message));
            }
            else{
                chatDisplay->append(QString("私聊 [%1]: %2").arg(receiver, content));
            }
            messageEdit->clear();
            return;
        } else {
            QMessageBox::warning(this, "格式错误", "私聊格式应为：@用户名 消息");
            return;
        }
    }
    client->sendMessage(message);
    chatDisplay->append(QString("我: %1").arg(message));
    messageEdit->clear();
}

void MainWindow::sendFile()
{
    if (!Fwindow) {
        Fwindow = new FileTransfer(client); // 使用成员变量 window
    }
    Fwindow->show();
    Fwindow->raise();
    Fwindow->activateWindow();
}

void MainWindow::updateChatDisplay(const QString &message)
{
    chatDisplay->append(message);

    if (message.contains("登录成功")) {
        isLoggedIn = true;
        sendButton->setEnabled(true);
        voiceButton->setEnabled(true);
        fileButton->setEnabled(true);
        historyButton->setEnabled(true);
        loginButton->setEnabled(false);
        registerButton->setEnabled(false);
    } else if (message.contains("登录失败")) {
        isLoggedIn = false;
        loginButton->setEnabled(true);
        registerButton->setEnabled(true);
    } else if (message.contains("注册成功")) {
        registerButton->setEnabled(false);
        loginButton->setEnabled(true);
        chatDisplay->append("请使用新注册的账户登录");
    } else if (message.contains("注册失败")) {
        registerButton->setEnabled(true);
        loginButton->setEnabled(true);
    }
}

void MainWindow::updateConnectionState(bool connected)
{
    // 停止所有定时器
    connectionTimer->stop();
    disconnectionTimer->stop();

    // 恢复按钮状态
    connectButton->setEnabled(true);

    qDebug() << "Connection state changed: " << connected;

    if (connected) {
        // 更新为已连接状态
        connectButton->setText("断开");
        connectButton->setStyleSheet(
            "QPushButton { background-color: #E53E3E; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
            "QPushButton:hover { background-color: #C53030; } "
            "QPushButton:disabled { background-color: #A0AEC0; }");
        chatDisplay->append("连接成功！");
        loginButton->setEnabled(true);
        registerButton->setEnabled(true);
        voiceButton->setEnabled(isLoggedIn);
        sendButton->setEnabled(isLoggedIn); // 仅当登录成功时启用发送按钮
        fileButton->setEnabled(isLoggedIn);
        historyButton->setEnabled(isLoggedIn);
    } else {
        // 更新为断开状态
        connectButton->setText("连接");
        connectButton->setStyleSheet(
            "QPushButton { background-color: #2B6CB0; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
            "QPushButton:hover { background-color: #2C5282; } "
            "QPushButton:disabled { background-color: #A0AEC0; }");
        chatDisplay->append("与服务端断开连接");

        // 禁用所有功能按钮
        loginButton->setEnabled(false);
        registerButton->setEnabled(false);
        voiceButton->setEnabled(false);
        sendButton->setEnabled(false);
        fileButton->setEnabled(false);
        historyButton->setEnabled(false);

        isLoggedIn = false; // 重置登录状态
    }
}

void MainWindow::showHistoryWindow()
{
    if (!isLoggedIn) {
        QMessageBox::warning(this, "未登录", "请先登录以查看历史消息！");
        return;
    }
    if (!Hwindow) {
        Hwindow = new HistoryWindow(client); // 使用成员变量 window
    }
    Hwindow->show();
    Hwindow->raise();
    Hwindow->activateWindow();
}

void MainWindow::onConnectionTimeout()
{
    if (!client->isConnected()) {
        client->disconnectFromServer();
        connectButton->setEnabled(true);
        connectButton->setText("连接");
        connectButton->setStyleSheet(
            "QPushButton { background-color: #2B6CB0; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
            "QPushButton:hover { background-color: #2C5282; } "
            "QPushButton:disabled { background-color: #A0AEC0; }");
        chatDisplay->append("连接失败：服务端未响应");
        loginButton->setEnabled(false);
        registerButton->setEnabled(false);
        voiceButton->setEnabled(false);
        sendButton->setEnabled(false);
        fileButton->setEnabled(false);
        historyButton->setEnabled(false);
        isLoggedIn = false;
    }
}

void MainWindow::onDisconnectionTimeout()
{
    if (client->isConnected()) {
        qDebug() << "Disconnection timeout, but still connected!";
        return;
    }

    connectButton->setEnabled(true);
    connectButton->setText("连接");
    connectButton->setStyleSheet(
        "QPushButton { background-color: #2B6CB0; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
        "QPushButton:hover { background-color: #2C5282; } "
        "QPushButton:disabled { background-color: #A0AEC0; }");
    chatDisplay->append("断开连接超时，已强制断开");
    loginButton->setEnabled(false);
    registerButton->setEnabled(false);
    voiceButton->setEnabled(false);
    sendButton->setEnabled(false);
    fileButton->setEnabled(false);
    historyButton->setEnabled(false);
    isLoggedIn = false;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    // 检查是否按下回车键
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 手动触发发送按钮的点击事件
        sendButton->click();
    }
    // 调用基类的处理方法
    QMainWindow::keyPressEvent(event);
}

void MainWindow::onVoiceChatButtonClicked()
{

    if (!isInVoiceChat) {
        vclient->connectToServer(QHostAddress(ipCombo->currentText().trimmed()), 11451); // 连接到服务器
        isInVoiceChat = true;
        voiceButton->setText("断开语音");
        voiceButton->setStyleSheet("QPushButton { background-color: #E53E3E; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
                                   "QPushButton:hover { background-color: #C53030; } "
                                   "QPushButton:disabled { background-color: #A0AEC0; }"); // 红色表示正在语音
    } else {
        // 退出语音聊天
        vclient->disconnectFromServer();
        isInVoiceChat = false;
        voiceButton->setText("加入语音");
        voiceButton->setStyleSheet("QPushButton { background-color: #2B6CB0; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; } "
                                   "QPushButton:hover { background-color: #2C5282; } "
                                   "QPushButton:disabled { background-color: #A0AEC0; }"); // 恢复默认样式
    }
}
