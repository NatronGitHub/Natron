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

#ifndef POWITER_ENGINE_SETTINGS_H_
#define POWITER_ENGINE_SETTINGS_H_

#include <string>
#include <map>
#include <vector>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/Singleton.h"

/*The current settings of Powiter in the preferences menu. This class implements the singleton pattern,
 that means the powiter settings are unique and there cannot be 2 instances living at the same time.
 
 
 
 @todo Move this class to QSettings instead*/


namespace Powiter {
    class LibraryBinary;
}

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
        Powiter::LibraryBinary* decoderForFiletype(const std::string& type);
        
        /*changes the decoder for files identified by the filetype*/
        void changeMapping(const std::string& filetype,Powiter::LibraryBinary* decoder);
        
        /*use to initialise default mapping*/
        void fillMap(std::map<std::string,Powiter::LibraryBinary*>& defaultMap);
        
        std::vector<std::string> supportedFileTypes() const;
    private:
        
        std::map<std::string,Powiter::LibraryBinary* > _fileTypesMap;
    };
    
    class WritersSettings{
    public:
        WritersSettings();
        
        /*Returns a pluginID if it could find an encoder for the filetype,
         otherwise returns NULL.*/
        Powiter::LibraryBinary* encoderForFiletype(const std::string& type);
        
        /*changes the encoder for files identified by the filetype*/
        void changeMapping(const std::string& filetype, Powiter::LibraryBinary* encoder);
        
        /*use to initialise default mapping*/
        void fillMap(std::map<std::string,Powiter::LibraryBinary*>& defaultMap);
        
        const std::map<std::string,Powiter::LibraryBinary*>& getFileTypesMap(){return _fileTypesMap;}
        
        
        int _maximumBufferSize;
        
    private:
        
        
        std::map<std::string,Powiter::LibraryBinary*> _fileTypesMap;
        
    };
    
    CachingSettings _cacheSettings;
    ViewerSettings _viewerSettings;
    GeneralSettings _generalSettings;
    ReadersSettings _readersSettings;
    WritersSettings _writersSettings;
};

#endif // POWITER_ENGINE_SETTINGS_H_
