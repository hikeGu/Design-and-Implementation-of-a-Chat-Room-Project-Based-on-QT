#include "historywindow.h"
#include <QDebug>

HistoryWindow::HistoryWindow(Client *client, QWidget *parent)
    : QMainWindow(parent), client(client)
{
    setupUI();
    setWindowTitle("历史消息");
    setMinimumSize(600, 400);
    setStyleSheet("background-color: #F7FAFC;");

    // 连接 Client 的 messageReceived 信号
    connect(client, &Client::messageHist, this, &HistoryWindow::appendMessage);

    // 请求历史消息
    requestHistory();
}

HistoryWindow::~HistoryWindow()
{
    // 不需要手动删除 client，它由 MainWindow 管理
}

void HistoryWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 标题
    QLabel *titleLabel = new QLabel("历史消息", centralWidget);
    titleLabel->setStyleSheet(
        "font-size: 20px;"
        "font-weight: 600;"
        "color: #2D3748;"
        );
    mainLayout->addWidget(titleLabel);

    // 消息显示区域
    historyDisplay = new QTextEdit(centralWidget);
    historyDisplay->setReadOnly(true);
    historyDisplay->setStyleSheet(
        "QTextEdit {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 10px;"
        "  padding: 15px;"
        "  font-family: 'Courier New';"
        "  font-size: 14px;"
        "  color: #2D3748;"
        "}"
        "QScrollBar:vertical {"
        "  border: none;"
        "  background: #EDF2F7;"
        "  width: 10px;"
        "  margin: 0px;"
        "  border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #A0AEC0;"
        "  min-height: 20px;"
        "  border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #718096;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        );
    mainLayout->addWidget(historyDisplay, 1);

    // 过滤区域
    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(10);

    // 按用户查找
    userFilterEdit = new QLineEdit(centralWidget);
    userFilterEdit->setPlaceholderText("输入用户名");
    userFilterEdit->setFixedWidth(150);
    userFilterEdit->setStyleSheet(
        "QLineEdit {"
        "  padding: 8px;"
        "  border: none;"
        "  border-radius: 8px;"
        "  background-color: #EDF2F7;"
        "  font-size: 14px;"
        "  color: #2D3748;"
        "}"
        "QLineEdit:hover {"
        "  background-color: #E2E8F0;"
        "}"
        );
    filterLayout->addWidget(userFilterEdit);

    userFilterButton = new QPushButton("按用户查找", centralWidget);
    userFilterButton->setFixedSize(100, 40);
    userFilterButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4A5568;"
        "  color: white;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  border-radius: 8px;"
        "  padding: 10px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2D3748;"
        "}"
        );
    filterLayout->addWidget(userFilterButton);

    // 按关键字查找
    keywordFilterEdit = new QLineEdit(centralWidget);
    keywordFilterEdit->setPlaceholderText("输入关键字");
    keywordFilterEdit->setFixedWidth(150);
    keywordFilterEdit->setStyleSheet(
        "QLineEdit {"
        "  padding: 8px;"
        "  border: none;"
        "  border-radius: 8px;"
        "  background-color: #EDF2F7;"
        "  font-size: 14px;"
        "  color: #2D3748;"
        "}"
        "QLineEdit:hover {"
        "  background-color: #E2E8F0;"
        "}"
        );
    filterLayout->addWidget(keywordFilterEdit);

    keywordFilterButton = new QPushButton("按关键字查找", centralWidget);
    keywordFilterButton->setFixedSize(120, 40);
    keywordFilterButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4A5568;"
        "  color: white;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  border-radius: 8px;"
        "  padding: 10px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2D3748;"
        "}"
        );
    filterLayout->addWidget(keywordFilterButton);

    // 显示所有信息
    showAllButton = new QPushButton("显示所有", centralWidget);
    showAllButton->setFixedSize(100, 40);
    showAllButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #38A169;"
        "  color: white;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  border-radius: 8px;"
        "  padding: 10px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2F855A;"
        "}"
        );
    filterLayout->addWidget(showAllButton);
    filterLayout->addStretch();

    mainLayout->addLayout(filterLayout);

    // 连接信号和槽
    connect(userFilterButton, &QPushButton::clicked, this, &HistoryWindow::filterByUser);
    connect(keywordFilterButton, &QPushButton::clicked, this, &HistoryWindow::filterByKeyword);
    connect(showAllButton, &QPushButton::clicked, this, &HistoryWindow::showAllMessages);
}

void HistoryWindow::requestHistory()
{
    // 向服务端发送历史消息请求
    client->requestHist();
}

void HistoryWindow::appendMessage(const QString &message)
{
    if (message.startsWith("HISTORY_OK|")) {
        historyDisplay->clear();
        historyMessages.clear();

        QStringList parts = message.split("|");
        if (parts.size() < 2) return;

        // 从后向前遍历，确保时间越近的在下面
        for (int i = parts.size() - 3; i >= 1; i -= 3) {
            QString sender = parts[i];
            QString content = parts[i + 1];
            QString time = parts[i + 2];
            QString formattedMessage = QString("[%1] [%2]: %3").arg(time, sender, content);
            historyMessages.append(formattedMessage);
            historyDisplay->append(formattedMessage);
        }
    }
}



void HistoryWindow::filterByUser()
{
    QString user = userFilterEdit->text().trimmed();
    if (user.isEmpty()) {
        showAllMessages();
        return;
    }

    historyDisplay->clear();
    for (const QString &msg : historyMessages) {
        if (msg.contains("[" + user + "]")) {
            historyDisplay->append(msg);
        }
    }
}

void HistoryWindow::filterByKeyword()
{
    QString keyword = keywordFilterEdit->text().trimmed();
    if (keyword.isEmpty()) {
        showAllMessages();
        return;
    }

    historyDisplay->clear();
    for (const QString &msg : historyMessages) {
        // 提取消息内容部分（即 ": " 之后的文本）
        int contentStart = msg.indexOf(": ") + 2; // 跳过 ": "
        if (contentStart > 1) { // 确保找到了 ": "
            QString content = msg.mid(contentStart);
            if (content.contains(keyword, Qt::CaseInsensitive)) {
                historyDisplay->append(msg);
            }
        }
    }
}

void HistoryWindow::showAllMessages()
{
    historyDisplay->clear();
    if (client) {
        client->requestHist();
    }
    for (const QString &msg : historyMessages) {
        historyDisplay->append(msg);
    }
}
