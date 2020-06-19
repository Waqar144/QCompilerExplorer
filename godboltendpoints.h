#ifndef GODBOLTENDPOINTS_H
#define GODBOLTENDPOINTS_H

#include <QHash>

namespace QGodBolt
{
enum Endpoints {
    Languages,
    Compilers
};

static const QHash<Endpoints, QString> endpointsToString = {
    { Endpoints::Languages, "languages" },
    { Endpoints::Compilers, "compilers" }
};

}

#endif // GODBOLTENDPOINTS_H
