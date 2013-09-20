//  Powiter
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

#include <ofxhPluginAPICache.h>
#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>

#include "Engine/OfxNode.h"
#include "Engine/OfxImageEffectInstance.h"

using namespace Powiter;

Powiter::OfxHost::OfxHost()
:_imageEffectPluginCache(*this)
{
    _properties.setStringProperty(kOfxPropName, "PowiterHost");
    _properties.setStringProperty(kOfxPropLabel, "Powiter");
    _properties.setIntProperty(kOfxImageEffectHostPropIsBackground, 0);
    _properties.setIntProperty(kOfxImageEffectPropSupportsOverlays, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultiResolution, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsTiles, 1);
    _properties.setIntProperty(kOfxImageEffectPropTemporalClipAccess, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentRGBA, 0);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentAlpha, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGenerator, 0 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextFilter, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGeneral, 2 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextTransition, 3 );

    _properties.setStringProperty(kOfxImageEffectPropSupportedPixelDepths,kOfxBitDepthFloat,0);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipDepths, 0);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipPARs, 0);
    _properties.setIntProperty(kOfxImageEffectPropSetableFrameRate, 0);
    _properties.setIntProperty(kOfxImageEffectPropSetableFielding, 0);
    _properties.setIntProperty(kOfxParamHostPropSupportsCustomInteract, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsStringAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsChoiceAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsBooleanAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsCustomAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropMaxParameters, -1);
    _properties.setIntProperty(kOfxParamHostPropMaxPages, 0);
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 0 );
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 1 );


}

Powiter::OfxHost::~OfxHost()
{
    writeOFXCache();
}

OFX::Host::ImageEffect::Instance* Powiter::OfxHost::newInstance(void* ,
                                                     OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                     OFX::Host::ImageEffect::Descriptor& desc,
                                                     const std::string& context)
{
    assert(plugin);

    
    
    return new Powiter::OfxImageEffectInstance(plugin,desc,context,false);
}

/// Override this to create a descriptor, this makes the 'root' descriptor
OFX::Host::ImageEffect::Descriptor *Powiter::OfxHost::makeDescriptor(OFX::Host::ImageEffect::ImageEffectPlugin* plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OFX::Host::ImageEffect::Descriptor(plugin);
    return desc;
}

/// used to construct a context description, rootContext is the main context
OFX::Host::ImageEffect::Descriptor *Powiter::OfxHost::makeDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                                                          OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OFX::Host::ImageEffect::Descriptor(rootContext, plugin);
    return desc;
}

/// used to construct populate the cache
OFX::Host::ImageEffect::Descriptor *Powiter::OfxHost::makeDescriptor(const std::string &bundlePath,
                                                          OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OFX::Host::ImageEffect::Descriptor(bundlePath, plugin);
    return desc;
}

/// message
OfxStatus Powiter::OfxHost::vmessage(const char* type,
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

OfxNode* Powiter::OfxHost::createOfxNode(const std::string& name,Model* model) {
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
    std::set<std::string>::iterator found = contexts.find(kOfxImageEffectContextGenerator);
    std::string context;
    if(found != contexts.end()){
        context = *found;
    }else{
        found = contexts.find(kOfxImageEffectContextFilter);
        if(found != contexts.end()){
            context = *found;
        }else{
            found = contexts.find(kOfxImageEffectContextTransition);
            if(found != contexts.end()){
                context = *found;
            }else{
                found = contexts.find(kOfxImageEffectContextPaint);
                if(found != contexts.end()){
                    context = *found;
                }else{
                    context = kOfxImageEffectContextGeneral;
                }
            }
        }
    }
    bool rval = false;
    try{
        rval = plugin->getPluginHandle();
    } catch (const std::exception &e) {
        std::cout << "Error: Could not get plugin handle for plugin \"" << name << "\":" << e.what() << std::endl;
    }
    if(!rval) {
        return NULL;
    }
    OfxNode* node = new OfxNode(model,plugin,context);
    assert(node);
    Powiter::OfxImageEffectInstance* effect = node->effectInstance();
    if (effect) {
        stat = effect->createInstanceAction();
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    } else {
        std::cout << "Error: Could not create effect instance for plugin \"" << name << "\"" << std::endl;
        delete node;
        node = NULL;
    }
    /*must be called AFTER createInstanceAction!*/
    node->tryInitializeOverlayInteracts();
    return node;
}

QStringList Powiter::OfxHost::loadOFXPlugins() {
    QStringList pluginNames;

    assert(OFX::Host::PluginCache::getPluginCache());
    /// set the version label in the global cache
    OFX::Host::PluginCache::getPluginCache()->setCacheVersion("PowiterOFXCachev1");

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
    std::ifstream ifs("PowiterOFXCache.xml");
    OFX::Host::PluginCache::getPluginCache()->readCache(ifs);
    OFX::Host::PluginCache::getPluginCache()->scanPluginFiles();
    ifs.close();

    //  _imageEffectPluginCache.dumpToStdOut();

    /*Filling node name list and plugin grouping*/
    const std::vector<OFX::Host::ImageEffect::ImageEffectPlugin *>& plugins = _imageEffectPluginCache.getPlugins();
    for (unsigned int i = 0 ; i < plugins.size(); ++i) {
        OFX::Host::ImageEffect::ImageEffectPlugin* p = plugins[i];
        assert(p);
        if(p->getContexts().size() == 0)
            continue;
        QString name = p->getDescriptor().getProps().getStringProperty(kOfxPropShortLabel).c_str();
        if(name.isEmpty()){
            name = p->getDescriptor().getProps().getStringProperty(kOfxPropLabel).c_str();
        }
        QString rawName = name;
        QString id = p->getIdentifier().c_str();
        QString grouping = p->getDescriptor().getPluginGrouping().c_str();
        
        int pluginCount = p->getBinary()->getNPlugins();
        QString bundlePath;
        bundlePath = pluginCount > 1 ? p->getBinary()->getBundlePath().c_str() : "";
        QStringList groups = ofxExtractAllPartsOfGrouping(grouping,bundlePath);
        if (groups.size() >= 1) {
            name.append("  [");
            name.append(groups[0]);
            name.append("]");
        }
        assert(p->getBinary());
        QString iconFilename = QString(p->getBinary()->getBundlePath().c_str()) + "/Contents/Resources/";
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
        emit toolButtonAdded(groups, rawName, iconFilename, groupIconFilename);
        _ofxPlugins.insert(make_pair(name.toStdString(), make_pair(id.toStdString(), grouping.toStdString())));
        pluginNames.append(name);
    }
    return pluginNames;
}



void Powiter::OfxHost::writeOFXCache(){
    /// and write a new cache, long version with everything in there
    std::ofstream of("PowiterOFXCache.xml");
    assert(OFX::Host::PluginCache::getPluginCache());
    OFX::Host::PluginCache::getPluginCache()->writePluginCache(of);
    of.close();
    //Clean up, to be polite.
    OFX::Host::PluginCache::clearPluginCache();
}

