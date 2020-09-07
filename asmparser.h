#ifndef ASMPARSER_H
#define ASMPARSER_H

#include <QByteArray>

namespace AsmParser
{
    QString process(const QByteArray& asmText);
    QString demangle(QString&& asmText);
};

#endif // ASMPARSER_H
