/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <QDebug>

#include "Engine/AppManager.h"

using namespace Natron;

#define BM_GET(i,j) &_map[( i - _bounds.bottom() ) * _bounds.width() + ( j - _bounds.left() )]

#define PIXEL_UNAVAILABLE 2

template <int trimap>
RectI minimalNonMarkedBbox_internal(const RectI& roi, const RectI& _bounds,const std::vector<char>& _map,
                                    bool* isBeingRenderedElsewhere)
{
    RectI bbox;
    assert(_bounds.contains(roi));
    bbox = roi;
    
    //find bottom
    for (int i = bbox.bottom(); i < bbox.top(); ++i) {
        const char* buf = BM_GET(i, bbox.left());
        
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
                if (!*buf || *buf == PIXEL_UNAVAILABLE) {
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
        const char* buf = BM_GET(i, bbox.left());
        
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
                if (!*buf || *buf == PIXEL_UNAVAILABLE) {
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

        for (int i = bbox.bottom(); i < bbox.top(); ++i, pix += _bounds.width()) {
            if (!*pix) {
                pix = 0;
                break;
            }
            
            else if (*pix == PIXEL_UNAVAILABLE) {
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

        for (int i = bbox.bottom(); i < bbox.top(); ++i, pix += _bounds.width()) {
            if (!*pix) {
                pix = 0;
                break;
            }

            else if (*pix == PIXEL_UNAVAILABLE) {
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

}


template <int trimap>
void
minimalNonMarkedRects_internal(const RectI & roi,const RectI& _bounds, const std::vector<char>& _map,
                               std::list<RectI>& ret,bool* isBeingRenderedElsewhere)
{
    ///Any out of bounds portion is pushed to the rectangles to render
    RectI intersection;
    roi.intersect(_bounds, &intersection);
    if (roi != intersection) {
        
        if (_bounds.x1 > roi.x1 && _bounds.y2 > _bounds.y1) {
            RectI left(roi.x1,_bounds.y1,_bounds.x1,_bounds.y2);
            ret.push_back(left);
        }
        
        
        if (roi.x2 > roi.x1 && _bounds.y1 > roi.y1) {
            RectI btm(roi.x1,roi.y1,roi.x2,_bounds.y1);
            ret.push_back(btm);
        }
        
        
        if (roi.x2 > _bounds.x2 && _bounds.y2 > _bounds.y1) {
            RectI right(_bounds.x2,_bounds.y1,roi.x2,_bounds.y2);
            ret.push_back(right);
        }
        
        
        if (roi.x2 > roi.x1 && roi.y2 > _bounds.y2) {
            RectI top(roi.x1,_bounds.y2,roi.x2,roi.y2);
            ret.push_back(top);
        }
    }
    
    if (intersection.isNull()) {
        return;
    }
    
    RectI bboxM = minimalNonMarkedBbox_internal<trimap>(intersection, _bounds, _map, isBeingRenderedElsewhere);
    assert((trimap && isBeingRenderedElsewhere) || (!trimap && !isBeingRenderedElsewhere));
    
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
        const char* buf = BM_GET(i, bboxX.left());
        if (trimap) {
            const char* lineEnd = buf + bboxX.width();
            bool metUnavailablePixel = false;
            while (buf < lineEnd) {
                if (*buf == 1) {
                    buf = 0;
                    break;
                } else if (*buf == PIXEL_UNAVAILABLE && trimap) {
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
        const char* buf = BM_GET(i, bboxX.left());
        
        if (trimap) {
            const char* lineEnd = buf + bboxX.width();
            bool metUnavailablePixel = false;
            while (buf < lineEnd) {
                if (*buf == 1) {
                    buf = 0;
                    break;
                } else if (*buf == PIXEL_UNAVAILABLE && trimap) {
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
	if (bboxX.bottom() < bboxX.top()) {
		for (int j = bboxX.left(); j < bboxX.right(); ++j) {
			const char* pix = BM_GET(bboxX.bottom(), j);

			bool metUnavailablePixel = false;

			for (int i = bboxX.bottom(); i < bboxX.top(); ++i, pix += _bounds.width()) {
				if (*pix == 1) {
					pix = 0;
					break;
				} else if (trimap && *pix == PIXEL_UNAVAILABLE) {
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
	if (bboxX.bottom() < bboxX.top()) {
		for (int j = bboxX.right() - 1; j >= bboxX.left(); --j) {
			const char* pix = BM_GET(bboxX.bottom(), j);

			bool metUnavailablePixel = false;

			for (int i = bboxX.bottom(); i < bboxX.top(); ++i, pix += _bounds.width()) {
				if (*pix == 1) {
					pix = 0;
					break;
				} else if (trimap && *pix == PIXEL_UNAVAILABLE) {
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
    bboxX = minimalNonMarkedBbox_internal<trimap>(bboxX,_bounds,_map,isBeingRenderedElsewhere);
    
    if ( !bboxX.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxX);
    }
    
#endif // NATRON_BITMAP_DISABLE_OPTIMIZATION

} // minimalNonMarkedRects

RectI
Bitmap::minimalNonMarkedBbox(const RectI & roi) const
{
    if (_dirtyZoneSet) {
        RectI realRoi;
        if (!roi.intersect(_dirtyZone, &realRoi)) {
            return RectI();
        }
        return minimalNonMarkedBbox_internal<0>(realRoi, _bounds, _map, NULL);
    } else {
        return minimalNonMarkedBbox_internal<0>(roi, _bounds, _map, NULL);
    }
}

void
Bitmap::minimalNonMarkedRects(const RectI & roi,std::list<RectI>& ret) const
{
    if (_dirtyZoneSet) {
        RectI realRoi;
        if (!roi.intersect(_dirtyZone, &realRoi)) {
            return;
        }
        minimalNonMarkedRects_internal<0>(realRoi, _bounds, _map,ret , NULL);
    } else {
        minimalNonMarkedRects_internal<0>(roi, _bounds, _map,ret , NULL);
    }
}

#if NATRON_ENABLE_TRIMAP
RectI
Bitmap::minimalNonMarkedBbox_trimap(const RectI & roi,bool* isBeingRenderedElsewhere) const
{
    if (_dirtyZoneSet) {
        RectI realRoi;
        if (!roi.intersect(_dirtyZone, &realRoi)) {
            *isBeingRenderedElsewhere = false;
            return RectI();
        }
        return minimalNonMarkedBbox_internal<1>(realRoi, _bounds, _map, isBeingRenderedElsewhere);
    } else {
        return minimalNonMarkedBbox_internal<1>(roi, _bounds, _map, isBeingRenderedElsewhere);
    }
}


void
Bitmap::minimalNonMarkedRects_trimap(const RectI & roi,std::list<RectI>& ret,bool* isBeingRenderedElsewhere) const
{
    if (_dirtyZoneSet) {
        RectI realRoi;
        if (!roi.intersect(_dirtyZone, &realRoi)) {
            *isBeingRenderedElsewhere = false;
            return;
        }
        minimalNonMarkedRects_internal<1>(realRoi, _bounds, _map ,ret , isBeingRenderedElsewhere);
    } else {
        minimalNonMarkedRects_internal<1>(roi, _bounds, _map ,ret , isBeingRenderedElsewhere);
    }
} 
#endif

void
Natron::Bitmap::markForRendered(const RectI & roi)
{
    char* buf = BM_GET(roi.bottom(), roi.left());
    int w = _bounds.width();
    int roiw = roi.width();
    for (int i = roi.y1; i < roi.y2; ++i, buf += w) {
        memset( buf, 1, roiw);
    }
}

#if NATRON_ENABLE_TRIMAP
void
Natron::Bitmap::markForRendering(const RectI & roi)
{
    char* buf = BM_GET(roi.bottom(), roi.left());
    int w = _bounds.width();
    int roiw = roi.width();
    for (int i = roi.y1; i < roi.y2; ++i, buf += w) {
        memset( buf, PIXEL_UNAVAILABLE , roiw );
    }
}
#endif

void
Natron::Bitmap::clear(const RectI& roi)
{
    char* buf = BM_GET(roi.bottom(), roi.left());
    int w = _bounds.width();
    int roiw = roi.width();
    for (int i = roi.y1; i < roi.y2; ++i, buf += w) {
        memset( buf, 0 , roiw );
    }
}

void
Natron::Bitmap::swap(Bitmap& other)
{
    _map.swap(other._map);
    _bounds = other._bounds;
    _dirtyZone.clear();//merge(other._dirtyZone);
    _dirtyZoneSet = false;
}

const char*
Natron::Bitmap::getBitmapAt(int x,
                            int y) const
{
    if ( ( x >= _bounds.left() ) && ( x < _bounds.right() ) && ( y >= _bounds.bottom() ) && ( y < _bounds.top() ) ) {
        return BM_GET(y,x);
    } else {
        return NULL;
    }
}

char*
Natron::Bitmap::getBitmapAt(int x,
                            int y)
{
    if ( ( x >= _bounds.left() ) && ( x < _bounds.right() ) && ( y >= _bounds.bottom() ) && ( y < _bounds.top() ) ) {
        return BM_GET(y,x);
    } else {
        return NULL;
    }
}

Image::Image(const ImageKey & key,
             const boost::shared_ptr<Natron::ImageParams>& params,
             const Natron::CacheAPI* cache,
             Natron::StorageModeEnum storage,
             const std::string & path)
    : CacheEntryHelper<unsigned char, ImageKey,ImageParams>(key, params, cache,storage,path)
    , _useBitmap(true)
{
    _bitDepth = params->getBitDepth();
    _rod = params->getRoD();
    _bounds = params->getBounds();
    _par = params->getPixelAspectRatio();
    
}

Image::Image(const ImageKey & key,
             const boost::shared_ptr<Natron::ImageParams>& params)
: CacheEntryHelper<unsigned char, ImageKey,ImageParams>(key, params, NULL,Natron::eStorageModeRAM,std::string())
, _useBitmap(false)
{
    _bitDepth = params->getBitDepth();
    _rod = params->getRoD();
    _bounds = params->getBounds();
    _par = params->getPixelAspectRatio();
    
    allocateMemory();
}


/*This constructor can be used to allocate a local Image. The deallocation should
   then be handled by the user. Note that no view number is passed in parameter
   as it is not needed.*/
Image::Image(const ImageComponents& components,
             const RectD & regionOfDefinition, //!< rod in canonical coordinates
             const RectI & bounds, //!< bounds in pixel coordinates
             unsigned int mipMapLevel,
             double par,
             Natron::ImageBitDepthEnum bitdepth,
             bool useBitmap)
    : CacheEntryHelper<unsigned char,ImageKey,ImageParams>()
    , _useBitmap(useBitmap)
{
    
    setCacheEntry(makeKey(0,false,0,0, false),
                  boost::shared_ptr<ImageParams>( new ImageParams( mipMapLevel,
                                                                   regionOfDefinition,
                                                                   par,
                                                                   mipMapLevel,
                                                                   bounds,
                                                                   bitdepth,
                                                                   false,
                                                                   components,
                                                                   std::map<int, std::map<int,std::vector<RangeD> > >() ) ),
                  NULL,
                  Natron::eStorageModeRAM,
                  std::string()
                  );

    _bitDepth = bitdepth;
    _rod = regionOfDefinition;
    _bounds = _params->getBounds();
    _par = par;
    
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
    QMutexLocker k(&_entryLock);
    _bitmap.setDirtyZone(zone);
}

ImageKey  
Image::makeKey(U64 nodeHashKey,
               bool frameVaryingOrAnimated,
               double time,
               int view,
               bool draftMode)
{
    return ImageKey(nodeHashKey,frameVaryingOrAnimated,time,view, 1., draftMode);
}

boost::shared_ptr<ImageParams>
Image::makeParams(int cost,
                  const RectD & rod,
                  const double par,
                  unsigned int mipMapLevel,
                  bool isRoDProjectFormat,
                  const ImageComponents& components,
                  Natron::ImageBitDepthEnum bitdepth,
                  const std::map<int, std::map<int,std::vector<RangeD> > > & framesNeeded)
{
    RectI bounds;

    rod.toPixelEnclosing(mipMapLevel, par, &bounds);

    return boost::shared_ptr<ImageParams>( new ImageParams(cost,
                                                           rod,
                                                           par,
                                                           mipMapLevel,
                                                           bounds,
                                                           bitdepth,
                                                           isRoDProjectFormat,
                                                           components,
                                                           framesNeeded) );
}

boost::shared_ptr<ImageParams>
Image::makeParams(int cost,
                  const RectD & rod,    // the image rod in canonical coordinates
                  const RectI& bounds,
                  const double par,
                  unsigned int mipMapLevel,
                  bool isRoDProjectFormat,
                  const ImageComponents& components,
                  Natron::ImageBitDepthEnum bitdepth,
                  const std::map<int, std::map<int,std::vector<RangeD> > > & framesNeeded)
{
#ifdef DEBUG
    RectI pixelRod;
    rod.toPixelEnclosing(mipMapLevel, par, &pixelRod);
    assert(bounds.left() >= pixelRod.left() && bounds.right() <= pixelRod.right() &&
           bounds.bottom() >= pixelRod.bottom() && bounds.top() <= pixelRod.top());
#endif
    
    return boost::shared_ptr<ImageParams>( new ImageParams(cost,
                                                           rod,
                                                           par,
                                                           mipMapLevel,
                                                           bounds,
                                                           bitdepth,
                                                           isRoDProjectFormat,
                                                           components,
                                                           framesNeeded) );
}



// code proofread and fixed by @devernay on 8/8/2014
template<typename PIX>
void
Image::pasteFromForDepth(const Natron::Image & srcImg,
                         const RectI & srcRoi,
                         bool copyBitmap,
                         bool takeSrcLock)
{
    ///Cannot copy images with different bit depth, this is not the purpose of this function.
    ///@see convert
    assert( getBitDepth() == srcImg.getBitDepth() );
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) || (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) || (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );
    // NOTE: before removing the following asserts, please explain why an empty image may happen
    
    QMutexLocker k(&_entryLock);
    
    boost::shared_ptr<QMutexLocker> k2;
    if (takeSrcLock) {
        k2.reset(new QMutexLocker(&srcImg._entryLock));
    }
    
    const RectI & bounds = _bounds;
    const RectI & srcBounds = srcImg._bounds;
    
    assert( !bounds.isNull() );
    assert( !srcBounds.isNull() );
    
    // only copy the intersection of roi, bounds and otherBounds
    RectI roi = srcRoi;
    bool doInteresect = roi.intersect(bounds, &roi);
    if (!doInteresect) {
        // no intersection between roi and the bounds of this image
        return;
    }
    doInteresect = roi.intersect(srcBounds, &roi);
    if (!doInteresect) {
        // no intersection between roi and the bounds of the other image
        return;
    }
    
    assert( getComponents() == srcImg.getComponents() );

    int components = getComponents().getNumComponents();
    if (copyBitmap) {
        copyBitmapPortion(roi, srcImg);
    }
    // now we're safe: both images contain the area in roi
    
    int srcRowElements = components * srcBounds.width();
    int dstRowElements = components * bounds.width();
    
    const PIX* src = (const PIX*)srcImg.pixelAt(roi.x1, roi.y1);
    PIX* dst = (PIX*)pixelAt(roi.x1, roi.y1);
    
    assert(src && dst);
    
    for (int y = roi.y1; y < roi.y2;
         ++y,
         src += srcRowElements,
         dst += dstRowElements) {
        memcpy(dst, src, roi.width() * sizeof(PIX) * components);
    }
}

void
Image::setRoD(const RectD& rod)
{
    QMutexLocker k(&_entryLock);
    _rod = rod;
    _params->setRoD(rod);
}

bool
Image::ensureBounds(const RectI& newBounds, bool fillWithBlackAndTransparant, bool setBitmapTo1)
{
    
    
    if (getBounds().contains(newBounds)) {
        return false;
    }
    
    QMutexLocker k(&_entryLock);
    
    RectI merge = newBounds;
    merge.merge(_bounds);
    
    
    
    ///Copy to a temp buffer of the good size
    boost::scoped_ptr<Image> tmpImg(new Image(getComponents(),
                                              getRoD(),
                                              merge,
                                              getMipMapLevel(),
                                              getPixelAspectRatio(),
                                              getBitDepth(),
                                              usesBitMap()));
    Natron::ImageBitDepthEnum depth = getBitDepth();

    if (fillWithBlackAndTransparant) {
        
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
        aRect.y1 = _bounds.y2;
        aRect.y2 = merge.y2;
        aRect.x2 = merge.x2;
        
        RectI bRect;
        bRect.x1 = _bounds.x2;
        bRect.y1 = _bounds.y1;
        bRect.x2 = merge.x2;
        bRect.y2 = _bounds.y2;
        
        RectI cRect;
        cRect.x1 = merge.x1;
        cRect.y1 = merge.y1;
        cRect.x2 = merge.x2;
        cRect.y2 = _bounds.y1;
        
        RectI dRect;
        dRect.x1 = merge.x1;
        dRect.y1 = _bounds.y1;
        dRect.x2 = _bounds.x1;
        dRect.y2 = _bounds.y2;
        
        WriteAccess wacc(tmpImg.get());
        std::size_t pixelSize = getComponentsCount() * getSizeOfForBitDepth(depth);
        
        if (!aRect.isNull()) {
            char* pix = (char*)tmpImg->pixelAt(aRect.x1, aRect.y1);
            assert(pix);
            double a = aRect.area();
            std::size_t memsize = a * pixelSize;
            memset(pix, 0, memsize);
            if (setBitmapTo1 && tmpImg->usesBitMap()) {
                char* bm = (char*)tmpImg->getBitmapAt(aRect.x1, aRect.y1);
                assert(bm);
                memset(bm, 1, a);
            }
        }
        if (!cRect.isNull()) {
            char* pix = (char*)tmpImg->pixelAt(cRect.x1, cRect.y1);
            assert(pix);
            double a = cRect.area();
            std::size_t memsize = a * pixelSize;
            memset(pix, 0, memsize);
            if (setBitmapTo1 && tmpImg->usesBitMap()) {
                char* bm = (char*)tmpImg->getBitmapAt(cRect.x1, cRect.y1);
                assert(bm);
                memset(bm, 1, a);
            }
        }
        if (!bRect.isNull()) {
            char* pix = (char*)tmpImg->pixelAt(bRect.x1, bRect.y1);
            assert(pix);
            int mw = merge.width();
            std::size_t rowsize = mw * pixelSize;
            
            int bw = bRect.width();
            std::size_t rectRowSize = bw * pixelSize;
            
            char* bm = (setBitmapTo1 && tmpImg->usesBitMap()) ? tmpImg->getBitmapAt(bRect.x1, bRect.y1) : 0;
            for (int y = bRect.y1; y < bRect.y2; ++y, pix += rowsize) {
                memset(pix, 0, rectRowSize);
                if (bm) {
                    memset(bm, 1, bw);
                    bm += mw;
                }
            }
        }
        if (!dRect.isNull()) {
            char* pix = (char*)tmpImg->pixelAt(dRect.x1, dRect.y1);
            assert(pix);
            int mw = merge.width();
            std::size_t rowsize = mw * pixelSize;
            
            int dw = dRect.width();
            std::size_t rectRowSize = dw * pixelSize;
            
            char* bm = (setBitmapTo1 && tmpImg->usesBitMap()) ? tmpImg->getBitmapAt(dRect.x1, dRect.y1) : 0;
            for (int y = dRect.y1; y < dRect.y2; ++y, pix += rowsize) {
                memset(pix, 0, rectRowSize);
                if (bm) {
                    memset(bm, 1, dw);
                    bm += mw;
                }
            }
        }
        
        
    } // fillWithBlackAndTransparant
    
    
    switch (depth) {
        case eImageBitDepthByte:
            tmpImg->pasteFromForDepth<unsigned char>(*this, _bounds, usesBitMap(), false);
            break;
        case eImageBitDepthShort:
            tmpImg->pasteFromForDepth<unsigned short>(*this, _bounds, usesBitMap(), false);
            break;
        case eImageBitDepthFloat:
            tmpImg->pasteFromForDepth<float>(*this, _bounds, usesBitMap(), false);
            break;
        case eImageBitDepthNone:
            break;
    }
      

    ///Change the size of the current buffer
    _bounds = merge;
    _params->setBounds(merge);
    assert(_bounds.contains(newBounds));
    swapBuffer(*tmpImg);
    if (usesBitMap()) {
        _bitmap.swap(tmpImg->_bitmap);
    }
    return true;
}
    

// code proofread and fixed by @devernay on 8/8/2014
void
Image::pasteFrom(const Natron::Image & src,
                 const RectI & srcRoi,
                 bool copyBitmap)
{
    
    
    Natron::ImageBitDepthEnum depth = getBitDepth();

    switch (depth) {
    case eImageBitDepthByte:
        pasteFromForDepth<unsigned char>(src, srcRoi, copyBitmap, true);
        break;
    case eImageBitDepthShort:
        pasteFromForDepth<unsigned short>(src, srcRoi, copyBitmap, true);
        break;
    case eImageBitDepthFloat:
        pasteFromForDepth<float>(src, srcRoi, copyBitmap, true);
        break;
    case eImageBitDepthNone:
        break;
    }
}

template <typename PIX,int maxValue, int nComps>
void
Image::fillForDepthForComponents(const RectI & roi_,
                                 float r,
                                 float g,
                                 float b,
                                 float a)
{
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) || (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) || (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );
    
    RectI roi = roi_;
    bool doInteresect = roi.intersect(_bounds, &roi);
    if (!doInteresect) {
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
    

    const ImageComponents& comps = getComponents();
    int nComps = comps.getNumComponents();
    switch (nComps) {
        case 0:
            return;
            break;
        case 1:
            fillForDepthForComponents<PIX, maxValue, 1>(roi_, r, g , b, a);
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
            float a)
{
    
    QMutexLocker k(&_entryLock);
    
    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        fillForDepth<unsigned char, 255>(roi, r, g, b, a);
        break;
    case eImageBitDepthShort:
        fillForDepth<unsigned short, 65535>(roi, r, g, b, a);
        break;
    case eImageBitDepthFloat:
        fillForDepth<float, 1>(roi, r, g, b, a);
        break;
    case eImageBitDepthNone:
        break;
    }
}

void
Image::fillZero(const RectI& roi)
{
    QMutexLocker k(&_entryLock);
    RectI intersection;
    if (!roi.intersect(_bounds, &intersection)) {
        return;
    }
    std::size_t rowSize =  getComponents().getNumComponents();
    switch ( getBitDepth() ) {
        case eImageBitDepthByte:
            rowSize *= sizeof(unsigned char);
            break;
        case eImageBitDepthShort:
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
        memset(dstPixels, 0, roiMemSize);
    }
}

void
Image::fillBoundsZero()
{
    QMutexLocker k(&_entryLock);
    
    std::size_t rowSize =  getComponents().getNumComponents();
    switch ( getBitDepth() ) {
        case eImageBitDepthByte:
            rowSize *= sizeof(unsigned char);
            break;
        case eImageBitDepthShort:
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
    memset(dstPixels, 0, roiMemSize);
}

unsigned char*
Image::pixelAt(int x,
               int y)
{
    int compsCount = getComponents().getNumComponents();

    if ( ( x < _bounds.left() ) || ( x >= _bounds.right() ) || ( y < _bounds.bottom() ) || ( y >= _bounds.top() )) {
        return NULL;
    } else {
        int compDataSize = getSizeOfForBitDepth( getBitDepth() ) * compsCount;
        
        unsigned char* ret =  (unsigned char*)this->_data.writable();
        if (!ret) {
            return 0;
        }
        ret = ret + (qint64)( y - _bounds.bottom() ) * compDataSize * _bounds.width()
        + (qint64)( x - _bounds.left() ) * compDataSize;
        return ret;
    }
}

const unsigned char*
Image::pixelAt(int x,
               int y) const
{
    int compsCount = getComponents().getNumComponents();
    
    if ( ( x < _bounds.left() ) || ( x >= _bounds.right() ) || ( y < _bounds.bottom() ) || ( y >= _bounds.top() )) {
        return NULL;
    } else {
        int compDataSize = getSizeOfForBitDepth( getBitDepth() ) * compsCount;
        
        unsigned char* ret = (unsigned char*)this->_data.readable();
        if (!ret) {
            return 0;
        }
        ret = ret + (qint64)( y - _bounds.bottom() ) * compDataSize * _bounds.width()
        + (qint64)( x - _bounds.left() ) * compDataSize;
        return ret;
    }
}

unsigned int
Image::getComponentsCount() const
{
    return getComponents().getNumComponents();
}

bool
Image::hasEnoughDataToConvert(Natron::ImageComponentsEnum from,
                              Natron::ImageComponentsEnum to)
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
Image::getFormatString(const Natron::ImageComponents& comps,
                       Natron::ImageBitDepthEnum depth)
{
    std::string s = comps.getLayerName() + '.' + comps.getComponentsGlobalName();
    s.append( getDepthString(depth) );

    return s;
}

std::string
Image::getDepthString(Natron::ImageBitDepthEnum depth)
{
    std::string s;

    switch (depth) {
    case Natron::eImageBitDepthByte:
        s += "8u";
        break;
    case Natron::eImageBitDepthShort:
        s += "16u";
        break;
    case Natron::eImageBitDepthFloat:
        s += "32f";
        break;
    case Natron::eImageBitDepthNone:
        break;
    }

    return s;
}


bool
Image::isBitDepthConversionLossy(Natron::ImageBitDepthEnum from,
                                 Natron::ImageBitDepthEnum to)
{
    int sizeOfFrom = getSizeOfForBitDepth(from);
    int sizeOfTo = getSizeOfForBitDepth(to);

    return sizeOfTo < sizeOfFrom;
}

unsigned int
Image::getRowElements() const
{
    QMutexLocker k(&_entryLock);
    return getComponentsCount() * _bounds.width();
}

// code proofread and fixed by @devernay on 4/12/2014
template <typename PIX, int maxValue>
void
Image::halveRoIForDepth(const RectI & roi,
                        bool copyBitMap,
                        Natron::Image* output) const
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
    QMutexLocker k1(&output->_entryLock);
    QMutexLocker k2(&_entryLock);

    ///The source rectangle, intersected to this image region of definition in pixels
    const RectI &srcBounds = _bounds;
    const RectI &dstBounds = output->_bounds;
    const RectI &srcBmBounds = _bitmap.getBounds();
    const RectI &dstBmBounds = output->_bitmap.getBounds();
    assert(!copyBitMap || usesBitMap());
    assert(!usesBitMap() ||(srcBmBounds == srcBounds && dstBmBounds == dstBounds));

    // the srcRoD of the output should be enclosed in half the roi.
    // It does not have to be exactly half of the input.
    //    assert(dstRoD.x1*2 >= roi.x1 &&
    //           dstRoD.x2*2 <= roi.x2 &&
    //           dstRoD.y1*2 >= roi.y1 &&
    //           dstRoD.y2*2 <= roi.y2 &&
    //           dstRoD.width()*2 <= roi.width() &&
    //           dstRoD.height()*2 <= roi.height());
    assert( getComponents() == output->getComponents() );

    int nComponents = getComponents().getNumComponents();
    RectI dstRoI;
    RectI srcRoI = roi;
    srcRoI.intersect(srcBounds, &srcRoI); // intersect srcRoI with the region of definition

    dstRoI.x1 = std::floor(srcRoI.x1 / 2.);
    dstRoI.y1 = std::floor(srcRoI.y1 / 2.);
    dstRoI.x2 = std::ceil(srcRoI.x2 / 2.);
    dstRoI.y2 = std::ceil(srcRoI.y2 / 2.);

    
  
    
    const PIX* const srcPixels      = (const PIX*)pixelAt(srcBounds.x1,   srcBounds.y1);
    const char* const srcBmPixels   = _bitmap.getBitmapAt(srcBmBounds.x1, srcBmBounds.y1);
    PIX* const dstPixels          = (PIX*)output->pixelAt(dstBounds.x1,   dstBounds.y1);
    char* const dstBmPixels = output->_bitmap.getBitmapAt(dstBmBounds.x1, dstBmBounds.y1);

    int srcRowSize = srcBounds.width() * nComponents;
    int dstRowSize = dstBounds.width() * nComponents;

    // offset pointers so that srcData and dstData correspond to pixel (0,0)
    const PIX* const srcData = srcPixels - (srcBounds.x1 * nComponents + srcRowSize * srcBounds.y1);
    PIX* const dstData       = dstPixels - (dstBounds.x1 * nComponents + dstRowSize * dstBounds.y1);

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
            const PIX* const srcPixStart    = srcLineStart   + x * 2 * nComponents;
            const char* const srcBmPixStart = srcBmLineStart + x * 2;
            PIX* const dstPixStart          = dstLineStart   + x * nComponents;
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

            for (int k = 0; k < nComponents; ++k) {
                ///a b
                ///c d

                const PIX a = (pickThisCol && pickThisRow) ? *(srcPixStart + k) : 0;
                const PIX b = (pickNextCol && pickThisRow) ? *(srcPixStart + k + nComponents) : 0;
                const PIX c = (pickThisCol && pickNextRow) ? *(srcPixStart + k + srcRowSize): 0;
                const PIX d = (pickNextCol && pickNextRow) ? *(srcPixStart + k + srcRowSize  + nComponents)  : 0;
                
                assert(sumW == 2 || (sumW == 1 && ((a == 0 && c == 0) || (b == 0 && d == 0))));
                assert(sumH == 2 || (sumH == 1 && ((a == 0 && b == 0) || (c == 0 && d == 0))));
                dstPixStart[k] = (a + b + c + d) / sum;
            }
            
            if (copyBitMap) {
                ///a b
                ///c d

                char a = (pickThisCol && pickThisRow) ? *(srcBmPixStart) : 0;
                char b = (pickNextCol && pickThisRow) ? *(srcBmPixStart + 1) : 0;
                char c = (pickThisCol && pickNextRow) ? *(srcBmPixStart + srcBmRowSize): 0;
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
                assert(sumW == 2 || (sumW == 1 && ((a == 0 && c == 0) || (b == 0 && d == 0))));
                assert(sumH == 2 || (sumH == 1 && ((a == 0 && b == 0) || (c == 0 && d == 0))));
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
                Natron::Image* output) const
{
    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        halveRoIForDepth<unsigned char,255>(roi, copyBitMap,  output);
        break;
    case eImageBitDepthShort:
        halveRoIForDepth<unsigned short,65535>(roi,copyBitMap, output);
        break;
    case eImageBitDepthFloat:
        halveRoIForDepth<float,1>(roi,copyBitMap,output);
        break;
    case eImageBitDepthNone:
        break;
    }
}

// code proofread and fixed by @devernay on 8/8/2014
template <typename PIX, int maxValue>
void
Image::halve1DImageForDepth(const RectI & roi,
                            Natron::Image* output) const
{
    int width = roi.width();
    int height = roi.height();

    assert(width == 1 || height == 1); /// must be 1D
    assert( output->getComponents() == getComponents() );
    
    /// Take the lock for both bitmaps since we're about to read/write from them!
    QMutexLocker k1(&output->_entryLock);
    QMutexLocker k2(&_entryLock);

    
    const RectI & srcBounds = _bounds;
    const RectI & dstBounds = output->_bounds;
//    assert(dstBounds.x1 * 2 == roi.x1 &&
//           dstBounds.y1 * 2 == roi.y1 &&
//           (
//               dstBounds.x2 * 2 == roi.x2 || // we halve in only 1 dimension
//               dstBounds.y2 * 2 == roi.y2)
//           );
    
    


    int components = getComponents().getNumComponents();
    int halfWidth = width / 2;
    int halfHeight = height / 2;
    if (height == 1) { //1 row
        assert(width != 1); /// widthxheight can't be 1x1

        const PIX* src = (const PIX*)pixelAt(roi.x1, roi.y1);
        PIX* dst = (PIX*)output->pixelAt(dstBounds.x1, dstBounds.y1);
        assert(src && dst);
        for (int x = 0; x < halfWidth; ++x) {
            for (int k = 0; k < components; ++k) {
                *dst++ = PIX( (float)( *src + *(src + components) ) / 2. );
                ++src;
            }
            src += components;
        }
    } else if (width == 1) {
        int rowSize = srcBounds.width() * components;
        const PIX* src = (const PIX*)pixelAt(roi.x1, roi.y1);
        PIX* dst = (PIX*)output->pixelAt(dstBounds.x1, dstBounds.y1);
        assert(src && dst);
        for (int y = 0; y < halfHeight; ++y) {
            for (int k = 0; k < components; ++k) {
                *dst++ = PIX( (float)( *src + (*src + rowSize) ) / 2. );
                ++src;
            }
            src += rowSize;
        }
    }
}

// code proofread and fixed by @devernay on 8/8/2014
void
Image::halve1DImage(const RectI & roi,
                    Natron::Image* output) const
{
    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        halve1DImageForDepth<unsigned char, 255>(roi, output);
        break;
    case eImageBitDepthShort:
        halve1DImageForDepth<unsigned short, 65535>(roi, output);
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
                       Natron::Image* output) const
{
    ///You should not call this function with a level equal to 0.
    assert(toLevel >  fromLevel);

    assert(_bounds.x1 <= roi.x1 && roi.x2 <= _bounds.x2 &&
           _bounds.y1 <= roi.y1 && roi.y2 <= _bounds.y2);
    double par = getPixelAspectRatio();
//    RectD roiCanonical;
//    roi.toCanonical(fromLevel, par , dstRod, &roiCanonical);
//    RectI dstRoI;
//    roiCanonical.toPixelEnclosing(toLevel, par , &dstRoI);
    unsigned int downscaleLvls = toLevel - fromLevel;

    assert(!copyBitMap || _bitmap.getBitmap());
    
    RectI dstRoI  = roi.downscalePowerOfTwoSmallestEnclosing(downscaleLvls);
    
    ImagePtr tmpImg( new Natron::Image( getComponents(), dstRod, dstRoI, toLevel, par, getBitDepth() , true) );

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
Image::checkForNaNs(const RectI& roi)
{
    if (getBitDepth() != eImageBitDepthFloat) {
        return false;
    }
 
    QMutexLocker k(&_entryLock);
    
    unsigned int compsCount = getComponentsCount();

    bool hasnan = false;
    for (int y = roi.y1; y < roi.y2; ++y) {
        
        float* pix = (float*)pixelAt(roi.x1, y);
        float* const end = pix +  compsCount * roi.width();
        
        for (;pix < end; ++pix) {
            // we remove NaNs, but infinity values should pose no problem
            // (if they do, please explain here which ones)
            if (*pix != *pix) { // check for NaN
                *pix = 1.;
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
                             Natron::Image* output) const
{
    assert( getBitDepth() == output->getBitDepth() );
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) || (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) || (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );

    ///You should not call this function with a level equal to 0.
    assert(fromLevel > toLevel);

    assert(roi.x1 <= _bounds.x1 && _bounds.x2 <= roi.x2 &&
           roi.y1 <= _bounds.y1 && _bounds.y2 <= roi.y2);

    ///The source rectangle, intersected to this image region of definition in pixels
    RectD roiCanonical;
    roi.toCanonical(fromLevel, _par, getRoD(), &roiCanonical);
    RectI dstRoi;
    roiCanonical.toPixelEnclosing(toLevel, _par, &dstRoi);

    const RectI & srcRoi = roi;

    dstRoi.intersect(output->_bounds, &dstRoi); //output may be a bit smaller than the upscaled RoI
    int scale = 1 << (fromLevel - toLevel);

    assert( output->getComponents() == getComponents() );
    int components = getComponents().getNumComponents();
    if (components == 0) {
        return;
    }
    
    QMutexLocker k1(&output->_entryLock);
    QMutexLocker k2(&_entryLock);
    
    int srcRowSize = _bounds.width() * components;
    int dstRowSize = output->_bounds.width() * components;
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
        for (int xo = dstRoi.x1; xo < dstRoi.x2; ++xi, srcPix += components, xo += xcount, dstPixFirst += xcount * components) {
            xcount = scale + xo - xi * scale;
            //assert(0 < xcount && xcount <= scale);
            // replicate srcPix as many times as necessary
            PIX * dstPix = dstPixFirst;
            //assert((srcPix-(PIX*)pixelAt(srcRoi.x1, srcRoi.y1)) % components == 0);
            for (int i = 0; i < xcount; ++i, dstPix += components) {
                assert( ( dstPix - (PIX*)output->pixelAt(dstRoi.x1, dstRoi.y1) ) % components == 0 );
                for (int c = 0; c < components; ++c) {
                    dstPix[c] = srcPix[c];
                }
            }
            //assert(dstPix == dstPixFirst + xcount*components);
        }
        PIX * dstLineStart = dstLineBatchStart + dstRowSize; // first line was filled already
        // now replicate the line as many times as necessary
        for (int i = 1; i < ycount; ++i, dstLineStart += dstRowSize) {
            std::copy(dstLineBatchStart, dstLineBatchStart + dstRowSize, dstLineStart);
        }
    }
} // upscaleMipMapForDepth

// code proofread and fixed by @devernay on 8/8/2014
void
Image::upscaleMipMap(const RectI & roi,
                     unsigned int fromLevel,
                     unsigned int toLevel,
                     Natron::Image* output) const
{
    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        upscaleMipMapForDepth<unsigned char, 255>(roi, fromLevel, toLevel, output);
        break;
    case eImageBitDepthShort:
        upscaleMipMapForDepth<unsigned short, 65535>(roi, fromLevel, toLevel, output);
        break;
    case eImageBitDepthFloat:
        upscaleMipMapForDepth<float,1>(roi, fromLevel, toLevel, output);
        break;
    case eImageBitDepthNone:
        break;
    }
}

// FIXME: the following function has plenty of bugs.
// - scale should be determined from the RoD, not bounds, so that tiles images can be scaled too
// - the scaled roi may not start at dstBounds.x1, dstBounds.y1
template<typename PIX>
void
Image::scaleBoxForDepth(const RectI & roi,
                        Natron::Image* output) const
{
    assert( getBitDepth() == output->getBitDepth() );
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) || (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) || (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );

    
    QMutexLocker k1(&output->_entryLock);
    QMutexLocker k2(&_entryLock);
    
    ///The destination rectangle
    const RectI & dstBounds = output->_bounds;

    ///The source rectangle, intersected to this image region of definition in pixels
    const RectI & srcBounds = _bounds;
    RectI srcRoi = roi;
    srcRoi.intersect(srcBounds, &srcRoi);

    ///If the roi is exactly twice the destination rect, just halve that portion into output.
    if ( (srcRoi.x1 == 2 * dstBounds.x1) &&
         ( srcRoi.x2 == 2 * dstBounds.x2) &&
         ( srcRoi.y1 == 2 * dstBounds.y1) &&
         ( srcRoi.y2 == 2 * dstBounds.y2) ) {
        halveRoI(srcRoi, false, output);

        return;
    }
   

    RenderScale scale;
    // FIXME: should use the RoD instead of RoI/bounds !
    scale.x = (double) srcRoi.width() / dstBounds.width();
    scale.y = (double) srcRoi.height() / dstBounds.height();

    int convx_int = std::floor(scale.x);
    int convy_int = std::floor(scale.y);
    double convx_float = scale.x - convx_int;
    double convy_float = scale.y - convy_int;
    double area = scale.x * scale.y;
    int lowy_int = 0;
    double lowy_float = 0.;
    int highy_int = convy_int;
    double highy_float = convy_float;

    // FIXME: dstBounds.(x1,y1) may not correspond to the scaled srcRoi!
    const PIX *src = (const PIX*)pixelAt(srcRoi.x1, srcRoi.y1);
    PIX* dst = (PIX*)output->pixelAt(dstBounds.x1, dstBounds.y1);
    assert(src && dst);
    assert( output->getComponents() == getComponents() );
    int components = getComponents().getNumComponents();
    int rowSize = srcBounds.width() * components;
    float totals[4];

    for (int y = 0; y < dstBounds.height(); ++y) {
        /* Clamp here to be sure we don't read beyond input buffer. */
        if ( highy_int >= srcRoi.height() ) {
            highy_int = srcRoi.height() - 1;
        }

        int lowx_int = 0;
        double lowx_float = 0.;
        int highx_int = convx_int;
        double highx_float = convx_float;

        for (int x = 0; x < dstBounds.width(); ++x) {
            if ( highx_int >= srcRoi.width() ) {
                highx_int = srcRoi.width() - 1;
            }

            /*
            ** Ok, now apply box filter to box that goes from (lowx, lowy)
            ** to (highx, highy) on input data into this pixel on output
            ** data.
            */
            totals[0] = totals[1] = totals[2] = totals[3] = 0.0;

            /// calculate the value for pixels in the 1st row
            int xindex = lowx_int * components;
            if ( (highy_int > lowy_int) && (highx_int > lowx_int) ) {
                double y_percent = 1. - lowy_float;
                double percent = y_percent * (1. - lowx_float);
                const PIX* temp = src + xindex + lowy_int * rowSize;
                const PIX* temp_index;
                int k;
                for (k = 0,temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
                const PIX* left = temp;
                for (int l = lowx_int + 1; l < highx_int; ++l) {
                    temp += components;
                    for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                        totals[k] += *temp_index * y_percent;
                    }
                    ;
                }

                temp += components;
                const PIX* right = temp;
                percent = y_percent * highx_float;

                for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }

                /// calculate the value for pixels in the last row
                y_percent = highy_float;
                percent = y_percent * (1. - lowx_float);
                temp = src + xindex + highy_int * rowSize;
                for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
                for (int l = lowx_int + 1; l < highx_int; ++l) {
                    temp += components;
                    for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                        totals[k] += *temp_index * y_percent;
                    }
                }
                temp += components;
                percent = y_percent * highx_float;
                for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }

                /* calculate the value for pixels in the 1st and last column */
                for (int m = lowy_int + 1; m < highy_int; ++m) {
                    left += rowSize;
                    right += rowSize;
                    for (k = 0; k < components; ++k, ++left, ++right) {
                        totals[k] += *left * (1 - lowx_float) + *right * highx_float;
                    }
                }
            } else if (highy_int > lowy_int) {
                double x_percent = highx_float - lowx_float;
                double percent = (1. - lowy_float) * x_percent;
                const PIX* temp = src + xindex + lowy_int * rowSize;
                const PIX* temp_index;
                int k;
                for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
                for (int m = lowy_int + 1; m < highy_int; ++m) {
                    temp += rowSize;
                    for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                        totals[k] += *temp_index * x_percent;
                    }
                }
                percent = x_percent * highy_float;
                temp += rowSize;
                for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
            } else if (highx_int > lowx_int) {
                double y_percent = highy_float - lowy_float;
                double percent = (1. - lowx_float) * y_percent;
                int k;
                const PIX* temp = src + xindex + lowy_int * rowSize;
                const PIX* temp_index;

                for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
                for (int l = lowx_int + 1; l < highx_int; ++l) {
                    temp += components;
                    for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                        totals[k] += *temp_index * y_percent;
                    }
                }
                temp += components;
                percent = y_percent * highx_float;
                for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
            } else {
                double percent = (highy_float - lowy_float) * (highx_float - lowx_float);
                const PIX* temp = src + xindex + lowy_int * rowSize;
                int k;
                const PIX* temp_index;

                for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
            }

            /// this is for the pixels in the body
            const PIX* temp0 = src + lowx_int + components + (lowy_int + 1) * rowSize;
            const PIX* temp;

            for (int m = lowy_int + 1; m < highy_int; ++m) {
                temp = temp0;
                for (int l = lowx_int + 1; l < highx_int; ++l) {
                    int k;
                    const PIX *temp_index;

                    for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                        totals[k] += *temp_index;
                    }
                    temp += components;
                }
                temp0 += rowSize;
            }

            int outindex = ( x + ( y * dstBounds.width() ) ) * components;
            for (int k = 0; k < components; ++k) {
                dst[outindex + k] = PIX(totals[k] / area);
            }
            lowx_int = highx_int;
            lowx_float = highx_float;
            highx_int += convx_int;
            highx_float += convx_float;
            if (highx_float > 1) {
                highx_float -= 1.0;
                ++highx_int;
            }
        }
        lowy_int = highy_int;
        lowy_float = highy_float;
        highy_int += convy_int;
        highy_float += convy_float;
        if (highy_float > 1) {
            highy_float -= 1.0;
            ++highy_int;
        }
    }
} // scaleBoxForDepth

//Image::scale should never be used: there should only be a method to *up*scale by a power of two, and the downscaling is done by
//buildMipMapLevel
// FIXME: this function has many bugs (see scaleBoxForDepth())
void
Image::scaleBox(const RectI & roi,
                Natron::Image* output) const
{
    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        scaleBoxForDepth<unsigned char>(roi, output);
        break;
    case eImageBitDepthShort:
        scaleBoxForDepth<unsigned short>(roi, output);
        break;
    case eImageBitDepthFloat:
        scaleBoxForDepth<float>(roi, output);
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
                        Natron::Image* output) const
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

    const Natron::Image* srcImg = this;
    Natron::Image* dstImg = NULL;
    bool mustFreeSrc = false;
    RectI previousRoI = roi;
    ///Build all the mipmap levels until we reach the one we are interested in
    for (unsigned int i = 1; i <= level; ++i) {
        ///Halve the smallest enclosing po2 rect as we need to render a minimum of the renderWindow
        RectI halvedRoI = previousRoI.downscalePowerOfTwoSmallestEnclosing(1);

        ///Allocate an image with half the size of the source image
        dstImg = new Natron::Image( getComponents(), dstRoD, halvedRoI, getMipMapLevel() + i, getPixelAspectRatio(),getBitDepth() , true);

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
    output->pasteFrom( *srcImg, srcImg->getBounds() , copyBitMap);

    ///Clean-up, we should use shared_ptrs for safety
    if (mustFreeSrc) {
        delete srcImg;
    }
} // buildMipMapLevel

double
Image::getScaleFromMipMapLevel(unsigned int level)
{
    return 1. / (1 << level);
}

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
Image::copyBitmapRowPortion(int x1, int x2,int y, const Image& other)
{
    _bitmap.copyRowPortion(x1, x2, y, other._bitmap);
}

void
Bitmap::copyRowPortion(int x1,int x2,int y,const Bitmap& other)
{
    const char* srcBitmap = other.getBitmapAt(x1, y);
    char* dstBitmap = getBitmapAt(x1, y);
    const char* end = dstBitmap + (x2 - x1);
    while (dstBitmap < end) {
        *dstBitmap = /**srcBitmap == PIXEL_UNAVAILABLE ? 0 : */*srcBitmap;
        ++dstBitmap;
        ++srcBitmap;
    }
    
}

void
Image::copyBitmapPortion(const RectI& roi, const Image& other)
{
    _bitmap.copyBitmapPortion(roi, other._bitmap);

}

void
Bitmap::copyBitmapPortion(const RectI& roi, const Bitmap& other)
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
            *dstCur = /**srcCur == PIXEL_UNAVAILABLE ? 0 : */*srcCur;
            ++srcCur;
            ++dstCur;
        }
    }
}

