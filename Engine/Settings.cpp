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

#include "Settings.h"

#include <QDir>

#include "Engine/Model.h"
#include "Global/LibraryBinary.h"



using namespace std;
using namespace Powiter;

Settings::CachingSettings::CachingSettings(){
    maxCacheMemoryPercent=0.5;
    maxPlayBackMemoryPercent = 0.14;
    maxDiskCache = 9000000000;
}

Settings::ViewerSettings::ViewerSettings(){
    byte_mode=1;
    stereo_mode=false;
}

Settings::GeneralSettings::GeneralSettings(){
}


Settings::ReadersSettings::ReadersSettings(){
    
}

/*changes the decoder for files identified by the filetype*/
void Settings::ReadersSettings::changeMapping(const std::string& filetype, LibraryBinary* decoder){
    _fileTypesMap.insert(make_pair(filetype, decoder));
}

/*use to initialise default mapping*/
void Settings::ReadersSettings::fillMap(std::map<std::string,LibraryBinary*>& defaultMap){
    for(std::map<std::string,LibraryBinary*>::iterator it = defaultMap.begin();it!=defaultMap.end();++it) {
        _fileTypesMap.insert(*it);
    }
}

LibraryBinary* Settings::ReadersSettings::decoderForFiletype(const std::string& type){
    for(std::map<std::string,LibraryBinary*>::iterator it = _fileTypesMap.begin();it!=_fileTypesMap.end();++it){
        QString sType(type.c_str());
        QString curType(it->first.c_str());
        sType = sType.toUpper();
        curType = curType.toUpper();
        if(sType == curType){
            return it->second;
        }
    }
    return NULL;
}

Settings::WritersSettings::WritersSettings():_maximumBufferSize(2){}

/*Returns a library if it could find an encoder for the filetype,
 otherwise returns NULL.*/
LibraryBinary* Settings::WritersSettings::encoderForFiletype(const std::string& type){
    for(std::map<std::string,LibraryBinary*>::iterator it = _fileTypesMap.begin();it!=_fileTypesMap.end();++it){
        QString sType(type.c_str());
        QString curType(it->first.c_str());
        sType = sType.toUpper();
        curType = curType.toUpper();
        if(sType == curType){
            return it->second;
        }
    }
    return NULL;
}

/*changes the encoder for files identified by the filetype*/
void Settings::WritersSettings::changeMapping(const std::string& filetype,LibraryBinary* encoder){
    _fileTypesMap.insert(make_pair(filetype, encoder));
}

/*use to initialise default mapping*/
void Settings::WritersSettings::fillMap(std::map<std::string,LibraryBinary*>& defaultMap){
    for(std::map<std::string,LibraryBinary*>::iterator it = defaultMap.begin();it!=defaultMap.end();++it) {
        _fileTypesMap.insert(*it);
    }
}

std::vector<std::string> Settings::ReadersSettings::supportedFileTypes() const {
    vector<string> out;
    for(std::map<std::string,LibraryBinary*>::const_iterator it = _fileTypesMap.begin();it!=_fileTypesMap.end();++it) {
        out.push_back(it->first);
    }
    return out;
}