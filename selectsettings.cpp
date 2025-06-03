#include "selectsettings.h"
#include "ui_selectsettings.h"

selectsettings::selectsettings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::selectsettings)
{
    ui->setupUi(this);
}

selectsettings::~selectsettings()
{
    delete ui;
}

void selectsettings::setReg(QString qs) const
{
    ui->lineEdit->setText(qs);
}

void selectsettings::setEmail(QString qs) const
{
    ui->emailEdit->setText(qs);
}

void selectsettings::setCheckBox(bool b) const
{
    ui->checkBox->setChecked(b);
}

void selectsettings::setPast(int i) const
{
    ui->spinBox->setValue(i);
}

QString selectsettings::getReg() const
{
    return ui->lineEdit->text();
}

QString selectsettings::getEmail() const
{
    return ui->emailEdit->text();
}

bool selectsettings::getCheckBox() const
{
    return ui->checkBox->checkState();
}

int selectsettings::getPast() const
{
    return ui->spinBox->value();
}

void selectsettings::on_pushButton_clicked()
{
    selectsettings::close();
}

