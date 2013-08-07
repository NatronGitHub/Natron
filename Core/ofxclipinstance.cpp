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
#include "Superviser/powiterFn.h"

using namespace Powiter;

OfxClipInstance::OfxClipInstance(OfxNode* effect, OFX::Host::ImageEffect::ClipDescriptor* desc):
OFX::Host::ImageEffect::ClipInstance(effect, *desc),
_node(effect)
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
    bool rgb = false;
    bool alpha = false;
    
    const ChannelSet& channels = _node->getInfo()->channels();
    if(channels & alpha) alpha = true;
    if(channels & Mask_RGB) rgb = true;
    
    if(!rgb && !alpha) return kOfxImageComponentNone;
    else if(rgb && !alpha) return kOfxImageComponentRGB;
    else if(!rgb && alpha) return kOfxImageComponentAlpha;
    else return kOfxImageComponentRGBA;
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
    startFrame = 0;
    endFrame = 25;
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
    unmappedStartFrame = 0;
    unmappedEndFrame = 25;
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
}
