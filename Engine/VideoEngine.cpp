//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

#include "VideoEngine.h"

#ifndef __NATRON_WIN32__
#include <unistd.h> //Provides STDIN_FILENO
#endif
#include <iterator>
#include <cassert>
#include <QtCore/QMutex>
#include <QtGui/QVector2D>
#include <QAction>
#include <QtCore/QThread>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QtCore/QSocketNotifier>

#include "Engine/ViewerInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Hash64.h"
#include "Engine/Project.h"
#include "Engine/Lut.h"
#include "Engine/FrameEntry.h"
#include "Engine/Row.h"
#include "Engine/MemoryFile.h"
#include "Engine/TimeLine.h"
#include "Engine/Timer.h"
#include "Engine/Log.h"

#include "Engine/EffectInstance.h"

#include "Global/AppManager.h"
#include "Global/MemoryInfo.h"


#define NATRON_FPS_REFRESH_RATE 10


using namespace Natron;
using std::make_pair;
using std::cout; using std::endl;


VideoEngine::VideoEngine(Natron::OutputEffectInstance* owner,QObject* parent)
    : QThread(parent)
    , _tree(owner)
    , _treeMutex(QMutex::Recursive)
    , _threadStarted(false)
    , _abortBeingProcessedMutex()
    , _abortBeingProcessed(false)
    , _abortedRequestedCondition()
    , _abortedRequestedMutex()
    , _abortRequested(0)
    , _mustQuitCondition()
    , _mustQuitMutex()
    , _mustQuit(false)
    , _treeVersionValid(false)
    , _loopModeMutex()
    , _loopMode(true)
    , _restart(true)
    , _startCondition()
    , _startMutex()
    , _startCount(0)
    , _workingMutex()
    , _working(false)
    , _timerMutex()
    , _timer(new Timer)
    , _timerFrameCount(0)
    , _lastRequestedRunArgs()
    , _currentRunArgs()
    , _startRenderFrameTime()
    , _timeline(owner->getNode()->getApp()->getTimeLine())
    , _getFrameRangeCond()
    , _getFrameRangeMutex()
    , _gettingFrameRange(false)
    , _firstFrame(0)
    , _lastFrame(0)
    , _doingARenderSingleThreaded(false)
{
    QObject::connect(this, SIGNAL(mustGetFrameRange()), this, SLOT(getFrameRange()));
}

VideoEngine::~VideoEngine() {
    _threadStarted = false;
}

void VideoEngine::quitEngineThread(){
    bool isThreadStarted = false;
    {
        QMutexLocker quitLocker(&_mustQuitMutex);
        isThreadStarted = _threadStarted;
    }
    if(isThreadStarted){
        abortRendering();
        {
            QMutexLocker locker(&_mustQuitMutex);
            _mustQuit = true;
        }
        {
            QMutexLocker locker(&_startMutex);
            ++_startCount;
            _startCondition.wakeAll();
        }
        
        {
            QMutexLocker locker(&_mustQuitMutex);
            while(_mustQuit){
                _mustQuitCondition.wait(&_mustQuitMutex);
            }
        }
        QMutexLocker quitLocker(&_mustQuitMutex);
        _threadStarted = false;
    }
}

void VideoEngine::render(int frameCount,
                         bool seekTimeline,
                         bool refreshTree,
                         bool fitFrameToViewer,
                         bool forward,
                         bool sameFrame,
                         bool forcePreview) {
    
    
    /*If the Tree was never built and we don't want to update the Tree, force an update
     so there's no null pointers hanging around*/
    if(!_tree.getOutput() && !refreshTree) refreshTree = true;
    
    
    /*setting the run args that are used by the run function*/
    _lastRequestedRunArgs._sameFrame = sameFrame;
    _lastRequestedRunArgs._fitToViewer = fitFrameToViewer;
    _lastRequestedRunArgs._recursiveCall = false;
    _lastRequestedRunArgs._forward = forward;
    _lastRequestedRunArgs._refreshTree = refreshTree;
    _lastRequestedRunArgs._seekTimeline = seekTimeline;
    _lastRequestedRunArgs._frameRequestsCount = frameCount;
    _lastRequestedRunArgs._frameRequestIndex = 0;
    _lastRequestedRunArgs._forcePreview = forcePreview;
    
    if (appPTR->getCurrentSettings()->isMultiThreadingDisabled()) {
        runSameThread();
    } else {
        
        /*Starting or waking-up the thread*/
        QMutexLocker quitLocker(&_mustQuitMutex);
        if (!_threadStarted && !_mustQuit) {
            start(HighestPriority);
            _threadStarted = true;
        } else {
            QMutexLocker locker(&_startMutex);
            ++_startCount;
            _startCondition.wakeOne();
        }
    }
}

bool VideoEngine::startEngine(bool singleThreaded) {
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
    

    
    
    if(!_tree.isOutputAViewer()){
        
        if (!singleThreaded) {
            {
                QMutexLocker l(&_abortedRequestedMutex);
                if (_abortRequested > 0) {
                    return false;
                }
            }
            QMutexLocker l(&_getFrameRangeMutex);
            _gettingFrameRange = true;
            emit mustGetFrameRange();
            while (_gettingFrameRange) {
                _getFrameRangeCond.wait(&_getFrameRangeMutex);
            }
        } else {
            getFrameRange();
        }
        
        Natron::OutputEffectInstance* output = dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput());
        output->setFirstFrame(_firstFrame);
        output->setLastFrame(_lastFrame);
    }

    
    if(_currentRunArgs._refreshTree)
        refreshTree();/*refresh the tree*/
    
    
    ViewerInstance* viewer = dynamic_cast<ViewerInstance*>(_tree.getOutput()); /*viewer might be NULL if the output is smthing else*/
    
    bool hasInput = false;
    for (RenderTree::TreeIterator it = _tree.begin() ; it != _tree.end() ; ++it) {
        if(it->first->isInputNode()){
            hasInput = true;
            break;
        }
    }

    if(!hasInput){
        if(viewer)
            viewer->disconnectViewer();
        return false;
    }
    
    {
        QMutexLocker workingLocker(&_workingMutex);
        _working = true;
    }

    /*beginRenderAction for all openFX nodes*/
    for (RenderTree::TreeIterator it = _tree.begin(); it!=_tree.end(); ++it) {
        OfxEffectInstance* n = dynamic_cast<OfxEffectInstance*>(it->second);
        if(n) {
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            assert(n->effectInstance());
            OfxStatus stat;
            stat = n->effectInstance()->beginRenderAction(_timeline->leftBound(),_timeline->rightBound(), //frame range
                                                          1, // frame step
                                                          true, //is interactive
                                                          renderScale); //scale
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
            
        }
    }
    
    if(!_currentRunArgs._sameFrame){
        emit engineStarted(_currentRunArgs._forward,_currentRunArgs._frameRequestsCount);
        _timer->playState = RUNNING; /*activating the timer*/

    }
    if(_tree.getOutput()->getApp()->isBackground()){
        appPTR->writeToOutputPipe(kRenderingStartedLong, kRenderingStartedShort);
    }
    return true;

}
bool VideoEngine::stopEngine() {
    /*reset the abort flag and wake up any thread waiting*/
    {
        // make sure startEngine is not running by locking _abortBeingProcessedMutex
        QMutexLocker abortBeingProcessedLocker(&_abortBeingProcessedMutex);
        _abortBeingProcessed = true; //_abortBeingProcessed is a dummy variable: it should be always false when stopeEngine is not running
        {
            QMutexLocker l(&_abortedRequestedMutex);
            _abortRequested = 0;
            
            /*Refresh preview for all nodes that have preview enabled & set the aborted flag to false.
             ONLY If we're not rendering the same frame (i.e: not panning & zooming) and the user is not scrubbing
             .*/
            bool shouldRefreshPreview = (_tree.getOutput()->getApp()->shouldRefreshPreview() && !_currentRunArgs._sameFrame)
            || _currentRunArgs._forcePreview;
            for (RenderTree::TreeIterator it = _tree.begin(); it != _tree.end(); ++it) {
                if(it->second->isPreviewEnabled() && shouldRefreshPreview){
                    it->second->getNode()->computePreviewImage(_timeline->currentFrame());
                }
                it->second->setAborted(false);
            }
            
            
            _abortedRequestedCondition.wakeOne();
        }

        emit engineStopped();
        
        _currentRunArgs._frameRequestsCount = 0;
        _restart = true;
        _timer->playState = PAUSE;
        

        {
            QMutexLocker workingLocker(&_workingMutex);
            _working = false;
        }

        _abortBeingProcessed = false;

    }
    
    
    /*endRenderAction for all openFX nodes*/
    for (RenderTree::TreeIterator it = _tree.begin(); it!=_tree.end(); ++it) {
        OfxEffectInstance* n = dynamic_cast<OfxEffectInstance*>(it->second);
        if(n){
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            assert(n->effectInstance());
            OfxStatus stat;
            stat = n->effectInstance()->endRenderAction(_timeline->leftBound(),_timeline->rightBound(), 1, true, renderScale);
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        }
    }
    
    if(_tree.getOutput()->getApp()->isBackground()){
        _tree.getOutput()->getApp()->notifyRenderFinished(dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput()));
        _mustQuit = false;
        _mustQuitCondition.wakeAll();
        _threadStarted = false;
        return true;
    }


    {
        QMutexLocker locker(&_mustQuitMutex);
        if (_mustQuit) {
            _mustQuit = false;
            _mustQuitCondition.wakeAll();
            _threadStarted = false;
            return true;
        }
    }

    return false;
    
}

void VideoEngine::run(){
    
    for(;;){ // infinite loop
        {
            /*First-off, check if the node holding this engine got deleted
             in which case we must quit the engine.*/
            QMutexLocker locker(&_mustQuitMutex);
            if(_mustQuit) {
                _mustQuit = false;
                _mustQuitCondition.wakeAll();
                return;
            }
        }
        
        /*If restart is on, start the engine. Restart is on for the 1st frame
         rendered of a sequence.*/
        if(_restart){
            if(!startEngine(false)){
                if(stopEngine())
                    return;
                continue;
            }
        }
        
        iterateKernel(false);
        
        if (stopEngine()) {
            return;
        } else {
            /*pause the thread if needed*/
            {
                QMutexLocker locker(&_startMutex);
                while(_startCount <= 0) {
                    _startCondition.wait(&_startMutex);
                }
                _startCount = 0;
            }

        }
       
    } // end for(;;)
    
}

void VideoEngine::runSameThread() {
    
    if (_doingARenderSingleThreaded) {
        return;
    } else {
        _doingARenderSingleThreaded = true;
    }
    
    if (!startEngine(true)) {
        stopEngine();
    } else {
        QCoreApplication::processEvents();
        iterateKernel(true);
        QCoreApplication::processEvents();
        stopEngine();
    }
    
    _doingARenderSingleThreaded = false;
}

void VideoEngine::iterateKernel(bool singleThreaded) {
    for(;;){ // infinite loop
        
        {
            QMutexLocker locker(&_abortedRequestedMutex);
            if(_abortRequested > 0) {
                locker.unlock();
                return;
            }
        }
        
        Natron::OutputEffectInstance* output = dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput());
        assert(output);
        ViewerInstance* viewer = _tree.outputAsViewer();
        
        /*update the tree hash */
        _tree.refreshKnobsAndHashAndClearPersistentMessage();
        
        
        if(viewer){
            if (singleThreaded) {
                getFrameRange();
            } else {
                QMutexLocker l(&_getFrameRangeMutex);
                _gettingFrameRange = true;
                emit mustGetFrameRange();
                while (_gettingFrameRange) {
                    _getFrameRangeCond.wait(&_getFrameRangeMutex);
                }
                
            }
            _timeline->setFrameRange(_firstFrame, _lastFrame);
        }
        
        int firstFrame,lastFrame;
        if (viewer) {
            firstFrame = _timeline->leftBound();
            lastFrame = _timeline->rightBound();
        } else {
            firstFrame = output->getFirstFrame();
            lastFrame = output->getLastFrame();
        }
        
        //////////////////////////////
        // Set the current frame
        //
        int currentFrame = 0;
        if (!_currentRunArgs._recursiveCall) {
            
            /*if writing on disk and not a recursive call, move back the timeline cursor to the start*/
            if (viewer) {
                currentFrame = _timeline->currentFrame();
            } else {
                output->setCurrentFrame(firstFrame);
                currentFrame = firstFrame;
            }
            
        } else if(!_currentRunArgs._sameFrame && _currentRunArgs._seekTimeline) {
            assert(_currentRunArgs._recursiveCall); // we're in the else part
            if (!viewer) {
                output->setCurrentFrame(output->getCurrentFrame()+1);
                currentFrame = output->getCurrentFrame();
                if(currentFrame > lastFrame){
                    return;
                }
            } else {
                // viewer
                assert(viewer);
                if (_currentRunArgs._forward) {
                    currentFrame = _timeline->currentFrame();
                    if (currentFrame < lastFrame) {
                        _timeline->incrementCurrentFrame(output);
                        ++currentFrame;
                    } else {
                        QMutexLocker loopModeLocker(&_loopModeMutex);
                        if (_loopMode) { // loop only for a viewer
                            currentFrame = firstFrame;
                            _timeline->seekFrame(currentFrame,output);
                        } else {
                            loopModeLocker.unlock();
                            return;
                        }
                    }
                    
                } else {
                    currentFrame = _timeline->currentFrame();
                    if (currentFrame > firstFrame) {
                        _timeline->decrementCurrentFrame(output);
                        --currentFrame;
                    } else {
                        QMutexLocker loopModeLocker(&_loopModeMutex);
                        if (_loopMode) { //loop only for a viewer
                            currentFrame = lastFrame;
                            _timeline->seekFrame(currentFrame,output);
                        } else {
                            loopModeLocker.unlock();
                            return;
                        }
                    }
                }
            }
        }
        
        ///////////////////////////////
        // Check whether we need to stop the engine or not for various reasons.
        //
        {
            QMutexLocker locker(&_abortedRequestedMutex);
            if(_abortRequested > 0 || // #1 aborted by the user
               
               (_tree.isOutputAViewer() // #2 the Tree contains only 1 frame and we rendered it
                &&  _currentRunArgs._recursiveCall
                &&  firstFrame == lastFrame
                && _currentRunArgs._frameRequestsCount == -1
                && _currentRunArgs._frameRequestIndex == 1)
               
               || _currentRunArgs._frameRequestsCount == 0) // #3 the sequence ended and it was not an infinite run
            {
                locker.unlock();
                return;
            }
        }
        
        ////////////////////////
        // Render currentFrame
        //
        // if the output is a writer, _tree.outputAsWriter() returns a valid pointer/
        Status stat;
        try {
            stat =  renderFrame(currentFrame,singleThreaded);
        } catch (const std::exception &e) {
            std::stringstream ss;
            ss << "Error while rendering" << " frame " << currentFrame << ": " << e.what();
            Natron::errorDialog("Error while rendering", ss.str());
            if (viewer) {
                viewer->disconnectViewer();
            }
            return;
            
        }
        
        if(stat == StatFailed){
            if (viewer) {
                viewer->disconnectViewer();
            }
            return;
        }
        
        
        /*The frame has been rendered , we call engineLoop() which will reset all the flags,
         update viewers
         and appropriately increment counters for the next frame in the sequence.*/
        emit frameRendered(currentFrame);
        if(_tree.getOutput()->getApp()->isBackground()){
            QString frameStr = QString::number(currentFrame);
            appPTR->writeToOutputPipe(kFrameRenderedStringLong + frameStr,kFrameRenderedStringShort + frameStr);
        }
        
        if (singleThreaded) {
            QCoreApplication::processEvents();
        }
        
        if(_currentRunArgs._frameRequestIndex == 0 && _currentRunArgs._frameRequestsCount == 1 && !_currentRunArgs._sameFrame){
            _currentRunArgs._frameRequestsCount = 0;
        }else if(_currentRunArgs._frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
            --_currentRunArgs._frameRequestsCount;
        }
        ++_currentRunArgs._frameRequestIndex;//incrementing the frame counter
        
        _currentRunArgs._fitToViewer = false;
        _currentRunArgs._recursiveCall = true;
    } // end for(;;)
}

Natron::Status VideoEngine::renderFrame(SequenceTime time,bool singleThreaded){
    Status stat;
    
  
    /*pre process frame*/
    
    stat = _tree.preProcessFrame(time);
    if (stat == StatFailed) {
        return stat;
        //don't throw an exception here, this is regular behaviour when a mandatory input is not connected.
        // We don't want to popup a dialog everytime it occurs
        //      throw std::runtime_error("PreProcessFrame failed, mandatory inputs are probably not connected.");
    }
    
    /*get the time at which we started rendering the frame*/
    gettimeofday(&_startRenderFrameTime, 0);
    if (_tree.isOutputAViewer() && !_tree.isOutputAnOpenFXNode()) {
        
        stat = _tree.outputAsViewer()->renderViewer(time, _currentRunArgs._fitToViewer,singleThreaded);
        
        if (!_currentRunArgs._sameFrame) {
            QMutexLocker timerLocker(&_timerMutex);
            _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
            if ((_timerFrameCount % NATRON_FPS_REFRESH_RATE) == 0) {
                emit fpsChanged(_timer->actualFrameRate(),_timer->getDesiredFrameRate()); // refreshing fps display on the GUI
                _timerFrameCount = 1; //reseting to 1
            } else {
                ++_timerFrameCount;
            }
            
        }
        
    } else {
        RenderScale scale;
        scale.x = scale.y = 1.;
        RectI rod;
        stat = _tree.getOutput()->getRegionOfDefinition(time, &rod);
        if(stat != StatFailed){
            int viewsCount = _tree.getOutput()->getApp()->getProjectViewsCount();
            for(int i = 0; i < viewsCount;++i){
                // Do not catch exceptions: if an exception occurs here it is probably fatal, since
                // it comes from Natron itself. All exceptions from plugins are already caught
                // by the HostSupport library.
                (void)_tree.getOutput()->renderRoI(time, scale,i ,rod);
            }
        }
        
    }
//    
//    if (stat == StatFailed) {
//        throw std::runtime_error("Render failed");
//    }
    return stat;

}

void VideoEngine::onProgressUpdate(int /*i*/){
    // cout << "progress: index = " << i ;
    //    if(i < (int)_currentFrameInfos._rows.size()){
    //        //  cout <<" y = "<< _lastFrameInfos._rows[i] << endl;
    //        checkAndDisplayProgress(_currentFrameInfos._rows[i],i);
    //    }
}


void VideoEngine::abortRendering(){
    {
        if(!isWorking()){
            return;
        }
    }
    {
        QMutexLocker locker(&_abortedRequestedMutex);
        ++_abortRequested;
        
        /*Note that we set the aborted flag in from output to inputs otherwise some aborted images
        might get rendered*/
        for (RenderTree::TreeReverseIterator it = _tree.rbegin(); it != _tree.rend(); ++it) {
            it->second->setAborted(true);
        }
        if(_tree.isOutputAViewer())
            _tree.outputAsViewer()->wakeUpAnySleepingThread();
        
        ///also wake up the run() thread if it is waiting for getFrameRange
        _gettingFrameRange = false;
        _getFrameRangeCond.wakeOne();
        
        if (QThread::currentThread() != this && isRunning()) {
            while (_abortRequested > 0) {
                _abortedRequestedCondition.wait(&_abortedRequestedMutex);
            }
        }
        
    }
}


void VideoEngine::refreshAndContinueRender(bool initViewer,bool forcePreview){
    //the changes will occur upon the next frame rendered. If the playback is running indefinately
    //we're sure that there will be a refresh. If the playback is for a determined amount of frame
    //we've to make sure the playback is not rendering the last frame, in which case we wouldn't see
    //the last changes.
    //The default case is if the playback is not running: just render the current frame

    bool isPlaybackRunning = isWorking() && (_currentRunArgs._frameRequestsCount == -1 ||
                                             (_currentRunArgs._frameRequestsCount > 1 && _currentRunArgs._frameRequestIndex < _currentRunArgs._frameRequestsCount - 1));
    if(!isPlaybackRunning){
        render(1,false,false,initViewer,_currentRunArgs._forward,true,forcePreview);
    }
}
void VideoEngine::updateTreeAndContinueRender(bool initViewer){
    //this is a bit more trickier than refreshAndContinueRender, we've to stop
    //the playback, and request a new render
    bool isPlaybackRunning = isWorking() && (_currentRunArgs._frameRequestsCount == -1 ||
                                             (_currentRunArgs._frameRequestsCount > 1 && _currentRunArgs._frameRequestIndex < _currentRunArgs._frameRequestsCount - 1));

    if(isPlaybackRunning){
        int count = _currentRunArgs._frameRequestsCount == - 1 ? -1 :
                                                                 _currentRunArgs._frameRequestsCount - _currentRunArgs._frameRequestIndex ;
        abortRendering();
        render(count,true,true,initViewer,_currentRunArgs._forward,false,false);
    }else{
        render(1,false,true,initViewer,_currentRunArgs._forward,true,false);
    }
}


RenderTree::RenderTree(EffectInstance *output):
    _output(output)
  ,_sorted()
  ,_isViewer(false)
  ,_isOutputOpenFXNode(false)
  ,_firstFrame(0)
  ,_lastFrame(0)
  ,_treeVersionValid(false)
  ,_renderOutputFormat()
  ,_projectViewsCount(1)
{
    assert(output);
}


void RenderTree::clearGraph(){
    for(TreeContainer::const_iterator it = _sorted.begin();it!=_sorted.end();++it) {
        (*it).second->setMarkedByTopologicalSort(false);
    }
    _sorted.clear();
}

void RenderTree::refreshTree(int knobsAge){
    _isViewer = dynamic_cast<ViewerInstance*>(_output) != NULL;
    _isOutputOpenFXNode = _output->isOpenFX();
    
    
    /*unmark all nodes already present in the graph*/
    clearGraph();
    fillGraph(_output);

    std::vector<U64> inputsHash;
    for(TreeContainer::iterator it = _sorted.begin();it!=_sorted.end();++it) {
        it->second->setMarkedByTopologicalSort(false);
        it->second->updateInputs(this);
        U64 ret = 0;
        it->second->clone();
        ret = it->second->computeHash(inputsHash,knobsAge);
        inputsHash.push_back(ret);
    }
}


void RenderTree::fillGraph(EffectInstance *effect){
    
    /*call fillGraph recursivly on all the node's inputs*/
    const Node::InputMap& inputs = effect->getNode()->getInputs();
    for(Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second){
            /*if the node is an inspector*/
            const InspectorNode* insp = dynamic_cast<const InspectorNode*>(effect->getNode());
            if (insp && it->first != insp->activeInput()) {
                continue;
            }
            Natron::EffectInstance* inputEffect = it->second->findOrCreateLiveInstanceClone(this);
            fillGraph(inputEffect);
        }
    }
    if(!effect->isMarkedByTopologicalSort()){
        effect->setMarkedByTopologicalSort(true);
        _sorted.push_back(std::make_pair(effect->getNode(),(EffectInstance*)effect));
    }
}

U64 RenderTree::cloneKnobsAndcomputeTreeHash(EffectInstance* effect,const std::vector<U64>& inputsHashs,int knobsAge){
    U64 ret = effect->hash().value();
    if(!effect->isHashValid()){
        effect->clone();
        ret = effect->computeHash(inputsHashs,knobsAge);
        //  std::cout << effect->getName() << ": " << ret << std::endl;
    }
    return ret;
}
void RenderTree::refreshKnobsAndHashAndClearPersistentMessage(){
    _renderOutputFormat = _output->getApp()->getProjectFormat();
    _projectViewsCount = _output->getApp()->getProjectViewsCount();
    
    //    bool oldVersionValid = _treeVersionValid;
    //    U64 oldVersion = 0;
    //    if (oldVersionValid) {
    //        oldVersion = _output->hash().value();
    //    }
    int knobsAge = _output->getAppAge();
    /*Computing the hash of the tree in topological ordering.
     For each effect in the tree, the hash of its inputs is guaranteed to have
     been computed.*/
    std::vector<U64> inputsHash;
    for (TreeIterator it = _sorted.begin(); it!=_sorted.end(); ++it) {
        inputsHash.push_back(cloneKnobsAndcomputeTreeHash(it->second,inputsHash,knobsAge));
        (*it).second->clearPersistentMessage();
    }
    _treeVersionValid = true;

}

Natron::EffectInstance* RenderTree::getEffectForNode(Natron::Node* node) const{
    for(TreeIterator it = _sorted.begin();it!=_sorted.end();++it){
        if (it->first == node) {
            return it->second;
        }
    }
    return NULL;
}

ViewerInstance* RenderTree::outputAsViewer() const {
    if(_output && _isViewer){
        return dynamic_cast<ViewerInstance*>(_output);
    }else{
        return NULL;
    }
}



void RenderTree::debug() const{
    cout << "Topological ordering of the Tree is..." << endl;
    for(RenderTree::TreeIterator it = begin(); it != end() ;++it) {
        cout << it->first->getName() << endl;
    }
}

Natron::Status RenderTree::preProcessFrame(SequenceTime time){
    /*Validating the Tree in topological order*/
    for (TreeIterator it = begin(); it != end(); ++it) {
        for (int i = 0; i < it->second->maximumInputs(); ++i) {
            if (!it->second->input(i) && !it->second->isInputOptional(i)) {
                return StatFailed;
            }
        }
        Natron::Status st = it->second->preProcessFrame(time);
        if(st == Natron::StatFailed){
            return st;
        }
    }
    return Natron::StatOK;
}


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
    //            std::cout << kProgressChangedString << floor(((double)y/(double)_currentFrameInfos._rows.size())*100) << std::endl;
    //        }
    //        return true;
    //    }else{
    //        return false;
    //    }
    return false;
}

void VideoEngine::toggleLoopMode(bool b){
    _loopMode = b;
}


void VideoEngine::setDesiredFPS(double d){
    QMutexLocker timerLocker(&_timerMutex);
    _timer->setDesiredFrameRate(d);
}



bool VideoEngine::isWorking() const {
    QMutexLocker workingLocker(&_workingMutex);
    return _working;
}

bool VideoEngine::mustQuit() const{
    QMutexLocker locker(&_mustQuitMutex);
    return _mustQuit;
}

void VideoEngine::refreshTree(){
    ///get the knobs age before locking to prevent deadlock
    int knobsAge = _tree.getOutput()->getAppAge();
    
    QMutexLocker l(&_treeMutex);
    _tree.refreshTree(knobsAge);
}

void VideoEngine::getFrameRange() {
    QMutexLocker l(&_getFrameRangeMutex);
    
    if(_tree.getOutput()){
        _tree.getOutput()->getFrameRange(&_firstFrame, &_lastFrame);
        if(_firstFrame == INT_MIN){
            _firstFrame = _timeline->leftBound();
        }
        if(_lastFrame == INT_MAX){
            _lastFrame = _timeline->rightBound();
        }
    }else{
        _firstFrame = _timeline->leftBound();
        _lastFrame = _timeline->rightBound();
    }
    
    _gettingFrameRange = false;
    _getFrameRangeCond.wakeOne();

}
