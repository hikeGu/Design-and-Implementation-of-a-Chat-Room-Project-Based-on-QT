#include "filetransfer.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QDateTime>
#include <QHBoxLayout>

FileTransfer::FileTransfer(Client *client, QWidget *parent)
    : QWidget(parent), client(client)
{
    setupUI();
    applyStyles();
    setWindowTitle("文件传输窗口");
    resize(700, 500); // 稍微增大窗口以适应美化布局

    // 连接 Client 信号
    connect(client, &Client::messageFL, this, &FileTransfer::handleFileList);
    connect(client, &Client::messageReceived, this, &FileTransfer::displayStatus);
}

FileTransfer::~FileTransfer()
{
    disconnect(client, &Client::messageFL, this, &FileTransfer::handleFileList);
    disconnect(client, &Client::messageReceived, this, &FileTransfer::displayStatus);
}

void FileTransfer::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15); // 添加外边距
    mainLayout->setSpacing(10);

    // 文件表格
    fileTable = new QTableWidget(0, 4, this);
    fileTable->setHorizontalHeaderLabels({"发送者", "文件名", "大小 (bytes)", "上传时间"});
    fileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileTable->verticalHeader()->setVisible(false); // 隐藏行号
    mainLayout->addWidget(fileTable);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    refreshButton = new QPushButton("刷新文件列表");
    uploadButton = new QPushButton("上传文件");
    downloadButton = new QPushButton("下载文件");

    buttonLayout->addStretch(); // 按钮居中
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(uploadButton);
    buttonLayout->addWidget(downloadButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    // 连接按钮信号
    connect(refreshButton, &QPushButton::clicked, this, &FileTransfer::refreshFileList);
    connect(uploadButton, &QPushButton::clicked, this, &FileTransfer::uploadFile);
    connect(downloadButton, &QPushButton::clicked, this, &FileTransfer::downloadFile);
    refreshFileList();
}

void FileTransfer::applyStyles()
{
    setStyleSheet(R"(
        /* 窗口整体样式 */
        QWidget {
            background-color: #F7FAFC;
            font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;
            font-size: 14px;
        }

        /* 表格样式 */
        QTableWidget {
            padding: 10px;
            border: none;
            border-radius: 8px;
            background-color: #EDF2F7;
            color: #2D3748;
        }
        QTableWidget::item {
            padding: 8px;
            color: #2D3748;
        }
        QTableWidget::item:selected {
            background-color: #E2E8F0;
            color: #2D3748;
        }
        QHeaderView::section {
            background-color: #E2E8F0;
            color: #2D3748;
            padding: 10px;
            border: none;
            font-weight: 600;
            font-size: 14px;
        }

        /* 按钮样式 */
        QPushButton {
            background-color: #38A169;
            color: white;
            font-size: 14px;
            font-weight: 600;
            border-radius: 8px;
            padding: 10px;
            border: none;
            min-width: 100px;
        }
        QPushButton:hover {
            background-color: #2F855A;
        }
        QPushButton:pressed {
            background-color: #276749;
        }
        QPushButton:disabled {
            background-color: #A0AEC0;
            color: white;
        }
    )");
}

void FileTransfer::refreshFileList()
{
    if (!client->isConnected()) {
        QMessageBox::warning(this, "错误", "未连接到服务器！");
        return;
    }
    client->requestFileList();
}

void FileTransfer::uploadFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件", "", "All Files (*.*)");
    if (filePath.isEmpty()) return;

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "错误", "文件不存在！");
        return;
    }

    client->sendFile(filePath, fileInfo.fileName(), fileInfo.size());
}

void FileTransfer::downloadFile()
{
    int row = fileTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "错误", "请选择要下载的文件！");
        return;
    }

    QString filename = fileTable->item(row, 1)->text();
    QString savePath = QFileDialog::getSaveFileName(this, "保存文件", filename, "All Files (*.*)");
    if (savePath.isEmpty()) return;

    client->requestFile(filename, savePath);
}

void FileTransfer::handleFileList(const QString &message)
{
    fileTable->clearContents();
    fileTable->setRowCount(0);

    if (message.isEmpty()) return;

    QStringList entries = message.split(",", Qt::SkipEmptyParts);
    for (const QString &entry : entries) {
        QStringList parts = entry.split("|", Qt::SkipEmptyParts);
        if (parts.size() >= 4) {
            int row = fileTable->rowCount();
            fileTable->insertRow(row);
            fileTable->setItem(row, 0, new QTableWidgetItem(parts[0])); // 发送者
            fileTable->setItem(row, 1, new QTableWidgetItem(parts[1])); // 文件名
            fileTable->setItem(row, 2, new QTableWidgetItem(parts[2])); // 大小
            fileTable->setItem(row, 3, new QTableWidgetItem(parts[3])); // 上传时间
        }
    }
}


void FileTransfer::displayStatus(const QString &message)
{
    if (message.contains("文件上传完成") || message.contains("文件下载完成") ||
        message.contains("失败") || message.contains("连接文件传输服务器")) {
        QMessageBox::information(this, "文件传输状态", message);
    }
}
