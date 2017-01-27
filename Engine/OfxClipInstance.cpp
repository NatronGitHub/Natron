/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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
#include <sstream> // stringstream

#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/EffectInstanceTLSData.h"
#include "Engine/Image.h"
#include "Engine/TimeLine.h"
#include "Engine/Hash64.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/ImageStorage.h"
#include "Engine/Node.h"
#include "Engine/OSGLContext.h"
#include "Engine/GPUContextPool.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Transform.h"
#include "Engine/TLSHolder.h"
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"

#include <nuke/fnOfxExtensions.h>
#include <ofxOpenGLRender.h>
#include <ofxNatron.h>

NATRON_NAMESPACE_ENTER;

struct OfxClipInstancePrivate
{
    OfxClipInstance* _publicInterface; // can not be a smart ptr
    boost::weak_ptr<OfxEffectInstance> nodeInstance;
    OfxImageEffectInstance* const effect;
    double aspectRatio;
    bool optional;
    bool mask;
    boost::shared_ptr<TLSHolder<OfxClipInstance::ClipTLSData> > tlsData;

    OfxClipInstancePrivate(OfxClipInstance* publicInterface,
                           const OfxEffectInstancePtr& nodeInstance,
                           OfxImageEffectInstance* effect)
        : _publicInterface(publicInterface)
        , nodeInstance(nodeInstance)
        , effect(effect)
        , aspectRatio(1.)
        , optional(false)
        , mask(false)
        , tlsData( new TLSHolder<OfxClipInstance::ClipTLSData>() )
    {
    }

    const std::vector<std::string>& getComponentsPresentInternal(const OfxClipInstance::ClipDataTLSPtr& tls) const;
};

OfxClipInstance::OfxClipInstance(const OfxEffectInstancePtr& nodeInstance,
                                 OfxImageEffectInstance* effect,
                                 int /*index*/,
                                 OFX::Host::ImageEffect::ClipDescriptor* desc)
    : OFX::Host::ImageEffect::ClipInstance(effect, *desc)
    , _imp( new OfxClipInstancePrivate(this, nodeInstance, effect) )
{
    assert(nodeInstance && effect);
    _imp->optional = isOptional();
    _imp->mask = isMask();
}

OfxClipInstance::~OfxClipInstance()
{
}

// callback which should update label
void
OfxClipInstance::setLabel()
{
    OfxEffectInstancePtr effect = _imp->nodeInstance.lock();
    if (effect) {
        int inputNb = getInputNb();
        if (inputNb >= 0) {
            effect->onClipLabelChanged(inputNb, getLabel());
        }
    }
}

// callback which should set secret state as appropriate
void OfxClipInstance::setSecret()
{
    OfxEffectInstancePtr effect = _imp->nodeInstance.lock();
    if (effect) {
        int inputNb = getInputNb();
        if (inputNb >= 0) {
            effect->onClipSecretChanged(inputNb, isSecret());
        }
    }
}

// callback which should update hint
void OfxClipInstance::setHint()
{
    OfxEffectInstancePtr effect = _imp->nodeInstance.lock();
    if (effect) {
        int inputNb = getInputNb();
        if (inputNb >= 0) {
            effect->onClipHintChanged(inputNb, getHint());
        }
    }
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

    EffectInstancePtr effect = getAssociatedNode();
    if (!effect) {
        return natronsDepthToOfxDepth( getEffectHolder()->getNode()->getClosestSupportedBitDepth(eImageBitDepthFloat) );
    } else {
        TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
        return natronsDepthToOfxDepth(effect->getBitDepth(render, -1));
    }

} // OfxClipInstance::getUnmappedBitDepth

const std::string &
OfxClipInstance::getUnmappedComponents() const
{

    EffectInstancePtr effect = getAssociatedNode();

    std::string ret;
    if (effect) {

        TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();

        ImagePlaneDesc metadataPlane, metadataPairedPlane;
        effect->getMetadataComponents(render, -1, &metadataPlane, &metadataPairedPlane);

        // Default to RGBA
        if (metadataPlane.getNumComponents() == 0) {
            metadataPlane = ImagePlaneDesc::getRGBAComponents();
        }
        ret = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(metadataPlane);
    } else {

        // The node is not connected but optional, return the closest supported components
        // of the first connected non optional input.
        if (_imp->optional) {
            effect = getEffectHolder();
            TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
            assert(effect);
            int nInputs = effect->getMaxInputCount();
            for (int i = 0; i < nInputs; ++i) {

                ImagePlaneDesc metadataPlane, metadataPairedPlane;
                effect->getMetadataComponents(render, i, &metadataPlane, &metadataPairedPlane);

                if (metadataPlane.getNumComponents() > 0) {
                    ret = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(metadataPlane);
                }
            }
        }


        // last-resort: black and transparent image means RGBA.
        if (ret.empty()) {
            ret = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(ImagePlaneDesc::getRGBAComponents());
        }
    }

    ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    tls->unmappedComponents = ret;
    return tls->unmappedComponents;

} // getUnmappedComponents

// PreMultiplication -
//
//  kOfxImageOpaque - the image is opaque and so has no premultiplication state
//  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
//  kOfxImageUnPreMultiplied - the image is unpremultiplied
const std::string &
OfxClipInstance::getPremult() const
{
    EffectInstancePtr effect = getAssociatedNode();

    if (!effect) {
        return natronsPremultToOfxPremult(eImagePremultiplicationPremultiplied);
    }
    TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
    return natronsPremultToOfxPremult(effect->getPremult(render));

}

const std::vector<std::string>&
OfxClipInstancePrivate::getComponentsPresentInternal(const OfxClipInstance::ClipDataTLSPtr& tls) const
{
    tls->componentsPresent.clear();

    EffectInstancePtr effect = _publicInterface->getEffectHolder();
    if (!effect) {
        return tls->componentsPresent;
    }

    int inputNb = _publicInterface->getInputNb();

    TimeValue time = effect->getCurrentTime_TLS();
    ViewIdx view = effect->getCurrentView_TLS();
    TreeRenderNodeArgsPtr renderArgs = effect->getCurrentRender_TLS();

    std::list<ImagePlaneDesc> availableLayers;
    ActionRetCodeEnum stat = effect->getAvailableLayers(time, view, inputNb, renderArgs, &availableLayers);
    if (isFailureRetCode(stat)) {
        return tls->componentsPresent;
    }

    for (std::list<ImagePlaneDesc>::iterator it = availableLayers.begin(); it != availableLayers.end(); ++it) {
        std::string ofxPlane = ImagePlaneDesc::mapPlaneToOFXPlaneString(*it);
        tls->componentsPresent.push_back(ofxPlane);
    }

    return tls->componentsPresent;
} // getComponentsPresentInternal

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

const std::string &
OfxClipInstance::getComponents() const
{
    EffectInstancePtr effect = getEffectHolder();

    std::string ret;
    if (!effect) {
        ret = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(ImagePlaneDesc::getRGBAComponents());
    } else {

        TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
        int inputNb = getInputNb();

        ImagePlaneDesc metadataPlane, metadataPairedPlane;
        effect->getMetadataComponents(render, inputNb, &metadataPlane, &metadataPairedPlane);

        // Default to RGBA
        if (metadataPlane.getNumComponents() == 0) {
            metadataPlane = ImagePlaneDesc::getRGBAComponents();
        }
        ret = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(metadataPlane);
    }

    ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    tls->components = ret;
    return tls->components;
    

}

// overridden from OFX::Host::ImageEffect::ClipInstance
// Pixel Aspect Ratio -
//
//  The pixel aspect ratio of a clip or image.
double
OfxClipInstance::getAspectRatio() const
{
    EffectInstancePtr effect = getEffectHolder();

    if (!effect) {
        return 1.;
    } else {

        TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
        int inputNb = getInputNb();
        return effect->getAspectRatio(render, inputNb);
    }

}


OfxRectI
OfxClipInstance::getFormat() const
{

    RectI nRect;

    EffectInstancePtr effect = getAssociatedNode();

    if (!effect) {
        Format f;
        _imp->effect->getOfxEffectInstance()->getApp()->getProject()->getProjectDefaultFormat(&f);
        nRect = f;
    } else {
        TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
        nRect = effect->getOutputFormat(render);
    }

    OfxRectI ret = {nRect.x1, nRect.y1, nRect.x2, nRect.y2};
    return ret;
}

// Frame Rate -
double
OfxClipInstance::getFrameRate() const
{

    EffectInstancePtr effect = getAssociatedNode();
    if (!effect) {
        return _imp->effect->getOfxEffectInstance()->getApp()->getProjectFrameRate();
    } else {
        TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
        return effect->getFrameRate(render);
    }
}

// overridden from OFX::Host::ImageEffect::ClipInstance
// Frame Range (startFrame, endFrame) -
//
//  The frame range over which a clip has images.
void
OfxClipInstance::getFrameRange(double &startFrame,
                               double &endFrame) const
{
    EffectInstancePtr effect = getAssociatedNode();

    if (!effect) {
        TimeValue left,right;
        _imp->effect->getOfxEffectInstance()->getApp()->getProject()->getFrameRange(&left, &right);
        startFrame = left;
        endFrame = right;
    } else {
        TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
        GetFrameRangeResultsPtr results;
        ActionRetCodeEnum stat = effect->getFrameRange_public(render, &results);
        if (!isFailureRetCode(stat)) {
            RangeD range;
            results->getFrameRangeResults(&range);
            startFrame = range.min;
            endFrame = range.max;
        }
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
    EffectInstancePtr effect = getAssociatedNode();
    if (!effect) {
        return natronsFieldingToOfxFielding(eImageFieldingOrderNone);
    }
    TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
    return natronsFieldingToOfxFielding( effect->getFieldingOrder(render) );

}

//  Says whether the clip is actually connected at the moment.
bool
OfxClipInstance::getConnected() const
{
    EffectInstancePtr effect = getEffectHolder();

    assert(effect);

    if (_isOutput) {
        return effect->hasOutputConnected();
    } else {

        int inputNb = getInputNb();
        EffectInstancePtr input = effect->resolveInputEffectForFrameNeeded(inputNb, 0);
        return input.get() != 0;
    }

}


//  The unmaped frame range over which an output clip has images.
double
OfxClipInstance::getUnmappedFrameRate() const
{
    EffectInstancePtr effect = getAssociatedNode();

    if (!effect) {
        // The node is not connected, return project frame rate
        return _imp->effect->getOfxEffectInstance()->getApp()->getProjectFrameRate();
    } else {
        // Get the node frame rate metadata
        TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
        return effect->getFrameRate(render);
    }
}

//  The unmaped frame range over which an output clip has images.
// this is applicable only to hosts and plugins that allow a plugin to change frame rates
void
OfxClipInstance::getUnmappedFrameRange(double &unmappedStartFrame,
                                       double &unmappedEndFrame) const
{
    return getFrameRange(unmappedStartFrame, unmappedEndFrame);
}


//  false if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
//  true if the images can be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
bool
OfxClipInstance::getContinuousSamples() const
{
    EffectInstancePtr effect = getAssociatedNode();

    if (!effect) {
        return false;
    }

    TreeRenderNodeArgsPtr render = effect->getCurrentRender_TLS();
    return effect->canRenderContinuously(render);
}

void
OfxClipInstance::getRegionOfDefinitionInternal(OfxTime time,
                                               ViewIdx view,
                                               const RenderScale& scale,
                                               EffectInstancePtr associatedNode,
                                               const TreeRenderNodeArgsPtr& associatedNodeRenderArgs,
                                               OfxRectD* ret) const
{
    if (!associatedNode) {
        ret->x1 = 0.;
        ret->x2 = 0.;
        ret->y1 = 0.;
        ret->y2 = 0.;
        return;
    }

    GetRegionOfDefinitionResultsPtr rodResults;
    ActionRetCodeEnum stat = associatedNode->getRegionOfDefinition_public(TimeValue(time), scale, view, associatedNodeRenderArgs, &rodResults);
    if (isFailureRetCode(stat)) {
        ret->x1 = 0.;
        ret->x2 = 0.;
        ret->y1 = 0.;
        ret->y2 = 0.;
    } else {
        const RectD& rod = rodResults->getRoD();
        ret->x1 = rod.left();
        ret->x2 = rod.right();
        ret->y1 = rod.bottom();
        ret->y2 = rod.top();
    }

} // OfxClipInstance::getRegionOfDefinitionInternal

// overridden from OFX::Host::ImageEffect::ClipInstance
OfxRectD
OfxClipInstance::getRegionOfDefinition(OfxTime time,
                                       int view) const
{
    RenderScale scale(1.);
    EffectInstancePtr effect = getAssociatedNode();
    TreeRenderNodeArgsPtr effectRenderArgs;
    if (effect) {
        EffectInstanceTLSDataPtr effectTLS = effect->getTLSObject();
        if (effectTLS) {
            effectTLS->getCurrentActionArgs(0, 0, &scale);
            effectRenderArgs = effectTLS->getRenderArgs();
        }
    }

    OfxRectD rod;
    getRegionOfDefinitionInternal(time, ViewIdx(view), scale, effect, effectRenderArgs, &rod);

    return rod;
}

// overridden from OFX::Host::ImageEffect::ClipInstance
/// override this to return the rod on the clip canonical coords!
OfxRectD
OfxClipInstance::getRegionOfDefinition(OfxTime time) const
{
    OfxRectD ret;
    RenderScale scale(1.);
    ViewIdx view(0);
    EffectInstancePtr effect = getAssociatedNode();
    TreeRenderNodeArgsPtr effectRenderArgs;

    if (effect) {
        EffectInstanceTLSDataPtr effectTLS = effect->getTLSObject();
        if (effectTLS) {
            effectTLS->getCurrentActionArgs(0, &view, &scale);
            effectRenderArgs = effectTLS->getRenderArgs();
        }
    }
    getRegionOfDefinitionInternal(time, view, scale, effect, effectRenderArgs, &ret);
    return ret;
} // getRegionOfDefinition

#ifdef OFX_SUPPORTS_OPENGLRENDER
// overridden from OFX::Host::ImageEffect::ClipInstance
/// override this to fill in the OpenGL texture at the given time.
/// The bounds of the image on the image plane should be
/// 'appropriate', typically the value returned in getRegionsOfInterest
/// on the effect instance. Outside a render call, the optionalBounds should
/// be 'appropriate' for the.
/// If bounds is not null, fetch the indicated section of the canonical image plane.
OFX::Host::ImageEffect::Texture*
OfxClipInstance::loadTexture(OfxTime time,
                             const char *format,
                             const OfxRectD *optionalBounds)
{
    ImageBitDepthEnum depth = eImageBitDepthNone;

    if (format) {
        depth = ofxDepthToNatronDepth( std::string(format) );
    }

    OFX::Host::ImageEffect::Texture* texture = 0;
    const ViewIdx* view = 0;
    if ( !getImagePlaneInternal(time, view, optionalBounds, 0 /*plane*/, format ? &depth : 0, 0 /*image*/, &texture) ) {
        return 0;
    }

    return texture;
}

#endif

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
    OFX::Host::ImageEffect::Image* image = 0;
    const ViewIdx* view = 0;
    if ( !getImagePlaneInternal(time, view, optionalBounds, 0 /*plane*/, 0 /*texdepth*/, &image, 0 /*tex*/) ) {
        return 0;
    }

    return image;
}

// overridden from OFX::Host::ImageEffect::ClipInstance
OFX::Host::ImageEffect::Image*
OfxClipInstance::getStereoscopicImage(OfxTime time,
                                      int view,
                                      const OfxRectD *optionalBounds)
{
    OFX::Host::ImageEffect::Image* image = 0;

    ViewIdx spec;
    // The Foundry Furnace plug-ins pass -1 to the view parameter, we need to deal with it.
    if (view != -1) {
        spec = ViewIdx(view);
    }
    if ( !getImagePlaneInternal(time, view == -1 ? 0 : &spec, optionalBounds, 0 /*plane*/, 0 /*texdepth*/, &image, 0 /*tex*/) ) {
        return 0;
    }

    return image;
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

    ViewIdx spec;
    // The Foundry Furnace plug-ins pass -1 to the view parameter, we need to deal with it.
    if (view != -1) {
        spec = ViewIdx(view);
    }

    OFX::Host::ImageEffect::Image* image = 0;
    if ( !getImagePlaneInternal(time, view == -1 ? 0 : &spec, optionalBounds, &plane, 0 /*texdepth*/, &image, 0 /*tex*/) ) {
        return 0;
    }

    return image;
}

bool
OfxClipInstance::getImagePlaneInternal(OfxTime time,
                                       const ViewIdx* view,
                                       const OfxRectD *optionalBounds,
                                       const std::string* ofxPlane,
                                       const ImageBitDepthEnum* textureDepth,
                                       OFX::Host::ImageEffect::Image** image,
                                       OFX::Host::ImageEffect::Texture** texture)
{
    if (time != time) {
        // time is NaN

        return false;
    }

    if ( isOutput() ) {
        return getOutputImageInternal(ofxPlane, textureDepth, image, texture);
    } else {
        return getInputImageInternal(time, view, optionalBounds, ofxPlane, textureDepth, image, texture);
    }
}

bool
OfxClipInstance::getInputImageInternal(const OfxTime time,
                                       const ViewIdx* viewParam,
                                       const OfxRectD *optionalBounds,
                                       const std::string* ofxPlane,
                                       const ImageBitDepthEnum* /*textureDepth*/, // < unused
                                       OFX::Host::ImageEffect::Image** retImage,
                                       OFX::Host::ImageEffect::Texture** retTexture)
{
    assert( !isOutput() );
    assert( (retImage && !retTexture) || (!retImage && retTexture) );

    if (time != time) {
        // time is NaN
        return false;
    }


    EffectInstancePtr effect = getEffectHolder();
    if (!effect) {
        return false;
    }

    EffectInstanceTLSDataPtr effectTLS = effect->getTLSObject();

    // If there's no tls object this is a bug in Natron
    assert(effectTLS);
    if (!effectTLS) {
        return false;
    }

    // Get the current action arguments from the thread local storage
    TreeRenderNodeArgsPtr renderArgs = effectTLS->getRenderArgs();

    TimeValue currentActionTime;
    ViewIdx currentActionView;
    RenderScale currentActionScale;
    bool gotTLS = effectTLS->getCurrentActionArgs(&currentActionTime, &currentActionView, &currentActionScale);
    assert(gotTLS);
    if (!gotTLS) {
        // If there's no tls object this is a bug in Natron
        return false;
    }

    // Request the current action view if no view is specified in input
    ViewIdx inputView = viewParam ? *viewParam : currentActionView;
    TimeValue inputTime(time);

    const int inputNb = getInputNb();

    // Figure out the plane to fetch
    ImagePlaneDesc plane;
    if (ofxPlane) {
        if (*ofxPlane == kFnOfxImagePlaneColour) {
            ImagePlaneDesc metadataPairedPlane;
            effect->getMetadataComponents(renderArgs, inputNb, &plane, &metadataPairedPlane);
        } else {
            plane = ImagePlaneDesc::mapOFXPlaneStringToPlane(*ofxPlane);
        }
    } else {

        // Use the results of the getLayersProducedAndNeeded action
        GetComponentsResultsPtr actionResults;
        ActionRetCodeEnum stat = effect->getLayersProducedAndNeeded_public(currentActionTime, currentActionView, renderArgs, &actionResults);
        if (isFailureRetCode(stat)) {
            return false;
        }

        std::map<int, std::list<ImagePlaneDesc> > neededInputLayers;
        std::list<ImagePlaneDesc> producedLayers, availableLayers;
        int passThroughInputNb;
        ViewIdx passThroughView;
        TimeValue passThroughTime;
        std::bitset<4> processChannels;
        bool processAll;
        actionResults->getResults(&neededInputLayers, &producedLayers, &availableLayers, &passThroughInputNb, &passThroughTime, &passThroughView, &processChannels, &processAll);

        std::map<int, std::list<ImagePlaneDesc> > ::const_iterator foundNeededLayers = neededInputLayers.find(inputNb);
        // The planes should have been specified for this clip
        if (foundNeededLayers == neededInputLayers.end() || foundNeededLayers->second.empty()) {
            ImagePlaneDesc metadataPairedPlane;
            effect->getMetadataComponents(renderArgs, inputNb, &plane, &metadataPairedPlane);
        } else {
            plane = foundNeededLayers->second.front();
        }
    }

    // No specified components, this is likely because the clip is disconnected.
    if (plane.getNumComponents() == 0) {
        return false;
    }

    // If the plug-in specified a region to render, ask for it, otherwise we render what was
    // requested from downstream nodes.
    RectD boundsParam;
    if (optionalBounds) {
        boundsParam.x1 = optionalBounds->x1;
        boundsParam.y1 = optionalBounds->y1;
        boundsParam.x2 = optionalBounds->x2;
        boundsParam.y2 = optionalBounds->y2;
    }


    EffectInstance::GetImageOutArgs outArgs;
    boost::scoped_ptr<EffectInstance::GetImageInArgs> inArgs(new EffectInstance::GetImageInArgs);
    {
        inArgs->currentTime = currentActionTime;
        inArgs->currentView = currentActionView;
        inArgs->currentScale = currentActionScale;
        RenderBackendTypeEnum backend = retTexture ? eRenderBackendTypeOpenGL : eRenderBackendTypeCPU;
        inArgs->renderBackend = &backend;
        inArgs->renderArgs = renderArgs;
        inArgs->inputTime = inputTime;
        inArgs->inputView = inputView;
        inArgs->inputNb = inputNb;
    }
    bool ok = effect->getImagePlanes(*inArgs, &outArgs);
    if (!ok || outArgs.imagePlanes.empty() || outArgs.roiPixel.isNull()) {
        return false;
    }


    ImagePtr image = outArgs.imagePlanes.begin()->second;
    assert(image);

    // If the effect is not multi-planar (most of effects) then the returned image might have a different
    // layer name than the color layer but should always have the same number of components than the components
    // set in getClipPreferences.
    bool multiPlanar = effect->isMultiPlanar();
    std::string componentsStr;
    int nComps;
    {
        const ImagePlaneDesc& imageLayer = image->getLayer();
        nComps = imageLayer.getNumComponents();

        if (multiPlanar) {
            // Multi-planar: return exactly the layer
            componentsStr = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(plane);
        } else {
            // Non multi-planar: map the layer name to the clip preferences layer name
            ImagePlaneDesc metadataPlane, metadataPairedPlane;
            effect->getMetadataComponents(renderArgs, inputNb, &metadataPlane, &metadataPairedPlane);
            assert(metadataPlane.getNumComponents() == imageLayer.getNumComponents());
            componentsStr = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(metadataPlane);
        }
    }
    

#ifdef DEBUG
    // This will dump the image as seen from the plug-in
    /*QString filename;
       QTextStream ts(&filename);
       QDateTime now = QDateTime::currentDateTime();
       ts << "img_" << time << "_"  << now.toMSecsSinceEpoch() << ".png";
       appPTR->debugImage(image.get(), renderWindow, filename);*/
#endif

    EffectInstancePtr inputEffect = getAssociatedNode();
    if (!inputEffect) {
        return false;
    }

    double par = effect->getAspectRatio(renderArgs, inputNb);
    ImageFieldingOrderEnum fielding = inputEffect->getFieldingOrder(renderArgs);
    ImagePremultiplicationEnum premult = inputEffect->getPremult(renderArgs);
    TreeRenderNodeArgsPtr inputRenderArgs = renderArgs->getInputRenderArgs(inputNb);
    assert(inputRenderArgs);

    U64 inputNodeFrameViewHash = 0;
    inputRenderArgs->getFrameViewHash(inputTime, inputView, &inputNodeFrameViewHash);

    RectD rod;
    {

        GetRegionOfDefinitionResultsPtr rodResults;
        ActionRetCodeEnum stat = inputEffect->getRegionOfDefinition_public(inputTime, currentActionScale, inputView, inputRenderArgs, &rodResults);
        if (isFailureRetCode(stat)) {
            return false;
        }
        rod = rodResults->getRoD();
    }


    OfxImageCommon* retCommon = 0;
    if (retImage) {
        OfxImage* ofxImage = new OfxImage(getEffectHolder(), inputNb, image, rod, premult, fielding, inputNodeFrameViewHash, outArgs.roiPixel, outArgs.distortionStack, componentsStr, nComps, par);
        *retImage = ofxImage;
        retCommon = ofxImage;
    } else if (retTexture) {
        OfxTexture* ofxTex = new OfxTexture(getEffectHolder(), inputNb, image, rod, premult, fielding, inputNodeFrameViewHash, outArgs.roiPixel, outArgs.distortionStack, componentsStr, nComps, par);
        *retTexture = ofxTex;
        retCommon = ofxTex;
    }


    return true;
} // OfxClipInstance::getInputImageInternal

bool
OfxClipInstance::getOutputImageInternal(const std::string* ofxPlane,
                                        const ImageBitDepthEnum* /*textureDepth*/, // < ignore requested texture depth because internally we use 32bit fp textures, so we offer the highest possible quality anyway.
                                        OFX::Host::ImageEffect::Image** retImage,
                                        OFX::Host::ImageEffect::Texture** retTexture)
{
    assert( (retImage && !retTexture) || (!retImage && retTexture) );

    EffectInstancePtr effect = getEffectHolder();
    if (!effect) {
        return false;
    }
    EffectInstanceTLSDataPtr effectTLS = effect->getTLSObject();

    std::map<ImagePlaneDesc, PlaneToRender> outputPlanes;

    // Get the current action arguments. The action must be the render action, otherwise it fails.
    TreeRenderNodeArgsPtr renderArgs = effectTLS->getRenderArgs();
    TimeValue currentActionTime;
    ViewIdx currentActionView;
    RenderScale currentActionScale;
    RectI currentRenderWindow;
    bool gotTLS = effectTLS->getCurrentRenderActionArgs(&currentActionTime, &currentActionView, &currentActionScale, &currentRenderWindow, &outputPlanes);

    if (!gotTLS) {
        std::cerr << effect->getScriptName_mt_safe() << ": clipGetImage on the output clip may only be called during the render action" << std::endl;
        return false;
    }


    // Figure out the plane requested
    ImagePlaneDesc plane;
    if (ofxPlane) {
        if (*ofxPlane == kFnOfxImagePlaneColour) {
            ImagePlaneDesc metadataPairedPlane;
            effect->getMetadataComponents(renderArgs, -1, &plane, &metadataPairedPlane);
        } else {
            plane = ImagePlaneDesc::mapOFXPlaneStringToPlane(*ofxPlane);
        }
    } else {

        // Get the plane from the output planes already allocated
        if (!outputPlanes.empty()) {
            plane = outputPlanes.begin()->first;
        } else {
            //  If the plugin is multi-planar, we are in the situation where it called the regular clipGetImage without a plane in argument
            // so the components will not have been set on the TLS hence just use regular components.
            ImagePlaneDesc metadataPairedPlane;
            effect->getMetadataComponents(renderArgs, -1, &plane, &metadataPairedPlane);
        }
    }

    // The components were not specified, this is likely because the node has never been connected first, or a bug in the plug-in.
    if (plane.getNumComponents() == 0) {
        return false;
    }

    // Find an image plane already allocated, if not create it.
    ImagePtr image;
    for (std::map<ImagePlaneDesc, PlaneToRender>::iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
        if (it->first.getPlaneID() == plane.getPlaneID()) {
            image = it->second.tmpImage;
            break;
        }
    }

    // The output image MAY not exist in the TLS in some cases:
    // e.g: Natron requested Motion.Forward but plug-ins only knows how to render Motion.Forward + Motion.Backward
    // We then just allocate on the fly the plane
    if (!image) {
        Image::InitStorageArgs initArgs;
        initArgs.bounds = currentRenderWindow;
        initArgs.renderArgs = renderArgs;
        initArgs.storage = retTexture ? eStorageModeGLTex : eStorageModeRAM;
        initArgs.layer = plane;
        initArgs.proxyScale = currentActionScale;
        initArgs.mipMapLevel = 0;

        OSGLContextAttacherPtr contextAttacher = appPTR->getGPUContextPool()->getThreadLocalContext();
        if (contextAttacher) {
            initArgs.glContext = contextAttacher->getContext();
        }
        image = Image::create(initArgs);
    }


    // If the effect is not multi-planar (most of effects) then the returned image might have a different
    // layer name than the color layer but should always have the same number of components then the components
    // set in getClipPreferences.
    const bool multiPlanar = effect->isMultiPlanar();
    std::string componentsStr;
    int nComps;
    {
        const ImagePlaneDesc& imageLayer = image->getLayer();
        nComps = imageLayer.getNumComponents();

        if (multiPlanar) {
            // Multi-planar: return exactly the layer
            componentsStr = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(plane);
        } else {
            // Non multi-planar: map the layer name to the clip preferences layer name
            ImagePlaneDesc metadataPlane, metadataPairedPlane;
            effect->getMetadataComponents(renderArgs, -1, &metadataPlane, &metadataPairedPlane);
            assert(metadataPlane.getNumComponents() == imageLayer.getNumComponents());
            componentsStr = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(metadataPlane);
        }
    }


    assert(!retTexture || image->getStorageMode() == eStorageModeGLTex);

    double par = effect->getAspectRatio(renderArgs, -1);
    ImageFieldingOrderEnum fielding = effect->getFieldingOrder(renderArgs);
    ImagePremultiplicationEnum premult = effect->getPremult(renderArgs);


    U64 nodeFrameViewHash = 0;
    renderArgs->getFrameViewHash(currentActionTime, currentActionView, &nodeFrameViewHash);

    RectD rod;
    {

        GetRegionOfDefinitionResultsPtr rodResults;
        ActionRetCodeEnum stat = effect->getRegionOfDefinition_public(currentActionTime, currentActionScale, currentActionView, renderArgs, &rodResults);
        if (isFailureRetCode(stat)) {
            return false;
        }
        rod = rodResults->getRoD();
    }


    OfxImageCommon* retCommon = 0;
    if (retImage) {
        OfxImage* ofxImage = new OfxImage(effect, -1, image, rod, premult, fielding, nodeFrameViewHash, currentRenderWindow, Distortion2DStackPtr(), componentsStr, nComps, par);
        *retImage = ofxImage;
        retCommon = ofxImage;
    } else if (retTexture) {
        OfxTexture* ofxTex = new OfxTexture(effect, -1, image, rod, premult, fielding, nodeFrameViewHash, currentRenderWindow, Distortion2DStackPtr(), componentsStr, nComps, par);
        *retTexture = ofxTex;
        retCommon = ofxTex;
    }

    return true;
} // OfxClipInstance::getOutputImageInternal


ImageBitDepthEnum
OfxClipInstance::ofxDepthToNatronDepth(const std::string & depth, bool throwOnFailure)
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
    } else if (throwOnFailure) {
        throw std::runtime_error(depth + ": unsupported bitdepth");
    }
    return eImageBitDepthNone;
}

const std::string&
OfxClipInstance::natronsDepthToOfxDepth(ImageBitDepthEnum depth)
{
    static const std::string byte(kOfxBitDepthByte);
    static const std::string shrt(kOfxBitDepthShort);
    static const std::string flt(kOfxBitDepthFloat);
    static const std::string hlf(kOfxBitDepthHalf);
    static const std::string none(kOfxBitDepthNone);

    switch (depth) {
    case eImageBitDepthByte:

        return byte;
    case eImageBitDepthShort:

        return shrt;
    case eImageBitDepthHalf:

        return hlf;
    case eImageBitDepthFloat:

        return flt;
    case eImageBitDepthNone:

        return none;
    }

    return none;
}

ImagePremultiplicationEnum
OfxClipInstance::ofxPremultToNatronPremult(const std::string& str)
{
    if (str == kOfxImagePreMultiplied) {
        return eImagePremultiplicationPremultiplied;
    } else if (str == kOfxImageUnPreMultiplied) {
        return eImagePremultiplicationUnPremultiplied;
    } else if (str == kOfxImageOpaque) {
        return eImagePremultiplicationOpaque;
    } else {
        assert(false);

        return eImagePremultiplicationPremultiplied;
    }
}

const std::string&
OfxClipInstance::natronsPremultToOfxPremult(ImagePremultiplicationEnum premult)
{
    static const std::string prem(kOfxImagePreMultiplied);
    static const std::string unprem(kOfxImageUnPreMultiplied);
    static const std::string opq(kOfxImageOpaque);

    switch (premult) {
    case eImagePremultiplicationPremultiplied:

        return prem;
    case eImagePremultiplicationUnPremultiplied:

        return unprem;
    case eImagePremultiplicationOpaque:

        return opq;
    }

    return prem;
}

ImageFieldingOrderEnum
OfxClipInstance::ofxFieldingToNatronFielding(const std::string& fielding)
{
    if (fielding == kOfxImageFieldNone) {
        return eImageFieldingOrderNone;
    } else if (fielding == kOfxImageFieldLower) {
        return eImageFieldingOrderLower;
    } else if (fielding == kOfxImageFieldUpper) {
        return eImageFieldingOrderUpper;
    } else {
        assert(false);
        throw std::invalid_argument("Unknown fielding " + fielding);
    }
}

const std::string&
OfxClipInstance::natronsFieldingToOfxFielding(ImageFieldingOrderEnum fielding)
{
    static const std::string noFielding(kOfxImageFieldNone);
    static const std::string upperFielding(kOfxImageFieldUpper);
    static const std::string lowerFielding(kOfxImageFieldLower);

    switch (fielding) {
    case eImageFieldingOrderNone:

        return noFielding;
    case eImageFieldingOrderUpper:

        return upperFielding;
    case eImageFieldingOrderLower:

        return lowerFielding;
    }

    return noFielding;
}

struct OfxImageCommonPrivate
{
    OFX::Host::ImageEffect::ImageBase* ofxImageBase;

    // A strong reference to the internal natron image.
    // It is vital to keep it here: as long as it lives,
    // it guarantees that the memory is not deallocated
    ImagePtr natronImage;

    std::string components;

    OfxImageCommonPrivate(OFX::Host::ImageEffect::ImageBase* ofxImageBase,
                          const ImagePtr& image)
        : ofxImageBase(ofxImageBase)
        , natronImage(image)
        , components()
    {
    }
};

ImagePtr
OfxImageCommon::getInternalImage() const
{
    return _imp->natronImage;
}

const std::string&
OfxImageCommon::getComponentsString() const
{
    return _imp->components;
}

static void ofxaApplyDistortionStack(double distortedX, double distortedY, const void* stack, double* undistortedX, double* undistortedY)
{
    const Distortion2DStack* dstack = (const Distortion2DStack*)stack;
    Distortion2DStack::applyDistortionStack(distortedX, distortedY, *dstack, undistortedX, undistortedY);
}

OfxImageCommon::OfxImageCommon(const EffectInstancePtr& outputClipEffect,
                               int inputNb,
                               OFX::Host::ImageEffect::ImageBase* ofxImageBase,
                               const ImagePtr& internalImage,
                               const RectD& rod,
                               ImagePremultiplicationEnum premult,
                               ImageFieldingOrderEnum fielding,
                               U64 nodeFrameViewHash,
                               const RectI& renderWindow,
                               const Distortion2DStackPtr& distortion,
                               const std::string& components,
                               int nComps,
                               double par)
    : _imp( new OfxImageCommonPrivate(ofxImageBase, internalImage) )
{
    _imp->components = components;

    assert(internalImage);

    unsigned int mipMapLevel = internalImage->getMipMapLevel();
    RenderScale scale( NATRON_NAMESPACE::Image::getScaleFromMipMapLevel(mipMapLevel) );
    ofxImageBase->setDoubleProperty(kOfxImageEffectPropRenderScale, scale.x, 0);
    ofxImageBase->setDoubleProperty(kOfxImageEffectPropRenderScale, scale.y, 1);

    StorageModeEnum storage = internalImage->getStorageMode();
    if (storage == eStorageModeGLTex) {
        GLImageStoragePtr texture = internalImage->getGLImageStorage();
        assert(texture);
        if (!texture) {
            throw std::bad_alloc();
        }
        ofxImageBase->setIntProperty( kOfxImageEffectPropOpenGLTextureTarget, texture->getGLTextureTarget() );
        ofxImageBase->setIntProperty( kOfxImageEffectPropOpenGLTextureIndex, texture->getGLTextureID() );
    }


    const int dataSizeOf = getSizeOfForBitDepth( internalImage->getBitDepth() );


    const RectI bounds = internalImage->getBounds();
    RectI pluginsSeenBounds;
    renderWindow.intersect(bounds, &pluginsSeenBounds);

    // OpenFX only supports RGBA packed buffers
    assert(internalImage->getBufferFormat() == eImageBufferLayoutRGBAPackedFullRect);
    const std::size_t srcRowSize = bounds.width() * nComps  * dataSizeOf;

    // row bytes
    ofxImageBase->setIntProperty(kOfxImagePropRowBytes, srcRowSize);

    if (storage == eStorageModeRAM ||
        storage == eStorageModeDisk) {


        assert(internalImage->getNumTiles() == 1);

        Image::Tile tile;
        internalImage->getTileAt(0, &tile);
        Image::CPUTileData tileData;
        internalImage->getCPUTileData(tile, &tileData);


        const unsigned char* ptr = Image::pixelAtStatic(pluginsSeenBounds.x1, pluginsSeenBounds.y1, tile.tileBounds, nComps, dataSizeOf, (const unsigned char*)tileData.ptrs[0]);

        assert(ptr);
        ofxImageBase->setPointerProperty( kOfxImagePropData, const_cast<unsigned char*>(ptr) );

    }


    // Do not activate this assert! The render window passed to renderRoI can be bigger than the actual RoD of the effect
    // in which case it is just clipped to the RoD.
    // assert(bounds.contains(renderWindow));

    // We set the render window that was given to the render thread instead of the actual bounds of the image
    // so we're sure the plug-in doesn't attempt to access outside pixels.
    ofxImageBase->setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.left(), 0);
    ofxImageBase->setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.bottom(), 1);
    ofxImageBase->setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.right(), 2);
    ofxImageBase->setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.top(), 3);

    // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImagePropRegionOfDefinition
    // " An image's region of definition, in *PixelCoordinates,* is the full frame area of the image plane that the image covers."
    // Image::getRoD() is in *CANONICAL* coordinates
    // OFX::Image RoD is in *PIXEL* coordinates
    RectI pixelRod;
    rod.toPixelEnclosing(mipMapLevel, par, &pixelRod);
    ofxImageBase->setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.left(), 0);
    ofxImageBase->setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.bottom(), 1);
    ofxImageBase->setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.right(), 2);
    ofxImageBase->setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.top(), 3);

    //pluginsSeenBounds must be contained in pixelRod
    assert( pluginsSeenBounds.left() >= pixelRod.left() && pluginsSeenBounds.right() <= pixelRod.right() &&
            pluginsSeenBounds.bottom() >= pixelRod.bottom() && pluginsSeenBounds.top() <= pixelRod.top() );


    ofxImageBase->setStringProperty( kOfxImageEffectPropComponents, components);
    ofxImageBase->setStringProperty( kOfxImageEffectPropPixelDepth, OfxClipInstance::natronsDepthToOfxDepth( internalImage->getBitDepth() ) );
    ofxImageBase->setStringProperty( kOfxImageEffectPropPreMultiplication, OfxClipInstance::natronsPremultToOfxPremult(premult) );
    ofxImageBase->setStringProperty( kOfxImagePropField, OfxClipInstance::natronsFieldingToOfxFielding(fielding) );
    ofxImageBase->setStringProperty( kOfxImagePropUniqueIdentifier, QString::number(nodeFrameViewHash, 16).toStdString() );
    ofxImageBase->setDoubleProperty( kOfxImagePropPixelAspectRatio, par );

    // Attach the transform matrix if any
    if (distortion) {

        bool supportsDeprecatedTransforms = outputClipEffect->getInputCanReceiveTransform(inputNb);
        bool supportsDistortion = outputClipEffect->getInputCanReceiveDistortion(inputNb);

        assert((supportsDeprecatedTransforms && !supportsDistortion) || (!supportsDeprecatedTransforms && supportsDistortion));

        const std::list<DistortionFunction2DPtr>& stack = distortion->getStack();

        if (supportsDeprecatedTransforms) {
            boost::shared_ptr<Transform::Matrix3x3> mat;
            if (stack.size() == 1) {
                const DistortionFunction2DPtr& func = stack.front();
                if (func->transformMatrix) {
                    mat = func->transformMatrix;
                }
            }
            if (mat) {
                ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->a, 0);
                ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->b, 1);
                ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->c, 2);

                ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->d, 3);
                ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->e, 4);
                ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->f, 5);

                ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->g, 6);
                ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->h, 7);
                ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->i, 8);
            } else {
                for (int i = 0; i < 9; ++i) {
                    ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, 0., i);
                }
            }
        } else if (supportsDistortion) {
            ofxImageBase->setPointerProperty(kOfxPropDistortionFunction, (void*)&ofxaApplyDistortionStack);
            ofxImageBase->setPointerProperty(kOfxPropDistortionFunctionData, (void*)distortion.get());
        }





    }
}
OfxImageCommon::~OfxImageCommon()
{
}

int
OfxClipInstance::getInputNb() const
{
    if (_isOutput) {
        return -1;
    }

    return _imp->nodeInstance.lock()->getClipInputNumber(this);
}

EffectInstancePtr
OfxClipInstance::getEffectHolder() const
{
    OfxEffectInstancePtr effect = _imp->nodeInstance.lock();

    if (!effect) {
        return effect;
    }
    if ( effect->isReader() ) {
        NodePtr ioContainer = effect->getNode()->getIOContainer();
        if (ioContainer) {
            return ioContainer->getEffectInstance();
        }
    }
    return effect;
}

EffectInstancePtr
OfxClipInstance::getAssociatedNode() const
{
    EffectInstancePtr effect = getEffectHolder();

    assert(effect);
    if (_isOutput) {
        return effect;
    } else {
        return effect->getInput( getInputNb() );
    }
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
    if ( isSupportedComponent(s) ) {
        return s;
    }

    if (s == xy) {
        if ( isSupportedComponent(motion) ) {
            return motion;
        } else if ( isSupportedComponent(disparity) ) {
            return disparity;
        } else if ( isSupportedComponent(rgb) ) {
            return rgb;
        } else if ( isSupportedComponent(rgba) ) {
            return rgba;
        } else if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    } else if (s == rgba) {
        if ( isSupportedComponent(rgb) ) {
            return rgb;
        }
        if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    } else if (s == alpha) {
        if ( isSupportedComponent(rgba) ) {
            return rgba;
        }
        if ( isSupportedComponent(rgb) ) {
            return rgb;
        }
    } else if (s == rgb) {
        if ( isSupportedComponent(rgba) ) {
            return rgba;
        }
        if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    } else if (s == motion) {
        if ( isSupportedComponent(xy) ) {
            return xy;
        } else if ( isSupportedComponent(disparity) ) {
            return disparity;
        } else if ( isSupportedComponent(rgb) ) {
            return rgb;
        } else if ( isSupportedComponent(rgba) ) {
            return rgba;
        } else if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    } else if (s == disparity) {
        if ( isSupportedComponent(xy) ) {
            return xy;
        } else if ( isSupportedComponent(disparity) ) {
            return disparity;
        } else if ( isSupportedComponent(rgb) ) {
            return rgb;
        } else if ( isSupportedComponent(rgba) ) {
            return rgba;
        } else if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    }

    /// wierd, must be some custom bit , if only one, choose that, otherwise no idea
    /// how to map, you need to derive to do so.
    const std::vector<std::string> &supportedComps = getSupportedComponents();
    if (supportedComps.size() == 1) {
        return supportedComps[0];
    }

    return none;
} // OfxClipInstance::findSupportedComp

NATRON_NAMESPACE_EXIT;

