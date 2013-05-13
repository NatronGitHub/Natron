//
//  VideoEngine.cpp
//  PowiterOsX
//
//  Created by Alexandre on 1/16/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//
#include <QtCore/QMutex>
#include <QtCore/qcoreapplication.h>
#include <QtGui/QPushButton>
#include <QtGui/QVector2D>
#include <iterator>
#include <cassert>
#include "Core/VideoEngine.h"
#include "Core/inputnode.h"
#include "Core/outputnode.h"
#include "Core/workerthread.h"
#include "Core/mappedfile.h"
#include "Core/viewerNode.h"
#include "Core/settings.h"
#include "Core/model.h"
#include "Core/hash.h"
#include "Core/Timer.h"
#include "Core/mappedfile.h"

#include "Gui/mainGui.h"
#include "Gui/viewerTab.h"
#include "Gui/timeline.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/GLViewer.h"

#include "Reader/Reader.h"
#include "Reader/Reader.h"
#include "Reader/readerInfo.h"

#include "Superviser/controler.h"
#include "Superviser/MemoryInfo.h"
#include "Superviser/PwStrings.h"


using Powiter_Enums::ROW_RANK;

/* Here's a drawing that represents how the video Engine works: 
                                            [thread pool]                       [OtherThread]
   [main thread]                                |                                   |
    -videoEngine(...)                           |                                   |
      >-computeFrameRequest(...)                |                                   |
     |    if(!cached)                           |                                   |
     |       -computeTreeForFrame------------>start worker threads                  |
     |          waitForDone();                  |                                   |
     |              |             <--notify----finished()                           |
     |       -allocatePBO                                                           |
     |       -fillPBO -------------QtConcurrent::Run-------------------------> copy frame data to PBO...
     |           |                                                                  |
     |       -engineLoop <-----------------------------------------------notify---finished()
     |           |
     |           |
     ------------...
     |     else
     |        -cachedFrameEngine(...)
     |          -retrieveFrame(...)
     |          -allocatePBO
     |          -fillPBO -------------QtConcurrent::Run-------------------------> copy frame data to PBO...
     |              |                                                               |
     ----------engineLoop <---------------------------------------------notify---finished()

 
    Might improve the engine to remove the waitForDone() after computeTreeForFrame with a notification 
    system instead as for other QtConcurrent calls. The improvment will be done using QtConcurrent::map.
 */

void VideoEngine::videoEngine(int frameCount,bool fitFrameToViewer,bool forward,bool sameFrame){
    if (_working || !_enginePostProcessResults->isFinished()) {
        return;
    }
    _frameRequestsCount = frameCount;
    _frameRequestIndex = 0;
    _forward = forward;
    _paused = false;
    _aborted = false;
    computeFrameRequest(sameFrame,forward,fitFrameToViewer);
}
void VideoEngine::stopEngine(){
    _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
    _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
    _frameRequestsCount = 0;
    _working = false;
    _aborted = false;
    _paused = false;
    _enginePostProcessResults->waitForFinished();
    _timer->playState=PAUSE;
    
}

void VideoEngine::computeFrameRequest(bool sameFrame,bool forward,bool fitFrameToViewer){
    _working = true;
    if(!sameFrame && _aborted){
        stopEngine();
        return;
    }else if(_paused || _frameRequestsCount==0){
        stopEngine();
        runTasks();
        return;
    }
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
    int firstFrame = frameSeeker->firstFrame();
    int lastFrame = frameSeeker->lastFrame();
    int currentFrame = gl_viewer->getCurrentReaderInfo()->currentFrame();
    
    /*if there's only 1 frame and we already computed it we stop*/
    if(lastFrame == firstFrame && _frameRequestsCount==-1 && _frameRequestIndex==1){
        stopEngine();
        runTasks();
        return;
    }
    
    if(!sameFrame){
        clearRowCache();
        /*clamping the current frame to the range [first,last] if it wasn't*/
       
        if(currentFrame < firstFrame){
            currentFrame = firstFrame;
        }
        else if(currentFrame > lastFrame){
            currentFrame = lastFrame;
        }
        for(int j=0;j<inputs.size();j++){
            InputNode* currentInput=inputs[j];
            if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->randomFrame(currentFrame);
                inp->setup_for_next_frame();
            }
        }
    }
    _timer->playState=RUNNING;
    // if inputs[0] is a reader we make its reader info the current ones
	if(!strcmp(inputs[0]->class_name(),"Reader")){
		makeReaderInfoCurrent(static_cast<Reader*>(inputs[0]));
	}
    
    char* currentFrameName=0;
    if(strcmp(inputs[0]->class_name(),"Reader")==0){
        currentFrameName = static_cast<Reader*>(inputs[0])->getCurrentFrameName();
    }
    
    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
    
    if(fitFrameToViewer){
        gl_viewer->initViewer();
    }
    if (!sameFrame) {
        float zoomFactor =  closestBuiltinZoom(gl_viewer->getZoomFactor());
        gl_viewer->setCurrentBuiltInZoom(zoomFactor);

    }
    // check the presence of the frame in the cache
    pair<FramesIterator,bool> iscached = _cache->isCached(currentFrameName,_treeVersion.getHashValue(),gl_viewer->currentBuiltinZoom(),
                                                          gl_viewer->getExposure(),gl_viewer->lutType(),gl_viewer->_byteMode);
    bool initTexture = true;
    if(sameFrame) initTexture = false;
    
    if(!iscached.second){
        int frameCountHint = 0;
        if(_frameRequestsCount==-1){
            if(_loopMode){
                frameCountHint = gl_viewer->getCurrentReaderInfo()->lastFrame()-gl_viewer->getCurrentReaderInfo()->firstFrame()+1;
            }else{
                frameCountHint = gl_viewer->getCurrentReaderInfo()->lastFrame()-gl_viewer->getCurrentReaderInfo()->currentFrame();
            }
        }else{
            frameCountHint = _frameRequestsCount+_frameRequestIndex;
        }
        computeTreeForFrame(output,inputs[0],ceil((double)frameCountHint/10.0),initTexture);
        
        int w = floorf(gl_viewer->displayWindow().w() * gl_viewer->currentBuiltinZoom());
        int h = floorf(gl_viewer->displayWindow().h() * gl_viewer->currentBuiltinZoom());
        
        std::pair<void*,size_t> pbo = gl_viewer->allocatePBO(w, h);
        *_enginePostProcessResults = QtConcurrent::run(gl_viewer,&ViewerGL::fillComputePBO,pbo.first,pbo.second);
        _engineLoopWatcher->setFuture(*_enginePostProcessResults);
        
    }else{
        cachedFrameEngine(iscached.first);
    }
    if(_frameRequestIndex == 0 && _frameRequestsCount == 1 && !sameFrame){
        _frameRequestsCount = 0;
        _frameRequestIndex = 1;
        return;
    }
    if(!sameFrame){
        if(forward){
            currentFrame = gl_viewer->getCurrentReaderInfo()->currentFrame()+1;
            if(currentFrame > lastFrame){
                if(_loopMode)
                    currentFrame = firstFrame;
                else{
                    _frameRequestsCount = 0;
                    return;
                }
            }
        }else{
            currentFrame = gl_viewer->getCurrentReaderInfo()->currentFrame()-1;
            if(currentFrame < firstFrame){
                if(_loopMode)
                    currentFrame = lastFrame;
                else{
                    _frameRequestsCount = 0;
                    return;
                }
            }
        }
        for(int j=0;j<inputs.size();j++){
            InputNode* currentInput=inputs[j];
            if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->randomFrame(currentFrame);
                inp->setup_for_next_frame();
            }
        }
       
    }
    if(_frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
        _frameRequestsCount--;
    }
    _frameRequestIndex++;
}

void VideoEngine::cachedFrameEngine(FramesIterator frame){
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
    int firstFrame = frameSeeker->firstFrame();
    int lastFrame = frameSeeker->lastFrame();
    int currentFrame = gl_viewer->getCurrentReaderInfo()->currentFrame();
    std::pair<char*,pair<int,int> > cachedFrame = make_pair((char*)0,make_pair(0,0));
    cachedFrame =  _cache->retrieveFrame(_frameRequestsCount==1 && firstFrame == lastFrame ? 0 : currentFrame,frame);
    gl_viewer->doEmitFrameChanged(_frameRequestsCount == 1 && firstFrame == lastFrame ? 0 : currentFrame);
    
    size_t dataSize = 0;
    if(frame->second._byteMode==1.0){
        dataSize  = frame->second._nbRows * frame->second._rowWidth*sizeof(U32) ;
    }else{
        dataSize  = frame->second._nbRows * frame->second._rowWidth*sizeof(float)*4;
    }
    /*resizing texture if needed*/
    gl_viewer->initTexturesRgb(cachedFrame.second.first, cachedFrame.second.second);
    checkGLErrors();
    gl_viewer->makeCurrent();
    gl_viewer->drawing(true);
    /*allocating pbo*/
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,gl_viewer->texBuffer[_pboIndex]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, NULL, GL_DYNAMIC_DRAW_ARB);
    checkGLErrors();
    _pboIndex = (_pboIndex+1)%2;

    void* output = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    checkGLErrors();
    *_enginePostProcessResults = QtConcurrent::run(gl_viewer,&ViewerGL::fillCachedPBO,cachedFrame.first,output,dataSize);
    _engineLoopWatcher->setFuture(*_enginePostProcessResults);

}

void VideoEngine::engineLoop(){
   
    std::pair<int,int> texSize = gl_viewer->getTextureSize();
    gl_viewer->copyPBOtoTexture(texSize.first, texSize.second);
    _timer->waitUntilNextFrameIsDue();
    if((_frameRequestIndex%24)==0){
        emit fpsChanged(_timer->actualFrameRate());
    }
    updateDisplay();
    computeFrameRequest(false, _forward ,false);
}


void VideoEngine::computeTreeForFrame(OutputNode *output,InputNode* input,int followingComputationsNb,bool initTexture){
    IntegerBox renderBox = gl_viewer->displayWindow(); // the display window held by the data
    
	int y=renderBox.y(); // bottom
    
    // REQUEST THE CHANNELS CONTAINED IN THE VIEWER TAB COMBOBOX IN THE GUI !!
    output->request(y,renderBox.top(),0,renderBox.right(),gl_viewer->displayChannels());
    // AT THIS POINT EVERY OPERATOR HAS ITS INFO SET!! AS WELL AS REQUESTED_BOX AND REQUESTED_CHANNELS
	if(gl_viewer->Ydirection()<0){ // Ydirection < 0 means we cycle from top to bottom
        y=renderBox.top()-1;
	}
	else{ // cycling from bottom to top
        y=renderBox.y();
	}
    
    //outChannels are the intersection between what the viewer requests and the ones available in the viewer node
    ChannelSet outChannels = gl_viewer->displayChannels() & output->getInfo()->channels();
    
    /*below are stuff to optimize zoom caching*/
    float builtinZoom = gl_viewer->currentBuiltinZoom();
    int zoomedHeight = floor((float)gl_viewer->displayWindow().h()*builtinZoom);
    float incrementNew = builtInZooms[builtinZoom].first;
    float incrementFullsize = builtInZooms[builtinZoom].second;
    gl_viewer->setZoomIncrement(make_pair(incrementNew,incrementFullsize));
    
    if(initTexture)
        gl_viewer->initTextures();
    
#ifdef __POWITER_WIN32__
	gl_viewer->doneCurrent(); // windows only, openGL needs this to deal with concurrency
#endif
    
    // selecting the right anchor of the row
    int right = 0;
    gl_viewer->dataWindow().right() > gl_viewer->displayWindow().right() ?
    right = gl_viewer->dataWindow().right() : right = gl_viewer->displayWindow().right();
    
    // selection the left anchor of the row
	int offset=0;
    gl_viewer->dataWindow().x() < gl_viewer->displayWindow().x() ?
    offset = gl_viewer->dataWindow().x() : offset = gl_viewer->displayWindow().x();
    
    //starting viewer preprocess : i.e initialize the cached frame
    gl_viewer->preProcess(followingComputationsNb);
    
	if(gl_viewer->Ydirection()<0){
        int rowY = zoomedHeight -1;
		while (y>=renderBox.y()){
            int k =y;
            while(k > (y -incrementNew) && rowY>=0 && k>=(renderBox.y())){
                if(_aborted){
                    _abort();
                    return;
                }else if(_paused){
                    pool->waitForDone();
                    return;
                }
                
                bool cached = false;
                Row* row ;
                map<int,Row*>::iterator foundCached = isRowContainedInCache(k);
                if(foundCached!=row_cache.end()){
                    row= foundCached->second;
                    cached = true;
                }else{
                    row=new Row(offset,k,right,outChannels);
                    addToRowCache(row);
                }
                row->zoomedY(rowY);
                WorkerThread* task;
                if((k==renderBox.y() || rowY==0) && !strcmp(output->class_name(),"Viewer")){
                    task =new WorkerThread(row,input,output,LAST,cached);
                }else if(k==(renderBox.top()-1) && !strcmp(output->class_name(),"Viewer")){
                    task =new WorkerThread(row,input,output,FIRST,cached);
                }else{
                    task =new WorkerThread(row,input,output,ND,cached);
                }
                rowY--;
                k--;
                pool->start(task);
                
            }
            y-=incrementFullsize;
        }
        
    }else{
        int rowY = renderBox.y(); // zoomed index
        while(y<renderBox.top()){ // y = full size index
            for(int k = y; k<incrementNew+y;k++){
                bool cached = false;
                Row* row ;
                if(_aborted){
                    _abort();
                    return;
                }else if(_paused){
                    return;
                }
                map<int,Row*>::iterator foundCached = isRowContainedInCache(k);
                if(foundCached!=row_cache.end()){
                    row= foundCached->second;
                    cached = true;
                }else{
                    row=new Row(offset,k,right,outChannels);
                    addToRowCache(row);
                }
                row->zoomedY(rowY);
                WorkerThread* task;
                if(k==(renderBox.top()-1) && !strcmp(output->class_name(),"Viewer")){
                    task =new WorkerThread(row,input,output,LAST,cached);
                }else if(k==renderBox.y() && !strcmp(output->class_name(),"Viewer")){
                    task =new WorkerThread(row,input,output,FIRST,cached);
                }
                else{
                    task =new WorkerThread(row,input,output,ND,cached);
                }
                pool->start(task);
                
                rowY++;
            }
            y+=incrementFullsize;
        }
        
        
    }
    pool->waitForDone();
    
}
void VideoEngine::drawOverlay(){
    if(output)
        _drawOverlay(output);
}

void VideoEngine::_drawOverlay(Node *output){
    output->drawOverlay();
    foreach(Node* n,output->getParents()){
        _drawOverlay(n);
    }
    
}


void VideoEngine::updateProgressBar(){
    //update progress bar
}
void VideoEngine::updateDisplay(){
    gl_viewer->resizeEvent(new QResizeEvent(gl_viewer->size(),gl_viewer->size()));
    gl_viewer->updateGL();
}

void VideoEngine::startEngine(int nbFrames){
    videoEngine(nbFrames,true,true);
}

VideoEngine::VideoEngine(ViewerGL *gl_viewer,Model* engine,QMutex* lock):
QObject(engine),_working(false),_aborted(false),_paused(true),_readerInfoHasChanged(false),
_forward(true),_frameRequestsCount(0),_pboIndex(0),_frameRequestIndex(0),_loopMode(true){
    
    _engineLoopWatcher = new QFutureWatcher<void>;
    _enginePostProcessResults = new QFuture<void>;
    connect(_engineLoopWatcher, SIGNAL(finished()), this, SLOT(engineLoop()));
    this->_coreEngine = engine;
    this->_lock= lock;
    this->gl_viewer = gl_viewer;
    gl_viewer->setVideoEngine(this);
    _timer=new Timer();
    pool = new QThreadPool(this);
    (QThread::idealThreadCount()) > 1 ? pool->setMaxThreadCount((QThread::idealThreadCount())) : pool->setMaxThreadCount(1);
    fillBuilInZoomsLut();
    output = 0;
    foreach(InputNode* i,inputs){ i = 0; }
    FeedBackSpinBox* fpsBox = engine->getControler()->getGui()->viewer_tab->fpsBox;
    TimeSlider* frameSeeker = engine->getControler()->getGui()->viewer_tab->frameSeeker;
    FeedBackSpinBox* frameNumber = engine->getControler()->getGui()->viewer_tab->frameNumberBox;
    QObject::connect(fpsBox, SIGNAL(valueChanged(double)),this, SLOT(setDesiredFPS(double)));
    QObject::connect(this, SIGNAL(fpsChanged(double)), fpsBox, SLOT(setValue(double)));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->play_Forward_Button,SIGNAL(toggled(bool)),this,SLOT(startPause(bool)));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->stop_Button,SIGNAL(clicked()),this,SLOT(pause()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->play_Backward_Button,SIGNAL(toggled(bool)),this,SLOT(startBackward(bool)));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->previousFrame_Button,SIGNAL(clicked()),this,SLOT(previousFrame()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->nextFrame_Button,SIGNAL(clicked()),this,SLOT(nextFrame()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->previousIncrement_Button,SIGNAL(clicked()),this,SLOT(previousIncrement()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->nextIncrement_Button,SIGNAL(clicked()),this,SLOT(nextIncrement()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->firstFrame_Button,SIGNAL(clicked()),this,SLOT(firstFrame()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->lastFrame_Button,SIGNAL(clicked()),this,SLOT(lastFrame()));
    QObject::connect(frameNumber,SIGNAL(valueChanged(double)),this,SLOT(seekRandomFrame(double)));
    QObject::connect(frameSeeker,SIGNAL(positionChanged(int)), this, SLOT(seekRandomFrame(int)));
    QObject::connect(frameSeeker,SIGNAL(positionChanged(int)), frameNumber, SLOT(setValue(int)));
    QObject::connect(frameNumber, SIGNAL(valueChanged(double)), frameSeeker, SLOT(seek(double)));
    qint64 maxDiskCacheSize = engine->getCurrentPowiterSettings()->maxDiskCache;
    qint64 maxMemoryCacheSize = (double)getSystemTotalRAM() * engine->getCurrentPowiterSettings()->maxCacheMemoryPercent;
    _cache = new DiskCache(gl_viewer,maxDiskCacheSize, maxMemoryCacheSize);
    QObject::connect(engine->getControler()->getGui()->actionClearDiskCache, SIGNAL(triggered()),this,SLOT(clearDiskCache()));
    QObject::connect(engine->getControler()->getGui()->actionClearPlayBackCache, SIGNAL(triggered()),this,SLOT(clearPlayBackCache()));
    QObject::connect(engine->getControler()->getGui()->actionClearBufferCache, SIGNAL(triggered()),this,SLOT(clearRowCache()));
    
}

void VideoEngine::clearDiskCache(){
    _cache->clearCache();
}

VideoEngine::~VideoEngine(){
    clearRowCache();
    _enginePostProcessResults->waitForFinished();
    delete _engineLoopWatcher;
    delete _enginePostProcessResults;
    delete _cache;
    delete pool;
    delete _timer;
}

void VideoEngine::fillBuilInZoomsLut(){
    builtInZooms[1.f/10.f] = make_pair(1,10);
    builtInZooms[1.f/4.f] = make_pair(1,4);
    builtInZooms[1.f/2.f] = make_pair(1,2);
    builtInZooms[3.f/4.f] = make_pair(3,4);
    builtInZooms[9.f/10.f]= make_pair(9,10);
    builtInZooms[1.f] = make_pair(1,1);
}
float VideoEngine::closestBuiltinZoom(float v){
    std::map<float, pair<int,int> >::iterator it = builtInZooms.begin();
    std::map<float, pair<int,int> >::iterator suiv = it;
    ++suiv;
    if( v < it->first)
        return it->first;
    for(;it!=builtInZooms.end();it++){
        if(it->first == v)
            return it->first;
        else if(suiv!=builtInZooms.end() && it->first <v && suiv->first >= v)
            return suiv->first;
        else if(suiv==builtInZooms.end() && it->first < v  )
            return 1.f;
        suiv++;
    }
    return -1.f;
}
float VideoEngine::inferiorBuiltinZoom(float v){
    map<float, pair<int,int> >::iterator it=builtInZooms.begin(); it++;
    map<float, pair<int,int> >::iterator prec =builtInZooms.begin();
    for(;it!=builtInZooms.end();it++){
        float value = it->first;
        if( value == v &&  value != 1.f/20.f)
            return prec->first;
        else if(value==v && value==1.f/20.f)
            return value;
        prec++;
    }
    return -1.f;
}
float VideoEngine::superiorBuiltinZoom(float v){
    map<float, pair<int,int> >::iterator suiv=builtInZooms.begin(); suiv++;
    map<float, pair<int,int> >::iterator it =builtInZooms.begin();
    for(;it!=builtInZooms.end();it++){
        float value = it->first;
        if( value == v &&  value != 19.f/20.f)
            return suiv->first;
        else if(value==v && value==19.f/20.f)
            return 1.f;
        suiv++;
    }
    return -1.f;
}

void VideoEngine::addToRowCache(Row* row){
    QMutexLocker lock(_lock);
	row_cache[row->y()]=row;
}

std::map<int,Row*>::iterator VideoEngine::isRowContainedInCache(int rowIndex){
    QMutexLocker lock(_lock);
	return row_cache.find(rowIndex);
}

void VideoEngine::clearInfos(Node* out){
    out->clear_info();
    foreach(Node* c,out->getParents()){
        clearInfos(c);
    }
}

void VideoEngine::pushReaderInfo(ReaderInfo* info,Reader* reader){
    std::map<Reader*, ReaderInfo* >::iterator it = readersInfos.find(reader);
    if(it!=readersInfos.end()){
        if(*(it->second) == *info){
            it->second->currentFrame(info->currentFrame());
            it->second->currentFrameName(info->currentFrameName());
            delete info;
            return;
        }
        it->second->copy(info);
        //        it->second = info;
    }else{
        readersInfos.insert(make_pair(reader ,info));
    }
    _readerInfoHasChanged = true;
}
void VideoEngine::popReaderInfo(Reader* reader){
    std::map<Reader*, ReaderInfo* >::iterator it = readersInfos.find(reader);
    if(it!=readersInfos.end()){
        delete it->second;
        readersInfos.erase(it);
    }
}
void VideoEngine::makeReaderInfoCurrent(Reader* reader){
    std::map<Reader*, ReaderInfo* >::iterator it = readersInfos.find(reader);
    if(it != readersInfos.end() && it->second)
    {
        gl_viewer->setCurrentReaderInfo(it->second,false,_readerInfoHasChanged);
        _readerInfoHasChanged = false;
    }
    
}
void VideoEngine::setDesiredFPS(double d){
    _timer->setDesiredFrameRate(d);
}

void VideoEngine::_abort(){
    pool->waitForDone(); // stopping worker threads
    clearRowCache(); // clearing row cache
    // QCoreApplication::processEvents(); // load events that may have been ignored
}
void VideoEngine::abort(){
    _aborted=true;
    _timer->playState=PAUSE; // pausing timer
    _working=false; // engine idling
}
void VideoEngine::pause(){
    _paused=true;
    _timer->playState=PAUSE; // pausing timer
    _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
    _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
}
void VideoEngine::startPause(bool c){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
        return;
    }
    
    
    if(c && output && inputs.size()>0){
        videoEngine(-1,false,true);
    }
    else if(!output || inputs.size()==0){
        _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
        _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::startBackward(bool c){
    
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()){
        pause();
        return;
    }
    if(c && output && inputs.size()>0){
        videoEngine(-1,false,false);
    }else if(!output || inputs.size()==0){
        _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
        _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::previousFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _previousFrame(gl_viewer->getCurrentReaderInfo()->currentFrame()-1, 1, false);
    else
        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()-1, 1, false,&VideoEngine::_previousFrame);
}

void VideoEngine::nextFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _nextFrame(gl_viewer->getCurrentReaderInfo()->currentFrame()+1, 1, false);
    else
        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()+1, 1, false,&VideoEngine::_nextFrame);
}

void VideoEngine::firstFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
    if(!_working)
        _firstFrame(frameSeeker->firstFrame(), 1, false);
    else
        appendTask(frameSeeker->firstFrame(), 1, false, &VideoEngine::_firstFrame);
}

void VideoEngine::lastFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
    if(!_working)
        _lastFrame(frameSeeker->lastFrame(), 1, false);
    else
        appendTask(frameSeeker->lastFrame(), 1, false, &VideoEngine::_lastFrame);
}

void VideoEngine::previousIncrement(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = gl_viewer->getCurrentReaderInfo()->currentFrame()-_coreEngine->getControler()->getGui()->viewer_tab->incrementSpinBox->value();
    if(!_working){
        _previousIncrement(frame, 1, false);
    }else{
        appendTask(frame,1, false, &VideoEngine::_previousIncrement);
    }
    
    
}

void VideoEngine::nextIncrement(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = gl_viewer->getCurrentReaderInfo()->currentFrame()+_coreEngine->getControler()->getGui()->viewer_tab->incrementSpinBox->value();
    if(!_working)
        _nextIncrement(frame, 1, false);
    else
        appendTask(frame,1, false, &VideoEngine::_nextIncrement);
}

void VideoEngine::seekRandomFrame(int f){
    if(!output || inputs.size()==0) return;
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _seekRandomFrame(f, 1, false);
    else
        appendTask(f, 1, false, &VideoEngine::_seekRandomFrame);
}

void VideoEngine::_previousFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB >= frameSeeker->firstFrame()){
            gl_viewer->currentFrame(frameNB);
            for(int j=0;j<inputs.size();j++){
                InputNode* currentInput=inputs[j];
                if(strcmp(currentInput->class_name(),"Reader")==0){
                    Reader* inp = static_cast<Reader*>(currentInput);
                    int f =inp->currentFrame();
                    inp->decrementCurrentFrameIndex();
                    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                    if(inp->currentFrame()!=f){ // if it has successfully decremented, i.e: we weren't on the first frame
                        static_cast<Reader*>(currentInput)->setup_for_next_frame();
                    }
                }
            }
            videoEngine(frameCount,initViewer,false);
        }
        
    }
    
}
void VideoEngine::_nextFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB <= frameSeeker->lastFrame()){
            gl_viewer->currentFrame(frameNB);
            for(int j=0;j<inputs.size();j++){
                InputNode* currentInput=inputs[j];
                if(strcmp(currentInput->class_name(),"Reader")==0){
                    Reader* inp = static_cast<Reader*>(currentInput);
                    int f =inp->currentFrame();
                    inp->incrementCurrentFrameIndex();
                    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                    if(inp->currentFrame()!=f){ // if it has successfully incremented, i.e: we weren't on the last frame
                        static_cast<Reader*>(currentInput)->setup_for_next_frame();
                    }
                }
            }
            videoEngine(frameCount,initViewer,true);
        }
    }
    
}
void VideoEngine::_previousIncrement(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB > frameSeeker->firstFrame())
            gl_viewer->currentFrame(frameNB);
        else
            gl_viewer->currentFrame(frameSeeker->firstFrame());
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                if(frameNB < frameSeeker->firstFrame()){
                    inp->randomFrame(frameNB);
                }else{
                    inp->randomFrame(frameSeeker->firstFrame());
                }
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                inp->setup_for_next_frame();
			}
		}
        videoEngine(frameCount,initViewer,false);
    }
    
}
void VideoEngine::_nextIncrement(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB < frameSeeker->lastFrame())
            gl_viewer->currentFrame(frameNB);
        else
            gl_viewer->currentFrame(frameSeeker->lastFrame());
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                if(frameNB < frameSeeker->lastFrame()){
                    inp->randomFrame(frameNB);
                }else{
                    inp->randomFrame(frameSeeker->lastFrame());
                }
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                inp->setup_for_next_frame();
			}
		}
        videoEngine(frameCount,initViewer,true);
    }
    
}
void VideoEngine::_firstFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        gl_viewer->currentFrame(frameNB);
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->randomFrame(frameNB);
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                static_cast<Reader*>(currentInput)->setup_for_next_frame();
			}
		}
        videoEngine(frameCount,initViewer,false);
    }
    
}
void VideoEngine::_lastFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        gl_viewer->currentFrame(frameNB);
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
				inp->randomFrame(frameNB);
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                static_cast<Reader*>(currentInput)->setup_for_next_frame();
                
			}
		}
        videoEngine(frameCount,initViewer,true);
    }
    
}
void VideoEngine::_seekRandomFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB < frameSeeker->firstFrame() || frameNB > frameSeeker->lastFrame())
            return;
        gl_viewer->currentFrame(frameNB);
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->randomFrame(frameNB);
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                inp->setup_for_next_frame();
            }
		}
        videoEngine(frameCount,initViewer,true);
        
    }
    
}
void VideoEngine::appendTask(int frameNB, int frameCount, bool initViewer, VengineFunction func){
    _waitingTasks.push_back(Task(frameNB,frameCount,initViewer,func));
}

void VideoEngine::runTasks(){
    for(unsigned int i=0; i < _waitingTasks.size(); i++){
        Task _t = _waitingTasks[i];
        VengineFunction f = _t._func;
        VideoEngine *vengine = this;
        (*vengine.*f)(_t._newFrameNB,_t._frameCount,_t._initViewer);
    }
    _waitingTasks.clear();
}


/*code needs to be reviewed*/
void VideoEngine::seekRandomFrameWithStart(int f){
    if(!output || inputs.size()==0) return;
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(f < frameSeeker->firstFrame() || f > frameSeeker->lastFrame())
            return;
        gl_viewer->currentFrame(f);
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                int o =inp->currentFrame();
                if(o < f){
                    while(o <f){
                        inp->incrementCurrentFrameIndex();
                        o++;
                    }
                }else{
                    while(o >f){
                        inp->decrementCurrentFrameIndex();
                        o--;
                    }
                }
                static_cast<Reader*>(currentInput)->setup_for_next_frame();
                
            }
		}
        videoEngine(-1,false,true);
        //videoSequenceEngine(output, inputs, -1,false);
    }
}

void VideoEngine::debugTree(){
    int nb=0;
    _debugTree(output,&nb);
    cout << "The tree contains " << nb << " nodes. " << endl;
}
void VideoEngine::_debugTree(Node* n,int* nb){
    *nb = *nb+1;
    cout << *n << endl;
    foreach(Node* c,n->getParents()){
        _debugTree(c,nb);
    }
}
void VideoEngine::computeTreeHash(std::vector< std::pair<char*,U32> > &alreadyComputed, Node *n){
    for(int i =0; i < alreadyComputed.size();i++){
        if(!strcmp(alreadyComputed[i].first,n->getName()))
            return;
    }
    std::vector<char*> v;
    n->computeTreeHash(v);
    U32 hashVal = n->getHash()->getHashValue();
    char* name = QstringCpy(n->getName());
    alreadyComputed.push_back(make_pair(name,hashVal));
    foreach(Node* parent,n->getParents()){
        computeTreeHash(alreadyComputed, parent);
    }
    
    
}

void VideoEngine::changeTreeVersion(){
    std::vector< std::pair<char*,U32> > nodeHashs;
    _treeVersion.reset();
    if(!output){
        return;
    }
    computeTreeHash(nodeHashs, output);
    for(int i =0 ;i < nodeHashs.size();i++){
        _treeVersion.appendNodeHashToHash(nodeHashs[i].second);
        //        free(nodeHashs[i].first);
    }
    _treeVersion.computeHash();
    
}

char* VideoEngine::currentFrameName(){
    if(strcmp(inputs[0]->class_name(),"Reader")==0){
        return static_cast<Reader*>(inputs[0])->getCurrentFrameName();
    }
    return 0;
}




std::pair<char*,FrameID> VideoEngine::mapNewFrame(int frameNb,char* filename,int width,int height,int nbFrameHint){
    return _cache->mapNewFrame(frameNb,filename, width, height, nbFrameHint,_treeVersion.getHashValue());}

void VideoEngine::closeMappedFile(){_cache->closeMappedFile();}

void VideoEngine::clearPlayBackCache(){_cache->clearPlayBackCache();}
