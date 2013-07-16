//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com


#ifndef PowiterOsX_exrCommons_h
#define PowiterOsX_exrCommons_h

#include "Superviser/powiterFn.h"
#include "Core/channels.h"
#include <iostream>

#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfVecAttribute.h>
#include <ImfBoxAttribute.h>
#include <ImfStringVectorAttribute.h>
#include <ImfTimeCodeAttribute.h>
#include <ImfStandardAttributes.h>
#include <ImfMatrixAttribute.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>

namespace EXR {
    

    
    static std::string const compressionNames[6]={
        "No compression",
        "Zip (1 scanline)",
        "Zip (16 scanlines)",
        "PIZ Wavelet (32 scanlines)",
        "RLE",
        "B44"
    };
    
    inline Imf::Compression stringToCompression(std::string& str){
        if(str == compressionNames[0]){
            return Imf::NO_COMPRESSION;
        }else if(str == compressionNames[1]){
            return Imf::ZIPS_COMPRESSION;
        }else if(str == compressionNames[2]){
            return Imf::ZIP_COMPRESSION;
        }else if(str == compressionNames[3]){
            return Imf::PIZ_COMPRESSION;
        }else if(str == compressionNames[4]){
            return Imf::RLE_COMPRESSION;
        }else{
            return Imf::B44_COMPRESSION;
        }
    }
    
    static  std::string const depthNames[2] = {
        "16 bit half", "32 bit float"
    };
    
    inline int depthNameToInt(std::string& name){
        if(name == depthNames[0]){
            return 16;
        }else{
            return 32;
        }
    }
    
    inline Channel fromExrChannel(std::string from)
    {
        if (from == "R")     return Powiter_Enums::Channel_red;
        if (from == "G")     return Powiter_Enums::Channel_green;
        if (from == "B")     return Powiter_Enums::Channel_blue;
        if (from == "A")     return Powiter_Enums::Channel_alpha;
        if (from == "Z")     return Powiter_Enums::Channel_DeepFront;
        if (from == "ZBack") return Powiter_Enums::Channel_DeepBack;
        try{
            Channel ret = getChannelByName(from.c_str());
            return ret;
        }catch(const char* str){
            throw str; // forward exception
            return Powiter_Enums::Channel_black;
        }
    }
    
    inline std::string toExrChannel(Channel from){
        if(from == Powiter_Enums::Channel_red) return "R";
        if(from == Powiter_Enums::Channel_green) return "G";
        if(from == Powiter_Enums::Channel_blue) return "B";
        if(from == Powiter_Enums::Channel_alpha) return "A";
        if(from == Powiter_Enums::Channel_DeepFront) return "Z";
        if(from == Powiter_Enums::Channel_DeepBack) return "ZBack";
        std::string ret = getChannelName(from);
        if(ret.empty()){
            throw "Couldn't find an appropriate OpenEXR channel for this channel"; // forward exception
            return "";
        }else{
            return ret;
        }
        
    }
    
    inline bool timeCodeFromString(const std::string& str, Imf::TimeCode& attr)
    {
        if (str.length() != 11)
            return false;
        
        int hours = 0, mins = 0, secs = 0, frames = 0;
        
        sscanf(str.c_str(), "%02d:%02d:%02d:%02d", &hours, &mins, &secs, &frames);
        try {
            attr.setHours(hours);
            attr.setMinutes(mins);
            attr.setSeconds(secs);
            attr.setFrame(frames);
        }
        catch (const std::exception& exc) {
            std::cout << "EXR: Time Code Metadata warning" <<  exc.what() << std::endl;
            return false;
        }
        return true;
    }
    inline bool edgeCodeFromString(const std::string& str, Imf::KeyCode& a)
    {
        int mfcCode, filmType, prefix, count, perfOffset;
        sscanf(str.c_str(), "%d %d %d %d %d", &mfcCode, &filmType, &prefix, &count, &perfOffset);
        
        try {
            a.setFilmMfcCode(mfcCode);
            a.setFilmType(filmType);
            a.setPrefix(prefix);
            a.setCount(count);
            a.setPerfOffset(perfOffset);
        }
        
        catch (const std::exception& exc) {
            std::cout << "EXR: Edge Code Metadata warning " << exc.what() << std::endl;
            return false;
        }
        
        return true;
    }
    
}

#endif
