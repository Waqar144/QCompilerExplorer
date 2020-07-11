#ifndef ASMTEXTEDIT_H
#define ASMTEXTEDIT_H

#include "widgets/QCodeEditor.h"
#include <QPlainTextEdit>

#include "asmhighlighter.h"

class AsmTextEdit : public QCodeEditor
{
    Q_OBJECT
public:
    AsmTextEdit(QWidget* parent = nullptr);
    void setText(QString text);

    // QWidget interface
protected:
    void mouseMoveEvent(QMouseEvent* event);
};

#endif // ASMTEXTEDIT_H
