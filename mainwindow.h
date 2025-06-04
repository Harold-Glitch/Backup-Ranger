#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include "copyworker.h"

#include "circleprogressbar.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QThread* m_copythread;
    CopyWorker* m_copyworker;

    CircleProgressBar *m_progressBar;

private slots:
    void on_pushButton_clicked();
    void on_pushStop_clicked();
    void on_pushBackup_clicked();

    void startCopy(QString source, QString desination, qint64 freeSpace, bool overwrite, bool backup, bool appData, int keep);
    void updateProgress(int filesCopied, int totalFiles);
    void fileUpdateProgress(qint64 filesCopied, qint64 totalFiles);
    void updateVerify(qint64 filesVerified);
    void updateDelete(int filesDeleted);
    void handleCopyFinished(bool success);
    void handleError(const QString &error);
    void handleMessage(const QString &error);
    void handleCopy(const QString &fn);

    void on_pushDrive_clicked();
    void on_pushSettings_clicked();

    void on_pushPaste_clicked();

    void on_pushExit_clicked();

    void onReplyFinished(QNetworkReply *reply);

private:
    Ui::MainWindow *ui;
    QString m_error;
    QString m_drive;
    int m_filecount;
    qint64 m_sourceSize;
    bool m_appData;
    int m_keep;
    QString m_reg;
    QString m_email;

    QString sizeToText(qint64 size);
    void log(QString qsLog);
    qint64 getDirectorySize(const QString &path);
    bool checkRegistrationCode(const QString& userId, const QString& scode);
    QString getUserDirectory();
};
#endif // MAINWINDOW_H
