//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef PROJECT_H
#define PROJECT_H
#include <vector>
#include <QtCore/qglobal.h>
#include "Superviser/powiterFn.h"
#include "Core/Singleton.h"
#include <map>
#include <vector>
class PluginID;
/*The current settings of Powiter in the preferences menu. This class implements the singleton pattern,
 that means the powiter settings are unique and there cannot be 2 instances living at the same time.
 
 
 
 @todo Move this class to QSettings instead*/

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
        PluginID* decoderForFiletype(const std::string& type);
        
        /*changes the decoder for files identified by the filetype*/
        void changeMapping(const std::string& filetype,PluginID* decoder);
        
        /*use to initialise default mapping*/
        void fillMap(std::map<std::string,PluginID*>& defaultMap);
        
        std::vector<std::string> supportedFileTypes() const;
    private:
        
        std::map<std::string,PluginID*> _fileTypesMap;
    };
    
    class WritersSettings{
    public:
        WritersSettings();
        
        /*Returns a pluginID if it could find an encoder for the filetype,
         otherwise returns NULL.*/
        PluginID* encoderForFiletype(const std::string& type);
        
        /*changes the encoder for files identified by the filetype*/
        void changeMapping(const std::string& filetype, PluginID* encoder);
        
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
