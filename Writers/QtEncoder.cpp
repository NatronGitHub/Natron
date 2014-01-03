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


#include "QtEncoder.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <QtGui/QImage>
#include <QtGui/QImageWriter>

#include "Global/AppManager.h"

#include "Engine/Lut.h"
#include "Engine/Format.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/TimeLine.h"

using namespace Natron;

QtWriter::QtWriter(Natron::Node* node)
:Natron::OutputEffectInstance(node)
, _lut(Natron::Color::LutManager::sRGBLut())
{
    
}

QtWriter::~QtWriter(){
}

std::string QtWriter::pluginID() const {
    return "QtWriter";
}
std::string QtWriter::pluginLabel() const{
    return "QtWriter";
}

std::string QtWriter::description() const {
    return "The QtWriter node can render on disk the output of a node graph using the QImage (Qt) library.";
}


/*Should return the list of file types supported by the encoder: "png","jpg", etc..*/
void QtWriter::supportedFileFormats(std::vector<std::string>* formats) {
    std::vector<std::string> out;
    // Qt Image reader should be the last solution (it cannot read 16-bits ppm or png)
    const QList<QByteArray>& supported = QImageWriter::supportedImageFormats();
    // Qt 4 supports: BMP, JPG, JPEG, PNG, PBM, PGM, PPM, TIFF, XBM, XPM
    // Qt 5 doesn't support TIFF
    for (int i = 0; i < supported.count(); ++i) {
        formats->push_back(std::string(supported.at(i).toLower().data()));
    }
}

void QtWriter::getFrameRange(SequenceTime *first,SequenceTime *last){
    int index = _frameRangeChoosal->getValue<int>();
    if(index == 0){
        EffectInstance* inp = input(0);
        if(inp){
            inp->getFrameRange(first, last);
        }else{
            *first = 0;
            *last = 0;
        }
    }else if(index == 1){
        *first = getApp()->getTimeLine()->leftBound();
        *last = getApp()->getTimeLine()->rightBound();
    }else{
        *first = _firstFrameKnob->getValue<int>();
        *last = _lastFrameKnob->getValue<int>();
    }
}


void QtWriter::initializeKnobs(){
    _premultKnob = Natron::createKnob<Bool_Knob>(this, "Premultiply by alpha");
    _premultKnob->turnOffAnimation();
    _premultKnob->setValue<bool>(false);
    
    _fileKnob = Natron::createKnob<OutputFile_Knob>(this, "File");
    _fileKnob->setAsOutputImageFile();
    
    _frameRangeChoosal = Natron::createKnob<Choice_Knob>(this, "Frame range");
    _frameRangeChoosal->turnOffAnimation();
    std::vector<std::string> frameRangeChoosalEntries;
    frameRangeChoosalEntries.push_back("Inputs union");
    frameRangeChoosalEntries.push_back("Timeline bounds");
    frameRangeChoosalEntries.push_back("Manual");
    _frameRangeChoosal->populate(frameRangeChoosalEntries);
    
    _firstFrameKnob = Natron::createKnob<Int_Knob>(this, "First frame");
    _firstFrameKnob->turnOffAnimation();
    
    _lastFrameKnob = Natron::createKnob<Int_Knob>(this, "Last frame");
    _lastFrameKnob->turnOffAnimation();
    
    _renderKnob = Natron::createKnob<Button_Knob>(this, "Render");
    _renderKnob->setAsRenderButton();
}

void QtWriter::onKnobValueChanged(Knob* k,Natron::ValueChangedReason /*reason*/){
    if(k == _frameRangeChoosal.get()){
        int index = _frameRangeChoosal->getValue<int>();
        if(index != 2){
            _firstFrameKnob->setSecret(true);
            _lastFrameKnob->setSecret(true);
        }else{
            int first = getApp()->getTimeLine()->firstFrame();
            int last = getApp()->getTimeLine()->lastFrame();
            _firstFrameKnob->setValue(first);
            _firstFrameKnob->setDisplayMinimum(first);
            _firstFrameKnob->setDisplayMaximum(last);
            _firstFrameKnob->setSecret(false);
            
            _lastFrameKnob->setValue(last);
            _lastFrameKnob->setDisplayMinimum(first);
            _lastFrameKnob->setDisplayMaximum(last);
            _lastFrameKnob->setSecret(false);
            
            createKnobDynamically();
        }
        
    }
}

Natron::Status QtWriter::render(SequenceTime time, RenderScale scale, const RectI& roi, int view, boost::shared_ptr<Natron::Image> output){
    
    boost::shared_ptr<Natron::Image> src = getImage(0, time, scale, view);
    
    if(hasOutputConnected()){
        output->copy(*src);
    }
    
    const Format& frmt = getApp()->getProjectFormat();
    
    ////initializes to black
    unsigned char* buf = (unsigned char*)calloc(frmt.area() * 4,1);
    
    
    QImage::Format type;
    bool premult = _premultKnob->getValue<bool>();
    if (premult) {
        type = QImage::Format_ARGB32_Premultiplied;
    }else{
        type = QImage::Format_ARGB32;
    }
    
    
    _lut->to_byte_packed(buf, src->pixelAt(0, 0), roi, roi, roi, Natron::Color::PACKING_RGBA, Natron::Color::PACKING_BGRA, true, premult);
    
    QImage img(buf,roi.width(),roi.height(),type);
    
    std::string filename = _fileKnob->filenameFromPattern(std::floor(time + 0.5));
    
    img.save(filename.c_str());
    free(buf);
    return StatOK;
}



