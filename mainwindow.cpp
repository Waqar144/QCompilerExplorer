#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "compilerservice.h"
#include "settingsdialog.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSplitter>

//#include <iostream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->asmTextEdit->setReadOnly(true);

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

    setupAsmTextEdit();

    CompileSvc::instance()->sendRequest(QGodBolt::Endpoints::Languages);
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
    //    std::cout << "\n\nRecieved:\n"
    //              << data.toStdString() << "\n";
    const QJsonArray assembly = QJsonDocument::fromJson(data).object().value("asm").toArray();
    QString asmText;
    for (const auto& line : assembly) {
        asmText.append(line["text"].toString() + "\n");
    }
    //    qDebug() << asmText;
    ui->asmTextEdit->setPlainText(asmText);
}

void MainWindow::setupAsmTextEdit()
{
    //    asmHighlighter = new AsmHighlighter { ui->asmTextEdit->document() };
    //    QString asmm = "section .text\n\tstr: db \"hello\", 0\nsection .text\n\tglobal _start\n";
    //    asmm.append("_start:\n\tmov rax, 10\n\t");
    //    asmm.append("cmovb ebx, 10\n\t");
    //    asmm.append("add eax, 20\n\t");
    //    asmm.append("push rbp\n\t");
    //    asmm.append("mov rbp, rsp\n\t");
    //    asmm.append("ret\n\t");
    //    ui->asmTextEdit->setPlainText(asmm);
}

void MainWindow::initConnections()
{
    connect(CompileSvc::instance(), &CompileSvc::languages, this, &MainWindow::setupLanguages);
    connect(CompileSvc::instance(), &CompileSvc::compilers, this, &MainWindow::updateCompilerComboBox);
    connect(CompileSvc::instance(), &CompileSvc::asmResult, this, &MainWindow::updateAsmTextEdit);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::openSettingsDialog);
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
    //    compilerOptions = compilerObj;

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
    const QString text = ui->codeTextEdit->toPlainText();
    const QString args = ui->argsLineEdit->text();
    bool isIntel = ui->isIntelSyntax->isChecked();
    auto data = getCompilationOptions(text, args, isIntel);

    //    qDebug() << data.toJson(QJsonDocument::JsonFormat::Compact);

    QString endpoint = "compiler/" + ui->compilerComboBox->currentData().toString() + "/compile";
    //    QString endpoint = "compiler/" + QString("g63") + "/compile";
    CompileSvc::instance()->compileRequest(endpoint, data.toJson());
}

void MainWindow::openSettingsDialog()
{
    SettingsDialog dialog(this);
    connect(&dialog, &SettingsDialog::fontChanged, ui->codeTextEdit, &QCodeEditor::updateFont);
    //    connect(&dialog, &SettingsDialog::fontSizeChanged, ui->codeTextEdit, &QCodeEditor::updateFontSize);
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
    settings.setValue("lastUsedCompilerFor" + ui->languagesComboBox->currentText(), arg1);
}
