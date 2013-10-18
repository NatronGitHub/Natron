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

#include "Writer.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtConcurrentRun>

#include "Global/LibraryBinary.h"
#include "Global/AppManager.h"

#include "Engine/Row.h"
#include "Engine/Settings.h"
#include "Engine/Model.h"
#include "Engine/Settings.h"
#include "Engine/Knob.h"
#include "Engine/TimeLine.h"

#include "Writers/Write.h"


using namespace std;
using namespace Powiter;

Writer::Writer(Model* model):
OutputNode(model),
_requestedChannels(Mask_RGBA), // temporary
_premult(false),
_buffer(Settings::getPowiterCurrentSettings()->_writersSettings._maximumBufferSize),
_writeHandle(0),
_writeOptions(0),
_frameRangeChoosal(0),
_firstFrameKnob(0),
_lastFrameKnob(0)
{
    
    _lock = new QMutex;
    QObject::connect(_model->getApp()->getTimeLine().get(),
                     SIGNAL(frameRangeChanged(int,int)),
                     this,
                     SLOT(onTimelineFrameRangeChanged(int, int)));
}

Writer::~Writer(){
    if(_writeOptions){
        //    _writeOptions->cleanUpKnobs();
        delete _writeOptions;
    }
    delete _lock;
}

std::string Writer::className() const {
    return "Writer";
}

std::string Writer::description() const {
    return "The Writer node can render on disk the output of a node graph.";
}

bool Writer::validate() const{
    /*Defaults writing range to readers range, but
     the user may change it through GUI.*/
    if(_filename.empty()){
        return false;
        
    }
    return true;
}
Powiter::Status Writer::preProcessFrame(SequenceTime time){
    Write* write = 0;
    // setFrameRange(info().getFirstFrame(), info().getLastFrame());
    Powiter::LibraryBinary* encoder = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(_fileType);
    if(!encoder){
        cout << "ERROR: Couldn't find an appropriate encoder for filetype: " << _fileType << " (" << getName()<< ")" << endl;
        return StatFailed;
    }else{
        
        
        pair<bool,WriteBuilder> func = encoder->findFunction<WriteBuilder>("BuildWrite");
        if(func.first)
            write = func.second(this);
        else{
            cout <<"ERROR: Couldn't create the encoder for " << getName() << ", something is wrong in the plugin." << endl;
            return StatFailed;
        }
        write->premultiplyByAlpha(_premult);
        /*check if the filename already contains the extension, otherwise appending it*/
        QString extension;
        QString filename(_filename.c_str());
        int i = filename.lastIndexOf(QChar('.'));
        if(i != -1){
            extension.append(filename.toStdString().substr(i).c_str());
            filename = filename.replace(i+1, extension.size(), _fileType.c_str());
        }else{
            filename.append(extension);
        }
        
        i = filename.lastIndexOf(QChar('#'));
        QString n = QString::number(time);
        if(i != -1){
            filename = filename.replace(i,1,n);
        }else{
            i = filename.lastIndexOf(QChar('.'));
            filename = filename.insert(i, n);
        }
        
        write->setOptionalKnobsPtr(_writeOptions);
        Box2D rod;
        getRegionOfDefinition(time, &rod);
        write->setupFile(filename,rod);
        write->initializeColorSpace();
        _writeHandle = write;
        return StatOK;
    }
    
}


void Writer::renderRow(SequenceTime time,int left,int right,int y,const ChannelSet& channels){
    _writeHandle->renderRow(time,left,right,y,channels);
}


void Writer::initKnobs(){
    std::string fileDesc("File");
    _fileKnob = dynamic_cast<OutputFile_Knob*>(appPTR->getKnobFactory().createKnob("OutputFile", this, fileDesc));
    QObject::connect(_fileKnob,SIGNAL(filesSelected()),this,SLOT(onFilesSelected()));
    assert(_fileKnob);
    
    std::string renderDesc("Render");
    Button_Knob* renderButton = static_cast<Button_Knob*>(appPTR->getKnobFactory().createKnob("Button", this, renderDesc));
    assert(renderButton);
    QObject::connect(renderButton, SIGNAL(valueChangedByUser()), this, SLOT(startRendering()));
    
    std::string premultString("Premultiply by alpha");
    Bool_Knob* premult = static_cast<Bool_Knob*>(appPTR->getKnobFactory().createKnob("Bool", this, premultString));
    premult->setValue(_premult);
    
    std::string filetypeStr("File type");
    _filetypeCombo = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", this, filetypeStr));
    QObject::connect(_filetypeCombo, SIGNAL(valueChangedByUser()), this, SLOT(fileTypeChanged()));
    const std::map<std::string,Powiter::LibraryBinary*>& _encoders = Settings::getPowiterCurrentSettings()->_writersSettings.getFileTypesMap();
    std::map<std::string,Powiter::LibraryBinary*>::const_iterator it = _encoders.begin();
    for(;it!=_encoders.end();++it) {
        _allFileTypes.push_back(it->first.c_str());
    }
    _filetypeCombo->populate(_allFileTypes);
    
    
    _frameRangeChoosal = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", this, "Frame range"));
    QObject::connect(_frameRangeChoosal, SIGNAL(valueChangedByUser()), this, SLOT(onFrameRangeChoosalChanged()));
    std::vector<std::string> frameRangeChoosalEntries;
    frameRangeChoosalEntries.push_back("Inputs union");
    frameRangeChoosalEntries.push_back("Manual");
    _frameRangeChoosal->populate(frameRangeChoosalEntries);
    
}
void Writer::onFrameRangeChoosalChanged(){
    int index = _frameRangeChoosal->value<int>();
    if(index == 0){
        if(_firstFrameKnob){
            _firstFrameKnob->deleteKnob();
            _firstFrameKnob = 0;
        }
        if(_lastFrameKnob){
            _lastFrameKnob->deleteKnob();
            _lastFrameKnob = 0;
        }
    }else if(index == 1){
        _frameRange.first = _model->getApp()->getTimeLine()->firstFrame();
        _frameRange.second = _model->getApp()->getTimeLine()->lastFrame();
        if(!_firstFrameKnob){
            _firstFrameKnob = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory().createKnob("Int", this, "First frame"));
            _firstFrameKnob->setValue(_frameRange.first);
            _firstFrameKnob->setDisplayMinimum(_frameRange.first);
            _firstFrameKnob->setDisplayMaximum(_frameRange.second);
            _firstFrameKnob->setMinimum(_frameRange.first);
            _firstFrameKnob->setMaximum(_frameRange.second);

        }
        if(!_lastFrameKnob){
            _lastFrameKnob = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory().createKnob("Int", this, "Last frame"));
            _lastFrameKnob->setValue(_frameRange.second);
            _lastFrameKnob->setDisplayMinimum(_frameRange.first);
            _lastFrameKnob->setDisplayMaximum(_frameRange.second);
            _lastFrameKnob->setMinimum(_frameRange.first);
            _lastFrameKnob->setMaximum(_frameRange.second);

        }
        createKnobDynamically();
    }
}

void Writer::write(Write* write,QFutureWatcher<void>* watcher){
    
    _buffer.appendTask(write, watcher);
    if(!write) return;
    write->writeAndDelete();
    _buffer.removeTask(write);
}

void Writer::startWriting(){
    if(_buffer.size() < _buffer.getMaximumBufferSize()){
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>;
        QObject::connect(watcher, SIGNAL(finished()), this, SLOT(notifyWriterForCompletion()));
        QFuture<void> future = QtConcurrent::run(this,&Writer::write,_writeHandle,watcher);
        _writeHandle = 0;
        watcher->setFuture(future);
        watcher->waitForFinished();
    }else{
        _writeQueue.push_back(_writeHandle);
        _writeHandle = 0;
    }
}

void Writer::notifyWriterForCompletion(){
    _buffer.emptyTrash();
    
    /*Several threads may fight here to try to start another task.
     We ensure that at least 1 thread get into the condition, as more
     are not needed. */
    QMutexLocker locker(_lock);
    if(_writeQueue.size() > 0 && _buffer.size() < _buffer.getMaximumBufferSize()){
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>;
        QObject::connect(watcher, SIGNAL(finished()), this, SLOT(notifyWriterForCompletion()));
        QFuture<void> future = QtConcurrent::run(this,&Writer::write,_writeQueue[0],watcher);
        _writeQueue.erase(_writeQueue.begin());
        watcher->setFuture(future);
        watcher->waitForFinished();
    }
}

void Writer::Buffer::appendTask(Write* task,QFutureWatcher<void>* future){
    _tasks.push_back(make_pair(task, future));
}

void Writer::Buffer::removeTask(Write* task){
    for (U32 i = 0 ; i < _tasks.size(); ++i) {
        std::pair<Write*,QFutureWatcher<void>* >& t = _tasks[i];
        if(t.first == task){
            _trash.push_back(t.second);
            _tasks.erase(_tasks.begin()+i);
            break;
        }
    }
}
void Writer::Buffer::emptyTrash(){
    for (U32 i = 0; i < _trash.size(); ++i) {
        delete _trash[i];
    }
    _trash.clear();
}

Writer::Buffer::~Buffer(){
    for (U32 i = 0 ; i < _tasks.size(); ++i) {
        std::pair<Write*,QFutureWatcher<void>* >& t = _tasks[i];
        delete t.second;
    }
}

bool Writer::validInfosForRendering(){
    /*check if filetype is valid*/
    Powiter::LibraryBinary* isValid = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(_fileType);
    if(!isValid) {
        return false;
    }
    /*checking if channels are supported*/
    pair<bool,WriteBuilder> func = isValid->findFunction<WriteBuilder>("BuildWrite");
    assert(func.second);
    Write* write = func.second(this);
    assert(write);
    try {
        write->supportsChannelsForWriting(_requestedChannels);
    } catch (const std::exception &e) {
        cout << "ERROR: " << e.what() << endl;
    }
    delete write;
    
    /*check if frame range makes sense*/
    if(_frameRange.first > _frameRange.second) return false;
    
    /*check if write specific knobs have valid values*/
    if (_writeOptions) {
        if (!_writeOptions->allValid()) {
            return false;
        }
    }
    if(_filename.empty()){
        return false;
    }
    if (!input(0)) {
        return false;
    }
    
    return true;
}

void Writer::startRendering(){
    
    _fileType = _allFileTypes[_filetypeCombo->value<int>()];
    _filename = _fileKnob->value<QString>().toStdString();
    int index = _frameRangeChoosal->value<int>();
    if(index == 0){
        getFrameRange(&_frameRange.first, &_frameRange.second);
    }else{
        _frameRange.first = _firstFrameKnob->value<int>();
        _frameRange.second = _lastFrameKnob->value<int>();
    }
    
    if(validInfosForRendering()){
        getVideoEngine()->refreshTree();
        updateTreeAndRender();
        emit renderingOnDiskStarted(this,_filename.c_str(),_frameRange.first,_frameRange.second);
        
    }
}

void Writer::onTimelineFrameRangeChanged(int f,int l){
    
    if(_firstFrameKnob){
        _firstFrameKnob->setValue(f);
        _firstFrameKnob->setMinimum(f);
        _firstFrameKnob->setDisplayMinimum(f);
        _firstFrameKnob->setDisplayMaximum(l);
        _firstFrameKnob->setMaximum(l);
        
    }
    if(_lastFrameKnob){
        _lastFrameKnob->setValue(l);
        _lastFrameKnob->setDisplayMinimum(f);
        _lastFrameKnob->setDisplayMaximum(l);
        _lastFrameKnob->setMinimum(f);
        _lastFrameKnob->setMaximum(l);
        
    }

}
void Writer::fileTypeChanged(){
    int index = _filetypeCombo->value<int>();
    assert(index < (int)_allFileTypes.size());
    _fileType = _allFileTypes[index];
    if(_writeOptions){
        _writeOptions->cleanUpKnobs();
        delete _writeOptions;
        _writeOptions = 0;
    }
    Powiter::LibraryBinary* isValid = Settings::getPowiterCurrentSettings()->_writersSettings.encoderForFiletype(_fileType);
    if(!isValid) return;
    
    QString file(_filename.c_str());
    int pos = file.lastIndexOf(QChar('.'));
    ++pos;
    file.replace(pos, file.size() - pos, _fileType.c_str());
    _fileKnob->setValue(file);
    
    /*checking if channels are supported*/
    pair<bool,WriteBuilder> func = isValid->findFunction<WriteBuilder>("BuildWrite");
    if(func.first){
        Write* write = func.second(this);
        _writeOptions = write->initSpecificKnobs();
        if(_writeOptions)
            _writeOptions->initKnobs(_fileType);
        delete write;
    }
}

void Writer::onFilesSelected(){
    _filename = _fileKnob->getValueAsVariant().toString().toStdString();
}
