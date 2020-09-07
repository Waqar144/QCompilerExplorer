#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName(QString("Waqar144"));
    a.setApplicationName("QCompilerExplorer");
    a.setWindowIcon(QIcon(":/icon.ico"));

    MainWindow w;
    w.show();
    return a.exec();
}
