/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "OfxClipInstance.h"

#include <cfloat>
#include <limits>
#include <bitset>
#include <cassert>
#include <stdexcept>

#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

#include "Global/Macros.h"

#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/TimeLine.h"
#include "Engine/Hash64.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/RotoContext.h"
#include "Engine/Transform.h"
#include "Engine/TLSHolder.h"

#include <nuke/fnOfxExtensions.h>
#include <ofxNatron.h>

NATRON_NAMESPACE_ENTER;

struct OfxClipInstancePrivate
{
    OfxClipInstance* _publicInterface;
    boost::weak_ptr<OfxEffectInstance> nodeInstance;
    OfxImageEffectInstance* const effect;
    double aspectRatio;
    
    bool optional;
    bool mask;
    
    boost::shared_ptr<TLSHolder<OfxClipInstance::ClipTLSData> > tlsData;
    
    OfxClipInstancePrivate(OfxClipInstance* publicInterface, const boost::shared_ptr<OfxEffectInstance>& nodeInstance, OfxImageEffectInstance* effect)
    : _publicInterface(publicInterface)
    , nodeInstance(nodeInstance)
    , effect(effect)
    , aspectRatio(1.)
    , optional(false)
    , mask(false)
    , tlsData(new TLSHolder<OfxClipInstance::ClipTLSData>())
    {
        
    }
    
    const std::vector<std::string>& getComponentsPresentInternal(const OfxClipInstance::ClipDataTLSPtr& tls) const;
};

OfxClipInstance::OfxClipInstance(const boost::shared_ptr<OfxEffectInstance>& nodeInstance,
                                 OfxImageEffectInstance* effect,
                                 int  /*index*/,
                                 OFX::Host::ImageEffect::ClipDescriptor* desc)
    : OFX::Host::ImageEffect::ClipInstance(effect, *desc)
    , _imp(new OfxClipInstancePrivate(this,nodeInstance,effect))
{
    assert(nodeInstance && effect);
    _imp->optional = isOptional();
    _imp->mask = isMask();
}

OfxClipInstance::~OfxClipInstance()
{
    
}

bool
OfxClipInstance::getIsOptional() const
{
    return _imp->optional;
}

bool
OfxClipInstance::getIsMask() const
{
    return _imp->mask;
}

const std::string &
OfxClipInstance::getUnmappedBitDepth() const
{
    
    static const std::string byteStr(kOfxBitDepthByte);
    static const std::string shortStr(kOfxBitDepthShort);
    static const std::string halfStr(kOfxBitDepthHalf);
    static const std::string floatStr(kOfxBitDepthFloat);
    static const std::string noneStr(kOfxBitDepthNone);
    EffectInstPtr inputNode = getAssociatedNode();
  
    if (inputNode) {
        ///Get the input node's output preferred bit depth and componentns
        std::list<ImageComponents> comp;
        ImageBitDepthEnum depth;
        inputNode->getPreferredDepthAndComponents(-1, &comp, &depth);
        
        switch (depth) {
            case eImageBitDepthByte:
                return byteStr;
                break;
            case eImageBitDepthShort:
                return shortStr;
                break;
            case eImageBitDepthHalf:
                return halfStr;
                break;
            case eImageBitDepthFloat:
                return floatStr;
                break;
            default:
                break;
        }
    }
    
    ///Return the hightest bit depth supported by the plugin
    boost::shared_ptr<OfxEffectInstance> effect = _imp->nodeInstance.lock();
    if (effect) {
        std::string ret = effect->effectInstance()->bestSupportedDepth(kOfxBitDepthFloat);
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

    EffectInstPtr inputNode = getAssociatedNode();
    /*if (!isOutput() && inputNode) {
        inputNode = inputNode->getNearestNonIdentity(_nodeInstance->getApp()->getTimeLine()->currentFrame());
    }*/
    if (inputNode) {
        ///Get the input node's output preferred bit depth and componentns
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();

        
        std::list<ImageComponents> comps;
        ImageBitDepthEnum depth;
        inputNode->getPreferredDepthAndComponents(-1, &comps, &depth);
        
        ImageComponents comp;
        if (comps.empty()) {
            comp = comp = ImageComponents::getRGBAComponents();
        } else {
            comp = comps.front();
        }
        
        //default to RGBA
        if (comp.getNumComponents() == 0) {
            comp = ImageComponents::getRGBAComponents();
        }
        tls->unmappedComponents = natronsComponentsToOfxComponents(comp);
        return tls->unmappedComponents;
        
    } else {
        ///The node is not connected but optional, return the closest supported components
        ///of the first connected non optional input.
        if (_imp->optional) {
            boost::shared_ptr<OfxEffectInstance> effect = _imp->nodeInstance.lock();
            assert(effect);
            int nInputs = effect->getMaxInputCount();
            for (int i  = 0 ; i < nInputs ; ++i) {
                OfxClipInstance* clip = effect->getClipCorrespondingToInput(i);
                if (clip && !clip->getIsOptional() && clip->getConnected() && clip->getComponents() != noneStr) {
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
    EffectInstPtr effect =  getAssociatedNode() ;
    
    ///The clip might be identity and we want the data of the node that will be upstream not of the identity one
    if (!isOutput() && effect) {
        effect = effect->getNearestNonIdentity(_imp->nodeInstance.lock()->getApp()->getTimeLine()->currentFrame());
    }
    if (effect) {
        
        ///Get the input node's output preferred bit depth and componentns
        std::list<ImageComponents> comps;
        ImageBitDepthEnum depth;
        effect->getPreferredDepthAndComponents(-1, &comps, &depth);
        
        ImageComponents comp;
        if (!comps.empty()) {
            comp = comps.front();
        } else {
            comp = ImageComponents::getRGBAComponents();
        }
        
        if (comp == ImageComponents::getRGBComponents()) {
            return opaqueStr;
        } else if (comp == ImageComponents::getAlphaComponents()) {
            return premultStr;
        }
        
        //case eImageComponentRGBA: // RGBA can be Opaque, PreMult or UnPreMult
        ImagePremultiplicationEnum premult = effect->getOutputPremultiplication();
        
        switch (premult) {
            case eImagePremultiplicationOpaque:
                return opaqueStr;
            case eImagePremultiplicationPremultiplied:
                return premultStr;
            case eImagePremultiplicationUnPremultiplied:
                return unPremultStr;
            default:
                return opaqueStr;
        }
    }
    
    ///Input is not connected
    
    const std::string& comps = getUnmappedComponents(); // warning: getComponents() returns None if the clip is not connected
    if (comps == kOfxImageComponentRGBA || comps == kOfxImageComponentAlpha) {
        return premultStr;
    }
    
    ///Default to opaque
    return opaqueStr;
}


const std::vector<std::string>&
OfxClipInstancePrivate::getComponentsPresentInternal(const OfxClipInstance::ClipDataTLSPtr& tls) const
{
    tls->componentsPresent.clear();
    
    EffectInstance::ComponentsAvailableMap compsAvailable;
    EffectInstPtr effect = _publicInterface->getAssociatedNode();
    if (!effect) {
        return tls->componentsPresent;
    }
    double time = effect->getCurrentTime();
    
    effect->getComponentsAvailable(true, !_publicInterface->isOutput(), time, &compsAvailable);
    //   } // if (isOutput())
    
    for (EffectInstance::ComponentsAvailableMap::iterator it = compsAvailable.begin(); it != compsAvailable.end(); ++it) {
        tls->componentsPresent.push_back(OfxClipInstance::natronsComponentsToOfxComponents(it->first));
    }
    
    return tls->componentsPresent;
}

// overridden from OFX::Host::ImageEffect::ClipInstance
/*
 * We have to use TLS here because the OpenFX API necessitate that strings
 * live through the entire duration of the calling action. The is the only way
 * to have it thread-safe and local to a current calling time.
 */
const std::vector<std::string>&
OfxClipInstance::getComponentsPresent() const OFX_EXCEPTION_SPEC
{
    try {
        //The components present have just been computed in the previous call to getDimension()
        //so we are fine here
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
        return tls->componentsPresent;
    } catch (...) {
        throw OFX::Host::Property::Exception(kOfxStatErrUnknown);
    }
}

// overridden from OFX::Host::ImageEffect::ClipInstance
int
OfxClipInstance::getDimension(const std::string &name) const OFX_EXCEPTION_SPEC
{
    if (name != kFnOfxImageEffectPropComponentsPresent) {
        return OFX::Host::ImageEffect::ClipInstance::getDimension(name);
    }
    try {
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
        const std::vector<std::string>& components = _imp->getComponentsPresentInternal(tls);
        return (int)components.size();
    } catch (...) {
        throw OFX::Host::Property::Exception(kOfxStatErrUnknown);
    }
}


// overridden from OFX::Host::ImageEffect::ClipInstance
// Pixel Aspect Ratio -
//
//  The pixel aspect ratio of a clip or image.
double
OfxClipInstance::getAspectRatio() const
{
    EffectInstPtr input = getAssociatedNode();
    if (input && input != _imp->nodeInstance.lock()) {
        input = input->getNearestNonDisabled();
        assert(input);
        return input->getPreferredAspectRatio();
    }
    return _imp->aspectRatio;
}

void
OfxClipInstance::setAspectRatio(double par)
{
    //This is protected by the clip preferences read/write lock in OfxEffectInstance
    _imp->aspectRatio = par;
}

// Frame Rate -
double
OfxClipInstance::getFrameRate() const
{
    assert(_imp->nodeInstance.lock());
    if (isOutput()) {
        return _imp->nodeInstance.lock()->effectInstance()->getOutputFrameRate();
    }
    
    EffectInstPtr input = getAssociatedNode();
    if (input) {
        input = input->getNearestNonDisabled();
        assert(input);
        return input->getPreferredFrameRate();
    }
    return 24.;
}


// overridden from OFX::Host::ImageEffect::ClipInstance
// Frame Range (startFrame, endFrame) -
//
//  The frame range over which a clip has images.
void
OfxClipInstance::getFrameRange(double &startFrame,
                               double &endFrame) const
{
    assert(_imp->nodeInstance.lock());
    EffectInstPtr n = getAssociatedNode();
    if (n) {
        U64 hash = n->getRenderHash();
        n->getFrameRange_public(hash,&startFrame, &endFrame);
        
    } else {
        double first,last;
        _imp->nodeInstance.lock()->getApp()->getFrameRange(&first, &last);
        startFrame = first;
        endFrame = last;
    }
}

// overridden from OFX::Host::ImageEffect::ClipInstance
/// Field Order - Which spatial field occurs temporally first in a frame.
/// \returns
///  - kOfxImageFieldNone - the clip material is unfielded
///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
const std::string &
OfxClipInstance::getFieldOrder() const
{
    return _imp->effect->getDefaultOutputFielding();
}

// overridden from OFX::Host::ImageEffect::ClipInstance
// Connected -
//
//  Says whether the clip is actually connected at the moment.
bool
OfxClipInstance::getConnected() const
{
    ///a roto brush is always connected
    boost::shared_ptr<OfxEffectInstance> effect = _imp->nodeInstance.lock();
    assert(effect);
    if ( (getName() == CLIP_OFX_ROTO) && effect->getNode()->isRotoNode() ) {
        return true;
    } else {
        if (_isOutput) {
            return effect->hasOutputConnected();
        } else {
            int inputNb = getInputNb();
            
            EffectInstPtr input;
            
            if (!effect->getNode()->isMaskEnabled(inputNb)) {
                return false;
            }
            ImageComponents comps;
            NodePtr maskInput;
            effect->getNode()->getMaskChannel(inputNb, &comps, &maskInput);
            if (maskInput) {
                input = maskInput->getEffectInstance();
            }
            
            if (!input) {
                input = effect->getInput(inputNb);
            }
            
            return input != NULL;
        }
    }
}

// overridden from OFX::Host::ImageEffect::ClipInstance
// Unmapped Frame Rate -
//
//  The unmaped frame range over which an output clip has images.
double
OfxClipInstance::getUnmappedFrameRate() const
{
    //return getNode().asImageEffectNode().getOutputFrameRate();
    return 25;
}

// overridden from OFX::Host::ImageEffect::ClipInstance
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

void
OfxClipInstance::getRegionOfDefinitionInternal(OfxTime time,
                                               ViewIdx view,
                                               unsigned int mipmapLevel,
                                               EffectInstance* associatedNode,
                                               OfxRectD* ret) const
{
    
    boost::shared_ptr<RotoDrawableItem> attachedStroke;
    boost::shared_ptr<OfxEffectInstance> effect = _imp->nodeInstance.lock();
    if (effect) {
        assert(effect->getNode());
        attachedStroke = effect->getNode()->getAttachedRotoItem();
    }
    
    bool inputIsMask = _imp->mask;
    
    RectD rod;
    if (attachedStroke && (inputIsMask || getName() == CLIP_OFX_ROTO)) {
        effect->getNode()->getPaintStrokeRoD(time, &rod);
        ret->x1 = rod.x1;
        ret->x2 = rod.x2;
        ret->y1 = rod.y1;
        ret->y2 = rod.y2;
        return;
    } else if (effect) {
        boost::shared_ptr<RotoContext> rotoCtx = effect->getNode()->getRotoContext();
        if (rotoCtx && getName() == CLIP_OFX_ROTO) {
            rotoCtx->getMaskRegionOfDefinition(time, view, &rod);
            ret->x1 = rod.x1;
            ret->x2 = rod.x2;
            ret->y1 = rod.y1;
            ret->y2 = rod.y2;
            return;
        }
    }

    if (associatedNode) {
        bool isProjectFormat;
        
        U64 nodeHash = associatedNode->getRenderHash();
        RectD rod;
        RenderScale scale(Image::getScaleFromMipMapLevel(mipmapLevel));
        StatusEnum st = associatedNode->getRegionOfDefinition_public(nodeHash, time, scale, view, &rod, &isProjectFormat);
        if (st == eStatusFailed) {
            ret->x1 = 0.;
            ret->x2 = 0.;
            ret->y1 = 0.;
            ret->y2 = 0.;
        } else {
            ret->x1 = rod.left();
            ret->x2 = rod.right();
            ret->y1 = rod.bottom();
            ret->y2 = rod.top();
        }
        
    } else {
        ret->x1 = 0.;
        ret->x2 = 0.;
        ret->y1 = 0.;
        ret->y2 = 0.;
    }

}

// overridden from OFX::Host::ImageEffect::ClipInstance
OfxRectD
OfxClipInstance::getRegionOfDefinition(OfxTime time, int view) const
{
    OfxRectD rod;
    unsigned int mipmapLevel;
    EffectInstPtr associatedNode = getAssociatedNode();
    
    /// The node might be disabled, hence we navigate upstream to find the first non disabled node.
    if (associatedNode) {
        associatedNode = associatedNode->getNearestNonDisabled();
    }
    ///We don't have to do the same kind of navigation if the effect is identity because the effect is supposed to have
    ///the same RoD as the input if it is identity.
    
    if (!associatedNode) {
        ///Doesn't matter, input is not connected
        mipmapLevel = 0;
    } else {
        
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
        if (!tls->mipMapLevel.empty()) {
            mipmapLevel = tls->mipMapLevel.back();
        } else {
            mipmapLevel = 0;
        }
        
        
    }
    getRegionOfDefinitionInternal(time, ViewIdx(view), mipmapLevel, associatedNode.get(), &rod);
    return rod;
}


// overridden from OFX::Host::ImageEffect::ClipInstance
/// override this to return the rod on the clip canonical coords!
OfxRectD
OfxClipInstance::getRegionOfDefinition(OfxTime time) const
{
    OfxRectD ret;
    unsigned int mipmapLevel;
    ViewIdx view(0);
    EffectInstPtr associatedNode = getAssociatedNode();

    /// The node might be disabled, hence we navigate upstream to find the first non disabled node.
    if (associatedNode) {
        associatedNode = associatedNode->getNearestNonDisabled();
    }
    ///We don't have to do the same kind of navigation if the effect is identity because the effect is supposed to have
    ///the same RoD as the input if it is identity.

    if (!associatedNode) {
        ///Doesn't matter, input is not connected
        mipmapLevel = 0;
    } else {
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
        if (!tls->view.empty()) {
            view = tls->view.back();
        }
        if (!tls->mipMapLevel.empty()) {
            mipmapLevel = tls->mipMapLevel.back();
        } else {
            mipmapLevel = 0;
        }
        
    }
    getRegionOfDefinitionInternal(time, view, mipmapLevel, associatedNode.get(), &ret);
    return ret;
} // getRegionOfDefinition



static std::string natronCustomCompToOfxComp(const ImageComponents &comp) {
    std::stringstream ss;
    const std::vector<std::string>& channels = comp.getComponentsNames();
    ss << kNatronOfxImageComponentsPlane << comp.getLayerName();
    for (U32 i = 0; i < channels.size(); ++i) {
        ss << kNatronOfxImageComponentsPlaneChannel << channels[i];
    }
    return ss.str();
}

static ImageComponents ofxCustomCompToNatronComp(const std::string& comp)
{
    std::string layerName;
    std::string compsName;
    std::vector<std::string> channelNames;
    static std::string foundPlaneStr(kNatronOfxImageComponentsPlane);
    static std::string foundChannelStr(kNatronOfxImageComponentsPlaneChannel);
    
    std::size_t foundPlane = comp.find(foundPlaneStr);
    if (foundPlane == std::string::npos) {
        throw std::runtime_error("Unsupported components type: " + comp);
    }
    
    std::size_t foundChannel = comp.find(foundChannelStr,foundPlane + foundPlaneStr.size());
    if (foundChannel == std::string::npos) {
        throw std::runtime_error("Unsupported components type: " + comp);
    }
    
    
    for (std::size_t i = foundPlane + foundPlaneStr.size(); i < foundChannel; ++i) {
        layerName.push_back(comp[i]);
    }
    
    while (foundChannel != std::string::npos) {
        
        std::size_t nextChannel = comp.find(foundChannelStr,foundChannel + foundChannelStr.size());
        
        std::size_t end = nextChannel == std::string::npos ? comp.size() : nextChannel;
        
        std::string chan;
        for (std::size_t i = foundChannel + foundChannelStr.size(); i < end; ++i) {
            chan.push_back(comp[i]);
        }
        channelNames.push_back(chan);
        compsName.append(chan);
        
        foundChannel = nextChannel;
    }
    
    
    
    return ImageComponents(layerName,compsName,channelNames);
}

ImageComponents
OfxClipInstance::ofxPlaneToNatronPlane(const std::string& plane)
{
    
    if (plane == kFnOfxImagePlaneColour) {
        std::list<ImageComponents> comps = ofxComponentsToNatronComponents(getComponents());
        assert(comps.size() == 1);
        return comps.front();
    } else if (plane == kFnOfxImagePlaneBackwardMotionVector) {
        return ImageComponents::getBackwardMotionComponents();
    } else if (plane == kFnOfxImagePlaneForwardMotionVector) {
        return ImageComponents::getForwardMotionComponents();
    } else if (plane == kFnOfxImagePlaneStereoDisparityLeft) {
        return ImageComponents::getDisparityLeftComponents();
    } else if (plane == kFnOfxImagePlaneStereoDisparityRight) {
        return ImageComponents::getDisparityRightComponents();
    } else {
        try {
            return ofxCustomCompToNatronComp(plane);
        } catch (...) {
            return ImageComponents::getNoneComponents();
        }
    }
}

std::string
OfxClipInstance::natronsPlaneToOfxPlane(const ImageComponents& plane)
{
    if (plane.getLayerName() == kNatronColorPlaneName) {
        return kFnOfxImagePlaneColour;
    } else {
        return natronCustomCompToOfxComp(plane);
    }
}


// overridden from OFX::Host::ImageEffect::ClipInstance
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
    return getImagePlaneInternal(time, ViewSpec::current(), optionalBounds, 0);
}

// overridden from OFX::Host::ImageEffect::ClipInstance
OFX::Host::ImageEffect::Image*
OfxClipInstance::getStereoscopicImage(OfxTime time,
                                      int view,
                                      const OfxRectD *optionalBounds)
{
    return getImagePlaneInternal(time, ViewSpec(view), optionalBounds, 0);
}

// overridden from OFX::Host::ImageEffect::ClipInstance
OFX::Host::ImageEffect::Image*
OfxClipInstance::getImagePlane(OfxTime time,
                               int view,
                               const std::string& plane,
                               const OfxRectD *optionalBounds)
{
    if (time != time) {
        // time is NaN

        return NULL;
    }
    return getImagePlaneInternal(time, ViewIdx(view), optionalBounds, &plane);
}

OFX::Host::ImageEffect::Image*
OfxClipInstance::getImagePlaneInternal(OfxTime time,
                                       ViewSpec view,
                                       const OfxRectD *optionalBounds,
                                       const std::string* ofxPlane)
{
    if (time != time) {
        // time is NaN

        return NULL;
    }

    if (isOutput()) {
        return getOutputImageInternal(ofxPlane);
    } else {
        return getInputImageInternal(time, view, optionalBounds, ofxPlane);
    }
}


OFX::Host::ImageEffect::Image*
OfxClipInstance::getInputImageInternal(const OfxTime time,
                                       const ViewSpec viewParam,
                                       const OfxRectD *optionalBounds,
                                       const std::string* ofxPlane)
{
    
    assert(!isOutput());

    boost::shared_ptr<OfxEffectInstance> effect = _imp->nodeInstance.lock();
    assert(effect);
    int inputnb = getInputNb();
    //If components param is not set (i.e: the plug-in uses regular clipGetImage call) then figure out the plane from the TLS set in OfxEffectInstance::render
    //otherwise use the param sent by the plug-in call of clipGetImagePlane
    ImageComponents comp;
    if (!ofxPlane) {
        
        boost::shared_ptr<EffectInstance::ComponentsNeededMap> neededComps;
        effect->getThreadLocalNeededComponents(&neededComps);
        bool foundCompsInTLS = false;
        if (neededComps) {
           EffectInstance::ComponentsNeededMap::iterator found = neededComps->find(inputnb);
            if (found != neededComps->end()) {
                if (found->second.empty()) {
                    
                    ///We are in the case of a multi-plane effect who did not specify correctly the needed components for an input
                    //fallback on the basic components indicated on the clip
                    //This could be the case for example for the Mask Input
                    std::list<ImageComponents> comps = ofxComponentsToNatronComponents(getComponents());
                    assert(comps.size() == 1);
                    comp = comps.front();
                    foundCompsInTLS = true;
                    //qDebug() << _imp->nodeInstance->getScriptName_mt_safe().c_str() << " didn't specify any needed components via getClipComponents for clip " << getName().c_str();
                    
                } else {
                    comp = found->second.front();
                    foundCompsInTLS = true;
                }
            }
        }
        
       if (!foundCompsInTLS) {
            ///We are in analysis or the effect does not have any input
            std::bitset<4> processChannels;
            bool isAll;
            bool hasUserComps = effect->getNode()->getSelectedLayer(inputnb, &processChannels, &isAll,&comp);
            if (!hasUserComps) {
                //There's no selector...fallback on the basic components indicated on the clip
                std::list<ImageComponents> comps = ofxComponentsToNatronComponents(getComponents());
                assert(comps.size() == 1);
                comp = comps.front();
            } 
        }

    } else {
        comp = ofxPlaneToNatronPlane(*ofxPlane);
    }

    if (comp.getNumComponents() == 0) {
        return 0;
    }
    if (time != time) {
        // time is NaN
        return 0;
    }


    unsigned int mipMapLevel = 0;
    // Get mipmaplevel and view from the TLS
    ClipDataTLSPtr tls = _imp->tlsData->getTLSData();
#ifdef DEBUG
    if (!tls || tls->view.empty()) {
        if (QThread::currentThread() != qApp->thread()) {
            qDebug() << effect->getNode()->getScriptName_mt_safe().c_str() << " is trying to call clipGetImage on a thread "
            "not controlled by Natron (probably from the multi-thread suite).\n If you're a developer of that plug-in, please "
            "fix it. Natron is now going to try to recover from that mistake but doing so can yield unpredictable results.";
        }
    }
#endif
    ViewIdx view;
    if (tls) {
        if (viewParam.isCurrent()) {
            if (tls->view.empty()) {
                view = ViewIdx(0);
            } else {
                view = tls->view.back();
            }
            
        }

        if (tls->mipMapLevel.empty()) {
            mipMapLevel = 0;
        } else {
            mipMapLevel = tls->mipMapLevel.back();
        }
    }
    
    
    RenderScale renderScale(Image::getScaleFromMipMapLevel(mipMapLevel));
    
    RectD bounds;
    if (optionalBounds) {
        bounds.x1 = optionalBounds->x1;
        bounds.y1 = optionalBounds->y1;
        bounds.x2 = optionalBounds->x2;
        bounds.y2 = optionalBounds->y2;
    }
    
    bool multiPlanar = effect->isMultiPlanar();
    
    ImageBitDepthEnum bitDepth = ofxDepthToNatronDepth( getPixelDepth() );
    double par = getAspectRatio();
    RectI renderWindow;
    boost::shared_ptr<Transform::Matrix3x3> transform;

    //If the plug-in used fetchImage and not fetchImagePlane it is expected that we return
    //an image mapped to the clip components
    const bool mapImageToClipPref = ofxPlane == 0;
    
    ImagePtr image = effect->getImage(inputnb, time, renderScale, view,
                                      optionalBounds ? &bounds : NULL,
                                      comp,
                                      bitDepth,
                                      par,
                                      false,
                                      mapImageToClipPref,
                                      &renderWindow,
                                      &transform);
    
    
    if (!image || renderWindow.isNull()) {
        return 0;
    }
    
    
    std::string components;
    int nComps;
    if (multiPlanar) {
        components = OfxClipInstance::natronsComponentsToOfxComponents(image->getComponents());
        nComps = image->getComponents().getNumComponents();
    } else {
        std::list<ImageComponents> natronComps = OfxClipInstance::ofxComponentsToNatronComponents(_components);
        assert(!natronComps.empty());
        components = _components;
        nComps = natronComps.front().getNumComponents();
    }

    
     // this will dump the image as seen from the plug-in
     /*QString filename;
     QTextStream ts(&filename);
     QDateTime now = QDateTime::currentDateTime();
     ts << "img_" << time << "_"  << now.toMSecsSinceEpoch() << ".png";
     appPTR->debugImage(image.get(), renderWindow, filename);*/

    return new NATRON_NAMESPACE::OfxImage(boost::shared_ptr<OfxClipInstance::RenderActionData>(), image,true,renderWindow,transform, components, nComps, *this);
}



OFX::Host::ImageEffect::Image*
OfxClipInstance::getOutputImageInternal(const std::string* ofxPlane)
{
    
    ClipDataTLSPtr tls = _imp->tlsData->getTLSData();
    
    boost::shared_ptr<RenderActionData> renderData;
    //If components param is not set (i.e: the plug-in uses regular clipGetImage call) then figure out the plane from the TLS set in OfxEffectInstance::render
    //otherwise use the param sent by the plug-in call of clipGetImagePlane
    if (tls) {
        if (!tls->renderData.empty()) {
            renderData = tls->renderData.back();
            assert(renderData);
        }
    }

    boost::shared_ptr<OfxEffectInstance> effect = _imp->nodeInstance.lock();
    
    ImageComponents natronPlane;
    if (!ofxPlane) {
    
        if (renderData) {
            natronPlane = renderData->clipComponents;
        }
        
        /*
         If the plugin is multi-planar, we are in the situation where it called the regular clipGetImage without a plane in argument
         so the components will not have been set on the TLS hence just use regular components.
         */
        if (natronPlane.getNumComponents() == 0 && effect->isMultiPlanar()) {
            std::list<ImageComponents> comps = ofxComponentsToNatronComponents(_components);
            assert(!comps.empty());
            natronPlane = comps.front();
        }
        assert(natronPlane.getNumComponents() > 0);
        
    } else {
        natronPlane = ofxPlaneToNatronPlane(*ofxPlane);
    }
    
    if (natronPlane.getNumComponents() == 0) {
        return 0;
    }
    
    
    //Look into TLS what planes are being rendered in the render action currently and the render window
    std::map<ImageComponents,EffectInstance::PlaneToRender> outputPlanes;
    RectI renderWindow;
    ImageComponents planeBeingRendered;
    bool ok = effect->getThreadLocalRenderedPlanes(&outputPlanes,&planeBeingRendered,&renderWindow);
    if (!ok) {
        return NULL;
    }
    
    ImagePtr outputImage;
    
    /*
     If the plugin is multiplanar return exactly what it requested.
     Otherwise, hack the clipGetImage and return the plane requested by the user via the interface instead of the colour plane.
     */
    bool multiPlanar = effect->isMultiPlanar();
    const std::string& layerName = /*multiPlanar ?*/ natronPlane.getLayerName();// : planeBeingRendered.getLayerName();
    
    for (std::map<ImageComponents,EffectInstance::PlaneToRender>::iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
        if (it->first.getLayerName() == layerName) {
            outputImage = it->second.tmpImage;
            break;
        }
    }
    
    //The output image MAY not exist in the TLS in some cases:
    //e.g: Natron requested Motion.Forward but plug-ins only knows how to render Motion.Forward + Motion.Backward
    //We then just allocate on the fly the plane and cache it.
    if (!outputImage) {
        outputImage = effect->allocateImagePlaneAndSetInThreadLocalStorage(natronPlane);
    }
    
    //If we don't have it by now then something is really wrong either in TLS or in the plug-in.
    assert(outputImage);
    if (!outputImage) {
        return 0;
    }
    
    
    //Check if the plug-in already called clipGetImage on this image, in which case we may already have an OfxImage laying around
    //so we try to re-use it.
    if (renderData) {
        for (std::list<OfxImage*>::const_iterator it = renderData->imagesBeingRendered.begin(); it != renderData->imagesBeingRendered.end(); ++it) {
            if ((*it)->getInternalImage() == outputImage) {
                (*it)->addReference();
                return *it;
            }
        }
    }
    
    //This is the firs time the plug-ins asks for this OfxImage, just allocate it and register it in the TLS
    std::string ofxComponents;
    int nComps;
    if (multiPlanar) {
        ofxComponents = OfxClipInstance::natronsComponentsToOfxComponents(outputImage->getComponents());
        nComps = outputImage->getComponents().getNumComponents();
    } else {
        std::list<ImageComponents> natronComps = OfxClipInstance::ofxComponentsToNatronComponents(_components);
        assert(!natronComps.empty());
        ofxComponents = _components;
        nComps = natronComps.front().getNumComponents();
    }
    
    //The output clip doesn't have any transform matrix
    OfxImage* ret =  new OfxImage(renderData,outputImage,false,renderWindow,boost::shared_ptr<Transform::Matrix3x3>(), ofxComponents, nComps, *this);
    if (renderData) {
        renderData->imagesBeingRendered.push_back(ret);
    }
    return ret;
}

#ifdef OFX_SUPPORTS_OPENGLRENDER
// overridden from OFX::Host::ImageEffect::ClipInstance
/// override this to fill in the OpenGL texture at the given time.
/// The bounds of the image on the image plane should be
/// 'appropriate', typically the value returned in getRegionsOfInterest
/// on the effect instance. Outside a render call, the optionalBounds should
/// be 'appropriate' for the.
/// If bounds is not null, fetch the indicated section of the canonical image plane.
OFX::Host::ImageEffect::Texture*
OfxClipInstance::loadTexture(OfxTime time, const char *format, const OfxRectD *optionalBounds)
{
    Q_UNUSED(time);
    Q_UNUSED(format);
    Q_UNUSED(optionalBounds);
    return NULL;
}
#endif

std::string
OfxClipInstance::natronsComponentsToOfxComponents(const ImageComponents& comp)
{
    if (comp == ImageComponents::getNoneComponents()) {
        return kOfxImageComponentNone;
    } else if (comp == ImageComponents::getAlphaComponents()) {
        return kOfxImageComponentAlpha;
    } else if (comp == ImageComponents::getRGBComponents()) {
        return kOfxImageComponentRGB;
    } else if (comp == ImageComponents::getRGBAComponents()) {
        return kOfxImageComponentRGBA;
    } else if (QString(comp.getComponentsGlobalName().c_str()).compare("UV", Qt::CaseInsensitive) == 0) {
        return kFnOfxImageComponentMotionVectors;
    } else if (QString(comp.getComponentsGlobalName().c_str()).compare("XY", Qt::CaseInsensitive) == 0) {
        return kFnOfxImageComponentStereoDisparity;
    } else {
        return natronCustomCompToOfxComp(comp);
    }
}

std::list<ImageComponents>
OfxClipInstance::ofxComponentsToNatronComponents(const std::string & comp)
{
    std::list<ImageComponents> ret;
    if (comp ==  kOfxImageComponentRGBA) {
        ret.push_back(ImageComponents::getRGBAComponents());
    } else if (comp == kOfxImageComponentAlpha) {
        ret.push_back(ImageComponents::getAlphaComponents());
    } else if (comp == kOfxImageComponentRGB) {
        ret.push_back(ImageComponents::getRGBComponents());
    } else if (comp == kOfxImageComponentNone) {
        ret.push_back(ImageComponents::getNoneComponents());
    } else if (comp == kFnOfxImageComponentMotionVectors) {
        ret.push_back(ImageComponents::getForwardMotionComponents());
        ret.push_back(ImageComponents::getBackwardMotionComponents());
    } else if (comp == kFnOfxImageComponentStereoDisparity) {
        ret.push_back(ImageComponents::getDisparityLeftComponents());
        ret.push_back(ImageComponents::getDisparityRightComponents());
    } else if (comp == kNatronOfxImageComponentXY) {
        ret.push_back(ImageComponents::getXYComponents());
    } else {
        try {
            ret.push_back(ofxCustomCompToNatronComp(comp));
        } catch (...) {
            
        }
    }
    return ret;
}

ImageBitDepthEnum
OfxClipInstance::ofxDepthToNatronDepth(const std::string & depth)
{
    if (depth == kOfxBitDepthByte) {
        return eImageBitDepthByte;
    } else if (depth == kOfxBitDepthShort) {
        return eImageBitDepthShort;
    } else if (depth == kOfxBitDepthHalf) {
        return eImageBitDepthHalf;
    } else if (depth == kOfxBitDepthFloat) {
        return eImageBitDepthFloat;
    } else if (depth == kOfxBitDepthNone) {
        return eImageBitDepthNone;
    } else {
        throw std::runtime_error(depth + ": unsupported bitdepth"); //< comp unsupported
    }
}

std::string
OfxClipInstance::natronsDepthToOfxDepth(ImageBitDepthEnum depth)
{
    switch (depth) {
    case eImageBitDepthByte:

        return kOfxBitDepthByte;
    case eImageBitDepthShort:

        return kOfxBitDepthShort;
    case eImageBitDepthHalf:

        return kOfxBitDepthHalf;
    case eImageBitDepthFloat:

        return kOfxBitDepthFloat;
    case eImageBitDepthNone:

        return kOfxBitDepthNone;
    default:
        assert(false);    //< shouldve been caught earlier
        throw std::logic_error("OfxClipInstance::natronsDepthToOfxDepth(): unknown depth");
    }
}


struct OfxImagePrivate
{
    ImagePtr natronImage;
    boost::shared_ptr<GenericAccess> access;
    boost::shared_ptr<OfxClipInstance::RenderActionData> tls;
    
    OfxImagePrivate(const ImagePtr& image,
                    const boost::shared_ptr<OfxClipInstance::RenderActionData>& tls)
    : natronImage(image)
    , access()
    , tls(tls)
    {
        
    }
};

ImagePtr
OfxImage::getInternalImage() const
{
    return _imp->natronImage;
}

OfxImage::OfxImage(const boost::shared_ptr<OfxClipInstance::RenderActionData>& renderData,
                   const boost::shared_ptr<NATRON_NAMESPACE::Image>& internalImage,
                   bool isSrcImage,
                   const RectI& renderWindow,
                   const boost::shared_ptr<Transform::Matrix3x3>& mat,
                   const std::string& components,
                   int nComps,
                   OfxClipInstance &clip)
: OFX::Host::ImageEffect::Image(clip)
, _imp(new OfxImagePrivate(internalImage, renderData))
{
    
    assert(internalImage);
    
    unsigned int mipMapLevel = internalImage->getMipMapLevel();
    RenderScale scale(NATRON_NAMESPACE::Image::getScaleFromMipMapLevel(mipMapLevel));
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.x, 0);
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.y, 1);
    
   
    
    // data ptr
    RectI bounds = internalImage->getBounds();
    
    ///Do not activate this assert! The render window passed to renderRoI can be bigger than the actual RoD of the effect
    ///in which case it is just clipped to the RoD.
    //assert(bounds.contains(renderWindow));
    RectI pluginsSeenBounds;
    renderWindow.intersect(bounds, &pluginsSeenBounds);
    
    const RectD & rod = internalImage->getRoD(); // Not the OFX RoD!!! Image::getRoD() is in *CANONICAL* coordinates

    if (isSrcImage) {
        boost::shared_ptr<NATRON_NAMESPACE::Image::ReadAccess> access(new NATRON_NAMESPACE::Image::ReadAccess(internalImage.get()));
        const unsigned char* ptr = access->pixelAt( pluginsSeenBounds.left(), pluginsSeenBounds.bottom() );
        assert(ptr);
        setPointerProperty( kOfxImagePropData, const_cast<unsigned char*>(ptr));
        _imp->access = access;
    } else {
        boost::shared_ptr<NATRON_NAMESPACE::Image::WriteAccess> access(new NATRON_NAMESPACE::Image::WriteAccess(internalImage.get()));
        unsigned char* ptr = access->pixelAt( pluginsSeenBounds.left(), pluginsSeenBounds.bottom() );
        assert(ptr);
        setPointerProperty( kOfxImagePropData, ptr);
        _imp->access = access;
    }
    
    ///We set the render window that was given to the render thread instead of the actual bounds of the image
    ///so we're sure the plug-in doesn't attempt to access outside pixels.
    setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.left(), 0);
    setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.bottom(), 1);
    setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.right(), 2);
    setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.top(), 3);

    // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImagePropRegionOfDefinition
    // " An image's region of definition, in *PixelCoordinates,* is the full frame area of the image plane that the image covers."
    // Image::getRoD() is in *CANONICAL* coordinates
    // OFX::Image RoD is in *PIXEL* coordinates
    RectI pixelRod;
    rod.toPixelEnclosing(mipMapLevel, internalImage->getPixelAspectRatio(), &pixelRod);
    setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.left(), 0);
    setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.bottom(), 1);
    setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.right(), 2);
    setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.top(), 3);
    
    //pluginsSeenBounds must be contained in pixelRod
    assert(pluginsSeenBounds.left() >= pixelRod.left() && pluginsSeenBounds.right() <= pixelRod.right() &&
           pluginsSeenBounds.bottom() >= pixelRod.bottom() && pluginsSeenBounds.top() <= pixelRod.top());
    
    // row bytes
    setIntProperty( kOfxImagePropRowBytes, bounds.width() * nComps *
                    getSizeOfForBitDepth( internalImage->getBitDepth() ) );
    setStringProperty( kOfxImageEffectPropComponents,components);
    setStringProperty( kOfxImageEffectPropPixelDepth, OfxClipInstance::natronsDepthToOfxDepth( internalImage->getBitDepth() ) );
    setStringProperty( kOfxImageEffectPropPreMultiplication, clip.getPremult() );
    setStringProperty(kOfxImagePropField, kOfxImageFieldNone);
    setStringProperty( kOfxImagePropUniqueIdentifier,QString::number(internalImage->getHashKey(), 16).toStdString() );
    setDoubleProperty( kOfxImagePropPixelAspectRatio, clip.getAspectRatio() );
    
    //Attach the transform matrix if any
    if (mat) {
        setDoubleProperty(kFnOfxPropMatrix2D, mat->a, 0);
        setDoubleProperty(kFnOfxPropMatrix2D, mat->b, 1);
        setDoubleProperty(kFnOfxPropMatrix2D, mat->c, 2);
        
        setDoubleProperty(kFnOfxPropMatrix2D, mat->d, 3);
        setDoubleProperty(kFnOfxPropMatrix2D, mat->e, 4);
        setDoubleProperty(kFnOfxPropMatrix2D, mat->f, 5);
        
        setDoubleProperty(kFnOfxPropMatrix2D, mat->g, 6);
        setDoubleProperty(kFnOfxPropMatrix2D, mat->h, 7);
        setDoubleProperty(kFnOfxPropMatrix2D, mat->i, 8);
    } else {
        for (int i = 0; i < 9; ++i) {
            setDoubleProperty(kFnOfxPropMatrix2D, 0., i);
        }
    }
    
}

OfxImage::~OfxImage()
{
    if (_imp->tls) {
        std::list<OfxImage*>::iterator found = std::find(_imp->tls->imagesBeingRendered.begin(), _imp->tls->imagesBeingRendered.end(), this);
        if (found != _imp->tls->imagesBeingRendered.end()) {
            _imp->tls->imagesBeingRendered.erase(found);
        }
    }
}

int
OfxClipInstance::getInputNb() const
{
    if (_isOutput) {
        return -1;
    }
    return _imp->nodeInstance.lock()->getClipInputNumber(this);
}

EffectInstPtr
OfxClipInstance::getAssociatedNode() const
{
    boost::shared_ptr<OfxEffectInstance> effect = _imp->nodeInstance.lock();
    assert(effect);
    if ( (getName() == CLIP_OFX_ROTO) && effect->getNode()->isRotoNode() ) {
        return effect;
    }
    if (_isOutput) {
        return effect;
    } else {
        ImageComponents comps;
        NodePtr maskInput;
        int inputNb = getInputNb();
        effect->getNode()->getMaskChannel(inputNb, &comps, &maskInput);
        if (maskInput) {
            return maskInput->getEffectInstance();
        }
        
        if (!maskInput) {
            return  effect->getInput(getInputNb());
        } else {
            return maskInput->getEffectInstance();
        }

    }
}

void
OfxClipInstance::setClipTLS(ViewIdx view,
                            unsigned int mipmapLevel,
                            const ImageComponents& components)
{
    ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    tls->view.push_back(view);
    tls->mipMapLevel.push_back(mipmapLevel);
    boost::shared_ptr<RenderActionData> d(new RenderActionData());
    d->clipComponents = components;
    tls->renderData.push_back(d);
}

void
OfxClipInstance::invalidateClipTLS()
{
    ClipDataTLSPtr tls = _imp->tlsData->getTLSData();
    assert(tls);
    assert(!tls->view.empty());
    tls->view.pop_back();
    assert(!tls->mipMapLevel.empty());
    tls->mipMapLevel.pop_back();
    assert(!tls->renderData.empty());
    tls->renderData.pop_back();
}

const std::string &
OfxClipInstance::getComponents() const
{
    return OFX::Host::ImageEffect::ClipInstance::getComponents();
}

const std::string &
OfxClipInstance::findSupportedComp(const std::string &s) const
{
    static const std::string none(kOfxImageComponentNone);
    static const std::string rgba(kOfxImageComponentRGBA);
    static const std::string rgb(kOfxImageComponentRGB);
    static const std::string alpha(kOfxImageComponentAlpha);
    static const std::string motion(kFnOfxImageComponentMotionVectors);
    static const std::string disparity(kFnOfxImageComponentStereoDisparity);
    static const std::string xy(kNatronOfxImageComponentXY);
    
    /// is it there
    if(isSupportedComponent(s))
        return s;
    
    if (s == xy) {
        if (isSupportedComponent(motion)) {
            return motion;
        } else if (isSupportedComponent(disparity)) {
            return disparity;
        } else if (isSupportedComponent(rgb)) {
            return rgb;
        } else if (isSupportedComponent(rgba)) {
            return rgba;
        } else if (isSupportedComponent(alpha)) {
            return alpha;
        }
    } else if(s == rgba) {
        if (isSupportedComponent(rgb)) {
            return rgb;
        }
        if (isSupportedComponent(alpha)) {
            return alpha;
        }
    } else if(s == alpha) {
        if (isSupportedComponent(rgba)) {
            return rgba;
        }
        if (isSupportedComponent(rgb)) {
            return rgb;
        }
    } else if (s == rgb) {
        if (isSupportedComponent(rgba)) {
            return rgba;
        }
        if (isSupportedComponent(alpha)) {
            return alpha;
        }
    } else if (s == motion) {
        
        if (isSupportedComponent(xy)) {
            return xy;
        } else if (isSupportedComponent(disparity)) {
            return disparity;
        } else if (isSupportedComponent(rgb)) {
            return rgb;
        } else if (isSupportedComponent(rgba)) {
            return rgba;
        } else if (isSupportedComponent(alpha)) {
            return alpha;
        }
    } else if (s == disparity) {
        if (isSupportedComponent(xy)) {
            return xy;
        } else if (isSupportedComponent(disparity)) {
            return disparity;
        } else if (isSupportedComponent(rgb)) {
            return rgb;
        } else if (isSupportedComponent(rgba)) {
            return rgba;
        } else if (isSupportedComponent(alpha)) {
            return alpha;
        }
    }
    
    /// wierd, must be some custom bit , if only one, choose that, otherwise no idea
    /// how to map, you need to derive to do so.
    const std::vector<std::string> &supportedComps = getSupportedComponents();
    if(supportedComps.size() == 1)
        return supportedComps[0];
    
    return none;

}

NATRON_NAMESPACE_EXIT;

