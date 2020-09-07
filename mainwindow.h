#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>

class QListWidgetItem;

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
    void openSettingsDialog();
    void on_compilerComboBox_currentIndexChanged(const QString& arg1);
    void on_compileButtonPress();
    void on_localCheckbox_stateChanged(int arg1);
    void saveToFile();
    void selectedFileChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void onActionOpenFoldertriggered();

    void on_actionFileBrowser_triggered(bool checked);

private:
    void initConnections();
    void loadLocalCompilers();
    /* Private Member Variables*/
private:
    Ui::MainWindow *ui;
    QSplitter* split;
//    AsmHighlighter* asmHighlighter;

    // QWidget interface
};
#endif // MAINWINDOW_H
