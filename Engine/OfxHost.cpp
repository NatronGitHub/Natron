/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "OfxHost.h"

#include <atomic>
#include <cassert>
#include <cstdarg>
#include <cstdint> // std::uint64_t
#include <memory>
#include <fstream>
#include <new> // std::bad_alloc
#include <stdexcept> // std::exception
#include <cctype> // tolower
#include <algorithm> // transform, min, max
#include <string>
#include <cstring> // for std::memcpy, std::memset, std::strcmp

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
// clang-format off
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QThreadPool>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTemporaryFile>
CLANG_DIAG_ON(deprecated-register)
// clang-format on
#ifdef OFX_SUPPORTS_MULTITHREAD
#include <QThread>
#include <QThreadStorage>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#endif
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

//ofx
#include <ofxParametricParam.h>
#include <ofxOpenGLRender.h>
#ifdef OFX_EXTENSIONS_NUKE
#include <nuke/fnOfxExtensions.h>
#endif
#include <ofxInteract.h>
#include <ofxNatron.h>

//ofx host support
#include <ofxhPluginAPICache.h>
// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
// clang-format off
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
// clang-format on
#include <ofxhPluginCache.h>
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>
#include <ofxhParam.h>

#include <tuttle/ofxReadWrite.h>
#include <ofxhPluginCache.h>
#include <ofxhParametricParam.h> //our version of parametric param suite support

#include "Global/GlobalDefines.h"
#include "Global/StrUtils.h"
#include "Global/FStreamsSupport.h"
#include "Global/QtCompat.h"
#include "Global/KeySymbols.h"
#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/MemoryInfo.h" // printAsRAM
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/OfxMemory.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/StandardPaths.h"
#include "Engine/TLSHolder.h"
#include "Engine/ThreadPool.h"

//An effect may not use more than this amount of threads
#define NATRON_MULTI_THREAD_SUITE_MAX_NUM_CPU 4

NATRON_NAMESPACE_ENTER
// to disambiguate with the global-scope ::OfxHost


#ifdef OFX_SUPPORTS_MULTITHREAD
// A mutex of the multithread suite: the OfxMutexHandle is its address. A suite function returns
// kOfxStatErrBadHandle "if the handle was bad" (ofxMultiThread.h): the magic number tells a handle that
// mutexCreate() returned from any other pointer, and from a destroyed mutex, as the property suite does
// with the magic number of its property sets (OFX::Host::Property::Set::verifyMagic()). The magic is
// atomic so that the store in the destructor is not optimized away.
struct OfxSuiteMutex
{
    static const std::uint64_t kMagic = 0x4f66784d75746578ULL; // "OfxMutex"

    std::atomic<std::uint64_t> magic;
    QRecursiveMutex mutex;

    OfxSuiteMutex()
        : magic(kMagic)
        , mutex()
    {
    }

    ~OfxSuiteMutex()
    {
        magic = 0;
    }
};

// The mutex of a suite handle, or NULL if the handle is NULL or not a live mutex of the suite.
static OfxSuiteMutex*
suiteMutexFromHandle(OfxMutexHandle handle)
{
    OfxSuiteMutex* m = reinterpret_cast<OfxSuiteMutex*>(handle);

    return ( m && (m->magic == OfxSuiteMutex::kMagic) ) ? m : nullptr;
}
#endif

struct OfxHostPrivate
{
    OFX::Host::ImageEffect::PluginCachePtr imageEffectPluginCache;
    std::shared_ptr<TLSHolder<OfxHost::OfxHostTLSData> > tlsData;

#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
    std::list<OfxSuiteMutex*> pluginsMutexes;
    QMutex* pluginsMutexesLock; //<protects _pluginsMutexes
#endif
    // The state of the plug-in loading that the host name hook (getStringProperty(kOfxPropName)) reads.
    // Only the thread that loads plug-ins (loadingThread) writes and reads the members below: the scan
    // (loadOFXPlugins(), loadingStatus(), loadingBinary()), or getPluginContextAndDescribe(), under
    // describeMutex. The other threads (renders...) only read loadingThread, which is never theirs, and
    // use the plug-in of the effect that is calling the action instead.
    std::atomic<QThread*> loadingThread;
    std::string loadingPluginID; // ID of the plugin being loaded
    int loadingPluginVersionMajor;
    int loadingPluginVersionMinor;
    bool settingHost; // the OfxSetHost (OFX 1.5) of a binary is being called, see loadingBinary()
    std::string setHostBundleID; // if settingHost, the bundle identifier of the binary, or ""
    // Serializes the loading and description of the plug-ins after the scan (getPluginContextAndDescribe()):
    // HostSupport requires that the getPluginHandle() call that loads a plug-in (OfxSetHost of its binary if
    // the binary was only known from the cache, kOfxActionLoad, kOfxActionDescribe) and getContext()
    // (kOfxImageEffectActionDescribeInContext) be serialized with every other getPluginHandle(),
    // getContext() and createInstance() call for all the plug-ins of the binary. The other callers are the
    // instances, which exist only once their plug-in is loaded and described: they only call
    // getPluginHandle() on a loaded plug-in, which only reads, and nothing is unloaded before exit
    // (keepPluginsLoadedAfterScan()).
    QMutex describeMutex;
    std::atomic<QThread*> describeThread; // the thread that holds describeMutex, or NULL
    std::atomic<bool> pluginsUnloaded; // unloadPlugins() was called

    void clearLoadingState()
    {
        loadingPluginID.clear();
        loadingPluginVersionMajor = 0;
        loadingPluginVersionMinor = 0;
        settingHost = false;
        setHostBundleID.clear();
        loadingThread = nullptr;
    }

    OfxHostPrivate()
        : imageEffectPluginCache()
        , tlsData( new TLSHolder<OfxHost::OfxHostTLSData>() )
#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
        , pluginsMutexes()
        , pluginsMutexesLock(0)
#endif
        , loadingThread(nullptr)
        , loadingPluginID()
        , loadingPluginVersionMajor(0)
        , loadingPluginVersionMinor(0)
        , settingHost(false)
        , setHostBundleID()
        , describeMutex()
        , describeThread(nullptr)
        , pluginsUnloaded(false)
    {
    }
};

OfxHost::OfxHost()
    : _imp( new OfxHostPrivate() )
{
    // Plug-in writes to properties that are read-only for plug-ins (e.g. a clip's
    // kOfxImageClipPropDepth in contexts where Natron can't handle multiple depths) were
    // silently accepted before openfx #91: warn instead of failing the action, so that
    // existing third-party plug-ins keep working.
    OFX::Host::Property::setPluginReadOnlyPolicy(OFX::Host::Property::ePluginReadOnlyWarn);
    _imp->imageEffectPluginCache = std::make_shared<OFX::Host::ImageEffect::PluginCache>((OFX::Host::ImageEffect::Host*)this);
}

OfxHost::~OfxHost()
{
    //Clean up, to be polite.
    unloadPlugins();

#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
    delete _imp->pluginsMutexesLock;
#endif
}

OfxHost::OfxHostDataTLSPtr
OfxHost::getTLSData() const
{
    return _imp->tlsData->getOrCreateTLSData();
}

void
OfxHost::setOfxHostOSHandle(void* handle)
{
    _properties.setPointerProperty(kOfxPropHostOSHandle, handle);
}

void
OfxHost::setProperties()
{
    /* Known OpenFX host names:
       uk.co.thefoundry.nuke
       com.eyeonline.Fusion
       com.sonycreativesoftware.vegas
       Autodesk Toxik
       Assimilator
       Dustbuster
       DaVinciResolve
       DaVinciResolveLite
       Mistika
       com.apple.shake
       Baselight
       IRIDAS Framecycler
       com.chinadigitalvideo.dx
       com.newblue.titlerpro
       Ramen
       TuttleOfx
       fr.inria.Natron

       Other possible names:
       Nuke
       Autodesk Toxik Render Utility
       Autodesk Toxik Python Bindings
       Toxik
       Fusion
       film master
       film cutter
       data conform
       nucoda
       phoenix
       Film Master
       Baselight
       Scratch
       DS OFX Host
       Avid DS
       Vegas
       CDV DX
       Resolve

     */

    // see hostStuffs in ofxhImageEffect.cpp

    _properties.setStringProperty( kOfxPropName, appPTR->getCurrentSettings()->getHostName() );
    _properties.setGetHook(kOfxPropName, this);
    _properties.setStringProperty(kOfxPropLabel, NATRON_APPLICATION_NAME); // "nuke" //< use this to pass for nuke
    // OpenFX API v1.5 (1.5.1). The 1.5 features are opt-ins that each have their own host property or
    // suite, so plug-ins must not rely on the version number alone. Natron supports the DrawSuite
    // (supportsDrawSuite()) and Bytes params (kOfxHostSupportHostPropSupportsBytes, below); HostSupport
    // declares the others (colour management, StrChoice params, CUDA/Metal/OpenCL render...) as
    // "not supported" until Natron implements them.
    _properties.setIntProperty(kOfxPropAPIVersion, 1, 0);
    _properties.setIntProperty(kOfxPropAPIVersion, 5, 1);
    _properties.setIntProperty(kOfxPropVersion, NATRON_VERSION_MAJOR, 0);
    _properties.setIntProperty(kOfxPropVersion, NATRON_VERSION_MINOR, 1);
    _properties.setIntProperty(kOfxPropVersion, NATRON_VERSION_REVISION, 2);
    _properties.setStringProperty(kOfxPropVersionLabel, NATRON_VERSION_STRING);
    _properties.setIntProperty( kOfxImageEffectHostPropIsBackground, (int)appPTR->isBackground() );
    _properties.setIntProperty(kOfxImageEffectPropSupportsOverlays, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultiResolution, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsTiles, 1);
    _properties.setIntProperty(kOfxImageEffectPropTemporalClipAccess, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentRGBA, 0);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentAlpha, 1);
    if ( appPTR->getCurrentSettings()->areRGBPixelComponentsSupported() ) {
        _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentRGB, 2);
    }
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kFnOfxImageComponentMotionVectors, 3);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kFnOfxImageComponentStereoDisparity, 4);
#ifdef OFX_EXTENSIONS_NATRON
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kNatronOfxImageComponentXY, 5);
#endif
    _properties.setStringProperty(kOfxImageEffectPropSupportedPixelDepths, kOfxBitDepthFloat, 0);
    _properties.setStringProperty(kOfxImageEffectPropSupportedPixelDepths, kOfxBitDepthShort, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedPixelDepths, kOfxBitDepthByte, 2);

    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGenerator, 0 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextFilter, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGeneral, 2 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextTransition, 3 );

    ///Setting these makes The Foundry Furnace plug-ins fail in the load action
    //_properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextReader, 4 );
    //_properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextWriter, 5 );

    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipDepths, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipPARs, 1);
    _properties.setIntProperty(kOfxImageEffectPropSetableFrameRate, 1);
    _properties.setIntProperty(kOfxImageEffectPropSetableFielding, 0);
    _properties.setIntProperty(kOfxParamHostPropSupportsCustomInteract, 1 );
    _properties.setIntProperty( kOfxParamHostPropSupportsStringAnimation, KnobString::canAnimateStatic() );
    _properties.setIntProperty( kOfxParamHostPropSupportsChoiceAnimation, KnobChoice::canAnimateStatic() );
    _properties.setIntProperty( kOfxParamHostPropSupportsBooleanAnimation, KnobBool::canAnimateStatic() );
    _properties.setIntProperty( kOfxParamHostPropSupportsCustomAnimation, KnobString::canAnimateStatic() );
    // OFX 1.5 Bytes params (OfxBytesInstance). The standard has no host property for them (plug-ins learn
    // that a host doesn't support them from paramDefine): this HostSupport property lets paramDefine accept them.
    _properties.setIntProperty(kOfxHostSupportHostPropSupportsBytes, 1);
    _properties.setPointerProperty(kOfxPropHostOSHandle, NULL);
    _properties.setIntProperty(kOfxParamHostPropSupportsParametricAnimation, 0);

    _properties.setIntProperty(kOfxParamHostPropMaxParameters, -1);
    _properties.setIntProperty(kOfxParamHostPropMaxPages, 0);
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 0 );
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 1 );
    _properties.setIntProperty(kOfxImageEffectInstancePropSequentialRender, 2); // OFX 1.2
#ifdef OFX_SUPPORTS_OPENGLRENDER
    // all host properties should be static and not depend on the settings, or the plugin cache may be wrong
    _properties.setStringProperty(kOfxImageEffectPropOpenGLRenderSupported, "true"); // OFX 1.3
    //if (appPTR->getCurrentSettings()->isOpenGLRenderingEnabled()) {
    //    _properties.setStringProperty(kOfxImageEffectPropOpenGLRenderSupported, "true"); // OFX 1.3
    //} else {
    //    _properties.setStringProperty(kOfxImageEffectPropOpenGLRenderSupported, "false"); // OFX 1.3
    //}
#endif
    _properties.setIntProperty(kOfxImageEffectPropRenderQualityDraft, 1); // OFX 1.4
    _properties.setStringProperty(kOfxImageEffectHostPropNativeOrigin, kOfxHostNativeOriginBottomLeft); // OFX 1.4

#ifdef OFX_EXTENSIONS_NUKE
    ///Plane suite
    _properties.setIntProperty(kFnOfxImageEffectPropMultiPlanar, 1);
    ///Nuke transform suite
    _properties.setIntProperty(kFnOfxImageEffectCanTransform, 1);
#endif
#ifdef OFX_EXTENSIONS_NATRON
    ///Natron extensions
    _properties.setIntProperty(kNatronOfxHostIsNatron, 1);
    _properties.setIntProperty(kNatronOfxParamHostPropSupportsDynamicChoices, 1);
    _properties.setIntProperty(kNatronOfxParamPropChoiceCascading, 1);
    _properties.setStringProperty(kNatronOfxImageEffectPropChannelSelector, kOfxImageComponentRGBA);
    _properties.setIntProperty(kNatronOfxImageEffectPropHostMasking, 1);
    _properties.setIntProperty(kNatronOfxImageEffectPropHostMixing, 1);

    _properties.setIntProperty(kNatronOfxPropDescriptionIsMarkdown, 1);

    // keyboard modifiers in the interact actions (ofxNatron.h extension 13): HostSupport then
    // declares kNatronOfxInteractPropKeyModifiers in the inArgs of the actions of the interacts
    // whose supportsKeyModifiers() returns true (OfxOverlayInteract).
    _properties.setIntProperty(kNatronOfxInteractHostPropSupportsKeyModifiers, 1);

    // interact cursors (NatronGitHub/openfx#4): the CSS cursor names Natron can display,
    // plus custom PNG cursors from the plug-in resources
    {
        std::vector<std::string> cursors;
        OfxImageEffectInstance::getSupportedOfxCursors(&cursors);
        cursors.push_back(kOfxInteractCursorCustom);
        for (std::size_t i = 0; i < cursors.size(); ++i) {
            _properties.setStringProperty(kOfxInteractHostPropSupportedCursors, cursors[i], (int)i);
        }
    }
#endif
} // OfxHost::setProperties

PluginOpenGLRenderSupport
OfxHost::applyCPURenderSupport(PluginOpenGLRenderSupport openGLSupport,
                               const OFX::Host::Property::Set& effectProps)
{
    // HostSupport declares kOfxImageEffectPropCPURenderSupported on every descriptor, and so on every
    // instance, with its spec default "true". Natron can't render with CUDA, Metal or OpenCL, so a
    // plug-in that supports neither the CPU nor OpenGL keeps ePluginOpenGLRenderSupportNone.
    if ( (openGLSupport == ePluginOpenGLRenderSupportYes) &&
         (effectProps.getStringProperty(kOfxImageEffectPropCPURenderSupported) == "false") ) {
        return ePluginOpenGLRenderSupportNeeded;
    }

    return openGLSupport;
}

bool
OfxHost::isPluginObsolete(const OFX::Host::Property::Set& descriptorProps)
{
    // kOfxImageEffectPluginPropObsolete: "available for use in old projects but not offered to users for
    // new use", which is what Natron does with these plug-ins (see Plugin::getIsUserCreatable()).
    // HostSupport declares it, with a default of 0, on every image effect descriptor, including the
    // ones loaded from the plug-in cache.
    return descriptorProps.getIntProperty(kOfxImageEffectPluginPropObsolete) != 0;
}

static
Settings::KnownHostNameEnum
getHostNameProxy(const std::string& pluginID,
                 int pluginVersionMajor,
                 int pluginVersionMinor)
{
    Q_UNUSED(pluginVersionMajor);
    Q_UNUSED(pluginVersionMinor);

    assert( !pluginID.empty() );

    static const std::string neatvideo("com.absoft.neatvideo");
    static const std::string hitfilm("com.FXHOME.HitFilm");
    static const std::string redgiant("com.redgiantsoftware.Universe_");
    static const std::string digitalfilmtools("com.digitalfilmtools.");
    static const std::string tiffen("com.tiffen.");
    //static const std::string digitalanarchy("com.digitalanarchy.");

    if ( !pluginID.compare(0, neatvideo.size(), neatvideo) ) {
        // Neat Video plugins work with Nuke, Resolve and Mistika
        // https://www.neatvideo.com/download.html

        // tested with neat video 4.0.9, maj=4,min=0
        return Settings::eKnownHostNameNuke;
    } else if ( !pluginID.compare(0, hitfilm.size(), hitfilm) ) {
        // HitFilm plugins (work with Vegas, Resolve and TitlerPro,
        // Vegas and TitlerPro support more plugins than Resolve
        // tested with HitFilm 3.1.0113
        // maj=1 or 2 (depends on plugin), min=0
        // HitFilm Ignite also supports NewBlue OFX Bridge, Sony Catalyst Edit, The Foundry Nuke
        // tested with HitFilm Ignite 1.0.0118
        // Clone Stamp from HitFilm Ignite is officially only compatible with Nuke
        if ( pluginID == (hitfilm + ".CloneStamp") ) {
            return Settings::eKnownHostNameNuke;
        }

        return Settings::eKnownHostNameVegas;
    } else if ( !pluginID.compare(0, redgiant.size(), redgiant) ) {
        // Red Giant Universe plugins 1.5 work with Vegas and Resolve
        return Settings::eKnownHostNameVegas;
    } else if ( !pluginID.compare(0, digitalfilmtools.size(), digitalfilmtools) ||
                !pluginID.compare(0, tiffen.size(), tiffen) ) {
        // Digital film tools plug-ins work with Nuke, Vegas, Scratch and Resolve

        // http://www.digitalfilmtools.com/supported-hosts/ofx-host-plugins.php
        return Settings::eKnownHostNameNuke;
        //} else if (!pluginID.compare(0, digitalanarchy.size(), digitalanarchy)) {
        //    // Digital Anarchy plug-ins work with Scratch, Resolve, and any OFX host, but they are tested with Scratch.
        //    // http://digitalanarchy.com/demos/psd_mac.html
        //    return Settings::eKnownHostNameScratch;
    }
    //printf("%s v%d.%d\n", pluginID.c_str(), pluginVersionMajor, pluginVersionMinor);

    return Settings::eKnownHostNameNone;
} // getHostNameProxy

const std::string &
OfxHost::getStringProperty(const std::string &name,
                           int n) const OFX_EXCEPTION_SPEC
{
    if ( (name == kOfxPropName) && (n == 0) ) {
        // depending on the current plugin ID and version, return a compatible host name.
        std::string pluginID;
        int pluginVersionMajor = 0;
        int pluginVersionMinor = 0;
        // the loading state is only that of the thread that loads plug-ins (see OfxHostPrivate)
        const bool loadingThread = ( _imp->loadingThread.load() == QThread::currentThread() );
        if (loadingThread && _imp->settingHost) {
            // OfxSetHost of a binary, before any of its plug-ins is known, at the scan and when a binary
            // known from the cache is loaded: the vendor prefixes of getHostNameProxy() are those of the
            // bundle identifier too. With no bundle identifier, the host name is Natron's.
            pluginID = _imp->setHostBundleID;
            if ( pluginID.empty() ) {
                return _properties.getStringPropertyRaw(kOfxPropName);
            }
        } else if ( loadingThread && !_imp->loadingPluginID.empty() ) {
            // plugin is not yet created: we are loading or describing it
            pluginID = _imp->loadingPluginID;
            pluginVersionMajor = _imp->loadingPluginVersionMajor;
            pluginVersionMinor = _imp->loadingPluginVersionMinor;
        } else {
            OfxHostDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
            // the effect whose action is running on this thread: outside actions (e.g. in a thread of the
            // plug-in), there is none
            OfxImageEffectInstance* effect = tls ? tls->effectCallingMainEntry() : NULL;
            if (effect) {
                pluginID = effect->getPlugin()->getIdentifier();
                pluginVersionMajor = effect->getPlugin()->getVersionMajor();
                pluginVersionMinor = effect->getPlugin()->getVersionMinor();
            }
        }

        ///Proxy known plug-ins that filter hostnames
        // (no plugin ID outside actions, e.g. in a thread of the plug-in: Natron's host name)
        if ( !pluginID.empty() ) {
            Settings::KnownHostNameEnum e = getHostNameProxy(pluginID, pluginVersionMajor, pluginVersionMinor);
            if (e != Settings::eKnownHostNameNone) {
                const std::string& ret = appPTR->getCurrentSettings()->getKnownHostName(e);

                return ret;
            }
        }

        // kOfxPropName was set at host creation, let the value decided by the user
        return _properties.getStringPropertyRaw(kOfxPropName);
    } else if (name == kOfxPropName) {
        // kOfxPropName has one dimension: "kOfxStatErrBadIndex - if the property index is invalid"
        // (ofxProperty.h, the propGet* functions)
        throw OFX::Host::Property::Exception(kOfxStatErrBadIndex);
    } else {
        // the hook is only installed for kOfxPropName (see the OfxHost constructor)
        throw OFX::Host::Property::Exception(kOfxStatErrUnknown);
    }
}

OFX::Host::ImageEffect::Instance*
OfxHost::newInstance(void*,
                     OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                     OFX::Host::ImageEffect::Descriptor & desc,
                     const std::string & context)
{
    assert(plugin);


    return new OfxImageEffectInstance(plugin, desc, context, false);
}

/// Override this to create a descriptor, this makes the 'root' descriptor
OFX::Host::ImageEffect::Descriptor *
OfxHost::makeDescriptor(OFX::Host::ImageEffect::ImageEffectPlugin* plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OfxImageEffectDescriptor(plugin);

    return desc;
}

/// used to construct a context description, rootContext is the main context
OFX::Host::ImageEffect::Descriptor *
OfxHost::makeDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                        OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OfxImageEffectDescriptor(rootContext, plugin);

    return desc;
}

/// used to construct populate the cache
OFX::Host::ImageEffect::Descriptor *
OfxHost::makeDescriptor(const std::string &bundlePath,
                        OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OfxImageEffectDescriptor(bundlePath, plugin);

    return desc;
}

/// message
OfxStatus
OfxHost::vmessage(const char* msgtype,
                  const char* /*id*/,
                  const char* format,
                  va_list args)
{
    assert(msgtype);
    assert(format);
    std::string message = StrUtils::vformat(format, args);
    std::string type(msgtype);

    if (type == kOfxMessageLog) {
        appPTR->writeToErrorLog_mt_safe( tr("Plug-in"), QDateTime::currentDateTime(), QString::fromUtf8( message.c_str() ) );
    } else if ( (type == kOfxMessageFatal) || (type == kOfxMessageError) ) {
        ///It seems that the only errors or warning that passes here are exceptions thrown by plug-ins
        ///(mainly Sapphire) while aborting a render. Instead of spamming the user of meaningless dialogs,
        ///just write to the log instead.
        //Dialogs::errorDialog(NATRON_APPLICATION_NAME, message);
        appPTR->writeToErrorLog_mt_safe(tr("Plug-in"), QDateTime::currentDateTime(), QString::fromUtf8( message.c_str() ) );
    } else if (type == kOfxMessageWarning) {
        ///It seems that the only errors or warning that passes here are exceptions thrown by plug-ins
        ///(mainly Sapphire) while aborting a render. Instead of spamming the user of meaningless dialogs,
        ///just write to the log instead.
        //        Dialogs::warningDialog(NATRON_APPLICATION_NAME, message);
        appPTR->writeToErrorLog_mt_safe( tr("Plug-in"), QDateTime::currentDateTime(), QString::fromUtf8( message.c_str() ) );
    } else if (type == kOfxMessageMessage) {
        Dialogs::informationDialog(NATRON_APPLICATION_NAME, message);
    } else if (type == kOfxMessageQuestion) {
        if (Dialogs::questionDialog(NATRON_APPLICATION_NAME, message, false) == eStandardButtonYes) {
            return kOfxStatReplyYes;
        } else {
            return kOfxStatReplyNo;
        }
    } else {
        // not one of the kOfxMessage* types: "kOfxStatFailed - if the message could not be posted for
        // some reason" (ofxMessage.h)
        return kOfxStatFailed;
    }

    // "kOfxStatOK - if the message was successfully posted" (ofxMessage.h)
    return kOfxStatOK;
}

OfxStatus
OfxHost::setPersistentMessage(const char* type,
                              const char* id,
                              const char* format,
                              va_list args)
{
    vmessage(type, id, format, args);

    return kOfxStatOK;
}

/// clearPersistentMessage
OfxStatus
OfxHost::clearPersistentMessage()
{
    return kOfxStatOK;
}

std::vector<std::string>
OfxHost::getSupportedContexts(const std::set<std::string> & pluginContexts)
{
    // The contexts Natron can instantiate a plug-in in, by order of preference. Generator, Filter,
    // General and Transition are the ones it advertises in kOfxImageEffectPropSupportedContexts (see
    // setProperties()); Reader, Writer and Tracker (a Natron extension) are not advertised, but Natron
    // implements them; Paint is handled like General with the clips of the Paint context. Retimer is not
    // supported: Natron does not drive the "SourceTime" parameter.
    static const char* const supportedContexts[] = {
        kOfxImageEffectContextReader,
        kOfxImageEffectContextWriter,
        kNatronOfxImageEffectContextTracker,
        kOfxImageEffectContextGeneral,
        kOfxImageEffectContextFilter,
        kOfxImageEffectContextPaint,
        kOfxImageEffectContextGenerator,
        kOfxImageEffectContextTransition,
    };
    std::vector<std::string> ret;

    for (std::size_t i = 0; i < sizeof(supportedContexts) / sizeof(supportedContexts[0]); ++i) {
        if ( pluginContexts.count(supportedContexts[i]) ) {
            ret.push_back(supportedContexts[i]);
        }
    }

    return ret;
}

OFX::Host::ImageEffect::Descriptor*
OfxHost::getPluginContextAndDescribe(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                     ContextEnum* ctx)
{
    // Re-entry from the thread that is describing a plug-in: e.g. a plug-in opens a modal dialog in
    // kOfxActionLoad or a describe action, and the user creates a node from its nested event loop.
    // describeMutex is not recursive, so this would deadlock, and a recursive mutex would be no better:
    // the nested call would overwrite the loading state of the plug-in being described, and could
    // describe a plug-in of the same binary (or the same plug-in) while it is half-loaded, which
    // HostSupport forbids. Refuse it: the node is not created, and the error is reported.
    if ( _imp->describeThread.load() == QThread::currentThread() ) {
        throw std::runtime_error( tr("Cannot load %1 while another OpenFX plug-in is being loaded.")
                                  .arg( QString::fromUtf8( plugin->getIdentifier().c_str() ) ).toStdString() );
    }
    QMutexLocker l(&_imp->describeMutex);

    // clear the loading state on every exit, including the exceptions
    struct DescribeScope
    {
        OfxHostPrivate* imp;
        explicit DescribeScope(OfxHostPrivate* imp_)
            : imp(imp_)
        {
            imp->clearLoadingState();
            imp->describeThread = QThread::currentThread();
            imp->loadingThread = QThread::currentThread();
        }

        ~DescribeScope()
        {
            imp->clearLoadingState();
            imp->describeThread = nullptr;
        }
    };
    DescribeScope scope( _imp.get() );

    OFX::Host::PluginBinary* binary = plugin->getBinary();
    std::unique_ptr<OFX::Host::PluginHandle> binaryHandle;
    if ( binary && !binary->isLoaded() ) {
        // A binary known from the plug-in cache, loaded at the first use of one of its plug-ins: load it
        // with no plug-in being loaded, so that its OfxSetHost (OFX 1.5) sees the same host name as at the
        // scan (from the bundle identifier, see loadingBinary()), and kOfxActionLoad and the describe
        // actions the same as at the scan too (from the plug-in, set below). HostSupport calls OfxSetHost
        // and Load from the same getPluginHandle() call, with no hook in between. This handle keeps the
        // binary loaded until getPluginHandle() below has taken its own reference, so that OfxSetHost is
        // called once. (It also calls the plug-in's OfxPlugin::setHost, which getPluginHandle() calls
        // again with the same host.) If OfxSetHost refuses this host, getPluginHandle() returns NULL.
        try {
            binaryHandle.reset( new OFX::Host::PluginHandle(plugin, this) );
        } catch (...) {
            throw std::runtime_error( tr("Error: Loading the binary (OfxSetHost) failed while loading %1.")
                                      .arg( QString::fromUtf8( plugin->getIdentifier().c_str() ) ).toStdString() );
        }
        _imp->settingHost = false;
        _imp->setHostBundleID.clear();
    }

    _imp->loadingPluginID = plugin->getRawIdentifier();
    _imp->loadingPluginVersionMajor = plugin->getVersionMajor();
    _imp->loadingPluginVersionMinor = plugin->getVersionMinor();

    OFX::Host::PluginHandle *pluginHandle;
    // getPluginHandle() must be called before getContexts():
    // it calls kOfxActionLoad on the plugin and kOfxActionDescribe, which may set properties (including supported contexts)
    try {
        pluginHandle = plugin->getPluginHandle();
    } catch (...) {
        throw std::runtime_error( tr("Error: Description (kOfxActionLoad and kOfxActionDescribe) failed while loading %1.")
                                  .arg( QString::fromUtf8( plugin->getIdentifier().c_str() ) ).toStdString() );
    }

    if (!pluginHandle) {
        throw std::runtime_error( tr("Error: Description (kOfxActionLoad and kOfxActionDescribe) failed while loading %1.")
                                  .arg( QString::fromUtf8( plugin->getIdentifier().c_str() ) ).toStdString() );
    }
    assert(pluginHandle->getOfxPlugin() && pluginHandle->getOfxPlugin()->mainEntry);

    // "kOfxImageEffectActionDescribeInContext will be called once for each context that the host and
    // plug-in mutually support" (ofxCore.h). A Natron node has a single context, so the contexts are
    // described lazily, by order of preference, until one succeeds: a context whose description returns
    // kOfxStatErrMissingHostFeature "will be ignored by the host" (ofxImageEffect.h), and the next one
    // is tried.
    const std::vector<std::string> contexts = getSupportedContexts( plugin->getContexts() );
    if ( contexts.empty() ) {
        throw std::invalid_argument( tr("OpenFX plug-in does not have any valid context.").toStdString() );
    }

    OFX::Host::PluginHandle* ph = plugin->getPluginHandle();
    assert( ph->getOfxPlugin() );
    assert(ph->getOfxPlugin()->mainEntry);
    Q_UNUSED(ph);
    OFX::Host::ImageEffect::Descriptor* desc = NULL;
    std::string context;
    QStringList failedContexts;
    for (std::vector<std::string>::const_iterator it = contexts.begin(); it != contexts.end(); ++it) {
        //This will call kOfxImageEffectActionDescribeInContext
        desc = plugin->getContext(*it);
        if (desc) {
            context = *it;
            break;
        }
        failedContexts.push_back( QString::fromUtf8( it->c_str() ) );
        if ( (*it == kOfxImageEffectContextReader) || (*it == kOfxImageEffectContextWriter) ) {
            // Natron registers the plug-in as a reader or a writer (Plugin::isReader(), isWriter()), and
            // creates its nodes as such: don't make it a General or Filter effect instead.
            break;
        }
    }
    if (!desc) {
        throw std::runtime_error( tr("Plug-in parameters and inputs description (kOfxImageEffectActionDescribeInContext) failed in context %1.")
                                  .arg( failedContexts.join( QString::fromUtf8(", ") ) ).toStdString() );
    }

    //Create the mask clip if needed
    if ( desc->isHostMaskingEnabled() ) {
        const std::map<std::string, OFX::Host::ImageEffect::ClipDescriptor*>& clips = desc->getClips();
        std::map<std::string, OFX::Host::ImageEffect::ClipDescriptor*>::const_iterator found = clips.find("Mask");
        if ( found == clips.end() ) {
            OFX::Host::ImageEffect::ClipDescriptor* clip = desc->defineClip("Mask");
            OFX::Host::Property::Set& props = clip->getProps();
            props.setIntProperty(kOfxImageClipPropIsMask, 1);
            props.setStringProperty(kOfxImageEffectPropSupportedComponents, kOfxImageComponentAlpha, 0);
            if (context == kOfxImageEffectContextGeneral) {
                props.setIntProperty(kOfxImageClipPropOptional, 1);
            }
            props.setIntProperty(kOfxImageEffectPropSupportsTiles, desc->getProps().getIntProperty(kOfxImageEffectPropSupportsTiles) != 0);
            props.setIntProperty(kOfxImageEffectPropTemporalClipAccess, 0);
        }
    }


    *ctx = OfxEffectInstance::mapToContextEnum(context);

    return desc;
} // OfxHost::getPluginContextAndDescribe

AbstractOfxEffectInstancePtr
OfxHost::createOfxEffect(NodePtr node,
                         const CreateNodeArgs& args
#ifndef NATRON_ENABLE_IO_META_NODES
                         ,
                         bool allowFileDialogs,
                         bool *hasUsedFileDialog
#endif
                         )
{
    assert(node);
    const Plugin* natronPlugin = node->getPlugin();
    assert(natronPlugin);
    ContextEnum ctx;
    OFX::Host::ImageEffect::Descriptor* desc = natronPlugin->getOfxDesc(&ctx);
    OFX::Host::ImageEffect::ImageEffectPlugin* plugin = natronPlugin->getOfxPlugin();
    assert(plugin && desc && ctx != eContextNone);


    AbstractOfxEffectInstancePtr hostSideEffect( new OfxEffectInstance(node) );
    NodeSerializationPtr serialization = args.getProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);
    std::string fixedName = args.getProperty<std::string>(kCreateNodeArgsPropNodeInitialName);

    if ( node && !node->getEffectInstance() ) {
        node->setEffect(hostSideEffect);
        node->initNodeScriptName(serialization.get(), QString::fromUtf8(fixedName.c_str()));
    }

    hostSideEffect->createOfxImageEffectInstance(plugin, desc, ctx, serialization.get(), args
#ifndef NATRON_ENABLE_IO_META_NODES
                                                 , allowFileDialogs,
                                                 hasUsedFileDialog
#endif
                                                 );

    return hostSideEffect;
}

///Return the xml cache file used before Natron 2 RC2
static QString
getOldCacheFilePath()
{
    QString cachePath = appPTR->getDiskCacheLocation() + QLatin1Char('/');
    QString oldOfxCache = cachePath + QString::fromUtf8("OFXCache.xml");

    return oldOfxCache;
}

static QString
getOFXCacheDirPath()
{
    QString cachePath = appPTR->getDiskCacheLocation() + QLatin1Char('/');
    QString ofxCachePath = cachePath + QString::fromUtf8("OFXLoadCache");

    return ofxCachePath;
}

///Return the xml cache file used after Natron 2 RC2
static QString
getCacheFilePath()
{
    QString ofxCachePath = getOFXCacheDirPath() + QLatin1Char('/');
    QString ofxCacheFilePath = ofxCachePath + QString::fromUtf8("OFXCache_") +
                               QString::fromUtf8(NATRON_VERSION_STRING) + QString::fromUtf8("_") +
                               QString::fromUtf8(NATRON_DEVELOPMENT_STATUS) + QString::fromUtf8("_") +
                               QString::number(NATRON_BUILD_NUMBER) + QString::fromUtf8(".xml");

    return ofxCacheFilePath;
}


static void
getPluginShortcuts(const OFX::Host::ImageEffect::Descriptor& desc, std::list<PluginActionShortcut>* shortcuts)
{
    int nDims = desc.getProps().getDimension(kNatronOfxImageEffectPropInViewerContextDefaultShortcuts);
    if (nDims == 0) {
        return;
    }
    {
        // Check that all props have the same dimension

        int nSymDims = desc.getProps().getDimension(kNatronOfxImageEffectPropInViewerContextShortcutSymbol);
        int nCtrlDims = desc.getProps().getDimension(kNatronOfxImageEffectPropInViewerContextShortcutHasControlModifier);
        int nShiftDims = desc.getProps().getDimension(kNatronOfxImageEffectPropInViewerContextShortcutHasShiftModifier);
        int nAltDims = desc.getProps().getDimension(kNatronOfxImageEffectPropInViewerContextShortcutHasAltModifier);
        int nMetaDims = desc.getProps().getDimension(kNatronOfxImageEffectPropInViewerContextShortcutHasMetaModifier);
        int nKeypadDims = desc.getProps().getDimension(kNatronOfxImageEffectPropInViewerContextShortcutHasKeypadModifier);

        if (nSymDims != nDims ||
            nCtrlDims != nDims ||
            nShiftDims != nDims ||
            nAltDims != nDims ||
            nMetaDims != nDims ||
            nKeypadDims != nDims) {
            std::cerr << desc.getPlugin()->getIdentifier() << ": Invalid dimension setup of the NatronOfxImageEffectPropInViewerContextDefaultShortcuts property." << std::endl;
            return;
        }
    }

    const std::map<std::string, OFX::Host::Param::Descriptor*> & paramDescriptors = desc.getParams();

    for (int i = 0; i < nDims; ++i) {
        const std::string& paramName = desc.getProps().getStringProperty(kNatronOfxImageEffectPropInViewerContextDefaultShortcuts, i);
        int symbol = desc.getProps().getIntProperty(kNatronOfxImageEffectPropInViewerContextShortcutSymbol, i);
        int hasCtrl = desc.getProps().getIntProperty(kNatronOfxImageEffectPropInViewerContextShortcutHasControlModifier, i);
        int hasShift = desc.getProps().getIntProperty(kNatronOfxImageEffectPropInViewerContextShortcutHasShiftModifier, i);
        int hasAlt = desc.getProps().getIntProperty(kNatronOfxImageEffectPropInViewerContextShortcutHasAltModifier, i);
        int hasMeta = desc.getProps().getIntProperty(kNatronOfxImageEffectPropInViewerContextShortcutHasMetaModifier, i);
        int hasKeypad = desc.getProps().getIntProperty(kNatronOfxImageEffectPropInViewerContextShortcutHasKeypadModifier, i);

        std::map<std::string, OFX::Host::Param::Descriptor*>::const_iterator foundParamDesc = paramDescriptors.find(paramName);
        if (foundParamDesc == paramDescriptors.end()) {
            // Hmm the plug-in probably wrongly set the kNatronOfxImageEffectPropInViewerContextDefaultShortcuts property
            std::cerr << desc.getPlugin()->getIdentifier() << ": " << paramName << " was set to the NatronOfxImageEffectPropInViewerContextDefaultShortcuts property but does not appear to exist in the parameters described." << std::endl;
            continue;
        }


        // The Key enum is a mapping 1:1 of the symbols defined in ofxKeySymbols.h
        Key eSymbol = (Key)symbol;
        KeyboardModifiers eMods;
        if (hasCtrl) {
            eMods |= eKeyboardModifierControl;
        }
        if (hasShift) {
            eMods |= eKeyboardModifierShift;
        }
        if (hasAlt) {
            eMods |= eKeyboardModifierAlt;
        }
        if (hasMeta) {
            eMods |= eKeyboardModifierMeta;
        }
        if (hasKeypad) {
            eMods |= eKeyboardModifierKeypad;
        }
        shortcuts->push_back(PluginActionShortcut(paramName, foundParamDesc->second->getLabel(), eSymbol, eMods));
    }
}

#ifndef QT_NO_DEBUG_OUTPUT
// used by qDebug() below (with QT_NO_DEBUG_OUTPUT, qDebug() accepts anything)
static inline
QDebug operator<<(QDebug dbg, const std::list<std::string> &l)
{
    for (std::list<std::string>::const_iterator it = l.begin(); it != l.end(); ++it) {
        dbg.nospace() << QString::fromUtf8( it->c_str() ) << ' ';
    }

    return dbg.space();
}
#endif

void
OfxHost::loadOFXPlugins(IOPluginsMap* readersMap,
                        IOPluginsMap* writersMap)
{
    qDebug() << "Load OFX Plugins...";
    SettingsPtr settings = appPTR->getCurrentSettings();
    assert(settings);
    bool useStdOFXPluginsLocation = settings->getUseStdOFXPluginsLocation();
    if (!useStdOFXPluginsLocation) {
        qDebug() << "Load OFX Plugins: do not use std plugins location";
        // only set if false, else use the previous value (which is set for example in BaseTest::SetUp())
        OFX::Host::PluginCache::useStdOFXPluginsLocation(useStdOFXPluginsLocation);
    }
    OFX::Host::PluginCache* pluginCache = OFX::Host::PluginCache::getPluginCache();
    assert(pluginCache);
    /// set the version label in the global cache
    // v2: HostSupport of openfx 4476791d (read-only properties, bundle architecture folders): describe the plug-ins again
    pluginCache->setCacheVersion( getPluginCacheVersion( appPTR->getCurrentSettings()->getHostName() ) );

    // the host passed to each binary's OfxSetHost (OFX 1.5) at the scan: the same host as when a plug-in
    // known from the cache is loaded later (HostSupport's default, the image effect host, is this one too)
    pluginCache->setScanHost(this);

    /// register the image effect cache with the global plugin cache
    _imp->imageEffectPluginCache->registerInCache( *pluginCache );

    if (useStdOFXPluginsLocation) {
        pluginCache->setPluginHostPath(NATRON_APPLICATION_NAME);
        pluginCache->setPluginHostPath("Nuke"); // most Nuke OFX plugins are compatible
    }
    std::list<std::string> extraPluginsSearchPaths;
    settings->getOpenFXPluginsSearchPaths(&extraPluginsSearchPaths);
    for (std::list<std::string>::iterator it = extraPluginsSearchPaths.begin(); it != extraPluginsSearchPaths.end(); ++it) {
        if ( !(*it).empty() ) {
            qDebug() << "Load OFX Plugins: append extra plugins dir" << it->c_str();
            pluginCache->addFileToPath(*it);
        }
    }

    // if Natron is /usr/bin/Natron, /usr/bin/../OFX/Natron points to Natron-specific plugins
    QDir dir( QCoreApplication::applicationDirPath() );
    dir.cdUp();
    std::string natronBundledPluginsPath = QString( dir.absolutePath() +  QString::fromUtf8("/Plugins/OFX/") + QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString();
    try {
        if ( settings->loadBundledPlugins() ) {
            if ( settings->preferBundledPlugins() ) {
                qDebug() << "Load OFX Plugins: prepend bundled plugins dir" << natronBundledPluginsPath.c_str();
                pluginCache->prependFileToPath(natronBundledPluginsPath);
            } else {
                qDebug() << "Load OFX Plugins: append bundled plugins dir" << natronBundledPluginsPath.c_str();
                pluginCache->addFileToPath(natronBundledPluginsPath);
            }
        }
    } catch (std::logic_error&) {
        // ignore
    }

    // The cache location depends on the OS.
    // On OSX, it will be ~/Library/Caches/<organization>/<application>/OFXLoadCache/
    //on Linux ~/.cache/<organization>/<application>/OFXLoadCache/
    //on windows: C:\Users\<username>\App Data\Local\<organization>\<application>\Caches\OFXLoadCache
    QString ofxCacheFilePath = getCacheFilePath();
    qDebug() << "Load OFX Plugins: reading cache file" << ofxCacheFilePath;

    {
        FStreamsSupport::ifstream ifs;
        FStreamsSupport::open( &ifs, ofxCacheFilePath.toStdString() );
        if (!ifs) {
            qDebug() << "Load OFX Plugins: cannot open cache file" << ofxCacheFilePath;
        } else {
            try {
                pluginCache->readCache(ifs);
                qDebug() << "Load OFX Plugins: reading cache file... done!";
            } catch (const std::exception& e) {
                qDebug() << "Load OFX Plugins: reading cache file... failed!";
                appPTR->writeToErrorLog_mt_safe( QLatin1String("OpenFX"), QDateTime::currentDateTime(),
                                                 tr("Failure to read OpenFX plug-ins cache: %1").arg( QString::fromUtf8( e.what() ) ) );
            }
        }
    }
    
    qDebug() << "Load OFX Plugins: plugin path is" << pluginCache->getPluginPath();
    qDebug() << "Load OFX Plugins: scan plugins...";
    pluginCache->scanPluginFiles();
    qDebug() << "Load OFX Plugins: scan plugins... done!";
    _imp->clearLoadingState(); // finished loading plugins

    if ( pluginCache->dirty() ) {
        // write the cache NOW (it won't change anyway)
        qDebug() << "Load OFX Plugins: writing cache file" << ofxCacheFilePath;
        /// flush out the current cache
        writeOFXCache();
        qDebug() << "Load OFX Plugins: writing cache file... done!";
    }

    /*Filling node name list and plugin grouping*/
    typedef std::map<OFX::Host::ImageEffect::MajorPlugin, OFX::Host::ImageEffect::ImageEffectPlugin *> PMap;
    const PMap& ofxPlugins =
        _imp->imageEffectPluginCache->getPluginsByIDMajor();


    for (PMap::const_iterator it = ofxPlugins.begin();
         it != ofxPlugins.end(); ++it) {
        OFX::Host::ImageEffect::ImageEffectPlugin* p = it->second;
        assert(p);
        // "If a plug-in does not report to support any context supported by host, host should not enable
        // the plug-in." (ofxCore.h)
        if ( getSupportedContexts( p->getContexts() ).empty() ) {
            if ( !p->getContexts().empty() ) {
                QStringList pluginContexts;
                for (std::set<std::string>::const_iterator itc = p->getContexts().begin(); itc != p->getContexts().end(); ++itc) {
                    pluginContexts.push_back( QString::fromUtf8( itc->c_str() ) );
                }
                QString message = tr("%1 is not loaded: none of its contexts (%2) is supported.")
                                  .arg( QString::fromUtf8( p->getIdentifier().c_str() ) )
                                  .arg( pluginContexts.join( QString::fromUtf8(", ") ) );
                qDebug() << "Load OFX Plugins:" << message;
                appPTR->writeToErrorLog_mt_safe( QLatin1String("OpenFX"), QDateTime::currentDateTime(), message );
            }
            continue;
        }
        assert( p->getBinary() );
        if ( !p->getBinary() ) {
            continue;
        }

        std::string openfxId = p->getIdentifier();
        const std::string & grouping = p->getDescriptor().getPluginGrouping();
        const std::string & bundlePath = p->getBinary()->getBundlePath();
        std::string pluginLabel = OfxEffectInstance::makePluginLabel( p->getDescriptor().getShortLabel(),
                                                                      p->getDescriptor().getLabel(),
                                                                      p->getDescriptor().getLongLabel() );
        QStringList groups = OfxEffectInstance::makePluginGrouping(p->getIdentifier(),
                                                                   p->getVersionMajor(), p->getVersionMinor(),
                                                                   pluginLabel, grouping);
        for (int i = 0; i < groups.size(); ++i) {
            groups[i] = groups[i].trimmed();
        }

        const std::string resourcesPathStr(bundlePath + "/Contents/Resources/");
        QString resourcesPath = QString::fromUtf8( resourcesPathStr.c_str() );
        QString iconFileName;
        std::string pngIcon;
        try {
            // kOfxPropIcon is normally only defined for parameter desctriptors
            // (see <http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ParameterProperties>)
            // but let's assume it may also be defained on the plugin descriptor.
            pngIcon = p->getDescriptor().getProps().getStringProperty(kOfxPropIcon, 1); // dimension 1 is PNG icon
        } catch (OFX::Host::Property::Exception) {
        }

        if ( pngIcon.empty() ) {
            // no icon defined by kOfxPropIcon, use the default value
            pngIcon = openfxId + ".png";
        }
        iconFileName.append(resourcesPath);
        iconFileName.append( QString::fromUtf8( pngIcon.c_str() ) );
        // the plugin grouping has no descriptor, just try the default filename:
        // Resources/<group>.png, Resources/<group>/<subgroup>.png, etc.
        // If that file does not exist, use the safe slug of each level (see Plugin::makeGroupingId()),
        // e.g. Resources/GMIC.png for the "G'MIC" group.
        QStringList groupIcons;
        if ( groups.isEmpty() ) {
            //Use default Misc group when the plug-in doesn't belong to a group
            groups.push_back( QString::fromUtf8(PLUGIN_GROUP_DEFAULT) );
            groupIcons << QString();
        }
        for (int i = groupIcons.size(); i < groups.size(); ++i) {
            QString groupIconPath = resourcesPath + groups.mid(0, i + 1).join( QLatin1Char('/') ) + QString::fromUtf8(".png");
            if ( !QFile::exists(groupIconPath) ) {
                QStringList slugs;
                for (int j = 0; j <= i; ++j) {
                    slugs << Plugin::makeGroupingId( QStringList(groups[j]) );
                }
                QString slugIconPath = resourcesPath + slugs.join( QLatin1Char('/') ) + QString::fromUtf8(".png");
                if ( QFile::exists(slugIconPath) ) {
                    groupIconPath = slugIconPath;
                }
            }
            groupIcons << groupIconPath;
        }

        const std::set<std::string> & contexts = p->getContexts();
        std::set<std::string>::const_iterator foundReader = contexts.find(kOfxImageEffectContextReader);
        std::set<std::string>::const_iterator foundWriter = contexts.find(kOfxImageEffectContextWriter);
        const bool isObsolete = isPluginObsolete( p->getDescriptor().getProps() );
        Plugin* natronPlugin = appPTR->registerPlugin( resourcesPath,
                                                       groups,
                                                       QString::fromUtf8( openfxId.c_str() ),
                                                       QString::fromUtf8( pluginLabel.c_str() ),
                                                       iconFileName,
                                                       groupIcons,
                                                       foundReader != contexts.end(),
                                                       foundWriter != contexts.end(),
                                                       new LibraryBinary(LibraryBinary::eLibraryTypeBuiltin),
                                                       p->getDescriptor().getRenderThreadSafety() == kOfxImageEffectRenderUnsafe,
                                                       p->getVersionMajor(), p->getVersionMinor(), isObsolete );
        bool isInternalOnly = openfxId == PLUGINID_OFX_ROTO;
        if (isInternalOnly) {
            natronPlugin->setForInternalUseOnly(true);
        }

        PluginOpenGLRenderSupport glSupport = ePluginOpenGLRenderSupportNone;
        {
            const std::string& str = p->getDescriptor().getProps().getStringProperty(kOfxImageEffectPropOpenGLRenderSupported);
            if (str == "false") {
                glSupport = ePluginOpenGLRenderSupportNone;
            } else if (str == "needed") {
                glSupport = ePluginOpenGLRenderSupportNeeded;
            } else if (str == "true") {
                glSupport = ePluginOpenGLRenderSupportYes;
            }
        }
        glSupport = applyCPURenderSupport( glSupport, p->getDescriptor().getProps() );
        natronPlugin->setOpenGLRenderSupport(glSupport);

        natronPlugin->setOfxPlugin(p);

        std::list<PluginActionShortcut> shortcuts;
        getPluginShortcuts(p->getDescriptor(), &shortcuts);
        natronPlugin->setShorcuts(shortcuts);

        ///if this plugin's descriptor has the kTuttleOfxImageEffectPropSupportedExtensions property,
        ///use it to fill the readersMap and writersMap
        int formatsCount = p->getDescriptor().getProps().getDimension(kTuttleOfxImageEffectPropSupportedExtensions);
        std::vector<std::string> formats(formatsCount);
        for (int k = 0; k < formatsCount; ++k) {
            formats[k] = p->getDescriptor().getProps().getStringProperty(kTuttleOfxImageEffectPropSupportedExtensions, k);
            std::transform(formats[k].begin(), formats[k].end(), formats[k].begin(), ::tolower);
        }

        double evaluation = p->getDescriptor().getProps().getDoubleProperty(kTuttleOfxImageEffectPropEvaluation);


        if (!isObsolete && ( foundReader != contexts.end() ) && (formatsCount > 0) && readersMap) {
            ///we're safe to assume that this plugin is a reader
            for (std::size_t k = 0; k < formats.size(); ++k) {
                IOPluginSetForFormat& evalForFormat = (*readersMap)[formats[k]];
                evalForFormat.insert( IOPluginEvaluation(openfxId, evaluation) );
            }
        } else if (!isObsolete && ( foundWriter != contexts.end() ) && (formatsCount > 0) && writersMap) {
            ///we're safe to assume that this plugin is a writer.
            for (std::size_t k = 0; k < formats.size(); ++k) {
                IOPluginSetForFormat& evalForFormat = (*writersMap)[formats[k]];
                evalForFormat.insert( IOPluginEvaluation(openfxId, evaluation) );
            }
        }
    }
    qDebug() << "Load OFX Plugins... done!";
} // loadOFXPlugins

void
OfxHost::writeOFXCache()
{
    /// and write a new cache, long version with everything in there
    QString ofxCachePath = getOFXCacheDirPath();
    QDir().mkpath(ofxCachePath);
    QString ofxCacheFilePath = getCacheFilePath();

    QTemporaryFile tmpf;
    tmpf.open();
    QString tmpFileName = tmpf.fileName();
    tmpf.remove();

    FStreamsSupport::ofstream ofile;
    FStreamsSupport::open( &ofile, tmpFileName.toStdString() );
    if (!ofile) {
        return;
    }
    OFX::Host::PluginCache* pluginCache = OFX::Host::PluginCache::getPluginCache();
    assert(pluginCache);
    pluginCache->writePluginCache(ofile);

    ofile.close();
    if (QFile::exists(ofxCacheFilePath)) {
        QFile::remove(ofxCacheFilePath);
    }
    QFile::copy(tmpFileName, ofxCacheFilePath);
    QFile::remove(tmpFileName);
}

void
OfxHost::clearPluginsLoadedCache()
{
    QString oldOfxCache = getOldCacheFilePath();

    if ( QFile::exists(oldOfxCache) ) {
        QFile::remove(oldOfxCache);
    }

    QDir OFXCacheDir( getOFXCacheDirPath() );
    OFXCacheDir.removeRecursively();
}

void
OfxHost::unloadPlugins()
{
    // clearPluginCache() deletes the plug-in cache and forgets it: a second call deletes nothing, so the plug-ins
    // get kOfxActionUnload once, whether this was called at Python exit (AppManager::cleanUpAtPythonExit()) or not.
    _imp->pluginsUnloaded = true;
    OFX::Host::PluginCache::clearPluginCache();
}

bool
OfxHost::arePluginsUnloaded() const
{
    return _imp->pluginsUnloaded;
}

bool
OfxHost::isActionRunningInThisThread() const
{
    // mainEntry() pushes -1 on the thread indexes of the thread that calls an action (setThreadAsActionCaller())
    OfxHostDataTLSPtr tls = _imp->tlsData->getTLSData();

    return tls && !tls->threadIndexes.empty();
}

void
OfxHost::loadingStatus(bool loading,
                       const std::string & pluginId,
                       int versionMajor,
                       int versionMinor)
{
    // set the pluginID in case the plug-in tries to fetch the hostname property (called by the scan,
    // before kOfxActionLoad and kOfxActionDescribe, and after them)
    _imp->loadingThread = QThread::currentThread();
    _imp->loadingPluginID = pluginId;
    _imp->loadingPluginVersionMajor = versionMajor;
    _imp->loadingPluginVersionMinor = versionMinor;
    _imp->settingHost = false;
    _imp->setHostBundleID.clear();
    if (loading && appPTR) {
        appPTR->setLoadingStatus( QString::fromUtf8("OpenFX: loading ") + QString::fromUtf8( pluginId.c_str() ) + QString::fromUtf8(" v") + QString::number(versionMajor) + QLatin1Char('.') + QString::number(versionMinor) );
#     ifdef DEBUG
        qDebug() << QString::fromUtf8("OpenFX: loading ") + QString::fromUtf8( pluginId.c_str() ) + QString::fromUtf8(" v") + QString::number(versionMajor) + QLatin1Char('.') + QString::number(versionMinor);
#     endif
    }
}

void
OfxHost::loadingBinary(const std::string& filePath)
{
    // OfxSetHost is called next, before any plug-in of the binary is known: at the scan, and when a
    // binary known from the cache is loaded (see getPluginContextAndDescribe(), which loads it with no
    // plug-in being loaded). The host name it sees follows the bundle identifier, not the plug-in that
    // was loaded before (the last plug-in of the scan, from another binary). The next loadingStatus()
    // or getPluginContextAndDescribe() ends it.
    _imp->loadingThread = QThread::currentThread();
    _imp->loadingPluginID.clear();
    _imp->loadingPluginVersionMajor = 0;
    _imp->loadingPluginVersionMinor = 0;
    _imp->settingHost = true;
    _imp->setHostBundleID = getBundleIdentifier(filePath);
}

std::string
OfxHost::getPluginCacheVersion(const std::string& hostName)
{
    // v3: OfxSetHost (OFX 1.5). A binary whose OfxSetHost refuses this host is cached with no plug-ins,
    // and its answer may depend on the host name and version (kOfxPropName, kOfxPropVersion): the
    // plug-ins are scanned again when they change. The version is in the cache file name (see
    // getCacheFilePath()). The host name, which the user can set in the preferences, goes in as a hash:
    // HostSupport writes the version unescaped in an XML attribute (<cache version="...">), so a '"',
    // '<' or '&' would make the cache unreadable, and the plug-ins would be scanned at every launch.
    const QByteArray hash = QCryptographicHash::hash(QByteArray( hostName.data(), (int)hostName.size() ),
                                                     QCryptographicHash::Sha1).toHex();

    return std::string(NATRON_APPLICATION_NAME "OFXCachev3-") + std::string( hash.constData(), hash.size() );
}

std::string
OfxHost::getBundleIdentifier(const std::string& binaryFilePath)
{
    // The bundle is the ancestor directory whose name ends with ".bundle". HostSupport builds the paths
    // with the native separator ("\\" on Windows, see DIRSEP in ofxhPluginCache.cpp), and a directory of
    // OFX_PLUGIN_PATH may use either one: on Windows, the separators are made "/" first, and a "\\" after
    // ".bundle" is a separator on every system (a directory name that ends with ".bundle\\" is unlikely).
    const QString path = QDir::fromNativeSeparators( QString::fromUtf8( binaryFilePath.c_str() ) );
    const QString bundleSuffix = QString::fromUtf8(".bundle");
    const int pos = std::max( path.lastIndexOf(bundleSuffix + QLatin1Char('/'), -1, Qt::CaseInsensitive),
                              path.lastIndexOf(bundleSuffix + QLatin1Char('\\'), -1, Qt::CaseInsensitive) );
    if (pos < 0) {
        return std::string();
    }
    QFile plist( path.left( pos + bundleSuffix.size() ) + QString::fromUtf8("/Contents/Info.plist") );
    if ( !plist.open(QIODevice::ReadOnly) ) {
        return std::string();
    }
    const QString contents = QString::fromUtf8( plist.readAll() );
    const QRegularExpression re( QString::fromUtf8("<key>\\s*CFBundleIdentifier\\s*</key>\\s*<string>\\s*([^<]*?)\\s*</string>") );
    const QRegularExpressionMatch m = re.match(contents);

    return m.hasMatch() ? m.captured(1).toStdString() : std::string();
}

bool
OfxHost::pluginSupported(OFX::Host::ImageEffect::ImageEffectPlugin */*plugin*/,
                         std::string & /*reason*/) const
{
    //We support all OpenFX plug-ins.
    return true;
}

const void*
OfxHost::fetchSuite(const char *suiteName,
                    int suiteVersion)
{
    if ( (std::strcmp(suiteName, kOfxParametricParameterSuite) == 0) && (suiteVersion == 1) ) {
        return OFX::Host::ParametricParam::GetSuite(suiteVersion);
    } else {
        return OFX::Host::ImageEffect::Host::fetchSuite(suiteName, suiteVersion);
    }
}

OFX::Host::Memory::Instance*
OfxHost::newMemoryInstance(size_t nBytes)
{
    OfxMemory* ret = new OfxMemory( OfxEffectInstancePtr() );
    bool allocated = ret->alloc(nBytes);

    // (test allocated first: after a failed allocation, PluginMemory::getPtr() must not be called)
    if ( !allocated || ( (nBytes != 0) && !ret->getPtr() ) ) {
        delete ret;
        // This may be called from a render thread: log the error, never open a dialog.
        appPTR->writeToErrorLog_mt_safe( QLatin1String("OpenFX"), QDateTime::currentDateTime(),
                                         tr("Failed to allocate memory (%1).").arg( printAsRAM(nBytes) ) );
        // Returning NULL would make HostSupport allocate the memory itself. Throwing bad_alloc makes
        // imageMemoryAlloc() return kOfxStatErrMemory and set *memoryHandle to NULL, as the spec says.
        throw std::bad_alloc();
    }

    return ret;
}

/////////////////
/////////////////////////////////////////////////// MULTI_THREAD SUITE ///////////////////////////////////////////////////
/////////////////


#ifdef OFX_SUPPORTS_MULTITHREAD

void
OfxHost::setThreadAsActionCaller(OfxImageEffectInstance* instance,
                                 bool actionCaller)
{
    OfxHostDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();

    if (actionCaller) {
        // both or none (push_back may throw std::bad_alloc)
        tls->effectsCallingMainEntry.push_back(instance);
        try {
            tls->threadIndexes.push_back(-1);
        } catch (...) {
            tls->effectsCallingMainEntry.pop_back();
            throw;
        }
    } else {
        // back to the action that called this one on this thread, if any: keeping the effect whose action
        // returned would give a later action of the caller its host name, and the effect may be destroyed
        // after its action returned (#1497)
        assert( !tls->effectsCallingMainEntry.empty() && (tls->effectsCallingMainEntry.back() == instance) );
        if ( !tls->effectsCallingMainEntry.empty() ) {
            tls->effectsCallingMainEntry.pop_back();
        }
        assert( !tls->threadIndexes.empty() );
        if ( !tls->threadIndexes.empty() ) {
            tls->threadIndexes.pop_back();
        }
    }
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

// In a thread function of the multithread suite, the effect whose action called multiThread() is the one whose
// action runs on the thread, as in the sequential path, which runs the functions in the calling thread: the
// plug-in gets the same host name (#1497). The calling thread waits for the thread functions, so the effect
// lives longer than they do. Nothing if effect is NULL (multiThread() called outside actions).
class SpawnedThreadEffect_RAII
{
    OfxHost::OfxHostDataTLSPtr _tls;
    OfxImageEffectInstance* _effect;

public:
    SpawnedThreadEffect_RAII(const OfxHost::OfxHostDataTLSPtr& tls,
                             OfxImageEffectInstance* effect)
        : _tls(tls)
        , _effect(effect)
    {
        if (_effect) {
            _tls->effectsCallingMainEntry.push_back(_effect);
        }
    }

    ~SpawnedThreadEffect_RAII()
    {
        if (_effect) {
            assert( !_tls->effectsCallingMainEntry.empty() && (_tls->effectsCallingMainEntry.back() == _effect) );
            if ( !_tls->effectsCallingMainEntry.empty() ) {
                _tls->effectsCallingMainEntry.pop_back();
            }
        }
    }
};

///Using QtConcurrent doesn't work with The Foundry Furnace plug-ins because they expect fresh threads
///to be created. As QtConcurrent's thread-pool recycles thread, it seems to make Furnace crash.
///We think this is because Furnace must keep an internal thread-local state that becomes then dirty
///if we re-use the same thread.

static OfxStatus
threadFunctionWrapper(OfxThreadFunctionV1 func,
                      unsigned int threadIndex,
                      unsigned int threadMax,
                      QThread* spawnerThread,
                      OfxImageEffectInstance* effect,
                      void *customArg)
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    assert(threadIndex < threadMax);
    OfxHost::OfxHostDataTLSPtr tls = appPTR->getOFXHost()->getTLSData();
    tls->threadIndexes.push_back( (int)threadIndex );

    QThread* spawnedThread = QThread::currentThread();
    if (spawnedThread != spawnerThread) {
        appPTR->getAppTLS()->softCopy(spawnerThread, spawnedThread);
    }

    OfxStatus ret = kOfxStatOK;
    try {
#ifdef DEBUG
        // Uncomment if using plugins that generate FP exceptions
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        SpawnedThreadEffect_RAII spawnedEffect(tls, effect);
        func(threadIndex, threadMax, customArg);
    } catch (const std::bad_alloc & ba) {
        ret =  kOfxStatErrMemory;
    } catch (...) {
        ret =  kOfxStatFailed;
    }

    ///reset back the index otherwise it could mess up the indexes if the same thread is re-used
    tls->threadIndexes.pop_back();

    if (spawnedThread != spawnerThread) {
        appPTR->getAppTLS()->cleanupTLSForThread();
    }

    return ret;
}

class OfxThread
    : public QThread
      , public AbortableThread
{
public:
    OfxThread(OfxThreadFunctionV1 func,
              unsigned int threadIndex,
              unsigned int threadMax,
              QThread* spawnerThread,
              OfxImageEffectInstance* effect,
              void *customArg,
              OfxStatus *stat)
        : QThread()
        , AbortableThread(this)
        , _func(func)
        , _threadIndex(threadIndex)
        , _threadMax(threadMax)
        , _spawnerThread(spawnerThread)
        , _effect(effect)
        , _customArg(customArg)
        , _stat(stat)
    {
        setThreadName("Multi-thread suite");
    }

    void run() OVERRIDE
    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                               boost_adaptbx::floating_point::exception_trapping::invalid |
                                                               boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
       assert(_threadIndex < _threadMax);
        OfxHost::OfxHostDataTLSPtr tls = appPTR->getOFXHost()->getTLSData();
        tls->threadIndexes.push_back( (int)_threadIndex );

        appPTR->getAppTLS()->softCopy(_spawnerThread, this);

        assert(*_stat == kOfxStatFailed);
        try {
            SpawnedThreadEffect_RAII spawnedEffect(tls, _effect);
            _func(_threadIndex, _threadMax, _customArg);
            *_stat = kOfxStatOK;
        } catch (const std::bad_alloc & ba) {
            *_stat = kOfxStatErrMemory;
        } catch (...) {
        }

        ///reset back the index otherwise it could mess up the indexes if the same thread is re-used
        tls->threadIndexes.pop_back();

        appPTR->getAppTLS()->cleanupTLSForThread();
    }

private:
    OfxThreadFunctionV1 *_func;
    unsigned int _threadIndex;
    unsigned int _threadMax;
    QThread* _spawnerThread;
    OfxImageEffectInstance* _effect;
    void *_customArg;
    OfxStatus *_stat;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


// Function to spawn SMP threads
//  This function will spawn nThreads separate threads of computation (typically one per CPU) to allow something to perform symmetric multi processing. Each thread will call 'func' passing in the index of the thread and the number of threads actually launched.
// multiThread will not return until all the spawned threads have returned. It is up to the host how it waits for all the threads to return (busy wait, blocking, whatever).
// nThreads can be more than the value returned by multiThreadNumCPUs, however the threads will be limited to the number of CPUs returned by multiThreadNumCPUs.
// This function cannot be called recursively.
// Note that the thread indexes are from 0 to nThreads-1.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThread

OfxStatus
OfxHost::multiThread(OfxThreadFunctionV1 func,
                     unsigned int nThreads,
                     void *customArg)
{
    if (!func) {
        return kOfxStatFailed;
    }

    // "This function cannot be called recursively", "kOfxStatErrExists, failed in an attempt to call
    // multiThread recursively" (ofxMultiThread.h): a thread function (in a spawned thread, or in the
    // calling thread on the sequential path below) may not call multiThread. An action called from a
    // thread function (e.g. the render of an upstream effect through clipGetImage) pushes -1 on the
    // thread index stack (setThreadAsActionCaller()), so that effect may call multiThread again.
    if ( multiThreadIsSpawnedThread() ) {
        return kOfxStatErrExists;
    }

    unsigned int maxConcurrentThread;
    OfxStatus st = multiThreadNumCPUS(&maxConcurrentThread);
    if (st != kOfxStatOK) {
        return st;
    }

    // from the documentation:
    // "nThreads can be more than the value returned by multiThreadNumCPUs, however
    // the threads will be limited to the number of CPUs returned by multiThreadNumCPUs."

    if ( (nThreads == 1) || (maxConcurrentThread <= 1) || (appPTR->getCurrentSettings()->getNumberOfThreads() == -1) ) {
        // Run the thread functions one after the other in the calling thread. Inside func(i, ...),
        // multiThreadIndex() must give i ("the same as the threadIndex argument passed to the
        // OfxThreadFunctionV1", ofxMultiThread.h) and multiThreadIsSpawnedThread() 1, as in the
        // spawned threads: push the index as threadFunctionWrapper() does.
        OfxHostDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
        for (unsigned int i = 0; i < nThreads; ++i) {
            tls->threadIndexes.push_back( (int)i );
            OfxStatus stat = kOfxStatOK;
            try {
                func(i, nThreads, customArg);
            } catch (const std::bad_alloc &) {
                stat = kOfxStatErrMemory;
            } catch (...) {
                stat = kOfxStatFailed;
            }
            tls->threadIndexes.pop_back();
            if (stat != kOfxStatOK) {
                return stat;
            }
        }

        return kOfxStatOK;
    }

    QThread* spawnerThread = QThread::currentThread();
    // the effect whose action called multiThread(), for the thread functions (see SpawnedThreadEffect_RAII)
    OfxImageEffectInstance* const effect = _imp->tlsData->getOrCreateTLSData()->effectCallingMainEntry();
    bool useThreadPool = appPTR->getUseThreadPool();

    if (useThreadPool) {
        std::vector<uint32_t> threadIndexes(nThreads);
        for (uint32_t i = 0; i < nThreads; ++i) {
            threadIndexes[i] = i;
        }

        /// DON'T set the maximum thread count, this is a global application setting, and see the documentation excerpt above
        //QThreadPool::globalInstance()->setMaxThreadCount(nThreads);
        std::function<OfxStatus (const uint32_t&)> process = [&](const uint32_t tid) {
            return threadFunctionWrapper(func, tid, nThreads, spawnerThread, effect, customArg);
        };
        QFuture<OfxStatus> future = QtConcurrent::mapped( threadIndexes, process );
        future.waitForFinished();
        ///DON'T reset back to the original value the maximum thread count
        //QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

        for (QFuture<OfxStatus>::const_iterator it = future.begin(); it != future.end(); ++it) {
            OfxStatus stat = *it;
            if (stat != kOfxStatOK) {
                return stat;
            }
        }
    } else {
        QVector<OfxStatus> status(nThreads); // vector for the return status of each thread
        status.fill(kOfxStatFailed); // by default, a thread fails
        {
            // at most maxConcurrentThread should be running at the same time
            QVector<OfxThread*> threads(nThreads);
            for (unsigned int i = 0; i < nThreads; ++i) {
                threads[i] = new OfxThread(func, i, nThreads, spawnerThread, effect, customArg, &status[i]);
            }
            unsigned int i = 0; // index of next thread to launch
            unsigned int running = 0; // number of running threads
            unsigned int j = 0; // index of first running thread. all threads before this one are finished running
            while (j < nThreads) {
                // have no more than maxConcurrentThread threads launched at the same time
                int threadsStarted = 0;
                while (i < nThreads && running < maxConcurrentThread) {
                    threads[i]->start();
                    ++i;
                    ++running;
                    ++threadsStarted;
                }

                ///We just started threadsStarted threads
                appPTR->fetchAndAddNRunningThreads(threadsStarted);

                // now we've got at most maxConcurrentThread running. wait for each thread and launch a new one
                threads[j]->wait();
                assert( !threads[j]->isRunning() );
                assert( threads[j]->isFinished() );
                delete threads[j];
                ++j;
                --running;

                ///We just stopped 1 thread
                appPTR->fetchAndAddNRunningThreads(-1);
            }
            assert(running == 0);
        }
        // check the return status of each thread, return the first error found
        for (QVector<OfxStatus>::const_iterator it = status.begin(); it != status.end(); ++it) {
            OfxStatus stat = *it;
            if (stat != kOfxStatOK) {
                return stat;
            }
        }
    } // useThreadPool

    return kOfxStatOK;
} // multiThread

// Function which indicates the number of CPUs available for SMP processing
//  This value may be less than the actual number of CPUs on a machine, as the host may reserve other CPUs for itself.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThreadNumCPUs
OfxStatus
OfxHost::multiThreadNumCPUS(unsigned int *nCPUs) const
{
    if (!nCPUs) {
        return kOfxStatFailed;
    }

    int nThreadsToRender, nThreadsPerEffect;
    appPTR->getNThreadsSettings(&nThreadsToRender, &nThreadsPerEffect);

    if (nThreadsToRender == -1) {
        *nCPUs = 1;
    } else {
        // activeThreadCount may be negative (for example if releaseThread() is called)
        int activeThreadsCount = QThreadPool::globalInstance()->activeThreadCount();

        // Add the number of threads already running by the multiThreadSuite + parallel renders
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
        activeThreadsCount += appPTR->getNRunningThreads();
#endif

        // Clamp to 0
        activeThreadsCount = std::max( 0, activeThreadsCount);

        assert(activeThreadsCount >= 0);

        // better than QThread::idealThreadCount();, because it can be set by a global preference:
        int maxThreadsCount = QThreadPool::globalInstance()->maxThreadCount();
        assert(maxThreadsCount >= 0);

        if (nThreadsPerEffect == 0) {
            ///Simple heuristic: limit 1 effect to start at most 8 threads because otherwise it might spend too much
            ///time scheduling than just processing
            int hwConcurrency = appPTR->getMaxThreadCount();

            if (hwConcurrency <= 0) {
                nThreadsPerEffect = 1;
            } else {
                nThreadsPerEffect = hwConcurrency;
            }
            /*else if (hwConcurrency <= NATRON_MULTI_THREAD_SUITE_MAX_NUM_CPU) {
                nThreadsPerEffect = hwConcurrency;
               } else {
                nThreadsPerEffect = NATRON_MULTI_THREAD_SUITE_MAX_NUM_CPU;
               }*/
        }
        ///+1 because the current thread is going to wait during the multiThread call so we're better off
        ///not counting it.
        *nCPUs = std::max( 1, std::min(maxThreadsCount - activeThreadsCount + 1, nThreadsPerEffect) );
    }

    return kOfxStatOK;
}

// Function which indicates the index of the current thread
//  This function returns the thread index, which is the same as the threadIndex argument passed to the OfxThreadFunctionV1.
// If there are no threads currently spawned, then this function will set threadIndex to 0
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThreadIndex
// Note that the thread indexes are from 0 to nThreads-1, so a return value of 0 does not mean that it's not a spawned thread
// (use multiThreadIsSpawnedThread() to check if it's a spawned thread)
OfxStatus
OfxHost::multiThreadIndex(unsigned int *threadIndex) const
{
    if (!threadIndex) {
        return kOfxStatFailed;
    }
    OfxHostDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();

    if ( !tls->threadIndexes.empty() && (tls->threadIndexes.back() != -1) ) {
        *threadIndex = tls->threadIndexes.back();
    } else {
        *threadIndex = 0;
    }


    return kOfxStatOK;
}

// Function to enquire if the calling thread was spawned by multiThread
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThreadIsSpawnedThread
int
OfxHost::multiThreadIsSpawnedThread() const
{
    OfxHostDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();

    return !tls->threadIndexes.empty() && tls->threadIndexes.back() != -1;
}

// Create a mutex
//  Creates a new mutex with lockCount locks on the mutex initially set.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexCreate
OfxStatus
OfxHost::mutexCreate(OfxMutexHandle *mutex,
                     int lockCount)
{
    if (!mutex) {
        return kOfxStatFailed;
    }

    // suite functions should not throw
    try {
        OfxSuiteMutex* m = new OfxSuiteMutex();

        for (int i = 0; i < lockCount; ++i) {
            m->mutex.lock();
        }
        *mutex = (OfxMutexHandle)(m);
#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
        {
            QMutexLocker l(_pluginsMutexesLock);
            _pluginsMutexes.push_back(m);
        }
#endif

        return kOfxStatOK;
    } catch (std::bad_alloc&) {
        qDebug() << "mutexCreate(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexCreate(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexCreate(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

// Destroy a mutex
//  Destroys a mutex initially created by mutexCreate.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexDestroy
OfxStatus
OfxHost::mutexDestroy(const OfxMutexHandle mutex)
{
    OfxSuiteMutex* m = suiteMutexFromHandle(mutex);
    if (!m) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
        {
            QMutexLocker l(_pluginsMutexesLock);
            std::list<OfxSuiteMutex*>::iterator found = std::find(_pluginsMutexes.begin(), _pluginsMutexes.end(), m);
            if ( found != _pluginsMutexes.end() ) {
                delete *found;
                _pluginsMutexes.erase(found);
            }
        }
#else
        // mutexCreate() allocates an OfxSuiteMutex, whose QRecursiveMutex is destroyed as one (in Qt6 it
        // does not derive from QMutex)
        delete m;
#endif

        return kOfxStatOK;
    } catch (std::bad_alloc&) {
        qDebug() << "mutexDestroy(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexDestroy(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexDestroy(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

// Blocking lock on the mutex
//  This tries to lock a mutex and blocks the thread it is in until the lock succeeds.
// A successful lock causes the mutex's lock count to be increased by one and to block any other calls to lock the mutex until it is unlocked.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexLock
OfxStatus
OfxHost::mutexLock(const OfxMutexHandle mutex)
{
    OfxSuiteMutex* m = suiteMutexFromHandle(mutex);
    if (!m) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
        m->mutex.lock();

        return kOfxStatOK;
    } catch (std::bad_alloc&) {
        qDebug() << "mutexLock(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexLock(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexLock(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

// Unlock the mutex
//  This unlocks a mutex. Unlocking a mutex decreases its lock count by one.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexUnLock
OfxStatus
OfxHost::mutexUnLock(const OfxMutexHandle mutex)
{
    OfxSuiteMutex* m = suiteMutexFromHandle(mutex);
    if (!m) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
        m->mutex.unlock();

        return kOfxStatOK;
    } catch (std::bad_alloc&) {
        qDebug() << "mutexUnLock(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexUnLock(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexUnLock(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

// Non blocking attempt to lock the mutex
//  This attempts to lock a mutex, if it cannot, it returns and says so, rather than blocking.
// A successful lock causes the mutex's lock count to be increased by one, if the lock did not succeed, the call returns immediately and the lock count remains unchanged.
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_mutexTryLock
OfxStatus
OfxHost::mutexTryLock(const OfxMutexHandle mutex)
{
    OfxSuiteMutex* m = suiteMutexFromHandle(mutex);
    if (!m) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
        if ( m->mutex.tryLock() ) {
            return kOfxStatOK;
        } else {
            return kOfxStatFailed;
        }
    } catch (std::bad_alloc&) {
        qDebug() << "mutexTryLock(): memory error.";

        return kOfxStatErrMemory;
    } catch (const std::exception & e) {
        qDebug() << "mutexTryLock(): " << e.what();

        return kOfxStatErrUnknown;
    } catch (...) {
        qDebug() << "mutexTryLock(): unknown error.";

        return kOfxStatErrUnknown;
    }
}

#endif // ifdef OFX_SUPPORTS_MULTITHREAD

#ifdef OFX_SUPPORTS_DIALOG
// dialog
/// @see OfxDialogSuiteV1.RequestDialog()
OfxStatus
OfxHost::requestDialog(OfxImageEffectHandle instance,
                       OfxPropertySetHandle inArgs,
                       void* instanceData)
{
    Q_UNUSED(inArgs);
    OfxImageEffectInstance *effectInstance = NULL;
    if (instance) {
        OFX::Host::ImageEffect::Base *effectBase = reinterpret_cast<OFX::Host::ImageEffect::Base*>(instance);
        effectInstance = dynamic_cast<OfxImageEffectInstance*>(effectBase);
    } else {
        // A NULL instance handle (OfxDialogSuiteV2 with a NULL handle; HostSupport also passes NULL for
        // OfxDialogSuiteV1, which is not compiled in: OFX_SUPPORTS_DIALOG_V1 is not defined): the effect is the
        // one whose action runs on the requesting thread. It must be found here, on that thread: the request is
        // queued to the main thread, where another effect's action, or none, may be running (#1497).
        effectInstance = _imp->tlsData->getOrCreateTLSData()->effectCallingMainEntry();
    }
    if (!effectInstance) {
        // ofxDialog.h: "kOfxStatFailed - The host has no provision for this or can not deal with it currently."
        return kOfxStatFailed;
    }
    appPTR->requestOFXDIalogOnMainThread(effectInstance, instanceData);

    return kOfxStatOK;
}

/// @see OfxDialogSuiteV1.NotifyRedrawPending()
OfxStatus
OfxHost::notifyRedrawPending(OfxImageEffectHandle instance,
                             OfxPropertySetHandle inArgs)
{
    Q_UNUSED(instance);
    Q_UNUSED(inArgs);

    return kOfxStatReplyDefault;
}

#endif

#ifdef OFX_SUPPORTS_OPENGLRENDER
/// @see OfxImageEffectOpenGLRenderSuiteV1.flushResources()
OfxStatus
OfxHost::flushOpenGLResources() const
{
    // Natron releases no OpenGL resource here. "kOfxStatOK - the host has actually released some
    // resources" would tell the plug-in to retry its allocation: "kOfxStatReplyDefault - nothing the host
    // could do" (ofxGPURender.h).
    return kOfxStatReplyDefault;
}

#endif

NATRON_NAMESPACE_EXIT

