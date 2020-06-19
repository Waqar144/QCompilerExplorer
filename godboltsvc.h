#ifndef GODBOLTSVC_H
#define GODBOLTSVC_H

#include "godboltendpoints.h"
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>

class GodboltSvc : public QObject
{
    Q_OBJECT
public:
    static GodboltSvc* instance(QObject* parent = nullptr);
    void sendRequest(QGodBolt::Endpoints endpoint);

private slots:
    void slotNetworkReply(QNetworkReply* reply);

private:
    GodboltSvc(QObject* parent);
    QNetworkAccessManager* mgr;
};

#endif // GODBOLTSVC_H
