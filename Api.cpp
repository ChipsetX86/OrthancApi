#include "Api.h"
#include <QDebug>
#include <QNetworkReply>
#include <QEventLoop>
#include <QAuthenticator>

const QString httpUserName = QStringLiteral("DicomNode");
const QString httpPassword = QStringLiteral("");

Api::Api(QString host, quint16 port, QObject *parent) :
    QObject(parent),
    m_host(host),
    m_port(port)
{

}

QString Api::createUrl(QString resource) const
{
    return QStringLiteral("http://%1:%2/%3").arg(m_host).arg(m_port).arg(resource);
}

Api::CallResult Api::handleReply(QNetworkReply *reply)
{
    if (reply->error()) {
        qDebug() << reply->error() << reply->errorString();
    }
    CallResult results;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid()) {
        QByteArray response = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            results = {true, true, response};
        } else {
            results = {true, false, response};
        }
    } else {
        results = {false, false, QByteArray()};
    }
    reply->deleteLater();
    return results;
}

Api::CallResult Api::call(QString resource, Method method, const QByteArray &data)
{
    QNetworkAccessManager network;
    connect(&network, &QNetworkAccessManager::authenticationRequired, this,
            [](QNetworkReply *reply, QAuthenticator *authenticator) {
        Q_UNUSED(reply)
        authenticator->setUser(httpUserName);
        authenticator->setPassword(httpPassword);
    }, Qt::DirectConnection);
    QNetworkRequest req(createUrl(resource));
    QNetworkReply *reply;
    QEventLoop loop;
    switch (method) {
    case GET:
        reply = network.get(req);
        break;
    case POST:
        reply = network.post(req, data);
        break;
    case DELETE:
        reply = network.deleteResource(req);
        break;
    case PUT:
        reply = network.put(req, data);
        break;
    default:
        //should never happen
        break;
    }
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>
    (&QNetworkReply::error), [&loop](QNetworkReply::NetworkError) { loop.quit(); });
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    return handleReply(reply);
}
