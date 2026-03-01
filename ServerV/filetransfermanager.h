#ifndef FILETRANSFERMANAGER_H
#define FILETRANSFERMANAGER_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include "databasemanager.h"

class FileTransferManager : public QDialog
{
    Q_OBJECT

public:
    FileTransferManager(DatabaseManager *db, QWidget *parent = nullptr);
    ~FileTransferManager();

private slots:
    void uploadFile();
    void deleteFile();
    // void downloadFile();
    void refreshFileRecords();

private:
    DatabaseManager *dbManager;
    QTableWidget *fileTable;
    QPushButton *uploadButton;
    QPushButton *deleteButton;
    // QPushButton *downloadButton; // 新增下载按钮
    QPushButton *refreshButton;

    void setupUI();
    void populateFileTable();

    const QString fileStoragePath = "./files/"; // 文件存储目录
};

#endif // FILETRANSFERMANAGER_H
