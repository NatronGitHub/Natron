

//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <QtCore/QMutex>
#include <QtCore/qcoreapplication.h>
#include "Gui/Button.h"
#include <QtGui/QVector2D>
#include <QtWidgets/QAction>
#include <QtCore/QThread>
#include <iterator>
#include <cassert>
#include "Core/VideoEngine.h"
#include "Core/inputnode.h"
#include "Core/outputnode.h"
#include "Core/viewerNode.h"
#include "Core/settings.h"
#include "Core/model.h"
#include "Core/hash.h"
#include "Core/Timer.h"
#include "Core/lookUpTables.h"
#include "Core/viewercache.h"
#include "Core/nodecache.h"
#include "Core/row.h"
#include "Writer/Writer.h"

#include "Gui/mainGui.h"
#include "Gui/viewerTab.h"
#include "Gui/timeline.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/GLViewer.h"
#include "Gui/texturecache.h"

#include "Superviser/controler.h"
#include "Superviser/MemoryInfo.h"

#include <ImfThreading.h>

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

#define gl_viewer currentViewer->getUiContext()->viewer

void VideoEngine::videoEngine(int frameCount,bool fitFrameToViewer,bool forward,bool sameFrame){
    if (_working || !_enginePostProcessResults->isFinished()) {
        return;
    }
    _timer->playState=RUNNING;
    _frameRequestsCount = frameCount;
    _frameRequestIndex = 0;
    _forward = forward;
    _paused = false;
    _aborted = false;
    _dag.validate(false); // < validating sequence (mostly getting the same frame range for all nodes).
    computeFrameRequest(sameFrame,fitFrameToViewer,false);
}
void VideoEngine::stopEngine(){
    if(_dag.isOutputAViewer()){
        currentViewer->getUiContext()->play_Forward_Button->setChecked(false);
        currentViewer->getUiContext()->play_Backward_Button->setChecked(false);
    }
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
    if(_dag.isOutputAViewer()){
        GLint boundBuffer;
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &boundBuffer);
        if(boundBuffer != 0){
            glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
            glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,0);
        }
    }
}

void VideoEngine::computeFrameRequest(bool sameFrame,bool fitFrameToViewer,bool recursiveCall){
    _working = true;
    _sameFrame = sameFrame;
    
    int firstFrame = INT_MAX,lastFrame = INT_MIN, currentFrame = 0;
    TimeLine* frameSeeker = 0;
    if(_dag.isOutputAViewer()){
        frameSeeker = currentViewer->getUiContext()->frameSeeker;
    }
    Writer* writer = dynamic_cast<Writer*>(_dag.getOutput());
    Viewer* viewer = dynamic_cast<Viewer*>(_dag.getOutput());
    if (!_dag.isOutputAViewer()) {
        assert(writer);
        if(!recursiveCall){
            firstFrame = writer->firstFrame();
            lastFrame = writer->lastFrame();
            currentFrame = writer->firstFrame();
            writer->setCurrentFrameToStart();
            
        }else{
            firstFrame = writer->firstFrame();
            lastFrame = writer->lastFrame();
            writer->incrementCurrentFrame();
            currentFrame = writer->currentFrame();
        }
    }
    
    /*check whether we need to stop the engine*/
    if(!sameFrame && _aborted){
        /*aborted by the user*/
        _waitingTasks.clear();
        stopEngine();
        return;
    }else if(_paused || _frameRequestsCount==0){
        /*paused or frame request count ended.*/
        stopEngine();
        runTasks();
        stopEngine();
        return;
    }else if(_dag.isOutputAViewer() &&  recursiveCall && _dag.lastFrame() == _dag.firstFrame() && _frameRequestsCount == -1 && _frameRequestIndex == 1){
        /*1 frame is the sequence and we already compute it*/
        stopEngine();
        runTasks();
        stopEngine();
        return;
    }else if(!_dag.isOutputAViewer() && currentFrame == lastFrame+1){
        /*stoping the engine for writers*/
        stopEngine();
        return;
    }
    
    if(_dag.isOutputAViewer()){
        /*Determine what is the current frame when output is a viewer*/
        /*!recursiveCall means this is the first time it's called for the sequence.*/
        if(!recursiveCall){
            currentFrame = frameSeeker->currentFrame();
            if(!sameFrame){
                firstFrame = _dag.firstFrame();
                lastFrame = _dag.lastFrame();
                
                /*clamping the current frame to the range [first,last] if it wasn't*/
                if(currentFrame < firstFrame){
                    currentFrame = firstFrame;
                }
                else if(currentFrame > lastFrame){
                    currentFrame = lastFrame;
                }
                frameSeeker->seek(currentFrame);
            }
        }else{ // if the call is recursive, i.e: the next frame in the sequence
            /*clear the node cache, as it is very unlikely the user will re-use
             data from previous frame.*/
            NodeCache::getNodeCache()->clear();
            lastFrame = _dag.lastFrame();
            firstFrame = _dag.firstFrame();
            if(_forward){
                currentFrame = currentViewer->currentFrame()+1;
                if(currentFrame > lastFrame){
                    if(_loopMode)
                        currentFrame = firstFrame;
                    else{
                        _frameRequestsCount = 0;
                        return;
                    }
                }
            }else{
                currentFrame = currentFrame = currentViewer->currentFrame()-1;
                if(currentFrame < firstFrame){
                    if(_loopMode)
                        currentFrame = lastFrame;
                    else{
                        _frameRequestsCount = 0;
                        return;
                    }
                }
            }
            frameSeeker->seek(currentFrame);
        }
    }
    
    std::vector<Reader*> readers;
    const std::vector<InputNode*>& inputs = _dag.getInputs();
    for(U32 j=0;j<inputs.size();j++){
        InputNode* currentInput=inputs[j];
        if(currentInput->className() == string("Reader")){
            Reader* inp = static_cast<Reader*>(currentInput);
            inp->fitFrameToViewer(fitFrameToViewer);
            readers.push_back(inp);
        }
    }
    
    
        
    _executingCachedFrame = false;
    
    /*1 slot in the vector corresponds to 1 frame read by a reader.*/
    FramesVector readFrames = startReading(readers, true , true);
    
    if(!_dag.validate(true)){
        stopEngine();
        return;
    }
    
    if(_dag.isOutputAViewer() && fitFrameToViewer){
        gl_viewer->fitToFormat(gl_viewer->displayWindow());
    }
    /*Now that we called validate we can check if the texture is in the cache*/
    map<int,int> rows;
    size_t dataSize = 0;
    const Box2D& dataW = _dag.getOutput()->getInfo()->getDataWindow();
    ViewerGL::CACHING_MODE mode = ViewerGL::TEXTURE_CACHE;

    if(_dag.isOutputAViewer()){
        const Format &_dispW = _dag.getOutput()->getInfo()->getDisplayWindow();
        map<int,int>::iterator it;
        
        float zoomFactor = gl_viewer->getZoomFactor();
        rows = gl_viewer->computeRowSpan(_dispW, zoomFactor);
        it = rows.begin();
        map<int,int>::iterator last = rows.end();
        int firstRow,lastRow;
        if(rows.size() > 0){
            last--;
            firstRow = it->first;
            lastRow = last->first;
            if(rows.size() >= 2){
                it++;
                int gap = it->first - firstRow; // gap between first and second rows
                if (firstRow <= _dispW.y()+gap && lastRow >= _dispW.h()-1-gap) {
                    mode = ViewerGL::VIEWER_CACHE;
                }
            }
        }else{
            firstRow = _dispW.y();
            lastRow = _dispW.h()-1;
        }
        gl_viewer->setRowSpan(make_pair(firstRow, lastRow));
        int w = zoomFactor <= 1.f ? _dispW.w() * zoomFactor : _dispW.w();
        int h = rows.size();
        
        /*Now checking if the frame is already in either the ViewerCache*/
        std::string currentFrameName ;
        currentFrameName = readers[0]->getRandomFrameName(currentViewer->currentFrame());
        U64 key = FrameEntry::computeHashKey(currentFrameName,
                                             _treeVersion.getHashValue(),
                                             gl_viewer->getZoomFactor(),
                                             gl_viewer->getExposure(),
                                             gl_viewer->lutType(),
                                             gl_viewer->byteMode(),
                                             gl_viewer->dataWindow(),
                                             gl_viewer->displayWindow(),
                                             firstRow,
                                             lastRow);
        FrameEntry* iscached = viewer->get(key);
        
        /*Found in viewer cache, we execute the cached engine and leave*/
        if(iscached){
            _executingCachedFrame = true;
            viewer->cachedFrameEngine(iscached);
            return;
        }
        
        
        
        /*checking whether it is texture cached (only when not in playback)*/
        if(!recursiveCall && viewer->isTextureCached(key)){
            engineLoop();
            return;
        }
        
        /*If it reaches here, it means the frame neither belong
         to the ViewerCache nor to the TextureCache, we must
         allocate resources and render the frame.
         If this is a recursive call, we explicitly fallback 
         to the viewer cache storage as the texture cache is not
         meant for playback.*/
        if(recursiveCall && mode == ViewerGL::TEXTURE_CACHE) mode = ViewerGL::VIEWER_CACHE;
        
        dataSize = gl_viewer->determineFrameDataContainer(key,currentFrameName,w,h,mode);
               
    }else{
        for (int i = dataW.y(); i < dataW.top(); i++) {
            rows.insert(make_pair(i,i));
        }
    }
    
    computeTreeForFrame(rows,dataSize,_dag.getOutput());
}
void VideoEngine::finishComputeFrameRequest(){
    _sequenceToWork.clear();
    if(_dag.isOutputAViewer()){
        *_enginePostProcessResults = QtConcurrent::run(currentViewer->getUiContext()->viewer,
                                                       &ViewerGL::fillPBO,_gpuTransferInfo.src,_gpuTransferInfo.dst,_gpuTransferInfo.byteCount);
        _engineLoopWatcher->setFuture(*_enginePostProcessResults);
        
    }else{
        _dag.outputAsWriter()->startWriting();
        engineLoop();
    }
}


void VideoEngine::engineLoop(){
    if(_frameRequestIndex == 0 && _frameRequestsCount == 1 && !_sameFrame){
        _frameRequestsCount = 0;
    }else if(_frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
        _frameRequestsCount--;
    }
    
    _frameRequestIndex++;
    
    if(_dag.isOutputAViewer()){
        TextureEntry* currentTex = currentViewer->getUiContext()->viewer->getCurrentTexture();
        currentViewer->getUiContext()->viewer->copyPBOtoTexture(currentTex->w(),currentTex->h()); // fill texture, returns instantly
    }
    if(!_executingCachedFrame &&  _frameRequestsCount!=0 && !_paused){
        std::vector<Reader*> readers;
        const std::vector<InputNode*>& inputs = _dag.getInputs();
        for(U32 j=0;j<inputs.size();j++){
            InputNode* currentInput=inputs[j];
            if(currentInput->className() == string("Reader")){
                Reader* inp =static_cast<Reader*>(currentInput);
                inp->fitFrameToViewer(false);
                readers.push_back(inp);
            }
        }
        startReading(readers , false , true);
    }
    if(_dag.isOutputAViewer()){
        _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
        if((_frameRequestIndex%24)==0){
            emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
        }
        updateDisplay(); // updating viewer & pixel aspect ratio if needed
    }
    computeFrameRequest(false ,false,true); // recursive call for following frame.
    
}


void VideoEngine::computeTreeForFrame(const std::map<int,int>& rows,size_t dataSize,OutputNode *output){
    ChannelSet outChannels;
    if(_dag.isOutputAViewer()) outChannels = currentViewer->getUiContext()->displayChannels();
    else{// channels requested are those requested by the user
        outChannels = static_cast<Writer*>(_dag.getOutput())->requestedChannels();
    }
    const Box2D& dataW = output->getInfo()->getDataWindow();
    
    map<int,int>::const_iterator it = rows.begin();
    // selecting the right anchor of the row
    int right = dataW.right();
    //  _dataW.right() > _dispW.right() ? right = _dataW.right() : right = _dispW.right();
    
    // selecting the left anchor of the row
	int offset= dataW.x();
    // _dataW.x() < _dispW.x() ? offset = _dataW.x() : offset = _dispW.x();
    int counter = 0;
    for(; it!=rows.end() ; it++){
        if(_aborted){
            _waitingTasks.clear();
            stopEngine();
            return;
        }else if(_paused){
            _workerThreadsWatcher->cancel();
            stopEngine();
            runTasks();
            stopEngine();
            return;
        }
        int y = it->first;
        Row* row =new Row(offset,y,right,outChannels);
        row->zoomedY(counter);
        _sequenceToWork.push_back(row);
        counter++;
    }
    if(_dag.isOutputAViewer()){
        gl_viewer->drawing(true);
        void* gpuMappedBuffer = gl_viewer->allocateAndMapPBO(dataSize,gl_viewer->getPBOId(0));
        checkGLErrors();
        _gpuTransferInfo.set(gl_viewer->getFrameData(), gpuMappedBuffer, dataSize);
    }
    *_workerThreadsResults = QtConcurrent::map(_sequenceToWork,boost::bind(&VideoEngine::metaEnginePerRow,_1,output));
    _workerThreadsWatcher->setFuture(*_workerThreadsResults);
    
}
VideoEngine::FramesVector VideoEngine::startReading(std::vector<Reader*>& readers,bool useMainThread,bool useOtherThread){
    FramesVector frames;
    if(readers.size() == 0) return frames;
    Reader::DecodeMode mode = Reader::DEFAULT_DECODE;
    if(useMainThread){
        bool useOtherThreadOnNextLoop = false;
        for(U32 i = 0;i < readers.size(); i++){
            Reader* reader = readers[i];
            std::vector<Reader::Buffer::DecodedFrameDescriptor> ret;
            
            if(useOtherThreadOnNextLoop && useOtherThread){
                ret = reader->decodeFrames(mode, false, true , _forward);
            }else{
                ret = reader->decodeFrames(mode, true, false , _forward);
            }
            useOtherThreadOnNextLoop = !useOtherThreadOnNextLoop;
            for(U32 j = 0; j < ret.size() ; j ++){
                frames.push_back(ret[j]);
            }
            
        }
        for(U32 i = 0 ;i < frames.size() ;i++){
            Reader::Buffer::DecodedFrameDescriptor& it = frames[i];
            if(it._asynchTask && !it._asynchTask->isFinished()){
                it._asynchTask->waitForFinished();
            }
        }
    }else{
        if(!useOtherThread) return frames;
        std::vector<Reader::Buffer::DecodedFrameDescriptor> ret;
        Reader* reader = readers[0];
        ret = reader->decodeFrames(mode, false, true, _forward);
        for(U32 j = 0; j < ret.size() ; j ++){
            frames.push_back(ret[j]);
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
    output->engine(row->y(), row->offset(), row->right(), row->channels(), row);
    delete row;
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
    if (dagHasInputs()) {
        videoEngine(nbFrames,true,true);
        
    }
}

VideoEngine::VideoEngine(Model* engine,QMutex* lock):
_working(false),_aborted(false),_paused(true),
_forward(true),_frameRequestsCount(0),_frameRequestIndex(0),_loopMode(true),_sameFrame(false),
_executingCachedFrame(false){
    
    _engineLoopWatcher = new QFutureWatcher<void>;
    _enginePostProcessResults = new QFuture<void>;
    _workerThreadsResults = new QFuture<void>;
    _workerThreadsWatcher = new QFutureWatcher<void>;
    connect(_workerThreadsWatcher,SIGNAL(finished()),this,SLOT(finishComputeFrameRequest()));
    connect(_engineLoopWatcher, SIGNAL(finished()), this, SLOT(engineLoop()));
    connect(this,SIGNAL(engineLoopNeeded()),this,SLOT(engineLoop()));
    this->_coreEngine = engine;
    this->_lock= lock;
    
    /*Adjusting multi-threading for OpenEXR library.*/
    Imf::setGlobalThreadCount(QThread::idealThreadCount());
    
    _timer=new Timer();
    
}

VideoEngine::~VideoEngine(){
    _enginePostProcessResults->waitForFinished();
    _workerThreadsResults->waitForFinished();
    delete _workerThreadsResults;
    delete _workerThreadsWatcher;
    delete _engineLoopWatcher;
    delete _enginePostProcessResults;
    delete _timer;
}



void VideoEngine::clearInfos(Node* out){
    out->clear_info();
    foreach(Node* c,out->getParents()){
        clearInfos(c);
    }
}

void VideoEngine::setDesiredFPS(double d){
    _timer->setDesiredFrameRate(d);
}


void VideoEngine::abort(){
    _aborted=true;
    if(currentViewer){
        currentViewer->getUiContext()->play_Backward_Button->setChecked(false);
        currentViewer->getUiContext()->play_Forward_Button->setChecked(false);
    }
}
void VideoEngine::pause(){
    _paused=true;
    
}
void VideoEngine::startPause(bool c){
    if(currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        abort();
        return;
    }
    
    
    if(c && _dag.getOutput()){
        videoEngine(-1,false,true);
    }
    else if(!_dag.getOutput() || _dag.getInputs().size()==0){
        currentViewer->getUiContext()->play_Forward_Button->setChecked(false);
        currentViewer->getUiContext()->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::startBackward(bool c){
    
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()){
        pause();
        return;
    }
    if(c && _dag.getOutput()){
        videoEngine(-1,false,false);
    }
    else if(!_dag.getOutput() || _dag.getInputs().size()==0){
        currentViewer->getUiContext()->play_Forward_Button->setChecked(false);
        currentViewer->getUiContext()->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::previousFrame(){
    if( currentViewer->getUiContext()->play_Forward_Button->isChecked()
       || currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->currentFrame()-1, 1, false,false);
    //    else
    //        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()-1, 1, false,&VideoEngine::_previousFrame);
}

void VideoEngine::nextFrame(){
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()
       || currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    
    if(!_working)
        _startEngine(currentViewer->currentFrame()+1, 1, false,true);
    //    else
    //        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()+1, 1, false,&VideoEngine::_nextFrame);
}

void VideoEngine::firstFrame(){
    if( currentViewer->getUiContext()->play_Forward_Button->isChecked()
       || currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    
    if(!_working)
        _startEngine(currentViewer->firstFrame(), 1, false,false);
    //    else
    //        appendTask(frameSeeker->firstFrame(), 1, false, &VideoEngine::_firstFrame);
}

void VideoEngine::lastFrame(){
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()
       ||  currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->lastFrame(), 1, false,true);
    //    else
    //        appendTask(frameSeeker->lastFrame(), 1, false, &VideoEngine::_lastFrame);
}

void VideoEngine::previousIncrement(){
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()
       ||  currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = currentViewer->currentFrame()- currentViewer->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,false);
    //    else{
    //        appendTask(frame,1, false, &VideoEngine::_previousIncrement);
    //    }
    
    
}

void VideoEngine::nextIncrement(){
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()
       ||  currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = currentViewer->currentFrame()+currentViewer->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,true);
    //    else
    //        appendTask(frame,1, false, &VideoEngine::_nextIncrement);
}

void VideoEngine::seekRandomFrame(int f){
    if(!_dag.getOutput() || _dag.getInputs().size()==0) return;
    //if( ctrlPTR->getGui()->viewer_tab->play_Forward_Button->isChecked()
    //  ||  ctrlPTR->getGui()->viewer_tab->play_Backward_Button->isChecked()){
    pause();
    // }
    
    if(!_working)
        _startEngine(f, 1, false,true);
    else
        appendTask(f, -1, false,true, _dag.getOutput(),&VideoEngine::_startEngine);
}




void VideoEngine::changeDAGAndStartEngine(OutputNode* output){
    pause();
    if(!_working)
        _changeDAGAndStartEngine(currentViewer->currentFrame(), -1, false,true,output);
    else
        appendTask(currentViewer->currentFrame(), -1, false,true, output, &VideoEngine::_changeDAGAndStartEngine);
}

void VideoEngine::appendTask(int frameNB, int frameCount, bool initViewer,bool forward,OutputNode* output, VengineFunction func){
    _waitingTasks.push_back(Task(frameNB,frameCount,initViewer,forward,output,func));
}

void VideoEngine::runTasks(){
    for(unsigned int i=0; i < _waitingTasks.size(); i++){
        Task _t = _waitingTasks[i];
        VengineFunction f = _t._func;
        VideoEngine *vengine = this;
        _waitingTasks.clear();
        (*vengine.*f)(_t._newFrameNB,_t._frameCount,_t._initViewer,_t._forward,_t._output);
    }
}

void VideoEngine::_startEngine(int frameNB,int frameCount,bool initViewer,bool forward,OutputNode* ){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        if(frameNB < currentViewer->firstFrame() || frameNB > currentViewer->lastFrame())
            return;
        currentViewer->getUiContext()->frameSeeker->seek(frameNB);
        videoEngine(frameCount,initViewer,forward);
        
    }
}

void VideoEngine::_changeDAGAndStartEngine(int, int frameCount, bool initViewer,bool forward,OutputNode* output){
    _dag.resetAndSort(output,true);
    bool hasFrames = false;
    bool hasInputDifferentThanReader = false;
    for (U32 i = 0; i< _dag.getInputs().size(); i++) {
        Reader* r = static_cast<Reader*>(_dag.getInputs()[i]);
        if (r) {
            if (r->hasFrames()) {
                hasFrames = true;
            }
        }else{
            hasInputDifferentThanReader = true;
        }
    }
    changeTreeVersion();
    if(hasInputDifferentThanReader || hasFrames)
        videoEngine(-1,initViewer,_forward);
}


void VideoEngine::computeTreeHash(std::vector< std::pair<std::string,U64> > &alreadyComputed, Node *n){
    for(U32 i =0; i < alreadyComputed.size();i++){
        if(alreadyComputed[i].first == n->getName().toStdString())
            return;
    }
    std::vector<std::string> v;
    n->computeTreeHash(v);
    U64 hashVal = n->getHash()->getHashValue();
    alreadyComputed.push_back(make_pair(n->getName().toStdString(),hashVal));
    foreach(Node* parent,n->getParents()){
        computeTreeHash(alreadyComputed, parent);
    }
    
    
}

void VideoEngine::changeTreeVersion(){
    std::vector< std::pair<std::string,U64> > nodeHashs;
    _treeVersion.reset();
    if(!_dag.getOutput()){
        return;
    }
    computeTreeHash(nodeHashs, _dag.getOutput());
    for(U32 i =0 ;i < nodeHashs.size();i++){
        _treeVersion.appendNodeHashToHash(nodeHashs[i].second);
    }
    _treeVersion.computeHash();
    
}


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

void VideoEngine::DAG::reset(){
    _output = 0;
    
    clearGraph();
}

Viewer* VideoEngine::DAG::outputAsViewer() const {
    if(_output && _isViewer)
        return dynamic_cast<Viewer*>(_output);
    else
        return NULL;
}

Writer* VideoEngine::DAG::outputAsWriter() const {
    if(_output && !_isViewer)
        return dynamic_cast<Writer*>(_output);
    else
        return NULL;
}

void VideoEngine::DAG::resetAndSort(OutputNode* out,bool isViewer){
    _output = out;
    _isViewer = isViewer;
    
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

/*sets infos accordingly across all the DAG*/
bool VideoEngine::DAG::validate(bool forReal){
    /*Validating the DAG in topological order*/
    for (DAGIterator it = begin(); it!=end(); it++) {
        if (!(*it)->validate(forReal)) {
            return false;
        }
    }
    return true;
}


int VideoEngine::DAG::firstFrame() const {
    return _output->getInfo()->firstFrame();
}
int VideoEngine::DAG::lastFrame() const{
    return _output->getInfo()->lastFrame();
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

void VideoEngine::resetAndMakeNewDag(OutputNode* output,bool isViewer){
    _dag.resetAndSort(output,isViewer);
}