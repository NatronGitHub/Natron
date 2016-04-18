#ifndef DOCUMENTATIONMANAGER_H
#define DOCUMENTATIONMANAGER_H

#include <QObject>

#include "Gui/GuiFwd.h"

#include "qhttpserver.h"
#include "qhttprequest.h"
#include "qhttpresponse.h"

NATRON_NAMESPACE_ENTER;

class DocumentationManager : public QObject
{
    Q_OBJECT
public:
    explicit DocumentationManager(QObject *parent = 0);
    ~DocumentationManager();

public Q_SLOTS:
    void startServer();
    void stopServer();
    QString parser(QString html, QString path) const;
private Q_SLOTS:
    void handler(QHttpRequest *req, QHttpResponse *resp);
    void setPort(int port);
private:
    QHttpServer *server;
};

NATRON_NAMESPACE_EXIT;
#endif // DOCUMENTATIONMANAGER_H
