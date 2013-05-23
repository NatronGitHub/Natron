//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <cassert>
#include "Reader/Reader.h"
#include "Reader/readffmpeg.h"
#include "Reader/readExr.h"
#include "Reader/readQt.h"
#include "Core/node.h"
#include "Gui/GLViewer.h"
#include "Superviser/controler.h"
#include "Core/VideoEngine.h"
#include "Core/model.h"
#include "Core/outputnode.h"
/*#ifdef __cplusplus
 extern "C" {
 #endif
 #ifdef _WIN32
 READER_EXPORT Reader* BuildReader(Node *node){return new Reader(node);}
 #elif defined(unix) || defined(__unix__) || defined(__unix)
 Reader* BuildReader(Node *node){return new Reader(node);}
 #endif
 #ifdef __cplusplus
 }
 #endif*/

using namespace std;


Reader::Reader(Node* node,ViewerGL* ui_context,ViewerCache* cache):InputNode(node),current_frame(0),video_sequence(0),
readHandle(0),has_preview(false),ui_context(ui_context),_cache(cache),_pboIndex(0){}

Reader::Reader(Reader& ref):InputNode(ref){}

Reader::~Reader(){
	delete preview;
	delete readHandle;
}
const char* Reader::className(){return "Reader";}

const char* Reader::description(){
    return "InputNode";
}
void Reader::initKnobs(Knob_Callback *cb){
	QString desc("File");
	
	Knob::file_Knob(cb,desc,fileNameList);
	
	Node::initKnobs(cb);
}


int Reader::currentFrame(){return current_frame;}

void Reader::incrementCurrentFrameIndex(){current_frame<lastFrame() ? current_frame++ : current_frame;}
void Reader::decrementCurrentFrameIndex(){current_frame>firstFrame() ? current_frame-- : current_frame;}
void Reader::seekFirstFrame(){current_frame=firstFrame();}
void Reader::seekLastFrame(){current_frame=lastFrame();}
void Reader::randomFrame(int f){if(f>=firstFrame() && f<=lastFrame()) current_frame=f;}

Reader::Buffer::DecodedFrameDescriptor Reader::open(QString filename,DecodeMode mode,bool startNewThread){
    QByteArray ba = filename.toLatin1();
    const char* filename_cstr=ba.constData();
    Reader::Buffer::DecodedFrameIterator found = _buffer.isEnqueued(filename_cstr,Buffer::NOTCACHED_SEARCH);
    if(found !=_buffer.end()){
        if(found->_asynchTask && !found->_asynchTask->isFinished()){
            found->_asynchTask->waitForFinished();
        }
        return *found;
    }
    QFuture<void> *future = 0;
    Read* _read = 0;
    /*TODO :should do a more appropriate extension mapping in the future*/
    QString extension=filename.right(4);
    if(extension.at(0) == QChar('.') && extension.at(1) == QChar('e') && extension.at(2) == QChar('x') && extension.at(3) == QChar('r') ){
        
        _read=new ReadExr(this,ui_context);
    }else if(extension.at(0) == QChar('.') && extension.at(1) == QChar('j') && extension.at(2) == QChar('p') && extension.at(3) == QChar('g')){
        
        _read=new ReadQt(this,ui_context);
    }else if(extension.at(0) == QChar('.') && extension.at(1) == QChar('p') && extension.at(2) == QChar('n') && extension.at(3) == QChar('g')){
        
        _read=new ReadQt(this,ui_context);
    }
    else if(extension.at(0) == QChar('.') && extension.at(1) == QChar('d') && extension.at(2) == QChar('p') && extension.at(3) == QChar('x') ){
        
        _read=new ReadFFMPEG(this,ui_context);
    }else if(extension.at(0) == QChar('t') && extension.at(1) == QChar('i') && extension.at(2) == QChar('f') && extension.at(3) == QChar('f') ){
    }else{
    }
    
    if(_read == 0){
        Reader::Buffer::DecodedFrameDescriptor desc;
        return desc;
    }
    bool openStereo = false;
    if(_read->supports_stereo() && mode == STEREO_DECODE) openStereo = true;
    pair<Reader::Buffer::DecodedFrameIterator,bool> ret;
    if(startNewThread){
        future = new QFuture<void>;
        *future = QtConcurrent::run(_read,&Read::open,filename,openStereo);
        return _buffer.insert(filename, future, NULL , _read,NULL);
    }else{
        _read->open(filename,openStereo);
        return _buffer.insert(filename, NULL, _read->getReaderInfo(), _read,NULL);
    }
    
}

Reader::Buffer::DecodedFrameDescriptor Reader::openCachedFrame(ViewerCache::FramesIterator frame,int frameNb,bool startNewThread){
    Reader::Buffer::DecodedFrameIterator found = _buffer.isEnqueued(frame->first, Buffer::BOTH_SEARCH);
    if(found !=_buffer.end()){
        if(found->_cacheWatcher || found->_cachedFrame){
            if(found->_cacheWatcher){
                if(!found->_cacheWatcher->isFinished())
                    found->_cacheWatcher->waitForFinished();
                found->_cachedFrame = found->_cacheWatcher->result();
            }
            return *found;
        }else{
            /*if the frame is already present in the buffer but was not a cached frame, override it*/
            _buffer.erase(found);
        }
    }
    removeCachedFramesFromBuffer();
    
    ReaderInfo *info = new ReaderInfo;
    info->copy(frame->second._frameInfo);
    int w = frame->second._actualW ;
    int h = frame->second._actualH ;
    size_t dataSize = 0;
    if(frame->second._byteMode==1.0){
        dataSize  = w * h * sizeof(U32) ;
    }else{
        dataSize  = w * h  * sizeof(float) * 4;
    }
    /*allocating pbo*/
    
   
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,ui_context->getPBOId(_pboIndex));
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, NULL, GL_DYNAMIC_DRAW_ARB);
    void* output = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    _pboIndex = (_pboIndex+1)%2;
    QFutureWatcher<const char*>* watcher = 0;
    QFuture<const char*>* future = 0;
    const char* cachedFrame = 0 ;
    if(!startNewThread){
        cachedFrame =  _cache->retrieveFrame(frameNb,frame);
        ui_context->fillPBO(cachedFrame, output, dataSize);
        return _buffer.insert(info->currentFrameName(), 0 , info, 0 , cachedFrame,watcher);
    }else{
        future = new QFuture<const char*>;
        watcher = new QFutureWatcher<const char*>;
        Reader::Buffer::DecodedFrameDescriptor newFrame =_buffer.insert(info->currentFrameName(), 0 , info, 0 , cachedFrame,watcher);
        *future = QtConcurrent::run(this,&Reader::retrieveCachedFrame,frame,frameNb,output,dataSize);
        watcher->setFuture(*future);
        return newFrame;
    }
}

const char* Reader::retrieveCachedFrame(ViewerCache::FramesIterator frame,int frameNb,void* dst,size_t dataSize){
    const char* cachedFrame =  _cache->retrieveFrame(frameNb,frame);
    ui_context->fillPBO(cachedFrame, dst, dataSize);
    return cachedFrame;
}
std::vector<Reader::Buffer::DecodedFrameDescriptor>
Reader::decodeFrames(DecodeMode mode,bool useCurrentThread,bool useOtherThread,bool forward){
    
    std::vector<Reader::Buffer::DecodedFrameDescriptor> out;
    if(useCurrentThread){
        Buffer::DecodedFrameDescriptor ret = open(files[current_frame], mode , false);
        out.push_back(ret);
    }else{
        if(useOtherThread){
            int followingFrame = current_frame;
            forward ? followingFrame++ : followingFrame--;
            if(followingFrame > lastFrame()) followingFrame = firstFrame();
            if(followingFrame < firstFrame()) followingFrame = lastFrame();
            Buffer::DecodedFrameDescriptor ret = open(files[followingFrame], mode ,true);
            out.push_back(ret);
            // must review this part
        }
    }
    return out;
}
void Reader::removeCurrentFrameFromBuffer(){
    _buffer.remove(getCurrentFrameName());
}

void Reader::createReadHandle(){
	if(readHandle){
        ui_context->clearViewer();
        Controler* ctrlPTR= node_gui->getControler();
        VideoEngine* vengine=ctrlPTR->getModel()->getVideoEngine();
        Node* output =dynamic_cast<Node*>(vengine->getOutputNode());
        if(output){
            ctrlPTR->getModel()->getVideoEngine()->clearInfos(output);
        }
    }
    _buffer.clear();
    
    getVideoSequenceFromFilesList();
    
    open(fileNameList.at(0), DEFAULT_DECODE ,false);
    
    if(!makeCurrentDecodedFrame()){
        cout << "Reader failed to open current frame file" << endl;
    }
    QByteArray ba = fileNameList.at(0).toLatin1();
    readHandle->make_preview(ba.constData());
    
}

void Reader::removeCachedFramesFromBuffer(){
    _buffer.removeAllCachedFrames();
}
void Reader::Buffer::removeAllCachedFrames(){
    bool recursive = false;
    for(DecodedFrameIterator it = _buffer.begin();it != _buffer.end();it++){
        if(it->_cachedFrame || it->_cacheWatcher){
            _buffer.erase(it);
            recursive = true;
            break;
        }
    }
    if(recursive)
        removeAllCachedFrames();
}

bool Reader::makeCurrentDecodedFrame(){
    QString currentFile = files[current_frame];
    Reader::Buffer::DecodedFrameIterator frame = _buffer.isEnqueued(currentFile.toStdString(),Buffer::BOTH_SEARCH);
    if(frame == _buffer.end() || (frame->_asynchTask && !frame->_asynchTask->isFinished())) return false;
    
    readHandle = frame->_readHandle;
    ReaderInfo* readInfo = frame->_frameInfo;
    if(!readInfo && frame->_asynchTask){
        readInfo = readHandle->getReaderInfo();
    }
    readInfo->currentFrame(currentFrame());
    readInfo->firstFrame(firstFrame());
    readInfo->lastFrame(lastFrame());
    Box2D bbox = readInfo->dataWindow();
    _info->set(bbox.x(), bbox.y(), bbox.right(), bbox.top());
    _info->setDisplayWindow(readInfo->displayWindow());
    _info->set_channels(readInfo->channels());
    _info->setYdirection(readInfo->Ydirection());
    _info->rgbMode(readInfo->rgbMode());
    ui_context->getControler()->getModel()->getVideoEngine()->popReaderInfo(this);
    ui_context->getControler()->getModel()->getVideoEngine()->pushReaderInfo(readInfo, this);
    return true;
}


void Reader::engine(int y,int offset,int range,ChannelMask c,Row* out){
	readHandle->engine(y,offset,range,c,out);
	
}

void Reader::createKnobDynamically(){
	readHandle->createKnobDynamically();
	Node::createKnobDynamically();
}


Reader::Buffer::DecodedFrameDescriptor Reader::Buffer::insert(QString filename,
                                                              QFuture<void> *future,
                                                              ReaderInfo* info,
                                                              Read* readHandle,
                                                              const char* cachedFrame,
                                                              QFutureWatcher<const char*>* cacheWatcher){
    //if buffer is full, we remove previously computed frame
    if(_buffer.size() == _bufferSize){
        for(U32 i = 0 ; i < _buffer.size() ;i++){
            DecodedFrameDescriptor frameToRemove = _buffer[i];
            if((!frameToRemove._asynchTask && !frameToRemove._cacheWatcher) ||
               (frameToRemove._asynchTask && frameToRemove._asynchTask->isFinished()) ||
               (frameToRemove._cacheWatcher && frameToRemove._cacheWatcher->isFinished())){
                _buffer.erase(_buffer.begin()+i);
                break;
            }
        }
    }
    std::string _name  = QStringToStdString(filename);
    DecodedFrameDescriptor desc(future,readHandle,info,cachedFrame,cacheWatcher,_name);
    _buffer.push_back(desc);
    
    return desc;
}
Reader::Buffer::DecodedFrameIterator Reader::Buffer::find(std::string filename){
    for(int i = _buffer.size()-1; i >= 0 ; i--){
        if(_buffer[i]._filename==filename) return _buffer.begin()+i;
    }
    return _buffer.end();
}

void Reader::Buffer::remove(std::string filename){
    DecodedFrameIterator it = find(filename);
    if(it!=_buffer.end()){
        if(it->_asynchTask)
            delete it->_asynchTask;//delete future
        if(it->_cacheWatcher)
            delete it->_cacheWatcher;//delete watcher for cached frame
        if(it->_frameInfo)
            delete it->_frameInfo; // delete readerInfo
        if(it->_readHandle)
            delete it->_readHandle; // delete readHandle
        _buffer.erase(it);
    }
}

QFuture<void>* Reader::Buffer::getFuture(std::string filename){
    Buffer::DecodedFrameIterator it = find(filename);
    if(it->_asynchTask)
        return it->_asynchTask;
    else
        return NULL;
}
bool Reader::Buffer::decodeFinished(std::string filename){
    Buffer::DecodedFrameIterator it = find(filename);
    return (it->_asynchTask && it->_asynchTask->isFinished())
    || (it->_cacheWatcher && it->_cacheWatcher->isFinished());
}
void Reader::Buffer::debugBuffer(){
    cout << "=========BUFFER DUMP=============" << endl;
    for(DecodedFrameIterator it = _buffer.begin(); it != _buffer.end() ; it++){
        if(it->_cachedFrame || it->_cacheWatcher)
            cout << it->_filename << " (cached) " << endl;
        else
            cout << it->_filename << endl;
    }
    cout << "=================================" << endl;
}

Reader::Buffer::DecodedFrameIterator Reader::Buffer::isEnqueued(std::string filename,SEARCH_TYPE searchMode){
    if(searchMode == CACHED_SEARCH){
        DecodedFrameIterator ret = find(filename);
        if(ret != _buffer.end()){
            if(ret->_cachedFrame || ret->_cacheWatcher){
                return ret;
            }else{
                return _buffer.end();
            }
        }else{
            return _buffer.end();
        }
    }else if(searchMode == NOTCACHED_SEARCH){
        DecodedFrameIterator ret = find(filename);
        if(ret != _buffer.end()){
            if(!ret->_cachedFrame){
                return ret;
            }else{
                return _buffer.end();
            }
        }else{
            return _buffer.end();
        }
    }else{
        return find(filename);
    }
}

void Reader::Buffer::clear(){
    DecodedFrameIterator it = _buffer.begin();
    for(;it!=_buffer.end();it++){
        if(it->_asynchTask)
            delete it->_asynchTask;//delete future
        if(it->_cacheWatcher)
            delete it->_cacheWatcher;//delete watcher for cached frame
        if(it->_frameInfo)
            delete it->_frameInfo; // delete readerInfo
        if(it->_readHandle)
            delete it->_readHandle; // delete readHandle
    }
    _buffer.clear();
}
void Reader::Buffer::erase(DecodedFrameIterator it){
    if(it->_asynchTask)
        delete it->_asynchTask;//delete future
    if(it->_cacheWatcher)
        delete it->_cacheWatcher;//delete watcher for cached frame
    if(it->_frameInfo)
        delete it->_frameInfo; // delete readerInfo
    if(it->_readHandle)
        delete it->_readHandle; // delete readHandle
    _buffer.erase(it);}

void Reader::getVideoSequenceFromFilesList(){
    files.clear();
	if(fileNameList.size() > 1 ){
		video_sequence=true;
	}else{
        video_sequence=false;
    }
    bool first_time=true;
    QString originalName;
    foreach(QString Qfilename,fileNameList)
    {	if(Qfilename.at(0) == QChar('.')) continue;
        QString const_qfilename=Qfilename;
        if(first_time){
            Qfilename=Qfilename.remove(Qfilename.length()-4,4);
            int j=Qfilename.length()-1;
            QString frameIndex;
            while(j>0 && Qfilename.at(j).isDigit()){
                frameIndex.push_front(Qfilename.at(j));
                j--;
            }
            if(j>0){
				int number=0;
                if(fileNameList.size() > 1){
                    number = frameIndex.toInt();
                }
				files.insert(make_pair(number,const_qfilename));
                originalName=Qfilename.remove(j+1,frameIndex.length());
                
            }else{
                files[0]=const_qfilename;
            }
            first_time=false;
        }else{
            if(Qfilename.contains(originalName) /*&& (extension)*/){
                Qfilename.remove(Qfilename.length()-4,4);
                int j=Qfilename.length()-1;
                QString frameIndex;
                while(j>0 && Qfilename.at(j).isDigit()){
                    frameIndex.push_front(Qfilename.at(j));
                    j--;
                }
                if(j>0){
                    int number = frameIndex.toInt();
                    files[number]=const_qfilename;
                }else{
                    cout << " Read handle : WARNING !! several frames read but no frame count found in their name " << endl;
                }
            }
        }
    }
    std::map<int,QString>::iterator it;
    it=files.begin();
    current_frame=(*it).first;
}
int Reader::firstFrame(){
    int minimum=INT_MAX;
	for(std::map<int,QString>::iterator it=files.begin();it!=files.end();it++){
        int nb=(*it).first;
        if(nb<minimum){
            minimum=nb;
        }
    }
	
	return minimum;
}
int Reader::lastFrame(){
	int maximum=-1;
	for(std::map<int,QString>::iterator it=files.begin();it!=files.end();it++){
        int nb=(*it).first;
        if(nb>maximum){
            maximum=nb;
        }
    }
    return maximum;
}

std::string Reader::getCurrentFrameName(){
    return QStringToStdString(files[current_frame]);
}
std::string Reader::getRandomFrameName(int f){
    return QStringToStdString(files[f]);
}
