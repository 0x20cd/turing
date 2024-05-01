#ifndef EDITTAPEDIALOG_H
#define EDITTAPEDIALOG_H

#include <QDialog>

namespace Ui {
class EditTapeDialog;
}

class EditTapeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditTapeDialog(QWidget *parent, QString content);
    QString getContent();
    ~EditTapeDialog();

private:
    Ui::EditTapeDialog *ui;
};

#endif // EDITTAPEDIALOG_H
