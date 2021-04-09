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
    void updateAsmTextEdit(const QByteArray& data);

    void on_languagesComboBox_currentIndexChanged(const QString& arg1);
    void openSettingsDialog();
    void on_compileButtonPress();
    void saveToFile();

private:
    void initConnections();
    void loadLocalCompilers();
    /* Private Member Variables*/
private:
    Ui::MainWindow *ui;
    QSplitter* split;

    // QWidget interface
};
#endif // MAINWINDOW_H
