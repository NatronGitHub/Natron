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


#include "QtEncoder.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <QtGui/QImage>
#include <QtGui/QImageWriter>

#include "Engine/Lut.h"
#include "Engine/Format.h"
#include "Writers/Writer.h"
#include "Engine/Row.h"
#include "Engine/Image.h"

using namespace Powiter;

QtEncoder::QtEncoder(Writer* writer)
:Encoder(writer)
,_rod()
,_buf(NULL)
,_outputImage(NULL)
{
    
}
QtEncoder::~QtEncoder(){
    
}

/*Should return the list of file types supported by the encoder: "png","jpg", etc..*/
std::vector<std::string> QtEncoder::fileTypesEncoded() const {
    std::vector<std::string> out;
    // Qt Image reader should be the last solution (it cannot read 16-bits ppm or png)
    const QList<QByteArray>& supported = QImageWriter::supportedImageFormats();
    // Qt 4 supports: BMP, JPG, JPEG, PNG, PBM, PGM, PPM, TIFF, XBM, XPM
    // Qt 5 doesn't support TIFF
    for (int i = 0; i < supported.count(); ++i) {
        out.push_back(std::string(supported.at(i).toLower().data()));
    }
    return out;
}

/*Should return the name of the write handle : "ffmpeg", "OpenEXR" ...*/
std::string QtEncoder::encoderName() const {
    return "QImage (Qt)";
}

/*Must be implemented to tell whether this file type supports stereovision*/
bool QtEncoder::supports_stereo() const {
    return false;
}

/*Must implement it to initialize the appropriate colorspace  for
 the file type. You can initialize the _lut member by calling the
 function getLut(datatype) */
void QtEncoder::initializeColorSpace(){
    _lut = Color::getLut(Color::LUT_DEFAULT_INT8);
}

/*This function initialises the output file/output storage structure and put necessary info in it, like
 meta-data, channels, etc...This is called on the main thread so don't do any extra processing here,
 otherwise it would stall the GUI.*/
Powiter::Status QtEncoder::setupFile(const QString& /*filename*/,const Box2D& rod){
    _rod = rod;
    size_t dataSize = 4 * rod.area();
    _buf = (uchar*)malloc(dataSize);
    const ChannelSet& channels = _writer->requestedChannels();
    QImage::Format type;
    if (channels & Channel_alpha && _premult) {
        type = QImage::Format_ARGB32_Premultiplied;
    }else if(channels & Channel_alpha && !_premult){
        type = QImage::Format_ARGB32;
    }else{
        type = QImage::Format_RGB32;
    }
    _outputImage = new QImage(_buf,_rod.width(),_rod.height(),type);
    return Powiter::StatOK;
}

void QtEncoder::finalizeFile(){
    
    _outputImage->save(filename());
    delete _outputImage;
    free(_buf);
}

void QtEncoder::supportsChannelsForWriting(ChannelSet& channels) const {
    foreachChannels(z, channels){
        if(z!= Channel_red &&
           z!= Channel_green &&
           z!= Channel_blue &&
           z!= Channel_alpha){
            throw std::runtime_error("Qt only supports writing image files with red/green/blue/alpha channels.");
            return;
        }
    }
}

void QtEncoder::render(boost::shared_ptr<const Powiter::Image> inputImage,int /*view*/,const Box2D& roi){
    to_byte_rect(_buf, inputImage->pixelAt(0, 0), roi,inputImage->getRoD(),Powiter::Color::Lut::BGRA,true);
}


