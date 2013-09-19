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


Reader::Reader(Model* model):Node(model),
preview(0),
has_preview(false),
video_sequence(0),
readHandle(0),
_fileKnob(0),
_readMutex(new QMutex)
{

   

}

Reader::~Reader(){
    _buffer.clear();
	delete preview;
    delete _readMutex;
}

std::string Reader::className() {
    return "Reader";
}

std::string Reader::description() {
    return "InputNode";
}

void Reader::initKnobs(){
    std::string desc("File");
    _fileKnob = dynamic_cast<File_Knob*>(appPTR->getKnobFactory()->createKnob("InputFile", this, desc,1, Knob::NONE));
    QObject::connect(_fileKnob, SIGNAL(valueChangedByUser()), this, SLOT(showFilePreview()));
    assert(_fileKnob);
}




bool Reader::readCurrentHeader(int current_frame){
    current_frame = clampToRange(current_frame);
    QString filename;
    map<int,QString>::iterator it = files.find(current_frame);
    if(it == files.end()){
        cout << "ERROR: Failed to find a frame for frame number " << current_frame << endl;
        return false;
    }else{
        filename = it->second;
    }
    
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
        readHandle = (*found)->_readHandle;
    }else{
        _read->initializeColorSpace();
        if(_read->supportsScanLine()){
            _buffer.insert(new Reader::Buffer::ScanLineDescriptor(_read,filenameStr,slContext));
        }else{
            _read->readHeader(filename, false);
            _buffer.insert(new Reader::Buffer::FullFrameDescriptor(_read,filenameStr));
        }
        _info = _read->readerInfo();
        readHandle = _read;
    }
    return true;
}

void Reader::readCurrentData(int current_frame){
    current_frame = clampToRange(current_frame);
    QString filename;
    map<int,QString>::iterator it = files.find(current_frame);
    if(it == files.end()){
        cout << "ERROR: Failed to find a frame for frame number " << current_frame << endl;
        return;
    }else{
        filename = it->second;
    }
    
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
    
    _buffer.clear();
    fileNameList = _fileKnob->getValueAsVariant().toStringList();
    getVideoSequenceFromFilesList();
    
    fitFrameToViewer(false);
    
    _readMutex->lock();
    if(readCurrentHeader(firstFrame())){ 
        readCurrentData(firstFrame());
        assert(readHandle);
        readHandle->make_preview();
    }
    _readMutex->unlock();
    _buffer.clear();
}




bool Reader::_validate(bool){
   
    _info.set_firstFrame(firstFrame());
    _info.set_lastFrame(lastFrame());
    return true;
}

void Reader::engine(int y,int offset,int range,ChannelSet c,Row* out){
	assert(readHandle);
    readHandle->engine(y,offset,range,c,out);
	
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

void Reader::getVideoSequenceFromFilesList(){
    files.clear();
	if(fileNameList.size() > 1 ){
		video_sequence=true;
	}else{
        video_sequence=false;
    }
    bool first_time=true;
    QString originalName;
    foreach(QString Qfilename,fileNameList)
    {	if(Qfilename.at(0) == QChar('.')) continue;
        QString const_qfilename=Qfilename;
        if(first_time){
            Qfilename=Qfilename.remove(Qfilename.length()-4,4);
            int j=Qfilename.length()-1;
            QString frameIndex;
            while(j>0 && Qfilename.at(j).isDigit()){
                frameIndex.push_front(Qfilename.at(j));
                j--;
            }
            if(j>0){
				int number=0;
                if(fileNameList.size() > 1){
                    number = frameIndex.toInt();
                }
				files.insert(make_pair(number,const_qfilename));
                originalName=Qfilename.remove(j+1,frameIndex.length());
                
            }else{
                files[0]=const_qfilename;
            }
            first_time=false;
        }else{
            if(Qfilename.contains(originalName) /*&& (extension)*/){
                Qfilename.remove(Qfilename.length()-4,4);
                int j=Qfilename.length()-1;
                QString frameIndex;
                while(j>0 && Qfilename.at(j).isDigit()){
                    frameIndex.push_front(Qfilename.at(j));
                    j--;
                }
                if(j>0){
                    int number = frameIndex.toInt();
                    files[number]=const_qfilename;
                }else{
                    cout << " Read handle : WARNING !! several frames read but no frame count found in their name " << endl;
                }
            }
        }
    }
    _info.set_firstFrame(firstFrame());
    _info.set_lastFrame(lastFrame());
    
}
int Reader::firstFrame(){
    std::map<int,QString>::iterator it = files.begin();
    if (it == files.end()) {
        return INT_MIN;
    }
    return it->first;
}
int Reader::lastFrame(){
    std::map<int,QString>::iterator it = files.end();
    if(it == files.begin()) {
        return INT_MAX;
    }
    --it;
    return it->first;
}
int Reader::clampToRange(int f){
    int first = firstFrame();
    int last = lastFrame();
    if(f < first) return first;
    if(f > last) return last;
    return f;
}

std::string Reader::getRandomFrameName(int f){
    return files[f].toStdString();
}

void Reader::setPreview(QImage* img){
    if(preview)
        delete preview;
    preview=img;
    hasPreview(true);
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
std::string ReaderInfo::printOut(){
    const ChannelSet& chan = channels();
    ostringstream oss;
    oss << _currentFrameName <<  "<" << firstFrame() << "."
    << lastFrame() << "."
    << rgbMode() << "."
    << displayWindow().left() << "."
    << displayWindow().bottom() << "."
    << displayWindow().right() << "."
    << displayWindow().top() << "."
    << dataWindow().left() << "."
    << dataWindow().bottom() << "."
    << dataWindow().right() << "."
    << dataWindow().top() << ".";
    foreachChannels(z, chan){
        oss << getChannelName(z) << "|";
    }
    oss << " " ;
    return oss.str();
}

ReaderInfo* ReaderInfo::fromString(const QString& from){
    ReaderInfo* out = new ReaderInfo;
    QString name;
    QString firstFrameStr,lastFrameStr,rgbStr,frmtXStr,frmtYStr,frmtRStr,frmtTStr;
    QString bboxXStr,bboxYStr,bboxRStr,bboxTStr,channelsStr;
    
    int i = 0;
    while(from.at(i) != QChar('<')){name.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){firstFrameStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){lastFrameStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){rgbStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){frmtXStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){frmtYStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){frmtRStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){frmtTStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){bboxXStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){bboxYStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){bboxRStr.append(from.at(i)); ++i;}
    ++i;
    while(from.at(i) != QChar('.')){bboxTStr.append(from.at(i)); ++i;}
    ++i;
    while(i < from.size()){channelsStr.append(from.at(i)); ++i;}
    ++i;
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
    out->setCurrentFrameName(name.toStdString());
    return out;
    
}
