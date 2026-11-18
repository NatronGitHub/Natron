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


// The OpenFX host descriptor that Natron gives to plug-ins: the API version (OpenFX 1.5) and the
// OFX 1.5 / 1.5.1 host capabilities, which stay "not supported" until Natron implements them.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include <ofxCore.h>
#include <ofxColour.h>
#include <ofxDrawSuite.h>
#include <ofxGPURender.h>
#include <ofxImageEffect.h>
#include <ofxParam.h>
#include <ofxhImageEffect.h>
#include <ofxhPropertySuite.h>

#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_USING

namespace {
OFX::Host::Property::Set&
hostProps()
{
    // set when AppManager loads the OpenFX plug-ins (the Natron OfxHost)
    assert(OFX::Host::ImageEffect::gImageEffectHost);

    return OFX::Host::ImageEffect::gImageEffectHost->getProperties();
}
}

TEST(OfxHostProperties, APIVersion)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    OFX::Host::Property::Set& props = hostProps();
    ASSERT_EQ( 2, props.getDimension(kOfxPropAPIVersion) );
    EXPECT_EQ( 1, props.getIntProperty(kOfxPropAPIVersion, 0) );
    EXPECT_EQ( 5, props.getIntProperty(kOfxPropAPIVersion, 1) );
}

TEST(OfxHostProperties, HostIdentity)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    OFX::Host::Property::Set& props = hostProps();
    // OFX 1.5.1: "Image Effect Hosts property kOfxPropType must have the value kOfxTypeImageEffectHost"
    EXPECT_EQ( std::string(kOfxTypeImageEffectHost), props.getStringProperty(kOfxPropType) );
    ASSERT_EQ( 3, props.getDimension(kOfxPropVersion) );
    EXPECT_EQ( NATRON_VERSION_MAJOR, props.getIntProperty(kOfxPropVersion, 0) );
    EXPECT_EQ( NATRON_VERSION_MINOR, props.getIntProperty(kOfxPropVersion, 1) );
    EXPECT_EQ( NATRON_VERSION_REVISION, props.getIntProperty(kOfxPropVersion, 2) );
    EXPECT_EQ( std::string(NATRON_VERSION_STRING), props.getStringProperty(kOfxPropVersionLabel) );
}

// The OFX 1.5 / 1.5.1 opt-ins that Natron doesn't implement yet must say so: a plug-in may only
// use a 1.5 feature if the host property or suite says that the host supports it. Update these
// expectations when one of them is implemented.
TEST(OfxHostProperties, NotYetSupported15Features)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    OFX::Host::Property::Set& props = hostProps();
    EXPECT_EQ( std::string("true"), props.getStringProperty(kOfxImageEffectPropCPURenderSupported) );
    EXPECT_EQ( std::string("false"), props.getStringProperty(kOfxImageEffectPropCudaRenderSupported) );
    EXPECT_EQ( std::string("false"), props.getStringProperty(kOfxImageEffectPropMetalRenderSupported) );
    EXPECT_EQ( std::string("false"), props.getStringProperty(kOfxImageEffectPropOpenCLRenderSupported) );
    EXPECT_EQ( std::string(kOfxImageEffectColourManagementNone), props.getStringProperty(kOfxImageEffectPropColourManagementStyle) );
    EXPECT_EQ( 0, props.getIntProperty(kOfxParamHostPropSupportsStrChoice) );
    EXPECT_TRUE( OFX::Host::ImageEffect::gImageEffectHost->fetchSuite(kOfxDrawSuite, 1) == NULL );
}
