#ifndef ASMPARSER_H
#define ASMPARSER_H

class QString;

namespace AsmParser
{
    QString process(const QString& asmText);
    QString demangle(QString&& asmText);
}

#endif // ASMPARSER_H
