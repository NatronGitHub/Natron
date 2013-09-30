//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef POWITER_ENGINE_VIDEOENGINE_H_
#define POWITER_ENGINE_VIDEOENGINE_H_

#include <cassert>
#include <vector>
#include <QtCore/QObject>
#include <QtCore/QThreadPool>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include <QFutureWatcher>
#include <QtCore/QRunnable> // for RowRunnable => to remove if class RowRunnable is moved to VideoEngine.cpp

#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Gui/Texture.h" // for TextureRect
#include "Engine/Format.h"

class FrameEntry;
class ViewerGL;
class Node;
class Row;
class ReaderInfo;
class Reader;
class Model;
class ViewerNode;
class Writer;
class Timer;
class OfxNode;
class OutputNode;

/**
 *@class VideoEngine
 *@brief This is the engine that runs the playback. It handles all graph computations for the time range given by
 *the graph.
 **/
class VideoEngine : public QThread{
    
    Q_OBJECT
    
public:
    
    
    
    /**
     *@class DAG
     *@brief This class represents the direct acyclic graph as seen internally by the video engine.
     *It provides means to sort the graph and access to the nodes in topological order. It also
     *provides access to the input nodes and the output node of the graph.
     *An input node is a node that does not depend on any upstream node,i.e :
     *it can generate data.
     *An output node is a node whose output cannot be connected to any other node and whose
     *sole purpose is to visulize the data flowing through the graph in a given configuration.
     *A DAG is represented by 1 output node, connected to its input, and so on recursively.
     *
     **/
    class DAG{
        
    public:
        typedef std::vector<Node*>::const_iterator DAGIterator;
        typedef std::vector<Node*>::const_reverse_iterator DAGReverseIterator;
        typedef std::vector<Node*>::const_iterator InputsIterator;
        
        /**
         *@brief Construct an empty DAG that can be filled with nodes.
         *To actually fill the dag you need to call DAG::resetAndSort(OutputNode*,bool) .
         *Once filled up, you can access the nodes in topological order with the iterators.
         *The reverse iterator will give you the opposite of the topological order.
         */
        DAG():
        _output(0)
        ,_isViewer(false)
        ,_isOutputOpenFXNode(false)
        ,_dagMutex(QMutex::Recursive) /*recursive lock*/
        {}
        
        /**
         *@brief Clears the structure and fill it with a new graph, represented by the OutputNode.
         *@param out[in] This is the output of the graph, i.e either a viewer or a writer. The rest
         *of the graph is fetched recursivly starting from this node.
         *@TODO Throw some exception to detect cycles in the graph
         */
        void resetAndSort(OutputNode* out);
        
        /*Lock the dag. You should call this before any access*/
        void lock() const { _dagMutex.lock(); }
        
        void unlock() const { _dagMutex.unlock(); }
                
        /**
         *@brief Returns an iterator pointing to the first node in the graph in topological order.
         *Generally the first node is an input node.
         */
        DAGIterator begin() const {return _sorted.begin();}
        
        /**
         *@brief Returns an iterator pointing after the last node in the graph in topological order.
         *Generally the last node is an output node.
         */
        DAGIterator end() const {return _sorted.end();}
        
        /**
         *@brief Returns an iterator pointing to the last node in the graph in topological order.
         *Generally the last node is an output node.
         */
        DAGReverseIterator rbegin() const {return _sorted.rbegin();}
        
        /**
         *@brief Returns an iterator pointing before the first node in the graph in topological order.
         *Generally the first node is an input node.
         */
        DAGReverseIterator rend() const {return _sorted.rend();}
        
        /**
         *@brief Returns a pointer to the output node of the graph.
         *WARNING : It will return NULL if DAG::resetAndSort(OutputNode*,bool) has never been called.
         */
        OutputNode* getOutput() const {return _output;}
        
        
        /**
         *@brief Convenience function. Returns NULL in case the output node is not of the requested type.
         *WARNING : It will return NULL if DAG::resetAndSort(OutputNode*,bool) has never been called.
         */
        ViewerNode* outputAsViewer() const;
        
        /**
         *@brief Convenience function. Returns NULL in case the output node is not of the requested type.
         *WARNING : It will return NULL if DAG::resetAndSort(OutputNode*,bool) has never been called.
         */
        Writer* outputAsWriter() const;
        
        /**
         *@brief Convenience function. Returns NULL in case the output node is not of the requested type.
         *WARNING : It will return NULL if DAG::resetAndSort(OutputNode*,bool) has never been called.
         */
        OfxNode* outputAsOpenFXNode() const;
        
        /**
         *@brief Returns true if the output node is a viewer.
         */
        bool isOutputAViewer() const {return _isViewer;}
        
        
        /**
         *@brief Returns true if the output node is an OpenFX node.
         */
        bool isOutputAnOpenFXNode() const {return _isOutputOpenFXNode;}
        
        /**
         *@brief Accesses the input nodes of the graph.
         *@returns A reference to a vector filled with all input nodes of the graph.
         */
        const std::vector<Node*>& getInputs()const{return _inputs;}
        
        /**
         *@brief Validates that all nodes parameters can fit together in the graph.
         *This function will propagate in topological order all infos in the graph,
         *starting from input nodes and finishing to the output node. During that pass all frame ranges
         *are merged (retaining only the minimum and the maximum) and some other influencial infos are
         *passed as well. i.e : channels that a node needs from its input, the requested region of processing...etc
         *@param doFullWork[in] If true,this pass will actually do more work and will make sure to propagate all infos
         *through the graph. Generally this is called AFTER that all readers have read the headers of their current frame
         *so they know what display window/channels...etc to pass down to their outputs.
         *If false, validate is much lighter and will only merge the frame range.
         *@returns Returns true if the validation passed detected that there is no issue in the graph. Otherwise returns
         *false and all subsequent computations should be canceled.
         *@TODO: validate should throw a detailed exception of what failed.
         */
        bool validate(bool doFullWork);
        
        /**
         *@brief Generate a string containing the concatenation of all
         *the current frame file-names in all readers.
         *For example if Reader_1 has for current frame toto1.jpg and 
         *Reader_2 has for current frame lala.png, this would return
         *toto1.jpglala2.jpg
         **/
        const QString generateConcatenationOfAllReadersFileNames() const;
        
        void debug();
    private:
        
        /*recursive topological sort*/
        void topologicalSort();
        /*function called internally by the sorting
         *algorithm*/
        void _depthCycle(Node* n);
        /*called by resetAndSort(...) to fill the structure
         *upstream of the output given in parameter of resetAndSort(...)*/
        void fillGraph(Node* n);
        /*clears out the structure*/
        void clearGraph();
        
        OutputNode* _output; /*!<the output of the DAG*/
        std::vector<Node*> _graph;/*!<the un-sorted DAG*/
        std::vector<Node*> _sorted; /*!<the sorted DAG*/
        std::vector<Node*> _inputs; /*!<all the inputs of the dag*/
        bool _isViewer; /*!< true if the outputNode is a viewer, it avoids many dynamic_casts*/
        bool _isOutputOpenFXNode; /*!< true if the outputNode is an OpenFX node*/
        mutable QMutex _dagMutex; /*!< protects the dag*/
    };
    
private:
    
    /**
     * @brief The RunArgs class contains info on the size of last frame computed
     * by the engine.
     */
    struct LastFrameInfos{
        LastFrameInfos():
        _cachedEntry(NULL)
        , _rows()
        , _textureRect()
        ,_dataSize(0){}
        
        FrameEntry* _cachedEntry;
        std::vector<int> _rows;
        TextureRect _textureRect;
        U64 _dataSize;
        
    };
    
    /**
     * @brief The RunArgs class is a convenience class storing the arguments
     * passed to the render function for the last frame computed.
     */
    struct RunArgs{
        RunArgs():
        _startingFrame(INT_MAX),
        _zoomFactor(1.f),
        _sameFrame(false),
        _fitToViewer(false),
        _recursiveCall(false),
        _forward(true),
        _updateDAG(false),
        _frameRequestsCount(0),
        _frameRequestIndex(0),
        _output(0)
        {}
        
        int _startingFrame;
        float _zoomFactor;
        bool _sameFrame;/*!< on if we want the subsequent videoEngine call to be on the same frame(zoom)*/
        bool _fitToViewer;
        bool _recursiveCall;
        bool _forward;/*!< forwards/backwards video engine*/
        bool _updateDAG;
        int _frameRequestsCount;/*!< The index of the last frame +1 if the engine
                                 is forward (-1 otherwise). This value is -1 if we're looping.*/
        int _frameRequestIndex;/*!< counter of the frames computed:used to refresh the fps only every 24 frames*/
        OutputNode* _output;/*!< the output that will be used to build the DAG that will serve to render*/
    };
    
    Model* _model;/*!< pointer to the model*/
    
    DAG _dag; /*!< The internal DAG instance.*/
    
    QMutex _timerMutex;///protects timer
    boost::scoped_ptr<Timer> _timer; /*!< Timer regulating the engine execution. It is controlled by the GUI.*/
    
    QMutex _abortBeingProcessedMutex; /*!< protecting _abortBeingProcessed (in startEngine and stopEngine, when we process abort)*/
    bool _abortBeingProcessed; /*true when someone is processing abort*/

    QWaitCondition _abortedRequestedCondition;
    QMutex _abortedRequestedMutex; //!< protects _abortRequested
    int _abortRequested ;/*!< true when the user wants to stop the engine, e.g: the user disconnected the viewer*/
    
    QMutex _mustQuitMutex; //!< protects _mustQuit
    bool _mustQuit;/*!< true when we quit the engine (i.e: we delete the OutputNode associated to this engine)*/
    
    U64 _treeVersion;/*!< the hash key associated to the current graph*/
    bool _treeVersionValid;/*!< was _treeVersion initialized? */
    
    QMutex _loopModeMutex;///protects _loopMode
    bool _loopMode; /*!< on if the player will loop*/
    
    /*Accessed and modified only by the run() thread*/
    bool _restart; /*!< if true, the run() function should call startEngine() on the next loop*/
    
    QMutex _forceRenderMutex;
    bool _forceRender;/*!< true when we want to by-pass the cache*/
    
    QMutex _workerThreadsWatcherMutex;
    QFutureWatcher<void>* _workerThreadsWatcher;/*!< watcher of the thread pool running the meta engine for all rows of
                                                 the current frame. Its finished() signal will call
                                                 Worker::finishComputeFrameRequest()*/    

    QWaitCondition _pboUnMappedCondition;
    QMutex _pboUnMappedMutex; //!< protects *_openGLCount
    int _pboUnMappedCount;

       
    QWaitCondition _startCondition;
    QMutex _startMutex; //!< protects _startCount
    int _startCount; //!< if > 0 that means start requests are pending
    
    QMutex _workingMutex;//!< protects _working
    bool _working; //!< true if a thread is working

    /*These member doesn't need to be protected by a mutex: 
     _lastRequestedRunArgs is modified upon a call to render() and 
     _currentRunArgs just retrieves the last value found in _lastRequestedRunArgs,
     they're accessed both by 1 (different) thread exclusivly.*/
    RunArgs _lastRequestedRunArgs; /*called upon render()*/
    RunArgs _currentRunArgs; /*the args used in the run() func*/
    
    /*Accessed only by the run() thread*/
    LastFrameInfos _currentFrameInfos; /*!< The stored infos generated for the last frame. Used by the gui thread slots.*/
    
    /*Accessed only by the run() thread*/
    timeval _startRenderFrameTime;/*!< stores the time at which the QtConcurrent::map call was made*/
    
protected:
    
    /*The function doing all the processing, called by render()*/
    virtual void run();
    
public slots:
        /**
     @brief Aborts all computations. This turns on the flag _abortRequested and will inform the engine that it needs to stop.
     **/
    void abort();
    /**
     *@brief Calls the video engine for the frame number frame. This is the slot called when the user scrub in the timeline.
     **/
    void seek(int frame);
   
    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);
    
    
    /*
     *@brief Slot called internally by the render() function when it reports progress for the current frame.
     *Do not call this yourself.
     */
    void toggleLoopMode(bool b);
    
    
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
     *@brief Slot called internally by the render() function when it wants to refresh the viewer if
     *the output is a viewer.
     *Do not call this yourself.
     */
    void updateViewer();
    
    /*
     *@brief Slot called internally by the render() function when it found a frame in cache.
     *Do not call this yourself
     **/
    void cachedEngine();
    
    /*
     *@brief Slot called internally by the render() function when the output is a viewer and it
     *needs to allocate the output buffer for the current frame.
     *Do not call this yourself.
     */
    void allocateFrameStorage();
    
    /*
     *@brief Slot called when the output node owning this VideoEngine is about to be deleted.
     *Do not call this yourself.
     */
    void quitEngineThread();
    /************************************************************************************************************
     ************************************************************************************************************
     **************************************END PRIVATE SLOTS*****************************************************
     ************************************************************************************************************
     ***********************************************************************************************************/
   
    
signals:
    /**
     *@brief Signal emitted when the function waits the time due to display the frame.
     **/
    void fpsChanged(double d);
    
    /**
     *@brief Signal emitted when the engine needs to inform the main thread that it should refresh the
     *viewer in case the output of the graph is a viewer.
     **/
    void doUpdateViewer();
    
    /**
     *@brief Signal emitted when the engine needs to pass-on to the main thread the rendering of a cached frame.
     **/
    void doCachedEngine();
    
    /**
     *@brief Signal emitted when the engine needs to warn the main thread that the storage for the current frame
     *should be allocated in case the output is a viewer.
     **/
    void doFrameStorageAllocation();
    
    /**
     *@brief emitted when the engine started to render a sequence (which can be only a sequence of 1 frame).
     **/
    void engineStarted(bool forward);
    
    /**
     *@brief emitted when the engine finished rendering for a sequence (which can be only a sequence of 1 frame).
     **/
    void engineStopped();
    
    /**
     *@brief emitted when progress is reported for the current frame.
     *Progress varies between [0,100].
     **/
    void progressChanged(int progress);
    
    /**
     *@brief emitted when a frame has been rendered successfully.
     **/
    void frameRendered(int frameNumber);
    
public:
   
    
    VideoEngine(Model* model, QObject* parent = NULL);
    
    virtual ~VideoEngine();
    
    
    /**
     *@brief Starts the video engine. It can be called from anywhere and at anytime. It starts off at the current
     *frame indicated on the timeline.
     *@param startingFrame[in] The frame to start with.
     *@param output[in] The output from which we should build the DAG that will serve to render.
     *@param frameCount[in] This is the number of frames you want to execute the engine for. -1 will make the
     *engine run until the end of the sequence is reached. If loop mode is enabled, the engine will never stops
     *until the user explicitly stops it.
     *@param updateDAG if true, the engine will rebuild the internal DAG in the startEngine() function.
     *@param fitFrameToViewer[in] If true, it will fit the first frame to the viewport.
     *@param forward[in] If true, the engine runs forwards, otherwise backwards.
     *@param sameFrame[in] If true, that means the engine will not increment/decrement the frame indexes and will run
     *for the same frame than the last frame  computed. This is used exclusively when zooming/panning. When sameFrame
     *is on, frameCount MUST be 1.
     **/
    void render(OutputNode* output,
                int startingFrame,
                int frameCount,
                bool updateDAG,
                bool fitFrameToViewer = false,
                bool forward = true,
                bool sameFrame = false);
    
    
    
    /**
     *@brief This function internally calls render(). If the playback is running, then it will resume the playback
     *taking into account the new parameters changed. Otherwise it will just refresh the same frame than the last frame rendered
     *on the viewer. This function is to be called exclusivly by the Viewer. This function should be called whenever
     *a parameter changed but not the DAG itself.
     *@param initViewer[in] If true,this will fit the next frame rendered to the viewer in case output is a viewer.
     *serve to render the frames.
     *@param output[in] A pointer to the output whose inputs will determine the DAG that will
     *serve to render the frames in case we need to build the DAG.
     *@param startingFrame[in] The frame to start rendering with.
     **/
    void refreshAndContinueRender(bool initViewer,OutputNode* output,int startingFrame);
    
    /**
     *@brief This function internally calls render(). If the playback is running, then it will resume the playback
     *taking into account the new DAG that it has rebuilt using output.
     *Otherwise it will just refresh the same frame than the last frame rendered
     *on the viewer. This function should be called whenever
     *a change has been made (potentially) to the DAG.
     *@param initViewer[in] If true,this will fit the next frame rendered to the viewer in case output is a viewer.
     *@param output[in] A pointer to the output whose inputs will determine the DAG that will
     *serve to render the frames.
     *@param startingFrame[in] The frame to start rendering with.
     **/
    void updateDAGAndContinueRender(bool initViewer,OutputNode* output,int startingFrame);

    
    /**
     *@brief Bypasses the cache so the next frame will be rendered fully
     **/
    void forceFullComputationOnNextFrame(){
        QMutexLocker forceRenderLocker(&_forceRenderMutex);
        _forceRender = true;
    }
    
    /**
     *@brief Convenience function calling DAG::isOutputAViewer()
     **/
    bool isOutputAViewer() const {return _dag.isOutputAViewer();}
    
    /**
     *@returns Returns a const reference to the DAG used by the video engine.
     *You should bracket dag.lock() and dag.unlock() before any operation on 
     *the dag.
     **/
    const DAG& getCurrentDAG() const { return _dag; }
    
    /*@brief Calls DAG::validate(bool)*/
    void validate(bool doFullWork){
        _dag.validate(doFullWork);
    }
    
	/**
     *@returns Returns true if the engine is currently working.
     **/
	bool isWorking();
    
    /**
     *@returns Returns the 64-bits key associated to the output node of the current graph. This key
     *represents the version of the graph.
     **/
    U64 getCurrentTreeVersion() { assert(_treeVersionValid); return _treeVersion;}
    
    bool hasBeenAborted() const {return _abortRequested;}
    
private:
    
    
    /**
     *@brief Resets and computes the hash key for all the nodes in the graph. The tree version is the hash key of the output node
     *of the graph.
     **/
    void updateTreeVersion();
    
    /**
     *@brief displays progress if the time to compute the current frame exeeded
     * 0.5 sec.
     **/
    bool checkAndDisplayProgress(int y,int zoomedY);

    void engineLoop();

    /**
     *@brief Resets the video engine state and ensures that all worker threads are stopped. It is called
     *internally by the run function.
     **/
    void stopEngine();
    
    /**
     *@brief Set up the video engine state, e.g: build DAG from the output node and activate some flags.
     *It is used internally by the run function
     *@returns Returns true if started,false otherwise
     **/
    bool startEngine();
    
    /**
     *@brief Forces each reader in the input nodes of the graph to read the header of their current frame's file.
     *@param readers[in] A vector of all the readers in the current graph.
     */
    void readHeaders(const std::vector<Reader*>& readers);
    
    /**
     *@brief Forces each reader in the input nodes of the graph to read the data of their current frame's file.
     *@param readers[in] A vector of all the readers in the current graph.
     */
    void readFrames(const std::vector<Reader*>& readers);
    
    /**
     *@brief Calls QGLWidget::updateGL() and causes the viewer to refresh.
     *It also adjusts the pixel aspect ratio of the viewer.
     */
    void updateDisplay();
    
    
#ifdef POWITER_DEBUG
    /*
     *@brief Range-check to be sure buffers are allocated correctly
     *@param columns the indexes of the columns to compute.
     *@param x The left edge of the rectangle to compute.
     *@param r The right edge of the rectangle to compute.
     */
    bool rangeCheck(const std::vector<int>& columns,int x,int r);
#endif
};


/*Not used but leave it here if we need to use QThreadPool instead of
 QtConcurrent::map.
 */
class RowRunnable : public QObject, public QRunnable{
    Q_OBJECT
    
    Row* _row;
    Node* _output;
    
public:
    
    RowRunnable(Row* row,Node* output):
    QRunnable(),_row(row),_output(output)
    {
    }
    
    virtual void run();
    
    virtual ~RowRunnable(){}
    
signals:
    void finished(int,int);
    
};



#endif /* defined(POWITER_ENGINE_VIDEOENGINE_H_) */
