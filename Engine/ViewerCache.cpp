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

#include "ViewerCache.h"

#include <cassert>
#include <sstream>
#include <QtCore/QDir>
#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>
#include <QtGui/QVector2D>
#include <QtCore/QFile>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include "Global/AppManager.h"
#include "Engine/Model.h"
#include "Engine/Settings.h"
#include "Engine/MemoryFile.h"
#include "Engine/Hash64.h"
#include "Engine/Row.h"
#include "Engine/Format.h"
#include "Engine/ViewerNode.h"

#include "Readers/Reader.h"

using namespace std;
using namespace Powiter;


ViewerCache::ViewerCache()
: AbstractDiskCache(0)
{
}

ViewerCache::~ViewerCache()
{
}


FrameEntry::FrameEntry()
: MemoryMappedEntry()
, _exposure(0.)
, _lut(0.)
, _zoom(0.)
, _treeVers(0)
, _frameInfo (new ReaderInfo)
, _byteMode(0)
, _textureRect() {
    _frameInfo = new ReaderInfo;
}

FrameEntry::FrameEntry(QString inputFileNames, float zoom, float exp, float lut, U64 treeVers,
                       float byteMode, const Box2D& bbox, const Format& dispW,
                       const ChannelSet& channels, const TextureRect& textureRect)
: MemoryMappedEntry()
, _exposure(exp)
, _lut(lut)
, _zoom(zoom)
, _treeVers(treeVers)
, _frameInfo (new ReaderInfo)
, _byteMode(byteMode)
, _textureRect(textureRect) {
    _frameInfo->set_dataWindow(bbox);
    _frameInfo->set_displayWindow(dispW);
    _frameInfo->set_channels(channels);
    _frameInfo->setCurrentFrameName(inputFileNames.toStdString());
    
}

FrameEntry::FrameEntry(float zoom, float exp, float lut, U64 treeVers,
                       float byteMode, ReaderInfo* info, const TextureRect& textureRect)
: MemoryMappedEntry()
, _exposure(exp)
, _lut(lut)
, _zoom(zoom)
, _treeVers(treeVers)
, _frameInfo(info)
, _byteMode(byteMode)
, _textureRect(textureRect)
{
}

FrameEntry::~FrameEntry(){
    delete _frameInfo;
}
void FrameEntry::writeToXml(QXmlStreamWriter* writer){
    writer->writeAttribute("Path",_path.c_str());
    writer->writeAttribute("Exposure", QString::number(_exposure));
    writer->writeAttribute("Lut", QString::number(_lut));
    writer->writeAttribute("TreeVersion",QString::number(_treeVers));
    writer->writeAttribute("ByteMode",QString::number(_byteMode));
    writer->writeStartElement("ReaderInfo");
    _frameInfo->writeToXml(writer);
    writer->writeEndElement();
    writer->writeStartElement("TextureRect");
    writer->writeAttribute("x",QString::number(_textureRect.x));
    writer->writeAttribute("y",QString::number(_textureRect.y));
    writer->writeAttribute("r",QString::number(_textureRect.r));
    writer->writeAttribute("t",QString::number(_textureRect.y));
    writer->writeAttribute("w",QString::number(_textureRect.w));
    writer->writeAttribute("h",QString::number(_textureRect.h));
    writer->writeEndElement();

}

/*Recover an entry from string*/
FrameEntry* FrameEntry::entryFromXml(QXmlStreamReader* reader){
    QString zoomStr,expStr,lutStr,treeStr,byteStr,xStr,yStr,rStr,tStr,wStr,hStr;
    ReaderInfo * info = 0;
    if(!reader->atEnd() && !reader->hasError()){
        QXmlStreamAttributes attributes = reader->attributes();
        if (attributes.hasAttribute("Exposure")) {
            expStr = attributes.value("Exposure").toString();
        }else{
            return NULL;
        }
        if (attributes.hasAttribute("Lut")) {
            lutStr = attributes.value("Lut").toString();
        }else{
            return NULL;
        }
        if (attributes.hasAttribute("TreeVersion")) {
            treeStr = attributes.value("TreeVersion").toString();
        }else{
            return NULL;
        }
        if (attributes.hasAttribute("ByteMode")) {
            byteStr = attributes.value("ByteMode").toString();
        }else{
            return NULL;
        }
        QXmlStreamReader::TokenType token = reader->readNext();
        int i =0;
        while(token == QXmlStreamReader::StartElement && i < 2){
            if (reader->name() == "ReaderInfo") {
                info = ReaderInfo::fromXml(reader);
                if(!info)
                    return NULL;
            }else if(reader->name() == "TextureRect"){
                QXmlStreamAttributes textureAttributes = reader->attributes();
                if(textureAttributes.hasAttribute("x")){
                    xStr = textureAttributes.value("x").toString();
                }else{
                    return NULL;
                }
                if(textureAttributes.hasAttribute("y")){
                    yStr = textureAttributes.value("y").toString();
                }else{
                    return NULL;
                }
                if(textureAttributes.hasAttribute("r")){
                    rStr = textureAttributes.value("r").toString();
                }else{
                    return NULL;
                }
                if(textureAttributes.hasAttribute("t")){
                    tStr = textureAttributes.value("t").toString();
                }else{
                    return NULL;
                }
                if(textureAttributes.hasAttribute("w")){
                    wStr = textureAttributes.value("w").toString();
                }else{
                    return NULL;
                }
                if(textureAttributes.hasAttribute("h")){
                    hStr = textureAttributes.value("h").toString();
                }else{
                    return NULL;
                }
            }
            ++i;
        }
    }
    
      
    
    TextureRect textureRect(xStr.toInt(),yStr.toInt(),rStr.toInt(),tStr.toInt(),wStr.toInt(),hStr.toInt());
    FrameEntry* entry = new FrameEntry(zoomStr.toFloat(),expStr.toFloat(),lutStr.toFloat(),treeStr.toULongLong(),byteStr.toFloat(),
                                       info,textureRect);
    
    return entry;
}

std::pair<U64,MemoryMappedEntry*> ViewerCache::entryFromXml(QXmlStreamReader* reader){
    QXmlStreamAttributes attributes = reader->attributes();
    QString path;
    if (attributes.hasAttribute("Path")) {
        path = attributes.value("Path").toString();
    }else{
        return make_pair(0, (MemoryMappedEntry*)NULL);
    }

    if (!QFile::exists(path)) {
        return make_pair(0, (MemoryMappedEntry*)NULL);;
    }
    
    FrameEntry* entry = FrameEntry::entryFromXml(reader);
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
        entry->lock();
        if(!entry->allocate(dataSize,pathStd.c_str())){
            entry->unlock();
            QFile::remove(path);
            delete entry;
            return make_pair(0, (MemoryMappedEntry*)NULL);
        }
        entry->unlock();
    }
    QString hashKey;
    int j = path.size() - 1;
    while(path.at(j) != QChar('.')) {
        --j;
    }
    --j;
    while(path.at(j) != QChar('/')) {
        hashKey.prepend(path.at(j));
        --j;
    }
    --j;
    while (path.at(j) != QChar('/')) {
        hashKey.prepend(path.at(j));
        --j;
    }
    bool ok;
    U64 key = hashKey.toULongLong(&ok,16);
    
    return make_pair(key,entry);
}



/*Construct a frame entry,adds it to the cache and returns a pointer to it.*/
// on output, the FrameEntry is locked, and must be unlocked using FrameEntry::unlock()
FrameEntry* ViewerCache::addFrame(U64 key,
                                  QString inputFileNames,
                                  U64 treeVersion,
                                  float zoomFactor,
                                  float exposure,
                                  float lut ,
                                  float byteMode,
                                  const TextureRect& textureRect,
                                  const Box2D& bbox,
                                  const Format& dispW){
    
    
    FrameEntry* out  = new FrameEntry(inputFileNames,zoomFactor,exposure,lut,
                                      treeVersion,byteMode,bbox,dispW,Mask_RGBA,textureRect);
    string name((getCachePath()+QDir::separator()).toStdString());
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
    out->lock();
    if(!out->allocate(dataSize,name.c_str())){
        delete out;
        return NULL;
    }
    if(!out->getMappedFile()->data()){
        delete out;
        return NULL;
    }
    emit addedFrame();
    out->addReference(); //increase refcount BEFORE adding it to the cache and exposing it to the other threads
    if (AbstractDiskCache::add(key, out)) {
        emit removedFrame();
    }
    //assert(out->getMappedFile());
    return out;
}

void ViewerCache::clearInMemoryPortion(){
    emit clearedInMemoryFrames();
    clearInMemoryCache();
}

// on output, the FrameEntry is locked, and must be unlocked using FrameEntry::unlock()
FrameEntry* ViewerCache::get(U64 key){
    CacheEntry* entry = isInMemory(key);
    if (!entry) {// not in memory
        entry = getCacheEntry(key);
        if (!entry) { //neither on disk
            return NULL;
        } else { // found on disk
            FrameEntry* frameEntry = dynamic_cast<FrameEntry*>(entry);
            assert(frameEntry);
            if(!frameEntry->reOpen()){
                return NULL;
            }
            emit addedFrame();
            return frameEntry;
        }
    } else { // found in memory
        FrameEntry* frameEntry = dynamic_cast<FrameEntry*>(entry);
        assert(frameEntry);
        emit addedFrame();
        return frameEntry;
    }
    return NULL;
}


U64 FrameEntry::computeHashKey(int frameNb,
                               QString inputFileNames,
                               U64 treeVersion,
                               float zoomFactor,
                               float exposure,
                               float lut ,
                               float byteMode,
                               const Box2D& bbox,
                               const Format& dispW,
                               const TextureRect& frameRect){
    Hash64 _hash;
    _hash.append(frameNb);
    for (int i = 0; i < inputFileNames.size(); ++i) {
        _hash.append(inputFileNames.at(i).unicode());
    }
    _hash.append(treeVersion);
    _hash.append((U64)*(reinterpret_cast<U32*>(&zoomFactor)));
    _hash.append((U64)*(reinterpret_cast<U32*>(&exposure)));
    _hash.append((U64)*(reinterpret_cast<U32*>(&lut)));
    _hash.append((U64)*(reinterpret_cast<U32*>(&byteMode)));
    _hash.append(bbox.left());
    _hash.append(bbox.bottom());
    _hash.append(bbox.top());
    _hash.append(bbox.right());
    _hash.append(dispW.left());
    _hash.append(dispW.bottom());
    _hash.append(dispW.top());
    _hash.append(dispW.right());
    _hash.append(frameRect.x);
    _hash.append(frameRect.y);
    _hash.append(frameRect.t);
    _hash.append(frameRect.r);
    _hash.append(frameRect.w);
    _hash.append(frameRect.h);
    _hash.computeHash();
    return _hash.value();
}

