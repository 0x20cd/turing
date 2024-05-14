#include <QFile>
#include "helpwindow.h"
#include "ui_helpwindow.h"

HelpWindow::HelpWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
    , ui(new Ui::HelpWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose, true);

    QFile help_md(":/res/help.md");
    help_md.open(QFile::ReadOnly);
    ui->textEdit->setMarkdown(QString::fromUtf8(help_md.readAll()));
    help_md.close();
}

HelpWindow::~HelpWindow()
{
    delete ui;
}
