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


#include "WriteQt.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <QtGui/QImage>
#include <QtGui/QImageWriter>

#include "Engine/Lut.h"
#include "Engine/Format.h"
#include "Writers/Writer.h"
#include "Engine/Row.h"

using namespace std;
using namespace Powiter;

WriteQt::WriteQt(Writer* writer)
:Write(writer)
,_rod()
,_buf(NULL)
,_filename(){
    
}
WriteQt::~WriteQt(){
    
}

/*Should return the list of file types supported by the encoder: "png","jpg", etc..*/
std::vector<std::string> WriteQt::fileTypesEncoded() const {
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
std::string WriteQt::encoderName() const {
    return "QImage (Qt)";
}

/*Must be implemented to tell whether this file type supports stereovision*/
bool WriteQt::supports_stereo() const {
    return false;
}


/*Must implement it to initialize the appropriate colorspace  for
 the file type. You can initialize the _lut member by calling the
 function getLut(datatype) */
void WriteQt::initializeColorSpace(){
    _lut = Color::getLut(Color::LUT_DEFAULT_INT8);
}

/*This function initialises the output file/output storage structure and put necessary info in it, like
 meta-data, channels, etc...This is called on the main thread so don't do any extra processing here,
 otherwise it would stall the GUI.*/
void WriteQt::setupFile(const QString& filename,const Box2D& rod){
    _filename = filename;
    _rod = rod;
    size_t dataSize = 4* rod.width() * rod.height();
    _buf = (uchar*)malloc(dataSize);
}

/*This function must fill the pre-allocated structure with the data calculated by engine.
 This function must close the file as writeAllData is the LAST function called before the
 destructor of Write.*/
void WriteQt::writeAllData(){
    const ChannelSet& channels = op->requestedChannels();
    QImage::Format type;
    if (channels & Channel_alpha && _premult) {
        type = QImage::Format_ARGB32_Premultiplied;
    }else if(channels & Channel_alpha && !_premult){
        type = QImage::Format_ARGB32;
    }else{
        type = QImage::Format_RGB32;
    }
    QImage img(_buf,_rod.width(),_rod.height(),type);
    img.save(_filename);
    free(_buf);
}

void WriteQt::supportsChannelsForWriting(ChannelSet& channels) const {
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

void WriteQt::renderRow(SequenceTime time,int left,int right,int y,const ChannelSet& channels){
    boost::shared_ptr<const Row> row = op->input(0)->get(time,y,left,right,channels);
    
    /*invert y to be in top-to-bottom increasing order*/
    y = _rod.height()-y-1;
    
    uchar* toB  = _buf + y*4*_rod.width();
    uchar* toG = toB+1;
    uchar* toR = toG+1;
    uchar* toA = toR+1;
    
    const float* red = row->begin(Channel_red);
    const float* green = row->begin(Channel_green);
    const float* blue = row->begin(Channel_blue);
    const float* alpha = row->begin(Channel_alpha);
    
    to_byte(Channel_red, toR, red, alpha, row->width(),4);
    to_byte(Channel_green, toG, green, alpha, row->width(),4);
    to_byte(Channel_blue, toB, blue, alpha, row->width(),4);
    to_byte(Channel_alpha, toA, alpha, alpha, row->width(),4);
}


