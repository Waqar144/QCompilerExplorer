#ifndef ASMPARSER_H
#define ASMPARSER_H

class QString;
class QStringRef;

template <typename T>
class QVector;

template <typename K, typename V>
class QHash;

class AsmParser
{
public:
    QString process(const QString& asmText);
    static QString demangle(QString&& asmText);

private:
    QVector<QString> removeUnusedLabels(const QVector<QStringRef> &allLines, QHash<QStringRef, bool> &labels);

    int maxOpcodeLen = 4;
};

#endif // ASMPARSER_H
