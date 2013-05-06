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
    /*This class represent one task that has to be performed by
     the engine and that is waiting.*/
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
    std::vector<InputNode*> inputs;
    OutputNode* output;
    Timer* _timer;
    QThreadPool* pool;
    
    std::map<int,Row*> row_cache; // buffer cache : for zoom 
    
    QMutex* _lock;
    std::map<Reader*, ReaderInfo* > readersInfos;
    bool _readerInfoHasChanged;
    std::map<float, std::pair<int,int> > builtInZooms;
    bool _aborted ;
    bool _paused;
    Hash _treeVersion;
    
    
public slots:
    void startEngine(int nbFrames =-1);
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
    
    void computeTreeForFrame(OutputNode *output,InputNode* input,int followingComputationsNb,bool initTexture);
    void computeFrameRequest();
    
    float closestBuiltinZoom(float v);
    float inferiorBuiltinZoom(float v);
    float superiorBuiltinZoom(float v);
    
    void clearInfos(Node* out);

	bool isWorking(){return _working;}
    
    VideoEngine(ViewerGL* gl_viewer,Model* engine,QMutex* lock);
    virtual ~VideoEngine();
    
    

	void pushReaderInfo(ReaderInfo* info,Reader* reader);
	void popReaderInfo(Reader* reader);
	void makeReaderInfoCurrent(Reader* reader);
	ReaderInfo* getReaderInfo(Reader* reader){return readersInfos[reader];}
    void drawOverlay();
    char* currentFrameName();
    
    std::pair<char*,FrameID> mapNewFrame(int frameNb,char* filename,int width,int height,int nbFrameHint);
    void closeMappedFile();
    
private:
    void updateDisplay();
    void _drawOverlay(Node *output);
    void video_sequence_engine(OutputNode *output,std::vector<InputNode*> inputs,
                               int nbFrames,bool initViewer=true,bool sameFrame=false,bool _runTasks=true);
    void backward_video_sequence_engine(OutputNode *output,std::vector<InputNode*> inputs,int nbFrames,bool _runTasks=true);
    void cached_video_engine(bool oneFrame,int startNb,std::vector<std::pair<char *,FramesIterator> > fileNames,bool firstFrame,bool forward);
    
    void fillBuilInZoomsLut();
    
    /*Row cache related functions*/
	std::map<int,Row*> get_row_cache(){return row_cache;}
    void addToRowCache(Row* row);
    void debugTree();
    void _debugTree(Node* n,int* nb);
    std::map<int,Row*>::iterator contained_in_cache(int rowIndex);
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
