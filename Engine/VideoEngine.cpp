

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
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <ImfThreading.h>

#include "Engine/ViewerNode.h"
#include "Engine/OfxNode.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Model.h"
#include "Engine/Hash64.h"
#include "Engine/Lut.h"
#include "Engine/ViewerCache.h"
#include "Engine/NodeCache.h"
#include "Engine/Row.h"
#include "Engine/MemoryFile.h"
#include "Engine/Timer.h"
#include "Writers/Writer.h"
#include "Readers/Reader.h"


#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/ViewerGL.h"
#include "Gui/Button.h"


#include "Global/AppManager.h"
#include "Global/MemoryInfo.h"



using namespace std;
using namespace Powiter;


/**
 *@brief The callback reading the header of the current frame for a reader.
 *@param reader[in] A pointer to the reader that will read the header.
 *@param current_frame[in] The frame number in the sequence to decode.
 */

bool metaReadHeader(Reader* reader,int current_frame){
    if(!reader->readCurrentHeader(current_frame))
        return false;
    return true;
}

/**
 *@brief The callback reading the data of the current frame for a reader.
 *@param reader[in] A pointer to the reader that will read the data.
 *@param current_frame[in] The frame number in the sequence to decode.
 */
void metaReadData(Reader* reader,int current_frame){
    reader->readCurrentData(current_frame);
}

/**
 *@brief The callback cycling through the DAG for one scan-line
 *@param row[in] The row to compute. Note that after that function row will be deleted and cannot be accessed any longer.
 *@param output[in] The output node of the graph.
 */
void metaEnginePerRow(Row* row, Node* output){
    output->engine(row->y(), row->offset(), row->right(), row->channels(), row);
    delete row;
    // QMetaObject::invokeMethod(_engine, "onProgressUpdate", Qt::QueuedConnection, Q_ARG(int, zoomedY));
}

void VideoEngine::render(int frameCount,bool fitFrameToViewer,bool forward,bool sameFrame){
    if (_working) {
        return;
    }
    // cout << "+ STARTING ENGINE " << endl;
    _timer->playState=RUNNING;
   
    _aborted = false;
    
    double zoomFactor;
    if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
        ViewerNode* viewer = _dag.outputAsViewer();
        zoomFactor = viewer->getUiContext()->viewer->getZoomFactor();
        emit engineStarted(forward);
    }else{
        zoomFactor = 1.f;
    }
    
    bool oldVersionValid = _treeVersionValid;
    U64 oldVersion;
    if (oldVersionValid) {
        oldVersion = getCurrentTreeVersion();
    }
    changeTreeVersion();
    
    /*If the DAG changed we clear the playback cache.*/
    if(!oldVersionValid || (_treeVersion != oldVersion)){
        _model->getApp()->clearPlaybackCache(); // FIXME: the playback cache seems to be global to the application, why should we clear it here?
    }
    
    _lastRunArgs._zoomFactor = zoomFactor;
    _lastRunArgs._sameFrame = sameFrame;
    _lastRunArgs._fitToViewer = fitFrameToViewer;
    _lastRunArgs._recursiveCall = false;
    _lastRunArgs._forward = forward;
    _lastRunArgs._frameRequestsCount = frameCount;
    _lastRunArgs._frameRequestIndex = 0;
    
    if (!isRunning()) {
        start(HighestPriority);
    } else {
        _startCondition.wakeOne();
    }
}
void VideoEngine::stopEngine(){
    if(_dag.isOutputAViewer()){
        emit engineStopped();

    }
    // cout << "- STOPPING ENGINE"<<endl;
    _lastRunArgs._frameRequestsCount = 0;
    _aborted = false;
    _working = false;
    _timer->playState=PAUSE;
    _model->getGeneralMutex()->unlock();
    

}

void VideoEngine::run(){
    
    for(;;){ // infinite loop
        
        if(_mustQuit)
            return;
        
        /*Locking out other rendering tasks so 1 VideoEngine gets access to all
         nodes.That means only 1 frame can be rendered at any time. We would have to copy
         all the nodes that have a varying state (such as Readers/Writers) for every VideoEngine
         running simultaneously which is not very efficient and adds the burden to synchronize
         states of nodes etc...*/
        _model->getGeneralMutex()->lock();
        if(!_dag.validate(false)){ // < validating sequence (mostly getting the same frame range for all nodes).
            stopEngine();
            return;
        }

        
        _working = true; 
        
        /*beginRenderAction for all openFX nodes*/
        for (DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            OfxNode* n = dynamic_cast<OfxNode*>(*it);
            if(n){
                OfxPointD renderScale;
                renderScale.x = renderScale.y = 1.0;
                n->effectInstance()->beginRenderAction(0, 25, 1, true,renderScale);
            }
        }
        
        int firstFrame = INT_MAX,lastFrame = INT_MIN, currentFrame = 0;
        
        Writer* writer = dynamic_cast<Writer*>(_dag.getOutput());
        ViewerNode* viewer = dynamic_cast<ViewerNode*>(_dag.getOutput());
        OfxNode* ofxOutput = dynamic_cast<OfxNode*>(_dag.getOutput());
        if (!_dag.isOutputAViewer()) {
            if(!_dag.isOutputAnOpenFXNode()){
                assert(writer);
                if(!_lastRunArgs._recursiveCall){
                    lastFrame = writer->getTimeLine().lastFrame();
                    currentFrame = writer->getTimeLine().firstFrame();
                    writer->getTimeLine().seek(writer->getTimeLine().firstFrame());
                    
                }else{
                    lastFrame = writer->getTimeLine().lastFrame();
                    writer->getTimeLine().incrementCurrentFrame();
                    currentFrame = writer->getTimeLine().currentFrame();
                }
            }else{
                assert(ofxOutput);
                if(!_lastRunArgs._recursiveCall){
                    lastFrame = ofxOutput->getTimeLine().lastFrame();
                    currentFrame = ofxOutput->getTimeLine().firstFrame();
                    ofxOutput->getTimeLine().seek(ofxOutput->getTimeLine().firstFrame());
                    
                }else{
                    lastFrame = ofxOutput->getTimeLine().lastFrame();
                    ofxOutput->getTimeLine().incrementCurrentFrame();
                    currentFrame = ofxOutput->getTimeLine().currentFrame();
                }
            }
        }
        
        /*check whether we need to stop the engine*/
        if(_aborted){
            /*aborted by the user*/
            stopEngine();
            emit doRunTasks();
            return;
        }
        if((_dag.isOutputAViewer()
            &&  _lastRunArgs._recursiveCall
            && _dag.lastFrame() == _dag.firstFrame()
            && _lastRunArgs._frameRequestsCount == -1
            && _lastRunArgs._frameRequestIndex == 1)
           || _lastRunArgs._frameRequestsCount == 0){
            /*1 frame in the sequence and we already computed it*/
            stopEngine();
            emit doRunTasks();
            //runTasks();
            _mutex->lock();
            _startCondition.wait(_mutex);
            _mutex->unlock();
            continue;
        }else if(!_dag.isOutputAViewer() && currentFrame == lastFrame+1){
            /*stoping the engine for writers*/
            stopEngine();
            _mutex->lock();
            _startCondition.wait(_mutex);
            _mutex->unlock();
            continue;
        }
        
        
        
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){ //openfx viewers are UNSUPPORTED
            /*Determine what is the current frame when output is a viewer*/
            /*!recursiveCall means this is the first time it's called for the sequence.*/
            if(!_lastRunArgs._recursiveCall){
                currentFrame = viewer->getTimeLine().currentFrame();
                firstFrame = _dag.firstFrame();
                lastFrame = _dag.lastFrame();
                
                /*clamping the current frame to the range [first,last] if it wasn't*/
                if(currentFrame < firstFrame){
                    currentFrame = firstFrame;
                }
                else if(currentFrame > lastFrame){
                    currentFrame = lastFrame;
                }
                viewer->getTimeLine().seek(currentFrame);
                
            }else{ // if the call is recursive, i.e: the next frame in the sequence
                /*clear the node cache, as it is very unlikely the user will re-use
                 data from previous frame.*/
                //  NodeCache::getNodeCache()->clear();
                lastFrame = _dag.lastFrame();
                firstFrame = _dag.firstFrame();
                if(_lastRunArgs._forward){
                    currentFrame = _dag.outputAsViewer()->currentFrame()+1;
                    if(currentFrame > lastFrame){
                        if(_loopMode)
                            currentFrame = firstFrame;
                        else{
                            stopEngine();
                            return;
                        }
                    }
                }else{
                    currentFrame  = _dag.outputAsViewer()->currentFrame()-1;
                    if(currentFrame < firstFrame){
                        if(_loopMode)
                            currentFrame = lastFrame;
                        else{
                            stopEngine();
                            return;
                        }
                    }
                }
                viewer->getTimeLine().seek(currentFrame);
            }
        }
        
        QList<Reader*> readers;
        
        const std::vector<Node*>& inputs = _dag.getInputs();
        for(U32 j=0;j<inputs.size();++j) {
            Node* currentInput=inputs[j];
            if(currentInput->className() == string("Reader")){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->fitFrameToViewer(_lastRunArgs._fitToViewer);
                readers << inp;
            }
        }
        
        
        QList<bool> readHeaderResults = QtConcurrent::blockingMapped(readers,boost::bind(metaReadHeader,_1,currentFrame));
        for (int i = 0; i < readHeaderResults.size(); i++) {
            if (readHeaderResults.at(i) == false) {
                _mutex->lock();
                _startCondition.wait(_mutex);
                _mutex->unlock();
                continue;
            }
        }
        
        _dag.validate(true);
        
        const Format &_dispW = _dag.getOutput()->info().displayWindow();
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            viewer->makeCurrentViewer();
            if(_lastRunArgs._fitToViewer){
                viewer->getUiContext()->viewer->fitToFormat(_dispW);
                _lastRunArgs._zoomFactor = viewer->getUiContext()->viewer->getZoomFactor();
            }
            
        }        /*Now that we called validate we can check if the frame is in the cache
         and return the appropriate EngineStatus code.*/
        vector<int> rows;
        vector<int> columns;
        int x=0,r=0;
        const Box2D& dataW = _dag.getOutput()->info().dataWindow();
        FrameEntry* iscached= 0;
        U64 key = 0;
        float lut = 0.f;
        float exposure = 0.f;
        float zoomFactor = 0.f;
        float byteMode = 0.f;
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            viewer->getUiContext()->viewer->drawing(true);
            
            std::pair<int,int> rowSpan = viewer->getUiContext()->viewer->computeRowSpan(rows,_dispW);
            std::pair<int,int> columnSpan = viewer->getUiContext()->viewer->computeColumnSpan(columns, _dispW);
            
            TextureRect textureRect(columnSpan.first,rowSpan.first,columnSpan.second,rowSpan.second,columns.size(),rows.size());
            
            /*Now checking if the frame is already in either the ViewerCache*/
            _lastFrameInfos._textureRect = textureRect;
            if(textureRect.w == 0 || textureRect.h == 0){
                stopEngine();
                return;
            }
            zoomFactor = _lastRunArgs._zoomFactor;
            exposure = viewer->getUiContext()->viewer->getExposure();
            lut =  viewer->getUiContext()->viewer->lutType();
            byteMode = viewer->getUiContext()->viewer->byteMode();
            key = FrameEntry::computeHashKey(currentFrame,
                                             getCurrentTreeVersion(),
                                             zoomFactor,
                                             exposure,
                                             lut,
                                             byteMode,
                                             dataW,
                                             _dispW,
                                             textureRect);
            x = columnSpan.first;
            r = columnSpan.second+1;
            
            
            iscached = ViewerCache::getViewerCache()->get(key);
            
            /*Found in viewer cache, we execute the cached engine and leave*/
            if(iscached){
                _lastFrameInfos._cachedEntry = iscached;

                /*Checking that the entry retrieve matches absolutely what we
                 asked for.*/
                assert(iscached->_textureRect == textureRect);
                assert(iscached->_treeVers == getCurrentTreeVersion());
                // assert(iscached->_zoom == _viewerCacheArgs._zoomFactor);
                assert(iscached->_lut == lut);
                assert(iscached->_exposure == exposure);
                assert(iscached->_byteMode == byteMode);
                assert(iscached->_frameInfo->displayWindow() == _dispW);
                assert(iscached->_frameInfo->dataWindow() == dataW);
                
                _lastFrameInfos._textureRect = iscached->_textureRect;
                
                _mutex->lock();
                emit doCachedEngine();
                _openGLCondition->wait(_mutex);
                _mutex->unlock();
                _lastFrameInfos._cachedEntry->removeReference(); // the cached engine has finished using this frame
                engineLoop();
                _model->getGeneralMutex()->unlock();
                continue;
            }
            
        }else{
            for (int i = dataW.bottom(); i < dataW.top(); ++i) {
                rows.push_back(i);
            }
            x = dataW.left();
            r = dataW.right();
        }
        /*If it reaches here, it means the frame doesn't belong
         to the ViewerCache, we must
         allocate resources and render the frame.*/
        _lastFrameInfos._rows = rows;
        QtConcurrent::blockingMap(readers,boost::bind(metaReadData,_1,currentFrame));
        if (_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()) {
            _mutex->lock();
            emit doFrameStorageAllocation();
            _openGLCondition->wait(_mutex);
            _mutex->unlock();
        }
        
        if(!_lastRunArgs._sameFrame){
            NodeCache::getNodeCache()->clear();
        }
        ChannelSet outChannels;
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            outChannels = viewer->getUiContext()->displayChannels();
        }
        else{// channels requested are those requested by the user
            if(!_dag.isOutputAnOpenFXNode()){
                outChannels = writer->requestedChannels();
            }else{
                //openfx outputs can only output RGBA
                outChannels = Mask_RGBA;
            }
        }
        
        int counter = 0;
        gettimeofday(&_lastComputeFrameTime, 0);
        _sequence.clear();
        for (vector<int>::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
            Row* row = new Row(x,*it,r,outChannels);
            row->zoomedY(counter);
            // RowRunnable* worker = new RowRunnable(row,_dag.getOutput());
//            if(counter%10 == 0){
//            // UNCOMMENT to report progress.
//                QObject::connect(worker, SIGNAL(finished(int,int)), this ,SLOT(checkAndDisplayProgress(int,int)),Qt::QueuedConnection);
//            }
            _sequence.push_back(row);
            //  _threadPool->start(worker);
            ++counter;
        }
        _workerThreadsWatcher->setFuture(QtConcurrent::map(_sequence,boost::bind(metaEnginePerRow,_1,_dag.getOutput())));
        _workerThreadsWatcher->waitForFinished();
        //_threadPool->waitForDone();
        
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            //copying the frame data stored into the PBO to the viewer cache if it was a normal engine
            //This is done only if we run a sequence (i.e: playback) because the viewer cache isn't meant for
            //panning/zooming.
            viewer->getUiContext()->viewer->stopDisplayingProgressBar();
            if(!_lastRunArgs._sameFrame){
                FrameEntry* entry = ViewerCache::getViewerCache()->addFrame(key,
                                                                            _treeVersion,
                                                                            zoomFactor,
                                                                            exposure,
                                                                            lut,
                                                                            byteMode,
                                                                            _lastFrameInfos._textureRect,
                                                                            dataW,
                                                                            _dispW);
                
                if(entry){
                    memcpy(entry->getMappedFile()->data(),viewer->getUiContext()->viewer->getFrameData(),_lastFrameInfos._dataSize);
                    entry->removeReference(); // removing reference as we're done with the entry.
                }
            }
            
        }else if(!_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            /*if the output is a writer we actually start writing on disk now*/
            _dag.outputAsWriter()->startWriting();
        }
        engineLoop();
        
        /*endRenderAction for all openFX nodes*/
        for (DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            OfxNode* n = dynamic_cast<OfxNode*>(*it);
            if(n){
                OfxPointD renderScale;
                renderScale.x = renderScale.y = 1.0;
                n->effectInstance()->endRenderAction(0, 25, 1, true, renderScale);
            }
        }
        _model->getGeneralMutex()->unlock();

        
    } // end for(;;)
    
}
void VideoEngine::onProgressUpdate(int i){
    // cout << "progress: index = " << i ;
    if(i < (int)_lastFrameInfos._rows.size()){
        //   cout <<" y = "<< _lastFrameInfos._rows[i] << endl;
        if(_dag.outputAsViewer())
            checkAndDisplayProgress(_lastFrameInfos._rows[i],i);
    }
}

void VideoEngine::engineLoop(){
    if(_lastRunArgs._frameRequestIndex == 0 && _lastRunArgs._frameRequestsCount == 1 && !_lastRunArgs._sameFrame){
        _lastRunArgs._frameRequestsCount = 0;
    }else if(_lastRunArgs._frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
        --_lastRunArgs._frameRequestsCount;
    }
    ++_lastRunArgs._frameRequestIndex;//incrementing the frame counter
    
    if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
        _mutex->lock();
        emit doUpdateViewer();
        _openGLCondition->wait(_mutex);
        _mutex->unlock();
        
    }
    _lastRunArgs._fitToViewer = false;
    _lastRunArgs._recursiveCall = true;
    if(_autoSaveOnNextRun){
        _autoSaveOnNextRun = false;
        _model->getApp()->autoSave();
    }

}

void VideoEngine::updateViewer(){
    _mutex->lock();
    ViewerGL* viewer = _dag.outputAsViewer()->getUiContext()->viewer;
    viewer->copyPBOToRenderTexture(_lastFrameInfos._textureRect); // returns instantly
    _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
    if((_lastRunArgs._frameRequestIndex%24)==0){
        emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
    }
    _lastRunArgs._zoomFactor = viewer->getZoomFactor();
    
    updateDisplay(); // updating viewer & pixel aspect ratio if needed
    _openGLCondition->wakeOne();
    _mutex->unlock();
}
void VideoEngine::cachedEngine(){
    _mutex->lock();
    _dag.outputAsViewer()->cachedFrameEngine(_lastFrameInfos._cachedEntry);
    _openGLCondition->wakeOne();
    _mutex->unlock();

}
void VideoEngine::allocateFrameStorage(){
    _mutex->lock();
    _lastFrameInfos._dataSize = _dag.outputAsViewer()->getUiContext()->viewer->allocateFrameStorage(
                                                _lastFrameInfos._textureRect.w,
                                               _lastFrameInfos._textureRect.h);
    _openGLCondition->wakeOne();
    _mutex->unlock();
}







void VideoEngine::drawOverlay() const{
    if(_dag.getOutput())
        _drawOverlay(_dag.getOutput());
}

void VideoEngine::_drawOverlay(Node *output) const{
    output->drawOverlay();
    const Node::InputMap& inputs = output->getInputs();
    for(Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second)
            _drawOverlay(it->second);
    }
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
    ViewerGL* viewer  = _dag.outputAsViewer()->getUiContext()->viewer;
    int width = viewer->width();
    int height = viewer->height();
    double ap = viewer->displayWindow().pixel_aspect();
    if(ap > 1.f){
        glViewport (0, 0, width*ap, height);
    }else{
        glViewport (0, 0, width, height/ap);
    }
    viewer->updateGL();
}

void VideoEngine::startEngine(int nbFrames){
    if (dagHasInputs()) {
        render(nbFrames,true,true);
        
    }
}
void VideoEngine::repeatSameFrame(bool initViewer){
    if (dagHasInputs()) {
        if(_working){
            appendTask(_dag.outputAsViewer()->currentFrame(), 1, initViewer,_lastRunArgs._forward,true, _dag.getOutput(),&VideoEngine::_startEngine);
        }else{
            render(1,initViewer,true,true);
        }
    }
}

VideoEngine::VideoEngine(Model* model,QWaitCondition* openGLCondition,QMutex* mutex,QObject* parent):QThread(parent),
_model(model),
_working(false),
_aborted(false),
_mustQuit(false),
_treeVersionValid(false),
_loopMode(true),
_autoSaveOnNextRun(false),
_openGLCondition(openGLCondition),
_mutex(mutex)
{
    
    connect(this,SIGNAL(doUpdateViewer()),this,SLOT(updateViewer()));
    connect(this,SIGNAL(doCachedEngine()),this,SLOT(cachedEngine()));
    connect(this,SIGNAL(doFrameStorageAllocation()),this,SLOT(allocateFrameStorage()));
    connect(this, SIGNAL(doRunTasks()), this, SLOT(runTasks()));
    _workerThreadsWatcher = new QFutureWatcher<void>;
    connect(_workerThreadsWatcher, SIGNAL(progressValueChanged(int)), this, SLOT(onProgressUpdate(int)),Qt::QueuedConnection);
    /*Adjusting multi-threading for OpenEXR library.*/
    Imf::setGlobalThreadCount(QThread::idealThreadCount());
    
    _timer=new Timer();
    //_threadPool = new QThreadPool;
    
    
    
}

VideoEngine::~VideoEngine(){
    _workerThreadsWatcher->waitForFinished();
    //  _threadPool->waitForDone();
    _mutex->lock();
    _aborted = true;
    _startCondition.wakeOne();
    _mutex->unlock();
    wait();
    delete _workerThreadsWatcher;
    //  delete _threadPool;
    delete _timer;
    
}


void VideoEngine::setDesiredFPS(double d){
    _timer->setDesiredFrameRate(d);
}


void VideoEngine::abort(){
    _workerThreadsWatcher->cancel();
    _mutex->lock();
    _aborted=true;
    _mutex->unlock();
    if(_dag.outputAsViewer()){
        // _dag.outputAsViewer()->getUiContext()->viewer->forceUnmapPBO();
        emit engineStopped();
    }
}

void VideoEngine::startPause(bool c){
    if(_working){
        abort();
        return;
    }
    
    
    if(c && _dag.getOutput()){
        render(-1,false,true);
    }else{
        abort();
    }
}
void VideoEngine::startBackward(bool c){
    
    if(_dag.outputAsViewer()->getUiContext()->play_Forward_Button->isChecked()){
        abort();
        return;
    }
    if(c && _dag.getOutput()){
        render(-1,false,false);
    }else{
        abort();
    }
}
void VideoEngine::previousFrame(){
    if( _working){
        abort();
    }
    if(!_working)
        _startEngine(_dag.outputAsViewer()->currentFrame()-1, 1, false,false,false);
    else
        appendTask(_dag.outputAsViewer()->currentFrame()-1, 1,  false,_lastRunArgs._forward,false,_dag.getOutput(), &VideoEngine::_startEngine);
}

void VideoEngine::nextFrame(){
    if(_working){
        abort();
    }
    
    if(!_working)
        _startEngine(_dag.outputAsViewer()->currentFrame()+1, 1, false,true,false);
    else
        appendTask(_dag.outputAsViewer()->currentFrame()+1,  1,false,_lastRunArgs._forward,false,_dag.getOutput(), &VideoEngine::_startEngine);
}

void VideoEngine::firstFrame(){
    if( _working){
        abort();
    }
    
    if(!_working)
        _startEngine(_dag.outputAsViewer()->firstFrame(), 1, false,false,false);
    else
        appendTask(_dag.outputAsViewer()->firstFrame(), 1,  false,_lastRunArgs._forward,false,_dag.getOutput(),  &VideoEngine::_startEngine);
}

void VideoEngine::lastFrame(){
    if(_working){
        abort();
    }
    if(!_working)
        _startEngine(_dag.outputAsViewer()->lastFrame(), 1, false,true,false);
    else
        appendTask(_dag.outputAsViewer()->lastFrame(), 1,  false,_lastRunArgs._forward,false,_dag.getOutput(),  &VideoEngine::_startEngine);
}

void VideoEngine::previousIncrement(){
    if(_working){
        abort();
    }
    int frame = _dag.outputAsViewer()->currentFrame()- _dag.outputAsViewer()->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,false,false);
    else{
        appendTask(frame,1, false,_lastRunArgs._forward,false,_dag.getOutput(), &VideoEngine::_startEngine);
    }
    
    
}

void VideoEngine::nextIncrement(){
    if(_working){
        abort();
    }
    int frame = _dag.outputAsViewer()->currentFrame()+_dag.outputAsViewer()->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,true,false);
    else
        appendTask(frame,1, false,_lastRunArgs._forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
}

void VideoEngine::seekRandomFrame(int f){
    if(!_dag.getOutput() || _dag.getInputs().size()==0) return;
    
    if(_working){
        abort();
    }
    
    if(!_working){
        if(_lastRunArgs._frameRequestsCount == -1){
            _startEngine(f, -1, false,_lastRunArgs._forward,false);
        }else{
            _startEngine(f, 1, false,_lastRunArgs._forward,false);
        }
    }
    else{
        if(_lastRunArgs._frameRequestsCount == -1){
            appendTask(f, -1, false,_lastRunArgs._forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
        }else{
            appendTask(f, 1, false,_lastRunArgs._forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
        }
    }
}
void VideoEngine::recenterViewer(){
    if(_working){
        abort();
    }
    if(!_working){
        if(_lastRunArgs._frameRequestsCount == -1)
            _startEngine(_dag.outputAsViewer()->currentFrame(), -1, true,_lastRunArgs._forward,false);
        else
            _startEngine(_dag.outputAsViewer()->currentFrame(), 1, true,_lastRunArgs._forward,false);
    }else{
        appendTask(_dag.outputAsViewer()->currentFrame(), -1, true,_lastRunArgs._forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
    }
}



void VideoEngine::changeDAGAndStartEngine(OutputNode* output){
    abort();
    if(!_working){
        if(_dag.getOutput()){
            _changeDAGAndStartEngine(_dag.getOutput()->getTimeLine().currentFrame(), -1, false,true,false,output);
        }else{
            _changeDAGAndStartEngine(0, -1, false,true,false,output);
        }
    }else{
        if(_dag.getOutput()){
            appendTask(_dag.getOutput()->getTimeLine().currentFrame(), 1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
        }else{
            appendTask(0,1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
        }
    }
}

void VideoEngine::appendTask(int frameNB, int frameCount, bool initViewer,bool forward,bool sameFrame,OutputNode* output, VengineFunction func){
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

void VideoEngine::_startEngine(int frameNB,int frameCount,bool initViewer,bool forward,bool sameFrame,OutputNode* ){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        if(frameNB < _dag.outputAsViewer()->firstFrame() || frameNB > _dag.outputAsViewer()->lastFrame())
          return;
        _dag.outputAsViewer()->getTimeLine().seek(frameNB);

        render(frameCount,initViewer,forward,sameFrame);
        
    }
}

void VideoEngine::_changeDAGAndStartEngine(int , int frameCount, bool initViewer,bool,bool sameFrame,OutputNode* output){
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
        render(frameCount,initViewer,_lastRunArgs._forward,sameFrame);
}



void VideoEngine::changeTreeVersion(){
    
    if(!_dag.getOutput()){
        return;
    }
    Node* output = _dag.getOutput();
    std::vector<std::string> v;
    output->computeTreeHash(v);
    _treeVersion = output->hash().value();
    _treeVersionValid = true;
}


void VideoEngine::DAG::fillGraph(Node* n){
    if(std::find(_graph.begin(),_graph.end(),n)==_graph.end()){
        n->setMarked(false);
        _graph.push_back(n);
        if(n->isInputNode()){
            _inputs.push_back(n);
        }
    }
    
    const Node::InputMap& inputs = n->getInputs();
    for(Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second)
            fillGraph(it->second);
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
    const Node::InputMap& inputs = n->getInputs();
    for(Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second && !it->second->isMarked()){
            _depthCycle(it->second);
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

void VideoEngine::DAG::resetAndSort(OutputNode* out,bool isViewer){
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
        assert(*it);
        if(!(*it)->validate(forReal)){
            return false;
        }else{
            (*it)->setExecutingEngine(_output->getVideoEngine());
        }
    }
    return true;
}


int VideoEngine::DAG::firstFrame() const {
    return _output->info().firstFrame();
}
int VideoEngine::DAG::lastFrame() const {
    return _output->info().lastFrame();
}



void VideoEngine::resetAndMakeNewDag(OutputNode* output,bool isViewer){
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



bool VideoEngine::checkAndDisplayProgress(int y,int zoomedY){
    timeval now;
    gettimeofday(&now, 0);
    double t =  now.tv_sec  - _lastComputeFrameTime.tv_sec +
    (now.tv_usec - _lastComputeFrameTime.tv_usec) * 1e-6f;
    if(t >= 0.3){
        //   cout << zoomedY << endl;
        _dag.outputAsViewer()->getUiContext()->viewer->updateProgressOnViewer(_lastFrameInfos._textureRect, y,zoomedY);
        return true;
    }else{
        return false;
    }
}
void VideoEngine::quitEngineThread(){
    _mutex->lock();
    _mustQuit = true;
    _startCondition.wakeOne();
    _mutex->unlock();
}

void VideoEngine::toggleLoopMode(bool b){
    _loopMode = b;
}
