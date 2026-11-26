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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <functional>
#include <list>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"

#include "TestUtils.h"

NATRON_NAMESPACE_USING

// getDerivativeAtTime(), getIntegrateFromTimeToTime() and the keyframe edits (deleteValue(s)AtTime(),
// moveDerivative(s)AtTime()) accepted dimension == getDimension() (an off-by-one in their range check)
// and then read the curve of a dimension that doesn't exist (#1743).
// They throw std::invalid_argument for it, as for a negative dimension.
TEST(KnobDimensionRange, DerivativeAndIntegral)
{
    // Dot is a built-in node: no OpenFX plug-in is needed.
    NatronTest::NodeScope nodes;
    NodePtr node = nodes.create(PLUGINID_NATRON_DOT);
    ASSERT_TRUE( bool(node) );

    {
        KnobDoublePtr knob = AppManager::createKnob<KnobDouble>(node->getEffectInstance().get(), std::string("size"), 2);
        knob->setName("size");
        knob->setValueAtTime(0., 0., ViewSpec::all(), 1);
        knob->setValueAtTime(10., 10., ViewSpec::all(), 1);

        // the last dimension is in range
        EXPECT_NO_THROW( ignore_result( knob->getDerivativeAtTime( 5., ViewSpec::current(), 1 ) ) );
        EXPECT_NO_THROW( ignore_result( knob->getIntegrateFromTimeToTime( 0., 10., ViewSpec::current(), 1 ) ) );
        EXPECT_THROW( ignore_result( knob->getDerivativeAtTime( 5., ViewSpec::current(), 2 ) ), std::invalid_argument );
        EXPECT_THROW( ignore_result( knob->getIntegrateFromTimeToTime( 0., 10., ViewSpec::current(), 2 ) ), std::invalid_argument );
        EXPECT_THROW( ignore_result( knob->getDerivativeAtTime( 5., ViewSpec::current(), -1 ) ), std::invalid_argument );
        EXPECT_THROW( ignore_result( knob->getIntegrateFromTimeToTime( 0., 10., ViewSpec::current(), -1 ) ), std::invalid_argument );
        // the same off-by-one in the keyframe edits
        // (KnobHelper's override is private: call it through KnobI, as KnobSlave_Test.cpp does)
        EXPECT_THROW( static_cast<KnobI*>( knob.get() )->deleteValueAtTime( eCurveChangeReasonInternal, 10., ViewSpec::all(), 2, false ), std::invalid_argument );
        EXPECT_EQ( 2, knob->getKeyFramesCount( ViewIdx(0), 1 ) );
    }
}

// From Python, a dimension out of range gives 0, as getValue(dimension) does (a C++ exception
// can't go through the Python bindings).
TEST(KnobDimensionRange, PythonDerivativeAndIntegral)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    const std::string a = app->getAppIDString();
    const std::string script =
        "g = " + a + ".createNode('fr.inria.built-in.Group')\n"
        "try:\n"
        "    d = g.createDouble2DParam('natronTestRange', 'Range')\n"
        "    values = (d.getDerivativeAtTime(5, 2), d.getDerivativeAtTime(5, -1),\n"
        "              d.getIntegrateFromTimeToTime(0, 10, 2), d.getIntegrateFromTimeToTime(0, 10, -1))\n"
        "    del d\n"
        "finally:\n"
        "    g.destroy()\n"
        "    del g\n"
        "assert values == (0., 0., 0., 0.), values\n"
        "del values\n";
    std::string error, output;
    EXPECT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, &output) ) << error << output;
}

// The message of an std::invalid_argument thrown by f, empty if it doesn't throw
static std::string
invalidArgumentMessage(const std::function<void()>& f)
{
    try {
        f();
    } catch (const std::invalid_argument& e) {
        return std::string( e.what() );
    }

    return std::string();
}

// All the dimension checks are KnobHelper::checkDimension(): getAnimationLevel() read the master of the
// dimension before checking it, and the messages named other functions.
TEST(KnobDimensionRange, CheckedDimensionAndMessage)
{
    NatronTest::NodeScope nodes;
    NodePtr node = nodes.create(PLUGINID_NATRON_DOT);
    ASSERT_TRUE( bool(node) );

    {
        KnobDoublePtr knob = AppManager::createKnob<KnobDouble>(node->getEffectInstance().get(), std::string("size"), 2);
        knob->setName("size");
        KnobI* knobI = knob.get();

        // the lambdas return the WARN_UNUSED_RETURN results, which std::function<void()> discards
        EXPECT_EQ( std::string(), invalidArgumentMessage([knobI] { return knobI->getAnimationLevel(1); }) );
        EXPECT_EQ( std::string("KnobHelper::getAnimationLevel(): Dimension out of range"),
                   invalidArgumentMessage([knobI] { return knobI->getAnimationLevel(2); }) );
        EXPECT_EQ( std::string("KnobHelper::getAnimationLevel(): Dimension out of range"),
                   invalidArgumentMessage([knobI] { return knobI->getAnimationLevel(-1); }) );
        EXPECT_EQ( std::string("KnobHelper::moveDerivativeAtTime(): Dimension out of range"),
                   invalidArgumentMessage([knobI] { return knobI->moveDerivativeAtTime(eCurveChangeReasonInternal, ViewSpec::all(), 2, 10., 1., true); }) );
        EXPECT_EQ( std::string("KnobHelper::deleteValuesAtTime(): Dimension out of range"),
                   invalidArgumentMessage([knobI] { knobI->deleteValuesAtTime(eCurveChangeReasonInternal, std::list<double>(1, 10.), ViewSpec::all(), 2, false); }) );
    }

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
