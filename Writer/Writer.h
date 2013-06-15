//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com


#ifndef __PowiterOsX__Writer__
#define __PowiterOsX__Writer__

#include <map>
#include <iostream>
#include "Superviser/powiterFn.h"
#include "Core/outputnode.h"
#include <QtCore/QStringList>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>
class Write;
class Row;
class Knob_Callback;
class Writer: public QObject, public OutputNode{
    
    Q_OBJECT
    
public:
    /*The Write::Buffer is a simple buffer storing Write handles that are
     still working. A Write handle can be seen as the task of writing 1 frame
     to a file. When done,the write will be removed from the buffer.*/
    class Buffer{
        int _maxBufferSize;
        std::vector< std::pair<Write*,QFutureWatcher<void>* > > _tasks;
        
        std::vector<QFutureWatcher<void>* > _trash;
    public:
        Buffer(int size):_maxBufferSize(size){}
        ~Buffer();
        
        /*Append a task with its future*/
        void appendTask(Write* task,QFutureWatcher<void>* future);
        
        /*Remove the task.Does not delete it, but deletes its future.*/
        void removeTask(Write* task);
        
        /*Deletes futures that needs to be deleted*/
        void emptyTrash();
        
        /*see Writer::setMaximumBufferSize()*/
        void setMaximumBufferSize(int size){_maxBufferSize = size;}
        
        int getMaximumBufferSize(){return _maxBufferSize;}
        
        int size(){return _tasks.size();}
        
    };
    
    
    Writer(Node* node);
    
    Writer(Writer& ref);
    
    virtual ~Writer();
    
    virtual std::string className();
    
    virtual std::string description();
    
    /*If forReal is true, it creates a new Write and set _writeHandle
     to this newly created Write. It also calls Write::setupFile with 
     the filename selected in the GUI. If the buffer is full, it appends
     the Write task to the _queuedTasks vector. They will be launched
     by the notifyWriterForCompletion() function.*/
    virtual void _validate(bool forReal);
	
    /*Does the colorspace conversion using the appropriate LUT (using Write::engine)*/
	virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
        
	virtual void createKnobDynamically();
    
    /* This function is called by startWriting()
     - Initialises the output color-space for _writeHandle
      - Calls Write::writeAllData for _writeHandle, and removes it from the buffer
     when done.
     WARNING: This function assumes _validate(true) has been called ! Otherwise
     the _writeHandle member would be NULL.*/
    void write(Write* write,QFutureWatcher<void>*);
    
    /*calls write() appropriatly in another thread. When finished, the thread
     calls notifyWriterForCompletion(). */
    void startWriting();
    
    /*controls how many simultaneous writes the Writer can do.
     Default is 2*/
    void setMaximumBufferSize(int size){_buffer.setMaximumBufferSize(size);}
    
    /*Returns true if all parameters are OK to start the rendering.
     This is used to know whether it should start the engine or not.*/
    bool validInfosForRendering();
    
    int currentFrame(){return _currentFrame;}
    
    int firstFrame(){return _frameRange.first;}
    
    int lastFrame(){return _frameRange.second;}
    
    void setCurrentFrameToStart(){_currentFrame = _frameRange.first;}
    
    void incrementCurrentFrame(){_currentFrame++;}
    
    
public slots:
    void notifyWriterForCompletion();
    void startRendering();
    
protected:
	virtual void initKnobs(Knob_Callback *cb);
    
private:
    
    
    int _currentFrame; // the current frame being rendered
    std::pair<int,int> _frameRange; // the range to render
    
    bool _premult;
    Buffer _buffer;
    Write* _writeHandle;
    std::string _filename;
    std::string _fileType;
    
};

#endif /* defined(__PowiterOsX__Writer__) */
