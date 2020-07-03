#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_fontComboBox_currentFontChanged(const QFont& f)
{
    emit fontChanged(f.family());
}

void SettingsDialog::on_spinBox_valueChanged(int arg1)
{
    emit fontSizeChanged(static_cast<qreal>(arg1));
}
