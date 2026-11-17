/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "DocumentationGroups.h"

#include <list>
#include <string>
#include <stdexcept>
#include <iostream>

#include <QCoreApplication>

#include "Engine/AppManager.h"
#include "Engine/Plugin.h"

NATRON_NAMESPACE_ENTER

bool
DocumentationGroups::isDocumented(const Plugin* plugin)
{
    return plugin && !plugin->getIsDeprecated() && ( !plugin->getIsForInternalUseOnly() || plugin->isReader() || plugin->isWriter() );
}

QString
DocumentationGroups::makeGroupId(const QStringList& path)
{
    return Plugin::makeGroupingId(path);
}

static QString
groupLevelLabel(const QString& level)
{
    if ( ( level == QLatin1String("GMIC") ) || ( level == QString::fromUtf8("G'MIC") ) ) {
        // openfx-gmic used "GMIC" as its top-level grouping, and now uses "G'MIC".
        // Both have the same id ("GMIC"), so they are the same group, displayed "G'MIC".
        return QString::fromUtf8("G'MIC");
    }

    // the group names are marked for translation in the DocumentationManager constructor
    return QCoreApplication::translate( "DocumentationManager", level.toUtf8().constData() );
}

QString
DocumentationGroups::makeGroupLabel(const QStringList& path)
{
    QStringList labels;
    Q_FOREACH(const QString& level, path) {
        labels << groupLevelLabel(level);
    }

    return labels.join( QString::fromUtf8(" / ") );
}

QString
DocumentationGroups::makeGroupTitle(const QStringList& path)
{
    return QCoreApplication::translate("DocumentationManager", "%1 nodes").arg( makeGroupLabel(path) );
}

QString
DocumentationGroups::makeSubGroupTitle(const QStringList& path)
{
    if ( path.isEmpty() ) {
        return QString();
    }

    return QCoreApplication::translate("DocumentationManager", "%1 nodes").arg( groupLevelLabel( path.last() ) );
}

DocumentationGroups::DocumentationGroups()
{
    QStringList topLevelIds; // in the order the plug-ins are found
    std::list<std::string> pluginIDs = appPTR->getPluginIDs();

    for (std::list<std::string>::iterator it = pluginIDs.begin(); it != pluginIDs.end(); ++it) {
        QString pluginID = QString::fromUtf8( it->c_str() );
        if ( pluginID.isEmpty() ) {
            continue;
        }
        Plugin* plugin = 0;
        try {
            plugin = appPTR->getPluginBinary(pluginID, -1, -1, false);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        if ( !isDocumented(plugin) ) {
            continue;
        }
        const QStringList& grouping = plugin->getGrouping();
        if ( grouping.isEmpty() ) {
            continue;
        }
        // create the group of each level, and link it to its parent
        QString parentId;
        for (int i = 0; i < grouping.size(); ++i) {
            QStringList path = grouping.mid(0, i + 1);
            QString id = makeGroupId(path);
            if ( !_groups.contains(id) ) {
                Group& group = _groups[id];
                group.path = path;
                if ( parentId.isEmpty() ) {
                    topLevelIds << id;
                } else {
                    _groups[parentId].subGroupIds << id;
                }
            }
            parentId = id;
        }
        _groups[parentId].pluginsByLabel[Plugin::makeLabelWithoutSuffix( plugin->getPluginLabel() )] = pluginID;
    }

    // sort the sub-groups by label
    for (QMap<QString, Group>::iterator it = _groups.begin(); it != _groups.end(); ++it) {
        QMap<QString, QString> subGroupsByLabel;
        Q_FOREACH(const QString& id, it->subGroupIds) {
            subGroupsByLabel[makeGroupLabel(_groups[id].path)] = id;
        }
        it->subGroupIds = subGroupsByLabel.values();
    }

    // top-level groups: the ones from the left toolbar first, in the same order,
    // then the other ones in the order they were found.
    // Group ordering is set at every place in the code where GROUP_ORDER appears in the comments
    QStringList order;
    order << QString::fromUtf8(PLUGIN_GROUP_IMAGE);
    order << QString::fromUtf8(PLUGIN_GROUP_PAINT);
    order << QString::fromUtf8(PLUGIN_GROUP_TIME);
    order << QString::fromUtf8(PLUGIN_GROUP_CHANNEL);
    order << QString::fromUtf8(PLUGIN_GROUP_COLOR);
    order << QString::fromUtf8(PLUGIN_GROUP_FILTER);
    order << QString::fromUtf8(PLUGIN_GROUP_KEYER);
    order << QString::fromUtf8(PLUGIN_GROUP_MERGE);
    order << QString::fromUtf8(PLUGIN_GROUP_TRANSFORM);
    order << QString::fromUtf8(PLUGIN_GROUP_MULTIVIEW);
    order << QString::fromUtf8(PLUGIN_GROUP_OTHER);
    order << QString::fromUtf8("G'MIC"); // openfx-gmic (older versions use "GMIC", which has the same id)
    order << QString::fromUtf8("Extra"); // openfx-arena
    Q_FOREACH(const QString& group, order) {
        QString id = makeGroupId( QStringList(group) );
        if ( topLevelIds.contains(id) ) {
            _topLevelIds << id;
        }
    }
    Q_FOREACH(const QString& id, topLevelIds) {
        if ( !_topLevelIds.contains(id) ) {
            _topLevelIds << id;
        }
    }
}

const DocumentationGroups::Group*
DocumentationGroups::getGroup(const QString& id) const
{
    QMap<QString, Group>::const_iterator it = _groups.find( id.contains( QLatin1Char('/') ) ? makeGroupId( id.split( QLatin1Char('/') ) ) : id );
    if ( it == _groups.end() ) {
        return 0;
    }

    return &it.value();
}

NATRON_NAMESPACE_EXIT
