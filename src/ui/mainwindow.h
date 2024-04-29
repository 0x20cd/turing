#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QIcon>
#include <QTableWidget>
#include <QTableWidgetItem>
#include "tur/emulator.hpp"
#include "tur/loader.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

enum Status {NOTREADY, READY, RUNNING, PAUSED, HALTED};

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_buttonPlayPause_clicked();
    void on_buttonReset_clicked();
    void on_actionLoadProgram_triggered();
    void on_actionLoadTape_triggered();
    void on_actionSaveTape_triggered();
    void on_speedSlider_valueChanged(int value);

    void updateCellCount();
    void updateCellValues();
    void updateCurrentState();
    void makeStep();
    void setStatus(Status status);


private:
    static const int T_NORMAL_MS, T_MIN_MS, T_MAX_MS;
    float basePow;
    Ui::MainWindow *ui;
    tur::Emulator emu;
    tur::Loader loader;
//    TableModel tableModel;
    virtual void resizeEvent(QResizeEvent *event) override;
    static QString sym_repr(char32_t sym, bool useCodes = false, bool useQuotes = false);
    QLabel *labelStatus;
    QTimer stepTimer;
    QIcon playIcon, pauseIcon;
    QString loadedTape;
    Status status;
};

#endif // MAINWINDOW_H
