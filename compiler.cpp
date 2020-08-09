#include "compiler.h"

#include <QFile>
#include <QProcess>
#include <QDebug>

Compiler::Compiler(QString compiler)
    : m_compiler{compiler}
{}

QStringList Compiler::getArgs(QStringList args)
{
    QStringList argsList{std::move(args)};
    argsList.append({"-S",                              //output assembly
                     "-fno-asynchronous-unwind-tables", //omit cfi directives
                     "-fno-dwarf2-cfi-asm"});
    return argsList;
}

std::pair<QString, bool> Compiler::compileToAsm(const QString &source,
                                                QStringList args,
                                                bool intelSyntax) const
{
    QString fileName {QStringLiteral("./x.cpp")};
    QFile f(fileName);
    if (f.open(QFile::ReadWrite | QFile::Truncate | QFile::Unbuffered)) {
        f.write(source.toUtf8());
        f.waitForBytesWritten(3000);
    }

    QStringList argsList = getArgs(std::move(args));
    if (intelSyntax) {
        argsList.append(QStringLiteral("-masm=intel"));
    }
    argsList.append(fileName);

    QProcess p;
    p.setProgram(m_compiler);
    p.setArguments(argsList);
    p.start();
    if (!p.waitForFinished()) {
        qWarning () << "Exit status: " <<  p.exitStatus();
        return {p.readAllStandardError(), false};
    }

    const QString error = p.readAllStandardError();

    if (!error.isEmpty()) {
        if (error.contains("error:")) {
            return {error, false};
        }
    }

    QFile file("./x.s");
    if (file.open(QFile::ReadOnly)) {
        return {file.readAll(), true};
    } else {
        qWarning () << "Failed to open the assembly output file!";
        return {file.errorString(), false};
    }
}
