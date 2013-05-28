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
#include "Core/viewercache.h"
#include "Core/hash.h"
#include "Reader/Reader.h"
#include "outputnode.h"
class InputNode;
class ViewerGL;
class Node;
class Row;
class ReaderInfo;
class Reader;
class Model;
class MMAPfile;
class Timer;

// Class that handles the core engine for video sequences, that's where all the work is done
class VideoEngine :public QObject{
    Q_OBJECT
    
public:
    /*Class describing the DAG internally */
    class DAG{
    public:
        typedef std::vector<Node*>::iterator DAGIterator;
        typedef std::vector<Node*>::reverse_iterator DAGReverseIterator;
        
        typedef std::vector<InputNode*>::iterator InputsIterator;
        DAG():_output(0){}
        
        /*Clear the structure and sorts the graph
         *represented by the OutputNode out*/
        void resetAndSort(OutputNode* out);
        
        /*Accessors to the sorted graph. Must be called
         *after resetAndSort(...) has been called*/
        DAGIterator begin(){return _sorted.begin();}
        DAGIterator end(){return _sorted.end();}
        DAGReverseIterator rbegin(){return _sorted.rbegin();}
        DAGReverseIterator rend(){return _sorted.rend();}
        Node* operator[](int index){return _sorted[index];}
        OutputNode* getOutput(){return _output;}
        
        /*Accessors to the inputs of the graph*/
        std::vector<InputNode*>& getInputs(){return _inputs;}
        
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
        /**/
        std::vector<InputNode*> _inputs;
    };
    
private:
    
    friend class ViewerGL;
    
    typedef void (VideoEngine::*VengineFunction)(int,int,bool);
    /*This class represents one task that has to be performed by
     the engine and that is waiting to be done.*/
    class Task{
    public:
        int _newFrameNB;
        int _frameCount;
        bool _initViewer;
        VengineFunction _func;
        Task(int newFrameNB,int frameCount,bool initViewer,VengineFunction func):
        _newFrameNB(newFrameNB),_frameCount(frameCount),_initViewer(initViewer),
        _func(func){}
        
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
    
    ViewerCache* _viewerCache ; // disk cache (physically stored, saved between 2 runs).    
    ViewerGL* gl_viewer;
    Model* _coreEngine;
	bool _working;
       
    DAG _dag; // object encapsulating the graph
    
    Timer* _timer; // fps timer
    
    std::map<int,Row*> row_cache; // buffer cache : for zoom 
    
    QMutex* _lock; // general lock for the engine
    std::map<Reader*, ReaderInfo* > readersInfos; // map of all readersInfos associated to the readers
    bool _readerInfoHasChanged; // true when the readerInfo for one reader has changed.This is used for the GlViewer


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
    void _abort();
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
    void seekRandomFrameWithStart(int f);
    void changeTreeVersion();
    void clearDiskCache();
    void clearPlayBackCache();

	/*clears the ROW CACHE*/
    void clearRowCache(){
        std::map<int,Row*>::iterator it = row_cache.begin();
        for(;it!=row_cache.end();it++) delete it->second;
        row_cache.clear();
    }
    void engineLoop();
    
    /*called internally by computeTreeForFrame*/
    void finishComputeFrameRequest();
    
signals:
    void fpsChanged(double d);

public:
  
    DAG& getCurrentDAG(){return _dag;}
    
	/*Executes the tree for one frame*/
    void computeTreeForFrame(std::string filename,OutputNode *output,int followingComputationsNb);
    
	/*clears-out all the node-infos in the graph*/
    void clearInfos(Node* out);

	/*Returns true if the engine is currently working*/
	bool isWorking(){return _working;}
    
    VideoEngine(ViewerGL* gl_viewer,Model* engine,QMutex* lock);
    virtual ~VideoEngine();
    
    /*Associates the info to the reader in the map*/
	void pushReaderInfo(ReaderInfo* info,Reader* reader);

	/*Removes the readerInfo associated to reader in the map*/
	void popReaderInfo(Reader* reader);

	/*Tells the GlViewer that the current readerInfo will be the info
	 *of the Reader "reader"*/
	void makeReaderInfoCurrent(Reader* reader);

	ReaderInfo* getReaderInfo(Reader* reader){return readersInfos[reader];}

	/*Tell all the nodes in the grpah to draw overlays*/
    void drawOverlay();
	
    
	/*Inserts a new frame in the disk cache*/
    std::pair<char*,ViewerCache::FrameID> mapNewFrame(int frameNb,
                                                      std::string filename,
                                                      int width,
                                                      int height,
                                                      int nbFrameHint);

	/*Close the LRU frame in the disk cache*/
    void closeMappedFile();
    
    /*starts the videoEngine.
     *frameCount is the number of frame you want to cycle. For all the sequence frameCount = -1
     *fitFrameToViewer is true if you want the first frame that will be played to fit to the viewer.
     *forward is true if you want the engine to go forward when cycling through frames. It goes backwards otherwise
     *sameFrame is true if the engine will play the same frame as before. It is exclusively used when zooming.*/
    void videoEngine(int frameCount,bool fitFrameToViewer = false,bool forward = true,bool sameFrame = false);
    
    /*used internally by the zoom or the videoEngine, do not call this*/
    void computeFrameRequest(bool sameFrame,bool forward,bool fitFrameToViewer,bool recursiveCall = false);
    
    /*the function cycling through the DAG for one scan-line*/
    void metaEnginePerRow(Row* row,OutputNode* output);
    
    typedef std::pair<Reader::Buffer::DecodedFrameDescriptor,ViewerCache::FramesIterator > ReadFrame;
    typedef std::vector< ReadFrame > FramesVector;
    
    
    VideoEngine::FramesVector startReading(std::vector<Reader*> readers,bool useMainThread,bool useOtherThread );
    
    void resetReadingBuffers();
    
    ViewerCache* getDiskCache(){return _viewerCache;}
    
private:
	/*calls updateGL() and cause the viewer to refresh*/
    void updateDisplay();
    void _drawOverlay(Node *output);
    
    /*Row cache related functions*/
	std::map<int,Row*> get_row_cache(){return row_cache;}
    void addToRowCache(Row* row);
    void debugTree();
    void _debugTree(Node* n,int* nb);
    std::map<int,Row*>::iterator isRowContainedInCache(int rowIndex);
    void computeTreeHash(std::vector< std::pair<char*,U64> > &alreadyComputed,Node* n);
    /*============================*/
    
    /*These are the functions called by the tasks*/
    void _previousFrame(int frameNB,int frameCount,bool initViewer);
    void _nextFrame(int frameNB,int frameCount,bool initViewer);
    void _previousIncrement(int frameNB,int frameCount,bool initViewer);
    void _nextIncrement(int frameNB,int frameCount,bool initViewer);
    void _firstFrame(int frameNB,int frameCount,bool initViewer);
    void _lastFrame(int frameNB,int frameCount,bool initViewer);
    void _seekRandomFrame(int frameNB,int frameCount,bool initViewer);
    
    
    /*functions handling tasks*/
    void appendTask(int frameNB,int frameCount,bool initViewer,VengineFunction func);
    void runTasks();
    
    /*Used internally by the computeFrameRequest func for cached frames*/
    void cachedFrameEngine(ViewerCache::FramesIterator fram);
    
    /*reset variables used by the videoEngine*/
    void stopEngine();
    
    void debugRowSequence();
    
};

#endif /* defined(__PowiterOsX__VideoEngine__) */
