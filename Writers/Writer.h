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

 

 





#ifndef __PowiterOsX__Writer__
#define __PowiterOsX__Writer__

#include <map>
#include <iostream>
#include "Global/GlobalDefines.h"
#include "Engine/Node.h"
#include <QtCore/QStringList>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>
class Write;
class QMutex;
class Row;
class OutputFile_Knob;
class ComboBox_Knob;
class WriteKnobs;
class KnobCallback;
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
    
    
    Writer();
        
    virtual ~Writer();
    
    virtual const std::string className();
    
    virtual const std::string description();
    
    /*If forReal is true, it creates a new Write and set _writeHandle
     to this newly created Write. It also calls Write::setupFile with 
     the filename selected in the GUI. If the buffer is full, it appends
     the Write task to the _queuedTasks vector. They will be launched
     by the notifyWriterForCompletion() function.*/
    virtual bool _validate(bool forReal);
	
    /*Does the colorspace conversion using the appropriate LUT (using Write::engine)*/
	virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out);
        
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
    
    void incrementCurrentFrame(){++_currentFrame;}
    
    void setCurrentFrame(int f){
        if(f >= _frameRange.first && f <= _frameRange.second)
            _currentFrame = f;
    }
    
    const ChannelSet& requestedChannels() const {return _requestedChannels;} // FIXME: never return reference to internals!
    
    
    virtual int maximumInputs(){return 1;}
    
    virtual int minimumInputs(){return 1;}
    
    virtual bool cacheData(){return false;}
    
public slots:
    void notifyWriterForCompletion();
    void fileTypeChanged(int);
    void startRendering();
    void onFilesSelected();
    
protected:
	virtual void initKnobs(KnobCallback *cb);
    
    virtual ChannelSet supportedComponents(){return Powiter::Mask_All;}
    
private:
    
    ChannelSet _requestedChannels;
    int _currentFrame; // the current frame being rendered
    std::pair<int,int> _frameRange; // the range to render
    QMutex* _lock;
    bool _premult;
    Buffer _buffer;
    Write* _writeHandle;
    std::vector<Write*> _writeQueue;
    WriteKnobs* _writeOptions;
    std::string _filename;
    std::string _fileType;
    std::vector<std::string> _allFileTypes;
    
    
    OutputFile_Knob* _fileKnob;
    ComboBox_Knob* _filetypeCombo;
    
};

#endif /* defined(__PowiterOsX__Writer__) */
