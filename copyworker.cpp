#include "copyworker.h"
#include <QFileInfo>
#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QDirIterator>
#include <QDateTime>
#include <windows.h>
#include <QThread>


CopyWorker::CopyWorker(const QString &sourceDir, const QString &destDir, qint64 destSize, bool overwrite, bool backup, bool appData, int keep, QObject *parent)
    : QObject(parent), m_sourceDir(sourceDir), m_destDir(destDir), m_destSize(destSize), m_overwrite(overwrite), m_backup(backup), m_appData(appData), m_keep(keep), m_filesCopied(0) {
}

void CopyWorker::startCopy() {
    m_stopRequested = false;
    m_totalFiles = 0;
    m_totalSize = 0;

    countFiles(m_sourceDir);

    //QMutexLocker locker(&m_copymutex);
    if (m_stopRequested) {
        return;
    }
    QDateTime dateTime1 = dateTime1.currentDateTime();

     emit messageOccurred(QString("Files: %1 (%2)").arg(QLocale(QLocale::English).toString(m_totalFiles)).arg(sizeToText(m_totalSize)));

    if(true == m_backup && m_totalSize > m_destSize) {
        emit messageOccurred("Not enought free space on destination drive");
        emit errorOccurred("Not enought free space on destination drive");
        return;
    }

    QStringList parts = m_destDir.split("/", Qt::SkipEmptyParts);
    QString filePath = parts[0] + "/" + parts[1] + "/" + parts[2] + ".ini";
    QString dataPath = parts[0] + "/" + parts[1] ;

    QDir dir(dataPath);
    QStringList filters;
    filters << "*.ini";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    // Sort by filename (case-insensitive)
    std::sort(fileList.begin(), fileList.end(), [](const QFileInfo &a, const QFileInfo &b) {
        return a.fileName().toLower() > b.fileName().toLower();
    });

    if(m_backup) {
        int cpt = 0;
        for (const QFileInfo &fileInfo : fileList) {
            if(cpt++ >= m_keep) {

                emit messageOccurred("Please wait while deleting old backup " + fileInfo.baseName());

                QDir deldir(dataPath +"/" + fileInfo.baseName());
                m_deletedFiles = 0;
                m_stopRequested = false;
                deleteRecursively(deldir);

                //QMutexLocker locker(&m_copymutex);
                if (m_stopRequested) {
                    return;
                }

                QFile file(dataPath +"/" + fileInfo.baseName() + ".ini");
                file.remove();

                qDebug() << "delete " + dataPath +"/" + fileInfo.baseName();
            }
            else
                qDebug() << fileInfo.baseName();
        }
    }

    QSettings settings(filePath, QSettings::IniFormat);

    if(m_backup) {
        settings.setValue("General/version", "1.0");
        settings.setValue("General/firstOpened", QDateTime::currentDateTime());
        settings.sync();
    }

    bool success = copyDirectoryRecursively(m_sourceDir, m_destDir, m_totalFiles);

    if(success ) {
        if(m_backup) {
            settings.setValue("General/lastOpened", QDateTime::currentDateTime());
            settings.sync();
        }
    }

    if(success) {
      bool success_dt = copyDateTimeRecursively(m_sourceDir, m_destDir);
      emit copyFinished(success);

      QDateTime dateTime2 = dateTime2.currentDateTime();
      qint64 msDifference = dateTime2.toMSecsSinceEpoch() - dateTime1.toMSecsSinceEpoch();

      // Calculate difference in seconds
      qint64 secondsDifference = msDifference / 1000;
      double minutesDifference = secondsDifference / 60.0;
      QString formattedMinutes = QString::number(minutesDifference, 'f', 2);
      emit messageOccurred(QString("Elapsed time: %1 minutes").arg(formattedMinutes));
    }
    else {
        emit errorOccurred("Unknown error");
    }

}

QString CopyWorker::sizeToText(qint64 size)
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

bool CopyWorker::deleteRecursively(QDir &dir)
{
    bool success = true;
    QDirIterator it(dir.absolutePath(), QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        QString itemPath = info.absoluteFilePath();

        if (info.isDir()) {
            QDir subDir(itemPath);
            //qDebug() << "D " + itemPath;
            if (!deleteRecursively(subDir)) {
                success = false;
                break;
            }
        } else {
            //qDebug() << "F " + itemPath;
            QFile destFile(itemPath);

            if (destFile.remove()) {

                m_deletedFiles++;
                emit deleteUpdated(m_deletedFiles);//emit progress(deletedFiles, totalFiles, QString("Deleted: %1").arg(itemPath));
            } else {
                success = false;
                qDebug() << destFile.errorString();
                break;
                //emit progress(deletedFiles, totalFiles, QString("Failed to delete: %1").arg(itemPath));
            }
        }

       // QMutexLocker locker(&m_copymutex);
        if (m_stopRequested) {
            emit errorOccurred("Terminated by User");
            break;
        }
    }

    // Delete the directory itself if it's empty
    if (!dir.removeRecursively()) {
        success = false;
        //emit progress(deletedFiles, totalFiles, QString("Failed to delete directory: %1").arg(dir.absolutePath()));
    }

    return success;
}

void CopyWorker::countFiles(const QString &dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) return;

    QFileInfoList entries;

    if(m_appData)
        entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Hidden | QDir::AllEntries);
    else
        entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);


    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            emit verifyUpdated(m_totalSize);

            countFiles(entry.absoluteFilePath());
        } else if (entry.isFile()) {            

            m_totalSize += entry.size();
            m_totalFiles++;
        }

        QMutexLocker locker(&m_copymutex);
        if (m_stopRequested) {
            emit errorOccurred("Terminated by User");
            break;
        }
    }

    return;
}

void CopyWorker::requestStop() {
    QMutexLocker locker(&m_copymutex);
    m_stopRequested = true;
}

bool CopyWorker::copyDateTimeRecursively(const QString &sourceDir, const QString &destDir)
{
    QDir source(sourceDir);
    QDir destination(destDir);

    if (!source.exists()) {
        emit errorOccurred("Source directory does not exist: " + sourceDir);
        return false;
    }

    if (!destination.exists()) {
        //qDebug() << destDir;
        if(!destination.mkpath(".")) {
            emit errorOccurred("Failed to create destination directory: " + destDir);
            return false;
        }
    }

    QFileInfoList entries;

    if(m_appData)
        entries = source.entryInfoList(QDir::NoDotAndDotDot | QDir::Hidden | QDir::AllEntries);
    else
        entries = source.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);

    for (const QFileInfo &entry : entries) {
        QString sourcePath = entry.absoluteFilePath();
        QString destPath = destDir + QDir::separator() + entry.fileName();

        if (entry.isDir()) {

            HANDLE hFind;
            FILETIME CurrentTime;
            FILETIME CurrentTime2;
            FILETIME CurrentTime3;

            std::wstring src = sourcePath.toStdWString();
            std::wstring dst = destPath.toStdWString();

            // Get the date from an old file, to set current time use GetSystemTimeAsFileTime(&CurrentTime)
            hFind = CreateFileW( src.c_str(), GENERIC_WRITE,FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,0);
            GetFileTime(hFind, &CurrentTime, &CurrentTime2, &CurrentTime3);
            CloseHandle(hFind);

            CreateDirectory( dst.c_str(), NULL);

            hFind = CreateFileW( dst.c_str(), GENERIC_WRITE,FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,0);
            SetFileTime(hFind, &CurrentTime, &CurrentTime2, &CurrentTime3);
            CloseHandle(hFind);

            if (!copyDateTimeRecursively(sourcePath, destPath)) {
                emit errorOccurred("Failed to create destination directory: " + destPath);
                return false;
            }
        }
    }

    return true;
}

bool CopyWorker::copyDirectoryRecursively(const QString &sourceDir, const QString &destDir, int totalFiles) {
    QDir source(sourceDir);
    QDir destination(destDir);

    if (!source.exists()) {
        emit errorOccurred("Source directory does not exist: " + sourceDir);
        return false;
    }

    if (!destination.exists()) {
        //qDebug() << destDir;
        if(!destination.mkpath(".")) {

            if(destDir.endsWith(".lnk")) {
                return true;
            }

            QThread::msleep(500);
            if(!destination.mkpath(".")) {

                emit errorOccurred(destDir);
                return false;
            }
        }
    }

    QFileInfoList entries;

    if(m_appData)
        entries = source.entryInfoList(QDir::NoDotAndDotDot | QDir::Hidden | QDir::AllEntries);
    else
        entries = source.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);


    for (const QFileInfo &entry : entries) {
        QString sourcePath = entry.absoluteFilePath();

        QString efn = entry.fileName();

        QString destPath = destDir + "/" + entry.fileName();

        QStringList parts = sourcePath.split("/", Qt::SkipEmptyParts);

        if (entry.isDir()) {
            if (!copyDirectoryRecursively(sourcePath, destPath, totalFiles)) {
                //emit errorOccurred("Failed to copy destination directory: " + destPath);
                return false;
            }
        } else if (entry.isFile()) {
            if (QFile::exists(destPath)) {
                if (m_overwrite) {
                    if (!QFile::remove(destPath)) {
                        emit errorOccurred("Failed to remove existing file: " + destPath);
                        return false;
                    }
                } else {
                    m_filesCopied++; // copy simulation
                    emit progressUpdated(m_filesCopied, totalFiles);
                    continue;
                }
            }
            //qDebug() << destPath;

            if(parts[3] != "CrossDevice") {

            QFileInfo sourceInfo(sourcePath);
            QDateTime creationTime = sourceInfo.birthTime();    // Creation time (if available)
            QDateTime lastModified = sourceInfo.lastModified(); // Last modification time
            emit copyOccurred(destPath);

            //qDebug() << sourcePath;

            if (!copyFileByChunks(sourcePath, destPath)){
                qDebug() << sourcePath;
                //emit errorOccurred("Failed to copyFileByChunks" + destPath);
         //              emit errorOccurred("Failed to copy file: " + sourcePath + " to " + destPath);
                //    return false;

            }

            QFile destFile(destPath);

            destFile.open(QIODevice::Append);

            if (lastModified.isValid()) {

                destFile.setFileTime(lastModified, QFileDevice::FileModificationTime);
                if (creationTime.isValid()) {
                    destFile.setFileTime(creationTime, QFileDevice::FileBirthTime);
                }
            }

            destFile.close();
            }

            m_filesCopied++;
            emit progressUpdated(m_filesCopied, totalFiles);
            QMutexLocker locker(&m_copymutex);
            if (m_stopRequested) {
                emit errorOccurred("Terminated by User");
                //break;
                return false;
            }
        }
    }

    return true;
}

bool CopyWorker::copyFileByChunks(const QString& sourcePath, const QString& destinationPath) // 1MB default chunk size
{
    qint64 chunkSize = (1024 * 1024);
    QFile sourceFile(sourcePath);
    QFile destFile(destinationPath);
    QFileInfo fileInfo(sourcePath);

    qint64 totalSize = sourceFile.size();

    if (!fileInfo.isReadable()) {
        qDebug() << "Not Readable:";
        return false;
    }

    if(totalSize < chunkSize || fileInfo.isSymLink())
    {
        if(!QFile::copy(sourcePath, destinationPath)) {
            qDebug() << "Failed to copy:" << sourceFile.errorString();
            return false;
        }
        else {
            return true;
        }
    }

    // Open source file for reading
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open source file:" << sourceFile.errorString();
        return false;
    }

    // Open destination file for writing
    if (!destFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open destination file:" << destFile.errorString();
        sourceFile.close();
        return false;
    }

    // Get total file size
    //qint64 totalSize = sourceFile.size();
    qint64 bytesWritten = 0;

    // Buffer for chunk reading
    QByteArray buffer;
    buffer.resize(chunkSize);

    // Copy file in chunks
    while (!sourceFile.atEnd()) {
        // Read a chunk
        qint64 bytesRead = sourceFile.read(buffer.data(), chunkSize);
        if (bytesRead < 0) {
            qDebug() << "Error reading source file:" << sourceFile.errorString();
            sourceFile.close();
            destFile.close();
            return false;
        }

        // Write the chunk
        qint64 currentBytesWritten = destFile.write(buffer.constData(), bytesRead);
        if (currentBytesWritten != bytesRead) {


            qDebug() << "Error writing to destination file:" << destFile.errorString();
            sourceFile.close();
            destFile.close();
            return false;
        }

        bytesWritten += currentBytesWritten;

        emit fileProgressUpdated(bytesWritten, totalSize);

        QMutexLocker locker(&m_copymutex);
        if (m_stopRequested) {
            sourceFile.close();
            destFile.close();
            emit errorOccurred("Terminated by User");
            return false;
        }
    }

    // Clean up
    sourceFile.close();
    destFile.close();

    // Verify the copy
    if (bytesWritten != totalSize) {
        qDebug() << "Copy incomplete: wrote" << bytesWritten << "of" << totalSize << "bytes";
        //return QFile::copy(sourcePath, destinationPath);
        return false;
    }

    return true;
}
