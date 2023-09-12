/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Image.h"

#include <algorithm> // min, max
#include <cassert>
#include <cstring> // for std::memcpy, std::memset
#include <stdexcept>

#include <QtCore/QDebug>

#include "Engine/AppManager.h"
#include "Engine/ViewIdx.h"
#include "Engine/GPUContextPool.h"
#include "Engine/OSGLContext.h"
#include "Engine/GLShader.h"

NATRON_NAMESPACE_ENTER

#define BM_GET(i, j) (&_map[( i - _bounds.bottom() ) * _bounds.width() + ( j - _bounds.left() )])

#define PIXEL_UNAVAILABLE 2

template <int trimap>
RectI
minimalNonMarkedBbox_internal(const RectI& roi,
                              const RectI& _bounds,
                              const std::vector<char>& _map,
                              bool* isBeingRenderedElsewhere)
{
    RectI bbox;

    assert( _bounds.contains(roi) );
    bbox = roi;

    //find bottom
    for (int i = bbox.bottom(); i < bbox.top(); ++i) {
        const char* buf = BM_GET( i, bbox.left() );

        if (trimap) {
            const char* lineEnd = buf + bbox.width();
            bool metUnavailablePixel = false;
            while (buf < lineEnd) {
                if (!*buf) {
                    buf = 0;
                    break;
                } else if (*buf == PIXEL_UNAVAILABLE) {
                    metUnavailablePixel = true;
                }
                ++buf;
            }
            if (!buf) {
                break;
            } else if (metUnavailablePixel) {
                *isBeingRenderedElsewhere = true; //< only flag if the whole row is not 0
                ++bbox.y1;
            } else {
                ++bbox.y1;
            }
        } else {
            const char* lineEnd = buf + bbox.width();
            while (buf < lineEnd) {
                if ( !*buf || (*buf == PIXEL_UNAVAILABLE) ) {
                    buf = 0;
                    break;
                }
                ++buf;
            }
            if (!buf) {
                break;
            } else {
                ++bbox.y1;
            }
        }
    }

    //find top (will do zero iteration if the bbox is already empty)
    for (int i = bbox.top() - 1; i >= bbox.bottom(); --i) {
        const char* buf = BM_GET( i, bbox.left() );

        if (trimap) {
            const char* lineEnd = buf + bbox.width();
            bool metUnavailablePixel = false;
            while (buf < lineEnd) {
                if (!*buf) {
                    buf = 0;
                    break;
                } else if (*buf == PIXEL_UNAVAILABLE) {
                    metUnavailablePixel = true;
                }
                ++buf;
            }
            if (!buf) {
                break;
            } else if (metUnavailablePixel) {
                *isBeingRenderedElsewhere = true; //< only flag if the whole row is not 0
                --bbox.y2;
            } else {
                --bbox.y2;
            }
        } else {
            const char* lineEnd = buf + bbox.width();
            while (buf < lineEnd) {
                if ( !*buf || (*buf == PIXEL_UNAVAILABLE) ) {
                    buf = 0;
                    break;
                }
                ++buf;
            }
            if (!buf) {
                break;
            } else {
                --bbox.y2;
            }
        }
    }

    // avoid making bbox.width() iterations for nothing
    if ( bbox.isNull() ) {
        return bbox;
    }

    //find left
    for (int j = bbox.left(); j < bbox.right(); ++j) {
        const char* pix = BM_GET(bbox.bottom(), j);
        bool metUnavailablePixel = false;

        for ( int i = bbox.bottom(); i < bbox.top(); ++i, pix += _bounds.width() ) {
            if (!*pix) {
                pix = 0;
                break;
            } else if (*pix == PIXEL_UNAVAILABLE) {
                if (trimap) {
                    metUnavailablePixel = true;
                } else {
                    pix = 0;
                    break;
                }
            }
        }
        if (pix) {
            ++bbox.x1;
            if (trimap && metUnavailablePixel) {
                *isBeingRenderedElsewhere = true; //< only flag is the whole column is not 0
            }
        } else {
            break;
        }
    }

    //find right
    for (int j = bbox.right() - 1; j >= bbox.left(); --j) {
        const char* pix = BM_GET(bbox.bottom(), j);
        bool metUnavailablePixel = false;

        for ( int i = bbox.bottom(); i < bbox.top(); ++i, pix += _bounds.width() ) {
            if (!*pix) {
                pix = 0;
                break;
            } else if (*pix == PIXEL_UNAVAILABLE) {
                if (trimap) {
                    metUnavailablePixel = true;
                } else {
                    pix = 0;
                    break;
                }
            }
        }
        if (pix) {
            --bbox.x2;
            if (trimap && metUnavailablePixel) {
                *isBeingRenderedElsewhere = true; //< only flag is the whole column is not 0
            }
        } else {
            break;
        }
    }

    return bbox;
} // minimalNonMarkedBbox_internal


template <int trimap>
void
minimalNonMarkedRects_internal(const RectI & roi,
                               const RectI& _bounds,
                               const std::vector<char>& _map,
                               std::list<RectI>& ret,
                               bool* isBeingRenderedElsewhere)
{
    assert(ret.empty());
    ///Any out of bounds portion is pushed to the rectangles to render
    const RectI intersection = roi.intersect(_bounds);
    if (roi != intersection) {
        if ( (_bounds.x1 > roi.x1) && (_bounds.y2 > _bounds.y1) ) {
            RectI left(roi.x1, _bounds.y1, _bounds.x1, _bounds.y2);
            ret.push_back(left);
        }


        if ( (roi.x2 > roi.x1) && (_bounds.y1 > roi.y1) ) {
            RectI btm(roi.x1, roi.y1, roi.x2, _bounds.y1);
            ret.push_back(btm);
        }


        if ( (roi.x2 > _bounds.x2) && (_bounds.y2 > _bounds.y1) ) {
            RectI right(_bounds.x2, _bounds.y1, roi.x2, _bounds.y2);
            ret.push_back(right);
        }


        if ( (roi.x2 > roi.x1) && (roi.y2 > _bounds.y2) ) {
            RectI top(roi.x1, _bounds.y2, roi.x2, roi.y2);
            ret.push_back(top);
        }
    }

    if ( intersection.isNull() ) {
        return;
    }

    RectI bboxM = minimalNonMarkedBbox_internal<trimap>(intersection, _bounds, _map, isBeingRenderedElsewhere);
    assert( (trimap && isBeingRenderedElsewhere) || (!trimap && !isBeingRenderedElsewhere) );

    //#define NATRON_BITMAP_DISABLE_OPTIMIZATION
#ifdef NATRON_BITMAP_DISABLE_OPTIMIZATION
    if ( !bboxM.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxM);
    }
#else
    if ( bboxM.isNull() ) {
        return; // return an empty rectangle list
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
    bboxA.set_top( bboxX.bottom() );
    for (int i = bboxX.bottom(); i < bboxX.top(); ++i) {
        const char* buf = BM_GET( i, bboxX.left() );
        if (trimap) {
            const char* lineEnd = buf + bboxX.width();
            bool metUnavailablePixel = false;
            while (buf < lineEnd) {
                if (*buf == 1) {
                    buf = 0;
                    break;
                } else if ( (*buf == PIXEL_UNAVAILABLE) && trimap ) {
                    buf = 0;
                    metUnavailablePixel = true;
                    break;
                }
                ++buf;
            }
            if (buf) {
                ++bboxX.y1;
                bboxA.y2 = bboxX.y1;
            } else {
                if (metUnavailablePixel) {
                    *isBeingRenderedElsewhere = true;
                }
                break;
            }
        } else {
            if ( !memchr( buf, 1, bboxX.width() ) ) {
                ++bboxX.y1;
                bboxA.y2 = bboxX.y1;
            } else {
                break;
            }
        }
    }
    if ( !bboxA.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxA);
    }

    // Now, find the "B" rectangle
    //find top
    RectI bboxB = bboxX;
    bboxB.set_bottom( bboxX.top() );
    for (int i = bboxX.top() - 1; i >= bboxX.bottom(); --i) {
        const char* buf = BM_GET( i, bboxX.left() );

        if (trimap) {
            const char* lineEnd = buf + bboxX.width();
            bool metUnavailablePixel = false;
            while (buf < lineEnd) {
                if (*buf == 1) {
                    buf = 0;
                    break;
                } else if ( (*buf == PIXEL_UNAVAILABLE) && trimap ) {
                    buf = 0;
                    metUnavailablePixel = true;
                    break;
                }
                ++buf;
            }
            if (buf) {
                --bboxX.y2;
                bboxB.y1 = bboxX.y2;
            } else {
                if (metUnavailablePixel) {
                    *isBeingRenderedElsewhere = true;
                }
                break;
            }
        } else {
            if ( !memchr( buf, 1, bboxX.width() ) ) {
                --bboxX.y2;
                bboxB.y1 = bboxX.y2;
            } else {
                break;
            }
        }
    }
    if ( !bboxB.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxB);
    }

    //find left
    RectI bboxC = bboxX;
    bboxC.set_right( bboxX.left() );
    if ( bboxX.bottom() < bboxX.top() ) {
        for (int j = bboxX.left(); j < bboxX.right(); ++j) {
            const char* pix = BM_GET(bboxX.bottom(), j);
            bool metUnavailablePixel = false;

            for ( int i = bboxX.bottom(); i < bboxX.top(); ++i, pix += _bounds.width() ) {
                if (*pix == 1) {
                    pix = 0;
                    break;
                } else if ( trimap && (*pix == PIXEL_UNAVAILABLE) ) {
                    pix = 0;
                    metUnavailablePixel = true;
                    break;
                }
            }
            if (pix) {
                ++bboxX.x1;
                bboxC.x2 = bboxX.x1;
            } else {
                if (metUnavailablePixel) {
                    *isBeingRenderedElsewhere = true;
                }
                break;
            }
        }
    }
    if ( !bboxC.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxC);
    }

    //find right
    RectI bboxD = bboxX;
    bboxD.set_left( bboxX.right() );
    if ( bboxX.bottom() < bboxX.top() ) {
        for (int j = bboxX.right() - 1; j >= bboxX.left(); --j) {
            const char* pix = BM_GET(bboxX.bottom(), j);
            bool metUnavailablePixel = false;

            for ( int i = bboxX.bottom(); i < bboxX.top(); ++i, pix += _bounds.width() ) {
                if (*pix == 1) {
                    pix = 0;
                    break;
                } else if ( trimap && (*pix == PIXEL_UNAVAILABLE) ) {
                    pix = 0;
                    metUnavailablePixel = true;
                    break;
                }
            }
            if (pix) {
                --bboxX.x2;
                bboxD.x1 = bboxX.x2;
            } else {
                if (metUnavailablePixel) {
                    *isBeingRenderedElsewhere = true;
                }
                break;
            }
        }
    }
    if ( !bboxD.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxD);
    }

    assert( bboxA.bottom() == bboxM.bottom() );
    assert( bboxA.left() == bboxM.left() );
    assert( bboxA.right() == bboxM.right() );
    assert( bboxA.top() == bboxX.bottom() );

    assert( bboxB.top() == bboxM.top() );
    assert( bboxB.left() == bboxM.left() );
    assert( bboxB.right() == bboxM.right() );
    assert( bboxB.bottom() == bboxX.top() );

    assert( bboxC.top() == bboxX.top() );
    assert( bboxC.left() == bboxM.left() );
    assert( bboxC.right() == bboxX.left() );
    assert( bboxC.bottom() == bboxX.bottom() );

    assert( bboxD.top() == bboxX.top() );
    assert( bboxD.left() == bboxX.right() );
    assert( bboxD.right() == bboxM.right() );
    assert( bboxD.bottom() == bboxX.bottom() );

    // get the bounding box of what's left (the X rectangle in the drawing above)
    bboxX = minimalNonMarkedBbox_internal<trimap>(bboxX, _bounds, _map, isBeingRenderedElsewhere);

    if ( !bboxX.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxX);
    }

#endif // NATRON_BITMAP_DISABLE_OPTIMIZATION
} // minimalNonMarkedRects

RectI
Bitmap::minimalNonMarkedBbox(const RectI & roi) const
{
    RectI realRoi = roi;
    if (_dirtyZoneSet && !realRoi.clipIfOverlaps(_dirtyZone)) {
        return RectI();
    }

    return minimalNonMarkedBbox_internal<0>(realRoi, _bounds, _map, NULL);
}

void
Bitmap::minimalNonMarkedRects(const RectI & roi,
                              std::list<RectI>& ret) const
{
    RectI realRoi = roi;
    if (_dirtyZoneSet && !realRoi.clipIfOverlaps(_dirtyZone)) {
        return;
    }
    minimalNonMarkedRects_internal<0>(realRoi, _bounds, _map, ret, NULL);
}

#if NATRON_ENABLE_TRIMAP
RectI
Bitmap::minimalNonMarkedBbox_trimap(const RectI & roi,
                                    bool* isBeingRenderedElsewhere) const
{
    RectI realRoi = roi;
    if (_dirtyZoneSet && !realRoi.clipIfOverlaps(_dirtyZone)) {
        *isBeingRenderedElsewhere = false;
        return RectI();
    }

    return minimalNonMarkedBbox_internal<1>(realRoi, _bounds, _map, isBeingRenderedElsewhere);
}

void
Bitmap::minimalNonMarkedRects_trimap(const RectI & roi,
                                     std::list<RectI>& ret,
                                     bool* isBeingRenderedElsewhere) const
{
    RectI realRoi = roi;
    if (_dirtyZoneSet && !realRoi.clipIfOverlaps(_dirtyZone)) {
        *isBeingRenderedElsewhere = false;
        return;
    }
    minimalNonMarkedRects_internal<1>(realRoi, _bounds, _map, ret, isBeingRenderedElsewhere);
}

#endif

void
Bitmap::markFor(const RectI & roi, char value)
{
    int x1 = std::max(roi.x1, _bounds.x1);
    int y1 = std::max(roi.y1, _bounds.y1);
    int x2 = std::min(roi.x2, _bounds.x2);
    int y2 = std::min(roi.y2, _bounds.y2);

    char* buf = BM_GET(y1, x1);
    int w = _bounds.width();
    int roiw = x2 - x1;

    for (int i = y1; i < y2; ++i, buf += w) {
        std::memset( buf, value, roiw);
    }
}

bool
Bitmap::isNonMarked(const RectI & roi) const
{
    int x1 = std::max(roi.x1, _bounds.x1);
    int y1 = std::max(roi.y1, _bounds.y1);
    int x2 = std::min(roi.x2, _bounds.x2);
    int y2 = std::min(roi.y2, _bounds.y2);

    const char* buf = BM_GET(y1, x1);
    int w = _bounds.width();
    int roiw = x2 - x1;

    for (int i = y1; i < y2; ++i, buf += w) {
        for (int j = 0; j < roiw; ++j) {
            if (buf[j]) {
                return false;
            }
        }
    }
    return true;
}

#if NATRON_ENABLE_TRIMAP
void
Bitmap::markForRendering(const RectI & roi)
{
    return markFor(roi, PIXEL_UNAVAILABLE);
}

#endif

void
Bitmap::swap(Bitmap& other)
{
    _map.swap(other._map);
    _bounds = other._bounds;
    _dirtyZone.clear(); //merge(other._dirtyZone);
    _dirtyZoneSet = false;
}

const char*
Bitmap::getBitmapAt(int x,
                    int y) const
{
    if ( ( x >= _bounds.left() ) && ( x < _bounds.right() ) && ( y >= _bounds.bottom() ) && ( y < _bounds.top() ) ) {
        return BM_GET(y, x);
    } else {
        return NULL;
    }
}

char*
Bitmap::getBitmapAt(int x,
                    int y)
{
    if ( ( x >= _bounds.left() ) && ( x < _bounds.right() ) && ( y >= _bounds.bottom() ) && ( y < _bounds.top() ) ) {
        return BM_GET(y, x);
    } else {
        return NULL;
    }
}

#ifdef DEBUG
void
Image::printUnrenderedPixels(const RectI& roi) const
{
    if (!_useBitmap) {
        return;
    }
    QReadLocker k(&_entryLock);
    const char* bm = _bitmap.getBitmapAt(roi.x1, roi.y1);
    int roiw = roi.x2 - roi.x1;
    int boundsW = _bitmap.getBounds().width();
    RectD bboxUnrendered;
    bboxUnrendered.setupInfinity();
    RectD bboxUnavailable;
    bboxUnavailable.setupInfinity();

    bool hasUnrendered = false;
    bool hasUnavailable = false;

    for ( int y = roi.y1; y < roi.y2; ++y,
          bm += (boundsW - roiw) ) {
        for (int x = roi.x1; x < roi.x2; ++x, ++bm) {
            if (*bm == 0) {
                if (x < bboxUnrendered.x1) {
                    bboxUnrendered.x1 = x;
                }
                if (x > bboxUnrendered.x2) {
                    bboxUnrendered.x2 = x;
                }
                if (y < bboxUnrendered.y1) {
                    bboxUnrendered.y1 = y;
                }
                if (y > bboxUnrendered.y2) {
                    bboxUnrendered.y2 = y;
                }
                hasUnrendered = true;
            } else if (*bm == PIXEL_UNAVAILABLE) {
                if (x < bboxUnavailable.x1) {
                    bboxUnavailable.x1 = x;
                }
                if (x > bboxUnavailable.x2) {
                    bboxUnavailable.x2 = x;
                }
                if (y < bboxUnavailable.y1) {
                    bboxUnavailable.y1 = y;
                }
                if (y > bboxUnavailable.y2) {
                    bboxUnavailable.y2 = y;
                }
                hasUnavailable = true;
            }
        } // for x
    } // for y
    if (hasUnrendered) {
        qDebug() << "Unrenderer pixels in the following region:";
        bboxUnrendered.debug();
    }
    if (hasUnavailable) {
        qDebug() << "Unavailable pixels in the following region:";
        bboxUnavailable.debug();
    }
} // Image::printUnrenderedPixels

#endif \
    // ifdef DEBUG

Image::Image(const ImageKey & key,
             const ImageParamsPtr& params,
             const CacheAPI* cache)
    : CacheEntryHelper<unsigned char, ImageKey, ImageParams>(key, params, cache)
    , _useBitmap(true)
{
    _bitDepth = params->getBitDepth();
    _depthBytesSize = getSizeOfForBitDepth(_bitDepth);
    _nbComponents = params->getComponents().getNumComponents();
    _rod = params->getRoD();
    _bounds = params->getBounds();
    _par = params->getPixelAspectRatio();
    _premult = params->getPremultiplication();
    _fielding = params->getFieldingOrder();
}

Image::Image(const ImageKey & key,
             const ImageParamsPtr& params)
    : CacheEntryHelper<unsigned char, ImageKey, ImageParams>( key, params, NULL )
    , _useBitmap(false)
{
    _bitDepth = params->getBitDepth();
    _depthBytesSize = getSizeOfForBitDepth(_bitDepth);
    _nbComponents = params->getComponents().getNumComponents();
    _rod = params->getRoD();
    _bounds = params->getBounds();
    _par = params->getPixelAspectRatio();
    _premult = params->getPremultiplication();
    _fielding = params->getFieldingOrder();

    allocateMemory();
}

/*This constructor can be used to allocate a local Image. The deallocation should
   then be handled by the user. Note that no view number is passed in parameter
   as it is not needed.*/
Image::Image(const ImagePlaneDesc& components,
             const RectD & regionOfDefinition, //!< rod in canonical coordinates
             const RectI & bounds, //!< bounds in pixel coordinates
             unsigned int mipMapLevel,
             double par,
             ImageBitDepthEnum bitdepth,
             ImagePremultiplicationEnum premult,
             ImageFieldingOrderEnum fielding,
             bool useBitmap,
             StorageModeEnum storage,
             U32 textureTarget)
    : CacheEntryHelper<unsigned char, ImageKey, ImageParams>()
    , _useBitmap(useBitmap)
{
    setCacheEntry(makeKey(0, 0, false, 0, ViewIdx(0), false, false),
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
                  ImageParamsPtr( new ImageParams(regionOfDefinition,
                                                                  par,
                                                                  mipMapLevel,
                                                                  bounds,
                                                                  bitdepth,
                                                                  fielding,
                                                                  premult,
                                                                  false /*isRoDProjectFormat*/,
                                                                  components,
                                                                  storage,
                                                                  textureTarget) ),
#else
                  std::make_shared<ImageParams>(regionOfDefinition,
                                                  par,
                                                  mipMapLevel,
                                                  bounds,
                                                  bitdepth,
                                                  fielding,
                                                  premult,
                                                  false /*isRoDProjectFormat*/,
                                                  components,
                                                  storage,
                                                  textureTarget),
#endif
                  NULL /*cacheAPI*/
                  );

    _bitDepth = bitdepth;
    _depthBytesSize = getSizeOfForBitDepth(_bitDepth);
    _nbComponents = components.getNumComponents();
    _rod = regionOfDefinition;
    _bounds = _params->getBounds();
    _par = par;
    _premult = premult;
    _fielding = fielding;

    allocateMemory();
}

Image::~Image()
{
    deallocate();
}

void
Image::onMemoryAllocated(bool diskRestoration)
{
    if (_cache || _useBitmap) {
        _bitmap.initialize(_bounds);
    }

    if (diskRestoration) {
        _bitmap.setTo1();
    }

#ifdef DEBUG
    if (!diskRestoration) {
        ///fill with red, to recognize unrendered pixels
        //fill(_bounds,1.,0.,0.,1.);
    }
#endif
}

void
Image::setBitmapDirtyZone(const RectI& zone)
{
    QWriteLocker k(&_entryLock);

    _bitmap.setDirtyZone(zone);
}

ImageKey
Image::makeKey(const CacheEntryHolder* holder,
               U64 nodeHashKey,
               bool frameVaryingOrAnimated,
               double time,
               ViewIdx view,
               bool draftMode,
               bool fullScaleWithDownscaleInputs)
{
    return ImageKey(holder, nodeHashKey, frameVaryingOrAnimated, time, view, 1., draftMode, fullScaleWithDownscaleInputs);
}

ImageParamsPtr
Image::makeParams(const RectD & rod,
                  const double par,
                  unsigned int mipMapLevel,
                  bool isRoDProjectFormat,
                  const ImagePlaneDesc& components,
                  ImageBitDepthEnum bitdepth,
                  ImagePremultiplicationEnum premult,
                  ImageFieldingOrderEnum fielding,
                  StorageModeEnum storage,
                  U32 textureTarget)
{
    const RectI bounds = rod.toPixelEnclosing(mipMapLevel, par);

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    return ImageParamsPtr( new ImageParams(rod,
                                                           par,
                                                           mipMapLevel,
                                                           bounds,
                                                           bitdepth,
                                                           fielding,
                                                           premult,
                                                           isRoDProjectFormat,
                                                           components,
                                                           storage,
                                                           textureTarget) );
#else
    return std::make_shared<ImageParams>(rod,
                                           par,
                                           mipMapLevel,
                                           bounds,
                                           bitdepth,
                                           fielding,
                                           premult,
                                           isRoDProjectFormat,
                                           components,
                                           storage,
                                           textureTarget);
#endif
}

ImageParamsPtr
Image::makeParams(const RectD & rod,    // the image rod in canonical coordinates
                  const RectI& bounds,
                  const double par,
                  unsigned int mipMapLevel,
                  bool isRoDProjectFormat,
                  const ImagePlaneDesc& components,
                  ImageBitDepthEnum bitdepth,
                  ImagePremultiplicationEnum premult,
                  ImageFieldingOrderEnum fielding,
                  StorageModeEnum storage,
                  U32 textureTarget)
{
#ifdef DEBUG
    RectI pixelRod;
    rod.toPixelEnclosing(mipMapLevel, par, &pixelRod);
    assert( bounds.left() >= pixelRod.left() && bounds.right() <= pixelRod.right() &&
            bounds.bottom() >= pixelRod.bottom() && bounds.top() <= pixelRod.top() );
#endif

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    return ImageParamsPtr( new ImageParams(rod,
                                                           par,
                                                           mipMapLevel,
                                                           bounds,
                                                           bitdepth,
                                                           fielding,
                                                           premult,
                                                           isRoDProjectFormat,
                                                           components,
                                                           storage,
                                                           textureTarget) );
#else
    return std::make_shared<ImageParams>(rod,
                                           par,
                                           mipMapLevel,
                                           bounds,
                                           bitdepth,
                                           fielding,
                                           premult,
                                           isRoDProjectFormat,
                                           components,
                                           storage,
                                           textureTarget);
#endif
}

// code proofread and fixed by @devernay on 8/8/2014
template<typename PIX>
void
Image::pasteFromForDepth(const Image & srcImg,
                         const RectI & srcRoi,
                         bool copyBitmap,
                         bool takeSrcLock)
{
    ///Cannot copy images with different bit depth, this is not the purpose of this function.
    ///@see convert
    assert( getBitDepth() == srcImg.getBitDepth() );
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) || (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) || (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );
    // NOTE: before removing the following asserts, please explain why an empty image may happen

    QWriteLocker k(&_entryLock);
    std::unique_ptr<QReadLocker> k2;
    if (takeSrcLock && &srcImg != this) {
        k2.reset( new QReadLocker(&srcImg._entryLock) );
    }

    const RectI & bounds = _bounds;
    const RectI & srcBounds = srcImg._bounds;

    assert( !bounds.isNull() );
    assert( !srcBounds.isNull() );

    // only copy the intersection of roi, bounds and otherBounds
    RectI roi = srcRoi;
    if (!roi.clipIfOverlaps(bounds)) {
        // no intersection between roi and the bounds of this image
        return;
    }
    if (!roi.clipIfOverlaps(srcBounds)) {
        // no intersection between roi and the bounds of the other image
        return;
    }

    assert( getComponents() == srcImg.getComponents() );

    if (copyBitmap && _useBitmap) {
        copyBitmapPortion(roi, srcImg);
    }
    // now we're safe: both images contain the area in roi

    int srcRowElements = _nbComponents * srcBounds.width();
    int dstRowElements = _nbComponents * bounds.width();
    const PIX* src = (const PIX*)srcImg.pixelAt(roi.x1, roi.y1);
    PIX* dst = (PIX*)pixelAt(roi.x1, roi.y1);

    assert(src && dst);

    for (int y = roi.y1; y < roi.y2;
         ++y,
         src += srcRowElements,
         dst += dstRowElements) {
        std::memcpy(dst, src, roi.width() * sizeof(PIX) * _nbComponents);
#ifdef DEBUG_NAN
        for (int i = 0; i < roi.width() * _nbComponents; ++i) {
            assert( !std::isnan(src[i]) ); // check for NaN
            assert( !std::isnan(dst[i]) ); // check for NaN
        }
#endif
    }
} // Image::pasteFromForDepth

void
Image::setRoD(const RectD& rod)
{
    QWriteLocker k(&_entryLock);

    _rod = rod;
    _params->setRoD(rod);
}

void
Image::resizeInternal(const Image* srcImg,
                      const RectI& srcBounds,
                      const RectI& merge,
                      bool fillWithBlackAndTransparent,
                      bool setBitmapTo1,
                      bool createInCache,
                      ImagePtr* outputImage)
{
    ///Allocate to resized image
    if (!createInCache) {
        *outputImage = std::make_shared<Image>( srcImg->getComponents(),
                                       srcImg->getRoD(),
                                       merge,
                                       srcImg->getMipMapLevel(),
                                       srcImg->getPixelAspectRatio(),
                                       srcImg->getBitDepth(),
                                       srcImg->getPremultiplication(),
                                       srcImg->getFieldingOrder(),
                                       srcImg->usesBitMap() );
    } else {
        ImageParamsPtr params = std::make_shared<ImageParams>( *srcImg->getParams() );
        params->setBounds(merge);
        *outputImage = std::make_shared<Image>( srcImg->getKey(), params, srcImg->getCacheAPI() );
        (*outputImage)->allocateMemory();
    }
    ImageBitDepthEnum depth = srcImg->getBitDepth();

    if (fillWithBlackAndTransparent) {
        /*
           Compute the rectangles (A,B,C,D) where to set the image to 0

           AAAAAAAAAAAAAAAAAAAAAAAAAAAA
           AAAAAAAAAAAAAAAAAAAAAAAAAAAA
           DDDDDXXXXXXXXXXXXXXXXXXBBBBB
           DDDDDXXXXXXXXXXXXXXXXXXBBBBB
           DDDDDXXXXXXXXXXXXXXXXXXBBBBB
           DDDDDXXXXXXXXXXXXXXXXXXBBBBB
           CCCCCCCCCCCCCCCCCCCCCCCCCCCC
           CCCCCCCCCCCCCCCCCCCCCCCCCCCC
         */
        RectI aRect;
        aRect.x1 = merge.x1;
        aRect.y1 = srcBounds.y2;
        aRect.y2 = merge.y2;
        aRect.x2 = merge.x2;

        RectI bRect;
        bRect.x1 = srcBounds.x2;
        bRect.y1 = srcBounds.y1;
        bRect.x2 = merge.x2;
        bRect.y2 = srcBounds.y2;

        RectI cRect;
        cRect.x1 = merge.x1;
        cRect.y1 = merge.y1;
        cRect.x2 = merge.x2;
        cRect.y2 = srcBounds.y1;

        RectI dRect;
        dRect.x1 = merge.x1;
        dRect.y1 = srcBounds.y1;
        dRect.x2 = srcBounds.x1;
        dRect.y2 = srcBounds.y2;

        Image::WriteAccess wacc( outputImage->get() );
        std::size_t pixelSize = srcImg->getComponentsCount() * getSizeOfForBitDepth(depth);

        if ( !aRect.isNull() ) {
            char* pix = (char*)wacc.pixelAt(aRect.x1, aRect.y1);
            assert(pix);
            U64 a = aRect.area();
            std::size_t memsize = a * pixelSize;
            std::memset(pix, 0, memsize);
            if ( setBitmapTo1 && (*outputImage)->usesBitMap() ) {
                char* bm = wacc.bitmapAt(aRect.x1, aRect.y1);
                assert(bm);
                std::memset(bm, 1, a);
            }
        }
        if ( !cRect.isNull() ) {
            char* pix = (char*)wacc.pixelAt(cRect.x1, cRect.y1);
            assert(pix);
            U64 a = cRect.area();
            std::size_t memsize = a * pixelSize;
            std::memset(pix, 0, memsize);
            if ( setBitmapTo1 && (*outputImage)->usesBitMap() ) {
                char* bm = (char*)wacc.bitmapAt(cRect.x1, cRect.y1);
                assert(bm);
                std::memset(bm, 1, a);
            }
        }
        if ( !bRect.isNull() ) {
            char* pix = (char*)wacc.pixelAt(bRect.x1, bRect.y1);
            assert(pix);
            int mw = merge.width();
            std::size_t rowsize = mw * pixelSize;
            int bw = bRect.width();
            std::size_t rectRowSize = bw * pixelSize;
            char* bm = ( setBitmapTo1 && (*outputImage)->usesBitMap() ) ? wacc.bitmapAt(bRect.x1, bRect.y1) : 0;
            for (int y = bRect.y1; y < bRect.y2; ++y, pix += rowsize) {
                std::memset(pix, 0, rectRowSize);
                if (bm) {
                    std::memset(bm, 1, bw);
                    bm += mw;
                }
            }
        }
        if ( !dRect.isNull() ) {
            char* pix = (char*)wacc.pixelAt(dRect.x1, dRect.y1);
            assert(pix);
            int mw = merge.width();
            std::size_t rowsize = mw * pixelSize;
            int dw = dRect.width();
            std::size_t rectRowSize = dw * pixelSize;
            char* bm = ( setBitmapTo1 && (*outputImage)->usesBitMap() ) ? wacc.bitmapAt(dRect.x1, dRect.y1) : 0;
            for (int y = dRect.y1; y < dRect.y2; ++y, pix += rowsize) {
                std::memset(pix, 0, rectRowSize);
                if (bm) {
                    std::memset(bm, 1, dw);
                    bm += mw;
                }
            }
        }
    } // fillWithBlackAndTransparent


    switch (depth) {
    case eImageBitDepthByte:
        (*outputImage)->pasteFromForDepth<unsigned char>(*srcImg, srcBounds, srcImg->usesBitMap(), false);
        break;
    case eImageBitDepthShort:
        (*outputImage)->pasteFromForDepth<unsigned short>(*srcImg, srcBounds, srcImg->usesBitMap(), false);
        break;
    case eImageBitDepthHalf:
        assert(false);
        break;
    case eImageBitDepthFloat:
        (*outputImage)->pasteFromForDepth<float>(*srcImg, srcBounds, srcImg->usesBitMap(), false);
        break;
    case eImageBitDepthNone:
        break;
    }
} // Image::resizeInternal

bool
Image::copyAndResizeIfNeeded(const RectI& newBounds,
                             bool fillWithBlackAndTransparent,
                             bool setBitmapTo1,
                             ImagePtr* output)
{
    assert(getStorageMode() != eStorageModeGLTex);
    if ( getBounds().contains(newBounds) ) {
        return false;
    }
    assert(output);

    QReadLocker k(&_entryLock);
    RectI merge = newBounds;
    merge.merge(_bounds);

    resizeInternal(this, _bounds, merge, fillWithBlackAndTransparent, setBitmapTo1, usesBitMap(), output);

    return true;
}

bool
Image::ensureBounds(const RectI& newBounds,
                    bool fillWithBlackAndTransparent,
                    bool setBitmapTo1)
{
    // OpenGL textures are not resizable yet
    assert(_params->getStorageInfo().mode != eStorageModeGLTex);
    if ( getBounds().contains(newBounds) ) {
        return false;
    }

    QWriteLocker k(&_entryLock);
    RectI merge = newBounds;
    merge.merge(_bounds);

    ImagePtr tmpImg;
    resizeInternal(this, _bounds, merge, fillWithBlackAndTransparent, setBitmapTo1, false, &tmpImg);


    ///Change the size of the current buffer
    _bounds = merge;
    _params->setBounds(merge);
    assert( _bounds.contains(newBounds) );
    swapBuffer(*tmpImg);
    if ( usesBitMap() ) {
        _bitmap.swap(tmpImg->_bitmap);
    }

    return true;
}

void
Image::applyTextureMapping(const RectI& bounds,
                           const RectI& roi)
{
    glViewport( roi.x1 - bounds.x1, roi.y1 - bounds.y1, roi.width(), roi.height() );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho( roi.x1, roi.x2,
             roi.y1, roi.y2,
             -10.0 * (roi.y2 - roi.y1), 10.0 * (roi.y2 - roi.y1) );
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glCheckError();

    // Compute the texture coordinates to match the srcRoi
    Point srcTexCoords[4], vertexCoords[4];
    vertexCoords[0].x = roi.x1;
    vertexCoords[0].y = roi.y1;
    srcTexCoords[0].x = (roi.x1 - bounds.x1) / (double)bounds.width();
    srcTexCoords[0].y = (roi.y1 - bounds.y1) / (double)bounds.height();

    vertexCoords[1].x = roi.x2;
    vertexCoords[1].y = roi.y1;
    srcTexCoords[1].x = (roi.x2 - bounds.x1) / (double)bounds.width();
    srcTexCoords[1].y = (roi.y1 - bounds.y1) / (double)bounds.height();

    vertexCoords[2].x = roi.x2;
    vertexCoords[2].y = roi.y2;
    srcTexCoords[2].x = (roi.x2 - bounds.x1) / (double)bounds.width();
    srcTexCoords[2].y = (roi.y2 - bounds.y1) / (double)bounds.height();

    vertexCoords[3].x = roi.x1;
    vertexCoords[3].y = roi.y2;
    srcTexCoords[3].x = (roi.x1 - bounds.x1) / (double)bounds.width();
    srcTexCoords[3].y = (roi.y2 - bounds.y1) / (double)bounds.height();

    glBegin(GL_POLYGON);
    for (int i = 0; i < 4; ++i) {
        glTexCoord2d(srcTexCoords[i].x, srcTexCoords[i].y);
        glVertex2d(vertexCoords[i].x, vertexCoords[i].y);
    }
    glEnd();
    glCheckError();
}

// code proofread and fixed by @devernay on 8/8/2014
void
Image::pasteFrom(const Image & src,
                 const RectI & srcRoi,
                 bool copyBitmap,
                 const OSGLContextPtr& glContext)
{
    if (this == &src) {
        return;
    }
    StorageModeEnum thisStorage = getStorageMode();
    StorageModeEnum otherStorage = src.getStorageMode();

    if ( (thisStorage == eStorageModeGLTex) && (otherStorage == eStorageModeGLTex) ) {
        // OpenGL texture to OpenGL texture
        assert(_params->getStorageInfo().textureTarget == src.getParams()->getStorageInfo().textureTarget);

        RectI dstBounds = getBounds();
        RectI srcBounds = src.getBounds();
        GLuint fboID = glContext->getFBOId();

        glBindFramebuffer(GL_FRAMEBUFFER, fboID);
        int target = getGLTextureTarget();
        glEnable(target);
        glBindTexture( target, getGLTextureID() );
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, getGLTextureID(), 0 /*LoD*/);
        glCheckFramebufferError();
        glBindTexture( target, src.getGLTextureID() );

        applyTextureMapping(dstBounds, srcRoi);

        glBindTexture(target, 0);
        glCheckError();
    } else if ( (thisStorage == eStorageModeGLTex) && (otherStorage != eStorageModeGLTex) ) {
        // RAM image to OpenGL texture
        RectI dstBounds = getBounds();
        RectI srcBounds = src.getBounds();

        // only copy the intersection of roi, bounds and otherBounds
        RectI roi = srcRoi;
        if (!roi.clipIfOverlaps(dstBounds)) {
            // no intersection between roi and the bounds of this image
            return;
        }
        if (!roi.clipIfOverlaps(srcBounds)) {
            // no intersection between roi and the bounds of the other image
            return;
        }
        GLuint pboID = glContext->getPBOId();
        int target = getGLTextureTarget();
        glEnable(target);

        // bind PBO to update texture source
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboID);

        std::size_t dataSize = roi.area() * 4 * src.getParams()->getStorageInfo().dataTypeSize;

        // Note that glMapBufferARB() causes sync issue.
        // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
        // until GPU to finish its job. To avoid waiting (idle), you can call
        // first glBufferDataARB() with NULL pointer before glMapBufferARB().
        // If you do that, the previous data in PBO will be discarded and
        // glMapBufferARB() returns a new allocated pointer immediately
        // even if GPU is still working with the previous data.
        glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, 0, GL_DYNAMIC_DRAW_ARB);

        // map the buffer object into client's memory
        void* gpuData = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
        assert(gpuData);
        if (gpuData) {
            // update data directly on the mapped buffer
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
            ImagePtr tmpImg( new Image( ImagePlaneDesc::getRGBAComponents(), src.getRoD(), roi, 0, src.getPixelAspectRatio(), src.getBitDepth(), src.getPremultiplication(), src.getFieldingOrder(), false, eStorageModeRAM) );
#else
            ImagePtr tmpImg = std::make_shared<Image>( ImagePlaneDesc::getRGBAComponents(), src.getRoD(), roi, 0, src.getPixelAspectRatio(), src.getBitDepth(), src.getPremultiplication(), src.getFieldingOrder(), false, eStorageModeRAM);
#endif
            tmpImg->pasteFrom(src, roi);

            Image::ReadAccess racc(tmpImg ? tmpImg.get() : this);
            const unsigned char* srcdata = racc.pixelAt(roi.x1, roi.y1);
            assert(srcdata);
            
            memcpy(gpuData, srcdata, dataSize);

            GLboolean result = glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release the mapped buffer
            assert(result == GL_TRUE);
            Q_UNUSED(result);
        }

        // bind the texture
        glBindTexture( target, getGLTextureID() );
        // copy pixels from PBO to texture object
        // Use offset instead of pointer (last parameter is 0).
        glTexSubImage2D(target,
                        0,              // level
                        roi.x1, roi.y1,               // xoffset, yoffset
                        roi.width(), roi.height(),
                        src.getGLTextureFormat(),            // format
                        src.getGLTextureType(),       // type
                        0);

        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
        glBindTexture(target, 0);
        glCheckError();
    } else if ( (thisStorage != eStorageModeGLTex) && (otherStorage == eStorageModeGLTex) ) {
        // OpenGL texture to RAM image

        RectI dstBounds = getBounds();
        RectI srcBounds = src.getBounds();

        // only copy the intersection of roi, bounds and otherBounds
        RectI roi = srcRoi;
        if (!roi.clipIfOverlaps(dstBounds)) {
            // no intersection between roi and the bounds of this image
            return;
        }
        if (!roi.clipIfOverlaps(srcBounds)) {
            // no intersection between roi and the bounds of the other image
            return;
        }

        GLuint fboID = glContext->getFBOId();

        glBindFramebuffer(GL_FRAMEBUFFER, fboID);
        int target = src.getGLTextureTarget();
        glEnable(target);
        glBindTexture( target, src.getGLTextureID() );
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, src.getGLTextureID(), 0 /*LoD*/);
        //glViewport( 0, 0, srcBounds.width(), srcBounds.height() );
        glViewport( roi.x1 - srcBounds.x1, roi.y1 - srcBounds.y1, roi.width(), roi.height() );
        glCheckFramebufferError();
        // Ensure all drawing commands are finished
        glFlush();
        glFinish();
        glCheckError();
        // Read to a temporary RGBA buffer then convert to the image which may not be RGBA
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
        ImagePtr tmpImg( new Image( ImagePlaneDesc::getRGBAComponents(), getRoD(), roi, 0, getPixelAspectRatio(), getBitDepth(), getPremultiplication(), getFieldingOrder(), false, eStorageModeRAM) );
#else
        ImagePtr tmpImg = std::make_shared<Image>( ImagePlaneDesc::getRGBAComponents(), getRoD(), roi, 0, getPixelAspectRatio(), getBitDepth(), getPremultiplication(), getFieldingOrder(), false, eStorageModeRAM);
#endif

        {
            Image::WriteAccess tmpAcc(tmpImg ? tmpImg.get() : this);
            unsigned char* data = tmpAcc.pixelAt(roi.x1, roi.y1);

            glReadPixels(roi.x1 - srcBounds.x1, roi.y1 - srcBounds.y1, roi.width(), roi.height(), src.getGLTextureFormat(), src.getGLTextureType(), (GLvoid*)data);
            glBindTexture(target, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCheckError();

        // Ok now convert from RGBA to this image format if needed
        if ( tmpImg->getComponentsCount() != getComponentsCount() ) {
            tmpImg->convertToFormat(roi, eViewerColorSpaceLinear, eViewerColorSpaceLinear, 3, false, false, this);
        } else {
            pasteFrom(*tmpImg, roi, false);
        }
    } else {
        assert(getStorageMode() != eStorageModeGLTex && src.getStorageMode() != eStorageModeGLTex);
        ImageBitDepthEnum depth = getBitDepth();

        switch (depth) {
        case eImageBitDepthByte:
            pasteFromForDepth<unsigned char>(src, srcRoi, copyBitmap, true);
            break;
        case eImageBitDepthShort:
            pasteFromForDepth<unsigned short>(src, srcRoi, copyBitmap, true);
            break;
        case eImageBitDepthHalf:
            assert(false);
            break;
        case eImageBitDepthFloat:
            pasteFromForDepth<float>(src, srcRoi, copyBitmap, true);
            break;
        case eImageBitDepthNone:
            break;
        }
    }
} // pasteFrom

template <typename PIX, int maxValue, int nComps>
void
Image::fillForDepthForComponents(const RectI & roi_,
                                 float r,
                                 float g,
                                 float b,
                                 float a)
{
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) || (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) || (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );

    RectI roi = roi_;
    if (!roi.clipIfOverlaps(_bounds)) {
        // no intersection between roi and the bounds of the image
        return;
    }

    int rowElems = (int)getComponentsCount() * _bounds.width();
    const float fillValue[4] = {
        nComps == 1 ? a * maxValue : r * maxValue, g * maxValue, b * maxValue, a * maxValue
    };


    // now we're safe: the image contains the area in roi
    PIX* dst = (PIX*)pixelAt(roi.x1, roi.y1);
    for ( int i = 0; i < roi.height(); ++i, dst += (rowElems - roi.width() * nComps) ) {
        for (int j = 0; j < roi.width(); ++j, dst += nComps) {
            for (int k = 0; k < nComps; ++k) {
                dst[k] = fillValue[k];
#ifdef DEBUG_NAN
                assert( !std::isnan(dst[k]) ); // check for NaN
#endif
            }
        }
    }
}

// code proofread and fixed by @devernay on 8/8/2014
template <typename PIX, int maxValue>
void
Image::fillForDepth(const RectI & roi_,
                    float r,
                    float g,
                    float b,
                    float a)
{
    switch (_nbComponents) {
    case 0:

        return;
        break;
    case 1:
        fillForDepthForComponents<PIX, maxValue, 1>(roi_, r, g, b, a);
        break;
    case 2:
        fillForDepthForComponents<PIX, maxValue, 2>(roi_, r, g, b, a);
        break;
    case 3:
        fillForDepthForComponents<PIX, maxValue, 3>(roi_, r, g, b, a);
        break;
    case 4:
        fillForDepthForComponents<PIX, maxValue, 4>(roi_, r, g, b, a);
        break;
    default:
        break;
    }
}

// code proofread and fixed by @devernay on 8/8/2014
void
Image::fill(const RectI & roi,
            float r,
            float g,
            float b,
            float a,
            const OSGLContextPtr& glContext)
{
    QWriteLocker k(&_entryLock);

    if (getStorageMode() == eStorageModeGLTex) {
        RectI realRoI = roi;
        if (!realRoI.clipIfOverlaps(_bounds)) {
            // no intersection between roi and the bounds of the image
            return;
        }

        assert(glContext);
        GLShaderPtr shader = glContext->getOrCreateFillShader();
        assert(shader);
        GLuint fboID = glContext->getFBOId();

        glBindFramebuffer(GL_FRAMEBUFFER, fboID);
        int target = getGLTextureTarget();
        glEnable(target);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture( target, getGLTextureID() );

        glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, getGLTextureID(), 0 /*LoD*/);
        glCheckFramebufferError();

        shader->bind();
        OfxRGBAColourF fillColor = {r, g, b, a};
        shader->setUniform("fillColor", fillColor);
        applyTextureMapping(_bounds, realRoI);
        shader->unbind();


        glBindTexture(target, 0);
        glCheckError();

        return;
    }

    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        fillForDepth<unsigned char, 255>(roi, r, g, b, a);
        break;
    case eImageBitDepthShort:
        fillForDepth<unsigned short, 65535>(roi, r, g, b, a);
        break;
    case eImageBitDepthHalf:
        assert(false);
        break;
    case eImageBitDepthFloat:
        fillForDepth<float, 1>(roi, r, g, b, a);
        break;
    case eImageBitDepthNone:
        break;
    }
} // fill

void
Image::fillZero(const RectI& roi,
                const OSGLContextPtr& glContext)
{
    if (getStorageMode() == eStorageModeGLTex) {
        fill(roi, 0., 0., 0., 0., glContext);

        return;
    }

    QWriteLocker k(&_entryLock);
    const RectI intersection = roi.intersect(_bounds);

    if (intersection.isNull()) {
        return;
    }


    std::size_t rowSize =  (std::size_t)_nbComponents;
    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        rowSize *= sizeof(unsigned char);
        break;
    case eImageBitDepthShort:
        rowSize *= sizeof(unsigned short);
        break;
    case eImageBitDepthHalf:
        rowSize *= sizeof(unsigned short);
        break;
    case eImageBitDepthFloat:
        rowSize *= sizeof(float);
        break;
    case eImageBitDepthNone:

        return;
    }

    std::size_t roiMemSize = rowSize * intersection.width();
    rowSize *= _bounds.width();

    char* dstPixels = (char*)pixelAt(intersection.x1, intersection.y1);
    assert(dstPixels);
    for (int y = intersection.y1; y < intersection.y2; ++y, dstPixels += rowSize) {
        std::memset(dstPixels, 0, roiMemSize);
    }
}

void
Image::fillBoundsZero(const OSGLContextPtr& glContext)
{
    if (getStorageMode() == eStorageModeGLTex) {
        fill(getBounds(), 0., 0., 0., 0., glContext);

        return;
    }

    QWriteLocker k(&_entryLock);
    std::size_t rowSize =  (std::size_t)_nbComponents;

    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        rowSize *= sizeof(unsigned char);
        break;
    case eImageBitDepthShort:
        rowSize *= sizeof(unsigned short);
        break;
    case eImageBitDepthHalf:
        rowSize *= sizeof(unsigned short);
        break;
    case eImageBitDepthFloat:
        rowSize *= sizeof(float);
        break;
    case eImageBitDepthNone:

        return;
    }

    std::size_t roiMemSize = rowSize * _bounds.width() * _bounds.height();
    char* dstPixels = (char*)pixelAt(_bounds.x1, _bounds.y1);
    std::memset(dstPixels, 0, roiMemSize);
}

unsigned char*
Image::pixelAt(int x,
               int y)
{
    if ( ( x < _bounds.x1 ) || ( x >= _bounds.x2 ) || ( y < _bounds.y1 ) || ( y >= _bounds.y2 ) ) {
        return NULL;
    } else {
        unsigned char* ret =  (unsigned char*)this->_data.writable();
        if (!ret) {
            return 0;
        }
        int compDataSize = _depthBytesSize * _nbComponents;
        ret = ret + (qint64)( y - _bounds.y1 ) * compDataSize * _bounds.width()
              + (qint64)( x - _bounds.x1 ) * compDataSize;

        return ret;
    }
}

unsigned char*
Image::pixelAtStatic(int x,
                     int y,
                     const RectI& bounds,
                     int nComps,
                     int dataSizeOf,
                     unsigned char* buf)
{
    if ( ( x < bounds.x1 ) || ( x >= bounds.x2 ) || ( y < bounds.y1 ) || ( y >= bounds.y2 ) ) {
        return NULL;
    } else {
        unsigned char* ret = buf;
        if (!ret) {
            return 0;
        }
        int compDataSize = dataSizeOf * nComps;
        ret = ret + (qint64)( y - bounds.y1 ) * compDataSize * bounds.width()
              + (qint64)( x - bounds.x1 ) * compDataSize;

        return ret;
    }
}

const unsigned char*
Image::pixelAt(int x,
               int y) const
{
    if ( ( x < _bounds.x1 ) || ( x >= _bounds.x2 ) || ( y < _bounds.y1 ) || ( y >= _bounds.y2 ) ) {
        return NULL;
    } else {
        unsigned char* ret = (unsigned char*)this->_data.readable();
        if (!ret) {
            return 0;
        }
        int compDataSize = _depthBytesSize * _nbComponents;
        ret = ret + (qint64)( y - _bounds.y1 ) * compDataSize * _bounds.width()
              + (qint64)( x - _bounds.x1 ) * compDataSize;

        return ret;
    }
}

unsigned int
Image::getComponentsCount() const
{
    return _nbComponents;
}

bool
Image::hasEnoughDataToConvert(ImagePlaneDescEnum from,
                              ImagePlaneDescEnum to)
{
    switch (from) {
    case eImageComponentRGBA:

        return true;
    case eImageComponentRGB: {
        switch (to) {
        case eImageComponentRGBA:

            return true; //< let RGB fill the alpha with 0
        case eImageComponentRGB:

            return true;
        case eImageComponentAlpha:

            return false;
        default:

            return false;
        }
        break;
    }
    case eImageComponentAlpha: {
        switch (to) {
        case eImageComponentRGBA:

            return false;
        case eImageComponentRGB:

            return false;
        case eImageComponentAlpha:

            return true;
        default:

            return false;
        }
        break;
    }
    default:

        return false;
        break;
    }
}

std::string
Image::getFormatString(const ImagePlaneDesc& comps,
                       ImageBitDepthEnum depth)
{
    std::string s = comps.getPlaneLabel() + '.' + comps.getChannelsLabel();

    s.append( getDepthString(depth) );

    return s;
}

std::string
Image::getDepthString(ImageBitDepthEnum depth)
{
    std::string s;

    switch (depth) {
    case eImageBitDepthByte:
        s += "8u";
        break;
    case eImageBitDepthShort:
        s += "16u";
        break;
    case eImageBitDepthHalf:
        s += "16f";
        break;
    case eImageBitDepthFloat:
        s += "32f";
        break;
    case eImageBitDepthNone:
        break;
    }

    return s;
}

bool
Image::isBitDepthConversionLossy(ImageBitDepthEnum from,
                                 ImageBitDepthEnum to)
{
    int sizeOfFrom = getSizeOfForBitDepth(from);
    int sizeOfTo = getSizeOfForBitDepth(to);

    return sizeOfTo < sizeOfFrom;
}

double
Image::getPixelAspectRatio() const
{
    return this->_par;
}

ImageFieldingOrderEnum
Image::getFieldingOrder() const
{
    return this->_fielding;
}

ImagePremultiplicationEnum
Image::getPremultiplication() const
{
    return this->_premult;
}

unsigned int
Image::getRowElements() const
{
    QReadLocker k(&_entryLock);

    return getComponentsCount() * _bounds.width();
}

// code proofread and fixed by @devernay on 4/12/2014
template <typename PIX, int maxValue>
void
Image::halveRoIForDepth(const RectI & roi,
                        bool copyBitMap,
                        Image* output) const
{
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) ||
            (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) ||
            (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );

    ///handle case where there is only 1 column/row
    if ( (roi.width() == 1) || (roi.height() == 1) ) {
        assert( !(roi.width() == 1 && roi.height() == 1) ); /// can't be 1x1
        halve1DImage(roi, output);

        return;
    }

    /// Take the lock for both bitmaps since we're about to read/write from them!
    QWriteLocker k1(&output->_entryLock);
    QReadLocker k2(&_entryLock);

    ///The source rectangle, intersected to this image region of definition in pixels
    const RectI &srcBounds = _bounds;
    const RectI &dstBounds = output->_bounds;
    const RectI &srcBmBounds = _bitmap.getBounds();
    const RectI &dstBmBounds = output->_bitmap.getBounds();
    assert( !copyBitMap || usesBitMap() );
    assert( !usesBitMap() || (srcBmBounds == srcBounds && dstBmBounds == dstBounds) );

    // the srcRoD of the output should be enclosed in half the roi.
    // It does not have to be exactly half of the input.
    //    assert(dstRoD.x1*2 >= roi.x1 &&
    //           dstRoD.x2*2 <= roi.x2 &&
    //           dstRoD.y1*2 >= roi.y1 &&
    //           dstRoD.y2*2 <= roi.y2 &&
    //           dstRoD.width()*2 <= roi.width() &&
    //           dstRoD.height()*2 <= roi.height());
    assert( getComponents() == output->getComponents() );

    RectI dstRoI;
    RectI srcRoI = roi;
    srcRoI.clipIfOverlaps(srcBounds); // intersect srcRoI with the region of definition
#ifdef DEBUG_NAN
    assert(!checkForNaNsNoLock(srcRoI));
#endif
    dstRoI.x1 = (srcRoI.x1 + 1) / 2; // equivalent to ceil(srcRoI.x1/2.0)
    dstRoI.y1 = (srcRoI.y1 + 1) / 2; // equivalent to ceil(srcRoI.y1/2.0)
    dstRoI.x2 = srcRoI.x2 / 2; // equivalent to floor(srcRoI.x2/2.0)
    dstRoI.y2 = srcRoI.y2 / 2; // equivalent to floor(srcRoI.y2/2.0)


    const PIX* const srcPixels      = (const PIX*)pixelAt(srcBounds.x1,   srcBounds.y1);
    const char* const srcBmPixels   = _bitmap.getBitmapAt(srcBmBounds.x1, srcBmBounds.y1);
    PIX* const dstPixels          = (PIX*)output->pixelAt(dstBounds.x1,   dstBounds.y1);
    char* const dstBmPixels = output->_bitmap.getBitmapAt(dstBmBounds.x1, dstBmBounds.y1);
    int srcRowSize = srcBounds.width() * _nbComponents;
    int dstRowSize = dstBounds.width() * _nbComponents;

    // offset pointers so that srcData and dstData correspond to pixel (0,0)
    const PIX* const srcData = srcPixels - (srcBounds.x1 * _nbComponents + srcRowSize * srcBounds.y1);
    PIX* const dstData       = dstPixels - (dstBounds.x1 * _nbComponents + dstRowSize * dstBounds.y1);
    const int srcBmRowSize = srcBmBounds.width();
    const int dstBmRowSize = dstBmBounds.width();
    const char* const srcBmData = srcBmPixels - (srcBmBounds.x1 + srcBmRowSize * srcBmBounds.y1);
    char* const dstBmData       = dstBmPixels - (dstBmBounds.x1 + dstBmRowSize * dstBmBounds.y1);

    for (int y = dstRoI.y1; y < dstRoI.y2; ++y) {
        const PIX* const srcLineStart    = srcData + y * 2 * srcRowSize;
        PIX* const dstLineStart          = dstData + y     * dstRowSize;
        const char* const srcBmLineStart = srcBmData + y * 2 * srcBmRowSize;
        char* const dstBmLineStart       = dstBmData + y     * dstBmRowSize;

        // The current dst row, at y, covers the src rows y*2 (thisRow) and y*2+1 (nextRow).
        // Check that if are within srcBounds.
        int srcy = y * 2;
        bool pickThisRow = srcBounds.y1 <= (srcy + 0) && (srcy + 0) < srcBounds.y2;
        bool pickNextRow = srcBounds.y1 <= (srcy + 1) && (srcy + 1) < srcBounds.y2;
        int sumH = (int)pickNextRow + (int)pickThisRow;
        assert(sumH == 1 || sumH == 2);

        for (int x = dstRoI.x1; x < dstRoI.x2; ++x) {
            const PIX* const srcPixStart    = srcLineStart   + x * 2 * _nbComponents;
            const char* const srcBmPixStart = srcBmLineStart + x * 2;
            PIX* const dstPixStart          = dstLineStart   + x * _nbComponents;
            char* const dstBmPixStart       = dstBmLineStart + x;

            // The current dst col, at y, covers the src cols x*2 (thisCol) and x*2+1 (nextCol).
            // Check that if are within srcBounds.
            int srcx = x * 2;
            bool pickThisCol = srcBounds.x1 <= (srcx + 0) && (srcx + 0) < srcBounds.x2;
            bool pickNextCol = srcBounds.x1 <= (srcx + 1) && (srcx + 1) < srcBounds.x2;
            int sumW = (int)pickThisCol + (int)pickNextCol;
            assert(sumW == 1 || sumW == 2);
            const int sum = sumW * sumH;
            assert(0 < sum && sum <= 4);

            if (sum == 0) { // never happens
                for (int k = 0; k < _nbComponents; ++k) {
                    dstPixStart[k] = 0;
                }
                if (copyBitMap) {
                    dstBmPixStart[0] = 0;
                }
                continue;
            }

            for (int k = 0; k < _nbComponents; ++k) {
                ///a b
                ///c d

                const PIX a = (pickThisCol && pickThisRow) ? *(srcPixStart + k) : 0;
                const PIX b = (pickNextCol && pickThisRow) ? *(srcPixStart + k + _nbComponents) : 0;
                const PIX c = (pickThisCol && pickNextRow) ? *(srcPixStart + k + srcRowSize) : 0;
                const PIX d = (pickNextCol && pickNextRow) ? *(srcPixStart + k + srcRowSize  + _nbComponents)  : 0;
#ifdef DEBUG_NAN
                assert( !std::isnan(a) ); // check for NaN
                assert( !std::isnan(b) ); // check for NaN
                assert( !std::isnan(c) ); // check for NaN
                assert( !std::isnan(d) ); // check for NaN
#endif
                assert( sumW == 2 || ( sumW == 1 && ( (a == 0 && c == 0) || (b == 0 && d == 0) ) ) );
                assert( sumH == 2 || ( sumH == 1 && ( (a == 0 && b == 0) || (c == 0 && d == 0) ) ) );
                dstPixStart[k] = (a + b + c + d) / sum;
            }

            if (copyBitMap) {
                ///a b
                ///c d

                char a = (pickThisCol && pickThisRow) ? *(srcBmPixStart) : 0;
                char b = (pickNextCol && pickThisRow) ? *(srcBmPixStart + 1) : 0;
                char c = (pickThisCol && pickNextRow) ? *(srcBmPixStart + srcBmRowSize) : 0;
                char d = (pickNextCol && pickNextRow) ? *(srcBmPixStart + srcBmRowSize  + 1)  : 0;
#if NATRON_ENABLE_TRIMAP
                /*
                   The only correct solution is to convert pixels being rendered to 0 otherwise the caller
                   would have to wait for the original fullscale image render to be finished and then re-downscale again.
                 */
                if (a == PIXEL_UNAVAILABLE) {
                    a = 0;
                }
                if (b == PIXEL_UNAVAILABLE) {
                    b = 0;
                }
                if (c == PIXEL_UNAVAILABLE) {
                    c = 0;
                }
                if (d == PIXEL_UNAVAILABLE) {
                    d = 0;
                }
#endif
                assert( sumW == 2 || ( sumW == 1 && ( (a == 0 && c == 0) || (b == 0 && d == 0) ) ) );
                assert( sumH == 2 || ( sumH == 1 && ( (a == 0 && b == 0) || (c == 0 && d == 0) ) ) );
                assert(a + b + c + d <= sum); // bitmaps are 0 or 1
                // the following is an integer division, the result can be 0 or 1
                dstBmPixStart[0] = (a + b + c + d) / sum;
                assert(dstBmPixStart[0] == 0 || dstBmPixStart[0] == 1);
            }
        }
    }
} // halveRoIForDepth

// code proofread and fixed by @devernay on 8/8/2014
void
Image::halveRoI(const RectI & roi,
                bool copyBitMap,
                Image* output) const
{
    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        halveRoIForDepth<unsigned char, 255>(roi, copyBitMap,  output);
        break;
    case eImageBitDepthShort:
        halveRoIForDepth<unsigned short, 65535>(roi, copyBitMap, output);
        break;
    case eImageBitDepthHalf:
        assert(false);
        break;
    case eImageBitDepthFloat:
        halveRoIForDepth<float, 1>(roi, copyBitMap, output);
        break;
    case eImageBitDepthNone:
        break;
    }
}

// code proofread and fixed by @devernay on 8/8/2014
template <typename PIX, int maxValue>
void
Image::halve1DImageForDepth(const RectI & roi,
                            Image* output) const
{
    int width = roi.width();
    int height = roi.height();

    assert(width == 1 || height == 1); /// must be 1D
    assert( output->getComponents() == getComponents() );

    /// Take the lock for both bitmaps since we're about to read/write from them!
    QWriteLocker k1(&output->_entryLock);
    QReadLocker k2(&_entryLock);
    const RectI & srcBounds = _bounds;
    const RectI & dstBounds = output->_bounds;
//    assert(dstBounds.x1 * 2 == roi.x1 &&
//           dstBounds.y1 * 2 == roi.y1 &&
//           (
//               dstBounds.x2 * 2 == roi.x2 || // we halve in only 1 dimension
//               dstBounds.y2 * 2 == roi.y2)
//           );
    int halfWidth = width / 2;
    int halfHeight = height / 2;
    if (height == 1) { //1 row
        assert(width != 1); /// widthxheight can't be 1x1

        const PIX* src = (const PIX*)pixelAt(roi.x1, roi.y1);
        PIX* dst = (PIX*)output->pixelAt(dstBounds.x1, dstBounds.y1);
        assert(src && dst);
        for (int x = 0; x < halfWidth; ++x) {
            for (int k = 0; k < _nbComponents; ++k) {
#ifdef DEBUG_NAN
                assert( !std::isnan(*src) ); // check for NaN
                assert( !std::isnan(*(src + _nbComponents)) ); // check for NaN
#endif
                *dst = PIX( (float)( *src + *(src + _nbComponents) ) / 2. );
#ifdef DEBUG_NAN
                assert( !std::isnan(*dst) ); // check for NaN
#endif
                ++dst;
                ++src;
            }
            src += _nbComponents;
        }
    } else if (width == 1) {
        int rowSize = srcBounds.width() * _nbComponents;
        const PIX* src = (const PIX*)pixelAt(roi.x1, roi.y1);
        PIX* dst = (PIX*)output->pixelAt(dstBounds.x1, dstBounds.y1);
        assert(src && dst);
        for (int y = 0; y < halfHeight; ++y) {
            for (int k = 0; k < _nbComponents; ++k) {
#ifdef DEBUG_NAN
                assert( !std::isnan(*src) ); // check for NaN
                assert( !std::isnan(*(src + rowSize)) ); // check for NaN
#endif
                *dst = PIX( (float)( *src + (*src + rowSize) ) / 2. );
#ifdef DEBUG_NAN
                assert( !std::isnan(*dst) ); // check for NaN
#endif
                ++dst;
                ++src;
            }
            src += rowSize;
        }
    }
}

// code proofread and fixed by @devernay on 8/8/2014
void
Image::halve1DImage(const RectI & roi,
                    Image* output) const
{
    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        halve1DImageForDepth<unsigned char, 255>(roi, output);
        break;
    case eImageBitDepthShort:
        halve1DImageForDepth<unsigned short, 65535>(roi, output);
        break;
    case eImageBitDepthHalf:
        assert(false);
        break;
    case eImageBitDepthFloat:
        halve1DImageForDepth<float, 1>(roi, output);
        break;
    case eImageBitDepthNone:
        break;
    }
}

// code proofread and fixed by @devernay on 8/8/2014
void
Image::downscaleMipMap(const RectD& dstRod,
                       const RectI & roi,
                       unsigned int fromLevel,
                       unsigned int toLevel,
                       bool copyBitMap,
                       Image* output) const
{
    assert(getStorageMode() != eStorageModeGLTex);

    ///You should not call this function with a level equal to 0.
    assert(toLevel >  fromLevel);

    assert(_bounds.x1 <= roi.x1 && roi.x2 <= _bounds.x2 &&
           _bounds.y1 <= roi.y1 && roi.y2 <= _bounds.y2);
    double par = getPixelAspectRatio();
    unsigned int downscaleLvls = toLevel - fromLevel;

    assert( !copyBitMap || _bitmap.getBitmap() );

    RectI dstRoI  = roi.downscalePowerOfTwoSmallestEnclosing(downscaleLvls);
    ImagePtr tmpImg = std::make_shared<Image>( getComponents(), dstRod, dstRoI, toLevel, par, getBitDepth(), getPremultiplication(), getFieldingOrder(), true);

    buildMipMapLevel( dstRod, roi, downscaleLvls, copyBitMap, tmpImg.get() );

    // check that the downscaled mipmap is inside the output image (it may not be equal to it)
    assert(dstRoI.x1 >= output->_bounds.x1);
    assert(dstRoI.x2 <= output->_bounds.x2);
    assert(dstRoI.y1 >= output->_bounds.y1);
    assert(dstRoI.y2 <= output->_bounds.y2);

    ///Now copy the result of tmpImg into the output image
    output->pasteFrom(*tmpImg, dstRoI, copyBitMap);
}

bool
Image::checkForNaNsAndFix(const RectI& roi)
{
    if (getBitDepth() != eImageBitDepthFloat) {
        return false;
    }
    if (getStorageMode() == eStorageModeGLTex) {
        return false;
    }

    QWriteLocker k(&_entryLock);
    unsigned int compsCount = getComponentsCount();
    bool hasnan = false;
    for (int y = roi.y1; y < roi.y2; ++y) {
        float* pix = (float*)pixelAt(roi.x1, y);
        float* const end = pix +  compsCount * roi.width();

        for (; pix < end; ++pix) {
            // we remove NaNs, but infinity values should pose no problem
            // (if they do, please explain here which ones)
#ifdef DEBUG_NAN
            assert( !std::isnan(*pix) ); // check for NaN
#endif
            if ( std::isnan(*pix) ) { // check for NaN (std::isnan(x) is not slower than x != x and works with -Ofast)
                *pix = 1.;
                hasnan = true;
            }
        }
    }

    return hasnan;
}

bool
Image::checkForNaNsNoLock(const RectI& roi) const
{
    if (getBitDepth() != eImageBitDepthFloat) {
        return false;
    }
    if (getStorageMode() == eStorageModeGLTex) {
        return false;
    }

    //QWriteLocker k(&_entryLock);
    unsigned int compsCount = getComponentsCount();
    bool hasnan = false;
    for (int y = roi.y1; y < roi.y2; ++y) {
        float* pix = (float*)pixelAt(roi.x1, y);
        float* const end = pix +  compsCount * roi.width();

        for (; pix < end; ++pix) {
            // we remove NaNs, but infinity values should pose no problem
            // (if they do, please explain here which ones)
#ifdef DEBUG_NAN
            assert( !std::isnan(*pix) ); // check for NaN
#endif
            if ( std::isnan(*pix) ) { // check for NaN (std::isnan(x) is not slower than x != x and works with -Ofast)
                hasnan = true;
            }
        }
    }

    return hasnan;
}

// code proofread and fixed by @devernay on 8/8/2014
template <typename PIX, int maxValue>
void
Image::upscaleMipMapForDepth(const RectI & roi,
                             unsigned int fromLevel,
                             unsigned int toLevel,
                             Image* output) const
{
    assert( getBitDepth() == output->getBitDepth() );
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) || (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) || (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );

    ///You should not call this function with a level equal to 0.
    assert(fromLevel > toLevel);

    assert(roi.x1 <= _bounds.x1 && _bounds.x2 <= roi.x2 &&
           roi.y1 <= _bounds.y1 && _bounds.y2 <= roi.y2);

    const RectI & srcRoi = roi;
    // The source rectangle, intersected to this image region of definition in pixels and the output bounds.
    RectI dstRoi = roi.toNewMipMapLevel(fromLevel, toLevel, _par, getRoD());
    dstRoi.clipIfOverlaps(output->_bounds); //output may be a bit smaller than the upscaled RoI
    int scale = 1 << (fromLevel - toLevel);

    assert( output->getComponents() == getComponents() );

    if (_nbComponents == 0) {
        return;
    }

    QWriteLocker k1(&output->_entryLock);
    QReadLocker k2(&_entryLock);
    int srcRowSize = _bounds.width() * _nbComponents;
    int dstRowSize = output->_bounds.width() * _nbComponents;
    const PIX *src = (const PIX*)pixelAt(srcRoi.x1, srcRoi.y1);
    PIX* dst = (PIX*)output->pixelAt(dstRoi.x1, dstRoi.y1);
    assert(src && dst);

    // algorithm: fill the first line of output, and replicate it as many times as necessary
    // works even if dstRoi is not exactly a multiple of srcRoi (first/last column/line may not be complete)
    int yi = srcRoi.y1;
    int ycount; // how many lines should be filled
    for (int yo = dstRoi.y1; yo < dstRoi.y2; ++yi, src += srcRowSize, yo += ycount, dst += ycount * dstRowSize) {
        const PIX * const srcLineStart = src;
        PIX * const dstLineBatchStart = dst;
        ycount = scale - (yo - yi * scale); // how many lines should be filled
        ycount = std::min(ycount, dstRoi.y2 - yo);
        assert(0 < ycount && ycount <= scale);
        int xi = srcRoi.x1;
        int xcount = 0; // how many pixels should be filled
        const PIX * srcPix = srcLineStart;
        PIX * dstPixFirst = dstLineBatchStart;
        // fill the first line
        for (int xo = dstRoi.x1; xo < dstRoi.x2; ++xi, srcPix += _nbComponents, xo += xcount, dstPixFirst += xcount * _nbComponents) {
            xcount = scale - (xo - xi * scale);
            xcount = std::min(xcount, dstRoi.x2 - xo);
            //assert(0 < xcount && xcount <= scale);
            // replicate srcPix as many times as necessary
            PIX * dstPix = dstPixFirst;
            //assert((srcPix-(PIX*)pixelAt(srcRoi.x1, srcRoi.y1)) % components == 0);
            for (int i = 0; i < xcount; ++i, dstPix += _nbComponents) {
                assert( ( dstPix - (PIX*)output->pixelAt(dstRoi.x1, dstRoi.y1) ) % _nbComponents == 0 );
                assert(dstPix >= (PIX*)output->pixelAt(xo, yo) && dstPix < (PIX*)output->pixelAt(xo, yo) + xcount * _nbComponents);
                for (int c = 0; c < _nbComponents; ++c) {
#ifdef DEBUG_NAN
                    assert( !std::isnan(srcPix[c]) ); // check for NaN
#endif
                    dstPix[c] = srcPix[c];
                }
            }
            //assert(dstPix == dstPixFirst + xcount*components);
        }
        assert(dstLineBatchStart == (PIX*)output->pixelAt(dstRoi.x1, yo));
        PIX * dstLineStart = dstLineBatchStart + dstRowSize; // first line was filled already
        // now replicate the line as many times as necessary
        for (int i = 1; i < ycount; ++i, dstLineStart += dstRowSize) {
            assert(dstLineStart == (PIX*)output->pixelAt(dstRoi.x1, yo+i));
            assert(dstLineStart + dstRowSize == (PIX*)output->pixelAt(dstRoi.x2 - 1, yo+i) + _nbComponents);
            std::copy(dstLineBatchStart, dstLineBatchStart + dstRowSize, dstLineStart);
        }
    }
} // upscaleMipMapForDepth

// code proofread and fixed by @devernay on 8/8/2014
void
Image::upscaleMipMap(const RectI & roi,
                     unsigned int fromLevel,
                     unsigned int toLevel,
                     Image* output) const
{
    assert(getStorageMode() != eStorageModeGLTex);

    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        upscaleMipMapForDepth<unsigned char, 255>(roi, fromLevel, toLevel, output);
        break;
    case eImageBitDepthShort:
        upscaleMipMapForDepth<unsigned short, 65535>(roi, fromLevel, toLevel, output);
        break;
    case eImageBitDepthHalf:
        assert(false);
        break;
    case eImageBitDepthFloat:
        upscaleMipMapForDepth<float, 1>(roi, fromLevel, toLevel, output);
        break;
    case eImageBitDepthNone:
        break;
    }
}

// code proofread and fixed by @devernay on 8/8/2014
void
Image::buildMipMapLevel(const RectD& dstRoD,
                        const RectI & roi,
                        unsigned int level,
                        bool copyBitMap,
                        Image* output) const
{
    ///The last mip map level we will make with closestPo2
    RectI lastLevelRoI = roi.downscalePowerOfTwoSmallestEnclosing(level);

    ///The output image must contain the last level roi
    assert( output->getBounds().contains(lastLevelRoI) );

    assert( output->getComponents() == getComponents() );

    if (level == 0) {
        ///Just copy the roi and return
        output->pasteFrom(*this, roi, copyBitMap);

        return;
    }

    const Image* srcImg = this;
    Image* dstImg = NULL;
    bool mustFreeSrc = false;
    RectI previousRoI = roi;
    ///Build all the mipmap levels until we reach the one we are interested in
    for (unsigned int i = 1; i <= level; ++i) {
        ///Halve the smallest enclosing po2 rect as we need to render a minimum of the renderWindow
        RectI halvedRoI = previousRoI.downscalePowerOfTwoSmallestEnclosing(1);

        ///Allocate an image with half the size of the source image
        dstImg = new Image( getComponents(), dstRoD, halvedRoI, getMipMapLevel() + i, getPixelAspectRatio(), getBitDepth(), getPremultiplication(), getFieldingOrder(), true);

        ///Half the source image into dstImg.
        ///We pass the closestPo2 roi which might not be the entire size of the source image
        ///If the source image'sroi was originally a po2.
        srcImg->halveRoI(previousRoI, copyBitMap, dstImg);

        ///Clean-up, we should use shared_ptrs for safety
        if (mustFreeSrc) {
            delete srcImg;
        }

        ///Switch for next pass
        previousRoI = halvedRoI;
        srcImg = dstImg;
        mustFreeSrc = true;
    }

    assert(srcImg->getBounds() == lastLevelRoI);

    ///Finally copy the last mipmap level into output.
    output->pasteFrom( *srcImg, srcImg->getBounds(), copyBitMap);

    ///Clean-up, we should use shared_ptrs for safety
    if (mustFreeSrc) {
        delete srcImg;
    }
} // buildMipMapLevel

#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif
unsigned int
Image::getLevelFromScale(double s)
{
    assert(0. < s && s <= 1.);
    int retval = -std::floor(std::log(s) / M_LN2 + 0.5);
    assert(retval >= 0);

    return retval;
}

void
Image::copyBitmapRowPortion(int x1,
                            int x2,
                            int y,
                            const Image& other)
{
    _bitmap.copyRowPortion(x1, x2, y, other._bitmap);
}

void
Bitmap::copyRowPortion(int x1,
                       int x2,
                       int y,
                       const Bitmap& other)
{
    const char* srcBitmap = other.getBitmapAt(x1, y);
    char* dstBitmap = getBitmapAt(x1, y);
    const char* end = dstBitmap + (x2 - x1);

    assert(srcBitmap && dstBitmap);
    while (dstBitmap < end) {
        *dstBitmap = /**srcBitmap == PIXEL_UNAVAILABLE ? 0 : */ *srcBitmap;
        ++dstBitmap;
        ++srcBitmap;
    }
}

void
Image::copyBitmapPortion(const RectI& roi,
                         const Image& other)
{
    _bitmap.copyBitmapPortion(roi, other._bitmap);
}

void
Bitmap::copyBitmapPortion(const RectI& roi,
                          const Bitmap& other)
{
    assert(roi.x1 >= _bounds.x1 && roi.x2 <= _bounds.x2 && roi.y1 >= _bounds.y1 && roi.y2 <= _bounds.y2);
    assert(roi.x1 >= other._bounds.x1 && roi.x2 <= other._bounds.x2 && roi.y1 >= other._bounds.y1 && roi.y2 <= other._bounds.y2);

    int srcRowSize = other._bounds.width();
    int dstRowSize = _bounds.width();
    const char* srcBitmap = other.getBitmapAt(roi.x1, roi.y1);
    char* dstBitmap = getBitmapAt(roi.x1, roi.y1);

    for (int y = roi.y1; y < roi.y2; ++y,
         srcBitmap += srcRowSize,
         dstBitmap += dstRowSize) {
        const char* srcCur = srcBitmap;
        const char* srcEnd = srcBitmap + roi.width();
        char* dstCur = dstBitmap;
        while (srcCur < srcEnd) {
            *dstCur = /**srcCur == PIXEL_UNAVAILABLE ? 0 : */ *srcCur;
            ++srcCur;
            ++dstCur;
        }
    }
}

template <typename PIX, bool doPremult>
void
Image::premultInternal(const RectI& roi)
{
    WriteAccess acc(this);
    const RectI renderWindow = roi.intersect(_bounds);

    assert(getComponentsCount() == 4);

    int srcRowElements = 4 * _bounds.width();
    PIX* dstPix = (PIX*)acc.pixelAt(renderWindow.x1, renderWindow.y1);
    for ( int y = renderWindow.y1; y < renderWindow.y2; ++y, dstPix += (srcRowElements - (renderWindow.x2 - renderWindow.x1) * 4) ) {
        for (int x = renderWindow.x1; x < renderWindow.x2; ++x, dstPix += 4) {
#ifdef DEBUG_NAN
            assert( !std::isnan(dstPix[3]) ); // check for NaN
#endif
            for (int c = 0; c < 3; ++c) {
#ifdef DEBUG_NAN
                assert( !std::isnan(dstPix[c]) ); // check for NaN
#endif
                if (doPremult) {
                    dstPix[c] = PIX(float(dstPix[c]) * dstPix[3]);
                } else {
                    if (dstPix[3] != 0) {
                        dstPix[c] = PIX( dstPix[c] / float(dstPix[3]) );
                    }
                }
#ifdef DEBUG_NAN
                assert( !std::isnan(dstPix[c]) ); // check for NaN
#endif
            }
        }
    }
}

template <bool doPremult>
void
Image::premultForDepth(const RectI& roi)
{
    if (getComponentsCount() != 4) {
        return;
    }
    ImageBitDepthEnum depth = getBitDepth();
    switch (depth) {
    case eImageBitDepthByte:
        premultInternal<unsigned char, doPremult>(roi);
        break;
    case eImageBitDepthShort:
        premultInternal<unsigned short, doPremult>(roi);
        break;
    case eImageBitDepthFloat:
        premultInternal<float, doPremult>(roi);
        break;
    default:
        break;
    }
}

void
Image::premultImage(const RectI& roi)
{
    assert(getStorageMode() != eStorageModeGLTex);
    premultForDepth<true>(roi);
}

void
Image::unpremultImage(const RectI& roi)
{
    assert(getStorageMode() != eStorageModeGLTex);
    premultForDepth<false>(roi);
}

NATRON_NAMESPACE_EXIT
