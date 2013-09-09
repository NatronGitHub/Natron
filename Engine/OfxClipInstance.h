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

#ifndef POWITER_ENGINE_OFXCLIPINSTANCE_H_
#define POWITER_ENGINE_OFXCLIPINSTANCE_H_


#include <cassert>
//ofx
#include <ofxhImageEffect.h>
#include <ofxPixels.h>

#include "Engine/ChannelSet.h"

class OfxImage;
class OfxNode;
class Node;
namespace Powiter {
    class OfxImageEffectInstance;
}

class OfxClipInstance : public OFX::Host::ImageEffect::ClipInstance
{
public:
    OfxClipInstance(OfxNode* node,Powiter::OfxImageEffectInstance* effect,int index, OFX::Host::ImageEffect::ClipDescriptor* desc);
    
    virtual ~OfxClipInstance(){}
    
    
    /// Get the Raw Unmapped Pixel Depth from the host
    ///
    /// \returns
    ///    - kOfxBitDepthNone (implying a clip is unconnected image)
    ///    - kOfxBitDepthByte
    ///    - kOfxBitDepthShort
    ///    - kOfxBitDepthFloat
    const std::string &getUnmappedBitDepth() const OVERRIDE;
    
    /// Get the Raw Unmapped Components from the host
    ///
    /// \returns
    ///     - kOfxImageComponentNone (implying a clip is unconnected, not valid for an image)
    ///     - kOfxImageComponentRGBA
    ///     - kOfxImageComponentAlpha
    virtual const std::string &getUnmappedComponents() const OVERRIDE;
    
    // PreMultiplication -
    //
    //  kOfxImageOpaque - the image is opaque and so has no premultiplication state
    //  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
    //  kOfxImageUnPreMultiplied - the image is unpremultiplied
    virtual const std::string &getPremult() const OVERRIDE;
    
    // Pixel Aspect Ratio -
    //
    //  The pixel aspect ratio of a clip or image.
    virtual double getAspectRatio() const OVERRIDE;
    
    // Frame Rate -
    //
    //  The frame rate of a clip or instance's project.
    virtual double getFrameRate() const OVERRIDE;
    
    // Frame Range (startFrame, endFrame) -
    //
    //  The frame range over which a clip has images.
    virtual void getFrameRange(double &startFrame, double &endFrame) const OVERRIDE;
    
    /// Field Order - Which spatial field occurs temporally first in a frame.
    /// \returns
    ///  - kOfxImageFieldNone - the clip material is unfielded
    ///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
    ///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
    virtual const std::string &getFieldOrder() const OVERRIDE;
    
    // Connected -
    //
    //  Says whether the clip is actually connected at the moment.
    virtual bool getConnected() const OVERRIDE;
    
    // Unmapped Frame Rate -
    //
    //  The unmaped frame range over which an output clip has images.
    virtual double getUnmappedFrameRate() const OVERRIDE;
    
    // Unmapped Frame Range -
    //
    //  The unmaped frame range over which an output clip has images.
    virtual void getUnmappedFrameRange(double &unmappedStartFrame, double &unmappedEndFrame) const OVERRIDE;
    
    // Continuous Samples -
    //
    //  0 if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
    //  1 if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
    virtual bool getContinuousSamples() const OVERRIDE;
    
    /// override this to fill in the image at the given time.
    /// The bounds of the image on the image plane should be
    /// 'appropriate', typically the value returned in getRegionsOfInterest
    /// on the effect instance. Outside a render call, the optionalBounds should
    /// be 'appropriate' for the.
    /// If bounds is not null, fetch the indicated section of the canonical image plane.
    virtual OFX::Host::ImageEffect::Image* getImage(OfxTime time, OfxRectD *optionalBounds) OVERRIDE;
    
    /// override this to return the rod on the clip
    virtual OfxRectD getRegionOfDefinition(OfxTime time) const OVERRIDE;

private:
    Node* getAssociatedNode() const;

private:
    OfxNode* _node;
    Powiter::OfxImageEffectInstance* _effect;
    //int _clipIndex;
    OfxImage* _outputImage;
};

class OfxImage : public OFX::Host::ImageEffect::Image
{
     
    
    public :
    
    /** @brief Enumerates the pixel depths supported */
    enum BitDepthEnum {eBitDepthNone = 0, /**< @brief bit depth that indicates no data is present */
        eBitDepthUByte,
        eBitDepthUShort,
        eBitDepthFloat
    };
    /** @brief Enumerates the component types supported */
    enum PixelComponentEnum {ePixelComponentNone = 0,
        ePixelComponentRGBA,
        ePixelComponentRGB,
        ePixelComponentAlpha
    };
    

    
    explicit OfxImage(BitDepthEnum bitDepth,const OfxRectD& bounds,OfxClipInstance &clip, OfxTime t);
    
    BitDepthEnum bitDepth() const {return _bitDepth;}
    
    /*3 different functions depending on the current bitdepth.
     Calling a version that is not suited to the current bit depth
     will raise an assertion.*/
    OfxRGBAColourB* pixelB(int x, int y) const;
    OfxRGBAColourS* pixelS(int x, int y) const;
    OfxRGBAColourF* pixelF(int x, int y) const;
    
    //void writeToQImage_debug(const std::string& filename);
    
    
    virtual ~OfxImage()
    {
        free(_data);
    }
    
    protected :
    
    void   *_data; // where we are keeping our image data
    BitDepthEnum _bitDepth;
};

inline void rowPlaneToOfxPackedBuffer(Powiter::Channel channel,
                               const float* plane,
                               int w,
                               OfxRGBAColourF* dst
                               ){
    assert(dst);
    if(plane){
        if(channel == Powiter::Channel_red)
            for (int i = 0; i < w; ++i) {
                dst[i].r = plane[i];
            }
        else if(channel == Powiter::Channel_green)
            for (int i = 0; i < w; ++i) {
                dst[i].g = plane[i];
            }
        else if(channel == Powiter::Channel_blue)
            for (int i = 0; i < w; ++i) {
                dst[i].b = plane[i];
            }
        else if(channel == Powiter::Channel_alpha)
            for (int i = 0; i < w; ++i) {
                dst[i].a = plane[i];
            }
    }else{
        if(channel == Powiter::Channel_red)
            for (int i = 0; i < w; ++i) {
                dst[i].r = 0.f;
            }
        else if(channel == Powiter::Channel_green)
            for (int i = 0; i < w; ++i) {
                dst[i].g = 0.f;
            }
        else if(channel == Powiter::Channel_blue)
            for (int i = 0; i < w; ++i) {
                dst[i].b = 0.f;
            }
        else if(channel == Powiter::Channel_alpha)
            for (int i = 0; i < w; ++i) {
                dst[i].a = 1.f;
            }
    }
    
}
template<typename RGBA_PIX_DEPTH>
void ofxPackedBufferToRowPlane(Powiter::Channel channel,
                               const RGBA_PIX_DEPTH* src,
                               int w,
                               float* plane){
    if(channel == Powiter::Channel_red)
    for (int i = 0; i < w; ++i) {
        plane[i] =  src[i].r ;
    }
    else if(channel == Powiter::Channel_green)
        for (int i = 0; i < w; ++i) {
            plane[i] =  src[i].g ;
        }
    else if(channel == Powiter::Channel_blue)
        for (int i = 0; i < w; ++i) {
            plane[i] =  src[i].b ;
        }
    else if(channel == Powiter::Channel_alpha)
        for (int i = 0; i < w; ++i) {
            plane[i] =  src[i].a ;
        }
    
}

#endif // POWITER_ENGINE_OFXCLIPINSTANCE_H_
