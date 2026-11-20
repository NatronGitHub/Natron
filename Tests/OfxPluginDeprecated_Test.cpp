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


// Natron treats the plug-ins that set kOfxImageEffectPluginPropObsolete (OFX, after 1.5.1) like the
// ones that set kNatronOfxImageEffectPropDeprecated: they load old projects, but they are not offered
// to users for new nodes.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <ofxImageEffect.h>
#include <ofxhPropertySuite.h>
#ifdef OFX_EXTENSIONS_NATRON
#include <ofxNatron.h>
#endif

#include "Engine/OfxHost.h"

NATRON_NAMESPACE_USING

namespace {
// The two properties, as HostSupport declares them on every image effect descriptor.
const OFX::Host::Property::PropSpec descriptorProps[] = {
    { kOfxImageEffectPluginPropObsolete, OFX::Host::Property::eInt, 1, false, "0" },
#ifdef OFX_EXTENSIONS_NATRON
    { kNatronOfxImageEffectPropDeprecated, OFX::Host::Property::eInt, 1, false, "0" },
#endif
    OFX::Host::Property::propSpecEnd
};
}

TEST(OfxPluginDeprecated, Default)
{
    OFX::Host::Property::Set props(descriptorProps);

    EXPECT_FALSE( NATRON_NAMESPACE::OfxHost::isPluginDeprecated(props) );
}

TEST(OfxPluginDeprecated, Obsolete)
{
    OFX::Host::Property::Set props(descriptorProps);

    props.setIntProperty(kOfxImageEffectPluginPropObsolete, 1);
    EXPECT_TRUE( NATRON_NAMESPACE::OfxHost::isPluginDeprecated(props) );
}

#ifdef OFX_EXTENSIONS_NATRON
TEST(OfxPluginDeprecated, NatronDeprecated)
{
    OFX::Host::Property::Set props(descriptorProps);

    props.setIntProperty(kNatronOfxImageEffectPropDeprecated, 1);
    EXPECT_TRUE( NATRON_NAMESPACE::OfxHost::isPluginDeprecated(props) );
}
#endif
