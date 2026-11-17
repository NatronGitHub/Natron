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

// Regression tests for https://github.com/NatronGitHub/Natron/issues/1018
// ("Program crash when trying to move a keyframe").
//
// A slaved (linked) knob listens to the keyframe signals of its master knob. These signals carry the master's
// dimension, which must be mapped to the slave dimension(s) linked to it: the slave may have fewer dimensions
// than the master (a 1-D user parameter linked to the width of a 2-D Size parameter), or be linked to another
// master dimension than its own index.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Curve.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

class KnobSlaveTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        AppInstancePtr app = appPTR->getTopLevelInstance();
        ASSERT_TRUE( bool(app) );
        // Dot is a built-in node: no OpenFX plug-in is needed.
        {
            CreateNodeArgs args( PLUGINID_NATRON_DOT, app->getProject() );
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            _masterNode = app->createNode(args);
        }
        {
            CreateNodeArgs args( PLUGINID_NATRON_DOT, app->getProject() );
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            _slaveNode = app->createNode(args);
        }
        ASSERT_TRUE( bool(_masterNode) && bool(_slaveNode) );

        // Like the Size parameter of the Solid node in the project attached to the issue: 2 dimensions,
        // one keyframe at frame 250 on each.
        _master = AppManager::createKnob<KnobDouble>(_masterNode->getEffectInstance().get(), std::string("size"), 2);
        _master->setName("size");
        _master->setValueAtTime(250., 0., ViewSpec::all(), 0);
        _master->setValueAtTime(250., 0., ViewSpec::all(), 1);
        ASSERT_EQ( 1, _master->getKeyFramesCount(ViewIdx(0), 0) );
        ASSERT_EQ( 1, _master->getKeyFramesCount(ViewIdx(0), 1) );
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        _slave.reset();
        _master.reset();
        if (_slaveNode) {
            _slaveNode->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        if (_masterNode) {
            _masterNode->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        _slaveNode.reset();
        _masterNode.reset();
    }

    // Create a slave knob with the given number of dimensions on the slave node.
    void createSlave(int nDims)
    {
        _slave = AppManager::createKnob<KnobDouble>(_slaveNode->getEffectInstance().get(), std::string("progress"), nDims);
        _slave->setName("progress");
    }

    static double keyTime(const KnobDoublePtr& knob,
                          int dimension)
    {
        KeyFrame k;
        bool ok = knob->getCurve(ViewIdx(0), dimension)->getKeyFrameWithIndex(0, &k);
        EXPECT_TRUE(ok);

        return k.getTime();
    }

    NodePtr _masterNode, _slaveNode;
    KnobDoublePtr _master, _slave;
};

// The 1-D slave is linked to the width (dimension 0) of the master. Moving or deleting the keyframe of the
// height (dimension 1) used to clone dimension 1 of the master into the non-existing dimension 1 of the slave.
TEST_F(KnobSlaveTest, MoveKeyFrameOfUnlinkedMasterDimension)
{
    createSlave(1);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 0) );
    EXPECT_EQ( 1, _slave->getKeyFramesCount(ViewIdx(0), 0) );

    KeyFrame newKey;
    EXPECT_TRUE( _master->moveValueAtTime(eCurveChangeReasonInternal, 250., ViewSpec::all(), 1, 10., 0., &newKey) );
    EXPECT_EQ( 260., keyTime(_master, 1) );
    // The linked dimension did not change.
    EXPECT_EQ( 250., keyTime(_slave, 0) );

    std::vector<KeyFrame> keys(1, newKey);
    EXPECT_TRUE( _master->moveValuesAtTime(eCurveChangeReasonInternal, ViewSpec::all(), 1, -10., 0., &keys) );
    EXPECT_EQ( 250., keyTime(_master, 1) );

    static_cast<KnobI*>( _master.get() )->deleteValueAtTime(eCurveChangeReasonInternal, 250., ViewSpec::all(), 1, true);
    EXPECT_EQ( 0, _master->getKeyFramesCount(ViewIdx(0), 1) );
    EXPECT_EQ( 1, _slave->getKeyFramesCount(ViewIdx(0), 0) );

    _master->setValueAtTime(100., 1., ViewSpec::all(), 1);
    _master->removeAnimation(ViewSpec::all(), 1);
    EXPECT_EQ( 1, _slave->getKeyFramesCount(ViewIdx(0), 0) );
}

// Changes to the linked master dimension are still propagated to the slave.
TEST_F(KnobSlaveTest, MoveKeyFrameOfLinkedMasterDimension)
{
    createSlave(1);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 0) );

    KeyFrame newKey;
    EXPECT_TRUE( _master->moveValueAtTime(eCurveChangeReasonInternal, 250., ViewSpec::all(), 0, 10., 0., &newKey) );
    EXPECT_EQ( 260., keyTime(_master, 0) );
    EXPECT_EQ( 260., keyTime(_slave, 0) );

    _master->setValueAtTime(100., 1., ViewSpec::all(), 0);
    EXPECT_EQ( 2, _slave->getKeyFramesCount(ViewIdx(0), 0) );

    static_cast<KnobI*>( _master.get() )->deleteValueAtTime(eCurveChangeReasonInternal, 260., ViewSpec::all(), 0, true);
    EXPECT_EQ( 1, _slave->getKeyFramesCount(ViewIdx(0), 0) );
    EXPECT_EQ( 100., keyTime(_slave, 0) );

    _master->removeAnimation(ViewSpec::all(), 0);
    EXPECT_EQ( 0, _slave->getKeyFramesCount(ViewIdx(0), 0) );
}

// A slave dimension linked to another master dimension: the master dimension is mapped to the slave dimension.
TEST_F(KnobSlaveTest, MoveKeyFrameOfCrossLinkedDimension)
{
    createSlave(2);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 1) );
    EXPECT_EQ( 1, _slave->getKeyFramesCount(ViewIdx(0), 0) );
    EXPECT_EQ( 0, _slave->getKeyFramesCount(ViewIdx(0), 1) );

    KeyFrame newKey;
    EXPECT_TRUE( _master->moveValueAtTime(eCurveChangeReasonInternal, 250., ViewSpec::all(), 1, 10., 0., &newKey) );
    EXPECT_EQ( 260., keyTime(_slave, 0) );
    // The unlinked slave dimension 1 is left alone.
    EXPECT_EQ( 0, _slave->getKeyFramesCount(ViewIdx(0), 1) );

    // Master dimension 0 is not linked to anything in the slave.
    EXPECT_TRUE( _master->moveValueAtTime(eCurveChangeReasonInternal, 250., ViewSpec::all(), 0, 20., 0., &newKey) );
    EXPECT_EQ( 260., keyTime(_slave, 0) );
    EXPECT_EQ( 0, _slave->getKeyFramesCount(ViewIdx(0), 1) );
}
