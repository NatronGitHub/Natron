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

// KnobI::createDuplicateOnHolder() without an alias (the "Create link on group" and "Pick" menus of the
// GUI): the parameter gets an expression that reads the new parameter. It must read it at the frame
// being computed, else a link to an animated parameter is not animated (#1336).

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
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
class KnobDuplicateTest
    : public ::testing::Test
{
protected:
    NodePtr _from;
    NodePtr _to;

    virtual void SetUp() OVERRIDE
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
            return;
        }
        _from = createNode();
        _to = createNode();
    }

    virtual void TearDown() OVERRIDE
    {
        if (_from) {
            _from->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _from.reset();
        }
        if (_to) {
            _to->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _to.reset();
        }
    }

    static NodePtr createNode()
    {
        AppInstancePtr app = appPTR->getTopLevelInstance();
        CreateNodeArgs args( kNatronTestPluginParams, app->getProject() );

        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);

        return app->createNode(args);
    }

    bool skipped()
    {
        if (!_from || !_to) {
            NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

            return true;
        }

        return false;
    }

    // duplicate the parameter on _to as a link (an expression on the parameter of _from)
    KnobIPtr duplicate(const KnobIPtr& knob,
                       const std::string& name)
    {
        return knob->createDuplicateOnHolder(_to->getEffectInstance().get(), KnobPagePtr(), KnobGroupPtr(), -1,
                                             false /*makeAlias*/, name, name, std::string(),
                                             true /*refreshParams*/, true /*isUserKnob*/);
    }
};
} // anonymous namespace

TEST_F(KnobDuplicateTest, LinkIsAnimated)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = std::dynamic_pointer_cast<KnobDouble>( _from->getKnobByName("doubleFrom") );
    ASSERT_TRUE( bool(from) );
    from->setValueAtTime(1., 1., ViewSpec::all(), 0);
    from->setValueAtTime(11., 11., ViewSpec::all(), 0);

    KnobDoublePtr link = std::dynamic_pointer_cast<KnobDouble>( duplicate(from, "linkedDouble") );
    ASSERT_TRUE( bool(link) );
    // the new parameter got the animation, and the original one an expression that reads it
    EXPECT_EQ( 6., link->getValueAtTime(6.) );
    ASSERT_FALSE( from->getExpression(0).empty() );
    EXPECT_NE( std::string::npos, from->getExpression(0).find(".get(frame)") ) << from->getExpression(0);
    std::string error;
    EXPECT_TRUE( from->isExpressionValid(0, &error) ) << error;

    // the link follows the animation of the new parameter at every frame, not its value at the current time
    EXPECT_EQ( 3., from->getValueAtTime(3.) );
    EXPECT_EQ( 6., from->getValueAtTime(6.) );
    link->setValueAtTime(6., 100., ViewSpec::all(), 0);
    EXPECT_EQ( 100., from->getValueAtTime(6.) );
    EXPECT_EQ( 1., from->getValueAtTime(1.) );

    from->clearExpression(0, true);
}

// A parameter with several dimensions: each dimension reads its own dimension, at the frame being computed
TEST_F(KnobDuplicateTest, LinkIsAnimatedForEachDimension)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = std::dynamic_pointer_cast<KnobDouble>( _from->getKnobByName("double2DFrom") );
    ASSERT_TRUE( bool(from) );
    ASSERT_EQ( 2, from->getDimension() );
    from->setValueAtTime(1., 1., ViewSpec::all(), 0);
    from->setValueAtTime(11., 11., ViewSpec::all(), 0);
    from->setValueAtTime(1., -1., ViewSpec::all(), 1);
    from->setValueAtTime(11., -11., ViewSpec::all(), 1);

    KnobDoublePtr link = std::dynamic_pointer_cast<KnobDouble>( duplicate(from, "linkedDouble2D") );
    ASSERT_TRUE( bool(link) );
    for (int i = 0; i < 2; ++i) {
        ASSERT_FALSE( from->getExpression(i).empty() );
        EXPECT_NE( std::string::npos, from->getExpression(i).find(".get(frame)[dimension]") ) << from->getExpression(i);
    }
    EXPECT_EQ( 4., from->getValueAtTime(4., 0) );
    EXPECT_EQ( -4., from->getValueAtTime(4., 1) );

    from->clearExpression(0, true);
    from->clearExpression(1, true);
}
