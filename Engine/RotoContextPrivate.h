/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <QMutex>
#include <QCoreApplication>
#include <QThread>
#include <QReadWriteLock>
#include <QWaitCondition>

#include <cairo/cairo.h>


#include "Engine/BezierCP.h"
#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/EffectInstance.h"
#include "Engine/AppManager.h"
#include "Engine/RotoContext.h"
#include "Global/GlobalDefines.h"
#include "Engine/Transform.h"
#include "Engine/MergingEnum.h"

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

#define kRotoBrushTranslateParam "cloneTranslate"
#define kRotoBrushTranslateParamLabel "Translate"
#define kRotoBrushTranslateParamHint ""

#define kRotoBrushRotateParam "cloneRotate"
#define kRotoBrushRotateParamLabel "Rotate"
#define kRotoBrushRotateParamHint ""

#define kRotoBrushScaleParam "cloneScale"
#define kRotoBrushScaleParamLabel "Scale"
#define kRotoBrushScaleParamHint ""

#define kRotoBrushScaleUniformParam "cloneUniform"
#define kRotoBrushScaleUniformParamLabel "Uniform"
#define kRotoBrushScaleUniformParamHint ""

#define kRotoBrushSkewXParam "cloneSkewx"
#define kRotoBrushSkewXParamLabel "Skew X"
#define kRotoBrushSkewXParamHint ""

#define kRotoBrushSkewYParam "cloneSkewy"
#define kRotoBrushSkewYParamLabel "Skew Y"
#define kRotoBrushSkewYParamHint ""

#define kRotoBrushSkewOrderParam "cloneSkewOrder"
#define kRotoBrushSkewOrderParamLabel "Skew Order"
#define kRotoBrushSkewOrderParamHint ""

#define kRotoBrushCenterParam "cloneCenter"
#define kRotoBrushCenterParamLabel "Center"
#define kRotoBrushCenterParamHint ""

#define kRotoBrushFilterParam "cloneFilter"
#define kRotoBrushFilterParamLabel "Filter"
#define kRotoBrushFilterParamHint "Filtering algorithm - some filters may produce values outside of the initial range (*) or modify the values even if there is no movement (+)."

#define kRotoBrushBlackOutsideParam "blackOutside"
#define kRotoBrushBlackOutsideParamLabel "Black Outside"
#define kRotoBrushBlackOutsideParamHint "Fill the area outside the source image with black"

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


#define kRotoDrawableItemTranslateParam "translate"
#define kRotoDrawableItemTranslateParamLabel "Translate"
#define kRotoDrawableItemTranslateParamHint ""

#define kRotoDrawableItemRotateParam "rotate"
#define kRotoDrawableItemRotateParamLabel "Rotate"
#define kRotoDrawableItemRotateParamHint ""

#define kRotoDrawableItemScaleParam "scale"
#define kRotoDrawableItemScaleParamLabel "Scale"
#define kRotoDrawableItemScaleParamHint ""

#define kRotoDrawableItemScaleUniformParam "uniform"
#define kRotoDrawableItemScaleUniformParamLabel "Uniform"
#define kRotoDrawableItemScaleUniformParamHint ""

#define kRotoDrawableItemSkewXParam "skewx"
#define kRotoDrawableItemSkewXParamLabel "Skew X"
#define kRotoDrawableItemSkewXParamHint ""

#define kRotoDrawableItemSkewYParam "skewy"
#define kRotoDrawableItemSkewYParamLabel "Skew Y"
#define kRotoDrawableItemSkewYParamHint ""

#define kRotoDrawableItemSkewOrderParam "skewOrder"
#define kRotoDrawableItemSkewOrderParamLabel "Skew Order"
#define kRotoDrawableItemSkewOrderParamHint ""

#define kRotoDrawableItemCenterParam "center"
#define kRotoDrawableItemCenterParamLabel "Center"
#define kRotoDrawableItemCenterParamHint ""

#define kRotoDrawableItemLifeTimeParam "lifeTime"
#define kRotoDrawableItemLifeTimeParamLabel "Life Time"
#define kRotoDrawableItemLifeTimeParamHint "Controls the life-time of the shape/stroke"

#define kRotoDrawableItemLifeTimeAll "All"
#define kRotoDrawableItemLifeTimeAllHelp "All frames"

#define kRotoDrawableItemLifeTimeSingle "Single"
#define kRotoDrawableItemLifeTimeSingleHelp "Only for the specified frame"

#define kRotoDrawableItemLifeTimeFromStart "From start"
#define kRotoDrawableItemLifeTimeFromStartHelp "From the start of the sequence up to the specified frame"

#define kRotoDrawableItemLifeTimeToEnd "To end"
#define kRotoDrawableItemLifeTimeToEndHelp "From the specified frame to the end of the sequence"

#define kRotoDrawableItemLifeTimeCustom "Custom"
#define kRotoDrawableItemLifeTimeCustomHelp "Use the Activated parameter animation to control the life-time of the shape/stroke using keyframes"

#define kRotoDrawableItemLifeTimeFrameParam "lifeTimeFrame"
#define kRotoDrawableItemLifeTimeFrameParamLabel "Frame"
#define kRotoDrawableItemLifeTimeFrameParamHint "Use this to specify the frame when in mode Single/From start/To end"

class Bezier;


class BezierCP;


struct BezierPrivate
{
    BezierCPs points; //< the control points of the curve
    BezierCPs featherPoints; //< the feather points, the number of feather points must equal the number of cp.
    
    //updated whenever the Bezier is edited, this is used to determine if a point lies inside the bezier or not
    //it has a value for each keyframe
    mutable std::map<int,bool> isClockwiseOriented;
    mutable bool isClockwiseOrientedStatic; //< used when the bezier has no keyframes
    
    mutable std::map<int,bool> guiIsClockwiseOriented;
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

    bool hasKeyframeAtTime(bool useGuiCurves, double time) const
    {
        // PRIVATE - should not lock

        if ( points.empty() ) {
            return false;
        } else {
            KeyFrame k;

            return points.front()->hasKeyFrameAtTime(useGuiCurves, time);
        }
    }

    void getKeyframeTimes(bool useGuiCurves, std::set<int>* times) const
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
                                                     const Transform::Matrix3x3& transform,
                                                     int* index) const
    {
        // PRIVATE - should not lock
        int i = 0;

        for (BezierCPs::const_iterator it = points.begin(); it != points.end(); ++it, ++i) {
            Transform::Point3D p;
            p.z = 1;
            (*it)->getPositionAtTime(true, time, &p.x, &p.y);
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
                                                     const Transform::Matrix3x3& transform,
                                                     int* index) const
    {
        // PRIVATE - should not lock
        int i = 0;
        
        for (BezierCPs::const_iterator it = featherPoints.begin(); it != featherPoints.end(); ++it, ++i) {
            Transform::Point3D p;
            p.z = 1;
            (*it)->getPositionAtTime(true, time, &p.x, &p.y);
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
    
    double overlayColor[4]; //< the color the shape overlay should be drawn with, defaults to smooth red
    boost::shared_ptr<KnobDouble> opacity; //< opacity of the rendered shape between 0 and 1
    boost::shared_ptr<KnobDouble> feather; //< number of pixels to add to the feather distance (from the feather point), between -100 and 100
    boost::shared_ptr<KnobDouble> featherFallOff; //< the rate of fall-off for the feather, between 0 and 1,  0.5 meaning the
                                                   //alpha value is half the original value when at half distance from the feather distance
    boost::shared_ptr<KnobChoice> lifeTime;
    boost::shared_ptr<KnobBool> activated; //< should the curve be visible/rendered ? (animable)
    boost::shared_ptr<KnobInt> lifeTimeFrame;
#ifdef NATRON_ROTO_INVERTIBLE
    boost::shared_ptr<KnobBool> inverted; //< invert the rendering
#endif
    boost::shared_ptr<KnobColor> color;
    boost::shared_ptr<KnobChoice> compOperator;
    
    boost::shared_ptr<KnobDouble> translate;
    boost::shared_ptr<KnobDouble> rotate;
    boost::shared_ptr<KnobDouble> scale;
    boost::shared_ptr<KnobBool> scaleUniform;
    boost::shared_ptr<KnobDouble> skewX;
    boost::shared_ptr<KnobDouble> skewY;
    boost::shared_ptr<KnobChoice> skewOrder;
    boost::shared_ptr<KnobDouble> center;
    
    boost::shared_ptr<KnobDouble> brushSize;
    boost::shared_ptr<KnobDouble> brushSpacing;
    boost::shared_ptr<KnobDouble> brushHardness;
    boost::shared_ptr<KnobDouble> effectStrength;
    boost::shared_ptr<KnobBool> pressureOpacity,pressureSize,pressureHardness,buildUp;
    boost::shared_ptr<KnobDouble> visiblePortion; // [0,1] by default
    
    boost::shared_ptr<KnobDouble> cloneTranslate;
    boost::shared_ptr<KnobDouble> cloneRotate;
    boost::shared_ptr<KnobDouble> cloneScale;
    boost::shared_ptr<KnobBool> cloneScaleUniform;
    boost::shared_ptr<KnobDouble> cloneSkewX;
    boost::shared_ptr<KnobDouble> cloneSkewY;
    boost::shared_ptr<KnobChoice> cloneSkewOrder;
    boost::shared_ptr<KnobDouble> cloneCenter;
    boost::shared_ptr<KnobChoice> cloneFilter;
    boost::shared_ptr<KnobBool> cloneBlackOutside;
    
    boost::shared_ptr<KnobChoice> sourceColor;
    boost::shared_ptr<KnobInt> timeOffset;
    boost::shared_ptr<KnobChoice> timeOffsetMode;
    
    std::list<boost::shared_ptr<KnobI> > knobs; //< list for easy access to all knobs

    RotoDrawableItemPrivate(bool isPaintingNode)
    : effectNode()
    , mergeNode()
    , timeOffsetNode()
    , frameHoldNode()
    , opacity()
    , feather()
    , featherFallOff()
    , lifeTime()
    , activated()
    , lifeTimeFrame()
#ifdef NATRON_ROTO_INVERTIBLE
    , inverted()
#endif
    , color()
    , compOperator()
    , translate()
    , rotate()
    , scale()
    , scaleUniform()
    , skewX()
    , skewY()
    , skewOrder()
    , center()
    , brushSize(new KnobDouble(NULL, kRotoBrushSizeParamLabel, 1, false))
    , brushSpacing(new KnobDouble(NULL, kRotoBrushSpacingParamLabel, 1, false))
    , brushHardness(new KnobDouble(NULL, kRotoBrushHardnessParamLabel, 1, false))
    , effectStrength(new KnobDouble(NULL, kRotoBrushEffectParamLabel, 1, false))
    , pressureOpacity(new KnobBool(NULL, kRotoBrushPressureLabelParamLabel, 1, false))
    , pressureSize(new KnobBool(NULL, kRotoBrushPressureSizeParamLabel, 1, false))
    , pressureHardness(new KnobBool(NULL, kRotoBrushPressureHardnessParamLabel, 1, false))
    , buildUp(new KnobBool(NULL, kRotoBrushBuildupParamLabel, 1, false))
    , visiblePortion(new KnobDouble(NULL, kRotoBrushVisiblePortionParamLabel, 2, false))
    , cloneTranslate(new KnobDouble(NULL, kRotoBrushTranslateParamLabel, 2, false))
    , cloneRotate(new KnobDouble(NULL, kRotoBrushRotateParamLabel, 1, false))
    , cloneScale(new KnobDouble(NULL, kRotoBrushScaleParamLabel, 2, false))
    , cloneScaleUniform(new KnobBool(NULL, kRotoBrushScaleUniformParamLabel, 1, false))
    , cloneSkewX(new KnobDouble(NULL, kRotoBrushSkewXParamLabel, 1, false))
    , cloneSkewY(new KnobDouble(NULL, kRotoBrushSkewYParamLabel, 1, false))
    , cloneSkewOrder(new KnobChoice(NULL, kRotoBrushSkewOrderParamLabel, 1, false))
    , cloneCenter(new KnobDouble(NULL, kRotoBrushCenterParamLabel, 2, false))
    , cloneFilter(new KnobChoice(NULL, kRotoBrushFilterParamLabel, 1, false))
    , cloneBlackOutside(new KnobBool(NULL, kRotoBrushBlackOutsideParamLabel, 1, false))
    , sourceColor(new KnobChoice(NULL, kRotoBrushSourceColorLabel, 1, false))
    , timeOffset(new KnobInt(NULL, kRotoBrushTimeOffsetParamLabel, 1, false))
    , timeOffsetMode(new KnobChoice(NULL, kRotoBrushTimeOffsetModeParamLabel, 1, false))
    , knobs()
    {
        opacity.reset(new KnobDouble(NULL, kRotoOpacityParamLabel, 1, false));
        opacity->setHintToolTip(kRotoOpacityHint);
        opacity->setName(kRotoOpacityParam);
        opacity->populate();
        opacity->setDefaultValue(ROTO_DEFAULT_OPACITY);
        knobs.push_back(opacity);
        
        feather.reset(new KnobDouble(NULL, kRotoFeatherParamLabel, 1, false));
        feather->setHintToolTip(kRotoFeatherHint);
        feather->setName(kRotoFeatherParam);
        feather->populate();
        feather->setDefaultValue(ROTO_DEFAULT_FEATHER);
        knobs.push_back(feather);
        
        featherFallOff.reset(new KnobDouble(NULL, kRotoFeatherFallOffParamLabel, 1, false));
        featherFallOff->setHintToolTip(kRotoFeatherFallOffHint);
        featherFallOff->setName(kRotoFeatherFallOffParam);
        featherFallOff->populate();
        featherFallOff->setDefaultValue(ROTO_DEFAULT_FEATHERFALLOFF);
        knobs.push_back(featherFallOff);
        
        
        lifeTime.reset(new KnobChoice(NULL, kRotoDrawableItemLifeTimeParamLabel, 1 , false));
        lifeTime->setHintToolTip(kRotoDrawableItemLifeTimeParamHint);
        lifeTime->populate();
        lifeTime->setName(kRotoDrawableItemLifeTimeParam);
        {
            std::vector<std::string> choices;
            choices.push_back(kRotoDrawableItemLifeTimeSingle);
            choices.push_back(kRotoDrawableItemLifeTimeFromStart);
            choices.push_back(kRotoDrawableItemLifeTimeToEnd);
            choices.push_back(kRotoDrawableItemLifeTimeCustom);
            lifeTime->populateChoices(choices);
        }
        lifeTime->setDefaultValue(isPaintingNode ? 0 : 3);
        knobs.push_back(lifeTime);
        
        lifeTimeFrame.reset(new KnobInt(NULL , kRotoDrawableItemLifeTimeFrameParamLabel, 1, false));
        lifeTimeFrame->setHintToolTip(kRotoDrawableItemLifeTimeFrameParamHint);
        lifeTimeFrame->setName(kRotoDrawableItemLifeTimeFrameParam);
        lifeTimeFrame->populate();
        knobs.push_back(lifeTimeFrame);
        
        activated.reset(new KnobBool(NULL, kRotoActivatedParamLabel, 1, false));
        activated->setHintToolTip(kRotoActivatedHint);
        activated->setName(kRotoActivatedParam);
        activated->populate();
        activated->setDefaultValue(true);
        knobs.push_back(activated);

#ifdef NATRON_ROTO_INVERTIBLE
        inverted.reset(new KnobBool(NULL, kRotoInvertedParamLable, 1, false));
        inverted->setHintToolTip(kRotoInvertedHint);
        inverted->setName(kRotoInvertedParam);
        inverted->populate();
        inverted->setDefaultValue(false);
        knobs.push_back(inverted);
#endif
        

        color.reset(new KnobColor(NULL, kRotoColorParamLabel, 3, false));
        color->setHintToolTip(kRotoColorHint);
        color->setName(kRotoColorParam);
        color->populate();
        color->setDefaultValue(ROTO_DEFAULT_COLOR_R, 0);
        color->setDefaultValue(ROTO_DEFAULT_COLOR_G, 1);
        color->setDefaultValue(ROTO_DEFAULT_COLOR_B, 2);
        knobs.push_back(color);

        compOperator.reset(new KnobChoice(NULL, kRotoCompOperatorParamLabel, 1, false));
        compOperator->setHintToolTip(kRotoCompOperatorHint);
        compOperator->setName(kRotoCompOperatorParam);
        compOperator->populate();
        knobs.push_back(compOperator);
        
        
        
        translate.reset(new KnobDouble(NULL, kRotoDrawableItemTranslateParamLabel, 2, false));
        translate->setName(kRotoDrawableItemTranslateParam);
        translate->setHintToolTip(kRotoDrawableItemTranslateParamHint);
        translate->populate();
        knobs.push_back(translate);
        
        rotate.reset(new KnobDouble(NULL, kRotoDrawableItemRotateParamLabel, 1, false));
        rotate->setName(kRotoDrawableItemRotateParam);
        rotate->setHintToolTip(kRotoDrawableItemRotateParamHint);
        rotate->populate();
        knobs.push_back(rotate);
        
        scale.reset(new KnobDouble(NULL, kRotoDrawableItemScaleParamLabel, 2, false));
        scale->setName(kRotoDrawableItemScaleParam);
        scale->setHintToolTip(kRotoDrawableItemScaleParamHint);
        scale->populate();
        scale->setDefaultValue(1,0);
        scale->setDefaultValue(1,1);
        knobs.push_back(scale);
        
        scaleUniform.reset(new KnobBool(NULL, kRotoDrawableItemScaleUniformParamLabel, 1, false));
        scaleUniform->setName(kRotoDrawableItemScaleUniformParam);
        scaleUniform->setHintToolTip(kRotoDrawableItemScaleUniformParamHint);
        scaleUniform->populate();
        scaleUniform->setDefaultValue(true);
        knobs.push_back(scaleUniform);
        
        skewX.reset(new KnobDouble(NULL, kRotoDrawableItemSkewXParamLabel, 1, false));
        skewX->setName(kRotoDrawableItemSkewXParam);
        skewX->setHintToolTip(kRotoDrawableItemSkewXParamHint);
        skewX->populate();
        knobs.push_back(skewX);
        
        skewY.reset(new KnobDouble(NULL, kRotoDrawableItemSkewYParamLabel, 1, false));
        skewY->setName(kRotoDrawableItemSkewYParam);
        skewY->setHintToolTip(kRotoDrawableItemSkewYParamHint);
        skewY->populate();
        knobs.push_back(skewY);
        
        skewOrder.reset(new KnobChoice(NULL, kRotoDrawableItemSkewOrderParamLabel, 1, false));
        skewOrder->setName(kRotoDrawableItemSkewOrderParam);
        skewOrder->setHintToolTip(kRotoDrawableItemSkewOrderParamHint);
        skewOrder->populate();
        knobs.push_back(skewOrder);
        {
            std::vector<std::string> choices;
            choices.push_back("XY");
            choices.push_back("YX");
            skewOrder->populateChoices(choices);
        }
        
        center.reset(new KnobDouble(NULL, kRotoDrawableItemCenterParamLabel, 2, false));
        center->setName(kRotoDrawableItemCenterParam);
        center->setHintToolTip(kRotoDrawableItemCenterParamHint);
        center->populate();
        knobs.push_back(center);
        
        brushSize->setName(kRotoBrushSizeParam);
        brushSize->setHintToolTip(kRotoBrushSizeParamHint);
        brushSize->populate();
        brushSize->setDefaultValue(25);
        brushSize->setMinimum(1);
        brushSize->setMaximum(1000);
        knobs.push_back(brushSize);
        
        brushSpacing->setName(kRotoBrushSpacingParam);
        brushSpacing->setHintToolTip(kRotoBrushSpacingParamHint);
        brushSpacing->populate();
        brushSpacing->setDefaultValue(0.1);
        brushSpacing->setMinimum(0);
        brushSpacing->setMaximum(1);
        knobs.push_back(brushSpacing);
        
        brushHardness->setName(kRotoBrushHardnessParam);
        brushHardness->setHintToolTip(kRotoBrushHardnessParamHint);
        brushHardness->populate();
        brushHardness->setDefaultValue(0.2);
        brushHardness->setMinimum(0);
        brushHardness->setMaximum(1);
        knobs.push_back(brushHardness);
        
        effectStrength->setName(kRotoBrushEffectParam);
        effectStrength->setHintToolTip(kRotoBrushEffectParamHint);
        effectStrength->populate();
        effectStrength->setDefaultValue(15);
        effectStrength->setMinimum(0);
        effectStrength->setMaximum(100);
        knobs.push_back(effectStrength);
        
        pressureOpacity->setName(kRotoBrushPressureOpacityParam);
        pressureOpacity->setHintToolTip(kRotoBrushPressureOpacityParamHint);
        pressureOpacity->populate();
        pressureOpacity->setAnimationEnabled(false);
        pressureOpacity->setDefaultValue(true);
        knobs.push_back(pressureOpacity);
        
        pressureSize->setName(kRotoBrushPressureSizeParam);
        pressureSize->populate();
        pressureSize->setHintToolTip(kRotoBrushPressureSizeParamHint);
        pressureSize->setAnimationEnabled(false);
        pressureSize->setDefaultValue(false);
        knobs.push_back(pressureSize);
        
        
        pressureHardness->setName(kRotoBrushPressureHardnessParam);
        pressureHardness->populate();
        pressureHardness->setHintToolTip(kRotoBrushPressureHardnessParamHint);
        pressureHardness->setAnimationEnabled(false);
        pressureHardness->setDefaultValue(false);
        knobs.push_back(pressureHardness);
        
        buildUp->setName(kRotoBrushBuildupParam);
        buildUp->populate();
        buildUp->setHintToolTip(kRotoBrushBuildupParamHint);
        buildUp->setDefaultValue(false);
        buildUp->setAnimationEnabled(false);
        buildUp->setDefaultValue(true);
        knobs.push_back(buildUp);
        
        
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
        knobs.push_back(visiblePortion);
        
        cloneTranslate->setName(kRotoBrushTranslateParam);
        cloneTranslate->setHintToolTip(kRotoBrushTranslateParamHint);
        cloneTranslate->populate();
        knobs.push_back(cloneTranslate);
        
        cloneRotate->setName(kRotoBrushRotateParam);
        cloneRotate->setHintToolTip(kRotoBrushRotateParamHint);
        cloneRotate->populate();
        knobs.push_back(cloneRotate);
        
        cloneScale->setName(kRotoBrushScaleParam);
        cloneScale->setHintToolTip(kRotoBrushScaleParamHint);
        cloneScale->populate();
        cloneScale->setDefaultValue(1,0);
        cloneScale->setDefaultValue(1,1);
        knobs.push_back(cloneScale);
        
        cloneScaleUniform->setName(kRotoBrushScaleUniformParam);
        cloneScaleUniform->setHintToolTip(kRotoBrushScaleUniformParamHint);
        cloneScaleUniform->populate();
        cloneScaleUniform->setDefaultValue(true);
        knobs.push_back(cloneScaleUniform);
        
        cloneSkewX->setName(kRotoBrushSkewXParam);
        cloneSkewX->setHintToolTip(kRotoBrushSkewXParamHint);
        cloneSkewX->populate();
        knobs.push_back(cloneSkewX);
        
        cloneSkewY->setName(kRotoBrushSkewYParam);
        cloneSkewY->setHintToolTip(kRotoBrushSkewYParamHint);
        cloneSkewY->populate();
        knobs.push_back(cloneSkewY);
        
        cloneSkewOrder->setName(kRotoBrushSkewOrderParam);
        cloneSkewOrder->setHintToolTip(kRotoBrushSkewOrderParamHint);
        cloneSkewOrder->populate();
        knobs.push_back(cloneSkewOrder);
        {
            std::vector<std::string> choices;
            choices.push_back("XY");
            choices.push_back("YX");
            cloneSkewOrder->populateChoices(choices);
        }
        
        cloneCenter->setName(kRotoBrushCenterParam);
        cloneCenter->setHintToolTip(kRotoBrushCenterParamHint);
        cloneCenter->populate();
        knobs.push_back(cloneCenter);
        
        
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
        knobs.push_back(cloneFilter);
        
        
        cloneBlackOutside->setName(kRotoBrushBlackOutsideParam);
        cloneBlackOutside->setHintToolTip(kRotoBrushBlackOutsideParamHint);
        cloneBlackOutside->populate();
        cloneBlackOutside->setDefaultValue(true);
        knobs.push_back(cloneBlackOutside);
        
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
        knobs.push_back(sourceColor);
        
        timeOffset->setName(kRotoBrushTimeOffsetParam);
        timeOffset->setHintToolTip(kRotoBrushTimeOffsetParamHint);
        timeOffset->populate();
        timeOffset->setDisplayMinimum(-100);
        timeOffset->setDisplayMaximum(100);
        knobs.push_back(timeOffset);
        
        timeOffsetMode->setName(kRotoBrushTimeOffsetModeParam);
        timeOffsetMode->setHintToolTip(kRotoBrushTimeOffsetModeParamHint);
        timeOffsetMode->populate();
        {
            std::vector<std::string> modes;
            modes.push_back("Relative");
            modes.push_back("Absolute");
            timeOffsetMode->populateChoices(modes);
        }
        knobs.push_back(timeOffsetMode);

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
    
    struct StrokeCurves
    {
        boost::shared_ptr<Curve> xCurve,yCurve,pressureCurve;
    };
    
    /**
     * @brief A list of all storkes contained in this item. Basically each time penUp() is called it makes a new stroke
     **/
    std::list<StrokeCurves> strokes;
    double curveT0; // timestamp of the first point in curve
    double lastTimestamp;
    RectD bbox;
    
    RectD wholeStrokeBboxWhilePainting;
    
    
    mutable QMutex strokeDotPatternsMutex;
    std::vector<cairo_pattern_t*> strokeDotPatterns;
    
    RotoStrokeItemPrivate(Natron::RotoStrokeType type)
    : type(type)
    , finished(false)
    , strokes()
    , curveT0(0)
    , lastTimestamp(0)
    , bbox()
    , wholeStrokeBboxWhilePainting()
    , strokeDotPatternsMutex()
    , strokeDotPatterns()
    {
        
        bbox.x1 = std::numeric_limits<double>::infinity();
        bbox.x2 = -std::numeric_limits<double>::infinity();
        bbox.y1 = std::numeric_limits<double>::infinity();
        bbox.y2 = -std::numeric_limits<double>::infinity();
                
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
    bool isCurrentlyLoading;
    boost::weak_ptr<Natron::Node> node;
    U64 age;

    ///These are knobs that take the value of the selected splines info.
    ///Their value changes when selection changes.
    boost::weak_ptr<KnobDouble> opacity;
    boost::weak_ptr<KnobDouble> feather;
    boost::weak_ptr<KnobDouble> featherFallOff;
    boost::weak_ptr<KnobChoice> lifeTime;
    boost::weak_ptr<KnobBool> activated; //<allows to disable a shape on a specific frame range
    boost::weak_ptr<KnobInt> lifeTimeFrame;
#ifdef NATRON_ROTO_INVERTIBLE
    boost::weak_ptr<KnobBool> inverted;
#endif
    boost::weak_ptr<KnobColor> colorKnob;
    
    boost::weak_ptr<KnobDouble> brushSizeKnob;
    boost::weak_ptr<KnobDouble> brushSpacingKnob;
    boost::weak_ptr<KnobDouble> brushHardnessKnob;
    boost::weak_ptr<KnobDouble> brushEffectKnob;
    boost::weak_ptr<KnobString> pressureLabelKnob;
    boost::weak_ptr<KnobBool> pressureOpacityKnob;
    boost::weak_ptr<KnobBool> pressureSizeKnob;
    boost::weak_ptr<KnobBool> pressureHardnessKnob;
    boost::weak_ptr<KnobBool> buildUpKnob;
    boost::weak_ptr<KnobDouble> brushVisiblePortionKnob;
    
    boost::weak_ptr<KnobDouble> cloneTranslateKnob;
    boost::weak_ptr<KnobDouble> cloneRotateKnob;
    boost::weak_ptr<KnobDouble> cloneScaleKnob;
    boost::weak_ptr<KnobBool> cloneUniformKnob;
    boost::weak_ptr<KnobDouble> cloneSkewXKnob;
    boost::weak_ptr<KnobDouble> cloneSkewYKnob;
    boost::weak_ptr<KnobChoice> cloneSkewOrderKnob;
    boost::weak_ptr<KnobDouble> cloneCenterKnob;
    boost::weak_ptr<KnobChoice> cloneFilterKnob;
    boost::weak_ptr<KnobBool> cloneBlackOutsideKnob;
    
    
    boost::weak_ptr<KnobDouble> translateKnob;
    boost::weak_ptr<KnobDouble> rotateKnob;
    boost::weak_ptr<KnobDouble> scaleKnob;
    boost::weak_ptr<KnobBool> scaleUniformKnob;
    boost::weak_ptr<KnobDouble> skewXKnob;
    boost::weak_ptr<KnobDouble> skewYKnob;
    boost::weak_ptr<KnobChoice> skewOrderKnob;
    boost::weak_ptr<KnobDouble> centerKnob;
    
    boost::weak_ptr<KnobChoice> sourceTypeKnob;
    boost::weak_ptr<KnobInt> timeOffsetKnob;
    boost::weak_ptr<KnobChoice> timeOffsetModeKnob;

    std::list<boost::weak_ptr<KnobI> > knobs; //< list for easy access to all knobs
    std::list<boost::weak_ptr<KnobI> > cloneKnobs;
    std::list<boost::weak_ptr<KnobI> > strokeKnobs;
    std::list<boost::weak_ptr<KnobI> > shapeKnobs;

    ///This keeps track  of the items linked to the context knobs
    std::list<boost::shared_ptr<RotoItem> > selectedItems;
    boost::shared_ptr<RotoItem> lastInsertedItem;
    boost::shared_ptr<RotoItem> lastLockedItem;
    boost::shared_ptr<RotoStrokeItem> strokeBeingPainted;
    
    //Used to prevent 2 threads from writing the same image in the rotocontext
    mutable QMutex cacheAccessMutex;
    
    mutable QMutex doingNeatRenderMutex;
    QWaitCondition doingNeatRenderCond;
    bool doingNeatRender;
    bool mustDoNeatRender;

    RotoContextPrivate(const boost::shared_ptr<Natron::Node>& n )
    : rotoContextMutex()
    , isPaintNode(n->isRotoPaintingNode())
    , layers()
    , autoKeying(true)
    , rippleEdit(false)
    , featherLink(true)
    , isCurrentlyLoading(false)
    , node(n)
    , age(0)
    , doingNeatRender(false)
    , mustDoNeatRender(false)
    {
        assert( n && n->getLiveInstance() );
        Natron::EffectInstance* effect = n->getLiveInstance();
        
        boost::shared_ptr<KnobPage> shapePage,strokePage,generalPage,clonePage,transformPage;
        
        generalPage = Natron::createKnob<KnobPage>(effect, "General", 1, false);
        shapePage = Natron::createKnob<KnobPage>(effect, "Shape", 1, false);
        strokePage = Natron::createKnob<KnobPage>(effect, "Stroke", 1, false);
        clonePage = Natron::createKnob<KnobPage>(effect, "Clone", 1, false);
        transformPage = Natron::createKnob<KnobPage>(effect, "Transform", 1, false);
        
        boost::shared_ptr<KnobDouble> opacityKnob = Natron::createKnob<KnobDouble>(effect, kRotoOpacityParamLabel, 1, false);
        opacityKnob->setHintToolTip(kRotoOpacityHint);
        opacityKnob->setName(kRotoOpacityParam);
        opacityKnob->setMinimum(0.);
        opacityKnob->setMaximum(1.);
        opacityKnob->setDisplayMinimum(0.);
        opacityKnob->setDisplayMaximum(1.);
        opacityKnob->setDefaultValue(ROTO_DEFAULT_OPACITY);
        opacityKnob->setDefaultAllDimensionsEnabled(false);
        opacityKnob->setIsPersistant(false);
        generalPage->addKnob(opacityKnob);
        knobs.push_back(opacityKnob);
        opacity = opacityKnob;
        
        boost::shared_ptr<KnobColor> ck = Natron::createKnob<KnobColor>(effect, kRotoColorParamLabel, 3, false);
        ck->setHintToolTip(kRotoColorHint);
        ck->setName(kRotoColorParam);
        ck->setDefaultValue(ROTO_DEFAULT_COLOR_R, 0);
        ck->setDefaultValue(ROTO_DEFAULT_COLOR_G, 1);
        ck->setDefaultValue(ROTO_DEFAULT_COLOR_B, 2);
        ck->setDefaultAllDimensionsEnabled(false);
        generalPage->addKnob(ck);
        ck->setIsPersistant(false);
        knobs.push_back(ck);
        colorKnob = ck;
        
        boost::shared_ptr<KnobChoice> lifeTimeKnob = Natron::createKnob<KnobChoice>(effect, kRotoDrawableItemLifeTimeParamLabel, 1, false);
        lifeTimeKnob->setHintToolTip(kRotoDrawableItemLifeTimeParamHint);
        lifeTimeKnob->setName(kRotoDrawableItemLifeTimeParam);
        lifeTimeKnob->setAddNewLine(false);
        lifeTimeKnob->setIsPersistant(false);
        lifeTimeKnob->setDefaultAllDimensionsEnabled(false);
        lifeTimeKnob->setAnimationEnabled(false);
        {
            std::vector<std::string> choices,helps;
            choices.push_back(kRotoDrawableItemLifeTimeSingle);
            helps.push_back(kRotoDrawableItemLifeTimeSingleHelp);
            choices.push_back(kRotoDrawableItemLifeTimeFromStart);
            helps.push_back(kRotoDrawableItemLifeTimeFromStartHelp);
            choices.push_back(kRotoDrawableItemLifeTimeToEnd);
            helps.push_back(kRotoDrawableItemLifeTimeToEndHelp);
            choices.push_back(kRotoDrawableItemLifeTimeCustom);
            helps.push_back(kRotoDrawableItemLifeTimeCustomHelp);
            lifeTimeKnob->populateChoices(choices,helps);
        }
        lifeTimeKnob->setDefaultValue(isPaintNode ? 0 : 3);
        generalPage->addKnob(lifeTimeKnob);
        knobs.push_back(lifeTimeKnob);
        lifeTime = lifeTimeKnob;
        
        boost::shared_ptr<KnobInt> lifeTimeFrameKnob = Natron::createKnob<KnobInt>(effect, kRotoDrawableItemLifeTimeFrameParamLabel, 1, false);
        lifeTimeFrameKnob->setHintToolTip(kRotoDrawableItemLifeTimeFrameParamHint);
        lifeTimeFrameKnob->setName(kRotoDrawableItemLifeTimeFrameParam);
        lifeTimeFrameKnob->setSecretByDefault(!isPaintNode);
        lifeTimeFrameKnob->setDefaultAllDimensionsEnabled(false);
        lifeTimeFrameKnob->setAddNewLine(false);
        lifeTimeFrameKnob->setAnimationEnabled(false);
        generalPage->addKnob(lifeTimeFrameKnob);
        knobs.push_back(lifeTimeFrameKnob);
        lifeTimeFrame = lifeTimeFrameKnob;
        
        boost::shared_ptr<KnobBool> activatedKnob = Natron::createKnob<KnobBool>(effect, kRotoActivatedParamLabel, 1, false);
        activatedKnob->setHintToolTip(kRotoActivatedHint);
        activatedKnob->setName(kRotoActivatedParam);
        activatedKnob->setAddNewLine(true);
        activatedKnob->setSecretByDefault(isPaintNode);
        activatedKnob->setDefaultValue(true);
        activatedKnob->setDefaultAllDimensionsEnabled(false);
        generalPage->addKnob(activatedKnob);
        activatedKnob->setIsPersistant(false);
        knobs.push_back(activatedKnob);
        activated = activatedKnob;
        
#ifdef NATRON_ROTO_INVERTIBLE
        boost::shared_ptr<KnobBool> invertedKnob = Natron::createKnob<KnobBool>(effect, kRotoInvertedParamLabel, 1, false);
        invertedKnob->setHintToolTip(kRotoInvertedHint);
        invertedKnob->setName(kRotoInvertedParam);
        invertedKnob->setDefaultValue(false);
        invertedKnob->setDefaultAllDimensionsEnabled(false);
        invertedKnob->setIsPersistant(false);
        generalPage->addKnob(invertedKnob);
        knobs.push_back(invertedKnob);
        inverted = invertedKnob;
#endif
        
        boost::shared_ptr<KnobDouble> featherKnob = Natron::createKnob<KnobDouble>(effect, kRotoFeatherParamLabel, 1, false);
        featherKnob->setHintToolTip(kRotoFeatherHint);
        featherKnob->setName(kRotoFeatherParam);
        featherKnob->setMinimum(-100);
        featherKnob->setMaximum(100);
        featherKnob->setDisplayMinimum(-100);
        featherKnob->setDisplayMaximum(100);
        featherKnob->setDefaultValue(ROTO_DEFAULT_FEATHER);
        featherKnob->setDefaultAllDimensionsEnabled(false);
        featherKnob->setIsPersistant(false);
        shapePage->addKnob(featherKnob);
        knobs.push_back(featherKnob);
        shapeKnobs.push_back(featherKnob);
        feather = featherKnob;
        
        boost::shared_ptr<KnobDouble> featherFallOffKnob = Natron::createKnob<KnobDouble>(effect, kRotoFeatherFallOffParamLabel, 1, false);
        featherFallOffKnob->setHintToolTip(kRotoFeatherFallOffHint);
        featherFallOffKnob->setName(kRotoFeatherFallOffParam);
        featherFallOffKnob->setMinimum(0.001);
        featherFallOffKnob->setMaximum(5.);
        featherFallOffKnob->setDisplayMinimum(0.2);
        featherFallOffKnob->setDisplayMaximum(5.);
        featherFallOffKnob->setDefaultValue(ROTO_DEFAULT_FEATHERFALLOFF);
        featherFallOffKnob->setDefaultAllDimensionsEnabled(false);
        featherFallOffKnob->setIsPersistant(false);
        shapePage->addKnob(featherFallOffKnob);
        knobs.push_back(featherFallOffKnob);
        shapeKnobs.push_back(featherFallOffKnob);
        featherFallOff = featherFallOffKnob;
        
        {
            boost::shared_ptr<KnobChoice> sourceType = Natron::createKnob<KnobChoice>(effect,kRotoBrushSourceColorLabel,1,false);
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
            sourceType->setDefaultAllDimensionsEnabled(false);
            clonePage->addKnob(sourceType);
            knobs.push_back(sourceType);
            cloneKnobs.push_back(sourceType);
            sourceTypeKnob = sourceType;
            
            boost::shared_ptr<KnobDouble> translate = Natron::createKnob<KnobDouble>(effect, kRotoBrushTranslateParamLabel, 2, false);
            translate->setName(kRotoBrushTranslateParam);
            translate->setHintToolTip(kRotoBrushTranslateParamHint);
            translate->setDefaultAllDimensionsEnabled(false);
            translate->setIncrement(10);
            clonePage->addKnob(translate);
            knobs.push_back(translate);
            cloneKnobs.push_back(translate);
            cloneTranslateKnob = translate;
            
            boost::shared_ptr<KnobDouble> rotate = Natron::createKnob<KnobDouble>(effect, kRotoBrushRotateParamLabel, 1, false);
            rotate->setName(kRotoBrushRotateParam);
            rotate->setHintToolTip(kRotoBrushRotateParamHint);
            rotate->setDefaultAllDimensionsEnabled(false);
            rotate->setDisplayMinimum(-180);
            rotate->setDisplayMaximum(180);
            clonePage->addKnob(rotate);
            knobs.push_back(rotate);
            cloneKnobs.push_back(rotate);
            cloneRotateKnob = rotate;
            
            boost::shared_ptr<KnobDouble> scale = Natron::createKnob<KnobDouble>(effect, kRotoBrushScaleParamLabel, 2, false);
            scale->setName(kRotoBrushScaleParam);
            scale->setHintToolTip(kRotoBrushScaleParamHint);
            scale->setDefaultValue(1,0);
            scale->setDefaultValue(1,1);
            scale->setDisplayMinimum(0.1,0);
            scale->setDisplayMinimum(0.1,1);
            scale->setDisplayMaximum(10,0);
            scale->setDisplayMaximum(10,1);
            scale->setAddNewLine(false);
            scale->setDefaultAllDimensionsEnabled(false);
            clonePage->addKnob(scale);
            cloneKnobs.push_back(scale);
            knobs.push_back(scale);
            cloneScaleKnob = scale;
            
            boost::shared_ptr<KnobBool> scaleUniform = Natron::createKnob<KnobBool>(effect, kRotoBrushScaleUniformParamLabel, 1, false);
            scaleUniform->setName(kRotoBrushScaleUniformParam);
            scaleUniform->setHintToolTip(kRotoBrushScaleUniformParamHint);
            scaleUniform->setDefaultValue(true);
            scaleUniform->setDefaultAllDimensionsEnabled(false);
            scaleUniform->setAnimationEnabled(false);
            clonePage->addKnob(scaleUniform);
            cloneKnobs.push_back(scaleUniform);
            knobs.push_back(scaleUniform);
            cloneUniformKnob = scaleUniform;
            
            boost::shared_ptr<KnobDouble> skewX = Natron::createKnob<KnobDouble>(effect, kRotoBrushSkewXParamLabel, 1, false);
            skewX->setName(kRotoBrushSkewXParam);
            skewX->setHintToolTip(kRotoBrushSkewXParamHint);
            skewX->setDefaultAllDimensionsEnabled(false);
            skewX->setDisplayMinimum(-1,0);
            skewX->setDisplayMaximum(1,0);
            cloneKnobs.push_back(skewX);
            clonePage->addKnob(skewX);
            knobs.push_back(skewX);
            cloneSkewXKnob = skewX;
            
            boost::shared_ptr<KnobDouble> skewY = Natron::createKnob<KnobDouble>(effect, kRotoBrushSkewYParamLabel, 1, false);
            skewY->setName(kRotoBrushSkewYParam);
            skewY->setHintToolTip(kRotoBrushSkewYParamHint);
            skewY->setDefaultAllDimensionsEnabled(false);
            skewY->setDisplayMinimum(-1,0);
            skewY->setDisplayMaximum(1,0);
            clonePage->addKnob(skewY);
            cloneKnobs.push_back(skewY);
            knobs.push_back(skewY);
            cloneSkewYKnob = skewY;

            boost::shared_ptr<KnobChoice> skewOrder = Natron::createKnob<KnobChoice>(effect,kRotoBrushSkewOrderParamLabel,1,false);
            skewOrder->setName(kRotoBrushSkewOrderParam);
            skewOrder->setHintToolTip(kRotoBrushSkewOrderParamHint);
            skewOrder->setDefaultValue(0);
            {
                std::vector<std::string> choices;
                choices.push_back("XY");
                choices.push_back("YX");
                skewOrder->populateChoices(choices);
            }
            skewOrder->setDefaultAllDimensionsEnabled(false);
            skewOrder->setAnimationEnabled(false);
            clonePage->addKnob(skewOrder);
            cloneKnobs.push_back(skewOrder);
            knobs.push_back(skewOrder);
            cloneSkewOrderKnob = skewOrder;
            
            boost::shared_ptr<KnobDouble> center = Natron::createKnob<KnobDouble>(effect, kRotoBrushCenterParamLabel, 2, false);
            center->setName(kRotoBrushCenterParam);
            center->setHintToolTip(kRotoBrushCenterParamHint);
            center->setDefaultAllDimensionsEnabled(false);
            center->setAnimationEnabled(false);
            center->setDefaultValuesAreNormalized(true);
            center->setDefaultValue(0.5, 0);
            center->setDefaultValue(0.5, 1);
            clonePage->addKnob(center);
            cloneKnobs.push_back(center);
            knobs.push_back(center);
            cloneCenterKnob = center;
            
            boost::shared_ptr<KnobChoice> filter = Natron::createKnob<KnobChoice>(effect,kRotoBrushFilterParamLabel,1,false);
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
            filter->setDefaultAllDimensionsEnabled(false);
            filter->setAddNewLine(false);
            clonePage->addKnob(filter);
            cloneKnobs.push_back(filter);
            knobs.push_back(filter);
            cloneFilterKnob = filter;

            boost::shared_ptr<KnobBool> blackOutside = Natron::createKnob<KnobBool>(effect, kRotoBrushBlackOutsideParamLabel, 1, false);
            blackOutside->setName(kRotoBrushBlackOutsideParam);
            blackOutside->setHintToolTip(kRotoBrushBlackOutsideParamHint);
            blackOutside->setDefaultValue(true);
            blackOutside->setDefaultAllDimensionsEnabled(false);
            clonePage->addKnob(blackOutside);
            knobs.push_back(blackOutside);
            cloneKnobs.push_back(blackOutside);
            cloneBlackOutsideKnob = blackOutside;
            
            boost::shared_ptr<KnobInt> timeOffset = Natron::createKnob<KnobInt>(effect, kRotoBrushTimeOffsetParamLabel, 1, false);
            timeOffset->setName(kRotoBrushTimeOffsetParam);
            timeOffset->setHintToolTip(kRotoBrushTimeOffsetParamHint);
            timeOffset->setDisplayMinimum(-100);
            timeOffset->setDisplayMaximum(100);
            timeOffset->setDefaultAllDimensionsEnabled(false);
            timeOffset->setIsPersistant(false);
            timeOffset->setAddNewLine(false);
            clonePage->addKnob(timeOffset);
            cloneKnobs.push_back(timeOffset);
            knobs.push_back(timeOffset);
            timeOffsetKnob = timeOffset;
            
            boost::shared_ptr<KnobChoice> timeOffsetMode = Natron::createKnob<KnobChoice>(effect, kRotoBrushTimeOffsetModeParamLabel, 1, false);
            timeOffsetMode->setName(kRotoBrushTimeOffsetModeParam);
            timeOffsetMode->setHintToolTip(kRotoBrushTimeOffsetModeParamHint);
            {
                std::vector<std::string> modes;
                modes.push_back("Relative");
                modes.push_back("Absolute");
                timeOffsetMode->populateChoices(modes);
            }
            timeOffsetMode->setDefaultAllDimensionsEnabled(false);
            timeOffsetMode->setIsPersistant(false);
            clonePage->addKnob(timeOffsetMode);
            knobs.push_back(timeOffsetMode);
            cloneKnobs.push_back(timeOffsetMode);
            timeOffsetModeKnob = timeOffsetMode;
            
            boost::shared_ptr<KnobDouble> brushSize = Natron::createKnob<KnobDouble>(effect, kRotoBrushSizeParamLabel, 1, false);
            brushSize->setName(kRotoBrushSizeParam);
            brushSize->setHintToolTip(kRotoBrushSizeParamHint);
            brushSize->setDefaultValue(25);
            brushSize->setMinimum(1.);
            brushSize->setMaximum(1000);
            brushSize->setDefaultAllDimensionsEnabled(false);
            brushSize->setIsPersistant(false);
            strokePage->addKnob(brushSize);
            knobs.push_back(brushSize);
            strokeKnobs.push_back(brushSize);
            brushSizeKnob = brushSize;
            
            boost::shared_ptr<KnobDouble> brushSpacing = Natron::createKnob<KnobDouble>(effect, kRotoBrushSpacingParamLabel, 1, false);
            brushSpacing->setName(kRotoBrushSpacingParam);
            brushSpacing->setHintToolTip(kRotoBrushSpacingParamHint);
            brushSpacing->setDefaultValue(0.1);
            brushSpacing->setMinimum(0.);
            brushSpacing->setMaximum(1.);
            brushSpacing->setDefaultAllDimensionsEnabled(false);
            brushSpacing->setIsPersistant(false);
            strokePage->addKnob(brushSpacing);
            knobs.push_back(brushSpacing);
            strokeKnobs.push_back(brushSpacing);
            brushSpacingKnob = brushSpacing;
            
            boost::shared_ptr<KnobDouble> brushHardness = Natron::createKnob<KnobDouble>(effect, kRotoBrushHardnessParamLabel, 1, false);
            brushHardness->setName(kRotoBrushHardnessParam);
            brushHardness->setHintToolTip(kRotoBrushHardnessParamHint);
            brushHardness->setDefaultValue(0.2);
            brushHardness->setMinimum(0.);
            brushHardness->setMaximum(1.);
            brushHardness->setDefaultAllDimensionsEnabled(false);
            brushHardness->setIsPersistant(false);
            strokePage->addKnob(brushHardness);
            knobs.push_back(brushHardness);
            strokeKnobs.push_back(brushHardness);
            brushHardnessKnob = brushHardness;
            
            boost::shared_ptr<KnobDouble> effectStrength = Natron::createKnob<KnobDouble>(effect, kRotoBrushEffectParamLabel, 1, false);
            effectStrength->setName(kRotoBrushEffectParam);
            effectStrength->setHintToolTip(kRotoBrushEffectParamHint);
            effectStrength->setDefaultValue(15);
            effectStrength->setMinimum(0.);
            effectStrength->setMaximum(100.);
            effectStrength->setDefaultAllDimensionsEnabled(false);
            effectStrength->setIsPersistant(false);
            strokePage->addKnob(effectStrength);
            knobs.push_back(effectStrength);
            strokeKnobs.push_back(effectStrength);
            brushEffectKnob = effectStrength;
            
            boost::shared_ptr<KnobString> pressureLabel = Natron::createKnob<KnobString>(effect, kRotoBrushPressureLabelParamLabel);
            pressureLabel->setName(kRotoBrushPressureLabelParam);
            pressureLabel->setHintToolTip(kRotoBrushPressureLabelParamHint);
            pressureLabel->setAsLabel();
            pressureLabel->setAnimationEnabled(false);
            pressureLabel->setDefaultAllDimensionsEnabled(false);
            strokePage->addKnob(pressureLabel);
            knobs.push_back(pressureLabel);
            strokeKnobs.push_back(pressureLabel);
            pressureLabelKnob = pressureLabel;
            
            boost::shared_ptr<KnobBool> pressureOpacity = Natron::createKnob<KnobBool>(effect, kRotoBrushPressureOpacityParamLabel);
            pressureOpacity->setName(kRotoBrushPressureOpacityParam);
            pressureOpacity->setHintToolTip(kRotoBrushPressureOpacityParamHint);
            pressureOpacity->setAnimationEnabled(false);
            pressureOpacity->setDefaultValue(true);
            pressureOpacity->setAddNewLine(false);
            pressureOpacity->setDefaultAllDimensionsEnabled(false);
            pressureOpacity->setIsPersistant(false);
            strokePage->addKnob(pressureOpacity);
            knobs.push_back(pressureOpacity);
            strokeKnobs.push_back(pressureOpacity);
            pressureOpacityKnob = pressureOpacity;
            
            boost::shared_ptr<KnobBool> pressureSize = Natron::createKnob<KnobBool>(effect, kRotoBrushPressureSizeParamLabel);
            pressureSize->setName(kRotoBrushPressureSizeParam);
            pressureSize->setHintToolTip(kRotoBrushPressureSizeParamHint);
            pressureSize->setAnimationEnabled(false);
            pressureSize->setDefaultValue(false);
            pressureSize->setAddNewLine(false);
            pressureSize->setDefaultAllDimensionsEnabled(false);
            pressureSize->setIsPersistant(false);
            knobs.push_back(pressureSize);
            strokeKnobs.push_back(pressureSize);
            strokePage->addKnob(pressureSize);
            pressureSizeKnob = pressureSize;
            
            boost::shared_ptr<KnobBool> pressureHardness = Natron::createKnob<KnobBool>(effect, kRotoBrushPressureHardnessParamLabel);
            pressureHardness->setName(kRotoBrushPressureHardnessParam);
            pressureHardness->setHintToolTip(kRotoBrushPressureHardnessParamHint);
            pressureHardness->setAnimationEnabled(false);
            pressureHardness->setDefaultValue(false);
            pressureHardness->setAddNewLine(true);
            pressureHardness->setDefaultAllDimensionsEnabled(false);
            pressureHardness->setIsPersistant(false);
            knobs.push_back(pressureHardness);
            strokeKnobs.push_back(pressureHardness);
            strokePage->addKnob(pressureHardness);
            pressureHardnessKnob = pressureHardness;
            
            boost::shared_ptr<KnobBool> buildUp = Natron::createKnob<KnobBool>(effect, kRotoBrushBuildupParamLabel);
            buildUp->setName(kRotoBrushBuildupParam);
            buildUp->setHintToolTip(kRotoBrushBuildupParamHint);
            buildUp->setAnimationEnabled(false);
            buildUp->setDefaultValue(false);
            buildUp->setAddNewLine(true);
            buildUp->setDefaultAllDimensionsEnabled(false);
            buildUp->setIsPersistant(false);
            knobs.push_back(buildUp);
            strokeKnobs.push_back(buildUp);
            strokePage->addKnob(buildUp);
            buildUpKnob = buildUp;
            
            boost::shared_ptr<KnobDouble> visiblePortion = Natron::createKnob<KnobDouble>(effect, kRotoBrushVisiblePortionParamLabel, 2, false);
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
            visiblePortion->setDefaultAllDimensionsEnabled(false);
            visiblePortion->setIsPersistant(false);
            strokePage->addKnob(visiblePortion);
            visiblePortion->setDimensionName(0, "start");
            visiblePortion->setDimensionName(1, "end");
            knobs.push_back(visiblePortion);
            strokeKnobs.push_back(visiblePortion);
            brushVisiblePortionKnob = visiblePortion;
    }
    
        boost::shared_ptr<KnobDouble> translate = Natron::createKnob<KnobDouble>(effect, kRotoDrawableItemTranslateParamLabel, 2, false);
        translate->setName(kRotoDrawableItemTranslateParam);
        translate->setHintToolTip(kRotoDrawableItemTranslateParamHint);
        translate->setDefaultAllDimensionsEnabled(false);
        translate->setIncrement(10);
        transformPage->addKnob(translate);
        knobs.push_back(translate);
        translateKnob = translate;
        
        boost::shared_ptr<KnobDouble> rotate = Natron::createKnob<KnobDouble>(effect, kRotoDrawableItemRotateParamLabel, 1, false);
        rotate->setName(kRotoDrawableItemRotateParam);
        rotate->setHintToolTip(kRotoDrawableItemRotateParamHint);
        rotate->setDefaultAllDimensionsEnabled(false);
        rotate->setDisplayMinimum(-180);
        rotate->setDisplayMaximum(180);
        transformPage->addKnob(rotate);
        knobs.push_back(rotate);
        rotateKnob = rotate;
        
        boost::shared_ptr<KnobDouble> scale = Natron::createKnob<KnobDouble>(effect, kRotoDrawableItemScaleParamLabel, 2, false);
        scale->setName(kRotoDrawableItemScaleParam);
        scale->setHintToolTip(kRotoDrawableItemScaleParamHint);
        scale->setDefaultValue(1,0);
        scale->setDefaultValue(1,1);
        scale->setDisplayMinimum(0.1,0);
        scale->setDisplayMinimum(0.1,1);
        scale->setDisplayMaximum(10,0);
        scale->setDisplayMaximum(10,1);
        scale->setAddNewLine(false);
        scale->setDefaultAllDimensionsEnabled(false);
        transformPage->addKnob(scale);
        knobs.push_back(scale);
        scaleKnob = scale;
        
        boost::shared_ptr<KnobBool> scaleUniform = Natron::createKnob<KnobBool>(effect, kRotoDrawableItemScaleUniformParamLabel, 1, false);
        scaleUniform->setName(kRotoDrawableItemScaleUniformParam);
        scaleUniform->setHintToolTip(kRotoDrawableItemScaleUniformParamHint);
        scaleUniform->setDefaultValue(true);
        scaleUniform->setDefaultAllDimensionsEnabled(false);
        scaleUniform->setAnimationEnabled(false);
        transformPage->addKnob(scaleUniform);
        knobs.push_back(scaleUniform);
        scaleUniformKnob = scaleUniform;
        
        boost::shared_ptr<KnobDouble> skewX = Natron::createKnob<KnobDouble>(effect, kRotoDrawableItemSkewXParamLabel, 1, false);
        skewX->setName(kRotoDrawableItemSkewXParam);
        skewX->setHintToolTip(kRotoDrawableItemSkewXParamHint);
        skewX->setDefaultAllDimensionsEnabled(false);
        skewX->setDisplayMinimum(-1,0);
        skewX->setDisplayMaximum(1,0);
        transformPage->addKnob(skewX);
        knobs.push_back(skewX);
        skewXKnob = skewX;
        
        boost::shared_ptr<KnobDouble> skewY = Natron::createKnob<KnobDouble>(effect, kRotoDrawableItemSkewYParamLabel, 1, false);
        skewY->setName(kRotoDrawableItemSkewYParam);
        skewY->setHintToolTip(kRotoDrawableItemSkewYParamHint);
        skewY->setDefaultAllDimensionsEnabled(false);
        skewY->setDisplayMinimum(-1,0);
        skewY->setDisplayMaximum(1,0);
        transformPage->addKnob(skewY);
        knobs.push_back(skewY);
        skewYKnob = skewY;
        
        boost::shared_ptr<KnobChoice> skewOrder = Natron::createKnob<KnobChoice>(effect,kRotoDrawableItemSkewOrderParamLabel,1,false);
        skewOrder->setName(kRotoDrawableItemSkewOrderParam);
        skewOrder->setHintToolTip(kRotoDrawableItemSkewOrderParamHint);
        skewOrder->setDefaultValue(0);
        {
            std::vector<std::string> choices;
            choices.push_back("XY");
            choices.push_back("YX");
            skewOrder->populateChoices(choices);
        }
        skewOrder->setDefaultAllDimensionsEnabled(false);
        skewOrder->setAnimationEnabled(false);
        transformPage->addKnob(skewOrder);
        knobs.push_back(skewOrder);
        skewOrderKnob = skewOrder;
        
        boost::shared_ptr<KnobDouble> center = Natron::createKnob<KnobDouble>(effect, kRotoDrawableItemCenterParamLabel, 2, false);
        center->setName(kRotoDrawableItemCenterParam);
        center->setHintToolTip(kRotoDrawableItemCenterParamHint);
        center->setDefaultAllDimensionsEnabled(false);
        center->setAnimationEnabled(false);
        center->setDefaultValuesAreNormalized(true);
        center->setDefaultValue(0.5, 0);
        center->setDefaultValue(0.5, 1);
        transformPage->addKnob(center);
        knobs.push_back(center);
        centerKnob = center;
        
        node.lock()->addTransformInteract(translate, scale, scaleUniform, rotate, skewX, skewY, skewOrder, center);
        
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
    
    boost::shared_ptr<RotoLayer>
    findDeepestSelectedLayer() const
    {
        assert( !rotoContextMutex.tryLock() );
        
        int minLevel = -1;
        boost::shared_ptr<RotoLayer> minLayer;
        for (std::list< boost::shared_ptr<RotoItem> >::const_iterator it = selectedItems.begin();
             it != selectedItems.end(); ++it) {
            int lvl = (*it)->getHierarchyLevel();
            if (lvl > minLevel) {
                boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
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
                        const std::list<std::list<std::pair<Natron::Point,double> > >& strokes,
                        double distToNext,
                        const boost::shared_ptr<RotoDrawableItem>& stroke,
                        bool doBuildup,
                        double opacity, 
                        double time,
                        unsigned int mipmapLevel);
    
    void renderBezier(cairo_t* cr,const Bezier* bezier, double opacity, double time, unsigned int mipmapLevel);
    
    void renderFeather(const Bezier* bezier,double time, unsigned int mipmapLevel, bool inverted, double shapeColor[3], double opacity, double featherDist, double fallOff, cairo_pattern_t* mesh);

    void renderInternalShape(double time,unsigned int mipmapLevel,double shapeColor[3], double opacity,const Transform::Matrix3x3& transform, cairo_t* cr, cairo_pattern_t* mesh, const BezierCPs & cps);
    
    static void bezulate(double time,const BezierCPs& cps,std::list<BezierCPs>* patches);

    void applyAndDestroyMask(cairo_t* cr,cairo_pattern_t* mesh);
};


#endif // ROTOCONTEXTPRIVATE_H
