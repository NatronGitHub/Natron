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

// The range of an OFX parameter that sets no limits (kOfxParamPropMin/Max, kOfxParamPropDisplayMin/Max),
// as the test plug-in net.natron.test.Params (Tests/OfxTestPlugin) defines them. The OFX defaults are
// -DBL_MAX and DBL_MAX for doubles (ofxParam.h), INT_MIN and INT_MAX for integers, and 0 to 1 for the
// display range of colours. Since openfx b193f839 (#218), HostSupport gives exactly -DBL_MAX and DBL_MAX:
// before, it gave -1.79769e+308 and 1.79769e+308 (6 significant digits), which Natron's knobs and curves
// did not see as unbounded (they compare with std::numeric_limits<double>::lowest() and max()).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <climits>
#include <limits>
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
class OfxParamRangeTest
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

    // a double property of a parameter instance, read through the property suite, as a plug-in does
    bool doubleProperty(const std::string& param,
                        const char* name,
                        int index,
                        double* value) const
    {
        OFX::Host::Param::Instance* p = NatronTestPlugin::ofxParam(_node, param);
        OfxPropertySetHandle props = 0;

        return p && (NatronTestPlugin::parameterSuite()->paramGetPropertySet(p->getHandle(), &props) == kOfxStatOK) &&
               (NatronTestPlugin::propertySuite()->propGetDouble(props, name, index, value) == kOfxStatOK);
    }
};
} // anonymous namespace

// A double parameter without limits: the OFX properties are exactly -DBL_MAX and DBL_MAX, and so are the
// knob's range and display range, so its curve is unbounded.
TEST_F(OfxParamRangeTest, DoubleUnbounded)
{
    if ( skipped() ) {
        return;
    }
    const double lowest = std::numeric_limits<double>::lowest();
    const double highest = std::numeric_limits<double>::max();
    const char* props[] = { kOfxParamPropMin, kOfxParamPropDisplayMin, kOfxParamPropMax, kOfxParamPropDisplayMax };
    for (int i = 0; i < 4; ++i) {
        double v = 0.;
        ASSERT_TRUE( doubleProperty("doubleFrom", props[i], 0, &v) ) << props[i];
        EXPECT_EQ(i < 2 ? lowest : highest, v) << props[i];
    }

    KnobDoublePtr k = knob<KnobDouble>("doubleFrom");
    ASSERT_TRUE(k != NULL);
    EXPECT_EQ( lowest, k->getMinimum(0) );
    EXPECT_EQ( highest, k->getMaximum(0) );
    EXPECT_EQ( lowest, k->getDisplayMinimum(0) );
    EXPECT_EQ( highest, k->getDisplayMaximum(0) );

    CurvePtr curve = k->getCurve(ViewIdx(0), 0);
    ASSERT_TRUE(curve != NULL);
    Curve::YRange r = curve->getCurveYRange();
    EXPECT_EQ(lowest, r.min);
    EXPECT_EQ(highest, r.max);
    r = curve->getCurveDisplayYRange();
    EXPECT_EQ(lowest, r.min);
    EXPECT_EQ(highest, r.max);

    // regression guard (it passed with the old 1.79769e+308 bounds too): a value beyond any slider range
    // is kept, not clamped, and the integral over an unbounded range stays finite. With exact +/-DBL_MAX,
    // Interpolation::integrate_clamp() returns integrate() without solving for the bounds.
    EXPECT_TRUE( curve->addKeyFrame( KeyFrame(0., -1e300) ) );
    EXPECT_TRUE( curve->addKeyFrame( KeyFrame(10., 1e300) ) );
    EXPECT_EQ( -1e300, curve->getValueAt(0.) );
    EXPECT_EQ( 1e300, curve->getValueAt(10.) );
    const double integral = curve->getIntegrateFromTo(0., 10.);
    EXPECT_TRUE( integral == integral ); // not NaN
    EXPECT_NEAR( 0., integral / 1e300, 1e-9 );
}

// The 2D and 3D double parameters, and the colours: each dimension is unbounded, and a colour's display
// range is 0 to 1 (ofxParam.h: "kOfxParamPropDisplayMin ... 0 for colour parameters").
TEST_F(OfxParamRangeTest, MultiDimensional)
{
    if ( skipped() ) {
        return;
    }
    const double lowest = std::numeric_limits<double>::lowest();
    const double highest = std::numeric_limits<double>::max();
    struct Case
    {
        const char* name;
        int dim;
        bool colour;
    };
    const Case cases[] = {
        { "double2DFrom", 2, false },
        { "double3DFrom", 3, false },
        { "rgbFrom", 3, true },
        { "rgbaFrom", 4, true },
    };
    for (std::size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); ++c) {
        std::shared_ptr<KnobDoubleBase> k = std::dynamic_pointer_cast<KnobDoubleBase>( _node->getKnobByName(cases[c].name) );
        ASSERT_TRUE(k != NULL) << cases[c].name;
        ASSERT_EQ( cases[c].dim, k->getDimension() ) << cases[c].name;
        for (int i = 0; i < cases[c].dim; ++i) {
            EXPECT_EQ( lowest, k->getMinimum(i) ) << cases[c].name << "[" << i << "]";
            EXPECT_EQ( highest, k->getMaximum(i) ) << cases[c].name << "[" << i << "]";
            EXPECT_EQ( cases[c].colour ? 0. : lowest, k->getDisplayMinimum(i) ) << cases[c].name << "[" << i << "]";
            EXPECT_EQ( cases[c].colour ? 1. : highest, k->getDisplayMaximum(i) ) << cases[c].name << "[" << i << "]";
        }
    }
}

// A 2D parameter with a range (0 to 1) but no display range: the display range stays exactly -DBL_MAX to
// DBL_MAX, which the GUI (KnobGuiValue::createWidget()) recognizes as unset, so the slider covers the range
// and the dimensions can be folded. With -1.79769e+308 to 1.79769e+308 it showed neither.
TEST_F(OfxParamRangeTest, RangeWithoutDisplayRange)
{
    if ( skipped() ) {
        return;
    }
    std::shared_ptr<KnobDoubleBase> k = std::dynamic_pointer_cast<KnobDoubleBase>( _node->getKnobByName("double2DRanged") );
    ASSERT_TRUE(k != NULL);
    ASSERT_EQ( 2, k->getDimension() );
    for (int i = 0; i < 2; ++i) {
        EXPECT_EQ( 0., k->getMinimum(i) ) << i;
        EXPECT_EQ( 1., k->getMaximum(i) ) << i;
        EXPECT_EQ( std::numeric_limits<double>::lowest(), k->getDisplayMinimum(i) ) << i;
        EXPECT_EQ( std::numeric_limits<double>::max(), k->getDisplayMaximum(i) ) << i;
    }
}

// An integer parameter without limits: INT_MIN and INT_MAX.
TEST_F(OfxParamRangeTest, IntUnbounded)
{
    if ( skipped() ) {
        return;
    }
    KnobIntPtr k = knob<KnobInt>("intFrom");
    ASSERT_TRUE(k != NULL);
    EXPECT_EQ( INT_MIN, k->getMinimum(0) );
    EXPECT_EQ( INT_MAX, k->getMaximum(0) );
    EXPECT_EQ( INT_MIN, k->getDisplayMinimum(0) );
    EXPECT_EQ( INT_MAX, k->getDisplayMaximum(0) );
}
