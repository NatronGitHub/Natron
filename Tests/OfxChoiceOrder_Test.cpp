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

// The UI order of the options of Choice params (kOfxParamPropChoiceOrder, OFX 1.5), #1137: "If the host
// supports this property, it should sort the options and the order list together, and display the options
// in increasing order" (ofxParameter.rst, "Setting Choice Param Order"). It "does not affect project storage
// or operation": the value is still the index of the option, and projects restore it by option string. The
// menus show the entries of a KnobChoice in KnobChoice::getDisplayOrder_mt_safe() (Gui/KnobGuiChoice.cpp).
// The test plug-in is Tests/OfxTestPlugin/NatronTestPluginChoice.cpp.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format off
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(sign-compare)
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_ON(sign-compare)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
// clang-format on

#include <ofxParam.h>
#include <ofxhParam.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

// Tests/OfxTestPlugin/NatronTestPluginChoice.cpp
#define kNatronTestPluginChoiceOrder "net.natron.test.ChoiceOrder"

NATRON_NAMESPACE_USING

namespace {
typedef std::vector<int> Order;

NodePtr
createChoiceOrderNode(const NodeSerializationPtr& serialization = NodeSerializationPtr())
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginChoiceOrder, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    if (serialization) {
        args.setProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization, serialization);
        args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
    }

    return app->createNode(args);
}

std::vector<std::string>
entryIDs(const KnobChoicePtr& knob)
{
    std::vector<std::string> ids;
    std::vector<ChoiceOption> entries = knob->getEntries_mt_safe();

    for (std::size_t i = 0; i < entries.size(); ++i) {
        ids.push_back(entries[i].id);
    }

    return ids;
}

// the labels of the entries, in the order of the menu
std::vector<std::string>
displayedLabels(const KnobChoicePtr& knob)
{
    std::vector<std::string> labels;
    std::vector<ChoiceOption> entries = knob->getEntries_mt_safe();
    Order order = knob->getDisplayOrder_mt_safe();

    for (std::size_t i = 0; i < order.size(); ++i) {
        labels.push_back( (order[i] >= 0 && order[i] < (int)entries.size()) ? entries[order[i]].label : std::string("?") );
    }

    return labels;
}

std::vector<std::string>
strings(const char* const* s,
        int n)
{
    return std::vector<std::string>(s, s + n);
}

class OfxChoiceOrderTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginChoiceOrder) ) {
            return;
        }
        _node = createChoiceOrderNode();
        ASSERT_TRUE( bool(_node) );
        _ordered = choiceKnob(_node, "ordered");
        ASSERT_TRUE( bool(_ordered) );
        OFX::Host::Param::Instance* param = NatronTestPlugin::ofxParam(_node, "ordered");
        ASSERT_TRUE(param != 0);
        _orderedHandle = param->getHandle();
        ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetPropertySet(_orderedHandle, &_orderedProps) );
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        _ordered.reset();
        destroy(_node);
    }

    static void destroy(NodePtr& node)
    {
        if (node) {
            node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        node.reset();
    }

    static KnobChoicePtr choiceKnob(const NodePtr& node,
                                    const char* name)
    {
        return std::dynamic_pointer_cast<KnobChoice>( node->getKnobByName(name) );
    }

    NodePtr _node;
    KnobChoicePtr _ordered;
    OfxParamHandle _orderedHandle = 0;
    OfxPropertySetHandle _orderedProps = 0;
};
} // anonymous namespace

#define SKIP_IF_NO_PLUGIN() \
    if (!_node) { \
        NatronTestPlugin::reportSkipped(kNatronTestPluginChoiceOrder); \
        return; \
    }

// "To query whether a host supports this, a plugin should attempt to set the property and check the return
// status": HostSupport declares it on Choice params, so setting it succeeds (and Natron must then use it).
TEST_F(OfxChoiceOrderTest, PropertyIsAccepted)
{
    SKIP_IF_NO_PLUGIN();
    EXPECT_GE( NatronTestPlugin::counter("ChoiceOrder.orderSet"), 1 );
    EXPECT_EQ( 0, NatronTestPlugin::counter("ChoiceOrder.orderFailed") );
}

// Options OptA, OptB, OptC, NewOpt with the order 0, 10, 30, 20 are displayed as OptA, OptB, NewOpt, OptC; the
// entries (and their indices, the values) keep the plug-in's order.
TEST_F(OfxChoiceOrderTest, DisplayOrder)
{
    SKIP_IF_NO_PLUGIN();
    const char* const ids[] = { "OptA", "OptB", "OptC", "NewOpt" };
    EXPECT_EQ( strings(ids, 4), entryIDs(_ordered) );
    const int expected[] = { 0, 1, 3, 2 };
    EXPECT_EQ( Order(expected, expected + 4), _ordered->getDisplayOrder_mt_safe() );
    const char* const labels[] = { "OptA", "OptB", "NewOpt", "OptC" };
    EXPECT_EQ( strings(labels, 4), displayedLabels(_ordered) );
}

// Without the property, or with fewer values than options (the property must have one value per option), the
// options are displayed in their natural order.
TEST_F(OfxChoiceOrderTest, NaturalOrder)
{
    SKIP_IF_NO_PLUGIN();
    const int expected[] = { 0, 1, 2 };
    KnobChoicePtr natural = choiceKnob(_node, "natural");
    ASSERT_TRUE( bool(natural) );
    EXPECT_EQ( Order(expected, expected + 3), natural->getDisplayOrder_mt_safe() );
    KnobChoicePtr partial = choiceKnob(_node, "partialOrder");
    ASSERT_TRUE( bool(partial) );
    EXPECT_EQ( Order(expected, expected + 3), partial->getDisplayOrder_mt_safe() );
}

// "the plugin will get the param value 2, as it did in its previous version": the value is the index of the
// option in the plug-in's list, whatever the display order.
TEST_F(OfxChoiceOrderTest, ValuesAreOptionIndices)
{
    SKIP_IF_NO_PLUGIN();
    _ordered->setValue(2);
    int value = -1;
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetValue(_orderedHandle, &value) );
    EXPECT_EQ(2, value);
    EXPECT_EQ( std::string("OptC"), _ordered->getActiveEntry().id );

    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(_orderedHandle, 3) );
    EXPECT_EQ( 3, _ordered->getValue() );
    EXPECT_EQ( std::string("NewOpt"), _ordered->getActiveEntry().id );
}

// The plug-in changes the order of an instance: the menu follows.
TEST_F(OfxChoiceOrderTest, InstanceOrderChange)
{
    SKIP_IF_NO_PLUGIN();
    int populated = 0;
    QObject context; // disconnects the lambda at the end of the test
    QObject::connect( _ordered.get(), &KnobChoice::populated, &context, [&populated]() { ++populated; } );
    const int reversed[] = { 3, 2, 1, 0 };
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetIntN(_orderedProps, kOfxParamPropChoiceOrder, 4, reversed) );
    const int expected[] = { 3, 2, 1, 0 };
    EXPECT_EQ( Order(expected, expected + 4), _ordered->getDisplayOrder_mt_safe() );
    EXPECT_GE(populated, 1);
    // the values are unchanged
    const char* const ids[] = { "OptA", "OptB", "OptC", "NewOpt" };
    EXPECT_EQ( strings(ids, 4), entryIDs(_ordered) );
}

// The plug-in appends an option to an instance (kOfxParamPropChoiceOption), then gives its order: the new option
// is displayed last until its order is set.
TEST_F(OfxChoiceOrderTest, AppendedOption)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetString(_orderedProps, kOfxParamPropChoiceOption, 4, "Late") );
    const int appended[] = { 0, 1, 3, 2, 4 };
    EXPECT_EQ( Order(appended, appended + 5), _ordered->getDisplayOrder_mt_safe() );
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetInt(_orderedProps, kOfxParamPropChoiceOrder, 4, 15) );
    const char* const labels[] = { "OptA", "OptB", "Late", "NewOpt", "OptC" };
    EXPECT_EQ( strings(labels, 5), displayedLabels(_ordered) );
}

// The display order doesn't change what the project stores (the index and the option string), and a project
// is restored by option string: an option saved at another index (e.g. by a version of the plug-in that had
// it elsewhere) is found again.
TEST_F(OfxChoiceOrderTest, ProjectRestoresByOptionString)
{
    SKIP_IF_NO_PLUGIN();
    _ordered->setValue(2); // OptC
    NodeSerializationPtr saved = std::make_shared<NodeSerialization>(_node);
    std::stringstream ss;
    {
        boost::archive::xml_oarchive oArchive(ss);
        oArchive << boost::serialization::make_nvp("Node", *saved);
    }
    NodeSerializationPtr loaded = std::make_shared<NodeSerialization>();
    {
        boost::archive::xml_iarchive iArchive(ss);
        iArchive >> boost::serialization::make_nvp("Node", *loaded);
    }
    bool found = false;
    for (NodeSerialization::KnobValues::const_iterator it = loaded->getKnobsValues().begin(); it != loaded->getKnobsValues().end(); ++it) {
        if ( (*it)->getName() == "ordered" ) {
            const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>( (*it)->getExtraData() );
            ASSERT_TRUE(data != 0);
            EXPECT_EQ( std::string("OptC"), data->_choiceString );
            // the index saved by a version of the plug-in where OptC was the first option
            KnobChoicePtr serializedKnob = std::dynamic_pointer_cast<KnobChoice>( (*it)->getKnob() );
            ASSERT_TRUE( bool(serializedKnob) );
            serializedKnob->setValue(0);
            found = true;
        }
    }
    ASSERT_TRUE(found);
    NodePtr restored = createChoiceOrderNode(loaded);
    ASSERT_TRUE( bool(restored) );
    KnobChoicePtr knob = choiceKnob(restored, "ordered");
    ASSERT_TRUE( bool(knob) );
    EXPECT_EQ( 2, knob->getValue() );
    EXPECT_EQ( std::string("OptC"), knob->getActiveEntry().id );
    const int expected[] = { 0, 1, 3, 2 };
    EXPECT_EQ( Order(expected, expected + 4), knob->getDisplayOrder_mt_safe() );
    destroy(restored);
}

// KnobChoice: an order that is not a permutation of the entry indices gives the natural order; appended entries
// are displayed last; resetChoices() and populateChoices() without an order give the natural order;
// setDisplayOrder() emits populated() only if the order changed.
TEST_F(OfxChoiceOrderTest, KnobChoiceDisplayOrder)
{
    SKIP_IF_NO_PLUGIN();
    KnobChoicePtr knob = AppManager::createKnob<KnobChoice>(_node->getEffectInstance().get(), std::string("displayOrderTest"), 1);
    std::vector<ChoiceOption> entries;
    entries.push_back( ChoiceOption("a") );
    entries.push_back( ChoiceOption("b") );
    entries.push_back( ChoiceOption("c") );
    const int natural[] = { 0, 1, 2 };
    const int valid[] = { 2, 0, 1 };
    const int duplicate[] = { 0, 0, 1 };
    const int outOfRange[] = { 0, 1, 3 };
    knob->populateChoices( entries, Order(valid, valid + 3) );
    EXPECT_EQ( Order(valid, valid + 3), knob->getDisplayOrder_mt_safe() );
    knob->populateChoices( entries, Order(duplicate, duplicate + 3) );
    EXPECT_EQ( Order(natural, natural + 3), knob->getDisplayOrder_mt_safe() );
    knob->populateChoices( entries, Order(outOfRange, outOfRange + 3) );
    EXPECT_EQ( Order(natural, natural + 3), knob->getDisplayOrder_mt_safe() );
    knob->populateChoices( entries, Order(valid, valid + 2) );
    EXPECT_EQ( Order(natural, natural + 3), knob->getDisplayOrder_mt_safe() );

    knob->populateChoices( entries, Order(valid, valid + 3) );
    knob->appendChoice( ChoiceOption("d") );
    const int appended[] = { 2, 0, 1, 3 };
    EXPECT_EQ( Order(appended, appended + 4), knob->getDisplayOrder_mt_safe() );

    int populated = 0;
    QObject context; // disconnects the lambda at the end of the test
    QObject::connect( knob.get(), &KnobChoice::populated, &context, [&populated]() { ++populated; } );
    knob->setDisplayOrder( Order(appended, appended + 4) );
    EXPECT_EQ(0, populated);
    const int reversed[] = { 3, 2, 1, 0 };
    knob->setDisplayOrder( Order(reversed, reversed + 4) );
    EXPECT_EQ(1, populated);
    EXPECT_EQ( Order(reversed, reversed + 4), knob->getDisplayOrder_mt_safe() );

    knob->populateChoices(entries);
    EXPECT_EQ( Order(natural, natural + 3), knob->getDisplayOrder_mt_safe() );
    knob->resetChoices();
    EXPECT_TRUE( knob->getDisplayOrder_mt_safe().empty() );
}

// The tooltip lists the options in the order of the menu ("display the options in increasing order"), and
// getExplicitDisplayOrder_mt_safe() (the order that copies take) is empty without an order.
TEST_F(OfxChoiceOrderTest, TooltipAndExplicitOrder)
{
    SKIP_IF_NO_PLUGIN();
    KnobChoicePtr knob = AppManager::createKnob<KnobChoice>(_node->getEffectInstance().get(), std::string("tooltipOrderTest"), 1);
    std::vector<ChoiceOption> entries;
    entries.push_back( ChoiceOption("a", "", "help a") );
    entries.push_back( ChoiceOption("b", "", "help b") );
    entries.push_back( ChoiceOption("c", "", "help c") );
    knob->populateChoices(entries);
    EXPECT_TRUE( knob->getExplicitDisplayOrder_mt_safe().empty() );
    EXPECT_EQ( std::string("a: help a\nb: help b\nc: help c"), knob->getHintToolTipFull() );
    const int order[] = { 2, 0, 1 };
    knob->populateChoices( entries, Order(order, order + 3) );
    EXPECT_EQ( Order(order, order + 3), knob->getExplicitDisplayOrder_mt_safe() );
    EXPECT_EQ( std::string("c: help c\na: help a\nb: help b"), knob->getHintToolTipFull() );
    // the order of "ordered" was set by the plug-in
    const int expected[] = { 0, 1, 3, 2 };
    EXPECT_EQ( Order(expected, expected + 4), _ordered->getExplicitDisplayOrder_mt_safe() );
    EXPECT_TRUE( choiceKnob(_node, "natural")->getExplicitDisplayOrder_mt_safe().empty() );
}
