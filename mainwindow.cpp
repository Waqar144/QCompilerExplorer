#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "compilerservice.h"
#include "settingsdialog.h"
#include "asmparser.h"

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

    Qt::CheckState isLocal = static_cast<Qt::CheckState>(settings.value("localCE", true).toInt());
    if (isLocal != Qt::Checked) {
        CompileSvc::instance()->sendRequest(QGodBolt::Endpoints::Languages);
    } else {
        ui->localCheckbox->setCheckState(Qt::Checked);
        ui->languagesComboBox->setDisabled(true);
        loadLocalCompilers();
    }
}

MainWindow::~MainWindow()
{
    //save the value of intel syntax
    QSettings settings;
    settings.setValue("intelSyntax", ui->isIntelSyntax->isChecked());
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
    connect(CompileSvc::instance(), &CompileSvc::languages, this, &MainWindow::setupLanguages);
    connect(CompileSvc::instance(), &CompileSvc::compilers, this, &MainWindow::updateCompilerComboBox);
    connect(CompileSvc::instance(), &CompileSvc::asmResult, this, &MainWindow::updateAsmTextEdit);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::openSettingsDialog);
    connect(ui->actionSave_asm_to_file, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->actionSave_code_to_file, &QAction::triggered, this, &MainWindow::saveToFile);
}

static bool isCompilerAvailable(const QString& compiler)
{
    return QProcess::execute(compiler, {}) != -2;
}

static QString getCompilerVersion(const QString& compiler)
{
    QProcess p;
    p.start(compiler, {"--version"});
    if (!p.waitForFinished()) {
        qWarning () << "Error: " << p.errorString();
    }
    QString result = p.readAllStandardOutput();
    return result.split(QRegularExpression("\\s|\\n")).at(2);
}

void MainWindow::loadLocalCompilers()
{
    if (!ui->localCheckbox->isChecked())
        return;
    ui->compilerComboBox->clear();
    if (isCompilerAvailable("g++")) {
        const QString version = getCompilerVersion("g++");
        ui->compilerComboBox->addItem("g++ " + version, "g++");
    }
    if (isCompilerAvailable("clang++")) {
        const QString version = getCompilerVersion("clang++");
        ui->compilerComboBox->addItem("clang++ " + version, "clang++");
    }
}

QJsonDocument MainWindow::getCompilationOptions(const QString& source, const QString& userArgs, bool isIntel) const
{
    //opt obj
    QJsonObject optObj;
    optObj["userArguments"] = userArgs;

    //compiler options obj
    QJsonObject compilerObj;
    compilerObj["skipAsm"] = false;
    compilerObj["executorRequest"] = false;

    //add compileropts to opt obj
    optObj["compilerOptions"] = compilerObj;

    //filters
    QJsonObject filterObj;
    filterObj["binary"] = false;
    filterObj["commentOnly"] = true;
    filterObj["demangle"] = true;
    filterObj["directives"] = true;
    filterObj["intel"] = isIntel;
    filterObj["labels"] = true;
    filterObj["execute"] = false;

    optObj["filters"] = filterObj;

    QJsonObject main;
    //    main["source"] = "int sum(){ return 2 + 2; }";
    main["source"] = source;
    main["options"] = optObj;

    QJsonDocument doc { main };
    return doc;
}

void MainWindow::on_languagesComboBox_currentIndexChanged(const QString& arg1)
{
    Q_UNUSED(arg1)
    const QString language = ui->languagesComboBox->currentData().toString();
    const QString languageId = '/' + language;
    CompileSvc::instance()->sendRequest(QGodBolt::Endpoints::Compilers, languageId);
    ui->codeTextEdit->setCurrentLanguage(language);
    ui->compilerComboBox->clear();
    QSettings settings;
    settings.setValue("lastUsedLanguage", arg1);
}

void MainWindow::on_compileButton_clicked()
{
    if (ui->codeTextEdit->toPlainText().isEmpty())
        return;
    if (ui->localCheckbox->isChecked()) {
        on_compileButtonPress();
        return;
    }
    const QString text = ui->codeTextEdit->toPlainText();
    const QString args = ui->argsLineEdit->text();
    bool isIntel = ui->isIntelSyntax->isChecked();
    auto data = getCompilationOptions(text, args, isIntel);

    QString endpoint = "compiler/" + ui->compilerComboBox->currentData().toString() + "/compile";
    CompileSvc::instance()->compileRequest(endpoint, data.toJson());
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
    int isLocal = settings.value("localCE").toInt();
    if (isLocal != Qt::Checked) {
        settings.setValue("lastUsedCompilerFor" + ui->languagesComboBox->currentText(), arg1);
    }
}

void MainWindow::on_compileButtonPress()
{
    if (!ui->localCheckbox->isChecked())
        return;

    const QString source = ui->codeTextEdit->toPlainText();

    QFile f("./x.cpp");
    if (f.open(QFile::ReadWrite | QFile::Truncate | QFile::Unbuffered)) {
        f.write(source.toUtf8());
        f.waitForBytesWritten(3000);
    }

    QProcess p;
    auto compiler = ui->compilerComboBox->currentData().toString();
    p.setProgram(compiler);

    QString args = ui->argsLineEdit->text();
    QStringList argsList;
    if (!args.isEmpty())
        argsList = args.split(QLatin1Char(' '));

    argsList.append(QStringLiteral("-S"));
    if (ui->isIntelSyntax->isChecked()) {
        argsList.append(QStringLiteral("-masm=intel"));;
    }
    argsList.append({"-fno-asynchronous-unwind-tables",
                     "-fno-dwarf2-cfi-asm",
                     "./x.cpp"});

    p.setArguments(argsList);
    p.start();
    if (!p.waitForFinished()) {
        qWarning () << "Exit status: " <<  p.exitStatus();
        qWarning () << "Error: " << p.readAllStandardError();
        return;
    }

    const QString error = p.readAllStandardError();

    if (!error.isEmpty()) {
        qWarning () << error;
        qWarning () << p.error();
        if (error.contains("error:")) {
            ui->asmTextEdit->setPlainText("<compilation failed>\n" + error);
            return;
        }
    }

    QFile file("./x.s");
    if (file.open(QFile::ReadOnly)) {
        auto all = file.readAll();
        AsmParser p;
        QString demangled = p.demangle(std::move(all));
        QString cleanAsm = p.process(demangled.toUtf8());
        ui->asmTextEdit->setPlainText(cleanAsm);
    } else {
        qDebug () << "failed to open x.s";
    }
}

void MainWindow::on_localCheckbox_stateChanged(int state)
{
    QSettings settings;
    settings.setValue("localCE", state);
    if (state == Qt::Checked) {
        ui->languagesComboBox->setDisabled(true);
        loadLocalCompilers();
    } else {
        ui->languagesComboBox->setDisabled(false);
        CompileSvc::instance()->sendRequest(QGodBolt::Endpoints::Languages);
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
