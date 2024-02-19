#include "cell.h"

const int Cell::CELL_SIZE = 50;

Cell::Cell(QWidget *parent)
    : QPushButton(parent)
{
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->setMinimumSize(CELL_SIZE, CELL_SIZE);
    this->setMaximumSize(CELL_SIZE, CELL_SIZE);
}

void Cell::setSelected(bool isSelected)
{
    this->setStyleSheet(isSelected ? "border-color: blue; border-width: 3;" : "");
}
