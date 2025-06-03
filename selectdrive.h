#ifndef SELECTDRIVE_H
#define SELECTDRIVE_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
class selectdrive;
}

class selectdrive : public QDialog
{
    Q_OBJECT

public:
    explicit selectdrive(QWidget *parent = nullptr);
    ~selectdrive();
    QString getDrive(); // Getter for the result
    QString getLabel(); // Getter for the result

private slots:
    void on_pushButton_3_clicked();

    void on_listWidget_itemClicked(QListWidgetItem *item);

private:
    Ui::selectdrive *ui;
    void displayDrives();
    void addImageItem(const QString &imagePath, const QString &rootPath, qint64 freeSize, qint64 totalSize);
    QString sizeToText(qint64 size);
    QString getUserDirectory();
    QString m_drive;
    QString m_label;

};

#endif // SELECTDRIVE_H
