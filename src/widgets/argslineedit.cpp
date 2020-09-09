#include "argslineedit.h"

#include <QCompleter>

ArgsLineEdit::ArgsLineEdit(QWidget *parent) : QLineEdit(parent)
{
    QStringList suggestions =
    {
        "-O3",
        "-O2",
        "-fomit-frame-pointer",
        "-Os",
    };
    m_argCompleter = new QCompleter(suggestions, this);
    this->setCompleter(m_argCompleter);
}


void ArgsLineEdit::focusInEvent(QFocusEvent *event)
{
    this->completer()->complete();
    return QLineEdit::focusInEvent(event);
}
