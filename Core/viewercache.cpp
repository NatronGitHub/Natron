//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include <QtCore/QDir>
#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>
#include <QtGui/QVector2D>
#include <cassert>
#include "Core/viewercache.h"
#include "Superviser/controler.h"
#include "Core/model.h"
#include "Core/settings.h"
#include "Gui/GLViewer.h"
#include "Core/mappedfile.h"
#include "Gui/mainGui.h"
#include "Gui/timeline.h"
#include <sstream>
#include "Reader/Reader.h"
#include "Core/hash.h"
#include <QtCore/QFile>
#include "Core/row.h"
#include "Core/displayFormat.h"

using namespace std;

#define gl_viewer currentViewer->viewer

ViewerCache::ViewerCache() : AbstractDiskCache(0){}

ViewerCache::~ViewerCache(){}


FrameEntry::FrameEntry():_exposure(0),_lut(0),_zoom(0),_treeVers(0),_byteMode(0),_actualH(0),_actualW(0){
    _frameInfo = new ReaderInfo;
}

FrameEntry::FrameEntry(float zoom,float exp,float lut,U64 treeVers,
                       float byteMode,ReaderInfo* info,int actualW,int actualH):
_zoom(zoom), _exposure(exp),_lut(lut),_treeVers(treeVers),
_byteMode(byteMode),_actualW(actualW),_actualH(actualH){
    _frameInfo = new ReaderInfo;
    *_frameInfo = *info;
}

FrameEntry::FrameEntry(const ViewerCache::FrameEntry& other):_zoom(other._zoom),_exposure(other._exposure),_lut(other._lut),
_treeVers(other._treeVers),_byteMode(other._byteMode),_actualW(other._actualW),_actualH(other._actualH)
{
    _frameInfo = new ReaderInfo;
    *_frameInfo = *other._frameInfo;
}

FrameEntry::~FrameEntry(){
    delete _frameInfo;
}

std::string FrameEntry::printOut(){
    ostringstream oss;
    oss << getMappedFile()->path() << " " <<_zoom << " "
    << _exposure << " "
    << _lut << " "
    << _treeVers << " "
    << _byteMode << " "
    << _frameInfo->printOut() << " "
    << _actualW << " "
    << _actualH << " " << endl;
    return oss.str();
}

/*Recover an entry from string*/
std::pair<U64,MemoryMappedEntry*> FrameEntry::recoverEntryFromString(QString str){
    ViewerCache::FrameEntry* entry = new FrameEntry;
    QString path;
    QString zoomStr,expStr,lutStr,treeStr,byteStr,frameInfoStr,actualWStr,actualHStr;
    int i =0 ;
    while(str.at(i)!= QChar(' ')){path.append(str.at(i));i++}
    i++;
    while(str.at(i)!= QChar(' ')){zoomStr.append(str.at(i));i++}
    i++;
    while(str.at(i)!= QChar(' ')){expStr.append(str.at(i));i++}
    i++;
    while(str.at(i)!= QChar(' ')){lutStr.append(str.at(i));i++}
    i++;
    while(str.at(i)!= QChar(' ')){treeStr.append(str.at(i));i++}
    i++;
    while(str.at(i)!= QChar(' ')){byteStr.append(str.at(i));i++}
    i++;
    while(str.at(i)!= QChar(' ')){frameInfoStr.append(str.at(i));i++}
    i++;
    while(str.at(i)!= QChar(' ')){actualWStr.append(str.at(i));i++}
    i++;
    while(str.at(i)!= QChar('\n')){actualHStr.append(str.at(i));i++}
    entry->_zoom = zoomStr.toFloat();
    entry->_exposure = expStr.toFloat();
    entry->_lut = lutStr.toFloat();
    entry->_byteMode = byteStr.toFloat();
    entry->_treeVers = treeStr.toULongLong();
    entry->_frameInfo = ReaderInfo::fromString(frameInfoStr);
    entry->_actualW = actualWStr.toInt();
    entry->_actualH = actualHStr.toInt();
    
    QString hashKey;
    int j = path.size() - 1;
    while(path.at(j) != QChar('.')) j--;
    j--;
    while(path.at(j) != QChar('/')){hashKey.append(path.at(j));j--;}
    U64 key = hashKey.toULongLong();
    return make_pair(key,entry);
}


ViewerCache* ViewerCache::getViewerCache(){
    return ViewerCache::instance();
}

/*Construct a frame entry,adds it to the cache and returns a pointer to it.*/
FrameEntry* add(U64 key,
                std::string filename,
                U64 treeVersion,
                float zoomFactor,
                float exposure,
                float lut ,
                float byteMode,
                int w,
                int h,
                ReaderInfo* info){
    
    FrameEntry* out = new FrameEntry(zoomFactor,exposure,lut,
                                     treeVersion,byteMode,info,w,h);
    string name(getCachePath().toStdString());
    {
        QMutexLocker guard(&_mutex);
        ostringstream oss1;
        oss1 << hex << (key >> 56);
        name.append(oss1.str());
        ostringstream oss2;
        oss2 << hex << ((key << 8) >> 8);
        QDir subfolder(name.c_str());
        if(!subfolder.exists()){
            cout << "Something is wrong in cache... couldn't find : " << name << endl;
        }else{
            name.append("/");
            name.append(oss2.str());
            name.append(".powc");
        }
    }
    U64 dataSize = 0;
    if(byteMode==1.f){
        dataSize = w*h*4;
    }else{
        dataSize = w*h*4*sizeof(float);
    }
    out->allocate(dataSize,name.c_str());
    AbstractDiskCache::add(key, out);
    
}

std::pair<U64,FrameEntry*> ViewerCache::get(std::string filename,
                                            U64 treeVersion,
                                            float zoomFactor,
                                            float exposure,
                                            float lut ,
                                            float byteMode,
                                            int w,
                                            int h,
                                            ReaderInfo* info){
    
    
    
}






