//
//  VideoEngine.h
//  PowiterOsX
//
//  Created by Alexandre on 1/16/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#ifndef __PowiterOsX__VideoEngine__
#define __PowiterOsX__VideoEngine__

#include <iostream>
#include <vector>
#include <QtCore/qobject.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/QMutex>
#include <map>
#include "Core/diskcache.h"
#include "Core/hash.h"

class OutputNode;
class InputNode;
class ViewerGL;
class Node;
class Row;
class WorkerThread;
class ReaderInfo;
class Reader;
class Model;
class MMAPfile;
class Timer;

// Class that handles the core engine for video sequences, that's where all the work is done
class VideoEngine :public QObject{
    Q_OBJECT
    
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
    std::vector<Task> _waitingTasks;
    
    DiskCache* _cache ; // disk cache (physically stored, saved between 2 runs).    
    ViewerGL* gl_viewer;
    Model* _coreEngine;
    bool _firstTime;
	bool _working;
    std::vector<InputNode*> inputs; // the inputs of the current graph
    OutputNode* output; // the output of the current graph
    Timer* _timer; // fps timer
    QThreadPool* pool; // thread pool used by the workerthreads
    
    std::map<int,Row*> row_cache; // buffer cache : for zoom 
    
    QMutex* _lock; // general lock for the engine
    std::map<Reader*, ReaderInfo* > readersInfos; // map of all readersInfos associated to the readers
    bool _readerInfoHasChanged; // true when the readerInfo for one reader has changed.This is used for the GlViewer

	/*Map of the bultin zooms available. This is used by the down-scaling algorithm
	 *A builtin zoom is a factor, associated to a pair of ints.
	 *E.g if the zoom factor is 0.5, the pair of int will be 1,2. The first int tells how many
	 *rows (or pixels in case of line) we will keep in the down-res version. The Second int tells
	 *how many rows we will scan in the full-res version. It can be read as such :
	 *We keep X rows in Y rows of the input. */
    std::map<float, std::pair<int,int> > builtInZooms; 

    bool _aborted ;// true when the engine has been aborted, i.e: the user disconnected the viewer
    bool _paused; // true when the user pressed pause

    Hash _treeVersion;// the hash key associated to the current graph
    
    
public slots:
	/*starts the engine with initialisation of the viewer so the frame fits in.
	 *If nbFrames = -1, the engine cycles through all remaining frames*/
    void startEngine(int nbFrames =-1);

	/*starts the engine with or without the viewer initialisation so the frame fits in*/
    void startEngineWithOption(int nbFrames =-1,bool initViewer=true);
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
    void clearCache(){
        std::map<int,Row*>::iterator it = row_cache.begin();
        for(;it!=row_cache.end();it++) delete it->second;
        row_cache.clear();
    }
    
signals:
    void fpsChanged(double d);

public:
    void setInputNodes(std::vector<InputNode*>  nodes){ inputs = nodes;}
    void setOutputNode(OutputNode* node){ output = node;}
    
    std::vector<InputNode*> getInputNodes(){return inputs;}
    OutputNode* getOutputNode(){return output;}
    
	/*Executes the tree for one frame*/
    void computeTreeForFrame(OutputNode *output,InputNode* input,int followingComputationsNb,bool initTexture);

	/*starts the engine so it executes the tree for the SAME FRAME as the last
	 *execution of the engine. This is used by the zoom so it uses the rowCache.*/
    void computeFrameRequest();
    
	/*utility functions to get builtinzooms for the downscaling algorithm*/
    float closestBuiltinZoom(float v);
    float inferiorBuiltinZoom(float v);
    float superiorBuiltinZoom(float v);
    
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
	
	/*Get the name of the current frame*/
	char* currentFrameName();
    
	/*Inserts a new frame in the disk cache*/
    std::pair<char*,FrameID> mapNewFrame(int frameNb,char* filename,int width,int height,int nbFrameHint);

	/*Close the LRU frame in the disk cache*/
    void closeMappedFile();
    
private:
	/*calls updateGL() and cause the viewer to refresh*/
    void updateDisplay();
    void _drawOverlay(Node *output);

	/*Calls the video engine for the graph represented by output and inputs for nbFrames.
	 *initViewer is on if this is a new sequence (make the frame fit into the viewer).
	 *SameFrame is on if we call the engine for the sameFrame(for zoom purpose)
	 *RunTasks is on if we'd like run the waiting events (seek, previous, last;etc..)
	 *at the end of this run*/
    void videoSequenceEngine(OutputNode *output,std::vector<InputNode*> inputs,
                               int nbFrames,bool initViewer=true,bool sameFrame=false,bool _runTasks=true);

	/*Same as videoSequenceEngine but in reverse*/
    void backwardsVideoSequenceEngine(OutputNode *output,std::vector<InputNode*> inputs,int nbFrames,bool _runTasks=true);

	/*This function is called by videoSequenceEngine internally when it recognises that the frames to render are present in the cache*/
    void cachedVideoEngine(bool oneFrame,int startNb,std::vector<std::pair<char *,FramesIterator> > fileNames,bool firstFrame,bool forward);
    
    void fillBuilInZoomsLut();
    
    /*Row cache related functions*/
	std::map<int,Row*> get_row_cache(){return row_cache;}
    void addToRowCache(Row* row);
    void debugTree();
    void _debugTree(Node* n,int* nb);
    std::map<int,Row*>::iterator isRowContainedInCache(int rowIndex);
    void computeTreeHash(std::vector< std::pair<char*,U32> > &alreadyComputed,Node* n);
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
    
};

#endif /* defined(__PowiterOsX__VideoEngine__) */
