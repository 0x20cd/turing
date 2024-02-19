#ifndef CELL_H
#define CELL_H

#include <QPushButton>
#include <QObject>
#include <QWidget>

class Cell : public QPushButton
{
    Q_OBJECT
public:
    static const int CELL_SIZE;
    explicit Cell(QWidget *parent = nullptr);
    void setSelected(bool isSelected = true);
};

#endif // CELL_H
