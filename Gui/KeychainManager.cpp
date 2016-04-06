#include "KeychainManager.h"
#include "keychain.h"
#include <QEventLoop>
#include <iostream>

KeychainManager::KeychainManager(QObject *parent) : QObject(parent)
{

}

QString KeychainManager::readValue(QString key)
{
    QString result;
    if (!key.isEmpty()) {
        QKeychain::ReadPasswordJob keychain(QString::fromUtf8("natron-keychain"));
        keychain.setKey(key);
        QEventLoop loop;
        keychain.connect(&keychain, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
        keychain.start();
        loop.exec();
        if (!keychain.error()) {
            result = keychain.textData();
        }
        else {
            std::cout << keychain.errorString().toStdString() << std::endl;
        }
    }
    return result;
}

bool KeychainManager::writeValue(QString key, QString value)
{
    bool result = false;
    if (!key.isEmpty() && !value.isEmpty()) {
        QKeychain::WritePasswordJob keychain(QString::fromUtf8("natron-keychain"));
        keychain.setKey(key);
        keychain.setTextData(value);
        QEventLoop loop;
        keychain.connect(&keychain, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
        keychain.start();
        loop.exec();
        if (!keychain.error()) {
            result = true;
        }
        else {
            std::cout << keychain.errorString().toStdString() << std::endl;
        }
    }
    return result;
}

bool KeychainManager::removeValue(QString key)
{
    bool result = false;
    if (!key.isEmpty()) {
        QKeychain::DeletePasswordJob keychain(QString::fromUtf8("natron-keychain"));
        keychain.setKey(key);
        QEventLoop loop;
        keychain.connect(&keychain, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
        keychain.start();
        loop.exec();
        if (!keychain.error()) {
            result = true;
        }
        else {
            std::cout << keychain.errorString().toStdString() << std::endl;
        }
    }
    return result;
}

