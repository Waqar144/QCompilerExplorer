#ifndef GODBOLTSVC_H
#define GODBOLTSVC_H

#include "ce_endpoints.h"
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>

class CompileSvc : public QObject
{
    Q_OBJECT
public:
    static CompileSvc* instance();
    void sendRequest(QGodBolt::Endpoints endpoint, const QString& additional = QString());
    void compileRequest(const QString& endpoint, const QByteArray& obj);

private slots:
    void slotNetworkReply(QNetworkReply* reply);

signals:
    void languages(const QByteArray& data);
    void compilers(const QByteArray& data);
    void asmResult(const QByteArray& data);

private:
    CompileSvc(QObject* parent = nullptr);
    QNetworkAccessManager* mgr;
};

#endif // GODBOLTSVC_H
