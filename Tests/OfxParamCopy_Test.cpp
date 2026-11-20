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

// paramCopy (OfxParameterSuiteV1, OFX 1.5.1 ofxParam.h) on the parameters of every type of the test
// plug-in net.natron.test.Params (Tests/OfxTestPlugin): "This copies the value of paramFrom to paramTo,
// including any animation it may have", with the time offset dstOffset, and only the keys in frameRange
// if it is not NULL ("To choose all animation in paramFrom set frameRange to [0, 0]").

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <set>
#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const char* const kParamsPrefixes[] = {
    "int", "double", "bool", "choice", "rgb", "rgba", "double2D", "int2D", "double3D", "int3D", "string", "custom",
    "parametric",
};

class OfxParamCopyTest
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

    KnobIPtr knob(const std::string& name) const
    {
        return _node->getKnobByName(name);
    }

    // paramCopy(paramTo, paramFrom, dstOffset, frameRange), as the plug-in calls it
    OfxStatus paramCopy(const std::string& to,
                        const std::string& from,
                        OfxTime dstOffset,
                        const OfxRangeD* frameRange) const
    {
        OFX::Host::Param::Instance* toParam = NatronTestPlugin::ofxParam(_node, to);
        OFX::Host::Param::Instance* fromParam = NatronTestPlugin::ofxParam(_node, from);

        EXPECT_TRUE(toParam && fromParam) << to << ' ' << from;
        if (!toParam || !fromParam) {
            return kOfxStatErrBadHandle;
        }

        return NatronTestPlugin::parameterSuite()->paramCopy(toParam->getHandle(), fromParam->getHandle(), dstOffset, frameRange);
    }
};

// A value of dimension dim of knob k: v is 0, 1 or 2 (0 for the "To" parameters before the copy).
void
setValue(const KnobIPtr& k,
         int dim,
         int v,
         double time,
         bool animate)
{
    static const char* const strings[] = { "zero", "one", "two" };
    Knob<int>* isInt = dynamic_cast<Knob<int>*>( k.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( k.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( k.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( k.get() );
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>( k.get() );

    if (isParametric) {
        // the curve has v control points
        ASSERT_EQ( eStatusOK, isParametric->deleteAllControlPoints(eValueChangedReasonNatronInternalEdited, dim) );
        for (int i = 0; i < v; ++i) {
            ASSERT_EQ( eStatusOK, isParametric->addControlPoint(eValueChangedReasonNatronInternalEdited, dim, 0.5 * i, 0.25 * i) );
        }
    } else if (isInt) {
        if (animate) {
            isInt->setValueAtTime(time, v + dim, ViewSpec::all(), dim);
        } else {
            isInt->setValue(v + dim, ViewSpec::all(), dim);
        }
    } else if (isDouble) {
        if (animate) {
            isDouble->setValueAtTime(time, v + 0.5 + dim, ViewSpec::all(), dim);
        } else {
            isDouble->setValue(v + 0.5 + dim, ViewSpec::all(), dim);
        }
    } else if (isBool) {
        if (animate) {
            isBool->setValueAtTime(time, v == 1, ViewSpec::all(), dim);
        } else {
            isBool->setValue(v == 1, ViewSpec::all(), dim);
        }
    } else if (isString) {
        if (animate) {
            isString->setValueAtTime(time, strings[v], ViewSpec::all(), dim);
        } else {
            isString->setValue(strings[v], ViewSpec::all(), dim);
        }
    } else {
        FAIL() << k->getName() << ": unexpected knob type " << k->typeName();
    }
}

bool
canAnimate(const KnobIPtr& k)
{
    return k->canAnimate() && k->isAnimationEnabled() && !dynamic_cast<KnobParametric*>( k.get() );
}

// the times of the keyframes of dimension dim
std::set<double>
keyTimes(const KnobIPtr& k,
         int dim)
{
    std::set<double> times;
    CurvePtr curve = k->getCurve(ViewIdx(0), dim);

    if (curve) {
        KeyFrameSet keys = curve->getKeyFrames_mt_safe();
        for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            times.insert( it->getTime() );
        }
    }

    return times;
}

// the value of dimension dim at time, as a string
std::string
valueAt(const KnobIPtr& k,
        int dim,
        double time)
{
    Knob<int>* isInt = dynamic_cast<Knob<int>*>( k.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( k.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( k.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( k.get() );
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>( k.get() );

    if (isParametric) {
        int n = 0;
        if (isParametric->getNControlPoints(dim, &n) != eStatusOK) {
            return "error";
        }
        std::string ret;
        for (int i = 0; i < n; ++i) {
            double key, value;
            if (isParametric->getNthControlPoint(dim, i, &key, &value) != eStatusOK) {
                return "error";
            }
            ret += std::to_string(key) + ':' + std::to_string(value) + ' ';
        }

        return ret;
    }
    if (isInt) {
        return std::to_string( isInt->getValueAtTime(time, dim) );
    }
    if (isDouble) {
        return std::to_string( isDouble->getValueAtTime(time, dim) );
    }
    if (isBool) {
        return isBool->getValueAtTime(time, dim) ? "true" : "false";
    }
    if (isString) {
        return isString->getValueAtTime(time, dim);
    }

    return "?";
}

// "<prefix>From": value 1 at time 1 and 2 at time 5 (or 1 if it can't animate); "<prefix>To": value 0
void
setFromAndTo(const KnobIPtr& from,
             const KnobIPtr& to)
{
    for (int dim = 0; dim < from->getDimension(); ++dim) {
        if ( canAnimate(from) ) {
            setValue(from, dim, 1, 1., true);
            setValue(from, dim, 2, 5., true);
        } else {
            setValue(from, dim, 1, 1., false);
        }
        setValue(to, dim, 0, 1., false);
    }
}
} // anonymous namespace

// paramCopy(to, from, 0, NULL) copies the values and the keyframes, for all the parameter types
// (it copied nothing for 10 of the 13 types).
TEST_F(OfxParamCopyTest, AllTypes)
{
    if ( skipped() ) {
        return;
    }
    for (std::size_t i = 0; i < sizeof(kParamsPrefixes) / sizeof(kParamsPrefixes[0]); ++i) {
        const std::string prefix = kParamsPrefixes[i];
        KnobIPtr from = knob(prefix + "From");
        KnobIPtr to = knob(prefix + "To");
        ASSERT_TRUE(from && to) << prefix;
        setFromAndTo(from, to);
        ASSERT_NE( valueAt(from, 0, 3.), valueAt(to, 0, 3.) ) << prefix;

        EXPECT_EQ( kOfxStatOK, paramCopy(prefix + "To", prefix + "From", 0., NULL) ) << prefix;
        for (int dim = 0; dim < from->getDimension(); ++dim) {
            EXPECT_EQ( keyTimes(from, dim), keyTimes(to, dim) ) << prefix << " dimension " << dim;
            for (double t = 0.; t <= 6.; t += 1.) {
                EXPECT_EQ( valueAt(from, dim, t), valueAt(to, dim, t) ) << prefix << " dimension " << dim << " time " << t;
            }
        }
    }
}

// dstOffset moves the keyframes, also without a frame range, and it is not rounded to an integer.
TEST_F(OfxParamCopyTest, Offset)
{
    if ( skipped() ) {
        return;
    }
    const char* const prefixes[] = { "double", "int", "double2D", "string", "custom" };
    for (std::size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
        const std::string prefix = prefixes[i];
        KnobIPtr from = knob(prefix + "From");
        KnobIPtr to = knob(prefix + "To");
        ASSERT_TRUE(from && to) << prefix;
        setFromAndTo(from, to);

        EXPECT_EQ( kOfxStatOK, paramCopy(prefix + "To", prefix + "From", 10.5, NULL) ) << prefix;
        std::set<double> expected;
        expected.insert(11.5);
        expected.insert(15.5);
        for (int dim = 0; dim < from->getDimension(); ++dim) {
            EXPECT_EQ( expected, keyTimes(to, dim) ) << prefix << " dimension " << dim;
            EXPECT_EQ( valueAt(from, dim, 1.), valueAt(to, dim, 11.5) ) << prefix << " dimension " << dim;
            EXPECT_EQ( valueAt(from, dim, 5.), valueAt(to, dim, 15.5) ) << prefix << " dimension " << dim;
        }
    }
}

// The frame range [0, 0] means all the keyframes; another range copies only the keyframes in it.
TEST_F(OfxParamCopyTest, FrameRange)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = std::dynamic_pointer_cast<KnobDouble>( knob("doubleFrom") );
    KnobDoublePtr to = std::dynamic_pointer_cast<KnobDouble>( knob("doubleTo") );
    ASSERT_TRUE(from && to);
    from->setValueAtTime(0., 10., ViewSpec::all(), 0);
    from->setValueAtTime(1., 11., ViewSpec::all(), 0);
    from->setValueAtTime(5., 15., ViewSpec::all(), 0);
    from->setValueAtTime(7., 17., ViewSpec::all(), 0);

    const OfxRangeD all = { 0., 0. };
    EXPECT_EQ( kOfxStatOK, paramCopy("doubleTo", "doubleFrom", 0., &all) );
    EXPECT_EQ( keyTimes(from, 0), keyTimes(to, 0) );

    const OfxRangeD range = { 2., 6. };
    EXPECT_EQ( kOfxStatOK, paramCopy("doubleTo", "doubleFrom", 2., &range) );
    std::set<double> expected;
    expected.insert(7.);
    EXPECT_EQ( expected, keyTimes(to, 0) );
    EXPECT_EQ( 15., to->getValueAtTime(7., 0) );
}

// Copying between parameters of different types fails, and changes nothing.
TEST_F(OfxParamCopyTest, DifferentTypes)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr double2D = std::dynamic_pointer_cast<KnobDouble>( knob("double2DTo") );
    ASSERT_TRUE( bool(double2D) );
    double2D->setValue(1., ViewSpec::all(), 0);
    std::dynamic_pointer_cast<KnobDouble>( knob("double3DFrom") )->setValue(3., ViewSpec::all(), 0);
    std::dynamic_pointer_cast<KnobDouble>( knob("doubleFrom") )->setValue(4., ViewSpec::all(), 0);

    EXPECT_EQ( kOfxStatErrBadHandle, paramCopy("double2DTo", "double3DFrom", 0., NULL) );
    EXPECT_EQ( kOfxStatErrBadHandle, paramCopy("double2DTo", "doubleFrom", 0., NULL) );
    EXPECT_EQ( kOfxStatErrBadHandle, paramCopy("double2DTo", "intFrom", 0., NULL) );
    EXPECT_EQ( 1., double2D->getValue(0) );
}
