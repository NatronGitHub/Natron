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
         double pixelAspect)
: KeyHelper<U64>()
, _nodeHashKey(nodeHashKey)
, _time(time)
, _view(view)
, _pixelAspect(pixelAspect)
{ _mipMapLevel = mipMapLevel; }

void ImageKey::fillHash(Hash64* hash) const {
    hash->append(_nodeHashKey);
    hash->append(_mipMapLevel);
    hash->append(_time);
    hash->append(_view);
    hash->append(_pixelAspect);
}



bool ImageKey::operator==(const ImageKey& other) const {
    return _nodeHashKey == other._nodeHashKey &&
    _mipMapLevel == other._mipMapLevel &&
    _time == other._time &&
    _view == other._view &&
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
CacheEntryHelper<float,ImageKey>(key,params,restore,path)
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
Image::Image(ImageComponents components,const RectI& regionOfDefinition,unsigned int mipMapLevel)
: CacheEntryHelper<float,ImageKey>(makeKey(0,0,mipMapLevel,0),
                                   boost::shared_ptr<const NonKeyParams>(new ImageParams(0,
                                                                                         regionOfDefinition,
                                                                                         regionOfDefinition.downscalePowerOfTwoSmallestEnclosing(mipMapLevel),
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
                        int view){
    return ImageKey(nodeHashKey,time,mipMapLevel,view);
}

boost::shared_ptr<ImageParams> Image::makeParams(int cost,const RectI& rod,unsigned int mipMapLevel,
                                                 bool isRoDProjectFormat,ImageComponents components,
                                                 int inputNbIdentity,int inputTimeIdentity,
                                                 const std::map<int, std::vector<RangeD> >& framesNeeded) {
    return boost::shared_ptr<ImageParams>(new ImageParams(cost,rod,rod.downscalePowerOfTwoSmallestEnclosing(mipMapLevel)
                                                          ,isRoDProjectFormat,components,inputNbIdentity,inputTimeIdentity,framesNeeded));
}


void Natron::Image::copy(const Natron::Image& other,const RectI& roi,bool copyBitmap)
{
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
    
    assert(getComponents() == other.getComponents());
    int components = getElementsCountForComponents(getComponents());
    
    for (int y = intersection.y1; y < intersection.y2; ++y) {
        const float* src = other.pixelAt(dstRoD.x1, y);
        float* dst = pixelAt(intersection.x1, y);
        memcpy(dst, src, intersection.width() * sizeof(float) * components);
        
        if (copyBitmap) {
            const char* srcBm = other.getBitmapAt(dstRoD.x1, y);
            char* dstBm = getBitmapAt(intersection.x1, y);
            memcpy(dstBm, srcBm, intersection.width());
        }
        
    }
}

void Natron::Image::fill(const RectI& rect,float r,float g,float b,float a) {
    ImageComponents comps = getComponents();
    if (comps == ImageComponentNone) {
        return;
    }
    
    float fillValue[4] = {r,g,b,a};
    
    int nComps = getElementsCountForComponents(comps);
    int rowElems = (int)getRowElements();
    float* dst = pixelAt(rect.x1, rect.y1);
    for (int i = 0; i < rect.height();++i,dst += (rowElems - rect.width() * nComps)) {
        for (int j = 0; j < rect.width();++j,dst+=nComps) {
            for (int k = 0; k < nComps; ++k) {
                if (comps == Natron::ImageComponentAlpha) {
                    dst[k] = a;
                } else {
                    dst[k] = fillValue[k];
                }
            }
        }
    }
}

float* Image::pixelAt(int x,int y){
    int compsCount = getElementsCountForComponents(getComponents());
    if (x >= _pixelRod.left() && x < _pixelRod.right() && y >= _pixelRod.bottom() && y < _pixelRod.top()) {
        return this->_data.writable() + (y-_pixelRod.bottom()) * compsCount * _pixelRod.width() + (x-_pixelRod.left()) * compsCount;
    } else {
        return NULL;
    }
}

const float* Image::pixelAt(int x,int y) const {
    int compsCount = getElementsCountForComponents(getComponents());
    if (x >= _pixelRod.left() && x < _pixelRod.right() && y >= _pixelRod.bottom() && y < _pixelRod.top()) {
        return this->_data.readable() + (y-_pixelRod.bottom()) * compsCount * _pixelRod.width() + (x-_pixelRod.left()) * compsCount;
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

    int srcRoIWidth = srcRoI.width();
    //int srcRoIHeight = srcRoI.height();
    int dstRoIWidth = dstRoI.width();
    int dstRoIHeight = dstRoI.height();
    const float* src = pixelAt(srcRoI.x1, srcRoI.y1);
    float* dst = output->pixelAt(dstRoI.x1, dstRoI.y1);
    
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
            assert(dst == output->pixelAt(dstRoI.x1+x, dstRoI.y1+y));
            assert(srcRoD.x1 <= srcRoI.x1+2*x && srcRoI.x1+2*x < srcRoD.x2);
            assert(srcRoD.y1 <= srcRoI.y1+2*y && srcRoI.y1+2*y < srcRoD.y2);
            assert(src == pixelAt(srcRoI.x1+2*x, srcRoI.y1+2*y));
            for (int k = 0; k < components; ++k, ++dst, ++src) {
                *dst = (*src +
                        *(src + components) +
                        *(src + srcRowSize) +
                        *(src + srcRowSize  + components)) / 4;
            }
        }
    }
}

void Image::halve1DImage(const RectI& roi,Natron::Image* output) const
{
    int width = roi.width();
    int height = roi.height();
    
    int halfWidth = width / 2;
    int halfHeight = height / 2;
    
    assert(width == 1 || height == 1); /// must be 1D
    assert(output->getComponents() == getComponents());
    assert(output->getPixelRoD().x1*2 == roi.x1 &&
           output->getPixelRoD().x2*2 == roi.x2 &&
           output->getPixelRoD().y1*2 == roi.y1 &&
           output->getPixelRoD().y2*2 == roi.y2);

    int components = getElementsCountForComponents(getComponents());
    
    
    if (height == 1) { //1 row
        assert(width != 1);	/// widthxheight can't be 1x1
        
        const float* src = pixelAt(roi.x1, roi.y1);
        float* dst = output->pixelAt(output->getRoD().x1, output->getRoD().y1);
        
        for (int x = 0; x < halfWidth; ++x) {
            for (int k = 0; k < components; ++k) {
                *dst++ = (*src + *(src + components)) / 2.;
                ++src;
            }
            src += components;
        }
        
    } else if (width == 1) {
        
        int rowSize = getPixelRoD().width() * components;
        
        const float* src = pixelAt(roi.x1, roi.y1);
        float* dst = output->pixelAt(output->getRoD().x1, output->getRoD().y1);
        
        for (int y = 0; y < halfHeight; ++y) {
            for (int k = 0; k < components;++k) {
                *dst++ = (*src + (*src + rowSize)) / 2.;
                ++src;
            }
            src += rowSize;
        }
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
    Natron::Image* tmpImg = new Natron::Image(getComponents(),dstRoI,0);
    
    buildMipMapLevel(tmpImg, roi, level);
  
    ///Now copy the result of tmpImg into the output image
    output->copy(*tmpImg, dstRoI,false);
        
    ///clean-up
    delete tmpImg;
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

    const float *src = pixelAt(srcRod.x1, srcRod.y1);
    float* dst = output->pixelAt(dstRod.x1, dstRod.y1);

    assert(output->getComponents() == getComponents());
    int components = getElementsCountForComponents(getComponents());

    int srcRowSize = getPixelRoD().width() * components;
    int dstRowSize = output->getPixelRoD().width() * components;


    for (int y = 0; y < srcRod.height(); ++y) {
        const float *srcLineStart = src + y*srcRowSize;
        float *dstLineStart = dst + y*scale*dstRowSize;
        for (int x = 0; x < srcRod.width(); ++x) {
            const float *srcPix = srcLineStart + x*components;
            float *dstPix = dstLineStart + x*scale*components;
            for (unsigned int j = 0; j < scale; ++j) {
                float *dstSubPixLineStart = dstPix + j*dstRowSize;
                for (unsigned int i = 0; i < scale; ++i) {
                    float *dstSubPix = dstSubPixLineStart + i*components;
                    for (int c = 0; c < components; ++c) {
                        dstSubPix[c] = srcPix[c];
                    }
                }
            }
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
    
    const float *src = pixelAt(srcRod.x1, srcRod.y1);
    float* dst = output->pixelAt(dstRod.x1, dstRod.y1);
    
    assert(output->getComponents() == getComponents());
    int components = getElementsCountForComponents(getComponents());
    
    int rowSize = getPixelRoD().width() * components;
    
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
                const float* temp = src + xindex + lowy_int * rowSize;
                const float* temp_index ;
                int k;
                for (k = 0,temp_index = temp; k < components; ++k,++temp_index) {
                     totals[k] += *temp_index * percent;
                }
                const float* left = temp;
                for(int l = lowx_int + 1; l < highx_int; ++l) {
                    temp += components;
                    for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                        totals[k] += *temp_index * y_percent;
                    };
                }
                
                temp += components;
                const float* right = temp;
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
                const float* temp = src + xindex + lowy_int * rowSize;
                const float* temp_index;
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
                const float* temp = src + xindex + lowy_int * rowSize;
                const float* temp_index;
                
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
                const float* temp = src + xindex + lowy_int * rowSize;
                int k;
                const float* temp_index;
                
                for (k = 0, temp_index = temp; k < components; ++k, ++temp_index) {
                    totals[k] += *temp_index * percent;
                }
            }
            
            /// this is for the pixels in the body
            const float* temp0 = src + lowx_int + components + (lowy_int + 1) * rowSize;
            const float* temp;
            
            for (int m = lowy_int + 1; m < highy_int; ++m) {
                temp = temp0;
                for(int l = lowx_int + 1; l < highx_int; ++l) {
                    int k;
                    const float *temp_index;
                    
                    for (k = 0, temp_index = temp; k < components;++k, ++temp_index) {
                        totals[k] += *temp_index;
                    }
                    temp += components;
                }
                temp0 += rowSize;
            }
            
            int outindex = (x + (y * dstRod.width())) * components;
            for (int k = 0; k < components; ++k) {
                dst[outindex + k] = totals[k] / area;
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
        dstImg = new Natron::Image(getComponents(),halvedRoI,0);
        
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

/**
 * @brief This function can be used to do the following conversion:
 * 1) RGBA to RGB
 * 2) RGBA to alpha
 * 3) RGB to RGBA
 * 4) RGB to alpha
 * @param channelForAlpha is used in cases 2) and 4) to determine from which channel we should
 * fill the alpha. If it is -1 it indicates you want to clear the mask.
 *
 * WARNING: The bitmap is NOT copied when converting. This function is not meant to cache images
 * that's why it allocates itself the return value.
 * The caller is responsible for freeing the image afterwards.
 **/
Natron::Image* Image::convertToFormat(Natron::ImageComponents comp,int channelForAlpha,bool invert) const
{
    assert(comp == Natron::ImageComponentRGBA || comp == Natron::ImageComponentRGB || comp == Natron::ImageComponentAlpha);
    Natron::Image* output = new Natron::Image(comp,getRoD(),getMipMapLevel());
    const RectI& r = getPixelRoD();
    
    float* dstPixels = output->pixelAt(r.x1, r.y1);

    ///special case comp == alpha && channelForAlpha = -1 clear out the mask
    if (comp == Natron::ImageComponentAlpha && channelForAlpha == -1) {
        std::fill(dstPixels, dstPixels + r.area(), 0.);
        return output;
    }
    
    int dstComponents = getElementsCountForComponents(comp);
    int srcComponents = getElementsCountForComponents(getComponents());
    const float* srcPixels = pixelAt(r.x1, r.y1);
    
    assert(srcPixels);
    assert(dstPixels);
    
    for (int y = 0; y < r.height(); ++y) {
        for (int x = 0; x < r.width(); ++x,srcPixels += srcComponents,dstPixels += dstComponents) {
            if (comp == Natron::ImageComponentAlpha) {
                assert(channelForAlpha < srcComponents && channelForAlpha >= 0);
                *dstPixels = srcPixels[channelForAlpha];
                if (invert) {
                    *dstPixels = 1. - *dstPixels;
                }
            } else {
                for (int k = 0; k < dstComponents; ++k) {
                    if (k < srcComponents) {
                        dstPixels[k] = invert ? 1. - srcPixels[k] : srcPixels[k];
                    } else {
                        dstPixels[k] = k == 3 ? 1. :  0.;
                        if (invert) {
                            dstPixels[k] = 1. - dstPixels[k];
                        }
                    }
                }
            }
        }
    }
    
    return output;
}
