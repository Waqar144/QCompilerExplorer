#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "compilerservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->asmTextEdit->setReadOnly(true);

    initConnections();
    setupCodeEditor();

    CompileSvc::instance()->sendRequest(QGodBolt::Endpoints::Languages);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupLanguages(const QByteArray& data)
{
    const QJsonArray json = QJsonDocument::fromJson(data).array();
    for (const auto& value : json) {
        ui->languagesComboBox->addItem(value["name"].toString(), value["id"].toString());
    }
}

void MainWindow::updateCompilerComboBox(const QByteArray& data)
{
    const QJsonArray json = QJsonDocument::fromJson(data).array();
    for (const auto& value : json) {
        ui->compilerComboBox->addItem(value["name"].toString(), value["id"].toString());
    }
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

void MainWindow::initConnections()
{
    connect(CompileSvc::instance(), &CompileSvc::languages, this, &MainWindow::setupLanguages);
    connect(CompileSvc::instance(), &CompileSvc::compilers, this, &MainWindow::updateCompilerComboBox);
    connect(CompileSvc::instance(), &CompileSvc::asmResult, this, &MainWindow::updateAsmTextEdit);
}

void MainWindow::setupCodeEditor()
{
    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPixelSize(12);
    ui->codeTextEdit->setFont(font);
    ui->codeTextEdit->setTabStopDistance(4 * QFontMetrics(font).horizontalAdvance(' '));
}

QJsonDocument MainWindow::getCompilationOptions(const QString& source, const QString& userArgs) const
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
    filterObj["intel"] = true;
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
    const QString languageId = '/' + ui->languagesComboBox->currentData().toString();
    CompileSvc::instance()->sendRequest(QGodBolt::Endpoints::Compilers, languageId);
    ui->compilerComboBox->clear();
}

void MainWindow::on_compileButton_clicked()
{
    if (ui->codeTextEdit->toPlainText().isEmpty())
        return;
    const QString text = ui->codeTextEdit->toPlainText();
    const QString args = ui->argsLineEdit->text();
    auto data = getCompilationOptions(text, args);

    //    qDebug() << data.toJson(QJsonDocument::JsonFormat::Compact);

    QString endpoint = "compiler/" + ui->compilerComboBox->currentData().toString() + "/compile";
    //    QString endpoint = "compiler/" + QString("g63") + "/compile";
    CompileSvc::instance()->compileRequest(endpoint, data.toJson());
}
