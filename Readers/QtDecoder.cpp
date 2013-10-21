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

#include "QtDecoder.h"

#include <QtGui/QImage>
#include <QtGui/QColor>
#include <QtGui/QImageReader>

#include "Readers/Reader.h"

#include "Engine/Lut.h"
#include "Engine/Row.h"

using namespace Powiter;
using std::cout; using std::endl;

QtDecoder::QtDecoder(Reader* op)
: Decoder(op)
, _img(0)
, filename()
{
}

void QtDecoder::initializeColorSpace(){
    _lut=Color::getLut(Color::LUT_DEFAULT_VIEWER);
}

QtDecoder::~QtDecoder(){
    if(_img)
        delete _img;
}

std::vector<std::string> QtDecoder::fileTypesDecoded() const {
    std::vector<std::string> out;
    const QList<QByteArray>& supported = QImageReader::supportedImageFormats();
    for (int i = 0; i < supported.size(); ++i) {
        out.push_back(supported.at(i).data());
    }
    return out;
};


void QtDecoder::render(SequenceTime /*time*/,Powiter::Row* out) {
    const ChannelSet& channels = out->channels();
    int y = out->y();
    switch(_img->format()) {
        case QImage::Format_Invalid:
        {
            foreachChannels(z, channels){
                const float* to = out->begin(z) ;
                if (to != NULL) {
                    std::fill(out->begin(z), out->end(z), 0.f);
                }
            }
        }
            break;
        case QImage::Format_RGB32: // The image is stored using a 32-bit RGB format (0xffRRGGBB).
        case QImage::Format_ARGB32: // The image is stored using a 32-bit ARGB format (0xAARRGGBB).
        case QImage::Format_ARGB32_Premultiplied: // The image is stored using a premultiplied 32-bit ARGB format (0xAARRGGBB).
        {
            int h = _img->height();
            int Y = h - y - 1;
//            if(autoAlpha() && !_img->hasAlphaChannel()){
//                out->turnOn(Channel_alpha);
//            }
            const QRgb* from = reinterpret_cast<const QRgb*>(_img->scanLine(Y));
            foreachChannels(z, channels){
                if((int)z <= 4){ // qrgb contains only rgba
                    float* to = out->begin(z) ;
                    if (to != NULL) {
                        from_byteQt(z, out->begin(z) - out->left(), from, _img->width(),1);
                    }
                }else{
                    //default initialize  the channel
                    if(z != Channel_alpha)
                        out->fill(z,0.f);
                    else
                        out->fill(z, 1.f);
                }
            }
        }
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
        default:
        {
            int h = _img->height();
            int Y = h - y - 1;
            for (int X = out->left(); X < out->right(); ++X) {
                QRgb c = _img->pixel(X, Y);
                foreachChannels(z, channels){
                    if((int) z < 4){
                        float* to = out->begin(z) ;
                        if (to != NULL) {
                            from_byteQt(z, to+X , &c, 1, 1);
                        }
                    }else{
                        //default initialize to 0 the channel
                        float *to = out->begin(z)+X;
                        if(z != Channel_alpha)
                            *to = 0.f;
                        else
                            *to = 1.f;
                    }
                }
            }
        }
            break;
    }
}
Powiter::Status QtDecoder::readHeader(const QString& filename_)
{
    filename = filename_;
    /*load does actually loads the data too. And we must call it to read the header.
     That means in this case the readAllData function is useless*/
    _img= new QImage(filename);
    
    if(_img->format() == QImage::Format_Invalid){
        cout << "Couldn't load this image format" << endl;
        return StatFailed;
    }
    int width = _img->width();
    int height= _img->height();
    double aspect = _img->dotsPerMeterX()/ _img->dotsPerMeterY();

	ChannelSet mask;    
    if(_autoCreateAlpha){
        
		mask = Mask_RGBA;
        
    }else{
        if(_img->format()==QImage::Format_ARGB32 || _img->format()==QImage::Format_ARGB32_Premultiplied
           || _img->format()==QImage::Format_ARGB4444_Premultiplied ||
           _img->format()==QImage::Format_ARGB6666_Premultiplied ||
           _img->format()==QImage::Format_ARGB8555_Premultiplied ||
           _img->format()==QImage::Format_ARGB8565_Premultiplied){
            
            if(_img->format()== QImage::Format_ARGB32_Premultiplied
               || _img->format()== QImage::Format_ARGB4444_Premultiplied
               || _img->format()== QImage::Format_ARGB6666_Premultiplied
               || _img->format()== QImage::Format_ARGB8555_Premultiplied
               || _img->format()== QImage::Format_ARGB8565_Premultiplied){
                _premult=true;
                
            }
            mask = Mask_RGBA;
        }
        else{
			mask = Mask_RGB;
            
        }
    }
    
    Format imageFormat(0,0,width,height,"",aspect);
    Box2D bbox(0,0,width,height);
    setReaderInfo(imageFormat, bbox, mask);
    return StatOK;
}

