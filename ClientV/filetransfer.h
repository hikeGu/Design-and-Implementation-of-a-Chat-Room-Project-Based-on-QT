#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include "client.h"

class FileTransfer : public QWidget
{
    Q_OBJECT
public:
    explicit FileTransfer(Client *client, QWidget *parent = nullptr);
    ~FileTransfer();

private slots:
    void refreshFileList();
    void uploadFile();
    void downloadFile();
    void handleFileList(const QString &message);
    void displayStatus(const QString &message);

private:
    void setupUI();
    void applyStyles();

    Client *client;
    QTableWidget *fileTable; // 使用表格显示文件详情
    QPushButton *refreshButton;
    QPushButton *uploadButton;
    QPushButton *downloadButton;
    QVBoxLayout *mainLayout;
};

#endif // FILETRANSFER_H
