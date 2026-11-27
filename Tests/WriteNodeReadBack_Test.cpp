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

// The internal Read node of a Write node ("Read back file", #1785) gets the frame range of the writer.
// Its First Frame was set to the writer's last frame; and as the internal Read is a Read node, which
// creates its decoder after the initial values are set, it actually kept the range guessed from the files
// on disk (1-1 when there are none yet), until the writer's range changed. A change used the writer's
// range cached at the first query.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include <QtCore/QTemporaryDir>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/WriteNode.h"

#include "TestUtils.h"

NATRON_NAMESPACE_USING

class WriteNodeReadBack
    : public testing::Test
{
protected:

    // the shared project stays empty for the next tests, also after a failed ASSERT
    virtual void TearDown() OVERRIDE FINAL
    {
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        _node.reset();
    }

    NodePtr _node;
};

TEST_F(WriteNodeReadBack, ReadNodeGetsTheFrameRangeOfTheWriter)
{
    if ( !NatronTest::requirePlugins({ PLUGINID_OFX_WRITEOIIO, PLUGINID_OFX_READOIIO }, "openfx-io") ) {
        return;
    }
    AppInstancePtr app = appPTR->getTopLevelInstance();
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );

    CreateNodeArgs args( PLUGINID_NATRON_WRITE, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.addParamDefaultValue<std::string>( kOfxImageEffectFileParamName, tmpDir.path().toStdString() + "/readback.####.exr" );
    _node = app->createNode(args);
    const NodePtr& node = _node;
    ASSERT_TRUE( bool(node) );
    WriteNode* write = dynamic_cast<WriteNode*>( node->getEffectInstance().get() );
    ASSERT_TRUE(write != NULL);

    // a manual frame range for the writer, then "Read back file", which creates the internal Read node
    KnobChoicePtr frameRange = std::dynamic_pointer_cast<KnobChoice>( node->getKnobByName("frameRange") );
    KnobIntPtr firstFrame = std::dynamic_pointer_cast<KnobInt>( node->getKnobByName("firstFrame") );
    KnobIntPtr lastFrame = std::dynamic_pointer_cast<KnobInt>( node->getKnobByName("lastFrame") );
    KnobBoolPtr readBack = std::dynamic_pointer_cast<KnobBool>( node->getKnobByName(kNatronWriteParamReadBack) );
    ASSERT_TRUE(frameRange && firstFrame && lastFrame && readBack);
    frameRange->setValue(2); // Manual
    firstFrame->setValue(10);
    lastFrame->setValue(20);
    readBack->setValue(true);

    NodePtr reader = write->getNodeByName("internalDecoderNode");
    ASSERT_TRUE( bool(reader) );
    KnobIntPtr readerFirstFrame = std::dynamic_pointer_cast<KnobInt>( reader->getKnobByName("firstFrame") );
    KnobIntPtr readerLastFrame = std::dynamic_pointer_cast<KnobInt>( reader->getKnobByName("lastFrame") );
    ASSERT_TRUE(readerFirstFrame && readerLastFrame);
    EXPECT_EQ( 10, readerFirstFrame->getValue() );
    EXPECT_EQ( 20, readerLastFrame->getValue() );

    // and follows the changes of the writer's range (the cached range of the writer was the first one)
    lastFrame->setValue(30);
    EXPECT_EQ( 10, readerFirstFrame->getValue() );
    EXPECT_EQ( 30, readerLastFrame->getValue() );
}
