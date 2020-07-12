#ifndef ASMPARSER_H
#define ASMPARSER_H

#include <QByteArray>

class AsmParser
{
public:
    AsmParser() = default;
    QString process(const QByteArray& asmText);
    QString demangle(QString&& asmText);
};

#endif // ASMPARSER_H
