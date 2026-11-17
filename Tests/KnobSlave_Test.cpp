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

    // The keyframes of the knob's own curve. KnobI::getKeyFramesCount() and getCurve() without byPassMaster
    // return those of the master for a slaved dimension, which would not test anything.
    static int ownKeyCount(const KnobIPtr& knob,
                           int dimension)
    {
        return knob->getCurve(ViewIdx(0), dimension, true /*byPassMaster*/)->getKeyFramesCount();
    }

    // The time of keyframe 'index' in the knob's own curve.
    static double ownKeyTime(const KnobIPtr& knob,
                             int dimension,
                             int index = 0)
    {
        KeyFrame k;
        bool ok = knob->getCurve(ViewIdx(0), dimension, true /*byPassMaster*/)->getKeyFrameWithIndex(index, &k);
        EXPECT_TRUE(ok);

        return k.getTime();
    }

    static bool moveKey(const KnobIPtr& knob,
                        int dimension,
                        double time,
                        double dt)
    {
        KeyFrame newKey;

        return knob->moveValueAtTime(eCurveChangeReasonInternal, time, ViewSpec::all(), dimension, dt, 0., &newKey);
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
    EXPECT_EQ( 1, ownKeyCount(_slave, 0) );

    EXPECT_TRUE( moveKey(_master, 1, 250., 10.) );
    EXPECT_EQ( 260., ownKeyTime(_master, 1) );
    // The linked dimension did not change.
    EXPECT_EQ( 250., ownKeyTime(_slave, 0) );

    KeyFrame k;
    ASSERT_TRUE( _master->getCurve(ViewIdx(0), 1)->getKeyFrameWithIndex(0, &k) );
    std::vector<KeyFrame> keys(1, k);
    EXPECT_TRUE( _master->moveValuesAtTime(eCurveChangeReasonInternal, ViewSpec::all(), 1, -10., 0., &keys) );
    EXPECT_EQ( 250., ownKeyTime(_master, 1) );

    static_cast<KnobI*>( _master.get() )->deleteValueAtTime(eCurveChangeReasonInternal, 250., ViewSpec::all(), 1, true);
    EXPECT_EQ( 0, ownKeyCount(_master, 1) );
    EXPECT_EQ( 1, ownKeyCount(_slave, 0) );

    _master->setValueAtTime(100., 1., ViewSpec::all(), 1);
    _master->removeAnimation(ViewSpec::all(), 1);
    EXPECT_EQ( 1, ownKeyCount(_slave, 0) );
    EXPECT_EQ( 250., ownKeyTime(_slave, 0) );
}

// Changes to the linked master dimension are still propagated to the slave's own curve.
TEST_F(KnobSlaveTest, MoveKeyFrameOfLinkedMasterDimension)
{
    createSlave(1);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 0) );

    EXPECT_TRUE( moveKey(_master, 0, 250., 10.) );
    EXPECT_EQ( 260., ownKeyTime(_master, 0) );
    EXPECT_EQ( 260., ownKeyTime(_slave, 0) );

    _master->setValueAtTime(100., 1., ViewSpec::all(), 0);
    EXPECT_EQ( 2, ownKeyCount(_slave, 0) );

    static_cast<KnobI*>( _master.get() )->deleteValueAtTime(eCurveChangeReasonInternal, 260., ViewSpec::all(), 0, true);
    EXPECT_EQ( 1, ownKeyCount(_slave, 0) );
    EXPECT_EQ( 100., ownKeyTime(_slave, 0) );

    _master->removeAnimation(ViewSpec::all(), 0);
    EXPECT_EQ( 0, ownKeyCount(_slave, 0) );
}

// The slave's own (unlinked) dimension 1 keeps its animation when dimension 1 of the master changes:
// it used to be silently overwritten by a copy of the master's dimension 1.
TEST_F(KnobSlaveTest, UnlinkedSlaveDimensionKeepsItsAnimation)
{
    createSlave(2);
    _slave->setValueAtTime(10., 1., ViewSpec::all(), 1);
    _slave->setValueAtTime(20., 2., ViewSpec::all(), 1);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 0) );
    ASSERT_EQ( 2, ownKeyCount(_slave, 1) );

    EXPECT_TRUE( moveKey(_master, 1, 250., 10.) );
    _master->setValueAtTime(300., 5., ViewSpec::all(), 1);
    static_cast<KnobI*>( _master.get() )->deleteValueAtTime(eCurveChangeReasonInternal, 300., ViewSpec::all(), 1, true);
    _master->removeAnimation(ViewSpec::all(), 1);

    EXPECT_EQ( 2, ownKeyCount(_slave, 1) );
    EXPECT_EQ( 10., ownKeyTime(_slave, 1, 0) );
    EXPECT_EQ( 20., ownKeyTime(_slave, 1, 1) );
    EXPECT_EQ( 250., ownKeyTime(_slave, 0) );
}

// A slave dimension linked to another master dimension: the master dimension is mapped to the slave dimension.
TEST_F(KnobSlaveTest, MoveKeyFrameOfCrossLinkedDimension)
{
    createSlave(2);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 1) );
    EXPECT_EQ( 1, ownKeyCount(_slave, 0) );
    EXPECT_EQ( 0, ownKeyCount(_slave, 1) );

    EXPECT_TRUE( moveKey(_master, 1, 250., 10.) );
    EXPECT_EQ( 260., ownKeyTime(_slave, 0) );
    // The unlinked slave dimension 1 is left alone.
    EXPECT_EQ( 0, ownKeyCount(_slave, 1) );

    // Master dimension 0 is not linked to anything in the slave.
    EXPECT_TRUE( moveKey(_master, 0, 250., 20.) );
    EXPECT_EQ( 260., ownKeyTime(_slave, 0) );
    EXPECT_EQ( 0, ownKeyCount(_slave, 1) );
}

// Each slave dimension is linked to a different master knob: a change of one master only updates its own
// slave dimension.
TEST_F(KnobSlaveTest, TwoMasterKnobs)
{
    KnobDoublePtr master2 = AppManager::createKnob<KnobDouble>(_masterNode->getEffectInstance().get(), std::string("size2"), 1);
    master2->setName("size2");
    master2->setValueAtTime(300., 0., ViewSpec::all(), 0);

    createSlave(2);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 0) );
    ASSERT_TRUE( _slave->slaveTo(1, master2, 0) );
    EXPECT_EQ( 250., ownKeyTime(_slave, 0) );
    EXPECT_EQ( 300., ownKeyTime(_slave, 1) );

    EXPECT_TRUE( moveKey(master2, 0, 300., 10.) );
    EXPECT_EQ( 250., ownKeyTime(_slave, 0) );
    EXPECT_EQ( 310., ownKeyTime(_slave, 1) );

    EXPECT_TRUE( moveKey(_master, 0, 250., 20.) );
    EXPECT_EQ( 270., ownKeyTime(_slave, 0) );
    EXPECT_EQ( 310., ownKeyTime(_slave, 1) );

    _slave->unSlave(1, false);
    _slave->unSlave(0, false);
}

// Several slave dimensions linked to the same master dimension are all updated.
TEST_F(KnobSlaveTest, SeveralSlaveDimensionsLinkedToOneMasterDimension)
{
    createSlave(2);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 0) );
    ASSERT_TRUE( _slave->slaveTo(1, _master, 0) );

    EXPECT_TRUE( moveKey(_master, 0, 250., 10.) );
    EXPECT_EQ( 260., ownKeyTime(_slave, 0) );
    EXPECT_EQ( 260., ownKeyTime(_slave, 1) );

    _master->setValueAtTime(100., 1., ViewSpec::all(), 0);
    EXPECT_EQ( 2, ownKeyCount(_slave, 0) );
    EXPECT_EQ( 2, ownKeyCount(_slave, 1) );
}

// An integer slave of a double master.
TEST_F(KnobSlaveTest, IntSlaveOfDoubleMaster)
{
    KnobIntPtr slave = AppManager::createKnob<KnobInt>(_slaveNode->getEffectInstance().get(), std::string("index"), 1);
    slave->setName("index");
    ASSERT_TRUE( slave->slaveTo(0, _master, 0) );
    EXPECT_EQ( 250., ownKeyTime(slave, 0) );

    EXPECT_TRUE( moveKey(_master, 1, 250., 10.) );
    EXPECT_EQ( 250., ownKeyTime(slave, 0) );

    EXPECT_TRUE( moveKey(_master, 0, 250., 10.) );
    EXPECT_EQ( 260., ownKeyTime(slave, 0) );

    slave->unSlave(0, false);
}

// Unlinking one dimension must not stop the keyframe updates of the other dimensions still linked to the same
// master knob: the master's signals are connected once per (master, slave) knob pair.
TEST_F(KnobSlaveTest, UnlinkOneDimensionOthersStillFollowTheMaster)
{
    createSlave(2);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 0) );
    ASSERT_TRUE( _slave->slaveTo(1, _master, 1) );
    EXPECT_EQ( 250., ownKeyTime(_slave, 1) );

    _slave->unSlave(0, true);
    EXPECT_FALSE( _slave->isSlave(0) );
    EXPECT_TRUE( _slave->isSlave(1) );

    // Dimension 1 is still linked: its own curve follows the master.
    EXPECT_TRUE( moveKey(_master, 1, 250., 10.) );
    EXPECT_EQ( 260., ownKeyTime(_slave, 1) );
    _master->setValueAtTime(100., 1., ViewSpec::all(), 1);
    EXPECT_EQ( 2, ownKeyCount(_slave, 1) );
    static_cast<KnobI*>( _master.get() )->deleteValueAtTime(eCurveChangeReasonInternal, 100., ViewSpec::all(), 1, true);
    EXPECT_EQ( 1, ownKeyCount(_slave, 1) );

    // Dimension 0 is not linked anymore: it keeps the copy made when it was unlinked.
    EXPECT_TRUE( moveKey(_master, 0, 250., 20.) );
    EXPECT_EQ( 1, ownKeyCount(_slave, 0) );
    EXPECT_EQ( 250., ownKeyTime(_slave, 0) );

    // Once the last dimension is unlinked, the slave does not follow the master anymore.
    _slave->unSlave(1, true);
    _master->setValueAtTime(50., 1., ViewSpec::all(), 1);
    EXPECT_EQ( 1, ownKeyCount(_slave, 1) );
    EXPECT_EQ( 260., ownKeyTime(_slave, 1) );
}

// After unlinking one of two dimensions linked to the same master, the master still lists the knob as a listener
// of the other dimension: value changes are still propagated, and deleting the master still unlinks it.
TEST_F(KnobSlaveTest, UnlinkOneDimensionKeepsTheOtherListening)
{
    KnobDoublePtr master = AppManager::createKnob<KnobDouble>(_masterNode->getEffectInstance().get(), std::string("offset"), 2);
    master->setName("offset");

    createSlave(2);
    ASSERT_TRUE( _slave->slaveTo(0, master, 0) );
    ASSERT_TRUE( _slave->slaveTo(1, master, 1) );
    _slave->unSlave(0, true);

    KnobI::ListenerDimsMap listeners;
    master->getListeners(listeners);
    KnobI::ListenerDimsMap::iterator found = listeners.find(_slave);
    ASSERT_TRUE( found != listeners.end() );
    ASSERT_EQ( 2, (int)found->second.size() );
    EXPECT_FALSE( found->second[0].isListening );
    EXPECT_TRUE( found->second[1].isListening );

    // A non-animated value change of the master reaches the still linked dimension 1, not dimension 0.
    master->setValue(5., ViewSpec::all(), 1);
    EXPECT_EQ( 5., _slave->getRawValue(1) );
    master->setValue(7., ViewSpec::all(), 0);
    EXPECT_EQ( 0., _slave->getRawValue(0) );

    // Deleting the master unlinks dimension 1.
    _masterNode->getEffectInstance()->deleteKnob(master.get(), true);
    EXPECT_FALSE( _slave->isSlave(1) );
    EXPECT_FALSE( bool(_slave->getMaster(1).second) );
    EXPECT_TRUE( _slave->isEnabled(1) );
}

// The "ignore master persistence" flag (used by the tracker's multi-instance links) applies to the whole knob:
// a partial unlink must not make the remaining link persistent.
TEST_F(KnobSlaveTest, PartialUnlinkKeepsIgnoreMasterPersistence)
{
    createSlave(2);
    ASSERT_TRUE( _slave->slaveTo(0, _master, 0, true /*ignoreMasterPersistence*/) );
    ASSERT_TRUE( _slave->slaveTo(1, _master, 1, true /*ignoreMasterPersistence*/) );
    EXPECT_TRUE( _slave->isMastersPersistenceIgnored() );

    _slave->unSlave(0, true);
    EXPECT_TRUE( _slave->isMastersPersistenceIgnored() );

    _slave->unSlave(1, true);
    EXPECT_FALSE( _slave->isMastersPersistenceIgnored() );
}
