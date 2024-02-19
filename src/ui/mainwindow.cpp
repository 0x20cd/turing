#include <QDebug>
#include <QFile>
#include "mainwindow.h"
#include "cell.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    loader(emu)
{
    ui->setupUi(this);

    QFile cell_css_f(":/res/cell.css");
    cell_css_f.open(QFile::ReadOnly);
    QString cell_css = QString::fromUtf8(cell_css_f.readAll());
    cell_css_f.close();

    this->setStyleSheet(cell_css);
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
    int hspacing = ui->tape->spacing();
    int tapeWidth = this->size().width() - (left + right);
    int count = ui->tape->count(), newCount = (tapeWidth + hspacing - 1) / (Cell::CELL_SIZE + hspacing);

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

}

void MainWindow::on_buttonPlayPause_clicked()
{

}


void MainWindow::on_buttonStep_clicked()
{

}


void MainWindow::on_buttonReset_clicked()
{

}


void MainWindow::on_actionOpen_triggered()
{

}


void MainWindow::on_actionLoadTape_triggered()
{

}


void MainWindow::on_actionSaveTape_triggered()
{

}

