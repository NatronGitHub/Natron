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

#include <QDebug>

#include "Engine/AppManager.h"
#include "Engine/ImageParams.h"

using namespace Natron;



ImageKey::ImageKey()
: KeyHelper<U64>()
, _nodeHashKey(0)
, _time(0)
, _mipMapLevel(0)
, _view(0)
, _pixelAspect(1)
{}


ImageKey::ImageKey(U64 nodeHashKey,
                   SequenceTime time,
                   unsigned int mipMapLevel,
                   int view,
                   Natron::ImageBitDepth bitdepth,
                   double pixelAspect)
: KeyHelper<U64>()
, _nodeHashKey(nodeHashKey)
, _time(time)
, _view(view)
, _bitdepth(bitdepth)
, _pixelAspect(pixelAspect)
{ _mipMapLevel = mipMapLevel; }

void ImageKey::fillHash(Hash64* hash) const {
    hash->append(_nodeHashKey);
    hash->append(_mipMapLevel);
    hash->append(_time);
    hash->append(_view);
    hash->append(_bitdepth);
    hash->append(_pixelAspect);
}



bool ImageKey::operator==(const ImageKey& other) const {
    return _nodeHashKey == other._nodeHashKey &&
    _mipMapLevel == other._mipMapLevel &&
    _time == other._time &&
    _view == other._view &&
    _bitdepth == other._bitdepth &&
    _pixelAspect == other._pixelAspect;
    
}




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
        qDebug() << "rect: " << "x1= "<<  it->x1 << " , x2= "<< it->x2 << " , y1= " << it->y1 << " , y2= " << it->y2;
    }
    return ret;
}

const RectI& Image::getPixelRoD() const
{
    return _pixelRod;
}

const RectI& Image::getRoD() const
{
    return _rod;
}

void Natron::Bitmap::markForRendered(const RectI& roi){
    for (int i = roi.bottom(); i < roi.top();++i) {
        char* buf = &_map[(i-_rod.bottom())*_rod.width() + (roi.left() - _rod.left())];
        memset(buf, 1, roi.width());
    }
}

const char* Natron::Bitmap::getBitmapAt(int x,int y) const
{
    if (x >= _rod.left() && x < _rod.right() && y >= _rod.bottom() && y < _rod.top()) {
        return _map + (y - _rod.bottom()) * _rod.width() + (x - _rod.left());
    } else {
        return NULL;
    }
}

char* Natron::Bitmap::getBitmapAt(int x,int y)
{
    if (x >= _rod.left() && x < _rod.right() && y >= _rod.bottom() && y < _rod.top()) {
        return _map + (y - _rod.bottom()) * _rod.width() + (x - _rod.left());
    } else {
        return NULL;
    }
}

Image::Image(const ImageKey& key,const boost::shared_ptr<const NonKeyParams>&  params,bool restore,const std::string& path):
CacheEntryHelper<unsigned char,ImageKey>(key,params,restore,path)
{
    const ImageParams* p = dynamic_cast<const ImageParams*>(params.get());
    _components = p->getComponents();
    _bitmap.initialize(p->getPixelRoD());
    _rod = p->getRoD();
    _pixelRod = p->getPixelRoD();
    
#ifdef NATRON_DEBUG
    ///fill with red, to recognize unrendered pixels
    fill(_pixelRod,1.,0.,0.,1.);
#endif
}

/*This constructor can be used to allocate a local Image. The deallocation should
 then be handled by the user. Note that no view number is passed in parameter
 as it is not needed.*/
Image::Image(ImageComponents components,const RectI& regionOfDefinition,unsigned int mipMapLevel,Natron::ImageBitDepth bitdepth)
: CacheEntryHelper<unsigned char,ImageKey>(makeKey(0,0,mipMapLevel,bitdepth,0),
            boost::shared_ptr<const NonKeyParams>(new ImageParams(0,
                                                regionOfDefinition,
                                                regionOfDefinition.downscalePowerOfTwoSmallestEnclosing(mipMapLevel),
                                                bitdepth,
                                                false ,
                                                components,
                                                -1,
                                                0,
                                                std::map<int,std::vector<RangeD> >())),
                                            false,"")
{
    // NOTE: before removing the following assert, please explain why an empty image may happen
    assert(!regionOfDefinition.isNull());
    
    const ImageParams* p = dynamic_cast<const ImageParams*>(_params.get());
    _components = components;
    _bitmap.initialize(p->getPixelRoD());
    _rod = regionOfDefinition;
    _pixelRod = p->getPixelRoD();
}

ImageKey Image::makeKey(U64 nodeHashKey,
                        SequenceTime time,
                        unsigned int mipMapLevel,
                        Natron::ImageBitDepth bitdepth,
                        int view){
    return ImageKey(nodeHashKey,time,mipMapLevel,view,bitdepth);
}

boost::shared_ptr<ImageParams> Image::makeParams(int cost,const RectI& rod,unsigned int mipMapLevel,
                                                 bool isRoDProjectFormat,ImageComponents components,
                                                 Natron::ImageBitDepth bitdepth,
                                                 int inputNbIdentity,int inputTimeIdentity,
                                                 const std::map<int, std::vector<RangeD> >& framesNeeded) {
    return boost::shared_ptr<ImageParams>(new ImageParams(cost,rod,rod.downscalePowerOfTwoSmallestEnclosing(mipMapLevel)
                                                          ,bitdepth,isRoDProjectFormat,components,
                                                          inputNbIdentity,inputTimeIdentity,framesNeeded));
}

template<typename PIX>
void copyInternal(const Image& srcImg,Image& dstImg,const RectI& dstRoD,int elemCount,const RectI& renderWindow,bool copyBitmap)
{
    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        const PIX* src = (const PIX*)srcImg.pixelAt(dstRoD.x1, y);
        PIX* dst = (PIX*)dstImg.pixelAt(renderWindow.x1, y);
        memcpy(dst, src, renderWindow.width() * sizeof(PIX) * elemCount);
        
        if (copyBitmap) {
            const char* srcBm = srcImg.getBitmapAt(dstRoD.x1, y);
            char* dstBm = dstImg.getBitmapAt(renderWindow.x1, y);
            memcpy(dstBm, srcBm, renderWindow.width());
        }
        
    }

}

void Natron::Image::copy(const Natron::Image& other,const RectI& roi,bool copyBitmap)
{
    ///Cannot copy images with different bit depth, this is not the purpose of this function.
    ///@see convert
    assert(getBitDepth() == other.getBitDepth());
    
    
    // NOTE: before removing the following asserts, please explain why an empty image may happen
    const RectI& srcRoD = getPixelRoD();
    const RectI& dstRoD = other.getPixelRoD();
    
    assert(!srcRoD.isNull());
    assert(!dstRoD.isNull());
    RectI intersection;
    bool doInteresect = srcRoD.intersect(dstRoD, &intersection);
    if (!doInteresect) {
        return;
    }
    
    if (!roi.intersect(intersection, &intersection)) {
        return;
    }
    
    Natron::ImageBitDepth depth = getBitDepth();
    assert(getComponents() == other.getComponents());
    int components = getElementsCountForComponents(getComponents());
    switch (depth) {
        case IMAGE_BYTE:
            copyInternal<unsigned char>(other, *this, dstRoD, components, intersection, copyBitmap);
            break;
        case IMAGE_SHORT:
            copyInternal<unsigned short>(other, *this, dstRoD, components, intersection, copyBitmap);
            break;
        case IMAGE_FLOAT:
            copyInternal<float>(other, *this, dstRoD, components, intersection, copyBitmap);
            break;
        default:
            break;
    }
}

template <typename PIX,int maxValue>
void fillInternal(Image& img,const RectI& rect,int rowElems,Natron::ImageComponents comps,float r,float g,float b,float a)
{
    
    float fillValue[4] = {r,g,b,a};

    int nComps = getElementsCountForComponents(comps);

    PIX* dst = (PIX*)img.pixelAt(rect.x1, rect.y1);
    for (int i = 0; i < rect.height();++i,dst += (rowElems - rect.width() * nComps)) {
        for (int j = 0; j < rect.width();++j,dst+=nComps) {
            for (int k = 0; k < nComps; ++k) {
                if (comps == Natron::ImageComponentAlpha) {
                    dst[k] = a * maxValue;
                } else {
                    dst[k] = fillValue[k] * maxValue;
                }
            }
        }
    }
}

void Natron::Image::fill(const RectI& rect,float r,float g,float b,float a) {
    ImageComponents comps = getComponents();
    if (comps == ImageComponentNone) {
        return;
    }
    
    int rowElems = (int)getRowElements();
    switch (getBitDepth()) {
        case IMAGE_BYTE:
            fillInternal<unsigned char, 255>(*this, rect, rowElems, comps, r, g, b, a);
            break;
        case IMAGE_SHORT:
            fillInternal<unsigned short, 65535>(*this, rect, rowElems, comps, r, g, b, a);
            break;
        case IMAGE_FLOAT:
            fillInternal<float, 1>(*this, rect, rowElems, comps, r, g, b, a);
            break;

        default:
            break;
    }
}

unsigned char* Image::pixelAt(int x,int y){
    int compsCount = getElementsCountForComponents(getComponents());
    if (x >= _pixelRod.left() && x < _pixelRod.right() && y >= _pixelRod.bottom() && y < _pixelRod.top()) {
        int compDataSize = getSizeOfForBitDepth(getBitDepth()) * compsCount;
        return this->_data.writable()
        + (y - _pixelRod.bottom()) * compDataSize * _pixelRod.width()
        + (x - _pixelRod.left()) * compDataSize;
    } else {
        return NULL;
    }
}

const unsigned char* Image::pixelAt(int x,int y) const {
    int compsCount = getElementsCountForComponents(getComponents());
    if (x >= _pixelRod.left() && x < _pixelRod.right() && y >= _pixelRod.bottom() && y < _pixelRod.top()) {
        int compDataSize = getSizeOfForBitDepth(getBitDepth()) * compsCount;
        return this->_data.readable()
        + (y - _pixelRod.bottom()) * compDataSize * _pixelRod.width()
        + (x - _pixelRod.left()) * compDataSize;
    } else {
        return NULL;
    }
}

unsigned int Image::getComponentsCount() const
{
    return getElementsCountForComponents(getComponents());
}

unsigned int Image::getRowElements() const
{
    return getComponentsCount() * _pixelRod.width();
}

template <typename PIX,int maxValue>
void halveRoIInternal(const Image& srcImg,const RectI& srcRoD,Image& dstImg,const RectI& dstRoD,
                      const RectI& srcRoI,const RectI& dstRoI,int components)
{
    int srcRoIWidth = srcRoI.width();
    //int srcRoIHeight = srcRoI.height();
    int dstRoIWidth = dstRoI.width();
    int dstRoIHeight = dstRoI.height();
    const PIX* src = (const PIX*)srcImg.pixelAt(srcRoI.x1, srcRoI.y1);
    PIX* dst = (PIX*)dstImg.pixelAt(dstRoI.x1, dstRoI.y1);
    
    int srcRowSize = srcRoD.width() * components;
    int dstRowSize = dstRoD.width() * components;
    
    // Loop with sliding pointers:
    // at each loop iteration, add the step to the pointer, minus what was done during previous iteration.
    // This is the *good* way to code it, let the optimizer do the rest!
    // Please don't change this, and don't remove the comments.
    for (int y = 0; y < dstRoIHeight;
         ++y,
         src += (srcRowSize+srcRowSize) - srcRoIWidth*components, // two rows minus what was done on previous iteration
         dst += (dstRowSize) - dstRoIWidth*components) { // one row minus what was done on previous iteration
        for (int x = 0; x < dstRoIWidth;
             ++x,
             src += (components+components) - components, // two pixels minus what was done on previous iteration
             dst += (components) - components) { // one pixel minus what was done on previous iteration
            assert(dstRoD.x1 <= dstRoI.x1+x && dstRoI.x1+x < dstRoD.x2);
            assert(dstRoD.y1 <= dstRoI.y1+y && dstRoI.y1+y < dstRoD.y2);
            assert(dst == (PIX*)dstImg.pixelAt(dstRoI.x1+x, dstRoI.y1+y));
            assert(srcRoD.x1 <= srcRoI.x1+2*x && srcRoI.x1+2*x < srcRoD.x2);
            assert(srcRoD.y1 <= srcRoI.y1+2*y && srcRoI.y1+2*y < srcRoD.y2);
            assert(src == (const PIX*)srcImg.pixelAt(srcRoI.x1+2*x, srcRoI.y1+2*y));
            for (int k = 0; k < components; ++k, ++dst, ++src) {
                *dst = PIX((float)(*src +
                        *(src + components) +
                        *(src + srcRowSize) +
                        *(src + srcRowSize  + components)) / 4.);
            }
        }
    }

}

void Image::halveRoI(const RectI& roi,Natron::Image* output) const
{
    ///handle case where there is only 1 column/row
    if (roi.width() == 1 || roi.height() == 1) {
        assert( !(roi.width() == 1 && roi.height() == 1) ); /// can't be 1x1
        halve1DImage(roi,output);
        return;
    }

    ///The source rectangle, intersected to this image region of definition in pixels
    const RectI &srcRoD = getPixelRoD();
    const RectI &dstRoD = output->getPixelRoD();

   // the srcRoD of the output should be enclosed in half the roi.
    // It does not have to be exactly half of the input.
    assert(dstRoD.x1*2 >= roi.x1 &&
           dstRoD.x2*2 <= roi.x2 &&
           dstRoD.y1*2 >= roi.y1 &&
           dstRoD.y2*2 <= roi.y2 &&
           dstRoD.width()*2 <= roi.width() &&
           dstRoD.height()*2 <= roi.height());
    assert(getComponents() == output->getComponents());
    
    int components = getElementsCountForComponents(getComponents());

    RectI srcRoI = roi;
    srcRoI.intersect(srcRoD, &srcRoI); // intersect srcRoI with the region of definition
    srcRoI = srcRoI.roundPowerOfTwoSmallestEnclosing(1);
    
    RectI dstRoI = srcRoI.downscalePowerOfTwo(1);
    // a few checks...
    // srcRoI must be inside srcRoD
    assert(srcRoI.x1 >= srcRoD.x1);
    assert(srcRoI.x2 <= srcRoD.x2);
    assert(srcRoI.y1 >= srcRoD.y1);
    assert(srcRoI.y2 <= srcRoD.y2);
    // srcRoI must be inside dstRoD*2
    assert(srcRoI.x1 >= dstRoD.x1*2);
    assert(srcRoI.x2 <= dstRoD.x2*2);
    assert(srcRoI.y1 >= dstRoD.y1*2);
    assert(srcRoI.y2 <= dstRoD.y2*2);
    // srcRoI must be equal to dstRoI*2
    assert(srcRoI.x1 == dstRoI.x1*2);
    assert(srcRoI.x2 == dstRoI.x2*2);
    assert(srcRoI.y1 == dstRoI.y1*2);
    assert(srcRoI.y2 == dstRoI.y2*2);
    assert(srcRoI.width() == dstRoI.width()*2);
    assert(srcRoI.height() == dstRoI.height()*2);

    switch (getBitDepth()) {
        case IMAGE_BYTE:
            halveRoIInternal<unsigned char,255>(*this,srcRoD, *output, dstRoD, srcRoI, dstRoI, components);
            break;
        case IMAGE_SHORT:
            halveRoIInternal<unsigned short,65535>(*this,srcRoD, *output, dstRoD, srcRoI, dstRoI, components);
            break;
        case IMAGE_FLOAT:
            halveRoIInternal<unsigned char,1>(*this,srcRoD, *output, dstRoD, srcRoI, dstRoI, components);
            break;
        default:
            break;
    }
}

template <typename PIX,int maxValue>
void halve1DImageInternal(const Image& srcImg,Image& dstImg,const RectI& roi,int width,int height,int components)
{
    int halfWidth = width / 2;
    int halfHeight = height / 2;
    if (height == 1) { //1 row
        assert(width != 1);	/// widthxheight can't be 1x1
        
        const PIX* src = (const PIX*)srcImg.pixelAt(roi.x1, roi.y1);
        PIX* dst = (PIX*)dstImg.pixelAt(dstImg.getRoD().x1, dstImg.getRoD().y1);
        
        for (int x = 0; x < halfWidth; ++x) {
            for (int k = 0; k < components; ++k) {
                *dst++ = PIX((float)(*src + *(src + components)) / 2.);
                ++src;
            }
            src += components;
        }
        
    } else if (width == 1) {
        
        int rowSize = srcImg.getPixelRoD().width() * components;
        
        const PIX* src = (const PIX*)srcImg.pixelAt(roi.x1, roi.y1);
        PIX* dst = (PIX*)dstImg.pixelAt(dstImg.getRoD().x1, dstImg.getRoD().y1);
        
        for (int y = 0; y < halfHeight; ++y) {
            for (int k = 0; k < components;++k) {
                *dst++ = PIX((float)(*src + (*src + rowSize)) / 2.);
                ++src;
            }
            src += rowSize;
        }
    }

}

void Image::halve1DImage(const RectI& roi,Natron::Image* output) const
{
    int width = roi.width();
    int height = roi.height();
    
    
    
    assert(width == 1 || height == 1); /// must be 1D
    assert(output->getComponents() == getComponents());
    assert(output->getPixelRoD().x1*2 == roi.x1 &&
           output->getPixelRoD().x2*2 == roi.x2 &&
           output->getPixelRoD().y1*2 == roi.y1 &&
           output->getPixelRoD().y2*2 == roi.y2);

    int components = getElementsCountForComponents(getComponents());
    switch (getBitDepth()) {
        case IMAGE_BYTE:
            halve1DImageInternal<unsigned char,255>(*this, *output, roi, width, height, components);
            break;
        case IMAGE_SHORT:
            halve1DImageInternal<unsigned short,65535>(*this, *output, roi, width, height, components);
            break;
        case IMAGE_FLOAT:
            halve1DImageInternal<float , 1>(*this, *output, roi, width, height, components);
            break;
        default:
            break;
    }
    
}

void Image::downscale_mipmap(const RectI& roi,Natron::Image* output,unsigned int level) const
{
    ///You should not call this function with a level equal to 0.
    assert(level > 0);
    
    ///This is the portion we computed in buildMipMapLevel
    RectI dstRoI = roi.downscalePowerOfTwoLargestEnclosed(level);
    
    ///Even if the roi is this image's RoD, the
    ///resulting mipmap of that roi should fit into output.
    Natron::Image* tmpImg = new Natron::Image(getComponents(),dstRoI,0,getBitDepth());
    
    buildMipMapLevel(tmpImg, roi, level);
  
    ///Now copy the result of tmpImg into the output image
    output->copy(*tmpImg, dstRoI,false);
        
    ///clean-up
    delete tmpImg;
}

template <typename PIX,int maxValue>
void upscale_mipmapInternal(const Image& srcImg,Image& dstImg,const RectI& srcRod,const RectI& dstRod,int components,
                            int srcRowSize,int dstRowSize,unsigned int scale)
{
    const PIX *src = (const PIX*)srcImg.pixelAt(srcRod.x1, srcRod.y1);
    PIX* dst = (PIX*)dstImg.pixelAt(dstRod.x1, dstRod.y1);
    
    for (int y = 0; y < srcRod.height(); ++y) {
        const PIX *srcLineStart = src + y*srcRowSize;
        PIX *dstLineStart = dst + y*scale*dstRowSize;
        for (int x = 0; x < srcRod.width(); ++x) {
            const PIX *srcPix = srcLineStart + x*components;
            PIX *dstPix = dstLineStart + x*scale*components;
            for (unsigned int j = 0; j < scale; ++j) {
                PIX *dstSubPixLineStart = dstPix + j*dstRowSize;
                for (unsigned int i = 0; i < scale; ++i) {
                    PIX *dstSubPix = dstSubPixLineStart + i*components;
                    for (int c = 0; c < components; ++c) {
                        dstSubPix[c] = srcPix[c];
                    }
                }
            }
        }
    }

}

void Image::upscale_mipmap(const RectI& roi,Natron::Image* output,unsigned int level) const
{
    ///You should not call this function with a level equal to 0.
    assert(level > 0);

    ///The source rectangle, intersected to this image region of definition in pixels
    RectI srcRod = roi;
    srcRod.intersect(getPixelRoD(), &srcRod);

    RectI dstRod = roi.upscalePowerOfTwo(level);
    unsigned int scale = 1 << level;

    assert(output->getComponents() == getComponents());
    int components = getElementsCountForComponents(getComponents());

    int srcRowSize = getPixelRoD().width() * components;
    int dstRowSize = output->getPixelRoD().width() * components;

    switch (getBitDepth()) {
        case IMAGE_BYTE:
            upscale_mipmapInternal<unsigned char, 255>(*this, *output, srcRod, dstRod, components, srcRowSize, dstRowSize, scale);
            break;
        case IMAGE_SHORT:
            upscale_mipmapInternal<unsigned short, 65535>(*this, *output, srcRod, dstRod, components, srcRowSize, dstRowSize, scale);
            break;
        case IMAGE_FLOAT:
            upscale_mipmapInternal<float,1>(*this, *output, srcRod, dstRod, components, srcRowSize, dstRowSize, scale);
            break;
        default:
            break;
    }
}

template<typename PIX>
void scale_box_genericInternal(const Image& srcImg,const RectI& srcRod,Image& dstImg,const RectI& dstRod)
{
    RenderScale scale;
    scale.x = (double) srcRod.width() / dstRod.width();
    scale.y = (double) srcRod.height() / dstRod.height();
    
    int convx_int = std::floor(scale.x);
    int convy_int = std::floor(scale.y);
    double convx_float = scale.x - convx_int;
    double convy_float = scale.y - convy_int;
    double area = scale.x * scale.y;
    
    int lowy_int = 0;
    double lowy_float = 0.;
    int highy_int = convy_int;
    double highy_float = convy_float;
    
    const PIX *src = (const PIX*)srcImg.pixelAt(srcRod.x1, srcRod.y1);
    PIX* dst = (PIX*)dstImg.pixelAt(dstRod.x1, dstRod.y1);
    
    assert(dstImg.getComponents() == srcImg.getComponents());
    int components = getElementsCountForComponents(srcImg.getComponents());
    
    int rowSize = srcImg.getPixelRoD().width() * components;
    
    float totals[4];
    
    for (int y = 0; y < dstRod.height(); ++y) {
        
        /* Clamp here to be sure we don't read beyond input buffer. */
        if (highy_int >= srcRod.height())
            highy_int = srcRod.height() - 1;
        
        int lowx_int = 0;
        double lowx_float = 0.;
        int highx_int = convx_int;
        double highx_float = convx_float;
        
        for (int x = 0; x < dstRod.width(); ++x) {
            if (highx_int >= srcRod.width()) {
                highx_int = srcRod.width() - 1;
            }
            
            /*
             ** Ok, now apply box filter to box that goes from (lowx, lowy)
             ** to (highx, highy) on input data into this pixel on output
             ** data.
             */
            totals[0] = totals[1] = totals[2] = totals[3] = 0.0;
            
            /// calculate the value for pixels in the 1st row
            int xindex = lowx_int * components;
            if ((highy_int > lowy_int) && (highx_int > lowx_int)) {
                double y_percent = 1. - lowy_float;
                double percent = y_percent * (1. - lowx_float);
                const PIX* temp = src + xindex + lowy_int * rowSize;
                const PIX* temp_index ;
                int k;
                for (k = 0,temp_index = temp; k < components; ++k,++temp_index) {
                    totals[k] += *temp_index * percent;
                }
                const PIX* left = temp;
                for(int l = lowx_int + 1; l < highx_int; ++l) {
                    temp += components;
                    for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                        totals[k] += *temp_index * y_percent;
                    };
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
                for(int l = lowx_int + 1; l < highx_int; ++l) {
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
                for(int m = lowy_int + 1; m < highy_int; ++m) {
                    left += rowSize;
                    right += rowSize;
                    for (k = 0; k < components;++k, ++left, ++right) {
                        totals[k] += *left * (1 - lowx_float) + *right * highx_float;
                    }
                }
            } else if (highy_int > lowy_int) {
                double x_percent = highx_float - lowx_float;
                double percent = (1. - lowy_float) * x_percent;
                const PIX* temp = src + xindex + lowy_int * rowSize;
                const PIX* temp_index;
                int k;
                for (k = 0, temp_index = temp; k < components;++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
                for(int m = lowy_int + 1; m < highy_int; ++m) {
                    temp += rowSize;
                    for (k = 0, temp_index = temp; k < components;++k, ++temp_index) {
                        totals[k] += *temp_index * x_percent;
                    }
                }
                percent = x_percent * highy_float;
                temp += rowSize;
                for (k = 0, temp_index = temp; k < components;++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
            } else if (highx_int > lowx_int) {
                double y_percent = highy_float - lowy_float;
                double percent = (1. - lowx_float) * y_percent;
                int k;
                const PIX* temp = src + xindex + lowy_int * rowSize;
                const PIX* temp_index;
                
                for (k = 0, temp_index = temp; k < components;++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
                for (int l = lowx_int + 1; l < highx_int; ++l) {
                    temp += components;
                    for (k = 0, temp_index = temp; k < components;++k, ++temp_index) {
                        totals[k] += *temp_index * y_percent;
                    }
                }
                temp += components;
                percent = y_percent * highx_float;
                for (k = 0, temp_index = temp; k < components;++k, ++temp_index) {
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
                for(int l = lowx_int + 1; l < highx_int; ++l) {
                    int k;
                    const PIX *temp_index;
                    
                    for (k = 0, temp_index = temp; k < components;++k, ++temp_index) {
                        totals[k] += *temp_index;
                    }
                    temp += components;
                }
                temp0 += rowSize;
            }
            
            int outindex = (x + (y * dstRod.width())) * components;
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

}

//Image::scale should never be used: there should only be a method to *up*scale by a power of two, and the downscaling is done by
//buildMipMapLevel
void Image::scale_box_generic(const RectI& roi,Natron::Image* output) const
{
    ///The destination rectangle
    const RectI& dstRod = output->getPixelRoD();
    
    ///The source rectangle, intersected to this image region of definition in pixels
    RectI srcRod = roi;
    srcRod.intersect(getPixelRoD(), &srcRod);

    ///If the roi is exactly twice the destination rect, just halve that portion into output.
    if (srcRod.x1 == 2 * dstRod.x1 &&
        srcRod.x2 == 2 * dstRod.x2 &&
        srcRod.y1 == 2 * dstRod.y1 &&
        srcRod.y2 == 2 * dstRod.y2) {
        halveRoI(srcRod,output);
        return;
    }
    
    switch (getBitDepth()) {
        case IMAGE_BYTE:
            scale_box_genericInternal<unsigned char>(*this,srcRod, *output, dstRod);
            break;
        case IMAGE_SHORT:
            scale_box_genericInternal<unsigned short>(*this,srcRod, *output, dstRod);
            break;
        case IMAGE_FLOAT:
            scale_box_genericInternal<float>(*this,srcRod, *output, dstRod);
            break;
        default:
            break;
    }
    
}

void Image::buildMipMapLevel(Natron::Image* output,const RectI& roi,unsigned int level) const
{
    ///The output image data window
    const RectI& dstRoD = output->getPixelRoD();

    
    ///The last mip map level we will make with closestPo2
    RectI lastLevelRoI = roi.downscalePowerOfTwoSmallestEnclosing(level);
    
    ///The output image must contain the last level roi
    assert(dstRoD.contains(lastLevelRoI));
    
    assert(output->getComponents() == getComponents());

    if (level == 0) {
        ///Just copy the roi and return
        output->copy(*this, roi);
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
        dstImg = new Natron::Image(getComponents(),halvedRoI,0,getBitDepth());
        
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
    
    assert(srcImg->getPixelRoD() == lastLevelRoI);
    
    ///Finally copy the last mipmap level into output.
    output->copy(*srcImg,srcImg->getPixelRoD());
    
    ///Clean-up, we should use shared_ptrs for safety
    if (mustFreeSrc) {
        delete srcImg;
    }
}


double
Image::getScaleFromMipMapLevel(unsigned int level)
{
    return 1./(1<<level);
}

#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif
unsigned int
Image::getLevelFromScale(double s)
{
    assert(0. < s && s <= 1.);
    int retval = -std::floor(std::log(s)/M_LN2 + 0.5);
    assert(retval >= 0);
    return retval;
}

void Image::clearBitmap()
{
    _bitmap.clear();
}

template <typename SRCPIX,typename DSTPIX>
DSTPIX convertPixelDepth(SRCPIX pix);

///explicit template instantiations
template <> float convertPixelDepth(unsigned char pix) { return (float)(pix) / 255.f; }
template <> unsigned short convertPixelDepth(unsigned char pix) { return (unsigned short)(pix) * 255.f; }
template <> unsigned char convertPixelDepth(unsigned char pix) { return pix; }

template <> unsigned char convertPixelDepth(unsigned short pix) { return (unsigned char)(pix / 255.f); }
template <> float convertPixelDepth(unsigned short pix) { return (float)(pix) / 65535.f; }
template <> unsigned short convertPixelDepth(unsigned short pix) { return pix; }

template <> unsigned char convertPixelDepth(float pix) { return (unsigned char)(pix * 255); }
template <> unsigned short convertPixelDepth(float pix) { return (unsigned short)(pix * 65535); }
template <> float convertPixelDepth(float pix) { return pix; }

///Fast version when components are the same
template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue>
void convertToFormatInternal_sameComps(const RectI& renderWindow,const Image& srcImg,Image& dstImg,bool invert,bool copyBitmap)
{
    const RectI& r = srcImg.getPixelRoD();

    RectI intersection;
    if (!renderWindow.intersect(r, &intersection)) {
        return;
    }
    
    int nComp = (int)srcImg.getComponentsCount();

    ///This is by how much we need to offset the pointer once a row is finished to go to the next one
    int rowExtraOffset = r.width() * nComp - intersection.width() * nComp;
    
    const SRCPIX* srcPixels = (const SRCPIX*)srcImg.pixelAt(intersection.x1, intersection.y1);
    DSTPIX* dstPixels = (DSTPIX*)dstImg.pixelAt(intersection.x1, intersection.y1);
    
    for (int y = 0; y < intersection.height();
         ++y, srcPixels += rowExtraOffset , dstPixels += rowExtraOffset) {
        
        for (int x = 0; x < intersection.width();
             ++x,srcPixels += nComp,dstPixels += nComp) {
            
            for (int k = 0; k < nComp; ++k) {
                
                DSTPIX pix = convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[k]);
                dstPixels[k] = invert ? dstMaxValue - pix : pix;
                
            }
        }
        
        if (copyBitmap) {
            const char* srcBitmap = srcImg.getBitmapAt(intersection.x1, intersection.y1 + y);
            char* dstBitmap = dstImg.getBitmapAt(intersection.x1, intersection.y1 + y);
            memcpy(dstBitmap, srcBitmap, intersection.width());
        }
    }
}

template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue>
void convertToFormatInternal(const RectI& renderWindow,const Image& srcImg,Image& dstImg,int channelForAlpha,bool invert,bool copyBitmap)
{
    
    const RectI& r = srcImg.getPixelRoD();
    
    RectI intersection;
    if (!renderWindow.intersect(r, &intersection)) {
        return;
    }
    
    Natron::ImageComponents dstComp = dstImg.getComponents();
    
    bool sameBitDepth = srcImg.getBitDepth() == dstImg.getBitDepth();

    DSTPIX* dstPixels = (DSTPIX*)dstImg.pixelAt(intersection.x1, intersection.y1);
    const SRCPIX* srcPixels = (const SRCPIX*)srcImg.pixelAt(intersection.x1, intersection.y1);

    int dstNComp = getElementsCountForComponents(dstComp);
    int srcNComp = getElementsCountForComponents(srcImg.getComponents());
    
    ///This is by how much we need to offset the pointer once a row is finished to go to the next one
    int srcExtraOffset = r.width() * srcNComp - intersection.width() * srcNComp;
    int dstExtraOffset = r.width() * dstNComp - intersection.width() * dstNComp;
    
    assert(srcPixels);
    assert(dstPixels);

    ///special case comp == alpha && channelForAlpha = -1 clear out the mask
    if (dstComp == Natron::ImageComponentAlpha && channelForAlpha == -1) {
        for (int y = 0; y < intersection.height();
             ++y, dstPixels += (r.width() * dstNComp)) {
            std::fill(dstPixels, dstPixels + intersection.width() * dstNComp, 0.);
            if (copyBitmap) {
                const char* srcBitmap = srcImg.getBitmapAt(intersection.x1, intersection.y1 + y);
                char* dstBitmap = dstImg.getBitmapAt(intersection.x1, intersection.y1 + y);
                memcpy(dstBitmap, srcBitmap, intersection.width());
            }
        }
        return;
    }
    

    for (int y = 0; y < intersection.height();
         ++y, srcPixels += srcExtraOffset, dstPixels += dstExtraOffset) {
        
        for (int x = 0; x < intersection.width();
             ++x,srcPixels += srcNComp,dstPixels += dstNComp) {
            
            if (dstComp == Natron::ImageComponentAlpha) {
                assert(channelForAlpha < srcNComp && channelForAlpha >= 0);
                *dstPixels = !sameBitDepth ? convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[channelForAlpha])
                : srcPixels[channelForAlpha];
                if (invert) {
                    *dstPixels = dstMaxValue - *dstPixels;
                }
            } else {
                for (int k = 0; k < dstNComp; ++k) {
                    if (k < srcNComp) {
                        DSTPIX pix = !sameBitDepth ? convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[k]) : srcPixels[k];
                        dstPixels[k] = invert ? dstMaxValue - pix : pix;
                    } else {
                        dstPixels[k] = k == 3 ? dstMaxValue :  0.;
                        if (invert) {
                            dstPixels[k] = dstMaxValue - dstPixels[k];
                        }
                    }
                }
            }
            if (copyBitmap) {
                const char* srcBitmap = srcImg.getBitmapAt(intersection.x1, intersection.y1 + y);
                char* dstBitmap = dstImg.getBitmapAt(intersection.x1, intersection.y1 + y);
                memcpy(dstBitmap, srcBitmap, intersection.width());
            }
        }
    }

}



void Image::convertToFormat(const RectI& renderWindow,Natron::Image* dstImg,int channelForAlpha,bool invert,bool copyBitmap) const
{
    assert(getPixelRoD() == dstImg->getPixelRoD());
    
    if (dstImg->getComponents() == getComponents()) {
        switch (dstImg->getBitDepth()) {
            case IMAGE_BYTE: {
                switch (getBitDepth()) {
                    case IMAGE_BYTE:
                        ///Same as a copy
                        convertToFormatInternal_sameComps<unsigned char, unsigned char, 255, 255>(renderWindow,*this, *dstImg,
                                                                                                  invert,copyBitmap);
                        break;
                    case IMAGE_SHORT:
                        convertToFormatInternal_sameComps<unsigned short, unsigned char, 65535, 255>(renderWindow,*this, *dstImg,
                                                                                                     invert,copyBitmap);
                        break;
                    case IMAGE_FLOAT:
                        convertToFormatInternal_sameComps<float, unsigned char, 1, 255>(renderWindow,*this, *dstImg,
                                                                                        invert,copyBitmap);
                        break;
                    default:
                        break;
                }
            } break;
            case IMAGE_SHORT: {
                switch (getBitDepth()) {
                    case IMAGE_BYTE:
                        convertToFormatInternal_sameComps<unsigned char, unsigned short, 255, 65535>(renderWindow,*this, *dstImg,
                                                                                                     invert,copyBitmap);
                        break;
                    case IMAGE_SHORT:
                        ///Same as a copy
                        convertToFormatInternal_sameComps<unsigned short, unsigned short, 65535, 65535>(renderWindow,*this, *dstImg,
                                                                                                        invert,copyBitmap);
                        break;
                    case IMAGE_FLOAT:
                        convertToFormatInternal_sameComps<float, unsigned short, 1, 65535>(renderWindow,*this, *dstImg,
                                                                                           invert,copyBitmap);
                        break;
                    default:
                        break;
                }
            } break;
            case IMAGE_FLOAT: {
                switch (getBitDepth()) {
                    case IMAGE_BYTE:
                        convertToFormatInternal_sameComps<unsigned char, float, 255, 1>(renderWindow,*this, *dstImg,
                                                                                        invert,copyBitmap);
                        break;
                    case IMAGE_SHORT:
                        convertToFormatInternal_sameComps<unsigned short, float, 65535, 1>(renderWindow,*this, *dstImg,
                                                                                           invert,copyBitmap);
                        break;
                    case IMAGE_FLOAT:
                        ///Same as a copy
                        convertToFormatInternal_sameComps<float, float, 1, 1>(renderWindow,*this, *dstImg,
                                                                              invert,copyBitmap);
                        break;
                    default:
                        break;
                }
            } break;
                
            default:
                break;
        }
    } else {
        switch (dstImg->getBitDepth()) {
            case IMAGE_BYTE: {
                switch (getBitDepth()) {
                    case IMAGE_BYTE:
                        convertToFormatInternal<unsigned char, unsigned char, 255, 255>(renderWindow,*this, *dstImg, channelForAlpha,
                                                                                        invert,copyBitmap);
                        break;
                    case IMAGE_SHORT:
                        convertToFormatInternal<unsigned short, unsigned char, 65535, 255>(renderWindow,*this, *dstImg, channelForAlpha,
                                                                                           invert,copyBitmap);
                        break;
                    case IMAGE_FLOAT:
                        convertToFormatInternal<float, unsigned char, 1, 255>(renderWindow,*this, *dstImg, channelForAlpha,
                                                                              invert,copyBitmap);
                        break;
                    default:
                        break;
                }
            } break;
            case IMAGE_SHORT: {
                switch (getBitDepth()) {
                    case IMAGE_BYTE:
                        convertToFormatInternal<unsigned char, unsigned short, 255, 65535>(renderWindow,*this, *dstImg, channelForAlpha,
                                                                                           invert,copyBitmap);
                        break;
                    case IMAGE_SHORT:
                        convertToFormatInternal<unsigned short, unsigned short, 65535, 65535>(renderWindow,*this, *dstImg,
                                                                                              channelForAlpha, invert,copyBitmap);
                        break;
                    case IMAGE_FLOAT:
                        convertToFormatInternal<float, unsigned short, 1, 65535>(renderWindow,*this, *dstImg, channelForAlpha,
                                                                                 invert,copyBitmap);
                        break;
                    default:
                        break;
                }
            } break;
            case IMAGE_FLOAT: {
                switch (getBitDepth()) {
                    case IMAGE_BYTE:
                        convertToFormatInternal<unsigned char, float, 255, 1>(renderWindow,*this, *dstImg, channelForAlpha,
                                                                              invert,copyBitmap);
                        break;
                    case IMAGE_SHORT:
                        convertToFormatInternal<unsigned short, float, 65535, 1>(renderWindow,*this, *dstImg, channelForAlpha,
                                                                                 invert,copyBitmap);
                        break;
                    case IMAGE_FLOAT:
                        convertToFormatInternal<float, float, 1, 1>(renderWindow,*this, *dstImg, channelForAlpha,
                                                                    invert,copyBitmap);
                        break;
                    default:
                        break;
                }
            } break;
                
            default:
                break;
        }
    }
}
