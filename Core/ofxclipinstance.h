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
#ifndef OFXCLIPINSTANCE_H
#define OFXCLIPINSTANCE_H

#include "Core/channels.h"

//ofx
#include "ofxhImageEffect.h"
#include "ofxpixels.h"

class OfxImage;
class OfxNode;
class Node;
class OfxClipInstance : public OFX::Host::ImageEffect::ClipInstance
{
    OfxNode* _node;
    int _clipIndex;
    OfxImage* _outputImage;
public:
    OfxClipInstance(int index,OfxNode* effect, OFX::Host::ImageEffect::ClipDescriptor* desc);
    
    virtual ~OfxClipInstance(){}
    
    /*Returns the node associated to this clip.
     For an input clip this is the input node
     corresponding to the clipIndex.
     For an output clip this is the _node member.*/
    Node* getAssociatedNode() const;
    
    /// Get the Raw Unmapped Pixel Depth from the host
    ///
    /// \returns
    ///    - kOfxBitDepthNone (implying a clip is unconnected image)
    ///    - kOfxBitDepthByte
    ///    - kOfxBitDepthShort
    ///    - kOfxBitDepthFloat
    const std::string &getUnmappedBitDepth() const;
    
    /// Get the Raw Unmapped Components from the host
    ///
    /// \returns
    ///     - kOfxImageComponentNone (implying a clip is unconnected, not valid for an image)
    ///     - kOfxImageComponentRGBA
    ///     - kOfxImageComponentAlpha
    virtual const std::string &getUnmappedComponents() const;
    
    // PreMultiplication -
    //
    //  kOfxImageOpaque - the image is opaque and so has no premultiplication state
    //  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
    //  kOfxImageUnPreMultiplied - the image is unpremultiplied
    virtual const std::string &getPremult() const;
    
    // Pixel Aspect Ratio -
    //
    //  The pixel aspect ratio of a clip or image.
    virtual double getAspectRatio() const;
    
    // Frame Rate -
    //
    //  The frame rate of a clip or instance's project.
    virtual double getFrameRate() const;
    
    // Frame Range (startFrame, endFrame) -
    //
    //  The frame range over which a clip has images.
    virtual void getFrameRange(double &startFrame, double &endFrame) const ;
    
    /// Field Order - Which spatial field occurs temporally first in a frame.
    /// \returns
    ///  - kOfxImageFieldNone - the clip material is unfielded
    ///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
    ///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
    virtual const std::string &getFieldOrder() const;
    
    // Connected -
    //
    //  Says whether the clip is actually connected at the moment.
    virtual bool getConnected() const;
    
    // Unmapped Frame Rate -
    //
    //  The unmaped frame range over which an output clip has images.
    virtual double getUnmappedFrameRate() const;
    
    // Unmapped Frame Range -
    //
    //  The unmaped frame range over which an output clip has images.
    virtual void getUnmappedFrameRange(double &unmappedStartFrame, double &unmappedEndFrame) const;
    
    // Continuous Samples -
    //
    //  0 if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
    //  1 if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
    virtual bool getContinuousSamples() const;
    
    /// override this to fill in the image at the given time.
    /// The bounds of the image on the image plane should be
    /// 'appropriate', typically the value returned in getRegionsOfInterest
    /// on the effect instance. Outside a render call, the optionalBounds should
    /// be 'appropriate' for the.
    /// If bounds is not null, fetch the indicated section of the canonical image plane.
    virtual OFX::Host::ImageEffect::Image* getImage(OfxTime time, OfxRectD *optionalBounds);
    
    /// override this to return the rod on the clip
    virtual OfxRectD getRegionOfDefinition(OfxTime time) const;
    
    
};


class OfxImage : public OFX::Host::ImageEffect::Image
{
    protected :
    OfxRGBAColourF   *_data; // where we are keeping our image data
    
    public :
    
    
    explicit OfxImage(const OfxRectD& bounds,OfxClipInstance &clip, OfxTime t);
    OfxRGBAColourF* pixel(int x, int y) const;
    
    static void rowPlaneToOfxPackedBuffer(Powiter::Channel channel,
                                          const float* plane,
                                          int w,
                                          OfxRGBAColourF* dst
                                          );

    static void ofxPackedBufferToRowPlane(Powiter::Channel channel,
                                          const OfxRGBAColourF* src,
                                          int w,
                                          float* plane);
    
    ~OfxImage();
};


#endif // OFXCLIPINSTANCE_H
