//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <QtCore/QMutex>
#include <QtCore/qcoreapplication.h>
#include <QtWidgets/QPushButton>
#include <QtGui/QVector2D>
#include <QtWidgets/QAction>
#include <iterator>
#include <cassert>
#include "Core/VideoEngine.h"
#include "Core/inputnode.h"
#include "Core/outputnode.h"
#include "Core/mappedfile.h"
#include "Core/viewerNode.h"
#include "Core/settings.h"
#include "Core/model.h"
#include "Core/hash.h"
#include "Core/Timer.h"
#include "Core/mappedfile.h"
#include "Core/lookUpTables.h"

#include "Gui/mainGui.h"
#include "Gui/viewerTab.h"
#include "Gui/timeline.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/GLViewer.h"

#include "Reader/readerInfo.h"

#include "Superviser/controler.h"
#include "Superviser/MemoryInfo.h"
#include "Superviser/PwStrings.h"


/* Here's a drawing that represents how the video Engine works:
 
 [thread pool]                       [OtherThread]
 [main thread]                              |                                   |
 -videoEngine(...)                          |                                   |
 >-computeFrameRequest(...)                 |                                   |
 |    if(!cached)                           |                                   |
 |       -computeTreeForFrame------------>start worker threads                  |
 |              |                           |                                   |
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
    computeFrameRequest(sameFrame,forward,fitFrameToViewer,false);
}
void VideoEngine::stopEngine(){
    _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
    _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
    _frameRequestsCount = 0;
    _working = false;
    _aborted = false;
    _paused = false;
    resetReadingBuffers();
    _enginePostProcessResults->waitForFinished();
    _workerThreadsResults->waitForFinished();
    _timer->playState=PAUSE;
    
}
void VideoEngine::resetReadingBuffers(){
    GLint boundBuffer;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &boundBuffer);
    if(boundBuffer != 0){
        glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,0);
    }
    std::vector<InputNode*>& inputs = _dag.getInputs();
    for(int j=0;j<inputs.size();j++){
        InputNode* currentInput=inputs[j];
        if(strcmp(currentInput->className(),"Reader")==0){
            static_cast<Reader*>(currentInput)->removeCachedFramesFromBuffer();
        }
    }
    
}

void VideoEngine::computeFrameRequest(bool sameFrame,bool forward,bool fitFrameToViewer,bool recursiveCall){
    _working = true;
    _sameFrame = sameFrame;
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
    
    
    /*if there's only 1 frame and we already computed it we stop*/
    if(lastFrame == firstFrame && _frameRequestsCount==-1 && _frameRequestIndex==1){
        stopEngine();
        runTasks();
        return;
    }
    
    int currentFrame = frameSeeker->currentFrame();
    if(recursiveCall){ // if the call is recursive, i.e: the next frame in the sequence
        if(_forward){
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
    }
    if(!sameFrame){ // clamp the frame to [firstFrame,lastFrame]
        clearRowCache();
        /*clamping the current frame to the range [first,last] if it wasn't*/
        
        if(currentFrame < firstFrame){
            currentFrame = firstFrame;
        }
        else if(currentFrame > lastFrame){
            currentFrame = lastFrame;
        }
        
    }
    std::vector<Reader*> readers;
    std::vector<InputNode*>& inputs = _dag.getInputs();
    for(int j=0;j<inputs.size();j++){
        InputNode* currentInput=inputs[j];
        if(strcmp(currentInput->className(),"Reader")==0){
            Reader* inp = static_cast<Reader*>(currentInput);
            inp->fitFrameToViewer(fitFrameToViewer);
            inp->randomFrame(currentFrame);
            readers.push_back(inp);
        }
    }
    gl_viewer->currentFrame(currentFrame);
    
    /*1 slot in the vector corresponds to 1 frame read by a reader. The second member indicates whether the frame
     was in cache or not.*/
    FramesVector readFrames = startReading(readers, true , true);
    for(U32 i = 0 ; i < readers.size() ; i++){
        readers[i]->makeCurrentDecodedFrame();
    }
    
    _timer->playState=RUNNING;
    
    // if inputs[0] is a reader we make its reader info the current ones
    // NEED TO ADJUST IT FOR STEREO AND SEVERAL INPUTS !
	if(!strcmp(inputs[0]->className(),"Reader")){
		makeReaderInfoCurrent(static_cast<Reader*>(inputs[0]));
	}
    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
    
    if(fitFrameToViewer){
        gl_viewer->initViewer(gl_viewer->displayWindow());
    }
    for(U32 i = 0; i < readFrames.size();i++){
        ReadFrame readFrame = readFrames[i];
        if (readFrame.second != _viewerCache->end()) {
            cachedFrameEngine(readFrame.second);
            if(_paused){
                stopEngine();
                runTasks();
                return;
            }
            engineLoop();
        }else{
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
            computeTreeForFrame(readFrame.first._filename,_dag.getOutput(),ceil((double)frameCountHint/10.0));
        }
    }
}
void VideoEngine::finishComputeFrameRequest(){
    _sequenceToWork.clear();
    
    *_enginePostProcessResults = QtConcurrent::run(gl_viewer,
                                                   &ViewerGL::fillPBO,_gpuTransferInfo.src,_gpuTransferInfo.dst,_gpuTransferInfo.byteCount);
    _engineLoopWatcher->setFuture(*_enginePostProcessResults);
}

void VideoEngine::cachedFrameEngine(ViewerCache::FramesIterator frame){
    int w = frame->second._actualW ;
    int h = frame->second._actualH ;
    size_t dataSize = 0;
    if(frame->second._byteMode==1.0){
        dataSize  = w * h * sizeof(U32) ;
    }else{
        dataSize  = w * h  * sizeof(float) * 4;
    }
    /*resizing texture if needed*/
    gl_viewer->initTexturesRgb(w,h);
    checkGLErrors();
    gl_viewer->drawing(true);
    QCoreApplication::processEvents();
    
}

void VideoEngine::engineLoop(){
    if(_frameRequestIndex == 0 && _frameRequestsCount == 1 && !_sameFrame){
        _frameRequestsCount = 0;
    }else if(_frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
        _frameRequestsCount--;
    }
    
    _frameRequestIndex++;
    
    std::pair<int,int> texSize = gl_viewer->getTextureSize();
    gl_viewer->copyPBOtoTexture(texSize.first, texSize.second); // fill texture, returns instantly
    std::vector<Reader*> readers;
    std::vector<InputNode*>& inputs = _dag.getInputs();
    for(int j=0;j<inputs.size();j++){
        InputNode* currentInput=inputs[j];
        if(strcmp(currentInput->className(),"Reader")==0){
            Reader* inp =static_cast<Reader*>(currentInput);
            inp->fitFrameToViewer(false);
            readers.push_back(inp);
        }
    }
    if(_frameRequestsCount!=0 && !_paused){
        startReading(readers , false , true);
    }
    
    _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
    if((_frameRequestIndex%24)==0){
        emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
    }
    
    updateDisplay(); // updating viewer & pixel aspect ratio if needed
    
    computeFrameRequest(false, _forward ,false,true); // recursive call for following frame.
    
}


void VideoEngine::computeTreeForFrame(std::string filename,OutputNode *output,int followingComputationsNb){
    Format _dispW = gl_viewer->displayWindow(); // the display window held by the data
    
    // REQUEST THE CHANNELS CONTAINED IN THE VIEWER TAB COMBOBOX IN THE GUI !!
    output->request(_dispW.y(),_dispW.top(),0,_dispW.right(),gl_viewer->displayChannels());
    // AT THIS POINT EVERY OPERATOR HAS ITS INFO SET!! AS WELL AS REQUESTED_BOX AND REQUESTED_CHANNELS
    
    //outChannels are the intersection between what the viewer requests and the ones available in the viewer node
    ChannelSet outChannels = gl_viewer->displayChannels() & output->getInfo()->channels();
    
    
    float zoomFactor = gl_viewer->getZoomFactor();
    std::vector<int> rows = gl_viewer->computeRowSpan(_dispW, zoomFactor);
    
    int w = _dispW.w() * zoomFactor;
    int h = rows.size();
    gl_viewer->initTextures(w,h);
    
    // selecting the right anchor of the row
    int right = 0;
    gl_viewer->dataWindow().right() > gl_viewer->displayWindow().right() ?
    right = gl_viewer->dataWindow().right() : right = gl_viewer->displayWindow().right();
    
    // selection the left anchor of the row
	int offset=0;
    gl_viewer->dataWindow().x() < gl_viewer->displayWindow().x() ?
    offset = gl_viewer->dataWindow().x() : offset = gl_viewer->displayWindow().x();
    
    //starting viewer preprocess : i.e initialize the cached frame
    gl_viewer->preProcess(filename,followingComputationsNb,w,h);
    for(U32 i = 0 ; i < rows.size() ; i++){
        if(_aborted){
            _abort();
            return;
        }else if(_paused){
            _workerThreadsWatcher->cancel();
            _workerThreadsResults->waitForFinished();
            return;
        }
        Row* row ;
        int y = rows[i];
        map<int,Row*>::iterator foundCached = isRowContainedInCache(y);
        if(foundCached!=row_cache.end()){
            row= foundCached->second;
            row->cached(true);
        }else{
            row=new Row(offset,y,right,outChannels);
            addToRowCache(row);
        }
        row->zoomedY(i);
        _sequenceToWork.push_back(row);
    }
    
    std::pair<void*,size_t> pbo = gl_viewer->allocatePBO(w, h);
    _gpuTransferInfo.set(gl_viewer->getFrameData(), pbo.first, pbo.second);
    
    *_workerThreadsResults = QtConcurrent::map(_sequenceToWork,boost::bind(&VideoEngine::metaEnginePerRow,this,_1,output));
    _workerThreadsWatcher->setFuture(*_workerThreadsResults);
    
}
VideoEngine::FramesVector VideoEngine::startReading(std::vector<Reader*> readers,bool useMainThread,bool useOtherThread){
    
    FramesVector frames;
    if(readers.size() == 0) return frames;
    
    Reader::DecodeMode mode = Reader::DEFAULT_DECODE;
    // if(isStereo ) mode = stereo
    
    if(useMainThread){
        bool useOtherThreadOnNextLoop = false;
        for(U32 i = 0;i < readers.size(); i++){
            Reader* reader = readers[i];
            std::string currentFrameName = reader->getCurrentFrameName();
            std::vector<Reader::Buffer::DecodedFrameDescriptor> ret;
            pair<ViewerCache::FramesIterator,bool> iscached =
            _viewerCache->isCached(currentFrameName,
                                   _treeVersion.getHashValue(),
                                   gl_viewer->getZoomFactor(),
                                   gl_viewer->getExposure(),
                                   gl_viewer->lutType(),
                                   gl_viewer->_byteMode,
                                   gl_viewer->getCurrentReaderInfo()->displayWindow(),
                                   gl_viewer->getCurrentReaderInfo()->dataWindow());
            if(!iscached.second){
                if(useOtherThreadOnNextLoop && useOtherThread){
                    ret = reader->decodeFrames(mode, false, true , _forward);
                }else{
                    ret = reader->decodeFrames(mode, true, false , _forward);
                }
                useOtherThreadOnNextLoop = !useOtherThreadOnNextLoop;
                for(U32 j = 0; j < ret.size() ; j ++){
                    frames.push_back(make_pair(ret[j],_viewerCache->end()));
                }
            }else{
                int frameNb = _frameRequestsCount==1 && reader->firstFrame() == reader->lastFrame() ? 0 : reader->currentFrame();
                frames.push_back(make_pair(reader->openCachedFrame(iscached.first,frameNb,false),
                                           iscached.first));
            }
        }
        for(U32 i = 0 ;i < frames.size() ;i++){
            Reader::Buffer::DecodedFrameDescriptor it = frames[i].first;
            if(it._asynchTask && !it._asynchTask->isFinished()){
                it._asynchTask->waitForFinished();
            }
        }
    }else{
        if(!useOtherThread) return frames;
        std::vector<Reader::Buffer::DecodedFrameDescriptor> ret;
        Reader* reader = readers[0];
        if(reader->firstFrame() == reader->lastFrame()) return frames;
        int followingFrame = reader->currentFrame();
        _forward ? followingFrame++ : followingFrame--;
        if(followingFrame > reader->lastFrame()) followingFrame = reader->firstFrame();
        if(followingFrame < reader->firstFrame()) followingFrame = reader->lastFrame();
        
        std::string followingFrameName = reader->getRandomFrameName(followingFrame);
        pair<ViewerCache::FramesIterator,bool> iscached = _viewerCache->isCached(followingFrameName,
                                                                                 _treeVersion.getHashValue(),
                                                                                 gl_viewer->getZoomFactor(),
                                                                                 gl_viewer->getExposure(),
                                                                                 gl_viewer->lutType(),
                                                                                 gl_viewer->_byteMode,
                                                                                 gl_viewer->getCurrentReaderInfo()->displayWindow(),
                                                                                 gl_viewer->getCurrentReaderInfo()->dataWindow());
        if(!iscached.second){
            ret = reader->decodeFrames(mode, false, true, _forward);
            for(U32 j = 0; j < ret.size() ; j ++){
                frames.push_back(make_pair(ret[j],_viewerCache->end()));
            }
        }else{
            int frameNb = _frameRequestsCount==1 && reader->firstFrame() == reader->lastFrame() ? 0 : followingFrame;
            frames.push_back(make_pair(reader->openCachedFrame(iscached.first,frameNb,true),
                                       iscached.first));
        }
        
    }
    return frames;
}

void VideoEngine::drawOverlay(){
    if(_dag.getOutput())
        _drawOverlay(_dag.getOutput());
}

void VideoEngine::_drawOverlay(Node *output){
    output->drawOverlay();
    foreach(Node* n,output->getParents()){
        _drawOverlay(n);
    }
    
}

void VideoEngine::metaEnginePerRow(Row* row, OutputNode* output){
    if(!row->cached())
        row->allocate();
    
    for(DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); it++){
        Node* node = *it;
        if((node->getOutputChannels() & node->getInfo()->channels())){
            
            if(!row->cached()){
                node->engine(row->y(),row->offset(),row->right(),node->getRequestedChannels() & node->getInfo()->channels(),row);
            }else{
                if(node == output){
                    node->engine(row->y(),row->offset(),row->right(),
                                 node->getRequestedChannels() & node->getInfo()->channels(),row);
                }
            }
        }else{
            cout << qPrintable(node->getName()) << " doesn't output any channel " << endl;
        }
    }
}

void VideoEngine::updateProgressBar(){
    //update progress bar
}
void VideoEngine::updateDisplay(){
    int width = gl_viewer->width();
    int height = gl_viewer->height();
    float ap = gl_viewer->displayWindow().pixel_aspect();
    if(ap > 1.f){
        glViewport (0, 0, width*ap, height);
    }else{
        glViewport (0, 0, width, height/ap);
    }
    gl_viewer->updateGL();
}

void VideoEngine::startEngine(int nbFrames){
    videoEngine(nbFrames,true,true);
}

VideoEngine::VideoEngine(ViewerGL *gl_viewer,Model* engine,QMutex* lock):
QObject(engine),_working(false),_aborted(false),_paused(true),_readerInfoHasChanged(false),
_forward(true),_frameRequestsCount(0),_frameRequestIndex(0),_loopMode(true),_sameFrame(false){
    
    _engineLoopWatcher = new QFutureWatcher<void>;
    _enginePostProcessResults = new QFuture<void>;
    _workerThreadsResults = new QFuture<void>;
    _workerThreadsWatcher = new QFutureWatcher<void>;
    connect(_workerThreadsWatcher,SIGNAL(finished()),this,SLOT(finishComputeFrameRequest()));
    connect(_engineLoopWatcher, SIGNAL(finished()), this, SLOT(engineLoop()));
    this->_coreEngine = engine;
    this->_lock= lock;
    this->gl_viewer = gl_viewer;
    gl_viewer->setVideoEngine(this);
    _timer=new Timer();
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
    _viewerCache = new ViewerCache(gl_viewer,maxDiskCacheSize/2, maxMemoryCacheSize);
    QObject::connect(engine->getControler()->getGui()->actionClearDiskCache, SIGNAL(triggered()),this,SLOT(clearDiskCache()));
    QObject::connect(engine->getControler()->getGui()->actionClearPlayBackCache, SIGNAL(triggered()),this,SLOT(clearPlayBackCache()));
    QObject::connect(engine->getControler()->getGui()->actionClearBufferCache, SIGNAL(triggered()),this,SLOT(clearRowCache()));
    
}

void VideoEngine::clearDiskCache(){
    _viewerCache->clearCache();
}

VideoEngine::~VideoEngine(){
    clearRowCache();
    _enginePostProcessResults->waitForFinished();
    _workerThreadsResults->waitForFinished();
    delete _workerThreadsResults;
    delete _workerThreadsWatcher;
    delete _engineLoopWatcher;
    delete _enginePostProcessResults;
    delete _viewerCache;
    delete _timer;
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
            //delete info;
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
    _workerThreadsWatcher->cancel();
    _workerThreadsResults->waitForFinished(); // stopping worker threads
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
    
    
    if(c && _dag.getOutput()){
        videoEngine(-1,false,true);
    }
    else if(!_dag.getOutput() || _dag.getInputs().size()==0){
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
    if(c && _dag.getOutput()){
        videoEngine(-1,false,false);
    }
    else if(!_dag.getOutput() || _dag.getInputs().size()==0){
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
    //    else
    //        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()-1, 1, false,&VideoEngine::_previousFrame);
}

void VideoEngine::nextFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _nextFrame(gl_viewer->getCurrentReaderInfo()->currentFrame()+1, 1, false);
    //    else
    //        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()+1, 1, false,&VideoEngine::_nextFrame);
}

void VideoEngine::firstFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
    if(!_working)
        _firstFrame(frameSeeker->firstFrame(), 1, false);
    //    else
    //        appendTask(frameSeeker->firstFrame(), 1, false, &VideoEngine::_firstFrame);
}

void VideoEngine::lastFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
    if(!_working)
        _lastFrame(frameSeeker->lastFrame(), 1, false);
    //    else
    //        appendTask(frameSeeker->lastFrame(), 1, false, &VideoEngine::_lastFrame);
}

void VideoEngine::previousIncrement(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = gl_viewer->getCurrentReaderInfo()->currentFrame()-_coreEngine->getControler()->getGui()->viewer_tab->incrementSpinBox->value();
    if(!_working)
        _previousIncrement(frame, 1, false);
    //    else{
    //        appendTask(frame,1, false, &VideoEngine::_previousIncrement);
    //    }
    
    
}

void VideoEngine::nextIncrement(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = gl_viewer->getCurrentReaderInfo()->currentFrame()+_coreEngine->getControler()->getGui()->viewer_tab->incrementSpinBox->value();
    if(!_working)
        _nextIncrement(frame, 1, false);
    //    else
    //        appendTask(frame,1, false, &VideoEngine::_nextIncrement);
}

void VideoEngine::seekRandomFrame(int f){
    if(!_dag.getOutput() || _dag.getInputs().size()==0) return;
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
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB >= frameSeeker->firstFrame()){
            gl_viewer->currentFrame(frameNB);
            std::vector<InputNode*>& inputs = _dag.getInputs();
            for(int j=0;j<inputs.size();j++){
                std::vector<InputNode*>& inputs = _dag.getInputs();
                InputNode* currentInput=inputs[j];
                if(strcmp(currentInput->className(),"Reader")==0){
                    Reader* inp = static_cast<Reader*>(currentInput);
                    //int f =inp->currentFrame();
                    inp->decrementCurrentFrameIndex();
                    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                    //                    if(inp->currentFrame()!=f){ // if it has successfully decremented, i.e: we weren't on the first frame
                    //                        static_cast<Reader*>(currentInput)->setup_for_next_frame();
                    //                    }
                }
            }
            videoEngine(frameCount,initViewer,false);
        }
        
    }
    
}
void VideoEngine::_nextFrame(int frameNB,int frameCount,bool initViewer){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB <= frameSeeker->lastFrame()){
            gl_viewer->currentFrame(frameNB);
            std::vector<InputNode*>& inputs = _dag.getInputs();
            for(int j=0;j<inputs.size();j++){
                InputNode* currentInput=inputs[j];
                if(strcmp(currentInput->className(),"Reader")==0){
                    Reader* inp = static_cast<Reader*>(currentInput);
                    //int f =inp->currentFrame();
                    inp->incrementCurrentFrameIndex();
                    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                    //                    if(inp->currentFrame()!=f){ // if it has successfully incremented, i.e: we weren't on the last frame
                    //                        static_cast<Reader*>(currentInput)->setup_for_next_frame();
                    //                    }
                }
            }
            videoEngine(frameCount,initViewer,true);
        }
    }
    
}
void VideoEngine::_previousIncrement(int frameNB,int frameCount,bool initViewer){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB > frameSeeker->firstFrame())
            gl_viewer->currentFrame(frameNB);
        else
            gl_viewer->currentFrame(frameSeeker->firstFrame());
        std::vector<InputNode*>& inputs = _dag.getInputs();
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->className(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                if(frameNB > frameSeeker->firstFrame()){
                    inp->randomFrame(frameNB);
                }else{
                    inp->randomFrame(frameSeeker->firstFrame());
                }
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
			}
		}
        videoEngine(frameCount,initViewer,false);
    }
    
}
void VideoEngine::_nextIncrement(int frameNB,int frameCount,bool initViewer){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB < frameSeeker->lastFrame())
            gl_viewer->currentFrame(frameNB);
        else
            gl_viewer->currentFrame(frameSeeker->lastFrame());
        std::vector<InputNode*>& inputs = _dag.getInputs();
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->className(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                if(frameNB < frameSeeker->lastFrame()){
                    inp->randomFrame(frameNB);
                }else{
                    inp->randomFrame(frameSeeker->lastFrame());
                }
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
			}
		}
        videoEngine(frameCount,initViewer,true);
    }
    
}
void VideoEngine::_firstFrame(int frameNB,int frameCount,bool initViewer){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        gl_viewer->currentFrame(frameNB);
        std::vector<InputNode*>& inputs = _dag.getInputs();
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->className(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->randomFrame(frameNB);
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
			}
		}
        videoEngine(frameCount,initViewer,false);
    }
    
}
void VideoEngine::_lastFrame(int frameNB,int frameCount,bool initViewer){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        gl_viewer->currentFrame(frameNB);
        std::vector<InputNode*>& inputs = _dag.getInputs();
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->className(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
				inp->randomFrame(frameNB);
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                
			}
		}
        videoEngine(frameCount,initViewer,true);
    }
    
}
void VideoEngine::_seekRandomFrame(int frameNB,int frameCount,bool initViewer){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB < frameSeeker->firstFrame() || frameNB > frameSeeker->lastFrame())
            return;
        gl_viewer->currentFrame(frameNB);
        std::vector<InputNode*>& inputs = _dag.getInputs();
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->className(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->randomFrame(frameNB);
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
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
        _waitingTasks.erase(_waitingTasks.begin()+i);
        (*vengine.*f)(_t._newFrameNB,_t._frameCount,_t._initViewer);
    }
    _waitingTasks.clear();
}


/*code needs to be reviewed*/
void VideoEngine::seekRandomFrameWithStart(int f){
    if(_dag.getOutput() && _dag.getInputs().size()>0) return;
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    std::vector<InputNode*>& inputs = _dag.getInputs();
    if(_dag.getOutput() && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(f < frameSeeker->firstFrame() || f > frameSeeker->lastFrame())
            return;
        gl_viewer->currentFrame(f);
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->className(),"Reader")==0){
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
                // static_cast<Reader*>(currentInput)->setup_for_next_frame();
                
            }
		}
        videoEngine(-1,false,true);
        //videoSequenceEngine(output, inputs, -1,false);
    }
}

void VideoEngine::debugTree(){
    int nb=0;
    _debugTree(_dag.getOutput(),&nb);
    cout << "The tree contains " << nb << " nodes. " << endl;
}
void VideoEngine::_debugTree(Node* n,int* nb){
    *nb = *nb+1;
    cout << *n << endl;
    foreach(Node* c,n->getParents()){
        _debugTree(c,nb);
    }
}
void VideoEngine::computeTreeHash(std::vector< std::pair<char*,U64> > &alreadyComputed, Node *n){
    for(int i =0; i < alreadyComputed.size();i++){
        if(!strcmp(alreadyComputed[i].first,n->getName()))
            return;
    }
    std::vector<char*> v;
    n->computeTreeHash(v);
    U64 hashVal = n->getHash()->getHashValue();
    char* name = QstringCpy(n->getName());
    alreadyComputed.push_back(make_pair(name,hashVal));
    foreach(Node* parent,n->getParents()){
        computeTreeHash(alreadyComputed, parent);
    }
    
    
}

void VideoEngine::changeTreeVersion(){
    std::vector< std::pair<char*,U64> > nodeHashs;
    _treeVersion.reset();
    if(!_dag.getOutput()){
        return;
    }
    computeTreeHash(nodeHashs, _dag.getOutput());
    for(int i =0 ;i < nodeHashs.size();i++){
        _treeVersion.appendNodeHashToHash(nodeHashs[i].second);
    }
    _treeVersion.computeHash();
    
}
std::pair<char*,ViewerCache::FrameID> VideoEngine::mapNewFrame(int frameNb,
                                                               std::string filename,
                                                               int width,
                                                               int height,
                                                               int nbFrameHint){
    return _viewerCache->mapNewFrame(frameNb,filename, width, height, nbFrameHint,_treeVersion.getHashValue());
}

void VideoEngine::closeMappedFile(){_viewerCache->closeMappedFile();}

void VideoEngine::clearPlayBackCache(){_viewerCache->clearPlayBackCache();}

void VideoEngine::DAG::fillGraph(Node* n){
    if(std::find(_graph.begin(),_graph.end(),n)==_graph.end()){
        n->setMarked(false);
        _graph.push_back(n);
        if(n->isInputNode()){
            _inputs.push_back(static_cast<InputNode*>(n));
        }
    }
    foreach(Node* p,n->getParents()){
        fillGraph(p);
    }
}
void VideoEngine::DAG::clearGraph(){
    _graph.clear();
    _sorted.clear();
    _inputs.clear();
    
}
void VideoEngine::DAG::topologicalSort(){
    for(U32 i = 0 ; i < _graph.size(); i++){
        Node* n = _graph[i];
        if(!n->isMarked())
            _depthCycle(n);
    }
    
}
void VideoEngine::DAG::_depthCycle(Node* n){
    n->setMarked(true);
    foreach(Node* p, n->getParents()){
        if(!p->isMarked()){
            _depthCycle(p);
        }
    }
    _sorted.push_back(n);
}
void VideoEngine::DAG::resetAndSort(OutputNode* out){
    _output = out;
    clearGraph();
    if(!_output){
        return;
    }
    fillGraph(dynamic_cast<Node*>(out));
    
    topologicalSort();
}
void VideoEngine::DAG::debug(){
    cout << "Topological ordering of the DAG is..." << endl;
    for(DAG::DAGIterator it = begin(); it != end() ;it++){
        cout << (*it)->getName().toStdString() << endl;
    }
}


void VideoEngine::debugRowSequence(){
    int h = _sequenceToWork.size();
    int w = _sequenceToWork[0]->right() - _sequenceToWork[0]->offset();
    if(h == 0 || w == 0) cout << "empty img" << endl;
    QImage img(w,h,QImage::Format_ARGB32);
    for(int i = 0; i < h ; i++){
        Row* from = _sequenceToWork[i];
        const float* r = (*from)[Channel_red];
        const float* g = (*from)[Channel_green];
        const float* b = (*from)[Channel_blue];
        const float* a = (*from)[Channel_alpha];
        for(int j = 0 ; j < w ; j++){
            QColor c(r ? Lut::clamp((*r++))*255 : 0,
                     g ? Lut::clamp((*g++))*255 : 0,
                     b ? Lut::clamp((*b++))*255 : 0,
                     a? Lut::clamp((*a++))*255 : 255);
            img.setPixel(j, i, c.rgba());
        }
    }
    string name("debug_");
    char tmp[60];
    sprintf(tmp,"%i",w);
    name.append(tmp);
    name.append("x");
    sprintf(tmp, "%i",h);
    name.append(tmp);
    name.append(".png");
    img.save(name.c_str());
}