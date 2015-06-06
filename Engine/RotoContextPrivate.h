//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef ROTOCONTEXTPRIVATE_H
#define ROTOCONTEXTPRIVATE_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <list>
#include <map>
#include <string>
#include <vector>
#include <limits>
#include <sstream>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include <QMutex>
#include <QCoreApplication>
#include <QThread>
#include <QReadWriteLock>

#include <cairo/cairo.h>


#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/EffectInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Rect.h"
#include "Engine/RotoContext.h"
#include "Global/GlobalDefines.h"


#define ROTO_DEFAULT_OPACITY 1.
#define ROTO_DEFAULT_FEATHER 1.5
#define ROTO_DEFAULT_FEATHERFALLOFF 1.
#define ROTO_DEFAULT_COLOR_R 1.
#define ROTO_DEFAULT_COLOR_G 1.
#define ROTO_DEFAULT_COLOR_B 1.


#define kRotoScriptNameHint "Script-name of the item for Python scripts. It cannot be edited."

#define kRotoLabelHint "Label of the layer or curve"

#define kRotoNameHint "Name of the layer or curve."


#define kRotoOpacityParam "opacity"
#define kRotoOpacityParamLabel "Opacity"
#define kRotoOpacityHint \
    "Controls the opacity of the selected shape(s)."

#define kRotoFeatherParam "feather"
#define kRotoFeatherParamLabel "Feather"
#define kRotoFeatherHint \
    "Controls the distance of feather (in pixels) to add around the selected shape(s)"

#define kRotoFeatherFallOffParam "featherFallOff"
#define kRotoFeatherFallOffParamLabel "Feather fall-off"
#define kRotoFeatherFallOffHint \
    "Controls the rate at which the feather is applied on the selected shape(s)."

#define kRotoActivatedParam "activated"
#define kRotoActivatedParamLabel "Activated"
#define kRotoActivatedHint \
    "Controls whether the selected shape(s) should be rendered or not." \
    "Note that you can animate this parameter so you can activate/deactive the shape " \
    "throughout the time."

#define kRotoLockedHint \
    "Control whether the layer/curve is editable or locked."

#ifdef NATRON_ROTO_INVERTIBLE
#define kRotoInvertedParam "inverted"
#define kRotoInvertedParamLabel "Inverted"

#define kRotoInvertedHint \
    "Controls whether the selected shape(s) should be inverted. When inverted everything " \
    "outside the shape will be set to 1 and everything inside the shape will be set to 0."
#endif

#define kRotoOverlayHint "Color of the display overlay for this curve. Doesn't affect output."

#define kRotoColorParam "color"
#define kRotoColorParamLabel "Color"
#define kRotoColorHint \
    "The color of the shape. This parameter is used when the output components are set to RGBA."

#define kRotoCompOperatorParam "operator"
#define kRotoCompOperatorParamLabel "Operator"
#define kRotoCompOperatorHint \
    "The compositing operator controls how this shape is merged with the shapes that have already been rendered.\n" \
    "The roto mask is initialised as black and transparent, then each shape is drawn in the selected order, with the selected color and operator.\n" \
    "Finally, the mask is composed with the source image, if connected, using the 'over' operator.\n" \
    "See http://cairographics.org/operators/ for a full description of available operators."

#define kRotoBrushSourceColor "sourceType"
#define kRotoBrushSourceColorLabel "Source"
#define kRotoBrushSourceColorHint "Source color used for painting the stroke when the Reveal/Clone tools are used:\n" \
"- foreground: the painted result at this point in the hierarchy\n" \
"- background: the original image unpainted connected to bg\n" \
"- backgroundN: the original image unpainted connected to bgN\n"

#define kRotoBrushSizeParam "brushSize"
#define kRotoBrushSizeParamLabel "Brush Size"
#define kRotoBrushSizeParamHint "This is the diameter of the brush in pixels. Shift + drag on the viewer to modify this value"

#define kRotoBrushSpacingParam "brushSpacing"
#define kRotoBrushSpacingParamLabel "brushSpacing"
#define kRotoBrushSpacingParamHint "Spacing between stamps of the paint brush"

#define kRotoBrushHardnessParam "brushHardness"
#define kRotoBrushHardnessParamLabel "Brush Hardness"
#define kRotoBrushHardnessParamHint "Fall off of the brush effect from the center to the edge"

#define kRotoBrushEffectParam "brushEffect"
#define kRotoBrushEffectParamLabel "Brush effect"
#define kRotoBrushEffectParamHint "The strength of the effect"

#define kRotoBrushVisiblePortionParam "strokeVisiblePortion"
#define kRotoBrushVisiblePortionParamLabel "Visible portion"
#define kRotoBrushVisiblePortionParamHint "Defines the range of the stroke that should be visible: 0 is the start of the stroke and 1 the end."

#define kRotoBrushPressureLabelParam "pressureAlters"
#define kRotoBrushPressureLabelParamLabel "Pressure alters:"
#define kRotoBrushPressureLabelParamHint ""

#define kRotoBrushPressureOpacityParam "pressureOpacity"
#define kRotoBrushPressureOpacityParamLabel "Opacity"
#define kRotoBrushPressureOpacityParamHint "Alters the opacity of the paint brush proportionate to changes in pen pressure"

#define kRotoBrushPressureSizeParam "pressureSize"
#define kRotoBrushPressureSizeParamLabel "Size"
#define kRotoBrushPressureSizeParamHint "Alters the size of the paint brush proportionate to changes in pen pressure"

#define kRotoBrushPressureHardnessParam "pressureHardness"
#define kRotoBrushPressureHardnessParamLabel "Hardness"
#define kRotoBrushPressureHardnessParamHint "Alters the hardness of the paint brush proportionate to changes in pen pressure"

#define kRotoBrushBuildupParam "buildUp"
#define kRotoBrushBuildupParamLabel "Build-up"
#define kRotoBrushBuildupParamHint "When checked, the paint stroke builds up when painted over itself"

#define kRotoBrushTimeOffsetParam "timeOffset"
#define kRotoBrushTimeOffsetParamLabel "Clone time offset"
#define kRotoBrushTimeOffsetParamHint "When the Clone tool is used, this determines depending on the time offset mode the source frame to " \
"clone. When in absolute mode, this is the frame number of the source, when in relative mode, this is an offset relative to the current frame."

#define kRotoBrushTimeOffsetModeParam "timeOffsetMode"
#define kRotoBrushTimeOffsetModeParamLabel "Mode"
#define kRotoBrushTimeOffsetModeParamHint "Time offset mode: when in absolute mode, this is the frame number of the source, when in relative mode, this is an offset relative to the current frame."

#define kRotoBrushTranslateParam "translate"
#define kRotoBrushTranslateParamLabel "Translate"
#define kRotoBrushTranslateParamHint ""

#define kRotoBrushRotateParam "rotate"
#define kRotoBrushRotateParamLabel "Rotate"
#define kRotoBrushRotateParamHint ""

#define kRotoBrushScaleParam "scale"
#define kRotoBrushScaleParamLabel "Scale"
#define kRotoBrushScaleParamHint ""

#define kRotoBrushScaleUniformParam "uniform"
#define kRotoBrushScaleUniformParamLabel "Uniform"
#define kRotoBrushScaleUniformParamHint ""

#define kRotoBrushSkewXParam "skewx"
#define kRotoBrushSkewXParamLabel "Skew X"
#define kRotoBrushSkewXParamHint ""

#define kRotoBrushSkewYParam "skewy"
#define kRotoBrushSkewYParamLabel "Skew Y"
#define kRotoBrushSkewYParamHint ""

#define kRotoBrushSkewOrderParam "skewOrder"
#define kRotoBrushSkewOrderParamLabel "Skew Order"
#define kRotoBrushSkewOrderParamHint ""

#define kRotoBrushCenterParam "center"
#define kRotoBrushCenterParamLabel "Center"
#define kRotoBrushCenterParamHint ""

#define kRotoBrushFilterParam "filter"
#define kRotoBrushFilterParamLabel "Filter"
#define kRotoBrushFilterParamHint "Filtering algorithm - some filters may produce values outside of the initial range (*) or modify the values even if there is no movement (+)."

#define kFilterImpulse "Impulse"
#define kFilterImpulseHint "(nearest neighbor / box) Use original values"
#define kFilterBilinear "Bilinear"
#define kFilterBilinearHint "(tent / triangle) Bilinear interpolation between original values"
#define kFilterCubic "Cubic"
#define kFilterCubicHint "(cubic spline) Some smoothing"
#define kFilterKeys "Keys"
#define kFilterKeysHint "(Catmull-Rom / Hermite spline) Some smoothing, plus minor sharpening (*)"
#define kFilterSimon "Simon"
#define kFilterSimonHint "Some smoothing, plus medium sharpening (*)"
#define kFilterRifman "Rifman"
#define kFilterRifmanHint "Some smoothing, plus significant sharpening (*)"
#define kFilterMitchell "Mitchell"
#define kFilterMitchellHint "Some smoothing, plus blurring to hide pixelation (*+)"
#define kFilterParzen "Parzen"
#define kFilterParzenHint "(cubic B-spline) Greatest smoothing of all filters (+)"
#define kFilterNotch "Notch"
#define kFilterNotchHint "Flat smoothing (which tends to hide moire' patterns) (+)"

#define kRotoBrushBlackOutsideParam "blackOutside"
#define kRotoBrushBlackOutsideParamLabel "Black Outside"
#define kRotoBrushBlackOutsideParamHint "Fill the area outside the source image with black"


class Bezier;

struct BezierCPPrivate
{
    boost::weak_ptr<Bezier> holder;

    ///the animation curves for the position in the 2D plane
    boost::shared_ptr<Curve> curveX,curveY;
    double x,y; //< used when there is no keyframe

    ///the animation curves for the derivatives
    ///They do not need to be protected as Curve is a thread-safe class.
    boost::shared_ptr<Curve> curveLeftBezierX,curveRightBezierX,curveLeftBezierY,curveRightBezierY;
    mutable QMutex staticPositionMutex; //< protects the  leftX,rightX,leftY,rightY
    double leftX,rightX,leftY,rightY; //< used when there is no keyframe
    mutable QReadWriteLock masterMutex; //< protects masterTrack & relativePoint
    boost::shared_ptr<Double_Knob> masterTrack; //< is this point linked to a track ?
    SequenceTime offsetTime; //< the time at which the offset must be computed

    BezierCPPrivate(const boost::shared_ptr<Bezier>& curve)
    : holder(curve)
    , curveX(new Curve)
    , curveY(new Curve)
    , x(0)
    , y(0)
    , curveLeftBezierX(new Curve)
    , curveRightBezierX(new Curve)
    , curveLeftBezierY(new Curve)
    , curveRightBezierY(new Curve)
    , staticPositionMutex()
    , leftX(0)
    , rightX(0)
    , leftY(0)
    , rightY(0)
    , masterMutex()
    , masterTrack()
    , offsetTime(0)
    {
    }
};

class BezierCP;
typedef std::list< boost::shared_ptr<BezierCP> > BezierCPs;


struct BezierPrivate
{
    BezierCPs points; //< the control points of the curve
    BezierCPs featherPoints; //< the feather points, the number of feather points must equal the number of cp.
    
    //updated whenever the Bezier is edited, this is used to determine if a point lies inside the bezier or not
    //it has a value for each keyframe
    mutable std::map<int,bool> isClockwiseOriented;
    mutable bool isClockwiseOrientedStatic; //< used when the bezier has no keyframes
    
    bool autoRecomputeOrientation; // when true, orientation will be computed automatically on editing
    
    bool finished; //< when finished is true, the last point of the list is connected to the first point of the list.

    BezierPrivate()
    : points()
    , featherPoints()
    , isClockwiseOriented()
    , isClockwiseOrientedStatic(false)
    , autoRecomputeOrientation(true)
    , finished(false)
    {
    }

    bool hasKeyframeAtTime(int time) const
    {
        // PRIVATE - should not lock

        if ( points.empty() ) {
            return false;
        } else {
            KeyFrame k;

            return points.front()->hasKeyFrameAtTime(time);
        }
    }

    void getKeyframeTimes(std::set<int>* times) const
    {
        // PRIVATE - should not lock

        if ( points.empty() ) {
            return;
        }
        points.front()->getKeyframeTimes(times);
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
                                                     int time,
                                                     int* index) const
    {
        // PRIVATE - should not lock
        int i = 0;

        for (BezierCPs::const_iterator it = points.begin(); it != points.end(); ++it, ++i) {
            double pX,pY;
            (*it)->getPositionAtTime(time, &pX, &pY);
            if ( ( pX >= (x - acceptance) ) && ( pX <= (x + acceptance) ) && ( pY >= (y - acceptance) ) && ( pY <= (y + acceptance) ) ) {
                *index = i;

                return it;
            }
        }

        return points.end();
    }

    BezierCPs::const_iterator findFeatherPointNearby(double x,
                                                     double y,
                                                     double acceptance,
                                                     int time,
                                                     int* index) const
    {
        // PRIVATE - should not lock
        int i = 0;

        for (BezierCPs::const_iterator it = featherPoints.begin(); it != featherPoints.end(); ++it, ++i) {
            double pX,pY;
            (*it)->getPositionAtTime(time, &pX, &pY);
            if ( ( pX >= (x - acceptance) ) && ( pX <= (x + acceptance) ) && ( pY >= (y - acceptance) ) && ( pY <= (y + acceptance) ) ) {
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
    std::string scriptName,label;
    boost::weak_ptr<RotoLayer> parentLayer;

    ////This controls whether the item (and all its children if it is a layer)
    ////should be visible/rendered or not at any time.
    ////This is different from the "activated" knob for RotoDrawableItem's which in that
    ////case allows to define a life-time
    bool globallyActivated;

    ////A locked item should not be modifiable by the GUI
    bool locked;

    RotoItemPrivate(const boost::shared_ptr<RotoContext> context,
                    const std::string & n,
                    const boost::shared_ptr<RotoLayer>& parent)
    : context(context)
    , scriptName(n)
    , label(n)
    , parentLayer(parent)
    , globallyActivated(true)
    , locked(false)
    {
    }
};

typedef std::list< boost::shared_ptr<RotoItem> > RotoItems;

struct RotoLayerPrivate
{
    RotoItems items;

    RotoLayerPrivate()
        : items()
    {
    }
};

/**
 * @brief Returns the name of the merge oeprator as described in @openfx-supportext/ofxsMerging.h
 * Keep this in sync with the Merge node's operators otherwise everything will fall apart.
 **/
inline std::string
getNatronOperationString(Natron::MergingFunctionEnum operation)
{
    switch (operation) {
        case Natron::eMergeATop:
            
            return "atop";
        case Natron::eMergeAverage:
            
            return "average";
        case Natron::eMergeColorBurn:
            
            return "color-burn";
        case Natron::eMergeColorDodge:
            
            return "color-dodge";
        case Natron::eMergeConjointOver:
            
            return "conjoint-over";
        case Natron::eMergeCopy:
            
            return "copy";
        case Natron::eMergeDifference:
            
            return "difference";
        case Natron::eMergeDisjointOver:
            
            return "disjoint-over";
        case Natron::eMergeDivide:
            
            return "divide";
        case Natron::eMergeExclusion:
            
            return "exclusion";
        case Natron::eMergeFreeze:
            
            return "freeze";
        case Natron::eMergeFrom:
            
            return "from";
        case Natron::eMergeGeometric:
            
            return "geometric";
        case Natron::eMergeHardLight:
            
            return "hard-light";
        case Natron::eMergeHypot:
            
            return "hypot";
        case Natron::eMergeIn:
            
            return "in";
        case Natron::eMergeInterpolated:
            
            return "interpolated";
        case Natron::eMergeMask:
            
            return "mask";
        case Natron::eMergeMatte:
            
            return "matte";
        case Natron::eMergeLighten:
            
            return "max";
        case Natron::eMergeDarken:
            
            return "min";
        case Natron::eMergeMinus:
            
            return "minus";
        case Natron::eMergeMultiply:
            
            return "multiply";
        case Natron::eMergeOut:
            
            return "out";
        case Natron::eMergeOver:
            
            return "over";
        case Natron::eMergeOverlay:
            
            return "overlay";
        case Natron::eMergePinLight:
            
            return "pinlight";
        case Natron::eMergePlus:
            
            return "plus";
        case Natron::eMergeReflect:
            
            return "reflect";
        case Natron::eMergeScreen:
            
            return "screen";
        case Natron::eMergeSoftLight:
            
            return "soft-light";
        case Natron::eMergeStencil:
            
            return "stencil";
        case Natron::eMergeUnder:
            
            return "under";
        case Natron::eMergeXOR:
            
            return "xor";
    } // switch
    
    return "unknown";
} // getOperationString

inline std::string
getNatronOperationHelpString(Natron::MergingFunctionEnum operation)
{
    switch (operation) {
        case Natron::eMergeATop:
            
            return "Ab + B(1 - a)";
        case Natron::eMergeAverage:
            
            return "(A + B) / 2";
        case Natron::eMergeColorBurn:
            
            return "darken B towards A";
        case Natron::eMergeColorDodge:
            
            return "brighten B towards A";
        case Natron::eMergeConjointOver:
            
            return "A + B(1-a)/b, A if a > b";
        case Natron::eMergeCopy:
            
            return "A";
        case Natron::eMergeDifference:
            
            return "abs(A-B)";
        case Natron::eMergeDisjointOver:
            
            return "A+B(1-a)/b, A+B if a+b < 1";
        case Natron::eMergeDivide:
            
            return "A/B, 0 if A < 0 and B < 0";
        case Natron::eMergeExclusion:
            
            return "A+B-2AB";
        case Natron::eMergeFreeze:
            
            return "1-sqrt(1-A)/B";
        case Natron::eMergeFrom:
            
            return "B-A";
        case Natron::eMergeGeometric:
            
            return "2AB/(A+B)";
        case Natron::eMergeHardLight:
            
            return "multiply if A < 0.5, screen if A > 0.5";
        case Natron::eMergeHypot:
            
            return "sqrt(A*A+B*B)";
        case Natron::eMergeIn:
            
            return "Ab";
        case Natron::eMergeInterpolated:
            
            return "(like average but better and slower)";
        case Natron::eMergeMask:
            
            return "Ba";
        case Natron::eMergeMatte:
            
            return "Aa + B(1-a) (unpremultiplied over)";
        case Natron::eMergeLighten:
            
            return "max(A, B)";
        case Natron::eMergeDarken:
            
            return "min(A, B)";
        case Natron::eMergeMinus:
            
            return "A-B";
        case Natron::eMergeMultiply:
            
            return "AB, 0 if A < 0 and B < 0";
        case Natron::eMergeOut:
            
            return "A(1-b)";
        case Natron::eMergeOver:
            
            return "A+B(1-a)";
        case Natron::eMergeOverlay:
            
            return "multiply if B<0.5, screen if B>0.5";
        case Natron::eMergePinLight:
            
            return "if B >= 0.5 then max(A, 2*B - 1), min(A, B * 2.0 ) else";
        case Natron::eMergePlus:
            
            return "A+B";
        case Natron::eMergeReflect:
            
            return "A*A / (1 - B)";
        case Natron::eMergeScreen:
            
            return "A+B-AB";
        case Natron::eMergeSoftLight:
            
            return "burn-in if A < 0.5, lighten if A > 0.5";
        case Natron::eMergeStencil:
            
            return "B(1-a)";
        case Natron::eMergeUnder:
            
            return "A(1-b)+B";
        case Natron::eMergeXOR:
            
            return "A(1-b)+B(1-a)";
    } // switch
    
    return "unknown";
} // getOperationHelpString

///Keep this in synch with the MergeOperatorEnum !
inline void
getNatronCompositingOperators(std::vector<std::string>* operators,
                             std::vector<std::string>* toolTips)
{
    for (int i = 0; i <= (int)Natron::eMergeXOR; ++i) {
        operators->push_back(getNatronOperationString((Natron::MergingFunctionEnum)i));
        toolTips->push_back(getNatronOperationHelpString((Natron::MergingFunctionEnum)i));
    }
    
}

///Keep this in synch with the cairo_operator_t enum !
///We are not going to create a similar enum just to represent the same thing
inline void
getCairoCompositingOperators(std::vector<std::string>* operators,
                        std::vector<std::string>* toolTips)
{
    assert(operators->size() == CAIRO_OPERATOR_CLEAR);
    operators->push_back("clear");
    toolTips->push_back("clear destination layer");

    assert(operators->size() == CAIRO_OPERATOR_SOURCE);
    operators->push_back("source");
    toolTips->push_back("replace destination layer");

    assert(operators->size() == CAIRO_OPERATOR_OVER);
    operators->push_back("over");
    toolTips->push_back("draw source layer on top of destination layer ");

    assert(operators->size() == CAIRO_OPERATOR_IN);
    operators->push_back("in");
    toolTips->push_back("draw source where there was destination content");

    assert(operators->size() == CAIRO_OPERATOR_OUT);
    operators->push_back("out");
    toolTips->push_back("draw source where there was no destination content");

    assert(operators->size() == CAIRO_OPERATOR_ATOP);
    operators->push_back("atop");
    toolTips->push_back("draw source on top of destination content and only there");

    assert(operators->size() == CAIRO_OPERATOR_DEST);
    operators->push_back("dest");
    toolTips->push_back("ignore the source");

    assert(operators->size() == CAIRO_OPERATOR_DEST_OVER);
    operators->push_back("dest-over");
    toolTips->push_back("draw destination on top of source");

    assert(operators->size() == CAIRO_OPERATOR_DEST_IN);
    operators->push_back("dest-in");
    toolTips->push_back("leave destination only where there was source content");

    assert(operators->size() == CAIRO_OPERATOR_DEST_OUT);
    operators->push_back("dest-out");
    toolTips->push_back("leave destination only where there was no source content");

    assert(operators->size() == CAIRO_OPERATOR_DEST_ATOP);
    operators->push_back("dest-atop");
    toolTips->push_back("leave destination on top of source content and only there ");

    assert(operators->size() == CAIRO_OPERATOR_XOR);
    operators->push_back("xor");
    toolTips->push_back("source and destination are shown where there is only one of them");

    assert(operators->size() == CAIRO_OPERATOR_ADD);
    operators->push_back("add");
    toolTips->push_back("source and destination layers are accumulated");

    assert(operators->size() == CAIRO_OPERATOR_SATURATE);
    operators->push_back("saturate");
    toolTips->push_back("like over, but assuming source and dest are disjoint geometries ");

    assert(operators->size() == CAIRO_OPERATOR_MULTIPLY);
    operators->push_back("multiply");
    toolTips->push_back("source and destination layers are multiplied. This causes the result to be at least as dark as the darker inputs.");

    assert(operators->size() == CAIRO_OPERATOR_SCREEN);
    operators->push_back("screen");
    toolTips->push_back("source and destination are complemented and multiplied. This causes the result to be at least as "
                        "light as the lighter inputs.");

    assert(operators->size() == CAIRO_OPERATOR_OVERLAY);
    operators->push_back("overlay");
    toolTips->push_back("multiplies or screens, depending on the lightness of the destination color. ");

    assert(operators->size() == CAIRO_OPERATOR_DARKEN);
    operators->push_back("darken");
    toolTips->push_back("replaces the destination with the source if it is darker, otherwise keeps the source");

    assert(operators->size() == CAIRO_OPERATOR_LIGHTEN);
    operators->push_back("lighten");
    toolTips->push_back("replaces the destination with the source if it is lighter, otherwise keeps the source.");

    assert(operators->size() == CAIRO_OPERATOR_COLOR_DODGE);
    operators->push_back("color-dodge");
    toolTips->push_back("brightens the destination color to reflect the source color. ");

    assert(operators->size() == CAIRO_OPERATOR_COLOR_BURN);
    operators->push_back("color-burn");
    toolTips->push_back("darkens the destination color to reflect the source color.");

    assert(operators->size() == CAIRO_OPERATOR_HARD_LIGHT);
    operators->push_back("hard-light");
    toolTips->push_back("Multiplies or screens, dependent on source color.");

    assert(operators->size() == CAIRO_OPERATOR_SOFT_LIGHT);
    operators->push_back("soft-light");
    toolTips->push_back("Darkens or lightens, dependent on source color.");

    assert(operators->size() == CAIRO_OPERATOR_DIFFERENCE);
    operators->push_back("difference");
    toolTips->push_back("Takes the difference of the source and destination color. ");

    assert(operators->size() == CAIRO_OPERATOR_EXCLUSION);
    operators->push_back("exclusion");
    toolTips->push_back("Produces an effect similar to difference, but with lower contrast. ");

    assert(operators->size() == CAIRO_OPERATOR_HSL_HUE);
    operators->push_back("HSL-hue");
    toolTips->push_back("Creates a color with the hue of the source and the saturation and luminosity of the target.");

    assert(operators->size() == CAIRO_OPERATOR_HSL_SATURATION);
    operators->push_back("HSL-saturation");
    toolTips->push_back("Creates a color with the saturation of the source and the hue and luminosity of the target."
                        " Painting with this mode onto a gray area produces no change.");

    assert(operators->size() == CAIRO_OPERATOR_HSL_COLOR);
    operators->push_back("HSL-color");
    toolTips->push_back("Creates a color with the hue and saturation of the source and the luminosity of the target."
                        " This preserves the gray levels of the target and is useful for coloring monochrome"
                        " images or tinting color images");

    assert(operators->size() == CAIRO_OPERATOR_HSL_LUMINOSITY);
    operators->push_back("HSL-luminosity");
    toolTips->push_back("Creates a color with the luminosity of the source and the hue and saturation of the target."
                        " This produces an inverse effect to HSL-color.");
} // getCompositingOperators

struct RotoDrawableItemPrivate
{
    double overlayColor[4]; //< the color the shape overlay should be drawn with, defaults to smooth red
    boost::shared_ptr<Double_Knob> opacity; //< opacity of the rendered shape between 0 and 1
    boost::shared_ptr<Double_Knob> feather; //< number of pixels to add to the feather distance (from the feather point), between -100 and 100
    boost::shared_ptr<Double_Knob> featherFallOff; //< the rate of fall-off for the feather, between 0 and 1,  0.5 meaning the
                                                   //alpha value is half the original value when at half distance from the feather distance
    boost::shared_ptr<Bool_Knob> activated; //< should the curve be visible/rendered ? (animable)
#ifdef NATRON_ROTO_INVERTIBLE
    boost::shared_ptr<Bool_Knob> inverted; //< invert the rendering
#endif
    boost::shared_ptr<Color_Knob> color;
    boost::shared_ptr<Choice_Knob> compOperator;
    
    std::list<boost::shared_ptr<KnobI> > knobs; //< list for easy access to all knobs

    RotoDrawableItemPrivate(bool isPaintingNode)
        : opacity()
          , feather()
          , featherFallOff()
          , activated()
#ifdef NATRON_ROTO_INVERTIBLE
          , inverted()
#endif
          , color()
          , compOperator()
          , knobs()
    {
        opacity.reset(new Double_Knob(NULL, kRotoOpacityParamLabel, 1, false));
        opacity->setHintToolTip(kRotoOpacityHint);
        opacity->setName(kRotoOpacityParam);
        opacity->populate();
        opacity->setDefaultValue(ROTO_DEFAULT_OPACITY);
        knobs.push_back(opacity);

        if (!isPaintingNode) {
            feather.reset(new Double_Knob(NULL, kRotoFeatherParamLabel, 1, false));
            feather->setHintToolTip(kRotoFeatherHint);
            feather->setName(kRotoFeatherParam);
            feather->populate();
            feather->setDefaultValue(ROTO_DEFAULT_FEATHER);
            knobs.push_back(feather);
            
            featherFallOff.reset(new Double_Knob(NULL, kRotoFeatherFallOffParamLabel, 1, false));
            featherFallOff->setHintToolTip(kRotoFeatherFallOffHint);
            featherFallOff->setName(kRotoFeatherFallOffParam);
            featherFallOff->populate();
            featherFallOff->setDefaultValue(ROTO_DEFAULT_FEATHERFALLOFF);
            knobs.push_back(featherFallOff);
        }
        
        activated.reset(new Bool_Knob(NULL, kRotoActivatedParamLabel, 1, false));
        activated->setHintToolTip(kRotoActivatedHint);
        activated->setName(kRotoActivatedParam);
        activated->populate();
        activated->setDefaultValue(true);
        knobs.push_back(activated);

#ifdef NATRON_ROTO_INVERTIBLE
        inverted.reset(new Bool_Knob(NULL, kRotoInvertedParamLable, 1, false));
        inverted->setHintToolTip(kRotoInvertedHint);
        inverted->setName(kRotoInvertedParam);
        inverted->populate();
        inverted->setDefaultValue(false);
        knobs.push_back(inverted);
#endif
        

        color.reset(new Color_Knob(NULL, kRotoColorParamLabel, 3, false));
        color->setHintToolTip(kRotoColorHint);
        color->setName(kRotoColorParam);
        color->populate();
        color->setDefaultValue(ROTO_DEFAULT_COLOR_R, 0);
        color->setDefaultValue(ROTO_DEFAULT_COLOR_G, 1);
        color->setDefaultValue(ROTO_DEFAULT_COLOR_B, 2);
        knobs.push_back(color);

        compOperator.reset(new Choice_Knob(NULL, kRotoCompOperatorParamLabel, 1, false));
        compOperator->setHintToolTip(kRotoCompOperatorHint);
        compOperator->setName(kRotoCompOperatorParam);
        compOperator->populate();
        knobs.push_back(compOperator);

        overlayColor[0] = 0.85164;
        overlayColor[1] = 0.196936;
        overlayColor[2] = 0.196936;
        overlayColor[3] = 1.;
    }

    ~RotoDrawableItemPrivate()
    {
    }
};

struct RotoStrokeItemPrivate
{
    Natron::RotoStrokeType type;
    bool finished;
    boost::shared_ptr<Double_Knob> brushSize;
    boost::shared_ptr<Double_Knob> brushSpacing;
    boost::shared_ptr<Double_Knob> brushHardness;
    boost::shared_ptr<Double_Knob> effectStrength;
    boost::shared_ptr<Bool_Knob> pressureOpacity,pressureSize,pressureHardness,buildUp;
    boost::shared_ptr<Double_Knob> visiblePortion; // [0,1] by default
    
    boost::shared_ptr<Double_Knob> cloneTranslate;
    boost::shared_ptr<Double_Knob> cloneRotate;
    boost::shared_ptr<Double_Knob> cloneScale;
    boost::shared_ptr<Bool_Knob> cloneScaleUniform;
    boost::shared_ptr<Double_Knob> cloneSkewX;
    boost::shared_ptr<Double_Knob> cloneSkewY;
    boost::shared_ptr<Choice_Knob> cloneSkewOrder;
    boost::shared_ptr<Double_Knob> cloneCenter;
    boost::shared_ptr<Choice_Knob> cloneFilter;
    boost::shared_ptr<Bool_Knob> cloneBlackOutside;
    
    boost::shared_ptr<Choice_Knob> sourceColor;
    boost::shared_ptr<Int_Knob> timeOffset;
    boost::shared_ptr<Choice_Knob> timeOffsetMode;
    Curve xCurve,yCurve,pressureCurve;
    RectD bbox;
    
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
    boost::shared_ptr<Natron::Node> effectNode;
    boost::shared_ptr<Natron::Node> mergeNode;
    boost::shared_ptr<Natron::Node> timeOffsetNode, frameHoldNode;
    
    
    int lastTickAge;

    struct StrokeTickData
    {
        RectD tickBbox;
        RectD wholeBbox;
        std::list<std::pair<Natron::Point,double> > points;
    };
    
    std::map<int,StrokeTickData> strokeTicks;
    
    mutable QMutex strokeDotPatternsMutex;
    std::vector<cairo_pattern_t*> strokeDotPatterns;
    
    RotoStrokeItemPrivate(Natron::RotoStrokeType type)
    : type(type)
    , finished(false)
    , brushSize(new Double_Knob(NULL, kRotoBrushSizeParamLabel, 1, false))
    , brushSpacing(new Double_Knob(NULL, kRotoBrushSpacingParamLabel, 1, false))
    , brushHardness(new Double_Knob(NULL, kRotoBrushHardnessParamLabel, 1, false))
    , effectStrength(new Double_Knob(NULL, kRotoBrushEffectParamLabel, 1, false))
    , pressureOpacity(new Bool_Knob(NULL, kRotoBrushPressureLabelParamLabel, 1, false))
    , pressureSize(new Bool_Knob(NULL, kRotoBrushPressureSizeParamLabel, 1, false))
    , pressureHardness(new Bool_Knob(NULL, kRotoBrushPressureHardnessParamLabel, 1, false))
    , buildUp(new Bool_Knob(NULL, kRotoBrushBuildupParamLabel, 1, false))
    , visiblePortion(new Double_Knob(NULL, kRotoBrushVisiblePortionParamLabel, 2, false))
    , cloneTranslate(new Double_Knob(NULL, kRotoBrushTranslateParamLabel, 2, false))
    , cloneRotate(new Double_Knob(NULL, kRotoBrushRotateParamLabel, 1, false))
    , cloneScale(new Double_Knob(NULL, kRotoBrushScaleParamLabel, 2, false))
    , cloneScaleUniform(new Bool_Knob(NULL, kRotoBrushScaleUniformParamLabel, 1, false))
    , cloneSkewX(new Double_Knob(NULL, kRotoBrushSkewXParamLabel, 1, false))
    , cloneSkewY(new Double_Knob(NULL, kRotoBrushSkewYParamLabel, 1, false))
    , cloneSkewOrder(new Choice_Knob(NULL, kRotoBrushSkewOrderParamLabel, 1, false))
    , cloneCenter(new Double_Knob(NULL, kRotoBrushCenterParamLabel, 2, false))
    , cloneFilter(new Choice_Knob(NULL, kRotoBrushFilterParamLabel, 1, false))
    , cloneBlackOutside(new Bool_Knob(NULL, kRotoBrushBlackOutsideParamLabel, 1, false))
    , sourceColor(new Choice_Knob(NULL, kRotoBrushSourceColorLabel, 1, false))
    , timeOffset(new Int_Knob(NULL, kRotoBrushTimeOffsetParamLabel, 1, false))
    , timeOffsetMode(new Choice_Knob(NULL, kRotoBrushTimeOffsetModeParamLabel, 1, false))
    , effectNode()
    , mergeNode()
    , timeOffsetNode()
    , frameHoldNode()
    , lastTickAge(-1)
    , strokeTicks()
    , strokeDotPatternsMutex()
    , strokeDotPatterns()
    {
        
        bbox.x1 = std::numeric_limits<double>::infinity();
        bbox.x2 = -std::numeric_limits<double>::infinity();
        bbox.y1 = std::numeric_limits<double>::infinity();
        bbox.y2 = -std::numeric_limits<double>::infinity();
                
        brushSize->setName(kRotoBrushSizeParam);
        brushSize->setHintToolTip(kRotoBrushSizeParamHint);
        brushSize->populate();
        brushSize->setDefaultValue(25);
        brushSize->setMinimum(1);
        brushSize->setMaximum(1000);
        
        brushSpacing->setName(kRotoBrushSpacingParam);
        brushSpacing->setHintToolTip(kRotoBrushSpacingParamHint);
        brushSpacing->populate();
        brushSpacing->setDefaultValue(0.1);
        brushSpacing->setMinimum(0);
        brushSpacing->setMaximum(1);
        
        brushHardness->setName(kRotoBrushHardnessParam);
        brushHardness->setHintToolTip(kRotoBrushHardnessParamHint);
        brushHardness->populate();
        brushHardness->setDefaultValue(0.2);
        brushHardness->setMinimum(0);
        brushHardness->setMaximum(1);

        effectStrength->setName(kRotoBrushEffectParam);
        effectStrength->setHintToolTip(kRotoBrushEffectParamHint);
        effectStrength->populate();
        effectStrength->setDefaultValue(15);
        effectStrength->setMinimum(0);
        effectStrength->setMaximum(100);
        
        pressureOpacity->setName(kRotoBrushPressureOpacityParam);
        pressureOpacity->setHintToolTip(kRotoBrushPressureOpacityParamHint);
        pressureOpacity->populate();
        pressureOpacity->setAnimationEnabled(false);
        pressureOpacity->setDefaultValue(true);
        
        pressureSize->setName(kRotoBrushPressureSizeParam);
        pressureSize->populate();
        pressureSize->setHintToolTip(kRotoBrushPressureSizeParamHint);
        pressureSize->setAnimationEnabled(false);
        pressureSize->setDefaultValue(false);
 
        
        pressureHardness->setName(kRotoBrushPressureHardnessParam);
        pressureHardness->populate();
        pressureHardness->setHintToolTip(kRotoBrushPressureHardnessParamHint);
        pressureHardness->setAnimationEnabled(false);
        pressureHardness->setDefaultValue(false);
        
        buildUp->setName(kRotoBrushBuildupParam);
        buildUp->populate();
        buildUp->setHintToolTip(kRotoBrushBuildupParamHint);
        buildUp->setDefaultValue(false);
        buildUp->setAnimationEnabled(false);
        buildUp->setDefaultValue(true);
 
        
        visiblePortion->setName(kRotoBrushVisiblePortionParam);
        visiblePortion->setHintToolTip(kRotoBrushVisiblePortionParamHint);
        visiblePortion->populate();
        visiblePortion->setDefaultValue(0,0);
        visiblePortion->setDefaultValue(1,1);
        std::vector<double> mins,maxs;
        mins.push_back(0);
        mins.push_back(0);
        maxs.push_back(1);
        maxs.push_back(1);
        visiblePortion->setMinimumsAndMaximums(mins, maxs);
        
        cloneTranslate->setName(kRotoBrushTranslateParam);
        cloneTranslate->setHintToolTip(kRotoBrushTranslateParamHint);
        cloneTranslate->populate();
        
        cloneRotate->setName(kRotoBrushRotateParam);
        cloneRotate->setHintToolTip(kRotoBrushRotateParamHint);
        cloneRotate->populate();
        
        cloneScale->setName(kRotoBrushScaleParam);
        cloneScale->setHintToolTip(kRotoBrushScaleParamHint);
        cloneScale->populate();
        cloneScale->setDefaultValue(1,0);
        cloneScale->setDefaultValue(1,1);
        
        cloneScaleUniform->setName(kRotoBrushScaleUniformParam);
        cloneScaleUniform->setHintToolTip(kRotoBrushScaleUniformParamHint);
        cloneScaleUniform->populate();
        cloneScaleUniform->setDefaultValue(true);
        
        cloneSkewX->setName(kRotoBrushSkewXParam);
        cloneSkewX->setHintToolTip(kRotoBrushSkewXParamHint);
        cloneSkewX->populate();
        
        cloneSkewY->setName(kRotoBrushSkewYParam);
        cloneSkewY->setHintToolTip(kRotoBrushSkewYParamHint);
        cloneSkewY->populate();
        
        cloneSkewOrder->setName(kRotoBrushSkewOrderParam);
        cloneSkewOrder->setHintToolTip(kRotoBrushSkewOrderParamHint);
        cloneSkewOrder->populate();
        {
            std::vector<std::string> choices;
            choices.push_back("XY");
            choices.push_back("YX");
            cloneSkewOrder->populateChoices(choices);
        }
        
        cloneCenter->setName(kRotoBrushCenterParam);
        cloneCenter->setHintToolTip(kRotoBrushCenterParamHint);
        cloneCenter->populate();
        //double defCenter[2] = {.5,.5};
        //cloneCenter->setDefaultValuesNormalized(2, defCenter);

        cloneFilter->setName(kRotoBrushFilterParam);
        cloneFilter->setHintToolTip(kRotoBrushFilterParamHint);
        cloneFilter->populate();
        {
            std::vector<std::string> choices,helps;
            
            choices.push_back(kFilterImpulse);
            helps.push_back(kFilterImpulseHint);
            choices.push_back(kFilterBilinear);
            helps.push_back(kFilterBilinearHint);
            choices.push_back(kFilterCubic);
            helps.push_back(kFilterCubicHint);
            choices.push_back(kFilterKeys);
            helps.push_back(kFilterKeysHint);
            choices.push_back(kFilterSimon);
            helps.push_back(kFilterSimonHint);
            choices.push_back(kFilterRifman);
            helps.push_back(kFilterRifmanHint);
            choices.push_back(kFilterMitchell);
            helps.push_back(kFilterMitchellHint);
            choices.push_back(kFilterParzen);
            helps.push_back(kFilterParzenHint);
            choices.push_back(kFilterNotch);
            helps.push_back(kFilterNotchHint);
            cloneFilter->populateChoices(choices);
        }
        cloneFilter->setDefaultValue(2);
        
        
        cloneBlackOutside->setName(kRotoBrushBlackOutsideParam);
        cloneBlackOutside->setHintToolTip(kRotoBrushBlackOutsideParamHint);
        cloneBlackOutside->populate();
        cloneBlackOutside->setDefaultValue(true);
        
        sourceColor->setName(kRotoBrushSourceColor);
        sourceColor->setHintToolTip(kRotoBrushSizeParamHint);
        sourceColor->populate();
        sourceColor->setDefaultValue(1);
        {
            std::vector<std::string> choices;
            choices.push_back("foreground");
            choices.push_back("background");
            for (int i = 1; i < 10; ++i) {
                std::stringstream ss;
                ss << "background " << i + 1;
                choices.push_back(ss.str());
            }
            sourceColor->populateChoices(choices);
        }
        
        timeOffset->setName(kRotoBrushTimeOffsetParam);
        timeOffset->setHintToolTip(kRotoBrushTimeOffsetParamHint);
        timeOffset->populate();
        timeOffset->setDisplayMinimum(-100);
        timeOffset->setDisplayMaximum(100);
        
        timeOffsetMode->setName(kRotoBrushTimeOffsetModeParam);
        timeOffsetMode->setName(kRotoBrushTimeOffsetModeParamHint);
        timeOffsetMode->populate();
        {
            std::vector<std::string> modes;
            modes.push_back("Relative");
            modes.push_back("Absolute");
            timeOffsetMode->populateChoices(modes);
        }
    }
};

struct RotoContextPrivate
{
    mutable QMutex rotoContextMutex;
    
    /*
     * We have chosen to disable rotopainting and roto shapes from the same RotoContext because the rendering techniques are
     * very much differents. The rotopainting systems requires an entire compositing tree held inside whereas the rotoshapes
     * are rendered and optimized by Cairo internally.
     */
    bool isPaintNode;
    
    std::list< boost::shared_ptr<RotoLayer> > layers;
    bool autoKeying;
    bool rippleEdit;
    bool featherLink;
    boost::weak_ptr<Natron::Node> node;
    U64 age;

    ///These are knobs that take the value of the selected splines info.
    ///Their value changes when selection changes.
    boost::weak_ptr<Double_Knob> opacity;
    boost::weak_ptr<Double_Knob> feather;
    boost::weak_ptr<Double_Knob> featherFallOff;
    boost::weak_ptr<Bool_Knob> activated; //<allows to disable a shape on a specific frame range
#ifdef NATRON_ROTO_INVERTIBLE
    boost::weak_ptr<Bool_Knob> inverted;
#endif
    boost::weak_ptr<Color_Knob> colorKnob;
    
    boost::weak_ptr<Double_Knob> brushSizeKnob;
    boost::weak_ptr<Double_Knob> brushSpacingKnob;
    boost::weak_ptr<Double_Knob> brushHardnessKnob;
    boost::weak_ptr<Double_Knob> brushEffectKnob;
    boost::weak_ptr<String_Knob> pressureLabelKnob;
    boost::weak_ptr<Bool_Knob> pressureOpacityKnob;
    boost::weak_ptr<Bool_Knob> pressureSizeKnob;
    boost::weak_ptr<Bool_Knob> pressureHardnessKnob;
    boost::weak_ptr<Bool_Knob> buildUpKnob;
    boost::weak_ptr<Double_Knob> brushVisiblePortionKnob;
    
    boost::weak_ptr<Double_Knob> translateKnob;
    boost::weak_ptr<Double_Knob> rotateKnob;
    boost::weak_ptr<Double_Knob> scaleKnob;
    boost::weak_ptr<Bool_Knob> uniformKnob;
    boost::weak_ptr<Double_Knob> skewXKnob;
    boost::weak_ptr<Double_Knob> skewYKnob;
    boost::weak_ptr<Choice_Knob> skewOrderKnob;
    boost::weak_ptr<Double_Knob> centerKnob;
    boost::weak_ptr<Choice_Knob> filterKnob;
    boost::weak_ptr<Bool_Knob> blackOutsideKnob;
    
    boost::weak_ptr<Choice_Knob> sourceTypeKnob;
    boost::weak_ptr<Int_Knob> timeOffsetKnob;
    boost::weak_ptr<Choice_Knob> timeOffsetModeKnob;

    std::list<boost::weak_ptr<KnobI> > knobs; //< list for easy access to all knobs
    std::list<boost::weak_ptr<KnobI> > cloneKnobs;

    ///This keeps track  of the items linked to the context knobs
    std::list<boost::shared_ptr<RotoItem> > selectedItems;
    boost::shared_ptr<RotoItem> lastInsertedItem;
    boost::shared_ptr<RotoItem> lastLockedItem;
    
    QMutex lastRenderedImageMutex;
    U64 lastRenderedHash;
    boost::shared_ptr<Natron::Image> lastRenderedImage;

    RotoContextPrivate(const boost::shared_ptr<Natron::Node>& n )
    : rotoContextMutex()
    , isPaintNode(n->isRotoPaintingNode())
    , layers()
    , autoKeying(true)
    , rippleEdit(false)
    , featherLink(true)
    , node(n)
    , age(0)
    , lastRenderedImageMutex()
    , lastRenderedHash(0)
    , lastRenderedImage()
    {
        assert( n && n->getLiveInstance() );
        Natron::EffectInstance* effect = n->getLiveInstance();
    
        boost::shared_ptr<Page_Knob> shapePage = Natron::createKnob<Page_Knob>(effect, "Stroke", 1, false);
        boost::shared_ptr<Page_Knob> clonePage = Natron::createKnob<Page_Knob>(effect, "Clone", 1, false);

        boost::shared_ptr<Double_Knob> opacityKnob = Natron::createKnob<Double_Knob>(effect, kRotoOpacityParamLabel, 1, false);
        opacityKnob->setHintToolTip(kRotoOpacityHint);
        opacityKnob->setName(kRotoOpacityParam);
        opacityKnob->setMinimum(0.);
        opacityKnob->setMaximum(1.);
        opacityKnob->setDisplayMinimum(0.);
        opacityKnob->setDisplayMaximum(1.);
        opacityKnob->setDefaultValue(ROTO_DEFAULT_OPACITY);
        opacityKnob->setAllDimensionsEnabled(false);
        opacityKnob->setIsPersistant(false);
        shapePage->addKnob(opacityKnob);
        knobs.push_back(opacityKnob);
        opacity = opacityKnob;
        
        if (!isPaintNode) {
            boost::shared_ptr<Double_Knob> featherKnob = Natron::createKnob<Double_Knob>(effect, kRotoFeatherParamLabel, 1, false);
            featherKnob->setHintToolTip(kRotoFeatherHint);
            featherKnob->setName(kRotoFeatherParam);
            featherKnob->setMinimum(-100);
            featherKnob->setMaximum(100);
            featherKnob->setDisplayMinimum(-100);
            featherKnob->setDisplayMaximum(100);
            featherKnob->setDefaultValue(ROTO_DEFAULT_FEATHER);
            featherKnob->setAllDimensionsEnabled(false);
            featherKnob->setIsPersistant(false);
            shapePage->addKnob(featherKnob);
            knobs.push_back(featherKnob);
            feather = featherKnob;
            
            boost::shared_ptr<Double_Knob> featherFallOffKnob = Natron::createKnob<Double_Knob>(effect, kRotoFeatherFallOffParamLabel, 1, false);
            featherFallOffKnob->setHintToolTip(kRotoFeatherFallOffHint);
            featherFallOffKnob->setName(kRotoFeatherFallOffParam);
            featherFallOffKnob->setMinimum(0.001);
            featherFallOffKnob->setMaximum(5.);
            featherFallOffKnob->setDisplayMinimum(0.2);
            featherFallOffKnob->setDisplayMaximum(5.);
            featherFallOffKnob->setDefaultValue(ROTO_DEFAULT_FEATHERFALLOFF);
            featherFallOffKnob->setAllDimensionsEnabled(false);
            featherFallOffKnob->setIsPersistant(false);
            shapePage->addKnob(featherFallOffKnob);
            knobs.push_back(featherFallOffKnob);
            featherFallOff = featherFallOffKnob;
        } 
        boost::shared_ptr<Bool_Knob> activatedKnob = Natron::createKnob<Bool_Knob>(effect, kRotoActivatedParamLabel, 1, false);
        activatedKnob->setHintToolTip(kRotoActivatedHint);
        activatedKnob->setName(kRotoActivatedParam);
        activatedKnob->setAddNewLine(false);
        activatedKnob->setDefaultValue(true);
        activatedKnob->setAllDimensionsEnabled(false);
        shapePage->addKnob(activatedKnob);
        activatedKnob->setIsPersistant(false);
        knobs.push_back(activatedKnob);
        activated = activatedKnob;
        
#ifdef NATRON_ROTO_INVERTIBLE
        boost::shared_ptr<Bool_Knob> invertedKnob = Natron::createKnob<Bool_Knob>(effect, kRotoInvertedParamLabel, 1, false);
        invertedKnob->setHintToolTip(kRotoInvertedHint);
        invertedKnob->setName(kRotoInvertedParam);
        invertedKnob->setDefaultValue(false);
        invertedKnob->setAllDimensionsEnabled(false);
        invertedKnob->setIsPersistant(false);
        shapePage->addKnob(invertedKnob);
        knobs.push_back(invertedKnob);
        inverted = invertedKnob;
#endif

        boost::shared_ptr<Color_Knob> ck = Natron::createKnob<Color_Knob>(effect, kRotoColorParamLabel, 3, false);
        ck->setHintToolTip(kRotoColorHint);
        ck->setName(kRotoColorParam);
        ck->setDefaultValue(ROTO_DEFAULT_COLOR_R, 0);
        ck->setDefaultValue(ROTO_DEFAULT_COLOR_G, 1);
        ck->setDefaultValue(ROTO_DEFAULT_COLOR_B, 2);
        ck->setAllDimensionsEnabled(false);
        shapePage->addKnob(ck);
        ck->setIsPersistant(false);
        knobs.push_back(ck);
        colorKnob = ck;
        
#ifdef ROTO_ENABLE_PAINT
        if (isPaintNode) {
            
            boost::shared_ptr<Choice_Knob> sourceType = Natron::createKnob<Choice_Knob>(effect,kRotoBrushSourceColorLabel,1,false);
            sourceType->setName(kRotoBrushSourceColor);
            sourceType->setHintToolTip(kRotoBrushSourceColorHint);
            sourceType->setDefaultValue(1);
            {
                std::vector<std::string> choices;
                choices.push_back("foreground");
                choices.push_back("background");
                for (int i = 1; i < 10; ++i) {
                    std::stringstream ss;
                    ss << "background " << i + 1;
                    choices.push_back(ss.str());
                }
                sourceType->populateChoices(choices);
            }
            sourceType->setAllDimensionsEnabled(false);
            clonePage->addKnob(sourceType);
            knobs.push_back(sourceType);
            cloneKnobs.push_back(sourceType);
            sourceTypeKnob = sourceType;
            
            boost::shared_ptr<Double_Knob> translate = Natron::createKnob<Double_Knob>(effect, kRotoBrushTranslateParamLabel, 2, false);
            translate->setName(kRotoBrushTranslateParam);
            translate->setHintToolTip(kRotoBrushTranslateParamHint);
            translate->setAllDimensionsEnabled(false);
            translate->setIncrement(10);
            clonePage->addKnob(translate);
            knobs.push_back(translate);
            cloneKnobs.push_back(translate);
            translateKnob = translate;
            
            boost::shared_ptr<Double_Knob> rotate = Natron::createKnob<Double_Knob>(effect, kRotoBrushRotateParamLabel, 1, false);
            rotate->setName(kRotoBrushRotateParam);
            rotate->setHintToolTip(kRotoBrushRotateParamHint);
            rotate->setAllDimensionsEnabled(false);
            rotate->setDisplayMinimum(-180);
            rotate->setDisplayMaximum(180);
            clonePage->addKnob(rotate);
            knobs.push_back(rotate);
            cloneKnobs.push_back(rotate);
            rotateKnob = rotate;
            
            boost::shared_ptr<Double_Knob> scale = Natron::createKnob<Double_Knob>(effect, kRotoBrushScaleParamLabel, 2, false);
            scale->setName(kRotoBrushScaleParam);
            scale->setHintToolTip(kRotoBrushScaleParamHint);
            scale->setDefaultValue(1,0);
            scale->setDefaultValue(1,1);
            scale->setDisplayMinimum(0.1,0);
            scale->setDisplayMinimum(0.1,1);
            scale->setDisplayMaximum(10,0);
            scale->setDisplayMaximum(10,1);
            scale->setAddNewLine(false);
            scale->setAllDimensionsEnabled(false);
            clonePage->addKnob(scale);
            cloneKnobs.push_back(scale);
            knobs.push_back(scale);
            scaleKnob = scale;
            
            boost::shared_ptr<Bool_Knob> scaleUniform = Natron::createKnob<Bool_Knob>(effect, kRotoBrushScaleUniformParamLabel, 1, false);
            scaleUniform->setName(kRotoBrushScaleUniformParam);
            scaleUniform->setHintToolTip(kRotoBrushScaleUniformParamHint);
            scaleUniform->setDefaultValue(true);
            scaleUniform->setAllDimensionsEnabled(false);
            scaleUniform->setAnimationEnabled(false);
            clonePage->addKnob(scaleUniform);
            cloneKnobs.push_back(scaleUniform);
            knobs.push_back(scaleUniform);
            uniformKnob = scaleUniform;
            
            boost::shared_ptr<Double_Knob> skewX = Natron::createKnob<Double_Knob>(effect, kRotoBrushSkewXParamLabel, 1, false);
            skewX->setName(kRotoBrushSkewXParam);
            skewX->setHintToolTip(kRotoBrushSkewXParamHint);
            skewX->setAllDimensionsEnabled(false);
            skewX->setDisplayMinimum(-1,0);
            skewX->setDisplayMaximum(1,0);
            cloneKnobs.push_back(skewX);
            clonePage->addKnob(skewX);
            knobs.push_back(skewX);
            skewXKnob = skewX;
            
            boost::shared_ptr<Double_Knob> skewY = Natron::createKnob<Double_Knob>(effect, kRotoBrushSkewYParamLabel, 1, false);
            skewY->setName(kRotoBrushSkewYParam);
            skewY->setHintToolTip(kRotoBrushSkewYParamHint);
            skewY->setAllDimensionsEnabled(false);
            skewY->setDisplayMinimum(-1,0);
            skewY->setDisplayMaximum(1,0);
            clonePage->addKnob(skewY);
            cloneKnobs.push_back(skewY);
            knobs.push_back(skewY);
            skewYKnob = skewY;

            boost::shared_ptr<Choice_Knob> skewOrder = Natron::createKnob<Choice_Knob>(effect,kRotoBrushSkewOrderParamLabel,1,false);
            skewOrder->setName(kRotoBrushSkewOrderParam);
            skewOrder->setHintToolTip(kRotoBrushSkewOrderParamHint);
            skewOrder->setDefaultValue(0);
            {
                std::vector<std::string> choices;
                choices.push_back("XY");
                choices.push_back("YX");
                skewOrder->populateChoices(choices);
            }
            skewOrder->setAllDimensionsEnabled(false);
            skewOrder->setAnimationEnabled(false);
            clonePage->addKnob(skewOrder);
            cloneKnobs.push_back(skewOrder);
            knobs.push_back(skewOrder);
            skewOrderKnob = skewOrder;
            
            boost::shared_ptr<Double_Knob> center = Natron::createKnob<Double_Knob>(effect, kRotoBrushCenterParamLabel, 2, false);
            center->setName(kRotoBrushCenterParam);
            center->setHintToolTip(kRotoBrushCenterParamHint);
            center->setAllDimensionsEnabled(false);
            center->setAnimationEnabled(false);
            double defCenter[2] = {.5,.5};
            center->setDefaultValuesNormalized(2, defCenter);
            clonePage->addKnob(center);
            cloneKnobs.push_back(center);
            knobs.push_back(center);
            centerKnob = center;
            
            boost::shared_ptr<Choice_Knob> filter = Natron::createKnob<Choice_Knob>(effect,kRotoBrushFilterParamLabel,1,false);
            filter->setName(kRotoBrushFilterParam);
            filter->setHintToolTip(kRotoBrushFilterParamHint);
            {
                std::vector<std::string> choices,helps;
                
                choices.push_back(kFilterImpulse);
                helps.push_back(kFilterImpulseHint);
                choices.push_back(kFilterBilinear);
                helps.push_back(kFilterBilinearHint);
                choices.push_back(kFilterCubic);
                helps.push_back(kFilterCubicHint);
                choices.push_back(kFilterKeys);
                helps.push_back(kFilterKeysHint);
                choices.push_back(kFilterSimon);
                helps.push_back(kFilterSimonHint);
                choices.push_back(kFilterRifman);
                helps.push_back(kFilterRifmanHint);
                choices.push_back(kFilterMitchell);
                helps.push_back(kFilterMitchellHint);
                choices.push_back(kFilterParzen);
                helps.push_back(kFilterParzenHint);
                choices.push_back(kFilterNotch);
                helps.push_back(kFilterNotchHint);
                filter->populateChoices(choices);
            }
            filter->setDefaultValue(2);
            filter->setAllDimensionsEnabled(false);
            filter->setAddNewLine(false);
            clonePage->addKnob(filter);
            cloneKnobs.push_back(filter);
            knobs.push_back(filter);
            filterKnob = filter;

            boost::shared_ptr<Bool_Knob> blackOutside = Natron::createKnob<Bool_Knob>(effect, kRotoBrushBlackOutsideParamLabel, 1, false);
            blackOutside->setName(kRotoBrushBlackOutsideParam);
            blackOutside->setHintToolTip(kRotoBrushBlackOutsideParamHint);
            blackOutside->setDefaultValue(true);
            blackOutside->setAllDimensionsEnabled(false);
            clonePage->addKnob(blackOutside);
            knobs.push_back(blackOutside);
            cloneKnobs.push_back(blackOutside);
            blackOutsideKnob = blackOutside;
            
            boost::shared_ptr<Int_Knob> timeOffset = Natron::createKnob<Int_Knob>(effect, kRotoBrushTimeOffsetParamLabel, 1, false);
            timeOffset->setName(kRotoBrushTimeOffsetParam);
            timeOffset->setHintToolTip(kRotoBrushTimeOffsetParamHint);
            timeOffset->setDisplayMinimum(-100);
            timeOffset->setDisplayMaximum(100);
            timeOffset->setAllDimensionsEnabled(false);
            timeOffset->setIsPersistant(false);
            timeOffset->setAddNewLine(false);
            clonePage->addKnob(timeOffset);
            cloneKnobs.push_back(timeOffset);
            knobs.push_back(timeOffset);
            timeOffsetKnob = timeOffset;
            
            boost::shared_ptr<Choice_Knob> timeOffsetMode = Natron::createKnob<Choice_Knob>(effect, kRotoBrushTimeOffsetModeParamLabel, 1, false);
            timeOffsetMode->setName(kRotoBrushTimeOffsetModeParam);
            timeOffsetMode->setHintToolTip(kRotoBrushTimeOffsetModeParamHint);
            {
                std::vector<std::string> modes;
                modes.push_back("Relative");
                modes.push_back("Absolute");
                timeOffsetMode->populateChoices(modes);
            }
            timeOffsetMode->setAllDimensionsEnabled(false);
            timeOffsetMode->setIsPersistant(false);
            clonePage->addKnob(timeOffsetMode);
            knobs.push_back(timeOffsetMode);
            cloneKnobs.push_back(timeOffsetMode);
            timeOffsetModeKnob = timeOffsetMode;
            
            boost::shared_ptr<Double_Knob> brushSize = Natron::createKnob<Double_Knob>(effect, kRotoBrushSizeParamLabel, 1, false);
            brushSize->setName(kRotoBrushSizeParam);
            brushSize->setHintToolTip(kRotoBrushSizeParamHint);
            brushSize->setDefaultValue(25);
            brushSize->setMinimum(1.);
            brushSize->setMaximum(1000);
            brushSize->setAllDimensionsEnabled(false);
            brushSize->setIsPersistant(false);
            shapePage->addKnob(brushSize);
            knobs.push_back(brushSize);
            brushSizeKnob = brushSize;
            
            boost::shared_ptr<Double_Knob> brushSpacing = Natron::createKnob<Double_Knob>(effect, kRotoBrushSpacingParamLabel, 1, false);
            brushSpacing->setName(kRotoBrushSpacingParam);
            brushSpacing->setHintToolTip(kRotoBrushSpacingParamHint);
            brushSpacing->setDefaultValue(0.1);
            brushSpacing->setMinimum(0.);
            brushSpacing->setMaximum(1.);
            brushSpacing->setAllDimensionsEnabled(false);
            brushSpacing->setIsPersistant(false);
            shapePage->addKnob(brushSpacing);
            knobs.push_back(brushSpacing);
            brushSpacingKnob = brushSpacing;
            
            boost::shared_ptr<Double_Knob> brushHardness = Natron::createKnob<Double_Knob>(effect, kRotoBrushHardnessParamLabel, 1, false);
            brushHardness->setName(kRotoBrushHardnessParam);
            brushHardness->setHintToolTip(kRotoBrushHardnessParamHint);
            brushHardness->setDefaultValue(0.2);
            brushHardness->setMinimum(0.);
            brushHardness->setMaximum(1.);
            brushHardness->setAllDimensionsEnabled(false);
            brushHardness->setIsPersistant(false);
            shapePage->addKnob(brushHardness);
            knobs.push_back(brushHardness);
            brushHardnessKnob = brushHardness;
            
            boost::shared_ptr<Double_Knob> effectStrength = Natron::createKnob<Double_Knob>(effect, kRotoBrushEffectParamLabel, 1, false);
            effectStrength->setName(kRotoBrushEffectParam);
            effectStrength->setHintToolTip(kRotoBrushEffectParamHint);
            effectStrength->setDefaultValue(15);
            effectStrength->setMinimum(0.);
            effectStrength->setMaximum(100.);
            effectStrength->setAllDimensionsEnabled(false);
            effectStrength->setIsPersistant(false);
            shapePage->addKnob(effectStrength);
            knobs.push_back(effectStrength);
            brushEffectKnob = effectStrength;
            
            boost::shared_ptr<String_Knob> pressureLabel = Natron::createKnob<String_Knob>(effect, kRotoBrushPressureLabelParamLabel);
            pressureLabel->setName(kRotoBrushPressureLabelParam);
            pressureLabel->setHintToolTip(kRotoBrushPressureLabelParamHint);
            pressureLabel->setAsLabel();
            pressureLabel->setAnimationEnabled(false);
            pressureLabel->setAllDimensionsEnabled(false);
            shapePage->addKnob(pressureLabel);
            knobs.push_back(pressureLabel);
            pressureLabelKnob = pressureLabel;
            
            boost::shared_ptr<Bool_Knob> pressureOpacity = Natron::createKnob<Bool_Knob>(effect, kRotoBrushPressureOpacityParamLabel);
            pressureOpacity->setName(kRotoBrushPressureOpacityParam);
            pressureOpacity->setHintToolTip(kRotoBrushPressureOpacityParamHint);
            pressureOpacity->setAnimationEnabled(false);
            pressureOpacity->setDefaultValue(true);
            pressureOpacity->setAddNewLine(false);
            pressureOpacity->setAllDimensionsEnabled(false);
            pressureOpacity->setIsPersistant(false);
            shapePage->addKnob(pressureOpacity);
            knobs.push_back(pressureOpacity);
            pressureOpacityKnob = pressureOpacity;
            
            boost::shared_ptr<Bool_Knob> pressureSize = Natron::createKnob<Bool_Knob>(effect, kRotoBrushPressureSizeParamLabel);
            pressureSize->setName(kRotoBrushPressureSizeParam);
            pressureSize->setHintToolTip(kRotoBrushPressureSizeParamHint);
            pressureSize->setAnimationEnabled(false);
            pressureSize->setDefaultValue(false);
            pressureSize->setAddNewLine(false);
            pressureSize->setAllDimensionsEnabled(false);
            pressureSize->setIsPersistant(false);
            knobs.push_back(pressureSize);
            shapePage->addKnob(pressureSize);
            pressureSizeKnob = pressureSize;
            
            boost::shared_ptr<Bool_Knob> pressureHardness = Natron::createKnob<Bool_Knob>(effect, kRotoBrushPressureHardnessParamLabel);
            pressureHardness->setName(kRotoBrushPressureHardnessParam);
            pressureHardness->setHintToolTip(kRotoBrushPressureHardnessParamHint);
            pressureHardness->setAnimationEnabled(false);
            pressureHardness->setDefaultValue(false);
            pressureHardness->setAddNewLine(true);
            pressureHardness->setAllDimensionsEnabled(false);
            pressureHardness->setIsPersistant(false);
            knobs.push_back(pressureHardness);
            shapePage->addKnob(pressureHardness);
            pressureHardnessKnob = pressureHardness;
            
            boost::shared_ptr<Bool_Knob> buildUp = Natron::createKnob<Bool_Knob>(effect, kRotoBrushBuildupParamLabel);
            buildUp->setName(kRotoBrushBuildupParam);
            buildUp->setHintToolTip(kRotoBrushBuildupParamHint);
            buildUp->setAnimationEnabled(false);
            buildUp->setDefaultValue(false);
            buildUp->setAddNewLine(true);
            buildUp->setAllDimensionsEnabled(false);
            buildUp->setIsPersistant(false);
            knobs.push_back(buildUp);
            shapePage->addKnob(buildUp);
            buildUpKnob = buildUp;
            
            boost::shared_ptr<Double_Knob> visiblePortion = Natron::createKnob<Double_Knob>(effect, kRotoBrushVisiblePortionParamLabel, 2, false);
            visiblePortion->setName(kRotoBrushVisiblePortionParam);
            visiblePortion->setHintToolTip(kRotoBrushVisiblePortionParamHint);
            visiblePortion->setDefaultValue(0, 0);
            visiblePortion->setDefaultValue(1, 1);
            std::vector<double> mins,maxs;
            mins.push_back(0);
            mins.push_back(0);
            maxs.push_back(1);
            maxs.push_back(1);
            visiblePortion->setMinimumsAndMaximums(mins, maxs);
            visiblePortion->setAllDimensionsEnabled(false);
            visiblePortion->setIsPersistant(false);
            shapePage->addKnob(visiblePortion);
            visiblePortion->setDimensionName(0, "start");
            visiblePortion->setDimensionName(1, "end");
            knobs.push_back(visiblePortion);
            brushVisiblePortionKnob = visiblePortion;
        }
#endif
        
    }

    /**
     * @brief Call this after any change to notify the mask has changed for the cache.
     **/
    void incrementRotoAge()
    {
        ///MT-safe: only called on the main-thread
        assert( QThread::currentThread() == qApp->thread() );

        QMutexLocker l(&rotoContextMutex);
        ++age;
    }

    void renderInternal(cairo_t* cr,const std::list< boost::shared_ptr<RotoDrawableItem> > & splines,
                        unsigned int mipmapLevel,int time);
    
    
    void renderDot(cairo_t* cr,
                   std::vector<cairo_pattern_t*>& dotPatterns,
                   const Natron::Point &center,
                   double internalDotRadius,
                   double externalDotRadius,
                   double pressure,
                   bool doBuildUp,
                   const std::vector<std::pair<double, double> >& opacityStops,
                   double opacity);

    
    double renderStroke(cairo_t* cr,
                        std::vector<cairo_pattern_t*>& dotPatterns,
                        const std::list<std::pair<Natron::Point,double> >& points,
                        double distToNext,
                        const RotoStrokeItem* stroke,
                        int time,
                        unsigned int mipmapLevel);
    void renderStroke(cairo_t* cr,const RotoStrokeItem* stroke, int time, unsigned int mipmapLevel);
    
    void renderBezier(cairo_t* cr,const Bezier* bezier,int time, unsigned int mipmapLevel);
    
    void renderFeather(const Bezier* bezier,int time, unsigned int mipmapLevel, bool inverted, double shapeColor[3], double opacity, double featherDist, double fallOff, cairo_pattern_t* mesh);

    void renderInternalShape(int time,unsigned int mipmapLevel,double shapeColor[3], double opacity,cairo_t* cr, cairo_pattern_t* mesh, const BezierCPs & cps);
    
    static void bezulate(int time,const BezierCPs& cps,std::list<BezierCPs>* patches);

    void applyAndDestroyMask(cairo_t* cr,cairo_pattern_t* mesh);
};


#endif // ROTOCONTEXTPRIVATE_H
