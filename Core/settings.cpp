//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "Core/settings.h"

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