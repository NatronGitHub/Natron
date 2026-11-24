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

// When the region of definition action of a node upstream fails (kOfxStatFailed: "something wrong, but no
// error code appropriate, plugin to post message"), the render of the nodes downstream fails, and they are
// not rendered with a black input: a Read node whose file is missing, with "On missing frame: Error", made
// NatronRenderer write black frames and report a successful render (#1602). An empty region of definition,
// a mandatory input that is not connected, or a disabled node without input still give a black input.
// net.natron.test.Generator with "fail" plays the Read node, and net.natron.test.MultiPAR (which counts its
// renders) the Write node: a working generator on its input Source gives it a RoD, the input tested is B.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#include <gtest/gtest.h>

#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const double kTime = 1.;

bool
canRunTest()
{
    const char* ids[] = { kNatronTestPluginGenerator, kNatronTestPluginMultiPAR, kNatronTestPluginParamRoD, kNatronTestPluginTimeBlur };

    for (const char* id : ids) {
        if ( !NatronTestPlugin::isAvailable(id) ) {
            NatronTestPlugin::reportSkipped(id);

            return false;
        }
    }

    return true;
}

// The nodes of a test, destroyed with it
class Nodes
{
public:
    ~Nodes()
    {
        for (std::vector<NodePtr>::reverse_iterator it = _nodes.rbegin(); it != _nodes.rend(); ++it) {
            (*it)->destroyNode(true, false);
        }
    }

    NodePtr create(const char* pluginID)
    {
        NodePtr node = NatronTestPlugin::createNode(pluginID);

        if (node) {
            _nodes.push_back(node);
        }

        return node;
    }

    // a net.natron.test.Generator, whose RoD and render fail if fail is true
    NodePtr generator(bool fail)
    {
        NodePtr node = create(kNatronTestPluginGenerator);

        if (node && fail) {
            KnobBoolPtr knob = std::dynamic_pointer_cast<KnobBool>( node->getKnobByName("fail") );
            if (!knob) {
                return NodePtr();
            }
            knob->setValue(true);
        }

        return node;
    }

private:
    std::vector<NodePtr> _nodes;
};

bool
connect(const NodePtr& input,
        int inputNb,
        const NodePtr& output)
{
    return input && output && output->getApp()->getProject()->connectNodes(inputNb, input, output);
}

// The number of renders of MultiPAR (net.natron.test.MultiPAR) when rendering it at kTime, and whether the
// render worked. b is connected to its input B (if not null), a generator that works to its input Source.
// Each render is done twice: the second one uses the results of the actions cached by the first one.
void
renderMultiPAR(Nodes& nodes,
               const NodePtr& b,
               bool expectSuccess)
{
    NodePtr multiPAR = nodes.create(kNatronTestPluginMultiPAR);
    ASSERT_TRUE( connect(nodes.generator(false), 0, multiPAR) );
    if (b) {
        ASSERT_TRUE( connect(b, 1, multiPAR) );
    }
    for (int i = 0; i < 2; ++i) {
        const int renders = NatronTestPlugin::counter("MultiPAR.render");
        const bool rendered = bool( NatronTestPlugin::renderFrame(multiPAR, kTime) );
        EXPECT_EQ(expectSuccess, rendered) << "render " << i;
        EXPECT_EQ(expectSuccess ? 1 : 0, NatronTestPlugin::counter("MultiPAR.render") - renders) << "render " << i;
    }
}
} // anon namespace

// The input B fails: MultiPAR is not rendered (its RoD is the one of Source, so it would be), and its render
// fails
TEST(FailedRoDRender, FailedInputFailsTheRender)
{
    if ( !canRunTest() ) {
        return;
    }
    Nodes nodes;
    renderMultiPAR(nodes, nodes.generator(true), false);
}

// Through a Dot, whose RoD is the one of its input: the RoD of the Dot fails too
TEST(FailedRoDRender, FailedInputThroughDotFailsTheRender)
{
    if ( !canRunTest() ) {
        return;
    }
    Nodes nodes;
    NodePtr dot = nodes.create(PLUGINID_NATRON_DOT);
    ASSERT_TRUE( connect(nodes.generator(true), 0, dot) );
    renderMultiPAR(nodes, dot, false);
}

// A render on disk (through a DiskCache, which renders like a Write node): no frame is rendered
TEST(FailedRoDRender, FailedInputFailsTheRenderOnDisk)
{
    if ( !canRunTest() ) {
        return;
    }
    Nodes nodes;
    NodePtr multiPAR = nodes.create(kNatronTestPluginMultiPAR);
    NodePtr output = nodes.create(PLUGINID_NATRON_DISKCACHE);
    ASSERT_TRUE( connect(nodes.generator(false), 0, multiPAR) );
    ASSERT_TRUE( connect(nodes.generator(true), 1, multiPAR) );
    ASSERT_TRUE( connect(multiPAR, 0, output) );
    const int renders = NatronTestPlugin::counter("MultiPAR.render");
    NatronTestPlugin::renderSequence(output, 1, 3);
    EXPECT_EQ( renders, NatronTestPlugin::counter("MultiPAR.render") ) << "frames rendered with a black input";
}

// The RoD of an OFX effect fails when it asks for the RoD of an input whose RoD fails, as for a built-in effect (a
// Dot above): a plug-in that gets an empty RoD for that input may otherwise ignore it (e.g. Merge is then the identity
// of B, a plug-in without tiles support asks for an empty region of its input), and render without it
TEST(FailedRoDRender, FailedInputFailsTheRoDOfAnOfxEffect)
{
    if ( !canRunTest() ) {
        return;
    }
    Nodes nodes;
    NodePtr timeBlur = nodes.create(kNatronTestPluginTimeBlur);
    NodePtr generator = nodes.generator(true);
    ASSERT_TRUE( connect(generator, 0, timeBlur) );
    RectD rod;
    EXPECT_EQ( eStatusFailed, timeBlur->getEffectInstance()->getRegionOfDefinition_public(timeBlur->getHashValue(), kTime, RenderScale::identity, ViewIdx(0), &rod, 0) );
    // the RoD of TimeBlur works again when the generator does
    KnobBoolPtr fail = std::dynamic_pointer_cast<KnobBool>( generator->getKnobByName("fail") );
    ASSERT_TRUE( bool(fail) );
    fail->setValue(false);
    EXPECT_EQ( eStatusOK, timeBlur->getEffectInstance()->getRegionOfDefinition_public(timeBlur->getHashValue(), kTime, RenderScale::identity, ViewIdx(0), &rod, 0) );
    EXPECT_FALSE( rod.isNull() );
}

// What still gives a black input: the node is rendered

TEST(FailedRoDRender, WorkingInputRenders)
{
    if ( !canRunTest() ) {
        return;
    }
    Nodes nodes;
    renderMultiPAR(nodes, nodes.generator(false), true);
}

TEST(FailedRoDRender, EmptyRoDInputRenders)
{
    if ( !canRunTest() ) {
        return;
    }
    Nodes nodes;
    NodePtr empty = nodes.create(kNatronTestPluginParamRoD);
    ASSERT_TRUE( bool(empty) );
    KnobDoublePtr size = std::dynamic_pointer_cast<KnobDouble>( empty->getKnobByName("size") );
    ASSERT_TRUE( bool(size) );
    size->setValue(0.);
    renderMultiPAR(nodes, empty, true);
}

// TimeBlur's input Source is mandatory and not connected; the Dot downstream has the RoD of TimeBlur
TEST(FailedRoDRender, DisconnectedMandatoryInputRenders)
{
    if ( !canRunTest() ) {
        return;
    }
    Nodes nodes;
    NodePtr dot = nodes.create(PLUGINID_NATRON_DOT);
    ASSERT_TRUE( connect(nodes.create(kNatronTestPluginTimeBlur), 0, dot) );
    renderMultiPAR(nodes, dot, true);
}

TEST(FailedRoDRender, DisabledNodeWithoutInputRenders)
{
    if ( !canRunTest() ) {
        return;
    }
    Nodes nodes;
    NodePtr disabled = nodes.create(kNatronTestPluginTimeBlur);
    ASSERT_TRUE( bool(disabled) );
    disabled->setNodeDisabled(true);
    renderMultiPAR(nodes, disabled, true);
}
