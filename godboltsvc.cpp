#include "godboltsvc.h"

#include <QtNetwork/QNetworkReply>

static QString url = QStringLiteral("https://godbolt.org/api/");

GodboltSvc* GodboltSvc::instance()
{
    static GodboltSvc s_instance;
    return &s_instance;
}

void GodboltSvc::sendRequest(QGodBolt::Endpoints endpoint, const QString& additional)
{
    QString endp = QGodBolt::endpointsToString.value(endpoint);
    QString requestUrl = url + endp + additional;
    QUrl url { requestUrl };
    QNetworkRequest req { url };
    req.setRawHeader("ACCEPT", "application/json");
    req.setRawHeader("Content-Type", "application/josn");

    mgr->get(req);
}

GodboltSvc::GodboltSvc(QObject* parent)
    : QObject(parent)
{
    mgr = new QNetworkAccessManager(this);
    connect(mgr, &QNetworkAccessManager::finished, this, &GodboltSvc::slotNetworkReply);
}

void GodboltSvc::slotNetworkReply(QNetworkReply* reply)
{
    const QString path = reply->url().path().split('/').at(2);
    QGodBolt::Endpoints endpoint;
    if (path.startsWith("compilers"))
        endpoint = QGodBolt::stringToEndpoint.value("compilers");
    else
        endpoint = QGodBolt::stringToEndpoint.value(path);
    const QByteArray data = reply->readAll();

    switch (endpoint) {
    case QGodBolt::Languages: {
        emit languages(data);
        break;
    }
    case QGodBolt::Compilers:
        emit compilers(data);
        break;
    }
}
