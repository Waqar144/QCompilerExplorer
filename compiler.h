#ifndef COMPILER_H
#define COMPILER_H

#include <QString>

class Compiler
{
public:
    Compiler(QString compiler);
    static QStringList getArgs(QStringList argsList);
    std::pair<QString, bool> compileToAsm(const QString& source,
                                             QStringList args,
                                             bool intelSyntax) const;
private:
    QString m_compiler;
};

#endif // COMPILER_H
