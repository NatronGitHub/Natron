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

#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QPointF>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThread>

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
#include "Engine/OverlaySupport.h"
#include "Engine/Project.h"
#include "Engine/RenderScale.h"
#include "Engine/Settings.h"
#include "Engine/ViewIdx.h"

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
    // HostSupport builds the paths with "\\" on Windows (DIRSEP), after a directory of OFX_PLUGIN_PATH
    // that may use "/"
    makeBundle(tmp.path(), QString::fromUtf8("Win"), "com.example.Win");
    const std::string winPath = ( tmp.path() + QString::fromUtf8("/Win.ofx.bundle\\Contents\\Win64\\Win.ofx") ).toStdString();
    EXPECT_EQ( std::string("com.example.Win"), NATRON_NAMESPACE::OfxHost::getBundleIdentifier(winPath) );
}

// The host name goes in the plug-in cache version, which HostSupport writes unescaped in an XML
// attribute: a host name with '"', '<' or '&' must not make the cache unreadable.
TEST(OfxHostProperties, PluginCacheVersion)
{
    const std::string v = NATRON_NAMESPACE::OfxHost::getPluginCacheVersion("My \"<host>\" & co");
    EXPECT_EQ( std::string::npos, v.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-") ) << v;
    EXPECT_EQ( v, NATRON_NAMESPACE::OfxHost::getPluginCacheVersion("My \"<host>\" & co") );
    EXPECT_NE( v, NATRON_NAMESPACE::OfxHost::getPluginCacheVersion("fr.inria.Natron") );
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
    // a binary with no bundle identifier: Natron's host name, not that of the plug-in loaded before
    host->loadingStatus(true, "com.absoft.neatvideo4", 4, 0);
    EXPECT_EQ( nuke, props.getStringProperty(kOfxPropName) );
    host->loadingBinary( makeBundle(tmp.path(), QString::fromUtf8("NoPlist2"), NULL) );
    EXPECT_EQ( natron, props.getStringProperty(kOfxPropName) );
    // the loading state is only that of the thread that loads the plug-ins: another thread (e.g. a
    // render thread) doesn't see it, and doesn't race with it
    host->loadingStatus(true, "com.absoft.neatvideo4", 4, 0);
    std::string otherThreadName;
    QThread* other = QThread::create([&props, &otherThreadName]() {
        otherThreadName = props.getStringProperty(kOfxPropName);
    });
    other->start();
    EXPECT_TRUE( other->wait() );
    delete other;
    EXPECT_EQ( natron, otherThreadName );
    EXPECT_EQ( nuke, props.getStringProperty(kOfxPropName) );
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
    // the scan calls OfxGetNumberOfPlugins; a binary known from the cache only gets OfxGetPlugin. With
    // NATRON_TESTS_KEEP_OFX_CACHE (TestsOfxCache in Tests/CMakeLists.txt), this proves that the plug-in
    // came from the cache, and was loaded at its first use.
    if ( qgetenv("NATRON_TESTS_KEEP_OFX_CACHE").isEmpty() ) {
        EXPECT_LE( 1, NatronTestPlugin::counter("Binary.getNumberOfPlugins") );
    } else {
        EXPECT_EQ( 0, NatronTestPlugin::counter("Binary.getNumberOfPlugins") );
    }
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

// The host name that a plug-in sees depends on the effect whose action is running on the thread
// (getHostNameProxy() in OfxHost.cpp). When an action returns, the host must go back to the action
// that called it on the same thread, if any, and to no effect at all after the outermost action:
// it used to keep the effect whose action returned last, and read its plug-in at the next host-name
// query, even after the effect was destroyed (#1497).
TEST(OfxHostProperties, ActionCallerStack)
{
    const NATRON_NAMESPACE::OfxHost* ofxHost = appPTR->getOFXHost();
    ASSERT_TRUE(ofxHost != NULL);
    NATRON_NAMESPACE::OfxHost::OfxHostDataTLSPtr tls = ofxHost->getTLSData();
    ASSERT_TRUE(tls != NULL);
    ASSERT_TRUE(tls->effectCallingMainEntry() == NULL);

    // only compared, never dereferenced
    char outerStorage, innerStorage;
    OfxImageEffectInstance* outer = reinterpret_cast<OfxImageEffectInstance*>(&outerStorage);
    OfxImageEffectInstance* inner = reinterpret_cast<OfxImageEffectInstance*>(&innerStorage);

    appPTR->setThreadAsActionCaller(outer, true);
    EXPECT_TRUE(tls->effectCallingMainEntry() == outer);
    // e.g. the render of an upstream effect, from clipGetImage in the action of outer
    appPTR->setThreadAsActionCaller(inner, true);
    EXPECT_TRUE(tls->effectCallingMainEntry() == inner);
    appPTR->setThreadAsActionCaller(inner, false);
    EXPECT_TRUE(tls->effectCallingMainEntry() == outer);
    appPTR->setThreadAsActionCaller(outer, false);
    EXPECT_TRUE(tls->effectCallingMainEntry() == NULL);
    EXPECT_EQ( 0, ofxHost->multiThreadIsSpawnedThread() );
}

// Create and destroy a node: no action is running any more, so the host-name query, outside actions,
// must not read the destroyed effect (#1497: it was a use after free).
TEST(OfxHostProperties, HostNameAfterEffectDestroyed)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParamInteract) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginParamInteract);

        return;
    }
    ASSERT_TRUE( createTestNode(kNatronTestPluginParamInteract) );
    NATRON_NAMESPACE::OfxHost::OfxHostDataTLSPtr tls = appPTR->getOFXHost()->getTLSData();
    ASSERT_TRUE(tls != NULL);
    EXPECT_TRUE(tls->effectCallingMainEntry() == NULL);
    EXPECT_EQ( appPTR->getCurrentSettings()->getHostName(), hostProps().getStringProperty(kOfxPropName) );
}

namespace {
// Tests/OfxTestPlugin/NatronTestPluginHostName.cpp: its ID gets the Nuke host name (getHostNameProxy())
#define kNatronTestPluginHostName "com.digitalfilmtools.NatronTestHostName"

// A viewport that does not draw anything: only what an OFX overlay interact queries.
class HostNameViewport
    : public OverlaySupport
{
public:
    virtual void swapOpenGLBuffers() OVERRIDE FINAL {}

    virtual void redraw() OVERRIDE FINAL {}

    virtual void getViewportSize(double &width,
                                 double &height) const OVERRIDE FINAL
    {
        width = 800.;
        height = 600.;
    }

    virtual void getPixelScale(double & xScale,
                               double & yScale) const OVERRIDE FINAL
    {
        xScale = yScale = 1.;
    }

#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE FINAL
    {
        return 1.;
    }

#endif

    virtual void getBackgroundColour(double &r,
                                     double &g,
                                     double &b) const OVERRIDE FINAL
    {
        r = g = b = 0.;
    }

    virtual void saveOpenGLContext() OVERRIDE FINAL {}

    virtual void restoreOpenGLContext() OVERRIDE FINAL {}

    virtual RectD getViewportRect() const OVERRIDE FINAL
    {
        return RectD(0., 0., 800., 600.);
    }

    virtual void getCursorPosition(double& x,
                                   double& y) const OVERRIDE FINAL
    {
        x = y = 0.;
    }

    virtual void toWidgetCoordinates(double * /*x*/,
                                     double * /*y*/) const OVERRIDE FINAL {}

    virtual void toCanonicalCoordinates(double * /*x*/,
                                        double * /*y*/) const OVERRIDE FINAL {}

    virtual int getWidgetFontHeight() const OVERRIDE FINAL
    {
        return 10;
    }

    virtual int getStringWidthForCurrentFont(const std::string& /*string*/) const OVERRIDE FINAL
    {
        return 10;
    }
};
} // anonymous namespace

// A plug-in gets its own host name (here, Nuke's) in the actions of its overlay interact, which don't go
// through the effect's mainEntry(), and in the thread functions of the multithread suite, in spawned threads
// as in the calling thread (#1497). Outside actions, it is Natron's.
TEST(OfxHostProperties, HostNameInInteractAndThreads)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginHostName) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginHostName);

        return;
    }
    const std::string nuke = appPTR->getCurrentSettings()->getKnownHostName(Settings::eKnownHostNameNuke);
    const std::string natron = appPTR->getCurrentSettings()->getHostName();
    ASSERT_NE(nuke, natron);
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    const std::string createInstance = "HostName.createInstance.name.";
    const int createInstanceNuke = NatronTestPlugin::counter(createInstance + nuke);
    CreateNodeArgs args( kNatronTestPluginHostName, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    // in an effect action: if this fails, the plug-in ID no longer gets the Nuke host name (getHostNameProxy())
    ASSERT_EQ( createInstanceNuke + 1, NatronTestPlugin::counter(createInstance + nuke) );
    EffectInstancePtr effect = node->getEffectInstance();
    ASSERT_TRUE( effect->hasOverlay() );
    HostNameViewport viewport;
    effect->setCurrentViewportForOverlays_public(&viewport);

    const std::string penDown = "HostName.penDown.name.";
    const std::string threaded = "HostName.threaded.name.";
    const std::string sequential = "HostName.sequential.name.";
    // both multiThread() paths: the thread pool (where QtConcurrent may run a thread function in the calling
    // thread, when waitForFinished() steals it) and one new thread per thread function (OfxThread)
    const bool oldUseThreadPool = appPTR->getUseThreadPool();
    for (int useThreadPool = 1; useThreadPool >= 0; --useThreadPool) {
        SCOPED_TRACE(useThreadPool ? "thread pool" : "new threads");
        appPTR->setUseThreadPool( (bool)useThreadPool );
        const int penDownNuke = NatronTestPlugin::counter(penDown + nuke);
        const int penDownNatron = NatronTestPlugin::counter(penDown + natron);
        const int threadedNuke = NatronTestPlugin::counter(threaded + nuke);
        const int threadedNatron = NatronTestPlugin::counter(threaded + natron);
        const int sequentialNuke = NatronTestPlugin::counter(sequential + nuke);
        const int sequentialNatron = NatronTestPlugin::counter(sequential + natron);
        const int failed = NatronTestPlugin::counter("HostName.multiThread.failed");
        const int spawned = NatronTestPlugin::counter("HostName.threaded.spawned");
        const int otherThread = NatronTestPlugin::counter("HostName.threaded.otherThread");
        const int numCPUs = NatronTestPlugin::counter("HostName.multiThread.numCPUs");
        ignore_result( effect->onOverlayPenDown_public( 1., RenderScale(), ViewIdx(0), QPointF(10., 20.), QPointF(0., 0.), 1., 0., ePenTypeLMB, KeyboardModifiers() ) );
        EXPECT_EQ( failed, NatronTestPlugin::counter("HostName.multiThread.failed") );
        EXPECT_EQ( penDownNuke + 1, NatronTestPlugin::counter(penDown + nuke) );
        EXPECT_EQ( penDownNatron, NatronTestPlugin::counter(penDown + natron) );
        // 2 thread functions (one after the other in the calling thread if only one CPU is used)
        EXPECT_EQ( threadedNuke + 2, NatronTestPlugin::counter(threaded + nuke) );
        EXPECT_EQ( threadedNatron, NatronTestPlugin::counter(threaded + natron) );
        EXPECT_EQ( sequentialNuke + 1, NatronTestPlugin::counter(sequential + nuke) );
        EXPECT_EQ( sequentialNatron, NatronTestPlugin::counter(sequential + natron) );
        // the thread functions are spawned threads for the suite (also when they run in the calling thread)
        EXPECT_EQ( spawned + 2, NatronTestPlugin::counter("HostName.threaded.spawned") );
        // with more than one CPU for the multithread suite and new threads, they ran in other threads than the
        // calling one: their host name came from the calling thread
        const int callNumCPUs = NatronTestPlugin::counter("HostName.multiThread.numCPUs") - numCPUs;
        if ( !useThreadPool && (callNumCPUs > 1) ) {
            EXPECT_EQ( otherThread + 2, NatronTestPlugin::counter("HostName.threaded.otherThread") );
        } else if ( !useThreadPool ) {
            std::cerr << "OfxHostProperties.HostNameInInteractAndThreads: one CPU for the multithread suite, spawned threads not tested." << std::endl;
        }
    }
    appPTR->setUseThreadPool(oldUseThreadPool);

    effect->setCurrentViewportForOverlays_public(NULL);
    effect.reset();
    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
    NATRON_NAMESPACE::OfxHost::OfxHostDataTLSPtr tls = appPTR->getOFXHost()->getTLSData();
    EXPECT_TRUE(tls->effectCallingMainEntry() == NULL);
    EXPECT_EQ( natron, hostProps().getStringProperty(kOfxPropName) );
}
