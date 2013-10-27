

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

#include "Engine/ViewerNode.h"
#include "Engine/OfxNode.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Hash64.h"
#include "Engine/Lut.h"
#include "Engine/FrameEntry.h"
#include "Engine/Row.h"
#include "Engine/MemoryFile.h"
#include "Engine/TimeLine.h"
#include "Writers/Writer.h"
#include "Readers/Reader.h"


#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/SpinBox.h"
#include "Gui/ViewerGL.h"
#include "Gui/Button.h"
#include "Gui/TimeLineGui.h"

#include "Global/AppManager.h"
#include "Global/MemoryInfo.h"

using namespace Powiter;
using std::make_pair;
using std::cout; using std::endl;

VideoEngine::VideoEngine(OutputNode* owner,QObject* parent)
: QThread(parent)
, _tree(owner)
, _abortBeingProcessedMutex()
, _abortBeingProcessed(false)
, _abortedRequestedCondition()
, _abortedRequestedMutex()
, _abortRequested(0)
, _mustQuitMutex()
, _mustQuit(false)
, _treeVersion(0)
, _treeVersionValid(false)
, _loopModeMutex()
, _loopMode(true)
, _restart(true)
, _startCondition()
, _startMutex()
, _startCount(0)
, _workingMutex()
, _working(false)
, _lastRequestedRunArgs()
, _currentRunArgs()
, _startRenderFrameTime()
, _timeline(owner->getApp()->getTimeLine())
{
    setTerminationEnabled();
    
}

VideoEngine::~VideoEngine() {
    abortRendering();
    quitEngineThread();
    {
        QMutexLocker startLocker(&_startMutex);
        _startCount = 1;
        _startCondition.wakeAll();
    }
    if(isRunning()){
        while(!isFinished()){
        }
    }
    
}


void VideoEngine::render(int frameCount,
                         bool refreshTree,
                         bool fitFrameToViewer,
                         bool forward,
                         bool sameFrame) {
    
    /*If the Tree was never built and we don't want to update the Tree, force an update
     so there's no null pointers hanging around*/
    if(!_tree.getOutput() && !refreshTree) refreshTree = true;
    
    
    
    /*setting the run args that are used by the run function*/
    _lastRequestedRunArgs._sameFrame = sameFrame;
    _lastRequestedRunArgs._fitToViewer = fitFrameToViewer;
    _lastRequestedRunArgs._recursiveCall = false;
    _lastRequestedRunArgs._forward = forward;
    _lastRequestedRunArgs._refreshTree = refreshTree;
    _lastRequestedRunArgs._frameRequestsCount = frameCount;
    _lastRequestedRunArgs._frameRequestIndex = 0;
    
    /*Starting or waking-up the thread*/
    QMutexLocker quitLocker(&_mustQuitMutex);
    if (!isRunning() && !_mustQuit) {
        start(HighestPriority);
    } else {
        QMutexLocker locker(&_startMutex);
        ++_startCount;
        _startCondition.wakeOne();
    }
}

bool VideoEngine::startEngine() {
    // don't allow "abort"s to be processed while starting engine by locking _abortBeingProcessedMutex
    QMutexLocker abortBeingProcessedLocker(&_abortBeingProcessedMutex);
    assert(!_abortBeingProcessed);

    {
        // let stopEngine run by unlocking abortBeingProcessedLocker()
        abortBeingProcessedLocker.unlock();
        QMutexLocker l(&_abortedRequestedMutex);
        if (_abortRequested > 0) {
            return false;
        }
        // make sure stopEngine is not running before releasing _abortedRequestedMutex
        abortBeingProcessedLocker.relock();
        assert(!_abortBeingProcessed);
    }
    
    _restart = false; /*we just called startEngine,we don't want to recall this function for the next frame in the sequence*/
    
    _currentRunArgs = _lastRequestedRunArgs;
    
    if(_currentRunArgs._refreshTree)
        _tree.refreshTree();/*refresh the tree*/
    
    
    ViewerNode* viewer = dynamic_cast<ViewerNode*>(_tree.getOutput()); /*viewer might be NULL if the output is smthing else*/
    const Tree::TreeContainer& inputs = _tree.getInputs();
    bool hasFrames = false;
    bool hasInputDifferentThanReader = false;
    for (Tree::TreeIterator it = inputs.begin() ; it != inputs.end() ; ++it) {
        Reader* r = dynamic_cast<Reader*>(*it);
        if (r) {
            if (r->hasFrames()) {
                hasFrames = true;
            }
        }else{
            hasInputDifferentThanReader = true;
        }
    }
    /*if there's no inputs or there's no input with frames to provide, disconnect the viewer associated*/
    if(!hasInputDifferentThanReader && !hasFrames){
        if(viewer)
            viewer->disconnectViewer();
        return false;
    }
    
    
    SequenceTime firstFrame,lastFrame;
    firstFrame = _tree.firstFrame();
    lastFrame = _tree.lastFrame();

    /*update the tree hash */
    bool oldVersionValid = _treeVersionValid;
    U64 oldVersion = 0;
    if (oldVersionValid) {
        oldVersion = getCurrentTreeVersion();
    }
    updateTreeVersion();
    /*If the Tree changed we clear the playback cache.*/
    if(!oldVersionValid || (_treeVersion != oldVersion)){
         // FIXME: the playback cache seems to be global to the application, why should we clear it here?
        //because the Tree changed, so the frame stored in memory are now useless for the playback.
        appPTR->clearPlaybackCache();
    }
    {
        QMutexLocker workingLocker(&_workingMutex);
        _working = true;
    }
    if(!_currentRunArgs._sameFrame)
        emit engineStarted(_currentRunArgs._forward);
    return true;

}
void VideoEngine::stopEngine() {
    /*reset the abort flag and wake up any thread waiting*/
    {
        // make sure startEngine is not running by locking _abortBeingProcessedMutex
        QMutexLocker abortBeingProcessedLocker(&_abortBeingProcessedMutex);
        _abortBeingProcessed = true; //_abortBeingProcessed is a dummy variable: it should be always false when stopeEngine is not running
        {
            QMutexLocker l(&_abortedRequestedMutex);
            _abortRequested = 0;
            _tree.lock();
            for (Tree::TreeIterator it = _tree.begin(); it != _tree.end(); ++it) {
                (*it)->setAborted(false);
            }
            _tree.unlock();
            _abortedRequestedCondition.wakeOne();
        }

        emit engineStopped();
        _currentRunArgs._frameRequestsCount = 0;
        _restart = true;
        {
            QMutexLocker workingLocker(&_workingMutex);
            _working = false;
        }
        _abortBeingProcessed = false;
    }
    /*pause the thread if needed*/
    {
        QMutexLocker locker(&_startMutex);
        while(_startCount <= 0) {
            _startCondition.wait(&_startMutex);
        }
        _startCount = 0;
    }
   
    
}
void VideoEngine::getFrameRange(int *firstFrame,int *lastFrame) const {
    if(_tree.isOutputAViewer()){
        *firstFrame = _timeline->firstFrame();
        *lastFrame = _timeline->lastFrame();
    }else{
        if(_tree.isOutputAnOpenFXNode()){
            OfxNode* ofxn = _tree.outputAsOpenFXNode();
            *firstFrame = ofxn->firstFrame();
            *lastFrame = ofxn->lastFrame();
        }else{
            Writer* w = _tree.outputAsWriter();
            *firstFrame = w->firstFrame();
            *lastFrame = w->lastFrame();
        }
    }

}

void VideoEngine::run(){
    
    for(;;){ // infinite loop
        {
            /*First-off, check if the node holding this engine got deleted
             in which case we must quit the engine.*/
            QMutexLocker locker(&_mustQuitMutex);
            if(_mustQuit) {
    
                return;
            }
        }
        
        /*If restart is on, start the engine. Restart is on for the 1st frame
         rendered of a sequence.*/
        if(_restart){
            if(!startEngine()){
                stopEngine();
                continue;
            }
        }

        /*beginRenderAction for all openFX nodes*/
        for (Tree::TreeIterator it = _tree.begin(); it!=_tree.end(); ++it) {
            OfxNode* n = dynamic_cast<OfxNode*>(*it);
            if(n) {
                OfxPointD renderScale;
                renderScale.x = renderScale.y = 1.0;
                assert(n->effectInstance());
                OfxStatus stat;
                stat = n->effectInstance()->beginRenderAction(0, 25, 1, true,renderScale);
                assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
            }
        }
        
        /*get the frame range*/
        int firstFrame = INT_MAX,lastFrame = INT_MIN, currentFrame = 0;
        ViewerNode* viewer = _tree.outputAsViewer();
        /*Get the frame range*/
        getFrameRange(&firstFrame, &lastFrame);
        _timeline->setFrameRange(firstFrame, lastFrame);

        if (!_currentRunArgs._recursiveCall) {
            
            /*if writing on disk and not a recursive call, move back the timeline cursor to the start*/
            if(!_tree.isOutputAViewer()){
                _writerCurrentFrame = firstFrame;
                currentFrame = _writerCurrentFrame;
            }else{
                currentFrame = _timeline->currentFrame();
            }
        } else if(!_currentRunArgs._sameFrame && _currentRunArgs._recursiveCall){
            if(_tree.isOutputAViewer()){
                if(_currentRunArgs._forward){
                    _timeline->incrementCurrentFrame();
                    currentFrame = _timeline->currentFrame();
                    int rightBound,leftBound;
                    leftBound = viewer->getUiContext()->frameSeeker->firstFrame();
                    rightBound = viewer->getUiContext()->frameSeeker->lastFrame();
                    if(currentFrame > lastFrame || currentFrame > rightBound){
                        QMutexLocker loopModeLocker(&_loopModeMutex);
                        if(_loopMode && _tree.isOutputAViewer()){ // loop only for a viewer
                            currentFrame = leftBound;
                            _timeline->seekFrame(currentFrame);
                        }else{
                            loopModeLocker.unlock();
                            stopEngine();
                            continue;
                        }
                    }
                    
                }else{
                    _timeline->decrementCurrentFrame();
                    currentFrame = _timeline->currentFrame();
                    int rightBound,leftBound;
                    leftBound = viewer->getUiContext()->frameSeeker->firstFrame();
                    rightBound = viewer->getUiContext()->frameSeeker->lastFrame();
                    if(currentFrame < firstFrame || currentFrame < leftBound){
                        QMutexLocker loopModeLocker(&_loopModeMutex);
                        if(_loopMode && _tree.isOutputAViewer()){ //loop only for a viewer
                            currentFrame = rightBound;
                            _timeline->seekFrame(currentFrame);
                        }else{
                            loopModeLocker.unlock();
                            stopEngine();
                            continue;
                        }
                    }
                }
            }else{
                ++_writerCurrentFrame;
                currentFrame = _writerCurrentFrame;
                if(currentFrame > lastFrame){
                    stopEngine();
                    continue;
                }
            }
        }
        
        /*Check whether we need to stop the engine or not for various reasons.
         */
        {
            QMutexLocker locker(&_abortedRequestedMutex);
            if(_abortRequested || // #1 aborted by the user

               (_tree.isOutputAViewer() // #2 the Tree contains only 1 frame and we rendered it
                &&  _currentRunArgs._recursiveCall
                && _timeline->lastFrame() == _timeline->firstFrame()
                && _currentRunArgs._frameRequestsCount == -1
                && _currentRunArgs._frameRequestIndex == 1)

               || _currentRunArgs._frameRequestsCount == 0) // #3 the sequence ended and it was not an infinite run
            {
                locker.unlock();
                stopEngine();
                continue;
            }
        }
        
        /*pre process frame*/
        
        Status stat = _tree.preProcessFrame(currentFrame);
        if(stat == StatFailed){
            stopEngine();
            continue;
        }
        /*get the time at which we started rendering the frame*/
        gettimeofday(&_startRenderFrameTime, 0);
        if (_tree.isOutputAViewer() && !_tree.isOutputAnOpenFXNode()) {
            stat = viewer->renderViewer(currentFrame, _currentRunArgs._fitToViewer);
            if(stat == StatFailed){
                viewer->disconnectViewer();
            }
        }else if(!_tree.isOutputAViewer() && !_tree.isOutputAnOpenFXNode()){
           stat =  _tree.outputAsWriter()->renderWriter(currentFrame);
        }else{
            RenderScale scale;
            scale.x = scale.y = 1.;
            Box2D rod;
            OfxNode* output = _tree.outputAsOpenFXNode();
            stat = output->getRegionOfDefinition(currentFrame, &rod);
            if(stat != StatFailed){
                output->renderRoI(currentFrame, scale, rod);
            }
            
        }

        if(stat == StatFailed){
            stopEngine();
            continue;
        }
        
        /*The frame has been rendered , we call engineLoop() which will reset all the flags,
         update viewers
         and appropriately increment counters for the next frame in the sequence.*/
        emit frameRendered(currentFrame);

        if(_currentRunArgs._frameRequestIndex == 0 && _currentRunArgs._frameRequestsCount == 1 && !_currentRunArgs._sameFrame){
            _currentRunArgs._frameRequestsCount = 0;
        }else if(_currentRunArgs._frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
            --_currentRunArgs._frameRequestsCount;
        }
        ++_currentRunArgs._frameRequestIndex;//incrementing the frame counter
        
        _currentRunArgs._fitToViewer = false;
        _currentRunArgs._recursiveCall = true;
        
        /*endRenderAction for all openFX nodes*/
        for (Tree::TreeIterator it = _tree.begin(); it!=_tree.end(); ++it) {
            OfxNode* n = dynamic_cast<OfxNode*>(*it);
            if(n){
                OfxPointD renderScale;
                renderScale.x = renderScale.y = 1.0;
                assert(n->effectInstance());
                OfxStatus stat;
                stat = n->effectInstance()->endRenderAction(0, 25, 1, true, renderScale);
                assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
            }
        }
        
    } // end for(;;)
    
}
void VideoEngine::onProgressUpdate(int /*i*/){
    // cout << "progress: index = " << i ;
//    if(i < (int)_currentFrameInfos._rows.size()){
//        //  cout <<" y = "<< _lastFrameInfos._rows[i] << endl;
//        checkAndDisplayProgress(_currentFrameInfos._rows[i],i);
//    }
}





void VideoEngine::updateDisplay(){
    ViewerGL* viewer  = _tree.outputAsViewer()->getUiContext()->viewer;
    int width = viewer->width();
    int height = viewer->height();
    double ap = viewer->getDisplayWindow().getPixelAspect();
    if(ap > 1.f){
        glViewport (0, 0, (int)(width*ap), height);
    }else{
        glViewport (0, 0, width, (int)(height/ap));
    }
    viewer->updateColorPicker();
    viewer->updateGL();
}






void VideoEngine::abortRendering(){
    {
        QMutexLocker workingLocker(&_workingMutex);
        if(!_working){
            return;
        }
    }
    {
        QMutexLocker locker(&_abortedRequestedMutex);
        ++_abortRequested;
        _tree.lock();
        for (Tree::TreeIterator it = _tree.begin(); it != _tree.end(); ++it) {
            (*it)->setAborted(true);
        }
        _tree.unlock();
        // _abortedCondition.wakeOne();
    }
}


void VideoEngine::refreshAndContinueRender(bool initViewer){
    bool wasPlaybackRunning;
    {
        QMutexLocker startedLocker(&_workingMutex);
        wasPlaybackRunning = _working && _currentRunArgs._frameRequestsCount == -1;
    }
    if(wasPlaybackRunning){
        render(-1,false,initViewer,_currentRunArgs._forward,false);
    }else{
        render(1,false,initViewer,_currentRunArgs._forward,true);
    }
}
void VideoEngine::updateTreeAndContinueRender(bool initViewer){
    bool wasPlaybackRunning;
    {
        QMutexLocker startedLocker(&_workingMutex);
        wasPlaybackRunning = _working && _currentRunArgs._frameRequestsCount == -1;
    }
    if(wasPlaybackRunning){
        render(-1,true,initViewer,_currentRunArgs._forward,false);
    }else{
        render(1,true,initViewer,_currentRunArgs._forward,true);
    }
}



void VideoEngine::updateTreeVersion(){
    
    if(!_tree.getOutput()){
        return;
    }
    OutputNode* output = _tree.getOutput();
    std::vector<std::string> v;
    output->computeTreeHash(v);
    _treeVersion = output->hash().value();
    _treeVersionValid = true;
}




Tree::Tree(OutputNode* output):
_output(output)
,_isViewer(false)
,_isOutputOpenFXNode(false)
,_treeMutex(QMutex::Recursive) /*recursive lock*/
,_firstFrame(0)
,_lastFrame(0)
{
    assert(output);
    
}


void Tree::clearGraph(){
    for(TreeContainer::const_iterator it = _sorted.begin();it!=_sorted.end();++it) {
        (*it)->setMarkedByTopologicalSort(false);
        QObject::disconnect((*it), SIGNAL(frameRangeChanged(int,int)), this, SLOT(onInputFrameRangeChanged(int,int)));
    }
    _output->setMarkedByTopologicalSort(false);
    _sorted.clear();
    _inputs.clear();
    
}

void Tree::refreshTree(){
    QMutexLocker dagLocker(&_treeMutex);
    _isViewer = dynamic_cast<ViewerNode*>(_output) != NULL;
    _isOutputOpenFXNode = _output->isOpenFXNode();
    
    
    /*unmark all nodes already present in the graph*/
    clearGraph();
    fillGraph(_output);
    
    /*clear the marked flags for the sort*/
    for(TreeContainer::const_iterator it = _sorted.begin();it!=_sorted.end();++it) {
        (*it)->setMarkedByTopologicalSort(false);
    }
    _output->setMarkedByTopologicalSort(false);
    
    /*update frame range*/
    onInputFrameRangeChanged(0, 0); //non signifiant args
}
void Tree::fillGraph(Node* n){
    
    /*call fillGraph recursivly on all the node's inputs*/
    const Node::InputMap& inputs = n->getInputs();
    for(Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second){
            /*if the node is a viewer, we're only interested in the active input*/
            if(n->className() == "Viewer"){
                ViewerNode* v = dynamic_cast<ViewerNode*>(n);
                if (it->first!=v->activeInput()) {
                    continue;
                }
            }
            fillGraph(it->second);
        }
    }
    if(!n->isMarkedByTopologicalSort()){
        n->setMarkedByTopologicalSort(true);
        _sorted.push_back(n);
        if(n->isInputNode()){
            _inputs.push_back(n);
            QObject::connect(n, SIGNAL(frameRangeChanged(int,int)), this, SLOT(onInputFrameRangeChanged(int,int)));
        }
    }
}


ViewerNode* Tree::outputAsViewer() const {
    if(_output && _isViewer)
        return dynamic_cast<ViewerNode*>(_output);
    else
        return NULL;
}

Writer* Tree::outputAsWriter() const {
    if(_output && !_isViewer)
        return dynamic_cast<Writer*>(_output);
    else
        return NULL;
}
OfxNode* Tree::outputAsOpenFXNode() const{
    if(_output && _isOutputOpenFXNode){
        return dynamic_cast<OfxNode*>(_output);
    }else{
        return NULL;
    }
}


void Tree::debug(){
    cout << "Topological ordering of the Tree is..." << endl;
    for(Tree::TreeIterator it = begin(); it != end() ;++it) {
        cout << (*it)->getName() << endl;
    }
}

/*sets infos accordingly across all the Tree*/
Powiter::Status Tree::preProcessFrame(SequenceTime time){
    /*Validating the Tree in topological order*/
    for (TreeIterator it = begin(); it != end(); ++it) {
        Powiter::Status st = (*it)->preProcessFrame(time);
        if(st == Powiter::StatFailed){
            return st;
        }
    }
    return Powiter::StatOK;
}
void Tree::onInputFrameRangeChanged(int,int){
    _output->getFrameRange(&_firstFrame,&_lastFrame);
    _output->getApp()->getTimeLine()->setFrameRange(_firstFrame ,_lastFrame);
    
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



bool VideoEngine::checkAndDisplayProgress(int /*y*/,int/* zoomedY*/){
//    timeval now;
//    gettimeofday(&now, 0);
//    double t =  now.tv_sec  - _startRenderFrameTime.tv_sec +
//    (now.tv_usec - _startRenderFrameTime.tv_usec) * 1e-6f;
//    if(t >= 0.5){
//        if(_tree.isOutputAViewer()){
//            _tree.outputAsViewer()->getUiContext()->viewer->updateProgressOnViewer(_currentFrameInfos._textureRect, y,zoomedY);
//        }else{
//            emit progressChanged(floor(((double)y/(double)_currentFrameInfos._rows.size())*100));
//        }
//        return true;
//    }else{
//        return false;
//    }
    return false;
}
void VideoEngine::quitEngineThread(){
    {
        QMutexLocker locker(&_mustQuitMutex);
        _mustQuit = true;
    }
    {
        QMutexLocker locker(&_startMutex);
        ++_startCount;
        _startCondition.wakeAll();
    }
}

void VideoEngine::toggleLoopMode(bool b){
    _loopMode = b;
}


bool VideoEngine::isWorking() const {
    QMutexLocker workingLocker(&_workingMutex);
    return _working;
}


