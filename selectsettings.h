#ifndef SELECTSETTINGS_H
#define SELECTSETTINGS_H

#include <QDialog>

namespace Ui {
class selectsettings;
}

class selectsettings : public QDialog
{
    Q_OBJECT

public:
    explicit selectsettings(QWidget *parent = nullptr);
    int getPast() const;
    QString getReg() const;
    QString getEmail() const;
    bool getCheckBox() const;
    void setPast(int) const;
    void setReg(QString) const;
    void setEmail(QString) const;
    void setCheckBox(bool) const;
    ~selectsettings();

private slots:
    void on_pushButton_clicked();

private:
    Ui::selectsettings *ui;
};

#endif // SELECTSETTINGS_H
