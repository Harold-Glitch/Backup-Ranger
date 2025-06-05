#include "mainwindow.h"

#include <QApplication>
#include <QSharedMemory>

#include <Windows.h>
#include <QDebug>
#include <QMessageBox>
#include <ShlObj.h>

bool isRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        if (!CheckTokenMembership(nullptr, adminGroup, &isAdmin)) {
            DWORD error = GetLastError();
            QString hexString = QString("0x%1").arg(static_cast<unsigned long>(error), 8, 16, QChar('0'));
            qDebug() << "CheckTokenMembership failed with error:" << hexString;
        }
        FreeSid(adminGroup);
    }

    qDebug() << isAdmin;
    return isAdmin;
}

bool enableLongPaths() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\FileSystem",
        0,
        KEY_SET_VALUE,
        &hKey
        );

    if (result != ERROR_SUCCESS) {
        QString hexString = QString("0x%1").arg(static_cast<unsigned long>(result), 8, 16, QChar('0'));
        qDebug() << "RegOpenKeyEx failed with error:" << hexString;
        return false;
    }

    DWORD value = 1; // Enable long paths
    result = RegSetValueExW(
        hKey,
        L"LongPathsEnabled",
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&value),
        sizeof(value)
        );

    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        QString hexString = QString("0x%1").arg(static_cast<unsigned long>(result), 8, 16, QChar('0'));
        qDebug() << "RegSetValueEx failed with error:" << hexString;
        return false;
    }

    qDebug() << "LongPathsEnabled set to 1 successfully.";
    return true;
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (!isRunningAsAdmin()) {
        return 1;
    }

    qDebug() << "Running with administrative privileges.";

    enableLongPaths();

    // Create a unique identifier for the shared memory (e.g., application name)
    QSharedMemory sharedMemory("Backup-Ranger");

    // Try to attach to the shared memory
    if (!sharedMemory.create(1)) {
        // If creation fails, another instance is likely running
        qDebug() << "Another instance is already running.";
        return 1; // Exit the application
    }

    QCoreApplication::setApplicationVersion("1.56"); // Set your app's version

    MainWindow w;
    w.show();
    return a.exec();
}
