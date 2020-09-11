#include "asmparser.h"

#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

#include <cxxabi.h>

namespace {
QRegularExpression directiveRe { QStringLiteral("^\\s*\\..*$") };
QRegularExpression labelRe { QStringLiteral("^\\.*[a-zA-Z0-9]+[_]*[0-9]+:$") };
QRegularExpression hasOpcodeRe { QStringLiteral("^\\s*[a-zA-Z]") };
QRegularExpression numericLabelsRe { QStringLiteral("\\s*[0-9]:") };
}

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

QVector<QString> AsmParser::removeUnusedLabels(const QVector<QStringRef>& allLines,
                                      QHash<QStringRef, bool>& labels)
{
    static QString allowedDirectives[7] =
    {
        QStringLiteral(".string"),
        QStringLiteral(".ascii"),
        QStringLiteral(".zero"),
        QStringLiteral(".byte"),
        QStringLiteral(".value"),
        QStringLiteral(".long"),
        QStringLiteral(".quad")
    };

    QVector<QString> out;
    out.reserve(allLines.size());

    QString currentLabel;
    int prevLineNum = -1;
    //3 Remove unused labels from the asm
    for (const auto& line : allLines) {
        if (labelRe.match(line).hasMatch()) {
            auto l = line;
            l.chop(1);
            currentLabel = "";
            if (labels.contains(l)) {
                currentLabel = line.toString();
                out.append(line.toString());
            }
            continue;
        }

        if (directiveRe.match(line).hasMatch()) {
            if (line.trimmed().startsWith(".loc ")) {
                int lineNum = getLineNumber(line);
                if (prevLineNum != lineNum) {
                    out.append("\nSource Line: " + QString::number(lineNum));
                    prevLineNum = lineNum;
                }
            }
            //if we are in a label
            if (!currentLabel.isEmpty()) {
                for (const auto& allowed : allowedDirectives) {
                    if (line.trimmed().startsWith(allowed)) {
                        out.append(line.toString());
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
            out.append(line.toString());
            continue;
        }

        auto len = opcodeLen(line);
        if (len > maxOpcodeLen)
            maxOpcodeLen = len;

        out.append(line.toString());
    }
    return out;
}


/**
 * @brief merges the following:
 * Source Line: 4
 * Source Line: 5
 * into:
 * Source Line: 4 - 5
 * @param cleanedLines
 */
static void mergeAnnotatedLineNums(QVector<QString>& lines)
{
    for (int i = 0; i < lines.size(); ++i) {
        if (lines.at(i).startsWith("\t\nSource Line: ")) {
            QString cur = lines.at(i);
            int j;
            for (j = i + 1; j < lines.size(); ++j) {
                if (lines.at(j) != '\t' && !lines.at(j).startsWith("\t\nSource Line: "))
                    break;
            }
            if (j > 0) --j;
            if (j != i && lines.at(j).startsWith("\t\nSource Line: ")) {
                int numPos = lines.at(j).lastIndexOf(' ') + 1;
                if (numPos == - 1)
                    continue;
                int num = lines.at(j).midRef(numPos, lines.at(j).size()).toInt();
                lines[i].append(" - " + QString::number(num));
            }
            //collapse all the lines from i -> j
            for (int k = i + 1; k < j + 1; ++k)
                lines[k] = "";
        }
    }
    lines.removeAll("");
}

QString AsmParser::process(const QString &asmText)
{
    //1. collect all the labels
    //2. go through the asm line by line and check if the labels are used/unused
    //3. if the labels are unused, they get deleted
    //4. every line beginning with '.' gets deleted, unless it is in a label

    const QVector<QStringRef> allLinesTemp = asmText.splitRef('\n');
    //trim all Lines
    QVector<QStringRef> allLines(allLinesTemp.size());
    for (int i = 0; i < allLinesTemp.size(); ++i){
        allLines[i] = allLinesTemp.at(i).trimmed();
    }

    //1 collect all the labels
    //<label, used>
    LabelsHashmap labels = collectAllLabels(allLines);

    //2 go through the asm line by line and check if the labels are used/unused
    markLabelsUsed(allLines, labels);

    //remove unused labels from labels hash-map
    auto it = labels.begin();
    while (it != labels.end()) {
        if (it.value() == false)
            labels.erase(it++);
        else
            ++it;
    }

    //3 if the labels are unused, they get deleted
    //4. every line beginning with '.' gets deleted, unless it is in a label
    auto cleanedLines = removeUnusedLabels(allLines, labels);

    //We are done, lets clean up / fix indentation
    for (QString& line : cleanedLines) {
        //no indentation if it is a label
        int colonPos = line.lastIndexOf(':');
        if (colonPos > -1) {
            //we are at the end of the line, it is a label
            if (colonPos >= line.length() - 1) {
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

    mergeAnnotatedLineNums(cleanedLines);

    QString output;
    output.reserve(cleanedLines.size() * cleanedLines.at(0).size());
    for (auto&& line : cleanedLines)
    {
        line += '\n';
        output.append(std::move(line));
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
