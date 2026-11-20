/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

// Loading a project with a Precomp node whose project has nodes inside a group (#1501).
//
// A Precomp node loads its project in its own (background) AppInstance. In background mode,
// AppInstance::getAppIDString() returned "app", an alias of app1, for every instance, so the
// Python variables of the nodes of the Precomp's project were declared on app1, which doesn't
// have them. Creating a node inside a group of that project then failed in
// Node::declarePythonFields() ("attribute app.Write1.Output1 is not defined"): a Group lost its
// children, and a Write node crashed, since WriteNodePrivate::createWriteNode() called
// setScriptName() on the null Output node.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include <QtCore/QString>
#include <QtCore/QTemporaryDir>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Plugin.h"
#include "Engine/PrecompNode.h"
#include "Engine/Project.h"
#include "Engine/WriteNode.h"

NATRON_NAMESPACE_USING

namespace {
bool
isPluginAvailable(const char* pluginID)
{
    try {
        Plugin* p = appPTR->getPluginBinary(QString::fromUtf8(pluginID), -1, -1, false);

        return p && p->getLibraryBinary();
    } catch (...) {
        return false;
    }
}

// The bundled google-test predates GTEST_SKIP(): the tests return early and are reported as passed,
// so say clearly that they did not run, as RotoPaint_Test does.
// TODO: Convert to GTEST_SKIP() when gtest updated.
void
reportSkipped(const std::string& why)
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();

    std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name() << ": " << why << std::endl;
}
} // anonymous namespace

class PrecompLoadTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        _app = appPTR->getTopLevelInstance();
        ASSERT_TRUE( bool(_app) );
        ASSERT_TRUE( _tmpDir.isValid() );
        _dir = _tmpDir.path() + QLatin1Char('/');
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        // Leave the shared app with an untitled empty project, as the other tests find it,
        // before the temporary directory of the saved projects is removed.
        _app->getProject()->resetProject();
    }

    NodePtr createNode(const std::string& pluginID,
                       const NodeCollectionPtr& group)
    {
        CreateNodeArgs args(pluginID, group);

        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

        return _app->createNode(args);
    }

    void saveProject(const QString& name)
    {
        QString savedPath;

        ASSERT_TRUE( _app->getProject()->saveProject(_dir, name, &savedPath) );
    }

    // Creates a Precomp node that uses the project sub.ntp.
    void createPrecomp(PrecompNodePtr* precomp)
    {
        _app->getProject()->resetProject();
        NodePtr precompNode = createNode( PLUGINID_NATRON_PRECOMP, _app->getProject() );
        ASSERT_TRUE( bool(precompNode) );
        KnobFilePtr file = std::dynamic_pointer_cast<KnobFile>( precompNode->getKnobByName("projectFilename") );
        ASSERT_TRUE( bool(file) );
        file->setValue( (_dir + QString::fromUtf8("sub.ntp")).toStdString() );
        *precomp = std::dynamic_pointer_cast<PrecompNode>( precompNode->getEffectInstance() );
        ASSERT_TRUE( bool(*precomp) );
        _precompName = precompNode->getScriptName_mt_safe();
    }

    // Saves the project with the Precomp node in main.ntp and loads main.ntp.
    void saveAndLoadPrecomp(PrecompNodePtr* precomp)
    {
        saveProject( QString::fromUtf8("main.ntp") );
        ASSERT_TRUE( _app->getProject()->loadProject( _dir, QString::fromUtf8("main.ntp") ) );
        NodePtr precompNode = _app->getProject()->getNodeByName(_precompName);
        ASSERT_TRUE( bool(precompNode) );
        *precomp = std::dynamic_pointer_cast<PrecompNode>( precompNode->getEffectInstance() );
        ASSERT_TRUE( bool(*precomp) );
    }

    // The node of the project of the Precomp node with this fully specified name.
    NodePtr precompProjectNode(const PrecompNodePtr& precomp,
                                      const std::string& name)
    {
        AppInstancePtr precompApp = precomp->getPrecompApp();

        EXPECT_TRUE( bool(precompApp) );
        if (!precompApp) {
            return NodePtr();
        }
        // The Precomp's project is loaded in its own app, with its own Python variable.
        EXPECT_NE( precompApp.get(), _app.get() );
        EXPECT_NE( precompApp->getAppIDString(), _app->getAppIDString() );
        // and it does not replace the app the user works in.
        EXPECT_EQ( appPTR->getTopLevelInstance().get(), _app.get() );

        return precompApp->getProject()->getNodeByFullySpecifiedName(name);
    }

    AppInstancePtr _app;
    QTemporaryDir _tmpDir; // removed with the test fixture
    QString _dir;
    std::string _precompName;
};

// Built-in nodes only: always runs. The Dot inside the Group of the Precomp's project was not
// created (the Python variable of the Group was declared on app1).
TEST_F(PrecompLoadTest, GroupChildren)
{
    {
        NodePtr group = createNode( PLUGINID_NATRON_GROUP, _app->getProject() );
        ASSERT_TRUE( bool(group) );
        NodeGroupPtr isGroup = std::dynamic_pointer_cast<NodeGroup>( group->getEffectInstance() );
        ASSERT_TRUE( bool(isGroup) );
        NodePtr dot = createNode(PLUGINID_NATRON_DOT, isGroup);
        ASSERT_TRUE( bool(dot) );
        ASSERT_EQ( group->getScriptName_mt_safe(), std::string("Group1") );
        ASSERT_EQ( dot->getScriptName_mt_safe(), std::string("Dot1") );
    }
    saveProject( QString::fromUtf8("sub.ntp") );

    PrecompNodePtr precomp;
    createPrecomp(&precomp);
    if ( HasFatalFailure() ) {
        return;
    }
    EXPECT_TRUE( bool( precompProjectNode(precomp, "Group1") ) );
    EXPECT_TRUE( bool( precompProjectNode(precomp, "Group1.Dot1") ) );
    precomp.reset();

    saveAndLoadPrecomp(&precomp);
    if ( HasFatalFailure() ) {
        return;
    }
    EXPECT_TRUE( bool( precompProjectNode(precomp, "Group1") ) );
    EXPECT_TRUE( bool( precompProjectNode(precomp, "Group1.Dot1") ) );
}

// The case of #1501: a Write node in the Precomp's project crashed while its Output node was created.
TEST_F(PrecompLoadTest, WriteNode)
{
    if ( !isPluginAvailable(PLUGINID_OFX_WRITEOIIO) ) {
        reportSkipped("plug-in not found (needs " PLUGINID_OFX_WRITEOIIO " from openfx-io in OFX_PLUGIN_PATH)");

        return;
    }
    {
        CreateNodeArgs args( PLUGINID_NATRON_WRITE, _app->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args.addParamDefaultValue<std::string>( kOfxImageEffectFileParamName, (_dir + QString::fromUtf8("out.####.exr")).toStdString() );
        NodePtr write = _app->createNode(args);
        ASSERT_TRUE( bool(write) );
        ASSERT_EQ( write->getScriptName_mt_safe(), std::string("Write1") );
    }
    saveProject( QString::fromUtf8("sub.ntp") );

    // Created, then saved and loaded (the case of the issue).
    for (int i = 0; i < 2; ++i) {
        PrecompNodePtr precomp;
        if (i == 0) {
            createPrecomp(&precomp);
        } else {
            saveAndLoadPrecomp(&precomp);
        }
        if ( HasFatalFailure() ) {
            return;
        }
        NodePtr write = precompProjectNode(precomp, "Write1");
        ASSERT_TRUE( bool(write) );
        WriteNode* isWrite = dynamic_cast<WriteNode*>( write->getEffectInstance().get() );
        ASSERT_TRUE(isWrite != NULL);
        NodePtr writer = isWrite->getEmbeddedWriter();
        ASSERT_TRUE( bool(writer) );
        EXPECT_EQ( writer->getPluginID(), std::string(PLUGINID_OFX_WRITEOIIO) );
    }
}
