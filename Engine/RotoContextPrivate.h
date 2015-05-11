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
#include "Engine/EffectInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Rect.h"
#include "Engine/FitCurve.h"
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

///Keep this in synch with the cairo_operator_t enum !
///We are not going to create a similar enum just to represent the same thing
inline void
getCompositingOperators(std::vector<std::string>* operators,
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

    RotoDrawableItemPrivate()
        : opacity( new Double_Knob(NULL, kRotoOpacityParamLabel, 1, false) )
          , feather( new Double_Knob(NULL, kRotoFeatherParamLabel, 1, false) )
          , featherFallOff( new Double_Knob(NULL, kRotoFeatherFallOffParamLabel, 1, false) )
          , activated( new Bool_Knob(NULL, kRotoActivatedParamLabel, 1, false) )
#ifdef NATRON_ROTO_INVERTIBLE
          , inverted( new Bool_Knob(NULL, kRotoInvertedParamLable, 1, false) )
#endif
          , color( new Color_Knob(NULL, kRotoColorParamLabel, 3, false) )
          , compOperator( new Choice_Knob(NULL, kRotoCompOperatorParamLabel, 1, false) )
          , knobs()
    {
        opacity->setHintToolTip(kRotoOpacityHint);
        opacity->setName(kRotoOpacityParam);
        opacity->populate();
        opacity->setDefaultValue(ROTO_DEFAULT_OPACITY);
        knobs.push_back(opacity);

        feather->setHintToolTip(kRotoFeatherHint);
        feather->setName(kRotoFeatherParam);
        feather->populate();
        feather->setDefaultValue(ROTO_DEFAULT_FEATHER);
        knobs.push_back(feather);

        featherFallOff->setHintToolTip(kRotoFeatherFallOffHint);
        featherFallOff->setName(kRotoFeatherFallOffParam);
        featherFallOff->populate();
        featherFallOff->setDefaultValue(ROTO_DEFAULT_FEATHERFALLOFF);
        knobs.push_back(featherFallOff);

        activated->setHintToolTip(kRotoActivatedHint);
        activated->setName(kRotoActivatedParam);
        activated->populate();
        activated->setDefaultValue(true);
        knobs.push_back(activated);

#ifdef NATRON_ROTO_INVERTIBLE
        inverted->setHintToolTip(kRotoInvertedHint);
        inverted->setName(kRotoInvertedParam);
        inverted->populate();
        inverted->setDefaultValue(false);
        knobs.push_back(inverted);
#endif
        

        color->setHintToolTip(kRotoColorHint);
        color->setName(kRotoColorParam);
        color->populate();
        color->setDefaultValue(ROTO_DEFAULT_COLOR_R, 0);
        color->setDefaultValue(ROTO_DEFAULT_COLOR_G, 1);
        color->setDefaultValue(ROTO_DEFAULT_COLOR_B, 2);
        knobs.push_back(color);

        compOperator->setHintToolTip(kRotoCompOperatorHint);
        compOperator->setName(kRotoCompOperatorParam);
        compOperator->populate();
        std::vector<std::string> operators;
        std::vector<std::string> tooltips;
        getCompositingOperators(&operators, &tooltips);
        compOperator->populateChoices(operators,tooltips);
        compOperator->setDefaultValue( (int)CAIRO_OPERATOR_OVER );
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
    boost::shared_ptr<Double_Knob> brushSize;
    boost::shared_ptr<Double_Knob> brushSpacing;
    boost::shared_ptr<Double_Knob> brushHardness;
    boost::shared_ptr<Double_Knob> effectStrength;
    boost::shared_ptr<Double_Knob> visiblePortion; // [0,1] by default
    
#ifndef ROTO_STROKE_USE_FIT_CURVE
    Curve xCurve,yCurve,pressureCurve;
#endif
    
    RotoStrokeItemPrivate(Natron::RotoStrokeType type)
    : type(type)
    , brushSize(new Double_Knob(NULL, kRotoBrushSizeParamLabel, 1, false))
    , brushSpacing(new Double_Knob(NULL, kRotoBrushSpacingParamLabel, 1, false))
    , brushHardness(new Double_Knob(NULL, kRotoBrushHardnessParamLabel, 1, false))
    , effectStrength(new Double_Knob(NULL, kRotoBrushEffectParamLabel, 1, false))
    , visiblePortion(new Double_Knob(NULL, kRotoBrushVisiblePortionParamLabel, 2, false))
    {
                
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
    }
};

struct RotoContextPrivate
{
    mutable QMutex rotoContextMutex;
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
    
    boost::weak_ptr<Page_Knob> strokePage;
    boost::weak_ptr<Double_Knob> brushSizeKnob;
    boost::weak_ptr<Double_Knob> brushSpacingKnob;
    boost::weak_ptr<Double_Knob> brushHardnessKnob;
    boost::weak_ptr<Double_Knob> brushEffectKnob;
    boost::weak_ptr<Double_Knob> brushVisiblePortionKnob;
    

    std::list<boost::weak_ptr<KnobI> > knobs; //< list for easy access to all knobs


    ///This keeps track  of the items linked to the context knobs
    std::list<boost::shared_ptr<RotoItem> > selectedItems;
    boost::shared_ptr<RotoItem> lastInsertedItem;
    boost::shared_ptr<RotoItem> lastLockedItem;
    QMutex lastRenderArgsMutex; //< protects lastRenderArgs & lastRenderedImage
    U64 lastRenderHash;
    boost::shared_ptr<Natron::Image> lastRenderedImage;

    RotoContextPrivate(const boost::shared_ptr<Natron::Node>& n )
    : rotoContextMutex()
    , layers()
    , autoKeying(true)
    , rippleEdit(false)
    , featherLink(true)
    , node(n)
    , age(0)
    , lastRenderHash(0)
    {
        assert( n && n->getLiveInstance() );
        Natron::EffectInstance* effect = n->getLiveInstance();
    
    
        boost::shared_ptr<Page_Knob> shapePage = Natron::createKnob<Page_Knob>(effect, "Controls", 1, false);

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
        
        boost::shared_ptr<Page_Knob> strokePageKnob = Natron::createKnob<Page_Knob>(effect, "Stroke", 1, false);
        strokePage = strokePageKnob;
        
        boost::shared_ptr<Double_Knob> brushSize = Natron::createKnob<Double_Knob>(effect, kRotoBrushSizeParamLabel, 1, false);
        brushSize->setName(kRotoBrushSizeParam);
        brushSize->setHintToolTip(kRotoBrushSizeParamHint);
        brushSize->setDefaultValue(25);
        brushSize->setMinimum(1.);
        brushSize->setMaximum(1000);
        brushSize->setAllDimensionsEnabled(false);
        brushSize->setIsPersistant(false);
        strokePageKnob->addKnob(brushSize);
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
        strokePageKnob->addKnob(brushSpacing);
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
        strokePageKnob->addKnob(brushHardness);
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
        strokePageKnob->addKnob(effectStrength);
        knobs.push_back(effectStrength);
        brushEffectKnob = effectStrength;

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
        strokePageKnob->addKnob(visiblePortion);
        visiblePortion->setDimensionName(0, "start");
        visiblePortion->setDimensionName(0, "end");
        knobs.push_back(visiblePortion);
        brushVisiblePortionKnob = visiblePortion;
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

    void renderInternal(cairo_t* cr,cairo_surface_t* cairoImg,const std::list< boost::shared_ptr<RotoDrawableItem> > & splines,
                        unsigned int mipmapLevel,int time);
    
    void renderStroke(cairo_t* cr,const RotoStrokeItem* stroke, int time, unsigned int mipmapLevel);
    
    void renderBezier(cairo_t* cr,const Bezier* bezier,int time, unsigned int mipmapLevel);
    
    void renderFeather(const Bezier* bezier,int time, unsigned int mipmapLevel, bool inverted, double shapeColor[3], double opacity, double featherDist, double fallOff, cairo_pattern_t* mesh);

    void renderInternalShape(int time,unsigned int mipmapLevel,double shapeColor[3], double opacity,cairo_t* cr, cairo_pattern_t* mesh, const BezierCPs & cps);
    
    static void bezulate(int time,const BezierCPs& cps,std::list<BezierCPs>* patches);

    void applyAndDestroyMask(cairo_t* cr,cairo_pattern_t* mesh);
};


#endif // ROTOCONTEXTPRIVATE_H
