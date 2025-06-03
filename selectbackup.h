#ifndef SELECTBACKUP_H
#define SELECTBACKUP_H

#include <QDialog>
#include <QDir>
#include <QListWidgetItem>

namespace Ui {
class selectbackup;
}

class selectbackup : public QDialog
{
    Q_OBJECT

public:
    explicit selectbackup(const QString &data, QWidget *parent = nullptr);
    QString getResult() const; // Getter for the result
    bool getCheckBox() const; // Getter for the checkBox
    ~selectbackup();

private slots:
    void on_buttonBox_accepted();

    void on_listWidget_itemClicked(QListWidgetItem *item);

    void on_buttonBox_rejected();

    void on_selectbackup_rejected();

    void on_pushButton_clicked();


    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushOpen_clicked();

private:
    void fillList(QString &data_dir);
    void addImageItem(const QString &imagePath, const QString &dt, const QString &rootPath);
    Ui::selectbackup *ui;
    QString m_data; // Store the passed argument
    QString m_result;
    QString m_selected;
    bool isDirExists(QDir &dir);
    bool isRemoveRecursively(QDir &dir);
};

#endif // SELECTBACKUP_H
