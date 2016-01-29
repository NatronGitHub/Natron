/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include <list>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare)
#include <ofxhPluginCache.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include <ofxhImageEffectAPI.h>

#include "Global/Enums.h"
#include "Engine/EngineFwd.h"


//#define MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION

NATRON_NAMESPACE_ENTER;
    
struct OfxHostPrivate;
class OfxHost
    : public OFX::Host::ImageEffect::Host
    , private OFX::Host::Property::GetHook
{
public:

    OfxHost();

    virtual ~OfxHost();

    void setProperties();
    
    void setOfxHostOSHandle(void* handle);

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
    virtual bool pluginSupported(OFX::Host::ImageEffect::ImageEffectPlugin *plugin, std::string &reason) const OVERRIDE;

    ///fetch the parametric parameters suite or returns the base class version
    virtual const void* fetchSuite(const char *suiteName, int suiteVersion) OVERRIDE;

#ifdef OFX_SUPPORTS_MULTITHREAD
    virtual OfxStatus multiThread(OfxThreadFunctionV1 func,unsigned int nThreads, void *customArg) OVERRIDE;
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
    
    boost::shared_ptr<AbstractOfxEffectInstance> createOfxEffect(NodePtr node,
                                               const NodeSerialization* serialization,
                                                const std::list<boost::shared_ptr<KnobSerialization> >& paramValues,
                                                bool allowFileDialogs,
                                                bool disableRenderScaleSupport,
                                                                 bool *hasUsedFileDialog);


    /*Reads OFX plugin cache and scan plugins directories
       to load them all.*/
    void loadOFXPlugins(std::map<std::string,std::vector< std::pair<std::string,double> > >* readersMap,
                        std::map<std::string,std::vector< std::pair<std::string,double> > >* writersMap);

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
    typedef boost::shared_ptr<OfxHostTLSData> OfxHostDataTLSPtr;
    
    OfxHostDataTLSPtr getTLSData() const;
    
private:
    
    /*Writes all plugins loaded and their descriptors to
     the OFX plugin cache. (called by the destructor) */
    void writeOFXCache();

    // get the virutals for viewport size, pixel scale, background colour
    const std::string &getStringProperty(const std::string &name, int n) const OFX_EXCEPTION_SPEC OVERRIDE;

    boost::scoped_ptr<OfxHostPrivate> _imp;
};
NATRON_NAMESPACE_EXIT;

#endif // ifndef NATRON_ENGINE_OFXHOST_H
