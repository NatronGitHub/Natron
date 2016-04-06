#ifndef KEYCHAINMANAGER_H
#define KEYCHAINMANAGER_H

#include <QObject>

class KeychainManager : public QObject
{
    Q_OBJECT
public:
    explicit KeychainManager(QObject *parent = 0);

public Q_SLOTS:
    QString readValue(QString key);
    bool writeValue(QString key, QString value);
    bool removeValue(QString key);
};

#endif // KEYCHAINMANAGER_H
