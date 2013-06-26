//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef __PowiterOsX__VideoEngine__
#define __PowiterOsX__VideoEngine__

#include <iostream>
#include <vector>
#include <QtCore/qobject.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/QMutex>
#include <QtConcurrent/QtConcurrent>
#include <boost/bind.hpp>
#include <map>
#include "Core/hash.h"
#include <boost/noncopyable.hpp>
#include "Reader/Reader.h"

class FrameEntry;
class InputNode;
class OutputNode;
class ViewerGL;
class Node;
class Row;
class ReaderInfo;
class Reader;
class Model;
class Viewer;
class Writer;
class Timer;

// Class that handles the core engine for video sequences, that's where all the work is done
class VideoEngine :public QObject,public boost::noncopyable{
    Q_OBJECT
    
public:
    /*Class describing the DAG internally */
    class DAG{
    public:
        typedef std::vector<Node*>::const_iterator DAGIterator;
        typedef std::vector<Node*>::const_reverse_iterator DAGReverseIterator;
        typedef void (VideoEngine::DAG::*ValidateFunc)(bool);
        typedef std::vector<InputNode*>::const_iterator InputsIterator;
        
        DAG():_output(0),_isViewer(false){}
        
        /*Clears the structure and sorts the graph
         *represented by the OutputNode out*/
        void resetAndSort(OutputNode* out, bool isViewer);
        
        /*Clears the dag.*/
        void reset();
        
        /*Accessors to the sorted graph. Must be called
         *after resetAndSort(...) has been called*/
        DAGIterator begin() const {return _sorted.begin();}
        DAGIterator end() const {return _sorted.end();}
        DAGReverseIterator rbegin() const {return _sorted.rbegin();}
        DAGReverseIterator rend() const {return _sorted.rend();}
        Node* operator[](int index) const {return _sorted[index];}
        OutputNode* getOutput() const {return _output;}
        
        /*Convenience functions. Returns NULL in case
         the outputNode is not of the requested type.*/
        Viewer* outputAsViewer() const;
        Writer* outputAsWriter() const;
        
        bool isOutputAViewer() const {return _isViewer;}
        
        /*Accessors to the inputs of the graph*/
        const std::vector<InputNode*>& getInputs()const{return _inputs;}
        
            
        /*handy function to get the frame range
         of input nodes in the dag based on node infos.*/
        int firstFrame() const ;
        int lastFrame() const;
        
        /*sets infos accordingly across all the DAG
         and check if it is correct. (channels, frame
         range, etc...)*/
        bool validate(bool forReal);
        
        /*debug*/
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
        /*the output of the DAG*/
        OutputNode* _output;
        /*the un-sorted DAG*/
        std::vector<Node*> _graph;
        /*the sorted DAG*/
        std::vector<Node*> _sorted;
        /*all the inputs of the dag*/
        std::vector<InputNode*> _inputs;
        
        bool _isViewer; // true if the outputNode is a viewer, it avoids many dynamic_casts
    };
    
private:
    
    friend class ViewerGL;
    
    typedef void (VideoEngine::*VengineFunction)(int,int,bool,bool,OutputNode*);
    /*This class represents one task that has to be performed by
     the engine and that is waiting to be done.*/
    class Task{
    public:
        int _newFrameNB;
        int _frameCount;
        bool _initViewer;
        bool _forward;
        OutputNode* _output;
        VengineFunction _func;
        Task(int newFrameNB,int frameCount,bool initViewer,bool forward,OutputNode* output,VengineFunction func):
        _newFrameNB(newFrameNB),_frameCount(frameCount),_initViewer(initViewer),_forward(forward),
        _func(func),_output(output){}
        
    };
    /*Class holding informations about src and dst 
     for gpu memory transfer*/
    class GPUTransferInfo{
    public:
        GPUTransferInfo():src(0),dst(0),byteCount(0){}
        GPUTransferInfo(const char* s,void* d,size_t size):src(s),dst(d),byteCount(size){}
        void set(const char* s,void* d,size_t size){
            src=s;
            dst=d;
            byteCount=size;
        }
        const char* src;
        void* dst;
        size_t byteCount;
    };
    
        
    std::vector<Task> _waitingTasks;
    
    Model* _coreEngine;
    
	bool _working;
       
    DAG _dag; // object encapsulating the graph
    
    Timer* _timer; // fps timer
        
    QMutex* _lock; // general lock for the engine
   // std::map<Reader*, ReaderInfo* > readersInfos; // map of all readersInfos associated to the readers
   // bool _readerInfoHasChanged; // true when the readerInfo for one reader has changed.This is used for the GlViewer


    bool _aborted ;// true when the engine has been aborted, i.e: the user disconnected the viewer
    bool _paused; // true when the user pressed pause

    Hash _treeVersion;// the hash key associated to the current graph
    
    int _frameRequestsCount; // total frames in the current videoEngine
    
    int _frameRequestIndex;//counter of the frames computed
    
    bool _forward; // forwards/backwards video engine
    
    bool _loopMode; //on if the player will loop
        
    bool _sameFrame;//on if we want the next videoEngine call to be on the same frame(zoom)
    
    
    QFutureWatcher<void>* _engineLoopWatcher;
    QFuture<void>* _enginePostProcessResults;
    
    /*computing engine results*/
    QFutureWatcher<void>* _workerThreadsWatcher;
    QFuture<void>* _workerThreadsResults;
    /*The sequence of rows to process*/
    std::vector<Row*> _sequenceToWork;
    
    /*infos used by the engine when filling PBO's*/
    GPUTransferInfo _gpuTransferInfo;
    
public slots:
	/*starts the engine with initialisation of the viewer so the frame fits in.
	 *If nbFrames = -1, the engine cycles through all remaining frames*/
    void startEngine(int nbFrames =-1);

    void updateProgressBar();
    void setDesiredFPS(double d);
    void abort();
    void pause();
    void startPause(bool c);
    void startBackward(bool c);
    void previousFrame();
    void nextFrame();
    void firstFrame();
    void lastFrame();
    void previousIncrement();
    void nextIncrement();
    void seekRandomFrame(int f);
    void seekRandomFrame(double d){seekRandomFrame((int)d);} // convenience func for the FeedBackSpinbox class
    void changeTreeVersion();

    void engineLoop();
    
    /*called internally by computeTreeForFrame*/
    void finishComputeFrameRequest();
    
signals:
    void fpsChanged(double d);

public:
    
    bool isOutputAViewer() const {return _dag.isOutputAViewer();}
    
    bool dagHasInputs() const {return !_dag.getInputs().empty();}
  
    /*Do not call this. This is called internally by the DAG GUI*/
    void changeDAGAndStartEngine(OutputNode* output);
    
    void resetDAG(){_dag.reset();}
    
    void resetAndMakeNewDag(OutputNode* output,bool isViewer);
    
    const DAG& getCurrentDAG(){return _dag;}
    
	/*Executes the tree for one frame*/
    void computeTreeForFrame(std::string filename,OutputNode *output,bool fitFrameToViewer);
    
	/*clears-out all the node-infos in the graph*/
    void clearInfos(Node* out);

	/*Returns true if the engine is currently working*/
	bool isWorking(){return _working;}
    
    VideoEngine(Model* engine,QMutex* lock);
    virtual ~VideoEngine();
    

	/*Tell all the nodes in the grpah to draw overlays*/
    void drawOverlay();
	
    
    /*starts the videoEngine.
     *frameCount is the number of frame you want to cycle. For all the sequence frameCount = -1
     *fitFrameToViewer is true if you want the first frame that will be played to fit to the viewer.
     *forward is true if you want the engine to go forward when cycling through frames. It goes backwards otherwise
     *sameFrame is true if the engine will play the same frame as before. It is exclusively used when zooming.*/
    void videoEngine(int frameCount,bool fitFrameToViewer = false,bool forward = true,bool sameFrame = false);
    
    /*used internally by the zoom or the videoEngine, do not call this*/
    void computeFrameRequest(bool sameFrame,bool forward,bool fitFrameToViewer,bool recursiveCall = false);
    
    /*the function cycling through the DAG for one scan-line*/
    static void metaEnginePerRow(Row* row,OutputNode* output);
    
    typedef std::pair<Reader::Buffer::DecodedFrameDescriptor,FrameEntry*> ReadFrame;
    typedef std::vector< ReadFrame > FramesVector;
    
    
    VideoEngine::FramesVector startReading(std::vector<Reader*>& readers,bool useMainThread,bool useOtherThread );
    
    void resetReadingBuffers();
    
    
    U64 getCurrentTreeVersion(){return _treeVersion.getHashValue();}
    
private:
	/*calls updateGL() and cause the viewer to refresh*/
    void updateDisplay();
    void _drawOverlay(Node *output);
    
    void computeTreeHash(std::vector<std::pair<std::string, U64> > &alreadyComputed, Node* n);
    /*============================*/
    
    /*These are the functions called by the tasks*/
    void _startEngine(int frameNB,int frameCount,bool initViewer,bool forward,OutputNode* output = NULL);
    void _changeDAGAndStartEngine(int frameNB,int frameCount,bool initViewer,bool forward,OutputNode* output = NULL);
    
    /*functions handling tasks*/
    void appendTask(int frameNB,int frameCount,bool initViewer,bool forward,OutputNode* output,VengineFunction func);
    void runTasks();
    
    /*Used internally by the computeFrameRequest func for cached frames*/
    void cachedFrameEngine(FrameEntry* frame);
    
    /*reset variables used by the videoEngine*/
    void stopEngine();
    
    void debugRowSequence();
    
};

#endif /* defined(__PowiterOsX__VideoEngine__) */
