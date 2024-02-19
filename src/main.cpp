#include <QApplication>
#include <QDebug>
#include <QChar>
#include <QFile>
#include <iterator>
#include "tur/emulator.hpp"
#include "tur/loader.hpp"
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
