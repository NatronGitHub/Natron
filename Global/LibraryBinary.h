//
//  PluginID.h
//  Powiter
//
//  Created by Frédéric Devernay on 01/09/13.
//
//

#ifndef POWITER_ENGINE_PLUGINID_H_
#define POWITER_ENGINE_PLUGINID_H_

#include <string>
#include <map>
#include <vector>
#include "Global/Macros.h"

namespace Powiter{
    
#ifdef __POWITER_UNIX__
#include <dlfcn.h>
#endif

#ifdef __POWITER_OSX__
    #define LIBRARY_EXT "dylib"
#elif defined(__POWITER_LINUX__)
    #define LIBRARY_EXT "so"
#elif defined(__POWITER_WIN32__)
    #define LIBRARY_EXT "dll"
#else
    #error "Operating system not supported by the library loader."
#endif
    
class LibraryBinary {
    
    
public:
    
#ifdef __POWITER_WIN32__
    typedef HINSTANCE value_type;
#elif defined(__POWITER_UNIX__)
    typedef void* value_type;
#endif
    
    LibraryBinary(const std::string& binaryPath,
                 const std::vector<std::string>& funcNames):
    _library(0),
    _valid(false)
    {
#ifdef __POWITER_WIN32__
        _library = LoadLibrary(binaryPath.c_str());
#elif defined(__POWITER_UNIX__)
        _library = dlopen(binaryPath.c_str(),RTLD_LAZY);
#endif
        if(!_library){
            std::cout << "Couldn't open library " << binaryPath  << ". You must delete this object to avoid memory leaks." << std::endl;
            return;
        }        
               
        _valid = true;
    }
    
    void loadFunctions(const std::vector<std::string>& funcNames){
        for (U32 i = 0; i < funcNames.size(); ++i) {
#ifdef __POWITER_WIN32__
            value_type v = (value_type)GetProcAddress(_library,funcNames[i].c_str())
#elif defined(__POWITER_UNIX__)
            value_type v = (value_type)dlsym(_library,funcNames[i].c_str());
#endif
            if(!v){
                std::cout << "Couldn't find function " << funcNames[i] << " in binary " << binaryPath <<
                "You must delete this object to avoid memory leaks." << std::endl;
                _valid = false;
                return;
            }
            _functions.insert(std::make_pair(funcNames[i],v));
        }

    }
    
    
    ~LibraryBinary() {
#ifdef __POWITER_WIN32__
            FreeLibarary(_library);
#elif defined(__POWITER_UNIX__)
            dlclose(_library);
#endif
    }
    
    /*Returns a pointer to the function with name functionName. The return value
     is a pair whose first member indicates whether it could find the function or not.s*/
    template <typename T>
    std::pair<bool,T> findFunction(const std::string& functionName) const{
        std::map<std::string,value_type>::const_iterator it = _functions.find(functionName);
        if(it == _functions.end()){
            return std::make_pair(false,(T)0);
        }else{
            return std::make_pair(true,(T)it->second);
        }
    }
    
    /*Call this after the constructor to find out if the binary has loaded successfully.
     If not, you should delete this object.*/
    bool isValid() const {return _valid;}
    
    
private:
    
    
#ifdef __POWITER_WIN32__
    HINSTANCE _library;
#elif defined(__POWITER_UNIX__)
    void* _library;
#endif
    bool _valid;
    std::map<std::string,value_type> _functions; // <function name, pointer>
};
    
} // namespace Powiter
#endif
