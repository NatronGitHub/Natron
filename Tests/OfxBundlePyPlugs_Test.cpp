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

// The PyPlugs that ship in an OFX plug-in bundle, in <bundle>/Contents/Resources/PyPlugs, are loaded after all the
// other PyPlug search paths, so that those override them. The test bundle (NatronTestPlugin.ofx.bundle) has
// Tests/data/ofx-bundle-pyplugs/NatronTestBundlePyPlug.py and its icon there. The TestsOfxBundlePyPlugOverride test
// runs these tests again with NATRON_PLUGIN_PATH=Tests/data/ofx-bundle-pyplugs-override, which has a PyPlug with the
// same ID and version; TestsOfxCache runs them with the plug-ins known from the OFX plug-in cache, not scanned.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <list>
#include <string>

#include <gtest/gtest.h>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/OfxHost.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const char* const kBundlePyPlugID = "net.natron.test.BundlePyPlug";

#ifdef NATRON_TESTS_OFX_PLUGIN_DIR
// The PyPlugs directory of the test bundle
QString
testBundlePyPlugsDir()
{
    return QDir::cleanPath( QString::fromUtf8(NATRON_TESTS_OFX_PLUGIN_DIR "/NatronTestPlugin.ofx.bundle/Contents/Resources/PyPlugs") );
}

#endif

// Whether NATRON_PLUGIN_PATH has the directory of the overriding PyPlug (the TestsOfxBundlePyPlugOverride test)
bool
isOverridden()
{
#ifdef NATRON_TESTS_DATA_DIR
    const QString overrideDir = QDir::cleanPath( QString::fromUtf8(NATRON_TESTS_DATA_DIR "/ofx-bundle-pyplugs-override") );
# ifdef __NATRON_WIN32__
    const QChar pathSep = QChar::fromLatin1(';');
# else
    const QChar pathSep = QChar::fromLatin1(':');
# endif
    Q_FOREACH(const QString &dir, QString::fromUtf8( qgetenv(NATRON_PLUGIN_PATH_ENV_VAR) ).split(pathSep)) {
        if ( !dir.isEmpty() && (QDir::cleanPath(dir) == overrideDir) ) {
            return true;
        }
    }
#endif

    return false;
}

QString
canonical(const QString& path)
{
    return QFileInfo(path).canonicalFilePath();
}

void
makeDir(const QString& path)
{
    ASSERT_TRUE( QDir().mkpath(path) ) << path.toStdString();
}
} // anonymous namespace

// OfxHost::makeBundlesPyPlugsPaths(): one PyPlugs directory per bundle that has one, in the order of the OFX plug-in
// search path
TEST(OfxBundlePyPlugs, MakeBundlesPyPlugsPaths)
{
    QTemporaryDir tmpDir;

    ASSERT_TRUE( tmpDir.isValid() );
    const QString root = QDir::cleanPath( tmpDir.path() );
    const QString first = root + QString::fromUtf8("/first");
    const QString second = root + QString::fromUtf8("/second");
    const QString other = root + QString::fromUtf8("/other");
    const QString pyPlugs = QString::fromUtf8("/Contents/Resources/PyPlugs");
    const QString a = first + QString::fromUtf8("/sub/A.ofx.bundle");
    const QString b = first + QString::fromUtf8("/B.ofx.bundle"); // without a PyPlugs directory
    const QString c = second + QString::fromUtf8("/C.ofx.bundle");
    const QString d = other + QString::fromUtf8("/D.ofx.bundle"); // in no entry of the search path
    const QString e = root + QString::fromUtf8("/firstbis/E.ofx.bundle"); // "first" is not a directory of its path
    makeDir(a + pyPlugs);
    makeDir(b + QString::fromUtf8("/Contents/Resources"));
    makeDir(c + pyPlugs);
    makeDir(d + pyPlugs);
    makeDir(e + pyPlugs);

    std::list<std::string> searchPath;
    searchPath.push_back( second.toStdString() + '/' );
    searchPath.push_back( first.toStdString() );
    std::list<std::string> bundles;
    // a bundle once per plug-in: A has three
    bundles.push_back( d.toStdString() );
    bundles.push_back( a.toStdString() );
    bundles.push_back( b.toStdString() );
    bundles.push_back( a.toStdString() );
    bundles.push_back( e.toStdString() );
    bundles.push_back( c.toStdString() );
    bundles.push_back( a.toStdString() + "/" );

    const QStringList expected = QStringList() << (c + pyPlugs) << (a + pyPlugs) << (d + pyPlugs) << (e + pyPlugs);
    EXPECT_EQ( expected, NATRON_NAMESPACE::OfxHost::makeBundlesPyPlugsPaths(searchPath, bundles) )
        << NATRON_NAMESPACE::OfxHost::makeBundlesPyPlugsPaths(searchPath, bundles).join( QLatin1Char('\n') ).toStdString();
    EXPECT_TRUE( NATRON_NAMESPACE::OfxHost::makeBundlesPyPlugsPaths( searchPath, std::list<std::string>() ).isEmpty() );
}

// The PyPlug of the test bundle is loaded, once, from the bundle, with the icon next to it; with
// NATRON_PLUGIN_PATH=Tests/data/ofx-bundle-pyplugs-override (TestsOfxBundlePyPlugOverride), the PyPlug of the same ID
// and version there is loaded instead
TEST(OfxBundlePyPlugs, BundlePyPlugIsFound)
{
#ifndef NATRON_TESTS_OFX_PLUGIN_DIR
    std::cerr << "[  SKIPPED ] OfxBundlePyPlugs.BundlePyPlugIsFound: no test plug-in bundle" << std::endl;
#else
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

        return;
    }
    const QString bundleDir = testBundlePyPlugsDir();
    ASSERT_TRUE( QFileInfo( bundleDir + QString::fromUtf8("/NatronTestBundlePyPlug.py") ).isFile() ) << bundleDir.toStdString();
    const bool overridden = isOverridden();

    // the bundle's PyPlugs directory is in the search path once, after all the entries that are not the PyPlugs
    // directory of a bundle (those of other bundles, e.g. openfx-misc's in OFX_PLUGIN_PATH, may follow)
    const QStringList searchPath = appPTR->getAllNonOFXPluginsPaths();
    const std::string searchPathStr = searchPath.join( QLatin1Char('\n') ).toStdString();
    ASSERT_EQ( 1, searchPath.count(bundleDir) ) << searchPathStr;
    for (int i = searchPath.indexOf(bundleDir) + 1; i < searchPath.size(); ++i) {
        EXPECT_TRUE( searchPath[i].endsWith( QString::fromUtf8(".ofx.bundle/Contents/Resources/PyPlugs") ) ) << searchPathStr;
    }

    // one version of the PyPlug, from the bundle (or from the overriding directory)
    const PluginsMap& plugins = appPTR->getPluginsList();
    PluginsMap::const_iterator found = plugins.find(kBundlePyPlugID);
    ASSERT_TRUE( found != plugins.end() );
    ASSERT_EQ( 1U, found->second.size() );
    Plugin* plugin = *found->second.begin();
    ASSERT_TRUE(plugin);
    const QString moduleFile = canonical( plugin->getPythonModule() + QString::fromUtf8(".py") );
    if (overridden) {
        EXPECT_EQ( canonical( QString::fromUtf8(NATRON_TESTS_DATA_DIR "/ofx-bundle-pyplugs-override/NatronTestBundlePyPlug.py") ), moduleFile );
        EXPECT_EQ( std::string("BundlePyPlugOverride"), plugin->getPluginLabel().toStdString() );
        EXPECT_TRUE( plugin->getIconFilePath().isEmpty() ) << plugin->getIconFilePath().toStdString();
    } else {
        EXPECT_EQ( canonical( bundleDir + QString::fromUtf8("/NatronTestBundlePyPlug.py") ), moduleFile );
        EXPECT_EQ( std::string("BundlePyPlug"), plugin->getPluginLabel().toStdString() );
        // the icon resolves relative to the PyPlug's directory
        EXPECT_EQ( canonical( bundleDir + QString::fromUtf8("/NatronTestBundlePyPlug.png") ), plugin->getIconFilePath() );
        EXPECT_FALSE( plugin->getIconFilePath().isEmpty() );
    }

    // it creates its group
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    ProjectPtr project = app->getProject();
    CreateNodeArgs args( kBundlePyPlugID, project );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    EXPECT_EQ( std::string(kBundlePyPlugID), node->getPluginID() );
    project->resetProject();
#endif // ifndef NATRON_TESTS_OFX_PLUGIN_DIR
}

// A PyPlugs directory of a bundle that is also in NATRON_PLUGIN_PATH is in the search path once, where
// NATRON_PLUGIN_PATH puts it
TEST(OfxBundlePyPlugs, NoDuplicateSearchPath)
{
#ifndef NATRON_TESTS_OFX_PLUGIN_DIR
    std::cerr << "[  SKIPPED ] OfxBundlePyPlugs.NoDuplicateSearchPath: no test plug-in bundle" << std::endl;
#else
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

        return;
    }
    const QString bundleDir = testBundlePyPlugsDir();
    const QByteArray oldPluginPath = qgetenv(NATRON_PLUGIN_PATH_ENV_VAR);
    const bool hadPluginPath = qEnvironmentVariableIsSet(NATRON_PLUGIN_PATH_ENV_VAR);
    // with a trailing slash, as a user may write it
    qputenv( NATRON_PLUGIN_PATH_ENV_VAR, ( bundleDir + QLatin1Char('/') ).toUtf8() );

    const QStringList searchPath = appPTR->getAllNonOFXPluginsPaths();

    if (hadPluginPath) {
        qputenv(NATRON_PLUGIN_PATH_ENV_VAR, oldPluginPath);
    } else {
        qunsetenv(NATRON_PLUGIN_PATH_ENV_VAR);
    }
    int count = 0;
    Q_FOREACH(const QString &dir, searchPath) {
        if (QDir::cleanPath(dir) == bundleDir) {
            ++count;
        }
    }
    const std::string searchPathStr = searchPath.join( QLatin1Char('\n') ).toStdString();
    EXPECT_EQ(1, count) << searchPathStr;
    // NATRON_PLUGIN_PATH comes right after the global PyPlugs directory
    const int globalIndex = searchPath.indexOf( appPTR->getPyPlugsGlobalPath() );
    ASSERT_GE(globalIndex, 0) << searchPathStr;
    ASSERT_LT(globalIndex + 1, searchPath.size()) << searchPathStr;
    EXPECT_EQ( bundleDir, QDir::cleanPath(searchPath[globalIndex + 1]) ) << searchPathStr;
#endif // ifndef NATRON_TESTS_OFX_PLUGIN_DIR
}
