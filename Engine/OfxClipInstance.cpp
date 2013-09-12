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
#include "OfxClipInstance.h"

#include <cfloat>

#include "Engine/OfxNode.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/ImageFetcher.h"
#include "Global/Macros.h"

using namespace Powiter;
using namespace std;

OfxClipInstance::OfxClipInstance(OfxNode* node,
                                 Powiter::OfxImageEffectInstance* effect,
                                 int /*index*/,
                                 OFX::Host::ImageEffect::ClipDescriptor* desc)
: OFX::Host::ImageEffect::ClipInstance(effect, *desc)
, _node(node)
, _effect(effect)
//, _clipIndex(index)
, _outputImage(NULL)
{
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
    
    //bool rgb = false;
    //bool alpha = false;
    
    //const ChannelSet& channels = _effect->info().channels();
    //if(channels & alpha) alpha = true;
    //if(channels & Mask_RGB) rgb = true;
    
//    if(!rgb && !alpha) return noneStr;
//    else if(rgb && !alpha) return rgbStr;
//    else if(!rgb && alpha) return alphaStr;
     return rgbaStr;
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
    return _effect->getProjectPixelAspectRatio();
}

// Frame Rate -
double OfxClipInstance::getFrameRate() const
{
    return _effect->getFrameRate();
}

// Frame Range (startFrame, endFrame) -
//
//  The frame range over which a clip has images.
void OfxClipInstance::getFrameRange(double &startFrame, double &endFrame) const
{
    if (_effect->getContext() == kOfxImageEffectContextGenerator) {
        startFrame = 0;
        endFrame = 0;
    }else{
        startFrame = _node->info().firstFrame();
        endFrame = _node->info().lastFrame();
    }
}

/// Field Order - Which spatial field occurs temporally first in a frame.
/// \returns
///  - kOfxImageFieldNone - the clip material is unfielded
///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
const std::string &OfxClipInstance::getFieldOrder() const
{
    return _effect->getDefaultOutputFielding();
}

// Connected -
//
//  Says whether the clip is actually connected at the moment.
bool OfxClipInstance::getConnected() const
{
    return _node->hasOutputConnected();
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
    if(getName() == kOfxImageEffectOutputClipName){
        const Box2D& bbox = _node->info().dataWindow();
        v.x1 = bbox.left();
        v.y1 = bbox.bottom();
        v.x2 = bbox.right();
        v.y2 = bbox.top();
    }else{
        
    }
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
    OfxRectD roi;
    if(optionalBounds){
        roi = *optionalBounds;
    }else{
        double w,h,x,y;
        _effect->getProjectExtent(w, h);
        _effect->getProjectOffset(x, y);
        roi.x1 = x;
        roi.x2 = x+w;
        roi.y1 = y;
        roi.y2 = y+h;
    }
    
    /*Return an empty image if the input is optional*/
    if(isOptional()){
        return new OfxImage(OfxImage::eBitDepthFloat,roi,*this,0);
    }
    
    /*SHOULD CHECK WHAT BIT DEPTH IS SUPPORTED BY THE PLUGIN INSTEAD OF GIVING FLOAT
     _effect->isPixelDepthSupported(...)
     */

    if(isOutput()){
        if (!_outputImage) {
            _outputImage = new OfxImage(OfxImage::eBitDepthFloat,roi,*this,0);
        }
        _outputImage->addReference();
        return _outputImage;
    }else{
        Node* input = getAssociatedNode();
        assert(input);
        if(input->isOpenFXNode()){
            OfxRectI roiInput;
            roiInput.x1 = roi.x1;
            roiInput.x2 = roi.x2;
            roiInput.y1 = roi.y1;
            roiInput.y2 = roi.y2;
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            OfxNode* ofxInputNode = dynamic_cast<OfxNode*>(input);
            ofxInputNode->effectInstance()->renderAction(0, kOfxImageFieldNone, roiInput , renderScale);
            OFX::Host::ImageEffect::ClipInstance* clip = ofxInputNode->effectInstance()->getClip(kOfxImageEffectOutputClipName);
            return clip->getImage(time, optionalBounds);
        }else{
            ImageFetcher srcImg(input,roi.x1,roi.y1,roi.x2-1,roi.y2-1,Mask_RGBA);
            srcImg.claimInterest(true);
            OfxImage* ret = new OfxImage(OfxImage::eBitDepthFloat,roi,*this,0);
            assert(ret);
            /*Copying all rows living in the InputFetcher to the ofx image*/
            for (int y = roi.y1; y < roi.y2; ++y) {
                OfxRGBAColourF* dstImg = ret->pixelF(0, y);
                assert(dstImg);
                Row* row = srcImg.at(y);
                if(!row){
                    cout << "Couldn't find row for index " << y << "...skipping this row" << endl;
                    continue;
                }
                const float* r = (*row)[Channel_red];
                const float* g = (*row)[Channel_green];
                const float* b = (*row)[Channel_blue];
                const float* a = (*row)[Channel_alpha];
                if(r)
                    rowPlaneToOfxPackedBuffer(Channel_red, r+row->offset(), row->right()-row->offset(), dstImg);
                else
                    rowPlaneToOfxPackedBuffer(Channel_red, NULL , row->right()-row->offset(), dstImg);
                if(g)
                    rowPlaneToOfxPackedBuffer(Channel_green, g+row->offset(), row->right()-row->offset(), dstImg);
                else
                    rowPlaneToOfxPackedBuffer(Channel_green, NULL , row->right()-row->offset(), dstImg);
                if(b)
                    rowPlaneToOfxPackedBuffer(Channel_blue, b+row->offset(), row->right()-row->offset(), dstImg);
                else
                    rowPlaneToOfxPackedBuffer(Channel_blue, NULL , row->right()-row->offset(), dstImg);
                if(a)
                    rowPlaneToOfxPackedBuffer(Channel_alpha, a+row->offset(), row->right()-row->offset(), dstImg);
                else
                    rowPlaneToOfxPackedBuffer(Channel_alpha, NULL , row->right()-row->offset(), dstImg);
            }
            
            return ret;
        }
    }
    return NULL;
}

OfxImage::OfxImage(BitDepthEnum bitDepth,const OfxRectD& bounds,OfxClipInstance &clip, OfxTime /*t*/):
OFX::Host::ImageEffect::Image(clip),_bitDepth(bitDepth){
    size_t pixSize = 0;
    if(bitDepth == eBitDepthUByte){
        pixSize = 4;
    }else if(bitDepth == eBitDepthUShort){
        pixSize = 8;
    }else if(bitDepth == eBitDepthFloat){
        pixSize = 16;
    }
    _data = malloc((int)((bounds.x2-bounds.x1) * (bounds.y2-bounds.y1))*pixSize) ;
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
    setIntProperty(kOfxImagePropRowBytes, (bounds.x2-bounds.x1) * pixSize);
    setStringProperty(kOfxImageEffectPropComponents, kOfxImageComponentRGBA);
}

OfxRGBAColourB* OfxImage::pixelB(int x, int y) const{
    assert(_bitDepth == eBitDepthUByte);
    OfxRectI bounds = getBounds();
    if ((x >= bounds.x1) && ( x< bounds.x2) && ( y >= bounds.y1) && ( y < bounds.y2) )
    {
        OfxRGBAColourB* p = reinterpret_cast<OfxRGBAColourB*>(_data);
        return &(p[(y - bounds.y1) * (bounds.x2-bounds.x1) + (x - bounds.x1)]);
    }
    return 0;
}
OfxRGBAColourS* OfxImage::pixelS(int x, int y) const{
    assert(_bitDepth == eBitDepthUShort);
    OfxRectI bounds = getBounds();
    if ((x >= bounds.x1) && ( x< bounds.x2) && ( y >= bounds.y1) && ( y < bounds.y2) )
    {
        OfxRGBAColourS* p = reinterpret_cast<OfxRGBAColourS*>(_data);
        return &(p[(y - bounds.y1) * (bounds.x2-bounds.x1) + (x - bounds.x1)]);
    }
    return 0;
}
OfxRGBAColourF* OfxImage::pixelF(int x, int y) const{
    assert(_bitDepth == eBitDepthFloat);
    OfxRectI bounds = getBounds();
    if ((x >= bounds.x1) && ( x< bounds.x2) && ( y >= bounds.y1) && ( y < bounds.y2) )
    {
        OfxRGBAColourF* p = reinterpret_cast<OfxRGBAColourF*>(_data);
        return &(p[(y - bounds.y1) * (bounds.x2-bounds.x1) + (x - bounds.x1)]);
    }
    return 0;
}

Node* OfxClipInstance::getAssociatedNode() const {
    if(isOutput())
        return _node;
    else{
        int index = 0;
        OfxNode::MappedInputV inputs = _node->inputClipsCopyWithoutOutput();
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (inputs[i]->getName() == getName()) {
                index = i;
                break;
            }
        }
        return _node->input(inputs.size()-1-index);
    }
}

