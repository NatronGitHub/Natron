//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
//  Created by Frédéric Devernay on 03/09/13.
//
//

#include "OfxHost.h"

#include <cassert>
#include <fstream>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#if QT_VERSION < 0x050000
#include <QtGui/QDesktopServices>
#else
#include <QStandardPaths>
#endif

#include <boost/thread.hpp>

//ofx
#include <ofxParametricParam.h>

//ofx host support
#include <ofxhPluginAPICache.h>
#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>
#include <ofxhParam.h>

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"

#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/KnobTypes.h"

//our version of parametric param suite support
#include "ofxhParametricParam.h"

using namespace Natron;

Natron::OfxHost::OfxHost()
:_imageEffectPluginCache(*this)
{
    _properties.setStringProperty(kOfxPropName,NATRON_APPLICATION_NAME "Host");
    _properties.setStringProperty(kOfxPropLabel, NATRON_APPLICATION_NAME);
    _properties.setIntProperty(kOfxPropAPIVersion, 1 , 0); //API v1.0
    _properties.setIntProperty(kOfxPropAPIVersion, 0 , 1);
    _properties.setIntProperty(kOfxPropVersion, NATRON_VERSION_MAJOR , 0); //Software version v1.0
    _properties.setIntProperty(kOfxPropVersion, NATRON_VERSION_MINOR , 1);
    _properties.setStringProperty(kOfxPropVersionLabel, NATRON_VERSION_STRING);
    _properties.setIntProperty(kOfxImageEffectHostPropIsBackground, 0);
    _properties.setIntProperty(kOfxImageEffectPropSupportsOverlays, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultiResolution, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsTiles, 1);
    _properties.setIntProperty(kOfxImageEffectPropTemporalClipAccess, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentRGBA, 0);
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGenerator, 0 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextFilter, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGeneral, 2 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextTransition, 3 );
    
    _properties.setStringProperty(kOfxImageEffectPropSupportedPixelDepths,kOfxBitDepthFloat,0);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipDepths, 0); 
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipPARs, 0);
    _properties.setIntProperty(kOfxImageEffectPropSetableFrameRate, 0);
    _properties.setIntProperty(kOfxImageEffectPropSetableFielding, 0);
    _properties.setIntProperty(kOfxParamHostPropSupportsCustomInteract, 1 );
    _properties.setIntProperty(kOfxParamHostPropSupportsStringAnimation, String_Knob::canAnimateStatic());
    _properties.setIntProperty(kOfxParamHostPropSupportsChoiceAnimation, Choice_Knob::canAnimateStatic());
    _properties.setIntProperty(kOfxParamHostPropSupportsBooleanAnimation, Bool_Knob::canAnimateStatic());
    _properties.setIntProperty(kOfxParamHostPropSupportsCustomAnimation, 0 /*Custom_Knob::canAnimateStatic()*/);
    _properties.setIntProperty(kOfxParamHostPropMaxParameters, -1);
    _properties.setIntProperty(kOfxParamHostPropMaxPages, 0);
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 0 );
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 1 );
    _properties.setIntProperty(kOfxImageEffectInstancePropSequentialRender, 0);
    _properties.setIntProperty(kOfxParamHostPropSupportsParametricAnimation, 0);

}

Natron::OfxHost::~OfxHost()
{
    //Clean up, to be polite.
    OFX::Host::PluginCache::clearPluginCache();
}

OFX::Host::ImageEffect::Instance* Natron::OfxHost::newInstance(void* ,
                                                                OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                                OFX::Host::ImageEffect::Descriptor& desc,
                                                                const std::string& context)
{
    assert(plugin);
    
    
    
    return new Natron::OfxImageEffectInstance(plugin,desc,context,false);
}

/// Override this to create a descriptor, this makes the 'root' descriptor
OFX::Host::ImageEffect::Descriptor *Natron::OfxHost::makeDescriptor(OFX::Host::ImageEffect::ImageEffectPlugin* plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OFX::Host::ImageEffect::Descriptor(plugin);
    return desc;
}

/// used to construct a context description, rootContext is the main context
OFX::Host::ImageEffect::Descriptor *Natron::OfxHost::makeDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                                                                     OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OFX::Host::ImageEffect::Descriptor(rootContext, plugin);
    return desc;
}

/// used to construct populate the cache
OFX::Host::ImageEffect::Descriptor *Natron::OfxHost::makeDescriptor(const std::string &bundlePath,
                                                                     OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OFX::Host::ImageEffect::Descriptor(bundlePath, plugin);
    return desc;
}



/// message
OfxStatus Natron::OfxHost::vmessage(const char* type,
                                     const char* ,
                                     const char* format,
                                     va_list args)
{
    assert(type);
    assert(format);
    char buf[10000];
    sprintf(buf, format,args);
    std::string message(buf);
    
    if (strcmp(type, kOfxMessageLog) == 0) {
        
        std::cout << message << std::endl;
        
    }else if(strcmp(type, kOfxMessageFatal) == 0 ||
             strcmp(type, kOfxMessageError) == 0) {
        
        Natron::errorDialog(NATRON_APPLICATION_NAME, message);
        
    }else if(strcmp(type, kOfxMessageWarning)){
        
        Natron::warningDialog(NATRON_APPLICATION_NAME, message);
        
    }else if(strcmp(type, kOfxMessageMessage)){
        
        Natron::informationDialog(NATRON_APPLICATION_NAME, message);
        
    }else if(strcmp(type, kOfxMessageQuestion) == 0) {
        
        if(Natron::questionDialog(NATRON_APPLICATION_NAME, message) == Natron::Yes){
            return kOfxStatReplyYes;
        }else{
            return kOfxStatReplyNo;
        }
        
    }
    return kOfxStatReplyDefault;
}

OfxStatus Natron::OfxHost::setPersistentMessage(const char* type,
                                                 const char* id,
                                                 const char* format,
                                                 va_list args){
    vmessage(type,id,format,args);
    return kOfxStatOK;
}

/// clearPersistentMessage
OfxStatus Natron::OfxHost::clearPersistentMessage(){
    return kOfxStatOK;
}

OfxEffectInstance* Natron::OfxHost::createOfxEffect(const std::string& name,Natron::Node* node) {
    assert(node); // the efgfect_ member should be owned by a Node
    // throws out_of_range if the plugin does not exist
    const OFXPluginEntry& ofxPlugin = _ofxPlugins.at(name);

    OFX::Host::ImageEffect::ImageEffectPlugin* plugin = _imageEffectPluginCache.getPluginById(ofxPlugin.openfxId);
    if (!plugin) {
        throw std::runtime_error(std::string("Error: Could not get plugin ") + ofxPlugin.openfxId);
    }

    // getPluginHandle() must be called before getContexts():
    // it calls kOfxActionLoad on the plugin, which may set properties (including supported contexts)
    OFX::Host::PluginHandle *ph;
    try {
        ph = plugin->getPluginHandle();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error: Could not get plugin handle for plugin ") + name + ": " + e.what());
    } catch (...) {
        throw std::runtime_error(std::string("Error: Could not get plugin handle for plugin ") + name);
    }
    if(!ph) {
        throw std::runtime_error(std::string("Error: Could not get plugin handle for plugin ") + name);
    }
    assert(ph->getOfxPlugin() && ph->getOfxPlugin()->mainEntry);

    const std::set<std::string>& contexts = plugin->getContexts();
    std::string context;
    
    if (contexts.size() == 0) {
        throw std::runtime_error(std::string("Error: Plugins supports no context"));
        //context = kOfxImageEffectContextGeneral;
        //plugin->addContext(kOfxImageEffectContextGeneral);
    } else if (contexts.size() == 1) {
        context = (*contexts.begin());
    } else {
        std::set<std::string>::iterator found = contexts.find(kOfxImageEffectContextGeneral);
        if(found != contexts.end()){
            context = *found;
        }else{
            found = contexts.find(kOfxImageEffectContextFilter);
            if(found != contexts.end()){
                context = *found;
            }else{
                found = contexts.find(kOfxImageEffectContextGenerator);
                if(found != contexts.end()){
                    context = *found;
                }else{
                    found = contexts.find(kOfxImageEffectContextTransition);
                    if(found != contexts.end()){
                        context = *found;
                    }else{
                        context = kOfxImageEffectContextPaint;
                    }
                }
            }
        }
        
    }

    OfxEffectInstance* hostSideEffect = new OfxEffectInstance(node);
    if(node){
        hostSideEffect->createOfxImageEffectInstance(plugin, context);
    }else{
        
        //if node is NULL that means we're just checking if the effect is describing and loading OK
        OFX::Host::ImageEffect::Descriptor* desc = plugin->getContext(context);
        if(!desc){
            throw std::runtime_error(std::string("Error: Could not get description for plugin ") + name + " in context " + context);
        }
    }

    return hostSideEffect;
}

void Natron::OfxHost::loadOFXPlugins(std::vector<Natron::Plugin*>* plugins) {
    
    assert(OFX::Host::PluginCache::getPluginCache());
    /// set the version label in the global cache
    OFX::Host::PluginCache::getPluginCache()->setCacheVersion(NATRON_APPLICATION_NAME "OFXCachev1");
    
    /// make an image effect plugin cache
    
    /// register the image effect cache with the global plugin cache
    _imageEffectPluginCache.registerInCache(*OFX::Host::PluginCache::getPluginCache());
    
    
#if defined(WINDOWS)
    OFX::Host::PluginCache::getPluginCache()->addFileToPath("C:\\Program Files\\Common Files\\OFX\\Nuke");
#endif
#if defined(__linux__)
    OFX::Host::PluginCache::getPluginCache()->addFileToPath("/usr/OFX/Nuke");
#endif
#if defined(__APPLE__)
    OFX::Host::PluginCache::getPluginCache()->addFileToPath("/Library/OFX/Nuke");
#endif
    
    /// now read an old cache
    // The cache location depends on the OS.
    // On OSX, it will be ~/Library/Caches/<organization>/<application>/OFXCache.xml
#if QT_VERSION < 0x050000
    QString ofxcachename = QDesktopServices::storageLocation(QDesktopServices::CacheLocation) + QDir::separator() + "OFXCache.xml";
#else
    QString ofxcachename = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator() + "OFXCache.xml";
#endif
    std::ifstream ifs(ofxcachename.toStdString().c_str());
    if (ifs.is_open()) {
        OFX::Host::PluginCache::getPluginCache()->readCache(ifs);
        ifs.close();
    }
    OFX::Host::PluginCache::getPluginCache()->scanPluginFiles();

    // write the cache NOW (it won't change anyway)
    /// flush out the current cache
    writeOFXCache();

    /*Filling node name list and plugin grouping*/
    const std::vector<OFX::Host::ImageEffect::ImageEffectPlugin *>& ofxPlugins = _imageEffectPluginCache.getPlugins();
    for (unsigned int i = 0 ; i < ofxPlugins.size(); ++i) {
        OFX::Host::ImageEffect::ImageEffectPlugin* p = ofxPlugins[i];
        assert(p);
        if(p->getContexts().size() == 0)
            continue;
       
        QString openfxId = p->getIdentifier().c_str();
        const std::string& grouping = p->getDescriptor().getPluginGrouping();
        
        const std::string& bundlePath = p->getBinary()->getBundlePath();
        const std::string& pluginLabel = OfxEffectInstance::getPluginLabel(p->getDescriptor().getShortLabel(),
                                                                    p->getDescriptor().getLabel(),
                                                                    p->getDescriptor().getLongLabel());
     
        
        std::string pluginId = OfxEffectInstance::generateImageEffectClassName(p->getDescriptor().getShortLabel(),
                                                                               p->getDescriptor().getLabel(),
                                                                               p->getDescriptor().getLongLabel(),
                                                                           grouping);

       
        
        QStringList groups = OfxEffectInstance::getPluginGrouping(pluginLabel, grouping);
        
        assert(p->getBinary());
        QString iconFilename = QString(bundlePath.c_str()) + "/Contents/Resources/";
        iconFilename.append(p->getDescriptor().getProps().getStringProperty(kOfxPropIcon,1).c_str());
        iconFilename.append(openfxId);
        iconFilename.append(".png");
        QString groupIconFilename;
        if (groups.size() > 0) {
            groupIconFilename = QString(p->getBinary()->getBundlePath().c_str()) + "/Contents/Resources/";
            groupIconFilename.append(p->getDescriptor().getProps().getStringProperty(kOfxPropIcon,1).c_str());
            groupIconFilename.append(groups[0]);
            groupIconFilename.append(".png");
        }
        
        _ofxPlugins[pluginId] = OFXPluginEntry(openfxId.toStdString(), grouping);

        // The following was commented out because:
        // - loading all plugins on startup may take a LOT of time
        // - it makes the plugin cache basically useless
        // - all OfxEffectInstance should be owned by a node in Natron (this is crazy. where are the smart pointers?)
        // If a plugin is not supported, the user gets an error when instanciating the plugin, which is the Right Thing To Do (TM).
#if 0
        //try to instantiate an effect for this plugin, if it crashes, don't add it
        OfxEffectInstance* tryInstance = 0;
        try {
            tryInstance = createOfxEffect(pluginId, NULL);
        } catch (const std::exception& e) {
            qDebug() << "Exception while loading " << pluginId.c_str() << ": " << e.what();
            delete tryInstance;
            tryInstance = 0;
        } catch (...) {
            qDebug() << "Exception while loading " << pluginId.c_str();
            delete tryInstance;
            tryInstance = 0;
        }

        if (!tryInstance) {
            qDebug() << "Error loading " << pluginId.c_str();
            _ofxPlugins.erase(pluginId);
            continue;
        } else {
            delete tryInstance;
        }
#endif
        emit toolButtonAdded(groups,pluginId.c_str(), pluginLabel.c_str(), iconFilename, groupIconFilename);
        QMutex* pluginMutex = NULL;
        if(p->getDescriptor().getRenderThreadSafety() == kOfxImageEffectRenderUnsafe){
            pluginMutex = new QMutex;
        }
        Natron::Plugin* plugin = new Natron::Plugin(new Natron::LibraryBinary(Natron::LibraryBinary::BUILTIN),
                                                    pluginId.c_str(),pluginLabel.c_str(),pluginMutex,p->getVersionMajor(),
                                                    p->getVersionMinor());
        plugins->push_back(plugin);
    }
}



void Natron::OfxHost::writeOFXCache(){
    /// and write a new cache, long version with everything in there
#if QT_VERSION < 0x050000
    QString ofxcachename = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
    
#else
    QString ofxcachename = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#endif
    QDir().mkpath(ofxcachename);
    ofxcachename +=  QDir::separator();
    ofxcachename += "OFXCache.xml";
    std::ofstream of(ofxcachename.toStdString().c_str());
    assert(of.is_open());
    assert(OFX::Host::PluginCache::getPluginCache());
    OFX::Host::PluginCache::getPluginCache()->writePluginCache(of);
    of.close();
}


void Natron::OfxHost::loadingStatus(const std::string & pluginId) {
    qDebug() << "Loading OFX plugin " << pluginId.c_str();
}

void* Natron::OfxHost::fetchSuite(const char *suiteName, int suiteVersion) {
    if (strcmp(suiteName, kOfxParametricParameterSuite)==0  && suiteVersion == 1) {
        return OFX::Host::ParametricParam::GetSuite(suiteVersion);
    }else{
        return OFX::Host::ImageEffect::Host::fetchSuite(suiteName, suiteVersion);
    }
}

/////////////////
/////////////////////////////////////////////////// MULTI_THREAD SUITE ///////////////////////////////////////////////////
/////////////////

struct Thread_Group {
    typedef std::list<boost::thread*> ThreadsList;
    ThreadsList threads;
    ThreadsList finishedThreads;
    boost::mutex lock;
    boost::condition_variable cond;
    
    Thread_Group()
    : threads()
    , lock()
    , cond()
    {
        
    }
    
    Thread_Group(const Thread_Group& o)
    : threads(o.threads)
    , lock()
    , cond()
    {
        
    }
    
    
    ~Thread_Group(){
        
    }
};

static Thread_Group tg = Thread_Group();



void OfxWrappedFunctor(OfxThreadFunctionV1 func,int i,unsigned int nThreads,void* customArgs){
    func(i,nThreads,customArgs);
    
    boost::mutex::scoped_lock lock(tg.lock);
    
    ///remove this thread from the thread group
    Thread_Group::ThreadsList::iterator found = tg.threads.end();
    for(Thread_Group::ThreadsList::iterator it = tg.threads.begin();it!= tg.threads.end();++it){
        if((*it)->get_id() == boost::this_thread::get_id()){
            found = it;
            break;
        }
    }
    
    //it shouldn't have been removed...
    assert(found != tg.threads.end());
    tg.threads.erase(found);
    tg.finishedThreads.push_back(*found);
    
    tg.cond.notify_one();
    
}



OfxStatus Natron::OfxHost::multiThread(OfxThreadFunctionV1 func,unsigned int nThreads, void *customArg) {
    U32 maxConcurrentThread;
    multiThreadNumCPUS(&maxConcurrentThread);
    
    for (U32 i = 0; i < nThreads; ++i) {
        
        ///if the threads count running is greater than the maxConcurrentThread,
        ///wait for a thread to finish
        boost::mutex::scoped_lock lock(tg.lock);
        
        while(tg.threads.size() > maxConcurrentThread){
            tg.cond.wait(lock);
        }
        
        boost::thread* t = new boost::thread(OfxWrappedFunctor,func,i,nThreads,customArg);
        
        ///register the thread
        tg.threads.push_back(t);
        
    }
    
    for (Thread_Group::ThreadsList::iterator it = tg.threads.begin(); it!= tg.threads.end(); ++it) {
        (*it)->join();
    }
    
    ///all functors must have returned and erase the thread from the list.
    assert(tg.threads.empty());
    
    for (Thread_Group::ThreadsList::iterator it = tg.finishedThreads.begin(); it!= tg.finishedThreads.end(); ++it) {
        delete (*it);
    }
    
    tg.finishedThreads.clear();
    
    return kOfxStatOK;
}

void Natron::OfxHost::multiThreadNumCPUS(unsigned int *nCPUs) const {
    *nCPUs = boost::thread::hardware_concurrency();
}

void Natron::OfxHost::multiThreadIndex(unsigned int *threadIndex) const {
    U32 i = 0;
    for (Thread_Group::ThreadsList::iterator it = tg.threads.begin(); it!= tg.threads.end(); ++it) {
        if((*it)->get_id() == boost::this_thread::get_id()){
            *threadIndex = i;
            return;
        }
        ++i;
    }
    *threadIndex = 0;
}

bool Natron::OfxHost::multiThreadIsSpawnedThread() const {
    for (Thread_Group::ThreadsList::iterator it = tg.threads.begin(); it!= tg.threads.end(); ++it) {
        if((*it)->get_id() == boost::this_thread::get_id()){
            return true;
        }
    }
    return false;
}

void Natron::OfxHost::mutexCreate(OfxMutexHandle *mutex, int lockCount) const{
    QMutex* m;
    if(lockCount > 1){
        m = new QMutex(QMutex::Recursive);
    }else{
        m = new QMutex;
    }
    for (int i = 0; i < lockCount; ++i) {
        m->lock();
    }
    *mutex = (OfxMutexHandle)(m);
}

void Natron::OfxHost::mutexDestroy(const OfxMutexHandle mutex) const {
    delete reinterpret_cast<const QMutex*>(mutex);
}

void Natron::OfxHost::mutexLock(const OfxMutexHandle mutex) const {
    const_cast<QMutex*>(reinterpret_cast<const QMutex*>(mutex))->lock();
}

void Natron::OfxHost::mutexUnLock(const OfxMutexHandle mutex) const {
    const_cast<QMutex*>(reinterpret_cast<const QMutex*>(mutex))->unlock();
}

OfxStatus Natron::OfxHost::mutexTryLock(const OfxMutexHandle mutex) const {
    if(const_cast<QMutex*>(reinterpret_cast<const QMutex*>(mutex))->tryLock()){
        return kOfxStatOK;
    }else{
        return kOfxStatFailed;
    }
}


