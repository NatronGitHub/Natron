/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "DocumentationManager.h"

#include <QFile>
#include <QFileInfo>
#include <QVector>

#include <iostream>
#include <exception>

#include "qhttpserver.h"
#include "qhttprequest.h"
#include "qhttpresponse.h"

#include "Gui/GuiApplicationManager.h" // appPTR
#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"


NATRON_NAMESPACE_ENTER;
DocumentationManager::DocumentationManager(QObject *parent) : QObject(parent)
{
}

DocumentationManager::~DocumentationManager()
{
}

void
DocumentationManager::startServer()
{
    server = new QHttpServer(this);
    connect( server, SIGNAL(newRequest(QHttpRequest*,QHttpResponse*)), this, SLOT(handler(QHttpRequest*,QHttpResponse*)) );
    connect( server, SIGNAL(newPort(int)), this, SLOT(setPort(int)) );
    server->listen(QHostAddress::Any, 0);
}

void
DocumentationManager::stopServer()
{
    server->close();
}

void
DocumentationManager::handler(QHttpRequest *req,
                              QHttpResponse *resp)
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
    if ( page.contains( QString::fromUtf8("?") ) ) {
        QStringList split = page.split(QString::fromUtf8("?"), QString::SkipEmptyParts);
        page = split.takeFirst();
        if (split.length() > 0) {
            QString rawOptions = split.takeLast();
            options = rawOptions.split(QString::fromUtf8("&"), QString::SkipEmptyParts);
        }
    }

    // default page
    if ( ( page == QString::fromUtf8("/") ) || page.isEmpty() ) {
        page = QString::fromUtf8("index.html");
    }

    // remove slashes
    if ( page.startsWith( QString::fromUtf8("/") ) ) {
        page.remove(0, 1);
    }
    if ( page.endsWith( QString::fromUtf8("/") ) ) {
        page = page.left(page.length() - 1);
    }

    // add custom content
    if ( page == QString::fromUtf8("_plugin.html") ) {
        for (int i = 0; i < options.size(); ++i) {
            if ( options.at(i).contains( QString::fromUtf8("id=") ) ) {
                QStringList split = options.at(i).split( QString::fromUtf8("=") );
                if (split.length() > 0) {
                    QString pluginID = split.takeLast();
                    if ( !pluginID.isEmpty() ) {
                        Plugin* plugin = 0;
                        try {
                            plugin = appPTR->getPluginBinary(pluginID, -1, -1, false);
                        }
                        catch (const std::exception& e) {
                            std::cerr << e.what() << std::endl;
                        }
                        if (plugin) {
                            QString isPyPlug = plugin->getPythonModule();
                            if ( !isPyPlug.isEmpty() ) { // loading pyplugs crash, so redirect to group
                                pluginID = QString::fromUtf8("fr.inria.built-in.Group");
                            }
                            CreateNodeArgs args( pluginID, eCreateNodeReasonInternal, boost::shared_ptr<NodeCollection>() );
                            args.createGui = false;
                            args.addToProject = false;
                            NodePtr node = appPTR->getTopLevelInstance()->createNode(args);
                            if (node) {
                                QString html = node->makeHTMLDocumentation(false);
                                /*QFileInfo moreinfo(docDir+QString::fromUtf8("_static/plugins/")+pluginID+QString::fromUtf8(".html"));
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
                                   }*/
                                html.replace( QString::fromUtf8("\n"), QString::fromUtf8("</p><p>") );
                                html = parser(html, docDir);
                                body = html.toLatin1();
                            }
                        }
                    }
                }
            }
        }
        if ( body.isEmpty() ) {
            QString notFound = QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\"><html><head><title>Plugin not found</title><link rel=\"stylesheet\" href=\"/_static/default.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"/_static/pygments.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"/_static/style.css\" type=\"text/css\" /><script type=\"text/javascript\" src=\"/_static/jquery.js\"></script><script type=\"text/javascript\" src=\"/_static/dropdown.js\"></script></head><body><div class=\"document\"><div class=\"documentwrapper\"><div class=\"body\"><h1>Plugin not found</h1></div></div></div></body></html>");
            body = parser(notFound, docDir).toLatin1();
        }
    } else if ( page == QString::fromUtf8("_prefs.html") )   {
        boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
        QString html = settings->makeHTMLDocumentation(false, false);
        html = parser(html, docDir);
        body = html.toLatin1();
    } else if ( page == QString::fromUtf8("_group.html") )   {
        QString html;
        QString group;
        QString groupHeader = QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\"><html><head><title>__REPLACE_TITLE__ - ") + QString::fromUtf8(NATRON_APPLICATION_NAME) + QString::fromUtf8(" ") + QString::fromUtf8(NATRON_VERSION_STRING) + QString::fromUtf8(" documentation") + QString::fromUtf8("</title><link rel=\"stylesheet\" href=\"_static/default.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"_static/pygments.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"_static/style.css\" type=\"text/css\" /></head><body>");
        QString groupBodyEnd = QString::fromUtf8("</ul></div></div></div></div>");
        QString groupFooter = QString::fromUtf8("</body></html>");
        QString navHeader = QString::fromUtf8("<div class=\"related\"><h3>Navigation</h3><ul><li><a href=\"/index.html\">Natron 2.0 documentation</a> &raquo;</li>");
        QString navFooter = QString::fromUtf8("</ul></div>");
        for (int i = 0; i < options.size(); ++i) {
            if ( options.at(i).contains( QString::fromUtf8("id=") ) ) {
                QStringList split = options.at(i).split(QString::fromUtf8("="), QString::SkipEmptyParts);
                if (split.length() > 0) {
                    QString groupID = split.takeLast();
                    group = groupID;
                }
            }
        }
        if ( !group.isEmpty() ) {
            QVector<QStringList> plugins;
            std::list<std::string> pluginIDs = appPTR->getPluginIDs();
            for (std::list<std::string>::iterator it = pluginIDs.begin(); it != pluginIDs.end(); ++it) {
                Plugin* plugin = 0;
                QString pluginID = QString::fromUtf8( it->c_str() );
                try {
                    plugin = appPTR->getPluginBinary(pluginID, -1, -1, false);
                }
                catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }
                if (plugin) {
                    QStringList groupList = plugin->getGrouping();
                    if (groupList.at(0) == group) {
                        QStringList result;
                        result << pluginID << plugin->getPluginLabel();
                        plugins.append(result);
                    }
                }
            }
            if ( !plugins.isEmpty() ) {
                QString groupBodyStart = QString::fromUtf8("<div class=\"document\"><div class=\"documentwrapper\"><div class=\"body\"><h1>") + group + QString::fromUtf8("</h1><p>") + QObject::tr("This manual is intended as a reference for all the parameters within each node in ") + group + QString::fromUtf8(".</p><div class=\"toctree-wrapper compound\"><ul>");
                html.append(groupHeader);
                html.replace(QString::fromUtf8("__REPLACE_TITLE__"), group);
                html.append(navHeader);
                html.append( QString::fromUtf8("<li><a href=\"/_group.html\">Reference Guide</a> &raquo;</li>") );
                html.append(navFooter);
                html.append(groupBodyStart);
                for (int i = 0; i < plugins.size(); ++i) {
                    QStringList pluginInfo = plugins.at(i);
                    QString plugID, plugName;
                    if (pluginInfo.length() == 2) {
                        plugID = pluginInfo.at(0);
                        plugName = pluginInfo.at(1);
                    }
                    if ( !plugID.isEmpty() && !plugName.isEmpty() ) {
                        html.append( QString::fromUtf8("<li class=\"toctree-l1\"><a href='/_plugin.html?id=") + plugID + QString::fromUtf8("'>") + plugName + QString::fromUtf8("</a></li>") );
                    }
                }
                html.append(groupBodyEnd);
                html.append(groupFooter);
            }
        } else   {
            QString groupBodyStart = QString::fromUtf8("<div class=\"document\"><div class=\"documentwrapper\"><div class=\"body\"><h1>") + QObject::tr("Reference Guide") + QString::fromUtf8("</h1><p>") + QObject::tr("This manual is intended as a reference for all the parameters within each node in Natron.") + QString::fromUtf8("</p><div class=\"toctree-wrapper compound\"><ul>");
            html.append(groupHeader);
            html.replace( QString::fromUtf8("__REPLACE_TITLE__"), QString::fromUtf8("Reference Guide") );
            html.append(navHeader);
            html.append(navFooter);
            html.append(groupBodyStart);
            QStringList groups;
            std::list<std::string> pluginIDs = appPTR->getPluginIDs();
            for (std::list<std::string>::iterator it = pluginIDs.begin(); it != pluginIDs.end(); ++it) {
                Plugin* plugin = 0;
                QString pluginID = QString::fromUtf8( it->c_str() );
                try {
                    plugin = appPTR->getPluginBinary(pluginID, -1, -1, false);
                }
                catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }
                if (plugin) {
                    QStringList groupList = plugin->getGrouping();
                    groups << groupList.at(0);
                }
            }
            groups.removeDuplicates();
            for (int i = 0; i < groups.size(); ++i) {
                html.append( QString::fromUtf8("\n<li class='toctree-l1'><a href='/_group.html?id=") + groups.at(i) + QString::fromUtf8("'>") + groups.at(i) + QString::fromUtf8("</a></li>") );
            }
            html.append(groupBodyEnd);
            html.append(groupFooter);
        }
        html = parser(html, docDir);
        body = html.toLatin1();
    }

    // get static file
    QFileInfo staticFileInfo;
    if ( page.endsWith( QString::fromUtf8(".html") ) || page.endsWith( QString::fromUtf8(".css") ) || page.endsWith( QString::fromUtf8(".js") ) || page.endsWith( QString::fromUtf8(".txt") ) || page.endsWith( QString::fromUtf8(".png") ) || page.endsWith( QString::fromUtf8(".jpg") ) ) {
        if ( page.startsWith( QString::fromUtf8("LOCAL_FILE/") ) ) {
            staticFileInfo = page.replace( QString::fromUtf8("LOCAL_FILE/"), QString::fromUtf8("") );
        } else   {
            staticFileInfo = docDir + page;
        }
    }
    if ( staticFileInfo.exists() && body.isEmpty() ) {
        QFile staticFile( staticFileInfo.absoluteFilePath() );
        if ( staticFile.open(QIODevice::ReadOnly) ) {
            if ( page.endsWith( QString::fromUtf8(".html") ) || page.endsWith( QString::fromUtf8(".htm") ) ) {
                QString input = QString::fromUtf8( staticFile.readAll() );
                if ( input.contains( QString::fromUtf8("http://sphinx.pocoo.org/") ) && !input.contains( QString::fromUtf8("mainMenu") ) ) {
                    body = parser(input, docDir).toLatin1();
                } else   {
                    body = input.toLatin1();
                }
            } else   {
                body = staticFile.readAll();
            }
            staticFile.close();
        }
    }

    // page not found
    if ( body.isEmpty() ) {
        QString notFound = QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\"><html><head><title>Page not found</title><link rel=\"stylesheet\" href=\"/_static/default.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"/_static/pygments.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"/_static/style.css\" type=\"text/css\" /><script type=\"text/javascript\" src=\"/_static/jquery.js\"></script><script type=\"text/javascript\" src=\"/_static/dropdown.js\"></script></head><body><div class=\"document\"><div class=\"documentwrapper\"><div class=\"body\"><h1>Page not found</h1></div></div></div></body></html>");
        body = parser(notFound, docDir).toLatin1();
    }

    // set header(s)
    resp->setHeader( QString::fromUtf8("Content-Length"), QString::number( body.size() ) );
    if ( page.endsWith( QString::fromUtf8(".png") ) ) {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("image/png") );
    } else if ( page.endsWith( QString::fromUtf8(".jpg") ) )   {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("image/jpeg") );
    } else if ( page.endsWith( QString::fromUtf8(".html") ) || page.endsWith( QString::fromUtf8(".htm") ) )   {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("text/html") );
    } else if ( page.endsWith( QString::fromUtf8(".css") ) )   {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("text/css") );
    } else if ( page.endsWith( QString::fromUtf8(".js") ) )   {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("application/javascript") );
    } else if ( page.endsWith( QString::fromUtf8(".txt") ) )   {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("text/plain") );
    }

    // return result
    resp->writeHead(200);
    resp->end(body);
} // DocumentationManager::handler

QString
DocumentationManager::parser(QString html,
                             QString path) const
{
    QString result = html;

    // get static menu from index.html and make a header+menu
    QFile indexFile( path + QString::fromUtf8("/index.html") );
    QString menuHTML;

    menuHTML.append( QString::fromUtf8("<body>\n") );
    menuHTML.append( QString::fromUtf8("<div id=\"header\"><a href=\"/\"><div id=\"logo\"></div></a><div id=\"mainMenu\">\n") );
    menuHTML.append( QString::fromUtf8("<ul>\n") );
    if ( indexFile.exists() ) {
        if ( indexFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
            QStringList menuResult;
            bool getMenu = false;
            while ( !indexFile.atEnd() ) {
                QString line = QString::fromLatin1( indexFile.readLine() );
                if ( line == QString::fromUtf8("<div class=\"toctree-wrapper compound\">\n") ) {
                    getMenu = true;
                }
                if (getMenu) {
                    menuResult << line;
                }
                if ( line == QString::fromUtf8("</div>\n") ) {
                    getMenu = false;
                }
            }
            if ( !menuResult.isEmpty() ) {
                int menuEnd = menuResult.size() - 2;
                for (int i = 0; i < menuEnd; ++i) {
                    QString tmp = menuResult.at(i);
                    tmp.replace( QString::fromUtf8("href=\""), QString::fromUtf8("href=\"/") );
                    menuHTML.append(tmp);
                }
            }
            indexFile.close();
            menuHTML.append( QString::fromUtf8("</div>\n</div></div>\n") );
        }
    } else   {
        menuHTML.append( QString::fromUtf8("</ul></div></div>") );
    }

    // add search
    menuHTML.append(QString::fromUtf8("<div id=\"search\"><form id=\"rtd-search-form\" class=\"wy-form\" action=\"/search.html\" method=\"get\"><input type=\"text\" name=\"q\" placeholder=\"Search docs\" /><input type=\"hidden\" name=\"check_keywords\" value=\"yes\" /><input type=\"hidden\" name=\"area\" value=\"default\" /></form></div>"));

    // preferences
    /*boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
       QString prefsHTML = settings->makeHTMLDocumentation(true, false);
       menuHTML.append(prefsHTML);

       /// TODO probably a better way to get categories...
       QStringList groups;
       std::list<std::string> pluginIDs = appPTR->getPluginIDs();
       for (std::list<std::string>::iterator it=pluginIDs.begin(); it != pluginIDs.end(); ++it) {
        Plugin* plugin = 0;
        QString pluginID = QString::fromUtf8(it->c_str());
        try {
            plugin = appPTR->getPluginBinary(pluginID, -1, -1, false);
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        if (plugin) {
            QStringList groupList = plugin->getGrouping();
            groups << groupList.at(0);
        }
       }
       groups.removeDuplicates();
       QString refHTML;
       refHTML.append(QString::fromUtf8("<li class=\"toctree-l1\"><a href=\"/_group.html\">Reference Guide</a>\n"));
       refHTML.append(QString::fromUtf8("<ul>\n"));
       for (int i = 0; i < groups.size(); ++i) {
        refHTML.append(QString::fromUtf8("\n<li class='toctree-l2'><a href='/_group.html?id=")+groups.at(i)+QString::fromUtf8("'>")+groups.at(i)+QString::fromUtf8("</a></li>"));
       }
       refHTML.append(QString::fromUtf8("\n</ul>\n</li>\n</ul>\n"));*/

    // make html
    //menuHTML.append(refHTML);

    result.replace(QString::fromUtf8("<body>"), menuHTML);

    // replace "Natron 2.0 documentation" with current version
    result.replace( QString::fromUtf8("Natron 2.0 documentation"), QString::fromUtf8(NATRON_APPLICATION_NAME) + QString::fromUtf8(" ") + QString::fromUtf8(NATRON_VERSION_STRING) + QString::fromUtf8(" documentation") );

    return result;
} // DocumentationManager::parser

void
DocumentationManager::setPort(int port)
{
#ifdef DEBUG
    qDebug() << "Documentation Server is attached to port" << port;
#endif
    appPTR->getCurrentSettings()->setServerPort(port);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_DocumentationManager.cpp"
