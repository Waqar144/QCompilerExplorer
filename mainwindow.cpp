#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "compilerservice.h"
#include "settingsdialog.h"
#include "asmparser.h"
#include "compiler.h"

#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QTemporaryFile>

#include <cxxabi.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initConnections();
    split = new QSplitter();
    split->addWidget(ui->codeTextEdit);
    split->addWidget(ui->asmTextEdit);
    ui->centralwidget->layout()->addWidget(split);

    QSettings settings;
    auto isIntel = settings.value("intelSyntax").toBool();
    ui->isIntelSyntax->setChecked(isIntel);

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    bool isLocal = settings.value("localCE", true).toBool();
    if (!isLocal) {
        CompilerExplorerSvc::instance()->sendRequest(QCompilerExplorer::Endpoints::Languages);
    } else {
        ui->localCheckbox->setCheckState(Qt::Checked);
        ui->languagesComboBox->setDisabled(true);
        loadLocalCompilers();
    }
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("intelSyntax", ui->isIntelSyntax->isChecked());
    settings.setValue("localCE", ui->localCheckbox->isChecked());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    delete ui;
}

void MainWindow::setupLanguages(const QByteArray& data)
{
    QSettings settings;
    const QJsonArray json = QJsonDocument::fromJson(data).array();
    ui->languagesComboBox->blockSignals(true);
    for (const auto& value : json) {
        const auto lang = value["name"].toString();
        ui->languagesComboBox->addItem(lang, value["id"].toString());
    }
    ui->languagesComboBox->blockSignals(false);
    auto lang = settings.value(QStringLiteral("lastUsedLanguage")).toString();
    ui->languagesComboBox->setCurrentText(lang);
}

void MainWindow::updateCompilerComboBox(const QByteArray& data)
{
    QSettings settings;
    const QJsonArray json = QJsonDocument::fromJson(data).array();
    ui->compilerComboBox->blockSignals(true);
    for (const auto& value : json) {
        const auto compiler = value["name"].toString();
        ui->compilerComboBox->addItem(compiler, value["id"].toString());
    }
    ui->compilerComboBox->blockSignals(false);
    auto compiler = settings.value(QStringLiteral("lastUsedCompilerFor") + ui->languagesComboBox->currentText()).toString();
    ui->compilerComboBox->setCurrentText(compiler);
}

void MainWindow::updateAsmTextEdit(const QByteArray& data)
{
    const QJsonArray assembly = QJsonDocument::fromJson(data).object().value("asm").toArray();
    QString asmText;
    for (const auto& line : assembly) {
        asmText.append(line["text"].toString() + "\n");
    }
    ui->asmTextEdit->setPlainText(asmText);
}

void MainWindow::initConnections()
{
    connect(CompilerExplorerSvc::instance(), &CompilerExplorerSvc::languages, this, &MainWindow::setupLanguages);
    connect(CompilerExplorerSvc::instance(), &CompilerExplorerSvc::compilers, this, &MainWindow::updateCompilerComboBox);
    connect(CompilerExplorerSvc::instance(), &CompilerExplorerSvc::asmResult, this, &MainWindow::updateAsmTextEdit);

    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::openSettingsDialog);
    connect(ui->actionSave_asm_to_file, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->actionSave_code_to_file, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->compileButton, &QPushButton::clicked, this, &MainWindow::on_compileButtonPress);
    connect(ui->compileButton, &QPushButton::clicked, this, &MainWindow::on_compileButton_clicked);
}

void MainWindow::loadLocalCompilers()
{
    if (!ui->localCheckbox->isChecked())
        return;
    //clear previous things in combobox
    ui->compilerComboBox->clear();
    //check compiler availability and populate the combobox
    QString gpp = QStringLiteral("g++");
    QString clang = QStringLiteral("clang++");
    if (Compiler::isCompilerAvailable(gpp)) {
        const QString version = Compiler::getCompilerVersion(gpp);
        ui->compilerComboBox->addItem(gpp + QLatin1Char(' ') + version, gpp);
    }
    if (Compiler::isCompilerAvailable(clang)) {
        const QString version = Compiler::getCompilerVersion(clang);
        ui->compilerComboBox->addItem(clang + QLatin1Char(' ') + version, clang);
    }
}

void MainWindow::on_languagesComboBox_currentIndexChanged(const QString& arg1)
{
    Q_UNUSED(arg1)
    const QString language = ui->languagesComboBox->currentData().toString();
    const QString languageId = '/' + language;
    CompilerExplorerSvc::instance()->sendRequest(QCompilerExplorer::Endpoints::Compilers, languageId);
    ui->codeTextEdit->setCurrentLanguage(language);
    ui->compilerComboBox->clear();
    QSettings settings;
    settings.setValue("lastUsedLanguage", arg1);
}

void MainWindow::on_compileButton_clicked()
{
    if (ui->codeTextEdit->toPlainText().isEmpty())
        return;
    //on_compileButtonPress slot wilil be executed for local compilation
    if (ui->localCheckbox->isChecked()) {
        return;
    }
    const QString text = ui->codeTextEdit->toPlainText();
    const QString args = ui->argsLineEdit->text();
    bool isIntel = ui->isIntelSyntax->isChecked();
    auto data = CompilerExplorerSvc::getCompilationOptions(text, args, isIntel);

    QString endpoint = "compiler/" + ui->compilerComboBox->currentData().toString() + "/compile";
    CompilerExplorerSvc::instance()->compileRequest(endpoint, data.toJson());
}

void MainWindow::openSettingsDialog()
{
    SettingsDialog dialog(this);
    connect(&dialog, &SettingsDialog::fontChanged, ui->codeTextEdit, &QCodeEditor::updateFont);
    connect(&dialog, &SettingsDialog::fontChanged, ui->asmTextEdit, [this](const QString& f) {
        ui->asmTextEdit->setFont(QFont(f));
    });
    connect(&dialog, &SettingsDialog::fontSizeChanged, ui->asmTextEdit, [this](const qreal f) {
        QFont font = ui->asmTextEdit->font();
        font.setPointSize(f);
        ui->asmTextEdit->setFont(font);
    });
    connect(&dialog, &SettingsDialog::fontSizeChanged, ui->codeTextEdit, [this](const qreal f) {
        QFont font = ui->asmTextEdit->font();
        font.setPointSize(f);
        ui->codeTextEdit->setFont(font);
    });

    dialog.exec();
}

void MainWindow::on_compilerComboBox_currentIndexChanged(const QString& arg1)
{
    QSettings settings;
    int isLocal = ui->localCheckbox->checkState();
    if (isLocal != Qt::Checked) {
        settings.setValue("lastUsedCompilerFor" + ui->languagesComboBox->currentText(), arg1);
    }
}

void MainWindow::on_compileButtonPress()
{
    if (!ui->localCheckbox->isChecked())
        return;

    const QString source = ui->codeTextEdit->toPlainText();
    auto compilerName = ui->compilerComboBox->currentData().toString();
    const bool intelSyntax = ui->isIntelSyntax->isChecked();
    const QString args = ui->argsLineEdit->text();
    const QStringList argsList = [&args]() -> QStringList
    {
            if (!args.isEmpty()) {
                return args.split(QLatin1Char(' '));
            }
            return {};
    }();

    const Compiler compiler(std::move(compilerName));
    std::pair<QString, bool> out = compiler.compileToAsm(source, argsList, intelSyntax);

    if (out.second) {
        AsmParser p;
        QString demangled = p.demangle(std::move(out.first));
        QString cleanAsm = p.process(demangled.toUtf8());
        ui->asmTextEdit->setPlainText(cleanAsm);
    } else {
        ui->asmTextEdit->setPlainText("<Compilation Failed>\n" + out.first);
    }
}

void MainWindow::on_localCheckbox_stateChanged(int state)
{
    if (state == Qt::Checked) {
        ui->languagesComboBox->setDisabled(true);
        loadLocalCompilers();
    } else {
        ui->languagesComboBox->setDisabled(false);
        CompilerExplorerSvc::instance()->sendRequest(QCompilerExplorer::Endpoints::Languages);
    }
}

void MainWindow::saveToFile()
{
    auto action = sender();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As..."));
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    QTextStream s(&file);
    if (action->objectName() == "actionSave_asm_to_file") {
        s << ui->asmTextEdit->toPlainText();
    } else {
        s << ui->codeTextEdit->toPlainText();
    }
    file.close();
}
