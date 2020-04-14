/*
  Software License :
  
  Copyright (c) 2007-2009, The Open Effects Association Ltd. All rights reserved.
  
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

#include <assert.h>

#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream> // stringstream
#include <stdexcept>

#include <string.h>
#include <stdlib.h>

#include "expat.h"

// ofx
#include "ofxCore.h"
#include "ofxImageEffect.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhMemory.h"
#include "ofxhPluginAPICache.h"
#include "ofxhPluginCache.h"
#include "ofxhHost.h"
#include "ofxhXml.h"
#include "ofxhUtilities.h"

#if defined (__linux__) || defined (__FreeBSD__)

#define DIRLIST_SEP_CHARS ":;"
#define DIRSEP "/"
#include <dirent.h>

static const char *getArchStr() 
{
  if(sizeof(void *) == 4) {
#if defined(__linux__)
    return  "Linux-x86";
#else
    return  "FreeBSD-x86";
#endif
  }
  else {
#if defined(__linux__)
    return  "Linux-x86-64";
#else
    return  "FreeBSD-x86-64";
#endif
  }
}

#define ARCHSTR getArchStr()

#elif defined (__APPLE__)

#define DIRLIST_SEP_CHARS ";:"
#if defined(__x86_64) || defined(__x86_64__)
#define ARCHSTR "MacOS-x86-64"
#else
#define ARCHSTR "MacOS"
#endif
#define DIRSEP "/"
#include <dirent.h>

#elif defined (WINDOWS)
#define DIRLIST_SEP_CHARS ";"
#ifdef _WIN64
#define ARCHSTR "win64"
#else
#define ARCHSTR "win32"
#endif
#define DIRSEP "\\"

#include "shlobj.h"
#endif


bool OFX::Host::PluginCache::_useStdOFXPluginsLocation = true;
OFX::Host::PluginCache* OFX::Host::PluginCache::gPluginCachePtr = 0;

// Define this to enable ofx plugin cache debug messages.
//#define CACHE_DEBUG

using namespace OFX::Host;


/// try to open the plugin bundle object and query it for plugins
void PluginBinary::loadPluginInfo(PluginCache *cache) {      
  if (isInvalid()) {
    return;
  }

  if (_binary) {
    _fileModificationTime = _binary->getTime();
    _fileSize = _binary->getSize();
    
    // Take a reference to load the binary only once per session. It will
    // eventually be unloaded in the destructor (see below).
    // This avoid lots of useless calls to dlopen()/dlclose().
    if (!_binary->isLoaded()) {
      _binary->ref();
    }
  }

  
  if (_binary) {
    _getNumberOfPlugins = (OfxGetNumberOfPluginsFunc) _binary->findSymbol("OfxGetNumberOfPlugins");
    _getPluginFunc = (OfxGetPluginFunc) _binary->findSymbol("OfxGetPlugin");
  }
  
  
  if (_getNumberOfPlugins == 0 || _getPluginFunc == 0) {
    if (_binary) {
      _binary->setInvalid(true);
    }
    
  } else {
    int pluginCount = (*_getNumberOfPlugins)();
    _plugins.clear();
    _plugins.reserve(pluginCount);
    
    for (int i=0;i<pluginCount;i++) {
      OfxPlugin *plug = (*_getPluginFunc)(i);
      
      APICache::PluginAPICacheI *api = cache->findApiHandler(plug->pluginApi, plug->apiVersion);
      assert(api);
      
      _plugins.push_back(api->newPlugin(this, i, plug));
    }
  }
}

PluginBinary::~PluginBinary() {
  std::vector<Plugin*>::iterator i = _plugins.begin();
  while (i != _plugins.end()) {
    const APICache::PluginAPICacheI &api = (*i)->getApiHandler();
    api.unloadPlugin(*i);
    delete *i;
    i++;
  }
  // release the last reference to the binary, which should unload it
  // if this reference was taken by loadPluginInfo().
  if (_binary) {
    if (_binary->isLoaded()) {
      _binary->unref();
    }
    assert(!_binary->isLoaded());
  }
}

PluginHandle::PluginHandle(Plugin *p, OFX::Host::Host *host)
{
  _b = p->getBinary();
  if (_b->_binary) {
    _b->_binary->ref();
  }
  _op = 0;
  OfxPlugin* (*getPlug)(int) = 0;
  if (_b->_getPluginFunc) {
    getPlug = _b->_getPluginFunc;
  }
  if (!getPlug && _b->_binary) {
    getPlug = (OfxPlugin*(*)(int)) _b->_binary->findSymbol("OfxGetPlugin");
  }
  if (getPlug) {
    _op = getPlug(p->getIndex());
    if (_op) {         
      _op->setHost(host->getHandle());
    }
  }
}

PluginHandle::~PluginHandle() {
  if (_b->_binary) {
    _b->_binary->unref();
  }
}


#if defined (WINDOWS)
const std::wstring&
PluginCache::getStdOFXPluginPath(const std::string &hostId)
{
  static std::wstring ret;
  static int gotIt = 0;
  if(!gotIt) {
	gotIt = 1;
    wchar_t buffer[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES_COMMON, NULL, SHGFP_TYPE_CURRENT, buffer);
	std::wstring str;
	str.append(L"\\OFX\\");
	str.append(OFX::utf8_to_utf16(hostId));
	wcscat_s(buffer, MAX_PATH, str.c_str());
	ret = std::wstring(buffer);
  }
  return ret;	   
}
#endif

static
std::string OFXGetEnv(const char* e)
{
#if defined(WINDOWS) && !defined(__MINGW32__)
  size_t requiredSize;
  getenv_s(&requiredSize, 0, 0, e);
  std::vector<char> buffer(requiredSize);
  if(requiredSize >0)
    {
      getenv_s(&requiredSize, &buffer.front(), requiredSize, e);
      return &buffer.front();
    }
  return "";
#else
  if(getenv(e))
    return getenv(e);
#endif
  return "";
}

PluginCache* PluginCache::getPluginCache()
{
  if(!gPluginCachePtr)
    gPluginCachePtr = new PluginCache();
  return gPluginCachePtr;
}

void PluginCache::clearPluginCache()
{
  delete gPluginCachePtr;
  gPluginCachePtr = 0;
}

PluginCache::~PluginCache()
{
  for(std::list<PluginBinary *>::iterator it=_binaries.begin(); it != _binaries.end(); ++it) {
    delete (*it);
  }
  _binaries.clear();
}

PluginCache::PluginCache()
: _hostSpec(NULL)
#ifdef OFX_USE_STATIC_PLUGINS
, _staticBinary(NULL)
#endif
, _xmlCurrentBinary(NULL)
, _xmlCurrentPlugin(NULL)
{
  _cacheVersion = "";
  _ignoreCache = false;
  _dirty = false;
  _enablePluginSeek = true;
  
  std::string s = OFXGetEnv("OFX_PLUGIN_PATH");
  
    
  while (s.length()) {
      
    int spos = int(s.find_first_of(DIRLIST_SEP_CHARS));
    
    std::string path;
    
    if (spos != -1) {
      path = s.substr(0, spos);
      s = s.substr(spos+1);
    } 
    else {
      path = s;
      s = "";
    }
    
    _pluginPath.push_back(path);
  }

  if (!_useStdOFXPluginsLocation)
    return;

#if defined(WINDOWS)

  std::wstring wpath = getStdOFXPluginPath();
  std::string path = OFX::utf16_to_utf8(wpath);

  _pluginPath.push_back(path);
  _pluginPath.push_back("C:\\Program Files\\Common Files\\OFX\\Plugins");
#endif
#if defined(__linux__) || defined(__FreeBSD__)
  _pluginPath.push_back("/usr/OFX/Plugins");
#endif
#if defined(__APPLE__)
  _pluginPath.push_back("/Library/OFX/Plugins");
#endif
}

void PluginCache::setPluginHostPath(const std::string &hostId) {
#if defined(WINDOWS)
  std::wstring wpath = getStdOFXPluginPath(hostId);
  std::string path = OFX::utf16_to_utf8(wpath);

  _pluginPath.push_back(path);
  _pluginPath.push_back("C:\\Program Files\\Common Files\\OFX\\" + hostId);
#endif
#if defined(__linux__) || defined(__FreeBSD__)
  _pluginPath.push_back("/usr/OFX/" + hostId);
#endif
#if defined(__APPLE__)
  _pluginPath.push_back("/Library/OFX/" + hostId);
#endif
}

void PluginCache::scanDirectory(std::set<std::string> &foundBinFiles, const std::string &dir, bool recurse)
{
#ifdef CACHE_DEBUG
  printf("looking in %s for plugins\n", dir.c_str());
#endif
  
#if defined (WINDOWS)
  WIN32_FIND_DATAW findData;
  HANDLE findHandle;
#else
  DIR *d = opendir(dir.c_str());
  if (!d) {
    return;
  }
#endif
  
  _pluginDirs.push_back(dir.c_str());
  
#if defined (UNIX)
  while (dirent *de = readdir(d))
#elif defined (WINDOWS)
    {

      std::wstring ws = OFX::utf8_to_utf16((dir + "\\*"));
      findHandle = FindFirstFileW(ws.c_str(), &findData);
    }
  if (findHandle == INVALID_HANDLE_VALUE) 
    {
      return;
    }

  while (1)
#endif
    {
#if defined (UNIX)
      std::string name = de->d_name;
      bool isdir = true;
#else
      std::wstring wname = findData.cFileName;
      std::string name = OFX::utf16_to_utf8(wname);

      bool isdir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
      if (name.find(".ofx.bundle") != std::string::npos) {
        std::string barename = name.substr(0, name.length() - strlen(".bundle"));
        std::string bundlename = dir + DIRSEP + name;
        std::string binpath = dir + DIRSEP + name + DIRSEP "Contents" DIRSEP + ARCHSTR + DIRSEP + barename;
          
        // don't insert binpath yet, do it later because of Mac OS X Universal stuff
        //foundBinFiles.insert(binpath);

#if defined(__APPLE__) && (defined(__x86_64) || defined(__x86_64__))
        /* From the OpenFX specification:
           
           MacOS-x86-64 - for Apple Macintosh OS X, specifically on
           intel x86 CPUs running AMD's 64 bit extensions. 64 bit host
           applications should check this first, and if it doesn't
           exist or is empty, fall back to "MacOS" looking for a
           universal binary.
        */
          
        std::string binpath_universal = dir + DIRSEP + name + DIRSEP "Contents" DIRSEP + "MacOS" + DIRSEP + barename;
        if (_knownBinFiles.find(binpath_universal) != _knownBinFiles.end()) {
          binpath = binpath_universal;
        }
#endif

        if (_knownBinFiles.find(binpath) == _knownBinFiles.end()) {
#ifdef CACHE_DEBUG
          printf("found non-cached binary %s\n", binpath.c_str());
#endif
          _dirty = true;

          // the binary was not in the cache
          
          PluginBinary *pb = 0;
#if defined(__x86_64) || defined(__x86_64__)
          pb = new PluginBinary(binpath, bundlename, this);
#  if defined(__APPLE__)
          if (pb->isInvalid()) {
            // fallback to "MacOS"
            delete pb;
            binpath = binpath_universal;
            pb = new PluginBinary(binpath, bundlename, this);
          }
#  endif
#else
          pb = new PluginBinary(binpath, bundlename, this);
#endif
          _binaries.push_back(pb);
          _knownBinFiles.insert(binpath);
          foundBinFiles.insert(binpath);

          for (int j=0;j<pb->getNPlugins();j++) {
            Plugin *plug = &pb->getPlugin(j);
            const APICache::PluginAPICacheI &api = plug->getApiHandler();
            api.loadFromPlugin(plug);
          }
        } else {
#ifdef CACHE_DEBUG
          printf("found cached binary %s\n", binpath.c_str());
#endif
        }
        // insert final path (universal or not) in the list of found files
        foundBinFiles.insert(binpath);
      } else {
        if (isdir && (recurse && !name.empty() && name[0] != '@' && name[name.size() - 1] != '.')) {
          scanDirectory(foundBinFiles, dir + DIRSEP + name, recurse);
        }
      }
#if defined(WINDOWS)
      int rval = FindNextFileW(findHandle, &findData);
      
      if (rval == 0) {
        break;
      }
#endif
    }
  
#if defined(UNIX)
  closedir(d);
#else
  FindClose(findHandle);
#endif
}

std::string PluginCache::seekPluginFile(const std::string &baseName) const {
  // Exit early if disabled
  if (!_enablePluginSeek)
    return "";
  
  for (std::list<std::string>::const_iterator paths= _pluginDirs.begin();
       paths != _pluginDirs.end();
       paths++) {
    std::string candidate = *paths + DIRSEP + baseName;
    FILE *f = fopen(candidate.c_str(), "r");
    if (f) {
      fclose(f);
      return candidate;
    }
  }
  return "";
}

void PluginCache::scanPluginFiles()
{
  std::set<std::string> foundBinFiles;
  
  for (std::list<std::string>::iterator paths= _pluginPath.begin();
       paths != _pluginPath.end();
       paths++) {
    scanDirectory(foundBinFiles, *paths, _nonrecursePath.find(*paths) == _nonrecursePath.end());
  }

#ifdef OFX_USE_STATIC_PLUGINS
  // Now add all static plug-ins

  // If already loaded from the cache, the static binary must point to the same host binary file
  assert(!_staticBinary || _staticBinary->getFilePath() == _hostAppBinFilePath);
  if (!_staticBinary) {
    _staticBinary = new PluginBinary(_hostAppBinFilePath, &OfxGetNumberOfPlugins,&OfxGetPlugin,this, 0, 0);
    if (_staticBinary->isInvalid()) {
      std::cerr << "WARNING: ignoring statically linked plug-ins because host application binary file path (" << _hostAppBinFilePath << ") is wrongly set" << std::endl;
      delete _staticBinary;
      _staticBinary = 0;
    } else {
      _dirty = true;
      for (int j=0;j<_staticBinary->getNPlugins();j++) {
         Plugin *plug = &_staticBinary->getPlugin(j);
         const APICache::PluginAPICacheI &api = plug->getApiHandler();
         api.loadFromPlugin(plug);
      }
      _binaries.push_back(_staticBinary);
    }
  }
#endif
  
  std::list<PluginBinary *>::iterator i=_binaries.begin();
  while (i!=_binaries.end()) {
    PluginBinary *pb = *i;
    
    if (!pb->isStaticallyLinkedPlugin() && foundBinFiles.find(pb->getFilePath()) == foundBinFiles.end()) {
      
      // the binary was in the cache, but was not on the path
      
      _dirty = true;
      i = _binaries.erase(i);
      delete pb;
      
    } else {
      
      bool binChanged = pb->hasBinaryChanged();
      
      // the binary was in the cache, but the binary has changed and thus we need to reload
      if (binChanged) {
        // The static PluginBinary already had loadPluginInfo called anyway at this point
        if (!pb->isStaticallyLinkedPlugin()) {
          pb->loadPluginInfo(this);
        }
        _dirty = true;
      }
      
      for (int j=0;j<pb->getNPlugins();j++) {
        Plugin *plug = &pb->getPlugin(j);
        APICache::PluginAPICacheI &api = plug->getApiHandler();
        
        if (binChanged) {
          api.loadFromPlugin(plug);
        }
        
        std::string reason;
        
        if (api.pluginSupported(plug, reason)) {
          _plugins.push_back(plug);
          api.confirmPlugin(plug, _pluginPath);
        } else {
          std::cerr << "ignoring plugin " << plug->getIdentifier() <<
            " as unsupported (" << reason << ")" << std::endl;
        }
      }
      
      i++;
    }
  }
}


/// callback for XML parser
static void elementBeginHandler(void *userData, const XML_Char *name, const XML_Char **atts) {
  PluginCache::getPluginCache()->elementBeginCallback(userData, name, atts);
}

/// callback for XML parser
static void elementCharHandler(void *userData, const XML_Char *data, int len) {
  PluginCache::getPluginCache()->elementCharCallback(userData, data, len);
}

/// callback for XML parser
static void elementEndHandler(void *userData, const XML_Char *name) {
  PluginCache::getPluginCache()->elementEndCallback(userData, name);
}

static bool mapHasAll(const std::map<std::string, std::string> &attmap, const char **atts) {
  while (*atts) {
    if (attmap.find(*atts) == attmap.end()) {
      return false;
    }
    atts++;
  }
  return true;
}

void PluginCache::elementBeginCallback(void */*userData*/, const XML_Char *name, const XML_Char **atts) {
  if (_ignoreCache) {
    return;
  }
  
  std::string ename = name;
  std::map<std::string, std::string> attmap;
  
  while (*atts) {
    attmap[atts[0]] = atts[1];
    atts += 2;
  }
  
  /// XXX: validate in general
  
  if (ename == "cache") {
    std::string cacheversion = attmap["version"];
    if (cacheversion != _cacheVersion) {
#ifdef CACHE_DEBUG
      printf("mismatched version, ignoring cache (got '%s', wanted '%s')\n",
             cacheversion.c_str(),
             _cacheVersion.c_str());
#endif
      _ignoreCache = true;
    }
  }
  
  if (ename == "binary") {
    const char *binAtts[] = {"static_bin", "path", "bundle_path", "mtime", "size", NULL};
    
    if (!mapHasAll(attmap, binAtts)) {
      // no path: bad XML
    }

#ifdef OFX_USE_STATIC_PLUGINS
    bool isStaticLinkedCachedBinary = OFX::Host::Property::stringToInt(attmap["static_bin"]);
#endif

    std::string fname = attmap["path"];
    std::string bname = attmap["bundle_path"];
    time_t mtime = OFX::Host::Property::stringToInt(attmap["mtime"]);
    off_t size = OFX::Host::Property::stringToInt(attmap["size"]);

    PluginBinary* pb;
#ifdef OFX_USE_STATIC_PLUGINS
    if (isStaticLinkedCachedBinary) {
      // only 1 static binary allowed!
      if (_staticBinary) {
        _ignoreCache = true;
        return;
      }
      // We need to provide the 2 function pointers for the static binary
      pb = new PluginBinary(_hostAppBinFilePath, &OfxGetNumberOfPlugins,&OfxGetPlugin, this, &fname, &mtime, &size);
      _staticBinary = pb;
    } else
#endif
    {
      pb = new PluginBinary(fname, bname, mtime, size);
    }
    _xmlCurrentBinary = pb;
    _binaries.push_back(_xmlCurrentBinary);
    _knownBinFiles.insert(fname);
    
    return;
  }
  
  if (ename == "plugin" && _xmlCurrentBinary && !_xmlCurrentBinary->hasBinaryChanged()) {
    const char *plugAtts[] = {"api", "name", "index", "api_version", "major_version", "minor_version", NULL};
    
    if (!mapHasAll(attmap, plugAtts)) {
      // no path: bad XML
    }
    
    std::string api = attmap["api"];
    std::string rawIdentifier = attmap["name"];
    
    std::string identifier = rawIdentifier;
    
    // Who says the pluginIdentifier is case-insensitive? OFX 1.3 spec doesn't mention this.
    // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id472588
    //for (size_t i=0;i<identifier.size();i++) {
    //  identifier[i] = tolower(identifier[i]);
    //}
    
    int idx = OFX::Host::Property::stringToInt(attmap["index"]);
    int api_version = OFX::Host::Property::stringToInt(attmap["api_version"]);
    int major_version = OFX::Host::Property::stringToInt(attmap["major_version"]);
    int minor_version = OFX::Host::Property::stringToInt(attmap["minor_version"]);
    
    APICache::PluginAPICacheI *apiCache = findApiHandler(api, api_version);
    if (apiCache) {
      
      Plugin *pe = apiCache->newPlugin(_xmlCurrentBinary, idx, api, api_version, identifier, rawIdentifier, major_version, minor_version);
      _xmlCurrentBinary->addPlugin(pe);
      _xmlCurrentPlugin = pe;
      apiCache->beginXmlParsing(pe);
    }
    
    return;
  }
  
  if (_xmlCurrentPlugin) {
    APICache::PluginAPICacheI &api = _xmlCurrentPlugin->getApiHandler();
    api.xmlElementBegin(name, attmap);
  }
  
}

void PluginCache::elementCharCallback(void */*userData*/, const XML_Char *data, int size)
{
  if (_ignoreCache) {
    return;
  }
  
  std::string s(data, size);
  if (_xmlCurrentPlugin) {
    APICache::PluginAPICacheI &api = _xmlCurrentPlugin->getApiHandler();
    api.xmlCharacterHandler(s);
  } else {
    /// XXX: we only want whitespace
  }
}

void PluginCache::elementEndCallback(void */*userData*/, const XML_Char *name) {
  if (_ignoreCache) {
    return;
  }
  
  std::string ename = name;
  
  /// XXX: validation?
  
  if (ename == "plugin") {
    if (_xmlCurrentPlugin) {
      APICache::PluginAPICacheI &api = _xmlCurrentPlugin->getApiHandler();
      api.endXmlParsing();
    }
    _xmlCurrentPlugin = 0;
    return;
  }
  
  if (ename == "bundle") {
    _xmlCurrentBinary = 0;
    return;
  }
  
  if (_xmlCurrentPlugin) {
    APICache::PluginAPICacheI &api = _xmlCurrentPlugin->getApiHandler();
    api.xmlElementEnd(name);
  }
}

void PluginCache::readCache(std::istream &ifs) {
  XML_Parser xP = XML_ParserCreate(NULL);
  if (!xP) {
    throw std::runtime_error("Error creating XML parser");

  }
  XML_SetElementHandler(xP, elementBeginHandler, elementEndHandler);
  XML_SetCharacterDataHandler(xP, elementCharHandler);
  int done = false;
  char buf[4096];
  while (!done && !ifs.eof()) {
    if (!ifs.good()) {
      throw std::runtime_error("Error reading cache stream");
    }
    ifs.read(buf, sizeof(buf));
    int n = (int)ifs.gcount();
    done = (n != sizeof(buf));

    if (XML_Parse(xP, buf, n, done) == XML_STATUS_ERROR) {
      XML_Error errorCode = XML_GetErrorCode(xP);
      XML_Size errorLine = XML_GetCurrentLineNumber(xP);
      XML_Size errorCol = XML_GetCurrentColumnNumber(xP);
      const XML_LChar *errorString = XML_ErrorString(errorCode);
      std::stringstream errorDesc;
      errorDesc << "XML parsing error at line " << errorLine << ":" << errorCol;
      errorDesc << ": " << errorString;
      XML_ParserFree(xP);
      throw std::runtime_error(errorDesc.str());
    }
  }
  XML_ParserFree(xP);
}

void PluginCache::writePluginCache(std::ostream &os) const {
#ifdef CACHE_DEBUG
  printf("writing pluginCache with version = %s\n", _cacheVersion.c_str());
#endif
  
  os << "<cache version=\"" << _cacheVersion << "\">\n";
  for (std::list<PluginBinary *>::const_iterator i=_binaries.begin();i!=_binaries.end();i++) {
    PluginBinary *b = *i;
    
    std::vector<Plugin*> plugins(b->getNPlugins());
    for (std::size_t j = 0; j < plugins.size(); ++j) {
      plugins[j] = &b->getPlugin((int)j);
    }
    
    os << "<bundle>\n";
    os << "  <binary "
       << XML::attribute("static_bin", int(b->isStaticallyLinkedPlugin()))
       << XML::attribute("bundle_path", b->getBundlePath())
       << XML::attribute("path", b->getFilePath())
       << XML::attribute("mtime", int(b->getFileModificationTime()))
       << XML::attribute("size", int(b->getFileSize())) << "/>\n";
    
    for (int j=0;j<(int)plugins.size();j++) {
      Plugin *p = plugins[j];
      
      
      os << "  <plugin " 
         << XML::attribute("name", p->getRawIdentifier()) 
         << XML::attribute("index", p->getIndex()) 
         << XML::attribute("api", p->getPluginApi())
         << XML::attribute("api_version", p->getApiVersion())
         << XML::attribute("major_version", p->getVersionMajor())
         << XML::attribute("minor_version", p->getVersionMinor())
         << ">\n";
      																      
      const APICache::PluginAPICacheI &api = p->getApiHandler();
      os << "    <apiproperties>\n"; 
      api.saveXML(p, os);
      os << "    </apiproperties>\n";											    
				
      os << "  </plugin>\n";
    }
    os << "</bundle>\n";
  }
  os << "</cache>\n";
}


APICache::PluginAPICacheI *PluginCache::findApiHandler(const std::string &api, int version) {
  std::list<PluginCacheSupportedApi>::iterator i = _apiHandlers.begin();
  while (i != _apiHandlers.end()) {
    if (i->matches(api, version)) {
      return i->handler;
    }
    i++;
  }
  return 0;
}
