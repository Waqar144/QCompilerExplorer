#include "asmparser.h"

#include <QTextStream>
#include <QDebug>

AsmParser::AsmParser()
{

}

QString AsmParser::process(const QByteArray &asmText)
{
    QString output;
    output.reserve(asmText.length());
    QTextStream s(asmText);

//    //skip first 6 lines
//    for (int i = 0; i < 6 && !s.atEnd(); ++i)
//        s.readLine();

    int LFEcount = 0;
    bool isLFEBlock = false;

    while (!s.atEnd()) {
        QString line = s.readLine();
        bool isLFBDirective = line.startsWith(QLatin1String(".LFB"));
        bool isLFEDirective = line.startsWith(QLatin1String(".LFE"));

        if (isLFEDirective) {
            isLFEBlock = true;
        }

        if (isLFEBlock) {
            LFEcount++;
            if (LFEcount == 4) {
                isLFEBlock = false;
            }
            continue;
        }

        if (!isLFBDirective && !isLFEDirective && !isLFEBlock) {
            output.append(line + "\n");
        }
    }

    return output;
}
