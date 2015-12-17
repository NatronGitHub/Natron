#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QCoreApplication::setApplicationName("Breakdown");
    QCoreApplication::setApplicationVersion("0.1");
    QCoreApplication::setOrganizationDomain("natron.fr");
    QCoreApplication::setOrganizationName("breakdown");
    w.show();

    return a.exec();
}
