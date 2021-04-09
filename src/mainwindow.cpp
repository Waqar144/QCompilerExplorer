#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    ui->horizontalLayout_4->layout()->addWidget(split);

    setWindowIcon(QIcon{":/qce.png"});

    QSettings settings;
    const auto isIntel = settings.value(QStringLiteral("intelSyntax")).toBool();
    ui->isIntelSyntax->setChecked(isIntel);

    restoreGeometry(settings.value(QStringLiteral("geometry")).toByteArray());
    restoreState(settings.value(QStringLiteral("windowState")).toByteArray());

    ui->languagesComboBox->setDisabled(true);
    loadLocalCompilers();
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue(QStringLiteral("intelSyntax"), ui->isIntelSyntax->isChecked());
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.setValue(QStringLiteral("windowState"), saveState());
    delete ui;
}

void MainWindow::updateAsmTextEdit(const QByteArray& data)
{
    const QJsonArray assembly = QJsonDocument::fromJson(data).object().value(QStringLiteral("asm")).toArray();
    QString asmText;
    asmText.reserve(assembly.size());
    for (const auto& line : assembly) {
        asmText.append(line["text"].toString() + QLatin1Char('\n'));
    }
    ui->asmTextEdit->setPlainText(asmText);
}

void MainWindow::initConnections()
{
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::openSettingsDialog);
    connect(ui->actionSave_asm_to_file, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->actionSave_code_to_file, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->compileButton, &QPushButton::clicked, this, &MainWindow::on_compileButtonPress);
}

void MainWindow::loadLocalCompilers()
{
    //clear previous things in combobox
    ui->compilerComboBox->clear();
    //check compiler availability and populate the combobox
    const QString gpp{QStringLiteral("g++")};
    const QString clang{QStringLiteral("clang++")};
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
    const QString languageId = QLatin1Char('/') + language;
    ui->codeTextEdit->setCurrentLanguage(language);
    ui->compilerComboBox->clear();
    QSettings().setValue(QStringLiteral("lastUsedLanguage"), arg1);
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

void MainWindow::on_compileButtonPress()
{
    const QString source = ui->codeTextEdit->toPlainText();
    const auto compilerName = ui->compilerComboBox->currentData().toString();
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
    const QString currentFile = QString();

    std::pair<QString, bool> out = compiler.compileToAsm(source, argsList, intelSyntax, currentFile);

    if (out.second) {
        QString demangled = AsmParser::demangle(std::move(out.first));
        const QString cleanAsm = AsmParser().process(demangled);
        ui->asmTextEdit->setPlainText(cleanAsm);
    } else {
        ui->asmTextEdit->setPlainText(QStringLiteral("<Compilation Failed>\n") + out.first);
    }
}

void MainWindow::saveToFile()
{
    auto action = sender();
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save As..."));
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    QTextStream s(&file);
    if (action->objectName() == QStringLiteral("actionSave_asm_to_file")) {
        s << ui->asmTextEdit->toPlainText();
    } else {
        s << ui->codeTextEdit->toPlainText();
    }
    file.close();
}
