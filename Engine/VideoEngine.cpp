

//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

#include "VideoEngine.h"

#include <iterator>
#include <cassert>
#include <QtCore/QMutex>
#include <QtGui/QVector2D>
#include <QAction>
#include <QtCore/QThread>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <ImfThreading.h>

#include "Gui/Button.h"
#include "Engine/ViewerNode.h"
#include "Engine/OfxNode.h"
#include "Engine/Settings.h"
#include "Engine/Model.h"
#include "Engine/Hash.h"
#include "Engine/Lut.h"
#include "Engine/ViewerCache.h"
#include "Engine/NodeCache.h"
#include "Engine/Row.h"
#include "Engine/MemoryFile.h"
#include "Writers/Writer.h"
#include "Engine/Timer.h"

#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/Timeline.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/ViewerGL.h"

#include "Global/Controler.h"
#include "Global/MemoryInfo.h"


/* Here's a drawing that represents how the video Engine works:
 
 1) Without caching
 
 [main thread]                    [OtherThread]
 |                                  |
 - videoEngine
 |------QtConcurrent::Run()>-computeFrameRequest_<
 |< -------------------------**finished()**      |
 -computeTreeForFrame()                  |           |
 |------QtConcurrent::map()--->metaEnginePerRow  |
 |<---------------------------**finished()**     |
 -engineLoop()                           |           |
 |-----------------------QtConcurrent::Run()------
 -updateDisplay()
 
 
 Whenever waiting for a **finished()** signal, the main thread is still executing
 the event loop. All OpenGL code is kept into the main thread and everything is done
 to avoid complications related to OpenGL.
 */

using namespace std;
using namespace Powiter;

#define gl_viewer currentViewer->getUiContext()->viewer

void VideoEngine::videoEngine(int frameCount,bool fitFrameToViewer,bool forward,bool sameFrame){
    if (_working) {
        return;
    }
    // cout << "+ STARTING ENGINE " << endl;
    _timer->playState=RUNNING;
    _frameRequestsCount = frameCount;
    _frameRequestIndex = 0;
    _forward = forward;
    _paused = false;
    _aborted = false;
    if(!_dag.validate(false)){ // < validating sequence (mostly getting the same frame range for all nodes).
        stopEngine();
        return;
    }
    double zoomFactor;
    if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
        zoomFactor = gl_viewer->getZoomFactor();
        currentViewer->getUiContext()->play_Forward_Button->setChecked(_forward);
        currentViewer->getUiContext()->play_Backward_Button->setChecked(!_forward);
    }else{
        zoomFactor = 1.f;
    }
    
    /*beginRenderAction for all openFX nodes*/
    for (DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
        OfxNode* n = dynamic_cast<OfxNode*>(*it);
        if(n){
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            n->beginRenderAction(0, 25, 1, true,renderScale);
        }
    }
    changeTreeVersion();
    _mainEntry->setArgsForNextRun(zoomFactor, sameFrame, fitFrameToViewer, false);
    _computeFrameWatcher->setFuture(QtConcurrent::run(_mainEntry,&EngineMainEntry::computeFrame));
    
}
void VideoEngine::stopEngine(){
    if(_dag.isOutputAViewer()){
        currentViewer->getUiContext()->play_Forward_Button->setChecked(false);
        currentViewer->getUiContext()->play_Backward_Button->setChecked(false);
    }
    // cout << "- STOPPING ENGINE"<<endl;
    _frameRequestsCount = 0;
    _aborted = false;
    _paused = false;
    _lastEngineStatus._returnCode = EngineStatus::ABORTED;
    _working = false;
    _timer->playState=PAUSE;
    
    /*endRenderAction for all openFX nodes*/
    for (DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
        OfxNode* n = dynamic_cast<OfxNode*>(*it);
        if(n){
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            n->endRenderAction(0, 25, 1, true, renderScale);
        }
    }
    
}

void EngineMainEntry::computeFrameRequest(float zoomFactor,bool sameFrame,bool fitFrameToViewer,bool recursiveCall){
    //cout << "     _computeFrameRequest()" << endl;
    _engine->_working = true;
    _sameFrame = sameFrame;
    VideoEngine::EngineStatus::RetCode returnCode;
    int firstFrame = INT_MAX,lastFrame = INT_MIN, currentFrame = 0;
    TimeLine* frameSeeker = 0;
    if(_engine->_dag.isOutputAViewer() && !_engine->_dag.isOutputAnOpenFXNode()){
        frameSeeker = currentViewer->getUiContext()->frameSeeker;
    }
    Writer* writer = dynamic_cast<Writer*>(_engine->_dag.getOutput());
    ViewerNode* viewer = dynamic_cast<ViewerNode*>(_engine->_dag.getOutput());
    OfxNode* ofxOutput = dynamic_cast<OfxNode*>(_engine->_dag.getOutput());
    if (!_engine->_dag.isOutputAViewer()) {
        if(!_engine->_dag.isOutputAnOpenFXNode()){
            assert(writer);
            if(!recursiveCall){
                lastFrame = writer->lastFrame();
                currentFrame = writer->firstFrame();
                writer->setCurrentFrameToStart();
                
            }else{
                lastFrame = writer->lastFrame();
                writer->incrementCurrentFrame();
                currentFrame = writer->currentFrame();
            }
        }else{
            assert(ofxOutput);
            if(!recursiveCall){
                lastFrame = ofxOutput->lastFrame();
                currentFrame = ofxOutput->firstFrame();
                ofxOutput->setCurrentFrameToStart();
                
            }else{
                lastFrame = ofxOutput->lastFrame();
                ofxOutput->incrementCurrentFrame();
                currentFrame = ofxOutput->currentFrame();
            }
        }
    }
    
    /*check whether we need to stop the engine*/
    if(_engine->_aborted){
        /*aborted by the user*/
        _engine->stopEngine();
        return;
    }
    if((_engine->_dag.isOutputAViewer()
        &&  recursiveCall
        && _engine->_dag.lastFrame() == _engine->_dag.firstFrame()
        && _engine->_frameRequestsCount == -1
        && _engine->_frameRequestIndex == 1)
       || _engine->_frameRequestsCount == 0
       || _engine->_paused){
        /*1 frame in the sequence and we already computed it*/
        _engine->stopEngine();
        _engine->runTasks();
        _engine->_lastEngineStatus._returnCode = VideoEngine::EngineStatus::ABORTED;
        return;
    }else if(!_engine->_dag.isOutputAViewer() && currentFrame == lastFrame+1){
        /*stoping the engine for writers*/
        _engine->stopEngine();
        return;
    }
    
    
    
    if(_engine->_dag.isOutputAViewer() && !_engine->_dag.isOutputAnOpenFXNode()){ //openfx viewers are UNSUPPORTED
        /*Determine what is the current frame when output is a viewer*/
        /*!recursiveCall means this is the first time it's called for the sequence.*/
        if(!recursiveCall){
            currentFrame = frameSeeker->currentFrame();
            if(!sameFrame){
                firstFrame = _engine->_dag.firstFrame();
                lastFrame = _engine->_dag.lastFrame();
                
                /*clamping the current frame to the range [first,last] if it wasn't*/
                if(currentFrame < firstFrame){
                    currentFrame = firstFrame;
                }
                else if(currentFrame > lastFrame){
                    currentFrame = lastFrame;
                }
                frameSeeker->seek_notSlot(currentFrame);
            }
        }else{ // if the call is recursive, i.e: the next frame in the sequence
            /*clear the node cache, as it is very unlikely the user will re-use
             data from previous frame.*/
            //  NodeCache::getNodeCache()->clear();
            lastFrame = _engine->_dag.lastFrame();
            firstFrame = _engine->_dag.firstFrame();
            if(_engine->_forward){
                currentFrame = currentViewer->currentFrame()+1;
                if(currentFrame > lastFrame){
                    if(_engine->_loopMode)
                        currentFrame = firstFrame;
                    else{
                        _engine->stopEngine();
                        return;
                    }
                }
            }else{
                currentFrame  = currentViewer->currentFrame()-1;
                if(currentFrame < firstFrame){
                    if(_engine->_loopMode)
                        currentFrame = lastFrame;
                    else{
                        _engine->stopEngine();
                        return;
                    }
                }
            }
            frameSeeker->seek_notSlot(currentFrame);
        }
    }
    
    QList<Reader*> readers;
    
    const std::vector<Node*>& inputs = _engine->_dag.getInputs();
    for(U32 j=0;j<inputs.size();++j) {
        Node* currentInput=inputs[j];
        if(currentInput->className() == string("Reader")){
            Reader* inp = static_cast<Reader*>(currentInput);
            inp->fitFrameToViewer(fitFrameToViewer);
            readers << inp;
        }
    }
    
    
    QList<bool> readHeaderResults = QtConcurrent::blockingMapped(readers,boost::bind(&VideoEngine::metaReadHeader,_1,currentFrame));
    for (int i = 0; i < readHeaderResults.size(); i++) {
        if (readHeaderResults.at(i) == false) {
            _engine->stopEngine();
            return;
        }
    }
    
    _engine->_dag.validate(true);
    
    const Format &_dispW = _engine->_dag.getOutput()->getInfo()->getDisplayWindow();
    if(_engine->_dag.isOutputAViewer() && !_engine->_dag.isOutputAnOpenFXNode() && fitFrameToViewer){
        gl_viewer->fitToFormat(_dispW);
        zoomFactor = gl_viewer->getZoomFactor();
    }
    /*Now that we called validate we can check if the frame is in the cache
     and return the appropriate EngineStatus code.*/
    vector<int> rows;
    vector<int> columns;
    const Box2D& dataW = _engine->_dag.getOutput()->getInfo()->getDataWindow();
    FrameEntry* iscached= 0;
    U64 key = 0;
    if(_engine->_dag.isOutputAViewer() && !_engine->_dag.isOutputAnOpenFXNode()){
        gl_viewer->drawing(true);
        
        std::pair<int,int> rowSpan = gl_viewer->computeRowSpan(rows,_dispW);
        std::pair<int,int> columnSpan = gl_viewer->computeColumnSpan(columns, _dispW);
        
        TextureRect textureRect(columnSpan.first,rowSpan.first,columnSpan.second,rowSpan.second,columns.size(),rows.size());
        
        
        //        cout << "[RECT CREATION] : " << "x = "<< textureRect.x  << " y = " << textureRect.y
        //        << " r = " << textureRect.r << " t = " << textureRect.t << " w = " << textureRect.w
        //        << " h = " << textureRect.h << endl;
        
        /*Now checking if the frame is already in either the ViewerCache*/
        _engine->_viewerCacheArgs._zoomFactor = zoomFactor;
        _engine->_viewerCacheArgs._exposure = gl_viewer->getExposure();
        _engine->_viewerCacheArgs._lut = gl_viewer->lutType();
        _engine->_viewerCacheArgs._byteMode = gl_viewer->byteMode();
        _engine->_viewerCacheArgs._dataWindow = dataW;
        _engine->_viewerCacheArgs._displayWindow = _dispW;
        _engine->_viewerCacheArgs._textureRect = textureRect;
        if(textureRect.w == 0 || textureRect.h == 0){
            _engine->stopEngine();
            return;
        }
        key = FrameEntry::computeHashKey(currentFrame,
                                         _engine->_treeVersion,
                                         _engine->_viewerCacheArgs._zoomFactor,
                                         _engine->_viewerCacheArgs._exposure,
                                         _engine->_viewerCacheArgs._lut,
                                         _engine->_viewerCacheArgs._byteMode,
                                         dataW,
                                         _dispW,
                                         textureRect);
        _engine->_lastEngineStatus._x = columnSpan.first;
        _engine->_lastEngineStatus._r = columnSpan.second+1;
        
        
        _engine->_viewerCacheArgs._hashKey = key;
        iscached = viewer->get(key);
        
        /*Found in viewer cache, we execute the cached engine and leave*/
        if(iscached){
            
            /*Checking that the entry retrieve matches absolutely what we
             asked for.*/
            assert(iscached->_textureRect == textureRect);
            assert(iscached->_treeVers == _engine->_treeVersion);
            // assert(iscached->_zoom == _viewerCacheArgs._zoomFactor);
            assert(iscached->_lut == _engine->_viewerCacheArgs._lut);
            assert(iscached->_exposure == _engine->_viewerCacheArgs._exposure);
            assert(iscached->_byteMode == _engine->_viewerCacheArgs._byteMode);
            assert(iscached->_frameInfo->getDisplayWindow() == _dispW);
            assert(iscached->_frameInfo->getDataWindow() == dataW);
            
            _engine->_viewerCacheArgs._textureRect = iscached->_textureRect;
            returnCode = VideoEngine::EngineStatus::CACHED_ENGINE;
            goto stop;
        }
        
    }else{
        for (int i = dataW.y(); i < dataW.top(); ++i) {
            rows.push_back(i);
        }
        _engine->_lastEngineStatus._x = dataW.x();
        _engine->_lastEngineStatus._r = dataW.right();
    }
    /*If it reaches here, it means the frame neither belong
     to the ViewerCache nor to the TextureCache, we must
     allocate resources and render the frame.
     If this is a recursive call, we explicitly fallback
     to the viewer cache storage as the texture cache is not
     meant for playback.*/
    QtConcurrent::blockingMap(readers,boost::bind(&VideoEngine::metaReadData,_1,currentFrame));
    returnCode = VideoEngine::EngineStatus::NORMAL_ENGINE;
    
stop:
    _engine->_lastEngineStatus._cachedEntry = iscached;
    _engine->_lastEngineStatus._key = key;
    _engine->_lastEngineStatus._returnCode = returnCode;
    _engine->_lastEngineStatus._rows = rows;
}

void VideoEngine::dispatchEngine(){
    // cout << "     _dispatchEngine()" << endl;
    if(_lastEngineStatus._returnCode == EngineStatus::NORMAL_ENGINE) {
        if (_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()) {
            ViewerNode* viewer = _dag.outputAsViewer();
            viewer->makeCurrentViewer();
            _viewerCacheArgs._dataSize = gl_viewer->allocateFrameStorage(_viewerCacheArgs._textureRect.w,
                                                                         _viewerCacheArgs._textureRect.h);
        }
        //   cout << "     _computeTreForFrame()" << endl;
        _worker->setArgsForNextRun(_lastEngineStatus._rows, _lastEngineStatus._x, _lastEngineStatus._r, _dag.getOutput());
        _workerThreadsWatcher->setFuture(QtConcurrent::run(_worker,&Worker::computeTreeForFrame));
        
    }else if(_lastEngineStatus._returnCode == EngineStatus::CACHED_ENGINE){
        // cout << "    _cachedFrameEngine()" << endl;
        ViewerNode* viewer = _dag.outputAsViewer();
        viewer->cachedFrameEngine(_lastEngineStatus._cachedEntry);
    }
    
}

void VideoEngine::copyFrameToCache(const char* src){
    
    FrameEntry* entry = ViewerCache::getViewerCache()->addFrame(_viewerCacheArgs._hashKey,
                                                                _treeVersion,
                                                                _viewerCacheArgs._zoomFactor,
                                                                _viewerCacheArgs._exposure,
                                                                _viewerCacheArgs._lut,
                                                                _viewerCacheArgs._byteMode,
                                                                _viewerCacheArgs._textureRect,
                                                                _viewerCacheArgs._dataWindow,
                                                                _viewerCacheArgs._displayWindow);
    
    if(entry){
        memcpy(entry->getMappedFile()->data(),src,_viewerCacheArgs._dataSize);
        entry->removeReference(); // removing reference as we're done with the entry.
    }else{
#ifdef POWITER_DEBUG
        cout << "WARNING: caching does not seem to work properly..failing to add the entry." << endl;
#endif
    }
}
void Worker::_computeTreeForFrame(const std::vector<int>& rows,int x,int r,Node *output){
    //  cout << "<<<COMPUTE FRAME>>>" << endl;
    /*If playback is on (i.e: not panning/zooming or changing the graph) we clear the cache
     for every frame.*/
    assert(_engine);
    if(!_engine->_sameFrame){
        NodeCache::getNodeCache()->clear();
    }
    ChannelSet outChannels;
    if(_engine->_dag.isOutputAViewer() && !_engine->_dag.isOutputAnOpenFXNode()){
        assert(currentViewer);
        assert(currentViewer->getUiContext());
        outChannels = currentViewer->getUiContext()->displayChannels();
    }
    else{// channels requested are those requested by the user
        if(!_engine->_dag.isOutputAnOpenFXNode()){
            assert(_engine->_dag.getOutput());
            outChannels = static_cast<Writer*>(_engine->_dag.getOutput())->requestedChannels();
        }else{
            //openfx outputs can only output RGBA
            assert(_engine->_dag.getOutput());
            outChannels = Mask_RGBA;
        }
    }
    
    int counter = 0;
    gettimeofday(&_engine->_lastComputeFrameTime, 0);
    for (vector<int>::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
        Row* row = new Row(x,*it,r,outChannels);
        row->zoomedY(counter);
        RowRunnable* worker = new RowRunnable(row,output);
        if(counter%10 == 0){
            /* UNCOMMENT to report progress.
             QObject::connect(worker, SIGNAL(finished(int,int)), _engine ,SLOT(checkAndDisplayProgress(int,int)));
             **/
        }
        _threadPool->start(worker);
        ++counter;
    }
    _threadPool->waitForDone();
}



void VideoEngine::engineLoop(){
    //  cout << "__ENGINE LOOP__" << endl;
    if(_frameRequestIndex == 0 && _frameRequestsCount == 1 && !_sameFrame){
        _frameRequestsCount = 0;
    }else if(_frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
        --_frameRequestsCount;
    }
    ++_frameRequestIndex;//incrementing the frame counter
    
    
    float zoomFactor = 1.f;
    if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
        
        //copying the frame data stored into the PBO to the viewer cache if it was a normal engine
        //This is done only if we run a sequence (i.e: playback) because the viewer cache isn't meant for
        //panning/zooming.
        gl_viewer->stopDisplayingProgressBar();
        if(_lastEngineStatus._returnCode == EngineStatus::NORMAL_ENGINE && !_sameFrame){
            copyFrameToCache(gl_viewer->getFrameData());
        }else if(_lastEngineStatus._returnCode == EngineStatus::CACHED_ENGINE){ // cached engine
            _lastEngineStatus._cachedEntry->removeReference(); // the cached engine has finished using this frame
        }
        gl_viewer->copyPBOToRenderTexture(_viewerCacheArgs._textureRect); // returns instantly
        
        _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
        if((_frameRequestIndex%24)==0){
            emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
        }
        zoomFactor = gl_viewer->getZoomFactor();
    }else if(!_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
        /*if the output is a writer we actually start writing on disk now*/
        _dag.outputAsWriter()->startWriting();
    }
    
    // recursive call, before the updateDisplay (swapBuffer) so it can run concurrently
    _mainEntry->setArgsForNextRun(zoomFactor, _sameFrame, false, true);
    _computeFrameWatcher->setFuture(QtConcurrent::run(_mainEntry,&EngineMainEntry::computeFrame));
    
    
    if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
        updateDisplay(); // updating viewer & pixel aspect ratio if needed
    }
    if(_autoSaveOnNextRun){
        _autoSaveOnNextRun = false;
        ctrlPTR->autoSave();
    }
}



bool VideoEngine::metaReadHeader(Reader* reader,int current_frame){
    if(!reader->readCurrentHeader(current_frame)) // FIXME: return value may be false and reader->readHandle may be NULL
        return false;
    return true;
}

void VideoEngine::metaReadData(Reader* reader,int current_frame){
    reader->readCurrentData(current_frame);
}



void VideoEngine::drawOverlay() const{
    if(_dag.getOutput())
        _drawOverlay(_dag.getOutput());
}

void VideoEngine::_drawOverlay(Node *output) const{
    output->drawOverlay();
    foreach(Node* n,output->getParents()){
        _drawOverlay(n);
    }
    
}

void VideoEngine::metaEnginePerRow(Row* row, Node* output){
    output->engine(row->y(), row->offset(), row->right(), row->channels(), row);
    delete row;
}
void RowRunnable::run() {
    _output->engine(_row->y(), _row->offset(), _row->right(), _row->channels(), _row);
    emit finished(_row->y(),_row->zoomedY());
    delete _row;
}


void VideoEngine::updateProgressBar(){
    //update progress bar
}
void VideoEngine::updateDisplay(){
    int width = gl_viewer->width();
    int height = gl_viewer->height();
    double ap = gl_viewer->displayWindow().pixel_aspect();
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
void VideoEngine::repeatSameFrame(){
    if (dagHasInputs()) {
        if(_working){
            appendTask(currentViewer->currentFrame(), 1, false,_forward,true, _dag.getOutput(),&VideoEngine::_startEngine);
        }else{
            videoEngine(1,false,true,true);
        }
    }
}

VideoEngine::VideoEngine(Model* engine):
_coreEngine(engine),
_working(false),
_aborted(false),
_paused(true),
_frameRequestsCount(0),
_frameRequestIndex(0),
_forward(true),
_loopMode(true),
_sameFrame(false),
_autoSaveOnNextRun(false)

{
    _workerThreadsResults = new QFuture<void>;
    _workerThreadsWatcher = new QFutureWatcher<void>;
    connect(_workerThreadsWatcher,SIGNAL(finished()),this,SLOT(engineLoop()));
    _computeFrameWatcher = new QFutureWatcher<void>;
    connect(_computeFrameWatcher,SIGNAL(finished()),this,SLOT(dispatchEngine()));
    //  connect(_workerThreadsWatcher,SIGNAL(canceled()),this,SLOT(stopEngine()));
    /*Adjusting multi-threading for OpenEXR library.*/
    Imf::setGlobalThreadCount(QThread::idealThreadCount());
    
    _timer=new Timer();
    
    _mainEntry = new EngineMainEntry(this);
    _worker = new Worker(this);
    
    
}

VideoEngine::~VideoEngine(){
    _workerThreadsResults->waitForFinished();
    delete _workerThreadsResults;
    delete _workerThreadsWatcher;
    delete _timer;
    delete _computeFrameWatcher;
    delete _mainEntry;
    delete _worker;
    
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
    _workerThreadsResults->cancel();
    if(currentViewer){
        gl_viewer->forceUnmapPBO();
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
    }else{
        pause();
    }
}
void VideoEngine::previousFrame(){
    if( _working){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->currentFrame()-1, 1, false,false,false);
    else
        appendTask(currentViewer->currentFrame()-1, 1,  false,_forward,false,_dag.getOutput(), &VideoEngine::_startEngine);
}

void VideoEngine::nextFrame(){
    if(_working){
        pause();
    }
    
    if(!_working)
        _startEngine(currentViewer->currentFrame()+1, 1, false,true,false);
    else
        appendTask(currentViewer->currentFrame()+1,  1,false,_forward,false,_dag.getOutput(), &VideoEngine::_startEngine);
}

void VideoEngine::firstFrame(){
    if( _working){
        pause();
    }
    
    if(!_working)
        _startEngine(currentViewer->firstFrame(), 1, false,false,false);
    else
        appendTask(currentViewer->firstFrame(), 1,  false,_forward,false,_dag.getOutput(),  &VideoEngine::_startEngine);
}

void VideoEngine::lastFrame(){
    if(_working){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->lastFrame(), 1, false,true,false);
    else
        appendTask(currentViewer->lastFrame(), 1,  false,_forward,false,_dag.getOutput(),  &VideoEngine::_startEngine);
}

void VideoEngine::previousIncrement(){
    if(_working){
        pause();
    }
    int frame = currentViewer->currentFrame()- currentViewer->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,false,false);
    else{
        appendTask(frame,1, false,_forward,false,_dag.getOutput(), &VideoEngine::_startEngine);
    }
    
    
}

void VideoEngine::nextIncrement(){
    if(_working){
        pause();
    }
    int frame = currentViewer->currentFrame()+currentViewer->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,true,false);
    else
        appendTask(frame,1, false,_forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
}

void VideoEngine::seekRandomFrame(int f){
    if(!_dag.getOutput() || _dag.getInputs().size()==0) return;
    if(_working){
        pause();
    }
    
    if(!_working){
        if(_frameRequestsCount == -1){
            _startEngine(f, -1, false,_forward,false);
        }else{
            _startEngine(f, 1, false,_forward,false);
        }
    }
    else{
        if(_frameRequestsCount == -1){
            appendTask(f, -1, false,_forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
        }else{
            appendTask(f, 1, false,_forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
        }
    }
}
void VideoEngine::recenterViewer(){
    if(_working){
        pause();
    }
    if(!_working){
        if(_frameRequestsCount == -1)
            _startEngine(currentViewer->currentFrame(), -1, true,_forward,false);
        else
            _startEngine(currentViewer->currentFrame(), 1, true,_forward,false);
    }else{
        appendTask(currentViewer->currentFrame(), -1, true,_forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
    }
}



void VideoEngine::changeDAGAndStartEngine(Node* output){
    pause();
    if(!_working){
        if(_dag.getOutput()){
            if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
                _changeDAGAndStartEngine(currentViewer->currentFrame(), -1, false,true,false,output);
            }else if(!_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
                _changeDAGAndStartEngine(_dag.outputAsWriter()->currentFrame(), -1, false,true,false,output);
            }else if(_dag.isOutputAnOpenFXNode()){
                _changeDAGAndStartEngine(_dag.outputAsOpenFXNode()->currentFrame(), -1, false,true,false,output);
            }
        }else{
            _changeDAGAndStartEngine(0, -1, false,true,false,output);
        }
    }else{
        if(_dag.getOutput()){
            if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
                appendTask(currentViewer->currentFrame(), 1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
            }else if(!_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
                appendTask(_dag.outputAsWriter()->currentFrame(),1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
            }else if(_dag.isOutputAnOpenFXNode()){
                appendTask(_dag.outputAsOpenFXNode()->currentFrame(),1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
            }
        }else{
            appendTask(0,1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
        }
    }
}

void VideoEngine::appendTask(int frameNB, int frameCount, bool initViewer,bool forward,bool sameFrame,Node* output, VengineFunction func){
    _waitingTasks.push_back(Task(frameNB,frameCount,initViewer,forward,sameFrame,output,func));
}

void VideoEngine::runTasks(){
    if(_waitingTasks.size() > 0){
        Task _t = _waitingTasks.back();
        VengineFunction f = _t._func;
        VideoEngine *vengine = this;
        (*vengine.*f)(_t._newFrameNB,_t._frameCount,_t._initViewer,_t._forward,_t._sameFrame,_t._output);
        _waitingTasks.clear();
    }
    
}

void VideoEngine::_startEngine(int frameNB,int frameCount,bool initViewer,bool forward,bool sameFrame,Node* ){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        if(frameNB < currentViewer->firstFrame() || frameNB > currentViewer->lastFrame())
            return;
        currentViewer->getUiContext()->frameSeeker->seek_notSlot(frameNB);
        videoEngine(frameCount,initViewer,forward,sameFrame);
        
    }
}

void VideoEngine::_changeDAGAndStartEngine(int , int frameCount, bool initViewer,bool,bool sameFrame,Node* output){
    bool isViewer = false;
    if(dynamic_cast<ViewerNode*>(output)){
        isViewer = true;
    }
    _dag.resetAndSort(output,isViewer);
    const vector<Node*>& inputs = _dag.getInputs();
    bool start = false;
    for (U32 i = 0 ; i < inputs.size(); ++i) {
        if(inputs[i]->className() == "Reader"){
            Reader* reader = dynamic_cast<Reader*>(inputs[i]);
            if(reader->hasFrames()) start = true;
            else{
                start = false;
                break;
            }
        }else{
            if(inputs[0]->isInputNode()){
                start = true;
            }
        }
    }
    if(start)
        videoEngine(frameCount,initViewer,_forward,sameFrame);
}



void VideoEngine::changeTreeVersion(){
    
    if(!_dag.getOutput()){
        return;
    }
    Node* output = _dag.getOutput();
    std::vector<std::string> v;
    output->computeTreeHash(v);
    _treeVersion = output->getHash()->getHashValue();
    
}


void VideoEngine::DAG::fillGraph(Node* n){
    if(std::find(_graph.begin(),_graph.end(),n)==_graph.end()){
        n->setMarked(false);
        _graph.push_back(n);
        if(n->isInputNode()){
            _inputs.push_back(n);
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
    for(U32 i = 0 ; i < _graph.size(); ++i) {
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

ViewerNode* VideoEngine::DAG::outputAsViewer() const {
    if(_output && _isViewer)
        return dynamic_cast<ViewerNode*>(_output);
    else
        return NULL;
}

Writer* VideoEngine::DAG::outputAsWriter() const {
    if(_output && !_isViewer)
        return dynamic_cast<Writer*>(_output);
    else
        return NULL;
}
OfxNode* VideoEngine::DAG::outputAsOpenFXNode() const{
    if(_output && _isOutputOpenFXNode){
        return dynamic_cast<OfxNode*>(_output);
    }else{
        return NULL;
    }
}

void VideoEngine::DAG::resetAndSort(Node* out,bool isViewer){
    _output = out;
    _isViewer = isViewer;
    if(out && out->isOpenFXNode()){
        _isOutputOpenFXNode = true;
    }else{
        _isOutputOpenFXNode = false;
    }
    clearGraph();
    if(!_output){
        return;
    }
    fillGraph(out);
    
    topologicalSort();
}
void VideoEngine::DAG::debug(){
    cout << "Topological ordering of the DAG is..." << endl;
    for(DAG::DAGIterator it = begin(); it != end() ;++it) {
        cout << (*it)->getName() << endl;
    }
}

/*sets infos accordingly across all the DAG*/
bool VideoEngine::DAG::validate(bool forReal){
    /*Validating the DAG in topological order*/
    for (DAGIterator it = begin(); it!=end(); ++it) {
        if(!(*it)->validate(forReal))
            return false;
    }
    return true;
}


int VideoEngine::DAG::firstFrame() const {
    return _output->getInfo()->firstFrame();
}
int VideoEngine::DAG::lastFrame() const{
    return _output->getInfo()->lastFrame();
}



void VideoEngine::resetAndMakeNewDag(Node* output,bool isViewer){
    _dag.resetAndSort(output,isViewer);
}
#ifdef POWITER_DEBUG
bool VideoEngine::rangeCheck(const std::vector<int>& columns,int x,int r){
    for (unsigned int i = 0; i < columns.size(); ++i) {
        if(columns[i] < x || columns[i] > r){
            return false;
        }
    }
    return true;
}
#endif



void VideoEngine::checkAndDisplayProgress(int y,int zoomedY){
    timeval now;
    gettimeofday(&now, 0);
    double t =  now.tv_sec  - _lastComputeFrameTime.tv_sec +
    (now.tv_usec - _lastComputeFrameTime.tv_usec) * 1e-6f;
    if(t >= 0.3){
        //   cout << zoomedY << endl;
        gl_viewer->updateProgressOnViewer(_viewerCacheArgs._textureRect, y,zoomedY);
    }
}
