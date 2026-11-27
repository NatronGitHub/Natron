/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

// The OFX plug-in scan skips what the packaging spec says to skip (#1380): "Any bundle or sub-directory
// name starting with the character '@' is to be ignored. Such directories or bundles must be skipped by
// the host." (ofxPackaging.rst), and a bundle is a directory whose name ends with ".ofx.bundle". The
// build puts copies of the test bundle next to it (Tests/CMakeLists.txt, OfxTestPlugin/OfxTestPlugin.pro):
// - @NatronTestPluginDisabled.ofx.bundle: a bundle whose name starts with '@' (it was loaded);
// - @Disabled/NatronTestPluginInDisabledDir.ofx.bundle: a bundle in a directory whose name starts with '@';
// - NatronTestPluginOld.ofx.bundle.old: not a bundle (it was recorded as an invalid binary).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <ofxhPluginCache.h>

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

// The plug-in cache lists every binary that the scan found (valid or not), with its path: the skipped
// ones must not be there.
TEST(OfxPluginScan, SkipsAtNamesAndNonBundles)
{
#ifndef NATRON_TESTS_OFX_PLUGIN_DIR
    NatronTestPlugin::reportMissing(kNatronTestPluginParams);
#else
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginParams);

        return;
    }
    // the copy (made by the build, so that the test is not vacuous), and a part of its path that only it
    // has (without directory separators, which are backslashes on Windows)
    const char* skipped[][2] = {
        { "@NatronTestPluginDisabled.ofx.bundle", "@NatronTestPluginDisabled" },
        { "@Disabled/NatronTestPluginInDisabledDir.ofx.bundle", "NatronTestPluginInDisabledDir" },
        { "NatronTestPluginOld.ofx.bundle.old", "NatronTestPluginOld" },
    };
    std::ostringstream os;
    OFX::Host::PluginCache::getPluginCache()->writePluginCache(os);
    const std::string cache = os.str();
    // the test bundle itself is there
    EXPECT_NE( std::string::npos, cache.find("NatronTestPlugin.ofx.bundle") );
    for (std::size_t i = 0; i < sizeof(skipped) / sizeof(skipped[0]); ++i) {
        EXPECT_TRUE( QFileInfo( QString::fromUtf8(NATRON_TESTS_OFX_PLUGIN_DIR "/") + QString::fromUtf8(skipped[i][0]) ).isDir() ) << skipped[i][0];
        EXPECT_EQ( std::string::npos, cache.find(skipped[i][1]) ) << skipped[i][0] << " was scanned";
    }
#endif
}

// The Tests binary starts with --setting useStdOFXPluginsLocation=False (wmain.cpp, #1761): the standard OFX
// plug-in locations of the machine are not scanned, so that the results don't depend on what is installed
// there (the plug-ins come from OFX_PLUGIN_PATH).
TEST(OfxPluginScan, IgnoresTheStandardLocations)
{
    // the directories that OFX_PLUGIN_PATH lists may be standard ones (HostSupport splits it on ';', and also
    // on ':' except on Windows, see DIRLIST_SEP_CHARS in ofxhPluginCache.cpp)
#if defined(Q_OS_WIN)
    const QRegularExpression separators( QString::fromUtf8(";") );
#else
    const QRegularExpression separators( QString::fromUtf8("[:;]") );
#endif
    const QStringList envPaths = QString::fromLocal8Bit( qgetenv("OFX_PLUGIN_PATH") ).split(separators);
    const std::list<std::string>& paths = OFX::Host::PluginCache::getPluginCache()->getPluginPath();

    EXPECT_FALSE( paths.empty() );
    for (const std::string& p : paths) {
        const QString path = QDir::fromNativeSeparators( QString::fromUtf8( p.c_str() ) );
        if ( envPaths.contains( QString::fromUtf8( p.c_str() ) ) ) {
            continue;
        }
        for (const char* host : { "Plugins", NATRON_APPLICATION_NAME, "Nuke" }) {
#if defined(Q_OS_WIN)
            // <Common Files>/OFX/<host> (OFX::Host::PluginCache::getStdOFXPluginPath())
            const bool standard = path.endsWith( QString::fromUtf8("/Common Files/OFX/") + QString::fromUtf8(host), Qt::CaseInsensitive );
#elif defined(Q_OS_DARWIN)
            const bool standard = ( path == QString::fromUtf8("/Library/OFX/") + QString::fromUtf8(host) );
#else
            const bool standard = ( path == QString::fromUtf8("/usr/OFX/") + QString::fromUtf8(host) );
#endif
            EXPECT_FALSE(standard) << p << " is a standard OFX plug-in location";
        }
    }
}
