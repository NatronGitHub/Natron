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

#include "Reader.h"

#include <cassert>
#include <sstream>

#include <QtCore/QMutex>
#include <QtGui/QImage>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include "Global/Macros.h"
#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"


#include "Engine/Node.h"
#include "Engine/MemoryFile.h"
#include "Engine/VideoEngine.h"
#include "Engine/Model.h"
#include "Engine/Settings.h"
#include "Engine/Box.h"
#include "Engine/Format.h"
#include "Engine/ViewerCache.h"
#include "Engine/ViewerNode.h"
#include "Engine/Knob.h"

#include "Readers/ReadFfmpeg_deprecated.h"
#include "Readers/ReadExr.h"
#include "Readers/ReadQt.h"
#include "Readers/Read.h"

#include "Writers/Writer.h"

using namespace Powiter;
using namespace std;


Reader::Reader(Model* model)
: Node(model)
, _preview()
, _has_preview(false)
, _fitFrameToViewer(false)
, _readHandle(0)
, _buffer()
, _fileKnob(0)
, _readMutex()
{
}

Reader::~Reader(){
    _buffer.clear();
}

std::string Reader::className() const {
    return "Reader";
}

std::string Reader::description() const {
    return "The reader node can read image files sequences.";
}

void Reader::initKnobs(){
    std::string desc("File");
    _fileKnob = dynamic_cast<File_Knob*>(appPTR->getKnobFactory()->createKnob("InputFile", this, desc));
    QObject::connect(_fileKnob, SIGNAL(valueChangedByUser()), this, SLOT(showFilePreview()));
    QObject::connect(_fileKnob, SIGNAL(frameRangeChanged(int,int)), this, SLOT(onFrameRangeChanged(int,int)));
    assert(_fileKnob);
}




bool Reader::readCurrentHeader(int current_frame){
    QString filename = _fileKnob->getRandomFrameName(current_frame);
    
    /*the read handle used to decode the frame*/
    Read* _read = 0;
    
    QString extension;
    for (int i = filename.size() - 1; i >= 0; --i) {
        QChar c = filename.at(i);
        if(c != QChar('.'))
            extension.prepend(c);
        else
            break;
    }
    
    Powiter::LibraryBinary* decoder = Settings::getPowiterCurrentSettings()->_readersSettings.decoderForFiletype(extension.toStdString());
    if(!decoder){
        cout << "ERROR: Couldn't find an appropriate decoder for this filetype ( " << extension.toStdString() << " )" << endl;
        return false;
    }

    pair<bool,ReadBuilder> func = decoder->findFunction<ReadBuilder>("BuildRead");
    if(func.first){
        _read = func.second(this);
    }else{
        cout << "ERROR: Failed to create the decoder for " << getName()  << ",something is wrong in the plugin."<< endl;
        return false;
    }
    /*In case the read handle supports scanlines, we read the header to determine
     how many scan-lines we would need, depending also on the viewer context.*/
    std::string filenameStr = filename.toStdString();
    std::vector<int> rows;
    /*the slContext is useful to check the equality of 2 scan-line based frames.*/
    Reader::Buffer::ScanLineContext *slContext = 0;
    if(_read->supportsScanLine()){
        _read->readHeader(filename, false);
        slContext = new Reader::Buffer::ScanLineContext;
        /*TEMPORARY FIX while OpenFX nodes still require the full RoD. This would let the Reads work
         not properly since they need more rows than just what the viewer wants to display. */
//        if(ctrlPTR->getModel()->getVideoEngine()->isOutputAViewer()){
//            const Format &dispW = _read->getReaderInfo()->displayWindow();
//            if(_fitFrameToViewer){
//                currentViewer->getUiContext()->viewer->fitToFormat(dispW);
//            }
//            currentViewer->getUiContext()->viewer->computeRowSpan(rows,dispW);
//        }else{
            const Box2D& dataW = _read->readerInfo().dataWindow();
            for (int i = dataW.bottom() ; i < dataW.top(); ++i) {
                rows.push_back(i);
            }            
        // }
        slContext->setRows(rows);
    }
    /*Now that we have the slContext we can check whether the frame is already enqueued in the buffer or not.*/
    Reader::Buffer::DecodedFrameIterator found = _buffer.isEnqueued(filenameStr,Buffer::ALL_FRAMES);
    if(found !=_buffer.end()){
        assert(*found);
        if(!(*found)->supportsScanLines()){
            delete _read;
        }else{
            Reader::Buffer::ScanLineDescriptor *slDesc = static_cast<Reader::Buffer::ScanLineDescriptor*>(*found);
            
            /*we found a buffered frame with a scanline context. We can now compute
             the intersection between the current scan-line context and the one found
             to find out which rows we need to compute*/
            slDesc->_slContext->computeIntersectionAndSetRowsToRead(slContext->getRows());
            delete _read;
        }
        assert((*found)->_readHandle);
        _info = (*found)->_readHandle->readerInfo();
        _readHandle = (*found)->_readHandle;
    }else{
        _read->initializeColorSpace();
        if(_read->supportsScanLine()){
            _buffer.insert(new Reader::Buffer::ScanLineDescriptor(_read,filenameStr,slContext));
        }else{
            _read->readHeader(filename, false);
            _buffer.insert(new Reader::Buffer::FullFrameDescriptor(_read,filenameStr));
        }
        _info = _read->readerInfo();
        _readHandle = _read;
    }
    return true;
}

void Reader::readCurrentData(int current_frame){
    
    QString filename = _fileKnob->getRandomFrameName(current_frame);
    
    /*Now that we have the slContext we can check whether the frame is already enqueued in the buffer or not.*/
    Reader::Buffer::DecodedFrameIterator found = _buffer.isEnqueued(filename.toStdString(),Buffer::ALL_FRAMES);
    if(found == _buffer.end()){
        cout << "ERROR: Buffer does not contains the header for frame " << filename.toStdString()
        <<". Something is wrong (" << getName() << ")" << endl;
        return;
    }
    assert(*found);
    if((*found)->hasToDecode()){
        if((*found)->supportsScanLines()){
            Buffer::ScanLineDescriptor* slDesc = static_cast<Buffer::ScanLineDescriptor*>(*found);
            assert(slDesc->_readHandle);
            slDesc->_readHandle->readScanLineData(slDesc->_slContext);
            slDesc->_hasRead = true;
        }else{
            Buffer::FullFrameDescriptor* ffDesc = static_cast<Buffer::FullFrameDescriptor*>(*found);
            assert(ffDesc->_readHandle);
            ffDesc->_readHandle->readData();
            ffDesc->_hasRead = true;
        }
    }
}


void Reader::showFilePreview(){
#warning preview is un-safe especially with saved projects.
    _buffer.clear();
    fitFrameToViewer(false);
    {
        QMutexLocker locker(&_readMutex);
        if(readCurrentHeader(firstFrame())){
            readCurrentData(firstFrame());
            assert(_readHandle);
            _readHandle->make_preview();
        }
    }
    _buffer.clear();
}




bool Reader::_validate(bool){
    _info.set_firstFrame(firstFrame());
    _info.set_lastFrame(lastFrame());
    
    return true;
}

void Reader::engine(int y,int offset,int range,ChannelSet c,Row* out){
	assert(_readHandle);
    _readHandle->engine(y,offset,range,c,out);
	
}

void Reader::createKnobDynamically(){
	Node::createKnobDynamically();
}


void Reader::Buffer::insert(Reader::Buffer::Descriptor* desc){
    //if buffer is full, we remove previously computed frame
    if(_buffer.size() == (U32)_bufferSize){
        for (U32 i = 0 ;i < _buffer.size(); ++i) {
            Reader::Buffer::Descriptor* frameToRemove = _buffer[i];
            assert(frameToRemove);
            if(!frameToRemove->hasToDecode()){
                erase(_buffer.begin()+i);
                break;
            }
        }
    }
    _buffer.push_back(desc);
}
Reader::Buffer::DecodedFrameIterator Reader::Buffer::find(const std::string& filename){
    for (int i = _buffer.size()-1; i >= 0 ; --i) {
        assert(_buffer[i]);
        if(_buffer[i]->_filename==filename) {
            return _buffer.begin()+i;
        }
    }
    return _buffer.end();
}

void Reader::Buffer::remove(const std::string& filename){
    DecodedFrameIterator it = find(filename);
    if (it!=_buffer.end()) {
        assert(*it);
        if((*it)->_readHandle)
            delete (*it)->_readHandle; // delete readHandle
        delete (*it);
        _buffer.erase(it);
    }
}


bool Reader::Buffer::decodeFinished(const std::string& filename){
    Buffer::DecodedFrameIterator it = find(filename);
    return (it!=_buffer.end());
}
void Reader::Buffer::debugBuffer(){
    cout << "=========BUFFER DUMP=============" << endl;
    for (DecodedFrameIterator it = _buffer.begin(); it != _buffer.end(); ++it) {
        assert(*it);
        cout << (*it)->_filename << endl;
    }
    cout << "=================================" << endl;
}

Reader::Buffer::DecodedFrameIterator Reader::Buffer::isEnqueued(const std::string& filename,SEARCH_TYPE searchMode){
    if(searchMode == SCANLINE_FRAME){
        DecodedFrameIterator ret = find(filename);
        if (ret != _buffer.end()) {
            assert(*ret);
            assert((*ret)->_readHandle);
            if(!(*ret)->_readHandle->supportsScanLine()){
                return _buffer.end();
            }
            return ret;
        }else{
            return _buffer.end();
        }
    }else if(searchMode == FULL_FRAME){
        DecodedFrameIterator ret = find(filename);
        if(ret != _buffer.end()){
            assert(*ret);
            assert((*ret)->_readHandle);
            if((*ret)->_readHandle->supportsScanLine()){
                return _buffer.end();
            }
            return ret;
        }else{
            return _buffer.end();
        }
    }else{ // all frames
        return find(filename);
    }
}

void Reader::Buffer::clear(){
    DecodedFrameIterator it = _buffer.begin();
    for (; it!=_buffer.end(); ++it) {
        assert(*it);
        if((*it)->_readHandle)
            delete (*it)->_readHandle; // delete readHandle
        delete *it;
    }
    _buffer.clear();
}

void Reader::Buffer::erase(DecodedFrameIterator it) {
    assert(*it);
    if((*it)->_readHandle)
        delete (*it)->_readHandle; // delete readHandle
    delete (*it);
    _buffer.erase(it);
}


int Reader::firstFrame(){
    return _fileKnob->firstFrame();
}
int Reader::lastFrame(){
    return _fileKnob->lastFrame();
}
int Reader::nearestFrame(int f){
    return _fileKnob->nearestFrame(f);
}

const QString Reader::getRandomFrameName(int f) const{
    return _fileKnob->getRandomFrameName(f);
}

void Reader::onFrameRangeChanged(int first,int last){
    _info.set_firstFrame(first);
    _info.set_lastFrame(last);
}

void Reader::setPreview(const QImage& img){
    _preview = img;
    if (!img.isNull()) {
        _has_preview = true;
    }
    notifyGuiPreviewChanged();
}


/*Adds to _rowsToRead the rows in others that are missing to _rows*/
void Reader::Buffer::ScanLineContext::computeIntersectionAndSetRowsToRead(const std::vector<int>& others){
    std::list<int> rowsCopy (_rows.begin(), _rows.end()); // std::list is more efficient with respect to erase()
    for (ScanLineConstIterator it = others.begin(); it!=others.end(); ++it) {
        std::list<int>::iterator found = std::find(rowsCopy.begin(),rowsCopy.end(),*it);
        if(found == rowsCopy.end()){ // if not found, we add the row to rows
            _rowsToRead.push_back(*it);
        }else{
            rowsCopy.erase(found); // otherwise , we erase the row from the copy to speed up the computation of the intersection
        }
    }
}

/*merges _rowsToRead and _rows*/
void Reader::Buffer::ScanLineContext::merge(){
    for( U32 i = 0; i < _rowsToRead.size(); ++i) {
        _rows.push_back(_rowsToRead[i]);
    }
    _rowsToRead.clear();
}


//int _firstFrame;
//int _lastFrame;
//int _ydirection;
//bool _blackOutside;
//bool _rgbMode;
//Format _displayWindow; // display window of the data, for the data window see x,y,range,offset parameters
void ReaderInfo::writeToXml(QXmlStreamWriter* writer){
    writer->writeAttribute("CurrentFrameName",_currentFrameName.c_str());
    writer->writeAttribute("FirstFrame",QString::number(firstFrame()));
    writer->writeAttribute("LastFrame",QString::number(lastFrame()));
    writer->writeAttribute("RgbMode",QString::number(rgbMode()));
    
    const Format& dispW = displayWindow();
    writer->writeStartElement("DisplayWindow");
    writer->writeAttribute("left",QString::number(dispW.left()));
    writer->writeAttribute("bottom",QString::number(dispW.bottom()));
    writer->writeAttribute("right",QString::number(dispW.right()));
    writer->writeAttribute("top",QString::number(dispW.top()));
    writer->writeEndElement();
    
    const Box2D& dataW = dataWindow();
    
    writer->writeStartElement("DataWindow");
    writer->writeAttribute("left",QString::number(dataW.left()));
    writer->writeAttribute("bottom",QString::number(dataW.bottom()));
    writer->writeAttribute("right",QString::number(dataW.right()));
    writer->writeAttribute("top",QString::number(dataW.top()));
    
    QString chans;
    foreachChannels(chan, channels()){
        chans += getChannelName(chan).c_str() + QString("|");
    }
    writer->writeAttribute("Channels",chans);
}

ReaderInfo* ReaderInfo::fromXml(QXmlStreamReader* reader){
    QString currentFrameName;
    QString firstFrameStr,lastFrameStr,rgbStr,frmtXStr,frmtYStr,frmtRStr,frmtTStr;
    QString bboxXStr,bboxYStr,bboxRStr,bboxTStr,channelsStr;
    
    QXmlStreamAttributes attributes = reader->attributes();
    if(attributes.hasAttribute("CurrentFrameName")){
        currentFrameName = attributes.value("CurrentFrameName").toString();
    }else{
        return NULL;
    }
    if(attributes.hasAttribute("FirstFrame")){
        currentFrameName = attributes.value("FirstFrame").toString();
    }else{
        return NULL;
    }
    if(attributes.hasAttribute("LastFrame")){
        lastFrameStr = attributes.value("LastFrame").toString();
    }else{
        return NULL;
    }
    if(attributes.hasAttribute("RgbMode")){
        rgbStr = attributes.value("RgbMode").toString();
    }else{
        return NULL;
    }
    if(attributes.hasAttribute("Channels")){
        channelsStr = attributes.value("Channels").toString();
    }else{
        return NULL;
    }
    QXmlStreamReader::TokenType token = reader->readNext();
    int i = 0;
    while(token == QXmlStreamReader::StartElement && i < 2){
        if(reader->name() == "DisplayWindow"){
            QXmlStreamAttributes dispWAtts = reader->attributes();
            if(attributes.hasAttribute("left")){
                frmtXStr = attributes.value("left").toString();
            }else{
                return NULL;
            }
            if(attributes.hasAttribute("bottom")){
                frmtYStr = attributes.value("bottom").toString();
            }else{
                return NULL;
            }
            if(attributes.hasAttribute("right")){
                frmtRStr = attributes.value("right").toString();
            }else{
                return NULL;
            }
            if(attributes.hasAttribute("top")){
                frmtTStr = attributes.value("top").toString();
            }else{
                return NULL;
            }
            

        }else if(reader->name() == "DataWindow"){
            QXmlStreamAttributes dataWAtts = reader->attributes();
            if(attributes.hasAttribute("left")){
                bboxXStr = attributes.value("left").toString();
            }else{
                return NULL;
            }
            if(attributes.hasAttribute("bottom")){
                bboxYStr = attributes.value("bottom").toString();
            }else{
                return NULL;
            }
            if(attributes.hasAttribute("right")){
                bboxRStr = attributes.value("right").toString();
            }else{
                return NULL;
            }
            if(attributes.hasAttribute("top")){
                bboxTStr = attributes.value("top").toString();
            }else{
                return NULL;
            }
        }
        ++i;
    }

    ReaderInfo* out = new ReaderInfo;

    ChannelSet channels;
    i = 0;
    while(i < channelsStr.size()){
        QString chan;
        while(channelsStr.at(i) != QChar('|')){
            chan.append(channelsStr.at(i));
            ++i;
        }
        ++i;
        // The following may throw if from is not a channel name which begins with "Channel_"
        channels += getChannelByName(chan.toStdString());
    }
    Format dispW(frmtXStr.toInt(),frmtYStr.toInt(),frmtRStr.toInt(),frmtTStr.toInt(),"");
    out->set_dataWindow(Box2D(bboxXStr.toInt(),bboxYStr.toInt(),bboxRStr.toInt(),bboxTStr.toInt()));
    out->set_channels(channels);
    out->set_rgbMode((bool)rgbStr.toInt());
    out->set_displayWindow(dispW);
    out->set_firstFrame(firstFrameStr.toInt());
    out->set_lastFrame(lastFrameStr.toInt());
    out->setCurrentFrameName(currentFrameName.toStdString());
    return out;
    
}
bool Reader::hasFrames() const{
    return _fileKnob->frameCount() > 0;
}
