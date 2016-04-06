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

Q_SIGNALS:
public Q_SLOTS:
    bool webserverStart();
    void webserverStop();
    void webserverHandler(QHttpRequest *req, QHttpResponse *resp);
    QString webserverHTMLParser(QString html, QString path) const;
    void webserverSetPort(int port);
private:
    QHttpServer *server;
};

NATRON_NAMESPACE_EXIT;
#endif // DOCUMENTATIONMANAGER_H
