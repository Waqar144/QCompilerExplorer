#ifndef GODBOLTSVC_H
#define GODBOLTSVC_H

#include "godboltendpoints.h"
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>

class GodboltSvc : public QObject
{
    Q_OBJECT
public:
    static GodboltSvc* instance();
    void sendRequest(QGodBolt::Endpoints endpoint, const QString& additional = QString());

private slots:
    void slotNetworkReply(QNetworkReply* reply);

signals:
    void languages(const QByteArray& data);
    void compilers(const QByteArray& data);

private:
    GodboltSvc(QObject* parent = nullptr);
    QNetworkAccessManager* mgr;
};

#endif // GODBOLTSVC_H
