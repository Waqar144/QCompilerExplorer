#include "asmparser.h"

#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <array>

#include <cxxabi.h>

using LabelsHashmap = QHash<QStringRef, bool>;

static bool isOpcodeLen4orMore(const QString& line)
{
    int pos = line.indexOf(QLatin1Char('\t'));
    return pos > 4;
}

static int opcodeLen(const QStringRef& line)
{
    int tabPos = line.indexOf(QLatin1Char('\t'));
    return tabPos;
}

static int getLineNumber(const QStringRef& line)
{
    return line.split(' ').at(2).toInt();
}

static LabelsHashmap collectAllLabels(const QVector<QStringRef>& lines)
{
    static QRegularExpression labelRe { QStringLiteral("^\\.*[a-zA-Z0-9]+[_]*[0-9]+:$") };
    LabelsHashmap labels;
    for (const auto& line : lines) {
        if (labelRe.match(line).hasMatch()) {
            auto label = line.trimmed();
            label.chop(1); // remove ':'
            labels[label] = false;
        }
    }
    return labels;
}

static void markLabelsUsed(const QVector<QStringRef>& lines, LabelsHashmap& allLabels)
{
    static QRegularExpression hasOpcodeRe { QStringLiteral("^\\s*[a-zA-Z]") };
    for (const auto& line : lines) {
        if (hasOpcodeRe.match(line).hasMatch()) {
            QHashIterator<QStringRef, bool> i(allLabels);
            while (i.hasNext()) {
                auto label = i.next();
                if (line.contains(label.key())) {
                    allLabels[label.key()] = true;
                }
            }
        }
    }
}

QString AsmParser::process(const QString &asmText)
{
    QString output;
    output.reserve(asmText.length());

    //1. collect all the labels
    //2. go through the asm line by line and check if the labels are used/unused
    //3. if the labels are unused, they get deleted
    //4. every line beginning with '.' gets deleted, unless it is in a label

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

    const QVector<QStringRef> allLinesTemp = asmText.splitRef('\n');
    //trim all Lines
    QVector<QStringRef> allLines(allLinesTemp.size());
    for (int i = 0; i < allLinesTemp.size(); ++i){
        allLines[i] = allLinesTemp.at(i).trimmed();
    }

    //1
    //<label, used>
    QHash<QStringRef, bool> labels = collectAllLabels(allLines);

    //2
    markLabelsUsed(allLines, labels);

    //remove unused labels from labels hash-map
    auto it = labels.begin();
    while (it != labels.end()) {
        if (it.value() == false)
            labels.erase(it++);
        else
            ++it;
    }

    int maxOpcodeLen = 4;

    QString currentLabel;
    //3 Remove unused labels from the asm
    for (const auto& line : allLines) {
        if (labelRe.match(line).hasMatch()) {
            auto l = line;
            l.chop(1);
            currentLabel = "";
            if (labels.contains(l)) {
                currentLabel = line.toString();
                output += line + "\n";
            }
            continue;
        }

        if (directiveRe.match(line).hasMatch()) {
            if (line.trimmed().startsWith(".loc ")) {
                output += "\nSource Line: " + QString::number(getLineNumber(line)) + "\n";
            }
            //if we are in a label
            if (!currentLabel.isEmpty()) {
                for (const auto& allowed : allowedDirectives) {
                    if (line.trimmed().startsWith(allowed)) {
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
            currentLabel = line.toString();
            output += line + '\n';
            continue;
        }

        auto len = opcodeLen(line);
        if (len > maxOpcodeLen)
            maxOpcodeLen = len;

        output += line + '\n';
    }
    //We are done, lets clean up

    QStringList cleanedLines = output.split('\n');
    output.clear();
    for (QString& line : cleanedLines) {
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
        } else if (maxOpcodeLen > 7 && opcodeLen(line.midRef(0)) <= 4) {
            line.replace(QLatin1Char('\t'), QStringLiteral("\t\t\t"));
        } else if (maxOpcodeLen > 7 && opcodeLen(line.midRef(0)) <= 7) {
            line.replace(QLatin1Char('\t'), QStringLiteral("\t\t"));
        }

        //remove @PLT
        line.replace(QStringLiteral("@PLT"), QLatin1String(""));
        line.prepend('\t');
    }
    output = cleanedLines.join('\n');

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
