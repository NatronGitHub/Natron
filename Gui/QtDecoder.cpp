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

#include "Engine/AppManager.h"
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
, _frameMode()
, _startingFrame()
, _timeOffset()
, _settingFrameRange(false)
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
    
    if (isLiveInstance()) {
        Natron::warningDialog(getName(), "This plugin exists only to help the developpers team to test " NATRON_APPLICATION_NAME
                              ". You cannot use it when rendering a project.");
    }
    
    _fileKnob = Natron::createKnob<File_Knob>(this, "File");
    _fileKnob->setAsInputImage();
    
    _firstFrame =  Natron::createKnob<Int_Knob>(this, "First frame");
    _firstFrame->setAnimationEnabled(false);
    _firstFrame->setDefaultValue<int>(0);
    
    
    _before = Natron::createKnob<Choice_Knob>(this, "Before");
    std::vector<std::string> beforeOptions;
    beforeOptions.push_back("hold");
    beforeOptions.push_back("loop");
    beforeOptions.push_back("bounce");
    beforeOptions.push_back("black");
    beforeOptions.push_back("error");
    _before->populate(beforeOptions);
    _before->setAnimationEnabled(false);
    _before->setDefaultValue<int>(0);
    
    _lastFrame =  Natron::createKnob<Int_Knob>(this, "Last frame");
    _lastFrame->setAnimationEnabled(false);
    _lastFrame->setDefaultValue<int>(0);
    
    
    _after = Natron::createKnob<Choice_Knob>(this, "After");
    std::vector<std::string> afterOptions;
    afterOptions.push_back("hold");
    afterOptions.push_back("loop");
    afterOptions.push_back("bounce");
    afterOptions.push_back("black");
    afterOptions.push_back("error");
    _after->populate(beforeOptions);
    _after->setAnimationEnabled(false);
    _after->setDefaultValue<int>(0);
    
    _missingFrameChoice = Natron::createKnob<Choice_Knob>(this, "On missing frame");
    std::vector<std::string> missingFrameOptions;
    missingFrameOptions.push_back("Load nearest");
    missingFrameOptions.push_back("Error");
    missingFrameOptions.push_back("Black image");
    _missingFrameChoice->populate(missingFrameOptions);
    _missingFrameChoice->setDefaultValue<int>(0);
    _missingFrameChoice->setAnimationEnabled(false);
    
    _frameMode = Natron::createKnob<Choice_Knob>(this, "Frame mode");
    _frameMode->setAnimationEnabled(false);
    std::vector<std::string> frameModeOptions;
    frameModeOptions.push_back("Starting frame");
    frameModeOptions.push_back("Time offset");
    _frameMode->populate(frameModeOptions);
    _frameMode->setDefaultValue<int>(0);
    
    _startingFrame = Natron::createKnob<Int_Knob>(this, "Starting frame");
    _startingFrame->setAnimationEnabled(false);
    _startingFrame->setDefaultValue<int>(0);
    
    _timeOffset = Natron::createKnob<Int_Knob>(this, "Time offset");
    _timeOffset->setAnimationEnabled(false);
    _timeOffset->setDefaultValue<int>(0);
    _timeOffset->setSecret(true);
    
}

void QtReader::onKnobValueChanged(Knob* k, Natron::ValueChangedReason /*reason*/) {
    if (k == _fileKnob.get()) {
        SequenceTime first,last;
        getSequenceTimeDomain(first,last);
        timeDomainFromSequenceTimeDomain(first,last, true);
        _startingFrame->setValue<int>(first);
    } else if(k == _firstFrame.get() && !_settingFrameRange) {
        
        int first = _firstFrame->getValue<int>();
        _lastFrame->setMinimum(first);
        
        int offset = _timeOffset->getValue<int>();
        _settingFrameRange = true;
        _startingFrame->setValue<int>(first +offset);
        _settingFrameRange = false;
    } else if(k == _lastFrame.get()) {
        int last = _lastFrame->getValue<int>();
        _firstFrame->setMaximum(last);

    } else if(k == _frameMode.get()) {
        int mode = _frameMode->getValue<int>();
        switch (mode) {
            case 0: //starting frame
                _startingFrame->setSecret(false);
                _timeOffset->setSecret(true);
                break;
            case 1: //time offset
                _startingFrame->setSecret(true);
                _timeOffset->setSecret(false);
                break;
            default:
                //no such case
                assert(false);
                break;
        }

    } else if( k == _startingFrame.get() && !_settingFrameRange) {
        //also update the time offset
        int startingFrame = _startingFrame->getValue<int>();
        int firstFrame = _firstFrame->getValue<int>();
        
        
        ///prevent recursive calls of setValue(...)
        _settingFrameRange = true;
        _timeOffset->setValue(startingFrame - firstFrame);
        _settingFrameRange = false;
        
    } else if( k == _timeOffset.get() && !_settingFrameRange) {
        //also update the starting frame
        int offset = _timeOffset->getValue<int>();
        int first = _firstFrame->getValue<int>();

        
        ///prevent recursive calls of setValue(...)
        _settingFrameRange = true;
        _startingFrame->setValue(offset + first);
        _settingFrameRange = false;
        
    }

}

void QtReader::getSequenceTimeDomain(SequenceTime& first,SequenceTime& last) {
    first = _fileKnob->firstFrame();
    last = _fileKnob->lastFrame();
}

void QtReader::timeDomainFromSequenceTimeDomain(SequenceTime& first,SequenceTime& last,bool mustSetFrameRange) {
    ///the values held by GUI parameters
    int frameRangeFirst,frameRangeLast;
    int startingFrame;
    if (mustSetFrameRange) {
        frameRangeFirst = first;
        frameRangeLast = last;
        startingFrame = first;
        _settingFrameRange = true;
        _firstFrame->setMinimum(first);
        _firstFrame->setMaximum(last);
        _lastFrame->setMinimum(first);
        _lastFrame->setMaximum(last);
        
        _firstFrame->setValue(first);
        _lastFrame->setValue(last);
        _settingFrameRange = false;
    } else {
        ///these are the value held by the "First frame" and "Last frame" param
        frameRangeFirst = _firstFrame->getValue<int>();
        frameRangeLast = _lastFrame->getValue<int>();
        startingFrame = _startingFrame->getValue<int>();
    }
    
    first = startingFrame;
    last =  startingFrame + frameRangeLast - frameRangeFirst;
}


void QtReader::getFrameRange(SequenceTime *first,SequenceTime *last){
    getSequenceTimeDomain(*first, *last);
    timeDomainFromSequenceTimeDomain(*first, *last, false);
}


void QtReader::supportedFileFormats_static(std::vector<std::string>* formats) {
    const QList<QByteArray>& supported = QImageReader::supportedImageFormats();
    for (int i = 0; i < supported.size(); ++i) {
        formats->push_back(supported.at(i).data());
    }
};

std::vector<std::string> QtReader::supportedFileFormats() const  {
    std::vector<std::string> ret;
    QtReader::supportedFileFormats_static(&ret);
    return ret;
}

SequenceTime QtReader::getSequenceTime(SequenceTime t)
{
    
    int timeOffset = _timeOffset->getValue<int>();

    
    
    SequenceTime first = _firstFrame->getValue<int>();
    SequenceTime last = _lastFrame->getValue<int>();
        
    
    ///offset the time wrt the starting time
    SequenceTime sequenceTime =  t - timeOffset;
    
    ///get the offset from the starting time of the sequence in case we bounce or loop
    int timeOffsetFromStart = sequenceTime -  first;
    
    if( sequenceTime < first) {
        /////if we're before the first frame
        int beforeChoice = _before->getValue<int>();
        switch (beforeChoice) {
            case 0: //hold
                sequenceTime = first;
                break;
            case 1: //loop
                    //call this function recursively with the appropriate offset in the time range
                timeOffsetFromStart %= (int)(last - first + 1);
                sequenceTime = last + timeOffsetFromStart;
                break;
            case 2: //bounce
                    //call this function recursively with the appropriate offset in the time range
            {
                int sequenceIntervalsCount = (last == first) ? 0 : (timeOffsetFromStart / (last - first));
                ///if the sequenceIntervalsCount is odd then do exactly like loop, otherwise do the load the opposite frame
                if (sequenceIntervalsCount % 2 == 0) {
                    timeOffsetFromStart %= (int)(last - first + 1);
                    sequenceTime = first - timeOffsetFromStart;
                } else {
                    timeOffsetFromStart %= (int)(last - first + 1);
                    sequenceTime = last + timeOffsetFromStart;
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
        
    } else if( sequenceTime > last) {
        /////if we're after the last frame
        int afterChoice = _after->getValue<int>();
        
        switch (afterChoice) {
            case 0: //hold
                sequenceTime = last;
                break;
            case 1: //loop
                    //call this function recursively with the appropriate offset in the time range
                timeOffsetFromStart %= (int)(last - first + 1);
                sequenceTime = first + timeOffsetFromStart;
                break;
            case 2: //bounce
                    //call this function recursively with the appropriate offset in the time range
            {
                int sequenceIntervalsCount = (last == first) ? 0 : (timeOffsetFromStart / (last - first));
                ///if the sequenceIntervalsCount is odd then do exactly like loop, otherwise do the load the opposite frame
                if (sequenceIntervalsCount % 2 == 0) {
                    timeOffsetFromStart %= (int)(last - first + 1);
                    sequenceTime = first + timeOffsetFromStart;
                } else {
                    timeOffsetFromStart %= (int)(last - first+ 1);
                    sequenceTime = last - timeOffsetFromStart;
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
    assert(sequenceTime >= first && sequenceTime <= last);

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

Natron::Status QtReader::getRegionOfDefinition(SequenceTime time,RectI* rod,bool* isProjectFormat){

    *isProjectFormat = false;
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

    return StatOK;
}

Natron::Status QtReader::render(SequenceTime /*time*/,RenderScale /*scale*/,
                                const RectI& roi,int /*view*/,
                                bool /*isSequentialRender*/,bool /*isRenderResponseToUserInteraction*/,
                                boost::shared_ptr<Natron::Image> output) {
    int missingFrameChoice = _missingFrameChoice->getValue<int>();
    if (!_img) {
        if (!_img && missingFrameChoice == 2) { // black image
            return StatOK;
        }
        assert(missingFrameChoice != 0); // nearest value - should never happen
        return StatFailed; // error
    }
    
    assert(_img);
    switch (_img->format()) {
        case QImage::Format_RGB32: // The image is stored using a 32-bit RGB format (0xffRRGGBB).
        case QImage::Format_ARGB32: // The image is stored using a 32-bit ARGB format (0xAARRGGBB).
            //might have to invert y coordinates here
            _lut->from_byte_packed(output->pixelAt(0, 0),_img->bits(), roi, output->getRoD(),output->getRoD(),
                                   Natron::Color::PACKING_BGRA,Natron::Color::PACKING_RGBA,true,false);
            break;
        case QImage::Format_ARGB32_Premultiplied: // The image is stored using a premultiplied 32-bit ARGB format (0xAARRGGBB).
            //might have to invert y coordinates here
            _lut->from_byte_packed(output->pixelAt(0, 0),_img->bits(), roi, output->getRoD(),output->getRoD(),
                                   Natron::Color::PACKING_BGRA,Natron::Color::PACKING_RGBA,true,true);
            break;
        case QImage::Format_Mono: // The image is stored using 1-bit per pixel. Bytes are packed with the most significant bit (MSB) first.
        case QImage::Format_MonoLSB: // The image is stored using 1-bit per pixel. Bytes are packed with the less significant bit (LSB) first.
        case QImage::Format_Indexed8: // The image is stored using 8-bit indexes into a colormap.
        case QImage::Format_RGB16: // The image is stored using a 16-bit RGB format (5-6-5).
        case QImage::Format_RGB666: // The image is stored using a 24-bit RGB format (6-6-6). The unused most significant bits is always zero.
        case QImage::Format_RGB555: // The image is stored using a 16-bit RGB format (5-5-5). The unused most significant bit is always zero.
        case QImage::Format_RGB888: // The image is stored using a 24-bit RGB format (8-8-8).
        case QImage::Format_RGB444: // The image is stored using a 16-bit RGB format (4-4-4). The unused bits are always zero.
        {
            QImage img = _img->convertToFormat(QImage::Format_ARGB32);
            _lut->from_byte_packed(output->pixelAt(0, 0), img.bits(), roi, output->getRoD(), output->getRoD(),
                                   Natron::Color::PACKING_BGRA, Natron::Color::PACKING_RGBA, true, false);
        }
            break;
        case QImage::Format_ARGB8565_Premultiplied: // The image is stored using a premultiplied 24-bit ARGB format (8-5-6-5).
        case QImage::Format_ARGB6666_Premultiplied: // The image is stored using a premultiplied 24-bit ARGB format (6-6-6-6).
        case QImage::Format_ARGB8555_Premultiplied: // The image is stored using a premultiplied 24-bit ARGB format (8-5-5-5).
        case QImage::Format_ARGB4444_Premultiplied: // The image is stored using a premultiplied 16-bit ARGB format (4-4-4-4).
        {
            QImage img = _img->convertToFormat(QImage::Format_ARGB32_Premultiplied);
            _lut->from_byte_packed(output->pixelAt(0, 0), img.bits(), roi, output->getRoD(), output->getRoD(),
                                   Natron::Color::PACKING_BGRA, Natron::Color::PACKING_RGBA, true, true);
        }
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

