#include "selectdrive.h"
#include "ui_selectdrive.h"

#include <QStorageInfo>
#include <QDebug>
#include <QList>
#include <QSettings>
#include <QFileInfo>


selectdrive::selectdrive(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::selectdrive)
{
    ui->setupUi(this);

    ui->listWidget->setViewMode(QListView::IconMode);  // Better for images
    ui->listWidget->setIconSize(QSize(128, 128));      // Set desired thumbnail size
    ui->listWidget->setResizeMode(QListWidget::Adjust);
    ui->listWidget->setSpacing(10);

    displayDrives();
}

selectdrive::~selectdrive()
{
    delete ui;
}

QString selectdrive::sizeToText(qint64 size)
{
    QString sizeText;
    if (size >= (qint64)1024 * 1024 * 1024 * 1024) {
        sizeText = QString("%1 TB").arg(size / (1024.0 * 1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    } else if (size >= 1024 * 1024 * 1024) {
        sizeText = QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    } else if (size >= 1024 * 1024) {
        sizeText = QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 2);
    } else if (size >= 1024) {
        sizeText = QString("%1 KB").arg(size / 1024.0, 0, 'f', 2);
    } else {
        sizeText = QString("%1 bytes").arg(size);
    }
    return sizeText;
}

void selectdrive::addImageItem(const QString &imagePath, const QString &rootPath, qint64 freeSize, qint64 totalSize) {
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

    QString sizeFreeText = sizeToText(freeSize);
    QString sizeTotalText = sizeToText(totalSize);
    QString rp = rootPath;
    rp = rp.removeLast();

    item->setText(rp + " " + sizeFreeText + " free on " + sizeTotalText);  // Show filename below image

    // Optional: Store original path in data for later use
    item->setData(Qt::UserRole, rootPath);

    // Adjust item size hint
    item->setSizeHint(QSize(targetSize.width() + 40, targetSize.height() + 40));

    // Add to list
    ui->listWidget->addItem(item);
}

QString selectdrive::getUserDirectory()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "BCK", "BCK");
    QString addin_path = QFileInfo(settings.fileName()).absolutePath();
    QStringList parts = addin_path.split("/", Qt::SkipEmptyParts);
    QString source = parts[0] + "/" + parts[1] + "/" + parts[2] + "/" ;
    return source;
}

void selectdrive::displayDrives()
{
    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();
    QString addin_path = getUserDirectory();
    QStringList parts = addin_path.split("/", Qt::SkipEmptyParts);
    QString sourcePath = parts[0] + "/";

    ui->listWidget->clear();

    for (const QStorageInfo &storage : drives) {
        if (storage.isValid() && storage.isReady()) {
            //qDebug() << "Drive:" << storage.rootPath();
            //qDebug() << "Name:" << storage.displayName();
            //qDebug() << "Type:" << storage.fileSystemType();
            //qDebug() << "Total Size:" << storage.bytesTotal() / (1024.0 * 1024.0) << "MB";
            //qDebug() << "Free Space:" << storage.bytesFree() / (1024.0 * 1024.0) << "MB";
            //qDebug() << "-------------------";

            if(sourcePath != storage.rootPath() ) {
                addImageItem(":/images/images/hdd.png", storage.rootPath(), storage.bytesFree(), storage.bytesTotal());
            }
        }
    }
}

void selectdrive::on_pushButton_3_clicked()
{
    displayDrives();
}

void selectdrive::on_listWidget_itemClicked(QListWidgetItem *item)
{
    QString str = item->data(Qt::UserRole).toString();
    m_drive = str;
    m_label = item->text();

    selectdrive::close();
}

QString selectdrive::getDrive()
{
    return m_drive;
}

QString selectdrive::getLabel()
{

    return m_label;
}

