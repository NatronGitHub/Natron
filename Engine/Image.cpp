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

#include "Engine/AppManager.h"
#include "Engine/ImageParams.h"
#include "Engine/Lut.h"

using namespace Natron;


ImageKey::ImageKey()
    : KeyHelper<U64>()
      , _nodeHashKey(0)
      , _time(0)
      , _mipMapLevel(0)
      , _view(0)
      , _pixelAspect(1)
{
}

ImageKey::ImageKey(U64 nodeHashKey,
                   SequenceTime time,
                   unsigned int mipMapLevel,
                   int view,
                   double pixelAspect)
    : KeyHelper<U64>()
      , _nodeHashKey(nodeHashKey)
      , _time(time)
      , _mipMapLevel(mipMapLevel)
      , _view(view)
      , _pixelAspect(pixelAspect)
{
}

void
ImageKey::fillHash(Hash64* hash) const
{
    hash->append(_nodeHashKey);
    hash->append(_mipMapLevel);
    hash->append(_time);
    hash->append(_view);
    hash->append(_pixelAspect);
}

bool
ImageKey::operator==(const ImageKey & other) const
{
    return _nodeHashKey == other._nodeHashKey &&
           _mipMapLevel == other._mipMapLevel &&
           _time == other._time &&
           _view == other._view &&
           _pixelAspect == other._pixelAspect;
}

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
    qDebug() << "render " << ret.size() << " rectangles";
    for (std::list<RectI>::const_iterator it = ret.begin(); it != ret.end(); ++it) {
        qDebug() << "rect: " << "x1= " <<  it->x1 << " , x2= " << it->x2 << " , y1= " << it->y1 << " , y2= " << it->y2;
    }
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
             const boost::shared_ptr<NonKeyParams> & params,
             const Natron::CacheAPI* cache)
    : CacheEntryHelper<unsigned char, ImageKey>(key, params, cache)
{
     ImageParams* p = dynamic_cast<ImageParams*>( params.get() );

    _components = p->getComponents();
    _bitDepth = p->getBitDepth();
    _bitmap.initialize( p->getBounds() );
    _rod = p->getRoD();
    _bounds = p->getBounds();
}

/*This constructor can be used to allocate a local Image. The deallocation should
   then be handled by the user. Note that no view number is passed in parameter
   as it is not needed.*/
Image::Image(ImageComponents components,
             const RectD & regionOfDefinition, //!< rod in canonical coordinates
             const RectI & bounds, //!< bounds in pixel coordinates
             unsigned int mipMapLevel,
             Natron::ImageBitDepth bitdepth)
    : CacheEntryHelper<unsigned char,ImageKey>()
{
    setCacheEntry(makeKey(0,0,mipMapLevel,0),
                  boost::shared_ptr<NonKeyParams>( new ImageParams( 0,
                                                                          regionOfDefinition,
                                                                          bounds,
                                                                          bitdepth,
                                                                          false,
                                                                          components,
                                                                          -1,
                                                                          0,
                                                                          std::map<int,std::vector<RangeD> >() ) ),
                  NULL);

    ImageParams* p = dynamic_cast<ImageParams*>( _params.get() );
    _components = components;
    _bitDepth = bitdepth;
    _bitmap.initialize( p->getBounds() );
    _rod = regionOfDefinition;
    _bounds = p->getBounds();
    allocateMemory(false,Natron::RAM, "");
}

#ifdef DEBUG
void
Image::onMemoryAllocated()
{
    ///fill with red, to recognize unrendered pixels
    fill(_bounds,1.,0.,0.,1.);
}

#endif // DEBUG

ImageKey  
Image::makeKey(U64 nodeHashKey,
               SequenceTime time,
               unsigned int mipMapLevel,
               int view)
{
    return ImageKey(nodeHashKey,time,mipMapLevel,view);
}

boost::shared_ptr<ImageParams>
Image::makeParams(int cost,
                  const RectD & rod,
                  unsigned int mipMapLevel,
                  bool isRoDProjectFormat,
                  ImageComponents components,
                  Natron::ImageBitDepth bitdepth,
                  int inputNbIdentity,
                  int inputTimeIdentity,
                  const std::map<int, std::vector<RangeD> > & framesNeeded)
{
    RectI bounds;

    rod.toPixelEnclosing(mipMapLevel, &bounds);

    return boost::shared_ptr<ImageParams>( new ImageParams(cost,
                                                           rod,
                                                           bounds,
                                                           bitdepth,
                                                           isRoDProjectFormat,
                                                           components,
                                                           inputNbIdentity,
                                                           inputTimeIdentity,
                                                           framesNeeded) );
}

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
    assert( (getBitDepth() == IMAGE_BYTE && sizeof(PIX) == 1) || (getBitDepth() == IMAGE_SHORT && sizeof(PIX) == 2) || (getBitDepth() == IMAGE_FLOAT && sizeof(PIX) == 4) );
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

    // now we're safe: both images contain the area in roi
    for (int y = roi.y1; y < roi.y2; ++y) {
        const PIX* src = (const PIX*)srcImg.pixelAt(roi.x1, y);
        PIX* dst = (PIX*)pixelAt(roi.x1, y);
        memcpy(dst, src, roi.width() * sizeof(PIX) * components);

        if (copyBitmap) {
            const char* srcBm = srcImg.getBitmapAt(roi.x1, y);
            char* dstBm = getBitmapAt(roi.x1, y);
            memcpy( dstBm, srcBm, roi.width() );
        }
    }
}

// code proofread and fixed by @devernay on 8/8/2014
void
Image::pasteFrom(const Natron::Image & src,
                 const RectI & srcRoi,
                 bool copyBitmap)
{
    Natron::ImageBitDepth depth = getBitDepth();

    switch (depth) {
    case IMAGE_BYTE:
        pasteFromForDepth<unsigned char>(src, srcRoi, copyBitmap);
        break;
    case IMAGE_SHORT:
        pasteFromForDepth<unsigned short>(src, srcRoi, copyBitmap);
        break;
    case IMAGE_FLOAT:
        pasteFromForDepth<float>(src, srcRoi, copyBitmap);
        break;
    case IMAGE_NONE:
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
    assert( (getBitDepth() == IMAGE_BYTE && sizeof(PIX) == 1) || (getBitDepth() == IMAGE_SHORT && sizeof(PIX) == 2) || (getBitDepth() == IMAGE_FLOAT && sizeof(PIX) == 4) );

    ImageComponents comps = getComponents();
    if (comps == ImageComponentNone) {
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
        comps == Natron::ImageComponentAlpha ? a : r, g, b, a
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
    case IMAGE_BYTE:
        fillForDepth<unsigned char, 255>(roi, r, g, b, a);
        break;
    case IMAGE_SHORT:
        fillForDepth<unsigned short, 65535>(roi, r, g, b, a);
        break;
    case IMAGE_FLOAT:
        fillForDepth<float, 1>(roi, r, g, b, a);
        break;
    case IMAGE_NONE:
        break;
    }
}

unsigned char*
Image::pixelAt(int x,
               int y)
{
    int compsCount = getElementsCountForComponents( getComponents() );

    if ( ( x >= _bounds.left() ) && ( x < _bounds.right() ) && ( y >= _bounds.bottom() ) && ( y < _bounds.top() ) ) {
        int compDataSize = getSizeOfForBitDepth( getBitDepth() ) * compsCount;

        return this->_data.writable()
               + ( y - _bounds.bottom() ) * compDataSize * _bounds.width()
               + ( x - _bounds.left() ) * compDataSize;
    } else {
        return NULL;
    }
}

const unsigned char*
Image::pixelAt(int x,
               int y) const
{
    int compsCount = getElementsCountForComponents( getComponents() );

    if ( ( x >= _bounds.left() ) && ( x < _bounds.right() ) && ( y >= _bounds.bottom() ) && ( y < _bounds.top() ) ) {
        int compDataSize = getSizeOfForBitDepth( getBitDepth() ) * compsCount;

        return this->_data.readable()
               + ( y - _bounds.bottom() ) * compDataSize * _bounds.width()
               + ( x - _bounds.left() ) * compDataSize;
    } else {
        return NULL;
    }
}

unsigned int
Image::getComponentsCount() const
{
    return getElementsCountForComponents( getComponents() );
}

bool
Image::hasEnoughDataToConvert(Natron::ImageComponents from,
                              Natron::ImageComponents to)
{
    switch (from) {
    case ImageComponentRGBA:

        return true;
    case ImageComponentRGB: {
        switch (to) {
        case ImageComponentRGBA:

            return false;
        case ImageComponentRGB:

            return true;
        case ImageComponentAlpha:

            return false;
        default:

            return false;
        }
        break;
    }
    case ImageComponentAlpha: {
        switch (to) {
        case ImageComponentRGBA:

            return false;
        case ImageComponentRGB:

            return false;
        case ImageComponentAlpha:

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
Image::getFormatString(Natron::ImageComponents comps,
                       Natron::ImageBitDepth depth)
{
    std::string s;

    switch (comps) {
    case Natron::ImageComponentRGBA:
        s += "RGBA";
        break;
    case Natron::ImageComponentRGB:
        s += "RGB";
        break;
    case Natron::ImageComponentAlpha:
        s += "Alpha";
        break;
    default:
        break;
    }
    s.append( getDepthString(depth) );

    return s;
}

std::string
Image::getDepthString(Natron::ImageBitDepth depth)
{
    std::string s;

    switch (depth) {
    case Natron::IMAGE_BYTE:
        s += "8u";
        break;
    case Natron::IMAGE_SHORT:
        s += "16u";
        break;
    case Natron::IMAGE_FLOAT:
        s += "32f";
        break;
    case Natron::IMAGE_NONE:
        break;
    }

    return s;
}

bool
Image::isBitDepthConversionLossy(Natron::ImageBitDepth from,
                                 Natron::ImageBitDepth to)
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

// code proofread and fixed by @devernay on 8/8/2014
template <typename PIX, int maxValue>
void
Image::halveRoIForDepth(const RectI & roi,
                        Natron::Image* output) const
{
    assert( (getBitDepth() == IMAGE_BYTE && sizeof(PIX) == 1) || (getBitDepth() == IMAGE_SHORT && sizeof(PIX) == 2) || (getBitDepth() == IMAGE_FLOAT && sizeof(PIX) == 4) );

    ///handle case where there is only 1 column/row
    if ( (roi.width() == 1) || (roi.height() == 1) ) {
        assert( !(roi.width() == 1 && roi.height() == 1) ); /// can't be 1x1
        halve1DImage(roi, output);

        return;
    }

    ///The source rectangle, intersected to this image region of definition in pixels
    const RectI &srcBounds = getBounds();
    const RectI &dstBounds = output->getBounds();

    // the srcRoD of the output should be enclosed in half the roi.
    // It does not have to be exactly half of the input.
    //    assert(dstRoD.x1*2 >= roi.x1 &&
    //           dstRoD.x2*2 <= roi.x2 &&
    //           dstRoD.y1*2 >= roi.y1 &&
    //           dstRoD.y2*2 <= roi.y2 &&
    //           dstRoD.width()*2 <= roi.width() &&
    //           dstRoD.height()*2 <= roi.height());
    assert( getComponents() == output->getComponents() );

    int components = getElementsCountForComponents( getComponents() );
    RectI dstRoI;
    RectI srcRoI = roi;
    srcRoI.intersect(srcBounds, &srcRoI); // intersect srcRoI with the region of definition

#if 0
    srcRoI = srcRoI.roundPowerOfTwoLargestEnclosed(1);
    dstRoI.x1 = srcRoI.x1 / 2;
    dstRoI.y1 = srcRoI.y1 / 2;
    dstRoI.x2 = srcRoI.x2 / 2;
    dstRoI.y2 = srcRoI.y2 / 2;

    // a few checks...
    // srcRoI must be inside srcBounds
    assert(srcRoI.x1 >= srcBounds.x1);
    assert(srcRoI.x2 <= srcBounds.x2);
    assert(srcRoI.y1 >= srcBounds.y1);
    assert(srcRoI.y2 <= srcBounds.y2);
    // srcRoI must be inside dstBounds*2
    assert(srcRoI.x1 >= dstBounds.x1 * 2);
    assert(srcRoI.x2 <= dstBounds.x2 * 2);
    assert(srcRoI.y1 >= dstBounds.y1 * 2);
    assert(srcRoI.y2 <= dstBounds.y2 * 2);
    // srcRoI must be equal to dstRoI*2
    assert(srcRoI.x1 == dstRoI.x1 * 2);
    assert(srcRoI.x2 == dstRoI.x2 * 2);
    assert(srcRoI.y1 == dstRoI.y1 * 2);
    assert(srcRoI.y2 == dstRoI.y2 * 2);
    assert(srcRoI.width() == dstRoI.width() * 2);
    assert(srcRoI.height() == dstRoI.height() * 2);
#else
    dstRoI.x1 = std::floor(srcRoI.x1 / 2.);
    dstRoI.y1 = std::floor(srcRoI.y1 / 2.);
    dstRoI.x2 = std::ceil(srcRoI.x2 / 2.);
    dstRoI.y2 = std::ceil(srcRoI.y2 / 2.);
#endif

    int srcRoIWidth = srcRoI.width();
    //int srcRoIHeight = srcRoI.height();
    int dstRoIWidth = dstRoI.width();
    int dstRoIHeight = dstRoI.height();
    const PIX* src = (const PIX*)pixelAt(srcRoI.x1, srcRoI.y1);
    PIX* dst = (PIX*)output->pixelAt(dstRoI.x1, dstRoI.y1);
    int srcRowSize = srcBounds.width() * components;
    int dstRowSize = dstBounds.width() * components;

    // Loop with sliding pointers:
    // at each loop iteration, add the step to the pointer, minus what was done during previous iteration.
    // This is the *good* way to code it, let the optimizer do the rest!
    // Please don't change this, and don't remove the comments.
    for (int y = 0; y < dstRoIHeight;
         ++y,
         src += (srcRowSize + srcRowSize) - srcRoIWidth * components, // two rows minus what was done on previous iteration
         dst += (dstRowSize) - dstRoIWidth * components) { // one row minus what was done on previous iteration
        for (int x = 0; x < dstRoIWidth;
             ++x,
             src += (components + components) - components, // two pixels minus what was done on previous iteration
             dst += (components) - components) { // one pixel minus what was done on previous iteration
#if 0
            assert(dstBounds.x1 <= dstRoI.x1 + x && dstRoI.x1 + x < dstBounds.x2);
            assert(dstBounds.y1 <= dstRoI.y1 + y && dstRoI.y1 + y < dstBounds.y2);
            assert( dst == (PIX*)output->pixelAt(dstRoI.x1 + x, dstRoI.y1 + y) );
            assert(srcBounds.x1 <= srcRoI.x1 + 2 * x && srcRoI.x1 + 2 * x < srcBounds.x2);
            assert(srcBounds.y1 <= srcRoI.y1 + 2 * y && srcRoI.y1 + 2 * y < srcBounds.y2);
            assert( src == (const PIX*)pixelAt(srcRoI.x1 + 2 * x, srcRoI.y1 + 2 * y) );

#endif
            for (int k = 0; k < components; ++k, ++dst, ++src) {
                *dst = PIX( (float)( *src +
                                     *(src + components) +
                                     *(src + srcRowSize) +
                                     *(src + srcRowSize  + components) ) / 4. );
            }
        }
    }
} // halveRoIForDepth

// code proofread and fixed by @devernay on 8/8/2014
void
Image::halveRoI(const RectI & roi,
                Natron::Image* output) const
{
    switch ( getBitDepth() ) {
    case IMAGE_BYTE:
        halveRoIForDepth<unsigned char,255>(roi, output);
        break;
    case IMAGE_SHORT:
        halveRoIForDepth<unsigned short,65535>(roi, output);
        break;
    case IMAGE_FLOAT:
        halveRoIForDepth<float,1>(roi, output);
        break;
    case IMAGE_NONE:
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
    assert(dstBounds.x1 * 2 == roi.x1 &&
           dstBounds.y1 * 2 == roi.y1 &&
           (
               dstBounds.x2 * 2 == roi.x2 || // we halve in only 1 dimension
               dstBounds.y2 * 2 == roi.y2)
           );

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
    case IMAGE_BYTE:
        halve1DImageForDepth<unsigned char, 255>(roi, output);
        break;
    case IMAGE_SHORT:
        halve1DImageForDepth<unsigned short, 65535>(roi, output);
        break;
    case IMAGE_FLOAT:
        halve1DImageForDepth<float, 1>(roi, output);
        break;
    case IMAGE_NONE:
        break;
    }
}

// code proofread and fixed by @devernay on 8/8/2014
void
Image::downscaleMipMap(const RectI & roi,
                       unsigned int fromLevel,
                       unsigned int toLevel,
                       Natron::Image* output) const
{
    ///You should not call this function with a level equal to 0.
    assert(toLevel >  fromLevel);

    assert(_bounds.x1 <= roi.x1 && roi.x2 <= _bounds.x2 &&
           _bounds.y1 <= roi.y1 && roi.y2 <= _bounds.y2);
    RectD roiCanonical;
    roi.toCanonical(fromLevel, getRoD(), &roiCanonical);
    RectI dstRoI;
    roiCanonical.toPixelEnclosing(toLevel, &dstRoI);

    std::auto_ptr<Natron::Image> tmpImg( new Natron::Image( getComponents(), getRoD(), dstRoI, toLevel, getBitDepth() ) );

    buildMipMapLevel( roi, toLevel - fromLevel, tmpImg.get() );

    // check that the downscaled mipmap is inside the output image (it may not be equal to it)
    assert(dstRoI.x1 >= output->getBounds().x1);
    assert(dstRoI.x2 <= output->getBounds().x2);
    assert(dstRoI.y1 >= output->getBounds().y1);
    assert(dstRoI.y2 <= output->getBounds().y2);

    ///Now copy the result of tmpImg into the output image
    output->pasteFrom(*tmpImg, dstRoI, false);
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
    assert( (getBitDepth() == IMAGE_BYTE && sizeof(PIX) == 1) || (getBitDepth() == IMAGE_SHORT && sizeof(PIX) == 2) || (getBitDepth() == IMAGE_FLOAT && sizeof(PIX) == 4) );

    ///You should not call this function with a level equal to 0.
    assert(fromLevel > toLevel);

    assert(roi.x1 <= _bounds.x1 && _bounds.x2 <= roi.x2 &&
           roi.y1 <= _bounds.y1 && _bounds.y2 <= roi.y2);

    ///The source rectangle, intersected to this image region of definition in pixels
    RectD roiCanonical;
    roi.toCanonical(fromLevel, getRoD(), &roiCanonical);
    RectI dstRoi;
    roiCanonical.toPixelEnclosing(toLevel, &dstRoi);

    const RectI & srcRoi = roi;

    dstRoi.intersect(output->getBounds(), &dstRoi); //output may be a bit smaller than the upscaled RoI
    int scale = 1 << (fromLevel - toLevel);

    assert( output->getComponents() == getComponents() );
    int components = getElementsCountForComponents( getComponents() );
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
        assert(0 < ycount && ycount <= scale);
        int xi = srcRoi.x1;
        int xcount = 0; // how many pixels should be filled
        const PIX * srcPix = srcLineStart;
        PIX * dstPixFirst = dstLineBatchStart;
        // fill the first line
        for (int xo = dstRoi.x1; xo < dstRoi.x2; ++xi, srcPix += components, xo += xcount, dstPixFirst += xcount * components) {
            xcount = scale + xo - xi * scale;
            assert(0 < xcount && xcount <= scale);
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
    case IMAGE_BYTE:
        upscaleMipMapForDepth<unsigned char, 255>(roi, fromLevel, toLevel, output);
        break;
    case IMAGE_SHORT:
        upscaleMipMapForDepth<unsigned short, 65535>(roi, fromLevel, toLevel, output);
        break;
    case IMAGE_FLOAT:
        upscaleMipMapForDepth<float,1>(roi, fromLevel, toLevel, output);
        break;
    case IMAGE_NONE:
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
    assert( (getBitDepth() == IMAGE_BYTE && sizeof(PIX) == 1) || (getBitDepth() == IMAGE_SHORT && sizeof(PIX) == 2) || (getBitDepth() == IMAGE_FLOAT && sizeof(PIX) == 4) );

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
        halveRoI(srcRoi, output);

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
    case IMAGE_BYTE:
        scaleBoxForDepth<unsigned char>(roi, output);
        break;
    case IMAGE_SHORT:
        scaleBoxForDepth<unsigned short>(roi, output);
        break;
    case IMAGE_FLOAT:
        scaleBoxForDepth<float>(roi, output);
        break;
    case IMAGE_NONE:
        break;
    }
}

// code proofread and fixed by @devernay on 8/8/2014
void
Image::buildMipMapLevel(const RectI & roi,
                        unsigned int level,
                        Natron::Image* output) const
{
    ///The last mip map level we will make with closestPo2
    RectI lastLevelRoI = roi.downscalePowerOfTwoSmallestEnclosing(level);

    ///The output image must contain the last level roi
    assert( output->getBounds().contains(lastLevelRoI) );

    assert( output->getComponents() == getComponents() );

    if (level == 0) {
        ///Just copy the roi and return
        output->pasteFrom(*this, roi);

        return;
    }

    const Natron::Image* srcImg = this;
    Natron::Image* dstImg = NULL;
    bool mustFreeSrc = false;
    RectI previousRoI = roi;
    ///Build all the mipmap levels until we reach the one we are interested in
    for (unsigned int i = 1; i <= level; ++i) {
        ///Halve the smallest enclosing po2 rect as we need to render a minimum of the renderWindow
        RectI halvedRoI = roi.downscalePowerOfTwoSmallestEnclosing(i);

        ///Allocate an image with half the size of the source image
        dstImg = new Natron::Image( getComponents(), getRoD(), halvedRoI, 0, getBitDepth() );

        ///Half the source image into dstImg.
        ///We pass the closestPo2 roi which might not be the entire size of the source image
        ///If the source image'sroi was originally a po2.
        srcImg->halveRoI(previousRoI, dstImg);

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
    output->pasteFrom( *srcImg, srcImg->getBounds() );

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
Image::clearBitmap()
{
    _bitmap.clear();
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
lutFromColorspace(Natron::ViewerColorSpace cs)
{
    const Natron::Color::Lut* lut;

    switch (cs) {
    case Natron::sRGB:
        lut = Natron::Color::LutManager::sRGBLut();
        break;
    case Natron::Rec709:
        lut = Natron::Color::LutManager::Rec709Lut();
        break;
    case Natron::Linear:
    default:
        lut = 0;
        break;
    }
    if (lut) {
        lut->validate();
    }

    return lut;
}

///Fast version when components are the same
template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue>
void
convertToFormatInternal_sameComps(const RectI & renderWindow,
                                  const Image & srcImg,
                                  Image & dstImg,
                                  Natron::ViewerColorSpace srcColorSpace,
                                  Natron::ViewerColorSpace dstColorSpace,
                                  bool invert,
                                  bool copyBitmap)
{
    const RectI & r = srcImg.getBounds();
    RectI intersection;

    if ( !renderWindow.intersect(r, &intersection) ) {
        return;
    }

    Natron::ImageBitDepth dstDepth = dstImg.getBitDepth();
    Natron::ImageBitDepth srcDepth = srcImg.getBitDepth();
    int nComp = (int)srcImg.getComponentsCount();
    const Natron::Color::Lut* srcLut = lutFromColorspace(srcColorSpace);
    const Natron::Color::Lut* dstLut = lutFromColorspace(dstColorSpace);

    ///no colorspace conversion applied when luts are the same
    if (srcLut == dstLut) {
        srcLut = dstLut = 0;
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
                            if (srcDepth == IMAGE_BYTE) {
                                pixFloat = srcLut->fromColorSpaceUint8ToLinearFloatFast(srcPixels[k]);
                            } else if (srcDepth == IMAGE_SHORT) {
                                pixFloat = srcLut->fromColorSpaceUint16ToLinearFloatFast(srcPixels[k]);
                            } else {
                                pixFloat = srcLut->fromColorSpaceFloatToLinearFloat(srcPixels[k]);
                            }
                        } else {
                            pixFloat = convertPixelDepth<SRCPIX, float>(srcPixels[k]);
                        }


                        DSTPIX pix;
                        if (dstDepth == IMAGE_BYTE) {
                            ///small increase in perf we use Luts. This should be anyway the most used case.
                            error[k] = (error[k] & 0xff) + ( dstLut ? dstLut->toColorSpaceUint8xxFromLinearFloatFast(pixFloat) :
                                                             Color::floatToInt<0xff01>(pixFloat) );
                            pix = error[k] >> 8;
                        } else if (dstDepth == IMAGE_SHORT) {
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
            const char* srcBitmap = srcImg.getBitmapAt(intersection.x1, intersection.y1 + y);
            char* dstBitmap = dstImg.getBitmapAt(intersection.x1, intersection.y1 + y);
            memcpy( dstBitmap, srcBitmap, intersection.width() );
        }
    }
} // convertToFormatInternal_sameComps

template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue>
void
convertToFormatInternal(const RectI & renderWindow,
                        const Image & srcImg,
                        Image & dstImg,
                        Natron::ViewerColorSpace srcColorSpace,
                        Natron::ViewerColorSpace dstColorSpace,
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

    Natron::ImageComponents dstComp = dstImg.getComponents();
    Natron::ImageBitDepth dstDepth = dstImg.getBitDepth();
    Natron::ImageBitDepth srcDepth = srcImg.getBitDepth();
    bool sameBitDepth = srcImg.getBitDepth() == dstImg.getBitDepth();
    int dstNComp = getElementsCountForComponents(dstComp);
    int srcNComp = getElementsCountForComponents( srcImg.getComponents() );

    ///special case comp == alpha && channelForAlpha = -1 clear out the mask
    if ( (dstComp == Natron::ImageComponentAlpha) && (channelForAlpha == -1) ) {
        DSTPIX* dstPixels = (DSTPIX*)dstImg.pixelAt(intersection.x1, intersection.y1);

        for ( int y = 0; y < intersection.height();
              ++y, dstPixels += (r.width() * dstNComp) ) {
            std::fill(dstPixels, dstPixels + intersection.width() * dstNComp, 0.);
            if (copyBitmap) {
                const char* srcBitmap = srcImg.getBitmapAt(intersection.x1, intersection.y1 + y);
                char* dstBitmap = dstImg.getBitmapAt(intersection.x1, intersection.y1 + y);
                memcpy( dstBitmap, srcBitmap, intersection.width() );
            }
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
                
                
                if (dstComp == Natron::ImageComponentAlpha) {
                    ///If we're converting to alpha, we just have to handle pixel depth conversion
                    assert(channelForAlpha < srcNComp && channelForAlpha >= 0);
                    *dstPixels = !sameBitDepth ? convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[channelForAlpha])
                                 : srcPixels[channelForAlpha];
                    if (invert) {
                        *dstPixels = dstMaxValue - *dstPixels;
                    }
                } else {
                    
                    if (srcImg.getComponents() == Natron::ImageComponentAlpha) {
                        ///If we're converting from alpha, we just fill all color channels with the alpha value
                        if (dstComp == Natron::ImageComponentRGB) {
                            for (int k = 0; k < dstNComp; ++k) {
                                DSTPIX pix = convertPixelDepth<SRCPIX, DSTPIX>(*srcPixels);
                                dstPixels[k] = invert ? dstMaxValue - pix : pix;
                            }
                        } else {
                            assert(dstComp == Natron::ImageComponentRGBA);
                            for (int k = 0; k < dstNComp - 1; ++k) {
                                dstPixels[k] = invert ? dstMaxValue : 0;
                            }
                            DSTPIX pix = convertPixelDepth<SRCPIX, DSTPIX>(*srcPixels);
                            dstPixels[3] = invert ? dstMaxValue : pix;
                        }
                    } else {
                        ///In this case we've RGB or RGBA input and outputs
                        assert(srcImg.getComponents() != dstImg.getComponents());
                        
                        bool unpremultChannel = srcImg.getComponents() == Natron::ImageComponentRGBA &&
                        dstImg.getComponents() == Natron::ImageComponentRGB &&
                        requiresUnpremult;
                        
                        ///This is only set if unpremultChannel is true
                        float alphaForUnPremult;
                        if (unpremultChannel) {
                            alphaForUnPremult = convertPixelDepth<SRCPIX, float>(srcPixels[dstNComp - 1]);
                        } else {
                            alphaForUnPremult = 0.;
                        }
                        
                        for (int k = 0; k < dstNComp; ++k) {
                            if (k < 3) {
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
                                        if (srcDepth == IMAGE_BYTE) {
                                            pixFloat = srcLut->fromColorSpaceUint8ToLinearFloatFast(srcPixels[k]);
                                        } else if (srcDepth == IMAGE_SHORT) {
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
                                if (dstDepth == IMAGE_BYTE) {
                                    error[k] = (error[k] & 0xff) + ( dstLut ? dstLut->toColorSpaceUint8xxFromLinearFloatFast(pixFloat) :
                                                                    Color::floatToInt<0xff01>(pixFloat) );
                                    pix = error[k] >> 8;
                                } else if (dstDepth == IMAGE_SHORT) {
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
                                
                            } else {
                                ///For alpha channel, fill with 1, we reach here only if converting RGB-->RGBA
                                DSTPIX pix = convertPixelDepth<float, DSTPIX>(1.f);
                                dstPixels[k] = invert ? dstMaxValue - pix : pix;
                            }
                        }
                    }
                }

                if (backward) {
                    --x;
                    srcPixels -= srcNComp;
                    dstPixels -= dstNComp;
                } else {
                    ++x;
                    srcPixels += srcNComp;
                    dstPixels += dstNComp;
                }
            }
            srcPixels = srcStart - srcNComp;
            dstPixels = dstStart - dstNComp;
        }

        ///Copy bitmap if needed
        if (copyBitmap) {
            const char* srcBitmap = srcImg.getBitmapAt(intersection.x1, intersection.y1 + y);
            char* dstBitmap = dstImg.getBitmapAt(intersection.x1, intersection.y1 + y);
            memcpy( dstBitmap, srcBitmap, intersection.width() );
        }
    }
} // convertToFormatInternal

void
Image::convertToFormat(const RectI & renderWindow,
                       Natron::ViewerColorSpace srcColorSpace,
                       Natron::ViewerColorSpace dstColorSpace,
                       int channelForAlpha,
                       bool invert,
                       bool copyBitmap,
                       bool requiresUnpremult,
                       Natron::Image* dstImg) const
{
    assert( getBounds() == dstImg->getBounds() );

    if ( dstImg->getComponents() == getComponents() ) {
        switch ( dstImg->getBitDepth() ) {
        case IMAGE_BYTE: {
            switch ( getBitDepth() ) {
            case IMAGE_BYTE:
                ///Same as a copy
                convertToFormatInternal_sameComps<unsigned char, unsigned char, 255, 255>(renderWindow,*this, *dstImg,
                                                                                          srcColorSpace,
                                                                                          dstColorSpace,invert,copyBitmap);
                break;
            case IMAGE_SHORT:
                convertToFormatInternal_sameComps<unsigned short, unsigned char, 65535, 255>(renderWindow,*this, *dstImg,
                                                                                             srcColorSpace,
                                                                                             dstColorSpace,invert,copyBitmap);
                break;
            case IMAGE_FLOAT:
                convertToFormatInternal_sameComps<float, unsigned char, 1, 255>(renderWindow,*this, *dstImg,
                                                                                srcColorSpace,
                                                                                dstColorSpace,invert,copyBitmap);
                break;
            case IMAGE_NONE:
                break;
            }
            break;
        }

        case IMAGE_SHORT: {
            switch ( getBitDepth() ) {
            case IMAGE_BYTE:
                convertToFormatInternal_sameComps<unsigned char, unsigned short, 255, 65535>(renderWindow,*this, *dstImg,
                                                                                             srcColorSpace,
                                                                                             dstColorSpace,invert,copyBitmap);
                break;
            case IMAGE_SHORT:
                ///Same as a copy
                convertToFormatInternal_sameComps<unsigned short, unsigned short, 65535, 65535>(renderWindow,*this, *dstImg,
                                                                                                srcColorSpace,
                                                                                                dstColorSpace,invert,copyBitmap);
                break;
            case IMAGE_FLOAT:
                convertToFormatInternal_sameComps<float, unsigned short, 1, 65535>(renderWindow,*this, *dstImg,
                                                                                   srcColorSpace,
                                                                                   dstColorSpace,invert,copyBitmap);
                break;
            case IMAGE_NONE:
                break;
            }
            break;
        }

        case IMAGE_FLOAT: {
            switch ( getBitDepth() ) {
            case IMAGE_BYTE:
                convertToFormatInternal_sameComps<unsigned char, float, 255, 1>(renderWindow,*this, *dstImg,
                                                                                srcColorSpace,
                                                                                dstColorSpace,invert,copyBitmap);
                break;
            case IMAGE_SHORT:
                convertToFormatInternal_sameComps<unsigned short, float, 65535, 1>(renderWindow,*this, *dstImg,
                                                                                   srcColorSpace,
                                                                                   dstColorSpace,invert,copyBitmap);
                break;
            case IMAGE_FLOAT:
                ///Same as a copy
                convertToFormatInternal_sameComps<float, float, 1, 1>(renderWindow,*this, *dstImg,
                                                                      srcColorSpace,
                                                                      dstColorSpace,invert,copyBitmap);
                break;
            case IMAGE_NONE:
                break;
            }
            break;
        }

        case IMAGE_NONE:
            break;
        } // switch
    } else {
        switch ( dstImg->getBitDepth() ) {
        case IMAGE_BYTE: {
            switch ( getBitDepth() ) {
            case IMAGE_BYTE:
                convertToFormatInternal<unsigned char, unsigned char, 255, 255>(renderWindow,*this, *dstImg,
                                                                                srcColorSpace,
                                                                                dstColorSpace,
                                                                                channelForAlpha,
                                                                                invert,copyBitmap,requiresUnpremult);
                break;
            case IMAGE_SHORT:
                convertToFormatInternal<unsigned short, unsigned char, 65535, 255>(renderWindow,*this, *dstImg,
                                                                                   srcColorSpace,
                                                                                   dstColorSpace,
                                                                                   channelForAlpha,
                                                                                   invert,copyBitmap,requiresUnpremult);
                break;
            case IMAGE_FLOAT:
                convertToFormatInternal<float, unsigned char, 1, 255>(renderWindow,*this, *dstImg,
                                                                      srcColorSpace,
                                                                      dstColorSpace,
                                                                      channelForAlpha,
                                                                      invert,copyBitmap,requiresUnpremult);
                break;
            case IMAGE_NONE:
                break;
            }
            break;
        }
        case IMAGE_SHORT: {
            switch ( getBitDepth() ) {
            case IMAGE_BYTE:
                convertToFormatInternal<unsigned char, unsigned short, 255, 65535>(renderWindow,*this, *dstImg,
                                                                                   srcColorSpace,
                                                                                   dstColorSpace,
                                                                                   channelForAlpha,
                                                                                   invert,copyBitmap,requiresUnpremult);
                break;
            case IMAGE_SHORT:
                convertToFormatInternal<unsigned short, unsigned short, 65535, 65535>(renderWindow,*this, *dstImg,
                                                                                      srcColorSpace,
                                                                                      dstColorSpace,channelForAlpha, invert,copyBitmap,requiresUnpremult);
                break;
            case IMAGE_FLOAT:
                convertToFormatInternal<float, unsigned short, 1, 65535>(renderWindow,*this, *dstImg,
                                                                         srcColorSpace,
                                                                         dstColorSpace,
                                                                         channelForAlpha,
                                                                         invert,copyBitmap,requiresUnpremult);
                break;
            case IMAGE_NONE:
                break;
            }
            break;
        }
        case IMAGE_FLOAT: {
            switch ( getBitDepth() ) {
            case IMAGE_BYTE:
                convertToFormatInternal<unsigned char, float, 255, 1>(renderWindow,*this, *dstImg,
                                                                      srcColorSpace,
                                                                      dstColorSpace, channelForAlpha,
                                                                      invert,copyBitmap,requiresUnpremult);
                break;
            case IMAGE_SHORT:
                convertToFormatInternal<unsigned short, float, 65535, 1>(renderWindow,*this, *dstImg,
                                                                         srcColorSpace,
                                                                         dstColorSpace, channelForAlpha,
                                                                         invert,copyBitmap,requiresUnpremult);
                break;
            case IMAGE_FLOAT:
                convertToFormatInternal<float, float, 1, 1>(renderWindow,*this, *dstImg,
                                                            srcColorSpace,
                                                            dstColorSpace, channelForAlpha,
                                                            invert,copyBitmap,requiresUnpremult);
                break;
            case IMAGE_NONE:
                break;
            }
            break;
        }

        default:
            break;
        } // switch
    }
} // convertToFormat

