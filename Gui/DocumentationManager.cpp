#include "DocumentationManager.h"
#include "Gui/GuiApplicationManager.h" // appPTR

NATRON_NAMESPACE_ENTER;
DocumentationManager::DocumentationManager(QObject *parent) : QObject(parent)
{
    server = new QHttpServer(this);
    connect(server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)), this, SLOT(webserverHandler(QHttpRequest*, QHttpResponse*)));
    connect(server,SIGNAL(newPort(int)), this, SLOT(webserverSetPort(int)));
}

bool DocumentationManager::webserverStart()
{
    webserverStop();
    if (server->listen(QHostAddress::Any, 0)) {
        return true;
    }
    else {
        return false;
    }
}

void DocumentationManager::webserverStop()
{
    server->close();
}

void DocumentationManager::webserverHandler(QHttpRequest *req, QHttpResponse *resp)
{
    Q_UNUSED(req)

    QString docDir = appPTR->getApplicationBinaryPath() + QString::fromUtf8("/../Resources/docs/html/");
    QString page = req->url().toString();
    QByteArray body;

#ifdef DEBUG
    qDebug() << "www client requested" << page;
#endif

    // get options
    QStringList options;
    if (page.contains(QString::fromUtf8("?"))) {
        QStringList split = page.split(QString::fromUtf8("?"),QString::SkipEmptyParts);
        page = split.takeFirst();
        if (split.length()>0) {
            QString rawOptions = split.takeLast();
            options = rawOptions.split(QString::fromUtf8("&"),QString::SkipEmptyParts);
        }
    }

    // default page
    if (page == QString::fromUtf8("/") || page.isEmpty()) {
        page = QString::fromUtf8("index.html");
    }

    // remove slashes
    if (page.startsWith(QString::fromUtf8("/"))) {
        page.remove(0,1);
    }
    if (page.endsWith(QString::fromUtf8("/"))) {
        page = page.left(page.length()-1);
    }

    // add custom content
    if (page == QString::fromUtf8("_plugin.html")) {
        for (int i = 0; i < options.size(); ++i) {
            if (options.at(i).contains(QString::fromUtf8("id="))) {
                QStringList split = options.at(i).split(QString::fromUtf8("="));
                if (split.length()>0) {
                    QString pluginID = split.takeLast();
                    if (!pluginID.isEmpty()) {
                        CreateNodeArgs args(pluginID, eCreateNodeReasonInternal, boost::shared_ptr<NodeCollection>());
                        args.createGui = false;
                        args.addToProject = false;
                        NodePtr node = appPTR->getTopLevelInstance()->createNode(args);
                        if (node) {
                            QString html = node->makeHTMLDocumentation();
                            QFileInfo screenshot(docDir+QString::fromUtf8("_static/plugins/")+pluginID+QString::fromUtf8(".png"));
                            QFileInfo moreinfo(docDir+QString::fromUtf8("_static/plugins/")+pluginID+QString::fromUtf8(".html"));
                            /// TODO also add actual plugin icon from it's folder
                            if (screenshot.exists()) {
                                html.replace(QString::fromUtf8("<!--ADD_SCREENSHOT_HERE-->"),QString::fromUtf8("<p><img class=\"screenshot\" src='/_static/plugins/")+pluginID+QString::fromUtf8(".png'></p>"));
                            }
                            /// TODO also add actual plugin html from it's folder
                            if (moreinfo.exists()) {
                                QString morehtml;
                                QFile moreinfoFile(moreinfo.absoluteFilePath());
                                if (moreinfoFile.open(QIODevice::Text|QIODevice::ReadOnly)) {
                                    morehtml = QString::fromUtf8(moreinfoFile.readAll());
                                    moreinfoFile.close();
                                }
                                if (!morehtml.isEmpty()) {
                                    html.replace(QString::fromUtf8("<!--ADD_MORE_HERE-->"),morehtml);
                                }
                            }
                            html.replace(QString::fromUtf8("\n"),QString::fromUtf8("</p><p>"));
                            html = webserverHTMLParser(html,docDir);
                            body=html.toAscii();
                        }
                    }
                }
            }
        }
    }
    else if (page == QString::fromUtf8("_group.html")) {
        QString html;
        QString group;
        QString groupHeader = QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\"><html><head><title>Natron User Guide Menu</title><link rel=\"stylesheet\" href=\"_static/default.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"_static/pygments.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"_static/style.css\" type=\"text/css\" /></head><body>");
        QString groupBodyEnd = QString::fromUtf8("</ul></div></div></div></div>");
        QString groupFooter = QString::fromUtf8("</body></html>");
        for (int i = 0; i < options.size(); ++i) {
            if (options.at(i).contains(QString::fromUtf8("id="))) {
                QStringList split = options.at(i).split(QString::fromUtf8("="),QString::SkipEmptyParts);
                if (split.length()>0) {
                    QString groupID = split.takeLast();
                    group = groupID;
                }
            }
        }
        if (!group.isEmpty()) {
            QVector<QStringList> plugins;
            std::list<std::string> pluginIDs = appPTR->getPluginIDs();
            for (std::list<std::string>::iterator it=pluginIDs.begin(); it != pluginIDs.end(); ++it) {
                Plugin* plugin = 0;
                QString pluginID = QString::fromUtf8(it->c_str());
                plugin = appPTR->getPluginBinary(pluginID,-1,-1,false);
                if (plugin) {
                    qDebug() << plugin->getIconFilePath();
                    QStringList groupList = plugin->getGrouping();
                        if (groupList.at(0)==group) {
                            QStringList result;
                            result<< pluginID << plugin->getPluginLabel();
                            plugins.append(result);
                        }
                }
            }
            if (!plugins.isEmpty()) {
                QString groupBodyStart = QString::fromUtf8("<div class=\"document\"><div class=\"documentwrapper\"><div class=\"body\"><h1>")+group+QString::fromUtf8("</h1><p>")+QObject::tr("This manual is intended as a reference for all the parameters within each node in ")+group+QString::fromUtf8(".</p><div class=\"toctree-wrapper compound\"><ul>");
                html.append(groupHeader);
                html.append(groupBodyStart);
                for (int i = 0; i < plugins.size(); ++i) {
                    QStringList pluginInfo = plugins.at(i);
                    QString plugID,plugName;
                    if (pluginInfo.length()==2) {
                        plugID=pluginInfo.at(0);
                        plugName=pluginInfo.at(1);
                    }
                    if (!plugID.isEmpty() && !plugName.isEmpty()) {
                        html.append(QString::fromUtf8("<li class=\"toctree-l1\"><a href='/_plugin.html?id=")+plugID+QString::fromUtf8("'>")+plugName+QString::fromUtf8("</a></li>"));
                    }
                }
                html.append(groupBodyEnd);
                html.append(groupFooter);
            }
        }
        else {
            QString groupBodyStart = QString::fromUtf8("<div class=\"document\"><div class=\"documentwrapper\"><div class=\"body\"><h1>")+QObject::tr("Reference Guide")+QString::fromUtf8("</h1><p>")+QObject::tr("This manual is intended as a reference for all the parameters within each node in Natron.")+QString::fromUtf8("</p><div class=\"toctree-wrapper compound\"><ul>");
            html.append(groupHeader);
            html.append(groupBodyStart);
            QStringList groups;
            std::list<std::string> pluginIDs = appPTR->getPluginIDs();
            for (std::list<std::string>::iterator it=pluginIDs.begin(); it != pluginIDs.end(); ++it) {
                Plugin* plugin = 0;
                QString pluginID = QString::fromUtf8(it->c_str());
                plugin = appPTR->getPluginBinary(pluginID,-1,-1,false);
                if (plugin) {
                    QStringList groupList = plugin->getGrouping();
                    groups << groupList.at(0);
                }
            }
            groups.removeDuplicates();
            for (int i = 0; i < groups.size(); ++i) {
                html.append(QString::fromUtf8("\n<li class='toctree-l1'><a href='/_group.html?id=")+groups.at(i)+QString::fromUtf8("'>")+groups.at(i)+QString::fromUtf8("</a></li>"));
            }
            html.append(groupBodyEnd);
            html.append(groupFooter);
        }
        html = webserverHTMLParser(html,docDir);
        body = html.toAscii();
    }

    // get static file
    QFileInfo staticFileInfo;
    if (page.endsWith(QString::fromUtf8(".html")) || page.endsWith(QString::fromUtf8(".css")) || page.endsWith(QString::fromUtf8(".js")) || page.endsWith(QString::fromUtf8(".txt")) || page.endsWith(QString::fromUtf8(".png")) || page.endsWith(QString::fromUtf8(".jpg"))) {
        staticFileInfo = docDir+page;
    }
    if (staticFileInfo.exists() && body.isEmpty()) {
        QFile staticFile(staticFileInfo.absoluteFilePath());
        if (staticFile.open(QIODevice::ReadOnly)) {
            if (page.endsWith(QString::fromUtf8(".html")) || page.endsWith(QString::fromUtf8(".htm"))) {
                QString input = QString::fromUtf8(staticFile.readAll());
                if (input.contains(QString::fromUtf8("http://sphinx.pocoo.org/")) && !input.contains(QString::fromUtf8("mainMenu"))) {
                    body = webserverHTMLParser(input,docDir).toAscii();
                }
                else {
                    body = input.toAscii();
                }
            }
            else {
                body = staticFile.readAll();
            }
            staticFile.close();
        }
    }

    // page not found
    /// should probably just issue a 404 and don't write anything to body?
    if (body.isEmpty()) {
        QString notFound = QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\"><html><head><title>Page not found</title></head><body><p>Page not found</p></body></html>");
        body = notFound.toAscii();
    }

    // set header(s)
    resp->setHeader(QString::fromUtf8("Content-Length"), QString::number(body.size()));
    if (page.endsWith(QString::fromUtf8(".png"))) {
        resp->setHeader(QString::fromUtf8("Content-Type"), QString::fromUtf8("image/png"));
    }
    else if (page.endsWith(QString::fromUtf8(".jpg"))) {
        resp->setHeader(QString::fromUtf8("Content-Type"), QString::fromUtf8("image/jpeg"));
    }
    else if (page.endsWith(QString::fromUtf8(".html")) || page.endsWith(QString::fromUtf8(".htm"))) {
        resp->setHeader(QString::fromUtf8("Content-Type"),QString::fromUtf8("text/html"));
    }
    else if (page.endsWith(QString::fromUtf8(".css"))) {
        resp->setHeader(QString::fromUtf8("Content-Type"),QString::fromUtf8("text/css"));
    }
    else if (page.endsWith(QString::fromUtf8(".js"))) {
        resp->setHeader(QString::fromUtf8("Content-Type"),QString::fromUtf8("application/javascript"));
    }
    else if (page.endsWith(QString::fromUtf8(".txt"))) {
        resp->setHeader(QString::fromUtf8("Content-Type"),QString::fromUtf8("text/plain"));
    }

    // return result
    resp->writeHead(200); /// should be fixed, can't always be 200 ;)
    resp->end(body);
}

QString DocumentationManager::webserverHTMLParser(QString html, QString path) const
{
    QString result = html;

    // get static menu from index.html
    QFile indexFile(path+QString::fromUtf8("/index.html"));
    QString menuHTML;
    menuHTML.append(QString::fromUtf8("<body>\n"));
    menuHTML.append(QString::fromUtf8("<div id=\"mainMenu\">\n"));
    menuHTML.append(QString::fromUtf8("<ul>\n"));
    if (indexFile.exists()) {
        if(indexFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
            qDebug() << "got index.html";
            QStringList menuResult;
            bool getMenu = false;
            while (!indexFile.atEnd()) {
                QString line = QString::fromAscii(indexFile.readLine());
                if (line == QString::fromUtf8("<div class=\"toctree-wrapper compound\">\n")) {
                    getMenu=true;
                }
                if (getMenu) {
                    menuResult << line;
                }
                if (line == QString::fromUtf8("</div>\n")) {
                    getMenu=false;
                }
            }
            if (!menuResult.isEmpty()) {
                int menuEnd = menuResult.size()-2; /// hackish, but needed to integrate static+dynamic
                for (int i = 0; i < menuEnd; ++i) {
                    QString tmp = menuResult.at(i);
                    menuHTML.append(tmp);
                }
            }
            indexFile.close();
        }
    }
    else {
        std::cout << "index.html does not exist! No menu will be generated for static content." << std::endl;
    }

    /// TODO probably a better way to get categories...
    QStringList groups;
    std::list<std::string> pluginIDs = appPTR->getPluginIDs();
    for (std::list<std::string>::iterator it=pluginIDs.begin(); it != pluginIDs.end(); ++it) {
        Plugin* plugin = 0;
        QString pluginID = QString::fromUtf8(it->c_str());
        plugin = appPTR->getPluginBinary(pluginID,-1,-1,false);
        if (plugin) {
            QStringList groupList = plugin->getGrouping();
            groups << groupList.at(0);
        }
    }
    groups.removeDuplicates();
    QString refHTML;
    refHTML.append(QString::fromUtf8("<li class=\"toctree-l1\"><a href=\"#\">Reference Guide</a>\n"));
    refHTML.append(QString::fromUtf8("<ul>\n"));
    for (int i = 0; i < groups.size(); ++i) {
        refHTML.append(QString::fromUtf8("\n<li class='toctree-l2'><a href='/_group.html?id=")+groups.at(i)+QString::fromUtf8("'>")+groups.at(i)+QString::fromUtf8("</a></li>"));
    }
    refHTML.append(QString::fromUtf8("\n</ul>\n</li>\n</ul>\n"));

    // return result
    menuHTML.append(refHTML);
    menuHTML.append(QString::fromUtf8("</div>\n</div>\n"));
    result.replace(QString::fromUtf8("<body>"),menuHTML);
    return result;
}

void DocumentationManager::webserverSetPort(int port)
{
#ifdef DEBUG
    qDebug() << "webserver is attached to port" << port;
#endif
    appPTR->getCurrentSettings()->setServerPort(port);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_DocumentationManager.cpp"
