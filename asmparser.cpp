#include "asmparser.h"

#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

#include <cxxabi.h>

static bool isOpcodeLen4orMore(const QString& line)
{
    int pos = line.indexOf(QLatin1Char('\t'));
    return pos > 4;
}

static int opcodeLen(const QString& line)
{
    int tabPos = line.indexOf(QLatin1Char('\t'));
    return tabPos;
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

    QRegularExpression directiveRe { QStringLiteral("^\\s*\\..*$") };
    QRegularExpression labelRe { QStringLiteral("^\\.*[a-zA-Z]+[0-9]+:$") };
    QRegularExpression hasOpcodeRe { QStringLiteral("^\\s*[a-zA-Z]") };
    QRegularExpression numericLabelsRe { QStringLiteral("\\s*[0-9]:") };

    const QVector<QString> allowedDirectives =
    {
        QStringLiteral(".string"),
        QStringLiteral(".ascii"),
        QStringLiteral(".zero"),
        QStringLiteral(".byte"),
        QStringLiteral(".value"),
        QStringLiteral(".long"),
        QStringLiteral(".quad")
    };

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

    int maxOpcodeLen = 4;

    QString currentLabel;

    //3
    while(!s.atEnd()) {
        QString line = s.readLine().trimmed();

        if (labelRe.match(line).hasMatch()) {
            auto l = line;
            l.chop(1);
            currentLabel = "";
            if (labels.contains(l)) {
                currentLabel = line;
                output += line + "\n";
            }
            continue;
        }

        if (directiveRe.match(line).hasMatch()) {
            //if we are in a label
            if (!currentLabel.isEmpty()) {
                for (const auto& allowed : allowedDirectives) {
                    if (line.trimmed().startsWith(allowed))
                        output += line + "\n";
                }
            }
            continue;
        }

        if (numericLabelsRe.match(line).hasMatch()) {
            continue;
        }

        if (line == QLatin1String("endbr64")) {
            continue;
        }

        if (line.endsWith(QLatin1Char(':'))) {
            currentLabel = line;
            output += line + '\n';
            continue;
        }

        auto len = opcodeLen(line);
        if (len > maxOpcodeLen)
            maxOpcodeLen = len;

        line.append('\n');

        output += line;
    }

    QTextStream finalOut(output.toUtf8());
    output.clear();
    while(!finalOut.atEnd()) {
        QString line = finalOut.readLine();

        //no indentation if it is a label
        if (line.endsWith(':') || line.contains(':')) {
            output.append(line).append('\n');
            continue;
        }

        if (maxOpcodeLen == 4) {
            line.replace(QLatin1Char('\t'), QStringLiteral("\t\t"));
        } else if (maxOpcodeLen > 4 && maxOpcodeLen < 8 && !isOpcodeLen4orMore(line)) {
            line.replace(QLatin1Char('\t'), QStringLiteral("\t\t"));
        } else if (maxOpcodeLen > 7 && opcodeLen(line) <= 4) {
            line.replace(QLatin1Char('\t'), QStringLiteral("\t\t\t"));
        } else if (maxOpcodeLen > 7 && opcodeLen(line) <= 7) {
            line.replace(QLatin1Char('\t'), QStringLiteral("\t\t"));
        }

        line.prepend('\t').append('\n');
        output.append(line);
    }

    return output;
}

QString AsmParser::demangle(QString &&asmText)
{
    int next = 0;
    int last = 0;

    while (next != -1) {
        next = asmText.indexOf(QLatin1String("_Z"), last);

        //get token
        if (next != -1) {
            last = next + 1;

            const auto it = std::find_if(asmText.cbegin() + last, asmText.cend(), [](const QChar c){
                return c == ':' || c == ',' || c == '[' || c== '@' || c == '(' ||
                        c == '+' || c == ' ' || c == '\n';
            });
            const int tokenEnd = std::distance(asmText.cbegin(), it);
            const int len = tokenEnd - next;
            const QStringRef tok = asmText.midRef(next, len);

            int status = 0;
            char* name = abi::__cxa_demangle(tok.toUtf8().constData(), 0, 0, &status);
            if (status != 0) {
                qWarning () << "Demangling of: " << tok << " failed, status: " << status;
                next = asmText.indexOf(QLatin1String("_Z"), last);
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
