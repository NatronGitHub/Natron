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

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <QString>
#include <QStringList>

#include "Engine/DocumentationGroups.h"
#include "Engine/Plugin.h"

NATRON_NAMESPACE_USING

static QString
groupingId(const char* grouping)
{
    return Plugin::makeGroupingId( QString::fromUtf8(grouping).split( QLatin1Char('/') ) );
}

TEST(PluginGrouping,
     MakeGroupingId)
{
    // ASCII punctuation and spaces are dropped: both spellings of G'MIC are the same group
    EXPECT_EQ( QString::fromUtf8("GMIC"), groupingId("GMIC") );
    EXPECT_EQ( QString::fromUtf8("GMIC"), groupingId("G'MIC") );
    EXPECT_EQ( QString::fromUtf8("GMIC_BlackWhite"), groupingId("G'MIC/Black & White") );
    EXPECT_EQ( QString::fromUtf8("Color_Math"), groupingId("Color/Math") );
    // non-ASCII characters are encoded, so that different names give different ids
    EXPECT_EQ( QString::fromUtf8("Fax00E7ade"), groupingId("Façade") );
    EXPECT_NE( groupingId("Façade"), groupingId("Faade") );
    EXPECT_NE( groupingId("Цвет 2"), groupingId("Фильтр 2") );
    // a level made only of punctuation still gives a non-empty id
    EXPECT_FALSE( groupingId("&").isEmpty() );
    // ids are safe in file names and URLs, and never contain '-'
    QString id = groupingId("A b/C'd/É-f?g#h");
    for (int i = 0; i < id.size(); ++i) {
        const QChar c = id.at(i);
        EXPECT_TRUE( ( c >= QLatin1Char('a') && c <= QLatin1Char('z') ) ||
                     ( c >= QLatin1Char('A') && c <= QLatin1Char('Z') ) ||
                     ( c >= QLatin1Char('0') && c <= QLatin1Char('9') ) ||
                     c == QLatin1Char('_') );
    }
}

TEST(PluginGrouping,
     NormalizeGrouping)
{
    // empty levels are removed, and an empty grouping goes to "Other"
    EXPECT_EQ( QStringList( QString::fromUtf8(PLUGIN_GROUP_OTHER) ), DocumentationGroups::normalizeGrouping( QStringList() ) );
    EXPECT_EQ( QStringList( QString::fromUtf8(PLUGIN_GROUP_OTHER) ), DocumentationGroups::normalizeGrouping( QStringList( QString() ) ) );
    EXPECT_EQ( QStringList( QString::fromUtf8("Foo") ), DocumentationGroups::normalizeGrouping( QString::fromUtf8("/Foo/ ").split( QLatin1Char('/') ) ) );
}

TEST(PluginGrouping,
     GroupLabel)
{
    // both spellings are displayed G'MIC
    EXPECT_EQ( QString::fromUtf8("G'MIC / Colors"), DocumentationGroups::makeGroupLabel( QString::fromUtf8("GMIC/Colors").split( QLatin1Char('/') ) ) );
    EXPECT_EQ( QString::fromUtf8("G'MIC / Colors"), DocumentationGroups::makeGroupLabel( QString::fromUtf8("G'MIC/Colors").split( QLatin1Char('/') ) ) );
}
