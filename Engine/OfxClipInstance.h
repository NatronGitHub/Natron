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

#ifndef NATRON_ENGINE_OFXCLIPINSTANCE_H_
#define NATRON_ENGINE_OFXCLIPINSTANCE_H_


#include <cassert>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)
//ofx
#include <ofxhImageEffect.h>
#include <ofxPixels.h>

#include "Global/GlobalDefines.h"

#include "Engine/ChannelSet.h"
#include "Engine/Image.h"
#include "Engine/ThreadStorage.h"

class OfxImage;
class OfxEffectInstance;
namespace Natron {
    class EffectInstance;
    class OfxImageEffectInstance;
    class Image;
    class Node;

}

class OfxClipInstance : public OFX::Host::ImageEffect::ClipInstance
{
public:
    OfxClipInstance(OfxEffectInstance* node
                    ,Natron::OfxImageEffectInstance* effect
                    ,int index
                    , OFX::Host::ImageEffect::ClipDescriptor* desc);
    
    virtual ~OfxClipInstance(){}
    
    
    /// Get the Raw Unmapped Pixel Depth from the host
    ///
    /// \returns
    ///    - kOfxBitDepthNone (implying a clip is unconnected image)
    ///    - kOfxBitDepthByte
    ///    - kOfxBitDepthShort
    ///    - kOfxBitDepthFloat
    const std::string &getUnmappedBitDepth() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    /// Get the Raw Unmapped Components from the host
    ///
    /// \returns
    ///     - kOfxImageComponentNone (implying a clip is unconnected, not valid for an image)
    ///     - kOfxImageComponentRGBA
    ///     - kOfxImageComponentAlpha
    virtual const std::string &getUnmappedComponents() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    // PreMultiplication -
    //
    //  kOfxImageOpaque - the image is opaque and so has no premultiplication state
    //  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
    //  kOfxImageUnPreMultiplied - the image is unpremultiplied
    virtual const std::string &getPremult() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    // Pixel Aspect Ratio -
    //
    //  The pixel aspect ratio of a clip or image.
    virtual double getAspectRatio() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    // Frame Rate -
    //
    //  The frame rate of a clip or instance's project.
    virtual double getFrameRate() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    // Frame Range (startFrame, endFrame) -
    //
    //  The frame range over which a clip has images.
    virtual void getFrameRange(double &startFrame, double &endFrame) const OVERRIDE FINAL;
    
    /// Field Order - Which spatial field occurs temporally first in a frame.
    /// \returns
    ///  - kOfxImageFieldNone - the clip material is unfielded
    ///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
    ///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
    virtual const std::string &getFieldOrder() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    // Connected -
    //
    //  Says whether the clip is actually connected at the moment.
    virtual bool getConnected() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    // Unmapped Frame Rate -
    //
    //  The unmaped frame range over which an output clip has images.
    virtual double getUnmappedFrameRate() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    // Unmapped Frame Range -
    //
    //  The unmaped frame range over which an output clip has images.
    virtual void getUnmappedFrameRange(double &unmappedStartFrame, double &unmappedEndFrame) const OVERRIDE FINAL;
    
    // Continuous Samples -
    //
    //  0 if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
    //  1 if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
    virtual bool getContinuousSamples() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    /// override this to fill in the image at the given time.
    /// The bounds of the image on the image plane should be
    /// 'appropriate', typically the value returned in getRegionsOfInterest
    /// on the effect instance. Outside a render call, the optionalBounds should
    /// be 'appropriate' for the.
    /// If bounds is not null, fetch the indicated section of the canonical image plane.
    virtual OFX::Host::ImageEffect::Image* getImage(OfxTime time, OfxRectD *optionalBounds) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    /// override this to return the rod on the clip
    virtual OfxRectD getRegionOfDefinition(OfxTime time) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// override this to fill in the image at the given time from a specific view
    /// (using the standard callback gets you the current view being rendered, @see getImage).
    /// The bounds of the image on the image plane should be
    /// 'appropriate', typically the value returned in getRegionsOfInterest
    /// on the effect instance. Outside a render call, the optionalBounds should
    /// be 'appropriate' for the.
    /// If bounds is not null, fetch the indicated section of the canonical image plane.
    virtual  OFX::Host::ImageEffect::Image* getStereoscopicImage(OfxTime time, int view, OfxRectD *optionalBounds) OVERRIDE FINAL WARN_UNUSED_RETURN;


    /// override this to set the view to be returned by getImage()
    /// This is called by Instance::renderAction() for each clip, before calling
    /// kOfxImageEffectActionRender on the Instance.
    /// The view number has to be stored in the Clip, so this is typically not thread-safe,
    /// except if thread-local storage is used.
    virtual void setView(int view) OVERRIDE FINAL;
    
    void setRenderScale(const OfxPointD& scale);

    //returns the index of this clip if it is an input clip, otherwise -1.
    int getInputNb() const WARN_UNUSED_RETURN;
    
    Natron::EffectInstance* getAssociatedNode() const WARN_UNUSED_RETURN;

private:
    
    OFX::Host::ImageEffect::Image* getImageInternal(OfxTime time,const OfxPointD& renderScale, int view, OfxRectD *optionalBounds);
    
    OfxEffectInstance* _nodeInstance;
    Natron::OfxImageEffectInstance* const _effect;
    
    struct LastRenderArgs
    {
        OfxPointD scale;
        int view;
    };
    
    Natron::ThreadStorage<LastRenderArgs> _lastRenderArgs; //< foreach render thread, the args
};

class OfxImage : public OFX::Host::ImageEffect::Image
{
     
    
    public :
    
    /** @brief Enumerates the pixel depths supported */
    enum BitDepthEnum {eBitDepthNone = 0, /**< @brief bit depth that indicates no data is present */
        eBitDepthUByte,
        eBitDepthUShort,
        eBitDepthFloat,
        eBitDepthDouble
    };
    /** @brief Enumerates the component types supported */
    enum PixelComponentEnum {ePixelComponentNone = 0,
        ePixelComponentRGBA,
        ePixelComponentRGB,
        ePixelComponentAlpha
    };
    
    
    explicit OfxImage(boost::shared_ptr<Natron::Image> internalImage,OfxClipInstance &clip);
    
    virtual ~OfxImage(){}
    
    BitDepthEnum bitDepth() const {return _bitDepth;}
    
    OfxRGBAColourF* pixelF(int x, int y) const;
   
    boost::shared_ptr<Natron::Image> getInternalImageF() const {return _floatImage;}

private :
    
    BitDepthEnum _bitDepth;
    boost::shared_ptr<Natron::Image> _floatImage;

};

#endif // NATRON_ENGINE_OFXCLIPINSTANCE_H_
