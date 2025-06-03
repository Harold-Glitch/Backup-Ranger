#include "sizeworker.h"
#include <QFileInfo>
#include <QDebug>
#include <windows.h>


SizeWorker::SizeWorker(const QString &sourceDir, QObject *parent)
    : QObject(parent), m_sourceDir(sourceDir) {
}

void SizeWorker::startSize() {
    m_stopRequested = false;
    qDebug() << "startSize";
    m_totalSize = 0;
    m_filecount = 0;
    qint64 size = getDirectorySize(m_sourceDir);
    emit sizeFinished(size, m_filecount);
}

void SizeWorker::requestStop() {
    QMutexLocker locker(&m_sizemutex);
    m_stopRequested = true;
}

qint64 SizeWorker::getDirectorySize(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) {
        qDebug() << "Directory does not exist:" << path;
        return -1;
    }

    qint64 totalSize = 0;

    // Get all entries in the directory
    QFileInfoList list = dir.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);

    for (const QFileInfo &fileInfo : list) {
        if (fileInfo.isDir()) {
            // Recursively calculate size of subdirectories
            totalSize += getDirectorySize(fileInfo.absoluteFilePath());
        } else if (fileInfo.isFile()) {
            // Add size of files
            totalSize += fileInfo.size();
            m_filecount++;
        }

        QMutexLocker locker(&m_sizemutex);
        if (m_stopRequested) {
            m_filecount = 0;
            //emit errorOccurred("Terminated by User");
            break;
        }
    }

   // qDebug() << "sizeUpdated";
    emit sizeUpdated(m_filecount);

    return totalSize;
}

