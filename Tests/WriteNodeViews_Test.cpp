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


#include <Python.h>

#include "Global/Macros.h"

#include <iostream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <QtCore/QTemporaryDir>
#include <QtCore/QString>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/WriteNode.h"

NATRON_NAMESPACE_USING

// The Views parameter of WriteOIIO in multi-view projects (#1312): it is hidden when the file name has
// a %v or %V view pattern (each view goes to its own file), and otherwise follows the plug-in, which
// shows it for formats that can hold several views in one file (EXR).

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

class WriteNodeViewsTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        _app = appPTR->getTopLevelInstance();
        ASSERT_TRUE( _tmpDir.isValid() );
        _dir = _tmpDir.path().toStdString();
        _viewsList = std::dynamic_pointer_cast<KnobPath>( _app->getProject()->getKnobByName("viewsList") );
        ASSERT_TRUE( bool(_viewsList) );
        _savedViewsList = _viewsList->getValue();
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        _node.reset();
        if (_projectLoaded) {
            // Leave the shared app with an untitled empty project, as the other tests find it,
            // before the temporary directory of the saved project is removed.
            _app->getProject()->resetProject();
            _projectLoaded = false;
        }
        // Back to the views the other tests expect (a single view).
        _viewsList = std::dynamic_pointer_cast<KnobPath>( _app->getProject()->getKnobByName("viewsList") );
        if (_viewsList) {
            _viewsList->setValue(_savedViewsList);
        }
    }

    void createWriteNode(const std::string& filename)
    {
        CreateNodeArgs args( PLUGINID_NATRON_WRITE, _app->getProject() );

        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
        _node = _app->createNode(args);
        ASSERT_TRUE( bool(_node) );
        WriteNode* write = dynamic_cast<WriteNode*>( _node->getEffectInstance().get() );
        ASSERT_TRUE(write != NULL);
        NodePtr writer = write->getEmbeddedWriter();
        ASSERT_TRUE( bool(writer) );
        ASSERT_EQ( writer->getPluginID(), std::string(PLUGINID_OFX_WRITEOIIO) );
    }

    void setFile(const std::string& filename)
    {
        KnobOutputFilePtr file = std::dynamic_pointer_cast<KnobOutputFile>( _node->getKnobByName(kOfxImageEffectFileParamName) );

        ASSERT_TRUE( bool(file) );
        file->setValue(filename);
    }

    // Fetched again after each file name change: a new extension may create a new WriteOIIO.
    KnobChoicePtr views() const
    {
        KnobChoicePtr knob = std::dynamic_pointer_cast<KnobChoice>( _node->getKnobByName(kWriteOIIOParamViewsSelector) );

        EXPECT_TRUE( bool(knob) );

        return knob;
    }

    // Whether the Views parameter is hidden (a missing parameter fails the test and counts as hidden).
    bool viewsSecret() const
    {
        KnobChoicePtr knob = views();

        return knob ? knob->getIsSecret() : true;
    }

    // Saves the project and loads it again, as a project saved by Natron 2.5 when secret is true:
    // the Views parameter is saved hidden.
    void saveAndLoad(bool secret)
    {
        KnobChoicePtr knob = views();
        ASSERT_TRUE( bool(knob) );
        knob->setSecret(secret);
        knob.reset();
        const std::string name = _node->getScriptName_mt_safe();
        _node.reset();

        const QString dir = QString::fromUtf8( _dir.c_str() ) + QLatin1Char('/');
        QString savedPath;
        ASSERT_TRUE( _app->getProject()->saveProject( dir, QString::fromUtf8("WriteNodeViews.ntp"), &savedPath ) );
        _projectLoaded = true;
        ASSERT_TRUE( _app->getProject()->loadProject( dir, QString::fromUtf8("WriteNodeViews.ntp") ) );
        _node = _app->getProject()->getNodeByName(name);
        ASSERT_TRUE( bool(_node) );
        ASSERT_TRUE( bool( views() ) );
    }

    AppInstancePtr _app;
    QTemporaryDir _tmpDir; // removed with the test fixture
    std::string _dir;
    KnobPathPtr _viewsList;
    std::string _savedViewsList;
    bool _projectLoaded = false;
    NodePtr _node;
};

TEST_F(WriteNodeViewsTest, ViewPattern)
{
    if ( !isPluginAvailable(PLUGINID_OFX_WRITEOIIO) ) {
        reportSkipped("plug-in not found (needs " PLUGINID_OFX_WRITEOIIO " from openfx-io in OFX_PLUGIN_PATH)");

        return;
    }
    _app->getProject()->setupProjectForStereo();
    ASSERT_EQ(_app->getProject()->getProjectViewsCount(), 2);

    // '%' in the frame number pattern is not a view pattern: EXR can hold both views.
    createWriteNode(_dir + "/shot.%04d.exr");
    EXPECT_FALSE( viewsSecret() );

    // Each view goes to its own file.
    setFile(_dir + "/shot_%V.%04d.exr");
    EXPECT_TRUE( viewsSecret() );
    setFile(_dir + "/shot_%v.%04d.exr");
    EXPECT_TRUE( viewsSecret() );

    // No view pattern any more: the Views parameter is back.
    setFile(_dir + "/shot.%04d.exr");
    EXPECT_FALSE( viewsSecret() );

    // A format that cannot hold several views in one file (TIFF; PNG files go to WritePNG, which has no
    // Views parameter): WriteOIIO hides the parameter, the host does not show it.
    setFile(_dir + "/shot.%04d.tif");
    EXPECT_TRUE( viewsSecret() );
}

// A project saved with the Views parameter hidden although the file name has no view pattern (Natron
// 2.5 took any '%' or 'v' for a view pattern): the parameter is shown after the load.
TEST_F(WriteNodeViewsTest, LoadProjectSavedWithViewsHidden)
{
    if ( !isPluginAvailable(PLUGINID_OFX_WRITEOIIO) ) {
        reportSkipped("plug-in not found (needs " PLUGINID_OFX_WRITEOIIO " from openfx-io in OFX_PLUGIN_PATH)");

        return;
    }
    _app->getProject()->setupProjectForStereo();
    createWriteNode(_dir + "/shot.%04d.exr");
    saveAndLoad(true);
    EXPECT_EQ(_app->getProject()->getProjectViewsCount(), 2);
    EXPECT_FALSE( viewsSecret() );
}

// A project saved with a view pattern keeps the Views parameter hidden after the load, even if it was
// saved visible.
TEST_F(WriteNodeViewsTest, LoadProjectWithViewPattern)
{
    if ( !isPluginAvailable(PLUGINID_OFX_WRITEOIIO) ) {
        reportSkipped("plug-in not found (needs " PLUGINID_OFX_WRITEOIIO " from openfx-io in OFX_PLUGIN_PATH)");

        return;
    }
    _app->getProject()->setupProjectForStereo();
    createWriteNode(_dir + "/shot_%V.%04d.exr");
    saveAndLoad(false);
    EXPECT_TRUE( viewsSecret() );
}

// Single-view projects follow the same rules: without a view pattern, the plug-in decides.
TEST_F(WriteNodeViewsTest, SingleView)
{
    if ( !isPluginAvailable(PLUGINID_OFX_WRITEOIIO) ) {
        reportSkipped("plug-in not found (needs " PLUGINID_OFX_WRITEOIIO " from openfx-io in OFX_PLUGIN_PATH)");

        return;
    }
    ASSERT_EQ(_app->getProject()->getProjectViewsCount(), 1);
    createWriteNode(_dir + "/shot.%04d.exr");
    EXPECT_FALSE( viewsSecret() );
    setFile(_dir + "/shot.%04d.tif");
    EXPECT_TRUE( viewsSecret() );
    setFile(_dir + "/shot.%04d.exr");
    saveAndLoad(true);
    EXPECT_FALSE( viewsSecret() );
}

// #693: a video file name is not a sequence pattern, so its '#' is not replaced by the frame number, while
// it still is in the name of an image sequence.
TEST_F(WriteNodeViewsTest, VideoFileNameKeepsHash)
{
    if ( !isPluginAvailable(PLUGINID_OFX_WRITEFFMPEG) || !isPluginAvailable(PLUGINID_OFX_WRITEOIIO) ) {
        reportSkipped("plug-in not found (needs " PLUGINID_OFX_WRITEFFMPEG " and " PLUGINID_OFX_WRITEOIIO " from openfx-io in OFX_PLUGIN_PATH)");

        return;
    }
    const std::string video = _dir + "/take#1.mov";
    CreateNodeArgs args( PLUGINID_NATRON_WRITE, _app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, video);
    _node = _app->createNode(args);
    ASSERT_TRUE( bool(_node) );
    WriteNode* write = dynamic_cast<WriteNode*>( _node->getEffectInstance().get() );
    ASSERT_TRUE(write != NULL);
    ASSERT_TRUE( bool( write->getEmbeddedWriter() ) );
    ASSERT_EQ( write->getEmbeddedWriter()->getPluginID(), std::string(PLUGINID_OFX_WRITEFFMPEG) );
    KnobOutputFilePtr file = std::dynamic_pointer_cast<KnobOutputFile>( _node->getKnobByName(kOfxImageEffectFileParamName) );
    ASSERT_TRUE( bool(file) );
    EXPECT_EQ( video, file->generateFileNameAtTime( 5, ViewIdx(0) ).toStdString() );

    setFile(_dir + "/shot.#.exr");
    file = std::dynamic_pointer_cast<KnobOutputFile>( _node->getKnobByName(kOfxImageEffectFileParamName) );
    ASSERT_TRUE( bool(file) );
    EXPECT_EQ( _dir + "/shot.5.exr", file->generateFileNameAtTime( 5, ViewIdx(0) ).toStdString() );
}
