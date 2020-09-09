#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

Q_SIGNALS:
    void fontChanged(const QString& fontName);
    void fontSizeChanged(qreal fontSize);

private slots:
    void on_fontComboBox_currentFontChanged(const QFont& f);

    void on_spinBox_valueChanged(int arg1);

    void on_SettingsDialog_accepted();

    void on_defaultPathBrowseButton_clicked();

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
