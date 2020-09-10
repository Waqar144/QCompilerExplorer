#include "asmparser.h"

#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <array>

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

static int getLineNumber(const QString& line)
{
    return line.splitRef(' ').at(2).toInt();
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
    QRegularExpression labelRe { QStringLiteral("^\\.*[a-zA-Z0-9]+[_]*[0-9]+:$") };
    QRegularExpression hasOpcodeRe { QStringLiteral("^\\s*[a-zA-Z]") };
    QRegularExpression numericLabelsRe { QStringLiteral("\\s*[0-9]:") };

    const std::array<QString, 7> allowedDirectives =
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
    int lineCount = 0;
    while(!s.atEnd()) {
        ++lineCount;
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

    QVector<QString> linesWithLoc;
    linesWithLoc.reserve(lineCount);
    QString currentLabel;
    //3
    int asmLines = 0;
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
            if (line.startsWith(".loc ")) {
                output += "\nSource Line: " + QString::number(getLineNumber(line)) + "\n";
                linesWithLoc.append(line);
            }
            //if we are in a label
            if (!currentLabel.isEmpty()) {
                for (const auto& allowed : allowedDirectives) {
                    if (line.trimmed().startsWith(allowed)) {
                        asmLines++;
                        linesWithLoc.append(line);
                        output += line + "\n";
                    }
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
            asmLines++;
            linesWithLoc.append(line);
            output += line + '\n';
            continue;
        }

        auto len = opcodeLen(line);
        if (len > maxOpcodeLen)
            maxOpcodeLen = len;

        asmLines++;
        linesWithLoc.append(line);
        line.append('\n');

        output += line;
    }

    //Collect corresponding source line numbers
    struct Data {
        int sourceLine;
        QVector<int> asmLines;
    };
    QVector<Data> data;
    for (int i = 0; i < linesWithLoc.size(); ++i) {
        if (linesWithLoc.at(i).startsWith(".loc")) {
            int sourceLineNum = getLineNumber(linesWithLoc.at(i));
            Data d;
            d.sourceLine = sourceLineNum;
            for (int j = i + 1; j <linesWithLoc.size(); j++) {
                if (linesWithLoc.at(j).startsWith(".loc")) {
                    break;
                }
                d.asmLines.append(j);
            }
            data.append(d);
        }
    }

    for (int i = 0; i < data.size(); ++i) {
        qDebug () << "For source line: " << data.at(i).sourceLine;
        qDebug () << "Asm lines are: " << data.at(i).asmLines;
        qDebug () << "--------------------";
    }

    QTextStream finalOut(output.toUtf8());
    output.clear();
    while(!finalOut.atEnd()) {
        QString line = finalOut.readLine();

        //no indentation if it is a label
        int colonPos = line.lastIndexOf(':');
        if (colonPos > -1) {
            //we are at the end of the line, it is a label
            if (colonPos >= line.length() - 1) {
                output.append(line).append('\n');
                continue;
            }

            //traverse the line and make sure it is a label
            //it can't be a label sometimes because there maybe a ':' inside the line
            bool isLabel = true;
            int i{};
            for (i = colonPos + 1; i < line.length(); ++i) {
                if (!line.at(i).isSpace()) {
                    isLabel = false;
                    break;
                }
            }

            //it is a label, but has a comment
            //this *may* result in a bug someday if there is a string with a ": #" pattern
            if (!isLabel && line.at(i) == '#') {
                output.append(line).append('\n');
                continue;
            }
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

        //remove @PLT
        line.replace(QStringLiteral("@PLT"), QLatin1String(""));

        line.prepend('\t').append('\n');
        output.append(line);
    }

    qDebug () << "Total lines: " << asmLines;

    qDebug () << "\nDone";

    QVector<int> lines;
    QTextStream l{asmText};
    while(!l.atEnd()) {
        QString line = l.readLine();
        if (line.trimmed().startsWith(".loc"))
            qDebug () << line;
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
