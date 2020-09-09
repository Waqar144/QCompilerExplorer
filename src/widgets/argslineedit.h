#ifndef ARGSLINEEDIT_H
#define ARGSLINEEDIT_H

#include <QLineEdit>


class ArgsLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    ArgsLineEdit(QWidget* parent = nullptr);

private:
    QCompleter* m_argCompleter;

    // QWidget interface
protected:
    void focusInEvent(QFocusEvent *event) override;
};

#endif // ARGSLINEEDIT_H
