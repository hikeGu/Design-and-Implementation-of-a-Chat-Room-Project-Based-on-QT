#ifndef AICHATCLIENT_H
#define AICHATCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class AIChatClient : public QObject
{
    Q_OBJECT

public:
    explicit AIChatClient(QObject *parent = nullptr);

    // 发送用户消息并获取AI回复
    void sendMessage(const QString &userMessage);

signals:
    // 当收到AI回复时发出信号
    void responseReceived(const QString &aiResponse);

    // 当发生错误时发出信号
    void errorOccurred(const QString &errorMessage);

private slots:
    // 处理网络请求的回复
    void handleResponse(QNetworkReply *reply);

private:
    // 发送HTTP请求到AI API
    void sendRequest(const QString &message);

    QNetworkAccessManager *networkManager; // 网络请求管理器
};

#endif // AICHATCLIENT_H
