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

    QFile cell_css_f(":/res/cell.css");
    cell_css_f.open(QFile::ReadOnly);
    QString cell_css = QString::fromUtf8(cell_css_f.readAll());
    cell_css_f.close();
    setStyleSheet(cell_css);

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

QString MainWindow::sym_repr(char32_t sym, bool useCodes, bool useQuotes)
{
    if (std::iswprint(sym)) {
        QString c = QString::fromUcs4(&sym, 1);
        if (!useQuotes)
            return c;
        return sym == '"' ? "'\"'" : QString("\"%1\"").arg(c);
    }

    if (useCodes)
        return QString("U+") + QString::number(sym, 16);
    return QString();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    updateCellCount();
}

void MainWindow::updateCellCount()
{
    int left, right;
    /*ui->gridLayout->getContentsMargins(&left, nullptr, &right, nullptr);
    int hspacing = ui->tape->spacing();
    int tapeWidth = size().width() - (left + right);
    int count = ui->tape->count(), newCount = (tapeWidth + hspacing - 1) / (Cell::CELL_SIZE + hspacing);*/
    //int tapeWidth = ui->tapeBox->width();
    ui->gridLayout->getContentsMargins(&left, nullptr, &right, nullptr);
    int tapeWidth = size().width() - (left + right);
    int hspacing = ui->tape->spacing();
    int count = ui->tape->count(), newCount = (tapeWidth + hspacing - 20) / (Cell::CELL_SIZE + hspacing);

    while (count < newCount) {
        auto *cell = new Cell(this);
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
    int index;
    int cellsCount = ui->tape->count();
    int carCellIndex = cellsCount / 2;

    char32_t symnull = emu.symnull();

    //qDebug() << loader.readTape();

    for (index = 0; index < cellsCount; ++index) {
        auto *cell = dynamic_cast<Cell*>(ui->tape->itemAt(index)->widget());
        cell->setText(sym_repr(symnull));
        cell->setSelected(false);
    }

    auto car = emu.carriage(), it = car;

    for (it = car, index = carCellIndex; ++it != tape.end() && ++index < cellsCount; ) {
        auto *cell = dynamic_cast<Cell*>(ui->tape->itemAt(index)->widget());
        cell->setText(sym_repr(*it));
    }

    dynamic_cast<Cell*>(ui->tape->itemAt(carCellIndex)->widget())->setSelected();

    for (it = car, index = carCellIndex; index >= 0; --it, --index) {
        auto *cell = dynamic_cast<Cell*>(ui->tape->itemAt(index)->widget());
        cell->setText(sym_repr(*it));

        if (it == tape.begin())
            break;
    }
}

void MainWindow::updateTable()
{
    return;

    const auto &states = emu.states();
    const auto &symbols = emu.symbols();

    ui->table->clear();
    ui->table->setColumnCount(states.size());
    ui->table->setRowCount(symbols.size());

    int index = 0;
    for (const uint32_t &state : states)
        ui->table->setHorizontalHeaderItem(index++, new QTableWidgetItem(QString::number(state)));

    index = 0;
    for (const uint32_t &symbol : symbols) {
        ui->table->setVerticalHeaderItem(index++, new QTableWidgetItem(sym_repr(symbol, true)));
    }


    int row = 0, column = 0;

    for (const uint32_t symbol : symbols) {
        column = 0;
        for (const uint32_t state : states) {
            auto *tr = emu.getRule(tur::emu::Condition{.state = state, .symbol = symbol});
            if (!tr) continue;


            ui->table->setCellWidget(row, column, new QLabel(
                QString("%1, %2, %3")
                      .arg(tr->state)
                      .arg(sym_repr(tr->symbol, true, true))
                      .arg((char)tr->direction),
                nullptr));
            ++column;
        }
        ++row;
    }
}

void MainWindow::updateCurrentState()
{
    ui->labelState->setText(QString::number(emu.state()));
}

void MainWindow::makeStep()
{
    if (status == NOTREADY || status == HALTED)
        return;

    try {
        emu.step();
    } catch (const tur::NoRuleError&) {
        QMessageBox::critical(this, tr("Error"), tr("Behaviour is not defined for current state"));
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
    } catch (const tur::ParseError &e) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to load program!"));
        return;
    }

    updateTable();
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

