#include "compilerservice.h"

#include <QtNetwork/QNetworkReply>

static const char url[] = "https://godbolt.org/api/";

CompileSvc* CompileSvc::instance()
{
    static CompileSvc s_instance;
    return &s_instance;
}

void CompileSvc::sendRequest(QGodBolt::Endpoints endpoint, const QString& additional)
{
    QString endp = QGodBolt::endpointsToString.value(endpoint);
    QString requestUrl = url + endp + additional;
    QUrl url { requestUrl };
    QNetworkRequest req { url };
    req.setRawHeader("ACCEPT", "application/json");
    req.setRawHeader("Content-Type", "application/josn");

    mgr->get(req);
}

void CompileSvc::compileRequest(const QString& endpoint, const QByteArray& obj)
{
    QString requestUrl = url + endpoint;
    QNetworkRequest req { QUrl { requestUrl } };
    req.setRawHeader("ACCEPT", "application/json");
    req.setRawHeader("Content-Type", "application/json");
    mgr->post(req, obj);
}

QNetworkReply* CompileSvc::tooltipRequest(const QString& asmWord)
{
    QNetworkRequest request;
    QString urlString = url;
    urlString += "asm/" + asmWord;
    request.setRawHeader("ACCEPT", "application/json");
    request.setRawHeader("Content-Type", "application/json");
    request.setUrl(urlString);
    return mgr->get(request);
}

CompileSvc::~CompileSvc()
{
    delete mgr;
}

CompileSvc::CompileSvc(QObject* parent)
    : QObject(parent)
{
    mgr = new QNetworkAccessManager(this);
    connect(mgr, &QNetworkAccessManager::finished, this, &CompileSvc::slotNetworkReply);
}

void CompileSvc::slotNetworkReply(QNetworkReply* reply)
{
    const QString path = reply->url().path().split('/').at(2);
    QGodBolt::Endpoints endpoint;
    if (path.startsWith("compilers"))
        endpoint = QGodBolt::stringToEndpoint.value("compilers");
    else if (path.startsWith("compiler"))
        endpoint = QGodBolt::stringToEndpoint.value("compiler");
    else if (path.startsWith("asm"))
        return;
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
    case QGodBolt::CompilerCompile:
        emit asmResult(data);
        break;
    }
}
