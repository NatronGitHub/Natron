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

// OFX 1.5 StrChoice params (kOfxParamTypeStrChoice, "String-Valued Choice Parameters" in ofxParameter.rst),
// #1101: "The host stores the string value rather than the index of the option, and when loading a project,
// finds and selects the option with the corresponding enum." paramGetValue/paramSetValue pass the enum
// (kOfxParamPropChoiceEnum), the menu shows the options (kOfxParamPropChoiceOption) in their
// kOfxParamPropChoiceOrder, the default is the plug-in's enum or the first option, and an enum that the
// plug-in removed gives the default when a project is loaded. They are KnobChoice knobs (ChoiceParam in
// Python) and don't animate. The test plug-in is Tests/OfxTestPlugin/NatronTestPluginChoice.cpp.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>
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
#include <ofxhImageEffect.h>
#include <ofxhParam.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Project.h"
#include "Engine/PyParameter.h"

#include "OfxTestPlugin.h"

// Tests/OfxTestPlugin/NatronTestPluginChoice.cpp
#define kNatronTestPluginStrChoice "net.natron.test.StrChoice"

NATRON_NAMESPACE_USING

namespace {
NodePtr
createStrChoiceNode(const NodeSerializationPtr& serialization = NodeSerializationPtr())
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginStrChoice, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    if (serialization) {
        args.setProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization, serialization);
        args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
    }

    return app->createNode(args);
}

OfxParamHandle
paramHandle(const NodePtr& node,
            const char* name)
{
    OFX::Host::Param::Instance* param = NatronTestPlugin::ofxParam(node, name);

    return param ? param->getHandle() : 0;
}

// the value of a StrChoice param, as the plug-in gets it
::testing::AssertionResult
getEnum(OfxParamHandle param,
        std::string* value)
{
    const char* v = 0;
    OfxStatus stat = NatronTestPlugin::parameterSuite()->paramGetValue(param, &v);

    if (stat != kOfxStatOK) {
        return ::testing::AssertionFailure() << "paramGetValue returned " << stat;
    }
    if (!v) {
        return ::testing::AssertionFailure() << "paramGetValue returned a NULL string";
    }
    *value = v;

    return ::testing::AssertionSuccess();
}

std::string
enumOf(OfxParamHandle param)
{
    std::string value;

    return getEnum(param, &value) ? value : std::string("<error>");
}

// save the node as in a project (XML)
std::string
saveNode(const NodePtr& node)
{
    std::stringstream ss;
    {
        NodeSerialization s(node);
        boost::archive::xml_oarchive oArchive(ss);
        oArchive << boost::serialization::make_nvp("Node", s);
    }

    return ss.str();
}

NodeSerializationPtr
loadNode(const std::string& xml)
{
    std::stringstream ss(xml);
    NodeSerializationPtr loaded = std::make_shared<NodeSerialization>();
    {
        boost::archive::xml_iarchive iArchive(ss);
        iArchive >> boost::serialization::make_nvp("Node", *loaded);
    }

    return loaded;
}

// the serialization of the knob name in a loaded node
KnobSerializationPtr
serializedKnob(const NodeSerializationPtr& node,
               const std::string& name)
{
    for (NodeSerialization::KnobValues::const_iterator it = node->getKnobsValues().begin(); it != node->getKnobsValues().end(); ++it) {
        if ( (*it)->getName() == name ) {
            return *it;
        }
    }

    return KnobSerializationPtr();
}

// replace every occurrence of from by to
std::string
replaceAll(std::string s,
           const std::string& from,
           const std::string& to)
{
    for (std::size_t pos = s.find(from); pos != std::string::npos; pos = s.find(from, pos + to.size())) {
        s.replace(pos, from.size(), to);
    }

    return s;
}

class OfxStrChoiceParamTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginStrChoice) ) {
            return;
        }
        _node = createStrChoiceNode();
        ASSERT_TRUE( bool(_node) );
        _str = paramHandle(_node, "str");
        ASSERT_TRUE(_str != 0);
        _knob = std::dynamic_pointer_cast<KnobChoice>( _node->getKnobByName("str") );
        ASSERT_TRUE( bool(_knob) );
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        _knob.reset();
        destroy(_node);
    }

    static void destroy(NodePtr& node)
    {
        if (node) {
            node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        node.reset();
    }

    // a node created from xml, the saved _node edited by the caller
    NodePtr restore(const std::string& xml)
    {
        return createStrChoiceNode( loadNode(xml) );
    }

    NodePtr _node;
    OfxParamHandle _str = 0;
    KnobChoicePtr _knob;
};
} // anonymous namespace

#define SKIP_IF_NO_PLUGIN() \
    if (!_node) { \
        NatronTestPlugin::reportMissing(kNatronTestPluginStrChoice); \
        return; \
    }

// "To check for availability of this param type, a plugin may check the host property
// kOfxParamHostPropSupportsStrChoice". They don't animate.
TEST(OfxStrChoiceParamHost, Supported)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    const OFX::Host::Property::Set& props = OFX::Host::ImageEffect::gImageEffectHost->getProperties();
    EXPECT_EQ( 1, props.getIntProperty(kOfxParamHostPropSupportsStrChoice) );
    EXPECT_EQ( 0, props.getIntProperty(kOfxParamHostPropSupportsStrChoiceAnimation) );
}

// paramDefine accepts StrChoice params (it returned kOfxStatErrUnsupported before): menus whose entry IDs are the
// enums and labels the options, which never animate, even with kOfxParamPropAnimates = 1 (strAnimates).
TEST_F(OfxStrChoiceParamTest, Defined)
{
    SKIP_IF_NO_PLUGIN();
    EXPECT_EQ( 0, NatronTestPlugin::counter("StrChoice.defineFailed") );
    EXPECT_GE( NatronTestPlugin::counter("StrChoice.defined"), 3 );
    const char* const names[] = { "str", "strNoDefault", "strAnimates" };
    for (int i = 0; i < 3; ++i) {
        KnobChoicePtr knob = std::dynamic_pointer_cast<KnobChoice>( _node->getKnobByName(names[i]) );
        ASSERT_TRUE( bool(knob) ) << names[i];
        EXPECT_FALSE( knob->isAnimationEnabled() ) << names[i];
        EXPECT_TRUE( knob->getIsPersistent() ) << names[i];
        EXPECT_FALSE( knob->getIsSecret() ) << names[i];
    }
    const std::vector<ChoiceOption> entries = _knob->getEntries_mt_safe();
    const char* const enums[] = { "red", "green", "blue", "alpha" };
    const char* const options[] = { "Red", "Green", "Blue", "Alpha" };
    ASSERT_EQ( 4u, entries.size() );
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ( std::string(enums[i]), entries[i].id );
        EXPECT_EQ( std::string(options[i]), entries[i].label );
    }
}

// The plug-in's default enum, or "If no default value is set by the plugin, the host should use the first
// defined option (index 0)".
TEST_F(OfxStrChoiceParamTest, Default)
{
    SKIP_IF_NO_PLUGIN();
    EXPECT_EQ( std::string("blue"), enumOf(_str) );
    EXPECT_EQ( 2, _knob->getValue() );
    EXPECT_EQ( 2, _knob->getDefaultValue(0) );
    EXPECT_EQ( std::string("first"), enumOf( paramHandle(_node, "strNoDefault") ) );
}

// paramSetValue takes an enum (kOfxStatErrValue for other strings); the values at a time are the same.
TEST_F(OfxStrChoiceParamTest, SetAndGet)
{
    SKIP_IF_NO_PLUGIN();
    const OfxParameterSuiteV1* suite = NatronTestPlugin::parameterSuite();
    ASSERT_EQ( kOfxStatOK, suite->paramSetValue(_str, "alpha") );
    EXPECT_EQ( 3, _knob->getValue() );
    EXPECT_EQ( std::string("alpha"), _knob->getActiveEntry().id );
    EXPECT_EQ( std::string("alpha"), enumOf(_str) );

    EXPECT_EQ( kOfxStatErrValue, suite->paramSetValue(_str, "Alpha") ); // an option, not an enum
    EXPECT_EQ( kOfxStatErrValue, suite->paramSetValue(_str, "nope") );
    EXPECT_EQ( kOfxStatErrValue, suite->paramSetValue(_str, (const char*)0) );
    EXPECT_EQ( std::string("alpha"), enumOf(_str) );

    // they don't animate: no keyframe
    ASSERT_EQ( kOfxStatOK, suite->paramSetValueAtTime(_str, 10., "green") );
    EXPECT_EQ( std::string("green"), enumOf(_str) );
    const char* atTime = 0;
    ASSERT_EQ( kOfxStatOK, suite->paramGetValueAtTime(_str, 20., &atTime) );
    ASSERT_TRUE(atTime != 0);
    EXPECT_EQ( std::string("green"), std::string(atTime) );
    unsigned int nKeys = 1;
    ASSERT_EQ( kOfxStatOK, suite->paramGetNumKeys(_str, &nKeys) );
    EXPECT_EQ(0u, nKeys);
    EXPECT_FALSE( _knob->isAnimated(0) );
}

// The menu shows the options in their kOfxParamPropChoiceOrder (0, 20, 10, 30): Red, Blue, Green, Alpha.
TEST_F(OfxStrChoiceParamTest, DisplayOrder)
{
    SKIP_IF_NO_PLUGIN();
    const int expected[] = { 0, 2, 1, 3 };
    EXPECT_EQ( std::vector<int>(expected, expected + 4), _knob->getDisplayOrder_mt_safe() );
    // without an order, OFX 1.5.1's default 0..n-1 is the natural order: no explicit display order (#1898)
    KnobChoicePtr noOrder = std::dynamic_pointer_cast<KnobChoice>( _node->getKnobByName("strNoDefault") );
    ASSERT_TRUE( bool(noOrder) );
    const int natural[] = { 0, 1 };
    EXPECT_EQ( std::vector<int>(natural, natural + 2), noOrder->getDisplayOrder_mt_safe() );
    EXPECT_TRUE( noOrder->getExplicitDisplayOrder_mt_safe().empty() );
}

// A user change (the menu) gives the plug-in the enum in kOfxActionInstanceChanged.
TEST_F(OfxStrChoiceParamTest, InstanceChanged)
{
    SKIP_IF_NO_PLUGIN();
    const int before = NatronTestPlugin::counter("StrChoice.instanceChanged");
    const int greenBefore = NatronTestPlugin::counter("StrChoice.value.green");
    _knob->onValueChanged(1, ViewSpec::all(), 0, eValueChangedReasonUserEdited, NULL);
    EXPECT_EQ( before + 1, NatronTestPlugin::counter("StrChoice.instanceChanged") );
    EXPECT_EQ( greenBefore + 1, NatronTestPlugin::counter("StrChoice.value.green") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("StrChoice.getFailed") );
}

// The project stores the enum (the option string of the KnobChoice), and a project is restored by enum, even if
// the option is at another index (the plug-in reordered its options).
TEST_F(OfxStrChoiceParamTest, ProjectStoresEnum)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(_str, "green") );
    NodeSerializationPtr loaded = loadNode( saveNode(_node) );
    KnobSerializationPtr saved = serializedKnob(loaded, "str");
    ASSERT_TRUE( bool(saved) );
    const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>( saved->getExtraData() );
    ASSERT_TRUE(data != 0);
    EXPECT_EQ( std::string("green"), data->_choiceString );
    // the index saved by a version of the plug-in where green was the last option
    KnobChoicePtr savedKnob = std::dynamic_pointer_cast<KnobChoice>( saved->getKnob() );
    ASSERT_TRUE( bool(savedKnob) );
    savedKnob->setValue(3);
    NodePtr restored = createStrChoiceNode(loaded);
    ASSERT_TRUE( bool(restored) );
    EXPECT_EQ( std::string("green"), enumOf( paramHandle(restored, "str") ) );
    destroy(restored);
}

// "If a plugin removes enums in later versions and a project is saved with the removed enum, behavior is
// undefined, but it is recommended that the host use the default value in that case." The enum must match
// exactly: a different case is another enum.
TEST_F(OfxStrChoiceParamTest, RemovedEnumGivesDefault)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(_str, "green") );
    const std::string xml = saveNode(_node);
    ASSERT_NE( std::string::npos, xml.find(">green<") );
    const char* const removed[] = { "removedInV2", "GREEN", "Green" };
    for (int i = 0; i < 3; ++i) {
        NodePtr restored = restore( replaceAll(xml, ">green<", std::string(">") + removed[i] + "<") );
        ASSERT_TRUE( bool(restored) ) << removed[i];
        EXPECT_EQ( std::string("blue"), enumOf( paramHandle(restored, "str") ) ) << removed[i];
        KnobChoicePtr knob = std::dynamic_pointer_cast<KnobChoice>( restored->getKnobByName("str") );
        ASSERT_TRUE( bool(knob) );
        // no "The value ... no longer exist in the menu" warning: the active entry is the default
        EXPECT_TRUE( knob->isActiveEntryPresentInEntries() ) << removed[i];
        EXPECT_EQ( std::string("blue"), knob->getActiveEntry().id ) << removed[i];
        destroy(restored);
    }
}

// The project's default (an option index, saved by a version of the plug-in that may have had other options) is not
// loaded: the default stays the plug-in's.
TEST_F(OfxStrChoiceParamTest, DefaultIsNotLoaded)
{
    SKIP_IF_NO_PLUGIN();
    // a knob that has its default value is not saved
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(_str, "green") );
    NodeSerializationPtr loaded = loadNode( saveNode(_node) );
    KnobSerializationPtr saved = serializedKnob(loaded, "str");
    ASSERT_TRUE( bool(saved) );
    KnobChoicePtr savedKnob = std::dynamic_pointer_cast<KnobChoice>( saved->getKnob() );
    ASSERT_TRUE( bool(savedKnob) );
    savedKnob->setDefaultValueWithoutApplying(0, 0);
    NodePtr restored = createStrChoiceNode(loaded);
    ASSERT_TRUE( bool(restored) );
    KnobChoicePtr knob = std::dynamic_pointer_cast<KnobChoice>( restored->getKnobByName("str") );
    ASSERT_TRUE( bool(knob) );
    EXPECT_EQ( 2, knob->getDefaultValue(0) );
    EXPECT_EQ( std::string("green"), enumOf( paramHandle(restored, "str") ) );
    destroy(restored);
}

// ChoiceParam.set() (PyPlugs and Python projects restore choices with it) matches the exact enum too: a removed or
// differently spelled enum leaves the value unchanged, the default for a new node.
TEST_F(OfxStrChoiceParamTest, PythonSetMatchesExactEnum)
{
    SKIP_IF_NO_PLUGIN();
    ::Natron::Python::ChoiceParam param(_knob);
    param.set( QString::fromUtf8("alpha") );
    EXPECT_EQ( std::string("alpha"), enumOf(_str) );
    const char* const wrong[] = { "GREEN", "Green", "removedInV2" };
    for (int i = 0; i < 3; ++i) {
        param.set( QString::fromUtf8(wrong[i]) );
        EXPECT_EQ( std::string("alpha"), enumOf(_str) ) << wrong[i];
        EXPECT_TRUE( _knob->isActiveEntryPresentInEntries() ) << wrong[i];
    }
}

// The plug-in removes the selected enum from an instance: the value becomes the default.
TEST_F(OfxStrChoiceParamTest, InstanceRemovesSelectedEnum)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(_str, "alpha") );
    OfxPropertySetHandle props = 0;
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetPropertySet(_str, &props) );
    // alpha removed, blue (the default) moved first
    const char* const options[] = { "Blue", "Red", "Green" };
    const char* const enums[] = { "blue", "red", "green" };
    const OfxPropertySuiteV1* propSuite = NatronTestPlugin::propertySuite();
    ASSERT_EQ( kOfxStatOK, propSuite->propSetStringN(props, kOfxParamPropChoiceOption, 3, options) );
    ASSERT_EQ( kOfxStatOK, propSuite->propSetStringN(props, kOfxParamPropChoiceEnum, 3, enums) );
    EXPECT_EQ( std::string("blue"), enumOf(_str) );
    EXPECT_EQ( 0, _knob->getValue() );
    EXPECT_EQ( 0, _knob->getDefaultValue(0) );
    // the order is cut to the options (0, 20, 10): Blue, Green, Red
    const int shrunk[] = { 0, 2, 1 };
    EXPECT_EQ( std::vector<int>(shrunk, shrunk + 3), _knob->getDisplayOrder_mt_safe() );
    // the menu shows that alpha is missing; it is selected again if the plug-in adds it back
    EXPECT_FALSE( _knob->isActiveEntryPresentInEntries() );
    const char* const backOptions[] = { "Blue", "Red", "Green", "Alpha" };
    const char* const backEnums[] = { "blue", "red", "green", "alpha" };
    ASSERT_EQ( kOfxStatOK, propSuite->propSetStringN(props, kOfxParamPropChoiceOption, 4, backOptions) );
    ASSERT_EQ( kOfxStatOK, propSuite->propSetStringN(props, kOfxParamPropChoiceEnum, 4, backEnums) );
    EXPECT_EQ( std::string("alpha"), enumOf(_str) );
}

// The plug-in rebuilds the options of an instance one at a time, as the Support library's resetOptions() and
// appendOption() do (each append sets the enum, then the option): the selected enum is selected again once it is
// appended, and the default follows its enum.
TEST_F(OfxStrChoiceParamTest, InstanceOptionsRebuiltOneByOne)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(_str, "green") );
    OfxPropertySetHandle props = 0;
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetPropertySet(_str, &props) );
    const OfxPropertySuiteV1* propSuite = NatronTestPlugin::propertySuite();
    ASSERT_EQ( kOfxStatOK, propSuite->propReset(props, kOfxParamPropChoiceOption) );
    ASSERT_EQ( kOfxStatOK, propSuite->propReset(props, kOfxParamPropChoiceEnum) );
    const char* const options[] = { "Red", "Green", "Blue", "Alpha" };
    const char* const enums[] = { "red", "green", "blue", "alpha" };
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ( kOfxStatOK, propSuite->propSetString(props, kOfxParamPropChoiceEnum, i, enums[i]) );
        ASSERT_EQ( kOfxStatOK, propSuite->propSetString(props, kOfxParamPropChoiceOption, i, options[i]) );
    }
    ASSERT_EQ( 4u, _knob->getEntries_mt_safe().size() );
    EXPECT_EQ( std::string("green"), enumOf(_str) );
    EXPECT_EQ( 2, _knob->getDefaultValue(0) );
    EXPECT_TRUE( _knob->isActiveEntryPresentInEntries() );
    // the reset of the options reset the order too: each appended option got its index (the natural order)
    EXPECT_TRUE( _knob->getExplicitDisplayOrder_mt_safe().empty() );
}

// The plug-in changes the options and enums of an instance (here, sorted by name): the value stays the same enum.
TEST_F(OfxStrChoiceParamTest, InstanceOptionsChange)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(_str, "green") );
    OfxPropertySetHandle props = 0;
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetPropertySet(_str, &props) );
    const char* const options[] = { "Alpha", "Blue", "Green", "Red" };
    const char* const enums[] = { "alpha", "blue", "green", "red" };
    const OfxPropertySuiteV1* propSuite = NatronTestPlugin::propertySuite();
    ASSERT_EQ( kOfxStatOK, propSuite->propSetStringN(props, kOfxParamPropChoiceOption, 4, options) );
    ASSERT_EQ( kOfxStatOK, propSuite->propSetStringN(props, kOfxParamPropChoiceEnum, 4, enums) );
    const std::vector<ChoiceOption> entries = _knob->getEntries_mt_safe();
    ASSERT_EQ( 4u, entries.size() );
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ( std::string(enums[i]), entries[i].id );
        EXPECT_EQ( std::string(options[i]), entries[i].label );
    }
    EXPECT_EQ( std::string("green"), enumOf(_str) );
    EXPECT_EQ( 2, _knob->getValue() );
    // the default enum (blue) is at another index
    EXPECT_EQ( 1, _knob->getDefaultValue(0) );
}

// paramCopy copies the enum.
TEST_F(OfxStrChoiceParamTest, Copy)
{
    SKIP_IF_NO_PLUGIN();
    NodePtr other = createStrChoiceNode();
    ASSERT_TRUE( bool(other) );
    OfxParamHandle otherStr = paramHandle(other, "str");
    const OfxParameterSuiteV1* suite = NatronTestPlugin::parameterSuite();
    ASSERT_EQ( kOfxStatOK, suite->paramSetValue(otherStr, "alpha") );
    EXPECT_EQ( kOfxStatOK, suite->paramCopy(_str, otherStr, 0., NULL) );
    EXPECT_EQ( std::string("alpha"), enumOf(_str) );
    // an enum that the destination doesn't have
    EXPECT_NE( kOfxStatOK, suite->paramCopy(_str, paramHandle(_node, "strNoDefault"), 0., NULL) );
    EXPECT_EQ( std::string("alpha"), enumOf(_str) );
    destroy(other);
}

// The Python ChoiceParam API works as for Choice params: the value is the option index, getOption() gives the
// enum, setValue() takes an index or an enum.
TEST_F(OfxStrChoiceParamTest, PythonChoiceParam)
{
    SKIP_IF_NO_PLUGIN();
    const std::string node = appPTR->getTopLevelInstance()->getAppIDString() + "." + _node->getFullyQualifiedName();
    const std::string script =
        "p = " + node + ".getParam(\"str\")\n"
        "assert type(p).__name__ == \"ChoiceParam\", type(p)\n"
        "assert p.getValue() == 2, p.getValue()\n"
        "assert p.getDefaultValue() == 2, p.getDefaultValue()\n"
        "assert p.getNumOptions() == 4, p.getNumOptions()\n"
        "assert p.getOption(2) == \"blue\", p.getOption(2)\n"
        "assert list(p.getOptions()) == [\"red\", \"green\", \"blue\", \"alpha\"], p.getOptions()\n"
        "p.setValue(\"alpha\")\n"
        "assert p.getValue() == 3, p.getValue()\n"
        "p.setValue(1)\n"
        "assert p.getValue() == 1, p.getValue()\n";
    std::string error, output;
    EXPECT_TRUE( ::Natron::Python::interpretPythonScript(script, &error, &output) ) << error << output;
    EXPECT_EQ( std::string(), error );
    EXPECT_EQ( std::string("green"), enumOf(_str) );
}

// The string returned to the plug-in is per thread: render threads may read the param at the same time, and the
// string returned to one thread stays valid until that thread's next call on the param (HostSupport's
// StrChoiceInstance::getV returns a pointer to a member of the instance, which another thread would overwrite).
TEST_F(OfxStrChoiceParamTest, PerThreadData)
{
    SKIP_IF_NO_PLUGIN();
    const OfxParamHandle param = _str;
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(param, "red") );
    // a thread reads the value, then waits while the value changes and another thread reads it
    std::atomic<int> step(0);
    std::string mineLater;
    std::thread mine([&step, &mineLater, param]() {
        const char* value = 0;
        NatronTestPlugin::parameterSuite()->paramGetValue(param, &value);
        step = 1;
        while (step < 2) {
            std::this_thread::yield();
        }
        // still valid: this thread made no other call on the param
        mineLater = value ? value : "<NULL>";
    });
    while (step < 1) {
        std::this_thread::yield();
    }
    // on the main thread, the plug-in's instanceChanged reads the param too (EXPECT: mine must be joined)
    EXPECT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(param, "alpha") );
    std::string theirs;
    std::thread other([&theirs, param]() {
        const char* value = 0;
        NatronTestPlugin::parameterSuite()->paramGetValue(param, &value);
        theirs = value ? value : "<NULL>";
    });
    other.join();
    step = 2;
    mine.join();
    EXPECT_EQ( std::string("alpha"), theirs );
    EXPECT_EQ( std::string("red"), mineLater );
}
