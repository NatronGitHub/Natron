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

#ifndef NATRON_TESTS_TESTUTILS_H
#define NATRON_TESTS_TESTUTILS_H

// Helpers shared by the tests (Tests/*_Test.cpp): skipping a test, the plug-ins it needs, the nodes
// it creates and the project it changes. The helpers for the OFX test plug-ins are in OfxTestPlugin.h.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <exception>
#include <initializer_list>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/OSGLContext.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

namespace NatronTest {
// The name of the current test, "Suite.Name" (empty outside a test, e.g. in an environment set-up)
inline std::string
currentTestName()
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();

    return info ? std::string( info->test_case_name() ) + '.' + info->name() : std::string();
}

// Says that the current test did not run, and why. The bundled google-test (1.7) predates GTEST_SKIP():
// the test then returns early and is reported as passed. Every skip prints this one line, so that the
// skips can be counted in the output:
//   [  SKIPPED ] Suite.Name: reason
// TODO: GTEST_SKIP() once google-test is upgraded.
inline void
skip(const std::string& reason)
{
    std::cerr << "[  SKIPPED ] " << currentTestName() << ": " << reason << std::endl;
}

// Says that a part of the current test did not run, and why; the rest of the test runs.
inline void
skipPart(const std::string& reason)
{
    std::cerr << "[  PARTIAL  ] " << currentTestName() << ": part not run: " << reason << std::endl;
}

// true if the host loaded a plug-in with that ID (any version)
inline bool
isPluginAvailable(const std::string& pluginID)
{
    try {
        NATRON_NAMESPACE::Plugin* p = NATRON_NAMESPACE::appPTR->getPluginBinary(QString::fromUtf8( pluginID.c_str() ), -1, -1, false);

        return p && p->getLibraryBinary();
    } catch (...) {
        return false;
    }
}

// false, and the test is skipped, if one of the plug-ins was not loaded. bundle says where they come
// from, e.g. "openfx-misc".
inline bool
requirePlugins(std::initializer_list<const char*> pluginIDs,
               const char* bundle)
{
    for (const char* id : pluginIDs) {
        if ( !isPluginAvailable(id) ) {
            skip(std::string("plug-in ") + id + " not found (needs " + bundle + " in OFX_PLUGIN_PATH)");

            return false;
        }
    }

    return true;
}

// Whether a new Group node gets its Input and Output nodes, as in the GUI
enum class GroupInitialNodes
{
    eCreate,
    eNone
};

// Creates a node in group (by default, the project of the top-level app): without undo/redo command,
// without connecting it and without opening its settings panel.
inline NATRON_NAMESPACE::NodePtr
createNode(const std::string& pluginID,
           NATRON_NAMESPACE::NodeCollectionPtr group = NATRON_NAMESPACE::NodeCollectionPtr(),
           GroupInitialNodes initialNodes = GroupInitialNodes::eCreate)
{
    NATRON_NAMESPACE::AppInstancePtr app = NATRON_NAMESPACE::appPTR->getTopLevelInstance();

    if (!group) {
        group = app->getProject();
    }
    NATRON_NAMESPACE::CreateNodeArgs args(pluginID, group);
    args.setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, initialNodes == GroupInitialNodes::eNone);
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

    return app->createNode(args);
}

// The nodes of a test, destroyed when it ends (in the reverse order), also when an ASSERT fails, so that
// they don't stay in the project that the next tests use.
class NodeScope
{
public:
    NodeScope() = default;
    NodeScope(const NodeScope&) = delete;
    NodeScope& operator=(const NodeScope&) = delete;

    // a destructor must not throw (C.36): a failure is reported as a test failure
    ~NodeScope()
    {
        for (std::vector<NATRON_NAMESPACE::NodePtr>::reverse_iterator it = _nodes.rbegin(); it != _nodes.rend(); ++it) {
            try {
                (*it)->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            } catch (const std::exception& e) {
                ADD_FAILURE() << "destroying node " << (*it)->getScriptName_mt_safe() << ": " << e.what();
            }
        }
    }

    // createNode(), and the node is destroyed with the scope
    NATRON_NAMESPACE::NodePtr create(const std::string& pluginID,
                                     const NATRON_NAMESPACE::NodeCollectionPtr& group = NATRON_NAMESPACE::NodeCollectionPtr())
    {
        return add( createNode(pluginID, group) );
    }

    // node (if not null) is destroyed with the scope
    NATRON_NAMESPACE::NodePtr add(const NATRON_NAMESPACE::NodePtr& node)
    {
        if (node) {
            _nodes.push_back(node);
        }

        return node;
    }

private:
    std::vector<NATRON_NAMESPACE::NodePtr> _nodes;
};

// Resets the project of the top-level app when the test ends, also when an ASSERT fails, so that the next
// tests get an untitled empty project (e.g. not one in a removed directory).
struct ProjectResetScope
{
    ProjectResetScope() = default;
    ProjectResetScope(const ProjectResetScope&) = delete;
    ProjectResetScope& operator=(const ProjectResetScope&) = delete;

    ~ProjectResetScope()
    {
        try {
            NATRON_NAMESPACE::appPTR->getTopLevelInstance()->getProject()->resetProject();
        } catch (const std::exception& e) {
            ADD_FAILURE() << "resetting the project: " << e.what();
        }
    }
};

// false, and the test is skipped, if OpenGL rendering can't be enabled. OpenGL may load without any renderer
// that can render: e.g. on the Linux CI (Xvfb, Mesa llvmpipe), OSGLContext_x11::getGPUInfos() skips the
// renderers that are not accelerated, so the "OpenGL Rendering" preference is hidden and
// Settings::isOpenGLRenderingEnabled() is always false.
inline bool
requireOpenGLRendering()
{
    if ( !NATRON_NAMESPACE::appPTR->isOpenGLLoaded() ) {
        skip("OpenGL could not be loaded");

        return false;
    }
    if ( !NATRON_NAMESPACE::appPTR->hasOpenGLForRequirements(NATRON_NAMESPACE::eOpenGLRequirementsTypeRendering) ||
         NATRON_NAMESPACE::appPTR->getOpenGLRenderers().empty() ) {
        skip("no OpenGL renderer can be used for OpenGL rendering");

        return false;
    }

    return true;
}

// Sets OpenGL rendering (Preferences/GPU Rendering) for the lifetime of the object.
// Changing that setting deletes the OpenFX plug-in load cache (Settings::onKnobValueChanged() calls
// AppManager::clearPluginsLoadedCache()). The cache files are saved first and written back afterwards:
// the TestsOfxCache test (Tests/CMakeLists.txt) runs on the cache written by the full Tests run.
class ScopedOpenGLRendering
{
public:
    explicit ScopedOpenGLRendering(NATRON_NAMESPACE::Settings::EnableOpenGLEnum value = NATRON_NAMESPACE::Settings::eEnableOpenGLEnabled)
        : _knob( dynamic_cast<NATRON_NAMESPACE::KnobChoice*>( NATRON_NAMESPACE::appPTR->getCurrentSettings()->getKnobByName("enableOpenGLRendering").get() ) )
        , _savedValue(0)
        , _ofxCacheFiles()
    {
        if (_knob) {
            const QDir dir( ofxCacheDirPath() );
            const QStringList files = dir.entryList(QDir::Files);
            for (int i = 0; i < files.size(); ++i) {
                QFile f( dir.absoluteFilePath(files[i]) );
                if ( f.open(QIODevice::ReadOnly) ) {
                    _ofxCacheFiles[files[i]] = f.readAll();
                }
            }
            _savedValue = _knob->getValue();
            _knob->setValue( (int)value );
        }
    }

    ScopedOpenGLRendering(const ScopedOpenGLRendering&) = delete;
    ScopedOpenGLRendering& operator=(const ScopedOpenGLRendering&) = delete;

    ~ScopedOpenGLRendering()
    {
        if (!_knob) {
            return;
        }
        try {
            _knob->setValue(_savedValue);
        } catch (const std::exception& e) {
            ADD_FAILURE() << "restoring the OpenGL rendering preference: " << e.what();
        }
        if ( _ofxCacheFiles.empty() ) {
            return;
        }
        const QDir dir( ofxCacheDirPath() );
        dir.mkpath( QString::fromUtf8(".") );
        for (std::map<QString, QByteArray>::const_iterator it = _ofxCacheFiles.begin(); it != _ofxCacheFiles.end(); ++it) {
            QFile f( dir.absoluteFilePath(it->first) );
            if ( f.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
                f.write(it->second);
            }
        }
    }

    bool isEnabled() const
    {
        return NATRON_NAMESPACE::appPTR->getCurrentSettings()->isOpenGLRenderingEnabled();
    }

private:
    static QString ofxCacheDirPath()
    {
        return NATRON_NAMESPACE::appPTR->getDiskCacheLocation() + QString::fromUtf8("/OFXLoadCache");
    }

    NATRON_NAMESPACE::KnobChoice* _knob;
    int _savedValue;
    std::map<QString, QByteArray> _ofxCacheFiles;
};
} // namespace NatronTest

#endif // NATRON_TESTS_TESTUTILS_H
