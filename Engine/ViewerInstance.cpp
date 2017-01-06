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

#include "ViewerInstance.h"

#include <algorithm> // min, max
#include <stdexcept>
#include <cassert>
#include <cstring> // for std::memcpy
#include <cfloat> // DBL_MAX



#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/MultiThread.h"
#include "Engine/RamBuffer.h"
#include "Engine/Settings.h"
#include "Engine/ViewerNode.h"

#define GAMMA_LUT_NB_VALUES 1023


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif


NATRON_NAMESPACE_ENTER;


NATRON_NAMESPACE_ANONYMOUS_ENTER

struct MinMaxVal {
    MinMaxVal(double min_, double max_)
    : min(min_)
    , max(max_)
    {
    }
    MinMaxVal()
    : min(DBL_MAX)
    , max(-DBL_MAX)
    {
    }

    double min;
    double max;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


/**
 *@brief Actually converting to ARGB... but it is called BGRA by
   the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static unsigned int toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

struct ViewerInstancePrivate
{

    ViewerInstance* _publicInterface;

    // The layer to render for RGB
    KnobChoiceWPtr layerChoiceKnob;

    // The layer to use in input for the alpha channel
    KnobChoiceWPtr alphaChannelChoiceKnob;

    // What to output
    KnobChoiceWPtr displayChannels;

    KnobDoubleWPtr gammaKnob;
    KnobDoubleWPtr gainKnob;
    KnobBoolWPtr autoContrastKnob;


    bool viewerChannelsAutoswitchedToAlpha;


    ViewerInstancePrivate(ViewerInstance* publicInterface)
    : _publicInterface(publicInterface)
    {

    }


    void buildGammaLut(double gamma, RamBuffer<float>* gammaLookup);

    float lookupGammaLut(float value, const float* gammaLookupBuffer) const;
    
};


const Color::Lut*
ViewerInstance::lutFromColorspace(ViewerColorSpaceEnum cs)
{
    const Color::Lut* lut;

    switch (cs) {
    case eViewerColorSpaceSRGB:
        lut = Color::LutManager::sRGBLut();
        break;
    case eViewerColorSpaceRec709:
        lut = Color::LutManager::Rec709Lut();
        break;
    case eViewerColorSpaceLinear:
    default:
        lut = 0;
        break;
    }
    if (lut) {
        lut->validate();
    }

    return lut;
}

EffectInstancePtr
ViewerInstance::create(const NodePtr& node)
{
    return EffectInstancePtr( new ViewerInstance(node) );
}

PluginPtr
ViewerInstance::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_IMAGE);
    PluginPtr ret = Plugin::create((void*)ViewerInstance::create, PLUGINID_NATRON_VIEWER_INTERNAL, "ViewerProcess", 1, 0, grouping);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/viewer_icon.png");
    QString desc =  tr("The Viewer node can display the output of a node graph.");
    ret->setProperty<bool>(kNatronPluginPropIsInternalOnly, true);
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    return ret;
}

ViewerInstance::ViewerInstance(const NodePtr& node)
    : EffectInstance(node)
    , _imp( new ViewerInstancePrivate(this) )
{

}

ViewerInstance::~ViewerInstance()
{
}

ViewerNodePtr
ViewerInstance::getViewerNodeGroup() const
{
    NodeCollectionPtr collection = getNode()->getGroup();
    assert(collection);
    NodeGroupPtr isGroup = toNodeGroup(collection);
    assert(isGroup);
    ViewerNodePtr ret = toViewerNode(isGroup);
    assert(ret);
    return ret;
}



std::string
ViewerInstance::getInputLabel(int /*inputNb*/) const
{
    return "Source";
}

int
ViewerInstance::getMaxInputCount() const
{
    return 1;
}

void
ViewerInstancePrivate::buildGammaLut(double gamma, RamBuffer<float>* gammaLookup)
{
    // gammaLookupMutex should already be locked
    gammaLookup->resize(GAMMA_LUT_NB_VALUES + 1);
    float* buf = gammaLookup->getData();
    if (gamma <= 0) {
        // gamma = 0: everything is zero, except gamma(1)=1
        memset(buf, 0, sizeof(float) * GAMMA_LUT_NB_VALUES);
        buf[GAMMA_LUT_NB_VALUES] = 1.f;
        return;
    }
    for (int position = 0; position <= GAMMA_LUT_NB_VALUES; ++position) {
        double parametricPos = double(position) / GAMMA_LUT_NB_VALUES;
        double value = std::pow(parametricPos, 1. / gamma);
        // set that in the lut
        buf[position] = (float)std::max( 0., std::min(1., value) );
    }
}

float
ViewerInstancePrivate::lookupGammaLut(float value, const float* gammaLookupBuffer) const
{
    if (value < 0.) {
        return 0.;
    } else if (value > 1.) {
        return 1.;
    } else {
        int i = (int)(value * GAMMA_LUT_NB_VALUES);
        assert(0 <= i && i <= GAMMA_LUT_NB_VALUES);
        float alpha = std::max( 0.f, std::min(value * GAMMA_LUT_NB_VALUES - i, 1.f) );
        float a = gammaLookupBuffer[i];
        float b = (i  < GAMMA_LUT_NB_VALUES) ? gammaLookupBuffer[i + 1] : 0.f;

        return a * (1.f - alpha) + b * alpha;
    }
}


bool
ViewerInstance::isMultiPlanar() const
{
    // We are multi-planar: we need to process multiple planes (The one selected in alpha + the output layer)
    return true;
}

EffectInstance::PassThroughEnum
ViewerInstance::isPassThroughForNonRenderedPlanes() const
{
    return ePassThroughPassThroughNonRenderedPlanes;
}

ActionRetCodeEnum
ViewerInstance::isIdentity(TimeValue time,
                           const RenderScale & scale,
                           const RectI & roi,
                           ViewIdx view,
                           const TreeRenderNodeArgsPtr& render,
                           TimeValue* inputTime,
                           ViewIdx* inputView,
                           int* inputNb)
{
    
} // isIdentity


ActionRetCodeEnum
ViewerInstance::getTimeInvariantMetaDatas(NodeMetadata& /*metadata*/)
{
    ViewerNodePtr node = getViewerNodeGroup();
    node->refreshFps();
    node->refreshViewsKnobVisibility();
    refreshLayerAndAlphaChannelComboBox();
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        uiContext->refreshFormatFromMetadata();
    }
} // getTimeInvariantMetadatas

ActionRetCodeEnum
ViewerInstance::getComponentsNeededInternal(TimeValue time,
                                            ViewIdx view,
                                            const TreeRenderNodeArgsPtr& render,
                                            std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                            std::list<ImageComponents>* layersProduced,
                                            TimeValue* passThroughTime,
                                            ViewIdx* passThroughView,
                                            int* passThroughInputNb,
                                            bool* processAll,
                                            std::bitset<4>* processChannels)
{

    return eActionStatusOK;
} // getComponentsNeededInternal


ActionRetCodeEnum
ViewerInstance::render(const RenderActionArgs& args)
{

} // render



void
ViewerInstance::refreshLayerAndAlphaChannelComboBox()
{
    ViewerNodePtr viewerGroup = getViewerNodeGroup();

    KnobChoicePtr layerKnob = _imp->layerChoiceKnob.lock();
    KnobChoicePtr alphaChannelKnob = _imp->alphaChannelChoiceKnob.lock();

    // Remember the current choices
    std::string layerCurChoice = layerKnob->getActiveEntryText(ViewIdx(0));
    std::string alphaCurChoice = alphaChannelKnob->getActiveEntryText(ViewIdx(0));

    std::list<ImageComponents> upstreamAvailableLayers;

    {
        const int passThroughPlanesInputNb = 0;
        ActionRetCodeEnum stat = getAvailableInputLayers(getCurrentTime_TLS(), ViewIdx(0), passThroughPlanesInputNb, TreeRenderNodeArgsPtr(), &upstreamAvailableLayers);
        (void)stat;
    }

    std::vector<std::string> layerOptions;
    std::vector<std::string> channelOptions;
    
    // Append none chocie
    layerOptions.push_back("-");
    channelOptions.push_back("-");

    // For each components, try to find the old user choice, if not found prefer color layer
    std::list<ImageComponents>::iterator foundOldLayer = upstreamAvailableLayers.end();
    std::list<ImageComponents>::iterator foundOldAlphaLayer = upstreamAvailableLayers.end();



    // This is the not the full channel name (e.g: Beauty.A), but just the channel name (e.g: A)
    std::string foundAlphaChannel;

    int foundLayerIndex = -1;
    int foundColorIndex = -1;
    int foundOtherIndex = -1;
    int foundAlphaIndex = -1;
    int foundFirstColorChannelIndex = -1;
    for (std::set<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
        const std::string& layerName = it->getLayerName();
        std::string itemName = layerName + '.' + it->getComponentsGlobalName();
        layerOptions.push_back(itemName);

        if (itemName == layerCurChoice) {
            // We found the user layer choice in the new layers set
            foundCurIt = it;
            foundLayerIndex = layerOptions.size() - 1;
        }

        bool isColorPlane = it->isColorPlane();
        if (isColorPlane) {
            // Ok we found the color plane
            foundColorIt = it;
            foundColorIndex = layerOptions.size() - 1;
            foundFirstColorChannelIndex = channelOptions.size();
        } else {
            // This is the first layer we found that is not the color plane
            // This is our last resort fallback
            foundOtherIt = it;
            foundOtherIndex = layerOptions.size() - 1;
        }

        const std::vector<std::string>& channels = it->getComponentsNames();
        for (U32 i = 0; i < channels.size(); ++i) {
            std::string channelName = layerName + '.' + channels[i];
            channelOptions.push_back(channelName);
            if (channelName == alphaCurChoice) {
                foundCurAlphaIt = it;
                foundAlphaChannel = channels[i];
                foundAlphaIndex = channelOptions.size() - 1;
            }
        }

    }

    layerKnob->populateChoices(layerOptions);
    alphaChannelKnob->populateChoices(channelOptions);

    // Check if we found the old user choice
    if ( ( layerCurChoice == "-" ) || layerCurChoice.empty() || ( foundCurIt == components.end() ) ) {
        // The user did not have a current choice or we could not find it's choice
        // Try to find color plane, otherwise fallback on any other layer
        if ( foundColorIt != components.end() ) {
            layerCurChoice = foundColorIt->getLayerName() + '.' + foundColorIt->getComponentsGlobalName();
            foundCurIt = foundColorIt;
            foundLayerIndex = foundColorIndex;
        } else if ( foundOtherIt != components.end() ) {
            layerCurChoice = foundOtherIt->getLayerName() + '.' + foundOtherIt->getComponentsGlobalName();
            foundCurIt = foundOtherIt;
            foundLayerIndex = foundOtherIndex;
        } else {
            layerCurChoice = "-";
            foundLayerIndex = 0;
            foundCurIt = components.end();
        }
    }

    // Validate current choice on the knob and in the viewer params
    assert(foundLayerIndex != -1);
    layerKnob->setValue(foundLayerIndex, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        _imp->viewerParamsLayer = (foundCurIt == components.end()) ? ImageComponents::getNoneComponents() : *foundCurIt;
    }

    // Check if we found the old user choice
    if ( ( alphaCurChoice == "-" ) || alphaCurChoice.empty() || ( foundCurAlphaIt == components.end() ) ) {
        // Try to find color plane, otherwise fallback on any other layer
        if ( ( foundColorIt != components.end() ) &&
            ( ( foundColorIt->getComponentsNames().size() == 4) || ( foundColorIt->getComponentsNames().size() == 1) ) ) {
            std::size_t lastComp = foundColorIt->getComponentsNames().size() - 1;
            alphaCurChoice = foundColorIt->getLayerName() + '.' + foundColorIt->getComponentsNames()[lastComp];
            foundAlphaChannel = foundColorIt->getComponentsNames()[lastComp];
            foundAlphaIndex = foundFirstColorChannelIndex + foundColorIt->getComponentsNames().size() - 1;
            foundCurAlphaIt = foundColorIt;
        } else {
            alphaCurChoice = "-";
            foundAlphaIndex = 0;
            foundCurAlphaIt = components.end();
        }
    }

    // Validate current choice on the knob and in the viewer params
    assert(foundAlphaIndex != -1);
    alphaChannelKnob->setValue(foundAlphaIndex, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        _imp->viewerParamsAlphaLayer = (foundCurAlphaIt == components.end() || foundAlphaChannel.empty()) ? ImageComponents::getNoneComponents() : *foundCurAlphaIt;

        // This is the not the full channel name (e.g: Beauty.A), but just the channel name (e.g: A)
        // If we selected "-", set it empty
        _imp->viewerParamsAlphaChannelName = (foundCurAlphaIt == components.end()) ? std::string() : foundAlphaChannel;
    }


    // Adjust display channels automatically
    if ( foundCurIt != components.end() ) {
        if (foundCurIt->getNumComponents() == 1) {
            // Switch auto to alpha if there's only this to view
            viewerGroup->setDisplayChannels((int)eDisplayChannelsA, true);
            _imp->viewerChannelsAutoswitchedToAlpha = true;
        } else {
            // Switch back to RGB if we auto-switched to alpha
            DisplayChannelsEnum curDisplayChannels = (DisplayChannelsEnum)viewerGroup->getDisplayChannels(0);
            if ( _imp->viewerChannelsAutoswitchedToAlpha && (foundCurIt->getNumComponents() > 1) && (curDisplayChannels == eDisplayChannelsA) ) {
                viewerGroup->setDisplayChannels((int)eDisplayChannelsRGB, true);
            }
        }
    }

} // refreshLayerAndAlphaChannelComboBox



void
ViewerInstance::fillGammaLut(double gamma)
{
    QWriteLocker k(&_imp->gammaLookupMutex);
    _imp->fillGammaLut(gamma);
}



ViewerInstance::ViewerRenderRetCode
ViewerInstance::renderViewer_internal(ViewIdx view,
                                      bool singleThreaded,
                                      bool isSequentialRender,
                                      const RotoStrokeItemPtr& activeStrokeItem,
                                      const boost::shared_ptr<ViewerCurrentFrameRequestSchedulerStartArgs>& request,
                                      const RenderStatsPtr& stats,
                                      ViewerArgs& inArgs)
{

    const double par = inArgs.activeInputToRender->getAspectRatio(-1);


    //We are going to render a non cached frame and not in playback, clear persistent messages
    if (!inArgs.params->isSequential) {
        clearPersistentMessage(true);
    }

    ImageComponents components = inArgs.activeInputToRender->getComponents(-1);
    ImageBitDepthEnum imageDepth = inArgs.activeInputToRender->getBitDepth(-1);
    std::list<ImageComponents> requestedComponents;
    int alphaChannelIndex = -1;
    if ( (inArgs.channels != eDisplayChannelsA) &&
         ( inArgs.channels != eDisplayChannelsMatte) ) {
        ///We fetch the Layer specified in the gui
        if (inArgs.params->layer.getNumComponents() > 0) {
            requestedComponents.push_back(inArgs.params->layer);
        }
    } else {
        ///We fetch the alpha layer
        if ( !inArgs.params->alphaChannelName.empty() ) {
            requestedComponents.push_back(inArgs.params->alphaLayer);
            const std::vector<std::string>& channels = inArgs.params->alphaLayer.getComponentsNames();
            for (std::size_t i = 0; i < channels.size(); ++i) {
                if (channels[i] == inArgs.params->alphaChannelName) {
                    alphaChannelIndex = i;
                    break;
                }
            }
            assert(alphaChannelIndex != -1);
        }
        if (inArgs.channels == eDisplayChannelsMatte) {
            //For the matte overlay also display the alpha mask on top of the red channel of the image
            if ( (inArgs.params->layer.getNumComponents() > 0) && (inArgs.params->layer != inArgs.params->alphaLayer) ) {
                requestedComponents.push_back(inArgs.params->layer);
            }
        }
    }

    if ( requestedComponents.empty() ) {
        return eViewerRenderRetCodeBlack;
    }


    BufferableObjectList partialUpdateObjects;
    for (std::size_t rectIndex = 0; rectIndex < splitRoi.size(); ++rectIndex) {
        //AlphaImage will only be set when displaying the Matte overlay
        ImagePtr alphaImage, colorImage;


        // Don't uncomment: if the image was rendered so far, render it to the display texture so that the user get some feedback
        /*if ( !_imp->checkAgeNoUpdate( inArgs.params->textureIndex, inArgs.params->abortInfo->getRenderAge() ) ) {
            return eViewerRenderRetCodeRedraw;
        }

        if ( inArgs.activeInputToRender->aborted() ) {
            return eViewerRenderRetCodeRedraw;
        }*/

        const bool viewerRenderRoiOnly = !inArgs.useViewerCache;
        ViewerColorSpaceEnum srcColorSpace = getApp()->getDefaultColorSpaceForBitDepth( colorImage->getBitDepth() );

        if ( ( (inArgs.channels == eDisplayChannelsA) && ( (alphaChannelIndex < 0) || ( alphaChannelIndex >= (int)colorImage->getComponentsCount() ) ) ) ||
            ( ( inArgs.channels == eDisplayChannelsMatte) && ( ( alphaChannelIndex < 0) || ( alphaChannelIndex >= (int)alphaImage->getComponentsCount() ) ) ) ) {
            return eViewerRenderRetCodeBlack;
        }


        assert( ( inArgs.channels != eDisplayChannelsMatte && alphaChannelIndex < (int)colorImage->getComponentsCount() ) ||
               ( inArgs.channels == eDisplayChannelsMatte && ( ( alphaImage && alphaChannelIndex < (int)alphaImage->getComponentsCount() ) || !alphaImage ) ) );


        //Make sure the viewer does not render something outside the bounds
        RectI viewerRenderRoI;
        splitRoi[rectIndex].intersect(colorImage->getBounds(), &viewerRenderRoI);


        boost::shared_ptr<UpdateViewerParams> updateParams;
        if (!inArgs.isDoingPartialUpdates) {
            updateParams = inArgs.params;
        } else {
            updateParams.reset( new UpdateViewerParams(*inArgs.params) );
            updateParams->mustFreeRamBuffer = true;
            updateParams->isPartialRect = true;
            UpdateViewerParams::CachedTile tile;
            tile.rect.set(viewerRenderRoI);
            tile.rectRounded = viewerRenderRoI;
            std::size_t pixelSize = 4;
            if (updateParams->depth == eImageBitDepthFloat) {
                pixelSize *= sizeof(float);
            }
            std::size_t dstRowSize = tile.rect.width() * pixelSize;
            tile.bytesCount = tile.rect.height() * dstRowSize;
            tile.ramBuffer =  (unsigned char*)malloc(tile.bytesCount);
            updateParams->tiles.clear();
            updateParams->tiles.push_back(tile);
        }

        // Allocate the texture on the CPU to apply the render viewer process
        FrameEntryLocker entryLocker( _imp.get() );
        RectI lastPaintBboxPixel;
        std::list<UpdateViewerParams::CachedTile> unCachedTiles;
        if (!inArgs.useViewerCache) {
            assert(updateParams->tiles.size() == 1);

            UpdateViewerParams::CachedTile& tile = updateParams->tiles.front();


            // If we are tracking, only update the partial rectangles
            if (inArgs.isDoingPartialUpdates) {
                QMutexLocker k(&_imp->viewerParamsMutex);
                updateParams->recenterViewport = _imp->viewportCenterSet;
                updateParams->viewportCenter = _imp->viewportCenter;
                unCachedTiles.push_back(tile);
            }
            // If we are actively painting, re-use the last texture instead of re-drawing everything
            else if (activeStrokeItem) {
                /*
                   Check if we have a last valid texture and re-use the old texture only if the new texture is at least as big
                   as the old texture.
                 */
                QMutexLocker k(&_imp->lastRenderParamsMutex);
                assert(!_imp->lastRenderParams[updateParams->textureIndex] || _imp->lastRenderParams[updateParams->textureIndex]->tiles.size() == 1);


                bool canUseOldTex = _imp->lastRenderParams[updateParams->textureIndex] &&
                                    updateParams->mipMapLevel == _imp->lastRenderParams[updateParams->textureIndex]->mipMapLevel &&
                                    tile.rect.contains(_imp->lastRenderParams[updateParams->textureIndex]->tiles.front().rect);

                if (!canUseOldTex) {
                    //The old texture did not exist or was not usable, just make a new buffer
                    updateParams->mustFreeRamBuffer = true;
                    tile.ramBuffer =  (unsigned char*)malloc(tile.bytesCount);
                    unCachedTiles.push_back(tile);
                } else {

                    // Overwrite the RoI to only the last portion rendered
                    // Read the stroke portion bbox on the render clone not on the "main" object which may have changed!
                    RotoStrokeItemPtr strokeRenderClone = toRotoStrokeItem(activeStrokeItem->getCachedDrawable(inArgs.params->abortInfo));
                    assert(strokeRenderClone);
                    RectD lastPaintBbox = strokeRenderClone->getLastStrokeMovementBbox();

                    lastPaintBbox.toPixelEnclosing(updateParams->mipMapLevel, par, &lastPaintBboxPixel);


                    //The last buffer must be valid
                    const UpdateViewerParams::CachedTile& lastTile = _imp->lastRenderParams[updateParams->textureIndex]->tiles.front();
                    assert(lastTile.ramBuffer);
                    tile.ramBuffer =  0;

                    //If the old buffer size is the same as the requested texture size, this function does nothing
                    //otherwise it allocates a new buffer and copies the content of the old one.
                    bool mustFreeSource = copyAndSwap(lastTile.rect, tile.rect, tile.bytesCount, updateParams->depth, lastTile.ramBuffer, &tile.ramBuffer);

                    if (mustFreeSource) {
                        //A new buffer was allocated and copied the previous content, free the old texture
                        _imp->lastRenderParams[updateParams->textureIndex]->mustFreeRamBuffer = true;
                    } else {
                        //The buffer did not change its size, make sure to keep it
                        _imp->lastRenderParams[updateParams->textureIndex]->mustFreeRamBuffer = false;
                    }

                    //This will delete the previous buffer if mustFreeRamBuffer was set to true
                    _imp->lastRenderParams[updateParams->textureIndex].reset();

                    //Allocation may have failed
                    if (!tile.ramBuffer) {
                        return eViewerRenderRetCodeFail;
                    }

                    unCachedTiles.push_back(tile);
                }
                _imp->lastRenderParams[updateParams->textureIndex] = updateParams;
            }
            // Make a new buffer that will be freed when the params get destroyed
            else {
                assert(updateParams->tiles.size() == 1);

                updateParams->mustFreeRamBuffer = true;
                tile.ramBuffer =  (unsigned char*)malloc(tile.bytesCount);
                unCachedTiles.push_back(tile);
            }
        } else { // useViewerCache
            //Look up the cache for a texture or create one
            {
                QMutexLocker k(&_imp->lastRenderParamsMutex);
                _imp->lastRenderParams[updateParams->textureIndex].reset();
            }

            // For the viewer, we need the enclosing rectangle to avoid black borders.
            // Do this here to avoid infinity values.

            RectI bounds;
            updateParams->rod.toPixelEnclosing(updateParams->mipMapLevel, updateParams->pixelAspectRatio, &bounds);

            RectI tileBounds;
            tileBounds.x1 = tileBounds.y1 = 0;
            tileBounds.x2 = tileBounds.y2 = inArgs.params->tileSize;
            assert(!tileBounds.isNull());
            if (tileBounds.isNull()) {
                return eViewerRenderRetCodeRedraw;
            }
            std::string inputToRenderName = inArgs.activeInputToRender->getNode()->getScriptName_mt_safe();
            for (std::list<UpdateViewerParams::CachedTile>::iterator it = updateParams->tiles.begin(); it != updateParams->tiles.end(); ++it) {
                if (it->isCached) {
                    assert(it->ramBuffer);
                } else {
                    assert(!it->ramBuffer);


                    FrameKey key(inArgs.params->time,
                                 inArgs.params->view,
                                 updateParams->frameViewHash,
                                 (int)inArgs.params->depth,
                                 it->rect,
                                 inArgs.params->depth == eImageBitDepthFloat, // use shaders,
                                 inArgs.draftModeEnabled);



                    boost::shared_ptr<FrameParams> cachedFrameParams( new FrameParams(bounds , key.getBitDepth(), tileBounds, ImagePtr() ) );
                    bool cached = appPTR->getTextureOrCreate(key, cachedFrameParams, &entryLocker, &it->cachedData);
                    if (!it->cachedData) {
                        std::size_t size = cachedFrameParams->getStorageInfo().numComponents * cachedFrameParams->getStorageInfo().dataTypeSize * cachedFrameParams->getStorageInfo().bounds.area();
                        QString s = tr("Failed to allocate a texture of %1.").arg( printAsRAM(size) );
                        Dialogs::errorDialog( tr("Out of memory").toStdString(), s.toStdString() );

                        return eViewerRenderRetCodeFail;
                    }

                    ///The entry has already been locked by the cache
                    if (!cached) {
                        it->cachedData->allocateMemory();
                    } else {
                        // If the tile is cached and we got it that means rendering is done
                        entryLocker.lock(it->cachedData);
                        it->ramBuffer = it->cachedData->data();
                        it->isCached = true;
                        continue;
                    }


                    assert(it->cachedData);
                    // how do you make sure cachedFrame->data() is not freed after this line?
                    ///It is not freed as long as the cachedFrame shared_ptr has a used_count greater than 1.
                    ///Since it is used during the whole function scope it is guaranteed not to be freed before
                    ///The viewer is actually done with it.
                    /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
                    it->ramBuffer = it->cachedData->data();
                    assert(it->ramBuffer);
                    unCachedTiles.push_back(*it);
                } // !it->isCached

                if (it->cachedData) {
                    it->cachedData->setInternalImage(colorImage);
                }
            }
        } // !useViewerCache

        /*
           If we are not using the texture cache, there is a unique tile which has the required texture size on the viewer, so we render the RoI instead
           of just the tile portion in the render function
         */


        //If we are painting, only render the portion needed
        if ( !lastPaintBboxPixel.isNull() ) {
            lastPaintBboxPixel.intersect(viewerRenderRoI, &viewerRenderRoI);
        }

        TimeLapsePtr viewerRenderTimeRecorder;
        if (stats) {
            viewerRenderTimeRecorder.reset( new TimeLapse() );
        }

        std::size_t tileRowElements = inArgs.params->tileSize;
        // Internally the buffer is interpreted as U32 when 8bit, so we do not multiply it by 4 for RGBA
        if (updateParams->depth == eImageBitDepthFloat) {
            tileRowElements *= 4;
        }

        if (singleThreaded) {
            if (inArgs.autoContrast && !inArgs.isDoingPartialUpdates) {
                double vmin, vmax;
                MinMaxVal vMinMax = findAutoContrastVminVmax(colorImage, inArgs.channels, viewerRenderRoI);
                vmin = vMinMax.min;
                vmax = vMinMax.max;

                ///if vmax - vmin is greater than 1 the gain will be really small and we won't see
                ///anything in the image
                if (vmin == vmax) {
                    vmin = vmax - 1.;
                }
                updateParams->gain = 1 / (vmax - vmin);
                updateParams->offset = -vmin / ( vmax - vmin);
            }

            const RenderViewerArgs args(colorImage,
                                        alphaImage,
                                        inArgs.channels,
                                        updateParams->srcPremult,
                                        updateParams->depth,
                                        updateParams->gain,
                                        updateParams->gamma,
                                        updateParams->offset,
                                        lutFromColorspace(srcColorSpace),
                                        lutFromColorspace(updateParams->lut),
                                        alphaChannelIndex,
                                        viewerRenderRoiOnly,
                                        tileRowElements);
            QReadLocker k(&_imp->gammaLookupMutex);
            for (std::list<UpdateViewerParams::CachedTile>::iterator it = unCachedTiles.begin(); it != unCachedTiles.end(); ++it) {
                renderFunctor(viewerRenderRoI,
                              args,
                              shared_from_this(),
                              *it);
            }
        } else {
            bool runInCurrentThread = QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount();
            if ( !runInCurrentThread && (splitRoi.size() > 1) ) {
                runInCurrentThread = true;
            }


            ///if autoContrast is enabled, find out the vmin/vmax before rendering and mapping against new values
            if (inArgs.autoContrast && !inArgs.isDoingPartialUpdates) {
                double vmin = std::numeric_limits<double>::infinity();
                double vmax = -std::numeric_limits<double>::infinity();

                if (runInCurrentThread) {
                    MinMaxVal vMinMax = findAutoContrastVminVmax(colorImage, inArgs.channels, viewerRenderRoI);
                    vmin = vMinMax.min;
                    vmax = vMinMax.max;
                } else {
                    std::vector<RectI> splitRects = viewerRenderRoI.splitIntoSmallerRects( appPTR->getHardwareIdealThreadCount() );
                    QFuture<MinMaxVal> future = QtConcurrent::mapped( splitRects,
                                                                                       boost::bind(findAutoContrastVminVmax,
                                                                                                   colorImage,
                                                                                                   inArgs.channels,
                                                                                                   _1) );
                    future.waitForFinished();
                    QList<MinMaxVal> results = future.results();
                    Q_FOREACH (const MinMaxVal &vMinMax, results) {
                        if (vMinMax.min < vmin) {
                            vmin = vMinMax.min;
                        }
                        if (vMinMax.max > vmax) {
                            vmax = vMinMax.max;
                        }
                    }
                } // runInCurrentThread

                if (vmax == vmin) {
                    vmin = vmax - 1.;
                }

                if (vmax <= 0) {
                    updateParams->gain = 0;
                    updateParams->offset = 0;
                } else {
                    updateParams->gain = 1 / (vmax - vmin);
                    updateParams->offset =  -vmin / (vmax - vmin);
                }
            }

            const RenderViewerArgs args(colorImage,
                                        alphaImage,
                                        inArgs.channels,
                                        updateParams->srcPremult,
                                        updateParams->depth,
                                        updateParams->gain,
                                        updateParams->gamma,
                                        updateParams->offset,
                                        lutFromColorspace(srcColorSpace),
                                        lutFromColorspace(updateParams->lut),
                                        alphaChannelIndex,
                                        viewerRenderRoiOnly,
                                        tileRowElements);

            if (runInCurrentThread) {
                QReadLocker k(&_imp->gammaLookupMutex);
                for (std::list<UpdateViewerParams::CachedTile>::iterator it = unCachedTiles.begin(); it != unCachedTiles.end(); ++it) {
                    renderFunctor(viewerRenderRoI,
                                  args, shared_from_this(), *it);
                }
            } else {
                QReadLocker k(&_imp->gammaLookupMutex);
                QtConcurrent::map( unCachedTiles,
                                   boost::bind(&renderFunctor,
                                               viewerRenderRoI,
                                               args,
                                               shared_from_this(),
                                               _1) ).waitForFinished();
            }

            if (inArgs.isDoingPartialUpdates) {
                partialUpdateObjects.push_back(updateParams);
            }
        } // if (singleThreaded)


        if ( stats && stats->isInDepthProfilingEnabled() ) {
            stats->addRenderInfosForNode( getNode(), NodePtr(), colorImage->getComponents().getComponentsGlobalName(), viewerRenderRoI, viewerRenderTimeRecorder->getTimeSinceCreation() );
        }
    } // for (std::vector<RectI>::iterator rect = splitRoi.begin(); rect != splitRoi.end(), ++rect) {


    /*
       If we were rendering only partial rectangles, update them all at once
     */
    if ( !partialUpdateObjects.empty() ) {
        getRenderEngine()->notifyFrameProduced(partialUpdateObjects, stats, request);

        //If we reply eViewerRenderRetCodeRender, it will append another updateViewer for nothing
        return eViewerRenderRetCodeRedraw;
    }

    return eViewerRenderRetCodeRender;
} // renderViewer_internal



void
renderFunctor(const RectI& roi,
              const RenderViewerArgs & args,
              const ViewerInstancePtr& viewer,
              UpdateViewerParams::CachedTile tile)
{
    if ( (args.bitDepth == eImageBitDepthFloat) ) {
        // image is stored as linear, the OpenGL shader with do gamma/sRGB/Rec709 decompression, as well as gain and offset
        scaleToTexture32bits(roi, args, tile, (float*)tile.ramBuffer);
    } else {
        // texture is stored as sRGB/Rec709 compressed 8-bit RGBA
        scaleToTexture8bits(roi, args, viewer, tile, (U32*)tile.ramBuffer);
    }
}

inline
MinMaxVal
findAutoContrastVminVmax_generic(boost::shared_ptr<const Image> inputImage,
                                 int nComps,
                                 DisplayChannelsEnum channels,
                                 const RectI & rect)
{
    double localVmin = std::numeric_limits<double>::infinity();
    double localVmax = -std::numeric_limits<double>::infinity();
    Image::ReadAccess acc = inputImage->getReadRights();

    for (int y = rect.bottom(); y < rect.top(); ++y) {
        const float* src_pixels = (const float*)acc.pixelAt(rect.left(), y);
        ///we fill the scan-line with all the pixels of the input image
        for (int x = rect.left(); x < rect.right(); ++x) {
            double r = 0.;
            double g = 0.;
            double b = 0.;
            double a = 0.;
            switch (nComps) {
            case 4:
                r = src_pixels[0];
                g = src_pixels[1];
                b = src_pixels[2];
                a = src_pixels[3];
                break;
            case 3:
                r = src_pixels[0];
                g = src_pixels[1];
                b = src_pixels[2];
                a = 1.;
                break;
            case 2:
                r = src_pixels[0];
                g = src_pixels[1];
                b = 0.;
                a = 1.;
                break;
            case 1:
                a = src_pixels[0];
                r = g = b = 0.;
                break;
            default:
                r = g = b = a = 0.;
            }

            double mini, maxi;
            switch (channels) {
            case eDisplayChannelsRGB:
                mini = std::min(std::min(r, g), b);
                maxi = std::max(std::max(r, g), b);
                break;
            case eDisplayChannelsY:
                mini = r = 0.299 * r + 0.587 * g + 0.114 * b;
                maxi = mini;
                break;
            case eDisplayChannelsR:
                mini = r;
                maxi = mini;
                break;
            case eDisplayChannelsG:
                mini = g;
                maxi = mini;
                break;
            case eDisplayChannelsB:
                mini = b;
                maxi = mini;
                break;
            case eDisplayChannelsA:
                mini = a;
                maxi = mini;
                break;
            default:
                mini = 0.;
                maxi = 0.;
                break;
            }
            if (mini < localVmin) {
                localVmin = mini;
            }
            if (maxi > localVmax) {
                localVmax = maxi;
            }

            src_pixels += nComps;
        }
    }

    return MinMaxVal(localVmin, localVmax);
} // findAutoContrastVminVmax_generic

template <int nComps>
MinMaxVal
findAutoContrastVminVmax_internal(boost::shared_ptr<const Image> inputImage,
                                  DisplayChannelsEnum channels,
                                  const RectI & rect)
{
    return findAutoContrastVminVmax_generic(inputImage, nComps, channels, rect);
}

MinMaxVal
findAutoContrastVminVmax(boost::shared_ptr<const Image> inputImage,
                         DisplayChannelsEnum channels,
                         const RectI & rect)
{
    int nComps = inputImage->getComponents().getNumComponents();

    if (nComps == 4) {
        return findAutoContrastVminVmax_internal<4>(inputImage, channels, rect);
    } else if (nComps == 3) {
        return findAutoContrastVminVmax_internal<3>(inputImage, channels, rect);
    } else if (nComps == 1) {
        return findAutoContrastVminVmax_internal<1>(inputImage, channels, rect);
    } else {
        return findAutoContrastVminVmax_generic(inputImage, nComps, channels, rect);
    }
} // findAutoContrastVminVmax

template <typename PIX, int maxValue, bool opaque, bool applyMatte, int rOffset, int gOffset, int bOffset>
void
scaleToTexture8bits_generic(const RectI& roi,
                            const RenderViewerArgs & args,
                            int nComps,
                            const ViewerInstancePtr& viewer,
                            const UpdateViewerParams::CachedTile& tile,
                            U32* tileBuffer)
{
    const size_t pixelSize = sizeof(PIX);
    const bool luminance = (args.channels == eDisplayChannelsY);
    Image::ReadAccess acc = Image::ReadAccess( args.inputImage.get() );
    const RectI srcImgBounds = args.inputImage->getBounds();

    if ( (args.renderOnlyRoI && !tile.rect.contains(roi)) || (!args.renderOnlyRoI && !roi.contains(tile.rect)) ) {
        return;
    }
    assert(tile.rect.x2 > tile.rect.x1);

    int dstRowElements;
    U32* dst_pixels;
    if (args.renderOnlyRoI) {
        dstRowElements = tile.rect.width();
        dst_pixels = tileBuffer + (roi.y1 - tile.rect.y1) * dstRowElements + (roi.x1 - tile.rect.x1);
    } else {
        dstRowElements = args.tileRowElements;
        dst_pixels = tileBuffer + (tile.rect.y1 - tile.rectRounded.y1) * args.tileRowElements + (tile.rect.x1 - tile.rectRounded.x1);
    }

    const int y1 = args.renderOnlyRoI ? roi.y1 : tile.rect.y1;
    const int y2 = args.renderOnlyRoI ? roi.y2 : tile.rect.y2;
    const int x1 = args.renderOnlyRoI ? roi.x1 : tile.rect.x1;
    const int x2 = args.renderOnlyRoI ? roi.x2 : tile.rect.x2;
    const PIX* src_pixels = (const PIX*)acc.pixelAt(x1, y1);
    const int srcRowElements = (int)args.inputImage->getRowElements();
    boost::shared_ptr<Image::ReadAccess> matteAcc;
    if (applyMatte) {
        matteAcc.reset( new Image::ReadAccess( args.matteImage.get() ) );
    }

    for (int y = y1; y < y2;
         ++y,
         dst_pixels += dstRowElements) {
        // coverity[dont_call]
        int start = (int)( rand() % (x2 - x1) );


        for (int backward = 0; backward < 2; ++backward) {
            int index = backward ? start - 1 : start;

            assert( backward == 1 || ( index >= 0 && index < (x2 - x1) ) );

            unsigned error_r = 0x80;
            unsigned error_g = 0x80;
            unsigned error_b = 0x80;

            while (index < (x2 - x1) && index >= 0) {
                double r = 0.;
                double g = 0.;
                double b = 0.;
                int uA = 0;
                double a = 0;
                if (nComps >= 4) {
                    r = (src_pixels ? src_pixels[index * nComps + rOffset] : 0.);
                    g = (src_pixels ? src_pixels[index * nComps + gOffset] : 0.);
                    b = (src_pixels ? src_pixels[index * nComps + bOffset] : 0.);
                    if (opaque) {
                        a = 1;
                        uA = 255;
                    } else {
                        a = src_pixels ? src_pixels[index * nComps + 3] : 0;
                        uA = Color::floatToInt<256>(a);
                    }
                } else if (nComps == 3) {
                    // coverity[dead_error_line]
                    r = (src_pixels && rOffset < nComps) ? src_pixels[index * nComps + rOffset] : 0.;
                    // coverity[dead_error_line]
                    g = (src_pixels && gOffset < nComps) ? src_pixels[index * nComps + gOffset] : 0.;
                    // coverity[dead_error_line]
                    b = (src_pixels && bOffset < nComps) ? src_pixels[index * nComps + bOffset] : 0.;
                    a = (src_pixels ? 1 : 0);
                    uA = a * 255;
                } else if (nComps == 2) {
                    // coverity[dead_error_line]
                    r = (src_pixels && rOffset < nComps) ? src_pixels[index * nComps + rOffset] : 0.;
                    // coverity[dead_error_line]
                    g = (src_pixels && gOffset < nComps) ? src_pixels[index * nComps + gOffset] : 0.;
                    b = 0;
                    a = (src_pixels ? 1 : 0);
                    uA = a * 255;
                } else if (nComps == 1) {
                    // coverity[dead_error_line]
                    r = (src_pixels && rOffset < nComps) ? src_pixels[index * nComps + rOffset] : 0.;
                    g = b = r;
                    a = (src_pixels ? 1 : 0);
                    uA = a * 255;
                } else {
                    assert(false);
                }


                switch (pixelSize) {
                case sizeof(unsigned char):     //byte
                    if (args.srcColorSpace) {
                        r = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)r );
                        g = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)g );
                        b = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)b );
                    } else {
                        r = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                        g = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)g );
                        b = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)b );
                    }
                    break;
                case sizeof(unsigned short):     //short
                    if (args.srcColorSpace) {
                        r = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)r );
                        g = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)g );
                        b = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)b );
                    } else {
                        r = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)r );
                        g = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)g );
                        b = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)b );
                    }
                    break;
                case sizeof(float):     //float
                    if (args.srcColorSpace) {
                        r = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(r);
                        g = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(g);
                        b = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(b);
                    }
                    break;
                default:
                    break;
                }

                r = r * args.gain + args.offset;
                g = g * args.gain + args.offset;
                b = b * args.gain + args.offset;
                if  (args.gamma <= 0) {
                    r = (r < 1.) ? 0. : (r == 1. ? 1. : std::numeric_limits<double>::infinity() );
                    g = (g < 1.) ? 0. : (g == 1. ? 1. : std::numeric_limits<double>::infinity() );
                    b = (b < 1.) ? 0. : (b == 1. ? 1. : std::numeric_limits<double>::infinity() );
                } else if (args.gamma != 1.) {
                    r = viewer->interpolateGammaLut(r);
                    g = viewer->interpolateGammaLut(g);
                    b = viewer->interpolateGammaLut(b);
                }

                if (luminance) {
                    r = 0.299 * r + 0.587 * g + 0.114 * b;
                    g = r;
                    b = r;
                }


                U8 uR, uG, uB;
                if (!args.colorSpace) {
                    uR = Color::floatToInt<256>(r);
                    uG = Color::floatToInt<256>(g);
                    uB = Color::floatToInt<256>(b);
                } else {
                    error_r = (error_r & 0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(r);
                    error_g = (error_g & 0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(g);
                    error_b = (error_b & 0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(b);
                    assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
                    uR = (U8)(error_r >> 8);
                    uG = (U8)(error_g >> 8);
                    uB = (U8)(error_b >> 8);
                }

                if (applyMatte) {
                    double alphaMatteValue = 0;
                    if (args.matteImage == args.inputImage) {
                        switch (args.alphaChannelIndex) {
                        case 0:
                            alphaMatteValue = r;
                            break;
                        case 1:
                            alphaMatteValue = g;
                            break;
                        case 2:
                            alphaMatteValue = b;
                            break;
                        case 3:
                            alphaMatteValue = a;
                            break;
                        default:
                            break;
                        }
                    } else {
                        const PIX* src_pixels = (const PIX*)matteAcc->pixelAt(x1 + index, y);
                        if (src_pixels) {
                            alphaMatteValue = (double)src_pixels[args.alphaChannelIndex];
                            switch (pixelSize) {
                            case sizeof(unsigned char):     //byte
                                alphaMatteValue = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                                break;
                            case sizeof(unsigned short):     //short
                                alphaMatteValue = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned short)r );
                                break;
                            default:
                                break;
                            }
                        }
                    }
                    U8 matteA;
                    if (args.colorSpace) {
                        matteA = args.colorSpace->toColorSpaceUint8FromLinearFloatFast(alphaMatteValue) / 2;
                    } else {
                        matteA = Color::floatToInt<256>(alphaMatteValue) / 2;
                    }
                    uR = Image::clampIfInt<U8>( (double)uR + matteA );
                }

                dst_pixels[index] = toBGRA(uR, uG, uB, uA);

                if (backward) {
                    --index;
                } else {
                    ++index;
                }
            } // while (index < args.texRect.w && index >= 0) {
        } // for (int backward = 0; backward < 2; ++backward) {
        if (src_pixels) {
            src_pixels += srcRowElements;
        }
    } // for (int y = yRange.first; y < yRange.second;
} // scaleToTexture8bits_generic

template <typename PIX, int maxValue, int nComps, bool opaque, bool matteOverlay, int rOffset, int gOffset, int bOffset>
void
scaleToTexture8bits_internal(const RectI& roi,
                             const RenderViewerArgs & args,
                             const ViewerInstancePtr& viewer,
                             const UpdateViewerParams::CachedTile& tile,
                             U32* output)
{
    scaleToTexture8bits_generic<PIX, maxValue, opaque, matteOverlay, rOffset, gOffset, bOffset>(roi, args, nComps, viewer, tile, output);
}

template <typename PIX, int maxValue, int nComps, bool opaque, int rOffset, int gOffset, int bOffset>
void
scaleToTexture8bitsForMatte(const RectI& roi,
                            const RenderViewerArgs & args,
                            const ViewerInstancePtr& viewer,
                            const UpdateViewerParams::CachedTile& tile,
                            U32* output)
{
    bool applyMate = args.matteImage.get() && args.alphaChannelIndex >= 0;

    if (applyMate) {
        scaleToTexture8bits_internal<PIX, maxValue, nComps, opaque, true, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
    } else {
        scaleToTexture8bits_internal<PIX, maxValue, nComps, opaque, false, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
    }
}

template <typename PIX, int maxValue, bool opaque, int rOffset, int gOffset, int bOffset>
void
scaleToTexture8bitsForDepthForComponents(const RectI& roi,
                                         const RenderViewerArgs & args,
                                         const ViewerInstancePtr& viewer,
                                         const UpdateViewerParams::CachedTile& tile,
                                         U32* output)
{
    int nComps = args.inputImage->getComponents().getNumComponents();

    switch (nComps) {
    case 4:
        scaleToTexture8bitsForMatte<PIX, maxValue, 4, opaque, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
        break;
    case 3:
        scaleToTexture8bitsForMatte<PIX, maxValue, 3, opaque, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
        break;
    case 2:
        scaleToTexture8bitsForMatte<PIX, maxValue, 2, opaque, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
        break;
    case 1:
        scaleToTexture8bitsForMatte<PIX, maxValue, 1, opaque, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
        break;
    default:
        bool applyMate = args.matteImage.get() && args.alphaChannelIndex >= 0;
        if (applyMate) {
            scaleToTexture8bits_generic<PIX, maxValue, opaque, true, rOffset, gOffset, bOffset>(roi, args, nComps, viewer, tile, output);
        } else {
            scaleToTexture8bits_generic<PIX, maxValue, opaque, false, rOffset, gOffset, bOffset>(roi, args, nComps, viewer, tile, output);
        }
        break;
    }
}

template <typename PIX, int maxValue, bool opaque>
void
scaleToTexture8bitsForPremult(const RectI& roi,
                              const RenderViewerArgs & args,
                              const ViewerInstancePtr& viewer,
                              const UpdateViewerParams::CachedTile& tile,
                              U32* output)
{
    switch (args.channels) {
    case eDisplayChannelsRGB:
    case eDisplayChannelsY:
    case eDisplayChannelsMatte:

        scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 0, 1, 2>(roi, args, viewer, tile, output);
        break;
    case eDisplayChannelsG:
        scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, viewer, tile, output);
        break;
    case eDisplayChannelsB:
        scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, viewer, tile, output);
        break;
    case eDisplayChannelsA:
        switch (args.alphaChannelIndex) {
        case -1:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, viewer, tile, output);
            break;
        case 0:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, viewer, tile, output);
            break;
        case 1:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, viewer, tile, output);
            break;
        case 2:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, viewer, tile, output);
            break;
        case 3:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, viewer, tile, output);
            break;
        default:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, viewer, tile, output);
        }

        break;
    case eDisplayChannelsR:
    default:
        scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, viewer, tile, output);

        break;
    }
}

template <typename PIX, int maxValue>
void
scaleToTexture8bitsForDepth(const RectI& roi,
                            const RenderViewerArgs & args,
                            const ViewerInstancePtr& viewer,
                            const UpdateViewerParams::CachedTile& tile,
                            U32* output)
{
    switch (args.srcPremult) {
    case eImagePremultiplicationOpaque:
        scaleToTexture8bitsForPremult<PIX, maxValue, true>(roi, args, viewer, tile, output);
        break;
    case eImagePremultiplicationPremultiplied:
    case eImagePremultiplicationUnPremultiplied:
    default:
        scaleToTexture8bitsForPremult<PIX, maxValue, false>(roi, args, viewer, tile, output);
        break;
    }
}

void
scaleToTexture8bits(const RectI& roi,
                    const RenderViewerArgs & args,
                    const ViewerInstancePtr& viewer,
                    const UpdateViewerParams::CachedTile& tile,
                    U32* output)
{
    assert(output);
    switch ( args.inputImage->getBitDepth() ) {
    case eImageBitDepthFloat:
        scaleToTexture8bitsForDepth<float, 1>(roi, args, viewer, tile, output);
        break;
    case eImageBitDepthByte:
        scaleToTexture8bitsForDepth<unsigned char, 255>(roi, args, viewer, tile, output);
        break;
    case eImageBitDepthShort:
        scaleToTexture8bitsForDepth<unsigned short, 65535>(roi, args, viewer, tile, output);
        break;
    case eImageBitDepthHalf:
        assert(false);
        break;
    case eImageBitDepthNone:
        break;
    }
} // scaleToTexture8bits

float
ViewerInstance::interpolateGammaLut(float value)
{
    return _imp->lookupGammaLut(value);
}


template <typename PIX, int maxValue, bool opaque, bool applyMatte, int rOffset, int gOffset, int bOffset>
void
scaleToTexture32bitsGeneric(const RectI& roi,
                            const RenderViewerArgs & args,
                            int nComps,
                            const UpdateViewerParams::CachedTile& tile,
                            float *tileBuffer)
{
    const size_t pixelSize = sizeof(PIX);
    const bool luminance = (args.channels == eDisplayChannelsY);
    const int dstRowElements = args.renderOnlyRoI ? tile.rect.width() * 4 : args.tileRowElements;
    Image::ReadAccess acc = Image::ReadAccess( args.inputImage.get() );
    boost::shared_ptr<Image::ReadAccess> matteAcc;

    if (applyMatte) {
        matteAcc.reset( new Image::ReadAccess( args.matteImage.get() ) );
    }

    assert( (args.renderOnlyRoI && roi.x1 >= tile.rect.x1 && roi.x2 <= tile.rect.x2 && roi.y1 >= tile.rect.y1 && roi.y2 <= tile.rect.y2) || (!args.renderOnlyRoI && tile.rect.x1 >= roi.x1 && tile.rect.x2 <= roi.x2 && tile.rect.y1 >= roi.y1 && tile.rect.y2 <= roi.y2) );
    assert(tile.rect.x2 > tile.rect.x1);

    float* dst_pixels;
    if (args.renderOnlyRoI) {
        dst_pixels = tileBuffer + (roi.y1 - tile.rect.y1) * dstRowElements + (roi.x1 - tile.rect.x1) * 4;
    } else {
        dst_pixels = tileBuffer + (tile.rect.y1 - tile.rectRounded.y1) * dstRowElements + (tile.rect.x1 - tile.rectRounded.x1) * 4;
    }

    const int y1 = args.renderOnlyRoI ? roi.y1 : tile.rect.y1;
    const int y2 = args.renderOnlyRoI ? roi.y2 : tile.rect.y2;
    const int x1 = args.renderOnlyRoI ? roi.x1 : tile.rect.x1;
    const int x2 = args.renderOnlyRoI ? roi.x2 : tile.rect.x2;
    const float* src_pixels = (const float*)acc.pixelAt(x1, y1);
    const int srcRowElements = (const int)args.inputImage->getRowElements();

    for (int y = y1; y < y2;
         ++y,
         dst_pixels += dstRowElements) {
        for (int x = 0; x < (x2 - x1);
             ++x) {
            double r = 0.;
            double g = 0.;
            double b = 0.;
            double a = 0.;

            if (nComps >= 4) {
                r = (src_pixels && rOffset < nComps) ? src_pixels[x * nComps + rOffset] : 0.;
                g = (src_pixels && gOffset < nComps) ? src_pixels[x * nComps + gOffset] : 0.;
                b = (src_pixels && bOffset < nComps) ? src_pixels[x * nComps + bOffset] : 0.;
                if (opaque) {
                    a = 1.;
                } else {
                    a = src_pixels ? src_pixels[x * nComps + 3] : 0.;
                }
            } else if (nComps == 3) {
                // coverity[dead_error_line]
                r = (src_pixels && rOffset < nComps) ? src_pixels[x * nComps + rOffset] : 0.;
                // coverity[dead_error_line]
                g = (src_pixels && gOffset < nComps) ? src_pixels[x * nComps + gOffset] : 0.;
                // coverity[dead_error_line]
                b = (src_pixels && bOffset < nComps) ? src_pixels[x * nComps + bOffset] : 0.;
                a = 1.;
            } else if (nComps == 2) {
                // coverity[dead_error_line]
                r = (src_pixels && rOffset < nComps) ? src_pixels[x * nComps + rOffset] : 0.;
                // coverity[dead_error_line]
                g = (src_pixels && gOffset < nComps) ? src_pixels[x * nComps + gOffset] : 0.;
                b = 0.;
                a = 1.;
            } else if (nComps == 1) {
                // coverity[dead_error_line]
                r = (src_pixels && rOffset < nComps) ? src_pixels[x * nComps + rOffset] : 0.;
                g = b = r;
                a = 1.;
            } else {
                assert(false);
            }


            switch (pixelSize) {
            case sizeof(unsigned char):
                if (args.srcColorSpace) {
                    r = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)r );
                    g = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)g );
                    b = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)b );
                } else {
                    r = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                    g = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)g );
                    b = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)b );
                }
                break;
            case sizeof(unsigned short):
                if (args.srcColorSpace) {
                    r = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)r );
                    g = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)g );
                    b = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)b );
                } else {
                    r = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)r );
                    g = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)g );
                    b = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)b );
                }
                break;
            case sizeof(float):
                if (args.srcColorSpace) {
                    r = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(r);
                    g = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(g);
                    b = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(b);
                }
                break;
            default:
                break;
            }


            if (luminance) {
                r = 0.299 * r + 0.587 * g + 0.114 * b;
                g = r;
                b = r;
            }

            if (applyMatte) {
                double alphaMatteValue = 0;
                if (args.matteImage == args.inputImage) {
                    switch (args.alphaChannelIndex) {
                    case 0:
                        alphaMatteValue = r;
                        break;
                    case 1:
                        alphaMatteValue = g;
                        break;
                    case 2:
                        alphaMatteValue = b;
                        break;
                    case 3:
                        alphaMatteValue = a;
                        break;
                    default:
                        break;
                    }
                } else {
                    const PIX* src_pixels = (const PIX*)matteAcc->pixelAt(x, y);
                    if (src_pixels) {
                        alphaMatteValue = (double)src_pixels[args.alphaChannelIndex];
                        switch (pixelSize) {
                        case sizeof(unsigned char):     //byte
                            alphaMatteValue = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                            break;
                        case sizeof(unsigned short):     //short
                            alphaMatteValue = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned short)r );
                            break;
                        default:
                            break;
                        }
                    }
                }
                r += alphaMatteValue * 0.5;
            }


            dst_pixels[x * 4] = r; // do not clamp! values may be more than 1 or less than 0
            dst_pixels[x * 4 + 1] = g;
            dst_pixels[x * 4 + 2] = b;
            dst_pixels[x * 4 + 3] = a;
        }
        if (src_pixels) {
            src_pixels += srcRowElements;
        }
    }
} // scaleToTexture32bitsGeneric

template <typename PIX, int maxValue, int nComps, bool opaque, bool applyMatte, int rOffset, int gOffset, int bOffset>
void
scaleToTexture32bitsInternal(const RectI& roi,
                             const RenderViewerArgs & args,
                             const UpdateViewerParams::CachedTile& tile,
                             float *output)
{
    scaleToTexture32bitsGeneric<PIX, maxValue, opaque, applyMatte, rOffset, gOffset, bOffset>(roi, args, nComps, tile, output);
}

template <typename PIX, int maxValue, int nComps, bool opaque, int rOffset, int gOffset, int bOffset>
void
scaleToTexture32bitsForMatte(const RectI& roi,
                             const RenderViewerArgs & args,
                             const UpdateViewerParams::CachedTile& tile,
                             float *output)
{
    bool applyMatte = args.matteImage.get() && args.alphaChannelIndex >= 0;

    if (applyMatte) {
        scaleToTexture32bitsInternal<PIX, maxValue, nComps, opaque, true, rOffset, gOffset, bOffset>(roi, args, tile, output);
    } else {
        scaleToTexture32bitsInternal<PIX, maxValue, nComps, opaque, false, rOffset, gOffset, bOffset>(roi, args, tile, output);
    }
}

template <typename PIX, int maxValue, bool opaque, int rOffset, int gOffset, int bOffset>
void
scaleToTexture32bitsForDepthForComponents(const RectI& roi,
                                          const RenderViewerArgs & args,
                                          const UpdateViewerParams::CachedTile& tile,
                                          float *output)
{
    int nComps = args.inputImage->getComponents().getNumComponents();

    switch (nComps) {
    case 4:
        scaleToTexture32bitsForMatte<PIX, maxValue, 4, opaque, rOffset, gOffset, bOffset>(roi, args, tile, output);
        break;
    case 3:
        scaleToTexture32bitsForMatte<PIX, maxValue, 3, opaque, rOffset, gOffset, bOffset>(roi, args, tile, output);
        break;
    case 2:
        scaleToTexture32bitsForMatte<PIX, maxValue, 2, opaque, rOffset, gOffset, bOffset>(roi, args, tile, output);
        break;
    case 1:
        scaleToTexture32bitsForMatte<PIX, maxValue, 1, opaque, rOffset, gOffset, bOffset>(roi, args, tile, output);
        break;
    default:
        bool applyMatte = args.matteImage.get() && args.alphaChannelIndex >= 0;
        if (applyMatte) {
            scaleToTexture32bitsGeneric<PIX, maxValue, opaque, true, rOffset, gOffset, bOffset>(roi, args, nComps, tile, output);
        } else {
            scaleToTexture32bitsGeneric<PIX, maxValue, opaque, false, rOffset, gOffset, bOffset>(roi, args, nComps, tile, output);
        }
        break;
    }
}

template <typename PIX, int maxValue, bool opaque>
void
scaleToTexture32bitsForPremultForComponents(const RectI& roi,
                                            const RenderViewerArgs & args,
                                            const UpdateViewerParams::CachedTile& tile,
                                            float *output)
{
    switch (args.channels) {
    case eDisplayChannelsRGB:
    case eDisplayChannelsY:
    case eDisplayChannelsMatte:
        scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 0, 1, 2>(roi, args, tile, output);
        break;
    case eDisplayChannelsG:
        scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, tile, output);
        break;
    case eDisplayChannelsB:
        scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, tile, output);
        break;
    case eDisplayChannelsA:
        switch (args.alphaChannelIndex) {
        case -1:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, tile, output);
            break;
        case 0:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, tile, output);
            break;
        case 1:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, tile, output);
            break;
        case 2:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, tile, output);
            break;
        case 3:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, tile, output);
            break;
        default:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, tile, output);
            break;
        }

        break;
    case eDisplayChannelsR:
    default:
        scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, tile, output);
        break;
    }
}

template <typename PIX, int maxValue>
void
scaleToTexture32bitsForPremult(const RectI& roi,
                               const RenderViewerArgs & args,
                               const UpdateViewerParams::CachedTile& tile,
                               float *output)
{
    switch (args.srcPremult) {
    case eImagePremultiplicationOpaque:
        scaleToTexture32bitsForPremultForComponents<PIX, maxValue, true>(roi, args, tile, output);
        break;
    case eImagePremultiplicationPremultiplied:
    case eImagePremultiplicationUnPremultiplied:
    default:
        scaleToTexture32bitsForPremultForComponents<PIX, maxValue, false>(roi, args, tile, output);
        break;
    }
}

void
scaleToTexture32bits(const RectI& roi,
                     const RenderViewerArgs & args,
                     const UpdateViewerParams::CachedTile& tile,
                     float *output)
{
    assert(output);

    switch ( args.inputImage->getBitDepth() ) {
    case eImageBitDepthFloat:
        scaleToTexture32bitsForPremult<float, 1>(roi, args, tile, output);
        break;
    case eImageBitDepthByte:
        scaleToTexture32bitsForPremult<unsigned char, 255>(roi, args, tile, output);
        break;
    case eImageBitDepthShort:
        scaleToTexture32bitsForPremult<unsigned short, 65535>(roi, args, tile, output);
        break;
    case eImageBitDepthHalf:
        assert(false);
        break;
    case eImageBitDepthNone:
        break;
    }
} // scaleToTexture32bits

void
ViewerInstance::ViewerInstancePrivate::updateViewer(boost::shared_ptr<UpdateViewerParams> params)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // QMutexLocker locker(&updateViewerMutex);
    // if (updateViewerRunning) {
    OpenGLViewerI* uiContext = instance->getUiContext();
    if (uiContext) {
        uiContext->makeOpenGLcontextCurrent();
    }
    if (params->tiles.empty()) {
        return;
    }
    bool doUpdate = true;


    bool isImageUpToDate = checkAndUpdateDisplayAge( params->textureIndex, params->abortInfo->getRenderAge() );
    (void)isImageUpToDate;

    // Don't uncomment: if the image was rendered so far, render it to the display texture so that the user get some feedback
   /* if ( !params->isPartialRect && !params->isSequential && !isImageUpToDate) {
        doUpdate = false;
    }*/
    if (doUpdate) {
        /*RectI bounds;
           params->rod.toPixelEnclosing(params->mipMapLevel, params->pixelAspectRatio, &bounds);*/

        assert( (params->isPartialRect && params->tiles.size() == 1) || !params->isPartialRect );

        boost::shared_ptr<Texture> texture;
        bool isFirstTile = true;
        for (std::list<UpdateViewerParams::CachedTile>::iterator it = params->tiles.begin(); it != params->tiles.end(); ++it) {
            if (!it->ramBuffer) {
                continue;
            }

            // For cached tiles, some tiles might not have the standard tile, (i.e: the last tile column/row).
            // Since the internal buffer is rounded to the tile size anyway we want the glTexSubImage2D call to ensure
            // that we upload the full internal buffer
            TextureRect texRect;
            texRect.par = it->rect.par;
            texRect.closestPo2 = it->rect.closestPo2;
            texRect.set(it->rectRounded);
    
            assert(params->roi.contains(texRect));
            if (uiContext) {
                uiContext->transferBufferFromRAMtoGPU(it->ramBuffer, it->bytesCount, params->roi, params->roiNotRoundedToTileSize, texRect, params->textureIndex, params->isPartialRect, isFirstTile, &texture);
            }
            isFirstTile = false;
        }


        if (uiContext && !instance->getApp()->isGuiFrozen()) {
            uiContext->updateColorPicker(params->textureIndex);
        }

        const UpdateViewerParams::CachedTile& firstTile = params->tiles.front();
        ImagePtr originalImage;
        originalImage = params->colorImage;
        if (firstTile.cachedData && !originalImage) {
            originalImage = firstTile.cachedData->getInternalImage();
        }
        ImageBitDepthEnum depth;
        if (originalImage) {
            depth = originalImage->getBitDepth();
        } else {
            assert(firstTile.cachedData);
            if (firstTile.cachedData) {
                depth = (ImageBitDepthEnum)firstTile.cachedData->getKey().getBitDepth();
            } else {
                depth = eImageBitDepthByte;
            }
        }

        if (uiContext) {
            uiContext->endTransferBufferFromRAMToGPU(params->textureIndex, texture, originalImage, params->time, params->rod,  params->pixelAspectRatio, depth, params->mipMapLevel, params->srcPremult, params->gain, params->gamma, params->offset, params->lut, params->recenterViewport, params->viewportCenter, params->isPartialRect);
        }
    }

    //
    //        updateViewerRunning = false;
    //    }
    //    updateViewerCond.wakeOne();
} // ViewerInstance::ViewerInstancePrivate::updateViewer

bool
ViewerInstance::isInputOptional(int /*n*/) const
{
    return false;
}



void
ViewerInstance::callRedrawOnMainThread()
{
    ViewerNodePtr node = getViewerNodeGroup();
    node->s_redrawOnMainThread();
}

void
ViewerInstance::addAcceptedComponents(int /*inputNb*/,
                                      std::bitset<4>* supported)
{
  (*supported)[0] = (*supported)[1] = (*supported)[2] = (*supported)[3] = 1;
}




void
ViewerInstance::getInputsComponentsAvailables(std::set<ImageComponents>* comps) const
{
    const int nInputs = getMaxInputCount();
    for (int i = 0; i < nInputs; ++i) {
        NodePtr input = getNode()->getInput(i);
        if (input) {
            EffectInstance::ComponentsAvailableMap compsAvailable;
            input->getEffectInstance()->getComponentsAvailable(true, true, getCurrentTime(), &compsAvailable);
            for (EffectInstance::ComponentsAvailableMap::iterator it = compsAvailable.begin(); it != compsAvailable.end(); ++it) {
                if ( it->second.lock() ) {
                    comps->insert(it->first);
                }
            }
        }
    }
}


void
ViewerInstance::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthByte);
}







NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING
#include "moc_ViewerInstance.cpp"
