#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QIcon>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QShortcut>
#include "tur/emulator.hpp"
#include "tur/loader.hpp"
#include "tur/utils.hpp"

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
    void on_actionLoadTape_triggered();
    void on_actionSaveTape_triggered();
    void on_speedSlider_valueChanged(int value);

    void updateCellCount();
    void updateCellValues();
    void updateCurrentState();
    void updateLabelStatus();
    void makeStep();
    void setStatus(Status status);
    bool loadProgram();
    void onNewFile();
    void onOpenProgram();
    bool onSaveProgram(bool is_save_as = false);
    bool saveBefore();
    void onTextChanged();

    void closeEvent(QCloseEvent *event) override;

private:
    static const int T_NORMAL_MS, T_MIN_MS, T_MAX_MS;
    float basePow;
    Ui::MainWindow *ui;
    tur::Emulator emu;
    tur::Loader loader;

    virtual void resizeEvent(QResizeEvent *event) override;
    QLabel *labelStatus;
    QTimer stepTimer;
    QIcon playIcon, pauseIcon;
    QString loadedTape, filename;
    Status status;
    bool is_table_uptodate, is_changes_unsaved;
};

#endif // MAINWINDOW_H
