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

// Regression tests for https://github.com/NatronGitHub/Natron/issues/1290
// ("Animated string params: keyframe values shift when other keys are added, deleted or moved").
//
// The value of a string keyframe used to be the index of the keyframe in the sorted list of strings, computed
// when the keyframe was set: adding, removing or moving another keyframe changed the string read by the
// following keyframes.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format off
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(sign-compare)
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_ON(sign-compare)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
// clang-format on

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Curve.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

class KnobStringAnimationTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        AppInstancePtr app = appPTR->getTopLevelInstance();
        ASSERT_TRUE( bool(app) );
        // Dot is a built-in node: no OpenFX plug-in is needed.
        CreateNodeArgs args( PLUGINID_NATRON_DOT, app->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        _node = app->createNode(args);
        ASSERT_TRUE( bool(_node) );
        _knob = createStringKnob("str");
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        _knob.reset();
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        _node.reset();
    }

    KnobStringPtr createStringKnob(const std::string& name)
    {
        KnobStringPtr knob = AppManager::createKnob<KnobString>(_node->getEffectInstance().get(), name, 1);
        knob->setName(name);
        EXPECT_TRUE( knob->isAnimationEnabled() );

        return knob;
    }

    static KnobHelper::ValueChangedReturnCodeEnum setKey(const KnobStringPtr& knob,
                                                         double time,
                                                         const std::string& v)
    {
        KeyFrame k;

        return knob->setValueAtTime(time, v, ViewSpec::all(), 0, eValueChangedReasonNatronInternalEdited, &k);
    }

    static std::string at(const KnobStringPtr& knob,
                          double time)
    {
        return knob->getValueAtTime(time, 0, ViewIdx(0), false /*clamp*/, true /*byPassMaster*/);
    }

    static int keyCount(const KnobStringPtr& knob)
    {
        return knob->getCurve(ViewIdx(0), 0, true /*byPassMaster*/)->getKeyFramesCount();
    }

    static void deleteKey(const KnobStringPtr& knob,
                          double time)
    {
        static_cast<KnobI*>( knob.get() )->deleteValueAtTime(eCurveChangeReasonInternal, time, ViewSpec::all(), 0, false);
    }

    NodePtr _node;
    KnobStringPtr _knob;
};

// First repro of the issue: keys a@0 and c@10, then insert b@5: t=10 read "b".
TEST_F(KnobStringAnimationTest, InsertKeyBetweenKeys)
{
    setKey(_knob, 0., "a");
    setKey(_knob, 10., "c");
    EXPECT_EQ( KnobHelper::eValueChangedReturnCodeKeyframeAdded, setKey(_knob, 5., "b") );
    ASSERT_EQ( 3, keyCount(_knob) );
    EXPECT_EQ( "a", at(_knob, -5.) );
    EXPECT_EQ( "a", at(_knob, 0.) );
    EXPECT_EQ( "a", at(_knob, 4.) );
    EXPECT_EQ( "b", at(_knob, 5.) );
    EXPECT_EQ( "b", at(_knob, 7.) );
    EXPECT_EQ( "c", at(_knob, 10.) );
    EXPECT_EQ( "c", at(_knob, 20.) );

    // a key before all the others
    setKey(_knob, -10., "z");
    EXPECT_EQ( "z", at(_knob, -10.) );
    EXPECT_EQ( "a", at(_knob, 0.) );
    EXPECT_EQ( "b", at(_knob, 5.) );
    EXPECT_EQ( "c", at(_knob, 10.) );

    // keyframe values by index
    bool ok = false;
    EXPECT_EQ( "z", _knob->getKeyFrameValueByIndex(ViewIdx(0), 0, 0, &ok) );
    EXPECT_TRUE(ok);
    EXPECT_EQ( "b", _knob->getKeyFrameValueByIndex(ViewIdx(0), 0, 2, &ok) );
    EXPECT_TRUE(ok);
    EXPECT_EQ( "c", _knob->getKeyFrameValueByIndex(ViewIdx(0), 0, 3, &ok) );
    EXPECT_TRUE(ok);
}

// Second repro of the issue: keys a@0, b@5, c@10, d@15, then delete the key at 5: t=10 read "d".
TEST_F(KnobStringAnimationTest, DeleteKey)
{
    setKey(_knob, 0., "a");
    setKey(_knob, 5., "b");
    setKey(_knob, 10., "c");
    setKey(_knob, 15., "d");
    deleteKey(_knob, 5.);
    ASSERT_EQ( 3, keyCount(_knob) );
    EXPECT_EQ( "a", at(_knob, 0.) );
    EXPECT_EQ( "a", at(_knob, 5.) );
    EXPECT_EQ( "c", at(_knob, 10.) );
    EXPECT_EQ( "d", at(_knob, 15.) );

    deleteKey(_knob, 0.);
    EXPECT_EQ( "c", at(_knob, 0.) );
    EXPECT_EQ( "c", at(_knob, 10.) );
    EXPECT_EQ( "d", at(_knob, 15.) );
}

// Third repro of the issue: editing at t=5 on {0:x2, 10:x4} gave {0:x2, 5:x3, 10:x3}.
TEST_F(KnobStringAnimationTest, ColorspaceMenuEdit)
{
    setKey(_knob, 0., "x2");
    setKey(_knob, 10., "x4");
    setKey(_knob, 5., "x3");
    EXPECT_EQ( "x2", at(_knob, 0.) );
    EXPECT_EQ( "x3", at(_knob, 5.) );
    EXPECT_EQ( "x4", at(_knob, 10.) );
}

// Re-setting an existing key with a different string is a modification: it was reported as "nothing changed"
// when the index of the key did not change.
TEST_F(KnobStringAnimationTest, OverwriteKey)
{
    {
        KnobStringPtr single = createStringKnob("single");
        EXPECT_EQ( KnobHelper::eValueChangedReturnCodeKeyframeAdded, setKey(single, 0., "a") );
        EXPECT_EQ( KnobHelper::eValueChangedReturnCodeKeyframeModified, setKey(single, 0., "b") );
        EXPECT_EQ( "b", at(single, 0.) );
        EXPECT_EQ( KnobHelper::eValueChangedReturnCodeNothingChanged, setKey(single, 0., "b") );
    }

    EXPECT_EQ( KnobHelper::eValueChangedReturnCodeKeyframeAdded, setKey(_knob, 0., "a") );
    EXPECT_EQ( KnobHelper::eValueChangedReturnCodeKeyframeAdded, setKey(_knob, 10., "b") );

    EXPECT_EQ( KnobHelper::eValueChangedReturnCodeKeyframeModified, setKey(_knob, 10., "x") );
    EXPECT_EQ( "x", at(_knob, 10.) );
    EXPECT_EQ( "a", at(_knob, 0.) );
    EXPECT_EQ( KnobHelper::eValueChangedReturnCodeKeyframeModified, setKey(_knob, 0., "y") );
    EXPECT_EQ( "y", at(_knob, 0.) );
    EXPECT_EQ( "x", at(_knob, 10.) );

    // the same string at the same time
    EXPECT_NE( KnobHelper::eValueChangedReturnCodeKeyframeAdded, setKey(_knob, 10., "x") );
    EXPECT_EQ( 2, keyCount(_knob) );
    EXPECT_EQ( "x", at(_knob, 10.) );

    // Undo of a modified key (MultipleKnobEditsUndoCommand) sets the old value back: the key stays.
    EXPECT_EQ( KnobHelper::eValueChangedReturnCodeKeyframeModified, setKey(_knob, 10., "b") );
    EXPECT_EQ( 2, keyCount(_knob) );
    EXPECT_EQ( "b", at(_knob, 10.) );
}

// Undo/redo and copy/paste re-add keyframes with their value: a keyframe value must not depend on the other
// keyframes of the curve, nor on the knob.
TEST_F(KnobStringAnimationTest, ReAddKeyFrame)
{
    setKey(_knob, 0., "a");
    setKey(_knob, 5., "b");
    setKey(_knob, 10., "c");
    KeyFrame removed;
    ASSERT_TRUE( _knob->getCurve(ViewIdx(0), 0)->getKeyFrameWithTime(5., &removed) );
    deleteKey(_knob, 5.);
    setKey(_knob, 2., "d");
    EXPECT_EQ( "c", at(_knob, 10.) );

    // undo of the removal
    EXPECT_TRUE( _knob->setKeyFrame(removed, ViewSpec::all(), 0, eValueChangedReasonNatronInternalEdited) );
    EXPECT_EQ( "a", at(_knob, 0.) );
    EXPECT_EQ( "d", at(_knob, 2.) );
    EXPECT_EQ( "b", at(_knob, 5.) );
    EXPECT_EQ( "c", at(_knob, 10.) );

    // paste into another knob
    KnobStringPtr other = createStringKnob("other");
    setKey(other, 0., "o");
    EXPECT_TRUE( other->setKeyFrame(removed, ViewSpec::all(), 0, eValueChangedReasonNatronInternalEdited) );
    EXPECT_EQ( "o", at(other, 0.) );
    EXPECT_EQ( "b", at(other, 5.) );
}

// moveValueAtTimeInternal() passed the time where the index should go.
TEST_F(KnobStringAnimationTest, MoveKey)
{
    setKey(_knob, 0., "a");
    setKey(_knob, 5., "b");
    setKey(_knob, 10., "c");

    KeyFrame newKey;
    ASSERT_TRUE( _knob->moveValueAtTime(eCurveChangeReasonInternal, 5., ViewSpec::all(), 0, 3., 0., &newKey) );
    EXPECT_EQ( 8., newKey.getTime() );
    EXPECT_EQ( "a", at(_knob, 5.) );
    EXPECT_EQ( "b", at(_knob, 8.) );
    EXPECT_EQ( "c", at(_knob, 10.) );

    // move past another keyframe
    ASSERT_TRUE( _knob->moveValueAtTime(eCurveChangeReasonInternal, 8., ViewSpec::all(), 0, 7., 0., &newKey) );
    EXPECT_EQ( "a", at(_knob, 8.) );
    EXPECT_EQ( "c", at(_knob, 10.) );
    EXPECT_EQ( "b", at(_knob, 15.) );

    // move the first keyframe
    ASSERT_TRUE( _knob->moveValueAtTime(eCurveChangeReasonInternal, 0., ViewSpec::all(), 0, 12., 0., &newKey) );
    EXPECT_EQ( "c", at(_knob, 10.) );
    EXPECT_EQ( "a", at(_knob, 12.) );
    EXPECT_EQ( "b", at(_knob, 15.) );
}

// Transforming keyframes (e.g. in the dope sheet) moves them in time: the string must follow, and the
// Y part of the transform must not change it.
TEST_F(KnobStringAnimationTest, TransformKeys)
{
    setKey(_knob, 0., "a");
    setKey(_knob, 5., "b");
    setKey(_knob, 10., "c");

    std::vector<KeyFrame> keys;
    KeyFrameSet set = _knob->getCurve(ViewIdx(0), 0)->getKeyFrames_mt_safe();
    keys.assign( set.begin(), set.end() );
    // translate time by 3, scale value by 2
    Transform::Matrix3x3 m(1., 0., 3.,
                           0., 2., 0.,
                           0., 0., 1.);
    ASSERT_TRUE( _knob->transformValuesAtTime(eCurveChangeReasonInternal, ViewSpec::all(), 0, m, &keys) );
    ASSERT_EQ( 3, keyCount(_knob) );
    EXPECT_EQ( "a", at(_knob, 3.) );
    EXPECT_EQ( "a", at(_knob, 5.) );
    EXPECT_EQ( "b", at(_knob, 8.) );
    EXPECT_EQ( "b", at(_knob, 10.) );
    EXPECT_EQ( "c", at(_knob, 13.) );
}

TEST_F(KnobStringAnimationTest, CloneAndLink)
{
    setKey(_knob, 0., "a");
    setKey(_knob, 5., "b");
    setKey(_knob, 10., "c");

    KnobStringPtr other = createStringKnob("other");
    setKey(other, 3., "o");
    static_cast<KnobI*>( other.get() )->clone( _knob.get() );
    EXPECT_EQ( 3, keyCount(other) );
    EXPECT_EQ( "a", at(other, 3.) );
    EXPECT_EQ( "b", at(other, 5.) );
    EXPECT_EQ( "c", at(other, 10.) );

    // copy with an offset, as OFX paramCopy does
    KnobStringPtr shifted = createStringKnob("shifted");
    RangeD range = {5., 10.};
    static_cast<KnobI*>( shifted.get() )->clone(_knob.get(), 100., &range);
    EXPECT_EQ( 2, keyCount(shifted) );
    EXPECT_EQ( "b", at(shifted, 105.) );
    EXPECT_EQ( "c", at(shifted, 110.) );

    // editing the source does not change the copy
    setKey(_knob, 7., "d");
    deleteKey(_knob, 5.);
    EXPECT_EQ( "b", at(other, 5.) );
    EXPECT_EQ( "b", at(other, 7.) );
    EXPECT_EQ( "c", at(other, 10.) );

    // a linked knob reads the keyframes of its master
    KnobStringPtr slave = createStringKnob("slave");
    ASSERT_TRUE( slave->slaveTo(0, _knob, 0) );
    EXPECT_EQ( "a", slave->getValueAtTime(5., 0) );
    EXPECT_EQ( "d", slave->getValueAtTime(7., 0) );
    setKey(_knob, 3., "e");
    EXPECT_EQ( "e", slave->getValueAtTime(5., 0) );
    EXPECT_EQ( "c", slave->getValueAtTime(10., 0) );
    slave->unSlave(0, true /*copyState*/);
    EXPECT_EQ( "e", at(slave, 5.) );
    EXPECT_EQ( "d", at(slave, 7.) );
    EXPECT_EQ( "c", at(slave, 10.) );
}

// Save the knob as in a project and load it in a new knob.
static KnobIPtr
roundTrip(const KnobIPtr& knob)
{
    std::stringstream ss;
    {
        KnobSerialization s(knob);
        boost::archive::xml_oarchive oArchive(ss);
        oArchive << boost::serialization::make_nvp("Knob", s);
    }
    KnobSerialization loaded;
    {
        boost::archive::xml_iarchive iArchive(ss);
        iArchive >> boost::serialization::make_nvp("Knob", loaded);
    }

    return loaded.getKnob();
}

TEST_F(KnobStringAnimationTest, SerializationRoundTrip)
{
    setKey(_knob, 0., "a");
    setKey(_knob, 10., "c");
    setKey(_knob, 5., "b");
    setKey(_knob, 20., "a");

    // Projects store the index of each keyframe in the curve, which older versions of Natron read.
    {
        Curve c;
        _knob->getCurveForSerialization(0, &c);
        KeyFrameSet keys = c.getKeyFrames_mt_safe();
        ASSERT_EQ( 4U, keys.size() );
        int i = 0;
        for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it, ++i) {
            EXPECT_EQ( (double)i, it->getValue() );
        }
        std::map<int, std::string> strings;
        _knob->saveAnimation(&strings);
        std::map<int, std::string> expected;
        expected[0] = "a";
        expected[5] = "b";
        expected[10] = "c";
        expected[20] = "a";
        EXPECT_EQ(expected, strings);
    }

    KnobStringPtr loaded = std::dynamic_pointer_cast<KnobString>( roundTrip(_knob) );
    ASSERT_TRUE( bool(loaded) );
    EXPECT_EQ( 4, keyCount(loaded) );
    EXPECT_EQ( "a", at(loaded, 0.) );
    EXPECT_EQ( "b", at(loaded, 5.) );
    EXPECT_EQ( "c", at(loaded, 10.) );
    EXPECT_EQ( "c", at(loaded, 15.) );
    EXPECT_EQ( "a", at(loaded, 20.) );
}

// A project saved by an older version of Natron after inserting b@5 between a@0 and c@10: the curve values are the
// indices computed when each key was set (0, 1, 1), the strings are saved by time.
TEST_F(KnobStringAnimationTest, LoadOldProjectWithStaleIndices)
{
    KeyFrameSet keys;
    keys.insert( KeyFrame(0., 0.) );
    keys.insert( KeyFrame(5., 1.) );
    keys.insert( KeyFrame(10., 1.) );
    _knob->getCurve(ViewIdx(0), 0)->setKeyframes(keys, false);
    std::map<int, std::string> strings;
    strings[0] = "a";
    strings[5] = "b";
    strings[10] = "c";
    _knob->loadAnimation(strings);
    EXPECT_EQ( "a", at(_knob, 0.) );
    EXPECT_EQ( "b", at(_knob, 5.) );
    EXPECT_EQ( "c", at(_knob, 10.) );

    // keyframe times are saved as integers
    KnobStringPtr fractional = createStringKnob("fractional");
    keys.clear();
    keys.insert( KeyFrame(0., 0.) );
    keys.insert( KeyFrame(2.5, 1.) );
    fractional->getCurve(ViewIdx(0), 0)->setKeyframes(keys, false);
    strings.clear();
    strings[0] = "a";
    strings[2] = "b";
    fractional->loadAnimation(strings);
    EXPECT_EQ( "a", at(fractional, 2.) );
    EXPECT_EQ( "b", at(fractional, 2.5) );
    EXPECT_EQ( "b", at(fractional, 3.) );
}
