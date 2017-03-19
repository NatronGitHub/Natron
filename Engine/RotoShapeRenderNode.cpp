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

#include "RotoShapeRenderNode.h"


#include "Engine/AppInstance.h"
#include "Engine/BezierCP.h"
#include "Engine/Bezier.h"
#include "Engine/Color.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/Hash64.h"
#include "Engine/NodeMetadata.h"
#include "Engine/KnobTypes.h"
#include "Engine/OSGLContext.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoShapeRenderNodePrivate.h"
#include "Engine/RotoShapeRenderCairo.h"
#include "Engine/RotoShapeRenderGL.h"
#include "Engine/RotoPaint.h"


NATRON_NAMESPACE_ENTER;

enum RotoShapeRenderTypeEnum
{
    eRotoShapeRenderTypeSolid,
    eRotoShapeRenderTypeSmear
};

PluginPtr
RotoShapeRenderNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_PAINT);
    PluginPtr ret = Plugin::create((void*)RotoShapeRenderNode::create, (void*)RotoShapeRenderNode::createRenderClone, PLUGINID_NATRON_ROTOSHAPE, "RotoShape", 1, 0, grouping);
    ret->setProperty<bool>(kNatronPluginPropIsInternalOnly, true);
    ret->setProperty<int>(kNatronPluginPropOpenGLSupport, (int)ePluginOpenGLRenderSupportYes);
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafeFrame);
    return ret;
}


RotoShapeRenderNode::RotoShapeRenderNode(NodePtr n)
: EffectInstance(n)
, _imp(new RotoShapeRenderNodePrivate())
{
}

RotoShapeRenderNode::RotoShapeRenderNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
: EffectInstance(mainInstance, key)
, _imp()
{
    RotoShapeRenderNode* other = dynamic_cast<RotoShapeRenderNode*>(mainInstance.get());
    assert(other);
    _imp = other->_imp;
}

RotoShapeRenderNode::~RotoShapeRenderNode()
{

}

bool
RotoShapeRenderNode::canCPUImplementationSupportOSMesa() const
{
#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
    return false;
#else
    return true;
#endif
}


void
RotoShapeRenderNode::addAcceptedComponents(int /*inputNb*/,
                                 std::bitset<4>* supported)
{
    (*supported)[0] = (*supported)[1] = (*supported)[2] = (*supported)[3] = 1;
}

void
RotoShapeRenderNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
}

void
RotoShapeRenderNode::fetchRenderCloneKnobs()
{
    EffectInstance::fetchRenderCloneKnobs();
    _imp->renderType = toKnobChoice(getKnobByName(kRotoShapeRenderNodeParamType));
}

void
RotoShapeRenderNode::initializeKnobs()
{
    KnobPagePtr page = createKnob<KnobPage>("controlsPage");
    page->setLabel(tr("Controls"));

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kRotoShapeRenderNodeParamType);
        param->setLabel(tr(kRotoShapeRenderNodeParamTypeLabel));
        {
            std::vector<ChoiceOption> options;
            options.push_back(ChoiceOption(kRotoShapeRenderNodeParamTypeSolid, "", ""));
            options.push_back(ChoiceOption(kRotoShapeRenderNodeParamTypeSmear, "", ""));
            param->populateChoices(options);
        }
        param->setIsMetadataSlave(true);
        page->addKnob(param);
        _imp->renderType = param;
    }
}

void
RotoShapeRenderNode::appendToHash(const ComputeHashArgs& args, Hash64* hash)
{
    RotoDrawableItemPtr item = getAttachedRotoItem();
    assert(item);

    if (args.hashType == HashableObject::eComputeHashTypeTimeViewVariant) {
        // The render of the Roto shape/stroke depends on the points at the current time/view
        RotoStrokeItemPtr isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(item);
        BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(item);

        // Append the hash of the shape for each motion blur sample
        RangeD range;
        int divisions;
        item->getMotionBlurSettings(args.time, args.view, &range, &divisions);
        double interval = divisions >= 1 ? (range.max - range.min) / divisions : 1.;

        for (int i = 0; i < divisions; ++i) {
            double t = divisions > 1 ? range.min + i * interval : args.time;

            ComputeHashArgs shapeArgs = args;
            shapeArgs.time = TimeValue(t);

            if (isBezier) {
                U64 bh = isBezier->computeHash(shapeArgs);
                hash->append(bh);

            } else if (isStroke) {
                U64 sh = isStroke->computeHash(shapeArgs);
                hash->append(sh);
            }
        }


    }


    RotoPaintPtr rotoPaintNode;
    KnobItemsTablePtr model = item->getModel();
    if (model) {
        rotoPaintNode = toRotoPaint(model->getNode()->getEffectInstance());
    }

    // we also depend on the motion-blur type knob of the RotoPaint node itself.
    // If we had a knob that would be linked to it we wouldn't need this, but since
    // we directly refer to this knob, we must explicitly add it to the hash.
    if  (rotoPaintNode) {
        U64 sh = rotoPaintNode->getMotionBlurTypeKnob()->computeHash(args);
        hash->append(sh);
    }


    EffectInstance::appendToHash(args, hash);

} // appendToHash


bool
RotoShapeRenderNode::supportsTiles() const
{
    return false;
}

bool
RotoShapeRenderNode::supportsMultiResolution() const
{
    return true;
}

bool
RotoShapeRenderNode::isMultiPlanar() const
{
    return true;
}

ActionRetCodeEnum
RotoShapeRenderNode::getLayersProducedAndNeeded(TimeValue time,
                                                ViewIdx view,
                                                std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                                std::list<ImagePlaneDesc>* layersProduced,
                                                TimeValue* passThroughTime,
                                                ViewIdx* passThroughView,
                                                int* passThroughInputNb)
{
    int renderType_i = _imp->renderType.lock()->getValue();
    if (renderType_i == 1) { // Smear
        return EffectInstance::getLayersProducedAndNeeded(time, view, inputLayersNeeded, layersProduced, passThroughTime, passThroughView, passThroughInputNb);
    } else {
        // Solid
        ImagePlaneDesc inputPlane, pairedInputPlane;
        getMetadataComponents(0, &inputPlane, &pairedInputPlane);
        (*inputLayersNeeded)[0].push_back(inputPlane);

        {
            std::vector<std::string> channels(1);
            channels[0] = "A";
            ImagePlaneDesc rotoMaskPlane("RotoMask", "", "Alpha", channels);
            layersProduced->push_back(rotoMaskPlane);
        }
        *passThroughTime = time;
        *passThroughView = view;
        *passThroughInputNb = 0;
        return eActionStatusOK;
    }
} // getLayersProducedAndNeeded

ActionRetCodeEnum
RotoShapeRenderNode::getTimeInvariantMetaDatas(NodeMetadata& metadata)
{


    RotoShapeRenderTypeEnum type = (RotoShapeRenderTypeEnum)_imp->renderType.lock()->getValue();
    int nComps;
    if (type == eRotoShapeRenderTypeSolid) {
        nComps = 1;
    } else {
        nComps = 4;
    }

    metadata.setColorPlaneNComps(-1, nComps);
    metadata.setColorPlaneNComps(0, nComps);
    metadata.setIsContinuous(true);
    return eActionStatusOK;
}

static void getRoDFromItem(const RotoDrawableItemPtr& item, TimeValue time, ViewIdx view, RectD* rod)
{
    // Account for motion-blur
    RangeD range;
    int divisions;
    item->getMotionBlurSettings(time, view, &range, &divisions);
    double interval = divisions >= 1 ? (range.max - range.min) / divisions : 1.;

    for (int i = 0; i < divisions; ++i) {
        RectD maskRod;
        try {
            double t = divisions > 1 ? range.min + i * interval : time;
            maskRod = item->getBoundingBox(TimeValue(t), view);
        } catch (...) {
        }

        if ( rod->isNull() ) {
            *rod = maskRod;
        } else {
            rod->merge(maskRod);
        }
    }

}


ActionRetCodeEnum
RotoShapeRenderNode::getRegionOfDefinition(TimeValue time, const RenderScale & scale, ViewIdx view, RectD* rod)
{
   

    ActionRetCodeEnum st = EffectInstance::getRegionOfDefinition(time, scale, view, rod);
    if (isFailureRetCode(st)) {
        rod->x1 = rod->y1 = rod->x2 = rod->y2 = 0.;
    }

    RotoDrawableItemPtr item = getAttachedRotoItem();
    assert(item);
    getRoDFromItem(item, time, view, rod);

    return eActionStatusOK;

}

ActionRetCodeEnum
RotoShapeRenderNode::isIdentity(TimeValue time,
                                const RenderScale & scale,
                                const RectI & roi,
                                ViewIdx view,
                                TimeValue* inputTime,
                                ViewIdx* inputView,
                                int* inputNb)
{
    *inputView = view;
    NodePtr node = getNode();
    

    RotoDrawableItemPtr rotoItem = getAttachedRotoItem();
    assert(rotoItem);
    Bezier* isBezier = dynamic_cast<Bezier*>(rotoItem.get());
    if (!rotoItem || !rotoItem->isActivated(time, view) || (isBezier && (!isBezier->isCurveFinished(view) || isBezier->getControlPointsCount(view) <= 1))) {
        *inputTime = time;
        *inputNb = 0;

        return eActionStatusOK;
    }

    RectD maskRod;
    getRoDFromItem(rotoItem, time, view, &maskRod);

    RectI maskPixelRod;
    maskRod.toPixelEnclosing(scale, getAspectRatio(-1), &maskPixelRod);
    if ( !maskPixelRod.intersects(roi) ) {
        *inputTime = time;
        *inputNb = 0;
    }
    
    return eActionStatusOK;
} // isIdentity



ActionRetCodeEnum
RotoShapeRenderNode::render(const RenderActionArgs& args)
{

#if !defined(ROTO_SHAPE_RENDER_ENABLE_CAIRO) && !defined(HAVE_OSMESA)
    setPersistentMessage(eMessageTypeError, tr("Roto requires either OSMesa (CONFIG += enable-osmesa) or Cairo (CONFIG += enable-cairo) in order to render on CPU").toStdString());
    return eStatusFailed;
#endif

#if !defined(ROTO_SHAPE_RENDER_ENABLE_CAIRO)
    if (args.backendType == eRenderBackendTypeCPU) {
        setPersistentMessage(eMessageTypeError, tr("An OpenGL context is required to draw with the Roto node. This might be because you are trying to render an image too big for OpenGL.").toStdString());
        return eActionStatusFailed;
    }
#endif

    RenderScale combinedScale = EffectInstance::getCombinedScale(args.mipMapLevel, args.proxyScale);

    // Get the Roto item attached to this node. It will be a render-local clone of the original item.
    RotoDrawableItemPtr rotoItem = getAttachedRotoItem();
    assert(rotoItem);
    if (!rotoItem) {
        return eActionStatusFailed;
    }

    // To be thread-safe we can only operate on a render clone.
    assert(rotoItem->isRenderClone());

    // Is it a smear or regular solid render ?
    RotoShapeRenderTypeEnum type = (RotoShapeRenderTypeEnum)_imp->renderType.lock()->getValue();

    // We only support rendering bezier or strokes
    RotoStrokeItemPtr isStroke = toRotoStrokeItem(rotoItem);
    BezierPtr isBezier = toBezier(rotoItem);

    // Get the real stroke (the one the user interacts with)
    RotoStrokeItemPtr nonRenderStroke = toRotoStrokeItem(getOriginalAttachedItem());

    if (type == eRotoShapeRenderTypeSmear && !isStroke) {
        return eActionStatusFailed;
    }

    // Check that the item is really activated... it should have been caught in isIdentity otherwise.
    assert(rotoItem->isActivated(args.time, args.view) && (!isBezier || (isBezier->isCurveFinished(args.view) && ( isBezier->getControlPointsCount(args.view) > 1 ))));

    const OSGLContextPtr& glContext = args.glContext;

    // There must be an OpenGL context bound when using OpenGL.
    if ((args.backendType == eRenderBackendTypeOpenGL || args.backendType == eRenderBackendTypeOSMesa) && !glContext) {
        setPersistentMessage(eMessageTypeError, tr("An OpenGL context is required to draw with the Roto node").toStdString());
        return eActionStatusFailed;
    }

    // This is the image plane where we render, we are not multiplane so we only render out one plane
    assert(args.outputPlanes.size() == 1);
    const std::pair<ImagePlaneDesc,ImagePtr>& outputPlane = args.outputPlanes.front();

    // True if this render was trigger because the user is painting (with a pen or mouse)
    bool isDuringPainting = isStroke && isStroke->isCurrentlyDrawing();

    // These variables are useful to pick the stroke drawing algorithm where it was at the previous draw step.
    double distNextIn = 0.;
    Point lastCenterIn = { INT_MIN, INT_MIN };
    int strokeStartPointIndex = 0;
    int strokeMultiIndex = 0;

    // For strokes and open-bezier evaluate them to get the points and their pressure
    // We also compute the bounding box of the item taking into account the motion blur
    if (isStroke) {
        strokeStartPointIndex = isStroke->getRenderCloneCurrentStrokeStartPointIndex();
        strokeMultiIndex = isStroke->getRenderCloneCurrentStrokeIndex();
        isStroke->getStrokeState(&lastCenterIn, &distNextIn);
    }

    // Ensure that the indices of the draw step are valid.
#ifdef DEBUG
    if (isDuringPainting) {
        if (strokeStartPointIndex == 0) {
            assert((isStroke->getRenderCloneCurrentStrokeEndPointIndex() + 1 - strokeStartPointIndex) == isStroke->getNumControlPoints(0));
        } else {
            // +2 because we also add the last point of the previous draw step
            assert((isStroke->getRenderCloneCurrentStrokeEndPointIndex() + 2 - strokeStartPointIndex) == isStroke->getNumControlPoints(0));
        }
    }
#endif

    // Now we are good to start rendering

    // This is the state of the stroke aglorithm in output of this draw step
    double distToNextOut = 0.;
    Point lastCenterOut;

    // Retrieve the OpenGL context local data that were allocated in attachOpenGLContext
    RotoShapeRenderNodeOpenGLDataPtr glData;
    if (args.glContextData) {
        glData = boost::dynamic_pointer_cast<RotoShapeRenderNodeOpenGLData>(args.glContextData);
        assert(glData);
    }

    // Firs time we draw this clear the background since we are not going to render the full image with OpenGL.
    if (strokeStartPointIndex == 0 && strokeMultiIndex == 0) {
        outputPlane.second->fillBoundsZero();
    }

    switch (type) {
        case eRotoShapeRenderTypeSolid: {

            // Account for motion-blur
            RangeD range;
            int divisions;
            rotoItem->getMotionBlurSettings(args.time, args.view, &range, &divisions);

            if (isDuringPainting) {
                // Do not use motion-blur when drawing.
                range.min = range.max = args.time;
                divisions = 1;
            }

#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
            // When cairo is enabled, render with it for a CPU render
            if (args.backendType == eRenderBackendTypeCPU) {
                RotoShapeRenderCairo::renderMaskInternal_cairo(rotoItem, args.roi, outputPlane.first, args.time, args.view, range, divisions, combinedScale, isDuringPainting, distNextIn, lastCenterIn, outputPlane.second, &distToNextOut, &lastCenterOut);
                if (isDuringPainting && isStroke) {
                    nonRenderStroke->updateStrokeData(lastCenterOut, distToNextOut, isStroke->getRenderCloneCurrentStrokeEndPointIndex());
                }
            } else
#endif
            // Otherwise render with OpenGL or OSMesa
            if (args.backendType == eRenderBackendTypeOpenGL || args.backendType == eRenderBackendTypeOSMesa) {

                // Figure out the shape color
                ColorRgbaD shapeColor;
                {
                    const double t = args.time;
                    KnobColorPtr colorKnob = rotoItem->getColorKnob();
                    if (colorKnob) {
                        shapeColor.r = colorKnob->getValueAtTime(TimeValue(t), DimIdx(0), args.view);
                        shapeColor.g = colorKnob->getValueAtTime(TimeValue(t), DimIdx(1), args.view);
                        shapeColor.b = colorKnob->getValueAtTime(TimeValue(t), DimIdx(2), args.view);
                        shapeColor.a = colorKnob->getValueAtTime(TimeValue(t), DimIdx(3), args.view);
                    }
                }

                // Figure out the opacity
                double opacity = rotoItem->getOpacityKnob()->getValueAtTime(args.time, DimIdx(0), args.view);

                // For a stroke or an opened bezier, use the generic stroke algorithm
                if ( isStroke || ( isBezier && isBezier->isOpenBezier() ) ) {
                    bool doBuildUp = isStroke->getBuildupKnob()->getValueAtTime(args.time, DimIdx(0), args.view);
                    RotoShapeRenderGL::renderStroke_gl(glContext, glData, args.roi, outputPlane.second, isDuringPainting, distNextIn, lastCenterIn, isStroke, doBuildUp, opacity, args.time, args.view, range, divisions, combinedScale, &distToNextOut, &lastCenterOut);

                    // Update the stroke algorithm in output
                    if (isDuringPainting && isStroke) {
                        nonRenderStroke->updateStrokeData(lastCenterOut, distToNextOut, isStroke->getRenderCloneCurrentStrokeEndPointIndex());
                    }
                } else {
                    // Render a bezier
                    RotoShapeRenderGL::renderBezier_gl(glContext, glData,
                                                       args.roi,
                                                       isBezier, outputPlane.second, opacity, args.time, args.view, range, divisions, combinedScale, GL_TEXTURE_2D);
                }
            } // useOpenGL
        }   break;
        case eRotoShapeRenderTypeSmear: {

            OSGLContextAttacherPtr contextAttacher;
            if (!glContext->isGPUContext()) {
                // When rendering smear with OSMesa we need to write to the full image bounds and not only the RoI, so re-attach the default framebuffer
                Image::CPUData imageData;
                outputPlane.second->getCPUData(&imageData);

                contextAttacher = OSGLContextAttacher::create(glContext, imageData.bounds.width(), imageData.bounds.height(), imageData.bounds.width(), imageData.ptrs[0]);
            }

            // Ensure that initially everything in the background is the source image
            if (strokeStartPointIndex == 0 && strokeMultiIndex == 0) {

                GetImageOutArgs outArgs;
                GetImageInArgs inArgs(&args.mipMapLevel, &args.proxyScale, &args.roi, &args.backendType);
                inArgs.inputNb = 0;
                if (!getImagePlane(inArgs, &outArgs)) {
                    setPersistentMessage(eMessageTypeError, tr("Failed to fetch source image").toStdString());
                    return eActionStatusFailed;
                }

                ImagePtr bgImage = outArgs.image;


                if (glContext->isGPUContext()) {

                    // Copy the BG image
                    Image::CopyPixelsArgs cpyArgs;
                    cpyArgs.roi = outputPlane.second->getBounds();
                    outputPlane.second->copyPixels(*bgImage, cpyArgs);
                } else {

                    // With OSMesa we cannot re-use the existing output plane as source because mesa clears the framebuffer out upon the first draw
                    // The only option is to draw in a tmp texture that will live for the whole stroke painting life

                    Image::InitStorageArgs initArgs;
                    initArgs.bounds = bgImage->getBounds();
                    initArgs.bitdepth = outputPlane.second->getBitDepth();
                    initArgs.storage = eStorageModeGLTex;
                    initArgs.glContext = glContext;
                    initArgs.textureTarget = GL_TEXTURE_2D;
                    _imp->osmesaSmearTmpTexture = Image::create(initArgs);
                    if (!_imp->osmesaSmearTmpTexture) {
                        return eActionStatusFailed;
                    }

                    // Make sure the texture is ready before rendering the smear
                    GL_CPU::Flush();
                    GL_CPU::Finish();
                }
            } else {
                if (!glContext->isGPUContext() && strokeStartPointIndex == 0) {
                    // Ensure the tmp texture has correct size
                    assert(_imp->osmesaSmearTmpTexture);
                    ActionRetCodeEnum stat = _imp->osmesaSmearTmpTexture->ensureBounds(outputPlane.second->getBounds());
                    if (isFailureRetCode(stat)) {
                        return stat;
                    }
                }
            }

            bool renderedDot;
#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
            // Render with cairo if we need to render on CPU
            if (args.backendType == eRenderBackendTypeCPU) {
                renderedDot = RotoShapeRenderCairo::renderSmear_cairo(args.time, args.view, combinedScale, isStroke, args.roi, outputPlane.second, distNextIn, lastCenterIn, &distToNextOut, &lastCenterOut);
            } else
#endif
            if (args.backendType == eRenderBackendTypeOpenGL || args.backendType == eRenderBackendTypeOSMesa) {

                // Render with OpenGL

                double opacity = rotoItem->getOpacityKnob()->getValueAtTime(args.time, DimIdx(0), args.view);
                ImagePtr dstImage = glContext->isGPUContext() ? outputPlane.second : _imp->osmesaSmearTmpTexture;
                assert(dstImage);
                renderedDot = RotoShapeRenderGL::renderSmear_gl(glContext, glData, args.roi, dstImage, distNextIn, lastCenterIn, isStroke, opacity, args.time, args.view, combinedScale, &distToNextOut, &lastCenterOut);
            }

            // Update the stroke algorithm in output
            if (isDuringPainting) {
                Q_UNUSED(renderedDot);
                nonRenderStroke->updateStrokeData(lastCenterOut, distToNextOut, isStroke->getRenderCloneCurrentStrokeEndPointIndex());
            }

        }   break;
    } // type



    return eActionStatusOK;

} // RotoShapeRenderNode::render



void
RotoShapeRenderNode::purgeCaches()
{
    RotoDrawableItemPtr rotoItem = getAttachedRotoItem();
    if (!rotoItem) {
        return;
    }
#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
    RotoShapeRenderCairo::purgeCaches_cairo(rotoItem);
#endif
}



ActionRetCodeEnum
RotoShapeRenderNode::attachOpenGLContext(TimeValue /*time*/, ViewIdx /*view*/, const RenderScale& /*scale*/, const OSGLContextPtr& glContext, EffectOpenGLContextDataPtr* data)
{
    RotoShapeRenderNodeOpenGLDataPtr ret(new RotoShapeRenderNodeOpenGLData(glContext->isGPUContext()));
    *data = ret;
    return eActionStatusOK;
}

ActionRetCodeEnum
RotoShapeRenderNode::dettachOpenGLContext(const OSGLContextPtr& /*glContext*/, const EffectOpenGLContextDataPtr& data)
{
    RotoShapeRenderNodeOpenGLDataPtr ret = boost::dynamic_pointer_cast<RotoShapeRenderNodeOpenGLData>(data);
    assert(ret);
    ret->cleanup();
    return eActionStatusOK;
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING
#include "moc_RotoShapeRenderNode.cpp"
