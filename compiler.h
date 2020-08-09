#ifndef COMPILER_H
#define COMPILER_H

#include <QString>

class Compiler
{
public:
    Compiler(QString compiler);

    static QStringList getArgs(QStringList argsList);
    static QString getCompilerVersion(const QString& compiler);
    static bool isCompilerAvailable(const QString& compiler);

    std::pair<QString, bool> compileToAsm(const QString& source,
                                             QStringList args,
                                             bool intelSyntax) const;
private:
    QString m_compiler;
};

#endif // COMPILER_H
