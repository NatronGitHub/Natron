//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_VIDEOENGINE_H_
#define NATRON_ENGINE_VIDEOENGINE_H_

#include <cassert>
#include <vector>
#include <list>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QThreadPool>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>


#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/FrameEntry.h"

namespace Natron{
class Node;
class EffectInstance;
class OutputEffectInstance;
}
class ViewerInstance;
class OfxNode;
class TimeLine;
class Timer;


/**
 *@class Tree
 *@brief This class represents the tree upstream of the output node as seen internally by the video engine.
 *It provides means to sort the tree and access to the nodes in topological order. It also
 *provides access to the input nodes and the output node of the tree.
 *An input node is a node that does not depend on any upstream node,i.e :
 *it can generate data.
 *An output node is a node whose output cannot be connected to any other node and whose
 *sole purpose is to visulize the data flowing through the graph in a given configuration.
 *A Tree is represented by 1 output node, connected to its input, and so on recursively.
 *
 **/
class RenderTree {
    
    
public:
    typedef std::vector<boost::shared_ptr<Natron::Node> > TreeContainer;
    typedef TreeContainer::const_iterator TreeIterator;
    typedef TreeContainer::const_reverse_iterator TreeReverseIterator;
    typedef TreeContainer::const_iterator InputsIterator;
    
    /**
     *@brief Construct an empty Tree that can be filled with nodes.
     *To actually fill the dag you need to call Tree::resetAndSort(OutputNode*,bool) .
     *Once filled up, you can access the nodes in topological order with the iterators.
     *The reverse iterator will give you the opposite of the topological order.
     */
    RenderTree(Natron::EffectInstance* output);
    
    /**
     *@brief Clears the structure and fill it with a new tree, represented by the OutputNode.
     *The rest of the tree is fetched recursivly starting from this node.
     *@TODO Throw some exception to detect cycles in the graph
     */
    void refreshTree();
    
    /**
     *@brief Returns an iterator pointing to the first node in the graph in topological order.
     *Generally the first node is an input node.
     */
    TreeIterator begin() const {return _sorted.begin();}
    
    /**
     *@brief Returns an iterator pointing after the last node in the graph in topological order.
     *Generally the last node is an output node.
     */
    TreeIterator end() const {return _sorted.end();}
    
    /**
     *@brief Returns an iterator pointing to the last node in the graph in topological order.
     *Generally the last node is an output node.
     */
    TreeReverseIterator rbegin() const {return _sorted.rbegin();}
    
    /**
     *@brief Returns an iterator pointing before the first node in the graph in topological order.
     *Generally the first node is an input node.
     */
    TreeReverseIterator rend() const {return _sorted.rend();}
    
    /**
     *@brief Returns a pointer to the output node of the graph.
     */
    Natron::EffectInstance* getOutput() const {return _output;}
    
    
    /**
     *@brief Convenience function. Returns NULL in case the output node is not of the requested type.
     *WARNING : It will return NULL if Tree::resetAndSort(OutputNode*,bool) has never been called.
     */
    ViewerInstance* outputAsViewer() const;

    
    /**
     *@brief Returns true if the output node is a viewer.
     */
    bool isOutputAViewer() const {return _isViewer;}
    
    
    /**
     *@brief Returns true if the output node is an OpenFX node.
     */
    bool isOutputAnOpenFXNode() const {return _isOutputOpenFXNode;}
    
    /**
     *@brief Checks whether the render tree can produce a valid result.
     */
    Natron::Status preProcessFrame();
    
    SequenceTime firstFrame() const {return _firstFrame;}
    
    SequenceTime lastFrame() const {return _lastFrame;}
    
    void debug() const;
    
    void refreshRenderInputs();
    
    void clearPersistentMessages();
    
    /**
     * @brief Calls begin sequence render on all effects in the tree
     **/
    Natron::Status beginSequentialRender(SequenceTime first,SequenceTime last,int view);
    
    /**
     * @brief Calls end sequence render on all effects in the tree
     **/
    Natron::Status endSequentialRender(SequenceTime first,SequenceTime last,int view);
    
    bool wasTreeEverBuilt() const { return _wasEverBuilt; }
    
private:
    /*called by resetAndSort(...) to fill the structure
     *upstream of the output given in parameter of resetAndSort(...)*/
    void fillGraph(const boost::shared_ptr<Natron::Node>& effect,std::vector<boost::shared_ptr<Natron::Node> >& markedNodes);
    /*clears out the structure*/
    void clearGraph();
    
    Natron::EffectInstance* _output; /*!<the output of the Tree*/
    TreeContainer _sorted; /*!<the sorted Tree*/
    bool _isViewer; /*!< true if the outputNode is a viewer, it avoids many dynamic_casts*/
    bool _isOutputOpenFXNode; /*!< true if the outputNode is an OpenFX node*/
    bool _wasEverBuilt;
    SequenceTime _firstFrame,_lastFrame;/*!< first frame and last frame of the union range of all inputs*/
};

/**
 *@class VideoEngine
 *@brief This is the engine that runs the playback. It handles all graph computations for the time range given by
 *the graph.
 **/
class VideoEngine : public QThread {
    
    Q_OBJECT
    
    
public slots:
    /**
     @brief Aborts all computations. This turns on the flag _abortRequested and will inform the engine that it needs to stop.
     * If blocking is true the function will not return before the ending has stopped.
     **/
    void abortRendering(bool blocking);
    
    
    ///convenience function for signal/slots
    void abortRenderingNonBlocking() { abortRendering(false); }
    
    /*
     *@brief Slot called internally by the render() function when it reports progress for the current frame.
     *Do not call this yourself.
     */
    void toggleLoopMode(bool b);
    
    
    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);
    

    
    /************************************************************************************************************
     ************************************************************************************************************
     **************************************PRIVATE SLOTS*********************************************************
     *************************************DO NO CALL THEM********************************************************
     ***********************************************************************************************************/
    /*
     *@brief Slot called internally by the render() function when it reports progress for the current frame.
     *Do not call this yourself.
     */
    void onProgressUpdate(int i);
    
    
    /*
     *@brief Slot called when the output node owning this VideoEngine is about to be deleted.
     *Do not call this yourself.
     */
    void quitEngineThread();
    
    
    void getFrameRange();
    /************************************************************************************************************
     ************************************************************************************************************
     **************************************END PRIVATE SLOTS*****************************************************
     ************************************************************************************************************
     ***********************************************************************************************************/
   
    
    
signals:
   
    /**
     *@brief Signal emitted when the function waits the time due to display the frame.
     **/
    void fpsChanged(double actualFrameRate,double desiredFrameRate);
    

    
    /**
     *@brief emitted when the engine started to render a sequence (which can be only a sequence of 1 frame).
     **/
    void engineStarted(bool forward,int frameCount);
    
    /**
     *@brief emitted when the engine finished rendering and is going to idle.
     *@param returnCode Set to 1 if the engine stopped because it was aborted, 0 otherwise.
     **/
    void engineStopped(int returnCode);
    
    /**
     *@brief emitted when progress is reported for the current frame.
     *Progress varies between [0,100].
     **/
    void progressChanged(int progress);
    
    /**
     *@brief emitted when a frame has been rendered successfully.
     **/
    void frameRendered(int frameNumber);
    
    void mustGetFrameRange();
    
public:
   
    
    VideoEngine(Natron::OutputEffectInstance* owner, QObject* parent = NULL);
    
    virtual ~VideoEngine();
    
    
    /**
     * @brief Starts the video engine. It can be called from anywhere and at anytime. It starts off at the current
     * frame indicated on the timeline. This function has a lot of parameters and you might find it more easy to
     * call refreshAndContinueRender or updateTreeAndContinueRender which just call render() with the good parameters.
     *
     * @param frameCount[in] This is the number of frames you want to execute the engine for. -1 will make the
     * engine run until the end of the sequence is reached. If loop mode is enabled, the engine will never stops
     * until the user explicitly stops it.
     * If frameCount is -1 and the application is background then the render will be considered to be
     * "non-interactive", in which sense that it is a hint to plug-ins that they have no interface.
     * If frameCount is equal to 1 then if the output is a viewer, the render will be considered as
     * "interactive" in a sense that the render is made in response to user interaction.
     *
     * @param seekTimeline If seekTimeline is false the timeline will not be moved by the playback.
     * @param refreshTree if true, the engine will rebuild the internal Tree in the startEngine() function.
     * @param fitFrameToViewer[in] If true, it will fit the first frame to the viewport.
     * @param forward[in] If true, the engine runs forwards, otherwise backwards.
     * @param sameFrame[in] If true, that means the engine will not increment/decrement the frame indexes and will run
     * for the same frame than the last frame  computed. This is used exclusively when zooming/panning. When sameFrame
     * is on, frameCount MUST be 1.
     *
     * @param view[in] This param is exclusive to tree which output is a writer node. This indicates what view
     * the tree should render.
     **/
    void render(int frameCount,
                bool seekTimeline,
                bool refreshTree,
                bool forward,
                bool sameFrame,
                bool forcePreview);
    
    
    
    /**
     *@brief This function internally calls render(). If the playback is running, then it will resume the playback
     *taking into account the new parameters changed. Otherwise it will just refresh the same frame than the last frame rendered
     *on the viewer. This function is to be called exclusivly by the Viewer. This function should be called whenever
     *a parameter changed but not the Tree itself.
     *@param initViewer[in] If true,this will fit the next frame rendered to the viewer in case output is a viewer.
     *serve to render the frames.
     **/
    void refreshAndContinueRender(bool forcePreview);
    
    /**
     *@brief This function internally calls render(). If the playback is running, then it will resume the playback
     *taking into account the new Tree that it has rebuilt using output.
     *Otherwise it will just refresh the same frame than the last frame rendered
     *on the viewer. This function should be called whenever
     *a change has been made (potentially) to the Tree.
     *@param initViewer[in] If true,this will fit the next frame rendered to the viewer in case output is a viewer.
     **/
    void updateTreeAndContinueRender();

    
    /**
     *@brief Convenience function calling Tree::isOutputAViewer()
     **/
    bool isOutputAViewer() const {return _tree.isOutputAViewer();}
  
    
    void refreshTree();
    
	/**
     *@returns Returns true if the engine is currently working.
     **/
	bool isWorking() const;
    
    bool mustQuit() const;
    
    bool hasBeenAborted() const {return _abortRequested;}
    
private:

    /*The function doing all the processing in a separate thread, called by render()*/
    virtual void run() OVERRIDE FINAL;
    
    /*Same as run() but in the same thread*/
    void runSameThread();
    
    /*Used by run() and runSameThread()*/
    void iterateKernel(bool singleThreaded);
    
    /**
     *@brief Resets and computes the hash key for all the nodes in the graph. The tree version is the hash key of the output node
     *of the graph.
     **/
    void computeTreeVersionAndLockKnobs();
    
    
    /**
     *@brief displays progress if the time to compute the current frame exeeded
     * 0.5 sec.
     **/
    bool checkAndDisplayProgress(int y,int zoomedY);

    /**
     *@brief Resets the video engine state and ensures that all worker threads are stopped. It is called
     *internally by the run function.
     **/
    bool stopEngine();
    
    /**
     *@brief Set up the video engine state, e.g: build Tree from the output node and activate some flags.
     *It is used internally by the run function
     *@returns Returns true if started,false otherwise
     **/
    bool startEngine(bool singleThreaded);
    
    Natron::Status renderFrame(SequenceTime time,bool singleThreaded);

private:
    // FIXME: PIMPL

    /**
     * @brief The RunArgs class is a convenience class storing the arguments
     * passed to the render function for the last frame computed.
     */
    struct RunArgs{
        RunArgs():
        _zoomFactor(1.f),
        _sameFrame(false),
        _recursiveCall(false),
        _forward(true),
        _refreshTree(false),
        _seekTimeline(true),
        _forcePreview(false),
        _frameRequestsCount(0),
        _frameRequestIndex(0),
        _forceSequential(false)
        {}

        float _zoomFactor;
        bool _sameFrame;/*!< on if we want the subsequent render call to be on the same frame(zoom)*/
        bool _recursiveCall;
        bool _forward;/*!< forwards/backwards video engine*/
        bool _refreshTree;
        bool _seekTimeline;
        bool _forcePreview;
        int _frameRequestsCount;/*!< The index of the last frame +1 if the engine
                                 is forward (-1 otherwise). This value is -1 if we're looping.*/
        int _frameRequestIndex;/*!< counter of the frames computed:used to refresh the fps only every 24 frames*/
        bool _forceSequential; /*!< if true, the render will be forced to be sequential and only on the main view.*/
    };

    RenderTree _tree; /*!< The internal Tree instance.*/

    bool _threadStarted;

    mutable QMutex _abortBeingProcessedMutex; /*!< protecting _abortBeingProcessed (in startEngine and stopEngine, when we process abort)*/
    bool _abortBeingProcessed; /*true when someone is processing abort*/

    QWaitCondition _abortedRequestedCondition;
    mutable QMutex _abortedRequestedMutex; //!< protects _abortRequested
    int _abortRequested ;/*!< true when the user wants to stop the engine, e.g: the user disconnected the viewer*/

    QWaitCondition _mustQuitCondition;
    mutable QMutex _mustQuitMutex; //!< protects _mustQuit
    bool _mustQuit;/*!< true when we quit the engine (i.e: we delete the OutputNode associated to this engine)*/

    mutable QMutex _loopModeMutex;///protects _loopMode
    bool _loopMode; /*!< on if the player will loop*/

    /*Accessed and modified only by the run() thread*/
    bool _restart; /*!< if true, the run() function should call startEngine() on the next loop*/


    QWaitCondition _startCondition;
    mutable QMutex _startMutex; //!< protects _startCount
    int _startCount; //!< if > 0 that means start requests are pending

    mutable QMutex _workingMutex;//!< protects _working
    bool _working; //!< true if a thread is working

    mutable QMutex _timerMutex;///protects timer
    boost::scoped_ptr<Timer> _timer; /*!< Timer regulating the engine execution. It is controlled by the GUI.*/
    int _timerFrameCount;

    /*These member doesn't need to be protected by a mutex:
     _lastRequestedRunArgs is modified upon a call to render() and
     _currentRunArgs just retrieves the last value found in _lastRequestedRunArgs,
     they're accessed both by 1 (different) thread exclusivly.*/
    RunArgs _lastRequestedRunArgs; /*called upon render()*/
    RunArgs _currentRunArgs; /*the args used in the run() func*/


    /*Accessed only by the run() thread*/
    timeval _startRenderFrameTime;/*!< stores the time at which the QtConcurrent::map call was made*/

    boost::shared_ptr<TimeLine> _timeline;/*!< ptr to the timeline*/
    
    QWaitCondition _getFrameRangeCond;
    mutable QMutex _getFrameRangeMutex;
    bool _gettingFrameRange;
    int _firstFrame;
    int _lastFrame;
    
    bool _doingARenderSingleThreaded;

};




#endif /* defined(NATRON_ENGINE_VIDEOENGINE_H_) */
