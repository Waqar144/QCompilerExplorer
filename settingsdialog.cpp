#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    QSettings settings;
    ui->fontComboBox->setCurrentText(settings.value(QStringLiteral("font")).toString());
    ui->spinBox->setValue(settings.value(QStringLiteral("fontSize"), 12).toInt());
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

void SettingsDialog::on_SettingsDialog_accepted()
{
    QSettings settings;
    settings.setValue(QStringLiteral("fontSize"), ui->spinBox->value());
    settings.setValue(QStringLiteral("font"), ui->fontComboBox->currentFont().family());
}
