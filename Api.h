#ifndef API_H
#define API_H

#include "CommonGlobal.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonDocument>

class COMMON_LIBRARY_EXPORT Api : public QObject
{
    Q_OBJECT
public:
    struct CallResult
    {
        bool serverOk;
        bool resourceFound;
        QByteArray answer;
    };
    enum Method
    {
        GET,
        POST,
        DELETE,
        PUT
    };

    Api(QString host, quint16 port, QObject *parent = 0);
    CallResult call(QString resource, Method method, const QByteArray &data);
private:
    QString m_host;
    quint16 m_port;
    QString createUrl(QString resource) const;
    CallResult handleReply(QNetworkReply* reply);
};

#endif // API_H
