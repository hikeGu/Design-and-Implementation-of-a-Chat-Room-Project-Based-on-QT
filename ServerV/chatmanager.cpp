#include "chatmanager.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QTextEdit>

ChatManager::ChatManager(DatabaseManager *db, QWidget *parent)
    : QDialog(parent), dbManager(db)
{
    setWindowTitle("聊天记录管理");
    setFixedSize(800, 500);
    setStyleSheet("QDialog { background-color: #F7FAFC; border-radius: 10px; }");

    setupUI();
    populateChatTable();
}

ChatManager::~ChatManager()
{
}

void ChatManager::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("聊天记录管理");
    titleLabel->setStyleSheet(
        "font-size: 22px; font-weight: bold; color: #2D3748; "
        "padding-bottom: 10px;");
    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);

    // 顶部按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(15);

    refreshButton = new QPushButton("刷新记录");
    refreshButton->setFixedSize(120, 40);
    refreshButton->setStyleSheet(
        "QPushButton { background-color: #2B6CB0; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.1); } "
        "QPushButton:hover { background-color: #2C5282; } "
        "QPushButton:pressed { background-color: #1A4971; }");

    clearButton = new QPushButton("清空记录");
    clearButton->setFixedSize(120, 40);
    clearButton->setStyleSheet(
        "QPushButton { background-color: #E53E3E; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.1); } "
        "QPushButton:hover { background-color: #C53030; } "
        "QPushButton:pressed { background-color: #9B2C2C; }");

    deleteButton = new QPushButton("删除记录");
    deleteButton->setFixedSize(120, 40);
    deleteButton->setStyleSheet(
        "QPushButton { background-color: #DD6B20; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.1); } "
        "QPushButton:hover { background-color: #C05621; } "
        "QPushButton:pressed { background-color: #9C4221; }");

    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // 聊天记录表格
    chatTable = new QTableWidget(this);
    chatTable->setColumnCount(3);
    chatTable->setHorizontalHeaderLabels({"发送者", "消息内容", "时间"});
    chatTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    chatTable->setStyleSheet(
        "QTableWidget { "
        "  background-color: #FFFFFF; "
        "  border: none; "
        "  border-radius: 10px; "
        "  padding: 10px; "
        "  font-size: 14px; "
        "  color: #2D3748; "
        "  box-shadow: 0 2px 8px rgba(0,0,0,0.05); "
        "} "
        "QTableWidget::item { "
        "  padding: 10px; "
        "  border-bottom: 1px solid #EDF2F7; "
        "} "
        "QTableWidget::item:selected { "
        "  background-color: #E2E8F0; "
        "} "
        "QHeaderView::section { "
        "  background-color: #E2E8F0; "
        "  padding: 12px; "
        "  border: none; "
        "  font-weight: bold; "
        "  color: #4A5568; "
        "  border-bottom: 2px solid #CBD5E0; "
        "}");
    chatTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    chatTable->verticalHeader()->setVisible(false);
    mainLayout->addWidget(chatTable, 1);

    // 搜索区域（表格下方）
    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->setSpacing(15);

    userSearchEdit = new QLineEdit;
    userSearchEdit->setPlaceholderText("按用户名查找");
    userSearchEdit->setFixedWidth(180);
    userSearchEdit->setStyleSheet(
        "QLineEdit { "
        "  padding: 10px; "
        "  border: 1px solid #CBD5E0; "
        "  border-radius: 8px; "
        "  background-color: #FFFFFF; "
        "  font-size: 14px; "
        "  color: #2D3748; "
        "  box-shadow: 0 1px 3px rgba(0,0,0,0.05); "
        "} "
        "QLineEdit:focus { "
        "  border-color: #2B6CB0; "
        "  box-shadow: 0 0 5px rgba(43,108,176,0.3); "
        "}");

    searchUserButton = new QPushButton("查找用户");
    searchUserButton->setFixedSize(100, 40);
    searchUserButton->setStyleSheet(
        "QPushButton { background-color: #38A169; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.1); } "
        "QPushButton:hover { background-color: #2F855A; } "
        "QPushButton:pressed { background-color: #276749; }");

    keywordSearchEdit = new QLineEdit;
    keywordSearchEdit->setPlaceholderText("按关键字查找");
    keywordSearchEdit->setFixedWidth(180);
    keywordSearchEdit->setStyleSheet(
        "QLineEdit { "
        "  padding: 10px; "
        "  border: 1px solid #CBD5E0; "
        "  border-radius: 8px; "
        "  background-color: #FFFFFF; "
        "  font-size: 14px; "
        "  color: #2D3748; "
        "  box-shadow: 0 1px 3px rgba(0,0,0,0.05); "
        "} "
        "QLineEdit:focus { "
        "  border-color: #805AD5; "
        "  box-shadow: 0 0 5px rgba(128,90,213,0.3); "
        "}");

    searchKeywordButton = new QPushButton("查找关键字");
    searchKeywordButton->setFixedSize(100, 40);
    searchKeywordButton->setStyleSheet(
        "QPushButton { background-color: #805AD5; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.1); } "
        "QPushButton:hover { background-color: #6B46C1; } "
        "QPushButton:pressed { background-color: #553C9A; }");

    showAllButton = new QPushButton("显示全部");
    showAllButton->setFixedSize(100, 40);
    showAllButton->setStyleSheet(
        "QPushButton { background-color: #4A5568; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.1); } "
        "QPushButton:hover { background-color: #2D3748; } "
        "QPushButton:pressed { background-color: #1A202C; }");

    searchLayout->addWidget(userSearchEdit);
    searchLayout->addWidget(searchUserButton);
    searchLayout->addWidget(keywordSearchEdit);
    searchLayout->addWidget(searchKeywordButton);
    searchLayout->addWidget(showAllButton);
    searchLayout->addStretch();
    mainLayout->addLayout(searchLayout);

    // 连接信号和槽
    connect(refreshButton, &QPushButton::clicked, this, &ChatManager::refreshChatRecords);
    connect(clearButton, &QPushButton::clicked, this, &ChatManager::clearChatRecords);
    connect(deleteButton, &QPushButton::clicked, this, &ChatManager::deleteChatRecord);
    connect(searchUserButton, &QPushButton::clicked, this, &ChatManager::searchByUser);
    connect(searchKeywordButton, &QPushButton::clicked, this, &ChatManager::searchByKeyword);
    connect(showAllButton, &QPushButton::clicked, this, &ChatManager::showAllRecords);
    connect(chatTable, &QTableWidget::cellDoubleClicked, this, &ChatManager::showMessageDetails); // 新增：双击事件
}

void ChatManager::populateChatTable(const QString &userFilter, const QString &keywordFilter)
{
    chatTable->setRowCount(0); // 清空表格
    QVector<QVector<QString>> records;

    if (userFilter.isEmpty() && keywordFilter.isEmpty()) {
        records = dbManager->getChatRecords(1000); // 无过滤条件，获取所有记录
    } else if (!userFilter.isEmpty()) {
        records = dbManager->getChatRecordsByUser(userFilter, 1000); // 按用户过滤
    } else if (!keywordFilter.isEmpty()) {
        records = dbManager->getChatRecordsByKeyword(keywordFilter, 1000); // 按关键字过滤
    }

    for (const auto &record : records) {
        int row = chatTable->rowCount();
        chatTable->insertRow(row);

        chatTable->setItem(row, 0, new QTableWidgetItem(record[0])); // 发送者
        chatTable->item(row, 0)->setTextAlignment(Qt::AlignCenter);

        chatTable->setItem(row, 1, new QTableWidgetItem(record[1])); // 消息内容

        chatTable->setItem(row, 2, new QTableWidgetItem(record[2])); // 时间
        chatTable->item(row, 2)->setTextAlignment(Qt::AlignCenter);
    }
}

void ChatManager::refreshChatRecords()
{
    populateChatTable();
    QMessageBox::information(this, "成功", "聊天记录已刷新！"); // 新增：刷新成功反馈
}

void ChatManager::clearChatRecords()
{
    if (QMessageBox::question(this, "确认", "确定清空所有聊天记录吗？此操作不可恢复！") == QMessageBox::Yes) {
        if (dbManager->clearChatRecords()) {
            populateChatTable();
        } else {
            QMessageBox::warning(this, "失败", "清空聊天记录失败！");
        }
    }
}

void ChatManager::deleteChatRecord()
{
    QList<QTableWidgetItem*> selectedItems = chatTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "未选择", "请先选择要删除的聊天记录！");
        return;
    }

    int row = chatTable->row(selectedItems.first());
    QString sender = chatTable->item(row, 0)->text();
    QString message = chatTable->item(row, 1)->text();
    QString timestampStr = chatTable->item(row, 2)->text();
    QDateTime timestamp = QDateTime::fromString(timestampStr, Qt::ISODate);

    if (QMessageBox::question(this, "确认删除", QString("确定删除 '%1' 的聊天记录吗？").arg(message)) == QMessageBox::Yes) {
        if (dbManager->deleteChatRecord(sender, message, timestamp)) {
            chatTable->removeRow(row);
        } else {
            QMessageBox::warning(this, "失败", "删除聊天记录失败！");
        }
    }
}

void ChatManager::searchByUser()
{
    QString user = userSearchEdit->text().trimmed();
    if (user.isEmpty()) {
        return; // 空输入时不执行查找
    }
    populateChatTable(user, "");
}

void ChatManager::searchByKeyword()
{
    QString keyword = keywordSearchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        return; // 空输入时不执行查找
    }
    populateChatTable("", keyword);
}

void ChatManager::showAllRecords()
{
    populateChatTable(); // 显示所有记录
}

void ChatManager::showMessageDetails(int row, int column)
{
    if (column != 1) { // 只在双击“消息内容”列（第1列）时触发
        return;
    }

    QString sender = chatTable->item(row, 0)->text();
    QString message = chatTable->item(row, 1)->text();
    QString timestamp = chatTable->item(row, 2)->text();

    // 创建详情对话框
    QDialog *detailsDialog = new QDialog(this);
    detailsDialog->setWindowTitle("消息详情");
    detailsDialog->setFixedSize(400, 300);
    detailsDialog->setStyleSheet("QDialog { background-color: #FFFFFF; border-radius: 10px; }");

    QVBoxLayout *layout = new QVBoxLayout(detailsDialog);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);

    // 发送者
    QLabel *senderLabel = new QLabel(QString("发送者: %1").arg(sender));
    senderLabel->setStyleSheet("font-size: 14px; color: #2D3748;");
    layout->addWidget(senderLabel);

    // 时间
    QLabel *timeLabel = new QLabel(QString("时间: %1").arg(timestamp));
    timeLabel->setStyleSheet("font-size: 14px; color: #2D3748;");
    layout->addWidget(timeLabel);

    // 消息内容（使用 QTextEdit 支持多行显示）
    QTextEdit *messageEdit = new QTextEdit;
    messageEdit->setText(message);
    messageEdit->setReadOnly(true);
    messageEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #F7FAFC; "
        "  border: 1px solid #CBD5E0; "
        "  border-radius: 8px; "
        "  padding: 10px; "
        "  font-size: 14px; "
        "  color: #2D3748; "
        "}");
    layout->addWidget(messageEdit, 1);

    // 关闭按钮
    QPushButton *closeButton = new QPushButton("关闭");
    closeButton->setFixedSize(100, 40);
    closeButton->setStyleSheet(
        "QPushButton { background-color: #4A5568; color: white; font-size: 14px; font-weight: 600; border-radius: 8px; padding: 10px; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.1); } "
        "QPushButton:hover { background-color: #2D3748; } "
        "QPushButton:pressed { background-color: #1A202C; }");
    connect(closeButton, &QPushButton::clicked, detailsDialog, &QDialog::accept);
    layout->addWidget(closeButton, 0, Qt::AlignRight);

    detailsDialog->exec();
    delete detailsDialog; // 手动删除对话框
}
