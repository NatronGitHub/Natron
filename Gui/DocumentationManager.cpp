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

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QVector>

#include <iostream>
#include <exception>
#include <cstddef>

#include "qhttpserver.h"
#include "qhttprequest.h"
#include "qhttpresponse.h"

#include "Gui/GuiApplicationManager.h" // appPTR
#include "Engine/AppInstance.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"


NATRON_NAMESPACE_ENTER;
DocumentationManager::DocumentationManager(QObject *parent)
    : QObject(parent)
    , server(NULL)
{
    // list of translatable group names
    (void)QT_TR_NOOP(PLUGIN_GROUP_IMAGE);
    (void)QT_TR_NOOP(PLUGIN_GROUP_IMAGE_READERS);
    (void)QT_TR_NOOP(PLUGIN_GROUP_IMAGE_WRITERS);
    (void)QT_TR_NOOP(PLUGIN_GROUP_COLOR);
    (void)QT_TR_NOOP(PLUGIN_GROUP_COLOR "/Math");
    (void)QT_TR_NOOP(PLUGIN_GROUP_COLOR "/OCIO");
    (void)QT_TR_NOOP(PLUGIN_GROUP_COLOR "/Transform");
    (void)QT_TR_NOOP(PLUGIN_GROUP_FILTER);
    (void)QT_TR_NOOP(PLUGIN_GROUP_FILTER "/Merges");
    (void)QT_TR_NOOP(PLUGIN_GROUP_TRANSFORM);
    (void)QT_TR_NOOP(PLUGIN_GROUP_TIME);
    (void)QT_TR_NOOP(PLUGIN_GROUP_PAINT);
    (void)QT_TR_NOOP(PLUGIN_GROUP_KEYER);
    (void)QT_TR_NOOP(PLUGIN_GROUP_CHANNEL);
    (void)QT_TR_NOOP(PLUGIN_GROUP_MERGE);
    (void)QT_TR_NOOP(PLUGIN_GROUP_MULTIVIEW);
    (void)QT_TR_NOOP(PLUGIN_GROUP_MULTIVIEW "/Stereo");
    (void)QT_TR_NOOP(PLUGIN_GROUP_DEEP);
    (void)QT_TR_NOOP(PLUGIN_GROUP_TOOLSETS);
    (void)QT_TR_NOOP(PLUGIN_GROUP_3D);
    (void)QT_TR_NOOP(PLUGIN_GROUP_OTHER);
    (void)QT_TR_NOOP(PLUGIN_GROUP_DEFAULT);
    (void)QT_TR_NOOP(PLUGIN_GROUP_OFX);
    // openfx-arena
    (void)QT_TR_NOOP("Extra");
    (void)QT_TR_NOOP("Extra/"PLUGIN_GROUP_COLOR);
    (void)QT_TR_NOOP("Extra/Distort");
    (void)QT_TR_NOOP("Extra/"PLUGIN_GROUP_PAINT);
    (void)QT_TR_NOOP("Extra/"PLUGIN_GROUP_FILTER);
    (void)QT_TR_NOOP("Extra/"PLUGIN_GROUP_DEFAULT);
    (void)QT_TR_NOOP("Extra/"PLUGIN_GROUP_TRANSFORM);
}

DocumentationManager::~DocumentationManager()
{
}

void
DocumentationManager::startServer()
{
    server = new QHttpServer(this);
    connect( server, SIGNAL(newRequest(QHttpRequest*,QHttpResponse*)), this, SLOT(handler(QHttpRequest*,QHttpResponse*)) );
    server->listen( QHostAddress::LocalHost, appPTR->getCurrentSettings()->getServerPort() );
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

    // override static docs
    if ( page.contains( QString::fromUtf8("/plugins/") ) ) {
        page.replace( QString::fromUtf8(".html"), QString::fromUtf8("") ).replace( QString::fromUtf8("/plugins/"), QString::fromUtf8("/_plugin.html?id=") );
    }
    if ( page.startsWith( QString::fromUtf8("/_group") ) && !page.contains( QString::fromUtf8("_group.html") ) ) {
        page.replace( QString::fromUtf8(".html"), QString::fromUtf8("") ).replace( QString::fromUtf8("_group"), QString::fromUtf8("_group.html?id=") );
    }

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
                        } catch (const std::exception& e) {
                            std::cerr << e.what() << std::endl;
                        }

                        if (plugin) {
        
                            CreateNodeArgs args( pluginID.toStdString(), appPTR->getTopLevelInstance()->getProject() );
                            args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
                            args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);

                            NodePtr node = appPTR->getTopLevelInstance()->createNode(args);
                            if (node) {
                                QString html = node->makeDocumentation(true);
                                html = parser(html, docDir);
                                body = html.toUtf8();
                            }
                        }
                    }
                }
            }
        }
        if ( body.isEmpty() ) {
            QString notFound = QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">"
                                                 "<html>"
                                                 "<head>"
                                                 "<title>Plugin not found</title>"
                                                 "<link rel=\"stylesheet\" href=\"/_static/default.css\" type=\"text/css\" />"
                                                 "<link rel=\"stylesheet\" href=\"/_static/pygments.css\" type=\"text/css\" />"
                                                 "<link rel=\"stylesheet\" href=\"/_static/style.css\" type=\"text/css\" />"
                                                 "<script type=\"text/javascript\" src=\"/_static/jquery.js\"></script>"
                                                 "<script type=\"text/javascript\" src=\"/_static/dropdown.js\"></script>"
                                                 "</head>"
                                                 "<body>"
                                                 "<div class=\"document\">"
                                                 "<div class=\"documentwrapper\">"
                                                 "<div class=\"body\">"
                                                 "<h1>Plugin not found</h1>"
                                                 "</div>"
                                                 "</div>"
                                                 "</div>"
                                                 "</body>"
                                                 "</html>");
            body = parser(notFound, docDir).toUtf8();
        }
    } else if ( page == QString::fromUtf8("_prefs.html") ) {
        SettingsPtr settings = appPTR->getCurrentSettings();
        QString html = settings->makeHTMLDocumentation(true);
        html = parser(html, docDir);
        body = html.toUtf8();
    } else if ( page == QString::fromUtf8("_group.html") ) {
        QString html;
        QString group;
        QString groupHeader = QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">"
                                                "<html>"
                                                "<head>"
                                                "<title>__REPLACE_TITLE__ - %1</title>"
                                                "<link rel=\"stylesheet\" href=\"_static/default.css\" type=\"text/css\" />"
                                                "<link rel=\"stylesheet\" href=\"_static/pygments.css\" type=\"text/css\" />"
                                                "<link rel=\"stylesheet\" href=\"_static/style.css\" type=\"text/css\" />"
                                                "</head>"
                                                "<body>")
                              .arg( tr("%1 %2 documentation")
                                    .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) )
                                    .arg( QString::fromUtf8(NATRON_VERSION_STRING) ) );
        QString groupBodyEnd = QString::fromUtf8("</ul>"
                                                 "</div>"
                                                 "</div>"
                                                 "</div>"
                                                 "</div>");
        QString groupFooter = QString::fromUtf8("</body>"
                                                "</html>");
        QString navHeader = QString::fromUtf8("<div class=\"related\">"
                                              "<h3>%1</h3>"
                                              "<ul>"
                                              "<li><a href=\"/index.html\">NATRON_DOCUMENTATION</a> &raquo;</li>")
                            .arg( tr("Navigation") );
        QString navFooter = QString::fromUtf8("</ul>"
                                              "</div>");
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
                } catch (const std::exception& e) {
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
                QString groupBodyStart = QString::fromUtf8("<div class=\"document\">"
                                                           "<div class=\"documentwrapper\">"
                                                           "<div class=\"body\">"
                                                           "<h1>%1</h1>"
                                                           "<p>%2</p>"
                                                           "<div class=\"toctree-wrapper compound\">"
                                                           "<ul>")
                                         .arg( tr( group.toUtf8().constData() ) )
                                         .arg( tr("The following sections contain documentation about every node in the  %1 group.").arg( tr( group.toUtf8().constData() ) + QLatin1Char(' ') + tr("Node groups are available by clicking on buttons in the left toolbar, or by right-clicking the mouse in the Node Graph area. Please note that documentation is also generated automatically for third-party OpenFX plugins.")
                                                )
                                               );
                html.append(groupHeader);
                html.replace(QString::fromUtf8("__REPLACE_TITLE__"), group);
                html.append(navHeader);
                html.append( QString::fromUtf8("<li><a href=\"/_group.html\">%1</a> &raquo;</li>")
                             .arg( tr("Reference Guide") ) );
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
                        html.append( QString::fromUtf8("<li class=\"toctree-l1\"><a href='/_plugin.html?id=%1'>%2</a></li>")
                                     .arg(plugID)
                                     .arg(plugName) );
                    }
                }
                html.append(groupBodyEnd);
                html.append(groupFooter);
            }
        } else {
            QString groupBodyStart = QString::fromUtf8("<div class=\"document\">"
                                                       "<div class=\"documentwrapper\">"
                                                       "<div class=\"body\">"
                                                       "<h1>%1</h1>"
                                                       "<p>%2</p>"
                                                       "<div class=\"toctree-wrapper compound\">"
                                                       "<ul>")
                                     .arg( tr("Reference Guide") )
            .arg ( tr("The following sections describe the various options available from the %1 preference settings, followed by one section for each node group in %1.")
                  .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) + QLatin1Char(' ') + tr("Node groups are available by clicking on buttons in the left toolbar, or by right-clicking the mouse in the Node Graph area. Please note that documentation is also generated automatically for third-party OpenFX plugins.") );
            html.append(groupHeader);
            html.replace( QString::fromUtf8("__REPLACE_TITLE__"), tr("Reference Guide") );
            html.append(navHeader);
            html.append(navFooter);
            html.append(groupBodyStart);
            html.append( QString::fromUtf8("<li class=\"toctree-l1\"><a href=\"/_prefs.html\">%1</a></li>").arg( tr("%1 preferences").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) ) );

            QStringList groups;
            groups << QString::fromUtf8(PLUGIN_GROUP_IMAGE);
            groups << QString::fromUtf8(PLUGIN_GROUP_COLOR);
            groups << QString::fromUtf8(PLUGIN_GROUP_CHANNEL);
            groups << QString::fromUtf8(PLUGIN_GROUP_MERGE);
            groups << QString::fromUtf8(PLUGIN_GROUP_FILTER);
            groups << QString::fromUtf8(PLUGIN_GROUP_TRANSFORM);
            groups << QString::fromUtf8(PLUGIN_GROUP_TIME);
            groups << QString::fromUtf8(PLUGIN_GROUP_PAINT);
            groups << QString::fromUtf8(PLUGIN_GROUP_KEYER);
            groups << QString::fromUtf8(PLUGIN_GROUP_MULTIVIEW);
            groups << QString::fromUtf8(PLUGIN_GROUP_OTHER);
            groups << QString::fromUtf8("Extra"); // openfx-arena

            std::list<std::string> pluginIDs = appPTR->getPluginIDs();
            for (std::list<std::string>::iterator it = pluginIDs.begin(); it != pluginIDs.end(); ++it) {
                Plugin* plugin = 0;
                QString pluginID = QString::fromUtf8( it->c_str() );
                try {
                    plugin = appPTR->getPluginBinary(pluginID, -1, -1, false);
                }catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }

                if (plugin) {
                    QStringList groupList = plugin->getGrouping();
                    groups << groupList.at(0);
                }
            }
            groups.removeDuplicates();
            for (int i = 0; i < groups.size(); ++i) {
                html.append( QString::fromUtf8("<li class='toctree-l1'><a href='/_group.html?id=%1'>%2</a></li>")
                             .arg( groups.at(i) )
                             .arg( tr( groups.at(i).toUtf8().constData() ) )
                             );
            }
            html.append(groupBodyEnd);
            html.append(groupFooter);
        }
        html = parser(html, docDir);
        body = html.toUtf8();
    }

    // get static file
    QFileInfo staticFileInfo;
    if ( page.endsWith( QString::fromUtf8(".html") ) || page.endsWith( QString::fromUtf8(".css") ) || page.endsWith( QString::fromUtf8(".js") ) || page.endsWith( QString::fromUtf8(".txt") ) || page.endsWith( QString::fromUtf8(".png") ) || page.endsWith( QString::fromUtf8(".jpg") ) ) {
        if ( page.startsWith( QString::fromUtf8("LOCAL_FILE/") ) ) {
            staticFileInfo = page.replace( QString::fromUtf8("LOCAL_FILE/"), QString::fromUtf8("") ).replace( QString::fromUtf8("%2520"), QString::fromUtf8(" ") ).replace( QString::fromUtf8("%20"), QString::fromUtf8(" ") );
        } else {
            staticFileInfo = docDir + page;
        }
    }
    if ( staticFileInfo.exists() && body.isEmpty() ) {
        QFile staticFile( staticFileInfo.absoluteFilePath() );
        if ( staticFile.open(QIODevice::ReadOnly) ) {
            if ( page.endsWith( QString::fromUtf8(".html") ) || page.endsWith( QString::fromUtf8(".htm") ) ) {
                QString input = QString::fromUtf8( staticFile.readAll() );
                body = parser(input, docDir).toUtf8();
            } else {
                body = staticFile.readAll();
            }
            staticFile.close();
        }
    }

    // page not found
    if ( body.isEmpty() ) {
        QString notFound = QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">"
                                             "<html>"
                                             "<head>"
                                             "<title>%1</title>"
                                             "<link rel=\"stylesheet\" href=\"/_static/default.css\" type=\"text/css\" />"
                                             "<link rel=\"stylesheet\" href=\"/_static/pygments.css\" type=\"text/css\" />"
                                             "<link rel=\"stylesheet\" href=\"/_static/style.css\" type=\"text/css\" />"
                                             "<script type=\"text/javascript\" src=\"/_static/jquery.js\"></script>"
                                             "<script type=\"text/javascript\" src=\"/_static/dropdown.js\"></script>"
                                             "</head><body><div class=\"document\">"
                                             "<div class=\"documentwrapper\">"
                                             "<div class=\"body\">"
                                             "<h1>%1</h1>"
                                             "</div>"
                                             "</div>"
                                             "</div>"
                                             "</body>"
                                             "</html>")
                           .arg( tr("Page not found") );
        body = parser(notFound, docDir).toUtf8();
    }

    // set header(s)
    resp->setHeader( QString::fromUtf8("Content-Length"), QString::number( body.size() ) );
    if ( page.endsWith( QString::fromUtf8(".png") ) ) {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("image/png") );
    } else if ( page.endsWith( QString::fromUtf8(".jpg") ) ) {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("image/jpeg") );
    } else if ( page.endsWith( QString::fromUtf8(".html") ) || page.endsWith( QString::fromUtf8(".htm") ) ) {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("text/html; charset=utf-8") );
    } else if ( page.endsWith( QString::fromUtf8(".css") ) ) {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("text/css; charset=utf-8") );
    } else if ( page.endsWith( QString::fromUtf8(".js") ) ) {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("application/javascript; charset=utf-8") );
    } else if ( page.endsWith( QString::fromUtf8(".txt") ) ) {
        resp->setHeader( QString::fromUtf8("Content-Type"), QString::fromUtf8("text/plain; charset=utf-8") );
    }

    // return result
    resp->writeHead(200);
    resp->end(body);
}                                             // DocumentationManager::handler

QString
DocumentationManager::parser(QString html,
                             QString path) const
{
    QString result = html;


    // sphinx compat
    bool plainBody = false;

    if ( result.contains( QString::fromUtf8("<body>") ) ) {             // 1.3 and lower
        plainBody = true;
    } else if ( result.contains( QString::fromUtf8("<body role=\"document\">") ) ) {             // 1.4+
        plainBody = false;
    }

    // get static menu from index.html and make a header+menu
    QFile indexFile( path + QString::fromUtf8("/index.html") );
    QString menuHTML;

    // fix sphinx compat
    if (plainBody) {
        menuHTML.append( QString::fromUtf8("<body>") );
    } else {
        menuHTML.append( QString::fromUtf8("<body role=\"document\">") );
    }
    menuHTML.append( QString::fromUtf8("<div id=\"header\">"
                                       "<a href=\"/\"><div id=\"logo\"></div></a>") );
    menuHTML.append( QString::fromUtf8("<div id=\"search\">"
                                       "<form id=\"rtd-search-form\" class=\"wy-form\" action=\"/search.html\" method=\"get\">"
                                       "<input type=\"text\" name=\"q\" placeholder=\"%1\" />"
                                       "<input type=\"hidden\" name=\"check_keywords\" value=\"yes\" />"
                                       "<input type=\"hidden\" name=\"area\" value=\"default\" />"
                                       "</form>"
                                       "</div>")
                     .arg( tr("Search docs") ) );
    menuHTML.append( QString::fromUtf8("<div id=\"mainMenu\">"
                                       "<ul>") );
    if ( indexFile.exists() ) {
        if ( indexFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
            QStringList menuResult;
            bool getMenu = false;
            while ( !indexFile.atEnd() ) {
                QString line = QString::fromUtf8( indexFile.readLine() );
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
            menuHTML.append( QString::fromUtf8("</div>"
                                               "</div>"
                                               "</div>") );
        }
    } else {
        menuHTML.append( QString::fromUtf8("</ul>"
                                           "</div>"
                                           "</div>") );
    }


    // fix sphinx compat
    if (plainBody) {
        result.replace(QString::fromUtf8("<body>"), menuHTML);
    } else {
        result.replace(QString::fromUtf8("<body role=\"document\">"), menuHTML);
    }

    // replace "NATRON_DOCUMENTATION" with current version
    result.replace( QString::fromUtf8("NATRON_DOCUMENTATION"), tr("%1 %2 documentation").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_VERSION_STRING) ) );

    return result;
}             // DocumentationManager::parser

int
DocumentationManager::serverPort()
{
    return server->serverPort();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_DocumentationManager.cpp"
