#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <cmath>
#include <cwctype>
#include "mainwindow.h"
#include "cell.h"
#include "ui_mainwindow.h"

const int MainWindow::T_NORMAL_MS = 1000;
const int MainWindow::T_MIN_MS = 0;
const int MainWindow::T_MAX_MS = 3000;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , loader(emu)
    , labelStatus(new QLabel(this))
    , playIcon(":/res/play.png")
    , pauseIcon(":/res/pause.png")
    , status(NOTREADY)
{
    ui->setupUi(this);

    basePow = (1.0 * (T_MAX_MS - T_MIN_MS) / (T_NORMAL_MS - T_MIN_MS) - 1.0);
    basePow *= basePow;

    ui->speedSlider->setMinimum(0);
    ui->speedSlider->setMaximum(10000);
    ui->speedSlider->setValue(5000);

    statusBar()->addWidget(labelStatus);

    setStatus(NOTREADY);

    QObject::connect(&stepTimer, &QTimer::timeout, this, &MainWindow::makeStep);
    QObject::connect(ui->buttonStep, &QPushButton::clicked, this, &MainWindow::makeStep);
    QObject::connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    updateCellCount();
}

void MainWindow::updateCellCount()
{
    int left, right;
    ui->gridLayout->getContentsMargins(&left, nullptr, &right, nullptr);
    int tapeWidth = size().width() - (left + right);
    int hspacing = ui->tape->spacing();
    int count = ui->tape->count(), newCount = (tapeWidth + hspacing - 20) / (Cell::CELL_SIZE + hspacing);

    while (count < newCount) {
        auto *cell = new Cell(this);
        QObject::connect(cell, &QPushButton::clicked, [this, cell](){
            this->emu.moveCarriage(cell->diff);
            this->updateCellValues();
        });
        ui->tape->addWidget(cell);
        ++count;
    }

    for (int index = count - 1; index >= newCount; --index) {
        auto *item = ui->tape->itemAt(index);
        ui->tape->removeItem(item);
        delete item->widget();
        delete item;
    }

    updateCellValues();
}

void MainWindow::updateCellValues()
{
    auto &tape = emu.tape();
    int index, diff;
    int cellsCount = ui->tape->count();
    int carCellIndex = cellsCount / 2;

    auto symnull = emu.symnull();

    for (index = 0; index < cellsCount; ++index) {
        auto *cell = dynamic_cast<Cell*>(ui->tape->itemAt(index)->widget());
        cell->setValue(symnull, this->emu.alph());
        cell->setSelected(false);
    }

    auto car = emu.carriage(), it = car;

    for (it = car, index = carCellIndex, diff = 0; ++it != tape.end() && ++index < cellsCount; ) {
        auto *cell = dynamic_cast<Cell*>(ui->tape->itemAt(index)->widget());
        cell->setValue(*it, this->emu.alph());
    }

    dynamic_cast<Cell*>(ui->tape->itemAt(carCellIndex)->widget())->setSelected(true);

    for (it = car, index = carCellIndex, diff = 0; index >= 0; --it, --index) {
        auto *cell = dynamic_cast<Cell*>(ui->tape->itemAt(index)->widget());
        cell->setValue(*it, this->emu.alph());

        if (it == tape.begin())
            break;
    }

    for (index = 0, diff = -carCellIndex; index < cellsCount; ++index, ++diff){
        auto *cell = dynamic_cast<Cell*>(ui->tape->itemAt(index)->widget());
        cell->diff = diff;
    }
}

void MainWindow::updateCurrentState()
{
    ui->labelState->setText(
        tur::utils::stateToString(emu.state(), this->emu.states())
    );
}

void MainWindow::makeStep()
{
    if (status == NOTREADY || status == HALTED)
        return;

    try {
        emu.step();
    } catch (const tur::NoRuleError &e) {
        QMessageBox::critical(
            this,
            tr("Error"),
            tr("Behaviour is not defined for state %1 and symbol %2")
                .arg(e.state)
                .arg(e.symbol)
        );
        setStatus(HALTED);
        return;
    }

    if (status != RUNNING)
        setStatus(PAUSED);

    if (emu.state() == tur::emu::STATE_END)
        setStatus(HALTED);

    if (status != RUNNING || this->stepTimer.interval() > T_MIN_MS) {
        updateCellValues();
        updateCurrentState();
    }
}

void MainWindow::setStatus(Status status)
{
    if (status == RUNNING) {
        stepTimer.start();
        ui->buttonPlayPause->setIcon(pauseIcon);
    } else {
        stepTimer.stop();
        ui->buttonPlayPause->setIcon(playIcon);
    }

    if (status == NOTREADY) {
        ui->buttonPlayPause->setEnabled(false);
        ui->buttonReset->setEnabled(false);
        ui->buttonStep->setEnabled(false);
    } else {
        ui->buttonPlayPause->setEnabled(status != HALTED);
        ui->buttonReset->setEnabled(status != RUNNING);
        ui->buttonStep->setEnabled(status != RUNNING && status != HALTED);
    }

    ui->actionLoadProgram->setEnabled(status != RUNNING && status != PAUSED);
    ui->actionLoadTape->setEnabled(status != RUNNING && status != PAUSED);
    ui->actionSaveTape->setEnabled(status != RUNNING);

    this->status = status;
}


void MainWindow::on_buttonPlayPause_clicked()
{
    setStatus(this->status == RUNNING ? PAUSED : RUNNING);
}


void MainWindow::on_buttonReset_clicked()
{
    emu.reset();
    if (this->status != READY) {
        loader.loadTape(this->loadedTape);
    } else {
        this->loadedTape.clear();
    }

    setStatus(READY);

    updateCellValues();
    updateCurrentState();
}


void MainWindow::on_actionLoadProgram_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this);
    if (filename.isNull())
        return;

    QFile file(filename);
    file.open(QFile::ReadOnly);
    QString source = QString::fromUtf8(file.readAll());
    file.close();

    try {
        this->loader.loadTable(source, false);
    } catch (const tur::CommonError &e) {
        QMessageBox::critical(
            this, tr("Error"),
            QString{"Row %1, column %2:\n%3"}
                .arg(e.srcRef.row)
                .arg(e.srcRef.col)
                .arg(e.msg)
        );
        return;
    }

    updateCellValues();
    updateCurrentState();

    setStatus(READY);
    labelStatus->setText(filename);
}


void MainWindow::on_actionLoadTape_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this);
    if (filename.isNull())
        return;

    QFile file(filename);
    file.open(QFile::ReadOnly);
    this->loadedTape = QString::fromUtf8(file.readAll());
    file.close();

    loader.loadTape(this->loadedTape);
    updateCellValues();
}


void MainWindow::on_actionSaveTape_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this);
    if (filename.isNull())
        return;

    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.write(loader.readTape().toUtf8());
    file.close();
}


void MainWindow::on_speedSlider_valueChanged(int value)
{
    double expPow = 1 - 1.0 * value / ui->speedSlider->maximum();
    double t = 1.0 * (T_MAX_MS - T_MIN_MS) / (basePow - 1) * (std::pow(basePow, expPow) - 1) + T_MIN_MS;
    stepTimer.setInterval((int)t);
}

