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

#include "Global/Macros.h"

#ifdef __POWITER_WIN32__
class PluginID{
public:
    PluginID(HINSTANCE first, const std::string& second){
        this->first=first;
        this->second=second;
    }
    ~PluginID(){

        FreeLibrary(first);

    }
    HINSTANCE first;
    std::string second;
};
#elif defined(__POWITER_UNIX__)
class PluginID{
public:
    PluginID(void* first, const std::string& second){
        this->first=first;
        this->second=second;
    }
    ~PluginID();
    void* first;
    std::string second;
};
#endif

#endif
