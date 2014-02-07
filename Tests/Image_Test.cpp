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

#include <gtest/gtest.h>
#include "Engine/Image.h"


TEST(BitmapTest,SimpleRect) {
    RectI rod(0,0,100,100);
    Natron::Bitmap bm(rod);

    ///assert that the union of all the non rendered rects is the rod
    std::list<RectI> nonRenderedRects = bm.minimalNonMarkedRects(rod);
    RectI nonRenderedRectsUnion;
    for(std::list<RectI>::iterator it = nonRenderedRects.begin();it!=nonRenderedRects.end();++it) {
        nonRenderedRectsUnion.merge(*it);
    }
}
