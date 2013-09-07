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
#include "Global/Macros.h"

#ifdef __POWITER_WIN32__
class PluginID{
public:
    PluginID(std::map<std::string,void*>, const std::string& name):
    _functions(functions),
    _pluginName(name),
    {
    }
    ~PluginID(){
        for(std::map<std::string,HINSTANCE>::iterator it = _functions.begin() ; it != _functions.end() ;++it){
            FreeLibrary(it->second);
        }

    }
    std::pair<bool,HINSTANCE> findFunction(const std::string& functionName) const{
        std::map<std::string,HINSTANCE>::const_iterator it = _functions.find(functionName);
        if(it == _functions.end()){
            return std::make_pair(false,(HINSTANCE)0);
        }else{
            return std::make_pair(true,it->second);
        }
    }
    
    std::map<std::string,HINSTANCE> _functions; //function name, pointer
    std::string _pluginName; // plugin name
};
#elif defined(__POWITER_UNIX__)
class PluginID{
public:
    PluginID(std::map<std::string,void*> functions, const std::string& name):
    _functions(functions),
    _pluginName(name)
    {
        
    }
    ~PluginID();
    
    std::pair<bool,void*> findFunction(const std::string& functionName) const{
        std::map<std::string,void*>::const_iterator it = _functions.find(functionName);
        if(it == _functions.end()){
            return std::make_pair(false,(void*)0);
        }else{
            return std::make_pair(true,it->second);
        }
    }
    
    std::map<std::string,void*> _functions; //function name, pointer
    std::string _pluginName; // plugin name
};
#endif

#endif
