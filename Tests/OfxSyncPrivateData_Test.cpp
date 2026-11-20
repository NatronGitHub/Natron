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

// kOfxActionSyncPrivateData "is called when a plugin should synchronise any private data structures to
// its parameter set. This generally occurs when an effect is about to be saved or copied" (ofxCore.h).
// The autosave serializes the project in another thread, where the sync request was only queued to the
// main thread: the parameters were saved before the plug-in synced them (#1368).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <thread>

#include <gtest/gtest.h>

#include <QtCore/QCoreApplication>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
// the value of changeCount in a serialization of node, made in another thread if otherThread
int
serializedChangeCount(const NodePtr& node,
                      bool otherThread)
{
    NodeSerializationPtr serialization;

    if (otherThread) {
        std::thread t([&]() {
            serialization = std::make_shared<NodeSerialization>(node);
        });
        t.join();
    } else {
        serialization = std::make_shared<NodeSerialization>(node);
    }
    const NodeSerialization::KnobValues& values = serialization->getKnobsValues();
    for (NodeSerialization::KnobValues::const_iterator it = values.begin(); it != values.end(); ++it) {
        if ( (*it)->getName() == "changeCount" ) {
            KnobIntPtr knob = std::dynamic_pointer_cast<KnobInt>( (*it)->getKnob() );

            return knob ? knob->getValue() : -1;
        }
    }

    return -1;
}

class OfxSyncPrivateDataTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginSyncPrivateData) ) {
            return;
        }
        AppInstancePtr app = appPTR->getTopLevelInstance();
        CreateNodeArgs args( kNatronTestPluginSyncPrivateData, app->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        _node = app->createNode(args);
        ASSERT_TRUE( bool(_node) );
        _value = std::dynamic_pointer_cast<KnobInt>( _node->getKnobByName("value") );
        ASSERT_TRUE( bool(_value) );
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        _node.reset();
    }

    // change "value" n times: the plug-in counts the changes in its private data
    void changeValue(int n)
    {
        for (int i = 0; i < n; ++i) {
            _value->setValue(_value->getValue() + 1);
        }
    }

    NodePtr _node;
    KnobIntPtr _value;
};
} // anonymous namespace

TEST_F(OfxSyncPrivateDataTest, SaveInMainThread)
{
    if (!_node) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginSyncPrivateData);

        return;
    }
    changeValue(3);
    EXPECT_EQ( 3, serializedChangeCount(_node, false) );
}

// The autosave: Project::syncOpenFXPrivateData() in the main thread, then the serialization in
// another thread.
TEST_F(OfxSyncPrivateDataTest, AutoSave)
{
    if (!_node) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginSyncPrivateData);

        return;
    }
    changeValue(3);
    const int syncs = NatronTestPlugin::counter("SyncPrivateData.sync");
    appPTR->getTopLevelInstance()->getProject()->syncOpenFXPrivateData();
    EXPECT_EQ( syncs + 1, NatronTestPlugin::counter("SyncPrivateData.sync") );
    EXPECT_EQ( 3, serializedChangeCount(_node, true) );
    // the serialization in the other thread doesn't request another sync, which would change the
    // parameters while they are being read
    QCoreApplication::processEvents();
    EXPECT_EQ( syncs + 1, NatronTestPlugin::counter("SyncPrivateData.sync") );
}
