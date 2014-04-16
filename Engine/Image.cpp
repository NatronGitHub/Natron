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
}

/*This constructor can be used to allocate a local Image. The deallocation should
 then be handled by the user. Note that no view number is passed in parameter
 as it is not needed.*/
Image::Image(ImageComponents components,const RectI& regionOfDefinition,unsigned int mipMapLevel)
: CacheEntryHelper<float,ImageKey>(makeKey(0,0,mipMapLevel,0),
                                   boost::shared_ptr<const NonKeyParams>(new ImageParams(0,
                                                                                         regionOfDefinition,
                                                                                         regionOfDefinition.downscale(1 << mipMapLevel),
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
    return boost::shared_ptr<ImageParams>(new ImageParams(cost,rod,rod.downscale(1 << mipMapLevel)
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
    for (int i = rect.bottom(); i < rect.top();++i) {
        float* dst = pixelAt(rect.left(),i);
        for (int j = 0; j < rect.width();++j) {
            switch (comps) {
                case ImageComponentAlpha:
                    dst[j * 4] = a;
                    break;
                case ImageComponentRGB:
                    dst[j * 4] = r;
                    dst[j * 4 + 1] = g;
                    dst[j * 4 + 2] = b;
                    break;
                case ImageComponentRGBA:
                    dst[j * 4] = r;
                    dst[j * 4 + 1] = g;
                    dst[j * 4 + 2] = b;
                    dst[j * 4 + 3] = a;
                    break;
                case ImageComponentNone:
                default:
                    break;
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

void Image::halveImage(const RectI& roi,Natron::Image* output) const
{
    ///handle case where there is only 1 column/row
    int width = roi.width();
    int height = roi.height();
    
    if (width == 1 || height == 1) {
        assert( !(width == 1 && height == 1) ); /// can't be 1x1
        halve1DImage(roi,output);
        return;
    }
    
    
    
    int newWidth = width / 2;
    int newHeight = height / 2;
    
    assert(output->getPixelRoD().width() == newWidth && output->getPixelRoD().height() == newHeight);
    assert(getComponents() == output->getComponents());
    
    int components = getElementsCountForComponents(getComponents());
    
    const float* src = pixelAt(roi.x1, roi.y1);
    float* dst = output->pixelAt(output->getPixelRoD().x1, output->getPixelRoD().y1);
    
    int rowSize = getPixelRoD().width() * components;
    
    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            for (int k = 0; k < components; ++k) {
                *dst++ =  (*src +
                        *(src + components) +
                        *(src + rowSize) +
                        *(src + rowSize  + components)) / 4;
                ++src;
            }
            src += components;
        }
        src += rowSize;
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
    
    int components = getElementsCountForComponents(getComponents());
    
    
    if (height == 1) { //1 row
        assert(width != 1);	/// widthxheight can't be 1x1
        halfHeight = 1;
        
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

void Image::scale_mipmap(const RectI& roi,Natron::Image* output,unsigned int level) const
{
    ///You should not call this function with a level equal to 0.
    assert(level > 0);
    
    ///This is the portion we computed in buildMipMapLevel
    RectI srcRoI = roi.mipMapLevel(level, false);
    
    ///Even if the roi is this image's RoD, the
    ///resulting mipmap of that roi should fit into output.
    Natron::Image* tmpImg = new Natron::Image(getComponents(),srcRoI,0);
    
    buildMipMapLevel(tmpImg, roi, level);
   
    RectI dstRoI = roi.downscale(1 << level);
    
    Natron::Image* tmpImg2  = new Natron::Image(getComponents(),dstRoI,0);
    tmpImg->scale(srcRoI, tmpImg2);
   
    ///Now copy the result of tmpImg2 into the output image
    output->copy(*tmpImg2, dstRoI,false);
        
    ///clean-up
    delete tmpImg;
    delete tmpImg2;
}

void Image::scale(const RectI& roi,Natron::Image* output) const
{
    ///The destination rectangle
    const RectI& dstRod = output->getPixelRoD();
    
    ///The source rectangle, intersected to this image region of definition in pixels
    RectI srcRod = roi;
    srcRod.intersect(getPixelRoD(), &srcRod);

    ///If the roi is exactly twice the destination rect, just halve that portion into output.
    if (srcRod.width() == 2 * dstRod.width() && srcRod.height() == 2 * dstRod.height()) {
        halveImage(srcRod,output);
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

static RectI nextRectLevel(const RectI& r) {
    RectI ret = r;
    ret.x1 /= 2;
    ret.y1 /= 2;
    ret.x2 /= 2;
    ret.y2 /= 2;
    return ret;
}

void Image::buildMipMapLevel(Natron::Image* output,const RectI& roi,unsigned int level) const
{
    ///The output image data window
    const RectI& dstRoD = output->getPixelRoD();
    
    ///The nearest po2 box of the roi
    RectI closestPo2 = roi.closestPo2Rect();
    
    ///The last mip map level we will make with closestPo2
    RectI lastLevelRoI = closestPo2.mipMapLevel(level,true);
    
    ///The output image must contain the last level roi
    assert(dstRoD.contains(lastLevelRoI));
    
    assert(output->getComponents() == getComponents());
    
    const Natron::Image* srcImg = NULL;
    Natron::Image* dstImg = NULL;
    bool mustFreeSrc = false;
    
    if (closestPo2 == roi) { ///The roi is already a Po2
        if (level == 0) {
            ///Just copy the roi and return
            output->copy(*this,closestPo2);
            return;
        } else {
            ///We want to downscale, use this image as a start
            srcImg = this;
        }
        
    } else { ///The roi is not a po2
             ///Allocate a new image of the size of the nearest po2 box of the roi
        Natron::Image* tmpImg = new Natron::Image(getComponents(),closestPo2,0);
        
        ///Scale the roi of this image into tmpImg using a box filter.
        scale(roi,tmpImg);
        srcImg = tmpImg;
        mustFreeSrc = true;
    }
    
    ///Build all the mipmap levels until we reach the one we are interested in
    for (unsigned int i = 0; i < level; ++i) {
        
        ///Halve the closestPo2 rect
        RectI halvedRoI = nextRectLevel(closestPo2);
        
        ///Allocate an image with half the size of the source image
        dstImg = new Natron::Image(getComponents(),halvedRoI,0);
        
        ///Half the source image into dstImg.
        ///We pass the closestPo2 roi which might not be the entire size of the source image
        ///If the source image'sroi was originally a po2.
        srcImg->halveImage(closestPo2,dstImg);
        
        ///Clean-up, we should use shared_ptrs for safety
        if (mustFreeSrc) {
            delete srcImg;
        }
        
        ///Switch for next pass
        closestPo2 = halvedRoI;
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


RenderScale Image::getScaleFromMipMapLevel(unsigned int level) {
    RenderScale scale;
    switch (level) {
        case 0:
            scale.x = 1.;
            break;
        case 1:
            scale.x = 0.5;
            break;
        case 2:
            scale.x = 0.25;
            break;
        case 3:
            scale.x = 0.125;
            break;
        case 4:
            scale.x = 0.0625;
            break;
        case 5:
            scale.x = 0.03125;
            break;
        default:
            assert(false);
            break;
    }
    scale.y = scale.x;
    return scale;
}

unsigned int Image::getLevelFromScale(const RenderScale& s)
{
    assert(s.x == s.y);
    if (s.x == 1.) {
        return 0;
    } else if (s.x == 0.5) {
        return 1;
    } else if (s.x == 0.25) {
        return 2;
    } else if (s.x == 0.125) {
        return 3;
    } else if (s.x == 0.0625) {
        return 4;
    } else if (s.x == 0.03125) {
        return 5;
    } else {
        assert(false);
    }
    return 0;
}
