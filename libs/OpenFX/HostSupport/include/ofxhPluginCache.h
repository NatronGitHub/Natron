#ifndef OFX_PLUGIN_CACHE_H
#define OFX_PLUGIN_CACHE_H

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

#include <string>
#include <vector>
#include <list>
#include <set>
#include <iostream>

#include <stdio.h>

#include "expat.h"

#include "ofxCore.h"
#include "ofxhPropertySuite.h"
#include "ofxhPluginAPICache.h"
#include "ofxhBinary.h"

#ifdef WINDOWS
#include "tchar.h"
#endif

namespace OFX {

  namespace Host {

    typedef int(*OfxGetNumberOfPluginsFunc)(void);
    typedef OfxPlugin*(*OfxGetPluginFunc)(int);
    
    class Host;

    // forward delcarations
    class PluginDesc;   
    class Plugin;
    class PluginBinary;
    class PluginCache;

    /// C++ version of the information kept inside an OfxPlugin struct
    class PluginDesc  {
    protected :
      std::string _pluginApi;  ///< the API I implement
      int _apiVersion;         ///< the version of the API

      std::string _identifier; ///< the identifier of the plugin
      std::string _rawIdentifier; ///< the original identifier of the plugin
      int _versionMajor;       ///< the plugin major version
      int _versionMinor;       ///< the plugin minor version

    public:

      const std::string &getPluginApi() const {
        return _pluginApi;
      }
      
      int getApiVersion() const {
        return _apiVersion;
      }
      
      const std::string &getIdentifier() const {
        return _identifier;
      }

      const std::string &getRawIdentifier() const {
        return _rawIdentifier;
      }
      
      int getVersionMajor() const {
        return _versionMajor;
      }
      
      int getVersionMinor() const  {
        return _versionMinor;
      }

      PluginDesc() : _apiVersion(-1) {
      }

      virtual ~PluginDesc() {}

      PluginDesc(const std::string &api,
                 int apiVersion,
                 const std::string &identifier,
                 const std::string &rawIdentifier,
                 int versionMajor,
                 int versionMinor)
        : _pluginApi(api)
        , _apiVersion(apiVersion)
        , _identifier(identifier)
        , _rawIdentifier(rawIdentifier)
        , _versionMajor(versionMajor)
        , _versionMinor(versionMinor)
      {
      }


      /// constructor for the case where we have already loaded the plugin binary and 
      /// are populating this object from it
      PluginDesc(OfxPlugin *ofxPlugin) {
        _pluginApi = ofxPlugin->pluginApi;
        _apiVersion = ofxPlugin->apiVersion;
        _rawIdentifier = ofxPlugin->pluginIdentifier;
        _identifier = ofxPlugin->pluginIdentifier;

        // Who says the pluginIdentifier is case-insensitive? OFX 1.3 spec doesn't mention this.
        // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id472588
        //for (size_t i=0;i<_identifier.size();i++) {
        //  _identifier[i] = tolower(_identifier[i]);
        //}
        _versionMajor = ofxPlugin->pluginVersionMajor;
        _versionMinor = ofxPlugin->pluginVersionMinor;
      }

    };
    
    /// class that we use to manipulate a plugin
    class Plugin : public PluginDesc {
    /// owned by the PluginBinary it lives inside
    /// Plugins can only be pass about either by pointer or reference
    private :
      Plugin(const Plugin&) : PluginDesc() {}                          ///< hidden
      Plugin &operator= (const Plugin&) {return *this;} ///< hidden

    protected :
      PluginBinary *_binary; ///< the file I live inside
      int           _index;  ///< where I live inside that file
    public :
      Plugin();

      PluginBinary *getBinary()
      {
        return _binary;
      }

      const PluginBinary *getBinary() const
      {
        return _binary;
      }

      int getIndex() const
      {
        return _index;
      }

      /// construct this based on the struct returned by the getNthPlugin() in the binary
      Plugin(PluginBinary *bin, int idx, OfxPlugin *o) : PluginDesc(o), _binary(bin), _index(idx)
      {
      }
      
      /// construct me from the cache
      Plugin(PluginBinary *bin, int idx, const std::string &api,
             int apiVersion, const std::string &identifier, 
             const std::string &rawIdentifier,
             int majorVersion, int minorVersion)
        : PluginDesc(api, apiVersion, identifier, rawIdentifier, majorVersion, minorVersion)
        , _binary(bin)
        , _index(idx) 
      {
      }

      virtual ~Plugin() {
      }

      virtual APICache::PluginAPICacheI &getApiHandler() = 0;

      bool trumps(Plugin *other) {
        int myMajor = getVersionMajor();
        int theirMajor = other->getVersionMajor();

        int myMinor = getVersionMinor();
        int theirMinor = other->getVersionMinor();

        if (myMajor > theirMajor) {
          return true;
        }
        
        if (myMajor == theirMajor && myMinor > theirMinor) {
          return true;
        }

        return false;
      }

      bool equals(Plugin *other) {
        int myMajor = getVersionMajor();
        int theirMajor = other->getVersionMajor();

        int myMinor = getVersionMinor();
        int theirMinor = other->getVersionMinor();

        if (myMajor == theirMajor && myMinor == theirMinor) {
          return true;
        }

        return false;
      }
    };

    class PluginHandle;

    /// class that represents a binary file which holds plugins
    class PluginBinary {
    /// has a set of plugins inside it and which it owns
    /// These are owned by a PluginCache
      friend class PluginHandle;

    protected :
      Binary* _binary;                 ///< our binary object, abstracted layer ontop of OS calls, defined in ofxhBinary.h, may not be valid if function pointers below are already set
      OfxGetNumberOfPluginsFunc _getNumberOfPlugins;
      OfxGetPluginFunc _getPluginFunc;
      std::string _filePath;          ///< full path to the file
      std::string _bundlePath;        ///< path to the .bundle directory
      std::vector<Plugin *> _plugins; ///< my plugins
      time_t _fileModificationTime;   ///< used as a time stamp to check modification times, used for caching
      off_t _fileSize;                ///< file size last time we check, used for caching
      bool _binaryChanged;            ///< whether the timestamp/filesize in this cache is different from that in the actual binary
      bool _binaryInvalid;            ///< whether we could open and stat the binary or not

    public :

      /// create one from the cache.  this will invoke the Binary() constructor which
      /// will stat() the file.
      explicit PluginBinary(const std::string &file, const std::string &bundlePath, time_t mtime, off_t size)
        : _binary(new Binary(file))
        , _getNumberOfPlugins(0)
        , _getPluginFunc(0)
        , _filePath(file)
        , _bundlePath(bundlePath)
        , _fileModificationTime(mtime)
        , _fileSize(size)
        , _binaryChanged(false)
        , _binaryInvalid(false)
      {
        if (isInvalid()) {
          _binaryInvalid = true;
          return;
        }
        if (_fileModificationTime != _binary->getTime() || _fileSize != _binary->getSize()) {
          _binaryChanged = true;
        }
      }


      /// constructor which will open a library file, call things inside it, and then 
      /// create Plugin objects as appropriate for the plugins exported therefrom
      explicit PluginBinary(const std::string &file, const std::string &bundlePath, PluginCache *cache)
        : _binary(new Binary(file))
        , _getNumberOfPlugins(0)
        , _getPluginFunc(0)
        , _filePath(file)
        , _bundlePath(bundlePath)
        , _binaryChanged(false)
        , _binaryInvalid(false)
      {
        if (_binary->isInvalid()) {
          _binaryInvalid = true;
        }
        loadPluginInfo(cache);
      }
      
       /// create the plug-in object from function pointers directly, this is useful for statically linked plug-ins.
      explicit PluginBinary(const std::string& hostAppBinFilePath,
                            OfxGetNumberOfPluginsFunc getNumberOfPlugins,
                            OfxGetPluginFunc getPlugins,
                            PluginCache *cache,
                            std::string* cachedHostAppBinFilePath = 0,
                            time_t* cachedFileModificationTime = 0,
                            off_t* cachedFileSize = 0)
      : _binary(0)
      , _getNumberOfPlugins(getNumberOfPlugins)
      , _getPluginFunc(getPlugins)
      , _filePath(hostAppBinFilePath)
      , _bundlePath(hostAppBinFilePath)
      , _plugins()
      , _fileModificationTime()
      , _fileSize()
      , _binaryChanged(true)
      , _binaryInvalid(false)
      {
        _binaryInvalid = !Binary::getFileModTimeAndSize(hostAppBinFilePath, _fileModificationTime, _fileSize);
        if (cachedHostAppBinFilePath && cachedFileModificationTime && cachedFileSize) {
          if (_filePath == *cachedHostAppBinFilePath && _fileModificationTime == *cachedFileModificationTime && _fileSize == *cachedFileSize) {
            _binaryChanged = false;
          }
        }
        if (_binaryChanged) {
          loadPluginInfo(cache);
        }
      }

    
      /// dtor
      virtual ~PluginBinary();

      
      bool isStaticallyLinkedPlugin() const
      {
        return _binary == 0;
      }

      time_t getFileModificationTime() const {
      	return _fileModificationTime;
      }
    
      off_t getFileSize() {
      	return _fileSize;
      }

      const std::string &getFilePath() const {
        return _filePath;
      }
      
      const std::string &getBundlePath() const {
        return _bundlePath;
      }
      
      bool hasBinaryChanged() const {
        return _binaryChanged;
      }

      bool isLoaded() const {
        return _binary ? _binary->isLoaded() : true;
      }
        
      bool isInvalid() const {
        return _binaryInvalid;
      }

      void addPlugin(Plugin *pe) {
        _plugins.push_back(pe);
      }

      void loadPluginInfo(PluginCache *);

      /// how many plugins?
      int getNPlugins() const {return (int)_plugins.size(); }

      /// get a plugin 
      Plugin &getPlugin(int idx) {return *_plugins[idx];}

      /// get a plugin 
      const Plugin &getPlugin(int idx) const {return *_plugins[idx];}
    };

    /// wrapper class for Plugin/PluginBinary.  use in a RAIA fashion to make sure the binary gets unloaded when needed and not before.
    class PluginHandle {
      PluginBinary *_b;
      OfxPlugin *_op;

    public:
      PluginHandle(Plugin *p, OFX::Host::Host *_host);
      virtual ~PluginHandle();

      OfxPlugin *getOfxPlugin() {
        return _op;
      }

      OfxPlugin *operator->() {
        return _op;
      }
    };
    
    /// for later 
    struct PluginCacheSupportedApi {
      std::string api;
      int minVersion;
      int maxVersion;
      APICache::PluginAPICacheI *handler;

      PluginCacheSupportedApi(const std::string &_api, int _minVersion, int _maxVersion, APICache::PluginAPICacheI *_handler) :
        api(_api), minVersion(_minVersion), maxVersion(_maxVersion), handler(_handler)
      {
      }
      
      bool matches(const std::string &_api, int _version) const
      {
        if (_api == api && _version >= minVersion && _version <= maxVersion) {
          return true;
        }
        return false;
      }
    };

    /// Where we keep our plugins.    
    class PluginCache {
    protected :
      OFX::Host::Property::PropSpec* _hostSpec;

      std::list<std::string>    _pluginPath;  ///< list of directories to look in
      std::set<std::string>     _nonrecursePath; ///< list of directories to look in (non-recursively)
      std::list<std::string>    _pluginDirs;  ///< list of directories we found
      std::list<PluginBinary *> _binaries; ///< all the binaries we know about, we own these
#ifdef OFX_USE_STATIC_PLUGINS
      PluginBinary* _staticBinary; /// < the statically linked binary
#endif
      std::list<Plugin *>       _plugins;  ///< all the plugins inside the binaries, we don't own these, populated from _binaries
      std::set<std::string>     _knownBinFiles;
#ifdef OFX_USE_STATIC_PLUGINS
      std::string               _hostAppBinFilePath; ///< file path of the host application binary, used to timestamp statically linked plug-ins
#endif

      PluginBinary *_xmlCurrentBinary;
      Plugin *_xmlCurrentPlugin;

      std::list<PluginCacheSupportedApi> _apiHandlers;

      void scanDirectory(std::set<std::string> &foundBinFiles, const std::string &dir, bool recurse);

      bool _ignoreCache;
      std::string _cacheVersion;

      bool _dirty;
      bool _enablePluginSeek;       ///< Turn off to make all seekPluginFile() calls return an empty string

      static bool _useStdOFXPluginsLocation;
      static PluginCache* gPluginCachePtr; ///< singleton plugin cache

    public:
      /// ctor, which inits _pluginPath to default locations and not much else
      PluginCache();

      /// dtor
      ~PluginCache();

      /// call this before the first call to getPluginCache() in order to ignore the standard OFX plugin path
      static void useStdOFXPluginsLocation(bool val) { _useStdOFXPluginsLocation = val; }

      /// get our plugin cache
      static PluginCache* getPluginCache();

      /// clear our plugin cache
      static void clearPluginCache();
        
#if defined (WINDOWS)
      /// This is windows only since it may vary on the location of CSIDL_PROGRAM_FILES_COMMON.
      static const std::wstring& getStdOFXPluginPath(const std::string &hostId = "Plugins");
#endif
        
      /// get the list in which plugins are sought
      const std::list<std::string> &getPluginPath() {
        return _pluginPath;
      }

      /// was the cache outdated?
      bool dirty() const {
        return _dirty;
      }

      /// add a file to the plugin path
      void addFileToPath(const std::string &f, bool recurse=true) {
        _pluginPath.push_back(f);
        if (!recurse) {
          _nonrecursePath.insert(f);
        }
      }

      /// prepend a file to the plugin path
      void prependFileToPath(const std::string &f, bool recurse=true) {
        _pluginPath.push_front(f);
        if (!recurse) {
          _nonrecursePath.insert(f);
        }
      }
#ifdef OFX_USE_STATIC_PLUGINS
      void setHostApplicationBinPath(const std::string& f) {
        _hostAppBinFilePath = f;
      }
#endif

      /// specify which subdirectory of /usr/OFX or equivilant
      /// (as well as 'Plugins') to look in for plugins.
      void setPluginHostPath(const std::string &hostId);

      /// set the version string to write to the cache, 
      /// and also that we expect on cachess read in
      void setCacheVersion(const std::string &cacheVersion) {
        _cacheVersion = cacheVersion;
      }

      // populate the cache.  must call scanPluginFiles() after to check for changes.
      void readCache(std::istream &is);

      // seek a particular file on the OFX plugin path
      std::string seekPluginFile(const std::string &baseName) const;
      
      /// Sets behaviour of seekPluginFile().
      /// Enable (the default): normal operation; disable: returns an empty string instead
      void setPluginSeekEnabled(bool enabled) { _enablePluginSeek = enabled; }

      /// scan for plugins
      void scanPluginFiles();

      // write the plugin cache output file to the given stream
      void writePluginCache(std::ostream &os) const;
      
      // callback function for the XML
      void elementBeginCallback(void *userData, const XML_Char *name, const XML_Char **attrs);
      void elementCharCallback(void *userData, const XML_Char *data, int len);
      void elementEndCallback(void *userData, const XML_Char *name);

      /// register an API cache handler
      void registerAPICache(const std::string &api, int min, int max, APICache::PluginAPICacheI *apiCache) {
        _apiHandlers.push_back(PluginCacheSupportedApi(api, min, max, apiCache));
      }
      
      /// find the API cache handler for the given api/apiverson
      APICache::PluginAPICacheI* findApiHandler(const std::string &api, int apiver);

      /// obtain a list of plugins to walk through
      const std::list<Plugin *> &getPlugins() const {
        return _plugins;
      }
    };

  }
}

#endif
