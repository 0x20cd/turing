#include <cwctype>
#include "cell.h"
#include "tur/utils.hpp"

const int Cell::CELL_SIZE = 50;
const QLatin1StringView Cell::CSS{
    "border-width: %1;"
    "border-color: %2;"
    "font-size: %3;"
    "border-style: solid;"
    "font-family: \"Courier New\";"
    "border-radius: 1.5;"
    "color: black;"
};

Cell::Cell(QWidget *parent)
    : QPushButton(parent)
    , diff(0)
    , is_selected(false)
    , is_named(false)
{
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->setMinimumSize(CELL_SIZE, CELL_SIZE);
    this->setMaximumSize(CELL_SIZE, CELL_SIZE);
    this->updateStyle();
}

void Cell::setValue(quint32 sym, std::shared_ptr<tur::parser::Alphabet> alph)
{
    bool named;
    QString sym_str = tur::utils::symbolToString(sym, alph, &named);

    this->setNamed(named);
    this->setText(sym_str);
    this->setToolTip(named ? sym_str : QString{});
}

void Cell::setSelected(bool selected)
{
    if (this->is_selected == selected)
        return;

    this->is_selected = selected;
    this->updateStyle();
}

void Cell::setNamed(bool named)
{
    if (this->is_named == named)
        return;

    this->is_named = named;
    this->updateStyle();
}

void Cell::updateStyle()
{
    const char *border_width = this->is_selected ? "3" : "1";
    const char *border_color = this->is_selected ? "blue" : "black";
    const char *font_size = this->is_named ? "8pt" : "21pt";

    this->setStyleSheet(Cell::CSS.arg(border_width, border_color, font_size));
}
