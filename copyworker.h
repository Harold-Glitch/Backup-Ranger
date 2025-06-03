#ifndef COPYWORKER_H
#define COPYWORKER_H

#include <QObject>
#include <QDir>
#include <QMutex>

class CopyWorker : public QObject {
    Q_OBJECT
public:
    explicit CopyWorker(const QString &sourceDir, const QString &destDir, qint64 destSize, bool overwrite = true, bool backup = true, bool appData = false, int keep = 0, QObject *parent = nullptr);
    void requestStop();

public slots:
    void startCopy();

signals:
    void progressUpdated(int filesCopied, int totalFiles);
    void fileProgressUpdated(qint64 filesCopied, qint64 totalFiles);
    void verifyUpdated(qint64 filesVerified);
    void deleteUpdated(int deletedFiles);
    void copyFinished(bool success);
    void copyOccurred(const QString &error);
    void errorOccurred(const QString &error);
    void messageOccurred(const QString &message);

private:
    bool copyDirectoryRecursively(const QString &sourceDir, const QString &destDir, int totalFiles);
    bool copyDateTimeRecursively(const QString &sourceDir, const QString &destDir);
    int countFiles(const QString &dirPath);
    bool copyFileByChunks(const QString &sourcePath, const QString &destPath);
    bool deleteRecursively(QDir &dir);
    QString sizeToText(qint64 size);

    QString m_sourceDir;
    QString m_destDir;
    bool m_overwrite;
    bool m_backup;
    bool m_appData;
    int m_keep;
    int m_filesCopied;
    int m_totalFiles;
    int m_deletedFiles;
    qint64 m_totalSize;
    qint64 m_destSize;

    volatile bool m_stopRequested;  // volatile for thread safety
    QMutex m_copymutex;
};

#endif // COPYWORKER_H
