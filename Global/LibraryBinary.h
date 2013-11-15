//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GLOBAL_LIBRARYBINARY_H_
#define NATRON_GLOBAL_LIBRARYBINARY_H_

#include <string>
#include <map>
#include <vector>
#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
namespace Natron{

#ifdef __NATRON_OSX__
    #define NATRON_LIBRARY_EXT "dylib"
#elif defined(__NATRON_LINUX__)
    #define NATRON_LIBRARY_EXT "so"
#elif defined(__NATRON_WIN32__)
    #define NATRON_LIBRARY_EXT "dll"
#else
    #error "Operating system not supported by the library loader."
#endif
    
class LibraryBinary {
    
#ifdef __NATRON_WIN32__
    typedef HINSTANCE value_type;
#elif defined(__NATRON_UNIX__)
    typedef void* value_type;
#endif
    
public:
    
    enum LibraryType{EXTERNAL=0,BUILTIN=1};
    
    LibraryBinary(LibraryBinary::LibraryType type);
    
    LibraryBinary(const std::map<std::string,void*>& functions);
    
    LibraryBinary(const std::string& binaryPath);
    
    LibraryBinary(const std::string& binaryPath,
                  const std::vector<std::string>& funcNames);
    
    ~LibraryBinary();
    
    bool loadBinary(const std::string& binaryPath);
    
    bool loadFunctions(const std::vector<std::string>& funcNames);
        
    /*Call this after the constructor to find out if the binary has loaded successfully.
     If not, you should delete this object.*/
    bool isValid() const {return _valid;}
    
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
    
    const std::map<std::string,value_type>& getAllFunctions() const {return _functions;}
    
    
private:
    
    LibraryType _type;
#ifdef __NATRON_WIN32__
    HINSTANCE _library;
#elif defined(__NATRON_UNIX__)
    void* _library;
#endif
    std::string _binaryPath;
    bool _valid;
    std::map<std::string,value_type> _functions; // <function name, pointer>
};
    
} // namespace Natron
#endif
