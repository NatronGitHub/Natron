/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "RotoSmear.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#include <boost/algorithm/clamp.hpp>

#include <cairo/cairo.h>


#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoContext.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

struct RotoSmearPrivate
{
    QMutex smearDataMutex;
    std::pair<Point, double> lastTickPoint, lastCur;
    double lastDistToNext;

    RotoSmearPrivate()
        : smearDataMutex()
        , lastTickPoint()
        , lastCur()
        , lastDistToNext(0)
    {
        lastCur.first.x = lastCur.first.y = INT_MIN;
    }
};

RotoSmear::RotoSmear(NodePtr node)
    : EffectInstance(node)
    , _imp( new RotoSmearPrivate() )
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

RotoSmear::~RotoSmear()
{
}

void
RotoSmear::addAcceptedComponents(int /*inputNb*/,
                                 std::list<ImagePlaneDesc>* comps)
{
    comps->push_back( ImagePlaneDesc::getRGBAComponents() );
    comps->push_back( ImagePlaneDesc::getRGBComponents() );
    comps->push_back( ImagePlaneDesc::getXYComponents() );
    comps->push_back( ImagePlaneDesc::getAlphaComponents() );
}

void
RotoSmear::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
}

StatusEnum
RotoSmear::getRegionOfDefinition(U64 hash,
                                 double time,
                                 const RenderScale & scale,
                                 ViewIdx view,
                                 RectD* rod)
{
    StatusEnum st = EffectInstance::getRegionOfDefinition(hash, time, scale, view, rod);

    if (st != eStatusOK) {
        rod->x1 = rod->y1 = rod->x2 = rod->y2 = 0.;
    }

    RectD maskRod;
    NodePtr node = getNode();
    try {
        node->getPaintStrokeRoD(time, &maskRod);
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
RotoSmear::isIdentity(double time,
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

static void
renderSmearDot(const unsigned char* maskData,
               const int maskStride,
               const int maskWidth,
               const int maskHeight,
               const Point& prev,
               const Point& next,
               const double brushSizePixels,
               int nComps,
               const ImagePtr& outputImage)
{
    /// First copy the portion of the image around the previous dot into tmpBuf
    RectD prevDotRoD(prev.x - brushSizePixels / 2., prev.y - brushSizePixels / 2., prev.x + brushSizePixels / 2., prev.y + brushSizePixels / 2.);
    RectI prevDotBounds;

    prevDotRoD.toPixelEnclosing(0, outputImage->getPixelAspectRatio(), &prevDotBounds);
    ImagePtr tmpBuf( new Image(outputImage->getComponents(),
                               prevDotRoD,
                               prevDotBounds,
                               0,
                               outputImage->getPixelAspectRatio(),
                               outputImage->getBitDepth(),
                               outputImage->getPremultiplication(),
                               outputImage->getFieldingOrder(),
                               false) );
    tmpBuf->pasteFrom(*outputImage, prevDotBounds, false);

    Image::ReadAccess tmpAcc( tmpBuf.get() );
    Image::WriteAccess wacc( outputImage.get() );
    RectI nextDotBounds;
    nextDotBounds.x1 = next.x - maskWidth / 2;
    nextDotBounds.x2 = next.x + maskWidth / 2;
    nextDotBounds.y1 = next.y - maskHeight / 2;
    nextDotBounds.y2 = next.y + maskHeight / 2;

    const unsigned char* mask_pixels = maskData;
    int yPrev = prevDotBounds.y1;
    for (int y = nextDotBounds.y1; y < nextDotBounds.y2;
         ++y,
         ++yPrev,
         mask_pixels += maskStride) {
        float* dstPixels = (float*)wacc.pixelAt(nextDotBounds.x1, y);
        assert(dstPixels);
        if (!dstPixels) {
            continue;
        }

        int xPrev = prevDotBounds.x1;
        for (int x = nextDotBounds.x1; x < nextDotBounds.x2;
             ++x, ++xPrev,
             dstPixels += nComps) {
            const float* srcPixels = (const float*)tmpAcc.pixelAt(xPrev, yPrev);

            if (srcPixels) {
                float mask_scale = Image::convertPixelDepth<unsigned char, float>(mask_pixels[x - nextDotBounds.x1]);
                float one_minus_mask_scale = 1. - mask_scale;

                for (int k = 0; k < nComps; ++k) {
                    dstPixels[k] = srcPixels[k] * mask_scale + dstPixels[k] * one_minus_mask_scale;
                }
            } else {
            }
        }
    }
} // renderSmearDot

StatusEnum
RotoSmear::render(const RenderActionArgs& args)
{
    NodePtr node = getNode();
    RotoDrawableItemPtr item = node->getAttachedRotoItem();
    RotoStrokeItemPtr stroke = boost::dynamic_pointer_cast<RotoStrokeItem>(item);
    RotoContextPtr context = stroke->getContext();

    assert(context);
    bool duringPainting = isDuringPaintStrokeCreationThreadLocal();
    unsigned int mipmapLevel = Image::getLevelFromScale(args.originalScale.x);
    std::list<std::list<std::pair<Point, double> > > strokes;
    int strokeIndex;
    node->getLastPaintStrokePoints(args.time, mipmapLevel, &strokes, &strokeIndex);


    bool isFirstStrokeTick = false;
    std::pair<Point, double> lastCur;
    if (!duringPainting) {
        QMutexLocker k(&_imp->smearDataMutex);
        _imp->lastCur.first.x = INT_MIN;
        _imp->lastCur.first.y = INT_MIN;
        lastCur = _imp->lastCur;
    } else {
        QMutexLocker k(&_imp->smearDataMutex);
        isFirstStrokeTick = _imp->lastCur.first.x == INT_MIN && _imp->lastCur.first.y == INT_MIN;
        lastCur = _imp->lastCur;
    }


    EffectInstance::ComponentsNeededMap neededComps;
    std::list<ImagePlaneDesc> ptPlanes;
    bool processAll;
    std::bitset<4> processChannels;
    double ptTime;
    int ptView;
    int ptInput;
    getComponentsNeededAndProduced_public(getRenderHash(), args.time, args.view, &neededComps, &ptPlanes, &processAll, &ptTime, &ptView, &processChannels, &ptInput);


    EffectInstance::ComponentsNeededMap::iterator foundBg = neededComps.find(0);
    RectI bgImgRoI;
    double brushHardness = stroke->getBrushHardnessKnob()->getValueAtTime(args.time);
    double brushSize = stroke->getBrushSizeKnob()->getValueAtTime(args.time);
    double brushSpacing = stroke->getBrushSpacingKnob()->getValueAtTime(args.time);
    double opacity = stroke->getOpacity(args.time);
    if (brushSpacing > 0) {
        brushSpacing = std::max(0.05, brushSpacing);
    }

    brushSpacing = std::max(brushSpacing, 0.05);

    double brushSizePixel = brushSize;
    if (mipmapLevel != 0) {
        brushSizePixel = std::max( 1., brushSizePixel / (1 << mipmapLevel) );
    }

    //This is the distance between each dot we render
    double maxDistPerSegment = brushSize * brushSpacing;
    double halfSize = maxDistPerSegment / 2.;
    double writeOnStart = stroke->getBrushVisiblePortionKnob()->getValueAtTime(args.time, 0);
    double writeOnEnd = stroke->getBrushVisiblePortionKnob()->getValueAtTime(args.time, 1);

    //prev is the previously rendered point. On initialization this is just the point in the list prior to cur.
    //cur is the last point we rendered or the point before "it"
    //renderPoint is the final point we rendered, recorded for the next call to render when we are building up the smear
    std::pair<Point, double> prev, cur, renderPoint;
    bool bgInitialized = false;
    CairoImageWrapper imgWrapper;
    if ( !RotoContext::allocateAndRenderSingleDotStroke(brushSizePixel, brushHardness, opacity, imgWrapper) ) {
        return eStatusFailed;
    }


    int maskWidth = cairo_image_surface_get_width(imgWrapper.cairoImg);
    int maskHeight = cairo_image_surface_get_height(imgWrapper.cairoImg);
    int maskStride = cairo_image_surface_get_stride(imgWrapper.cairoImg);
    unsigned char* maskData = cairo_image_surface_get_data(imgWrapper.cairoImg);

    for (std::list<std::list<std::pair<Point, double> > >::const_iterator itStroke = strokes.begin(); itStroke != strokes.end(); ++itStroke) {
        int firstPoint = (int)std::floor( (itStroke->size() * writeOnStart) );
        int endPoint = (int)std::ceil( (itStroke->size() * writeOnEnd) );
        assert( firstPoint >= 0 && firstPoint < (int)itStroke->size() && endPoint > firstPoint && endPoint <= (int)itStroke->size() );

        std::list<std::pair<Point, double> > visiblePortion;
        std::list<std::pair<Point, double> >::const_iterator startingIt = itStroke->begin();
        std::list<std::pair<Point, double> >::const_iterator endingIt = itStroke->begin();
        std::advance(startingIt, firstPoint);
        std::advance(endingIt, endPoint);
        for (std::list<std::pair<Point, double> >::const_iterator it = startingIt; it != endingIt; ++it) {
            visiblePortion.push_back(*it);
        }

        bool didPaint = false;
        double distToNext = 0.;
        ImagePtr bgImg;

        if (strokeIndex == 0) {
            ///For the first multi-stroke, init background
            if ( foundBg != neededComps.end() ) {
                bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, 0, false /*mapToClipPrefs*/, false /*dontUpscale*/, eStorageModeRAM /*returnOpenGLtexture*/, 0 /*textureDepth*/, &bgImgRoI);
            }
        }


        for (std::list<std::pair<ImagePlaneDesc, ImagePtr> >::const_iterator plane = args.outputPlanes.begin();
             plane != args.outputPlanes.end(); ++plane) {
            assert(plane->second->getMipMapLevel() == mipmapLevel);

            distToNext = 0.;
            int nComps = plane->first.getNumComponents();


            if ( !bgImg && !bgInitialized && (strokeIndex == 0) ) {
                plane->second->fillZero(args.roi);
                bgInitialized = true;
                continue;
            }

            // First copy the source image if this is the first stroke tick

            if ( (isFirstStrokeTick || !duringPainting) && !bgInitialized && (strokeIndex == 0) ) {
                // Make sure all areas are black and transparent
                plane->second->fillZero(args.roi);
                plane->second->pasteFrom(*bgImg, args.roi, false);
                bgInitialized = true;
            }

            if ( (brushSpacing == 0) || ( (writeOnEnd - writeOnStart) <= 0. ) || visiblePortion.empty() || (itStroke->size() <= 1) ) {
                continue;
            }


            std::list<std::pair<Point, double> >::iterator it = visiblePortion.begin();


            if (isFirstStrokeTick || !duringPainting) {
                // This is the very first dot we render
                prev = *it;
                ++it;
                renderSmearDot(maskData, maskStride, maskWidth, maskHeight, prev.first, it->first, brushSizePixel, nComps, plane->second);
                didPaint = true;
                renderPoint = *it;
                prev = renderPoint;
                ++it;
                if ( it != visiblePortion.end() ) {
                    cur = *it;
                } else {
                    cur = prev;
                }
            } else {
                QMutexLocker k(&_imp->smearDataMutex);
                prev = _imp->lastTickPoint;
                distToNext = _imp->lastDistToNext;
                renderPoint = prev;
                cur = _imp->lastCur;
            }

            isFirstStrokeTick = false;

            while ( it != visiblePortion.end() ) {
                if ( aborted() ) {
                    return eStatusOK;
                }

                //Render for each point a dot. Spacing is a percentage of brushSize:
                //Spacing at 1 means no dot is overlapping another (so the spacing is in fact brushSize)
                //Spacing at 0 we do not render the stroke

                double dx = it->first.x - cur.first.x;
                double dy = it->first.y - cur.first.y;
                double dist = std::sqrt(dx * dx + dy * dy);

                distToNext += dist;
                if ( (distToNext < maxDistPerSegment) || (dist == 0) ) {
                    //We did not cross maxDistPerSegment pixels yet along the segments since we rendered cur, continue
                    cur = *it;
                    ++it;
                    continue;
                }

                //Find next point by
                double a;
                if (maxDistPerSegment >= dist) {
                    a = (distToNext - dist) == 0 ? (maxDistPerSegment - dist) / dist : (maxDistPerSegment - dist) / (distToNext - dist);
                } else {
                    a = maxDistPerSegment / dist;
                }
                assert(a >= 0 && a <= 1);
                renderPoint.first.x = dx * a + cur.first.x;
                renderPoint.first.y = dy * a + cur.first.y;
                renderPoint.second = (it->second - cur.second) * a + cur.second;

                //prevPoint is the location of the center of the portion of the image we should copy to the renderPoint
                Point prevPoint;
                Point v;
                v.x = renderPoint.first.x - prev.first.x;
                v.y = renderPoint.first.y - prev.first.y;
                double vx = boost::algorithm::clamp(std::abs(v.x / halfSize), 0., .7);
                double vy = boost::algorithm::clamp(std::abs(v.y / halfSize), 0., .7);

                prevPoint.x = prev.first.x + vx * v.x;
                prevPoint.y = prev.first.y + vy * v.y;
                renderSmearDot(maskData, maskStride, maskWidth, maskHeight, prevPoint, renderPoint.first, brushSizePixel, nComps, plane->second);
                didPaint = true;
                prev = renderPoint;
                cur = renderPoint;
                distToNext = 0;
            } // while (it!=visiblePortion.end()) {
        } // for (std::list<std::pair<ImagePlaneDesc,ImagePtr> >::const_iterator plane = args.outputPlanes.begin();

        if (duringPainting && didPaint) {
            QMutexLocker k(&_imp->smearDataMutex);
            _imp->lastTickPoint = prev;
            _imp->lastDistToNext = distToNext;
            _imp->lastCur = cur;
        }
    } // for (std::list<std::list<std::pair<Point,double> > >::const_iterator itStroke = strokes.begin(); itStroke!=strokes.end(); ++itStroke) {
    return eStatusOK;
} // RotoSmear::render

NATRON_NAMESPACE_EXIT
