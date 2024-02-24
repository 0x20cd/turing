#include <QApplication>
#include <QDebug>
#include <QChar>
#include <QFile>
#include <QTranslator>
#include <iterator>
#include "tur/emulator.hpp"
#include "tur/loader.hpp"
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale::system(), "turing_ru.qm")) {
        app.installTranslator(&qtTranslator);
    }

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
