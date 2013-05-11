#include <QtCore/QDir>
#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>
#include <QtGui/QVector2D>
#include <cassert>
#include "Core/diskcache.h"
#include "Superviser/controler.h"
#include "Core/model.h"
#include "Core/settings.h"
#include "Gui/GLViewer.h"
#include "Core/mappedfile.h"
#include "Gui/mainGui.h"
#include "Gui/timeline.h"
using namespace std;
using Powiter_Enums::MMAPfile_mode;

DiskCache::DiskCache(ViewerGL* gl_viewer,qint64 maxDiskSize,qint64 maxRamSize) : cacheSize(0),MAX_DISK_CACHE(maxDiskSize),MAX_RAM_CACHE(maxRamSize)
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

void DiskCache::clearCache(){
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

DiskCache::~DiskCache(){
    
    saveCache();
    
    
}

FrameID::FrameID(float zoom,float exp,float lut,int rank,U32 treeVers,
                 char* cacheIndex,int rowWidth,int nbRows,
                 char* fileName,float byteMode):
_zoom(zoom), _exposure(exp),_lut(lut),_rank(rank),_treeVers(treeVers),
_cacheIndex(cacheIndex),_rowWidth(rowWidth),_nbRows(nbRows),
_filename(fileName),_byteMode(byteMode){}

FrameID::FrameID(const FrameID& other):_zoom(other._zoom),_exposure(other._exposure),_lut(other._lut),_rank(other._rank),_treeVers(other._treeVers),
_nbRows(other._nbRows),_cacheIndex(other._cacheIndex),_rowWidth(other._rowWidth),
_filename(other._filename),_byteMode(other._byteMode)
{ }




void DiskCache::saveCache(){
    QFile _restoreFile(CACHE_ROOT_PATH"settings.cachesettings");
    _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QTextStream out(&_restoreFile);
    out << CACHE_VERSION << endl;
    FramesIterator it = _frames.begin();
    for(;it!=_frames.end();it++){
        FrameID frame(it->second);
        out <<  frame._filename << " " << frame._zoom << " " << frame._exposure << " " << frame._lut << " "
        << frame._rank << " " <<  frame._treeVers << " "<<  frame._cacheIndex ;
        out << " " << frame._rowWidth << " "<< frame._nbRows << " "  << frame._byteMode ;
        out << "\n";
    }
    
    _restoreFile.close();
    
    // don't forget to close playback cache
    clearPlayBackCache();
}


void DiskCache::restoreCache(){
    
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
                QString zoomStr,expStr,lutStr,rankStr,treeVers,cacheIndexStr,nbRowsStr,rowWidthStr,fileNameStr;
                QString byteModeStr;
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
                while(line.at(i)!=QChar(' ')){ rowWidthStr.append(line.at(i));i++;}
                i++;
                while(line.at(i)!=QChar(' ')) {nbRowsStr.append(line.at(i));i++;}
                i++;
                while(i < line.size() && line.at(i)!=QChar('\n') ){byteModeStr.append(line.at(i));i++;}
                
                
                
                float byteMode = byteModeStr.toFloat();
                char* fileName = QstringCpy(fileNameStr);
                char* _cachedIndex = QstringCpy(cacheIndexStr);
                float zoom =zoomStr.toFloat();
                float exposure = expStr.toFloat();
                float lut =  lutStr.toFloat() ;
                int rank = rankStr.toInt();
                U32 treeHash = (U32)treeVers.toUInt();
                int rowWidth = rowWidthStr.toInt();
                int nbRows = nbRowsStr.toInt();
                FrameID _id( zoom , exposure  , lut,
                            rank,treeHash,_cachedIndex,rowWidth,nbRows,fileName,byteMode);
                _frames.insert(make_pair(_id._filename,_id));
                line = in.readLine();
                if(_id._byteMode==1.0){
                    cacheSize += _id._nbRows *  sizeof(U32) * (_id._rowWidth);
                }else{
                    cacheSize += _id._nbRows * 4 * sizeof(float) * (_id._rowWidth);
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
    FramesIterator it = _frames.begin();
    qint64 index = 0;
    map< int, pair<char*,FrameID> > ranks;
    for(;it!=_frames.end();it++){
        QString fileName(it->second._cacheIndex);
        QString str("frag_");
        char tmp[64];
        sprintf(tmp, "%llu",index);
        str.append(tmp);
        str.append(".cache");
        QFile::rename(fileName, str);
        ranks.insert(make_pair(it->second._rank,*it));
        char* strchr=QstringCpy(str);
        it->second._cacheIndex = strchr;
        index++;
    }
    assert( ranks.size() == _frames.size());
    //remapping ranks to be continuous
    map<int,pair<char*,FrameID> >::iterator ranksIT = ranks.begin();
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
std::pair<FramesIterator,bool> DiskCache::isCached(char* filename,U32 treeVersion,float builtinZoom,float exposure,float lut,bool byteMode ){
    pair< FramesIterator,FramesIterator> range = _frames.equal_range(filename);
    for(FramesIterator it=range.first;it!=range.second;++it){
        if((it->second._zoom == builtinZoom) && (it->second._exposure == exposure) && (it->second._lut == lut)
           && (it->second._treeVers== treeVersion) && (it->second._byteMode == byteMode)){
            return make_pair(it,true);
        }
    }
    
    return make_pair(_frames.end(),false);
}

/* add the frame to the cache if there's enough space, otherwise some free space is made (LRU) to insert it
 It appends the frame with rank 0 (remove last) and increments all the other frame present in cache
 */
void DiskCache::appendFrame(FrameID _info){
    
    // cycle through all the cache to increment the rank of every frame by 1 since we insert a new one
    if(_frames.begin() != _frames.end()){
        ReverseFramesIterator ritFrames = _frames.rbegin();
        map<int,FrameID>::reverse_iterator rit = _rankMap.rbegin();
        assert(_rankMap.size() == _frames.size());
        for(;rit!=_rankMap.rend();rit++){
            pair<int,FrameID> curRank = *rit;
            curRank.first++;
            _rankMap.erase(--(rit.base()));
            _rankMap.insert(curRank);
            ritFrames->second._rank++;
            ritFrames++;
        }
    }
    _frames.insert(make_pair(_info._filename, _info));
    _rankMap.insert(make_pair(_info._rank,_info));
    
}

/* get the frame*/
std::pair<char*,std::pair<int,int> > DiskCache::retrieveFrame(int frameNb,FramesIterator it){
    
    FrameID _id = it->second;
    string filename(CACHE_ROOT_PATH);
    filename.append(_id._cacheIndex);
    size_t dataSize;
    if(_id._byteMode==1.0){
        dataSize  = _id._nbRows * _id._rowWidth*sizeof(U32) ;
    }else{
        dataSize  = _id._nbRows * _id._rowWidth*sizeof(float)*4;
    }
    MMAPfile* cacheFile = 0;
    vector<pair<FrameID,MMAPfile*> >::iterator i = _playbackCache.begin();
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
            return make_pair((char*)0, make_pair(0,0));
        return make_pair(cacheFile->data(),make_pair(_id._rowWidth,_id._nbRows));
    }
    return make_pair((char*)0, make_pair(0,0));
}


void DiskCache::makeFreeSpace(int nbFrames){
    
    map<int,FrameID>::iterator it = _rankMap.end(); it--;
    FramesIterator it2 = _frames.end();it2--;
    int i =0;
    while( i < nbFrames){
        FrameID frame = it->second;
        qint64 frameSize;
        if(frame._byteMode==1.0){
            frameSize = frame._nbRows * sizeof(U32) * frame._rowWidth;
            
        }else{
            frameSize = 4 * frame._nbRows * sizeof(float) * frame._rowWidth;
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
        name.append(frame._cacheIndex);
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


std::pair<char*,FrameID> DiskCache::mapNewFrame(int frameNB,char* filename,int width,int height,int nbFrameHint,U32 treeVers){
    string name("frag_");
    char tmp[64];
    sprintf(tmp,"%llu",newCacheBlockIndex);
    name.append(tmp);
    name.append(".cache");
    newCacheBlockIndex++;
    char* name_ = stdStrCpy(name);
    FrameID _info(gl_viewer->currentBuiltInZoom,gl_viewer->exposure,gl_viewer->_lut,0,treeVers,name_,width,
                  height,filename,gl_viewer->_byteMode);
    size_t sizeNeeded;
    if(gl_viewer->_byteMode == 1.0 || !gl_viewer->_hasHW){
        sizeNeeded = width * sizeof(U32) * height;
    }else{
        sizeNeeded = width * sizeof(float) * height * 4;
    }
    if((cacheSize+ sizeNeeded) > MAX_DISK_CACHE){
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
void DiskCache::closeMappedFile(){
    vector<pair<FrameID,MMAPfile*> >::iterator it = _playbackCache.begin();
    gl_viewer->getControler()->getGui()->viewer_tab->frameSeeker->removeCachedFrame();
    MMAPfile* mf =  it->second;
    _playbackCacheSize-=mf->size();
    mf->close();
    _playbackCache.erase(it);
    delete mf;
    
}

void DiskCache::clearPlayBackCache(){
    
    for(int i = 0 ; i < _playbackCache.size() ;i++){
        _playbackCache[i].second->close();
        delete _playbackCache[i].second;
    }
    gl_viewer->getControler()->getGui()->viewer_tab->frameSeeker->clearCachedFrames();
    _playbackCache.clear();
    _playbackCacheSize = 0;
}
