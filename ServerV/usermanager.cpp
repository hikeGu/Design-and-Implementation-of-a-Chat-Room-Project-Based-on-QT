#include "usermanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

UserManager::UserManager(DatabaseManager *db, Server *server, QWidget *parent)
    : QDialog(parent), dbManager(db), server(server)
{
    setWindowTitle("用户管理");
    setFixedSize(800, 600);
    setStyleSheet("background-color: #F7FAFC;");

    setupUI();
    populateUserTable();
    connect(server, &Server::userStatusChanged, this, &UserManager::updateUserStatus);
}

UserManager::~UserManager()
{
}

void UserManager::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("用户管理");
    titleLabel->setStyleSheet(
        "font-size: 24px;"
        "font-weight: bold;"
        "color: #2D3748;"
        "margin-bottom: 10px;"
        );
    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);

    // 按钮区域
    QWidget *buttonContainer = new QWidget;
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setSpacing(15);

    addButton = new QPushButton("添加用户");
    addButton->setFixedSize(140, 45);
    addButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2B6CB0;"
        "  color: white;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "  border: none;"
        "  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);"
        "}"
        "QPushButton:hover {"
        "  background-color: #2C5282;"
        "  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);"
        "}"
        "QPushButton:pressed {"
        "  background-color: #2A4365;"
        "  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2);"
        "}"
        );

    removeButton = new QPushButton("删除用户");
    removeButton->setFixedSize(140, 45);
    removeButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #E53E3E;"
        "  color: white;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "  border: none;"
        "  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);"
        "}"
        "QPushButton:hover {"
        "  background-color: #C53030;"
        "  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);"
        "}"
        "QPushButton:pressed {"
        "  background-color: #9B2C2C;"
        "  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2);"
        "}"
        );

    updatePasswordButton = new QPushButton("修改密码");
    updatePasswordButton->setFixedSize(140, 45);
    updatePasswordButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #D69E2E;"
        "  color: white;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "  border: none;"
        "  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);"
        "}"
        "QPushButton:hover {"
        "  background-color: #B7791F;"
        "  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);"
        "}"
        "QPushButton:pressed {"
        "  background-color: #975A16;"
        "  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2);"
        "}"
        );

    refreshButton = new QPushButton("刷新列表");
    refreshButton->setFixedSize(140, 45);
    refreshButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #48BB78;"
        "  color: white;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "  border: none;"
        "  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);"
        "}"
        "QPushButton:hover {"
        "  background-color: #38A169;"
        "  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);"
        "}"
        "QPushButton:pressed {"
        "  background-color: #2F855A;"
        "  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.2);"
        "}"
        );

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(updatePasswordButton);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    mainLayout->addWidget(buttonContainer);

    // 用户列表表格
    userTable = new QTableWidget(this);
    userTable->setColumnCount(2); // 用户名和在线状态
    userTable->setHorizontalHeaderLabels({"用户名", "在线状态"});
    userTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    userTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 12px;"
        "  padding: 8px;"
        "  font-size: 14px;"
        "  color: #2D3748;"
        "  gridline-color: #E2E8F0;"
        "}"
        "QTableWidget::item {"
        "  padding: 12px;"
        "  border-bottom: 1px solid #EDF2F7;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #EBF8FF;"
        "  color: #2B6CB0;"
        "}"
        "QHeaderView::section {"
        "  background-color: #EDF2F7;"
        "  padding: 12px;"
        "  border: none;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  color: #4A5568;"
        "}"
        );
    userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    userTable->setSelectionMode(QAbstractItemView::SingleSelection);
    userTable->setShowGrid(false);
    mainLayout->addWidget(userTable);

    // 连接信号和槽
    connect(addButton, &QPushButton::clicked, this, &UserManager::addUser);
    connect(removeButton, &QPushButton::clicked, this, &UserManager::removeUser);
    connect(updatePasswordButton, &QPushButton::clicked, this, &UserManager::updatePassword);
    connect(refreshButton, &QPushButton::clicked, this, &UserManager::refreshUserList);
}

void UserManager::populateUserTable()
{
    userTable->setRowCount(0);
    QVector<QString> users = dbManager->getAllUsers();
    QMap<QTcpSocket*, QString> clients = server->getClients();

    for (const QString &username : users) {
        int row = userTable->rowCount();
        userTable->insertRow(row);

        // 用户名
        userTable->setItem(row, 0, new QTableWidgetItem(username));
        userTable->item(row, 0)->setTextAlignment(Qt::AlignCenter);

        // 在线状态
        bool isOnline = clients.values().contains(username);
        userTable->setItem(row, 1, new QTableWidgetItem(isOnline ? "在线" : "离线"));
        userTable->item(row, 1)->setTextAlignment(Qt::AlignCenter);
        userTable->item(row, 1)->setForeground(isOnline ? QBrush(Qt::darkGreen) : QBrush(Qt::gray));
    }
}

void UserManager::addUser()
{
    bool ok;
    QString username = QInputDialog::getText(this, "添加用户", "请输入用户名：",
                                             QLineEdit::Normal, "", &ok);
    if (!ok || username.trimmed().isEmpty()) {
        if (ok) QMessageBox::warning(this, "输入错误", "用户名不能为空！");
        return;
    }

    QString password = QInputDialog::getText(this, "添加用户", "请输入密码：",
                                             QLineEdit::Password, "", &ok);
    if (!ok || password.trimmed().isEmpty()) {
        if (ok) QMessageBox::warning(this, "输入错误", "密码不能为空！");
        return;
    }

    QVector<QString> users = dbManager->getAllUsers();
    if (users.contains(username)) {
        QMessageBox::warning(this, "失败", "用户名已存在！");
        return;
    }

    if (dbManager->addUser(username, password)) {
        QMessageBox::information(this, "成功", QString("用户 '%1' 添加成功！").arg(username));
        populateUserTable();
    } else {
        QMessageBox::warning(this, "失败", "添加用户失败！");
    }
}

void UserManager::removeUser()
{
    QList<QTableWidgetItem*> selectedItems = userTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "未选择", "请先选择要删除的用户！");
        return;
    }

    int row = userTable->row(selectedItems.first());
    QString username = userTable->item(row, 0)->text();

    if (QMessageBox::question(this, "确认删除",
                              QString("确定删除用户 '%1' 吗？").arg(username))
        == QMessageBox::Yes) {
        if (dbManager->removeUser(username)) {
            QMap<QTcpSocket*, QString> clients = server->getClients();
            QTcpSocket *socket = clients.key(username, nullptr);
            if (socket) {
                socket->disconnectFromHost(); // 断开在线用户的连接
            }
            userTable->removeRow(row);
            QMessageBox::information(this, "成功", QString("用户 '%1' 已删除！").arg(username));
        } else {
            QMessageBox::warning(this, "失败", "删除用户失败！");
        }
    }
}

void UserManager::updatePassword()
{
    QList<QTableWidgetItem*> selectedItems = userTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "未选择", "请先选择要修改密码的用户！");
        return;
    }

    int row = userTable->row(selectedItems.first());
    QString username = userTable->item(row, 0)->text();

    bool ok;
    QString newPassword = QInputDialog::getText(this, "修改密码",
                                                QString("为用户 '%1' 输入新密码：").arg(username),
                                                QLineEdit::Password, "", &ok);
    if (!ok || newPassword.trimmed().isEmpty()) {
        if (ok) QMessageBox::warning(this, "输入错误", "新密码不能为空！");
        return;
    }

    if (QMessageBox::question(this, "确认修改",
                              QString("确定为用户 '%1' 修改密码吗？").arg(username))
        == QMessageBox::Yes) {
        if (dbManager->updateUserPassword(username, newPassword)) {
            QMessageBox::information(this, "成功", QString("用户 '%1' 密码已修改！").arg(username));
        } else {
            QMessageBox::warning(this, "失败", "修改密码失败！");
        }
    }
}

void UserManager::refreshUserList()
{
    populateUserTable();
    QMessageBox::information(this, "成功", "用户列表已刷新！");
}

void UserManager::updateUserStatus(const QString &username, bool isOnline)
{
    for (int row = 0; row < userTable->rowCount(); ++row) {
        if (userTable->item(row, 0)->text() == username) {
            userTable->setItem(row, 1, new QTableWidgetItem(isOnline ? "在线" : "离线"));
            userTable->item(row, 1)->setTextAlignment(Qt::AlignCenter);
            userTable->item(row, 1)->setForeground(isOnline ? QBrush(Qt::darkGreen) : QBrush(Qt::gray));
            break;
        }
    }
}
