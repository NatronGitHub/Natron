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

#include <ofxhPluginAPICache.h>
#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>

#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"

using namespace Natron;

Natron::OfxHost::OfxHost()
:_imageEffectPluginCache(*this)
{
    _properties.setStringProperty(kOfxPropName, NATRON_APPLICATION_NAME "Host");
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
    _properties.setIntProperty(kOfxParamHostPropSupportsStringAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsChoiceAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsBooleanAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsCustomAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropMaxParameters, -1);
    _properties.setIntProperty(kOfxParamHostPropMaxPages, 0);
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 0 );
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 1 );
    _properties.setIntProperty(kOfxImageEffectInstancePropSequentialRender, 0);
    
}

Natron::OfxHost::~OfxHost()
{
    writeOFXCache();
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
    bool isQuestion = false;
    const char *prefix = "Message : ";
    if (strcmp(type, kOfxMessageLog) == 0) {
        prefix = "Log : ";
    }
    else if(strcmp(type, kOfxMessageFatal) == 0 ||
            strcmp(type, kOfxMessageError) == 0) {
        prefix = "Error : ";
    }
    else if(strcmp(type, kOfxMessageQuestion) == 0)  {
        prefix = "Question : ";
        isQuestion = true;
    }
    
    // Just dump our message to stdout, should be done with a proper
    // UI in a full ap, and post a dialogue for yes/no questions.
    fputs(prefix, stdout);
    vprintf(format, args);
    printf("\n");
    
    if(isQuestion) {
        /// cant do this properly inour example, as we need to raise a dialogue to ask a question, so just return yes
        return kOfxStatReplyYes;
    }
    else {
        return kOfxStatOK;
    }
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
    OfxStatus stat;
    OFXPluginsIterator ofxPlugin = _ofxPlugins.find(name);
    if (ofxPlugin == _ofxPlugins.end()) {
        return NULL;
    }
    OFX::Host::ImageEffect::ImageEffectPlugin* plugin = _imageEffectPluginCache.getPluginById(ofxPlugin->second.first);
    if (!plugin) {
        return NULL;
    }
    const std::set<std::string>& contexts = plugin->getContexts();
    std::string context;
    
    if (contexts.size() == 1) {
        context = (*contexts.begin());
    }else{
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
    
    
    bool rval = false;
    try{
        rval = plugin->getPluginHandle();
    } catch (const std::exception &e) {
        throw std::runtime_error("Error: Could not get plugin handle for plugin");
        return NULL;
    }
    if(!rval) {
        return NULL;
    }
    OfxEffectInstance* hostSideEffect = new OfxEffectInstance(node);
    
    try{
        hostSideEffect->createOfxImageEffectInstance(plugin, context);
    }catch(const std::exception& e){
        throw e;
        delete hostSideEffect;
        return NULL;
    }
    Natron::OfxImageEffectInstance* effect = hostSideEffect->effectInstance();
    if (effect) {
        stat = effect->createInstanceAction();
        if(stat != kOfxStatOK && stat != kOfxStatReplyDefault){
            throw std::runtime_error("Could not create effect instance for plugin");
            delete hostSideEffect;
            return NULL;
        }
    } else {
        throw std::runtime_error("Could not create effect instance for plugin");
        delete hostSideEffect;
        return NULL;
    }
    
    /*must be called AFTER createInstanceAction!*/
    hostSideEffect->tryInitializeOverlayInteracts();
    return hostSideEffect;
}

std::map<QString,QMutex*> Natron::OfxHost::loadOFXPlugins() {
    std::map<QString,QMutex*> pluginNames;
    
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
    
    /*Filling node name list and plugin grouping*/
    const std::vector<OFX::Host::ImageEffect::ImageEffectPlugin *>& plugins = _imageEffectPluginCache.getPlugins();
    for (unsigned int i = 0 ; i < plugins.size(); ++i) {
        OFX::Host::ImageEffect::ImageEffectPlugin* p = plugins[i];
        assert(p);
        if(p->getContexts().size() == 0)
            continue;
       
        
        
        QString id = p->getIdentifier().c_str();
        std::string grouping = p->getDescriptor().getPluginGrouping().c_str();
        
        std::string bundlePath = p->getBinary()->getBundlePath();
        int pluginsCount = p->getBinary()->getNPlugins();
        std::string name = OfxEffectInstance::generateImageEffectClassName(p->getDescriptor().getShortLabel(),
                                                                           p->getDescriptor().getLabel(),
                                                                           p->getDescriptor().getLongLabel(),
                                                                           pluginsCount,
                                                                           bundlePath,
                                                                           grouping);
        std::string rawName = name;

        QStringList groups = OfxEffectInstance::getPluginGrouping(bundlePath, pluginsCount, grouping);
        
        assert(p->getBinary());
        QString iconFilename = QString(bundlePath.c_str()) + "/Contents/Resources/";
        iconFilename.append(p->getDescriptor().getProps().getStringProperty(kOfxPropIcon,1).c_str());
        iconFilename.append(id);
        iconFilename.append(".png");
        QString groupIconFilename;
        if (groups.size() >= 1) {
            groupIconFilename = QString(p->getBinary()->getBundlePath().c_str()) + "/Contents/Resources/";
            groupIconFilename.append(p->getDescriptor().getProps().getStringProperty(kOfxPropIcon,1).c_str());
            groupIconFilename.append(groups[0]);
            groupIconFilename.append(".png");
        }
        emit toolButtonAdded(groups, rawName.c_str(), iconFilename, groupIconFilename);
        _ofxPlugins.insert(make_pair(name, make_pair(id.toStdString(), grouping)));
        QMutex* pluginMutex = NULL;
        if(p->getDescriptor().getRenderThreadSafety() == kOfxImageEffectRenderUnsafe){
            pluginMutex = new QMutex;
        }
        pluginNames.insert(std::make_pair(name.c_str(),pluginMutex));
    }
    return pluginNames;
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
    //Clean up, to be polite.
    OFX::Host::PluginCache::clearPluginCache();
}

