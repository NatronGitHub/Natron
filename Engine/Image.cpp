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

#include "Image.h"
#include "Writers/Encoder.h"
#include "Engine/Lut.h"

std::list<Box2D> Powiter::Bitmap::minimalNonMarkedRects(const Box2D& roi) const{
    /*for now a simple version that computes the bbox*/
    std::list<Box2D> ret;
    
    /*if we rendered everything we just append
     a NULL box to indicate we rendered it all.*/
    if(_pixelsRenderedCount >= _rod.area()){
        ret.push_back(Box2D());
        return ret;
    }
    
    Box2D bbox = roi;
    //find bottom
    for (int i = bbox.bottom(); i < bbox.top();++i) {
        char* buf = _map + (i-_rod.bottom())*_rod.width();
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
        char* buf = _map + (i-_rod.bottom())*_rod.width();
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

void Powiter::Bitmap::markForRendered(const Box2D& roi){
    for (int i = roi.bottom(); i < roi.top();++i) {
        char* buf = _map + (i-_rod.bottom())*_rod.width();
        memset(buf, 1, roi.width());
    }
    _pixelsRenderedCount += roi.area();
}
namespace Powiter{
    void debugImage(Powiter::Image* img){
        const Box2D& rod = img->getRoD();
        QImage output(rod.width(),rod.height(),QImage::Format_ARGB32_Premultiplied);
        const Powiter::Color::Lut* lut = Powiter::Color::getLut(Powiter::Color::LUT_DEFAULT_INT8);
        lut->to_byte_rect(output.bits(), img->pixelAt(0, 0), rod, rod,true,true, Powiter::Color::Lut::BGRA);
        U64 hashKey = img->getHashKey();
        QString hashKeyStr = QString::number(hashKey);
        output.save(hashKeyStr+".png");
    }
}

