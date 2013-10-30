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
#include "Engine/Settings.h"
#include "Engine/Box.h"
#include "Engine/Format.h"
#include "Engine/FrameEntry.h"
#include "Engine/ViewerNode.h"
#include "Engine/Knob.h"
#include "Engine/ImageInfo.h"
#include "Engine/Project.h"

#include "Readers/ExrDecoder.h"
#include "Readers/QtDecoder.h"
#include "Readers/Decoder.h"

#include "Writers/Writer.h"

using namespace Powiter;
using std::cout; using std::endl;

Reader::Reader(AppInstance* app)
: Node(app)
, _buffer()
, _fileKnob(0)
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
    _fileKnob = dynamic_cast<File_Knob*>(appPTR->getKnobFactory().createKnob("InputFile", this, desc));
    QObject::connect(_fileKnob, SIGNAL(frameRangeChanged(int,int)), this, SLOT(onFrameRangeChanged(int,int)));
    assert(_fileKnob);
}

void Reader::onFrameRangeChanged(int first,int last){
    _frameRange.first = first;
    _frameRange.second = last;
    notifyFrameRangeChanged(first,last);
}
boost::shared_ptr<Decoder> Reader::decoderForFileType(const QString& fileName){
    QString extension;
    for (int i = fileName.size() - 1; i >= 0; --i) {
        QChar c = fileName.at(i);
        if(c != QChar('.'))
            extension.prepend(c);
        else
            break;
    }

    Powiter::LibraryBinary* decoder = Settings::getPowiterCurrentSettings()->_readersSettings.decoderForFiletype(extension.toStdString());
    if (!decoder) {
        std::string err("Couldn't find an appropriate decoder for this filetype");
        err.append(extension.toStdString());
        throw std::invalid_argument(err);
    }
    
    std::pair<bool,ReadBuilder> func = decoder->findFunction<ReadBuilder>("BuildRead");
    if (func.first) {
       return  boost::shared_ptr<Decoder>(func.second(this));
    } else {
        std::string err("Failed to create the decoder for");
        err.append(getName());
        err.append(",something is wrong in the plugin.");
        throw std::invalid_argument(err);
    }
    return boost::shared_ptr<Decoder>();
}

Powiter::Status Reader::getRegionOfDefinition(SequenceTime time,Box2D* rod){
    QString filename = _fileKnob->getRandomFrameName(time);
    
    /*Locking any other thread: we want only 1 thread to create the descriptor*/
    QMutexLocker lock(&_lock);
    boost::shared_ptr<Decoder> found = _buffer.get(filename.toStdString());
    if (found) {
        *rod = found->readerInfo().getRoD();
    }else{
        boost::shared_ptr<Decoder> desc;
        try{
            desc = decodeHeader(filename);
        }catch(const std::invalid_argument& e){
            cout << e.what() << endl;
            return StatFailed;
        }
        if(desc){
            *rod =  desc->readerInfo().getRoD();
        }else{
            return StatFailed;
        }
    }
    return StatOK;
}

boost::shared_ptr<Decoder> Reader::decodeHeader(const QString& filename){
    /*the read handle used to decode the frame*/
    boost::shared_ptr<Decoder> decoderReadHandle;
    try{
        decoderReadHandle = decoderForFileType(filename);
    }catch(const std::invalid_argument& e){
        throw e;
    }
    assert(decoderReadHandle);
    Status st = decoderReadHandle->_readHeader(filename);
    if(st == StatFailed){
        return boost::shared_ptr<Decoder>();
    }
    getApp()->getProject()->lock();
    if(getApp()->shouldAutoSetProjectFormat()){
        getApp()->setAutoSetProjectFormat(false);
        getApp()->setProjectFormat(decoderReadHandle->readerInfo().getDisplayWindow());
    }else{
        getApp()->tryAddProjectFormat(decoderReadHandle->readerInfo().getDisplayWindow());
    }
    getApp()->getProject()->unlock();
    decoderReadHandle->initializeColorSpace();
    _buffer.insert(filename.toStdString(),decoderReadHandle);
    return decoderReadHandle;
}



void Reader::render(SequenceTime time,RenderScale scale,const Box2D& roi,int /*view*/,boost::shared_ptr<Powiter::Image> output){
    QString filename = _fileKnob->getRandomFrameName(time);
    boost::shared_ptr<Decoder> found;
    {
        /*This section is critical: if we don't find it in the buffer we want only 1 thread to re-create the descriptor.
         */
        QMutexLocker lock(&_lock);
        found = _buffer.get(filename.toStdString());
        if(!found){
#ifdef POWITER_DEBUG
            cout << "WARNING: Buffer does not contains the header for frame " << filename.toStdString()
            <<". re-decoding header...(" << getName() << "). It might indicate that the buffer of the reader is"
            " not big enough. You might want to change the maximum buffer size."<< endl;
#endif
            try{
                found = decodeHeader(filename);
            }catch(const std::invalid_argument& e){
                cout << e.what() << endl;
            }
        }
    }
    found->render(time,scale,roi,output);
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


bool Reader::hasFrames() const{
    return _fileKnob->frameCount() > 0;
}

