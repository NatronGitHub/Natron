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

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>

#include <ofxCore.h>
#include <ofxColour.h>
#include <ofxGPURender.h>
#include <ofxImageEffect.h>
#include <ofxParam.h>
#include <ofxhImageEffect.h>
#include <ofxhPropertySuite.h>

#include "Global/GlobalDefines.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/OfxHost.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "OfxTestPlugin.h"

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
}

namespace {
// make <dir>/<name>.ofx.bundle/Contents/MacOS/<name>.ofx (empty) and its Info.plist, and return the binary path
std::string
makeBundle(const QString& dir, const QString& name, const char* bundleIdentifier)
{
    const QString bundle = dir + QLatin1Char('/') + name + QString::fromUtf8(".ofx.bundle");
    QDir().mkpath( bundle + QString::fromUtf8("/Contents/MacOS") );
    if (bundleIdentifier) {
        QFile plist( bundle + QString::fromUtf8("/Contents/Info.plist") );
        if ( plist.open(QIODevice::WriteOnly) ) {
            plist.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<plist version=\"1.0\">\n<dict>\n"
                        "\t<key>CFBundleExecutable</key>\n\t<string>x.ofx</string>\n"
                        "\t<key>CFBundleIdentifier</key>\n\t<string>");
            plist.write(bundleIdentifier);
            plist.write("</string>\n</dict>\n</plist>\n");
        }
    }

    return ( bundle + QString::fromUtf8("/Contents/MacOS/") + name + QString::fromUtf8(".ofx") ).toStdString();
}
}

TEST(OfxHostProperties, BundleIdentifier)
{
    QTemporaryDir tmp;
    ASSERT_TRUE( tmp.isValid() );
    EXPECT_EQ( std::string("com.example.Foo"), NATRON_NAMESPACE::OfxHost::getBundleIdentifier( makeBundle(tmp.path(), QString::fromUtf8("Foo"), "com.example.Foo") ) );
    EXPECT_EQ( std::string(), NATRON_NAMESPACE::OfxHost::getBundleIdentifier( makeBundle(tmp.path(), QString::fromUtf8("NoPlist"), NULL) ) );
    EXPECT_EQ( std::string(), NATRON_NAMESPACE::OfxHost::getBundleIdentifier("/not/a/bundle/Foo.ofx") );
}

// OfxSetHost (OFX 1.5) is called with the host before any plug-in of the binary is known: the host
// name it sees follows the binary's bundle identifier (see getHostNameProxy() in OfxHost.cpp), not
// the plug-in that was loaded before.
TEST(OfxHostProperties, HostNameAtOfxSetHost)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    OFX::Host::ImageEffect::Host* host = OFX::Host::ImageEffect::gImageEffectHost;
    OFX::Host::Property::Set& props = hostProps();
    QTemporaryDir tmp;
    ASSERT_TRUE( tmp.isValid() );
    const std::string nuke = appPTR->getCurrentSettings()->getKnownHostName(Settings::eKnownHostNameNuke);
    const std::string natron = appPTR->getCurrentSettings()->getHostName();

    // a plug-in that was being loaded, from another binary (e.g. the last one of the scan)
    host->loadingStatus(true, "com.absoft.neatvideo4", 4, 0);
    EXPECT_EQ( nuke, props.getStringProperty(kOfxPropName) );
    host->loadingBinary( makeBundle(tmp.path(), QString::fromUtf8("Other"), "org.example.Other") );
    EXPECT_EQ( natron, props.getStringProperty(kOfxPropName) );
    // a Neat Video binary: the Nuke host name, as for its plug-ins
    host->loadingBinary( makeBundle(tmp.path(), QString::fromUtf8("NeatVideo"), "com.absoft.neatvideo5") );
    EXPECT_EQ( nuke, props.getStringProperty(kOfxPropName) );
    // the next plug-in being loaded replaces the binary
    host->loadingStatus(true, "org.example.Plugin", 1, 0);
    EXPECT_EQ( natron, props.getStringProperty(kOfxPropName) );
    host->loadingStatus(false, "", 0, 0);
}

namespace {
// create and destroy a node of a test plug-in, so that its binary is loaded and the plug-in is
// described, even if it was only known from the plug-in cache
bool
createTestNode(const char* pluginID)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    if (!app) {
        return false;
    }
    CreateNodeArgs args( pluginID, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    NodePtr node = app->createNode(args);
    if (!node) {
        return false;
    }
    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);

    return true;
}
}

// OFX 1.5 (ofxCore.h): "The order of calls is then: 1) OfxSetHost, 2) OfxGetNumberOfPlugins, 3)
// OfxGetPlugin". The test plug-in binary exports OfxSetHost. Natron keeps the scanned binaries loaded
// (OfxHost::keepPluginsLoadedAfterScan()), so the binary is loaded once, and OfxSetHost is called once,
// at the scan or, if the binary was only known from the plug-in cache, at the first use of one of its
// plug-ins. Its host sees API version 1.5.
TEST(OfxHostProperties, OfxSetHostOncePerBinary)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParamInteract) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginParamInteract);

        return;
    }
    // two plug-ins of the same binary
    ASSERT_TRUE( createTestNode(kNatronTestPluginParamInteract) );
    ASSERT_TRUE( createTestNode(kNatronTestPluginSingleInstance) );
    EXPECT_EQ( 1, NatronTestPlugin::counter("Binary.setHost") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("Binary.setHostBeforeGetNumberOfPlugins") );
    EXPECT_LE( 1, NatronTestPlugin::counter("Binary.getNumberOfPlugins") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("Binary.setHostAPIVersion0") );
    EXPECT_EQ( 5, NatronTestPlugin::counter("Binary.setHostAPIVersion1") );
}

// clipGetHandle() is "not a valid call in any of the describe actions": HostSupport used the descriptor
// as an instance (#1366). It must return kOfxStatErrBadHandle and set the clip to NULL.
TEST(OfxHostProperties, ClipGetHandleInDescribe)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParamInteract) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginParamInteract);

        return;
    }
    // describes the plug-in in its context, if no other test did
    ASSERT_TRUE( createTestNode(kNatronTestPluginParamInteract) );
    EXPECT_LE( 1, NatronTestPlugin::counter("ParamInteract.describeClipGetHandleBadHandle") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("ParamInteract.describeClipGetHandleOtherStatus") );
    EXPECT_LE( 1, NatronTestPlugin::counter("ParamInteract.describeClipGetHandleNull") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("ParamInteract.describeClipGetHandleNotNull") );
}

// The host descriptor properties are read only for plug-ins ("Properties on the Image Effect Host",
// ofxPropertiesByObject.rst), and HostSupport's private host pointer, which fetchSuite() dereferences
// for every plug-in, can never be written by a plug-in (#1367).
TEST(OfxHostProperties, HostPropertiesReadOnly)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    OFX::Host::Property::Set& set = hostProps();
    ::OfxHost* handle = OFX::Host::ImageEffect::gImageEffectHost->getHandle();
    ASSERT_TRUE(handle != NULL);
    const OfxPropertySuiteV1* props = (const OfxPropertySuiteV1*)handle->fetchSuite(handle->host, kOfxPropertySuite, 1);
    ASSERT_TRUE(props != NULL);

    const char* const readOnly[] = {
        kOfxPropAPIVersion, kOfxPropType, kOfxPropName, kOfxPropLabel, kOfxPropVersion, kOfxPropVersionLabel
    };
    for (std::size_t i = 0; i < sizeof(readOnly) / sizeof(readOnly[0]); ++i) {
        OFX::Host::Property::Property* p = set.fetchProperty(readOnly[i]);
        ASSERT_TRUE(p != NULL) << readOnly[i];
        EXPECT_TRUE( p->getPluginReadOnly() ) << readOnly[i];
    }

    // the private host pointer: refused whatever the policy (Natron uses ePluginReadOnlyWarn)
    EXPECT_EQ( kOfxStatErrValue, props->propSetPointer(handle->host, kOfxHostSupportHostPointer, 0, (void*)&props) );
    EXPECT_EQ( kOfxStatErrValue, props->propReset(handle->host, kOfxHostSupportHostPointer) );
    EXPECT_TRUE( handle->fetchSuite(handle->host, kOfxPropertySuite, 1) == props );

    // with the enforce policy, a plug-in can't rename the host
    const std::string label = set.getStringProperty(kOfxPropLabel);
    OFX::Host::Property::PluginReadOnlyPolicyEnum policy = OFX::Host::Property::getPluginReadOnlyPolicy();
    OFX::Host::Property::setPluginReadOnlyPolicy(OFX::Host::Property::ePluginReadOnlyEnforce);
    EXPECT_EQ( kOfxStatErrValue, props->propSetString(handle->host, kOfxPropLabel, 0, "x") );
    OFX::Host::Property::setPluginReadOnlyPolicy(policy);
    EXPECT_EQ( label, set.getStringProperty(kOfxPropLabel) );
}
