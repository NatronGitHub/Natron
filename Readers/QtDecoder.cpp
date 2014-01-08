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

#include "QtDecoder.h"

#include <stdexcept>

#include <QtGui/QImage>
#include <QtGui/QColor>
#include <QtGui/QImageReader>

#include "Global/AppManager.h"

#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Knob.h"
#include "Engine/Project.h"

using namespace Natron;
using std::cout; using std::endl;

QtReader::QtReader(Natron::Node* node)
: Natron::EffectInstance(node)
, _lut(Color::LutManager::sRGBLut())
, _img(0)
, _fileKnob()
, _firstFrame()
, _before()
, _lastFrame()
, _after()
, _missingFrameChoice()
, _startingFrame()
{
}


QtReader::~QtReader(){
    if(_img)
        delete _img;
}

std::string QtReader::pluginID() const {
    return "ReadQt";
}

std::string QtReader::pluginLabel() const {
    return "ReadQt";
}

std::string QtReader::description() const {
    return "A QImage (Qt) based image reader.";
}

void QtReader::initializeKnobs() {
    _fileKnob = Natron::createKnob<File_Knob>(this, "File");
    _fileKnob->setAsInputImage();
    
    _firstFrame =  Natron::createKnob<Int_Knob>(this, "First frame");
    _firstFrame->turnOffAnimation();
    _firstFrame->setValue<int>(0);
    
    
    _before = Natron::createKnob<Choice_Knob>(this, "Before");
    std::vector<std::string> beforeOptions;
    beforeOptions.push_back("hold");
    beforeOptions.push_back("loop");
    beforeOptions.push_back("bounce");
    beforeOptions.push_back("black");
    beforeOptions.push_back("error");
    _before->populate(beforeOptions);
    _before->turnOffAnimation();
    _before->setValue<int>(0);
    
    _lastFrame =  Natron::createKnob<Int_Knob>(this, "Last frame");
    _lastFrame->turnOffAnimation();
    _lastFrame->setValue<int>(0);
    
    
    _after = Natron::createKnob<Choice_Knob>(this, "After");
    std::vector<std::string> afterOptions;
    afterOptions.push_back("hold");
    afterOptions.push_back("loop");
    afterOptions.push_back("bounce");
    afterOptions.push_back("black");
    afterOptions.push_back("error");
    _after->populate(beforeOptions);
    _after->turnOffAnimation();
    _after->setValue<int>(0);
    
    _missingFrameChoice = Natron::createKnob<Choice_Knob>(this, "On missing frame");
    std::vector<std::string> missingFrameOptions;
    missingFrameOptions.push_back("Load nearest");
    missingFrameOptions.push_back("Error");
    missingFrameOptions.push_back("Black image");
    _missingFrameChoice->populate(missingFrameOptions);
    _missingFrameChoice->setValue<int>(0);
    _missingFrameChoice->turnOffAnimation();
    
    _startingFrame = Natron::createKnob<Int_Knob>(this, "Starting frame");
    _startingFrame->turnOffAnimation();
    _startingFrame->setValue<int>(0);
    
}

void QtReader::getFrameRange(SequenceTime *first,SequenceTime *last){
    int startingTime = _startingFrame->getValue<int>();
    *first =  startingTime;
    *last = _fileKnob->lastFrame() + startingTime;
}


void QtReader::supportedFileFormats(std::vector<std::string>* formats) {
    const QList<QByteArray>& supported = QImageReader::supportedImageFormats();
    for (int i = 0; i < supported.size(); ++i) {
        formats->push_back(supported.at(i).data());
    }
};


SequenceTime QtReader::getSequenceTime(SequenceTime t)
{
    int startingTime =  _startingFrame->getValue<int>();
    
    ///offset the time wrt the starting time
    SequenceTime sequenceTime =  t;
    
    
    SequenceTime first,last;
    getFrameRange(&first, &last);
    
    ///get the offset from the starting time of the sequence in case we bounce or loop
    int timeOffsetFromStart = t -  first;
    
    if( t < first) {
        /////if we're before the first frame
        int beforeChoice = _before->getValue<int>();
        switch (beforeChoice) {
            case 0: //hold
                sequenceTime = first;
                break;
            case 1: //loop
                    //call this function recursively with the appropriate offset in the time range
                timeOffsetFromStart %= (int)(last - first + 1);
                sequenceTime = last - std::abs(timeOffsetFromStart);
                break;
            case 2: //bounce
                    //call this function recursively with the appropriate offset in the time range
            {
                int sequenceIntervalsCount = timeOffsetFromStart / (last - first);
                ///if the sequenceIntervalsCount is odd then do exactly like loop, otherwise do the load the opposite frame
                if (sequenceIntervalsCount % 2 == 0) {
                    timeOffsetFromStart %= (int)(last - first + 1);
                    sequenceTime = first + std::abs(timeOffsetFromStart);
                } else {
                    timeOffsetFromStart %= (int)(last - first + 1);
                    sequenceTime = last - std::abs(timeOffsetFromStart);
                }
            }
                break;
            case 3: //black
                throw std::invalid_argument("Out of frame range.");
                break;
            case 4: //error
                setPersistentMessage(Natron::ERROR_MESSAGE,  "Missing frame");
                throw std::invalid_argument("Out of frame range.");
                break;
            default:
                break;
        }
        
    } else if( t > last) {
        /////if we're after the last frame
        int afterChoice = _after->getValue<int>();
        
        switch (afterChoice) {
            case 0: //hold
                sequenceTime = last;
                break;
            case 1: //loop
                    //call this function recursively with the appropriate offset in the time range
                timeOffsetFromStart %= (int)(last - first + 1);
                sequenceTime = first + std::abs(timeOffsetFromStart);
                break;
            case 2: //bounce
                    //call this function recursively with the appropriate offset in the time range
            {
                int sequenceIntervalsCount = timeOffsetFromStart / (last - first);
                ///if the sequenceIntervalsCount is odd then do exactly like loop, otherwise do the load the opposite frame
                if (sequenceIntervalsCount % 2 == 0) {
                    timeOffsetFromStart %= (int)(last - first + 1);
                    sequenceTime = first + std::abs(timeOffsetFromStart);
                } else {
                    timeOffsetFromStart %= (int)(last - first+ 1);
                    sequenceTime = last - std::abs(timeOffsetFromStart);
                }
            }
                
                break;
            case 3: //black
                throw std::invalid_argument("Out of frame range.");
                break;
            case 4: //error
                setPersistentMessage(Natron::ERROR_MESSAGE, "Missing frame");
                throw std::invalid_argument("Out of frame range.");
                break;
            default:
                break;
        }
        
    }
    sequenceTime -= startingTime;
    return sequenceTime;
}

void QtReader::getFilenameAtSequenceTime(SequenceTime time, std::string &filename)
{
   
    
    int missingChoice = _missingFrameChoice->getValue<int>();
    QString file = _fileKnob->getRandomFrameName(time, false);
    
    switch (missingChoice) {
        case 0: // Load nearest
                ///the nearest frame search went out of range and couldn't find a frame.
            if(file.isEmpty()){
                file = _fileKnob->getRandomFrameName(time, true);
                if (file.isEmpty()) {
                    setPersistentMessage(Natron::ERROR_MESSAGE, "Nearest frame search went out of range");
                } else {
                    filename = file.toStdString();
                }
            } else {
                filename = file.toStdString();
            }
            break;
        case 1: // Error
                /// For images sequences, if the offset is not 0, that means no frame were found at the  originally given
                /// time, we can safely say this is  a missing frame.
            if (file.isEmpty()) {
                filename.clear();
                setPersistentMessage(Natron::ERROR_MESSAGE, "Missing frame");
            } else {
                filename = file.toStdString();
            }
        case 2: // Black image
                /// For images sequences, if the offset is not 0, that means no frame were found at the  originally given
                /// time, we can safely say this is  a missing frame.
            if (file.isEmpty()) {
                filename.clear();
            } else {
                filename = file.toStdString();
            }
            break;
    }
    
    
    
}

Natron::Status QtReader::getRegionOfDefinition(SequenceTime time,RectI* rod){

    QMutexLocker l(&_lock);
    double sequenceTime;
    try {
        sequenceTime =  getSequenceTime(time);
    } catch (const std::exception& e) {
        return StatFailed;
    }
    std::string filename;
    
    getFilenameAtSequenceTime(sequenceTime, filename);
    
    if (filename.empty()) {
        return StatFailed;
    }
    
    if(filename != _filename){
        _filename = filename;
        if(_img){
            delete _img;
        }
        _img = new QImage(_filename.c_str());
        if(_img->format() == QImage::Format_Invalid){
            setPersistentMessage(Natron::ERROR_MESSAGE, "Failed to load the image " + filename);
            return StatFailed;
        }
    }
    
    rod->x1 = 0;
    rod->x2 = _img->width();
    rod->y1 = 0;
    rod->y2 = _img->height();
    
    Format dispW;
    dispW.set(*rod);
    getApp()->setOrAddProjectFormat(dispW); 

    return StatOK;
}

Natron::Status QtReader::render(SequenceTime /*time*/,RenderScale /*scale*/,
                                const RectI& roi,int /*view*/,boost::shared_ptr<Natron::Image> output) {
    int missingFrameCHoice = _missingFrameChoice->getValue<int>();
    if(!_img && missingFrameCHoice == 1){
        return StatFailed;
    }else if(!_img && missingFrameCHoice == 2){
        return StatOK;
    }
    
    switch (_img->format()) {
        case QImage::Format_RGB32: // The image is stored using a 32-bit RGB format (0xffRRGGBB).
        case QImage::Format_ARGB32: // The image is stored using a 32-bit ARGB format (0xAARRGGBB).
        case QImage::Format_ARGB32_Premultiplied: // The image is stored using a premultiplied 32-bit ARGB format (0xAARRGGBB).
            //might have to invert y coordinates here
            _lut->from_byte_packed(output->pixelAt(0, 0),_img->bits(), roi, output->getRoD(),output->getRoD(),
                                   Natron::Color::PACKING_BGRA,Natron::Color::PACKING_RGBA,true,true);
            break;
        case QImage::Format_Mono: // The image is stored using 1-bit per pixel. Bytes are packed with the most significant bit (MSB) first.
        case QImage::Format_MonoLSB: // The image is stored using 1-bit per pixel. Bytes are packed with the less significant bit (LSB) first.
        case QImage::Format_Indexed8: // The image is stored using 8-bit indexes into a colormap.
        case QImage::Format_RGB16: // The image is stored using a 16-bit RGB format (5-6-5).
        case QImage::Format_ARGB8565_Premultiplied: // The image is stored using a premultiplied 24-bit ARGB format (8-5-6-5).
        case QImage::Format_RGB666: // The image is stored using a 24-bit RGB format (6-6-6). The unused most significant bits is always zero.
        case QImage::Format_ARGB6666_Premultiplied: // The image is stored using a premultiplied 24-bit ARGB format (6-6-6-6).
        case QImage::Format_RGB555: // The image is stored using a 16-bit RGB format (5-5-5). The unused most significant bit is always zero.
        case QImage::Format_ARGB8555_Premultiplied: // The image is stored using a premultiplied 24-bit ARGB format (8-5-5-5).
        case QImage::Format_RGB888: // The image is stored using a 24-bit RGB format (8-8-8).
        case QImage::Format_RGB444: // The image is stored using a 16-bit RGB format (4-4-4). The unused bits are always zero.
        case QImage::Format_ARGB4444_Premultiplied: // The image is stored using a premultiplied 16-bit ARGB format (4-4-4-4).
            _lut->from_byte_packed(output->pixelAt(0, 0),_img->bits(), roi, output->getRoD(),output->getRoD(),
                                   Natron::Color::PACKING_BGRA,Natron::Color::PACKING_RGBA,true,true);
            break;
        case QImage::Format_Invalid:
        default:
            output->fill(roi,0.f,1.f);
            setPersistentMessage(Natron::ERROR_MESSAGE, "Invalid image format.");
            return StatFailed;
    }
    output->setPixelAspect(_img->dotsPerMeterX() / _img->dotsPerMeterY());
    return StatOK;
}

Natron::EffectInstance::CachePolicy QtReader::getCachePolicy(SequenceTime time) const{
    //if we're in nearest mode and the frame could not be found do not cache it, otherwise
    //we would cache multiple copies of the same frame
    if(_missingFrameChoice->getValue<int>() == 0){
        QString filename = _fileKnob->getRandomFrameName(time,false);
        if(filename.isEmpty()){
            return NEVER_CACHE;
        }
    }
    return ALWAYS_CACHE;
}

