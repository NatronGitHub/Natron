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

RectI Natron::Bitmap::minimalNonMarkedBbox(const RectI& roi) const
{
    /*if we rendered everything we just append
     a NULL box to indicate we rendered it all.*/
//    if(!memchr(_map.get(),0,_rod.area())){
//        ret.push_back(RectI());
//        return ret;
//    }
    
    RectI bbox = roi;
    //find bottom
    for (int i = bbox.bottom(); i < bbox.top();++i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        if(!memchr(buf, 0, _rod.width())){
            bbox.set_bottom(bbox.bottom()+1);
        } else {
            break;
        }
    }

    //find top (will do zero iteration if the bbox is already empty)
    for (int i = bbox.top()-1; i >= bbox.bottom();--i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        if (!memchr(buf, 0, _rod.width())) {
            bbox.set_top(bbox.top()-1);
        } else {
            break;
        }
    }

    // avoid making bbox.width() iterations for nothing
    if (bbox.isNull()) {
        return bbox;
    }

    //find left
    for (int j = bbox.left(); j < bbox.right(); ++j) {
        bool shouldStop = false;
        for (int i = bbox.bottom(); i < bbox.top(); ++i) {
            if (!_map[(i-_rod.bottom())*_rod.width() + (j-_rod.left())]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bbox.set_left(bbox.left()+1);
        } else {
            break;
        }
    }

    //find right
    for (int j = bbox.right()-1; j >= bbox.left(); --j) {
        bool shouldStop = false;
        for (int i = bbox.bottom(); i < bbox.top(); ++i) {
            if (!_map[(i-_rod.bottom())*_rod.width() + (j-_rod.left())]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bbox.set_right(bbox.right()-1);
        } else {
            break;
        }
    }
    return bbox;
}

std::list<RectI> Natron::Bitmap::minimalNonMarkedRects(const RectI& roi) const
{
    /*for now a simple version that computes the bbox*/
    std::list<RectI> ret;

    RectI bbox = minimalNonMarkedBbox(roi);
//#define NATRON_BITMAP_DISABLE_OPTIMIZATION
#ifndef NATRON_BITMAP_DISABLE_OPTIMIZATION
    if (bbox.isNull()) {
        return ret; // return an empty rectangle list
    }

    // optimization by Fred, Jan 31, 2014
    //
    // Now that we have the smallest enclosing bounding box,
    // let's try to find rectangles for the bottom, the top,
    // the left and the right part.
    // This happens quite often, for example when zooming out
    // (in this case the area to compute is formed of A, B, C and D,
    // and X is already rendered), or when panning (in this case the area
    // is just two rectangles, e.g. A and C, and the rectangles B, D and
    // X are already rendered).
    // The rectangles A, B, C and D from the following drawing are just
    // zeroes, and X contains zeroes and ones.
    //
    // BBBBBBBBBBBBBB
    // BBBBBBBBBBBBBB
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // AAAAAAAAAAAAAA

    // First, find if there's an "A" rectangle, and push it to the result
    //find bottom
    RectI bbox_sub = bbox;
    bbox_sub.set_top(bbox.bottom());
    for (int i = bbox.bottom(); i < bbox.top();++i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        if (!memchr(buf, 1, _rod.width())) {
            bbox.set_bottom(bbox.bottom()+1);
            bbox_sub.set_top(bbox.bottom());
        } else {
            break;
        }
    }
    if (!bbox_sub.isNull()) { // empty boxes should not be pushed
        ret.push_back(bbox_sub);
    }

    // Now, find the "B" rectangle
    //find top
    bbox_sub = bbox;
    bbox_sub.set_bottom(bbox.top());
    for (int i = bbox.top()-1; i >= bbox.bottom();--i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        if (!memchr(buf, 1, _rod.width())) {
            bbox.set_top(bbox.top()-1);
            bbox_sub.set_bottom(bbox.top());
        } else {
            break;
        }
    }
    if (!bbox_sub.isNull()) { // empty boxes should not be pushed
        ret.push_back(bbox_sub);
    }

    //find left
    bbox_sub = bbox;
    bbox_sub.set_right(bbox.left());
    for (int j = bbox.left(); j < bbox.right(); ++j) {
        bool shouldStop = false;
        for (int i = bbox.bottom(); i < bbox.top(); ++i) {
            if (_map[(i-_rod.bottom())*_rod.width()+(j-_rod.left())]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bbox.set_left(bbox.left()+1);
            bbox_sub.set_right(bbox.left());
        } else {
            break;
        }
    }
    if (!bbox_sub.isNull()) { // empty boxes should not be pushed
        ret.push_back(bbox_sub);
    }

    //find right
    bbox_sub = bbox;
    bbox_sub.set_left(bbox.right());
    for (int j = bbox.right()-1; j >= bbox.left(); --j) {
        bool shouldStop = false;
        for (int i = bbox.bottom(); i < bbox.top(); ++i) {
            if (_map[(i-_rod.bottom())*_rod.width()+(j-_rod.left())]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bbox.set_right(bbox.right()-1);
            bbox_sub.set_left(bbox.right());
        } else {
            break;
        }
    }

    // get the bounding box of what's left (the X rectangle in the drawing above)
    bbox = minimalNonMarkedBbox(bbox);
#endif // NATRON_BITMAP_DISABLE_OPTIMIZATION
    if (!bbox.isNull()) { // empty boxes should not be pushed
        ret.push_back(bbox);
    }
    //qDebug() << "render " << ret.size() << " rectangles";
    return ret;
}

void Natron::Bitmap::markForRendered(const RectI& roi){
    for (int i = roi.bottom(); i < roi.top();++i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        memset(buf, 1, roi.width());
    }
}

void Natron::Image::copy(const Natron::Image& other){
    
    RectI intersection;
    this->_params._rod.intersect(other._params._rod, &intersection);
    
    if (intersection.isNull()) {
        return;
    }
    
    const float* src = other.pixelAt(0, 0);
    float* dst = pixelAt(0, 0);
    memcpy(dst, src, intersection.area());
}

namespace Natron{
    void debugImage(Natron::Image* img,const QString& filename){
        const RectI& rod = img->getRoD();
        QImage output(rod.width(),rod.height(),QImage::Format_ARGB32);
        const Natron::Color::Lut* lut = Natron::Color::LutManager::sRGBLut();
        lut->to_byte_packed(output.bits(), img->pixelAt(0, 0), rod, rod, rod,
                            Natron::Color::PACKING_RGBA,Natron::Color::PACKING_BGRA, true,false);
        U64 hashKey = img->getHashKey();
        QString hashKeyStr = QString::number(hashKey);
        QString realFileName = filename.isEmpty() ? QString(hashKeyStr+".png") : filename;
        output.save(realFileName);
    }
    
}

