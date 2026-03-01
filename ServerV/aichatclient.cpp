#include "aichatclient.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

AIChatClient::AIChatClient(QObject *parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this))
{
    // 连接网络请求完成信号
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &AIChatClient::handleResponse);
}

void AIChatClient::sendMessage(const QString &userMessage)
{
    if (userMessage.isEmpty()) {
        emit errorOccurred("用户消息不能为空");
        return;
    }

    // 发送请求到AI API
    sendRequest(userMessage);
}

void AIChatClient::sendRequest(const QString &message)
{
    QUrl url("https://api.siliconflow.cn/v1/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer sk-zcbkxmyggvltudckjwdskauqlwmvrrbsfwaabbpwtbvdgoge");

    // 构造JSON请求体
    QJsonObject json;
    json["model"] = "deepseek-ai/DeepSeek-R1-Distill-Qwen-7B";
    json["stream"] = false;
    json["max_tokens"] = 8192;
    json["temperature"] = 0.6;
    json["top_p"] = 0.7;
    json["top_k"] = 50;
    json["frequency_penalty"] = 0.5;
    json["n"] = 1;
    json["stop"] = QJsonArray();

    QJsonArray messages;
    QJsonObject messageObj;
    messageObj["role"] = "user";
    messageObj["content"] = message;
    messages.append(messageObj);
    json["messages"] = messages;

    // 发送POST请求
    QByteArray data = QJsonDocument(json).toJson();
    networkManager->post(request, data);
}

void AIChatClient::handleResponse(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    // 读取响应数据
    QByteArray response = reply->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
    QJsonObject jsonObject = jsonResponse.object();

    // 解析AI回复
    if (jsonObject.contains("choices")) {
        QJsonArray choices = jsonObject["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject choice = choices[0].toObject();
            QJsonObject message = choice["message"].toObject();
            QString content = message["content"].toString();
            emit responseReceived(content);
        } else {
            emit errorOccurred("未收到有效的AI回复");
        }
    } else {
        emit errorOccurred("无效的API响应");
    }

    reply->deleteLater();
}
