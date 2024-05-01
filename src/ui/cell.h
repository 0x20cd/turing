#ifndef CELL_H
#define CELL_H

#include <QPushButton>
#include <QLabel>
#include <QObject>
#include <QWidget>
#include <QStyle>
#include "tur/parser.hpp"
#include "tur/emulator.hpp"

class Cell : public QPushButton
{
    Q_OBJECT
public:
    static const int CELL_SIZE;
    static const QLatin1StringView CSS;
    explicit Cell(QWidget *parent = nullptr);
    void setValue(quint32 sym, std::shared_ptr<tur::parser::Alphabet> alph);
    void setSelected(bool selected);

    int diff;
private:
    void setNamed(bool named);
    void updateStyle();

    bool is_selected, is_named;
};

#endif // CELL_H
