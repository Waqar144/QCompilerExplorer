#ifndef TESTASMPARSER_H
#define TESTASMPARSER_H

#include <QObject>

class TestAsmParser : public QObject
{
    Q_OBJECT
public:
    explicit TestAsmParser(QObject *parent = nullptr);

private slots:
    void test1();
};

#endif // TESTASMPARSER_H
