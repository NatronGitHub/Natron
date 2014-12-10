//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Image.h"

#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/math/special_functions/fpclassify.hpp>
#endif
#include "Engine/AppManager.h"
#include "Engine/Lut.h"

using namespace Natron;



RectI
Bitmap::minimalNonMarkedBbox(const RectI & roi) const
{
    /*if we rendered everything we just append
       a NULL box to indicate we rendered it all.*/
//    if(!memchr(_map.get(),0,_rod.area())){
//        ret.push_back(RectI());
//        return ret;
//    }

    RectI bbox;

    roi.intersect(_bounds, &bbox); // be safe
    //find bottom
    for (int i = bbox.bottom(); i < bbox.top(); ++i) {
        const char* buf = &_map[( i - _bounds.bottom() ) * _bounds.width()];
        if ( !memchr( buf, 0, _bounds.width() ) ) {
            bbox.set_bottom(bbox.bottom() + 1);
        } else {
            break;
        }
    }

    //find top (will do zero iteration if the bbox is already empty)
    for (int i = bbox.top() - 1; i >= bbox.bottom(); --i) {
        const char* buf = &_map[( i - _bounds.bottom() ) * _bounds.width()];
        if ( !memchr( buf, 0, _bounds.width() ) ) {
            bbox.set_top(bbox.top() - 1);
        } else {
            break;
        }
    }

    // avoid making bbox.width() iterations for nothing
    if ( bbox.isNull() ) {
        return bbox;
    }

    //find left
    for (int j = bbox.left(); j < bbox.right(); ++j) {
        bool shouldStop = false;
        for (int i = bbox.bottom(); i < bbox.top(); ++i) {
            if (!_map[( i - _bounds.bottom() ) * _bounds.width() + ( j - _bounds.left() )]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bbox.set_left(bbox.left() + 1);
        } else {
            break;
        }
    }

    //find right
    for (int j = bbox.right() - 1; j >= bbox.left(); --j) {
        bool shouldStop = false;
        for (int i = bbox.bottom(); i < bbox.top(); ++i) {
            if (!_map[( i - _bounds.bottom() ) * _bounds.width() + ( j - _bounds.left() )]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bbox.set_right(bbox.right() - 1);
        } else {
            break;
        }
    }

    return bbox;
} // minimalNonMarkedBbox

std::list<RectI>
Bitmap::minimalNonMarkedRects(const RectI & roi) const
{
    std::list<RectI> ret;
    RectI bboxM = minimalNonMarkedBbox(roi);

//#define NATRON_BITMAP_DISABLE_OPTIMIZATION
#ifdef NATRON_BITMAP_DISABLE_OPTIMIZATION
    if ( !bboxM.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxM);
    }
#else
    if ( bboxM.isNull() ) {
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
    bboxA.set_top( bboxX.bottom() );
    for (int i = bboxX.bottom(); i < bboxX.top(); ++i) {
        const char* buf = &_map[( i - _bounds.bottom() ) * _bounds.width()];
        if ( !memchr( buf, 1, _bounds.width() ) ) {
            bboxX.set_bottom(bboxX.bottom() + 1);
            bboxA.set_top( bboxX.bottom() );
        } else {
            break;
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
        const char* buf = &_map[( i - _bounds.bottom() ) * _bounds.width()];
        if ( !memchr( buf, 1, _bounds.width() ) ) {
            bboxX.set_top(bboxX.top() - 1);
            bboxB.set_bottom( bboxX.top() );
        } else {
            break;
        }
    }
    if ( !bboxB.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxB);
    }

    //find left
    RectI bboxC = bboxX;
    bboxC.set_right( bboxX.left() );
    for (int j = bboxX.left(); j < bboxX.right(); ++j) {
        bool shouldStop = false;
        for (int i = bboxX.bottom(); i < bboxX.top(); ++i) {
            if (_map[( i - _bounds.bottom() ) * _bounds.width() + ( j - _bounds.left() )]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bboxX.set_left(bboxX.left() + 1);
            bboxC.set_right( bboxX.left() );
        } else {
            break;
        }
    }
    if ( !bboxC.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxC);
    }

    //find right
    RectI bboxD = bboxX;
    bboxD.set_left( bboxX.right() );
    for (int j = bboxX.right() - 1; j >= bboxX.left(); --j) {
        bool shouldStop = false;
        for (int i = bboxX.bottom(); i < bboxX.top(); ++i) {
            if (_map[( i - _bounds.bottom() ) * _bounds.width() + ( j - _bounds.left() )]) {
                shouldStop = true;
                break;
            }
        }
        if (!shouldStop) {
            bboxX.set_right(bboxX.right() - 1);
            bboxD.set_left( bboxX.right() );
        } else {
            break;
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
    bboxX = minimalNonMarkedBbox(bboxX);
    if ( !bboxX.isNull() ) { // empty boxes should not be pushed
        ret.push_back(bboxX);
    }

#endif // NATRON_BITMAP_DISABLE_OPTIMIZATION
# ifdef DEBUG
    /*qDebug() << "render " << ret.size() << " rectangles";
    for (std::list<RectI>::const_iterator it = ret.begin(); it != ret.end(); ++it) {
        qDebug() << "rect: " << "x1= " <<  it->x1 << " , x2= " << it->x2 << " , y1= " << it->y1 << " , y2= " << it->y2;
    }*/
# endif // DEBUG
    return ret;
} // minimalNonMarkedRects

void
Natron::Bitmap::markForRendered(const RectI & roi)
{
    for (int i = roi.bottom(); i < roi.top(); ++i) {
        char* buf = &_map[( i - _bounds.bottom() ) * _bounds.width() + ( roi.left() - _bounds.left() )];
        memset( buf, 1, roi.width() );
    }
}

const char*
Natron::Bitmap::getBitmapAt(int x,
                            int y) const
{
    if ( ( x >= _bounds.left() ) && ( x < _bounds.right() ) && ( y >= _bounds.bottom() ) && ( y < _bounds.top() ) ) {
        return &_map[( y - _bounds.bottom() ) * _bounds.width() + ( x - _bounds.left() )];
    } else {
        return NULL;
    }
}

char*
Natron::Bitmap::getBitmapAt(int x,
                            int y)
{
    if ( ( x >= _bounds.left() ) && ( x < _bounds.right() ) && ( y >= _bounds.bottom() ) && ( y < _bounds.top() ) ) {
        return &_map[( y - _bounds.bottom() ) * _bounds.width() + ( x - _bounds.left() )];
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
    _components = params->getComponents();
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
    _components = params->getComponents();
    _bitDepth = params->getBitDepth();
    _rod = params->getRoD();
    _bounds = params->getBounds();
    _par = params->getPixelAspectRatio();
    allocateMemory();
}


/*This constructor can be used to allocate a local Image. The deallocation should
   then be handled by the user. Note that no view number is passed in parameter
   as it is not needed.*/
Image::Image(ImageComponentsEnum components,
             const RectD & regionOfDefinition, //!< rod in canonical coordinates
             const RectI & bounds, //!< bounds in pixel coordinates
             unsigned int mipMapLevel,
             double par,
             Natron::ImageBitDepthEnum bitdepth,
             bool useBitmap)
    : CacheEntryHelper<unsigned char,ImageKey,ImageParams>()
    , _useBitmap(useBitmap)
{
    setCacheEntry(makeKey(0,false,0,0),
                  boost::shared_ptr<ImageParams>( new ImageParams( 0,
                                                                   regionOfDefinition,
                                                                   1.,
                                                                   mipMapLevel,
                                                                   bounds,
                                                                   bitdepth,
                                                                   false,
                                                                   components,
                                                                   std::map<int,std::vector<RangeD> >() ) ),
                  NULL,
                  Natron::eStorageModeRAM,
                  std::string()
                  );

    _components = components;
    _bitDepth = bitdepth;
    _rod = regionOfDefinition;
    _bounds = _params->getBounds();
    _par = par;
    allocateMemory();
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
        fill(_bounds,1.,0.,0.,1.);
    }
#endif
    
}


ImageKey  
Image::makeKey(U64 nodeHashKey,
               bool frameVaryingOrAnimated,
               SequenceTime time,
               int view)
{
    return ImageKey(nodeHashKey,frameVaryingOrAnimated,time,view);
}

boost::shared_ptr<ImageParams>
Image::makeParams(int cost,
                  const RectD & rod,
                  const double par,
                  unsigned int mipMapLevel,
                  bool isRoDProjectFormat,
                  ImageComponentsEnum components,
                  Natron::ImageBitDepthEnum bitdepth,
                  const std::map<int, std::vector<RangeD> > & framesNeeded)
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
                  ImageComponentsEnum components,
                  Natron::ImageBitDepthEnum bitdepth,
                  const std::map<int, std::vector<RangeD> > & framesNeeded)
{
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

//boost::shared_ptr<ImageParams>
//Image::getParams() const WARN_UNUSED_RETURN
//{
//    return boost::dynamic_pointer_cast<ImageParams>(_params);
//}

// code proofread and fixed by @devernay on 8/8/2014
template<typename PIX>
void
Image::pasteFromForDepth(const Natron::Image & srcImg,
                         const RectI & srcRoi,
                         bool copyBitmap)
{
    ///Cannot copy images with different bit depth, this is not the purpose of this function.
    ///@see convert
    assert( getBitDepth() == srcImg.getBitDepth() );
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) || (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) || (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );
    // NOTE: before removing the following asserts, please explain why an empty image may happen
    const RectI & bounds = getBounds();
    const RectI & srcBounds = srcImg.getBounds();

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
    int components = getElementsCountForComponents( getComponents() );

    if (copyBitmap) {
        copyBitmapPortion(roi, srcImg);
    }
    // now we're safe: both images contain the area in roi
    for (int y = roi.y1; y < roi.y2; ++y) {
        const PIX* src = (const PIX*)srcImg.pixelAt(roi.x1, y);
        PIX* dst = (PIX*)pixelAt(roi.x1, y);
        memcpy(dst, src, roi.width() * sizeof(PIX) * components);
    }
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
        pasteFromForDepth<unsigned char>(src, srcRoi, copyBitmap);
        break;
    case eImageBitDepthShort:
        pasteFromForDepth<unsigned short>(src, srcRoi, copyBitmap);
        break;
    case eImageBitDepthFloat:
        pasteFromForDepth<float>(src, srcRoi, copyBitmap);
        break;
    case eImageBitDepthNone:
        break;
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
    assert( (getBitDepth() == eImageBitDepthByte && sizeof(PIX) == 1) || (getBitDepth() == eImageBitDepthShort && sizeof(PIX) == 2) || (getBitDepth() == eImageBitDepthFloat && sizeof(PIX) == 4) );

    ImageComponentsEnum comps = getComponents();
    if (comps == eImageComponentNone) {
        return;
    }

    RectI roi = roi_;
    bool doInteresect = roi.intersect(getBounds(), &roi);
    if (!doInteresect) {
        // no intersection between roi and the bounds of the image
        return;
    }

    int rowElems = (int)getRowElements();
    const float fillValue[4] = {
        comps == Natron::eImageComponentAlpha ? a : r, g, b, a
    };
    int nComps = getElementsCountForComponents(comps);

    // now we're safe: the image contains the area in roi
    PIX* dst = (PIX*)pixelAt(roi.x1, roi.y1);
    for ( int i = 0; i < roi.height(); ++i, dst += (rowElems - roi.width() * nComps) ) {
        for (int j = 0; j < roi.width(); ++j, dst += nComps) {
            for (int k = 0; k < nComps; ++k) {
                dst[k] = fillValue[k] * maxValue;
            }
        }
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

unsigned char*
Image::pixelAt(int x,
               int y)
{
    int compsCount = getElementsCountForComponents( getComponents() );

    if ( ( x < _bounds.left() ) || ( x >= _bounds.right() ) || ( y < _bounds.bottom() ) || ( y >= _bounds.top() )) {
        return NULL;
    } else {
        int compDataSize = getSizeOfForBitDepth( getBitDepth() ) * compsCount;
        
        return (unsigned char*)(this->_data.writable())
        + ( y - _bounds.bottom() ) * compDataSize * _bounds.width()
        + ( x - _bounds.left() ) * compDataSize;
    }
}

const unsigned char*
Image::pixelAt(int x,
               int y) const
{
    int compsCount = getElementsCountForComponents( getComponents() );
    
    if ( ( x < _bounds.left() ) || ( x >= _bounds.right() ) || ( y < _bounds.bottom() ) || ( y >= _bounds.top() )) {
        return NULL;
    } else {
        int compDataSize = getSizeOfForBitDepth( getBitDepth() ) * compsCount;
        
        return (unsigned char*)(this->_data.readable())
        + ( y - _bounds.bottom() ) * compDataSize * _bounds.width()
        + ( x - _bounds.left() ) * compDataSize;
    }
}

unsigned int
Image::getComponentsCount() const
{
    return getElementsCountForComponents( getComponents() );
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

            return false;
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
Image::getFormatString(Natron::ImageComponentsEnum comps,
                       Natron::ImageBitDepthEnum depth)
{
    std::string s;

    switch (comps) {
    case Natron::eImageComponentRGBA:
        s += "RGBA";
        break;
    case Natron::eImageComponentRGB:
        s += "RGB";
        break;
    case Natron::eImageComponentAlpha:
        s += "Alpha";
        break;
    default:
        break;
    }
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

    ///The source rectangle, intersected to this image region of definition in pixels
    const RectI &srcBounds = getBounds();
    const RectI &dstBounds = output->getBounds();
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

    int nComponents = getElementsCountForComponents( getComponents() );
    RectI dstRoI;
    RectI srcRoI = roi;
    srcRoI.intersect(srcBounds, &srcRoI); // intersect srcRoI with the region of definition

    dstRoI.x1 = std::floor(srcRoI.x1 / 2.);
    dstRoI.y1 = std::floor(srcRoI.y1 / 2.);
    dstRoI.x2 = std::ceil(srcRoI.x2 / 2.);
    dstRoI.y2 = std::ceil(srcRoI.y2 / 2.);

    
    /// Take the lock for both bitmaps since we're about to read/write from them!
    QWriteLocker k1(&output->_lock);
    QReadLocker k2(&_lock);
    
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

                const char a = (pickThisCol && pickThisRow) ? *(srcBmPixStart) : 0;
                const char b = (pickNextCol && pickThisRow) ? *(srcBmPixStart + 1) : 0;
                const char c = (pickThisCol && pickNextRow) ? *(srcBmPixStart + srcBmRowSize): 0;
                const char d = (pickNextCol && pickNextRow) ? *(srcBmPixStart + srcBmRowSize  + 1)  : 0;

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
        halveRoIForDepth<unsigned char,255>(roi, copyBitMap, output);
        break;
    case eImageBitDepthShort:
        halveRoIForDepth<unsigned short,65535>(roi,copyBitMap, output);
        break;
    case eImageBitDepthFloat:
        halveRoIForDepth<float,1>(roi,copyBitMap, output);
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
    const RectI & srcBounds = getBounds();
    const RectI & dstBounds = output->getBounds();
//    assert(dstBounds.x1 * 2 == roi.x1 &&
//           dstBounds.y1 * 2 == roi.y1 &&
//           (
//               dstBounds.x2 * 2 == roi.x2 || // we halve in only 1 dimension
//               dstBounds.y2 * 2 == roi.y2)
//           );

    int components = getElementsCountForComponents( getComponents() );
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
Image::downscaleMipMap(const RectI & roi,
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
//    roi.toCanonical(fromLevel, par , getRoD(), &roiCanonical);
//    RectI dstRoI;
//    roiCanonical.toPixelEnclosing(toLevel, par , &dstRoI);
    unsigned int downscaleLvls = toLevel - fromLevel;

    assert(!copyBitMap || _bitmap.getBitmap());
    
    RectI dstRoI  = roi.downscalePowerOfTwoSmallestEnclosing(downscaleLvls);
    
    ImagePtr tmpImg( new Natron::Image( getComponents(), getRoD(), dstRoI, toLevel, par, getBitDepth() , true) );

    buildMipMapLevel( roi, downscaleLvls, copyBitMap, tmpImg.get() );

    // check that the downscaled mipmap is inside the output image (it may not be equal to it)
    assert(dstRoI.x1 >= output->getBounds().x1);
    assert(dstRoI.x2 <= output->getBounds().x2);
    assert(dstRoI.y1 >= output->getBounds().y1);
    assert(dstRoI.y2 <= output->getBounds().y2);

    ///Now copy the result of tmpImg into the output image
    output->pasteFrom(*tmpImg, dstRoI, copyBitMap);
}


void
Image::checkForNaNs(const RectI& roi)
{
    if (getBitDepth() != eImageBitDepthFloat) {
        return;
    }
    
    unsigned int compsCount = getComponentsCount();
    
    for (int y = roi.y1; y < roi.y2; ++y) {
        
        float* pix = (float*)pixelAt(roi.x1, roi.y1);
        float* const end = pix +  compsCount * roi.width();
        
        for (;pix < end; ++pix) {
            assert(!boost::math::isnan(*pix) && !boost::math::isinf(*pix));
            if (boost::math::isnan(*pix) || boost::math::isinf(*pix)) {
                *pix = 1.;
            }
        }
    }


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

    dstRoi.intersect(output->getBounds(), &dstRoi); //output may be a bit smaller than the upscaled RoI
    int scale = 1 << (fromLevel - toLevel);

    assert( output->getComponents() == getComponents() );
    int components = getElementsCountForComponents( getComponents() );
    if (components == 0) {
        return;
    }
    int srcRowSize = getBounds().width() * components;
    int dstRowSize = output->getBounds().width() * components;
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
        ycount = scale + yo - yi * scale; // how many lines should be filled
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

    ///The destination rectangle
    const RectI & dstBounds = output->getBounds();

    ///The source rectangle, intersected to this image region of definition in pixels
    const RectI & srcBounds = getBounds();
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
    int components = getElementsCountForComponents( getComponents() );
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
                for (k = 0,temp_index = temp; k < components; ++k,++temp_index) {
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
Image::buildMipMapLevel(const RectI & roi,
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
        dstImg = new Natron::Image( getComponents(), getRoD(), halvedRoI, 0, getPixelAspectRatio(),getBitDepth() , true);

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

namespace Natron {
///explicit template instantiations

template <>
float
convertPixelDepth(unsigned char pix)
{
    return Color::intToFloat<256>(pix);
}

template <>
unsigned short
convertPixelDepth(unsigned char pix)
{
    // 0x01 -> 0x0101, 0x02 -> 0x0202, ..., 0xff -> 0xffff
    return (unsigned short)( (pix << 8) + pix );
}

template <>
unsigned char
convertPixelDepth(unsigned char pix)
{
    return pix;
}

template <>
unsigned char
convertPixelDepth(unsigned short pix)
{
    // the following is from ImageMagick's quantum.h
    return (unsigned char)( ( (pix + 128UL) - ( (pix + 128UL) >> 8 ) ) >> 8 );
}

template <>
float
convertPixelDepth(unsigned short pix)
{
    return Color::intToFloat<65536>(pix);
}

template <>
unsigned short
convertPixelDepth(unsigned short pix)
{
    return pix;
}

template <>
unsigned char
convertPixelDepth(float pix)
{
    return (unsigned char)Color::floatToInt<256>(pix);
}

template <>
unsigned short
convertPixelDepth(float pix)
{
    return (unsigned short)Color::floatToInt<65536>(pix);
}

template <>
float
convertPixelDepth(float pix)
{
    return pix;
}
}

static const Natron::Color::Lut*
lutFromColorspace(Natron::ViewerColorSpaceEnum cs)
{
    const Natron::Color::Lut* lut;

    switch (cs) {
    case Natron::eViewerColorSpaceSRGB:
        lut = Natron::Color::LutManager::sRGBLut();
        break;
    case Natron::eViewerColorSpaceRec709:
        lut = Natron::Color::LutManager::Rec709Lut();
        break;
    case Natron::eViewerColorSpaceLinear:
    default:
        lut = 0;
        break;
    }
    if (lut) {
        lut->validate();
    }

    return lut;
}

void
Image::copyBitmapRowPortion(int x1, int x2,int y, const Image& other)
{
    QWriteLocker k1(&_lock);
    QReadLocker k2(&other._lock);
    _bitmap.copyRowPortion(x1, x2, y, other._bitmap);
}

void
Bitmap::copyRowPortion(int x1,int x2,int y,const Bitmap& other)
{
    const char* srcBitmap = other.getBitmapAt(x1, y);
    char* dstBitmap = getBitmapAt(x1, y);
    memcpy( dstBitmap, srcBitmap, x2 - x1 );
    
}

void
Image::copyBitmapPortion(const RectI& roi, const Image& other)
{
    QWriteLocker k1(&_lock);
    QReadLocker k2(&other._lock);
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
        memcpy(dstBitmap, srcBitmap, roi.width());
    }
}

///Fast version when components are the same
template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue>
void
convertToFormatInternal_sameComps(const RectI & renderWindow,
                                  const Image & srcImg,
                                  Image & dstImg,
                                  Natron::ViewerColorSpaceEnum srcColorSpace,
                                  Natron::ViewerColorSpaceEnum dstColorSpace,
                                  bool invert,
                                  bool copyBitmap)
{
    const RectI & r = srcImg.getBounds();
    RectI intersection;

    if ( !renderWindow.intersect(r, &intersection) ) {
        return;
    }

    Natron::ImageBitDepthEnum dstDepth = dstImg.getBitDepth();
    Natron::ImageBitDepthEnum srcDepth = srcImg.getBitDepth();
    int nComp = (int)srcImg.getComponentsCount();
    const Natron::Color::Lut* srcLut = lutFromColorspace(srcColorSpace);
    const Natron::Color::Lut* dstLut = lutFromColorspace(dstColorSpace);

    ///no colorspace conversion applied when luts are the same
    if (srcLut == dstLut) {
        srcLut = dstLut = 0;
    }
    if (intersection.isNull()) {
        return;
    }
    for (int y = 0; y < intersection.height(); ++y) {
        int start = rand() % intersection.width();
        const SRCPIX* srcPixels = (const SRCPIX*)srcImg.pixelAt(intersection.x1 + start, intersection.y1 + y);
        DSTPIX* dstPixels = (DSTPIX*)dstImg.pixelAt(intersection.x1 + start, intersection.y1 + y);
        const SRCPIX* srcStart = srcPixels;
        DSTPIX* dstStart = dstPixels;

        for (int backward = 0; backward < 2; ++backward) {
            int x = backward ? start - 1 : start;
            int end = backward ? -1 : intersection.width();
            unsigned error[3] = {
                0x80,0x80,0x80
            };

            while ( x != end && x >= 0 && x < intersection.width() ) {
                for (int k = 0; k < nComp; ++k) {
                    if ( (k <= 2) && (srcLut || dstLut) ) {
                        float pixFloat;

                        if (srcLut) {
                            if (srcDepth == eImageBitDepthByte) {
                                pixFloat = srcLut->fromColorSpaceUint8ToLinearFloatFast(srcPixels[k]);
                            } else if (srcDepth == eImageBitDepthShort) {
                                pixFloat = srcLut->fromColorSpaceUint16ToLinearFloatFast(srcPixels[k]);
                            } else {
                                pixFloat = srcLut->fromColorSpaceFloatToLinearFloat(srcPixels[k]);
                            }
                        } else {
                            pixFloat = convertPixelDepth<SRCPIX, float>(srcPixels[k]);
                        }


                        DSTPIX pix;
                        if (dstDepth == eImageBitDepthByte) {
                            ///small increase in perf we use Luts. This should be anyway the most used case.
                            error[k] = (error[k] & 0xff) + ( dstLut ? dstLut->toColorSpaceUint8xxFromLinearFloatFast(pixFloat) :
                                                             Color::floatToInt<0xff01>(pixFloat) );
                            pix = error[k] >> 8;
                        } else if (dstDepth == eImageBitDepthShort) {
                            pix = dstLut ? dstLut->toColorSpaceUint16FromLinearFloatFast(pixFloat) :
                                  convertPixelDepth<float, DSTPIX>(pixFloat);
                        } else {
                            if (dstLut) {
                                pixFloat = dstLut->toColorSpaceFloatFromLinearFloat(pixFloat);
                            }
                            pix = convertPixelDepth<float, DSTPIX>(pixFloat);
                        }
                        dstPixels[k] = invert ? dstMaxValue - pix : pix;
                    } else {
                        DSTPIX pix = convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[k]);
                        dstPixels[k] = invert ? dstMaxValue - pix : pix;
                    }
                }

                if (backward) {
                    --x;
                    srcPixels -= nComp;
                    dstPixels -= nComp;
                } else {
                    ++x;
                    srcPixels += nComp;
                    dstPixels += nComp;
                }
            }
            srcPixels = srcStart - nComp;
            dstPixels = dstStart - nComp;
        }

        if (copyBitmap) {
            dstImg.copyBitmapRowPortion(intersection.x1, intersection.x2, intersection.y1 + y, srcImg);
        }
    }
} // convertToFormatInternal_sameComps

template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue,int srcNComps,int dstNComps>
void
convertToFormatInternal(const RectI & renderWindow,
                        const Image & srcImg,
                        Image & dstImg,
                        Natron::ViewerColorSpaceEnum srcColorSpace,
                        Natron::ViewerColorSpaceEnum dstColorSpace,
                        int channelForAlpha,
                        bool invert,
                        bool copyBitmap,
                        bool requiresUnpremult)
{
    const RectI & r = srcImg.getBounds();
    RectI intersection;

    if ( !renderWindow.intersect(r, &intersection) ) {
        return;
    }

    assert(!intersection.isNull() && intersection.width() > 0 && intersection.height() > 0);

    if (channelForAlpha == -1) {
        switch (srcNComps) {
            case 4:
                channelForAlpha = 3;
                break;
            case 3:
            case 1:
             default:
                break;
        }

    }

    Natron::ImageBitDepthEnum dstDepth = dstImg.getBitDepth();
    Natron::ImageBitDepthEnum srcDepth = dstDepth;

    ///special case comp == alpha && channelForAlpha = -1 clear out the mask
    if ( dstNComps == 1 && (channelForAlpha == -1) ) {
        DSTPIX* dstPixels = (DSTPIX*)dstImg.pixelAt(intersection.x1, intersection.y1);

        if (copyBitmap) {
            dstImg.copyBitmapPortion(intersection, srcImg);
        }
        for ( int y = 0; y < intersection.height();
              ++y, dstPixels += (r.width() * dstNComps) ) {
            std::fill(dstPixels, dstPixels + intersection.width() * dstNComps, 0.);
        }

        return;
    }

    const Natron::Color::Lut* srcLut = lutFromColorspace(srcColorSpace);
    const Natron::Color::Lut* dstLut = lutFromColorspace(dstColorSpace);
    
    for (int y = 0; y < intersection.height(); ++y) {
        
        ///Start of the line for error diffusion
        int start = rand() % intersection.width();
        
        const SRCPIX* srcPixels = (const SRCPIX*)srcImg.pixelAt(intersection.x1 + start, intersection.y1 + y);
        
        DSTPIX* dstPixels = (DSTPIX*)dstImg.pixelAt(intersection.x1 + start, intersection.y1 + y);
        
        const SRCPIX* srcStart = srcPixels;
        DSTPIX* dstStart = dstPixels;

        for (int backward = 0; backward < 2; ++backward) {
            
            ///We do twice the loop, once from starting point to end and once from starting point - 1 to real start
            int x = backward ? start - 1 : start;
            
            //End is pointing to the first pixel outside the line a la stl
            int end = backward ? -1 : intersection.width();
            unsigned error[3] = {
                0x80,0x80,0x80
            };

            while ( x != end && x >= 0 && x < intersection.width() ) {
                if (dstNComps == 1) {
                    ///If we're converting to alpha, we just have to handle pixel depth conversion
                    DSTPIX pix;

                    // convertPixelDepth is optimized when SRCPIX == DSTPIX

                    switch (srcNComps) {
                        case 4:
                            pix = convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[channelForAlpha]);
                            break;
                        case 3:
                            pix = convertPixelDepth<SRCPIX, DSTPIX>(1); // RGB is opaque
                            break;
                        case 1:
                            pix  = convertPixelDepth<SRCPIX, DSTPIX>(*srcPixels);
                            break;
                    }

                    dstPixels[0] = invert ? dstMaxValue - pix: pix;
                } else {
                    
                    if (srcNComps == 1) {
                        ///If we're converting from alpha, R G and B are 0.
                        //assert(dstNComps == 3 || dstNComps == 4);
                        for (int k = 0; k < std::min(3, dstNComps); ++k) {
                            dstPixels[k] = invert ? dstMaxValue : 0;
                        }
                        if (dstNComps == 4) {
                            DSTPIX pix = convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[0]);
                            dstPixels[dstNComps - 1] = invert ? dstMaxValue - pix: pix;
                        }
                    } else {
                        ///In this case we've RGB or RGBA input and outputs
                        assert(srcImg.getComponents() != dstImg.getComponents());
                        
                        bool unpremultChannel = (srcImg.getComponents() == Natron::eImageComponentRGBA &&
                                                 dstImg.getComponents() == Natron::eImageComponentRGB &&
                                                 requiresUnpremult);
                        
                        ///This is only set if unpremultChannel is true
                        float alphaForUnPremult;
                        if (unpremultChannel) {
                            alphaForUnPremult = convertPixelDepth<SRCPIX, float>(srcPixels[srcNComps - 1]);
                        } else {
                            alphaForUnPremult = 0.;
                        }
                        
                        for (int k = 0; k < dstNComps; ++k) {
                            if (k == 3) {
                                ///For alpha channel, fill with 1, we reach here only if converting RGB-->RGBA
                                DSTPIX pix = convertPixelDepth<float, DSTPIX>(0.f);
                                dstPixels[k] = invert ? dstMaxValue - pix : pix;
                            } else {
                                ///For RGB channels
                                float pixFloat;
                                
                                ///Unpremult before doing colorspace conversion from linear to X
                                if (unpremultChannel) {
                                    pixFloat = convertPixelDepth<SRCPIX, float>(srcPixels[k]);
                                    pixFloat = alphaForUnPremult == 0.f ? 0. : pixFloat / alphaForUnPremult;
                                    if (srcLut) {
                                        pixFloat = srcLut->fromColorSpaceFloatToLinearFloat(pixFloat);
                                    }
                                } else {
                                    
                                    if (srcLut) {
                                        if (srcDepth == eImageBitDepthByte) {
                                            pixFloat = srcLut->fromColorSpaceUint8ToLinearFloatFast(srcPixels[k]);
                                        } else if (srcDepth == eImageBitDepthShort) {
                                            pixFloat = srcLut->fromColorSpaceUint16ToLinearFloatFast(srcPixels[k]);
                                        } else {
                                            pixFloat = srcLut->fromColorSpaceFloatToLinearFloat(srcPixels[k]);
                                        }
                                    } else {
                                        pixFloat = convertPixelDepth<SRCPIX, float>(srcPixels[k]);
                                    }
                                }
                                
                                ///Apply dst color-space
                                DSTPIX pix;
                                if (dstDepth == eImageBitDepthByte) {
                                    error[k] = (error[k] & 0xff) + ( dstLut ? dstLut->toColorSpaceUint8xxFromLinearFloatFast(pixFloat) :
                                                                    Color::floatToInt<0xff01>(pixFloat) );
                                    pix = error[k] >> 8;
                                } else if (dstDepth == eImageBitDepthShort) {
                                    pix = dstLut ? dstLut->toColorSpaceUint16FromLinearFloatFast(pixFloat) :
                                    convertPixelDepth<float, DSTPIX>(pixFloat);
                                } else {
                                    if (dstLut) {
                                        pixFloat = dstLut->toColorSpaceFloatFromLinearFloat(pixFloat);
                                    } else {
                                        pix = convertPixelDepth<float, DSTPIX>(pixFloat);
                                    }
                                }
                                dstPixels[k] = invert ? dstMaxValue - pix : pix;
                            }
                        }
                    }
                }

                if (backward) {
                    --x;
                    srcPixels -= srcNComps;
                    dstPixels -= dstNComps;
                } else {
                    ++x;
                    srcPixels += srcNComps;
                    dstPixels += dstNComps;
                }
            }
            srcPixels = srcStart - srcNComps;
            dstPixels = dstStart - dstNComps;
        }
    }
    
    if (copyBitmap) {
        dstImg.copyBitmapPortion(intersection, srcImg);
    }
} // convertToFormatInternal



template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue>
void
convertToFormatInternalForDepth(const RectI & renderWindow,
                                const Image & srcImg,
                                Image & dstImg,
                                Natron::ViewerColorSpaceEnum srcColorSpace,
                                Natron::ViewerColorSpaceEnum dstColorSpace,
                                int channelForAlpha,
                                bool invert,
                                bool copyBitmap,
                                bool requiresUnpremult)
{
    int dstNComp = getElementsCountForComponents( dstImg.getComponents() );
    int srcNComp = getElementsCountForComponents( srcImg.getComponents() );
    
    switch (srcNComp) {
        case 1:
            switch (dstNComp) {
                case 3:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue,1,3>(renderWindow,srcImg, dstImg,
                                                                                        srcColorSpace,
                                                                                        dstColorSpace,
                                                                                        channelForAlpha,
                                                                                        invert,copyBitmap,requiresUnpremult);
                    break;
                case 4:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue,1,4>(renderWindow,srcImg, dstImg,
                                                                                        srcColorSpace,
                                                                                        dstColorSpace,
                                                                                        channelForAlpha,
                                                                                        invert,copyBitmap,requiresUnpremult);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        case 3:
            switch (dstNComp) {
                case 1:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue,3,1>(renderWindow,srcImg, dstImg,
                                                                                        srcColorSpace,
                                                                                        dstColorSpace,
                                                                                        channelForAlpha,
                                                                                        invert,copyBitmap,requiresUnpremult);
                    break;
                case 4:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue,3,4>(renderWindow,srcImg, dstImg,
                                                                                        srcColorSpace,
                                                                                        dstColorSpace,
                                                                                        channelForAlpha,
                                                                                        invert,copyBitmap,requiresUnpremult);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        case 4:
            switch (dstNComp) {
                case 1:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue,4,1>(renderWindow,srcImg, dstImg,
                                                                                        srcColorSpace,
                                                                                        dstColorSpace,
                                                                                        channelForAlpha,
                                                                                        invert,copyBitmap,requiresUnpremult);
                    break;
                case 3:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue,4,3>(renderWindow,srcImg, dstImg,
                                                                                        srcColorSpace,
                                                                                        dstColorSpace,
                                                                                        channelForAlpha,
                                                                                        invert,copyBitmap,requiresUnpremult);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        default:
            break;
    }

}

void
Image::convertToFormat(const RectI & renderWindow,
                       Natron::ViewerColorSpaceEnum srcColorSpace,
                       Natron::ViewerColorSpaceEnum dstColorSpace,
                       int channelForAlpha,
                       bool invert,
                       bool copyBitmap,
                       bool requiresUnpremult,
                       Natron::Image* dstImg) const
{
    assert( getBounds() == dstImg->getBounds() );

    if ( dstImg->getComponents() == getComponents() ) {
        switch ( dstImg->getBitDepth() ) {
        case eImageBitDepthByte: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                ///Same as a copy
                convertToFormatInternal_sameComps<unsigned char, unsigned char, 255, 255>(renderWindow,*this, *dstImg,
                                                                                          srcColorSpace,
                                                                                          dstColorSpace,invert,copyBitmap);
                break;
            case eImageBitDepthShort:
                convertToFormatInternal_sameComps<unsigned short, unsigned char, 65535, 255>(renderWindow,*this, *dstImg,
                                                                                             srcColorSpace,
                                                                                             dstColorSpace,invert,copyBitmap);
                break;
            case eImageBitDepthFloat:
                convertToFormatInternal_sameComps<float, unsigned char, 1, 255>(renderWindow,*this, *dstImg,
                                                                                srcColorSpace,
                                                                                dstColorSpace,invert,copyBitmap);
                break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }

        case eImageBitDepthShort: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                convertToFormatInternal_sameComps<unsigned char, unsigned short, 255, 65535>(renderWindow,*this, *dstImg,
                                                                                             srcColorSpace,
                                                                                             dstColorSpace,invert,copyBitmap);
                break;
            case eImageBitDepthShort:
                ///Same as a copy
                convertToFormatInternal_sameComps<unsigned short, unsigned short, 65535, 65535>(renderWindow,*this, *dstImg,
                                                                                                srcColorSpace,
                                                                                                dstColorSpace,invert,copyBitmap);
                break;
            case eImageBitDepthFloat:
                convertToFormatInternal_sameComps<float, unsigned short, 1, 65535>(renderWindow,*this, *dstImg,
                                                                                   srcColorSpace,
                                                                                   dstColorSpace,invert,copyBitmap);
                break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }

        case eImageBitDepthFloat: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                convertToFormatInternal_sameComps<unsigned char, float, 255, 1>(renderWindow,*this, *dstImg,
                                                                                srcColorSpace,
                                                                                dstColorSpace,invert,copyBitmap);
                break;
            case eImageBitDepthShort:
                convertToFormatInternal_sameComps<unsigned short, float, 65535, 1>(renderWindow,*this, *dstImg,
                                                                                   srcColorSpace,
                                                                                   dstColorSpace,invert,copyBitmap);
                break;
            case eImageBitDepthFloat:
                ///Same as a copy
                convertToFormatInternal_sameComps<float, float, 1, 1>(renderWindow,*this, *dstImg,
                                                                      srcColorSpace,
                                                                      dstColorSpace,invert,copyBitmap);
                break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }

        case eImageBitDepthNone:
            break;
        } // switch
    } else {
        
        switch ( dstImg->getBitDepth() ) {
        case eImageBitDepthByte: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                    convertToFormatInternalForDepth<unsigned char, unsigned char, 255, 255>(renderWindow,*this, *dstImg,
                                                                                        srcColorSpace,
                                                                                        dstColorSpace,
                                                                                        channelForAlpha,
                                                                                        invert,copyBitmap,requiresUnpremult);
                    break;
            case eImageBitDepthShort:
                    convertToFormatInternalForDepth<unsigned short, unsigned char, 65535, 255>(renderWindow,*this, *dstImg,
                                                                                           srcColorSpace,
                                                                                           dstColorSpace,
                                                                                           channelForAlpha,
                                                                                           invert,copyBitmap,requiresUnpremult);
                    break;
            case eImageBitDepthFloat:
                    convertToFormatInternalForDepth<float, unsigned char, 1, 255>(renderWindow,*this, *dstImg,
                                                                              srcColorSpace,
                                                                              dstColorSpace,
                                                                              channelForAlpha,
                                                                              invert,copyBitmap,requiresUnpremult);

                        break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }
        case eImageBitDepthShort: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                    convertToFormatInternalForDepth<unsigned char, unsigned short, 255, 65535>(renderWindow,*this, *dstImg,
                                                                                           srcColorSpace,
                                                                                           dstColorSpace,
                                                                                           channelForAlpha,
                                                                                           invert,copyBitmap,requiresUnpremult);

                        break;
            case eImageBitDepthShort:
                    convertToFormatInternalForDepth<unsigned short, unsigned short, 65535, 65535>(renderWindow,*this, *dstImg,
                                                                                              srcColorSpace,
                                                                                              dstColorSpace,
                                                                                              channelForAlpha,
                                                                                              invert,copyBitmap,requiresUnpremult);

                break;
            case eImageBitDepthFloat:
                    convertToFormatInternalForDepth<float, unsigned short, 1, 65535>(renderWindow,*this, *dstImg,
                                                                                 srcColorSpace,
                                                                                 dstColorSpace,
                                                                                 channelForAlpha,
                                                                                 invert,copyBitmap,requiresUnpremult);
                        break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }
        case eImageBitDepthFloat: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                    convertToFormatInternalForDepth<unsigned char, float, 255, 1>(renderWindow,*this, *dstImg,
                                                                              srcColorSpace,
                                                                              dstColorSpace,
                                                                              channelForAlpha,
                                                                              invert,copyBitmap,requiresUnpremult);
                        break;
            case eImageBitDepthShort:
                    convertToFormatInternalForDepth<unsigned short, float, 65535, 1>(renderWindow,*this, *dstImg,
                                                                                 srcColorSpace,
                                                                                 dstColorSpace,
                                                                                 channelForAlpha,
                                                                                 invert,copyBitmap,requiresUnpremult);

                        break;
            case eImageBitDepthFloat:
                    convertToFormatInternalForDepth<float, float, 1, 1>(renderWindow,*this, *dstImg,
                                                                    srcColorSpace,
                                                                    dstColorSpace,
                                                                    channelForAlpha,
                                                                    invert,copyBitmap,requiresUnpremult);
                    break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }

        default:
            break;
        } // switch
    }
} // convertToFormat

