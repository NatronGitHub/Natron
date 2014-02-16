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
    std::list<RectI> ret;

    RectI bboxM = minimalNonMarkedBbox(roi);
//#define NATRON_BITMAP_DISABLE_OPTIMIZATION
#ifdef NATRON_BITMAP_DISABLE_OPTIMIZATION
    if (!bboxM.isNull()) { // empty boxes should not be pushed
        ret.push_back(bboxM);
    }
#else
    if (bboxM.isNull()) {
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
    RectI bboxX = bboxM;
    RectI bboxA = bboxX;
    bboxA.set_top(bboxX.bottom());
    for (int i = bboxX.bottom(); i < bboxX.top();++i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        if (!memchr(buf, 1, _rod.width())) {
            bboxX.set_bottom(bboxX.bottom()+1);
            bboxA.set_top(bboxX.bottom());
        } else {
            break;
        }
    }
    if (!bboxA.isNull()) { // empty boxes should not be pushed
        ret.push_back(bboxA);
    }

    // Now, find the "B" rectangle
    //find top
    RectI bboxB = bboxX;
    bboxB.set_bottom(bboxX.top());
    for (int i = bboxX.top()-1; i >= bboxX.bottom();--i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width()];
        if (!memchr(buf, 1, _rod.width())) {
            bboxX.set_top(bboxX.top()-1);
            bboxB.set_bottom(bboxX.top());
        } else {
            break;
        }
    }
    if (!bboxB.isNull()) { // empty boxes should not be pushed
        ret.push_back(bboxB);
    }

    //find left
    RectI bboxC = bboxX;
    bboxC.set_right(bboxX.left());
    for (int j = bboxX.left(); j < bboxX.right(); ++j) {
        bool shouldStop = false;
        for (int i = bboxX.bottom(); i < bboxX.top(); ++i) {
            if (_map[(i-_rod.bottom())*_rod.width()+(j-_rod.left())]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bboxX.set_left(bboxX.left()+1);
            bboxC.set_right(bboxX.left());
        } else {
            break;
        }
    }
    if (!bboxC.isNull()) { // empty boxes should not be pushed
        ret.push_back(bboxC);
    }

    //find right
    RectI bboxD = bboxX;
    bboxD.set_left(bboxX.right());
    for (int j = bboxX.right()-1; j >= bboxX.left(); --j) {
        bool shouldStop = false;
        for (int i = bboxX.bottom(); i < bboxX.top(); ++i) {
            if (_map[(i-_rod.bottom())*_rod.width()+(j-_rod.left())]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bboxX.set_right(bboxX.right()-1);
            bboxD.set_left(bboxX.right());
        } else {
            break;
        }
    }
    if (!bboxD.isNull()) { // empty boxes should not be pushed
        ret.push_back(bboxD);
    }

    assert(bboxA.bottom() == bboxM.bottom());
    assert(bboxA.left() == bboxM.left());
    assert(bboxA.right() == bboxM.right());
    assert(bboxA.top() == bboxX.bottom());

    assert(bboxB.top() == bboxM.top());
    assert(bboxB.left() == bboxM.left());
    assert(bboxB.right() == bboxM.right());
    assert(bboxB.bottom() == bboxX.top());

    assert(bboxC.top() == bboxX.top());
    assert(bboxC.left() == bboxM.left());
    assert(bboxC.right() == bboxX.left());
    assert(bboxC.bottom() == bboxX.bottom());

    assert(bboxD.top() == bboxX.top());
    assert(bboxD.left() == bboxX.right());
    assert(bboxD.right() == bboxM.right());
    assert(bboxD.bottom() == bboxX.bottom());

    // get the bounding box of what's left (the X rectangle in the drawing above)
    bboxX = minimalNonMarkedBbox(bboxX);
    if (!bboxX.isNull()) { // empty boxes should not be pushed
        ret.push_back(bboxX);
    }

#endif // NATRON_BITMAP_DISABLE_OPTIMIZATION
    qDebug() << "render " << ret.size() << " rectangles";
    for (std::list<RectI>::const_iterator it = ret.begin(); it != ret.end(); ++it) {
        qDebug() << "rect: " << it->x1 << ',' << it->x2 << ',' << it->y1 << ',' << it->y2;
    }
    return ret;
}

void Natron::Bitmap::markForRendered(const RectI& roi){
    for (int i = roi.bottom(); i < roi.top();++i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width() + (roi.left() - _rod.left())];
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
