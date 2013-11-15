//  Natron
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
#include "Global/QtCompat.h" // for removeFileExtension

#include "Engine/Node.h"
#include "Engine/MemoryFile.h"
#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/RectI.h"
#include "Engine/Format.h"
#include "Engine/FrameEntry.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Knob.h"
#include "Engine/ImageInfo.h"
#include "Engine/Project.h"

#include "Readers/ExrDecoder.h"
#include "Readers/QtDecoder.h"
#include "Readers/Decoder.h"

#include "Writers/Writer.h"

using namespace Natron;
using std::cout; using std::endl;

Reader::Reader(Node* node)
: Natron::EffectInstance(node)
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

bool Reader::isInputOptional(int /*inputNb*/) const{
    return false;
}
Natron::Status Reader::preProcessFrame(SequenceTime time){
    int missingFrameChoice = _missingFrameChoice->value<int>();
    QString filename = _fileKnob->getRandomFrameName(time,missingFrameChoice == 0);
    if(filename.isEmpty() && (missingFrameChoice == 1 || missingFrameChoice == 0)){
        setPersistentMessage(Natron::ERROR_MESSAGE,QString(" couldn't find a file for frame "+QString::number(time)).toStdString());
        return StatFailed;
    }else{
        return StatOK;
    }
}

void Reader::initializeKnobs(){
    _fileKnob = dynamic_cast<File_Knob*>(appPTR->getKnobFactory().createKnob("InputFile", this, "File"));
    assert(_fileKnob);

    _missingFrameChoice = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", this, "On missing frame"));
    std::vector<std::string> missingFrameChoices;
    missingFrameChoices.push_back("Load nearest");
    missingFrameChoices.push_back("Error");
    missingFrameChoices.push_back("Black image");
    _missingFrameChoice->populate(missingFrameChoices);
    _missingFrameChoice->setValue(0);
}

void Reader::onKnobValueChanged(Knob* /*k*/,Knob::ValueChangedReason /*reason*/){}

void Reader::getFrameRange(SequenceTime *first,SequenceTime *last){
    *first = _fileKnob->firstFrame();
    *last = _fileKnob->lastFrame();
}

boost::shared_ptr<Decoder> Reader::decoderForFileType(const QString& fileName){
    QString fileNameCopy = fileName;
    QString extension = Natron::removeFileExtension(fileNameCopy);
    Natron::LibraryBinary* decoder = appPTR->getCurrentSettings()._readersSettings.decoderForFiletype(extension.toStdString());
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

Natron::Status Reader::getRegionOfDefinition(SequenceTime time,RectI* rod){
    QString filename = _fileKnob->getRandomFrameName(time,true);
    
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
    getNode()->getApp()->getProject()->lock();
    if(getNode()->getApp()->shouldAutoSetProjectFormat()){
        getNode()->getApp()->setAutoSetProjectFormat(false);
        getNode()->getApp()->setProjectFormat(decoderReadHandle->readerInfo().getDisplayWindow());
    }else{
        getNode()->getApp()->tryAddProjectFormat(decoderReadHandle->readerInfo().getDisplayWindow());
    }
    getNode()->getApp()->getProject()->unlock();
    decoderReadHandle->initializeColorSpace();
    _buffer.insert(filename.toStdString(),decoderReadHandle);
    return decoderReadHandle;
}



Natron::Status Reader::render(SequenceTime time,RenderScale scale,const RectI& roi,int /*view*/,boost::shared_ptr<Natron::Image> output){
    int missingFrameChoice = _missingFrameChoice->value<int>();
    QString filename = _fileKnob->getRandomFrameName(time,missingFrameChoice == 0);
    if(filename.isEmpty() && missingFrameChoice == 2){
        output->fill(roi,0.,0.); // render black
        return StatOK;
    }else if(filename.isEmpty()){
        std::cout << "Error should have been caught earlier in preProcessFrame." << std::endl;
        return StatFailed;
    }
    boost::shared_ptr<Decoder> found;
    {
        /*This section is critical: if we don't find it in the buffer we want only 1 thread to re-create the descriptor.
         */
        QMutexLocker lock(&_lock);
        found = _buffer.get(filename.toStdString());
        if(!found){
#ifdef NATRON_DEBUG
            cout << "WARNING: Buffer does not contains the header for frame " << filename.toStdString()
            <<". re-decoding header...(" << getName() << "). It might indicate that the buffer of the reader is"
            " not big enough. You might want to change the maximum buffer size."<< endl;
#endif
            try{
                found = decodeHeader(filename);
            }catch(const std::invalid_argument& e){
                setPersistentMessage(Natron::ERROR_MESSAGE, e.what());
                return StatFailed;
            }
        }
    }
    return found->render(time,scale,roi,output);
}

