#ifndef ASMPARSER_H
#define ASMPARSER_H

#include <QByteArray>

class AsmParser
{
public:
    AsmParser();
    QString process(const QByteArray& asmText);
};

#endif // ASMPARSER_H
