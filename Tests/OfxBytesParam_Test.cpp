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

// OFX 1.5 Bytes params (kOfxParamTypeBytes, "a Plug-in defined opaque data parameter", ofxParam.h), #1102:
// paramDefine accepts them, paramGetValue/paramSetValue pass an OfxBytes (the host copies the data, NUL
// bytes included; a NULL OfxBytes or a zero length gives an empty value), the default is a pointer to an
// OfxBytes, and the value (a KnobBytes) is saved in the project. The test plug-in is
// Tests/OfxTestPlugin/NatronTestPluginBytes.cpp.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <climits>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <QtCore/QTemporaryDir>

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
#include "Engine/Variant.h"

#include "OfxTestPlugin.h"

// Tests/OfxTestPlugin/NatronTestPluginBytes.cpp
#define kNatronTestPluginBytes "net.natron.test.Bytes"

NATRON_NAMESPACE_USING

namespace {
typedef std::vector<unsigned char> Bytes;

// the default of "bytes" in NatronTestPluginBytes.cpp
const unsigned char kDefaultData[] = { 'N', 0, 'a', 0xff, 0 };

Bytes
defaultBytes()
{
    return Bytes( kDefaultData, kDefaultData + sizeof(kDefaultData) );
}

// 256 bytes 0..255 (the value set by the plug-in for trigger = 1)
Bytes
allBytes()
{
    Bytes b;

    for (int i = 0; i < 256; ++i) {
        b.push_back( (unsigned char)i );
    }

    return b;
}

std::string
toString(const Bytes& b)
{
    return std::string( b.begin(), b.end() );
}

NodePtr
createBytesNode(const NodeSerializationPtr& serialization = NodeSerializationPtr())
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginBytes, app->getProject() );

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

// the value of a Bytes param, as the plug-in gets it
::testing::AssertionResult
getBytes(OfxParamHandle param,
         Bytes* value)
{
    OfxBytes b = { 0, 12345 };
    OfxStatus stat = NatronTestPlugin::parameterSuite()->paramGetValue(param, &b);

    if (stat != kOfxStatOK) {
        return ::testing::AssertionFailure() << "paramGetValue returned " << stat;
    }
    if ( (b.length > 0) && !b.data ) {
        return ::testing::AssertionFailure() << "NULL data with length " << b.length;
    }
    if ( (b.length == 0) && b.data ) {
        return ::testing::AssertionFailure() << "empty value with non-NULL data";
    }
    value->assign(b.data, b.data + b.length);

    return ::testing::AssertionSuccess();
}

OfxStatus
setBytes(OfxParamHandle param,
         const Bytes& value)
{
    OfxBytes b = { value.empty() ? 0 : &value[0], value.size() };

    return NatronTestPlugin::parameterSuite()->paramSetValue(param, &b);
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

class OfxBytesParamTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginBytes) ) {
            return;
        }
        _node = createBytesNode();
        ASSERT_TRUE( bool(_node) );
        _bytes = paramHandle(_node, "bytes");
        ASSERT_TRUE(_bytes != 0);
        _knob = std::dynamic_pointer_cast<KnobBytes>( _node->getKnobByName("bytes") );
        ASSERT_TRUE( bool(_knob) );
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        _knob.reset();
        destroy(_node);
        if (_projectLoaded) {
            // leave the shared app with an untitled empty project, as the other tests find it
            appPTR->getTopLevelInstance()->getProject()->resetProject();
            _projectLoaded = false;
        }
    }

    static void destroy(NodePtr& node)
    {
        if (node) {
            node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        node.reset();
    }

    void trigger(int mode)
    {
        KnobIntPtr knob = std::dynamic_pointer_cast<KnobInt>( _node->getKnobByName("trigger") );

        ASSERT_TRUE( bool(knob) );
        knob->onValueChanged(mode, ViewSpec::all(), 0, eValueChangedReasonUserEdited, NULL);
    }

    NodePtr _node;
    OfxParamHandle _bytes = 0;
    KnobBytesPtr _knob;
    bool _projectLoaded = false;
};
} // anonymous namespace

#define SKIP_IF_NO_PLUGIN() \
    if (!_node) { \
        NatronTestPlugin::reportSkipped(kNatronTestPluginBytes); \
        return; \
    }

// The standard has no host property for Bytes params; HostSupport's paramDefine accepts them when the host
// sets kOfxHostSupportHostPropSupportsBytes.
TEST(OfxBytesParamHost, Supported)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    EXPECT_EQ( 1, OFX::Host::ImageEffect::gImageEffectHost->getProperties().getIntProperty(kOfxHostSupportHostPropSupportsBytes) );
}

// paramDefine accepts Bytes params (it returned kOfxStatErrUnsupported before), and they are hidden,
// non-animated KnobBytes.
TEST_F(OfxBytesParamTest, Defined)
{
    SKIP_IF_NO_PLUGIN();
    EXPECT_EQ( 0, NatronTestPlugin::counter("Bytes.defineFailed") );
    EXPECT_GE( NatronTestPlugin::counter("Bytes.defined"), 3 );
    const char* names[] = { "bytes", "bytesNoDefault", "bytesAnimates" };
    for (int i = 0; i < 3; ++i) {
        KnobBytesPtr knob = std::dynamic_pointer_cast<KnobBytes>( _node->getKnobByName(names[i]) );
        ASSERT_TRUE( bool(knob) ) << names[i];
        EXPECT_EQ( KnobBytes::typeNameStatic(), std::static_pointer_cast<KnobI>(knob)->typeName() ) << names[i];
        EXPECT_TRUE( knob->getIsSecret() ) << names[i];
        // even with kOfxParamPropAnimates = 1 (bytesAnimates)
        EXPECT_FALSE( knob->isAnimationEnabled() ) << names[i];
        EXPECT_FALSE( std::static_pointer_cast<KnobI>(knob)->canAnimate() ) << names[i];
        EXPECT_TRUE( knob->getIsPersistent() ) << names[i];
        // kOfxParamPropCanUndo defaults to 1
        EXPECT_TRUE( knob->getCanUndo() ) << names[i];
    }
}

// kOfxParamPropDefault: "pointer to OfxBytes struct of one dimension, or nullptr"
TEST_F(OfxBytesParamTest, Default)
{
    SKIP_IF_NO_PLUGIN();
    Bytes value;
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ(defaultBytes(), value);
    EXPECT_EQ( toString( defaultBytes() ), _knob->getDefaultValue(0) );

    ASSERT_TRUE( getBytes(paramHandle(_node, "bytesNoDefault"), &value) );
    EXPECT_TRUE( value.empty() );
}

// The plug-in changes the default of an instance (kOfxParamPropDefault, from a buffer on its stack):
// setDefault() copies it during the call.
TEST_F(OfxBytesParamTest, SetDefault)
{
    SKIP_IF_NO_PLUGIN();
    trigger(4);
    EXPECT_EQ( std::string("dyn"), _knob->getDefaultValue(0) );
    // the value is unchanged, the default is restored by a reset
    Bytes value;
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ(defaultBytes(), value);
    _knob->resetToDefaultValue(0);
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ( std::string("dyn"), toString(value) );
}

// A default larger than KnobBytes::maxLength() is ignored (with a warning), without reading the data.
TEST_F(OfxBytesParamTest, SetDefaultTooLarge)
{
    SKIP_IF_NO_PLUGIN();
    trigger(5);
    EXPECT_EQ( toString( defaultBytes() ), _knob->getDefaultValue(0) );
}

// The host copies the data (NUL bytes included), and gives the plug-in a pointer to its own copy.
TEST_F(OfxBytesParamTest, SetAndGet)
{
    SKIP_IF_NO_PLUGIN();
    Bytes data = allBytes();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, data) );
    const Bytes set = data;
    std::fill(data.begin(), data.end(), 0x55); // the plug-in may free or reuse its buffer
    Bytes value;
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ(set, value);
    // the knob holds the raw bytes
    EXPECT_EQ( toString(set), _knob->getValue(0) );

    OfxBytes b = { 0, 0 };
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetValue(_bytes, &b) );
    EXPECT_TRUE(b.data != &data[0]);
    // paramGetValueAtTime: Bytes params don't animate, the value is the same at all times
    OfxBytes atTime = { 0, 0 };
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetValueAtTime(_bytes, 10., &atTime) );
    ASSERT_EQ( set.size(), atTime.length );
    EXPECT_EQ( 0, std::memcmp(atTime.data, &set[0], set.size()) );
    // paramSetValueAtTime sets the value: no keyframe
    const unsigned char one[] = { 1 };
    OfxBytes oneB = { one, 1 };
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValueAtTime(_bytes, 5., &oneB) );
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ( Bytes(one, one + 1), value );
    unsigned int nKeys = 1;
    EXPECT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetNumKeys(_bytes, &nKeys) );
    EXPECT_EQ(0u, nKeys);
    EXPECT_FALSE( _knob->isAnimated(0) );
}

// "A NULL OfxBytes pointer, or a zero length, sets an empty value" (HostSupport ofxhParam.h)
TEST_F(OfxBytesParamTest, NullAndEmpty)
{
    SKIP_IF_NO_PLUGIN();
    Bytes value;
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(_bytes, (const OfxBytes*)0) );
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_TRUE( value.empty() );

    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, allBytes()) );
    OfxBytes empty = { kDefaultData, 0 };
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(_bytes, &empty) );
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_TRUE( value.empty() );

    // NULL data with a non-zero length is an error, and doesn't change the value
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, allBytes()) );
    OfxBytes bad = { 0, 4 };
    EXPECT_EQ( kOfxStatErrValue, NatronTestPlugin::parameterSuite()->paramSetValue(_bytes, &bad) );
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ(allBytes(), value);

    // a NULL OfxBytes for paramGetValue is an error
    EXPECT_EQ( kOfxStatErrValue, NatronTestPlugin::parameterSuite()->paramGetValue(_bytes, (OfxBytes*)0) );
}

// A value too large for the undo commands and the project (a Qt 5 QByteArray holds less than INT_MAX bytes)
// is refused with kOfxStatErrMemory, and the value is unchanged. The data is not read.
TEST_F(OfxBytesParamTest, TooLarge)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, allBytes()) );
    const unsigned char small[1] = { 0 };
    const size_t lengths[] = { KnobBytes::maxLength() + 1, (size_t)INT_MAX, (size_t)INT_MAX + 1 };
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
        OfxBytes huge = { small, lengths[i] };
        EXPECT_EQ( kOfxStatErrMemory, NatronTestPlugin::parameterSuite()->paramSetValue(_bytes, &huge) ) << lengths[i];
        EXPECT_EQ( kOfxStatErrMemory, NatronTestPlugin::parameterSuite()->paramSetValueAtTime(_bytes, 1., &huge) ) << lengths[i];
    }
    EXPECT_LT( KnobBytes::maxLength(), (size_t)INT_MAX );
    Bytes value;
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ(allBytes(), value);
}

// The plug-in sets and reads the value in instanceChanged.
TEST_F(OfxBytesParamTest, InstanceChanged)
{
    SKIP_IF_NO_PLUGIN();
    const int changed = NatronTestPlugin::counter("Bytes.instanceChanged");
    const int ok = NatronTestPlugin::counter("Bytes.readBackOK");
    EXPECT_EQ( 0, NatronTestPlugin::counter("Bytes.readBackFailed") );

    trigger(1);
    EXPECT_EQ( changed + 1, NatronTestPlugin::counter("Bytes.instanceChanged") );
    Bytes value;
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ(allBytes(), value);

    trigger(2);
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_TRUE( value.empty() );

    trigger(3);
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_TRUE( value.empty() );

    EXPECT_EQ( ok + 3, NatronTestPlugin::counter("Bytes.readBackOK") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("Bytes.readBackFailed") );
}

// The render action reads the value (Node::makePreviewImage() renders the node).
TEST_F(OfxBytesParamTest, RenderAction)
{
    SKIP_IF_NO_PLUGIN();
    const Bytes data = allBytes();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, data) );
    const int renders = NatronTestPlugin::counter("Bytes.render");
    int w = 16, h = 16;
    std::vector<unsigned int> buf(w * h);
    _node->makePreviewImage(0, &w, &h, &buf.front());
    EXPECT_GT( NatronTestPlugin::counter("Bytes.render"), renders );
    EXPECT_EQ( 0, NatronTestPlugin::counter("Bytes.renderFailed") );
    EXPECT_EQ( (int)data.size(), NatronTestPlugin::counter("Bytes.renderLength") );
    EXPECT_EQ( 255 * 256 / 2, NatronTestPlugin::counter("Bytes.renderChecksum") );
}

// The value is saved in the project with its own serialization (base64 text) and restored when it is loaded:
// save the node as in a project (XML), and create a node from it.
TEST_F(OfxBytesParamTest, SaveAndLoadNode)
{
    SKIP_IF_NO_PLUGIN();
    const Bytes data = allBytes();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, data) );
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramSetValue(paramHandle(_node, "bytesNoDefault"), (const OfxBytes*)0) );

    const std::string xml = saveNode(_node);
    // the XML holds the data as base64 text, whatever the bytes
    EXPECT_EQ( std::string::npos, xml.find('\0') );
    // saved as a String knob, which Natron 2.5 can read (see Natron25ReadsItAsAString)
    EXPECT_EQ( std::string::npos, xml.find("<Type>Bytes</Type>") );
    EXPECT_NE( std::string::npos, xml.find( KnobBytes::encodeForSerialization( toString(data) ) ) );
    NodeSerializationPtr loaded = loadNode(xml);
    destroy(_node);

    // the loaded knob is a KnobBytes again
    bool found = false;
    for (NodeSerialization::KnobValues::const_iterator it = loaded->getKnobsValues().begin(); it != loaded->getKnobsValues().end(); ++it) {
        if ( (*it)->getName() == "bytes" ) {
            found = true;
            EXPECT_TRUE( bool( std::dynamic_pointer_cast<KnobBytes>( (*it)->getKnob() ) ) );
        }
    }
    EXPECT_TRUE(found);

    NodePtr node = createBytesNode(loaded);
    ASSERT_TRUE( bool(node) );
    Bytes value;
    EXPECT_TRUE( getBytes(paramHandle(node, "bytes"), &value) );
    EXPECT_EQ(data, value);
    EXPECT_TRUE( getBytes(paramHandle(node, "bytesNoDefault"), &value) );
    EXPECT_TRUE( value.empty() );
    EXPECT_TRUE( getBytes(paramHandle(node, "bytesAnimates"), &value) );
    EXPECT_TRUE( value.empty() );
    destroy(node);
}

// A value that is not valid base64 in the project (a damaged file) gives the default, not garbage.
TEST_F(OfxBytesParamTest, LoadInvalidBase64)
{
    SKIP_IF_NO_PLUGIN();
    const Bytes data = allBytes();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, data) );
    std::string xml = saveNode(_node);
    const std::string encoded = KnobBytes::encodeForSerialization( toString(data) );
    const std::size_t pos = xml.find(encoded);
    ASSERT_NE(std::string::npos, pos);
    xml.replace( pos, encoded.size(), KnobBytes::encodeForSerialization( std::string() ) + "not*base64!" );
    NodeSerializationPtr loaded = loadNode(xml);
    destroy(_node);

    NodePtr node = createBytesNode(loaded);
    ASSERT_TRUE( bool(node) );
    Bytes value;
    EXPECT_TRUE( getBytes(paramHandle(node, "bytes"), &value) );
    EXPECT_EQ(defaultBytes(), value);
    destroy(node);
}

// Natron 2.5 and older have no Bytes knobs, and get out of step when they read a knob of an unknown type (their
// KnobSerialization::load() returns before reading its elements), so that the whole project fails to load. A
// KnobBytes is saved exactly as a non-animated String knob whose value is the text of
// KnobBytes::encodeForSerialization(): the elements that Natron 2.5 reads for a String knob (its
// KnobSerialization and ValueSerialization have the same String layout as Natron 2.6, at the same class
// versions), so it reads it as a String knob, whatever plug-in it belongs to.
TEST_F(OfxBytesParamTest, Natron25ReadsItAsAString)
{
    SKIP_IF_NO_PLUGIN();
    const Bytes data = allBytes();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, data) );
    // the String knob that Natron 2.5 would read
    KnobStringPtr str = std::make_shared<KnobString>( (KnobHolder*)NULL, std::string("bytes"), 1, false );
    str->populate();
    str->setName("bytes");
    str->setSecret( _knob->getIsSecret() );
    str->setEnabled( 0, _knob->isEnabled(0) );
    str->setDefaultValueWithoutApplying( KnobBytes::encodeForSerialization( _knob->getDefaultValue(0) ), 0 );
    str->setValue( KnobBytes::encodeForSerialization( toString(data) ), ViewSpec::all(), 0 );
    std::string xml[2];
    KnobIPtr knobs[2] = { _knob, str };
    for (int i = 0; i < 2; ++i) {
        std::stringstream ss;
        KnobSerialization s(knobs[i]);
        boost::archive::xml_oarchive oArchive(ss);
        oArchive << boost::serialization::make_nvp("Knob", s);
        xml[i] = ss.str();
    }
    EXPECT_EQ(xml[1], xml[0]);
    EXPECT_NE( std::string::npos, xml[0].find("<Type>String</Type>") );

    // read back with a following element (a knob of a node): Natron 2.6 makes a KnobBytes of it, and the
    // archive stays in step
    std::stringstream ss;
    {
        KnobSerialization s(_knob);
        KnobSerialization s2(str);
        int after = 42;
        boost::archive::xml_oarchive oArchive(ss);
        oArchive << boost::serialization::make_nvp("Knob", s);
        oArchive << boost::serialization::make_nvp("Knob2", s2);
        oArchive << boost::serialization::make_nvp("After", after);
    }
    KnobSerialization loaded, loaded2;
    int after = 0;
    {
        boost::archive::xml_iarchive iArchive(ss);
        iArchive >> boost::serialization::make_nvp("Knob", loaded);
        iArchive >> boost::serialization::make_nvp("Knob2", loaded2);
        iArchive >> boost::serialization::make_nvp("After", after);
    }
    EXPECT_EQ(42, after);
    KnobBytesPtr bytes = std::dynamic_pointer_cast<KnobBytes>( loaded.getKnob() );
    ASSERT_TRUE( bool(bytes) );
    EXPECT_EQ( toString(data), bytes->getValue(0) );
    EXPECT_EQ( toString( defaultBytes() ), bytes->getDefaultValue(0) );
    EXPECT_TRUE( bool( std::dynamic_pointer_cast<KnobBytes>( loaded2.getKnob() ) ) );
}

// A String knob whose text doesn't come from a KnobBytes stays a String knob.
TEST(OfxBytesParamSerialization, StringKnobsStayStrings)
{
    const char* values[] = { "text", "natron-bytes-base64", "", "AAEC" };
    for (int i = 0; i < 4; ++i) {
        KnobStringPtr knob = std::make_shared<KnobString>( (KnobHolder*)NULL, std::string("s"), 1, false );
        knob->populate();
        knob->setValue( std::string(values[i]), ViewSpec::all(), 0 );
        std::stringstream ss;
        {
            KnobSerialization s(knob);
            boost::archive::xml_oarchive oArchive(ss);
            oArchive << boost::serialization::make_nvp("Knob", s);
        }
        KnobSerialization loaded;
        {
            boost::archive::xml_iarchive iArchive(ss);
            iArchive >> boost::serialization::make_nvp("Knob", loaded);
        }
        KnobStringPtr loadedKnob = std::dynamic_pointer_cast<KnobString>( loaded.getKnob() );
        ASSERT_TRUE( bool(loadedKnob) ) << values[i];
        EXPECT_EQ( std::string(values[i]), loadedKnob->getValue(0) );
    }
}

// Raw bytes are not text: a KnobBytes and a String knob refuse to be copied into each other (Param.copy(), paste).
TEST_F(OfxBytesParamTest, NotCompatibleWithStrings)
{
    SKIP_IF_NO_PLUGIN();
    KnobStringPtr str = std::make_shared<KnobString>( (KnobHolder*)NULL, std::string("s"), 1, false );
    str->populate();
    KnobBytesPtr other = std::make_shared<KnobBytes>( (KnobHolder*)NULL, std::string("b"), 1, false );
    other->populate();
    EXPECT_FALSE( _knob->isTypeCompatible(str) );
    EXPECT_FALSE( str->isTypeCompatible(_knob) );
    EXPECT_TRUE( _knob->isTypeCompatible(other) );
    EXPECT_TRUE( str->isTypeCompatible(str) );
    EXPECT_FALSE( KnobI::areTypesCompatibleForSlave( _knob.get(), str.get() ) );
}

// The value is saved in a project (.ntp) and restored when the project is loaded.
TEST_F(OfxBytesParamTest, ProjectSaveAndLoad)
{
    SKIP_IF_NO_PLUGIN();
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const Bytes data = allBytes();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, data) );
    const std::string name = _node->getScriptName_mt_safe();
    _knob.reset();
    _node.reset();

    AppInstancePtr app = appPTR->getTopLevelInstance();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( app->getProject()->saveProject( dir, QString::fromUtf8("Bytes.ntp"), &savedPath ) );
    _projectLoaded = true;
    ASSERT_TRUE( app->getProject()->loadProject( dir, QString::fromUtf8("Bytes.ntp") ) );

    _node = app->getProject()->getNodeByName(name);
    ASSERT_TRUE( bool(_node) );
    Bytes value;
    EXPECT_TRUE( getBytes(paramHandle(_node, "bytes"), &value) );
    EXPECT_EQ(data, value);
}

// A reset to the default restores the plug-in's default.
TEST_F(OfxBytesParamTest, DefaultIsRestoredAfterReset)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, allBytes()) );
    _knob->resetToDefaultValue(0);
    Bytes value;
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ(defaultBytes(), value);
}

// Copy/paste and clone (KnobI::clone(), used by node copy/paste, links and the undo commands): raw bytes.
TEST_F(OfxBytesParamTest, Clone)
{
    SKIP_IF_NO_PLUGIN();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, allBytes()) );
    KnobBytesPtr copy = std::make_shared<KnobBytes>( (KnobHolder*)NULL, std::string("copy"), 1, false );
    copy->populate();
    copy->clone( _knob.get() );
    EXPECT_EQ( toString( allBytes() ), copy->getValue(0) );
    copy->setValue( std::string("x\0y", 3), ViewSpec::all(), 0 );
    _knob->clone( copy.get() );
    Bytes value;
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ( std::string("x\0y", 3), toString(value) );
}

// Undo: the undo commands of the settings panel (Gui/KnobUndoCommand.cpp) hold the old and new values in a
// Variant: a QByteArray with the raw bytes for KnobBytes (a QString would stop at the first NUL byte and
// re-encode the others). The undo stacks themselves are in the GUI, which the tests don't have.
TEST_F(OfxBytesParamTest, UndoValues)
{
    SKIP_IF_NO_PLUGIN();
    const std::string data = toString( allBytes() );
    Variant v;
    KnobBytes::stringToVariant(_knob.get(), data, &v);
    EXPECT_EQ( data, KnobBytes::stringFromVariant(_knob.get(), v) );
    // other string knobs are unchanged: a QString
    KnobStringPtr str = std::make_shared<KnobString>( (KnobHolder*)NULL, std::string("s"), 1, false );
    Variant s;
    KnobBytes::stringToVariant(str.get(), std::string("text"), &s);
    EXPECT_EQ( QString::fromUtf8("text"), s.toString() );
    EXPECT_EQ( std::string("text"), KnobBytes::stringFromVariant(str.get(), s) );
}

// Python: BytesParam (Effect.getParam() of a Bytes param), whose values are raw bytes (Python bytes).
TEST_F(OfxBytesParamTest, PythonParam)
{
    SKIP_IF_NO_PLUGIN();
    ::Natron::Python::BytesParam param(_knob);
    EXPECT_EQ( toString( defaultBytes() ), param.getValue() );
    EXPECT_EQ( toString( defaultBytes() ), param.getDefaultValue() );
    param.setValue( toString( allBytes() ) );
    Bytes value;
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_EQ(allBytes(), value);
    param.restoreDefaultValue();
    EXPECT_EQ( toString( defaultBytes() ), param.getValue() );
}

// The PyPlug export (Group "Export as Python plug-in") writes the value as Python bytes, which a string literal
// can't hold.
TEST_F(OfxBytesParamTest, PyPlugExport)
{
    SKIP_IF_NO_PLUGIN();
    const Bytes data = allBytes();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, data) );
    QString script;
    appPTR->getTopLevelInstance()->getProject()->exportGroupToPython( QString::fromUtf8("net.natron.test.BytesExport"), QString::fromUtf8("BytesExport"),
                                                                       QString(), QString(), QString::fromUtf8("Other"), 1, script );
    const QByteArray hex = QByteArray( (const char*)&data[0], (int)data.size() ).toHex();
    EXPECT_TRUE( script.contains( QString::fromUtf8("param.setValue(bytes.fromhex(\"") + QString::fromLatin1(hex) + QString::fromUtf8("\"))") ) )
        << script.toStdString();
    // no raw bytes in the script
    EXPECT_FALSE( script.contains( QChar(0) ) );
    EXPECT_GT( KnobBytes::maxExportLength(), (std::size_t)0 );
    EXPECT_LT( KnobBytes::maxExportLength() * 4, (std::size_t)INT_MAX );
}

// The Python bindings of BytesParam (Shiboken): bytes in and out, any bytes-like object in, TypeError for str.
TEST_F(OfxBytesParamTest, PythonBindings)
{
    SKIP_IF_NO_PLUGIN();
    const std::string node = appPTR->getTopLevelInstance()->getAppIDString() + "." + _node->getFullyQualifiedName();
    const std::string script =
        "p = " + node + ".getParam(\"bytes\")\n"
        "assert type(p).__name__ == \"BytesParam\", type(p)\n"
        "assert p.getDefaultValue() == b\"N\\x00a\\xff\\x00\", p.getDefaultValue()\n"
        "assert p.getValue() == p.getDefaultValue()\n"
        "d = bytes(range(256))\n"
        "p.setValue(d)\n"
        "v = p.getValue()\n"
        "assert type(v) is bytes and v == d, v\n"
        "p.setValue(bytearray(b\"x\\x00y\"))\n"
        "assert p.getValue() == b\"x\\x00y\"\n"
        "p.setValue(memoryview(b\"mv\"))\n"
        "assert p.getValue() == b\"mv\"\n"
        "try:\n"
        "    p.setValue(\"text\")\n"
        "    raise RuntimeError(\"no TypeError for str\")\n"
        "except TypeError:\n"
        "    pass\n"
        "assert p.getValue() == b\"mv\"\n"
        "p.restoreDefaultValue()\n"
        "assert p.getValue() == b\"N\\x00a\\xff\\x00\"\n"
        "p.setValue(b\"\")\n"
        "assert p.getValue() == b\"\"\n";
    std::string error, output;
    EXPECT_TRUE( ::Natron::Python::interpretPythonScript(script, &error, &output) ) << error << output;
    EXPECT_EQ( std::string(), error );
    Bytes value;
    ASSERT_TRUE( getBytes(_bytes, &value) );
    EXPECT_TRUE( value.empty() );
}

// The data returned to the plug-in is per thread: render threads may read the param at the same time, and
// the data returned to one thread stays valid until that thread's next call on the param (HostSupport's
// BytesInstance::getV returns a pointer to a member of the instance, which another thread would overwrite).
TEST_F(OfxBytesParamTest, PerThreadData)
{
    SKIP_IF_NO_PLUGIN();
    const Bytes first = allBytes();
    ASSERT_EQ( kOfxStatOK, setBytes(_bytes, first) );
    OfxBytes mine = { 0, 0 };
    ASSERT_EQ( kOfxStatOK, NatronTestPlugin::parameterSuite()->paramGetValue(_bytes, &mine) );

    // the value changes (e.g. the user edits it), and another thread reads it
    const Bytes second(300, 0x42);
    _knob->setValue( toString(second), ViewSpec::all(), 0 );
    OfxBytes other = { 0, 0 };
    OfxStatus otherStat = kOfxStatFailed;
    const OfxParamHandle param = _bytes;
    std::thread t([&]() {
        otherStat = NatronTestPlugin::parameterSuite()->paramGetValue(param, &other);
    });
    t.join();
    ASSERT_EQ(kOfxStatOK, otherStat);
    ASSERT_EQ( second.size(), other.length );
    EXPECT_EQ( 0, std::memcmp(other.data, &second[0], second.size()) );
    // the data of this thread is still the value it read
    EXPECT_TRUE(mine.data != other.data);
    ASSERT_EQ( first.size(), mine.length );
    EXPECT_EQ( 0, std::memcmp(mine.data, &first[0], first.size()) );

    // and concurrent reads all get the value
    int failures[2] = { 0, 0 };
    const unsigned char* pointers[2] = { 0, 0 };
    std::atomic<int> done(0);
    std::vector<std::thread> threads;
    for (int i = 0; i < 2; ++i) {
        threads.push_back( std::thread([&failures, &pointers, &second, &done, param, i]() {
            for (int n = 0; n < 2000; ++n) {
                OfxBytes b = { 0, 0 };
                if ( (NatronTestPlugin::parameterSuite()->paramGetValue(param, &b) != kOfxStatOK) ||
                     (b.length != second.size()) || (std::memcmp(b.data, &second[0], second.size()) != 0) ) {
                    ++failures[i];
                }
                pointers[i] = b.data;
            }
            // both threads are alive (and own their data) when the pointers are compared
            ++done;
            while (done < 2) {
                std::this_thread::yield();
            }
        }) );
    }
    for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    EXPECT_EQ(0, failures[0]);
    EXPECT_EQ(0, failures[1]);
    EXPECT_TRUE(pointers[0] != pointers[1]);
}

// The project text of a value is base64, and its decoding is the inverse of its encoding; invalid base64 is
// refused (AbortOnBase64DecodingErrors), not decoded to garbage.
TEST(OfxBytesParamEncoding, RoundTrip)
{
    const std::string data = toString( allBytes() );
    const std::string encoded = KnobBytes::encodeForSerialization(data);
    const std::string prefix = KnobBytes::encodeForSerialization( std::string() );
    EXPECT_EQ( std::string("natron-bytes-base64:"), prefix );
    ASSERT_EQ( 0, encoded.compare( 0, prefix.size(), prefix ) );
    EXPECT_EQ( std::string::npos, encoded.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=", prefix.size()) );
    EXPECT_TRUE( KnobBytes::isSerializedBytes(encoded) );
    EXPECT_FALSE( KnobBytes::isSerializedBytes("AAEC") );
    std::string decoded("garbage");
    EXPECT_TRUE( KnobBytes::decodeFromSerialization(encoded, &decoded) );
    EXPECT_EQ(data, decoded);

    EXPECT_TRUE( KnobBytes::decodeFromSerialization(prefix, &decoded) );
    EXPECT_TRUE( decoded.empty() );
    EXPECT_EQ( prefix + "AA==", KnobBytes::encodeForSerialization( std::string(1, '\0') ) );

    decoded = "garbage";
    EXPECT_FALSE( KnobBytes::decodeFromSerialization(prefix + "not*base64!", &decoded) );
    EXPECT_TRUE( decoded.empty() );
    // a text that was not made by encodeForSerialization()
    decoded = "garbage";
    EXPECT_FALSE( KnobBytes::decodeFromSerialization("AA==", &decoded) );
    EXPECT_TRUE( decoded.empty() );
}
