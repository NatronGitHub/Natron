//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef PROJECT_H
#define PROJECT_H
#include <vector>
#include <QtCore/qglobal.h>
#include "Core/metadata.h"

/*The current settings of Powiter in the preferences menu.*/
class Settings
{
public:
    Settings(){
        maxCacheMemoryPercent=0.5;
        maxPlayBackMemoryPercent = 0.14;
        maxDiskCache = 9000000000;
        byte_mode=1;
        stereo_mode=false;
    }
    
    double maxCacheMemoryPercent ; // percentage of the total  RAM
    double maxPlayBackMemoryPercent; //percentage of maxCacheMemoryPercent
    qint64 maxDiskCache ; // 9GB for now
    // viewer related
    float byte_mode;
    bool stereo_mode;
};

#endif // PROJECT_H
