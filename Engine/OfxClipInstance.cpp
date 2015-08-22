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

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
#include "Engine/Transform.h"

#include <nuke/fnOfxExtensions.h>
#include <ofxNatron.h>

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
      , _lastActionData()
      , _componentsPresent()
      , _unmappedComponents()
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
        std::list<Natron::ImageComponents> comp;
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
        std::list<Natron::ImageComponents> comps;
        Natron::ImageBitDepthEnum depth;
        inputNode->getPreferredDepthAndComponents(-1, &comps, &depth);
        assert(!comps.empty());
        
        Natron::ImageComponents comp = comps.front();
        std::string& tls = _unmappedComponents.localData();
        
        //default to RGBA
        if (comp.getNumComponents() == 0) {
            comp = Natron::ImageComponents::getRGBAComponents();
        }
        tls = natronsComponentsToOfxComponents(comp);
        return tls;
        
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
        std::list<Natron::ImageComponents> comps;
        Natron::ImageBitDepthEnum depth;
        effect->getPreferredDepthAndComponents(-1, &comps, &depth);
        assert(!comps.empty());
        
        const Natron::ImageComponents& comp = comps.front();
        
        if (comp == ImageComponents::getRGBComponents()) {
            return opaqueStr;
        } else if (comp == ImageComponents::getAlphaComponents()) {
            return premultStr;
        }
        
        //case Natron::eImageComponentRGBA: // RGBA can be Opaque, PreMult or UnPreMult
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


/*
 * We have to use TLS here because the OpenFX API necessitate that strings
 * live through the entire duration of the calling action. The is the only way
 * to have it thread-safe and local to a current calling time.
 */
const std::vector<std::string>&
OfxClipInstance::getComponentsPresent() const
{
    
    ComponentsPresentMap& ret = _componentsPresent.localData();
    ret.clear();
    
    EffectInstance::ComponentsAvailableMap compsAvailable;
    if (isOutput()) {
        //If called on the output clip and by a multi-planar effect while in the getClipComponents action
        //this might lead to infinite recursion, so make sure we do not issue a call to getClipComponents again
        double time = _nodeInstance->getCurrentTime();
        int maxInputs = _nodeInstance->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            if (!_nodeInstance->isInputMask(i) && !_nodeInstance->isInputRotoBrush(i)) {
                
                EffectInstance::ComponentsAvailableMap comps;
                EffectInstance* input = _nodeInstance->getInput(i);
                if (input) {
                    input->getComponentsAvailable(time,&comps);
                }
                
                
                for (EffectInstance::ComponentsAvailableMap::iterator it = comps.begin(); it != comps.end(); ++it) {
                    
                    EffectInstance::ComponentsAvailableMap::iterator alreadyExisting = compsAvailable.end();
                    EffectInstance::ComponentsAvailableMap::iterator colorMatch = compsAvailable.end();
                    bool isColor = it->first.isColorPlane();
                    for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2 != compsAvailable.end(); ++it2) {
                        if (it2->first == it->first) {
                            alreadyExisting = it2;
                            break;
                        } else if (isColor && it2->first.isColorPlane()) {
                            colorMatch = it2;
                        }
                    }
                    if (alreadyExisting == compsAvailable.end()) {
                        if (colorMatch != compsAvailable.end()) {
                            if (colorMatch->first.getNumComponents() < it->first.getNumComponents()) {
                                compsAvailable.erase(colorMatch);
                            } else {
                                continue;
                            }
                        }
                        compsAvailable.insert(*it);
                    }
                }
                
            }
        } 
        std::list<ImageComponents> userComps;
        _nodeInstance->getNode()->getUserComponents(&userComps);
        
        ///Foreach user component, add it as an available component, but use this node only if it is also
        ///in the "needed components" list
        for (std::list<ImageComponents>::iterator it = userComps.begin(); it != userComps.end(); ++it) {
            
            EffectInstance::ComponentsAvailableMap::iterator alreadyExisting = compsAvailable.end();
            
            if (it->isColorPlane()) {
                
                EffectInstance::ComponentsAvailableMap::iterator colorMatch = compsAvailable.end();
                
                for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2 != compsAvailable.end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    } else if (it2->first.isColorPlane()) {
                        colorMatch = it2;
                    }
                }
                
                if (alreadyExisting == compsAvailable.end() && colorMatch != compsAvailable.end()) {
                    alreadyExisting = colorMatch;
                }
            } else {
                alreadyExisting = compsAvailable.find(*it);
            }
            
            //If the component already exists from below in the tree, do not add it
            if (alreadyExisting == compsAvailable.end()) {
                compsAvailable.insert(std::make_pair(*it, _nodeInstance->getNode()));
            }
            
        }

        
    } else {
        Natron::EffectInstance* effect = getAssociatedNode();
        if (!effect) {
            return ret;
        }
        double time = effect->getCurrentTime();
        
        effect->getComponentsAvailable(time, &compsAvailable);
    }
    
    for (EffectInstance::ComponentsAvailableMap::iterator it = compsAvailable.begin(); it != compsAvailable.end(); ++it) {
        ret.push_back(natronsComponentsToOfxComponents(it->first));
    }

    
    
    return ret;
}

int
OfxClipInstance::getDimension(const std::string &name) const OFX_EXCEPTION_SPEC
{
    if (name == kFnOfxImageEffectPropComponentsPresent) {
        return (int)getComponentsPresent().size();
    }
    return OFX::Host::ImageEffect::ClipInstance::getDimension(name);
}


// Pixel Aspect Ratio -
//
//  The pixel aspect ratio of a clip or image.
double
OfxClipInstance::getAspectRatio() const
{
    EffectInstance* input = getAssociatedNode();
    if (input && input != _nodeInstance) {
        return input->getPreferredAspectRatio();
    }
    return _aspectRatio;
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
    assert(_nodeInstance);
    if (isOutput()) {
        return _nodeInstance->effectInstance()->getOutputFrameRate();
    }
    
    EffectInstance* input = getAssociatedNode();
    if (input) {
        return input->getPreferredFrameRate();
    }
    return 24.;
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
        U64 hash = n->getRenderHash();
        n->getFrameRange_public(hash,&startFrame, &endFrame);
        
    } else {
        assert(_nodeInstance);
        assert( _nodeInstance->getApp() );
        assert( _nodeInstance->getApp()->getTimeLine() );
        double first,last;
        _nodeInstance->getApp()->getFrameRange(&first, &last);
        startFrame = first;
        endFrame = last;
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
    if ( (getName() == CLIP_OFX_ROTO) && _nodeInstance->getNode()->isRotoNode() ) {
        return true;
    } else {
        if (_isOutput) {
            return _nodeInstance->hasOutputConnected();
        } else {
            int inputNb = getInputNb();
            
            Natron::EffectInstance* input = 0;
            if (isMask()) {
                
                if (!_nodeInstance->getNode()->isMaskEnabled(inputNb)) {
                    return false;
                }
                ImageComponents comps;
                boost::shared_ptr<Natron::Node> maskInput;
                _nodeInstance->getNode()->getMaskChannel(inputNb, &comps, &maskInput);
                if (maskInput) {
                    input = maskInput->getLiveInstance();
                }
                
            } else {
                input = _nodeInstance->getInput(inputNb);
            }

            return input != NULL;
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

void
OfxClipInstance::getRegionOfDefinitionInternal(OfxTime time,int view, unsigned int mipmapLevel,Natron::EffectInstance* associatedNode,
                                               OfxRectD* ret) const
{
    
    boost::shared_ptr<RotoDrawableItem> attachedStroke;
    if (_nodeInstance) {
        assert(_nodeInstance->getNode());
        attachedStroke = _nodeInstance->getNode()->getAttachedRotoItem();
    }
    
    bool inputIsMask = isMask();
    
    RectD rod;
    if (attachedStroke && (inputIsMask || getName() == CLIP_OFX_ROTO)) {
        _nodeInstance->getNode()->getPaintStrokeRoD(time, &rod);
        ret->x1 = rod.x1;
        ret->x2 = rod.x2;
        ret->y1 = rod.y1;
        ret->y2 = rod.y2;
        return;
    } else if (_nodeInstance) {
        boost::shared_ptr<RotoContext> rotoCtx = _nodeInstance->getNode()->getRotoContext();
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
        RenderScale scale;
        scale.x = scale.y = Natron::Image::getScaleFromMipMapLevel(mipmapLevel);
        Natron::StatusEnum st = associatedNode->getRegionOfDefinition_public(nodeHash,time, scale, view, &rod, &isProjectFormat);
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

OfxRectD
OfxClipInstance::getRegionOfDefinition(OfxTime time, int view) const
{
    OfxRectD rod;
    unsigned int mipmapLevel;
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
    } else {
        if (_lastActionData.hasLocalData()) {
            const ActionLocalData& args = _lastActionData.localData();
            if (args.isMipmapLevelValid) {
                mipmapLevel = args.mipMapLevel;
            } else {
                mipmapLevel = 0;
            }
        } else {
            mipmapLevel = 0;
        }
        
    }
    getRegionOfDefinitionInternal(time, view, mipmapLevel, associatedNode, &rod);
    return rod;
}


/// override this to return the rod on the clip canonical coords!
OfxRectD
OfxClipInstance::getRegionOfDefinition(OfxTime time) const
{
    OfxRectD ret;
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
    getRegionOfDefinitionInternal(time, view, mipmapLevel, associatedNode, &ret);
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
        return ImageComponents::getRGBAComponents();
    } else if (plane == kFnOfxImagePlaneForwardMotionVector) {
        return ImageComponents::getForwardMotionComponents();
    } else if (plane == kFnOfxImagePlaneBackwardMotionVector) {
        return ImageComponents::getBackwardMotionComponents();
    } else if (plane == kFnOfxImagePlaneStereoDisparityLeft) {
        return ImageComponents::getDisparityLeftComponents();
    } else if (plane == kFnOfxImagePlaneStereoDisparityRight) {
        return ImageComponents::getDisparityRightComponents();
    } else {
        return ofxCustomCompToNatronComp(plane);
    }

}

std::string
OfxClipInstance::natronsPlaneToOfxPlane(const Natron::ImageComponents& plane)
{
    if (plane.getLayerName() == kNatronColorPlaneName) {
        return kFnOfxImagePlaneColour;
    } else if (plane.getLayerName() == kNatronForwardMotionVectorsPlaneName) {
        return kFnOfxImagePlaneForwardMotionVector;
    } else if (plane.getLayerName() == kNatronBackwardMotionVectorsPlaneName) {
        return kFnOfxImagePlaneBackwardMotionVector;
    } else if (plane.getLayerName() == kNatronDisparityLeftPlaneName) {
        return kFnOfxImagePlaneStereoDisparityLeft;
    } else if (plane.getLayerName() == kNatronDisparityRightPlaneName) {
        return kFnOfxImagePlaneStereoDisparityRight;
    } else {
        return natronCustomCompToOfxComp(plane);
    }
}

OFX::Host::ImageEffect::Image*
OfxClipInstance::getImagePlane(OfxTime time, int view, const std::string& plane,const OfxRectD *optionalBounds)
{
    
    
    
    OfxPointD scale;
    scale.x = scale.y = 1.;
    
    bool hasLocalData = true;
    if ( !_lastActionData.hasLocalData() ) {
        hasLocalData = false;
#ifdef DEBUG
        if (QThread::currentThread() != qApp->thread()) {
            qDebug() << _nodeInstance->getNode()->getScriptName_mt_safe().c_str() << " is trying to call clipGetImage on a thread "
            "not controlled by Natron (probably from the multi-thread suite).\n If you're a developer of that plug-in, please "
            "fix it.";
            
        }
#endif
    }
    assert( _lastActionData.hasLocalData() );
    if (isOutput()) {
        
        ImageComponents natronPlane = ofxPlaneToNatronPlane(plane);

        std::map<ImageComponents,EffectInstance::PlaneToRender> outputPlanes;
        RectI renderWindow;
        Natron::ImageComponents planeBeingRendered;
        bool ok = _nodeInstance->getThreadLocalRenderedPlanes(&outputPlanes,&planeBeingRendered,&renderWindow);
        if (!ok) {
            return NULL;
        }
        
        ImagePtr outputImage;
        
        bool multiPlanar = _nodeInstance->isMultiPlanar();

        if (!multiPlanar) {
            for (std::map<ImageComponents,EffectInstance::PlaneToRender>::iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
                if (it->first.getLayerName() == planeBeingRendered.getLayerName()) {
                    outputImage = it->second.tmpImage;
                    break;
                }
            }

        } else {
            for (std::map<ImageComponents,EffectInstance::PlaneToRender>::iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
                if (it->first.getLayerName() == natronPlane.getLayerName()) {
                    outputImage = it->second.tmpImage;
                    break;
                }
            }
        }
        
        
        if (!outputImage) {
            outputImage = _nodeInstance->allocateImagePlaneAndSetInThreadLocalStorage(natronPlane);
        }
        
        assert(outputImage);
        if (!outputImage) {
            return 0;
        }
        
        
        ActionLocalData& args = _lastActionData.localData();
        for (std::list<OfxImage*>::const_iterator it = args.imagesBeingRendered.begin(); it != args.imagesBeingRendered.end(); ++it) {
            if ((*it)->getInternalImage() == outputImage) {
                (*it)->addReference();
                return *it;
            }
        }
        
        std::string components;
        int nComps;
        if (multiPlanar) {
            components = OfxClipInstance::natronsComponentsToOfxComponents(outputImage->getComponents());
            nComps = outputImage->getComponents().getNumComponents();
        } else {
            std::list<Natron::ImageComponents> natronComps = OfxClipInstance::ofxComponentsToNatronComponents(_components);
            assert(!natronComps.empty());
            components = _components;
            nComps = natronComps.front().getNumComponents();
        }
        
        //The output clip doesn't have any transform matrix
        OfxImage* ret =  new OfxImage(outputImage,false,renderWindow,boost::shared_ptr<Transform::Matrix3x3>(), components, nComps, *this);
        args.imagesBeingRendered.push_back(ret);
        return ret;
    } // if (isOutput())
    
    
    boost::shared_ptr<Transform::Matrix3x3> transform;
    bool usingReroute ;
    int rerouteInputNb;
    Natron::EffectInstance* node =0;
    unsigned int mipMapLevel;
    ActionLocalData* tls = 0;
    if (hasLocalData) {
        tls = &_lastActionData.localData();
        if (!tls->isViewValid) {
#ifdef DEBUG
            if (QThread::currentThread() != qApp->thread()) {
                qDebug() << _nodeInstance->getNode()->getScriptName_mt_safe().c_str() << " is trying to call clipGetImage on a thread "
                "not controlled by Natron (probably from the multi-thread suite).\n If you're a developer of that plug-in, please "
                "fix it. Natron is now going to try to recover from that mistake but doing so can yield unpredictable results.";
            }
#endif
            view = 0;
        } else {
            if (view == -1) {
                view = tls->view;
            }
        }
        
        if (!tls->isMipmapLevelValid) {
            mipMapLevel = 0;
        } else {
            mipMapLevel = tls->mipMapLevel;
        }
        
        if (!tls->isTransformDataValid || !tls->rerouteNode) {
            node = _nodeInstance;
            usingReroute = false;
            rerouteInputNb = -1;
        } else {
            node = tls->rerouteNode;
            assert(node);
            rerouteInputNb = tls->rerouteInputNb;
            transform = tls->matrix;
            usingReroute = true;
        }
        
    } else {
        view = 0;
        mipMapLevel = 0;
        node = _nodeInstance;
        usingReroute = false;
        rerouteInputNb = -1;
    }
    
    scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;
    
    return getImageInternal(time, scale, view, optionalBounds, plane, usingReroute, rerouteInputNb, node, transform);
}

OFX::Host::ImageEffect::Image*
OfxClipInstance::getStereoscopicImage(OfxTime time,
                                      int view,
                                      const OfxRectD *optionalBounds)
{
    Natron::ImageComponents components;
    if (_lastActionData.hasLocalData()) {
        ActionLocalData & args = _lastActionData.localData();
        if (args.clipComponentsValid) {
            
            //Commented-out: Since pre-fetching is disabled for floating point dates, an effect such as retime may
            //end-up in this situation
            /*if (!args.hasImage) {
                EffectInstance* input = getAssociatedNode();
                QString inputName = input ? input->getNode()->getScriptName_mt_safe().c_str() : QString();
                qDebug() << "WARNING:" << _nodeInstance->getScriptName_mt_safe().c_str() << "is trying to fetch an image from"
                << inputName << "at time =" << time << "and view =" << view
                << "but it did not manage to pre-fetch any image from upstream."
                << "This may either be a bug in Natron when calling renderInputImagesForRoI or a plug-in that has mis-implemented the"
                << "getRegionsOfInterest action.";
                return 0;
            }*/
            components = args.clipComponents;
        }
    }
    if (components.getNumComponents() == 0) {
        std::list<ImageComponents> comps = ofxComponentsToNatronComponents(_components);
        assert(!comps.empty());
        components = comps.front();
    }

    std::string plane = natronsPlaneToOfxPlane(components);
    return getImagePlane(time, view, plane, optionalBounds);
} // getStereoscopicImage

OFX::Host::ImageEffect::Image*
OfxClipInstance::getImageInternal(OfxTime time,
                                  const OfxPointD & renderScale,
                                  int view,
                                  const OfxRectD *optionalBounds,
                                  const std::string& plane,
                                  bool usingReroute,
                                  int rerouteInputNb,
                                  Natron::EffectInstance* node,
                                  const boost::shared_ptr<Transform::Matrix3x3>& transform)
{
    assert( !isOutput() && node);
    // input has been rendered just find it in the cache
    RectD bounds;
    if (optionalBounds) {
        bounds.x1 = optionalBounds->x1;
        bounds.y1 = optionalBounds->y1;
        bounds.x2 = optionalBounds->x2;
        bounds.y2 = optionalBounds->y2;
    }
    
    bool multiPlanar = _nodeInstance->isMultiPlanar();
    
    Natron::ImageComponents comp;
    
    if (plane == kFnOfxImagePlaneColour) {
        std::list<ImageComponents> comps = ofxComponentsToNatronComponents(getComponents());
        assert(comps.size() == 1);
        comp = comps.front();
    } else if (plane == kFnOfxImagePlaneBackwardMotionVector) {
        comp = ImageComponents::getBackwardMotionComponents();
    } else if (plane == kFnOfxImagePlaneForwardMotionVector) {
        comp = ImageComponents::getForwardMotionComponents();
    } else if (plane == kFnOfxImagePlaneStereoDisparityLeft) {
        comp = ImageComponents::getDisparityLeftComponents();
    } else if (plane == kFnOfxImagePlaneStereoDisparityRight) {
        comp = ImageComponents::getDisparityRightComponents();
    } else {
        try {
            comp = ofxCustomCompToNatronComp(plane);
        } catch (...) {
            return 0;
        }
    }
    
    
    Natron::ImageBitDepthEnum bitDepth = ofxDepthToNatronDepth( getPixelDepth() );
    double par = getAspectRatio();
    RectI renderWindow;
    boost::shared_ptr<Natron::Image> image;
    
    if (usingReroute) {
        assert(rerouteInputNb != -1);
        
        unsigned int mipMapLevel = Image::getLevelFromScale(renderScale.x);
        
        EffectInstance::RoIMap regionsOfInterests;
        bool gotit = _nodeInstance->getThreadLocalRegionsOfInterests(regionsOfInterests);
        
        
        if (!gotit) {
            qDebug() << "Bug in transform concatenations: thread-storage has not been set on the new upstream input.";
            
            RectD rod;
            if (optionalBounds) {
                rod = bounds;
            } else {
                bool isProjectFormat;
                StatusEnum stat = node->getRegionOfDefinition_public(node->getHash(), time, renderScale, view, &rod, &isProjectFormat);
                assert(stat == Natron::eStatusOK);
                Q_UNUSED(stat);
            }
            node->getRegionsOfInterest_public(time, renderScale, rod, rod, 0,&regionsOfInterests);
        }
        
        EffectInstance* inputNode = node->getInput(rerouteInputNb);
        if (!inputNode) {
            return NULL;
        }
        
        RectD roi;
        if (!optionalBounds) {
            
            EffectInstance::RoIMap::iterator found = regionsOfInterests.find(inputNode);
            assert(found != regionsOfInterests.end());
            ///RoI is in canonical coordinates since the results of getRegionsOfInterest is in canonical coords.
            roi = found->second;
            
        } else {
            roi = bounds;
        }
        
        RectI pixelRoI;
        roi.toPixelEnclosing(mipMapLevel, par, &pixelRoI);
        
        EffectInstance::InputImagesMap inputImages;
        _nodeInstance->getThreadLocalInputImages(&inputImages);
        
        std::list<ImageComponents> requestedComps;
        requestedComps.push_back(comp);
        EffectInstance::RenderRoIArgs args((SequenceTime)time,renderScale,mipMapLevel,
                                           view,false,pixelRoI,RectD(),requestedComps,bitDepth,_nodeInstance,inputImages);
        ImageList planes;
        EffectInstance::RenderRoIRetCode retCode =  inputNode->renderRoI(args,&planes);
        assert(planes.size() == 1 || planes.empty());
        if (planes.empty() || retCode != EffectInstance::eRenderRoIRetCodeOk) {
            return 0;
        }
        
        image = planes.front();
        _nodeInstance->addThreadLocalInputImageTempPointer(rerouteInputNb,image);

        renderWindow = pixelRoI;
        
    } else {
        image = node->getImage(getInputNb(), time, renderScale, view,
                               optionalBounds ? &bounds : NULL,
                               comp,
                               bitDepth,
                               par,
                               false,&renderWindow);
    }
    if (!image) {
        return NULL;
    } else {
        if (renderWindow.isNull()) {
            return NULL;
        } else {
            std::string components;
            int nComps;
            if (multiPlanar) {
                components = OfxClipInstance::natronsComponentsToOfxComponents(image->getComponents());
                nComps = image->getComponents().getNumComponents();
            } else {
                std::list<Natron::ImageComponents> natronComps = OfxClipInstance::ofxComponentsToNatronComponents(_components);
                assert(!natronComps.empty());
                components = _components;
                nComps = natronComps.front().getNumComponents();
            }
            
           /* // this will dump the image as seen from the plug-in
            QString filename;
            QTextStream ts(&filename);
            QDateTime now = QDateTime::currentDateTime();
            ts << "img_" << time << "_"  << now.toMSecsSinceEpoch() << ".png";
            appPTR->debugImage(image.get(), renderWindow, filename);*/
   
            /*
             * When reaching here, the plug-in asked for a source image on an input clip. If the plug-in is in the render action,
             * the image should have been pre-computed hence the call to getImage does not involve writing the image, so no 
             * write lock is taken. In this situation, we lock the OfxImage for reading to ensure another thread is not trying to 
             * write the pixels at the same time.
             */
            return new OfxImage(image,true,renderWindow,transform, components, nComps, *this);
        }
    }
}

#ifdef OFX_SUPPORTS_OPENGLRENDER
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
OfxClipInstance::natronsComponentsToOfxComponents(const Natron::ImageComponents& comp)
{
    if (comp == ImageComponents::getNoneComponents()) {
        return kOfxImageComponentNone;
    } else if (comp == ImageComponents::getAlphaComponents()) {
        return kOfxImageComponentAlpha;
    } else if (comp == ImageComponents::getRGBComponents()) {
        return kOfxImageComponentRGB;
    } else if (comp == ImageComponents::getRGBAComponents()) {
        return kOfxImageComponentRGBA;
    } else if (comp.getComponentsGlobalName() == kNatronMotionComponentsName) {
        return kFnOfxImageComponentMotionVectors;
    } else if (comp.getComponentsGlobalName() == kNatronDisparityComponentsName) {
        return kFnOfxImageComponentStereoDisparity;
    } else {
        return natronCustomCompToOfxComp(comp);
    }
}

std::list<Natron::ImageComponents>
OfxClipInstance::ofxComponentsToNatronComponents(const std::string & comp)
{
    std::list<Natron::ImageComponents> ret;
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
        ret.push_back(ofxCustomCompToNatronComp(comp));
    }
    return ret;
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
                   bool isSrcImage,
                   const RectI& renderWindow,
                   const boost::shared_ptr<Transform::Matrix3x3>& mat,
                   const std::string& components,
                   int nComps,
                   OfxClipInstance &clip)
: OFX::Host::ImageEffect::Image(clip)
, _floatImage(internalImage)
, _imgAccess()
{
    
    assert(internalImage);
    
    
    unsigned int mipMapLevel = internalImage->getMipMapLevel();
    RenderScale scale;

    scale.x = Natron::Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.x, 0);
    setDoubleProperty(kOfxImageEffectPropRenderScale, scale.y, 1);
    
   
    
    // data ptr
    RectI bounds = internalImage->getBounds();
    
    ///Do not activate this assert! The render window passed to renderRoI can be bigger than the actual RoD of the effect
    ///in which case it is just clipped to the RoD.
    //assert(bounds.contains(renderWindow));
    RectI pluginsSeenBounds;
    renderWindow.intersect(bounds, &pluginsSeenBounds);
    
    const RectD & rod = internalImage->getRoD(); // Not the OFX RoD!!! Natron::Image::getRoD() is in *CANONICAL* coordinates

    if (isSrcImage) {
        boost::shared_ptr<Natron::Image::ReadAccess> access(new Natron::Image::ReadAccess(internalImage.get()));
        const unsigned char* ptr = access->pixelAt( pluginsSeenBounds.left(), pluginsSeenBounds.bottom() );
        assert(ptr);
        setPointerProperty( kOfxImagePropData, const_cast<unsigned char*>(ptr));
        _imgAccess = access;
    } else {
        boost::shared_ptr<Natron::Image::WriteAccess> access(new Natron::Image::WriteAccess(internalImage.get()));
        unsigned char* ptr = access->pixelAt( pluginsSeenBounds.left(), pluginsSeenBounds.bottom() );
        assert(ptr);
        setPointerProperty( kOfxImagePropData, ptr);
        _imgAccess = access;
    }
    
    
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


int
OfxClipInstance::getInputNb() const
{
    if (_isOutput) {
        return -1;
    }
    return _nodeInstance->getClipInputNumber(this);
}

Natron::EffectInstance*
OfxClipInstance::getAssociatedNode() const
{
    assert(_nodeInstance);
    if ( (getName() == CLIP_OFX_ROTO) && _nodeInstance->getNode()->isRotoNode() ) {
        return _nodeInstance;
    }
    if (_isOutput) {
        return _nodeInstance;
    } else {
        if (isMask()) {
            ImageComponents comps;
            boost::shared_ptr<Natron::Node> maskInput;
            int inputNb = getInputNb();
            _nodeInstance->getNode()->getMaskChannel(inputNb, &comps, &maskInput);
            if (maskInput) {
                return maskInput->getLiveInstance();
            }
            return 0;
        } else {
            return _nodeInstance->getInput( getInputNb() );
        }
    }
}



void
OfxClipInstance::setRenderedView(int view)
{
    ActionLocalData & args = _lastActionData.localData();
    args.view = view;
    args.isViewValid = true;
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
    ActionLocalData & args = _lastActionData.localData();
    args.mipMapLevel = mipMapLevel;
    args.isMipmapLevelValid =  true;
}

void
OfxClipInstance::discardMipMapLevel()
{
    assert( _lastActionData.hasLocalData() );
    ActionLocalData& data = _lastActionData.localData();
    data.isMipmapLevelValid = false;
    
    //Also clear images that may be left s
    data.imagesBeingRendered.clear();
}

void
OfxClipInstance::setTransformAndReRouteInput(const Transform::Matrix3x3& m,Natron::EffectInstance* rerouteInput,int newInputNb)
{
    assert(rerouteInput);
    ActionLocalData & args = _lastActionData.localData();
    args.matrix.reset(new Transform::Matrix3x3(m));
    args.rerouteInputNb = newInputNb;
    args.rerouteNode = rerouteInput;
    args.isTransformDataValid = true;
}

void
OfxClipInstance::clearTransform()
{
    assert(_lastActionData.hasLocalData());
    _lastActionData.localData().isTransformDataValid = false;
}

void
OfxClipInstance::clearOfxImagesTLS()
{
    assert(_lastActionData.hasLocalData());
    _lastActionData.localData().imagesBeingRendered.clear();
}

void
OfxClipInstance::setClipComponentTLS(bool hasImage,const Natron::ImageComponents& components)
{
    ActionLocalData & args = _lastActionData.localData();
    args.clipComponents = components;
    args.clipComponentsValid = true;
    args.hasImage = hasImage;
}

void
OfxClipInstance::clearClipComponentsTLS()
{
    assert(_lastActionData.hasLocalData());
    ActionLocalData & args = _lastActionData.localData();
    args.clipComponentsValid = false;
    args.clipComponents = ImageComponents::getNoneComponents();
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


