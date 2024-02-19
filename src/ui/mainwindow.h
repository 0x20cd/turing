#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tur/emulator.hpp"
#include "tur/loader.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_buttonPlayPause_clicked();
    void on_buttonStep_clicked();
    void on_buttonReset_clicked();
    void on_actionOpen_triggered();
    void on_actionLoadTape_triggered();
    void on_actionSaveTape_triggered();

    void updateCellCount();
    void updateCellValues();

private:
    Ui::MainWindow *ui;
    tur::Emulator emu;
    tur::Loader loader;
    virtual void resizeEvent(QResizeEvent *event) override;
};

#endif // MAINWINDOW_H
