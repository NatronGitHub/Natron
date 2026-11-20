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

#ifndef Engine_DocumentationGroups_h
#define Engine_DocumentationGroups_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QMap>
#include <QString>
#include <QStringList>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief The node groups of the Reference Guide ("Reference Guide > Nodes").
 *
 * Each documented plug-in is listed in the group given by its grouping
 * (kOfxImageEffectPluginPropGrouping, split on '/' by Plugin::getGrouping()).
 * Every level of the grouping is a group: a plug-in with grouping "GMIC/Colors"
 * is documented in the "Colors" sub-group of the "GMIC" group, whose page lists
 * its sub-groups first, then its own plug-ins.
 *
 * Used by AppInstance::exportDocs() (static Sphinx sources) and by
 * DocumentationManager (pages generated on the fly by the documentation server),
 * so that both produce the same layout.
 *
 * A group is identified by its id, built from its grouping path: "GMIC/Colors" -> "GMIC_Colors"
 * (see makeGroupId()). Two paths with the same id are the same group ("G'MIC" and "GMIC").
 * If two ids differ only by case, the second one gets a "-2" suffix, so that the file names
 * don't clash on case-insensitive file systems.
 * Its Sphinx page is "_group<id>.rst", and the documentation server serves it as
 * "_group.html?id=<id>" (or "_group<id>.html").
 **/
class DocumentationGroups
{
public:
    struct Group
    {
        QStringList path; // e.g. ("GMIC", "Colors")
        QStringList subGroupIds; // sorted by label
        QMap<QString, QString> pluginsByLabel; // plug-in label -> plug-in ID, sorted by label
    };

    /// Scan the plug-ins loaded by appPTR.
    DocumentationGroups();

    /// Does the Reference Guide document this plug-in?
    static bool isDocumented(const Plugin* plugin);

    /// The grouping used in the Reference Guide: empty levels are removed, and a plug-in
    /// without a grouping goes to the "Other" group.
    static QStringList normalizeGrouping(const QStringList& grouping);

    /// "GMIC/Colors" -> "GMIC_Colors" (see Plugin::makeGroupingId()). Only ASCII letters and digits
    /// are kept from each level, so that the id is safe in file names and URLs: "G'MIC/Colors" -> "GMIC_Colors" too.
    static QString makeGroupId(const QStringList& path);

    /// User-visible name of a group, e.g. "G'MIC / Colors".
    static QString makeGroupLabel(const QStringList& path);

    /// Title of a group page, e.g. "G'MIC / Colors nodes".
    static QString makeGroupTitle(const QStringList& path);

    /// Title of a group in the list of its parent's sub-groups, e.g. "Colors nodes".
    /// The full title is kept for the page itself, so that the PDF table of contents,
    /// the breadcrumbs and the search results never show two different groups with the same
    /// title (e.g. "Color nodes" and "Extra / Color nodes").
    static QString makeSubGroupTitle(const QStringList& path);

    /// Ids of the top-level groups, in the order of the left toolbar (GROUP_ORDER).
    const QStringList& getTopLevelGroupIds() const
    {
        return _topLevelIds;
    }

    /// All group ids (top-level and sub-groups).
    QStringList getGroupIds() const
    {
        return _groups.keys();
    }

    /// The group with that id, or NULL. The id may also be a grouping path, possibly
    /// percent-encoded ("G'MIC/Colors", "Sapphire%20Blur").
    const Group* getGroup(const QString& id) const;

    /// The ids of the group of each level of a plug-in grouping (as returned by Plugin::getGrouping()),
    /// e.g. ("GMIC", "GMIC_Colors"). Empty if the plug-in is not documented.
    QStringList getGroupIdsForGrouping(const QStringList& grouping) const;

private:
    QMap<QString, Group> _groups; // by id
    QMap<QString, QString> _idsByKey; // makeGroupId(path) -> id
    QStringList _topLevelIds;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_DocumentationGroups_h
