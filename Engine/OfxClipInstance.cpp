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

const std::string& OfxClipInstance::getUnmappedBitDepth() const
{
    // we always use floats
    static const std::string v(kOfxBitDepthFloat);
    return v;
}

const std::string &OfxClipInstance::getUnmappedComponents() const
{
    static const std::string rgbStr(kOfxImageComponentRGB);
    static const std::string noneStr(kOfxImageComponentNone);
    static const std::string rgbaStr(kOfxImageComponentRGBA);
    static const std::string alphaStr(kOfxImageComponentAlpha);
    
    ///Default to RGBA, let the plug-in clip prefs inform us of its preferences
    return rgbaStr;
}


// PreMultiplication -
//
//  kOfxImageOpaque - the image is opaque and so has no premultiplication state
//  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
//  kOfxImageUnPreMultiplied - the image is unpremultiplied
const std::string &OfxClipInstance::getPremult() const
{
    static const std::string v(kOfxImagePreMultiplied);
    OfxEffectInstance* effect = dynamic_cast<OfxEffectInstance*>(getAssociatedNode());
    if (effect) {
        return effect->ofxGetOutputPremultiplication();
    }
    
    ///Default to premultiplied, let the plug-in clip prefs inform us of its preferences
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
    assert(_nodeInstance);
    SequenceTime first,last;
    EffectInstance* n = getAssociatedNode();
    if(n) {
       n->getFrameRange_public(&first, &last);
    } else {
        assert(_nodeInstance);
        assert(_nodeInstance->getApp());
        assert(_nodeInstance->getApp()->getTimeLine());
        first = _nodeInstance->getApp()->getTimeLine()->leftBound();
        last = _nodeInstance->getApp()->getTimeLine()->rightBound();
    }
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
    ///a roto brush is always connected
    if (getName() == "Brush"  && _nodeInstance->getNode()->isRotoNode()) {
        return true;
    } else {
        if (_isOutput) {
            return _nodeInstance->hasOutputConnected();
        } else {
            int inputNb = getInputNb();
            if (isMask() && !_nodeInstance->getNode()->isMaskEnabled(inputNb)) {
                return false;
            }
            if (QThread::currentThread() == qApp->thread()) {
                return _nodeInstance->input(inputNb) != NULL;
            } else {
                return _nodeInstance->input_other_thread(inputNb) != NULL;
            }
        }
    }
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


/// override this to return the rod on the clip canonical coords!
OfxRectD OfxClipInstance::getRegionOfDefinition(OfxTime time) const
{
    OfxRectD ret;
    RectI rod;
    
    unsigned int mipmapLevel;
    int view;
    if (!_lastRenderArgs.hasLocalData() || !_lastRenderArgs.localData().isViewValid || !_lastRenderArgs.localData().isMipMapLevelValid) {
        qDebug() << "Clip thread storage not set in a call to OfxClipInstance::getRegionOfDefinition. Please investigate this bug.";
        std::list<ViewerInstance*> viewersConnected;
        _nodeInstance->getNode()->hasViewersConnected(&viewersConnected);
        if (viewersConnected.empty()) {
            view = 0;
            mipmapLevel = 0;
        } else {
            view = viewersConnected.front()->getCurrentView();
            mipmapLevel = (unsigned int)viewersConnected.front()->getMipMapLevel();
        }
    } else {
        mipmapLevel = _lastRenderArgs.localData().mipMapLevel;
        view = _lastRenderArgs.localData().view;

    }
    
    
    if (getName() == "Brush" && _nodeInstance->getNode()->isRotoNode()) {
        boost::shared_ptr<RotoContext> rotoCtx =  _nodeInstance->getNode()->getRotoContext();
        assert(rotoCtx);
        RectI rod;
        rotoCtx->getMaskRegionOfDefinition(time, view, &rod);
        ret.x1 = rod.x1;
        ret.x2 = rod.x2;
        ret.y1 = rod.y1;
        ret.y2 = rod.y2;
        return ret;
    }
    
    EffectInstance* n = getAssociatedNode();
    while (n && n->getNode()->isNodeDisabled()) {
        ///we forward this node to the last connected non-optional input
        ///if there's only optional inputs connected, we return the last optional input
        int lastOptionalInput = -1;
        int inputNb = -1;
        for (int i = n->maximumInputs() - 1; i >= 0; --i) {
            bool optional = n->isInputOptional(i);
            if (!optional && n->getNode()->input_other_thread(i)) {
                inputNb = i;
                break;
            } else if (optional && lastOptionalInput == -1) {
                lastOptionalInput = i;
            }
        }
        if (inputNb == -1) {
            inputNb = lastOptionalInput;
        }
        n = n->input_other_thread(inputNb);
    }
    if (n) {
        bool isProjectFormat;
        
        
        ///First thing: we try to find in the cache an image with the following key
        ///so that retrieving the RoD is cheap. If we can't do this then there are
        ///several cases:
        ///1) The clip is output, the caller should have set the rod in the clip thread storage
        /// If not we will call getRoD on the output clip, but this will trigger a recursive action
        ///2) The clip is input we call getRoD on the source clip.
        U64 nodeHash;
        bool foundRenderHash = n->getRenderHash(&nodeHash);
        if (!foundRenderHash) {
            nodeHash = n->hash();
        }
        
        boost::shared_ptr<const ImageParams> cachedImgParams;
        boost::shared_ptr<Image> image;
        
        Natron::ImageKey key = Natron::Image::makeKey(nodeHash, time,mipmapLevel,view);
        bool isCached = Natron::getImageFromCache(key, &cachedImgParams,&image);
        
        Format f;
        n->getRenderFormat(&f);
        
        ///If the RoD is cached accept it always if it doesn't depend on the project format.
        ///Otherwise cehck that it is really the current project format.
        if (isCached && (!cachedImgParams->isRodProjectFormat()
            || (cachedImgParams->isRodProjectFormat() && cachedImgParams->getRoD() == dynamic_cast<RectI&>(f)))) {
            rod = cachedImgParams->getRoD();
            ret.x1 = rod.left();
            ret.x2 = rod.right();
            ret.y1 = rod.bottom();
            ret.y2 = rod.top();
            
        } else {
            bool outputRoDStorageSet = _lastRenderArgs.localData().rodValid;
            if (n != _nodeInstance || (!outputRoDStorageSet && n == _nodeInstance)) {
                
                if (n == _nodeInstance && !outputRoDStorageSet) {
                    qDebug() << "Clip thread storage not set in a call to OfxClipInstance::getRegionOfDefinition. Please investigate this bug.";
                }
                RenderScale scale;
                scale.x = scale.y = 1.;
                Natron::Status st = n->getRegionOfDefinition_public(time,scale,view,&rod,&isProjectFormat);
                if (st == StatFailed) {
                    ret.x1 = kOfxFlagInfiniteMin;
                    ret.x2 = kOfxFlagInfiniteMax;
                    ret.y1 = kOfxFlagInfiniteMin;
                    ret.y2 = kOfxFlagInfiniteMax;
                } else {
                    ret.x1 = rod.left();
                    ret.x2 = rod.right();
                    ret.y1 = rod.bottom();
                    ret.y2 = rod.top();
                }
            } else {
                rod = _lastRenderArgs.localData().rod;
                ret.x1 = rod.left();
                ret.x2 = rod.right();
                ret.y1 = rod.bottom();
                ret.y2 = rod.top();
            }
            
        }
    } else {
        ret.x1 = 0.;
        ret.x2 = 1.;
        ret.y1 = 0.;
        ret.y2 = 1.;
    }
    return ret;
}


/// override this to fill in the image at the given time.
/// The bounds of the image on the image plane should be
/// 'appropriate', typically the value returned in getRegionsOfInterest
/// on the effect instance. Outside a render call, the optionalBounds should
/// be 'appropriate' for the.
/// If bounds is not null, fetch the indicated section of the canonical image plane.
OFX::Host::ImageEffect::Image* OfxClipInstance::getImage(OfxTime time, const OfxRectD *optionalBounds)
{
    return getStereoscopicImage(time, -1, optionalBounds);
}

OFX::Host::ImageEffect::Image* OfxClipInstance::getImageInternal(OfxTime time,
                                                                 const OfxPointD& renderScale,
                                                                 int view,
                                                                 const OfxRectD *optionalBounds)
{
    assert(!isOutput());
    // input has been rendered just find it in the cache
    RectD bounds;
    if (optionalBounds) {
        bounds.x1 = optionalBounds->x1;
        bounds.y1 = optionalBounds->y1;
        bounds.x2 = optionalBounds->x2;
        bounds.y2 = optionalBounds->y2;
    }
    boost::shared_ptr<Natron::Image> image = _nodeInstance->getImage(getInputNb(), time, renderScale, view,
                                                                     optionalBounds ? &bounds : NULL,
                                                                     ofxComponentsToNatronComponents(getComponents()),
                                                                     ofxDepthToNatronDepth(getPixelDepth()),false);
    if (!image) {
        return NULL;
    } else {
        return new OfxImage(image,*this);
    }
}

std::string OfxClipInstance::natronsComponentsToOfxComponents(Natron::ImageComponents comp) {
    switch (comp) {
        case Natron::ImageComponentNone:
            return kOfxImageComponentNone;
            break;
        case Natron::ImageComponentAlpha:
            return kOfxImageComponentAlpha;
            break;
        case Natron::ImageComponentRGB:
            return kOfxImageComponentRGB;
            break;
        case Natron::ImageComponentRGBA:
            return kOfxImageComponentRGBA;
            break;
        default:
            assert(false);//comp unsupported
            break;
    }
}

Natron::ImageComponents OfxClipInstance::ofxComponentsToNatronComponents(const std::string& comp)
{
    if (comp ==  kOfxImageComponentRGBA) {
        return Natron::ImageComponentRGBA;
    } else if (comp == kOfxImageComponentAlpha) {
        return Natron::ImageComponentAlpha;
    } else if (comp == kOfxImageComponentRGB) {
        return Natron::ImageComponentRGB;
    } else if (comp == kOfxImageComponentNone) {
        return Natron::ImageComponentNone;
    } else {
        assert(false); //< comp unsupported
    }
}

Natron::ImageBitDepth OfxClipInstance::ofxDepthToNatronDepth(const std::string& depth)
{
    if (depth == kOfxBitDepthByte) {
        return Natron::IMAGE_BYTE;
    } else if (depth == kOfxBitDepthShort) {
        return Natron::IMAGE_SHORT;
    } else if (depth == kOfxBitDepthFloat) {
        return Natron::IMAGE_FLOAT;
    } else {
        assert(false);//< shouldve been caught earlier
    }
}

std::string OfxClipInstance::natronsDepthToOfxDepth(Natron::ImageBitDepth depth)
{
    switch (depth) {
        case Natron::IMAGE_BYTE:
            return kOfxBitDepthByte;
        case Natron::IMAGE_SHORT:
            return kOfxBitDepthShort;
        case Natron::IMAGE_FLOAT:
            return kOfxBitDepthFloat;
        default:
            assert(false);//< shouldve been caught earlier
            break;
    }
}

OfxImage::OfxImage(boost::shared_ptr<Natron::Image> internalImage,OfxClipInstance &clip):
OFX::Host::ImageEffect::Image(clip)
,_bitDepth(OfxImage::eBitDepthFloat)
,_floatImage(internalImage)
{
    RenderScale scale;
    scale.x = Natron::Image::getScaleFromMipMapLevel(internalImage->getMipMapLevel());
    scale.y = scale.x;
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.x, 0);
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.y, 1);
    // data ptr
    const RectI& pixelrod = internalImage->getPixelRoD();
    const RectI& rod = internalImage->getRoD();
    setPointerProperty(kOfxImagePropData,internalImage->pixelAt(pixelrod.left(), pixelrod.bottom()));
    // bounds and rod
    setIntProperty(kOfxImagePropBounds, pixelrod.left(), 0);
    setIntProperty(kOfxImagePropBounds, pixelrod.bottom(), 1);
    setIntProperty(kOfxImagePropBounds, pixelrod.right(), 2);
    setIntProperty(kOfxImagePropBounds, pixelrod.top(), 3);
    setIntProperty(kOfxImagePropRegionOfDefinition, rod.left(), 0);
    setIntProperty(kOfxImagePropRegionOfDefinition, rod.bottom(), 1);
    setIntProperty(kOfxImagePropRegionOfDefinition, rod.right(), 2);
    setIntProperty(kOfxImagePropRegionOfDefinition, rod.top(), 3);
    // row bytes
    setIntProperty(kOfxImagePropRowBytes, pixelrod.width() *
                   Natron::getElementsCountForComponents(internalImage->getComponents()) *
                   getSizeOfForBitDepth(internalImage->getBitDepth()));
    setStringProperty(kOfxImageEffectPropComponents, OfxClipInstance::natronsComponentsToOfxComponents(internalImage->getComponents()));
    setStringProperty(kOfxImageEffectPropPixelDepth, OfxClipInstance::natronsDepthToOfxDepth(internalImage->getBitDepth()));
    setStringProperty(kOfxImageEffectPropPreMultiplication, clip.getPremult());
    setStringProperty(kOfxImagePropField, kOfxImageFieldNone);
    setStringProperty(kOfxImagePropUniqueIdentifier,QString::number(internalImage->getHashKey(), 16).toStdString());
    setDoubleProperty(kOfxImagePropPixelAspectRatio, clip.getAspectRatio());
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

Natron::EffectInstance* OfxClipInstance::getAssociatedNode() const
{
    assert(_nodeInstance);
    if (getName() == "Brush" && _nodeInstance->getNode()->isRotoNode()) {
        return _nodeInstance;
    }
    if(_isOutput) {
        return _nodeInstance;
    } else {
        if (QThread::currentThread() == qApp->thread()) {
            return _nodeInstance->input(getInputNb());
        } else {
            return _nodeInstance->input_other_thread(getInputNb());
        }
    }
}

OFX::Host::ImageEffect::Image* OfxClipInstance::getStereoscopicImage(OfxTime time, int view, const OfxRectD *optionalBounds)
{
    OfxPointD scale;
    scale.x = scale.y = 1.;
    if (!_lastRenderArgs.hasLocalData()) {
        qDebug() << _nodeInstance->getNode()->getName_mt_safe().c_str() << " is trying to call clipGetImage on a thread "
        "not controlled by Natron (probably from the multi-thread suite).\n If you're a developer of that plug-in, please "
        "fix it.";
    }
    assert(_lastRenderArgs.hasLocalData());
    if (isOutput()) {
        boost::shared_ptr<Natron::Image> outputImage;
        if (!_lastRenderArgs.localData().isImageValid) {
            qDebug() << _nodeInstance->getNode()->getName_mt_safe().c_str() << " is trying to call clipGetImage on a thread "
            "not controlled by Natron (probably from the multi-thread suite).\n If you're a developer of that plug-in, please "
            "fix it. Natron is now going to try to recover from that mistake but doing so can yield unpredictable results.";
            ///try to recover from the mistake of the plug-in.
            unsigned int mipmapLevel;
            std::list<ViewerInstance*> viewersConnected;
            _nodeInstance->getNode()->hasViewersConnected(&viewersConnected);
            if (viewersConnected.empty()) {
                if (view == -1) {
                    view = 0;
                }
                mipmapLevel = 0;
            } else {
                if (view == -1) {
                    view = viewersConnected.front()->getCurrentView();
                }
                mipmapLevel = (unsigned int)viewersConnected.front()->getMipMapLevel();
            }

            outputImage = _nodeInstance->getNode()->getImageBeingRendered(time,mipmapLevel, view);
        } else {
            outputImage = _lastRenderArgs.localData().image;
        }
        if (!outputImage) {
            return NULL;
        }
        return new OfxImage(outputImage,*this);
    }
    
    unsigned int mipMapLevel;
    if (!_lastRenderArgs.localData().isViewValid || !_lastRenderArgs.localData().isMipMapLevelValid) {
        qDebug() << _nodeInstance->getNode()->getName_mt_safe().c_str() << " is trying to call clipGetImage on a thread "
        "not controlled by Natron (probably from the multi-thread suite).\n If you're a developer of that plug-in, please "
        "fix it. Natron is now going to try to recover from that mistake but doing so can yield unpredictable results.";
        std::list<ViewerInstance*> viewersConnected;
        _nodeInstance->getNode()->hasViewersConnected(&viewersConnected);
        if (viewersConnected.empty()) {
            if (view == -1) {
                view = 0;
            }
            mipMapLevel = 0;
        } else {
            if (view == -1) {
                view = viewersConnected.front()->getCurrentView();
            }
            mipMapLevel = (unsigned int)viewersConnected.front()->getMipMapLevel();
        }

    } else {
        mipMapLevel = _lastRenderArgs.localData().mipMapLevel;
        scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
        scale.y = scale.x;
        if (view == -1) {
            view = _lastRenderArgs.localData().view;
        }
    }

    return getImageInternal(time, scale, view, optionalBounds);
}

void OfxClipInstance::setView(int view) {
    LastRenderArgs args;
    if (_lastRenderArgs.hasLocalData()) {
        args = _lastRenderArgs.localData();
        if (_lastRenderArgs.localData().isViewValid) {
            qDebug() << "Clips thread storage already set...most probably this is due to a recursive action being called. Please check this.";
        }
    } else {
        args.mipMapLevel = 0;
        args.image.reset();
    }
    args.view = view;
    args.isViewValid =  true;
    _lastRenderArgs.setLocalData(args);
}

void OfxClipInstance::setMipMapLevel(unsigned int mipMapLevel)
{
    LastRenderArgs args;
    if (_lastRenderArgs.hasLocalData()) {
        args = _lastRenderArgs.localData();
        if (_lastRenderArgs.localData().isMipMapLevelValid) {
            qDebug() << "Clips thread storage already set...most probably this is due to a recursive action being called. Please check this.";
        }
    } else {
        args.view = 0;
        args.image.reset();
    }
    args.mipMapLevel = mipMapLevel;
    args.isMipMapLevelValid = true;
    _lastRenderArgs.setLocalData(args);

}

///Set the view stored in the thread-local storage to be invalid
void OfxClipInstance::discardView()
{
    assert(_lastRenderArgs.hasLocalData());
    _lastRenderArgs.localData().isViewValid = false;
}

///Set the mipmap level stored in the thread-local storage to be invalid
void OfxClipInstance::discardMipMapLevel()
{
    assert(_lastRenderArgs.hasLocalData());
    _lastRenderArgs.localData().isMipMapLevelValid = false;
}

void OfxClipInstance::setRenderedImage(const boost::shared_ptr<Natron::Image>& image)
{
    LastRenderArgs args;
    if (_lastRenderArgs.hasLocalData()) {
        args = _lastRenderArgs.localData();
        if(_lastRenderArgs.localData().isImageValid) {
            qDebug() << "Clips thread storage already set...most probably this is due to a recursive action being called. Please check this.";
        }
    } else {
        args.mipMapLevel = 0;
        args.view = 0;
    }
    args.image = image;
    args.isImageValid = true;
    _lastRenderArgs.setLocalData(args);
}

void OfxClipInstance::discardRenderedImage()
{
    assert(_lastRenderArgs.hasLocalData());
    _lastRenderArgs.localData().isImageValid = false;
    _lastRenderArgs.localData().image.reset();
}

void OfxClipInstance::setOutputRoD(const RectI& rod)
{
    LastRenderArgs args;
    if (_lastRenderArgs.hasLocalData()) {
        args = _lastRenderArgs.localData();
        if(_lastRenderArgs.localData().rodValid) {
            qDebug() << "Clips thread storage already set...most probably this is due to a recursive action being called. Please check this.";
        }
    } else {
        args.mipMapLevel = 0;
        args.view = 0;
        args.image.reset();
    }
    args.rod = rod;
    args.rodValid = true;
    _lastRenderArgs.setLocalData(args);
}

void OfxClipInstance::discardOutputRoD()
{
    assert(_lastRenderArgs.hasLocalData());
    _lastRenderArgs.localData().rodValid = false;
}