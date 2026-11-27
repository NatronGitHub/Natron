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
// same ID and version; TestsOfxCache runs them with the plug-ins known from the OFX plug-in cache, not scanned. A
// project that saved another path for the bundle's PyPlug finds it in the bundle (ProjectFindsMovedPyPlugInBundle).

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
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/OfxHost.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

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

bool
makeDir(const QString& path)
{
    return QDir().mkpath(path);
}

QByteArray
readFile(const QString& path)
{
    QFile file(path);

    if ( !file.open(QIODevice::ReadOnly) ) {
        return QByteArray();
    }

    return file.readAll();
}

bool
writeFile(const QString& path,
          const QByteArray& contents)
{
    QFile file(path);

    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && (file.write(contents) == contents.size());
}

// The first non-empty <PythonModule> of an XML project (the nodes that are not PyPlugs have an empty one), or a null
// QByteArray
QByteArray
savedPythonModule(const QByteArray& contents)
{
    const QByteArray open("<PythonModule>");
    const QByteArray close("</PythonModule>");

    for (auto begin = contents.indexOf(open); begin >= 0; begin = contents.indexOf(open, begin + 1)) {
        const auto end = contents.indexOf(close, begin);
        if (end < 0) {
            break;
        }
        const QByteArray module = contents.mid( begin + open.size(), end - begin - open.size() );
        if ( !module.isEmpty() ) {
            return module;
        }
    }

    return QByteArray();
}

// Sets the preference "Load PyPlugs in projects from .py if possible" for its lifetime
class ScopedLoadFromPyFile
{
public:
    explicit ScopedLoadFromPyFile(bool value)
        : _knob( std::dynamic_pointer_cast<KnobBool>( appPTR->getCurrentSettings()->getKnobByName("loadFromPyFile") ) )
        , _old( _knob ? _knob->getValue() : false )
    {
        if (_knob) {
            _knob->setValue(value);
        }
    }

    ~ScopedLoadFromPyFile()
    {
        if (_knob) {
            _knob->setValue(_old);
        }
    }

    bool isValid() const
    {
        return bool(_knob);
    }

    ScopedLoadFromPyFile(const ScopedLoadFromPyFile&) = delete;
    ScopedLoadFromPyFile& operator=(const ScopedLoadFromPyFile&) = delete;

private:
    const KnobBoolPtr _knob;
    const bool _old;
};

// Resets the project when it goes out of scope, also when an ASSERT_* returns early, so that the next tests get an
// empty project
class ScopedProjectReset
{
public:
    explicit ScopedProjectReset(const ProjectPtr& project)
        : _project(project)
    {
    }

    ~ScopedProjectReset()
    {
        _project->resetProject();
    }

    ScopedProjectReset(const ScopedProjectReset&) = delete;
    ScopedProjectReset& operator=(const ScopedProjectReset&) = delete;

private:
    const ProjectPtr _project;
};

// Sets NATRON_PLUGIN_PATH for its lifetime
class ScopedNatronPluginPath
{
public:
    explicit ScopedNatronPluginPath(const QByteArray& value)
        : _old( qgetenv(NATRON_PLUGIN_PATH_ENV_VAR) )
        , _wasSet( qEnvironmentVariableIsSet(NATRON_PLUGIN_PATH_ENV_VAR) )
    {
        qputenv(NATRON_PLUGIN_PATH_ENV_VAR, value);
    }

    ~ScopedNatronPluginPath()
    {
        if (_wasSet) {
            qputenv(NATRON_PLUGIN_PATH_ENV_VAR, _old);
        } else {
            qunsetenv(NATRON_PLUGIN_PATH_ENV_VAR);
        }
    }

    ScopedNatronPluginPath(const ScopedNatronPluginPath&) = delete;
    ScopedNatronPluginPath& operator=(const ScopedNatronPluginPath&) = delete;

private:
    const QByteArray _old;
    const bool _wasSet;
};
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
    ASSERT_TRUE( makeDir(a + pyPlugs) );
    ASSERT_TRUE( makeDir(b + QString::fromUtf8("/Contents/Resources")) );
    ASSERT_TRUE( makeDir(c + pyPlugs) );
    ASSERT_TRUE( makeDir(d + pyPlugs) );
    ASSERT_TRUE( makeDir(e + pyPlugs) );

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
// and version there, in a file of another name, is loaded instead
TEST(OfxBundlePyPlugs, BundlePyPlugIsFound)
{
#ifndef NATRON_TESTS_OFX_PLUGIN_DIR
    NatronTest::skip("no test plug-in bundle");
#else
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginParams);

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
        EXPECT_EQ( canonical( QString::fromUtf8(NATRON_TESTS_DATA_DIR "/ofx-bundle-pyplugs-override/NatronTestBundlePyPlugOverride.py") ), moduleFile );
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
    NatronTest::skip("no test plug-in bundle");
#else
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginParams);

        return;
    }
    const QString bundleDir = testBundlePyPlugsDir();
    QStringList searchPath;
    {
        // with a trailing slash, as a user may write it
        const ScopedNatronPluginPath pluginPath( ( bundleDir + QLatin1Char('/') ).toUtf8() );
        searchPath = appPTR->getAllNonOFXPluginsPaths();
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

// A project saved when the PyPlug was elsewhere (e.g. PIKColor in Natron's Plugins/PyPlugs, before it came with
// openfx-misc's Misc.ofx.bundle) finds it by its file name in the PyPlug search path, which has the PyPlugs directories
// of the OFX bundles, when "Load PyPlugs in projects from .py if possible" is on: the node is created from the
// registered PyPlug of that ID (the bundle's, or the overriding one under TestsOfxBundlePyPlugOverride), the version
// change is offered (the background app answers yes) and the project is saved with it. When the preference is off,
// the node is loaded from the project and the file is not changed.
TEST(OfxBundlePyPlugs, ProjectFindsMovedPyPlugInBundle)
{
#ifndef NATRON_TESTS_OFX_PLUGIN_DIR
    NatronTest::skip("no test plug-in bundle");
#else
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginParams);

        return;
    }
    const PluginsMap& plugins = appPTR->getPluginsList();
    PluginsMap::const_iterator found = plugins.find(kBundlePyPlugID);
    ASSERT_TRUE( found != plugins.end() );
    ASSERT_EQ( 1U, found->second.size() );
    const Plugin* plugin = *found->second.begin();
    ASSERT_TRUE(plugin);
    const QString registeredModuleFile = canonical( plugin->getPythonModule() + QString::fromUtf8(".py") );

    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    // where the PyPlug was when the project was saved: it doesn't exist
    const QByteArray oldModule = ( dir + QString::fromUtf8("old/Plugins/PyPlugs/NatronTestBundlePyPlug.py") ).toUtf8();
    const QByteArray version0("<PythonModuleVersion>0</PythonModuleVersion>");
    const QByteArray version1("<PythonModuleVersion>1</PythonModuleVersion>");

    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    ProjectPtr project = app->getProject();
    ASSERT_TRUE( bool(project) );
    for (int loadFromPy = 0; loadFromPy < 2; ++loadFromPy) {
        const ScopedProjectReset reset(project);
        const ScopedLoadFromPyFile pref( loadFromPy != 0 );
        ASSERT_TRUE( pref.isValid() );
        const QString name = QString::fromUtf8("MovedBundlePyPlug%1.ntp").arg(loadFromPy);

        // a project with the PyPlug, saved as text to edit it: version 0, at oldModule
        project->resetProject();
        {
            CreateNodeArgs args( kBundlePyPlugID, project );
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            ASSERT_TRUE( bool( app->createNode(args) ) );
        }
        QString savedPath;
        ASSERT_TRUE( project->saveProject( dir, name, &savedPath, eProjectFileFormatText ) );
        project->resetProject();
        QByteArray contents = readFile(dir + name);
        ASSERT_EQ( 1, contents.count(version1) ) << contents.constData();
        const QByteArray savedModule = savedPythonModule(contents);
        ASSERT_FALSE( savedModule.isEmpty() ) << contents.constData();
        ASSERT_EQ( 1, contents.count("<PythonModule>" + savedModule + "</PythonModule>") );
        contents.replace("<PythonModule>" + savedModule + "</PythonModule>", "<PythonModule>" + oldModule + "</PythonModule>");
        contents.replace(version1, version0);
        ASSERT_TRUE( writeFile(dir + name, contents) );

        testing::internal::CaptureStdout();
        const bool loaded = project->loadProject(dir, name);
        const std::string output = testing::internal::GetCapturedStdout();
        ASSERT_TRUE(loaded) << output;
        const QByteArray reloaded = readFile(dir + name);
        if (loadFromPy) {
            // the version change was asked (the background app answers yes), then the project was updated to the
            // PyPlug found in the search path, and saved
            EXPECT_NE( std::string::npos, output.find("QUESTION ASKED: New PyPlug version") ) << output;
            EXPECT_EQ( 1, reloaded.count(version1) ) << output;
            const QString module = QString::fromUtf8( savedPythonModule(reloaded) );
            EXPECT_EQ( registeredModuleFile, canonical( module.endsWith( QString::fromUtf8(".py") ) ? module : module + QString::fromUtf8(".py") ) )
                << module.toStdString();
        } else {
            // loaded from the project, without looking at the PyPlug's version: nothing asked, the file is unchanged
            EXPECT_EQ( std::string::npos, output.find("PyPlug version") ) << output;
            EXPECT_EQ(contents, reloaded) << output;
        }
    }
#endif // ifndef NATRON_TESTS_OFX_PLUGIN_DIR
}
