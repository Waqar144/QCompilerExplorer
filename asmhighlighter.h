#ifndef ASMHIGHLIGHTER_H
#define ASMHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>

class AsmHighlighter : public QSyntaxHighlighter
{
public:
    AsmHighlighter(QTextDocument* parent = nullptr);
    void highlightBlock(const QString& text) override;
};

#endif // ASMHIGHLIGHTER_H
