#include "compiler.h"

#include <QFile>
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>

Compiler::Compiler(QString compiler)
    : m_compiler{std::move(compiler)}
{}

/**
 * @brief Takes the args set by user in the lineEdit and appends some default args
 * @param args
 * @return list of args
 */
static QStringList getArgs(QStringList args)
{
    QStringList argsList{std::move(args)};
    argsList.append({"-S",                              //output assembly
                     "-fno-asynchronous-unwind-tables", //omit cfi directives
                     "-fno-dwarf2-cfi-asm",
                    "-fno-stack-protector"});
    return argsList;
}


/**
 * @brief checks if the `compiler` is available on your computer
 * @param compiler - name of the compiler g++ / clang++
 * @return true if the compiler was found, false otherwise
 */
bool Compiler::isCompilerAvailable(const QString& compiler)
{
    return QProcess::execute(compiler, {}) != -2;
}

/**
 * @brief Gets the version for the compiler
 * @param compiler - name of the compiler g++ / clang++
 * @return version number
 */
QString Compiler::getCompilerVersion(const QString& compiler)
{
    QProcess p;
    p.start(compiler, {"--version"});
    if (!p.waitForFinished()) {
        qWarning () << "Error: " << p.errorString();
    }
    QString result = p.readAllStandardOutput();
    return result.split(QRegularExpression("\\s|\\n")).at(2);
}

/**
 * @brief Takes source code as argument and then compiles it to Asm. Compiler wil be whatever
 *        was selected in the combobox.
 * @param source
 * @param args - arguments that will be passed to compiler, i.e. O3 etc
 * @param intelSyntax - intel or AT&T syntax
 * @return Pair of "Asm Source" and success. If it failed, the string will be error string
 *         instead of the asm source
 */
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
