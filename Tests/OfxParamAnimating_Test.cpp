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

// kOfxParamPropIsAnimating ("Set by a host on a parameter instance to indicate if the parameter has a non-constant
// value set on it. This can be as a consequence of animation or of scripting modifying the value, or of a parameter
// being connected to an expression in the host", OFX 1.5.1 ofxParam.h) and kOfxParamPropIsAutoKeying ("Will a value
// change on the parameter add automatic keyframes"), as the test plug-in net.natron.test.Params
// (Tests/OfxTestPlugin) reads them, without a GUI.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
class OfxParamAnimatingTest
    : public ::testing::Test
{
protected:
    NodePtr _node;

    virtual void SetUp() OVERRIDE
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
            return;
        }
        AppInstancePtr app = appPTR->getTopLevelInstance();
        CreateNodeArgs args( kNatronTestPluginParams, app->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        _node = app->createNode(args);
    }

    virtual void TearDown() OVERRIDE
    {
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _node.reset();
        }
    }

    bool skipped()
    {
        if (!_node) {
            NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

            return true;
        }

        return false;
    }

    template <typename KNOB>
    std::shared_ptr<KNOB> knob(const std::string& name) const
    {
        return std::dynamic_pointer_cast<KNOB>( _node->getKnobByName(name) );
    }

    // an int property of a parameter, read through the property suite, as the plug-in does (-1 on error)
    int property(const std::string& param,
                 const char* name) const
    {
        OFX::Host::Param::Instance* p = NatronTestPlugin::ofxParam(_node, param);
        OfxPropertySetHandle props = 0;
        int value = -1;

        if ( !p || (NatronTestPlugin::parameterSuite()->paramGetPropertySet(p->getHandle(), &props) != kOfxStatOK) ||
             (NatronTestPlugin::propertySuite()->propGetInt(props, name, 0, &value) != kOfxStatOK) ) {
            return -1;
        }

        return value;
    }

    int isAnimating(const std::string& param) const
    {
        return property(param, kOfxParamPropIsAnimating);
    }

    int isAutoKeying(const std::string& param) const
    {
        return property(param, kOfxParamPropIsAutoKeying);
    }
};
} // anonymous namespace

TEST_F(OfxParamAnimatingTest, Keyframes)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr d = knob<KnobDouble>("doubleFrom");
    ASSERT_TRUE( bool(d) );
    EXPECT_EQ( 0, isAnimating("doubleFrom") );
    EXPECT_EQ( 0, isAutoKeying("doubleFrom") );

    d->setValueAtTime(1., 1., ViewSpec::all(), 0);
    d->setValueAtTime(5., 2., ViewSpec::all(), 0);
    EXPECT_EQ( 1, isAnimating("doubleFrom") );
    EXPECT_EQ( 1, isAutoKeying("doubleFrom") );

    d->removeAnimation(ViewSpec::all(), 0);
    EXPECT_EQ( 0, isAnimating("doubleFrom") );
    EXPECT_EQ( 0, isAutoKeying("doubleFrom") );
}

// all the dimensions count, not only the first one
TEST_F(OfxParamAnimatingTest, OtherDimensions)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr d2 = knob<KnobDouble>("double2DFrom");
    KnobIntPtr i3 = knob<KnobInt>("int3DFrom");
    KnobColorPtr rgba = knob<KnobColor>("rgbaFrom");
    ASSERT_TRUE(d2 && i3 && rgba);

    d2->setValueAtTime(1., 1., ViewSpec::all(), 1);
    d2->setValueAtTime(5., 2., ViewSpec::all(), 1);
    i3->setValueAtTime(1., 1, ViewSpec::all(), 2);
    i3->setValueAtTime(5., 2, ViewSpec::all(), 2);
    rgba->setValueAtTime(1., 0.1, ViewSpec::all(), 3);
    rgba->setValueAtTime(5., 0.2, ViewSpec::all(), 3);
    EXPECT_EQ( 1, isAnimating("double2DFrom") );
    EXPECT_EQ( 1, isAutoKeying("double2DFrom") );
    EXPECT_EQ( 1, isAnimating("int3DFrom") );
    EXPECT_EQ( 1, isAutoKeying("int3DFrom") );
    EXPECT_EQ( 1, isAnimating("rgbaFrom") );
    EXPECT_EQ( 1, isAutoKeying("rgbaFrom") );
    EXPECT_EQ( 0, isAnimating("double2DTo") );
}

// an expression makes the value non-constant, but a value change doesn't add a keyframe
TEST_F(OfxParamAnimatingTest, Expression)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr d = knob<KnobDouble>("doubleFrom");
    ASSERT_TRUE( bool(d) );
    d->setExpression(0, "frame", false, false);
    ASSERT_FALSE( d->getExpression(0).empty() );
    EXPECT_EQ( 1, isAnimating("doubleFrom") );
    EXPECT_EQ( 0, isAutoKeying("doubleFrom") );
}

// a parameter linked to an animated parameter is animating
TEST_F(OfxParamAnimatingTest, Link)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = knob<KnobDouble>("doubleFrom");
    KnobDoublePtr to = knob<KnobDouble>("doubleTo");
    ASSERT_TRUE(from && to);
    ASSERT_TRUE( to->slaveTo(0, from, 0) );
    EXPECT_EQ( 0, isAnimating("doubleTo") );
    from->setValueAtTime(1., 1., ViewSpec::all(), 0);
    from->setValueAtTime(5., 2., ViewSpec::all(), 0);
    EXPECT_EQ( 1, isAnimating("doubleTo") );
    // the value of a linked dimension can't be changed
    EXPECT_EQ( 0, isAutoKeying("doubleTo") );
    to->unSlave(0, false);
}

// the parameters without animation have the properties too
TEST_F(OfxParamAnimatingTest, NonAnimatedTypes)
{
    if ( skipped() ) {
        return;
    }
    EXPECT_EQ( 0, isAnimating("stringTo") );
    EXPECT_EQ( 0, isAnimating("boolTo") );
    EXPECT_EQ( 0, isAnimating("parametricTo") );
}
