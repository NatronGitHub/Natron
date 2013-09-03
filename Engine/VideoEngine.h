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

// FIXME: can the definitions of classes EngineMainEntry, Worker, and RowRunnable be moved to VideoEngine.cpp?
#include <vector>
#include <QtCore/QObject>
#include <QtCore/QThreadPool>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include <QFutureWatcher>
#include <QtCore/QRunnable> // for RowRunnable => to remove if class RowRunnable is moved to VideoEngine.cpp

#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#endif

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Gui/Texture.h" // for TextureRect
#include "Engine/Format.h"

class FrameEntry;
class InputNode;
class OutputNode;
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
class EngineMainEntry;
class Worker;

/**
 *@class VideoEngine
 *@brief This is the engine that runs the playback. It handles all graph computations for the time range given by
 *the graph. A lot of optimisations are made in here to keep the main thread refreshing the GUI and also do some
 *OpenGL computations. Take a look at the drawing in the source file if you want to understand the succession of calls
 *that are made by the video engine. This class is non copyable and for now the software can only support 1 engine.
 *That means that you cannot have 2 computations running at the same time. A future version will most likely introduce
 *multi-engine processing but this is not a priority.
 **/
class VideoEngine : public QThread{
    Q_OBJECT
    
    friend class EngineMainEntry;
    friend class Worker;
    
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
        DAG():_output(0),_isViewer(false),_isOutputOpenFXNode(false){}
        
        /**
         *@brief Clears the structure and fill it with a new graph, represented by the OutputNode.
         *@param out[in] This is the output of the graph, i.e either a viewer or a writer. The rest
         *of the graph is fetched recursivly starting from this node.
         *@param isViewer[in] If true, it indicates the DAG that the output node is a viewer, hence
         *allowing some optimisations to be made by the engine. The type of the output node should always
         *be known when calling this function.
         *@TODO Throw some exception to detect cycles in the graph
         */
        
        void resetAndSort(Node* out, bool isViewer);
        
        /**
         *@brief Clears out the structure. As a result the graph will do nothing.
         */
        void reset();
        
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
        Node* getOutput() const {return _output;}
        
        
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
         *@brief The first frame supported by the graph.
         *@decription The first frame is the minimum frame number that an input node of the graph can generate.
         *E.G: Imagine the DAG has 2 readers A and B. A has a frame range of 30-50 and B a frame range of 20-40.
         *The first frame in that case is 20.
         *@returns Returns the first frame number that can be read in the graph.
         */
        int firstFrame() const ;
        
        /**
         *@brief The last frame supported by the graph.
         *@decription The last frame is the maximum frame number that an input node of the graph can generate.
         *E.G: Imagine the DAG has 2 readers A and B. A has a frame range of 30-50 and B a frame range of 20-40.
         *The last frame in that case is 50.
         *@returns Returns the last frame number that can be read in the graph.
         */
        int lastFrame() const;
        
        /**
         *@brief Validates that all nodes parameters can fit together in the graph.
         *This function will propagate in topological order all infos in the graph,
         *starting from input nodes and finishing to the output node. During that pass all frame ranges
         *are merged (retaining only the minimum and the maximum) and some other influencial infos are
         *passed as well. i.e : channels that a node needs from its input, the requested region of processing...etc
         *@param forReal[in] If true,this pass will actually do more work and will make sure to propagate all infos
         *through the graph. Generally this is called AFTER that all readers have read the headers of their current frame
         *so they know what display window/channels...etc to pass down to their outputs.
         *If false, validate is much lighter and will only merge the frame range.
         *@returns Returns true if the validation passed detected that there is no issue in the graph. Otherwise returns
         *false and all subsequent computations should be canceled.
         *@TODO: validate should throw a detailed exception of what failed.
         */
        bool validate(bool forReal);
        
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
        
        Node* _output; /*!<the output of the DAG*/
        std::vector<Node*> _graph;/*!<the un-sorted DAG*/
        std::vector<Node*> _sorted; /*!<the sorted DAG*/
        std::vector<Node*> _inputs; /*!<all the inputs of the dag*/
        bool _isViewer; /*!< true if the outputNode is a viewer, it avoids many dynamic_casts*/
        bool _isOutputOpenFXNode; /*!< true if the outputNode is an OpenFX node*/
    };
    
    /**
     *@struct EngineStatus
     *@brief This class is used as a return code from the function
     *VideoEngine::computeFrameRequest(bool,bool,bool).
     *The members of this class helps the video engine to know what decision to make after
     *the thread as returned from VideoEngine::computeFrameRequest(bool,bool,bool). Thanks to this class
     *the engine will either go in cache mode (i.e: reading data from the cache) or start computations
     *in the graph.
     **/
    struct EngineStatus{
        enum RetCode{NORMAL_ENGINE = 0 , CACHED_ENGINE = 1 , ABORTED = 2};
        
        EngineStatus():_cachedEntry(0),_key(0),_x(0),_r(0),_returnCode(NORMAL_ENGINE){}
        
        EngineStatus(FrameEntry* cachedEntry,U64 key,int x,int r,const std::vector<int> rows,EngineStatus::RetCode state):
        _cachedEntry(cachedEntry),
        _key(key),
        _x(x),
        _r(r),
        _rows(rows),
        _returnCode(state)
        {}
        
        EngineStatus(const EngineStatus& other): _cachedEntry(other._cachedEntry),
        _key(other._key),_x(other._x),_r(other._r),
        _rows(other._rows),_returnCode(other._returnCode){}
        
        
        ~EngineStatus(){}
        
        FrameEntry* _cachedEntry;
        U64 _key;
        int _x,_r;
        std::vector<int> _rows;
        RetCode _returnCode;
    };
    
private:
    
    friend class ViewerGL;
    
    /**
     *@brief A typedef used to represent a generic signature of a function that represent a user action like Play, Pause, Seek...etc
     */
    typedef void (VideoEngine::*VengineFunction)(int,int,bool,bool,bool,Node*);
    
    /**
     *@struct VideoEngine::Task
     *@brief This class holds all necessary infos to store a function call that will be queued. This is used mainly to delay
     *user requested operation like "Seek" "Play" etc... so it doesn't mess with the currently running computations. Once
     *the engine finishes its computations, it will then execute the stored Task.
     **/
    struct Task{
        int _newFrameNB;
        int _frameCount;
        bool _initViewer;
        bool _forward;
        bool _sameFrame;
        Node* _output;
        VengineFunction _func;
        Task(int newFrameNB,int frameCount,bool initViewer,bool forward,bool sameFrame,Node* output,VengineFunction func):
        _newFrameNB(newFrameNB),_frameCount(frameCount),_initViewer(initViewer),_forward(forward),_sameFrame(sameFrame),
        _output(output),_func(func){}
        
    };
    
    /**
     * @brief The ViewerCacheArgs class is a convenience class storing the arguments
     * passed to the viewer cache for checking if a frame is cached or not.
     * This saves the burden of re-calling all the arguments separatly into
     * member variables or even re-compute some of them.
     */
    struct LastFrameInfos{
        LastFrameInfos():_cachedEntry(0),_dataSize(0){}
        
        FrameEntry* _cachedEntry;
        std::vector<int> _rows;
        TextureRect _textureRect;
        size_t _dataSize;
        
    };
    
    struct RunArgs{
        RunArgs():_zoomFactor(1.f),
        _sameFrame(false),
        _fitToViewer(false),
        _recursiveCall(false),
        _forward(true),
        _frameRequestsCount(0),
        _frameRequestIndex(0){}
        
        float _zoomFactor;
        bool _sameFrame;/*!< on if we want the subsequent videoEngine call to be on the same frame(zoom)*/
        bool _fitToViewer;
        bool _recursiveCall;
        bool _forward;/*!< forwards/backwards video engine*/
        int _frameRequestsCount;/*!< The index of the last frame +1 if the engine
                                 is forward (-1 otherwise). This value is -1 if we're looping.*/
        int _frameRequestIndex;/*!< counter of the frames computed:used to refresh the fps only every 24 frames*/
    };
    
    std::vector<Task> _waitingTasks; /*!< The queue of the user's waiting tasks.*/
        
	bool _working; /*!< True if the engine is working.*/
    
    DAG _dag; /*!< The internal DAG instance.*/
    
    Timer* _timer; /*!< Timer regulating the engine execution. It is controlled by the GUI.*/
    
    bool _aborted ;/*!< true when the engine has been aborted, i.e: the user disconnected the viewer*/
    
    bool _paused; /*!< true when the user pressed pause*/
        
    U64 _treeVersion;/*!< the hash key associated to the current graph*/
    
    bool _loopMode; /*!< on if the player will loop*/
        
    bool _autoSaveOnNextRun; /*!< on if we need to to an autosave at the end of the next compute frame.*/
    
    QFutureWatcher<void>* _workerThreadsWatcher;/*!< watcher of the thread pool running the meta engine for all rows of
                                                 the current frame. Its finished() signal will call
                                                 Worker::finishComputeFrameRequest()*/    
    QVector<Row*> _sequence;
    
    QFutureWatcher<void>* _computeFrameWatcher;/*!< watcher of the thread running the function
                                                EngineMainEntry::computeFrameRequest. It stores the
                                                results of the function and calls
                                                VideoEngine::dispatchComputeFrameRequestThread()
                                                when finished.*/
    // QThreadPool* _threadPool;
    
    QWaitCondition* _openGLCondition;
    
    QWaitCondition _startCondition;
    
    QMutex* _mutex;
    
    RunArgs _lastRunArgs;
        
    LastFrameInfos _lastFrameInfos; /*!< The stored infos generated for the last frame. Used by the gui thread slots.*/
    
    timeval _lastComputeFrameTime;/*!< stores the time at which the QtConcurrent::map call was made*/
    
protected:
    virtual void run();
    
    public slots:
	/**
     *@brief Starts the engine with initialisation of the viewer so the frame fits in the viewport.
	 *If nbFrames = -1, the engine cycles through all remaining frames(and loops if loop mode is activated).
     **/
    void startEngine(int nbFrames =-1);
    
    /**
     *@brief Starts the engine to repeat the same frame (usually called when panning/zooming)
     * The engine will not  initialise  the viewer to fit the frame in the viewport.
     **/
    void repeatSameFrame();
    
    /**
     *@brief Does nothing yet as there ain't no progress bar.
     **/
    void updateProgressBar();
    
    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);
    
    /**
     @brief Aborts all computations. This turns on the flag _aborted and will inform the engine that it needs to stop.
     **/
    void abort();
    
    /**
     @brief Aborts all computations. This turns on the flag _aborted and will inform the engine that it needs to stop.
     This is slightly different than abort() because the engine will run the waiting tasks queued when it stops.
     **/
    void pause();
    
    /**
     *@brief Starts or stops the video engine. This is the slot called when the user toggle on/off the play button.
     **/
    void startPause(bool c);
    
    /**
     *@brief Starts or stops the video engine in backwords fashion.
     *This is the slot called when the user toggle on/off the play backwards button.
     **/
    void startBackward(bool c);
    
    /**
     *@brief Calls the video engine for the previous frame. This is the slot called when the user press the previous button.
     **/
    void previousFrame();
    
    /**
     *@brief Calls the video engine for the next frame. This is the slot called when the user press the next button.
     **/
    void nextFrame();
    
    /**
     *@brief Calls the video engine for the first frame. This is the slot called when the user press the first frame button.
     **/
    void firstFrame();
    
    /**
     *@brief Calls the video engine for the last frame. This is the slot called when the user press the last frame button.
     **/
    void lastFrame();
    
    /**
     *@brief Calls the video engine for the frame located at (currentFrame - incrementValue). This is the slot called when the user press the
     *previous increment button.
     **/
    void previousIncrement();
    
    /**
     *@brief Calls the video engine for the frame located at (currentFrame + incrementValue). This is the slot called when the user press the
     *next increment button.
     **/
    void nextIncrement();
    
    /**
     *@brief Calls the video engine for the frame number f. This is the slot called when the user scrub in the timeline.
     **/
    void seekRandomFrame(int f);
    
    /**
     *@brief Convenience function for the FeedBackSpinbox class
     **/
    void seekRandomFrame(double d){seekRandomFrame((int)d);}
    
    /**
     *@brief Resets and computes the hash key for all the nodes in the graph. The tree version is the hash key of the output node
     *of the graph.
     **/
    void changeTreeVersion();
    
    /**
     *@brief Pauses the engine and restarts it right away, re-centering the image on the viewer.
     **/
    void recenterViewer();
    
    /**
     *@brief Resets the video engine state and ensures that all worker threads are stopped.
     **/
    void stopEngine();
    
    /**
     *@brief displays progress if the time to compute the current frame exeeded
     * 0.5 sec.
     **/
    bool checkAndDisplayProgress(int y,int zoomedY);
    
    void onProgressUpdate(int i);
        
    void updateViewer();
    
    void cachedEngine();
    
    void allocateFrameStorage();
    
signals:
    /**
     *@brief Signal emitted when the function waits the time due to display the frame.
     **/
    void fpsChanged(double d);
    
    void doUpdateViewer();
    
    void doCachedEngine();
    
    void doFrameStorageAllocation();
    
public:
    /**
     *@brief Sets the _autoSaveOnNextRun to true
     **/
    void triggerAutoSaveOnNextRun(){_autoSaveOnNextRun = true;}
    
    /**
     *@brief Convenience function calling DAG::isOutputAViewer()
     **/
    bool isOutputAViewer() const {return _dag.isOutputAViewer();}
    
    /**
     *@brief Convenience function checking whether the dag has inputs connected.
     **/
    bool dagHasInputs() const {return !_dag.getInputs().empty();}
    
    /**
     *@brief Do not call this. This is called internally by the DAG GUI when the user changes the graph.
     **/
    void changeDAGAndStartEngine(Node* output);
    
    /**
     *@brief Convenience function. It resets the graph, emptying all nodes stored in the DAG.
     **/
    void resetDAG(){_dag.reset();}
    
    /**
     *@brief Convenience function. It calls DAG::resetAndSort(OutputNode*,bool).
     **/
    void resetAndMakeNewDag(Node* output,bool isViewer);
    
    /**
     *@returns Return a const reference to the DAG used by the video engine.
     **/
    const DAG& getCurrentDAG() const {return _dag;}
    
    
	/**
     *@brief For all nodes in input of out and recursively until there is no more inputs, their info
     *are cleared.
     **/
    void clearInfos(Node* out);
    
	/**
     *@returns Returns true if the engine is currently working.
     **/
	bool isWorking() const {return _working;}
    
    /**
     *@returns Returns the number of frame the engine is being executed for.
     *If it runs indefinately it will return -1. If it is stopped, it will return 0.
     **/
    int getFrameCountForCurrentPlayback() const {return _working ?  _lastRunArgs._frameRequestsCount :  0;}
    
    /**
     *@brief Constructs a VideoEngine instance. Currently the software only supports 1 VideoEngine,but
     *in the future it will be able to handle several engines working at the same time.
     *@param engine A pointer to the Model. It is currently unused.
     *@param lock A pointer to the general lock used by the engine. It is useful when it needs to do
     engine-wise synchronisaton;
     **/
    VideoEngine(QWaitCondition* openGLCondition,QMutex* mutex,QObject* parent = NULL);
    
    
    virtual ~VideoEngine();
    
    /**
     *@brief Tells all the nodes in the grpah to draw their overlays
     **/
    void drawOverlay() const;
	
    
    /**
     *@brief Starts the video engine. It can be called from anywhere and at anytime. It starts off at the current
     *frame indicated on the timeline in case the output of the graph is a viewer. If it is a writer then is starts
     *off at Writer::firstFrame().
     *@param frameCount[in] This is the number of frames you want to execute the engine for. -1 will make the
     *engine run until the end of the sequence is reached. If loop mode is enabled, the engine will never stops
     *until the user explicitly stops it.
     *@param fitFrameToViewer[in] If true, it will fit the first frame to the viewport. This is used when connecting
     *a node to the viewer for the first time.
     *@param forward[in] If true, the engine runs forwards, otherwise backwards.
     *@param sameFrame[in] If true, that means the engine will not increment/decrement the frame indexes and will run
     *for the same frame than the last frame  computed. This is used exclusively when zooming/panning.
     **/
    void render(int frameCount,bool fitFrameToViewer = false,bool forward = true,bool sameFrame = false);
    
    /**
     *@returns Returns the 64-bits key associated to the output node of the current graph. This key
     *represents the version of the graph.
     **/
    U64 getCurrentTreeVersion(){return _treeVersion;}
    
private:

    void engineLoop();

    
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
    
    /**
     *@brief Called by VideoEngine::drawOverlay().
     *@param output[in] The output node of the graph
     **/
    void _drawOverlay(Node *output) const;
    
    /**
     *@brief Called by almost all VideoEngine slots that respond to a user request(Seek,play,pause etc..).
     *All the parameters are given to the function VideoEngine::videoEngine(int,bool,bool,bool). See the
     *documentation for that function.
     **/
    void _startEngine(int frameNB,int frameCount,bool initViewer,bool forward,bool sameFrame,Node* output = NULL);
    
    /**
     *@brief Called by VideoEngine::changeDAGAndStartEngine(OutputNode*); This function is slightly different
     *than _startEngine(...) because it resets the graph and calls VideoEngine::changeTreeVersion()
     *before actually starting the engine.
     **/
    void _changeDAGAndStartEngine(int frameNB,int frameCount,bool initViewer,bool forward,bool sameFrame,Node* output = NULL);
    
    /**
     *@brief Appends a new VideoEngine::Task to the the queue.
     **/
    void appendTask(int frameNB,int frameCount,bool initViewer,bool forward,bool sameFrame,Node* output,VengineFunction func);
    
    /**
     *@brief Runs the queued tasks. It is called when the video engine stops the current computations.
     **/
    void runTasks();
    
    
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
