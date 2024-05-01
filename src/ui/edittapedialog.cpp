#include "edittapedialog.h"
#include "ui_edittapedialog.h"

EditTapeDialog::EditTapeDialog(QWidget *parent, QString content)
    : QDialog(parent)
    , ui(new Ui::EditTapeDialog)
{
    ui->setupUi(this);
    ui->lineEdit->setText(content);
}

QString EditTapeDialog::getContent()
{
    return ui->lineEdit->text();
}

EditTapeDialog::~EditTapeDialog()
{
    delete ui;
}
