#include "filetransfermanager.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>

FileTransferManager::FileTransferManager(DatabaseManager *db, QWidget *parent)
    : QDialog(parent), dbManager(db)
{
    setWindowTitle("文件传输管理");
    setFixedSize(800, 500);
    setStyleSheet("background-color: #F7FAFC;");

    QDir dir;
    if (!dir.exists(fileStoragePath)) {
        dir.mkpath(fileStoragePath);
    }
    setupUI();
    populateFileTable();
}

FileTransferManager::~FileTransferManager()
{
}

void FileTransferManager::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 标题
    QLabel *titleLabel = new QLabel("文件传输管理");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: 600; color: #2D3748;");
    mainLayout->addWidget(titleLabel);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(15);

    uploadButton = new QPushButton("上传文件");
    uploadButton->setFixedSize(120, 40);
    uploadButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #38A169; "
        "  color: white; "
        "  font-size: 14px; "
        "  font-weight: 600; "
        "  border-radius: 8px; "
        "  padding: 10px; "
        "  border: none; "
        "} "
        "QPushButton:hover { "
        "  background-color: #2F855A; "
        "}"
        );

    deleteButton = new QPushButton("删除文件与记录");
    deleteButton->setFixedSize(120, 40);
    deleteButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #E53E3E; "
        "  color: white; "
        "  font-size: 14px; "
        "  font-weight: 600; "
        "  border-radius: 8px; "
        "  padding: 10px; "
        "  border: none; "
        "} "
        "QPushButton:hover { "
        "  background-color: #C53030; "
        "}"
        );

    // downloadButton = new QPushButton("下载文件");
    // downloadButton->setFixedSize(120, 40);
    // downloadButton->setStyleSheet(
    //     "QPushButton { "
    //     "  background-color: #2B6CB0; "
    //     "  color: white; "
    //     "  font-size: 14px; "
    //     "  font-weight: 600; "
    //     "  border-radius: 8px; "
    //     "  padding: 10px; "
    //     "  border: none; "
    //     "} "
    //     "QPushButton:hover { "
    //     "  background-color: #2C5282; "
    //     "}"
    //     );

    refreshButton = new QPushButton("刷新记录");
    refreshButton->setFixedSize(120, 40);
    refreshButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #805AD5; "
        "  color: white; "
        "  font-size: 14px; "
        "  font-weight: 600; "
        "  border-radius: 8px; "
        "  padding: 10px; "
        "  border: none; "
        "} "
        "QPushButton:hover { "
        "  background-color: #6B46C1; "
        "}"
        );

    buttonLayout->addWidget(uploadButton);
    buttonLayout->addWidget(deleteButton);
    // buttonLayout->addWidget(downloadButton);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // 文件传输记录表格
    fileTable = new QTableWidget(this);
    fileTable->setColumnCount(4);
    fileTable->setHorizontalHeaderLabels({"发送者", "文件名", "文件大小 (bytes)", "上传时间"});
    fileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fileTable->setStyleSheet(
        "QTableWidget { "
        "  background-color: #FFFFFF; "
        "  border: 1px solid #E2E8F0; "
        "  border-radius: 10px; "
        "  padding: 5px; "
        "  font-size: 14px; "
        "  color: #2D3748; "
        "} "
        "QTableWidget::item { "
        "  padding: 8px; "
        "} "
        "QHeaderView::section { "
        "  background-color: #EDF2F7; "
        "  padding: 10px; "
        "  border: none; "
        "  font-weight: 600; "
        "  color: #4A5568; "
        "}"
        );
    fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止编辑
    fileTable->setSelectionBehavior(QAbstractItemView::SelectRows); // 整行选择
    fileTable->setSelectionMode(QAbstractItemView::SingleSelection); // 单选
    mainLayout->addWidget(fileTable);

    // 连接信号和槽
    connect(uploadButton, &QPushButton::clicked, this, &FileTransferManager::uploadFile);
    connect(deleteButton, &QPushButton::clicked, this, &FileTransferManager::deleteFile);
    // connect(downloadButton, &QPushButton::clicked, this, &FileTransferManager::downloadFile);
    connect(refreshButton, &QPushButton::clicked, this, &FileTransferManager::refreshFileRecords);
    populateFileTable();
}

void FileTransferManager::populateFileTable()
{
    fileTable->setRowCount(0); // 清空表格
    QVector<QVector<QString>> records = dbManager->getFileTransferRecords(1000); // 获取最多1000条记录

    for (const auto &record : records) {
        int row = fileTable->rowCount();
        fileTable->insertRow(row);

        // 发送者
        fileTable->setItem(row, 0, new QTableWidgetItem(record[0]));
        fileTable->item(row, 0)->setTextAlignment(Qt::AlignCenter);

        // 文件名
        fileTable->setItem(row, 1, new QTableWidgetItem(record[1]));

        // 文件大小
        fileTable->setItem(row, 2, new QTableWidgetItem(record[2]));
        fileTable->item(row, 2)->setTextAlignment(Qt::AlignCenter);

        // 上传时间
        fileTable->setItem(row, 3, new QTableWidgetItem(record[3]));
        fileTable->item(row, 3)->setTextAlignment(Qt::AlignCenter);
    }
}

void FileTransferManager::uploadFile()
{
    // 选择文件
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件", "", "All Files (*.*)");
    if (filePath.isEmpty()) return;

    QFileInfo fileInfo(filePath);
    QString filename = fileInfo.fileName();
    qint64 size = fileInfo.size();

    // 目标路径：本地存储目录 + 文件名
    QString destinationPath = fileStoragePath + filename;

    // 检查文件是否已存在
    if (QFile::exists(destinationPath)) {
        if (QMessageBox::question(this, "文件已存在",
                                  QString("文件 %1 已存在，是否覆盖？").arg(filename))
            != QMessageBox::Yes) {
            return;
        }
        QFile::remove(destinationPath); // 删除现有文件
    }

    // 本地复制文件（模拟上传）
    if (QFile::copy(filePath, destinationPath)) {
        // 添加记录到数据库
        if (dbManager->addFileTransferRecord("管理员", filename, size)) {
            QMessageBox::information(this, "成功", QString("文件 %1 已上传到本地！").arg(filename));
            populateFileTable(); // 刷新文件表格
        } else {
            QMessageBox::warning(this, "失败", "文件记录添加失败！");
            QFile::remove(destinationPath); // 回滚：删除已复制的文件
        }
    } else {
        QMessageBox::warning(this, "失败", QString("文件 %1 上传失败：无法复制文件！").arg(filename));
    }
}

void FileTransferManager::deleteFile()
{
    QList<QTableWidgetItem*> selectedItems = fileTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "未选择", "请先选择要删除的文件记录！");
        return;
    }

    int row = fileTable->row(selectedItems.first());
    QString filename = fileTable->item(row, 1)->text();
    QString sender = fileTable->item(row, 0)->text();
    QString timestampStr = fileTable->item(row, 3)->text();
    QDateTime timestamp = QDateTime::fromString(timestampStr, Qt::ISODate);

    if (QMessageBox::question(this, "确认删除",
                              QString("确定删除文件记录和本地文件 '%1' 吗？").arg(filename))
        != QMessageBox::Yes) {
        return;
    }

    // 构造本地文件路径
    QString localFilePath = fileStoragePath + filename;
    bool fileRemoved = false;

    // 尝试删除本地文件
    if (QFile::exists(localFilePath)) {
        fileRemoved = QFile::remove(localFilePath);
        if (!fileRemoved) {
            QMessageBox::warning(this, "失败",
                                 QString("无法删除本地文件 '%1'，请检查权限或文件状态！").arg(filename));
            return; // 如果文件删除失败，不继续删除记录
        }
    } else {
        fileRemoved = true; // 文件不存在，视为删除成功
    }

    // 删除数据库记录
    if (dbManager->deleteFileTransferRecord(sender, filename, timestamp)) {
        fileTable->removeRow(row);
        QMessageBox::information(this, "成功",
                                 QString("文件记录和本地文件 '%1' 已删除！").arg(filename));
    } else {
        QMessageBox::warning(this, "失败", "删除文件记录失败！");
        // 如果记录删除失败，且文件已删除，可以选择回滚（这里未实现回滚）
    }
}

// void FileTransferManager::downloadFile()
// {
//     QList<QTableWidgetItem*> selectedItems = fileTable->selectedItems();
//     if (selectedItems.isEmpty()) {
//         QMessageBox::warning(this, "未选择", "请先选择要下载的文件记录！");
//         return;
//     }

//     int row = fileTable->row(selectedItems.first());
//     QString filename = fileTable->item(row, 1)->text();

//     QString savePath = QFileDialog::getSaveFileName(this, "保存文件", filename, "All Files (*.*)");
//     if (savePath.isEmpty()) return;

//     // 模拟下载，实际中需与 Server 通信
//     QMessageBox::information(this, "模拟下载", QString("文件 %1 下载完成（路径: %2）").arg(filename, savePath));
//     // TODO: 实际实现需通过 Server 下载文件内容到 savePath
// }

void FileTransferManager::refreshFileRecords()
{
    populateFileTable();
    QMessageBox::information(this, "成功", "文件传输记录已刷新！");
}
