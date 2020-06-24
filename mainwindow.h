#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "QSourceHighlite/qsourcehighliter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void setupLanguages(const QByteArray& data);
    void updateCompilerComboBox(const QByteArray& data);
    void updateAsmTextEdit(const QByteArray& data);

    void on_languagesComboBox_currentIndexChanged(const QString& arg1);
    void on_compileButton_clicked();

    /* Private Functions */
private:
    void initConnections();
    void initCodeLangs();
    void setupCodeEditor();
    QJsonDocument getCompilationOptions(const QString &source, const QString &userArgs) const;
    /* Private Member Variables*/
private:
    QSourceHighlite::QSourceHighliter* highlighter;
    static QHash<QString, QSourceHighlite::QSourceHighliter::Language> _langStringToEnum;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
