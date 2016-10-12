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

#ifndef ROTOCONTEXTPRIVATE_H
#define ROTOCONTEXTPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <map>
#include <string>
#include <vector>
#include <limits>
#include <sstream>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QReadWriteLock>
#include <QtCore/QWaitCondition>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/AppManager.h"
#include "Engine/BezierCP.h"
#include "Engine/Bezier.h"
#include "Engine/Curve.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/MergingEnum.h"
#include "Engine/Node.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoPaint.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"




#define kRotoScriptNameHint "Script-name of the item for Python scripts. It cannot be edited."

#define kRotoLabelHint "Label of the layer or curve"

#define kRotoNameHint "Name of the layer or curve."


NATRON_NAMESPACE_ENTER;

struct BezierPrivate
{
    BezierCPs points; //< the control points of the curve
    BezierCPs featherPoints; //< the feather points, the number of feather points must equal the number of cp.

    //updated whenever the Bezier is edited, this is used to determine if a point lies inside the bezier or not
    //it has a value for each keyframe
    mutable std::map<double, bool> isClockwiseOriented;
    mutable bool isClockwiseOrientedStatic; //< used when the bezier has no keyframes
    mutable std::map<double, bool> guiIsClockwiseOriented;
    mutable bool guiIsClockwiseOrientedStatic; //< used when the bezier has no keyframes
    bool autoRecomputeOrientation; // when true, orientation will be computed automatically on editing
    bool finished; //< when finished is true, the last point of the list is connected to the first point of the list.
    bool isOpenBezier;
    mutable QMutex guiCopyMutex;
    bool mustCopyGui;

    BezierPrivate(bool isOpenBezier)
        : points()
        , featherPoints()
        , isClockwiseOriented()
        , isClockwiseOrientedStatic(false)
        , guiIsClockwiseOriented()
        , guiIsClockwiseOrientedStatic(false)
        , autoRecomputeOrientation(true)
        , finished(false)
        , isOpenBezier(isOpenBezier)
        , guiCopyMutex()
        , mustCopyGui(false)
    {
    }

    void setMustCopyGuiBezier(bool copy)
    {
        QMutexLocker k(&guiCopyMutex);

        mustCopyGui = copy;
    }

    bool hasKeyframeAtTime(bool useGuiCurves,
                           double time) const
    {
        // PRIVATE - should not lock

        if ( points.empty() ) {
            return false;
        } else {
            KeyFrame k;

            return points.front()->hasKeyFrameAtTime(useGuiCurves, time);
        }
    }

    void getKeyframeTimes(bool useGuiCurves,
                          std::set<double>* times) const
    {
        // PRIVATE - should not lock

        if ( points.empty() ) {
            return;
        }
        points.front()->getKeyframeTimes(useGuiCurves, times);
    }

    BezierCPs::const_iterator atIndex(int index) const
    {
        // PRIVATE - should not lock

        if ( ( index >= (int)points.size() ) || (index < 0) ) {
            throw std::out_of_range("RotoSpline::atIndex: non-existent control point");
        }

        BezierCPs::const_iterator it = points.begin();
        std::advance(it, index);

        return it;
    }

    BezierCPs::iterator atIndex(int index)
    {
        // PRIVATE - should not lock

        if ( ( index >= (int)points.size() ) || (index < 0) ) {
            throw std::out_of_range("RotoSpline::atIndex: non-existent control point");
        }

        BezierCPs::iterator it = points.begin();
        std::advance(it, index);

        return it;
    }

    BezierCPs::const_iterator findControlPointNearby(double x,
                                                     double y,
                                                     double acceptance,
                                                     double time,
                                                     ViewIdx view,
                                                     const Transform::Matrix3x3& transform,
                                                     int* index) const
    {
        // PRIVATE - should not lock
        int i = 0;

        for (BezierCPs::const_iterator it = points.begin(); it != points.end(); ++it, ++i) {
            Transform::Point3D p;
            p.z = 1;
            (*it)->getPositionAtTime(true, time, view, &p.x, &p.y);
            p = Transform::matApply(transform, p);
            if ( ( p.x >= (x - acceptance) ) && ( p.x <= (x + acceptance) ) && ( p.y >= (y - acceptance) ) && ( p.y <= (y + acceptance) ) ) {
                *index = i;

                return it;
            }
        }

        return points.end();
    }

    BezierCPs::const_iterator findFeatherPointNearby(double x,
                                                     double y,
                                                     double acceptance,
                                                     double time,
                                                     ViewIdx view,
                                                     const Transform::Matrix3x3& transform,
                                                     int* index) const
    {
        // PRIVATE - should not lock
        int i = 0;

        for (BezierCPs::const_iterator it = featherPoints.begin(); it != featherPoints.end(); ++it, ++i) {
            Transform::Point3D p;
            p.z = 1;
            (*it)->getPositionAtTime(true, time, view, &p.x, &p.y);
            p = Transform::matApply(transform, p);
            if ( ( p.x >= (x - acceptance) ) && ( p.x <= (x + acceptance) ) && ( p.y >= (y - acceptance) ) && ( p.y <= (y + acceptance) ) ) {
                *index = i;

                return it;
            }
        }

        return featherPoints.end();
    }
};

class RotoLayer;
struct RotoItemPrivate
{
    boost::weak_ptr<RotoContext> context;
    std::string scriptName, label;
    boost::weak_ptr<RotoLayer> parentLayer;

    ////This controls whether the item (and all its children if it is a layer)
    ////should be visible/rendered or not at any time.
    ////This is different from the "activated" knob for RotoDrawableItem's which in that
    ////case allows to define a life-time
    KnobBoolWPtr globallyActivated;

    ////A locked item should not be modifiable by the GUI
    bool locked;

    RotoItemPrivate(const RotoContextPtr context,
                    const std::string & n,
                    const RotoLayerPtr& parent)
        : context(context)
        , scriptName(n)
        , label(n)
        , parentLayer(parent)
        , globallyActivated()
        , locked(false)
    {
    }
};

typedef std::list< RotoItemPtr > RotoItems;

struct RotoLayerPrivate
{
    RotoItems items;

    RotoLayerPrivate()
        : items()
    {
    }
};

struct RotoDrawableItemPrivate
{
    Q_DECLARE_TR_FUNCTIONS(RotoDrawableItem)

public:
    /*
     * The effect node corresponds to the following given the selected tool:
     * Stroke= RotoOFX
     * Blur = BlurCImg
     * Clone = TransformOFX
     * Sharpen = SharpenCImg
     * Smear = hand made tool
     * Reveal = Merge(over) with A being color type and B the tree upstream
     * Dodge/Burn = Merge(color-dodge/color-burn) with A being the tree upstream and B the color type
     *
     * Each effect is followed by a merge (except for the ones that use a merge) with the user given operator
     * onto the previous tree upstream of the effectNode.
     */
    NodePtr effectNode, maskNode;
    NodePtr mergeNode;
    NodePtr timeOffsetNode, frameHoldNode;
    KnobColorWPtr overlayColor; //< the color the shape overlay should be drawn with, defaults to smooth red
    KnobDoubleWPtr opacity; //< opacity of the rendered shape between 0 and 1
    KnobDoubleWPtr feather; //< number of pixels to add to the feather distance (from the feather point), between -100 and 100
    KnobDoubleWPtr featherFallOff; //< the rate of fall-off for the feather, between 0 and 1,  0.5 meaning the
                                                  //alpha value is half the original value when at half distance from the feather distance
    KnobChoiceWPtr fallOffRampType;
    KnobChoiceWPtr lifeTime;
    KnobBoolWPtr activated; //< should the curve be visible/rendered ? (animable)
    KnobIntWPtr lifeTimeFrame;
#ifdef NATRON_ROTO_INVERTIBLE
    KnobBoolWPtr inverted; //< invert the rendering
#endif
    KnobColorWPtr color;
    KnobChoiceWPtr compOperator;
    KnobDoubleWPtr translate;
    KnobDoubleWPtr rotate;
    KnobDoubleWPtr scale;
    KnobBoolWPtr scaleUniform;
    KnobDoubleWPtr skewX;
    KnobDoubleWPtr skewY;
    KnobChoiceWPtr skewOrder;
    KnobDoubleWPtr center;
    KnobDoubleWPtr extraMatrix;
    KnobDoubleWPtr brushSize;
    KnobDoubleWPtr brushSpacing;
    KnobDoubleWPtr brushHardness;
    KnobDoubleWPtr effectStrength;
    KnobBoolWPtr pressureOpacity, pressureSize, pressureHardness, buildUp;
    KnobDoubleWPtr visiblePortion; // [0,1] by default
    KnobDoubleWPtr cloneTranslate;
    KnobDoubleWPtr cloneRotate;
    KnobDoubleWPtr cloneScale;
    KnobBoolWPtr cloneScaleUniform;
    KnobDoubleWPtr cloneSkewX;
    KnobDoubleWPtr cloneSkewY;
    KnobChoiceWPtr cloneSkewOrder;
    KnobDoubleWPtr cloneCenter;
    KnobChoiceWPtr cloneFilter;
    KnobBoolWPtr cloneBlackOutside;
    KnobChoiceWPtr sourceColor;
    KnobIntWPtr timeOffset;
    KnobChoiceWPtr timeOffsetMode;

#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    KnobDoublePtr motionBlur;
    KnobDoublePtr shutter;
    KnobChoicePtr shutterType;
    KnobDoublePtr customOffset;
#endif

    RotoDrawableItemPrivate()
    : effectNode()
    , maskNode()
    , mergeNode()
    , timeOffsetNode()
    , frameHoldNode()
    {

    }

    ~RotoDrawableItemPrivate()
    {
    }
};

struct RotoStrokeItemPrivate
{
    RotoStrokeType type;
    bool finished;
    struct StrokeCurves
    {
        CurvePtr xCurve, yCurve, pressureCurve;
    };

    /**
     * @brief A list of all storkes contained in this item. Basically each time penUp() is called it makes a new stroke
     **/
    std::vector<StrokeCurves> strokes;
    double curveT0; // timestamp of the first point in curve
    double lastTimestamp;
    RectD bbox;
    RectD wholeStrokeBboxWhilePainting;
    mutable QMutex strokeDotPatternsMutex;
    std::vector<cairo_pattern_t*> strokeDotPatterns;

    OSGLContextWPtr drawingGlCpuContext,drawingGlGpuContext;

    RotoStrokeItemPrivate(RotoStrokeType type)
        : type(type)
        , finished(false)
        , strokes()
        , curveT0(0)
        , lastTimestamp(0)
        , bbox()
        , wholeStrokeBboxWhilePainting()
        , strokeDotPatternsMutex()
        , strokeDotPatterns()
        , drawingGlCpuContext()
        , drawingGlGpuContext()
    {
        bbox.x1 = std::numeric_limits<double>::infinity();
        bbox.x2 = -std::numeric_limits<double>::infinity();
        bbox.y1 = std::numeric_limits<double>::infinity();
        bbox.y2 = -std::numeric_limits<double>::infinity();
    }
};

struct RotoContextPrivate
{
    Q_DECLARE_TR_FUNCTIONS(RotoContext)

public:
    mutable QMutex rotoContextMutex;

    /*
     * We have chosen to disable rotopainting and roto shapes from the same RotoContext because the rendering techniques are
     * very much differents. The rotopainting systems requires an entire compositing tree held inside whereas the rotoshapes
     * are rendered and optimized by Cairo internally.
     */
    bool isPaintNode;
    std::list< RotoLayerPtr > layers;
    bool autoKeying;
    bool rippleEdit;
    bool featherLink;
    bool isCurrentlyLoading;
    NodeWPtr node;

    ///These are knobs that take the value of the selected splines info.
    ///Their value changes when selection changes.
    KnobDoubleWPtr opacity;
    KnobDoubleWPtr feather;
    KnobDoubleWPtr featherFallOff;
    KnobChoiceWPtr fallOffType;
    KnobChoiceWPtr lifeTime;
    KnobBoolWPtr activated; //<allows to disable a shape on a specific frame range
    KnobIntWPtr lifeTimeFrame;

#ifdef NATRON_ROTO_INVERTIBLE
    KnobBoolWPtr inverted;
#endif
    KnobColorWPtr colorKnob;
    KnobDoubleWPtr brushSizeKnob;
    KnobDoubleWPtr brushSpacingKnob;
    KnobDoubleWPtr brushHardnessKnob;
    KnobDoubleWPtr brushEffectKnob;
    KnobSeparatorWPtr pressureLabelKnob;
    KnobBoolWPtr pressureOpacityKnob;
    KnobBoolWPtr pressureSizeKnob;
    KnobBoolWPtr pressureHardnessKnob;
    KnobBoolWPtr buildUpKnob;
    KnobDoubleWPtr brushVisiblePortionKnob;
    KnobDoubleWPtr cloneTranslateKnob;
    KnobDoubleWPtr cloneRotateKnob;
    KnobDoubleWPtr cloneScaleKnob;
    KnobBoolWPtr cloneUniformKnob;
    KnobDoubleWPtr cloneSkewXKnob;
    KnobDoubleWPtr cloneSkewYKnob;
    KnobChoiceWPtr cloneSkewOrderKnob;
    KnobDoubleWPtr cloneCenterKnob;
    KnobButtonWPtr resetCloneCenterKnob;
    KnobChoiceWPtr cloneFilterKnob;
    KnobBoolWPtr cloneBlackOutsideKnob;
    KnobButtonWPtr resetCloneTransformKnob;
    KnobDoubleWPtr translateKnob;
    KnobDoubleWPtr rotateKnob;
    KnobDoubleWPtr scaleKnob;
    KnobBoolWPtr scaleUniformKnob;
    KnobBoolWPtr transformInteractiveKnob;
    KnobDoubleWPtr skewXKnob;
    KnobDoubleWPtr skewYKnob;
    KnobChoiceWPtr skewOrderKnob;
    KnobDoubleWPtr centerKnob;
    KnobButtonWPtr resetCenterKnob;
    KnobDoubleWPtr extraMatrixKnob;
    KnobButtonWPtr resetTransformKnob;
    KnobChoiceWPtr sourceTypeKnob;
    KnobIntWPtr timeOffsetKnob;
    KnobChoiceWPtr timeOffsetModeKnob;

#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    KnobChoiceWPtr motionBlurTypeKnob;
    KnobDoubleWPtr motionBlurKnob, globalMotionBlurKnob;
    KnobDoubleWPtr shutterKnob, globalShutterKnob;
    KnobChoiceWPtr shutterTypeKnob, globalShutterTypeKnob;
    KnobDoubleWPtr customOffsetKnob, globalCustomOffsetKnob;
#endif

    std::list<KnobIWPtr > knobs; //< list for easy access to all knobs
    std::list<KnobIWPtr > cloneKnobs;
    std::list<KnobIWPtr > strokeKnobs;
    std::list<KnobIWPtr > shapeKnobs;

    ///This keeps track  of the items linked to the context knobs
    std::list<RotoItemPtr > selectedItems;
    RotoItemPtr lastInsertedItem;
    RotoItemPtr lastLockedItem;


    /*
     * A merge node (or more if there are more than 64 items) used when all items share the same compositing operator to make the rotopaint tree shallow
     */
    NodesList globalMergeNodes;

    RotoContextPrivate(const NodePtr& n )
        : rotoContextMutex()
        , isPaintNode(false)
        , layers()
        , autoKeying(true)
        , rippleEdit(false)
        , featherLink(true)
        , isCurrentlyLoading(false)
        , node(n)
        , globalMergeNodes()
    {
        EffectInstancePtr effect = n->getEffectInstance();
        RotoPaint* isRotoNode = dynamic_cast<RotoPaint*>( effect.get() );

        if (isRotoNode) {
            isPaintNode = isRotoNode->isDefaultBehaviourPaintContext();
        } else {
            isPaintNode = false;
        }

    }



    RotoLayerPtr findDeepestSelectedLayer() const
    {
        assert( !rotoContextMutex.tryLock() );

        int minLevel = -1;
        RotoLayerPtr minLayer;
        for (std::list< RotoItemPtr >::const_iterator it = selectedItems.begin();
             it != selectedItems.end(); ++it) {
            int lvl = (*it)->getHierarchyLevel();
            if (lvl > minLevel) {
                RotoLayerPtr isLayer = toRotoLayer(*it);
                if (isLayer) {
                    minLayer = isLayer;
                } else {
                    minLayer = (*it)->getParentLayer();
                }
                minLevel = lvl;
            }
        }

        return minLayer;
    }

    

};

NATRON_NAMESPACE_EXIT;

#endif // ROTOCONTEXTPRIVATE_H
