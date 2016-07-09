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
#include "Engine/Bezier.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/OSGLContext.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoShapeRenderNodePrivate.h"
#include "Engine/RotoShapeRenderCairo.h"
#include "Engine/RotoShapeRenderGL.h"
#include "Engine/ParallelRenderArgs.h"


NATRON_NAMESPACE_ENTER;


RotoShapeRenderNode::RotoShapeRenderNode(NodePtr n)
: EffectInstance(n)
, _imp(new RotoShapeRenderNodePrivate())
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

RotoShapeRenderNode::~RotoShapeRenderNode()
{

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

StatusEnum
RotoShapeRenderNode::getRegionOfDefinition(U64 /*hash*/, double time, const RenderScale & /*scale*/, ViewIdx /*view*/, RectD* rod)
{
   

    NodePtr node = getNode();
    try {
        node->getPaintStrokeRoD(time, rod);
    } catch (...) {
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
    if (!rotoItem || !rotoItem->isActivated(time) || (isBezier && (!isBezier->isCurveFinished() || isBezier->getControlPointsCount() <= 1))) {
        *inputTime = time;
        *inputNb = 0;

        return true;
    }

    node->getPaintStrokeRoD(time, &maskRod);
    
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
    RotoDrawableItemPtr rotoItem = getNode()->getAttachedRotoItem();
    assert(rotoItem);
    if (!rotoItem) {
        return eStatusFailed;
    }

    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(rotoItem.get());
    Bezier* isBezier = dynamic_cast<Bezier*>(rotoItem.get());

    // Check that the item is really activated... it should have been caught in isIdentity otherwise.
    assert(rotoItem->isActivated(args.time) && (!isBezier || (isBezier->isCurveFinished() && ( isBezier->getControlPointsCount() > 1 ))));

    ParallelRenderArgsPtr frameArgs = getParallelRenderArgsTLS();
    OSGLContextPtr glContext;
    AbortableRenderInfoPtr abortInfo;
    if (frameArgs) {
        if (args.useOpenGL) {
            glContext = frameArgs->openGLContext.lock();
        } else {
            glContext = frameArgs->cpuOpenGLContext.lock();
        }
        abortInfo = frameArgs->abortInfo.lock();
    }
    assert(abortInfo && glContext);
    if (!glContext || !abortInfo) {
        setPersistentMessage(eMessageTypeError, tr("An OpenGL context is required to draw with the Roto node").toStdString());
        return eStatusFailed;
    }

    const unsigned int mipmapLevel = Image::getLevelFromScale(args.mappedScale.x);

    // This is the image plane where we render, we are not multiplane so we only render out one plane
    assert(args.outputPlanes.size() == 1);
    const std::pair<ImageComponents,ImagePtr>& outputPlane = args.outputPlanes.front();

    // Check that the supplied output image has the correct storage
    assert((outputPlane.second->getStorageMode() == eStorageModeGLTex && args.useOpenGL) || (outputPlane.second->getStorageMode() != eStorageModeGLTex && !args.useOpenGL));

    // Get per-shape motion blur parameters

    double startTime = args.time, mbFrameStep = 1., endTime = args.time;
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    if (isBezier) {
        int mbType_i = rotoItem->getContext()->getMotionBlurTypeKnob()->getValue();
        bool applyPerShapeMotionBlur = mbType_i == 0;
        if (applyPerShapeMotionBlur) {
            isBezier->getMotionBlurSettings(time, &startTime, &endTime, &mbFrameStep);
        }
    }
#endif


    bool isDuringPainting = isDuringPaintStrokeCreationThreadLocal();
    double distNextIn = 0.;

    // For strokes and open-bezier evaluate them to get the points and their pressure
    // We also compute the bounding box of the item taking into account the motion blur
    std::list<std::list<std::pair<Point, double> > > strokes;
    if (isStroke) {

        if (isDuringPainting) {
            RectD lastStrokeMovementBbox;
            std::list<std::pair<Point, double> > lastStrokePoints;
            getApp()->getRenderStrokeData(&lastStrokeMovementBbox, &lastStrokePoints, &distNextIn);

            // When drawing we must always write to the same buffer
            assert(getNode()->getPaintBuffer() == outputPlane.second);

            int pot = 1 << mipmapLevel;
            if (mipmapLevel == 0) {
                if (!lastStrokePoints.empty()) {
                    strokes.push_back(lastStrokePoints);
                }
            } else {
                std::list<std::pair<Point, double> > toScalePoints;
                for (std::list<std::pair<Point, double> >::const_iterator it = lastStrokePoints.begin(); it != lastStrokePoints.end(); ++it) {
                    std::pair<Point, double> p = *it;
                    p.first.x /= pot;
                    p.first.y /= pot;
                    toScalePoints.push_back(p);
                }
                if (!toScalePoints.empty()) {
                    strokes.push_back(toScalePoints);
                }
            }
        } else {
            isStroke->evaluateStroke(mipmapLevel, args.time, &strokes, 0);

        }

        if (strokes.empty()) {
            return eStatusOK;
        }

    } else if (isBezier) {

        if ( isBezier->isOpenBezier() ) {
            std::vector<std::vector< ParametricPoint> > decastelJauPolygon;
            isBezier->evaluateAtTime_DeCasteljau_autoNbPoints(false, args.time, mipmapLevel, &decastelJauPolygon, 0);
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

    // We do not support tiles so the renderWindow passed to render should be the RoD in pixels of the shape

    //  Attach the OpenGL context
    boost::scoped_ptr<Image::WriteAccess> wacc;
    boost::scoped_ptr<OSGLContextAttacher> contextLocker;
    if (glContext->isGPUContext()) {
        assert(args.useOpenGL);
        contextLocker.reset(new OSGLContextAttacher(glContext, abortInfo
#ifdef DEBUG
                                                    , args.time
#endif
                                                    ));
    } else {

#ifndef ROTO_ENABLE_CPU_RENDER_USES_CAIRO
        assert(!args.useOpenGL);
        wacc.reset(new Image::WriteAccess(outputPlane.second.get()));
        unsigned char* data = wacc->pixelAt(args.roi.x1, args.roi.y1);
        assert(data);
        contextLocker.reset(new OSGLContextAttacher(glContext, abortInfo
#ifdef DEBUG
                                                    , args.time
#endif
                                                    , args.roi.width()
                                                    , args.roi.height()
                                                    , data));
#endif

    }
    if (contextLocker) {
        contextLocker->attach();
    }

    // Now we are good to start rendering
#ifdef ROTO_ENABLE_CPU_RENDER_USES_CAIRO
    if (!args.useOpenGL) {
        double distToNextOut = RotoShapeRenderCairo::renderMaskInternal_cairo(rotoItem, args.roi, outputPlane.first, startTime, endTime, mbFrameStep, args.time, outputPlane.second->getBitDepth(), mipmapLevel, isDuringPainting, distNextIn, strokes, outputPlane.second);
        if (isDuringPainting) {
            getApp()->updateStrokeData(distToNextOut);
        }
    } else {
#endif

        boost::shared_ptr<RotoShapeRenderNodeOpenGLData> glData = boost::dynamic_pointer_cast<RotoShapeRenderNodeOpenGLData>(args.glContextData);
        assert(glData);

        double shapeColor[3];
        rotoItem->getColor(args.time, shapeColor);
        double opacity = rotoItem->getOpacity(args.time);

        if ( isStroke || !isBezier || ( isBezier && isBezier->isOpenBezier() ) ) {
            bool doBuildUp = rotoItem->getBuildupKnob()->getValueAtTime(args.time);
            double distToNextOut = RotoShapeRenderGL::renderStroke_gl(glContext, glData,
#ifndef NDEBUG
                                                                      args.roi,
#endif
                                                                      outputPlane.second->getGLTextureTarget(), strokes, distNextIn, isStroke, doBuildUp, opacity, args.time, mipmapLevel);
            if (isDuringPainting) {
                getApp()->updateStrokeData(distToNextOut);
            }
        } else {
            RotoShapeRenderGL::renderBezier_gl(glContext, glData,
#ifndef NDEBUG
                                               args.roi,
#endif
                                               isBezier, opacity, args.time, startTime, endTime, mbFrameStep, mipmapLevel, outputPlane.second->getGLTextureTarget());
        }

#ifdef ROTO_ENABLE_CPU_RENDER_USES_CAIRO
    }
#endif

    return eStatusOK;

} // RotoShapeRenderNode::render



void
RotoShapeRenderNode::purgeCaches()
{
    RotoDrawableItemPtr rotoItem = getNode()->getAttachedRotoItem();
    assert(rotoItem);
    if (!rotoItem) {
        return;
    }
#ifdef ROTO_ENABLE_CPU_RENDER_USES_CAIRO
    RotoShapeRenderCairo::purgeCaches_cairo(rotoItem);
#endif
}



StatusEnum
RotoShapeRenderNode::attachOpenGLContext(const OSGLContextPtr& glContext, EffectOpenGLContextDataPtr* data)
{
    boost::shared_ptr<RotoShapeRenderNodeOpenGLData> ret(new RotoShapeRenderNodeOpenGLData(glContext->isGPUContext()));
    *data = ret;
    return eStatusOK;
}

StatusEnum
RotoShapeRenderNode::dettachOpenGLContext(const OSGLContextPtr& /*glContext*/, const EffectOpenGLContextDataPtr& data)
{
    boost::shared_ptr<RotoShapeRenderNodeOpenGLData> ret = boost::dynamic_pointer_cast<RotoShapeRenderNodeOpenGLData>(data);
    assert(ret);
    ret->cleanup();
    return eStatusOK;
}

NATRON_NAMESPACE_EXIT;