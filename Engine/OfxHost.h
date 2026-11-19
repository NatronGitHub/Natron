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

#ifndef NATRON_ENGINE_OFXHOST_H
#define NATRON_ENGINE_OFXHOST_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
// clang-format off
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
// clang-format on

#include "Global/Enums.h"
#include "Engine/EngineFwd.h"
#include "Engine/Plugin.h"


//#define MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION

NATRON_NAMESPACE_ENTER

struct OfxHostPrivate;
class OfxHost
    : public OFX::Host::ImageEffect::Host
      , private OFX::Host::Property::GetHook
{
    Q_DECLARE_TR_FUNCTIONS(OfxHost)

public:

    OfxHost();

    virtual ~OfxHost();

    void setProperties();

    void setOfxHostOSHandle(void* handle);

    /// The OpenGL render support of a plug-in descriptor or instance, given the support read from its
    /// kOfxImageEffectPropOpenGLRenderSupported: a plug-in that supports OpenGL render ("true") but
    /// sets kOfxImageEffectPropCPURenderSupported (OFX 1.5.1) to "false" can't render on the CPU, so it
    /// needs OpenGL, like a plug-in that sets kOfxImageEffectPropOpenGLRenderSupported to "needed".
    static PluginOpenGLRenderSupport applyCPURenderSupport(PluginOpenGLRenderSupport openGLSupport,
                                                           const OFX::Host::Property::Set& effectProps);

    /// Is a plug-in with this descriptor deprecated, i.e. only kept to load old projects and not offered
    /// to users for new nodes? True if it sets kNatronOfxImageEffectPropDeprecated (Natron extension) or
    /// kOfxImageEffectPluginPropObsolete (OFX, after 1.5.1), which have the same meaning.
    static bool isPluginDeprecated(const OFX::Host::Property::Set& descriptorProps);

    /// Create a new instance of an image effect plug-in.
    ///
    /// It is called by ImageEffectPlugin::createInstance which the
    /// client code calls when it wants to make a new instance.
    ///
    ///   \arg clientData - the clientData passed into the ImageEffectPlugin::createInstance
    ///   \arg plugin - the plugin being created
    ///   \arg desc - the descriptor for that plugin
    ///   \arg context - the context to be created in
    virtual OFX::Host::ImageEffect::Instance* newInstance(void* clientData,
                                                          OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                          OFX::Host::ImageEffect::Descriptor & desc,
                                                          const std::string & context) OVERRIDE;

    /// Override this to create a descriptor, this makes the 'root' descriptor
    virtual OFX::Host::ImageEffect::Descriptor * makeDescriptor(OFX::Host::ImageEffect::ImageEffectPlugin* plugin) OVERRIDE;

    /// used to construct a context description, rootContext is the main context
    virtual OFX::Host::ImageEffect::Descriptor * makeDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                                                                OFX::Host::ImageEffect::ImageEffectPlugin *plug) OVERRIDE;

    /// used to construct populate the cache
    virtual OFX::Host::ImageEffect::Descriptor * makeDescriptor(const std::string &bundlePath,
                                                                OFX::Host::ImageEffect::ImageEffectPlugin *plug) OVERRIDE;

    /// vmessage
    virtual OfxStatus vmessage(const char* type,
                               const char* id,
                               const char* format,
                               va_list args) OVERRIDE;

    /// setPersistentMessage
    virtual OfxStatus setPersistentMessage(const char* type,
                                           const char* id,
                                           const char* format,
                                           va_list args) OVERRIDE;
    /// clearPersistentMessage
    virtual OfxStatus clearPersistentMessage() OVERRIDE;
    virtual void loadingStatus(bool loading, const std::string & pluginId, int versionMajor, int versionMinor) OVERRIDE;

    /// Called by HostSupport just before it calls the OfxSetHost entry point (OFX 1.5) of a plug-in
    /// binary, once per load of the binary, before any of its plug-ins is known: the host name that
    /// OfxSetHost sees (kOfxPropName, see getStringProperty()) is chosen from the binary's bundle
    /// identifier, not from the plug-in that was loaded before.
    virtual void loadingBinary(const std::string& filePath) OVERRIDE;

    /// The CFBundleIdentifier in the Info.plist of the OFX bundle that contains a plug-in binary
    /// (e.g. ".../Foo.ofx.bundle/Contents/MacOS/Foo.ofx"), or "" if there is none.
    static std::string getBundleIdentifier(const std::string& binaryFilePath);

    /// The plug-ins found by the scan stay loaded until exit (HostSupport's default since openfx #174
    /// is to unload each binary after the scan and load it again at first use). Natron keeps them: each
    /// binary gets one OfxSetHost, and each plug-in one kOfxActionLoad and kOfxActionDescribe, kept
    /// until the plug-in cache is cleared at exit, with no dlclose and no reload in between. Unloading
    /// and reloading a plug-in binary is risky: its static destructors run at dlclose (e.g. TBB's
    /// teardown, #1253), and on macOS thread-local statics can keep the image mapped or crash at the
    /// next load. It also avoids loading every new or changed binary twice at startup.
    virtual bool keepPluginsLoadedAfterScan() const OVERRIDE { return true; }

    virtual bool pluginSupported(OFX::Host::ImageEffect::ImageEffectPlugin *plugin, std::string &reason) const OVERRIDE;

    ///fetch the parametric parameters suite or returns the base class version
    virtual const void* fetchSuite(const char *suiteName, int suiteVersion) OVERRIDE;

#ifdef OFX_SUPPORTS_MULTITHREAD
    virtual OfxStatus multiThread(OfxThreadFunctionV1 func, unsigned int nThreads, void *customArg) OVERRIDE;
    virtual OfxStatus multiThreadNumCPUS(unsigned int *nCPUs) const OVERRIDE;
    virtual OfxStatus multiThreadIndex(unsigned int *threadIndex) const OVERRIDE;
    virtual int multiThreadIsSpawnedThread() const OVERRIDE;
    virtual OfxStatus mutexCreate(OfxMutexHandle *mutex, int lockCount) OVERRIDE;
    virtual OfxStatus mutexDestroy(const OfxMutexHandle mutex) OVERRIDE;
    virtual OfxStatus mutexLock(const OfxMutexHandle mutex) OVERRIDE;
    virtual OfxStatus mutexUnLock(const OfxMutexHandle mutex) OVERRIDE;
    virtual OfxStatus mutexTryLock(const OfxMutexHandle mutex) OVERRIDE;
#endif
#ifdef OFX_SUPPORTS_DIALOG
    // dialog suite
    // In OfxDialogSuiteV1, only the host can figure out which effect instance triggered
    // that request.

    /// @see OfxDialogSuiteV1.RequestDialog()
    virtual OfxStatus requestDialog(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs, void* instanceData) OVERRIDE;

    /// @see OfxDialogSuiteV1.NotifyRedrawPending()
    virtual OfxStatus notifyRedrawPending(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs) OVERRIDE;
#endif
#ifdef OFX_SUPPORTS_OPENGLRENDER
    /// @see OfxImageEffectOpenGLRenderSuiteV1.flushResources()
    virtual OfxStatus flushOpenGLResources() const OVERRIDE;
#endif

    virtual OFX::Host::Memory::Instance* newMemoryInstance(size_t nBytes) OVERRIDE FINAL WARN_UNUSED_RETURN;
    AbstractOfxEffectInstancePtr createOfxEffect(NodePtr node,
                                                                 const CreateNodeArgs& args
#ifndef NATRON_ENABLE_IO_META_NODES
                                                                 , bool allowFileDialogs,
                                                                 bool *hasUsedFileDialog
#endif
                                                                 );


    /*Reads OFX plugin cache and scan plugins directories
       to load them all.*/
    void loadOFXPlugins(IOPluginsMap* readersMap,
                        IOPluginsMap* writersMap);

    void clearPluginsLoadedCache();

    void setThreadAsActionCaller(OfxImageEffectInstance* instance, bool actionCaller);

    OFX::Host::ImageEffect::Descriptor* getPluginContextAndDescribe(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                                    ContextEnum* ctx);


    /**
     * @brief A application-wide TLS struct containing all stuff needed to workaround OFX poor specs:
     * missing image effect handles etc...
     **/
    struct OfxHostTLSData
    {
        OfxImageEffectInstance* lastEffectCallingMainEntry;

        ///Stored as int, because we need -1; list because we need it recursive for the multiThread func
        std::list<int> threadIndexes;

        OfxHostTLSData()
            : lastEffectCallingMainEntry(0)
            , threadIndexes()
        {
        }
    };

    typedef std::shared_ptr<OfxHostTLSData> OfxHostDataTLSPtr;

    OfxHostDataTLSPtr getTLSData() const;

private:

    /*Writes all plugins loaded and their descriptors to
       the OFX plugin cache. (called by the destructor) */
    void writeOFXCache();

    // get the virtuals for viewport size, pixel scale, background colour
    const std::string &getStringProperty(const std::string &name, int n) const OFX_EXCEPTION_SPEC OVERRIDE;
    std::unique_ptr<OfxHostPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_ENGINE_OFXHOST_H
