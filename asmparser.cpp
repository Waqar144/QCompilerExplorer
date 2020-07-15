#include "asmparser.h"

#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

#include <cxxabi.h>

static bool isOpcodeLen4orMore(const QString& line)
{
    int pos = line.indexOf(QLatin1Char('\t'));
    return pos > 3;
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

    QRegularExpression exp { QStringLiteral("^\\s*[_|a-zA-Z]") };

    QRegularExpression directiveRe { QStringLiteral("^\\s*\\..*$") };
    QRegularExpression labelRe { QStringLiteral("^\\.*[a-zA-Z]+[0-9]+:$") };
    QRegularExpression hasOpcodeRe { QStringLiteral("^\\s*[a-zA-Z]") };
    QRegularExpression numericLabelsRe { QStringLiteral("\\s*[0-9]:") };

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
        QString line = s.readLine().trimmed();

        if (labelRe.match(line).hasMatch()) {
            auto l = line;
            l.chop(1);
            if (labels.contains(l)) {
                output += line + "\n";
            }
            continue;
        }

        if (directiveRe.match(line).hasMatch()) {
            continue;
        }

        if (numericLabelsRe.match(line).hasMatch()) {
            continue;
        }

        if (line == QLatin1String("endbr64")) {
            continue;
        }

        if (line.endsWith(QLatin1Char(':'))) {
            output += line + '\n';
            continue;
        }

        if (!isOpcodeLen4orMore(line))
            line.replace(QLatin1Char('\t'), QStringLiteral("\t\t"));
        line.prepend(QLatin1Char('\t')).append('\n');

        output += line;
    }

    return output;
}

QString AsmParser::demangle(QString &&asmText)
{
    int next = 0;
    int last = 0;
    QRegularExpression nameEndRe { QStringLiteral(":|,|@|\\[|\\s|\\n") };
    while (next != -1) {
        next = asmText.indexOf(QLatin1String("_Z"), last);
        //get token
        if (next != -1) {
            int tokenEnd = asmText.indexOf(nameEndRe, next + 1);
            int len = tokenEnd - next;
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
