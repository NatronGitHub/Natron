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

// For a plug-in that supports multiple clip pixel aspect ratios (kOfxImageEffectPropSupportsMultipleClipPARs),
// each input clip gets the PAR of its own input, not the PAR of the first connected input (#1376).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include "Engine/KnobTypes.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
NodePtr
createGenerator(double par)
{
    NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);

    if (generator) {
        KnobDouble* parKnob = dynamic_cast<KnobDouble*>( generator->getKnobByName("par").get() );
        if (parKnob) {
            parKnob->setValue(par);
        }
    }

    return generator;
}
}

TEST(OfxMultiplePAR, EachInputGetsItsOwnPAR)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginMultiPAR) || !NatronTestPlugin::isAvailable(kNatronTestPluginGenerator) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginMultiPAR);

        return;
    }
    // a square-pixel and an anamorphic generator, connected in both orders
    const double pars[2] = { 1., 2. };
    for (int order = 0; order < 2; ++order) {
        const double sourcePAR = pars[order];
        const double bPAR = pars[1 - order];
        NodePtr source = createGenerator(sourcePAR);
        NodePtr b = createGenerator(bPAR);
        NodePtr node = NatronTestPlugin::createNode(kNatronTestPluginMultiPAR);
        ASSERT_TRUE( source && b && node );
        EXPECT_EQ( sourcePAR, source->getEffectInstance()->getAspectRatio(-1) );
        EXPECT_EQ( bPAR, b->getEffectInstance()->getAspectRatio(-1) );
        ASSERT_EQ(0, node->getInputNumberFromLabel(kOfxImageEffectSimpleSourceClipName));
        const int bInput = node->getInputNumberFromLabel("B");
        ASSERT_GE(bInput, 0);
        ASSERT_TRUE( node->getApp()->getProject()->connectNodes(0, source, node) );
        ASSERT_TRUE( node->getApp()->getProject()->connectNodes(bInput, b, node) );

        // the clip preferences
        EXPECT_EQ( sourcePAR, node->getEffectInstance()->getAspectRatio(0) ) << "order " << order;
        EXPECT_EQ( bPAR, node->getEffectInstance()->getAspectRatio(bInput) ) << "order " << order;

        // what the plug-in sees in render, on the clips and the images
        ASSERT_TRUE( bool( NatronTestPlugin::renderFrame(node, 1.) ) );
        EXPECT_EQ( (int)(sourcePAR * 1000), NatronTestPlugin::counter("MultiPAR.SourceClipPAR") ) << "order " << order;
        EXPECT_EQ( (int)(sourcePAR * 1000), NatronTestPlugin::counter("MultiPAR.SourceImagePAR") ) << "order " << order;
        EXPECT_EQ( (int)(bPAR * 1000), NatronTestPlugin::counter("MultiPAR.BClipPAR") ) << "order " << order;
        EXPECT_EQ( (int)(bPAR * 1000), NatronTestPlugin::counter("MultiPAR.BImagePAR") ) << "order " << order;

        node->destroyNode(true, false);
        b->destroyNode(true, false);
        source->destroyNode(true, false);
    }
}
