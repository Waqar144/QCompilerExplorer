#include "compilerservice.h"

#include <QtNetwork/QNetworkReply>

#include <QJsonDocument>
#include <QJsonObject>

static const char url[] = "https://godbolt.org/api/";

CompilerExplorerSvc* CompilerExplorerSvc::instance()
{
    static CompilerExplorerSvc s_instance;
    return &s_instance;
}

void CompilerExplorerSvc::sendRequest(QCompilerExplorer::Endpoints endpoint, const QString& additional)
{
    QString endp = QCompilerExplorer::endpointsToString.value(endpoint);
    QString requestUrl = url + endp + additional;
    QUrl url { requestUrl };
    QNetworkRequest req { url };
    req.setRawHeader("ACCEPT", "application/json");
    req.setRawHeader("Content-Type", "application/josn");

    mgr->get(req);
}

void CompilerExplorerSvc::compileRequest(const QString& endpoint, const QByteArray& obj)
{
    QString requestUrl = url + endpoint;
    QNetworkRequest req { QUrl { requestUrl } };
    req.setRawHeader("ACCEPT", "application/json");
    req.setRawHeader("Content-Type", "application/json");
    mgr->post(req, obj);
}

QNetworkReply* CompilerExplorerSvc::tooltipRequest(const QString& asmWord)
{
    QNetworkRequest request;
    QString urlString = url;
    urlString += "asm/" + asmWord;
    request.setRawHeader("ACCEPT", "application/json");
    request.setRawHeader("Content-Type", "application/json");
    request.setUrl(urlString);
    return mgr->get(request);
}

CompilerExplorerSvc::~CompilerExplorerSvc()
{
    delete mgr;
}

CompilerExplorerSvc::CompilerExplorerSvc(QObject* parent)
    : QObject(parent)
{
    mgr = new QNetworkAccessManager(this);
    connect(mgr, &QNetworkAccessManager::finished, this, &CompilerExplorerSvc::slotNetworkReply);
}

void CompilerExplorerSvc::slotNetworkReply(QNetworkReply* reply)
{
    const QString path = reply->url().path().split('/').at(2);
    QCompilerExplorer::Endpoints endpoint;
    if (path.startsWith("compilers"))
        endpoint = QCompilerExplorer::stringToEndpoint.value("compilers");
    else if (path.startsWith("compiler"))
        endpoint = QCompilerExplorer::stringToEndpoint.value("compiler");
    else if (path.startsWith("asm"))
        return;
    else
        endpoint = QCompilerExplorer::stringToEndpoint.value(path);
    const QByteArray data = reply->readAll();

    switch (endpoint) {
    case QCompilerExplorer::Languages: {
        emit languages(data);
        break;
    }
    case QCompilerExplorer::Compilers:
        emit compilers(data);
        break;
    case QCompilerExplorer::CompilerCompile:
        emit asmResult(data);
        break;
    }
}

QJsonDocument CompilerExplorerSvc::getCompilationOptions(const QString& source, const QString& userArgs, bool isIntel)
{
    //opt obj
    QJsonObject optObj;
    optObj["userArguments"] = userArgs;

    //compiler options obj
    QJsonObject compilerObj;
    compilerObj["skipAsm"] = false;
    compilerObj["executorRequest"] = false;

    //add compileropts to opt obj
    optObj["compilerOptions"] = compilerObj;

    //filters
    QJsonObject filterObj;
    filterObj["binary"] = false;
    filterObj["commentOnly"] = true;
    filterObj["demangle"] = true;
    filterObj["directives"] = true;
    filterObj["intel"] = isIntel;
    filterObj["labels"] = true;
    filterObj["execute"] = false;

    optObj["filters"] = filterObj;

    QJsonObject main;
    //    main["source"] = "int sum(){ return 2 + 2; }";
    main["source"] = source;
    main["options"] = optObj;

    QJsonDocument doc { main };
    return doc;
}
