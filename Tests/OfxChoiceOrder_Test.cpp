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
//
// HostSupport follows OFX 1.5.1 (ofxParam.h: "Default - Zero-based ordinal list of same length as
// OfxParamPropChoiceOption"): the property always has one value per option (an appended option gets its
// index, the order is cut when the options shrink), and a value the plug-in set is kept. Natron treats an
// order that sorts the options to 0..n-1 as the natural order (no explicit display order, #1898).

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

// the values of kOfxParamPropChoiceOrder, as the plug-in gets them
Order
orderProperty(OfxPropertySetHandle props)
{
    const OfxPropertySuiteV1* suite = NatronTestPlugin::propertySuite();
    int n = 0;

    if (suite->propGetDimension(props, kOfxParamPropChoiceOrder, &n) != kOfxStatOK) {
        return Order(1, -1);
    }
    Order order(n);
    if ( (n > 0) && (suite->propGetIntN(props, kOfxParamPropChoiceOrder, n, order.data()) != kOfxStatOK) ) {
        return Order(1, -1);
    }

    return order;
}

// the property set of the param name of node
OfxPropertySetHandle
paramProperties(const NodePtr& node,
                const char* name)
{
    OFX::Host::Param::Instance* param = NatronTestPlugin::ofxParam(node, name);
    OfxPropertySetHandle props = 0;

    if ( !param || (NatronTestPlugin::parameterSuite()->paramGetPropertySet(param->getHandle(), &props) != kOfxStatOK) ) {
        return 0;
    }

    return props;
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
        NatronTestPlugin::reportMissing(kNatronTestPluginChoiceOrder); \
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
    // the instance has the values of its descriptor
    const int values[] = { 0, 10, 30, 20 };
    EXPECT_EQ( Order(values, values + 4), orderProperty(_orderedProps) );
}

// Without the property, the order is OFX 1.5.1's default, 0..n-1: the options are displayed in their natural
// order, and the knob has no explicit display order.
TEST_F(OfxChoiceOrderTest, NaturalOrder)
{
    SKIP_IF_NO_PLUGIN();
    const int expected[] = { 0, 1, 2 };
    KnobChoicePtr natural = choiceKnob(_node, "natural");
    ASSERT_TRUE( bool(natural) );
    EXPECT_EQ( Order(expected, expected + 3), natural->getDisplayOrder_mt_safe() );
    EXPECT_TRUE( natural->getExplicitDisplayOrder_mt_safe().empty() );
    EXPECT_EQ( Order(expected, expected + 3), orderProperty( paramProperties(_node, "natural") ) );
}

// The plug-in sets the order of only some options (5 and 1 on P and Q of P, Q, R): the others keep their index
// (5, 1, 2), displayed as Q, R, P. An instance order with fewer values than options (5, 1) is completed the same
// way.
TEST_F(OfxChoiceOrderTest, PartialOrder)
{
    SKIP_IF_NO_PLUGIN();
    KnobChoicePtr partial = choiceKnob(_node, "partialOrder");
    ASSERT_TRUE( bool(partial) );
    const int values[] = { 5, 1, 2 };
    EXPECT_EQ( Order(values, values + 3), orderProperty( paramProperties(_node, "partialOrder") ) );
    const int expected[] = { 1, 2, 0 };
    EXPECT_EQ( Order(expected, expected + 3), partial->getExplicitDisplayOrder_mt_safe() );
    const char* const labels[] = { "Q", "R", "P" };
    EXPECT_EQ( strings(labels, 3), displayedLabels(partial) );

    // the natural param's instance, with a short order
    KnobChoicePtr natural = choiceKnob(_node, "natural");
    ASSERT_TRUE( bool(natural) );
    OfxPropertySetHandle props = paramProperties(_node, "natural");
    ASSERT_TRUE(props != 0);
    const int shortOrder[] = { 5, 1 };
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetIntN(props, kOfxParamPropChoiceOrder, 2, shortOrder) );
    EXPECT_EQ( Order(values, values + 3), orderProperty(props) );
    EXPECT_EQ( Order(expected, expected + 3), natural->getExplicitDisplayOrder_mt_safe() );
    const char* const naturalLabels[] = { "Y", "Z", "X" };
    EXPECT_EQ( strings(naturalLabels, 3), displayedLabels(natural) );
}

// The same value twice (0, 1, 0: the plug-in set 0 as the order of C) is undefined behaviour: Natron keeps the
// natural order of the options that have the same value (A, C, B), and the values stay the option indices.
TEST_F(OfxChoiceOrderTest, DuplicateOrderValues)
{
    SKIP_IF_NO_PLUGIN();
    KnobChoicePtr duplicate = choiceKnob(_node, "duplicate");
    ASSERT_TRUE( bool(duplicate) );
    OfxPropertySetHandle props = paramProperties(_node, "duplicate");
    ASSERT_TRUE(props != 0);
    const int values[] = { 0, 1, 0 };
    EXPECT_EQ( Order(values, values + 3), orderProperty(props) );
    const char* const labels[] = { "A", "C", "B" };
    EXPECT_EQ( strings(labels, 3), displayedLabels(duplicate) );
    duplicate->setValue(2);
    EXPECT_EQ( std::string("C"), duplicate->getActiveEntry().id );
    // more duplicates on the instance
    const int same[] = { 7, 7, 7 };
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetIntN(props, kOfxParamPropChoiceOrder, 3, same) );
    EXPECT_TRUE( duplicate->getExplicitDisplayOrder_mt_safe().empty() );
    const char* const natural[] = { "A", "B", "C" };
    EXPECT_EQ( strings(natural, 3), displayedLabels(duplicate) );
    EXPECT_EQ( 2, duplicate->getValue() );
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

// The plug-in appends an option to an instance (kOfxParamPropChoiceOption): until the plug-in sets its order, the
// new option's order is its index (0, 10, 30, 20, 4: displayed second); the value the plug-in then sets is kept.
TEST_F(OfxChoiceOrderTest, AppendedOption)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetString(_orderedProps, kOfxParamPropChoiceOption, 4, "Late") );
    const int values[] = { 0, 10, 30, 20, 4 };
    EXPECT_EQ( Order(values, values + 5), orderProperty(_orderedProps) );
    const char* const appended[] = { "OptA", "Late", "OptB", "NewOpt", "OptC" };
    EXPECT_EQ( strings(appended, 5), displayedLabels(_ordered) );
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetInt(_orderedProps, kOfxParamPropChoiceOrder, 4, 15) );
    const int set[] = { 0, 10, 30, 20, 15 };
    EXPECT_EQ( Order(set, set + 5), orderProperty(_orderedProps) );
    const char* const labels[] = { "OptA", "OptB", "Late", "NewOpt", "OptC" };
    EXPECT_EQ( strings(labels, 5), displayedLabels(_ordered) );
    // the value is the index of the option
    _ordered->setValue(4);
    int value = -1;
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetValue(_orderedHandle, &value) );
    EXPECT_EQ(4, value);
    EXPECT_EQ( std::string("Late"), _ordered->getActiveEntry().id );
}

// The plug-in shrinks the options of an instance to OptA, OptB: the order is cut (0, 10, the natural order);
// an option appended then gets its index (0, 10, 2: displayed second).
TEST_F(OfxChoiceOrderTest, ShrunkOptions)
{
    SKIP_IF_NO_PLUGIN();
    const char* const kept[] = { "OptA", "OptB" };
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetStringN(_orderedProps, kOfxParamPropChoiceOption, 2, kept) );
    EXPECT_EQ( strings(kept, 2), entryIDs(_ordered) );
    const int cut[] = { 0, 10 };
    EXPECT_EQ( Order(cut, cut + 2), orderProperty(_orderedProps) );
    EXPECT_TRUE( _ordered->getExplicitDisplayOrder_mt_safe().empty() );
    EXPECT_EQ( strings(kept, 2), displayedLabels(_ordered) );
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propSetString(_orderedProps, kOfxParamPropChoiceOption, 2, "Again") );
    const int values[] = { 0, 10, 2 };
    EXPECT_EQ( Order(values, values + 3), orderProperty(_orderedProps) );
    const char* const labels[] = { "OptA", "Again", "OptB" };
    EXPECT_EQ( strings(labels, 3), displayedLabels(_ordered) );
}

// A reset of the order gives 0..n-1 (the natural order); a reset of the options gives no options and no order.
TEST_F(OfxChoiceOrderTest, Reset)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propReset(_orderedProps, kOfxParamPropChoiceOrder) );
    const int natural[] = { 0, 1, 2, 3 };
    EXPECT_EQ( Order(natural, natural + 4), orderProperty(_orderedProps) );
    EXPECT_TRUE( _ordered->getExplicitDisplayOrder_mt_safe().empty() );
    const char* const labels[] = { "OptA", "OptB", "OptC", "NewOpt" };
    EXPECT_EQ( strings(labels, 4), displayedLabels(_ordered) );

    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::propertySuite()->propReset(_orderedProps, kOfxParamPropChoiceOption) );
    EXPECT_TRUE( orderProperty(_orderedProps).empty() );
    EXPECT_TRUE( _ordered->getEntries_mt_safe().empty() );
    EXPECT_TRUE( _ordered->getDisplayOrder_mt_safe().empty() );
}

// A copy of a param (KnobI::createDuplicateOnHolder(), e.g. a param exported to a group) takes its explicit display
// order: none for a param without an order (0..n-1), the plug-in's order otherwise.
TEST_F(OfxChoiceOrderTest, CopyTakesExplicitOrder)
{
    SKIP_IF_NO_PLUGIN();
    NodePtr other = createChoiceOrderNode();
    ASSERT_TRUE( bool(other) );
    KnobChoicePtr natural = choiceKnob(_node, "natural");
    ASSERT_TRUE( bool(natural) );
    KnobChoicePtr naturalCopy = std::dynamic_pointer_cast<KnobChoice>(
        natural->createDuplicateOnHolder(other->getEffectInstance().get(), KnobPagePtr(), KnobGroupPtr(), -1,
                                         false /*makeAlias*/, "naturalCopy", "naturalCopy", std::string(),
                                         true /*refreshParams*/, true /*isUserKnob*/) );
    ASSERT_TRUE( bool(naturalCopy) );
    EXPECT_TRUE( naturalCopy->getExplicitDisplayOrder_mt_safe().empty() );
    EXPECT_EQ( displayedLabels(natural), displayedLabels(naturalCopy) );
    KnobChoicePtr orderedCopy = std::dynamic_pointer_cast<KnobChoice>(
        _ordered->createDuplicateOnHolder(other->getEffectInstance().get(), KnobPagePtr(), KnobGroupPtr(), -1,
                                          false /*makeAlias*/, "orderedCopy", "orderedCopy", std::string(),
                                          true /*refreshParams*/, true /*isUserKnob*/) );
    ASSERT_TRUE( bool(orderedCopy) );
    const int expected[] = { 0, 1, 3, 2 };
    EXPECT_EQ( Order(expected, expected + 4), orderedCopy->getExplicitDisplayOrder_mt_safe() );
    EXPECT_EQ( displayedLabels(_ordered), displayedLabels(orderedCopy) );
    naturalCopy.reset();
    orderedCopy.reset();
    destroy(other);
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

// KnobChoice: an order that is not a permutation of the entry indices, or the identity, gives the natural order;
// appended entries are displayed last; resetChoices() and populateChoices() without an order give the natural order;
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
    // the identity is the natural order: no explicit display order (#1898)
    knob->populateChoices( entries, Order(natural, natural + 3) );
    EXPECT_EQ( Order(natural, natural + 3), knob->getDisplayOrder_mt_safe() );
    EXPECT_TRUE( knob->getExplicitDisplayOrder_mt_safe().empty() );

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
    const int identity[] = { 0, 1, 2, 3 };
    knob->setDisplayOrder( Order(identity, identity + 4) );
    EXPECT_EQ(2, populated);
    EXPECT_TRUE( knob->getExplicitDisplayOrder_mt_safe().empty() );
    knob->setDisplayOrder( Order() ); // the same (natural) order
    EXPECT_EQ(2, populated);

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
