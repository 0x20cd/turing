#include "helpwindow.h"
#include "ui_helpwindow.h"

HelpWindow::HelpWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
    , ui(new Ui::HelpWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose, true);
}

HelpWindow::~HelpWindow()
{
    delete ui;
}
