#include "mainwindow.h"
#include <QDateTime>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), startTime(0), isServerRunning(false)
{
    setupUI();
    setWindowTitle("聊天室服务端");
    setMinimumSize(900, 700);
    setStyleSheet("background-color: #F7FAFC;");

    // 初始化数据库和服务端
    dbManager = new DatabaseManager();
    if (!dbManager->initialize()) {
        logDisplay->append("数据库初始化失败！");
    }

    // 初始化聊天服务器
    server = new Server(dbManager, this);
    connect(server, &Server::logMessage, this, [this](const QString &msg) { logDisplay->append(msg); });

    vserver = new VoiceChatServer(this);

    // 初始化文件传输服务器
    fileServer = new FileTransferServer(dbManager, this);
    connect(fileServer, &FileTransferServer::logMessage, this, [this](const QString &msg) { logDisplay->append(msg); });
    connect(fileServer, &FileTransferServer::broadcastMessage, server, &Server::broadcastMessage);

    // 延迟初始化管理窗口
    userManager = nullptr;
    chatManager = nullptr;
    fileTransferManager = nullptr;

    // 初始化运行时间计时器
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateRuntime);
    timer->start(1000);

    // 设置状态栏
    statusBar = new QStatusBar(this);
    statusBar->setStyleSheet("background-color: #FFFFFF; color: #1A202C; font-size: 14px; padding: 8px; border-top: 1px solid #E2E8F0;");
    statusLabel = new QLabel("状态: 未运行");
    runtimeLabel = new QLabel("运行时长: 00:00:00");
    statusBar->addWidget(statusLabel);
    statusBar->addPermanentWidget(runtimeLabel);
    setStatusBar(statusBar);
}

MainWindow::~MainWindow()
{
    delete fileTransferManager;
    delete chatManager;
    delete userManager;
    delete fileServer; // 删除文件传输服务器
    delete server;
    delete dbManager;
    delete vserver;
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    // 服务端控制区域
    QHBoxLayout *serverLayout = new QHBoxLayout;
    serverLayout->setSpacing(20);

    QLabel *serverLabel = new QLabel("服务端控制");
    serverLabel->setStyleSheet("font-size: 20px; font-weight: 600; color: #2D3748;");

    QLabel *ipLabel = new QLabel("IP:");
    ipLabel->setStyleSheet(
        "QLabel { "
        "  font-size: 15px; "
        "  font-weight: 500; "
        "  color: #4A5568; "
        "  background-color: #EDF2F7; "
        "  padding: 8px 12px; "
        "  border-radius: 8px; "
        "}"
        );

    ipCombo = new QComboBox;
    ipCombo->addItems({"127.0.0.1", "192.168.1.1", "0.0.0.0"});
    ipCombo->setFixedWidth(200);
    ipCombo->setEditable(true);
    ipCombo->setStyleSheet(
        "QComboBox { "
        "  padding: 12px; "
        "  border: none; "
        "  border-radius: 12px; "
        "  background-color: #EDF2F7; "
        "  font-size: 15px; "
        "  color: #2D3748; "
        "} "
        "QComboBox:hover { "
        "  background-color: #E2E8F0; "
        "} "
        "QComboBox::drop-down { "
        "  width: 40px; "
        "  border: none; "
        "  background-color: transparent; "
        "} "
        "QComboBox::down-arrow { "
        "  width: 12px; "
        "  height: 12px; "
        "  image: url(:/icons/down-arrow.png); "
        "} "
        "QComboBox QAbstractItemView { "
        "  background-color: #FFFFFF; "
        "  border: none; "
        "  border-radius: 10px; "
        "  padding: 5px; "
        "  color: #2D3748; "
        "}"
        );

    QLabel *portLabel = new QLabel("端口:");
    portLabel->setStyleSheet(
        "QLabel { "
        "  font-size: 15px; "
        "  font-weight: 500; "
        "  color: #4A5568; "
        "  background-color: #EDF2F7; "
        "  padding: 8px 12px; "
        "  border-radius: 8px; "
        "}"
        );

    portSpin = new QSpinBox;
    portSpin->setRange(1024, 65535);
    portSpin->setValue(8888);
    portSpin->setFixedWidth(120);
    portSpin->setStyleSheet(
        "QSpinBox { "
        "  padding: 12px; "
        "  border: none; "
        "  border-radius: 12px; "
        "  background-color: #EDF2F7; "
        "  font-size: 15px; "
        "  color: #2D3748; "
        "} "
        "QSpinBox:hover { "
        "  background-color: #E2E8F0; "
        "} "
        "QSpinBox::up-button, QSpinBox::down-button { "
        "  width: 25px; "
        "  background-color: transparent; "
        "  border: none; "
        "} "
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover { "
        "  background-color: #CBD5E0; "
        "}"
        );

    toggleButton = new QPushButton("启动服务");
    toggleButton->setFixedSize(140, 50);
    toggleButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #2B6CB0; "
        "  color: white; "
        "  font-size: 16px; "
        "  font-weight: 600; "
        "  border-radius: 10px; "
        "  padding: 12px; "
        "  border: none; "
        "} "
        "QPushButton:hover { "
        "  background-color: #2C5282; "
        "} "
        "QPushButton:disabled { "
        "  background-color: #A0AEC0; "
        "}"
        );

    serverLayout->addWidget(serverLabel);
    serverLayout->addStretch();
    serverLayout->addWidget(ipLabel);
    serverLayout->addWidget(ipCombo);
    serverLayout->addWidget(portLabel);
    serverLayout->addWidget(portSpin);
    serverLayout->addWidget(toggleButton);
    mainLayout->addLayout(serverLayout);

    // 日志显示区域
    QLabel *logLabel = new QLabel("运行日志");
    logLabel->setStyleSheet("font-size: 20px; font-weight: 600; color: #2D3748;");
    mainLayout->addWidget(logLabel);

    logDisplay = new QTextEdit;
    logDisplay->setReadOnly(true);
    logDisplay->setStyleSheet(
        "QTextEdit { "
        "  background-color: #FFFFFF; "
        "  border: 1px solid #E2E8F0; "
        "  border-radius: 10px; "
        "  padding: 15px; "
        "  font-family: 'Courier New'; "
        "  font-size: 14px; "
        "  color: #2D3748; "
        "}"
        );
    mainLayout->addWidget(logDisplay, 1);

    // 管理功能区域
    QLabel *manageLabel = new QLabel("管理功能");
    manageLabel->setStyleSheet("font-size: 20px; font-weight: 600; color: #2D3748;");
    mainLayout->addWidget(manageLabel);

    QHBoxLayout *manageLayout = new QHBoxLayout;
    manageLayout->setSpacing(20);

    clearLogButton = new QPushButton("清理日志");
    clearLogButton->setFixedSize(140, 50);
    clearLogButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #E53E3E; "
        "  color: white; "
        "  font-size: 16px; "
        "  font-weight: 600; "
        "  border-radius: 10px; "
        "  padding: 12px; "
        "  border: none; "
        "} "
        "QPushButton:hover { "
        "  background-color: #C53030; "
        "}"
        );

    userManageButton = new QPushButton("管理用户");
    userManageButton->setFixedSize(140, 50);
    userManageButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #805AD5; "
        "  color: white; "
        "  font-size: 16px; "
        "  font-weight: 600; "
        "  border-radius: 10px; "
        "  padding: 12px; "
        "  border: none; "
        "} "
        "QPushButton:hover { "
        "  background-color: #6B46C1; "
        "}"
        );

    chatManageButton = new QPushButton("管理聊天记录");
    chatManageButton->setFixedSize(140, 50);
    chatManageButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #D69E2E; "
        "  color: white; "
        "  font-size: 16px; "
        "  font-weight: 600; "
        "  border-radius: 10px; "
        "  padding: 12px; "
        "  border: none; "
        "} "
        "QPushButton:hover { "
        "  background-color: #B7791F; "
        "}"
        );

    fileManageButton = new QPushButton("管理文件传输");
    fileManageButton->setFixedSize(140, 50);
    fileManageButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #38A169; "
        "  color: white; "
        "  font-size: 16px; "
        "  font-weight: 600; "
        "  border-radius: 10px; "
        "  padding: 12px; "
        "  border: none; "
        "} "
        "QPushButton:hover { "
        "  background-color: #2F855A; "
        "}"
        );

    manageLayout->addWidget(clearLogButton);
    manageLayout->addWidget(userManageButton);
    manageLayout->addWidget(chatManageButton);
    manageLayout->addWidget(fileManageButton);
    manageLayout->addStretch();
    mainLayout->addLayout(manageLayout);

    // 连接信号和槽
    connect(toggleButton, &QPushButton::clicked, this, &MainWindow::toggleServer);
    connect(clearLogButton, &QPushButton::clicked, this, &MainWindow::clearLogs);
    connect(userManageButton, &QPushButton::clicked, this, &MainWindow::manageUsers);
    connect(chatManageButton, &QPushButton::clicked, this, &MainWindow::manageChatHistory);
    connect(fileManageButton, &QPushButton::clicked, this, &MainWindow::manageFileTransfer);
}

void MainWindow::toggleServer()
{
    if (!isServerRunning) {
        QString ip = ipCombo->currentText();
        int port = portSpin->value();
        if (server->startServer(ip, port)) {
            vserver->startServer(11451);
            logDisplay->append("语音聊天服务已打开，端口11451");
            if (fileServer->startServer(ip, 12345)) { // 同时启动文件传输服务器
                isServerRunning = true;
                updateButtonState(true);
                statusLabel->setText("状态: 运行中");
                statusLabel->setStyleSheet("color: #38A169;");
                startTime = QDateTime::currentMSecsSinceEpoch();
            } else {
                server->stopServer(); // 如果文件服务器启动失败，回滚聊天服务器
                logDisplay->append("文件传输服务启动失败，无法启动服务");
            }
        }
    } else {
        if (QMessageBox::question(this, "确认停止", "确定要停止服务吗？所有客户端将被断开连接。",
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            server->stopServer();
            fileServer->stopServer(); // 同时停止文件传输服务器
            vserver->stopServer();
            isServerRunning = false;
            updateButtonState(false);
            statusLabel->setText("状态: 未运行");
            statusLabel->setStyleSheet("color: #1A202C;");
            startTime = 0;
            runtimeLabel->setText("运行时长: 00:00:00");
        }
    }
}


void MainWindow::updateButtonState(bool running)
{
    if (running) {
        toggleButton->setText("停止服务");
        toggleButton->setStyleSheet(
            "QPushButton { "
            "  background-color: #E53E3E; "
            "  color: white; "
            "  font-size: 16px; "
            "  font-weight: 600; "
            "  border-radius: 10px; "
            "  padding: 12px; "
            "  border: none; "
            "} "
            "QPushButton:hover { "
            "  background-color: #C53030; "
            "}"
            );
    } else {
        toggleButton->setText("启动服务");
        toggleButton->setStyleSheet(
            "QPushButton { "
            "  background-color: #2B6CB0; "
            "  color: white; "
            "  font-size: 16px; "
            "  font-weight: 600; "
            "  border-radius: 10px; "
            "  padding: 12px; "
            "  border: none; "
            "} "
            "QPushButton:hover { "
            "  background-color: #2C5282; "
            "} "
            "QPushButton:disabled { "
            "  background-color: #A0AEC0; "
            "}"
            );
    }
}

void MainWindow::clearLogs()
{
    logDisplay->clear();
    logDisplay->append("日志已清理完成");
}

void MainWindow::manageUsers()
{
    if (!userManager) {
        userManager = new UserManager(dbManager, server, this);
    }
    userManager->show();
    userManager->raise();
    userManager->activateWindow();
    logDisplay->append("用户管理界面已打开");
}

void MainWindow::manageChatHistory()
{
    if (!chatManager) {
        chatManager = new ChatManager(dbManager, this);
    }
    chatManager->show();
    chatManager->raise();
    chatManager->activateWindow();
    logDisplay->append("聊天记录管理界面已打开");
}

void MainWindow::manageFileTransfer()
{
    if (!fileTransferManager) {
        fileTransferManager = new FileTransferManager(dbManager, this);
    }
    fileTransferManager->show();
    fileTransferManager->raise();
    fileTransferManager->activateWindow();
    logDisplay->append("文件传输管理界面已打开");
}

void MainWindow::updateRuntime()
{
    if (isServerRunning && startTime > 0) {
        qint64 elapsed = (QDateTime::currentMSecsSinceEpoch() - startTime) / 1000;
        int hours = elapsed / 3600;
        int minutes = (elapsed % 3600) / 60;
        int seconds = elapsed % 60;
        runtimeLabel->setText(QString("运行时长: %1:%2:%3")
                                  .arg(hours, 2, 10, QChar('0'))
                                  .arg(minutes, 2, 10, QChar('0'))
                                  .arg(seconds, 2, 10, QChar('0')));
    }
}
