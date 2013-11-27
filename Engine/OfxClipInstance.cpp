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
#include "OfxClipInstance.h"

#include <cfloat>
#include <limits>

#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/ImageFetcher.h"
#include "Engine/Image.h"
#include "Engine/Hash64.h"

#include "Global/AppManager.h"
#include "Global/Macros.h"

using namespace Natron;

OfxClipInstance::OfxClipInstance(OfxEffectInstance* nodeInstance
                                 ,Natron::OfxImageEffectInstance* effect
                                 ,int /*index*/
                                 , OFX::Host::ImageEffect::ClipDescriptor* desc)
: OFX::Host::ImageEffect::ClipInstance(effect, *desc)
, _nodeInstance(nodeInstance)
, _effect(effect)
{
    assert(_nodeInstance);
    assert(_effect);
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
    static const std::string rgbStr(kOfxImageComponentRGB);
    static const std::string noneStr(kOfxImageComponentNone);
    static const std::string rgbaStr(kOfxImageComponentRGBA);
    static const std::string alphaStr(kOfxImageComponentAlpha);
    
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
    SequenceTime first = 0,last = 0;
    EffectInstance* n = getAssociatedNode();
    if(n)
        n->getFrameRange(&first, &last);
    startFrame = first;
    endFrame = last;
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
    return getAssociatedNode() != NULL;
}

// Unmapped Frame Rate -
//
//  The unmaped frame range over which an output clip has images.
double OfxClipInstance::getUnmappedFrameRate() const
{
    //return getNode().asImageEffectNode().getOutputFrameRate();
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
OfxRectD OfxClipInstance::getRegionOfDefinition(OfxTime time) const
{
    OfxRectD ret;
    RectI rod;
    EffectInstance* n = getAssociatedNode();
    if (n) {
        (void)n->getRegionOfDefinition(time,&rod);
        ret.x1 = rod.left();
        ret.x2 = rod.right();
        ret.y1 = rod.bottom();
        ret.y2 = rod.top();
    } else if(_nodeInstance->effectInstance()) {
        _nodeInstance->effectInstance()->getProjectOffset(ret.x1, ret.y1);
        _nodeInstance->effectInstance()->getProjectExtent(ret.x2, ret.y2);
    } else {
        // default value: should never happen
        assert(!"cannot compute ROD");
        ret.x1 = kOfxFlagInfiniteMin;
        ret.x2 = kOfxFlagInfiniteMax;
        ret.y1 = kOfxFlagInfiniteMin;
        ret.y2 = kOfxFlagInfiniteMax;
    }
    return ret;
}


/// override this to fill in the image at the given time.
/// The bounds of the image on the image plane should be
/// 'appropriate', typically the value returned in getRegionsOfInterest
/// on the effect instance. Outside a render call, the optionalBounds should
/// be 'appropriate' for the.
/// If bounds is not null, fetch the indicated section of the canonical image plane.
OFX::Host::ImageEffect::Image* OfxClipInstance::getImage(OfxTime time, OfxRectD *optionalBounds)
{
    return getImageInternal(time,_viewRendered.localData(),optionalBounds);
}

OFX::Host::ImageEffect::Image* OfxClipInstance::getImageInternal(OfxTime time, int view, OfxRectD */*optionalBounds*/){
    if(isOutput()){
        boost::shared_ptr<Natron::Image> outputImage = _nodeInstance->getImageBeingRendered(time,view);
        assert(outputImage);
        return new OfxImage(outputImage,*this);
    }else{
        RenderScale scale;
        scale.x = scale.y = 1.;
        // input has been rendered just find it in the cache
        boost::shared_ptr<const Natron::Image> image = _nodeInstance->getImage(getInputNb(),time, scale,view);
        if(!image){
            return NULL;
        }else{
            return new OfxImage(boost::const_pointer_cast<Natron::Image>(image),*this);
        }
    }

}

OfxImage::OfxImage(boost::shared_ptr<Natron::Image> internalImage,OfxClipInstance &clip):
OFX::Host::ImageEffect::Image(clip)
,_bitDepth(OfxImage::eBitDepthFloat)
,_floatImage(internalImage)
{
    RenderScale scale = internalImage->getRenderScale();
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.x, 0);
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.y, 1);
    // data ptr
    const RectI& rod = internalImage->getRoD();
    setPointerProperty(kOfxImagePropData,internalImage->pixelAt(rod.left(), rod.bottom()));
    // bounds and rod
    setIntProperty(kOfxImagePropBounds, rod.left(), 0);
    setIntProperty(kOfxImagePropBounds, rod.bottom(), 1);
    setIntProperty(kOfxImagePropBounds, rod.right(), 2);
    setIntProperty(kOfxImagePropBounds, rod.top(), 3);
    setIntProperty(kOfxImagePropRegionOfDefinition, rod.left(), 0);
    setIntProperty(kOfxImagePropRegionOfDefinition, rod.bottom(), 1);
    setIntProperty(kOfxImagePropRegionOfDefinition, rod.right(), 2);
    setIntProperty(kOfxImagePropRegionOfDefinition, rod.top(), 3);
    // row bytes
    setIntProperty(kOfxImagePropRowBytes, rod.width()*4*sizeof(float));
    setStringProperty(kOfxImageEffectPropComponents, kOfxImageComponentRGBA);
}

OfxRGBAColourF* OfxImage::pixelF(int x, int y) const{
    assert(_bitDepth == eBitDepthFloat);
    const RectI& bounds = _floatImage->getRoD();
    if ((x >= bounds.left()) && ( x < bounds.right()) && ( y >= bounds.bottom()) && ( y < bounds.top()) )
    {
        return reinterpret_cast<OfxRGBAColourF*>(_floatImage->pixelAt(x, y));
    }
    return 0;
}
int OfxClipInstance::getInputNb() const{
    if(_isOutput){
        return -1;
    }
    int index = 0;
    OfxEffectInstance::MappedInputV inputs = _nodeInstance->inputClipsCopyWithoutOutput();
    for (U32 i = 0; i < inputs.size(); ++i) {
        if (inputs[i]->getName() == getName()) {
            index = i;
            break;
        }
    }
    return inputs.size()-1-index;
}

Natron::EffectInstance* OfxClipInstance::getAssociatedNode() const {
    if(_isOutput)
        return _nodeInstance;
    else{
        return _nodeInstance->input(getInputNb());
    }
}
OFX::Host::ImageEffect::Image* OfxClipInstance::getStereoscopicImage(OfxTime time, int view, OfxRectD *optionalBounds) {
    return getImageInternal(time,view,optionalBounds);
}
void OfxClipInstance::setView(int view){
    _viewRendered.setLocalData(view);
}
