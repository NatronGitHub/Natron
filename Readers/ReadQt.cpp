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

#include "ReadQt.h"

#include <QtGui/QImage>
#include <QtGui/QColor>

#include "Gui/ViewerGL.h"
#include "Readers/Reader.h"
#include "Gui/NodeGui.h"
#include "Engine/Lut.h"
#include "Engine/Row.h"

using namespace std;
using namespace Powiter;

ReadQt::ReadQt(Reader* op)
: Read(op)
, _img(0)
, filename()
{
}

void ReadQt::initializeColorSpace(){
    _lut=Color::getLut(Color::LUT_DEFAULT_VIEWER);
}

ReadQt::~ReadQt(){
    if(_img)
        delete _img;
}

void ReadQt::engine(int y,int offset,int range,ChannelSet channels,Row* out) {
    switch(_img->format()) {
        case QImage::Format_Invalid:
        {
            foreachChannels(z, channels){
                float* to = out->writable(z) ;
                if (to != NULL) {
                    std::fill(to+out->offset(), to+out->offset() + (range-offset), 0.);
                }
            }
        }
            break;
        case QImage::Format_RGB32: // The image is stored using a 32-bit RGB format (0xffRRGGBB).
        case QImage::Format_ARGB32: // The image is stored using a 32-bit ARGB format (0xAARRGGBB).
        case QImage::Format_ARGB32_Premultiplied: // The image is stored using a premultiplied 32-bit ARGB format (0xAARRGGBB).
        {
            int h = op->info().displayWindow().height();
            int Y = h - y - 1;
            const uchar* buffer = _img->scanLine(Y);
            if(autoAlpha() && !_img->hasAlphaChannel()){
                out->turnOn(Channel_alpha);
            }
            const QRgb* from = reinterpret_cast<const QRgb*>(buffer) + offset;
            foreachChannels(z, channels){
                float* to = out->writable(z) ;
                if (to != NULL) {
                    from_byteQt(z, to+out->offset(), from, (range-offset),1);
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
            int h = op->info().displayWindow().height();
            int Y = h - y - 1;
            for (int X = offset; X < range; ++X) {
                QRgb c = _img->pixel(X, Y);
                foreachChannels(z, channels){
                    float* to = out->writable(z) ;
                    if (to != NULL) {
                        from_byteQt(z, to+out->offset()+X, &c, 1, 1);
                    }
                }
            }
        }
            break;
    }
}

bool ReadQt::supports_stereo() const {
    return false;
}

void ReadQt::readHeader(const QString& filename_, bool)
{
    filename = filename_;
    /*load does actually loads the data too. And we must call it to read the header.
     That means in this case the readAllData function is useless*/
    _img= new QImage(filename);

    if(_img->format() == QImage::Format_Invalid){
        cout << "Couldn't load this image format" << endl;
        return ;
    }
    int width = _img->width();
    int height= _img->height();
    double aspect = _img->dotsPerMeterX()/ _img->dotsPerMeterY();
    
	bool rgb=true;
	int ydirection = -1;
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
    set_readerInfo(imageFormat, bbox, filename, mask, ydirection, rgb);
}
void ReadQt::readAllData(bool){
// does nothing
}



void ReadQt::make_preview() {
    op->setPreview(_img->scaled(64, 64, Qt::KeepAspectRatio));
}
