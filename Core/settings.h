//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef PROJECT_H
#define PROJECT_H
#include <vector>
#include <QtCore/qglobal.h>
#include "Superviser/powiterFn.h"
#include "Core/singleton.h"
#include "Core/metadata.h"
#include <map>

class PluginID;
/*The current settings of Powiter in the preferences menu. This class implements the singleton pattern,
 that means the powiter settings are unique and there cannot be 2 instances living at the same time.*/
class Settings : public Singleton<Settings>
{
public:

	static Settings* getPowiterCurrentSettings(){return Settings::instance();}

    Settings():_cacheSettings(),_viewerSettings(),_generalSettings(),_readersSettings(){}
        
    class CachingSettings{
    public:
        double maxCacheMemoryPercent ; // percentage of the total  RAM
        double maxPlayBackMemoryPercent; //percentage of maxCacheMemoryPercent
        U64 maxDiskCache ; // total size of disk space used
        U64 maxTextureCache; //total size of the texture cache
        
        CachingSettings();
    };
    class ViewerSettings{
    public:
        float byte_mode;
        bool stereo_mode;
        
        ViewerSettings();
    };
    class GeneralSettings{
    public:
        
        GeneralSettings();
    };
    class ReadersSettings{
    public:
        
        ReadersSettings();
        
        /*Returns a pluginID if it could find a decoder for the filetype,
         otherwise returns NULL.*/
        PluginID* decoderForFiletype(std::string type);
        
        /*changes the decoder for files identified by the filetype*/
        void changeMapping(std::string filetype,PluginID* decoder);
        
        /*use to initialise default mapping*/
        void fillMap(std::map<std::string,PluginID*>& defaultMap);
    private:
        
        std::map<std::string,PluginID*> _fileTypesMap;
    };
    
    class WritersSettings{
    public:
        WritersSettings();
        
        /*Returns a pluginID if it could find an encoder for the filetype,
         otherwise returns NULL.*/
        PluginID* encoderForFiletype(std::string type);
        
        /*changes the encoder for files identified by the filetype*/
        void changeMapping(std::string filetype,PluginID* encoder);
        
        /*use to initialise default mapping*/
        void fillMap(std::map<std::string,PluginID*>& defaultMap);
        
        const std::map<std::string,PluginID*>& getFileTypesMap(){return _fileTypesMap;}
        
        
        int _maximumBufferSize;
        
    private:
        
        
        std::map<std::string,PluginID*> _fileTypesMap;
        
    };
    
    CachingSettings _cacheSettings;
    ViewerSettings _viewerSettings;
    GeneralSettings _generalSettings;
    ReadersSettings _readersSettings;
    WritersSettings _writersSettings;
};

#endif // PROJECT_H
