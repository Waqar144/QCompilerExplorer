#ifndef GODBOLTSVC_H
#define GODBOLTSVC_H

#include "ce_endpoints.h"
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>

class CompilerExplorerSvc : public QObject
{
    Q_OBJECT
public:
    static CompilerExplorerSvc* instance();
    void sendRequest(QCompilerExplorer::Endpoints endpoint, const QString& additional = QString());
    void compileRequest(const QString& endpoint, const QByteArray& obj);
    QNetworkReply* tooltipRequest(const QString& asmWord);

    ~CompilerExplorerSvc();

    static QJsonDocument getCompilationOptions(const QString &source, const QString &userArgs, bool isIntel);
private slots:
    void slotNetworkReply(QNetworkReply* reply);

signals:
    void languages(const QByteArray& data);
    void compilers(const QByteArray& data);
    void asmResult(const QByteArray& data);

private:
    CompilerExplorerSvc(QObject* parent = nullptr);
    QNetworkAccessManager* mgr;
};

#endif // GODBOLTSVC_H
