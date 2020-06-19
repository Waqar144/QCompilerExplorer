#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "godboltsvc.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    GodboltSvc::instance(this)->sendRequest(QGodBolt::Endpoints::Languages);
}

MainWindow::~MainWindow()
{
    delete ui;
}

