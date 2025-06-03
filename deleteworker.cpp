#include "deleteworker.h"
#include <QDirIterator>
#include <QFile>
#include <QDir>

deleteWorker::deleteWorker(const QString &dirPath, QObject *parent)
    : QObject(parent), m_dirPath(dirPath)
{
}

void deleteWorker::startDeletion()
{
    QDir dir(m_dirPath);
    if (!dir.exists()) {
        emit finished(false, "Directory does not exist: " + m_dirPath);
        return;
    }

    // Count total files and directories for progress
    int totalItems = 0;
    QDirIterator countIt(m_dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (countIt.hasNext()) {
        countIt.next();
        totalItems++;
    }

    if (totalItems == 0) {
        emit finished(true, "No items to delete in " + m_dirPath);
        return;
    }

    // Delete items and update progress
    int processedItems = 0;
    QDirIterator deleteIt(m_dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QStringList failedItems;

    while (deleteIt.hasNext()) {
        QString itemPath = deleteIt.next();
        QFileInfo info(itemPath);

        bool success = false;
        if (info.isFile()) {
            success = QFile(itemPath).remove();
        } else if (info.isDir()) {
            success = QDir(itemPath).removeRecursively();
        }

        if (!success) {
            failedItems << itemPath;
        }

        processedItems++;
        emit progress(processedItems, totalItems);
    }

    // Report result
    if (failedItems.isEmpty()) {
        emit finished(true, "Successfully deleted " + QString::number(processedItems) + " items.");
    } else {
        emit finished(false, "Failed to delete some items:\n" + failedItems.join("\n"));
    }
}
