#ifndef SIZEWORKER_H
#define SIZEWORKER_H

#include <QObject>
#include <QDir>
#include <QMutex>

class SizeWorker : public QObject {
    Q_OBJECT
public:
    explicit SizeWorker(const QString &sourceDir, QObject *parent = nullptr);
    void requestStop();

public slots:
    void startSize();

signals:
    void sizeUpdated(qint64 filesVerified);
    void sizeFinished(qint64 success, int filecount);

private:
    QString m_sourceDir;
    qint64 getDirectorySize(const QString &path);
    volatile bool m_stopRequested;  // volatile for thread safety
    QMutex m_sizemutex;
    qint64 m_totalSize;
    int m_filecount = 0;
};

#endif // SIZEWORKER_H
