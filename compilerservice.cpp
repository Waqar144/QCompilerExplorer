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
    if (path.startsWith(QLatin1String("compilers")))
        endpoint = QCompilerExplorer::stringToEndpoint.value(QStringLiteral("compilers"));
    else if (path.startsWith(QLatin1String("compiler")))
        endpoint = QCompilerExplorer::stringToEndpoint.value(QStringLiteral("compiler"));
    else if (path.startsWith(QStringLiteral("asm")))
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
    optObj[QStringLiteral("userArguments")] = userArgs;

    //compiler options obj
    QJsonObject compilerObj;
    compilerObj[QStringLiteral("skipAsm")] = false;
    compilerObj[QStringLiteral("executorRequest")] = false;

    //add compileropts to opt obj
    optObj[QStringLiteral("compilerOptions")] = compilerObj;

    //filters
    QJsonObject filterObj;
    filterObj[QStringLiteral("binary")] = false;
    filterObj[QStringLiteral("commentOnly")] = true;
    filterObj[QStringLiteral("demangle")] = true;
    filterObj[QStringLiteral("directives")] = true;
    filterObj[QStringLiteral("intel")] = isIntel;
    filterObj[QStringLiteral("labels")] = true;
    filterObj[QStringLiteral("execute")] = false;

    optObj[QStringLiteral("filters")] = filterObj;

    QJsonObject main;
    //    main["source"] = "int sum(){ return 2 + 2; }";
    main[QStringLiteral("source")] = source;
    main[QStringLiteral("options")] = optObj;

    QJsonDocument doc { main };
    return doc;
}
