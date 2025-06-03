#include "selectbackup.h"
#include "ui_selectbackup.h"
#include <QDir>
#include <QMessageBox>
#include <QThread>
#include <QDebug>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>

selectbackup::selectbackup(const QString &data, QWidget *parent)
    : QDialog(parent), m_data(data)
    , ui(new Ui::selectbackup)
{
    ui->setupUi(this);
    m_result = "";

    ui->listWidget->setViewMode(QListView::IconMode);  // Better for images
    ui->listWidget->setIconSize(QSize(128, 128));      // Set desired thumbnail size
    ui->listWidget->setResizeMode(QListWidget::Adjust);
    ui->listWidget->setSpacing(10);

    fillList(m_data);
    ui->pushButton_2->setEnabled(false);
    ui->pushButton->setEnabled(false);
    ui->pushOpen->setEnabled(false);
    ui->pushButton_3->setDefault(true);

}

void selectbackup::fillList(QString &data_dir)
{
    QDir dir(data_dir);

    ui->listWidget->setSortingEnabled(true);

    QStringList filters;
    filters << "*.ini";
    QFileInfoList fileInfoList = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo &fileInfo : fileInfoList) {
        QSettings settings( fileInfo.absoluteFilePath(), QSettings::IniFormat );
        QDateTime retLast = settings.value("General/lastOpened").toDateTime();
        QDateTime retFirst = settings.value("General/firstOpened").toDateTime();


        QString baseName = fileInfo.baseName(); // Get filename without extension
        //ui->listWidget->addItem(baseName);

        if(retLast.isValid()) {

            QString qs = retLast.toString("yyyy-MM-dd hh:mm:ss");
            addImageItem(":/images/images/archive.png", qs, baseName);
        }
        else {
            QString qs = retFirst.toString("yyyy-MM-dd hh:mm:ss");
            addImageItem(":/images/images/archive-grey.png", qs, baseName);
        }
    }

    ui->listWidget->sortItems(Qt::AscendingOrder);
}

void selectbackup::addImageItem(const QString &imagePath, const QString &dt, const QString &rootPath) {
    // Load the image
    QPixmap originalPixmap(imagePath);

    if (originalPixmap.isNull()) {
        qDebug() << "Failed to load image:" << imagePath;
        return;
    }

    // Scale the image while maintaining aspect ratio
    QSize targetSize = ui->listWidget->iconSize();

    QPixmap scaledPixmap = originalPixmap.scaled(
        targetSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation  // High quality scaling
        );

    // Create list item
    QListWidgetItem *item = new QListWidgetItem;
    item->setIcon(QIcon(originalPixmap));

    //QString sizeFreeText = sizeToText(freeSize);
    //QString sizeTotalText = sizeToText(totalSize);

    item->setText(dt);//+ " " + sizeFreeText + " free on " + sizeTotalText);  // Show filename below image

    // Optional: Store original path in data for later use
    item->setData(Qt::UserRole, rootPath);

    // Adjust item size hint
    item->setSizeHint(QSize(targetSize.width() + 40, targetSize.height() + 40));

    // Add to list
    ui->listWidget->addItem(item);
}


selectbackup::~selectbackup()
{
    delete ui;
}

QString selectbackup::getResult() const
{
    return m_result;
}

bool selectbackup::getCheckBox() const
{
    return ui->chkBoxRemove->checkState();
}

void selectbackup::on_buttonBox_accepted()
{

}

void selectbackup::on_listWidget_itemClicked(QListWidgetItem *item)
{
    m_selected = item->data(Qt::UserRole).toString();
    ui->pushButton_2->setEnabled(true);
    ui->pushButton->setEnabled(true);
    ui->pushOpen->setEnabled(true);
}


void selectbackup::on_buttonBox_rejected()
{

}

void selectbackup::on_selectbackup_rejected()
{
}


bool selectbackup::isRemoveRecursively(QDir &dir)
{
    if (dir.removeRecursively())
        return true;

    QThread::msleep(500);

    if (dir.removeRecursively())
        return true;

    return false;
}


bool selectbackup::isDirExists(QDir &dir)
{
     if (dir.exists())
        return true;

     QThread::msleep(500);

     if (dir.exists())
         return true;

     return false;
}


void selectbackup::on_pushButton_clicked()
{
    if(m_selected != "") {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Confirm Deletion",
            "Are you sure you want to delete " + m_selected + " and all its contents?\nThis may take a moment.",
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply != QMessageBox::Yes) {
           // m_textEdit->setText("Deletion cancelled.");
            return;
        }

        QString dirPath = m_data + m_selected;

        QDir dir(dirPath);
        if (isDirExists(dir)) {
            bool success = isRemoveRecursively(dir);
            if (success) {
                qDebug() << "Successfully deleted: " + dirPath;
                QFile file (m_data + m_selected + ".ini");
                file.remove();
            } else {
                qDebug() << "Failed to delete retry: " + dirPath;
                QThread::msleep(500);
                if(!isRemoveRecursively(dir)) {
                    qDebug() << "Failed to delete : " + dirPath;
                }
            }
        } else {
            qDebug() << "Directory does not exist: " + dirPath;
        }

        ui->listWidget->clear();

        fillList(m_data);

        ui->pushButton_2->setEnabled(false);
        ui->pushButton->setEnabled(false);
        m_selected = "";
        m_result = "";
    }
}


void selectbackup::on_pushButton_2_clicked()
{
    m_result = m_selected;
    selectbackup::close();
}


void selectbackup::on_pushButton_3_clicked()
{
    m_selected = "";
    m_result = "";
    selectbackup::close();
}


void selectbackup::on_pushOpen_clicked()
{
    if(m_selected != "") {
        QString dirPath = m_data + m_selected;
        QDesktopServices::openUrl( QUrl::fromLocalFile( dirPath ) );
    }
}

