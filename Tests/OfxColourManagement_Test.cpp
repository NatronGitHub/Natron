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


// OFX 1.5 colour management (ofxColour.h): Natron's host properties, the style negotiation, the working
// space, and the colourspace labels of the node metadata.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include <ofxColour.h>
#include <ofx-native-v1.5_aces-v1.3_ocio-v2.3.h>
#include <ofxhImageEffect.h>
#include <ofxhPropertySuite.h>

#include "Engine/NodeMetadata.h"
#include "Engine/OfxImageEffectInstance.h"

NATRON_NAMESPACE_USING

TEST(OfxColourManagement, HostProperties)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    OFX::Host::Property::Set& props = OFX::Host::ImageEffect::gImageEffectHost->getProperties();
    EXPECT_EQ( std::string(kOfxImageEffectColourManagementOCIO), props.getStringProperty(kOfxImageEffectPropColourManagementStyle) );
    ASSERT_EQ( 1, props.getDimension(kOfxImageEffectPropColourManagementAvailableConfigs) );
    EXPECT_EQ( std::string(kOfxConfigIdentifier), props.getStringProperty(kOfxImageEffectPropColourManagementAvailableConfigs) );
}

TEST(OfxColourManagement, Style)
{
    const std::string ocio = kOfxImageEffectColourManagementOCIO;
    const std::string full = kOfxImageEffectColourManagementFull;
    const std::string core = kOfxImageEffectColourManagementCore;
    const std::string basic = kOfxImageEffectColourManagementBasic;
    const std::string none = kOfxImageEffectColourManagementNone;

    // plug-ins that don't do colour management (all the plug-ins written before OFX 1.5) get none
    EXPECT_EQ( none, OfxImageEffectInstance::chooseColourManagementStyle(ocio, none, true) );
    EXPECT_EQ( none, OfxImageEffectInstance::chooseColourManagementStyle(ocio, "", true) );
    EXPECT_EQ( none, OfxImageEffectInstance::chooseColourManagementStyle(ocio, "SomethingElse", true) );
    // the highest style both support
    EXPECT_EQ( ocio, OfxImageEffectInstance::chooseColourManagementStyle(ocio, ocio, true) );
    EXPECT_EQ( full, OfxImageEffectInstance::chooseColourManagementStyle(ocio, full, true) );
    EXPECT_EQ( core, OfxImageEffectInstance::chooseColourManagementStyle(ocio, core, true) );
    EXPECT_EQ( basic, OfxImageEffectInstance::chooseColourManagementStyle(ocio, basic, true) );
    // the OCIO style needs an OCIO config
    EXPECT_EQ( full, OfxImageEffectInstance::chooseColourManagementStyle(ocio, ocio, false) );
    EXPECT_EQ( basic, OfxImageEffectInstance::chooseColourManagementStyle(ocio, basic, false) );
}

TEST(OfxColourManagement, WorkingColourspace)
{
    EXPECT_EQ( std::string("scene_linear"), OfxImageEffectInstance::workingColourspace(kOfxImageEffectColourManagementOCIO) );
    EXPECT_EQ( std::string(kOfxColourspaceOfxSceneLinear), OfxImageEffectInstance::workingColourspace(kOfxImageEffectColourManagementFull) );
    EXPECT_EQ( std::string(kOfxColourspaceOfxSceneLinear), OfxImageEffectInstance::workingColourspace(kOfxImageEffectColourManagementCore) );
    EXPECT_EQ( std::string(kOfxColourspaceOfxSceneLinear), OfxImageEffectInstance::workingColourspace(kOfxImageEffectColourManagementBasic) );
    EXPECT_EQ( std::string(), OfxImageEffectInstance::workingColourspace(kOfxImageEffectColourManagementNone) );
    EXPECT_EQ( std::string(), OfxImageEffectInstance::workingColourspace("") );
}

TEST(OfxColourManagement, MetadataColourspace)
{
    NodeMetadata a;
    a.clearAndResize(2);
    // the default is the working space
    EXPECT_EQ( std::string(), a.getColourspace(-1) );
    EXPECT_EQ( std::string(), a.getColourspace(0) );
    EXPECT_EQ( std::string(), a.getColourspace(5) ); // no such input

    NodeMetadata b(a);
    EXPECT_TRUE(a == b);
    b.setColourspace(-1, "ACEScct");
    EXPECT_EQ( std::string("ACEScct"), b.getColourspace(-1) );
    EXPECT_FALSE(a == b); // a new output colourspace is a metadata change, which is propagated downstream
    a = b;
    EXPECT_TRUE(a == b);
    b.setColourspace(1, "lin_rec709");
    EXPECT_EQ( std::string("lin_rec709"), b.getColourspace(1) );
    EXPECT_EQ( std::string(), b.getColourspace(0) );
    EXPECT_FALSE(a == b);
    b.setColourspace(5, "ignored"); // no such input
    EXPECT_EQ( std::string(), b.getColourspace(5) );
}
