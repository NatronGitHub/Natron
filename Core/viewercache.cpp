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
using namespace std;
using Powiter_Enums::MMAPfile_mode;

ViewerCache::ViewerCache(ViewerGL* gl_viewer,qint64 maxCacheSize,qint64 maxRamSize) : cacheSize(0),MAX_VIEWER_CACHE_SIZE(maxCacheSize),MAX_RAM_CACHE(maxRamSize)
{
    QDir root(ROOT);
    QStringList entries = root.entryList();
    bool foundDir = false;
    for(int i =0 ; i < entries.size();i++){
        if(entries[i]=="Cache"){
            foundDir=true;
            break;
        }
    }
    if(!foundDir){
        root.mkdir("Cache");
    }
    
    this->gl_viewer = gl_viewer;
    
    restoreCache();
    
    MAX_PLAYBACK_CACHE_SIZE = gl_viewer->getControler()->getModel()->getCurrentPowiterSettings()->maxPlayBackMemoryPercent * MAX_RAM_CACHE;
    _playbackCacheSize = 0;
}

void ViewerCache::clearCache(){
    /*first clear the playback cache*/
    clearPlayBackCache();
    
    QFile::remove(CACHE_ROOT_PATH"settings.cachesettings");
    QDir directory(CACHE_ROOT_PATH);
    QStringList files = directory.entryList(QStringList("*.cache"));
    for(int i =0 ; i< files.size() ;i++){
        QString name(CACHE_ROOT_PATH);
        name.append(files[i]);
        QFile::remove(name);
    }
    _frames.clear();
    _rankMap.clear();
    cacheSize=0;
    newCacheBlockIndex=0;
    
}

ViewerCache::~ViewerCache(){
    
    saveCache();
    
    
}

ViewerCache::FrameID::FrameID(float zoom,float exp,float lut,int rank,U32 treeVers,
                              std::string cacheIndex,float byteMode,ReaderInfo* info,int actualW,int actualH):
_zoom(zoom), _exposure(exp),_lut(lut),_rank(rank),_treeVers(treeVers),
_cacheIndex(cacheIndex),_byteMode(byteMode),_actualW(actualW),_actualH(actualH){
    _frameInfo = new ReaderInfo;
    _frameInfo->copy(info);
}

ViewerCache::FrameID::FrameID(const ViewerCache::FrameID& other):_zoom(other._zoom),_exposure(other._exposure),_lut(other._lut),
_rank(other._rank),_treeVers(other._treeVers),
_cacheIndex(other._cacheIndex),_byteMode(other._byteMode),_actualW(other._actualW),_actualH(other._actualH)
{
    _frameInfo = new ReaderInfo;
    _frameInfo->copy(other._frameInfo);
}




void ViewerCache::saveCache(){
    QFile _restoreFile(CACHE_ROOT_PATH"settings.cachesettings");
    _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QTextStream out(&_restoreFile);
    out << CACHE_VERSION << endl;
    ViewerCache::FramesIterator it = _frames.begin();
    for(;it!=_frames.end();it++){
        FrameID frame(it->second);
        std::string filename = frame._frameInfo->currentFrameName().toStdString();
        out <<  filename.c_str() << " " << frame._zoom << " " << frame._exposure << " " << frame._lut << " "
        << frame._rank << " " <<  frame._treeVers << " "<<  frame._cacheIndex.c_str() ;
        out << " " << frame._byteMode << " ";
        out << frame._actualW << " " << frame._actualH << " ";
        ChannelSet channels = frame._frameInfo->channels();
        foreachChannels(z, channels){
            out << getChannelName(z) << "|";
        }
        out << " " << frame._frameInfo->currentFrame() << " " << frame._frameInfo->firstFrame() << " "
        << frame._frameInfo->lastFrame() << " ";
        Format format = frame._frameInfo->displayWindow();
        Box2D bbox = frame._frameInfo->dataWindow();
        out << format.x() << " " << format.y() << " " << format.right() << " " << format.top() <<" " <<format.pixel_aspect() <<  " ";
        out << bbox.x() << " " << bbox.y() << " " << bbox.right() << " " << bbox.top() << " ";
        out << "\n";
    }
    
    _restoreFile.close();
    
    // don't forget to close playback cache
    clearPlayBackCache();
}


void ViewerCache::restoreCache(){
    
    // restoring the unordered multimap from file
    
    if(QFile::exists(CACHE_ROOT_PATH"settings.cachesettings")){
        QFile _restoreFile(CACHE_ROOT_PATH"settings.cachesettings");
        _restoreFile.open(QIODevice::ReadWrite);
        QTextStream in(&_restoreFile);
        QString line = in.readLine();
        int frameNumber = 0;
        if(line.contains(QString(CACHE_VERSION))){
            line = in.readLine();
            while(!line.isNull()){
                int i=0;
                QString zoomStr,expStr,lutStr,rankStr,treeVers,cacheIndexStr,fileNameStr,channelsStr;
                QString formatX,formatY,formatR,formatT,formatAP;
                QString bboxX,bboxY,bboxR,bboxT;
                QString byteModeStr,currentFrameStr,firstFrameStr,lastFrameStr;
                QString actualWStr,actualHStr;
                while(line.at(i)!=QChar(' ')) {fileNameStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {zoomStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')){ expStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')){ lutStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')){ rankStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {treeVers.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')){ cacheIndexStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {byteModeStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {actualWStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {actualHStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {channelsStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {currentFrameStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {firstFrameStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {lastFrameStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {formatX.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {formatY.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {formatR.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {formatT.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {formatAP.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {bboxX.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {bboxY.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {bboxR.append(line.at(i));i++;}
                i++;
                while(i < line.size() && line.at(i)!=QChar('\n') ){bboxT.append(line.at(i));i++;}
                
                
                float byteMode = byteModeStr.toFloat();
                std::string fileName = QStringToStdString(fileNameStr);
                std::string _cachedIndex = QStringToStdString(cacheIndexStr);
                float zoom =zoomStr.toFloat();
                float exposure = expStr.toFloat();
                float lut =  lutStr.toFloat() ;
                int rank = rankStr.toInt();
                U32 treeHash = (U32)treeVers.toUInt();
                ChannelSet channels;
                int j = 0;
                while(!channelsStr.isEmpty()){
                    j= 0;
                    Channel z;
                    QString chan;
                    while(j!=channelsStr.size() && channelsStr.at(j) != QChar('|')) j++;
                    chan = channelsStr.left(j);
                    if(j+1 < channelsStr.size() && channelsStr.at(j+1) != QChar('\n')){
                        channelsStr.remove(0, j+1);
                    }else{
                        channelsStr.clear();
                    }
                    QByteArray ba = chan.toLatin1();
                    try{
                        z = getChannelByName(ba.constData());
                    }catch(const char* str){
                        cout << "Error while restoring cache: " << str << endl;
                    }
                    channels += z;
                }
                Format format(formatX.toInt(),formatY.toInt(),formatR.toInt(),formatT.toInt(),"",formatAP.toDouble());
                Box2D bbox(bboxX.toInt(),bboxY.toInt(),bboxR.toInt(),bboxT.toInt());
                ReaderInfo* infos = new ReaderInfo(format,bbox,fileNameStr,channels,
                                                   -1,true,currentFrameStr.toInt(),firstFrameStr.toInt(),lastFrameStr.toInt());
                ViewerCache::FrameID _id( zoom , exposure  , lut,
                                         rank,treeHash,_cachedIndex,byteMode,infos,actualWStr.toInt(),actualHStr.toInt());
                _frames.insert(make_pair(fileName,_id));
                line = in.readLine();
                if(_id._byteMode==1.0){
                    cacheSize += sizeof(U32) * format.w() * format.h();
                }else{
                    cacheSize += 4 * sizeof(float) * format.w() * format.h();
                }
                
                frameNumber++;
            }
            // now we just check if there is the same number of cached frames than it is said in the settings file
            QDir directory(CACHE_ROOT_PATH);
            QStringList filters;
            filters << "*.cache";
            QStringList files = directory.entryList(filters);
            if(files.size() != frameNumber){ // issue in cache, we delete everything
                for(int i =0 ; i< files.size() ;i++){
                    QString name(CACHE_ROOT_PATH);
                    name.append(files[i]);
                    QFile::remove(name);
                }
                
                _frames.clear();
                _rankMap.clear();
                cacheSize=0;
                QFile::remove(CACHE_ROOT_PATH"settings.cachesettings");
            }
            
        }else{
            _restoreFile.resize(0);
            
            QDir directory(CACHE_ROOT_PATH);
            QStringList files = directory.entryList(QStringList("*.cache"));
            for(int i =0 ; i< files.size() ;i++){
                QString name(CACHE_ROOT_PATH);
                name.append(files[i]);
                QFile::remove(name);
            }
            
        }
        _restoreFile.close();
    }else{
        QDir directory(CACHE_ROOT_PATH);
        QStringList files = directory.entryList(QStringList("*.cache"));
        for(int i =0 ; i< files.size() ;i++){
            QString name(CACHE_ROOT_PATH);
            name.append(files[i]);
            QFile::remove(name);
            
        }
        QFile _restoreFile(CACHE_ROOT_PATH"settings.cachesettings");
        _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
        QTextStream outsetti(&_restoreFile);
        outsetti << CACHE_VERSION << endl;
        _restoreFile.close();
    }
    
    
    // renaming the cache files to match the number of files in the map
    ViewerCache::FramesIterator it = _frames.begin();
    qint64 index = 0;
    map< int, pair<std::string,ViewerCache::FrameID> > ranks;
    for(;it!=_frames.end();it++){
        QString fileName(it->second._cacheIndex.c_str());
        QString str("frag_");
        char tmp[64];
        sprintf(tmp, "%llu",index);
        str.append(tmp);
        str.append(".cache");
        QFile::rename(fileName, str);
        ranks.insert(make_pair(it->second._rank,*it));
        it->second._cacheIndex = QStringToStdString(str);
        index++;
    }
    assert( ranks.size() == _frames.size());
    //remapping ranks to be continuous
    map<int,pair<std::string,ViewerCache::FrameID> >::iterator ranksIT = ranks.begin();
    int count = 0;
    for(;ranksIT!=ranks.end();ranksIT++){
        ranksIT->second.second._rank = count;
        _rankMap.insert(make_pair(count, ranksIT->second.second));
        count++;
    }
    newCacheBlockIndex = count; // start position
    assert(_rankMap.size() == _frames.size());
}

/*checks whether the frame is present or not*/
std::pair<ViewerCache::FramesIterator,bool> ViewerCache::isCached(std::string filename,
                                                                  U32 treeVersion,
                                                                  float zoomFactor,
                                                                  float exposure,
                                                                  float lut,
                                                                  bool byteMode,
                                                                  Format format,
                                                                  Box2D bbox){
    pair< ViewerCache::FramesIterator,ViewerCache::FramesIterator> range = _frames.equal_range(filename);
    for(ViewerCache::FramesIterator it=range.first;it!=range.second;++it){
        if((it->second._zoom == zoomFactor) &&
           (it->second._exposure == exposure) &&
           (it->second._lut == lut) &&
           (it->second._treeVers== treeVersion) &&
           (it->second._byteMode == byteMode) &&
           (it->second._frameInfo->dataWindow() == bbox) &&
           (it->second._frameInfo->displayWindow() == format)){
            return make_pair(it,true);
        }
    }
    
    return make_pair(_frames.end(),false);
}

/* add the frame to the cache if there's enough space, otherwise some free space is made (LRU) to insert it
 It appends the frame with rank 0 (remove last) and increments all the other frame present in cache
 */
void ViewerCache::appendFrame(ViewerCache::FrameID _info){
    
    // cycle through all the cache to increment the rank of every frame by 1 since we insert a new one
    if(_frames.begin() != _frames.end()){
        ViewerCache::ReverseFramesIterator ritFrames = _frames.rbegin();
        map<int,ViewerCache::FrameID>::reverse_iterator rit = _rankMap.rbegin();
        assert(_rankMap.size() == _frames.size());
        for(;rit!=_rankMap.rend();rit++){
            pair<int,ViewerCache::FrameID> curRank = *rit;
            curRank.first++;
            _rankMap.erase(--(rit.base()));
            _rankMap.insert(curRank);
            ritFrames->second._rank++;
            ritFrames++;
        }
    }
    _frames.insert(make_pair(_info._frameInfo->currentFrameName().toStdString(), _info));
    _rankMap.insert(make_pair(_info._rank,_info));
    
}

/* get the frame*/
const char* ViewerCache::retrieveFrame(int frameNb,ViewerCache::FramesIterator it){
    
    ViewerCache::FrameID _id = it->second;
    string filename(CACHE_ROOT_PATH);
    filename.append(_id._cacheIndex);
    size_t dataSize;
    
    if(_id._byteMode==1.0){
        dataSize  = _id._actualW * _id._actualH *sizeof(U32) ;
    }else{
        dataSize  = _id._actualW * _id._actualH *sizeof(float)*4;
    }
    MMAPfile* cacheFile = 0;
    vector<pair<ViewerCache::FrameID,MMAPfile*> >::iterator i = _playbackCache.begin();
    for(;i!=_playbackCache.end();i++){
        if(i->first == _id){
            cacheFile = i->second;
        }
    }
    if(!cacheFile){
        while(_playbackCacheSize > MAX_PLAYBACK_CACHE_SIZE){
            closeMappedFile();
        }
        
        cacheFile = new MMAPfile(filename,ReadWrite,dataSize);
        _playbackCache.push_back(make_pair(_id,cacheFile));
        _playbackCacheSize += dataSize;
        
        // also adding the feedback on the timeline for this frame
        gl_viewer->getControler()->getGui()->viewer_tab->frameSeeker->addCachedFrame(frameNb);
        
    }
    
    if(cacheFile->is_open()){
        if(!cacheFile->data())
            return NULL;
        
        
        return cacheFile->data();
    }
    return NULL;
}


void ViewerCache::makeFreeSpace(int nbFrames){
    
    map<int,ViewerCache::FrameID>::iterator it = _rankMap.end(); it--;
    FramesIterator it2 = _frames.end();it2--;
    int i =0;
    while( i < nbFrames){
        ViewerCache::FrameID frame = it->second;
        qint64 frameSize;
        Format frmt = frame._frameInfo->displayWindow();
        if(frame._byteMode==1.0){
            frameSize = frmt.w() * frmt.h() * sizeof(U32);
            
        }else{
            frameSize = 4 * sizeof(float) * frmt.w() * frmt.h();
        }
        
        /*If the frame is in RAM, leave it*/
        bool cont = false;
        for(int i = _playbackCache.size()-1;i >= 0;i--){
            if(_playbackCache[i].first == frame){
                cont = true;
                break;
            }
        }
        if(cont){
            it--;
            it2--;
            continue;
        }
        
        
        QString name(CACHE_ROOT_PATH);
        name.append(frame._cacheIndex.c_str());
        QFile::remove(name);
        cacheSize-=frameSize;
        _rankMap.erase(it);
        _frames.erase(it2);
        if(it2==_frames.begin())
            break;
        
        it--;
        it2--;
        i++;
    }
}


std::pair<char*,ViewerCache::FrameID> ViewerCache::mapNewFrame(int frameNB,
                                                               std::string filename,
                                                               int width,
                                                               int height,
                                                               int nbFrameHint,
                                                               U32 treeVers){
    string name("frag_");
    char tmp[64];
    sprintf(tmp,"%llu",newCacheBlockIndex);
    name.append(tmp);
    name.append(".cache");
    newCacheBlockIndex++;
    ReaderInfo* frameInfo = new ReaderInfo;
    frameInfo->copy(gl_viewer->getCurrentReaderInfo());
    frameInfo->currentFrameName(QString::fromStdString(filename));
    ViewerCache::FrameID _info(gl_viewer->getZoomFactor(),gl_viewer->exposure,
                               gl_viewer->_lut,0,treeVers,name,gl_viewer->_byteMode,
                               frameInfo,width,height);
    size_t sizeNeeded;
    if(gl_viewer->_byteMode == 1.0 || !gl_viewer->_hasHW){
        sizeNeeded = width * sizeof(U32) * height;
    }else{
        sizeNeeded = width * sizeof(float) * height * 4;
    }
    if((cacheSize+ sizeNeeded) > MAX_VIEWER_CACHE_SIZE){
        makeFreeSpace(nbFrameHint);
    }
    while(_playbackCacheSize > MAX_PLAYBACK_CACHE_SIZE){
        closeMappedFile();
    }
    cacheSize+=sizeNeeded;
    _playbackCacheSize+=sizeNeeded;
    string _cacheFileName(CACHE_ROOT_PATH);
    _cacheFileName.append(name);
    MMAPfile* mf = new MMAPfile(_cacheFileName,ReadWrite,sizeNeeded,0,sizeNeeded);
    _playbackCache.push_back(make_pair(_info,mf));
    
    // also adding feedback on the timeline for this frame
    gl_viewer->getControler()->getGui()->viewer_tab->frameSeeker->addCachedFrame(frameNB);
    
    return make_pair(mf->data(),_info);
}
void ViewerCache::closeMappedFile(){
    vector<pair<ViewerCache::FrameID,MMAPfile*> >::iterator it = _playbackCache.begin();
    gl_viewer->getControler()->getGui()->viewer_tab->frameSeeker->removeCachedFrame();
    MMAPfile* mf =  it->second;
    _playbackCacheSize-=mf->size();
    mf->close();
    _playbackCache.erase(it);
    delete mf;
    
}

void ViewerCache::clearPlayBackCache(){
    
    for(int i = 0 ; i < _playbackCache.size() ;i++){
        _playbackCache[i].second->close();
        delete _playbackCache[i].second;
    }
    gl_viewer->getControler()->getGui()->viewer_tab->frameSeeker->clearCachedFrames();
    _playbackCache.clear();
    _playbackCacheSize = 0;
}


void ViewerCache::debugCache(){
    cout << "=========CACHE DUMP===========" << endl;
    for(ViewerCache::FramesIterator it = _frames.begin();it!=_frames.end();it++){
        cout << it->first << endl;
    }
    cout <<"===============================" << endl;
}