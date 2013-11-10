//  Natron
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

/*The current settings in the preferences menu.
 @todo Move this class to QSettings instead*/


namespace Natron {
    class LibraryBinary;
}

class Settings
{
public:

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
        Natron::LibraryBinary* decoderForFiletype(const std::string& type) const;
        
        /*changes the decoder for files identified by the filetype*/
        void changeMapping(const std::string& filetype,Natron::LibraryBinary* decoder);
        
        /*use to initialise default mapping*/
        void fillMap(const std::map<std::string,Natron::LibraryBinary*>& defaultMap);
        
        std::vector<std::string> supportedFileTypes() const;
    private:
        
        std::map<std::string,Natron::LibraryBinary* > _fileTypesMap;
    };
    
    class WritersSettings{
    public:
        WritersSettings();
        
        /*Returns a pluginID if it could find an encoder for the filetype,
         otherwise returns NULL.*/
        Natron::LibraryBinary* encoderForFiletype(const std::string& type) const;
        
        /*changes the encoder for files identified by the filetype*/
        void changeMapping(const std::string& filetype, Natron::LibraryBinary* encoder);
        
        /*use to initialise default mapping*/
        void fillMap(const std::map<std::string,Natron::LibraryBinary*>& defaultMap);
        
        const std::map<std::string,Natron::LibraryBinary*>& getFileTypesMap() const {return _fileTypesMap;}
        
        
        int _maximumBufferSize;
        
    private:
        
        
        std::map<std::string,Natron::LibraryBinary*> _fileTypesMap;
        
    };
    
    CachingSettings _cacheSettings;
    ViewerSettings _viewerSettings;
    GeneralSettings _generalSettings;
    ReadersSettings _readersSettings;
    WritersSettings _writersSettings;
};

#endif // POWITER_ENGINE_SETTINGS_H_
