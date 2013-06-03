//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "Core/settings.h"
#include "Core/model.h"

using namespace std;
Settings::Caching::Caching(){
    maxCacheMemoryPercent=0.5;
    maxPlayBackMemoryPercent = 0.14;
    maxDiskCache = 9000000000;
    maxTextureCache = 256000000;
}

Settings::Viewer::Viewer(){
    byte_mode=1;
    stereo_mode=false;
}

Settings::General::General(){
    
}


Settings::Readers::Readers(){
    
}

/*changes the decoder for files identified by the filetype*/
void Settings::Readers::changeMapping(std::string filetype,PluginID* decoder){
    _fileTypesMap.insert(make_pair(filetype, decoder));
}

/*use to initialise default mapping*/
void Settings::Readers::fillMap(std::map<std::string,PluginID*>& defaultMap){
    for(std::map<std::string,PluginID*>::iterator it = defaultMap.begin();it!=defaultMap.end();it++){
        _fileTypesMap.insert(*it);
    }
}

PluginID* Settings::Readers::decoderForFiletype(std::string type){
    std::map<std::string,PluginID*>::iterator found = _fileTypesMap.find(type);
    if (found!=_fileTypesMap.end()) {
        return found->second;
    }
    return NULL;
}