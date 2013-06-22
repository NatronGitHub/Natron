//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <cassert>
#include "Superviser/powiterFn.h"
#include "Reader/Reader.h"
#include "Reader/readffmpeg.h"
#include "Reader/readExr.h"
#include "Reader/readQt.h"
#include "Core/node.h"
#include "Gui/node_ui.h"
#include "Gui/GLViewer.h"
#include "Core/mappedfile.h"
#include "Superviser/controler.h"
#include "Core/VideoEngine.h"
#include "Core/model.h"
#include "Core/outputnode.h"
#include "Core/settings.h"
#include "Reader/Read.h"
#include "Gui/timeline.h"
#include "Gui/viewerTab.h"
#include "Gui/mainGui.h"
#include "Core/viewercache.h"
#include "Gui/knob.h"
#include <QtGui/QImage>
#include <sstream>
#include "Core/Box.h"
#include "Core/displayFormat.h"
#include "Writer/Writer.h"
#include "Core/viewerNode.h"
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


Reader::Reader(Node* node):InputNode(node),video_sequence(0),
readHandle(0),has_preview(false),_pboIndex(0),preview(0){}

Reader::Reader(Reader& ref):InputNode(ref){}

Reader::~Reader(){
    _buffer.clear();
	delete preview;
}
std::string Reader::className(){return "Reader";}

std::string Reader::description(){
    return "InputNode";
}
void Reader::initKnobs(Knob_Callback *cb){
    std::string desc("File");
    Knob* knob = KnobFactory::createKnob("InputFile", cb, desc, Knob::NONE);
    File_Knob* file = static_cast<File_Knob*>(knob);
    assert(file);
	file->setPointer(&fileNameList);
	Node::initKnobs(cb);
}


Reader::Buffer::DecodedFrameDescriptor Reader::open(QString filename,DecodeMode mode,bool startNewThread){
    /*the future used to check asynchronous results if the read happens in a new thread*/
    QFuture<void> *future = 0;
    
    /*the read handle used to decode the frame*/
    Read* _read = 0;
    
    QString extension;
    for(int i = filename.size() - 1 ; i>= 0 ; i--){
        QChar c = filename.at(i);
        if(c != QChar('.'))
            extension.prepend(c);
        else
            break;
    }
    
    PluginID* decoder = Settings::getPowiterCurrentSettings()->_readersSettings.decoderForFiletype(extension.toStdString());
    if(!decoder){
        cout << "ERROR : Couldn't find an appropriate decoder for this filetype ( " << extension.toStdString() << " )" << endl;
        return Buffer::DecodedFrameDescriptor();
    }
    ReadBuilder builder = (ReadBuilder)(decoder->first);
    _read = builder(this);
    
    if(!_read){
        return _buffer.insert(filename.toStdString(), NULL, _read, NULL,  NULL);
    }
    if(mode == DO_NOT_DECODE){
        _read->readHeader(filename, false);
        return _buffer.insert(filename.toStdString(), NULL, _read, _read->getReaderInfo(),  NULL);
    }
    
    /*checking whether the reader should open both views or not*/
    bool openStereo = false;
    if(_read->supports_stereo() && mode == STEREO_DECODE) openStereo = true;
    
    /*In case the read handle supports scanlines, we read the header to determine
     how many scan-lines we would need, depending also on the viewer context.*/
    std::string filenameStr = filename.toStdString();
    std::map<int,int> rows;
    /*the slContext is useful to check the equality of 2 scan-line based frames.*/
    Reader::Buffer::ScanLineContext *slContext = 0;
    if(_read->supportsScanLine()){
        slContext = new Reader::Buffer::ScanLineContext;
        _read->readHeader(filename,openStereo);
        if(ctrlPTR->getModel()->getVideoEngine()->isOutputAViewer()){
            float zoomFactor;
            const Format &dispW = _read->getReaderInfo()->getDisplayWindow();
            if(_fitFrameToViewer){
                float h = (float)(dispW.h());
                zoomFactor = (float)currentViewer->getUiContext()->height()/h -0.05;
                currentViewer->getUiContext()->viewer->fitToFormat(dispW);
            }else{
                zoomFactor = currentViewer->getUiContext()->viewer->getZoomFactor();
            }
            
            slContext->setRows(currentViewer->getUiContext()->viewer->computeRowSpan(dispW, zoomFactor));
        }else{
            const Box2D& dataW = _read->getReaderInfo()->getDataWindow();
            std::map<int,int> rows;
            for (int i =dataW.y() ; i < dataW.top(); i++) {
                rows.insert(make_pair(i,i));
            }
            slContext->setRows(rows);
            
        }
    }
    /*Now that we have the slContext we can check whether the frame is already enqueued in the buffer or not.*/
    Reader::Buffer::DecodedFrameIterator found = _buffer.isEnqueued(filenameStr,Buffer::NOT_CACHED_FRAME);
    if(found !=_buffer.end()){
        if(found->_asynchTask && !found->_asynchTask->isFinished()){
            found->_asynchTask->waitForFinished();
        }
        if(!found->_slContext){
            delete _read;
            return *found;
        }else{
            /*we found a buffered frame with a scanline context. We can now compute
             the intersection between the current scan-line context and the one found
             to find out which rows we need to compute*/
            found->_slContext->computeIntersectionAndSetRowsToRead(slContext->getRows());
            if(startNewThread){
                future = new QFuture<void>;
                *future = QtConcurrent::run(found->_readHandle,&Read::readScanLineData,filename,found->_slContext,true,openStereo);
                found->_slContext->merge();
                found->_asynchTask = future;
                delete _read;
                return *found;
            }else{
                found->_readHandle->readScanLineData(filename, found->_slContext,true,openStereo);
                found->_slContext->merge();
                delete _read;
                return *found;
            }
        }
    }else{
        
        /*Now that we're sure that we will decode data, we can initialize the color-space*/
        _read->initializeColorSpace();
        
        /*starting to read data, in a new thread or on the main thread*/
        pair<Reader::Buffer::DecodedFrameIterator,bool> ret;
        if(startNewThread){
            future = new QFuture<void>;
            if(_read->supportsScanLine()){
                *future = QtConcurrent::run(_read,&Read::readScanLineData,filename,slContext,false,openStereo);
            }else{
                *future = QtConcurrent::run(_read,&Read::readData,filename,openStereo);
            }
            return _buffer.insert(filenameStr, future,  _read, _read->getReaderInfo() ,NULL ,  slContext);
        }else{
            if(_read->supportsScanLine()){
                _read->readScanLineData(filename, slContext,false,openStereo);
            }else{
                _read->readData(filename,openStereo);
            }
            return _buffer.insert(filenameStr, NULL,  _read,_read->getReaderInfo(), NULL ,slContext);
        }
    }
    
}

Reader::Buffer::DecodedFrameDescriptor Reader::openCachedFrame(FrameEntry* frame,bool startNewThread){
    Reader::Buffer::DecodedFrameIterator found = _buffer.isEnqueued(frame->_frameInfo->getCurrentFrameName(),
                                                                    Buffer::CACHED_FRAME);
    if(found !=_buffer.end()){
        if(found->_cachedFrame){
            if(found->_asynchTask){
                if(!found->_asynchTask->isFinished())
                    found->_asynchTask->waitForFinished();
            }
            return *found;
        }else{
            /*if the frame is already present in the buffer but was not a cached frame, override it*/
            _buffer.erase(found);
        }
    }
    removeCachedFramesFromBuffer();
    
    ReaderInfo *info = new ReaderInfo;
    *info = *frame->_frameInfo;
    int w = frame->_actualW ;
    int h = frame->_actualH ;
    size_t dataSize = 0;
    if(frame->_byteMode==1.0){
        dataSize  = w * h * sizeof(U32) ;
    }else{
        dataSize  = w * h  * sizeof(float) * 4;
    }
    /*allocating pbo*/
    
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,currentViewer->getUiContext()->viewer->getPBOId(_pboIndex));
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, NULL, GL_DYNAMIC_DRAW_ARB);
    void* output = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    _pboIndex = (_pboIndex+1)%2;
    QFuture<void>* future = 0;
    const char* cachedFrame = frame->getMappedFile()->data();
    if(!startNewThread){
        currentViewer->getUiContext()->viewer->fillPBO(cachedFrame, output, dataSize);
        return _buffer.insert(info->getCurrentFrameName(), 0 , 0, info ,cachedFrame,NULL);
    }else{
        future = new QFuture<void>;
        Reader::Buffer::DecodedFrameDescriptor newFrame =_buffer.insert(info->getCurrentFrameName(),
                                                                        0 ,
                                                                        0,
                                                                        info ,
                                                                        cachedFrame,
                                                                        NULL);
        *future = QtConcurrent::run(this,&Reader::retrieveCachedFrame,cachedFrame,output,dataSize);
        return newFrame;
    }
}

void Reader::retrieveCachedFrame(const char* cachedFrame,void* dst,size_t dataSize){
    currentViewer->getUiContext()->viewer->fillPBO(cachedFrame, dst, dataSize);
}
std::vector<Reader::Buffer::DecodedFrameDescriptor>
Reader::decodeFrames(DecodeMode mode,bool useCurrentThread,bool useOtherThread,bool forward){
    
    std::vector<Reader::Buffer::DecodedFrameDescriptor> out;
    int current_frame;
    Writer* writer = dynamic_cast<Writer*>(ctrlPTR->getModel()->getVideoEngine()->getCurrentDAG().getOutput());
    if(!writer)
        current_frame = clampToRange(currentViewer->currentFrame());
    else
        current_frame = writer->currentFrame();
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

void Reader::showFilePreview(){
    
    _buffer.clear();
    
    getVideoSequenceFromFilesList();
    
    fitFrameToViewer(false);
    
    Buffer::DecodedFrameDescriptor ret = open(files[firstFrame()], DO_NOT_DECODE ,false);
    if(!makeCurrentDecodedFrame(false)){
        cout << "ERROR: Couldn't make current read handle ( " << _name.toStdString() << " )" << endl;
        return;
    }
    _info->firstFrame(firstFrame());
    _info->lastFrame(lastFrame());
    readHandle->make_preview();
    _buffer.clear();
}

void Reader::removeCachedFramesFromBuffer(){
    _buffer.removeAllCachedFrames();
}
void Reader::Buffer::removeAllCachedFrames(){
    bool recursive = false;
    for(DecodedFrameIterator it = _buffer.begin();it != _buffer.end();it++){
        if(it->_cachedFrame){
            erase(it);
            recursive = true;
            break;
        }
    }
    if(recursive)
        removeAllCachedFrames();
}

bool Reader::makeCurrentDecodedFrame(bool forReal){
    int current_frame;
    if(!forReal)
        current_frame = firstFrame();
    else{
        Writer* writer = dynamic_cast<Writer*>(ctrlPTR->getModel()->getVideoEngine()->getCurrentDAG().getOutput());
        if(!writer)
            current_frame = clampToRange(currentViewer->currentFrame());
        else
            current_frame = writer->currentFrame();
    }
    
    QString currentFile = files[current_frame];
    Reader::Buffer::DecodedFrameIterator frame = _buffer.isEnqueued(currentFile.toStdString(),
                                                                    Buffer::ALL_FRAMES);
    if(frame == _buffer.end() || (frame->_asynchTask && !frame->_asynchTask->isFinished())) return false;
    
    Node::Info* infos = 0;
    if(frame->_readInfo && !frame->_readHandle){ // cached frame
        infos = dynamic_cast<Node::Info*>(frame->_readInfo);
    }else{
        readHandle = frame->_readHandle;
        infos = dynamic_cast<Node::Info*>(readHandle->getReaderInfo());
        assert(infos);
        *_info = *infos;
    }
    assert(infos);
    *_info = *infos;
    
    return true;
}

void Reader::_validate(bool forReal){
    if(forReal && !makeCurrentDecodedFrame(true)){
        cout << "ERROR: Couldn't make current read handle ( " << _name.toStdString() << " )" << endl;
        return;
    }
    
    _info->firstFrame(firstFrame());
    _info->lastFrame(lastFrame());
}

void Reader::engine(int y,int offset,int range,ChannelMask c,Row* out){
	readHandle->engine(y,offset,range,c,out);
	
}

void Reader::createKnobDynamically(){
	Node::createKnobDynamically();
}


Reader::Buffer::DecodedFrameDescriptor Reader::Buffer::insert(std::string filename,
                                                              QFuture<void> *future,
                                                              Read* readHandle,
                                                              ReaderInfo* readInfo,
                                                              const char* cachedFrame,
                                                              ScanLineContext* slContext){
    //if buffer is full, we remove previously computed frame
    if(_buffer.size() == _bufferSize){
        for(U32 i = 0 ; i < _buffer.size() ;i++){
            DecodedFrameDescriptor frameToRemove = _buffer[i];
            if((!frameToRemove._asynchTask) ||
               (frameToRemove._asynchTask && frameToRemove._asynchTask->isFinished())){
                erase(_buffer.begin()+i);
                break;
            }
        }
    }
    DecodedFrameDescriptor desc(future,readHandle,readInfo,cachedFrame,filename,slContext);
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
        if(it->_readInfo)
            delete it->_readInfo; // delete readerInfo
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
    return (it!=_buffer.end() && ((it->_asynchTask && it->_asynchTask->isFinished()) || !it->_asynchTask));
}
void Reader::Buffer::debugBuffer(){
    cout << "=========BUFFER DUMP=============" << endl;
    for(DecodedFrameIterator it = _buffer.begin(); it != _buffer.end() ; it++){
        if(it->_cachedFrame)
            cout << it->_filename << " (cached) " << endl;
        else
            cout << it->_filename << endl;
    }
    cout << "=================================" << endl;
}

Reader::Buffer::DecodedFrameIterator Reader::Buffer::isEnqueued(std::string filename,SEARCH_TYPE searchMode){
    if(searchMode == CACHED_FRAME){
        DecodedFrameIterator ret = find(filename);
        if(ret != _buffer.end()){
            if(ret->_cachedFrame){
                return ret;
            }else{
                return _buffer.end();
            }
        }else{
            return _buffer.end();
        }
    }else if(searchMode == SCANLINE_FRAME){
        DecodedFrameIterator ret = find(filename);
        if(ret != _buffer.end()){
            if(!ret->_readHandle->supportsScanLine()){
                return _buffer.end();
            }
            if(!ret->_cachedFrame){
                return ret;
            }else{
                return _buffer.end();
            }
        }else{
            return _buffer.end();
        }
    }else if(searchMode == FULL_FRAME){
        DecodedFrameIterator ret = find(filename);
        if(ret != _buffer.end()){
            if(ret->_readHandle->supportsScanLine()){
                return _buffer.end();
            }
            if(!ret->_cachedFrame){
                return ret;
            }else{
                return _buffer.end();
            }
        }else{
            return _buffer.end();
        }
    }else{ // all frames
        return find(filename);
    }
}

void Reader::Buffer::clear(){
    DecodedFrameIterator it = _buffer.begin();
    for(;it!=_buffer.end();it++){
        if(it->_asynchTask)
            delete it->_asynchTask;//delete future
        if(it->_readInfo)
            delete it->_readInfo; // delete readerInfo
        if(it->_readHandle)
            delete it->_readHandle; // delete readHandle
    }
    _buffer.clear();
}
void Reader::Buffer::erase(DecodedFrameIterator it){
    if(it->_asynchTask)
        delete it->_asynchTask;//delete future
    if(it->_readInfo)
        delete it->_readInfo; // delete readerInfo
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
    
}
int Reader::firstFrame(){
    std::map<int,QString>::iterator it=files.begin();
    if(it == files.end()) return INT_MIN;
    return it->first;
}
int Reader::lastFrame(){
    std::map<int,QString>::iterator it=files.end();
    if(it == files.begin()) return INT_MAX;
    it--;
    return it->first;
}
int Reader::clampToRange(int f){
    int first = firstFrame();
    int last = lastFrame();
    if(f < first) return first;
    if(f > last) return last;
    return f;
}

std::string Reader::getRandomFrameName(int f){
    return files[f].toStdString();
}

void Reader::setPreview(QImage* img){
    if(preview)
        delete preview;
    preview=img;
    hasPreview(true);
    getNodeUi()->updatePreviewImageForReader();
}


/*Adds to _rowsToRead the rows in others that are missing to _rows*/
void Reader::Buffer::ScanLineContext::computeIntersectionAndSetRowsToRead(std::map<int,int>& others){
    ScanLineIterator it = others.begin();
    std::map<int,int> rowsCopy = _rows;
    for(;it!=others.end();it++){
        ScanLineIterator found = rowsCopy.find(it->first);
        if(found == rowsCopy.end()){ // if not found, we add the row to rows
            _rowsToRead.push_back(it->first);
        }else{
            rowsCopy.erase(found); // otherwise , we erase the row from the copy to speed up the computation of the intersection
        }
    }
}

/*merges _rowsToRead and _rows*/
void Reader::Buffer::ScanLineContext::merge(){
    for(U32 i = 0;i < _rowsToRead.size(); i++){
        _rows.insert(make_pair(_rowsToRead[i],0));
    }
    _rowsToRead.clear();
}


//int _firstFrame;
//int _lastFrame;
//int _ydirection;
//bool _blackOutside;
//bool _rgbMode;
//Format _displayWindow; // display window of the data, for the data window see x,y,range,offset parameters
//ChannelMask _channels;
std::string ReaderInfo::printOut(){
    const Format &dispW = getDisplayWindow();
    const ChannelSet& chan = channels();
    ostringstream oss;
    oss << _currentFrameName <<  "<" << firstFrame() << "."
    << lastFrame() << "."
    << rgbMode() << "."
    << dispW.x() << "."
    << dispW.y() << "."
    << dispW.right() << "."
    << dispW.top() << "."
    << x() << "."
    << y() << "."
    << right() << "."
    << top() << ".";
    foreachChannels(z, chan){
        oss << getChannelName(z) << "|";
    }
    oss << " " ;
    return oss.str();
}

ReaderInfo* ReaderInfo::fromString(QString from){
    ReaderInfo* out = new ReaderInfo;
    QString name;
    QString firstFrameStr,lastFrameStr,rgbStr,frmtXStr,frmtYStr,frmtRStr,frmtTStr;
    QString bboxXStr,bboxYStr,bboxRStr,bboxTStr,channelsStr;
    
    int i = 0;
    while(from.at(i) != QChar('<')){name.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){firstFrameStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){lastFrameStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){rgbStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){frmtXStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){frmtYStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){frmtRStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){frmtTStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){bboxXStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){bboxYStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){bboxRStr.append(from.at(i)); i++;}
    i++;
    while(from.at(i) != QChar('.')){bboxTStr.append(from.at(i)); i++;}
    i++;
    while(i < from.size()){channelsStr.append(from.at(i)); i++;}
    i++;
    ChannelSet channels;
    i = 0;
    while(i < channelsStr.size()){
        QString chan;
        while(channelsStr.at(i) != QChar('|')){
            chan.append(channelsStr.at(i));
            i++;
        }
        i++;
        string chanStd = chan.toStdString();
        channels += getChannelByName(chanStd.c_str());
    }
    Format dispW(frmtXStr.toInt(),frmtYStr.toInt(),frmtRStr.toInt(),frmtTStr.toInt(),"");
    out->set(bboxXStr.toInt(),bboxYStr.toInt(),bboxRStr.toInt(),bboxTStr.toInt());
    out->setChannels(channels);
    out->rgbMode((bool)rgbStr.toInt());
    out->setDisplayWindow(dispW);
    out->firstFrame(firstFrameStr.toInt());
    out->lastFrame(lastFrameStr.toInt());
    return out;
    
}