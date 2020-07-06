#ifndef ASMTEXTEDIT_H
#define ASMTEXTEDIT_H

#include <QPlainTextEdit>

#include "asmhighlighter.h"

class AsmTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    AsmTextEdit(QWidget* parent = nullptr);

    // QWidget interface
protected:
    void mouseMoveEvent(QMouseEvent* event);

private:
    AsmHighlighter* m_highlighter;
};

#endif // ASMTEXTEDIT_H
