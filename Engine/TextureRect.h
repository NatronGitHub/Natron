//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_TEXTURERECT_H_
#define NATRON_ENGINE_TEXTURERECT_H_

/** @class This class describes the rectangle (or portion) of an image that is contained
 *into a texture. x,y,r,t are respectivly the image coordinates of the left,bottom,right,top
 *edges of the texture. w,h are the width and height of the texture. Note that r - x != w
 *and likewise t - y != h , this is because a texture might not contain all the lines/columns
 *of the image in the portion defined by x,y,r,t.
 **/
class TextureRect{
public:
    
    TextureRect() : x(0) , y(0) , r(0) , t(0) , w(0) , h(0) {}
    
    TextureRect(int x_, int y_, int r_, int t_, int w_, int h_) :
    x(x_),
    y(y_),
    r(r_),
    t(t_),
    w(w_),
    h(h_){}
    
    int x,y,r,t; // the edges of the texture. These are coordinates in the full size image
                 // unlike the RectI class, r and t are INCLUDED in the rectangle such as:
                 // x <= column <= r
                 // and
                 // y <= row <= t
    int w,h; // the width and height of the texture. This has nothing to do with x,y,r,t
};
inline bool operator==(const TextureRect& first ,const TextureRect& second){
   return first.x == second.x &&
    first.y == second.y &&
    first.r == second.r &&
    first.t == second.t;// &&
    //first.w == second.w &&
    //first.h == second.h;
}

inline bool operator!=(const TextureRect& first ,const TextureRect& second){ return !(first == second); }

#endif // NATRON_ENGINE_TEXTURERECT_H_
