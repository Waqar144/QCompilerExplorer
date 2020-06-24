#ifndef GODBOLTENDPOINTS_H
#define GODBOLTENDPOINTS_H

#include <QHash>

namespace QGodBolt
{
enum Endpoints {
    Languages,
    Compilers,
    CompilerCompile
};

static const QHash<Endpoints, QString> endpointsToString = {
    { Endpoints::Languages, "languages" },
    { Endpoints::Compilers, "compilers" },
    { Endpoints::CompilerCompile, "compiler" }
};

static const QHash<QString, Endpoints> stringToEndpoint = {
    { "languages", Endpoints::Languages },
    { "compilers", Endpoints::Compilers },
    { "compiler", Endpoints::CompilerCompile }
};
}

#endif // GODBOLTENDPOINTS_H
