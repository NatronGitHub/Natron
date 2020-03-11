/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include <cassert>
#include <cstdarg>
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
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QThreadPool>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QTemporaryFile>
CLANG_DIAG_ON(deprecated-register)
CLANG_DIAG_ON(uninitialized)

#ifdef OFX_SUPPORTS_MULTITHREAD
#include <QtCore/QThread>
#include <QtCore/QThreadStorage>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif // OFX_SUPPORTS_MULTITHREAD
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

//ofx
#include <ofxParametricParam.h>
#include <ofxOpenGLRender.h>
#ifdef OFX_EXTENSIONS_NUKE
#include <nuke/fnOfxExtensions.h>
#endif
#include <ofxNatron.h>

//ofx host support
#include <ofxhPluginAPICache.h>
// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include <ofxhPluginCache.h>
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>
#include <ofxhParam.h>

#include <tuttle/ofxReadWrite.h>
#include <ofxhPluginCache.h>
#include <ofxhParametricParam.h> //our version of parametric param suite support

#include "Global/GlobalDefines.h"
#include "Global/FStreamsSupport.h"
#include "Global/QtCompat.h"
#include "Global/KeySymbols.h"
#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectDescription.h"
#include "Engine/InputDescription.h"
#include "Engine/LibraryBinary.h"
#include "Engine/MultiThread.h"
#include "Engine/MemoryInfo.h" // printAsRAM
#include "Engine/Node.h"
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

#include "Serialization/NodeSerialization.h"


NATRON_NAMESPACE_ENTER
// to disambiguate with the global-scope ::OfxHost

// see second answer of http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
static
std::string
string_format(const std::string fmt,
              ...)
{
    int size = ( (int)fmt.size() ) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;

    while (1) {     // Maximum two passes on a POSIX system...
        str.reserve(size);
        va_start(ap, fmt);
        int n = vsnprintf( (char *)str.data(), size, fmt.c_str(), ap );
        va_end(ap);
        if ( (n > -1) && (n < size) ) {  // Everything worked
            str.resize(n);

            return str;
        }
        if (n > -1) { // Needed size returned
            size = n + 1;   // For null char
        } else {
            size *= 2;      // Guess at a larger size (OS specific)
        }
    }

    return str;
}

struct OfxHostPrivate
{
    OFX::Host::ImageEffect::PluginCachePtr imageEffectPluginCache;
    boost::shared_ptr<TLSHolder<OfxHost::OfxHostTLSData> > tlsData;

    std::string loadingPluginID; // ID of the plugin being loaded
    int loadingPluginVersionMajor;
    int loadingPluginVersionMinor;

    OfxHostPrivate()
        : imageEffectPluginCache()
        , tlsData()
        , loadingPluginID()
        , loadingPluginVersionMajor(0)
        , loadingPluginVersionMinor(0)
    {
        tlsData = boost::make_shared<TLSHolder<OfxHost::OfxHostTLSData> >();
    }
};

OfxHost::OfxHost()
    : _imp( new OfxHostPrivate() )
{
    _imp->imageEffectPluginCache = boost::make_shared<OFX::Host::ImageEffect::PluginCache>((OFX::Host::ImageEffect::Host*)this);
}

OfxHost::~OfxHost()
{
    //Clean up, to be polite.
    OFX::Host::PluginCache::clearPluginCache();

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
    _properties.setIntProperty(kOfxPropAPIVersion, 1, 0);  //OpenFX API v1.4
    _properties.setIntProperty(kOfxPropAPIVersion, 4, 1);
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

    // Natron distortion suite
    _properties.setIntProperty(kOfxImageEffectPropCanDistort, 1);

    _properties.setIntProperty(kNatronOfxPropDescriptionIsMarkdown, 1);

    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxArrowCursor, 0);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxUpArrowCursor, 1);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxCrossCursor, 2);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxIBeamCursor, 3);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxWaitCursor, 4);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxBusyCursor, 5);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxForbiddenCursor, 6);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxPointingHandCursor, 7);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxWhatsThisCursor, 8);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxSizeVerCursor, 9);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxSizeHorCursor, 10);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxSizeBDiagCursor, 11);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxSizeFDiagCursor, 12);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxSizeAllCursor, 13);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxSplitVCursor, 14);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxSplitHCursor, 15);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxOpenHandCursor, 16);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxClosedHandCursor, 17);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxBlankCursor, 18);
    _properties.setStringProperty(kNatronOfxImageEffectPropDefaultCursors, kNatronOfxDefaultCursor, 19);
#endif
} // OfxHost::setProperties

static
KnownHostNameEnum
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
        return eKnownHostNameNuke;
    } else if ( !pluginID.compare(0, hitfilm.size(), hitfilm) ) {
        // HitFilm plugins (work with Vegas, Resolve and TitlerPro,
        // Vegas and TitlerPro support more plugins than Resolve
        // tested with HitFilm 3.1.0113
        // maj=1 or 2 (depends on plugin), min=0
        // HitFilm Ignite also supports NewBlue OFX Bridge, Sony Catalyst Edit, The Foundry Nuke
        // tested with HitFilm Ignite 1.0.0118
        // Clone Stamp from HitFilm Ignite is officially only compatible with Nuke
        if ( pluginID == (hitfilm + ".CloneStamp") ) {
            return eKnownHostNameNuke;
        }

        return eKnownHostNameVegas;
    } else if ( !pluginID.compare(0, redgiant.size(), redgiant) ) {
        // Red Giant Universe plugins 1.5 work with Vegas and Resolve
        return eKnownHostNameVegas;
    } else if ( !pluginID.compare(0, digitalfilmtools.size(), digitalfilmtools) ||
                !pluginID.compare(0, tiffen.size(), tiffen) ) {
        // Digital film tools plug-ins work with Nuke, Vegas, Scratch and Resolve

        // http://www.digitalfilmtools.com/supported-hosts/ofx-host-plugins.php
        return eKnownHostNameNuke;
        //} else if (!pluginID.compare(0, digitalanarchy.size(), digitalanarchy)) {
        //    // Digital Anarchy plug-ins work with Scratch, Resolve, and any OFX host, but they are tested with Scratch.
        //    // http://digitalanarchy.com/demos/psd_mac.html
        //    return Settings::eKnownHostNameScratch;
    }
    //printf("%s v%d.%d\n", pluginID.c_str(), pluginVersionMajor, pluginVersionMinor);

    return eKnownHostNameNone;
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
        if ( !_imp->loadingPluginID.empty() ) {
            // plugin is not yet created: we are loading or describing it
            pluginID = _imp->loadingPluginID;
            pluginVersionMajor = _imp->loadingPluginVersionMajor;
            pluginVersionMinor = _imp->loadingPluginVersionMinor;
        } else {
            OfxEffectInstancePtr effect = getCurrentEffect_TLS();
            if (effect) {
                OfxImageEffectInstance* ofxEffect = effect->effectInstance();
                pluginID = ofxEffect->getPlugin()->getIdentifier();
                pluginVersionMajor = ofxEffect->getPlugin()->getVersionMajor();
                pluginVersionMinor = ofxEffect->getPlugin()->getVersionMinor();
            }
        }

        ///Proxy known plug-ins that filter hostnames
        if ( pluginID.empty() ) {
            qDebug() << "OfxHost::getStringProperty(" kOfxPropName "): Error: no plugin ID! (ignoring)";
        } else {
            KnownHostNameEnum e = getHostNameProxy(pluginID, pluginVersionMajor, pluginVersionMinor);
            if (e != eKnownHostNameNone) {
                const std::string& ret = appPTR->getCurrentSettings()->getKnownHostName(e);

                return ret;
            }
        }

        // kOfxPropName was set at host creation, let the value decided by the user
        return _properties.getStringPropertyRaw(kOfxPropName);
    } else {
        throw OFX::Host::Property::Exception(kOfxStatErrValue);
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
    std::string message = string_format(format, args);
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
    }

    return kOfxStatReplyDefault;
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

static std::string
getContext_internal(const std::set<std::string> & contexts)
{
    std::string context;

    if (contexts.size() == 0) {
        throw std::runtime_error( std::string("Error: Plug-in does not support any context") );
        //context = kOfxImageEffectContextGeneral;
        //plugin->addContext(kOfxImageEffectContextGeneral);
    } else if (contexts.size() == 1) {
        context = ( *contexts.begin() );

        return context;
    } else {
        std::set<std::string>::iterator found = contexts.find(kOfxImageEffectContextReader);
        bool reader = found != contexts.end();
        if (reader) {
            context = kOfxImageEffectContextReader;

            return context;
        }

        found = contexts.find(kOfxImageEffectContextWriter);
        bool writer = found != contexts.end();
        if (writer) {
            context = kOfxImageEffectContextWriter;

            return context;
        }

        found = contexts.find(kNatronOfxImageEffectContextTracker);
        bool tracker = found != contexts.end();
        if (tracker) {
            context = kNatronOfxImageEffectContextTracker;

            return context;
        }


        found = contexts.find(kOfxImageEffectContextGeneral);
        bool general = found != contexts.end();
        if (general) {
            context = kOfxImageEffectContextGeneral;

            return context;
        }

        found = contexts.find(kOfxImageEffectContextFilter);
        bool filter = found != contexts.end();
        if (filter) {
            context = kOfxImageEffectContextFilter;

            return context;
        }

        found = contexts.find(kOfxImageEffectContextPaint);
        bool paint = found != contexts.end();
        if (paint) {
            context = kOfxImageEffectContextPaint;

            return context;
        }

        found = contexts.find(kOfxImageEffectContextGenerator);
        bool generator = found != contexts.end();
        if (generator) {
            context = kOfxImageEffectContextGenerator;

            return context;
        }

        found = contexts.find(kOfxImageEffectContextTransition);
        bool transition = found != contexts.end();
        if (transition) {
            context = kOfxImageEffectContextTransition;

            return context;
        }
    }

    return context;
} // getContext_internal

OFX::Host::ImageEffect::Descriptor*
OfxHost::getPluginContextAndDescribe(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                     ContextEnum* ctx)
{
    _imp->loadingPluginID = plugin->getRawIdentifier();
    _imp->loadingPluginVersionMajor = plugin->getVersionMajor();
    _imp->loadingPluginVersionMinor = plugin->getVersionMajor();

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

    const std::set<std::string> & contexts = plugin->getContexts();
    std::string context = getContext_internal(contexts);
    if ( context.empty() ) {
        throw std::invalid_argument( tr("OpenFX plug-in does not have any valid context.").toStdString() );
    }

    OFX::Host::PluginHandle* ph = plugin->getPluginHandle();
    assert( ph->getOfxPlugin() );
    assert(ph->getOfxPlugin()->mainEntry);
    Q_UNUSED(ph);
    OFX::Host::ImageEffect::Descriptor* desc = NULL;
    //This will call kOfxImageEffectActionDescribeInContext
    desc = plugin->getContext(context);
    if (!desc) {
        throw std::runtime_error( tr("Plug-in parameters and inputs description (kOfxImageEffectActionDescribeInContext) failed in context %1.")
                                  .arg( QString::fromUtf8( context.c_str() ) ).toStdString() );
    }


    *ctx = OfxEffectInstance::mapToContextEnum(context);
    _imp->loadingPluginID.clear();

    return desc;
} // OfxHost::getPluginContextAndDescribe


///Return the xml cache file used before Natron 2 RC2
static QString
getOldCacheFilePath()
{
    QString cachePath = QString::fromUtf8( appPTR->getCacheDirPath().c_str() ) + QLatin1Char('/');
    QString oldOfxCache = cachePath + QString::fromUtf8("OFXCache.xml");

    return oldOfxCache;
}

static QString
getOFXCacheDirPath()
{
    QString cachePath = QString::fromUtf8( appPTR->getCacheDirPath().c_str() ) + QLatin1Char('/');
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
        shortcuts->push_back(PluginActionShortcut(paramName, foundParamDesc->second->getLabel(), foundParamDesc->second->getHint(), eSymbol, eMods));
    }
} // getPluginShortcuts

static inline
QDebug operator<<(QDebug dbg, const std::list<std::string> &l)
{
    for (std::list<std::string>::const_iterator it = l.begin(); it != l.end(); ++it) {
        dbg.nospace() << QString::fromUtf8( it->c_str() ) << ' ';
    }

    return dbg.space();
}


void
OfxHost::loadOFXPlugins()
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
    pluginCache->setCacheVersion(NATRON_APPLICATION_NAME "OFXCachev1");

    /// register the image effect cache with the global plugin cache
    _imp->imageEffectPluginCache->registerInCache( *pluginCache );


    pluginCache->setPluginHostPath(NATRON_APPLICATION_NAME);
    pluginCache->setPluginHostPath("Nuke"); // most Nuke OFX plugins are compatible
    std::list<std::string> extraPluginsSearchPaths;
    settings->getOpenFXPluginsSearchPaths(&extraPluginsSearchPaths);
    for (std::list<std::string>::iterator it = extraPluginsSearchPaths.begin(); it != extraPluginsSearchPaths.end(); ++it) {
        if ( !(*it).empty() ) {
            qDebug() << "Load OFX Plugins: append extra plugins dir" << it->c_str();
            pluginCache->addFileToPath(*it);
        }
    }

    // if Natron is /usr/bin/Natron, /usr/bin/../OFX/Natron points to Natron-specific plugins
    QDir pluginsDir = appPTR->getBundledPluginDirectory();
    pluginsDir.cd(QString::fromUtf8("OFX/" NATRON_APPLICATION_NAME));
    std::string natronBundledPluginsPath = pluginsDir.absolutePath().toStdString();
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
    _imp->loadingPluginID.clear(); // finished loading plugins

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
        if (p->getContexts().size() == 0) {
            continue;
        }
        assert( p->getBinary() );
        if ( !p->getBinary() ) {
            continue;
        }

        const std::string& openfxId = p->getIdentifier();
        const std::string & grouping = p->getDescriptor().getPluginGrouping();
        const std::string & bundlePath = p->getBinary()->getBundlePath();
        std::string pluginLabel = OfxEffectInstance::makePluginLabel( p->getDescriptor().getShortLabel(),
                                                                      p->getDescriptor().getLabel(),
                                                                      p->getDescriptor().getLongLabel() );
        std::vector<std::string> groups = OfxEffectInstance::makePluginGrouping(p->getIdentifier(),
                                                                   p->getVersionMajor(), p->getVersionMinor(),
                                                                   pluginLabel, grouping);
        const std::string resourcesPath(bundlePath + "/Contents/Resources/");
        std::string iconFileName;
        {
            try {
                // kOfxPropIcon is normally only defined for parameter desctriptors
                // (see <http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ParameterProperties>)
                // but let's assume it may also be defained on the plugin descriptor.
                iconFileName = p->getDescriptor().getProps().getStringProperty(kOfxPropIcon, 1); // dimension 1 is PNG icon
            } catch (OFX::Host::Property::Exception) {
            }

            if ( iconFileName.empty() ) {
                // no icon defined by kOfxPropIcon, use the plug-in id value
                iconFileName = openfxId + ".png";
            }
        }
        std::string groupIconFilename;
        if (groups.size() > 0) {
            groupIconFilename = resourcesPath;
            // the plugin grouping has no descriptor, just try the default filename.
            groupIconFilename.append(groups[0]);
            groupIconFilename.append(".png");
        } else {
            //Use default Misc group when the plug-in doesn't belong to a group
            groups.push_back(PLUGIN_GROUP_DEFAULT);
        }
        std::vector<std::string> groupIcons;
        groupIcons.push_back(groupIconFilename);
        for (std::size_t i = 1; i < groups.size(); ++i) {
            std::string groupIconPath = resourcesPath;
            for (std::size_t j = 0; j <= i; ++j) {
                groupIconPath += groups[j];
                if (j < i) {
                    groupIconPath += '/';
                } else {
                    groupIconPath.append(".png");
                }
            }
            groupIcons.push_back(groupIconPath);
        }

        const bool isDeprecated = p->getDescriptor().isDeprecated();
        std::string description = p->getDescriptor().getProps().getStringProperty(kOfxPropPluginDescription);

        bool isDescMarkdown = (bool)p->getDescriptor().getProps().getIntProperty(kNatronOfxPropDescriptionIsMarkdown);

        PluginPtr natronPlugin = Plugin::create(OfxEffectInstance::create, OfxEffectInstance::createRenderClone, openfxId, pluginLabel, p->getVersionMajor(), p->getVersionMinor(), groups, groupIcons);
        natronPlugin->setProperty<std::string>(kNatronPluginPropDescription, description);
        natronPlugin->setProperty<bool>(kNatronPluginPropDescriptionIsMarkdown, isDescMarkdown);
        natronPlugin->setProperty<std::string>(kNatronPluginPropResourcesPath, resourcesPath);
        natronPlugin->setProperty<std::string>(kNatronPluginPropIconFilePath, iconFileName);
        natronPlugin->setProperty<bool>(kNatronPluginPropIsDeprecated, isDeprecated);
        natronPlugin->setProperty<void*>(kNatronPluginPropOpenFXPluginPtr, (void*)p);


        std::list<PluginActionShortcut> shortcuts;
        getPluginShortcuts(p->getDescriptor(), &shortcuts);
        for (std::list<PluginActionShortcut>::iterator it = shortcuts.begin(); it!=shortcuts.end(); ++it) {
            natronPlugin->addActionShortcut(*it);
        }

        Key symbol = (Key)0;
        KeyboardModifiers mods = eKeyboardModifierNone;
        if (openfxId == PLUGINID_OFX_TRANSFORM) {
            symbol = Key_T;
        } else if (openfxId == PLUGINID_OFX_MERGE) {
            symbol = Key_M;
        } else if (openfxId == PLUGINID_OFX_GRADE) {
            symbol = Key_G;
        } else if (openfxId == PLUGINID_OFX_COLORCORRECT) {
            symbol = Key_C;
        } else if (openfxId == PLUGINID_OFX_BLURCIMG) {
            symbol = Key_B;
        }

        if (openfxId == PLUGINID_OFX_ROTOMERGE) {
            // RotoMerge is to be used only be the RotoPaint tree
            natronPlugin->setProperty<bool>(kNatronPluginPropIsInternalOnly, true);
        }

        natronPlugin->setProperty<int>(kNatronPluginPropShortcut, (int)symbol, 0);
        natronPlugin->setProperty<int>(kNatronPluginPropShortcut, (int)mods, 1);

        ///if this plugin's descriptor has the kTuttleOfxImageEffectPropSupportedExtensions property,
        ///use it to fill the readersMap and writersMap
        int formatsCount = p->getDescriptor().getProps().getDimension(kTuttleOfxImageEffectPropSupportedExtensions);
        std::vector<std::string> formats(formatsCount);
        for (int k = 0; k < formatsCount; ++k) {
            formats[k] = p->getDescriptor().getProps().getStringProperty(kTuttleOfxImageEffectPropSupportedExtensions, k);
            std::transform(formats[k].begin(), formats[k].end(), formats[k].begin(), ::tolower);
        }

        natronPlugin->setPropertyN<std::string>(kNatronPluginPropSupportedExtensions, formats);

        double evaluation = p->getDescriptor().getProps().getDoubleProperty(kTuttleOfxImageEffectPropEvaluation);

        natronPlugin->setProperty<double>(kNatronPluginPropIOEvaluation, evaluation);

        appPTR->registerPlugin(natronPlugin);
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
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QtCompat::removeRecursively( getOFXCacheDirPath() );
#else
    QDir OFXCacheDir( getOFXCacheDirPath() );
    OFXCacheDir.removeRecursively();
#endif
}

void
OfxHost::loadingStatus(bool loading,
                       const std::string & pluginId,
                       int versionMajor,
                       int versionMinor)
{
    // set the pluginID in case the plug-in tries to fetch the hostname property
    _imp->loadingPluginID = pluginId;
    _imp->loadingPluginVersionMajor = versionMajor;
    _imp->loadingPluginVersionMinor = versionMinor;
    if (loading && appPTR) {
        appPTR->setLoadingStatus( QString::fromUtf8("OpenFX: loading ") + QString::fromUtf8( pluginId.c_str() ) + QString::fromUtf8(" v") + QString::number(versionMajor) + QLatin1Char('.') + QString::number(versionMinor) );
#     ifdef DEBUG
        qDebug() << QString::fromUtf8("OpenFX: loading ") + QString::fromUtf8( pluginId.c_str() ) + QString::fromUtf8(" v") + QString::number(versionMajor) + QLatin1Char('.') + QString::number(versionMinor);
#     endif
    }
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
    OfxMemory* ret = new OfxMemory(EffectInstancePtr());
    bool allocated = ret->alloc(nBytes);

    if ( ( (nBytes != 0) && !ret->getPtr() ) || !allocated ) {
        Dialogs::errorDialog( tr("Out of memory").toStdString(),
                              tr("Failed to allocate memory (%1).").arg( printAsRAM(nBytes) ).toStdString() );
    }

    return ret;
}

/////////////////
/////////////////////////////////////////////////// MULTI_THREAD SUITE ///////////////////////////////////////////////////
/////////////////


#ifdef OFX_SUPPORTS_MULTITHREAD

void
OfxHost::setOFXLastActionCaller_TLS(const OfxEffectInstancePtr& effect)
{
    OfxHostDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    if (effect) {
        tls->effectActionsStack.push_back(effect);
    } else {
        assert(!tls->effectActionsStack.empty());
        tls->effectActionsStack.pop_back();
    }

}

OfxEffectInstancePtr
OfxHost::getCurrentEffect_TLS() const
{
    OfxHostDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls || tls->effectActionsStack.empty()) {
        return OfxEffectInstancePtr();
    }
    return tls->effectActionsStack.back();
}

struct OfxFunctorArgs
{
    void* customArg;
    OfxEffectInstancePtr effect;
    OfxThreadFunctionV1* ofxFunc;
};

static ActionRetCodeEnum ofxMultiThreadFunctor(unsigned int threadIndex,
                                               unsigned int threadMax,
                                               void *customArg)
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    OfxFunctorArgs* args = (OfxFunctorArgs*)customArg;
    ThreadIsActionCaller_RAII actionCallerRaii(args->effect);
    try {
        args->ofxFunc(threadIndex, threadMax, args->customArg);
        return eActionStatusOK;
    } catch (...) {
        return eActionStatusFailed;
    }

} // ofxMultiThreadFunctor


OfxStatus
OfxHost::multiThread(OfxThreadFunctionV1 func,
                     unsigned int nThreads,
                     void *customArg)
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    if (!func) {
        return kOfxStatFailed;
    }

    // Recover the effect calling multiThread using TLS
    OfxEffectInstancePtr effect = getCurrentEffect_TLS();

    OfxFunctorArgs args;
    args.ofxFunc = func;
    args.effect = effect;
    args.customArg = customArg;
    // OpenFX does not pass the image effect instance pointer back to us so we loose the context...
    // The only way to recover it will be in the aborted() function entry point with help of thread local storage.
    ActionRetCodeEnum stat = MultiThread::launchThreadsBlocking(ofxMultiThreadFunctor, nThreads, (void*)&args, effect);
    if (stat == eActionStatusFailed) {
        return kOfxStatFailed;
    } else if (stat == eActionStatusOutOfMemory) {
        return kOfxStatErrMemory;
    } else {
        return kOfxStatOK;
    }
} // multiThread

OfxStatus
OfxHost::multiThreadNumCPUS(unsigned int *nCPUs) const
{
    if (!nCPUs) {
        return kOfxStatFailed;
    }

    // Always return the max num CPUs, let multiThread handle the actual threading
    *nCPUs = appPTR->getMaxThreadCount();//MultiThread::getNCPUsAvailable(getCurrentEffect_TLS());
    return kOfxStatOK;
}

OfxStatus
OfxHost::multiThreadIndex(unsigned int *threadIndex) const
{
    if (!threadIndex) {
        return kOfxStatFailed;
    }
    ActionRetCodeEnum stat = MultiThread::getCurrentThreadIndex(threadIndex);
    if (stat == eActionStatusFailed) {
        return kOfxStatFailed;
    }
    return kOfxStatOK;
}

int
OfxHost::multiThreadIsSpawnedThread() const
{
    return (int)MultiThread::isCurrentThreadSpawnedThread();
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
        QMutex* m = new QMutex(QMutex::Recursive);
        for (int i = 0; i < lockCount; ++i) {
            m->lock();
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
    if (mutex == 0) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
#ifdef MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION
        const QMutex* mutexqt = reinterpret_cast<const QMutex*>(mutex);
        {
            QMutexLocker l(_pluginsMutexesLock);
            std::list<QMutex*>::iterator found = std::find(_pluginsMutexes.begin(), _pluginsMutexes.end(), mutexqt);
            if ( found != _pluginsMutexes.end() ) {
                delete *found;
                _pluginsMutexes.erase(found);
            }
        }
#else
        delete reinterpret_cast<const QMutex*>(mutex);
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
    if (mutex == 0) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
        reinterpret_cast<QMutex*>(mutex)->lock();

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
    if (mutex == 0) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
        reinterpret_cast<QMutex*>(mutex)->unlock();

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
    if (mutex == 0) {
        return kOfxStatErrBadHandle;
    }
    // suite functions should not throw
    try {
        if ( reinterpret_cast<QMutex*>(mutex)->tryLock() ) {
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
    OFX::Host::ImageEffect::Base *effectBase = reinterpret_cast<OFX::Host::ImageEffect::Base*>(instance);
    OfxImageEffectInstance *effectInstance = dynamic_cast<OfxImageEffectInstance*>(effectBase);
    if (effectInstance) {
        appPTR->requestOFXDIalogOnMainThread(appPTR->getOFXCurrentEffect_TLS(), instanceData);
    }

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
    return kOfxStatOK;
}

#endif

NATRON_NAMESPACE_EXIT
