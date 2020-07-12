#include "asmparser.h"

#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

#include <cxxabi.h>

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

    //1. collect all the labels
    //2. go through the asm line by line and check if the labels are used/unused
    //3. if the labels are unused, they get deleted
    //4. every line beginning with '.' gets deleted, unless it is a used label

    QRegularExpression exp{"^\\s*[_|a-zA-Z]"};

    while (!s.atEnd()) {
        QString line = s.readLine();
//        auto match = exp.match(line);
//        if (match.hasMatch()) {
            output.append(line + "\n");
//        }
//        bool isLFBDirective = line.startsWith(QLatin1String(".LFB"));
//        bool isLFEDirective = line.startsWith(QLatin1String(".LFE"));

//        if (isLFEDirective) {
//            isLFEBlock = true;
//        }

//        if (isLFEBlock) {
//            LFEcount++;
//            if (LFEcount == 4) {
//                isLFEBlock = false;
//            }
//            continue;
//        }

//        if (!isLFBDirective && !isLFEDirective && !isLFEBlock) {
//            if (line.startsWith(".LFB"))
//                qDebug () << line;
//            output.append(line + "\n");
//        }
    }

    return output;
}

QString AsmParser::demangle(QString &&asmText)
{
    int next = 0;
    int last = 0;
    while (next != -1) {
        next = asmText.indexOf("_Z", last);
        //get token
        if (next != -1) {
            int token = asmText.indexOf(QRegularExpression(":|,|@|\\[|\\s|\\n"), next + 1);
            int len = token - next;
            QStringRef tok = asmText.midRef(next, len);
            int status = 0;
            char* name = abi::__cxa_demangle(tok.toUtf8().constData(), 0, 0, &status);
            if (status != 0) {
                qDebug () << "Demangling of: " << tok << " failed, status: " << status;
                continue;
            }
            QString qName{name};
            qName.prepend(' ');
            asmText.replace(next, len, qName);
            free((void*)name);
        }
    }
    return std::move(asmText);
}
