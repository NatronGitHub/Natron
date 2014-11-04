//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#include "OfxClipInstance.h"

#include <cfloat>
#include <limits>
#include <QDebug>
#include "Global/Macros.h"

#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/ImageFetcher.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/TimeLine.h"
#include "Engine/Hash64.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/RotoContext.h"

using namespace Natron;

OfxClipInstance::OfxClipInstance(OfxEffectInstance* nodeInstance
                                 ,
                                 Natron::OfxImageEffectInstance* effect
                                 ,
                                 int  /*index*/
                                 ,
                                 OFX::Host::ImageEffect::ClipDescriptor* desc)
    : OFX::Host::ImageEffect::ClipInstance(effect, *desc)
      , _nodeInstance(nodeInstance)
      , _effect(effect)
      , _aspectRatio(1.)
{
    assert(_nodeInstance);
    assert(_effect);
}

const std::string &
OfxClipInstance::getUnmappedBitDepth() const
{
    
    static const std::string byteStr(kOfxBitDepthByte);
    static const std::string shortStr(kOfxBitDepthShort);
    static const std::string floatStr(kOfxBitDepthFloat);
    static const std::string noneStr(kOfxBitDepthNone);
    EffectInstance* inputNode = getAssociatedNode();
    if (!isOutput() && inputNode) {
        inputNode = inputNode->getNearestNonIdentity(_nodeInstance->getApp()->getTimeLine()->currentFrame());
    }
    if (inputNode) {
        ///Get the input node's output preferred bit depth and componentns
        Natron::ImageComponentsEnum comp;
        Natron::ImageBitDepthEnum depth;
        inputNode->getPreferredDepthAndComponents(-1, &comp, &depth);
        
        
        
        
        switch (depth) {
            case Natron::eImageBitDepthByte:
                return byteStr;
                break;
            case Natron::eImageBitDepthShort:
                return shortStr;
                break;
            case Natron::eImageBitDepthFloat:
                return floatStr;
                break;
            default:
                break;
        }
    }
    
    ///Return the hightest bit depth supported by the plugin
    if (_nodeInstance) {
        std::string ret = _nodeInstance->effectInstance()->bestSupportedDepth(kOfxBitDepthFloat);
        if (ret == floatStr) {
            return floatStr;
        } else if (ret == shortStr) {
            return shortStr;
        } else if (ret == byteStr) {
            return byteStr;
        }
    }
    return noneStr;

}

const std::string &
OfxClipInstance::getUnmappedComponents() const
{

    static const std::string rgbStr(kOfxImageComponentRGB);
    static const std::string noneStr(kOfxImageComponentNone);
    static const std::string rgbaStr(kOfxImageComponentRGBA);
    static const std::string alphaStr(kOfxImageComponentAlpha);

    EffectInstance* inputNode = getAssociatedNode();
    if (!isOutput() && inputNode) {
        inputNode = inputNode->getNearestNonIdentity(_nodeInstance->getApp()->getTimeLine()->currentFrame());
    }
    if (inputNode) {
        ///Get the input node's output preferred bit depth and componentns
        Natron::ImageComponentsEnum comp;
        Natron::ImageBitDepthEnum depth;
        inputNode->getPreferredDepthAndComponents(-1, &comp, &depth);
        
        switch (comp) {
            case Natron::eImageComponentRGBA:
                return rgbaStr;
                break;
            case Natron::eImageComponentRGB:
                return rgbStr;
                break;
            case Natron::eImageComponentAlpha:
                return alphaStr;
                break;
            default:
                return rgbaStr;
                break;
        }
    } else {
        ///The node is not connected but optional, return the closest supported components
        ///of the first connected non optional input.
        if ( isOptional() ) {
            int nInputs = _nodeInstance->getMaxInputCount();
            for (int i  = 0 ; i < nInputs ; ++i) {
                OfxClipInstance* clip = _nodeInstance->getClipCorrespondingToInput(i);
                if (clip && !clip->isOptional() && clip->getConnected() && clip->getComponents() != noneStr) {
                    return clip->getComponents();
                }
            }
        }
        
        
        ///last-resort: black and transparant image means RGBA.
        return rgbaStr;
    }
}

// PreMultiplication -
//
//  kOfxImageOpaque - the image is opaque and so has no premultiplication state
//  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
//  kOfxImageUnPreMultiplied - the image is unpremultiplied
const std::string &
OfxClipInstance::getPremult() const
{
    static const std::string premultStr(kOfxImagePreMultiplied);
    static const std::string unPremultStr(kOfxImageUnPreMultiplied);
    static const std::string opaqueStr(kOfxImageOpaque);
    EffectInstance* effect =  getAssociatedNode() ;
    
    ///The clip might be identity and we want the data of the node that will be upstream not of the identity one
    if (!isOutput() && effect) {
        effect = effect->getNearestNonIdentity(_nodeInstance->getApp()->getTimeLine()->currentFrame());
    }
    if (effect) {
        
        ///Get the input node's output preferred bit depth and componentns
        Natron::ImageComponentsEnum comp;
        Natron::ImageBitDepthEnum depth;
        effect->getPreferredDepthAndComponents(-1, &comp, &depth);
        
        switch (comp) {
            case Natron::eImageComponentRGB:
                return opaqueStr;
            //case Natron::eImageComponentRGBA: // RGBA can be Opaque, PreMult or UnPreMult
            case Natron::eImageComponentAlpha:
                return premultStr;
            default:
                break;
        }
        Natron::ImagePremultiplicationEnum premult = effect->getOutputPremultiplication();
        
        switch (premult) {
            case Natron::eImagePremultiplicationOpaque:
                return opaqueStr;
            case Natron::eImagePremultiplicationPremultiplied:
                return premultStr;
            case Natron::eImagePremultiplicationUnPremultiplied:
                return unPremultStr;
            default:
                return opaqueStr;
        }
    }
    
    ///Input is not connected
    
    const std::string& comps = getComponents();
    if (comps == kOfxImageComponentRGBA || comps == kOfxImageComponentAlpha) {
        return premultStr;
    }
    
    ///Default to opaque
    return opaqueStr;
}

// Pixel Aspect Ratio -
//
//  The pixel aspect ratio of a clip or image.
double
OfxClipInstance::getAspectRatio() const
{
    if (isOutput()) {
        return _aspectRatio;
    }
    
    EffectInstance* input = getAssociatedNode();
    if (input) {
        return input->getPreferredAspectRatio();
    } else {
        Format f;
        _nodeInstance->getRenderFormat(&f);
        return f.getPixelAspectRatio();
    }
}

void
OfxClipInstance::setAspectRatio(double par)
{
    _aspectRatio = par;
}

// Frame Rate -
double
OfxClipInstance::getFrameRate() const
{
    return _effect->getFrameRate();
}

// Frame Range (startFrame, endFrame) -
//
//  The frame range over which a clip has images.
void
OfxClipInstance::getFrameRange(double &startFrame,
                               double &endFrame) const
{
    assert(_nodeInstance);
    EffectInstance* n = getAssociatedNode();
    if (n) {
        SequenceTime first,last;
        U64 hash = n->getRenderHash();
        n->getFrameRange_public(hash,&first, &last);
        startFrame = first;
        endFrame = last;
        
    } else {
        assert(_nodeInstance);
        assert( _nodeInstance->getApp() );
        assert( _nodeInstance->getApp()->getTimeLine() );
        startFrame = _nodeInstance->getApp()->getTimeLine()->leftBound();
        endFrame = _nodeInstance->getApp()->getTimeLine()->rightBound();
    }
}

/// Field Order - Which spatial field occurs temporally first in a frame.
/// \returns
///  - kOfxImageFieldNone - the clip material is unfielded
///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
const std::string &
OfxClipInstance::getFieldOrder() const
{
    return _effect->getDefaultOutputFielding();
}

// Connected -
//
//  Says whether the clip is actually connected at the moment.
bool
OfxClipInstance::getConnected() const
{
    ///a roto brush is always connected
    if ( (getName() == "Roto") && _nodeInstance->getNode()->isRotoNode() ) {
        return true;
    } else {
        if (_isOutput) {
            return _nodeInstance->hasOutputConnected();
        } else {
            int inputNb = getInputNb();
            if ( isMask() && !_nodeInstance->getNode()->isMaskEnabled(inputNb) ) {
                return false;
            }

            return _nodeInstance->getInput(inputNb) != NULL;
        }
    }
}

// Unmapped Frame Rate -
//
//  The unmaped frame range over which an output clip has images.
double
OfxClipInstance::getUnmappedFrameRate() const
{
    //return getNode().asImageEffectNode().getOutputFrameRate();
    return 25;
}

// Unmapped Frame Range -
//
//  The unmaped frame range over which an output clip has images.
// this is applicable only to hosts and plugins that allow a plugin to change frame rates
void
OfxClipInstance::getUnmappedFrameRange(double &unmappedStartFrame,
                                       double &unmappedEndFrame) const
{
    unmappedStartFrame = 1;
    unmappedEndFrame = 1;
}

// Continuous Samples -
//
//  0 if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
//  1 if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
bool
OfxClipInstance::getContinuousSamples() const
{
    return false;
}

/// override this to return the rod on the clip canonical coords!
OfxRectD
OfxClipInstance::getRegionOfDefinition(OfxTime time) const
{
    OfxRectD ret;
    RectD rod; // rod is in canonical coordinates
    unsigned int mipmapLevel;
    int view;
    Natron::EffectInstance* associatedNode = getAssociatedNode();

    /// The node might be disabled, hence we navigate upstream to find the first non disabled node.
    if (associatedNode) {
        associatedNode = associatedNode->getNearestNonDisabled();
    }
    ///We don't have to do the same kind of navigation if the effect is identity because the effect is supposed to have
    ///the same RoD as the input if it is identity.

    if (!associatedNode) {
        ///Doesn't matter, input is not connected
        mipmapLevel = 0;
        view = 0;
    } else {
        if (_lastActionData.hasLocalData()) {
            const ActionLocalData& args = _lastActionData.localData();
            if (args.isViewValid) {
                view = args.view;
            } else {
                view = 0;
            }
            if (args.isMipmapLevelValid) {
                mipmapLevel = args.mipMapLevel;
            } else {
                mipmapLevel = 0;
            }
        } else {
            mipmapLevel = 0;
            view = 0;
        }
        
    }


    if ( (getName() == "Roto") && _nodeInstance->getNode()->isRotoNode() ) {
        boost::shared_ptr<RotoContext> rotoCtx =  _nodeInstance->getNode()->getRotoContext();
        assert(rotoCtx);
        RectD rod;
        rotoCtx->getMaskRegionOfDefinition(time, view, &rod);
        ret.x1 = rod.x1;
        ret.x2 = rod.x2;
        ret.y1 = rod.y1;
        ret.y2 = rod.y2;

        return ret;
    }


    if (associatedNode) {
        bool isProjectFormat;

        U64 nodeHash = associatedNode->getRenderHash();
        
        RenderScale scale;
        scale.x = scale.y = Natron::Image::getScaleFromMipMapLevel(mipmapLevel);
        Natron::StatusEnum st = associatedNode->getRegionOfDefinition_public(nodeHash,time, scale, view, &rod, &isProjectFormat);
        if (st == eStatusFailed) {
            ret.x1 = 0.;
            ret.x2 = 0.;
            ret.y1 = 0.;
            ret.y2 = 0.;
        } else {
            ret.x1 = rod.left();
            ret.x2 = rod.right();
            ret.y1 = rod.bottom();
            ret.y2 = rod.top();
        }
        
    } else {
        ret.x1 = 0.;
        ret.x2 = 0.;
        ret.y1 = 0.;
        ret.y2 = 0.;
    }

    return ret;
} // getRegionOfDefinition

/// override this to fill in the image at the given time.
/// The bounds of the image on the image plane should be
/// 'appropriate', typically the value returned in getRegionsOfInterest
/// on the effect instance. Outside a render call, the optionalBounds should
/// be 'appropriate' for the.
/// If bounds is not null, fetch the indicated section of the canonical image plane.
OFX::Host::ImageEffect::Image*
OfxClipInstance::getImage(OfxTime time,
                          const OfxRectD *optionalBounds)
{
    return getStereoscopicImage(time, -1, optionalBounds);
}

OFX::Host::ImageEffect::Image*
OfxClipInstance::getImageInternal(OfxTime time,
                                  const OfxPointD & renderScale,
                                  int view,
                                  const OfxRectD *optionalBounds)
{
    assert( !isOutput() );
    // input has been rendered just find it in the cache
    RectD bounds;
    if (optionalBounds) {
        bounds.x1 = optionalBounds->x1;
        bounds.y1 = optionalBounds->y1;
        bounds.x2 = optionalBounds->x2;
        bounds.y2 = optionalBounds->y2;
    }

    Natron::ImageComponentsEnum comps =  ofxComponentsToNatronComponents( getComponents() );
    Natron::ImageBitDepthEnum bitDepth = ofxDepthToNatronDepth( getPixelDepth() );
    double par = getAspectRatio();
    RectI renderWindow;
    boost::shared_ptr<Natron::Image> image = _nodeInstance->getImage(getInputNb(), time, renderScale, view,
                                                                     optionalBounds ? &bounds : NULL,
                                                                     comps,
                                                                     bitDepth,
                                                                     par,
                                                                     false,&renderWindow);
    if (!image) {
        return NULL;
    } else {
        return new OfxImage(image,renderWindow,*this);
    }
}

std::string
OfxClipInstance::natronsComponentsToOfxComponents(Natron::ImageComponentsEnum comp)
{
    switch (comp) {
    case Natron::eImageComponentNone:

        return kOfxImageComponentNone;
        break;
    case Natron::eImageComponentAlpha:

        return kOfxImageComponentAlpha;
        break;
    case Natron::eImageComponentRGB:

        return kOfxImageComponentRGB;
        break;
    case Natron::eImageComponentRGBA:

        return kOfxImageComponentRGBA;
        break;
    default:
        assert(false);    //comp unsupported
        break;
    }
}

Natron::ImageComponentsEnum
OfxClipInstance::ofxComponentsToNatronComponents(const std::string & comp)
{
    if (comp ==  kOfxImageComponentRGBA) {
        return Natron::eImageComponentRGBA;
    } else if (comp == kOfxImageComponentAlpha) {
        return Natron::eImageComponentAlpha;
    } else if (comp == kOfxImageComponentRGB) {
        return Natron::eImageComponentRGB;
    } else if (comp == kOfxImageComponentNone) {
        return Natron::eImageComponentNone;
    } else {
        throw std::runtime_error(comp + ": unsupported component "); //< comp unsupported
    }
}

Natron::ImageBitDepthEnum
OfxClipInstance::ofxDepthToNatronDepth(const std::string & depth)
{
    if (depth == kOfxBitDepthByte) {
        return Natron::eImageBitDepthByte;
    } else if (depth == kOfxBitDepthShort) {
        return Natron::eImageBitDepthShort;
    } else if (depth == kOfxBitDepthFloat) {
        return Natron::eImageBitDepthFloat;
    } else if (depth == kOfxBitDepthNone) {
        return Natron::eImageBitDepthNone;
    } else {
        throw std::runtime_error(depth + ": unsupported bitdepth"); //< comp unsupported
    }
}

std::string
OfxClipInstance::natronsDepthToOfxDepth(Natron::ImageBitDepthEnum depth)
{
    switch (depth) {
    case Natron::eImageBitDepthByte:

        return kOfxBitDepthByte;
    case Natron::eImageBitDepthShort:

        return kOfxBitDepthShort;
    case Natron::eImageBitDepthFloat:

        return kOfxBitDepthFloat;
    case Natron::eImageBitDepthNone:

        return kOfxBitDepthNone;
    default:
        assert(false);    //< shouldve been caught earlier
        break;
    }
}

OfxImage::OfxImage(boost::shared_ptr<Natron::Image> internalImage,
                   const RectI& renderWindow,
                   OfxClipInstance &clip)
    : OFX::Host::ImageEffect::Image(clip)
      , _bitDepth(OfxImage::eBitDepthFloat)
      , _floatImage(internalImage)
{
    unsigned int mipMapLevel = internalImage->getMipMapLevel();
    RenderScale scale;

    scale.x = Natron::Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.x, 0);
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.y, 1);
    
   
    
    // data ptr
    const RectI & bounds = internalImage->getBounds();
    
    ///Do not activate this assert! The render window passed to renderRoI can be bigger than the actual RoD of the effect
    ///in which case it is just clipped to the RoD.
    //assert(bounds.contains(renderWindow));
    RectI pluginsSeenBounds;
    renderWindow.intersect(bounds, &pluginsSeenBounds);
    
    const RectD & rod = internalImage->getRoD(); // Not the OFX RoD!!! Natron::Image::getRoD() is in *CANONICAL* coordinates
    setPointerProperty( kOfxImagePropData,internalImage->pixelAt( pluginsSeenBounds.left(), pluginsSeenBounds.bottom() ) );
    
    ///We set the render window that was given to the render thread instead of the actual bounds of the image
    ///so we're sure the plug-in doesn't attempt to access outside pixels.
    setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.left(), 0);
    setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.bottom(), 1);
    setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.right(), 2);
    setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.top(), 3);
#pragma message WARN("The Image RoD should be in pixels everywhere in Natron!")
    // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImagePropRegionOfDefinition
    // " An image's region of definition, in *PixelCoordinates,* is the full frame area of the image plane that the image covers."
    // Natron::Image::getRoD() is in *CANONICAL* coordinates
    // OFX::Image RoD is in *PIXEL* coordinates
    RectI pixelRod;
    rod.toPixelEnclosing(mipMapLevel, internalImage->getPixelAspectRatio(), &pixelRod);
    setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.left(), 0);
    setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.bottom(), 1);
    setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.right(), 2);
    setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.top(), 3);
    // row bytes
    setIntProperty( kOfxImagePropRowBytes, bounds.width() *
                    Natron::getElementsCountForComponents( internalImage->getComponents() ) *
                    getSizeOfForBitDepth( internalImage->getBitDepth() ) );
    setStringProperty( kOfxImageEffectPropComponents, OfxClipInstance::natronsComponentsToOfxComponents( internalImage->getComponents() ) );
    setStringProperty( kOfxImageEffectPropPixelDepth, OfxClipInstance::natronsDepthToOfxDepth( internalImage->getBitDepth() ) );
    setStringProperty( kOfxImageEffectPropPreMultiplication, clip.getPremult() );
    setStringProperty(kOfxImagePropField, kOfxImageFieldNone);
    setStringProperty( kOfxImagePropUniqueIdentifier,QString::number(internalImage->getHashKey(), 16).toStdString() );
    setDoubleProperty( kOfxImagePropPixelAspectRatio, clip.getAspectRatio() );
}

OfxRGBAColourF*
OfxImage::pixelF(int x,
                 int y) const
{
    assert(_bitDepth == eBitDepthFloat);
    const RectI & bounds = _floatImage->getBounds();
    if ( ( x >= bounds.left() ) && ( x < bounds.right() ) && ( y >= bounds.bottom() ) && ( y < bounds.top() ) ) {
        return reinterpret_cast<OfxRGBAColourF*>( _floatImage->pixelAt(x, y) );
    }

    return 0;
}

int
OfxClipInstance::getInputNb() const
{
    if (_isOutput) {
        return -1;
    }
    int index = 0;
    OfxEffectInstance::MappedInputV inputs = _nodeInstance->inputClipsCopyWithoutOutput();
    for (U32 i = 0; i < inputs.size(); ++i) {
        if ( inputs[i]->getName() == getName() ) {
            index = i;
            break;
        }
    }

    return inputs.size() - 1 - index;
}

Natron::EffectInstance*
OfxClipInstance::getAssociatedNode() const
{
    assert(_nodeInstance);
    if ( (getName() == "Roto") && _nodeInstance->getNode()->isRotoNode() ) {
        return _nodeInstance;
    }
    if (_isOutput) {
        return _nodeInstance;
    } else {
        return _nodeInstance->getInput( getInputNb() );
    }
}

OFX::Host::ImageEffect::Image*
OfxClipInstance::getStereoscopicImage(OfxTime time,
                                      int view,
                                      const OfxRectD *optionalBounds)
{
    OfxPointD scale;
    
    scale.x = scale.y = 1.;
    
    bool hasLocalData = true;
    if ( !_lastActionData.hasLocalData() ) {
        hasLocalData = false;
#ifdef DEBUG
        if (QThread::currentThread() != qApp->thread()) {
            qDebug() << _nodeInstance->getNode()->getName_mt_safe().c_str() << " is trying to call clipGetImage on a thread "
            "not controlled by Natron (probably from the multi-thread suite).\n If you're a developer of that plug-in, please "
            "fix it.";
            
        }
#endif
    }
    assert( _lastActionData.hasLocalData() );
    if ( isOutput() ) {
        boost::shared_ptr<Natron::Image> outputImage;
        RectI renderWindow;
        bool ok = _nodeInstance->getThreadLocalRenderedImage(&outputImage,&renderWindow);
        if (!ok) {
            return NULL;
        }

        return new OfxImage(outputImage,renderWindow,*this);
    }

    unsigned int mipMapLevel;
    if (hasLocalData) {
        const ActionLocalData& args = _lastActionData.localData();
        if (!args.isViewValid) {
#ifdef DEBUG
            if (QThread::currentThread() != qApp->thread()) {
                qDebug() << _nodeInstance->getNode()->getName_mt_safe().c_str() << " is trying to call clipGetImage on a thread "
                "not controlled by Natron (probably from the multi-thread suite).\n If you're a developer of that plug-in, please "
                "fix it. Natron is now going to try to recover from that mistake but doing so can yield unpredictable results.";
            }
#endif
            view = 0;
        } else {
            if (view == -1) {
                view = args.view;
            }
        }
        
        if (!args.isMipmapLevelValid) {
            mipMapLevel = 0;
        } else {
            mipMapLevel = args.mipMapLevel;
        }
        
    } else {
        view = 0;
        mipMapLevel = 0;
    }
    
    scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;

    return getImageInternal(time, scale, view, optionalBounds);
} // getStereoscopicImage

void
OfxClipInstance::setRenderedView(int view)
{
    if ( _lastActionData.hasLocalData() ) {
        ActionLocalData & args = _lastActionData.localData();
#ifdef DEBUG
        if (QThread::currentThread() != qApp->thread() && args.isViewValid) {
            qDebug() << "Clips thread storage already set...most probably this is due to a recursive action being called. Please check this.";
        }
#endif
        args.view = view;
        args.isViewValid =  true;
        _lastActionData.setLocalData(args);
    } else {
        ActionLocalData args;
        args.view = view;
        args.isViewValid =  true;
        _lastActionData.setLocalData(args);
    }
}


///Set the view stored in the thread-local storage to be invalid
void
OfxClipInstance::discardView()
{
    assert( _lastActionData.hasLocalData() );
    _lastActionData.localData().isViewValid = false;
}

void
OfxClipInstance::setMipMapLevel(unsigned int mipMapLevel)
{
    if ( _lastActionData.hasLocalData() ) {
        ActionLocalData & args = _lastActionData.localData();
#ifdef DEBUG
        if (QThread::currentThread() != qApp->thread() && args.isMipmapLevelValid) {
            qDebug() << "Clips thread storage already set...most probably this is due to a recursive action being called. Please check this.";
        }
#endif
        args.mipMapLevel = mipMapLevel;
        args.isMipmapLevelValid =  true;
        _lastActionData.setLocalData(args);
    } else {
        ActionLocalData args;
        args.mipMapLevel = mipMapLevel;
        args.isMipmapLevelValid =  true;
        _lastActionData.setLocalData(args);
    }
}

void
OfxClipInstance::discardMipMapLevel()
{
    assert( _lastActionData.hasLocalData() );
    _lastActionData.localData().isMipmapLevelValid = false;
}
