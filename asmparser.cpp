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

    //1. collect all the labels
    //2. go through the asm line by line and check if the labels are used/unused
    //3. if the labels are unused, they get deleted
    //4. every line beginning with '.' gets deleted, unless it is a used label

    QRegularExpression exp{"^\\s*[_|a-zA-Z]"};

    QRegularExpression directiveRe{"^\\s*\\..*$"};
    QRegularExpression labelRe{"^\\.*[a-zA-Z]+[0-9]+:$"};
    QRegularExpression hasOpcodeRe{"^\\s*[a-zA-Z]"};

    //<label, used>
    QHash<QString, bool> labels;

    //1
    while (!s.atEnd()) {
        QString line = s.readLine();
        if (labelRe.match(line).hasMatch()) {
            auto label = line.trimmed();
            label.chop(1); // remove ':'
            labels[label] = false;
        }
    }

    s.seek(0);

    //2
    while(!s.atEnd()) {
        QString line = s.readLine();
        if (hasOpcodeRe.match(line).hasMatch()) {
            QHashIterator<QString, bool> i(labels);
            while (i.hasNext()) {
                auto label = i.next();
                if (line.contains(label.key())) {
                    labels[label.key()] = true;
                }
            }
        }
    }

    //remove false labels from labels hash-map
    auto labelsCopy = labels;
    auto it = labelsCopy.constBegin();
    for (; it != labelsCopy.constEnd(); ++it) {
        if ( it.value() == false ) {
            labels.remove(it.key());
        }
    }

    s.seek(0);

    //3
    while(!s.atEnd()) {
        QString line = s.readLine();
        if (labelRe.match(line).hasMatch()) {
            auto l = line.trimmed();
            l.chop(1);
            if (labels.contains(l)) {
                output += line + "\n";
            }
            continue;
        }

        if (directiveRe.match(line).hasMatch()) {
                continue;
        }

        line.replace("\t", "\t\t");
        output += line + "\n";
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
