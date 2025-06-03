#ifndef DELETEWORKER_H
#define DELETEWORKER_H


#include <QObject>

class deleteWorker : public QObject
{
    Q_OBJECT

public:
    explicit deleteWorker(const QString &dirPath, QObject *parent = nullptr);

public slots:
    void startDeletion(); // Start the deletion process

signals:
    void progress(int value, int total); // Emit progress updates
    void finished(bool success, const QString &message); // Emit completion status

private:
    QString m_dirPath; // Directory to delete
};

#endif // DELETEWORKER_H
