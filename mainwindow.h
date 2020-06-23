#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

    /* Private Functions */
    void on_languagesComboBox_currentIndexChanged(const QString& arg1);

    void on_compileButton_clicked();

private:
    void initConnections();
    /* Private Member Variables*/
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
