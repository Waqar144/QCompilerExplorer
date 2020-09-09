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
    // QWidget interface
protected:
    void mouseMoveEvent(QMouseEvent* event);

private:
    QString getCurrentWordUnderCursor();
};

#endif // ASMTEXTEDIT_H
