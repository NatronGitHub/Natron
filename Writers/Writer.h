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

#ifndef POWITER_WRITERS_WRITER_H_
#define POWITER_WRITERS_WRITER_H_

#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "Global/Macros.h"
#include "Engine/Node.h"

class Encoder;
class QMutex;
namespace Powiter{
class Row;
}
class OutputFile_Knob;
class ComboBox_Knob;
class EncoderKnobs;
class Int_Knob;
class Writer: public OutputNode{
    
    Q_OBJECT
    
public:
    /*The Write::Buffer is a simple buffer storing Write handles that are
     still working. A Write handle can be seen as the task of writing 1 frame
     to a file. When done,the write will be removed from the buffer.*/
    class Buffer{
        int _maxBufferSize;
        std::vector< std::pair<Encoder*,QFutureWatcher<void>* > > _tasks;
        
        std::vector<QFutureWatcher<void>* > _trash;
    public:
        Buffer(int size):_maxBufferSize(size){}
        ~Buffer();
        
        /*Append a task with its future*/
        void appendTask(Encoder* task,QFutureWatcher<void>* future);
        
        /*Remove the task.Does not delete it, but deletes its future.*/
        void removeTask(Encoder* task);
        
        /*Deletes futures that needs to be deleted*/
        void emptyTrash();
        
        /*see Writer::setMaximumBufferSize()*/
        void setMaximumBufferSize(int size){_maxBufferSize = size;}
        
        int getMaximumBufferSize(){return _maxBufferSize;}
        
        int size(){return _tasks.size();}
        
    };
      
    Writer(AppInstance* app);
        
    virtual ~Writer();
    
    virtual std::string className() const OVERRIDE;
    
    virtual std::string description() const OVERRIDE;
    
    /*If doFullWork is true, it creates a new Write and set _writeHandle
     to this newly created Write. It also calls Write::setupFile with 
     the filename selected in the GUI. If the buffer is full, it appends
     the Write task to the _queuedTasks vector. They will be launched
     by the notifyWriterForCompletion() function.*/
    virtual bool validate() const;
    
    virtual Powiter::Status preProcessFrame(SequenceTime time) OVERRIDE;
	
    /*Does the colorspace conversion using the appropriate LUT (using Write::engine)*/
	virtual void renderRow(SequenceTime time,int left,int right,int y,const ChannelSet& channels);
            
    /* This function is called by startWriting()
     - Initialises the output color-space for _writeHandle
      - Calls Write::writeAllData for _writeHandle, and removes it from the buffer
     when done.
     WARNING: This function assumes _validate(true) has been called ! Otherwise
     the _writeHandle member would be NULL.*/
    void write(Encoder* write,QFutureWatcher<void>*);
    
    /*calls write() appropriatly in another thread. When finished, the thread
     calls notifyWriterForCompletion(). */
    void startWriting();
    
    /*controls how many simultaneous writes the Writer can do.
     Default is 2*/
    void setMaximumBufferSize(int size){_buffer.setMaximumBufferSize(size);}
    
    /*Returns true if all parameters are OK to start the rendering.
     This is used to know whether it should start the engine or not.*/
    bool validInfosForRendering();
    
    const ChannelSet& requestedChannels() const {return _requestedChannels;}    
    
    virtual int maximumInputs() const OVERRIDE {return 1;}
    
    virtual int minimumInputs() const OVERRIDE {return 1;}
    
    virtual bool cacheData() const OVERRIDE {return false;}
    
    
    int firstFrame() const {return _frameRange.first;}
    
    int lastFrame() const {return _frameRange.second;}
    
public slots:
    void notifyWriterForCompletion();
    void onFilesSelected();
    void fileTypeChanged();
    void startRendering();
    void onFrameRangeChoosalChanged();
    void onTimelineFrameRangeChanged(int,int);
signals:
    void renderingOnDiskStarted(Writer*,QString,int,int);
    
protected:
	virtual void initKnobs() OVERRIDE;
    
    
private:
    
    ChannelSet _requestedChannels;
    QMutex* _lock;
    bool _premult;
    Buffer _buffer;
    Encoder* _writeHandle;
    std::vector<Encoder*> _writeQueue;
    EncoderKnobs* _writeOptions;
    std::string _filename;
    std::string _fileType;
    std::vector<std::string> _allFileTypes;
    
    std::pair<int,int> _frameRange;
    
    OutputFile_Knob* _fileKnob;
    ComboBox_Knob* _filetypeCombo;
    ComboBox_Knob* _frameRangeChoosal;
    Int_Knob* _firstFrameKnob;
    Int_Knob* _lastFrameKnob;
    
};
Q_DECLARE_METATYPE(Writer*);
#endif /* defined(POWITER_WRITERS_WRITER_H_) */
