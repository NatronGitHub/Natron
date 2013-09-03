//
//  OfxHost.h
//  Powiter
//
//  Created by Frédéric Devernay on 03/09/13.
//
//

#ifndef POWITER_ENGINE_OFXHOST_H_
#define POWITER_ENGINE_OFXHOST_H_

#include <QtCore/QStringList>
#include <ofxhPluginCache.h>
#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>

#include "Global/Macros.h"

class OfxNode;

namespace Powiter {

class OfxHost : public OFX::Host::ImageEffect::Host {
public:
    OfxHost();

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
                                                          OFX::Host::ImageEffect::Descriptor& desc,
                                                          const std::string& context) OVERRIDE;

    /// Override this to create a descriptor, this makes the 'root' descriptor
    virtual OFX::Host::ImageEffect::Descriptor *makeDescriptor(OFX::Host::ImageEffect::ImageEffectPlugin* plugin) OVERRIDE;

    /// used to construct a context description, rootContext is the main context
    virtual OFX::Host::ImageEffect::Descriptor *makeDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                                                               OFX::Host::ImageEffect::ImageEffectPlugin *plug) OVERRIDE;

    /// used to construct populate the cache
    virtual OFX::Host::ImageEffect::Descriptor *makeDescriptor(const std::string &bundlePath,
                                                               OFX::Host::ImageEffect::ImageEffectPlugin *plug) OVERRIDE;

    /// vmessage
    virtual OfxStatus vmessage(const char* type,
                               const char* id,
                               const char* format,
                               va_list args) OVERRIDE;

    OfxNode* createOfxNode(const std::string& name);

    /*Reads OFX plugin cache and scan plugins directories
     to load them all.*/
    QStringList loadOFXPlugins();

private:
    OFX::Host::ImageEffect::PluginCache _imageEffectPluginCache;


    /*plugin name -> pair< plugin id , plugin grouping >
     The name of the plugin is followed by the first part of the grouping in brackets
     to help identify two distinct plugins with the same name. e.g :
     1)Invert [OFX]  with plugin id net.sourceforge.openfx.invert and grouping OFX/
     2)Invert [Toto] with plugin id com.toto.invert and grouping Toto/SuperPlugins/OFX/
     */
    typedef std::map<std::string,std::pair<std::string,std::string> > OFXPluginsMap;
    typedef OFXPluginsMap::const_iterator OFXPluginsIterator;

    OFXPluginsMap _ofxPlugins;
    
};

} // namespace Powiter

#endif
