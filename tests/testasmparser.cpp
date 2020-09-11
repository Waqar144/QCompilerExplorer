#include "testasmparser.h"
#include "asmparser.h"

#include <QtTest>

TestAsmParser::TestAsmParser(QObject *parent) : QObject(parent) {}

void TestAsmParser::test1()
{
    QFile f{":/data/test1.s"};
    if (!f.open(QIODevice::ReadOnly))
        QFAIL("Failed to open file test1.s");
    auto before = f.readAll().trimmed();
    auto asmSrc = AsmParser().process(before).toUtf8().trimmed();

    QTextStream s{asmSrc};
    QTextStream d{before};
    while(!s.atEnd() && !d.atEnd())
        QCOMPARE(s.readLine().trimmed(), d.readLine().trimmed());
}


QTEST_MAIN(TestAsmParser)
