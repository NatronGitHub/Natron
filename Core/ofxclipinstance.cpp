//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#include "ofxclipinstance.h"

#include "Core/ofxnode.h"
#include "Core/settings.h"
#include "Core/imagefetcher.h"
#include "Superviser/powiterFn.h"

using namespace Powiter;
using namespace std;


OfxClipInstance::OfxClipInstance(int index,OfxNode* effect, OFX::Host::ImageEffect::ClipDescriptor* desc):
OFX::Host::ImageEffect::ClipInstance(effect, *desc),
_node(effect),_clipIndex(index),_outputImage(NULL)
{
}
Node* OfxClipInstance::getAssociatedNode() const{
    if(isOutput())
        return _node;
    else{
        return _node->input(_clipIndex);
    }
}

/// Get the Raw Unmapped Pixel Depth from the host. We are always 8 bits in our example
const std::string& OfxClipInstance::getUnmappedBitDepth() const
{
    // we always use floats
    static const std::string v(kOfxBitDepthFloat);
    return v;
}

/// Get the Raw Unmapped Components from the host. In our example we are always RGBA
const std::string &OfxClipInstance::getUnmappedComponents() const
{
    static const string rgbStr(kOfxImageComponentRGB);
    static const string noneStr(kOfxImageComponentNone);
    static const string rgbaStr(kOfxImageComponentRGBA);
    static const string alphaStr(kOfxImageComponentAlpha);
    
    bool rgb = false;
    bool alpha = false;
    
    const ChannelSet& channels = _node->getInfo()->channels();
    if(channels & alpha) alpha = true;
    if(channels & Mask_RGB) rgb = true;
    
    if(!rgb && !alpha) return noneStr;
    else if(rgb && !alpha) return rgbStr;
    else if(!rgb && alpha) return alphaStr;
    else return rgbaStr;
}


// PreMultiplication -
//
//  kOfxImageOpaque - the image is opaque and so has no premultiplication state
//  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
//  kOfxImageUnPreMultiplied - the image is unpremultiplied
const std::string &OfxClipInstance::getPremult() const
{
    static const std::string v(kOfxImageUnPreMultiplied);
    return v;
}


// Pixel Aspect Ratio -
//
//  The pixel aspect ratio of a clip or image.
double OfxClipInstance::getAspectRatio() const
{
    return _node->getProjectPixelAspectRatio();
}

// Frame Rate -
double OfxClipInstance::getFrameRate() const
{
    return _node->getFrameRate();
}

// Frame Range (startFrame, endFrame) -
//
//  The frame range over which a clip has images.
void OfxClipInstance::getFrameRange(double &startFrame, double &endFrame) const
{
    startFrame = 1;
    endFrame = 1;
}

/// Field Order - Which spatial field occurs temporally first in a frame.
/// \returns
///  - kOfxImageFieldNone - the clip material is unfielded
///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
const std::string &OfxClipInstance::getFieldOrder() const
{
    return _node->getDefaultOutputFielding();
}

// Connected -
//
//  Says whether the clip is actually connected at the moment.
bool OfxClipInstance::getConnected() const
{
    return _node->inputCount() > 0;
}

// Unmapped Frame Rate -
//
//  The unmaped frame range over which an output clip has images.
double OfxClipInstance::getUnmappedFrameRate() const
{
    
    return 25;
}

// Unmapped Frame Range -
//
//  The unmaped frame range over which an output clip has images.
// this is applicable only to hosts and plugins that allow a plugin to change frame rates
void OfxClipInstance::getUnmappedFrameRange(double &unmappedStartFrame, double &unmappedEndFrame) const
{
    unmappedStartFrame = 1;
    unmappedEndFrame = 1;
}

// Continuous Samples -
//
//  0 if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
//  1 if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
bool OfxClipInstance::getContinuousSamples() const
{
    return false;
}


/// override this to return the rod on the clip cannoical coords!
OfxRectD OfxClipInstance::getRegionOfDefinition(OfxTime) const
{
    OfxRectD v;
    const Box2D& bbox = _node->getInfo()->getDataWindow();
    v.x1 = bbox.x();
    v.y1 = bbox.y();
    v.x2 = bbox.right()-1;
    v.y2 = bbox.top()-1;
    return v;
}


/// override this to fill in the image at the given time.
/// The bounds of the image on the image plane should be
/// 'appropriate', typically the value returned in getRegionsOfInterest
/// on the effect instance. Outside a render call, the optionalBounds should
/// be 'appropriate' for the.
/// If bounds is not null, fetch the indicated section of the canonical image plane.
OFX::Host::ImageEffect::Image* OfxClipInstance::getImage(OfxTime time, OfxRectD *optionalBounds)
{
    //    if(_name == "Output") {
    //        if(!_outputImage) {
    //            // make a new ref counted image
    //            _outputImage = new MyImage(*this, 0);
    //        }
    //
    //        // add another reference to the member image for this fetch
    //        // as we have a ref count of 1 due to construction, this will
    //        // cause the output image never to delete by the plugin
    //        // when it releases the image
    //        _outputImage->addReference();
    //
    //        // return it
    //        return _outputImage;
    //    }
    //    else {
    //        // Fetch on demand for the input clip.
    //        // It does get deleted after the plugin is done with it as we
    //        // have not incremented the auto ref
    //        //
    //        // You should do somewhat more sophisticated image management
    //        // than this.
    //        MyImage *image = new MyImage(*this, time);
    //        return image;
    //    }
    OfxRectD roi;
    if(optionalBounds){
        roi = *optionalBounds;
    }else{
        double w,h,x,y;
        _node->getProjectExtent(w, h);
        _node->getProjectOffset(x, y);
        roi.x1 = x;
        roi.x2 = w;
        roi.y1 = y;
        roi.y2 = h;
    }
    
    if(isOutput()){
        if (!_outputImage) {
            _outputImage = new OfxImage(roi,*this,0);
        }
        _outputImage->addReference();
        return _outputImage;
    }else{
        Node* input = getAssociatedNode();
        if(input->isOpenFXNode()){
            OfxRectI roiInput;
            roiInput.x1 = roi.x1;
            roiInput.x2 = roi.x2;
            roiInput.y1 = roi.y1;
            roiInput.y2 = roi.y2;
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            OfxNode* ofxInputNode = dynamic_cast<OfxNode*>(input);
            ofxInputNode->renderAction(0, kOfxImageFieldNone, roiInput , renderScale);
            OFX::Host::ImageEffect::ClipInstance* clip = ofxInputNode->getClip("Output");
            return clip->getImage(time, optionalBounds);
        }else{
            ImageFetcher srcImg(input,roi.x1,roi.y1,roi.x2-1,roi.y2-1,Mask_RGBA);
            srcImg.claimInterest(true);
            OfxImage* ret = new OfxImage(roi,*this,0);
            
            /*Copying all rows living in the InputFetcher to the ofx image*/
            try{
                for (int y = roi.y1; y < roi.y2; y++) {
                    OfxRGBAColourF* dstImg = ret->pixel(0, y);
                    const InputRow& row = srcImg.at(y);
                    const float* r = row[Channel_red];
                    const float* g = row[Channel_green];
                    const float* b = row[Channel_blue];
                    const float* a = row[Channel_alpha];
                    if(r)
                        OfxImage::rowPlaneToOfxPackedBuffer(Channel_red, r+row.offset(), row.right()-row.offset(), dstImg);
                    else
                        OfxImage::rowPlaneToOfxPackedBuffer(Channel_red, NULL , row.right()-row.offset(), dstImg);
                    if(g)
                        OfxImage::rowPlaneToOfxPackedBuffer(Channel_green, g+row.offset(), row.right()-row.offset(), dstImg);
                    else
                        OfxImage::rowPlaneToOfxPackedBuffer(Channel_green, NULL , row.right()-row.offset(), dstImg);
                    if(b)
                        OfxImage::rowPlaneToOfxPackedBuffer(Channel_blue, b+row.offset(), row.right()-row.offset(), dstImg);
                    else
                        OfxImage::rowPlaneToOfxPackedBuffer(Channel_blue, NULL , row.right()-row.offset(), dstImg);
                    if(a)
                        OfxImage::rowPlaneToOfxPackedBuffer(Channel_alpha, a+row.offset(), row.right()-row.offset(), dstImg);
                    else
                        OfxImage::rowPlaneToOfxPackedBuffer(Channel_alpha, NULL , row.right()-row.offset(), dstImg);
                }
            }catch(const std::string& str){
                cout << str << endl;
            }
            return ret;
        }
    }
    return NULL;
}

void OfxImage::rowPlaneToOfxPackedBuffer(Channel channel,
                                         const float* plane,
                                         int w,
                                         OfxRGBAColourF* dst
                                         ){
    
    if(plane){
        if(channel == Channel_red)
            for (int i = 0; i < w; i++) {
                dst[i].r = plane[i];
            }
        else if(channel == Channel_green)
            for (int i = 0; i < w; i++) {
                dst[i].g = plane[i];
            }
        else if(channel == Channel_blue)
            for (int i = 0; i < w; i++) {
                dst[i].b = plane[i];
            }
        else if(channel == Channel_alpha)
            for (int i = 0; i < w; i++) {
                dst[i].a = plane[i];
            }
    }else{
        if(channel == Channel_red)
            for (int i = 0; i < w; i++) {
                dst[i].r = 0.f;
            }
        else if(channel == Channel_green)
            for (int i = 0; i < w; i++) {
                dst[i].g = 0.f;
            }
        else if(channel == Channel_blue)
            for (int i = 0; i < w; i++) {
                dst[i].b = 0.f;
            }
        else if(channel == Channel_alpha)
            for (int i = 0; i < w; i++) {
                dst[i].a = 1.f;
            }
    }
}

void OfxImage::ofxPackedBufferToRowPlane(Powiter::Channel channel,
                               const OfxRGBAColourF* src,
                               int w,
                               float* plane){
    if(channel == Channel_red)
        for (int i = 0; i < w; i++) {
            plane[i] =  src[i].r ;
        }
    else if(channel == Channel_green)
        for (int i = 0; i < w; i++) {
            plane[i] =  src[i].g ;
        }
    else if(channel == Channel_blue)
        for (int i = 0; i < w; i++) {
            plane[i] =  src[i].b ;
        }
    else if(channel == Channel_alpha)
        for (int i = 0; i < w; i++) {
            plane[i] =  src[i].a ;
        }
}


OfxImage::OfxImage(const OfxRectD& bounds,OfxClipInstance &clip, OfxTime)
: OFX::Host::ImageEffect::Image(clip) /// this ctor will set basic props on the image
{
    // make some memory
    _data = new OfxRGBAColourF[(int)((bounds.x2-bounds.x1) * (bounds.y2-bounds.y1))] ;
    
    // render scale x and y of 1.0
    setDoubleProperty(kOfxImageEffectPropRenderScale, 1.0, 0);
    setDoubleProperty(kOfxImageEffectPropRenderScale, 1.0, 1);
    
    // data ptr
    setPointerProperty(kOfxImagePropData,_data);
    
    // bounds and rod
    setIntProperty(kOfxImagePropBounds, bounds.x1, 0);
    setIntProperty(kOfxImagePropBounds, bounds.y1, 1);
    setIntProperty(kOfxImagePropBounds, bounds.x2, 2);
    setIntProperty(kOfxImagePropBounds, bounds.y2, 3);
    
    setIntProperty(kOfxImagePropRegionOfDefinition, bounds.x1, 0);
    setIntProperty(kOfxImagePropRegionOfDefinition, bounds.y1, 1);
    setIntProperty(kOfxImagePropRegionOfDefinition, bounds.x2, 2);
    setIntProperty(kOfxImagePropRegionOfDefinition, bounds.y2, 3);
    
    // row bytes
    setIntProperty(kOfxImagePropRowBytes, (bounds.x2-bounds.x1) * sizeof(OfxRGBAColourF));
    
    setStringProperty(kOfxImageEffectPropComponents, kOfxImageComponentRGBA);
}

OfxRGBAColourF* OfxImage::pixel(int x, int y) const
{
    OfxRectI bounds = getBounds();
    if ((x >= bounds.x1) && ( x< bounds.x2) && ( y >= bounds.y1) && ( y < bounds.y2) )
    {
        int rowBytes = getIntProperty(kOfxImagePropRowBytes);
        int offset = (y - bounds.y1) * (rowBytes/sizeof(OfxRGBAColourF)) + (x - bounds.x1);
        return &(_data[offset]);
    }
    return 0;
}

OfxImage::~OfxImage()
{
    delete [] _data;
}

