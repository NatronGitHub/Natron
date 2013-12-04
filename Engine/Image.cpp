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

#include "Image.h"

#include "Engine/Lut.h"

#include "Writers/Encoder.h"

std::list<RectI> Natron::Bitmap::minimalNonMarkedRects(const RectI& roi) const{
    /*for now a simple version that computes the bbox*/
    std::list<RectI> ret;
    
    /*if we rendered everything we just append
     a NULL box to indicate we rendered it all.*/
    if(!memchr(_map.get(),0,_rod.area())){
        ret.push_back(RectI());
        return ret;
    }
    
    RectI bbox = roi;
    //find bottom
    for (int i = bbox.bottom(); i < bbox.top();++i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        if(!memchr(buf,0,_rod.width())){
            bbox.set_bottom(bbox.bottom()+1);
            if(bbox.top() <= bbox.bottom()){
                bbox.clear();
                ret.push_back(bbox);
                return ret;
            }
        }else{
            break;
        }
    }

    //find top
    for (int i = bbox.top()-1; i >= bbox.bottom();--i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        if(!memchr(buf,0,_rod.width())){
            bbox.set_top(bbox.top()-1);
        }else{
            break;
        }
    }

    //find left
    for (int j = bbox.left(); j < bbox.right(); ++j) {
        bool shouldStop = false;
        for (int i = bbox.bottom(); i < bbox.top(); ++i) {
            if (!_map[(i-_rod.bottom())*_rod.width()+(j-_rod.left())]) {
                shouldStop = true;
                break;
            }
        }
        if(!shouldStop){
            bbox.set_left(bbox.left()+1);
        }else{
            break;
        }
    }
    
    //find right
    for (int j = bbox.right()-1; j >= bbox.left(); --j) {
        bool shouldStop = false;
        for (int i = bbox.bottom(); i < bbox.top(); ++i) {
            if (!_map[(i-_rod.bottom())*_rod.width()+(j-_rod.left())]) {
                shouldStop = true;
                break;
            }
        }
        if(!shouldStop){
            bbox.set_right(bbox.right()-1);
        }else{
            break;
        }
    }

    ret.push_back(bbox);
    return ret;
}

void Natron::Bitmap::markForRendered(const RectI& roi){
    for (int i = roi.bottom(); i < roi.top();++i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        memset(buf, 1, roi.width());
    }
}
namespace Natron{
    void debugImage(Natron::Image* img,const QString& filename){
        const RectI& rod = img->getRoD();
        QImage output(rod.width(),rod.height(),QImage::Format_ARGB32_Premultiplied);
        const Natron::Color::Lut* lut = Natron::Color::getLut(Natron::Color::LUT_DEFAULT_INT8);
        lut->to_byte_rect_premult(output.bits(), img->pixelAt(0, 0), rod, rod, rod, true, Natron::Color::Lut::BGRA);
        U64 hashKey = img->getHashKey();
        QString hashKeyStr = QString::number(hashKey);
        QString realFileName = filename.isEmpty() ? QString(hashKeyStr+".png") : filename;
        output.save(realFileName);
    }
    
}

