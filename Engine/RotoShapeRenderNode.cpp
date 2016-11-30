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

#include "RotoShapeRenderNode.h"


#include "Engine/AppInstance.h"
#include "Engine/BezierCP.h"
#include "Engine/Bezier.h"
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
#include "Engine/ParallelRenderArgs.h"


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
    PluginPtr ret = Plugin::create((void*)RotoShapeRenderNode::create, PLUGINID_NATRON_ROTOSHAPE, "RotoShape", 1, 0, grouping);
    ret->setProperty<bool>(kNatronPluginPropIsInternalOnly, true);
    ret->setProperty<int>(kNatronPluginPropOpenGLSupport, (int)ePluginOpenGLRenderSupportYes);
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafeFrame);
    return ret;
}


RotoShapeRenderNode::RotoShapeRenderNode(NodePtr n)
: EffectInstance(n)
, _imp(new RotoShapeRenderNodePrivate())
{
    setSupportsRenderScaleMaybe(eSupportsYes);
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
                                 std::list<ImageComponents>* comps)
{
    comps->push_back( ImageComponents::getRGBAComponents() );
    comps->push_back( ImageComponents::getRGBComponents() );
    comps->push_back( ImageComponents::getXYComponents() );
    comps->push_back( ImageComponents::getAlphaComponents() );
}

void
RotoShapeRenderNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
}

void
RotoShapeRenderNode::initializeKnobs()
{
    KnobPagePtr page = AppManager::createKnob<KnobPage>(shared_from_this(), tr("Controls"));
    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(shared_from_this(), tr(kRotoShapeRenderNodeParamOutputComponentsLabel));
        param->setName(kRotoShapeRenderNodeParamOutputComponents);
        {
            std::vector<std::string> options;
            options.push_back(kRotoShapeRenderNodeParamOutputComponentsRGBA);
            options.push_back(kRotoShapeRenderNodeParamOutputComponentsAlpha);
            param->populateChoices(options);
        }
        page->addKnob(param);
        _imp->outputComponents = param;
    }
    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(shared_from_this(), tr(kRotoShapeRenderNodeParamTypeLabel));
        param->setName(kRotoShapeRenderNodeParamType);
        {
            std::vector<std::string> options;
            options.push_back(kRotoShapeRenderNodeParamTypeSolid);
            options.push_back(kRotoShapeRenderNodeParamTypeSmear);
            param->populateChoices(options);
        }
        page->addKnob(param);
        _imp->renderType = param;
    }
}

void
RotoShapeRenderNode::appendToHash(double time, ViewIdx view, Hash64* hash)
{
    // Append the position of the curve at the given time
    RotoDrawableItemPtr item = getNode()->getAttachedRotoItem();
    assert(item);

    RotoStrokeItemPtr isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(item);
    BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(item);
    if (isBezier) {
        U64 bh = isBezier->computeHash(time, view);
        hash->append(bh);

    } else if (isStroke) {
        U64 sh = isStroke->computeHash(time, view);
        hash->append(sh);
        
    }

    
    EffectInstance::appendToHash(time, view, hash);

}

StatusEnum
RotoShapeRenderNode::getPreferredMetaDatas(NodeMetadata& metadata)
{

#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
    int index = _imp->outputComponents.lock()->getValue();
    ImageComponents comps = index == 0 ? ImageComponents::getRGBAComponents() : ImageComponents::getAlphaComponents();
#else
    const ImageComponents& comps = ImageComponents::getRGBAComponents();
#endif
    metadata.setImageComponents(-1, comps);
    metadata.setImageComponents(0, comps);
    metadata.setIsContinuous(true);
    return eStatusOK;
}

StatusEnum
RotoShapeRenderNode::getRegionOfDefinition(double time, const RenderScale & scale, ViewIdx view, RectD* rod)
{
   

    StatusEnum st = EffectInstance::getRegionOfDefinition(time, scale, view, rod);
    if (st != eStatusOK && st != eStatusReplyDefault) {
        rod->x1 = rod->y1 = rod->x2 = rod->y2 = 0.;
    }

    NodePtr node = getNode();
    RectD maskRod;
    try {
        RotoDrawableItemPtr item = getNode()->getAttachedRotoItem();
        maskRod = item->getBoundingBox(time, view);
    } catch (...) {
    }
    if ( rod->isNull() ) {
        *rod = maskRod;
    } else {
        rod->merge(maskRod);
    }
    return eStatusOK;

}

bool
RotoShapeRenderNode::isIdentity(double time,
                const RenderScale & scale,
                const RectI & roi,
                ViewIdx view,
                double* inputTime,
                ViewIdx* inputView,
                int* inputNb)
{
    *inputView = view;
    RectD maskRod;
    NodePtr node = getNode();


    RotoDrawableItemPtr rotoItem = node->getAttachedRotoItem();
    assert(rotoItem);
    Bezier* isBezier = dynamic_cast<Bezier*>(rotoItem.get());
    if (!rotoItem || !rotoItem->isActivated(time, view) || (isBezier && (!isBezier->isCurveFinished(view) || isBezier->getControlPointsCount(view) <= 1))) {
        *inputTime = time;
        *inputNb = 0;

        return true;
    }

    RotoDrawableItemPtr item = getNode()->getAttachedRotoItem();
    maskRod = item->getBoundingBox(time, view);

    RectI maskPixelRod;
    maskRod.toPixelEnclosing(scale, getAspectRatio(-1), &maskPixelRod);
    if ( !maskPixelRod.intersects(roi) ) {
        *inputTime = time;
        *inputNb = 0;
        
        return true;
    }
    
    return false;
}



StatusEnum
RotoShapeRenderNode::render(const RenderActionArgs& args)
{

#if !defined(ROTO_SHAPE_RENDER_ENABLE_CAIRO) && !defined(HAVE_OSMESA)
    setPersistentMessage(eMessageTypeError, tr("Roto requires either OSMesa (CONFIG += enable-osmesa) or Cairo (CONFIG += enable-cairo) in order to render on CPU").toStdString());
    return eStatusFailed;
#endif

#if !defined(ROTO_SHAPE_RENDER_ENABLE_CAIRO)
    if (!args.useOpenGL) {
        setPersistentMessage(eMessageTypeError, tr("An OpenGL context is required to draw with the Roto node. This might be because you are trying to render an image too big for OpenGL.").toStdString());
        return eStatusFailed;
    }
#endif

    RotoDrawableItemPtr rotoItem = getNode()->getAttachedRotoItem();
    assert(rotoItem);
    if (!rotoItem) {
        return eStatusFailed;
    }

    RotoShapeRenderTypeEnum type = (RotoShapeRenderTypeEnum)_imp->renderType.lock()->getValue();

    RotoStrokeItemPtr isStroke = toRotoStrokeItem(rotoItem);
    BezierPtr isBezier = toBezier(rotoItem);

    if (type == eRotoShapeRenderTypeSmear && !isStroke) {
        return eStatusFailed;
    }

    // Check that the item is really activated... it should have been caught in isIdentity otherwise.
    assert(rotoItem->isActivated(args.time, args.view) && (!isBezier || (isBezier->isCurveFinished(args.view) && ( isBezier->getControlPointsCount(args.view) > 1 ))));

    ParallelRenderArgsPtr frameArgs = getParallelRenderArgsTLS();
    const OSGLContextPtr& glContext = args.glContext;
    AbortableRenderInfoPtr abortInfo;
    if (frameArgs) {
        abortInfo = frameArgs->abortInfo.lock();
    }
    assert( abortInfo && (!args.useOpenGL || glContext) );
    if (args.useOpenGL && (!glContext || !abortInfo)) {
        setPersistentMessage(eMessageTypeError, tr("An OpenGL context is required to draw with the Roto node").toStdString());
        return eStatusFailed;
    }

    const unsigned int mipmapLevel = Image::getLevelFromScale(args.mappedScale.x);

    // This is the image plane where we render, we are not multiplane so we only render out one plane
    assert(args.outputPlanes.size() == 1);
    const std::pair<ImageComponents,ImagePtr>& outputPlane = args.outputPlanes.front();

    bool isDuringPainting = isStroke && isStroke->isPaintBuffersEnabled();
    double distNextIn = 0.;
    Point lastCenterIn = { INT_MIN, INT_MIN };
    int strokePointIndex = 0;
    int strokeMultiIndex = 0;

    // For strokes and open-bezier evaluate them to get the points and their pressure
    // We also compute the bounding box of the item taking into account the motion blur
    std::list<std::list<std::pair<Point, double> > > strokes;
    if (isStroke) {
        isStroke->evaluateStroke(mipmapLevel, args.time, args.view, &strokes, 0);
        strokePointIndex = isStroke->getRenderCloneCurrentStrokeStartPointIndex();
        strokeMultiIndex = isStroke->getRenderCloneCurrentStrokeIndex();
        isStroke->getStrokeState(&lastCenterIn, &distNextIn);

        if (strokes.empty()) {
            return eStatusOK;
        }

    } else if (isBezier) {

        if ( isBezier->isOpenBezier() ) {
            std::vector<std::vector< ParametricPoint> > decastelJauPolygon;
            isBezier->evaluateAtTime_DeCasteljau_autoNbPoints(args.time, args.view, mipmapLevel, &decastelJauPolygon, 0);
            std::list<std::pair<Point, double> > points;
            for (std::vector<std::vector< ParametricPoint> > ::iterator it = decastelJauPolygon.begin(); it != decastelJauPolygon.end(); ++it) {
                for (std::vector< ParametricPoint>::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
                    Point p = {it2->x, it2->y};
                    points.push_back( std::make_pair(p, 1.) );
                }
            }
            if ( !points.empty() ) {
                strokes.push_back(points);
            }

            if (strokes.empty()) {
                return eStatusOK;
            }

        }
    }

    // Now we are good to start rendering

    double distToNextOut = 0.;
    Point lastCenterOut;

    RotoShapeRenderNodeOpenGLDataPtr glData;
    if (args.glContextData) {
        glData = boost::dynamic_pointer_cast<RotoShapeRenderNodeOpenGLData>(args.glContextData);
        assert(glData);
    }


    switch (type) {
        case eRotoShapeRenderTypeSolid: {

            double startTime = args.time, mbFrameStep = 1., endTime = args.time;

            if (isBezier) {
                RotoPaintPtr rotoPaintNode;
                KnobItemsTablePtr model = isBezier->getModel();
                if (model) {
                    rotoPaintNode = toRotoPaint(model->getNode()->getEffectInstance());
                }
            }

#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
            if (!args.useOpenGL) {
                RotoShapeRenderCairo::renderMaskInternal_cairo(rotoItem, args.roi, outputPlane.first, startTime, endTime, mbFrameStep, args.time, args.view, outputPlane.second->getBitDepth(), mipmapLevel, isDuringPainting, distNextIn, lastCenterIn, strokes, outputPlane.second, &distToNextOut, &lastCenterOut);
                if (isDuringPainting && isStroke) {
                    isStroke->updateStrokeData(lastCenterOut, distToNextOut);
                }
            }
#endif
            if (args.useOpenGL) {
                double shapeColor[3];
                {
                    KnobColorPtr colorKnob = rotoItem->getColorKnob();
                    if (!colorKnob) {
                        shapeColor[0] = shapeColor[1] = shapeColor[2] = 1.;
                    } else {
                        for (int i = 0; i < 3; ++i) {
                            shapeColor[i] = colorKnob->getValueAtTime(args.time, DimIdx(i), args.view);
                        }
                    }
                }
                
                double opacity = rotoItem->getOpacityKnob()->getValueAtTime(args.time, DimIdx(0), args.view);

                if ( isStroke || ( isBezier && isBezier->isOpenBezier() ) ) {
                    bool doBuildUp = isStroke->getBuildupKnob()->getValueAtTime(args.time, DimIdx(0), args.view);
                    RotoShapeRenderGL::renderStroke_gl(glContext, glData, args.roi, outputPlane.second, strokes, distNextIn, lastCenterIn, isStroke, doBuildUp, opacity, args.time, args.view, mipmapLevel, &distToNextOut, &lastCenterOut);
                    if (isDuringPainting && isStroke) {
                        isStroke->updateStrokeData(lastCenterOut, distToNextOut);
                    }
                } else {
                    RotoShapeRenderGL::renderBezier_gl(glContext, glData,
                                                       args.roi,
                                                       isBezier, opacity, args.time, args.view, startTime, endTime, mbFrameStep, mipmapLevel, outputPlane.second->getGLTextureTarget());
                }
            }
        }   break;
        case eRotoShapeRenderTypeSmear: {

            OSGLContextAttacherPtr contextLocker;
            if (!glContext->isGPUContext()) {
                // When rendering smear with OSMesa we need to write to the full image bounds and not only the RoI, so re-attach the default framebuffer
                RectI bounds = outputPlane.second->getBounds();
                Image::WriteAccess outputWriteAccess(outputPlane.second.get());
                unsigned char* data = outputWriteAccess.pixelAt(bounds.x1, bounds.y1);
                assert(data);
                contextLocker.reset(new OSGLContextAttacher(glContext, frameArgs->abortInfo.lock(),
#ifdef DEBUG
                                                            args.time,
#endif
                                                            bounds.width(), bounds.height(), bounds.width(), data));
                contextLocker->attach();
            }

            // Ensure that initially everything in the background is the source image
            if (strokePointIndex == 0 && strokeMultiIndex == 0) {

                RectI bgImgRoI;
                ImagePtr bgImg = getImage(0 /*inputNb*/, args.time, args.mappedScale, args.view, 0 /*optionalBounds*/, 0 /*optionalLayer*/, false /*mapToClipPrefs*/, false /*dontUpscale*/, (args.useOpenGL && glContext->isGPUContext()) ? eStorageModeGLTex : eStorageModeRAM /*returnOpenGLtexture*/, 0 /*textureDepth*/, &bgImgRoI);

                if (!bgImg) {
                    setPersistentMessage(eMessageTypeError, tr("Failed to fetch source image").toStdString());
                    return eStatusFailed;
                }

                // With OSMesa we cannot re-use the existing output plane as source because mesa clears the framebuffer out upon the first draw
                // The only option is to draw in a tmp texture that will live for the whole stroke painting life
                if (!glContext->isGPUContext()) {
                    const RectD& rod = outputPlane.second->getRoD();
                    RectI pixelRoD;
                    rod.toPixelEnclosing(0, outputPlane.second->getPixelAspectRatio(), &pixelRoD);
                    _imp->osmesaSmearTmpTexture = EffectInstance::convertRAMImageRoIToOpenGLTexture(bgImg, pixelRoD, glContext);
                    // Make sure the texture is ready before rendering the smear
                    GL_CPU::Flush();
                    GL_CPU::Finish();
                } else {
                    outputPlane.second->pasteFrom(*bgImg, outputPlane.second->getBounds(), false, glContext);
                }
            } else {
                if (strokePointIndex == 0 && !glContext->isGPUContext()) {
                    // Ensure the tmp texture has correct size
                    assert(_imp->osmesaSmearTmpTexture);
                    const RectD& rod = outputPlane.second->getRoD();
                    RectI pixelRoD;
                    rod.toPixelEnclosing(0, outputPlane.second->getPixelAspectRatio(), &pixelRoD);
                    _imp->osmesaSmearTmpTexture->ensureBounds(glContext, pixelRoD);
                }
            }

            bool renderedDot;
#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
            if (!args.useOpenGL) {
                renderedDot = RotoShapeRenderCairo::renderSmear_cairo(args.time, args.view, mipmapLevel, isStroke, args.roi, outputPlane.second, distNextIn, lastCenterIn, strokes, &distToNextOut, &lastCenterOut);
            }
#endif
            if (args.useOpenGL) {
                double opacity = rotoItem->getOpacityKnob()->getValueAtTime(args.time, DimIdx(0), args.view);
                ImagePtr dstImage = glContext->isGPUContext() ? outputPlane.second : _imp->osmesaSmearTmpTexture;
                assert(dstImage);
                renderedDot = RotoShapeRenderGL::renderSmear_gl(glContext, glData, args.roi, dstImage, strokes, distNextIn, lastCenterIn, isStroke, opacity, args.time, args.view, mipmapLevel, &distToNextOut, &lastCenterOut);
            }

            if (isDuringPainting) {
                Q_UNUSED(renderedDot);
                /*if (!renderedDot) {
                    lastCenterOut = lastCenterIn;
                    distToNextOut = distNextIn;
                }*/
                isStroke->updateStrokeData(lastCenterOut, distToNextOut);
            }

        }   break;
    } // type



    return eStatusOK;

} // RotoShapeRenderNode::render



void
RotoShapeRenderNode::purgeCaches()
{
    RotoDrawableItemPtr rotoItem = getNode()->getAttachedRotoItem();
    if (!rotoItem) {
        return;
    }
#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
    RotoShapeRenderCairo::purgeCaches_cairo(rotoItem);
#endif
}



StatusEnum
RotoShapeRenderNode::attachOpenGLContext(const OSGLContextPtr& glContext, EffectOpenGLContextDataPtr* data)
{
    RotoShapeRenderNodeOpenGLDataPtr ret(new RotoShapeRenderNodeOpenGLData(glContext->isGPUContext()));
    *data = ret;
    return eStatusOK;
}

StatusEnum
RotoShapeRenderNode::dettachOpenGLContext(const OSGLContextPtr& /*glContext*/, const EffectOpenGLContextDataPtr& data)
{
    RotoShapeRenderNodeOpenGLDataPtr ret = boost::dynamic_pointer_cast<RotoShapeRenderNodeOpenGLData>(data);
    assert(ret);
    ret->cleanup();
    return eStatusOK;
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING
#include "moc_RotoShapeRenderNode.cpp"
