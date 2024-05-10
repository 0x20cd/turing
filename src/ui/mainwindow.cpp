#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <cmath>
#include <cwctype>
#include <QTextBlock>
#include "mainwindow.h"
#include "cell.h"
#include "ui_mainwindow.h"
#include "edittapedialog.h"

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
    , status(READY)
    , is_table_uptodate(false)
    , is_changes_unsaved(false)
{
    ui->setupUi(this);

    basePow = (1.0 * (T_MAX_MS - T_MIN_MS) / (T_NORMAL_MS - T_MIN_MS) - 1.0);
    basePow *= basePow;

    ui->speedSlider->setMinimum(0);
    ui->speedSlider->setMaximum(10000);
    ui->speedSlider->setValue(5000);

    statusBar()->addWidget(labelStatus);

    setStatus(READY);
    updateLabelStatus();

    QObject::connect(&stepTimer, &QTimer::timeout, this, &MainWindow::makeStep);
    QObject::connect(ui->buttonStep, &QPushButton::clicked, this, &MainWindow::makeStep);

    QObject::connect(ui->actionNew, &QAction::triggered, this, &MainWindow::onNewFile);
    QObject::connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenProgram);
    QObject::connect(ui->actionSave, &QAction::triggered, [this](){ this->onSaveProgram(); });
    QObject::connect(ui->actionSaveAs, &QAction::triggered, [this](){ this->onSaveProgram(true); });
    QObject::connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);

    QObject::connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateLabelStatus);
    QObject::connect(ui->textEdit, &QPlainTextEdit::textChanged, this, &MainWindow::onTextChanged);
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

void MainWindow::updateLabelStatus()
{
    auto cursorPos = ui->textEdit->textCursor();
    labelStatus->setText(tr("%1 | Row %2, Column %3")
        .arg(this->filename + (this->is_changes_unsaved ? "*" : QString{}))
        .arg(cursorPos.block().blockNumber() + 1)
        .arg(cursorPos.positionInBlock() + 1)
    );
}

void MainWindow::makeStep()
{
    if (status == NOTREADY || status == HALTED)
        return;

    if (!this->is_table_uptodate && status != RUNNING && status != PAUSED) {
        if (!loadProgram())
            return;
    }

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

    ui->actionOpen->setEnabled(status != RUNNING && status != PAUSED);
    ui->actionLoadTape->setEnabled(status != RUNNING && status != PAUSED);
    ui->actionSaveTape->setEnabled(status != RUNNING);

    this->status = status;
}

bool MainWindow::loadProgram()
{
    try {
        this->loader.loadTable(ui->textEdit->toPlainText());
    } catch (const tur::CommonError &e) {
        QMessageBox::critical(
            this, tr("Error"),
            tr("Row %1, Column %2:\n%3")
                .arg(e.srcRef.row)
                .arg(e.srcRef.col)
                .arg(e.msg)
        );
        return false;
    }

    this->is_table_uptodate = true;

    updateCellValues();
    updateCurrentState();

    setStatus(READY);
    updateLabelStatus();

    return true;
}

bool MainWindow::onSaveProgram(bool is_save_as)
{
    QString filename = this->filename;

    if (is_save_as || this->filename.isNull()) {
        filename = QFileDialog::getSaveFileName(this);
        if (filename.isNull())
            return false;
    }

    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.write(ui->textEdit->toPlainText().toUtf8());
    file.close();

    this->filename = filename;
    this->is_changes_unsaved = false;

    updateLabelStatus();
    return true;
}

bool MainWindow::saveBefore()
{
    if (!this->is_changes_unsaved)
        return true;

    auto answer = QMessageBox::question(
        this, tr("Save changes?"), tr("Do you want to save file before closing?"),
        QMessageBox::StandardButtons(QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::Save),
        QMessageBox::Cancel
    );

    if (answer == QMessageBox::Cancel)
        return false;

    if (answer == QMessageBox::Save) {
        if (!onSaveProgram())
            return false;
    }

    return true;
}

void MainWindow::onTextChanged()
{
    this->is_table_uptodate = false;
    this->is_changes_unsaved = true;

    updateLabelStatus();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!saveBefore()) {
        event->ignore();
        return;
    }

    if (!this->help_window.isNull())
        this->help_window->close();

    event->accept();
}


void MainWindow::on_buttonPlayPause_clicked()
{
    if (this->status == RUNNING) {
        setStatus(PAUSED);
    } else {
        if (!this->is_table_uptodate && this->status != PAUSED) {
            if (!this->loadProgram())
                return;
        }
        setStatus(RUNNING);
    }
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


void MainWindow::onOpenProgram()
{
    if (!saveBefore())
        return;

    QString filename = QFileDialog::getOpenFileName(this);
    if (filename.isNull())
        return;

    QFile file(filename);
    file.open(QFile::ReadOnly);
    QString source = QString::fromUtf8(file.readAll());
    file.close();

    this->ui->textEdit->setPlainText(source);
    this->filename = filename;
    this->is_changes_unsaved = false;

    loadProgram();

    updateLabelStatus();
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


void MainWindow::onNewFile()
{
    if (!saveBefore())
        return;

    this->emu.drop();
    this->filename.clear();
    this->ui->textEdit->clear();
    this->is_changes_unsaved = false;
    updateLabelStatus();
    updateCellValues();
}


void MainWindow::on_actionEditTape_triggered()
{
    EditTapeDialog dialog(this, this->loader.readTape());
    if (dialog.exec() != QDialog::Accepted)
        return;

    this->loadedTape = dialog.getContent();
    loader.loadTape(loadedTape);
    updateCellValues();
}


void MainWindow::on_actionHelp_triggered()
{
    if (this->help_window.isNull())
        this->help_window = new HelpWindow();

    this->help_window->showNormal();
    this->help_window->activateWindow();
}

