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

// OFX parameter properties that follow others, for the plug-in's writes only (openfx#384, #2023): "Setting this will
// also reset kOfxParamPropDisplayMin" (ofxParam.h, kOfxParamPropMin; and the same for the max), and "Note that
// resetting this will also reset kOfxPropShortLabel and kOfxPropLongLabel" (ofxCore.h, kOfxPropLabel). HostSupport
// applies these rules to what the plug-in sets through the property suite, not to the host's own setters: when Natron
// mirrors a knob's new range or label into the OFX properties (OfxParamToKnob::onMinMaxChanged() and
// onLabelChanged()), the display range and the short and long labels keep their values. The parameter is "doubleFrom"
// of the test plug-in net.natron.test.Params (Tests/OfxTestPlugin), a 1D double parameter.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <limits>
#include <string>

#include <gtest/gtest.h>

#include <ofxCore.h>
#include <ofxParam.h>
#include <ofxProperty.h>

#include "Engine/KnobTypes.h"
#include "Engine/Node.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const char* const kParam = "doubleFrom";

class OfxParamLinkedPropertiesTest
    : public NatronTestPlugin::ParamsNodeTest
{
protected:
    KnobDoublePtr knob() const
    {
        return std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName(kParam) );
    }

    // the property set of the parameter instance, as the plug-in gets it from paramGetPropertySet()
    OfxPropertySetHandle properties() const
    {
        OFX::Host::Param::Instance* p = NatronTestPlugin::ofxParam(_node, kParam);
        OfxPropertySetHandle props = nullptr;

        if ( !p || (NatronTestPlugin::parameterSuite()->paramGetPropertySet(p->getHandle(), &props) != kOfxStatOK) ) {
            return nullptr;
        }

        return props;
    }

    // a property read through the property suite, as the plug-in does (NaN or "<none>" if that fails)
    double doubleProperty(const char* name) const
    {
        double v = std::numeric_limits<double>::quiet_NaN();

        NatronTestPlugin::propertySuite()->propGetDouble(properties(), name, 0, &v);

        return v;
    }

    std::string stringProperty(const char* name) const
    {
        const char* v = nullptr;

        if ( (NatronTestPlugin::propertySuite()->propGetString(properties(), name, 0, &v) != kOfxStatOK) || !v ) {
            return "<none>";
        }

        return v;
    }
};
} // anonymous namespace

// Natron changes the knob's range: the OFX range follows, the display range stays.
TEST_F(OfxParamLinkedPropertiesTest, HostRangeKeepsDisplayRange)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr k = knob();
    ASSERT_TRUE(k != NULL);
    ASSERT_TRUE(properties() != nullptr);
    const double displayMin = doubleProperty(kOfxParamPropDisplayMin);
    const double displayMax = doubleProperty(kOfxParamPropDisplayMax);
    ASSERT_NE(-5., displayMin);
    ASSERT_NE(5., displayMax);

    k->setMinimum(-5.);
    k->setMaximum(5.);
    EXPECT_EQ( -5., doubleProperty(kOfxParamPropMin) );
    EXPECT_EQ( 5., doubleProperty(kOfxParamPropMax) );
    EXPECT_EQ( displayMin, doubleProperty(kOfxParamPropDisplayMin) );
    EXPECT_EQ( displayMax, doubleProperty(kOfxParamPropDisplayMax) );
    EXPECT_EQ( displayMin, k->getDisplayMinimum(0) );
    EXPECT_EQ( displayMax, k->getDisplayMaximum(0) );
}

// The plug-in sets the range: the display range follows, and so does the knob (setRange(), then setDisplayRange()).
TEST_F(OfxParamLinkedPropertiesTest, PluginRangeResetsDisplayRange)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr k = knob();
    ASSERT_TRUE(k != NULL);
    OfxPropertySetHandle props = properties();
    ASSERT_TRUE(props != nullptr);

    EXPECT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetDouble(props, kOfxParamPropMin, 0, -2.) );
    EXPECT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetDouble(props, kOfxParamPropMax, 0, 3.) );
    EXPECT_EQ( -2., doubleProperty(kOfxParamPropDisplayMin) );
    EXPECT_EQ( 3., doubleProperty(kOfxParamPropDisplayMax) );
    EXPECT_EQ( -2., k->getMinimum(0) );
    EXPECT_EQ( 3., k->getMaximum(0) );
    EXPECT_EQ( -2., k->getDisplayMinimum(0) );
    EXPECT_EQ( 3., k->getDisplayMaximum(0) );
}

// Natron changes the knob's label: the OFX label follows, the short and long labels stay.
TEST_F(OfxParamLinkedPropertiesTest, HostLabelKeepsShortAndLongLabels)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr k = knob();
    ASSERT_TRUE(k != NULL);
    ASSERT_TRUE(properties() != nullptr);
    const std::string shortLabel = stringProperty(kOfxPropShortLabel);
    const std::string longLabel = stringProperty(kOfxPropLongLabel);
    ASSERT_NE("Renamed", shortLabel);
    ASSERT_NE("Renamed", longLabel);

    k->setLabel( std::string("Renamed") );
    EXPECT_EQ( "Renamed", stringProperty(kOfxPropLabel) );
    EXPECT_EQ( shortLabel, stringProperty(kOfxPropShortLabel) );
    EXPECT_EQ( longLabel, stringProperty(kOfxPropLongLabel) );
}

// The plug-in sets the label: the short and long labels follow, and so does the knob.
TEST_F(OfxParamLinkedPropertiesTest, PluginLabelResetsShortAndLongLabels)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr k = knob();
    ASSERT_TRUE(k != NULL);
    OfxPropertySetHandle props = properties();
    ASSERT_TRUE(props != nullptr);

    EXPECT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetString(props, kOfxPropLabel, 0, "PluginLabel") );
    EXPECT_EQ( "PluginLabel", stringProperty(kOfxPropShortLabel) );
    EXPECT_EQ( "PluginLabel", stringProperty(kOfxPropLongLabel) );
    EXPECT_EQ( "PluginLabel", k->getLabel() );
}
