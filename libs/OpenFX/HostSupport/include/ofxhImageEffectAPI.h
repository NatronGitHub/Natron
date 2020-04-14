
/*
Software License :

Copyright (c) 2007-2009, The Open Effects Association Ltd.  All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name The Open Effects Association Ltd, nor the names of its 
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OFXH_IMAGE_EFFECT_API_H
#define OFXH_IMAGE_EFFECT_API_H

#include <string>
#include <map>
#include <set>
#include <memory>

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxhImageEffect.h"
#include "ofxhHost.h"

namespace OFX {
  
  namespace Host {
    
    namespace ImageEffect {

      class PluginCache;

      /// subclass of Plugin representing an ImageEffect plugin.  used to store API-specific
      /// data
      class ImageEffectPlugin : public Plugin {

        PluginCache &_pc;

        // this comes off Descriptor's property set after a describe
        // context independent
        Descriptor *_baseDescriptor; /// NEEDS TO BE MADE WITH A FACTORY FUNCTION ON THE HOST!!!!!!
        
        /// map to store contexts in
        std::map<std::string, Descriptor *> _contexts;

        mutable std::set<std::string> _knownContexts;
        mutable bool _madeKnownContexts;

        auto_ptr<PluginHandle> _pluginHandle;

        void addContextInternal(const std::string &context) const;

      public:
		ImageEffectPlugin(PluginCache &pc, PluginBinary *pb, int pi, OfxPlugin *pl);

        ImageEffectPlugin(PluginCache &pc,
                          PluginBinary *pb,
                          int pi,
                          const std::string &api,
                          int apiVersion,
                          const std::string &pluginId,
                          const std::string &rawId,
                          int pluginMajorVersion,
                          int pluginMinorVersion);

        virtual ~ImageEffectPlugin();

        /// return the API handler this plugin was constructed by
        APICache::PluginAPICacheI &getApiHandler();


        /// get the base image effect descriptor
        Descriptor &getDescriptor();

        /// get the base image effect descriptor, const version
        const Descriptor &getDescriptor() const;

        /// get the image effect descriptor for the context
        Descriptor *getContext(const std::string &context);

        void addContext(const std::string &context);
        void addContext(const std::string &context, Descriptor *ied);

        virtual void saveXML(std::ostream &os);

        const std::set<std::string>& getContexts() const;

        PluginHandle *getPluginHandle();

        /// this is called to make an instance of the effect
        /// the client data ptr is what is passed back to the client creation function
        ImageEffect::Instance* createInstance(const std::string &context, void *clientDataPtr);

      private:
        void unload();
      };

      class MajorPlugin {
        std::string _id;
        int _major;

      public:
        MajorPlugin(const std::string &id, int major) : _id(id), _major(major) {
        }

        MajorPlugin(ImageEffectPlugin *iep) : _id(iep->getIdentifier()), _major(iep->getVersionMajor()) {
        }

        const std::string &getId() const {
          return _id;
        }

        int getMajor() const {
          return _major;
        }

        bool operator<(const MajorPlugin &other) const {
          if (_id < other._id)
            return true;

          if (_id > other._id)
            return false;
          
          if (_major < other._major)
            return true;

          return false;
        }
      };

      /// implementation of the specific Image Effect handler API cache.
      class PluginCache : public APICache::PluginAPICacheI {
      public:      

      private:
        /// all plugins
        std::vector<ImageEffectPlugin *> _plugins;

        /// latest version of each plugin by ID
        std::map<std::string, ImageEffectPlugin *> _pluginsByID;

        /// latest minor version of each plugin by (ID,major)
        std::map<MajorPlugin, ImageEffectPlugin *> _pluginsByIDMajor;

        /// xml parsing state
        ImageEffectPlugin *_currentPlugin;
        /// xml parsing state
        Property::Property *_currentProp;
        
        Descriptor *_currentContext;
        Param::Descriptor *_currentParam;
        ClipDescriptor *_currentClip;

        /// pointer to our image effect host
        OFX::Host::ImageEffect::Host* _host;

      public:  

        explicit PluginCache(OFX::Host::ImageEffect::Host* host);

        virtual ~PluginCache();        

        /// get the plugin by id.  vermaj and vermin can be specified.  if they are not it will
        /// pick the highest found version.
        ImageEffectPlugin *getPluginById(const std::string &id, int vermaj=-1, int vermin=-1);

        /// get the plugin by label.  vermaj and vermin can be specified.  if they are not it will
        /// pick the highest found version.
        ImageEffectPlugin *getPluginByLabel(const std::string &label, int vermaj=-1, int vermin=-1);

        OFX::Host::ImageEffect::Host *getHost() {
          return _host;
        }

        const std::vector<ImageEffectPlugin *>& getPlugins() const;

        const std::map<std::string, ImageEffectPlugin *>& getPluginsByID() const;

        const std::map<MajorPlugin, ImageEffectPlugin *>& getPluginsByIDMajor() const
        {
          return _pluginsByIDMajor;
        }

        /// handle the case where the info needs filling in from the file.  runs the "describe" action on the plugin.
        void loadFromPlugin(Plugin *p) const;

        /// hook run before deleting the ImageEffect
        void unloadPlugin(Plugin *p) const;

        /// handler for preparing to read in a chunk of XML from the cache, set up context to do this
        void beginXmlParsing(Plugin *p);

        /// XML handler : element begins (everything is stored in elements and attributes)       
        virtual void xmlElementBegin(const std::string &el, const std::map<std::string, std::string> &map);

        virtual void xmlCharacterHandler(const std::string &);
        
        virtual void xmlElementEnd(const std::string &el);
        
        virtual void endXmlParsing();
        
        virtual void saveXML(Plugin *ip, std::ostream &os) const;

        void confirmPlugin(Plugin *p, const std::list<std::string>& pluginPath);

        virtual bool pluginSupported(Plugin *p, std::string &reason) const;

        Plugin *newPlugin(PluginBinary *pb,
                          int pi,
                          OfxPlugin *pl);

        Plugin *newPlugin(PluginBinary *pb,
                          int pi,
                          const std::string &api,
                          int apiVersion,
                          const std::string &pluginId,
                          const std::string &rawId,
                          int pluginMajorVersion,
                          int pluginMinorVersion);

        void dumpToStdOut();
      };
    
    } // ImageEffect

  } // Host

} // OFX

#endif
