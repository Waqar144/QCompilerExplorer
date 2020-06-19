#include "godboltsvc.h"

#include <QtNetwork/QNetworkReply>

static QString url = QStringLiteral("https://godbolt.org/api/");

GodboltSvc* GodboltSvc::instance(QObject* parent)
{
    static GodboltSvc s_instance(parent);
    return &s_instance;
}

void GodboltSvc::sendRequest(QGodBolt::Endpoints endpoint)
{
    QString endp = QGodBolt::endpointsToString.value(endpoint);
    QString requestUrl = url + endp;
    QUrl url { requestUrl };
    QNetworkRequest req { url };
    req.setRawHeader("ACCEPT", "application/json");
    req.setRawHeader("Content-Type", "application/josn");

    qDebug() << req.rawHeaderList();
    //    qDebug() << req.url().toString();
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
    qDebug() << reply->readAll();
}
