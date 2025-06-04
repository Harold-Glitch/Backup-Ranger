#include <QThread>
#include <QMessageBox>
#include <QSettings>
#include <QFileInfo>
#include <QStorageInfo>
#include <QDebug>
#include <QList>
#include <QApplication>
#include <QVBoxLayout>
#include <QSystemTrayIcon>
#include <QClipboard>
#include <QMessageAuthenticationCode>

#include <windows.h>

#include "copyworker.h"
#include "selectbackup.h"
#include "selectdrive.h"
#include "selectsettings.h"
#include <QFileDialog>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include <QDebug>
#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    QNetworkAccessManager *manager;

    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &MainWindow::onReplyFinished);

    QNetworkRequest request(QUrl("https://backupranger.com/version.json"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->get(request);

    m_copyworker = nullptr;
    m_drive = "";
    m_filecount = 0;

    ui->setupUi(this);
    setFixedSize(size());

    QVBoxLayout *layout = new QVBoxLayout(ui->widget);

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "backup-ranger", "backup-ranger");

    log("Welcome to Backup-Ranger " + QCoreApplication::applicationVersion());

    m_reg = settings.value("General/Registration").toString();
    m_email = settings.value("General/Email").toString();
    m_appData = settings.value("General/AppData").toBool();
    m_keep = settings.value("General/Keep").toInt();

    checkRegistrationCode(m_email, m_reg);

    if(m_reg == "" && m_keep == 0)
    {
        m_reg = "unregistered";
        m_appData = false;
        m_keep = 4;
    }

    m_progressBar = new CircleProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setProgressColor(Qt::darkGreen);

    QPalette palette = ui->label->palette();
    QColor textColor = palette.color(QPalette::WindowText); // Foreground color for text
    m_progressBar->setTextColor(textColor);

    m_progressBar->setValue(0);

    layout->addWidget(m_progressBar);

    ui->pushStop->setEnabled(false);
    ui->pushDrive->setEnabled(true);
    ui->pushBackup->setEnabled(true);
    ui->pushButton->setEnabled(true);

    QString source = getUserDirectory();
    log("User Directory: " + source);

    bool drive_found = false;
    m_drive = settings.value("General/Drive").toString();

    if(m_drive != "") {

        QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();

        for (const QStorageInfo &storage : drives) {
            if (storage.isValid() && storage.isReady()) {
                if(m_drive == storage.rootPath() ) {

                    QString qsDrive = storage.rootPath().removeLast() + " " + sizeToText(storage.bytesFree()) + " free on " + sizeToText(storage.bytesTotal());
                    ui->label_5->setText(qsDrive);
                    log("Backup Drive: " + qsDrive);
                    ui->pushDrive->setIcon(QIcon(":/images/images/hdd.png"));
                    drive_found = true;

                }
            }
        }

        if(drive_found == false) {
            m_drive = "";
        }
    }

    if(m_drive == "") {
        ui->pushBackup->setEnabled(false);
        ui->pushButton->setEnabled(false);
    }

    // Enable read-only mode (optional, if you want to prevent editing)
    ui->textEdit->setReadOnly(true);

}

MainWindow::~MainWindow()
{
    delete ui;

    if (m_copyworker && m_copythread->isRunning()) {
        m_copyworker->requestStop();        
        m_copythread->wait(100);
    }

}

void MainWindow::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            QString latestVersion = obj["version"].toString();
            QString currentVersion = QCoreApplication::applicationVersion(); // e.g., "2.1.2"

            QVersionNumber current = QVersionNumber::fromString(currentVersion);
            QVersionNumber latest = QVersionNumber::fromString(latestVersion);

            if (latest > current) {

                QMessageBox::information(nullptr, "Update Available",
                    QString("A new version (%1) is available!\n\n%2\n\nDownload from: %3").arg(latestVersion).arg(obj["release_notes"].toString()).arg(obj["download_url"].toString()));

                qDebug() << "New version available:" << latestVersion;
                qDebug() << "Release notes:" << obj["release_notes"].toString();
                qDebug() << "Download URL:" << obj["download_url"].toString();
                // Optionally, show a dialog (see below)
            } else {
                qDebug() << "Your software is up to date.";
            }
        } else {
            qDebug() << "Invalid JSON response.";
        }
    } else {
        qDebug() << "Network error:" << reply->errorString();
    }
    reply->deleteLater();
}

void MainWindow::log(QString qsLog)
{
    ui->textEdit->append(QString("%1 - %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(qsLog));

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEdit->setTextCursor(cursor);
    ui->textEdit->ensureCursorVisible();
}


void MainWindow::on_pushButton_clicked() // RESTORE
{
    if(m_drive == "") {
        QMessageBox::information(
            nullptr,
            "Backup Notification",
            "Please select the destination drive",
            QMessageBox::Ok
            );
        return;
    }

    QString data = m_drive + "Backups/";

    selectbackup *dialog = new selectbackup(data, this);
    dialog->exec();

    QString formatted = dialog->getResult();
    bool overwrite = dialog->getCheckBox();

    if(formatted != "") {

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Restore",
            "Are you sure you want to restore archive " + formatted + "?",
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply != QMessageBox::Yes) {            
            return;
        }

        ui->pushBackup->setEnabled(false);
        ui->pushButton->setEnabled(false);
        ui->pushDrive->setEnabled(false);
        ui->pushStop->setEnabled(true);
        ui->pushExit->setEnabled(false);
        ui->pushPaste->setEnabled(false);
        ui->pushSettings->setEnabled(false);

        QString destination = getUserDirectory();
        QString source = m_drive + "Backups/" + formatted;
        m_filecount = 0;
        startCopy(source, destination, 0, overwrite, false, m_appData, m_keep);
    }
    else {
    }
}

void MainWindow::on_pushStop_clicked() // STOP BUTTON
{
    if (m_copyworker && m_copythread->isRunning()) {
        m_copyworker->requestStop();       
        m_copythread->wait(100);
    }

    m_copyworker = nullptr;
}

QString MainWindow::getUserDirectory()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "backup-ranger", "backup-ranger");
    QString addin_path = QFileInfo(settings.fileName()).absolutePath();
    QStringList parts = addin_path.split("/", Qt::SkipEmptyParts);
    QString source = parts[0] + "/" + parts[1] + "/" + parts[2] + "/" ;    
    return source;
}

void MainWindow::startCopy(QString source, QString desination, qint64 freeSpace, bool overwrite, bool backup, bool appData, int keep)
{
    m_error = "";

    if (source.isEmpty() || desination.isEmpty()) {
        ui->label->setText("Please select both source and destination directories");
        return;
    }

    m_progressBar->setValue(0);

    m_copyworker = new CopyWorker(source, desination, freeSpace, overwrite, backup, appData, keep); // true for overwrite
    m_copythread = new QThread;

    m_copyworker->moveToThread(m_copythread);

    connect(m_copythread, &QThread::started, m_copyworker, &CopyWorker::startCopy);
    connect(m_copyworker, &CopyWorker::verifyUpdated, this, &MainWindow::updateVerify);
    connect(m_copyworker, &CopyWorker::deleteUpdated, this, &MainWindow::updateDelete);

    connect(m_copyworker, &CopyWorker::progressUpdated, this, &MainWindow::updateProgress);
    connect(m_copyworker, &CopyWorker::fileProgressUpdated, this, &MainWindow::fileUpdateProgress);
    connect(m_copyworker, &CopyWorker::copyFinished, this, &MainWindow::handleCopyFinished);
    connect(m_copyworker, &CopyWorker::errorOccurred, this, &MainWindow::handleError);
    connect(m_copyworker, &CopyWorker::messageOccurred, this, &MainWindow::handleMessage);
    connect(m_copyworker, &CopyWorker::copyOccurred, this, &MainWindow::handleCopy);
    connect(m_copyworker, &CopyWorker::copyFinished, m_copythread, &QThread::quit);
    connect(m_copyworker, &CopyWorker::copyFinished, m_copyworker, &CopyWorker::deleteLater);
    connect(m_copythread, &QThread::finished, m_copythread, &QThread::deleteLater);

    m_copythread->start();
}

void MainWindow::on_pushBackup_clicked() // BACKUP
{
    if(m_drive != "") {

        QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();
        qint64 freeSpace = 0;
        bool drive_found = false;

        for (const QStorageInfo &storage : drives) {
            if (storage.isValid() && storage.isReady()) {
                if(m_drive == storage.rootPath() ) {
                    freeSpace = storage.bytesFree();
                    drive_found = true;
                }
            }
        }

        if(drive_found == false) {
            ui->pushDrive->setIcon(QIcon(":/images/images/hdd-grey.png"));
            m_drive = "";
            ui->label_5->setText("");
            return;
        }

        ui->pushBackup->setEnabled(false);
        ui->pushButton->setEnabled(false);
        ui->pushDrive->setEnabled(false);
        ui->pushStop->setEnabled(true);
        ui->pushExit->setEnabled(false);
        ui->pushPaste->setEnabled(false);
        ui->pushSettings->setEnabled(false);


        QString source = getUserDirectory();

        QString formatted = QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
        QString dest = m_drive + "Backups/" + formatted;

        startCopy(source, dest, freeSpace, false, true, m_appData, m_keep);
    }
    else {
        QMessageBox::information(
            nullptr,
            "Backup Notification",
            "Please select the destination drive",
            QMessageBox::Ok
            );
    }
}

void MainWindow::handleMessage(const QString &message) {
    ui->label->setText(message);
    log(message);
}

void MainWindow::updateDelete(int filesDeleted) {

    //m_progressBar->setValue(0);

    ui->label->setText(QString("Deleting... %1 files").arg(QLocale(QLocale::English).toString(filesDeleted)));
}

void MainWindow::updateVerify(qint64 filesVerified) {

    m_progressBar->setValue(0);

    ui->label->setText(QString("Verifying... %1").arg(sizeToText(filesVerified)));
}

void MainWindow::updateProgress(int filesCopied, int totalFiles) {
    m_progressBar->setMaximum(totalFiles);
    m_progressBar->setValue(filesCopied);

    QString qsFiles = QLocale(QLocale::English).toString(filesCopied);
    QString qsTotal = QLocale(QLocale::English).toString(totalFiles);

    ui->label->setText(QString("Copying... (%1 / %2 files)").arg(qsFiles).arg(qsTotal));
}

void MainWindow::fileUpdateProgress(qint64 filesCopied, qint64 totalFiles) {
    qint64 ratio = (1024*1024);
    qint64 total64 = totalFiles / ratio;
    qint64 size64 = filesCopied / ratio;

    int total = total64;
    int size = size64;

    ui->progressBar_2->setMaximum(total);
    ui->progressBar_2->setValue(size);
}


QString MainWindow::sizeToText(qint64 size)
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


void MainWindow::handleCopyFinished(bool success) {

    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(QIcon(":/images/images/hdd.png")); // Set your icon
    trayIcon.show();

    if (success && m_error == "") {
        ui->label->setText("Copy completed successfully");
        log("Copy completed successfully");

        m_progressBar->setMaximum(100);
        m_progressBar->setValue(0);

        trayIcon.showMessage(
            "Backup Notification",
            "Copy completed successfully",
            QSystemTrayIcon::Information,
            3000
            );
    } else {

        if(m_error == "") {
            trayIcon.showMessage(
                "Backup Notification",
                m_error,
                QSystemTrayIcon::Information,
                3000
                );
        }

        ui->label->setText(m_error);
        log(m_error);
    }

    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();

    for (const QStorageInfo &storage : drives) {
        if (storage.isValid() && storage.isReady() && m_drive == storage.rootPath() ) {

            QString qsDrive = storage.rootPath().removeLast() + " " + sizeToText(storage.bytesFree()) + " free on " + sizeToText(storage.bytesTotal());
            ui->label_5->setText(qsDrive);
            log("Backup Drive: " + qsDrive);
        }
    }

    ui->pushBackup->setEnabled(true);
    ui->pushStop->setEnabled(false);
    ui->pushButton->setEnabled(true);
    ui->pushDrive->setEnabled(true);
    ui->pushExit->setEnabled(true);
    ui->pushPaste->setEnabled(true);
    ui->pushSettings->setEnabled(true);

}

void MainWindow::handleError(const QString &error) {
    ui->label->setText("Error: " + error);

    if(m_error == "") {
        m_error = error;
        log("Error: " + m_error);
        m_progressBar->setValue(0);
        QSystemTrayIcon trayIcon;
        trayIcon.setIcon(QIcon(":/images/images/hdd.png")); // Set your icon
        trayIcon.show();

        trayIcon.showMessage(
            "Backup Notification",
            "Error: " + m_error,
            QSystemTrayIcon::Information,
            3000 // Duration in milliseconds
            );
    }

    ui->pushBackup->setEnabled(true);
    ui->pushStop->setEnabled(false);
    ui->pushButton->setEnabled(true);
    ui->pushDrive->setEnabled(true);

}

void MainWindow::handleCopy(const QString &fn) {
    QString path = QDir::fromNativeSeparators(fn);
    QStringList parts = path.split("/");
    QString lastBit = parts.at(parts.size()-1);
    ui->label_4->setText(lastBit);
}

void MainWindow::on_pushDrive_clicked()
{
    selectdrive *dialog = new selectdrive(this);
    dialog->exec();
    QString drive = dialog->getDrive();

    if(drive != "") {
        m_drive = drive;
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "backup-ranger", "backup-ranger");
        settings.setValue("General/Drive", m_drive);
        settings.sync();

        ui->pushDrive->setIcon(QIcon(":/images/images/hdd.png"));
        ui->label_5->setText(dialog->getLabel());
        QString qsDrive = dialog->getLabel();
        log("Backup Drive: " + qsDrive);

        ui->pushBackup->setEnabled(true);
        ui->pushButton->setEnabled(true);
    }
    else if(m_drive == ""){
        ui->pushBackup->setEnabled(false);
        ui->pushButton->setEnabled(false);
    }
}


void MainWindow::on_pushSettings_clicked()
{
    selectsettings *dialog = new selectsettings(this);

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "backup-ranger", "backup-ranger");

    m_reg = settings.value("General/Registration").toString();
    m_email = settings.value("General/Email").toString();
    m_appData = settings.value("General/AppData").toBool();
    m_keep = settings.value("General/Keep").toInt();

    dialog->setReg(m_reg);
    dialog->setEmail(m_email);
    dialog->setCheckBox(m_appData);
    dialog->setPast(m_keep);

    dialog->exec();

    m_email = dialog->getEmail();
    m_reg = dialog->getReg();
    m_appData = dialog->getCheckBox();
    m_keep = dialog->getPast();

    //QSettings settings(QSettings::IniFormat, QSettings::UserScope, "backup-ranger", "backup-ranger");
    settings.setValue("General/Email", m_email);
    settings.setValue("General/Registration", m_reg);
    settings.setValue("General/AppData", m_appData);
    settings.setValue("General/Keep", m_keep);
    settings.sync();

    checkRegistrationCode(m_email, m_reg);
}


bool MainWindow::checkRegistrationCode(const QString& userId, const QString& scode) {
    if (userId.isEmpty()) {
        log("Product unregisterd");
        return false;
    }

    // Secret key (must match the PHP secret key)
    const QString secretKey = "Backup-Ranger-ID";

    // Convert to hex and take first 16 characters
    QString shortHash = QString(QMessageAuthenticationCode::hash(userId.toUtf8(), secretKey.toUtf8(), QCryptographicHash::Sha256).toHex()).left(16);

    // Format with dashes (e.g., XXXX-XXXX-XXXX-XXXX)
    QString formattedCode;
    for (int i = 0; i < shortHash.length(); i += 4) {
        formattedCode += shortHash.mid(i, 4);
        if (i < shortHash.length() - 4) {
            formattedCode += "-";
        }
    }

    QString scomputed = formattedCode.toUpper();

    if(scomputed == scode) {
        log("Product registerd to: " + userId);
        return true;
    }
    log("Product unregisterd");
    return false;
}


void MainWindow::on_pushPaste_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    // Copy all text from QTextEdit to clipboard
    clipboard->setText(ui->textEdit->toPlainText());
 }


 void MainWindow::on_pushExit_clicked()
 {
     MainWindow::close();
 }

