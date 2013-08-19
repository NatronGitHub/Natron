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








#include <QtCore/QDir>
#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>
#include <QtGui/QVector2D>
#include <cassert>
#include "Engine/ViewerCache.h"
#include "Global/Controler.h"
#include "Engine/Model.h"
#include "Engine/Settings.h"
#include "Gui/ViewerGL.h"
#include "Engine/MemoryFile.h"
#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/Timeline.h"
#include <sstream>
#include "Readers/Reader.h"
#include "Engine/Hash.h"
#include <QtCore/QFile>
#include "Engine/Row.h"
#include "Engine/Format.h"
#include "Engine/ViewerNode.h"
using namespace std;
using namespace Powiter;
#define gl_viewer currentViewer->viewer

ViewerCache::ViewerCache() : AbstractDiskCache(0){}

ViewerCache::~ViewerCache(){}


FrameEntry::FrameEntry():MemoryMappedEntry(), _exposure(0),_lut(0),_zoom(0),_treeVers(0),_byteMode(0){
    _frameInfo = new ReaderInfo;
}

FrameEntry::FrameEntry(float zoom,float exp,float lut,U64 treeVers,
                       float byteMode,const Box2D& bbox,const Format& dispW,
                       const ChannelSet& channels,const TextureRect& textureRect):MemoryMappedEntry(),
_exposure(exp),_lut(lut),_zoom(zoom),_treeVers(treeVers),
_byteMode(byteMode),_textureRect(textureRect){
    _frameInfo = new ReaderInfo;
    _frameInfo->set(bbox);
    _frameInfo->setDisplayWindow(dispW);
    _frameInfo->setChannels(channels);
    
}

FrameEntry::FrameEntry(float zoom,float exp,float lut,U64 treeVers,
                       float byteMode,ReaderInfo* info,const TextureRect& textureRect):MemoryMappedEntry(),
_exposure(exp),_lut(lut),_zoom(zoom),_treeVers(treeVers),
_byteMode(byteMode),_textureRect(textureRect)
{
    _frameInfo = info;
}

FrameEntry::FrameEntry(const FrameEntry& other):_exposure(other._exposure),_lut(other._lut),_zoom(other._zoom),
_treeVers(other._treeVers),_byteMode(other._byteMode),_textureRect(other._textureRect)
{
    _frameInfo = new ReaderInfo;
    *_frameInfo = *other._frameInfo;
}

FrameEntry::~FrameEntry(){
    delete _frameInfo;
}

std::string FrameEntry::printOut(){
    ostringstream oss;
    oss << _path << " " <<_zoom << " "
    << _exposure << " "
    << _lut << " "
    << _treeVers << " "
    << _byteMode << " "
    << _frameInfo->printOut()
    << _textureRect.x << " "
    << _textureRect.y << " "
    << _textureRect.r << " "
    << _textureRect.t << " "
    << _textureRect.w << " "
    << _textureRect.h << " "
    << endl;
    return oss.str();
}

/*Recover an entry from string*/
FrameEntry* FrameEntry::recoverFromString(QString str){
    
    QString zoomStr,expStr,lutStr,treeStr,byteStr,frameInfoStr,xStr,yStr,rightStr,topStr,wStr,hStr;
    int i =0 ;
    while(str.at(i)!= QChar(' ')){zoomStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){expStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){lutStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){treeStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){byteStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){frameInfoStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){xStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){yStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){rightStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){topStr.append(str.at(i));i++;}
    i++;
    while(str.at(i)!= QChar(' ')){wStr.append(str.at(i));i++;}
    i++;
    while(i < str.size()){hStr.append(str.at(i));i++;}
      
    
    TextureRect textureRect(xStr.toInt(),yStr.toInt(),rightStr.toInt(),topStr.toInt(),wStr.toInt(),hStr.toInt());
    FrameEntry* entry = new FrameEntry(zoomStr.toFloat(),expStr.toFloat(),lutStr.toFloat(),treeStr.toULongLong(),byteStr.toFloat(),
                                       ReaderInfo::fromString(frameInfoStr),textureRect);
    
    return entry;
}

std::pair<U64,MemoryMappedEntry*> ViewerCache::recoverEntryFromString(QString str){
    if(str.isEmpty()) return make_pair(0, (MemoryMappedEntry*)NULL);
    QString path,entryStr;
    int i =0 ;
    while(str.at(i)!= QChar(' ')){path.append(str.at(i));i++;}
    i++;
    while(i < str.size()){entryStr.append(str.at(i));i++;}
    i++;
    
    if (!QFile::exists(path)) {
        return make_pair(0, (MemoryMappedEntry*)NULL);;
    }
    
    FrameEntry* entry = FrameEntry::recoverFromString(entryStr);
    if(!entry){
        cout << "Invalid entry : " << qPrintable(path) << endl;
        return make_pair(0,(FrameEntry*)NULL);
    }else{
        U64 dataSize ;
        if(entry->_byteMode == 1){
            dataSize = entry->_textureRect.w * entry->_textureRect.h * 4;
        }else{
            dataSize = entry->_textureRect.w * entry->_textureRect.h * 4 * sizeof(float);
        }
        string pathStd = path.toStdString();
        if(!entry->allocate(dataSize,pathStd.c_str())){
            QFile::remove(path);
            delete entry;
            return make_pair(0, (MemoryMappedEntry*)NULL);
        }
    }
    QString hashKey;
    int j = path.size() - 1;
    while(path.at(j) != QChar('.')) j--;
    j--;
    while(path.at(j) != QChar('/')){hashKey.prepend(path.at(j));j--;}
    j--;
    while (path.at(j) != QChar('/')) {
        hashKey.prepend(path.at(j));
        j--;
    }
    bool ok;
    U64 key = hashKey.toULongLong(&ok,16);
    
    return make_pair(key,entry);
}

ViewerCache* ViewerCache::getViewerCache(){
    return ViewerCache::instance();
}

/*Construct a frame entry,adds it to the cache and returns a pointer to it.*/
FrameEntry* ViewerCache::addFrame(U64 key,
                                  U64 treeVersion,
                                  float zoomFactor,
                                  float exposure,
                                  float lut ,
                                  float byteMode,
                                  const TextureRect& textureRect,
                                  const Box2D& bbox,
                                  const Format& dispW){
    
    
    FrameEntry* out  = new FrameEntry(zoomFactor,exposure,lut,
                                      treeVersion,byteMode,bbox,dispW,Mask_RGBA,textureRect);
    string name(getCachePath().toStdString());
    {
        ostringstream oss1;
        oss1 << hex << (key >> 60);
        oss1 << hex << ((key << 4) >> 60);
        
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
        dataSize = textureRect.w*textureRect.h*4;
    }else{
        dataSize = textureRect.w*textureRect.h*4*sizeof(float);
    }
    
    if(!out->allocate(dataSize,name.c_str())){
        delete out;
        return NULL;
    }
    if(!out->getMappedFile()->data()){
        delete out;
        return NULL;
    }
    currentViewer->getUiContext()->frameSeeker->addCachedFrame(currentViewer->currentFrame());
    out->addReference(); //increase refcount BEFORE adding it to the cache and exposing it to the other threads
    if(AbstractDiskCache::add(key, out)){
        currentViewer->getUiContext()->frameSeeker->removeCachedFrame();
    }
    
    return out;
}

void ViewerCache::clearInMemoryPortion(){
    if(currentViewer)
        currentViewer->getUiContext()->frameSeeker->clearCachedFrames();
    clearInMemoryCache();
}

FrameEntry* ViewerCache::get(U64 key){
    CacheEntry* entry = isInMemory(key);
    if (!entry) {// not in memory
        entry = getCacheEntry(key);
        if(!entry){ //neither on disk
            return NULL;
        }else{ // found on disk
            FrameEntry* frameEntry = dynamic_cast<FrameEntry*>(entry);
            assert(frameEntry);
            if(!frameEntry->reOpen()){
                return NULL;
            }
            currentViewer->getUiContext()->frameSeeker->addCachedFrame(currentViewer->currentFrame());
            return frameEntry;
        }
    }else{ // found in memory
        FrameEntry* frameEntry = dynamic_cast<FrameEntry*>(entry);
        assert(frameEntry);
        currentViewer->getUiContext()->frameSeeker->addCachedFrame(currentViewer->currentFrame());
        return frameEntry;
    }
    return NULL;
}


U64 FrameEntry::computeHashKey(int frameNB,
                               U64 treeVersion,
                               float zoomFactor,
                               float exposure,
                               float lut ,
                               float byteMode,
                               const Box2D& bbox,
                               const Format& dispW,
                               const TextureRect& frameRect){
    Hash _hash;
    _hash.appendValueToHash(frameNB);
    _hash.appendValueToHash(treeVersion);
    _hash.appendValueToHash((U64)*(reinterpret_cast<U32*>(&zoomFactor)));
    _hash.appendValueToHash((U64)*(reinterpret_cast<U32*>(&exposure)));
    _hash.appendValueToHash((U64)*(reinterpret_cast<U32*>(&lut)));
    _hash.appendValueToHash((U64)*(reinterpret_cast<U32*>(&byteMode)));
    _hash.appendValueToHash(bbox.x());
    _hash.appendValueToHash(bbox.y());
    _hash.appendValueToHash(bbox.top());
    _hash.appendValueToHash(bbox.right());
    _hash.appendValueToHash(dispW.x());
    _hash.appendValueToHash(dispW.y());
    _hash.appendValueToHash(dispW.top());
    _hash.appendValueToHash(dispW.right());
    _hash.appendValueToHash(frameRect.x);
    _hash.appendValueToHash(frameRect.y);
    _hash.appendValueToHash(frameRect.t);
    _hash.appendValueToHash(frameRect.r);
    _hash.appendValueToHash(frameRect.w);
    _hash.appendValueToHash(frameRect.h);
    _hash.computeHash();
    return _hash.getHashValue();
}

