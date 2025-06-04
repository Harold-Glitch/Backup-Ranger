#include "mainwindow.h"

#include <QApplication>
#include <QSharedMemory>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Create a unique identifier for the shared memory (e.g., application name)
    QSharedMemory sharedMemory("Backup-Ranger");

    // Try to attach to the shared memory
    if (!sharedMemory.create(1)) {
        // If creation fails, another instance is likely running
        qDebug() << "Another instance is already running.";
        return 1; // Exit the application
    }

    QCoreApplication::setApplicationVersion("1.55"); // Set your app's version

    MainWindow w;
    w.show();
    return a.exec();
}
