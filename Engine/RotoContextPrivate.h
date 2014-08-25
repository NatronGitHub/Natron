//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef ROTOCONTEXTPRIVATE_H
#define ROTOCONTEXTPRIVATE_H

#include <list>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

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

#include "Global/GlobalDefines.h"

#define ROTO_DEFAULT_OPACITY 1.
#define ROTO_DEFAULT_FEATHER 1.5
#define ROTO_DEFAULT_FEATHERFALLOFF 1.
#define ROTO_DEFAULT_COLOR_R 1.
#define ROTO_DEFAULT_COLOR_G 1.
#define ROTO_DEFAULT_COLOR_B 1.

#define kRotoNameHint "Name of the layer or curve"

#define kRotoOpacityParamName "Opacity"
#define kRotoOpacityHint \
"Controls the opacity of the selected shape(s)."

#define kRotoFeatherParamName "Feather"
#define kRotoFeatherHint \
"Controls the distance of feather (in pixels) to add around the selected shape(s)"

#define kRotoFeatherFallOffParamName "Feather fall-off"
#define kRotoFeatherFallOffHint \
"Controls the rate at which the feather is applied on the selected shape(s)."

#define kRotoActivatedParamName "Activated"
#define kRotoActivatedHint \
"Controls whether the selected shape(s) should be visible and rendered or not." \
"Note that you can animate this parameter so you can activate/deactive the shape " \
"throughout the time."

#define kRotoLockedHint \
"Control whether the layer/curve is editable or locked."

#ifdef NATRON_ROTO_INVERTIBLE
#define kRotoInvertedParamName "Inverted"
#define kRotoInvertedHint \
"Controls whether the selected shape(s) should be inverted. When inverted everything " \
"outside the shape will be set to 1 and everything inside the shape will be set to 0."
#endif

#define kRotoOverlayHint "Color of the display overlay for this curve. Doesn't affect output."

#define kRotoColorParamName "Color"
#define kRotoColorHint \
"The color of the shape. This parameter is used when the output components are set to RGBA."

#define kRotoCompOperatorParamName "Operator"
#define kRotoCompOperatorHint \
"The compositing operator controls how this shape is merged with the shapes that have already been rendered.\n" \
"The roto mask is initialised as black and transparent, then each shape is drawn in the selected order, with the selected color and operator.\n" \
"Finally, the mask is composed with the source image, if connected, using the 'over' operator.\n" \
"See http://cairographics.org/operators/ for a full description of available operators."

class Bezier;

struct BezierCPPrivate
{
    Bezier* holder;

    ///the animation curves for the position in the 2D plane
    Curve curveX,curveY;
    double x,y; //< used when there is no keyframe
    
    ///the animation curves for the derivatives
    ///They do not need to be protected as Curve is a thread-safe class.
    Curve curveLeftBezierX,curveRightBezierX,curveLeftBezierY,curveRightBezierY;
    
    mutable QMutex staticPositionMutex; //< protects the  leftX,rightX,leftY,rightY
    double leftX,rightX,leftY,rightY; //< used when there is no keyframe
    
    mutable QReadWriteLock masterMutex; //< protects masterTrack & relativePoint
    boost::shared_ptr<Double_Knob> masterTrack; //< is this point linked to a track ?
    SequenceTime offsetTime; //< the time at which the offset must be computed
    
    BezierCPPrivate(Bezier* curve)
    : holder(curve)
    , curveX()
    , curveY()
    , x(0)
    , y(0)
    , curveLeftBezierX()
    , curveRightBezierX()
    , curveLeftBezierY()
    , curveRightBezierY()
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

#pragma message WARN("Roto: use these new fields, update them where you need them (getBoundingBox, render)")
    BezierCPs pointsAtDistance; //< same as points, but empty beziers at cusp points with angle <180 are added
    BezierCPs featherPointsAtDistance; //< the precomputed feather points at featherDistance. may
    double featherPointsAtDistanceVal; //< the distance value used to compute featherPointsAtDistance. if == 0., use featherPoints. if Bezier::getFeatherDistance() returns a different value, featherPointsAtDistance must be updated.

    bool finished; //< when finished is true, the last point of the list is connected to the first point of the list.
    
    BezierPrivate()
    : points()
    , featherPoints()
    , pointsAtDistance()
    , featherPointsAtDistance()
    , featherPointsAtDistanceVal(0.)
    , finished(false)
    {
    }
    
    bool hasKeyframeAtTime(int time) const
    {
        // PRIVATE - should not lock
        
        if (points.empty()) {
            return false;
        } else {
            KeyFrame k;
            return points.front()->hasKeyFrameAtTime(time);
        }
    }
    
    void getKeyframeTimes(std::set<int>* times) const
    {
        // PRIVATE - should not lock
       
        if (points.empty()) {
            return ;
        }
        points.front()->getKeyframeTimes(times);
    }
    
    BezierCPs::const_iterator atIndex(int index) const
    {
        // PRIVATE - should not lock
        
        if (index >= (int)points.size() || index < 0) {
            throw std::out_of_range("RotoSpline::atIndex: non-existent control point");
        }
        
        BezierCPs::const_iterator it = points.begin();
        std::advance(it, index);
        return it;
    }
    
    BezierCPs::iterator atIndex(int index)
    {
        // PRIVATE - should not lock
        
        if (index >= (int)points.size() || index < 0) {
            throw std::out_of_range("RotoSpline::atIndex: non-existent control point");
        }
        
        BezierCPs::iterator it = points.begin();
        std::advance(it, index);
        return it;
    }
    
    BezierCPs::const_iterator findControlPointNearby(double x,double y,double acceptance,int time,int* index) const
    {
         // PRIVATE - should not lock
        int i = 0;
        for (BezierCPs::const_iterator it = points.begin(); it != points.end(); ++it,++i) {
            double pX,pY;
            (*it)->getPositionAtTime(time, &pX, &pY);
            if (pX >= (x - acceptance) && pX <= (x + acceptance) && pY >= (y - acceptance) && pY <= (y + acceptance)) {
                *index = i;
                return it;
            }
        }
        return points.end();
    }
    
    BezierCPs::const_iterator findFeatherPointNearby(double x,double y,double acceptance,int time,int* index) const
    {
         // PRIVATE - should not lock
        int i = 0;
        for (BezierCPs::const_iterator it = featherPoints.begin(); it != featherPoints.end(); ++it,++i) {
            double pX,pY;
            (*it)->getPositionAtTime(time, &pX, &pY);
            if (pX >= (x - acceptance) && pX <= (x + acceptance) && pY >= (y - acceptance) && pY <= (y + acceptance)) {
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
    RotoContext* context;
    std::string name;
    RotoLayer* parentLayer;
    
    ////This controls whether the item (and all its children if it is a layer)
    ////should be visible/rendered or not at any time.
    ////This is different from the "activated" knob for RotoDrawableItem's which in that
    ////case allows to define a life-time
    bool globallyActivated;
    
    ////A locked item should not be modifiable by the GUI
    bool locked;
    
    RotoItemPrivate(RotoContext* context,const std::string& n,RotoLayer* parent)
    : context(context)
    , name(n)
    , parentLayer(parent)
    , globallyActivated(true)
    , locked(false)
    {}
    
    

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
inline void getCompositingOperators(std::vector<std::string>* operators,std::vector<std::string>* toolTips)
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
    
}

struct RotoDrawableItemPrivate
{
    double overlayColor[4]; //< the color the shape overlay should be drawn with, defaults to smooth red
    
    
    boost::shared_ptr<Double_Knob> opacity; //< opacity of the rendered shape between 0 and 1
    boost::shared_ptr<Double_Knob> feather;//< number of pixels to add to the feather distance (from the feather point), between -100 and 100
    boost::shared_ptr<Double_Knob> featherFallOff; //< the rate of fall-off for the feather, between 0 and 1,  0.5 meaning the
                                                   //alpha value is half the original value when at half distance from the feather distance
    boost::shared_ptr<Bool_Knob> activated; //< should the curve be visible/rendered ? (animable)
#ifdef NATRON_ROTO_INVERTIBLE
    boost::shared_ptr<Bool_Knob> inverted; //< invert the rendering
#endif
    boost::shared_ptr<Color_Knob> color;
    boost::shared_ptr<Choice_Knob> compOperator;
    
    RotoDrawableItemPrivate()
    : opacity(new Double_Knob(NULL, kRotoOpacityParamName, 1, false))
    , feather(new Double_Knob(NULL, kRotoFeatherParamName, 1, false))
    , featherFallOff(new Double_Knob(NULL, kRotoFeatherFallOffParamName, 1, false))
    , activated(new Bool_Knob(NULL, kRotoActivatedParamName, 1, false))
#ifdef NATRON_ROTO_INVERTIBLE
    , inverted(new Bool_Knob(NULL, kRotoInvertedParamName, 1, false))
#endif
    , color(new Color_Knob(NULL, kRotoColorParamName, 3, false))
    , compOperator(new Choice_Knob(NULL, kRotoCompOperatorParamName, 1, false))
    {
        opacity->setHintToolTip(kRotoOpacityHint);
        opacity->populate();
        opacity->setDefaultValue(ROTO_DEFAULT_OPACITY);
        {
            boost::shared_ptr<KnobSignalSlotHandler> handler(new KnobSignalSlotHandler(opacity));
            opacity->setSignalSlotHandler(handler);
        }

        feather->setHintToolTip(kRotoFeatherHint);
        feather->populate();
        feather->setDefaultValue(ROTO_DEFAULT_FEATHER);
        {
            boost::shared_ptr<KnobSignalSlotHandler> handler(new KnobSignalSlotHandler(feather));
            feather->setSignalSlotHandler(handler);
        }

        featherFallOff->setHintToolTip(kRotoFeatherFallOffHint);
        featherFallOff->populate();
        featherFallOff->setDefaultValue(ROTO_DEFAULT_FEATHERFALLOFF);
        {
            boost::shared_ptr<KnobSignalSlotHandler> handler(new KnobSignalSlotHandler(featherFallOff));
            featherFallOff->setSignalSlotHandler(handler);
        }
        
        activated->setHintToolTip(kRotoActivatedHint);
        activated->populate();
        activated->setDefaultValue(true);
        {
            boost::shared_ptr<KnobSignalSlotHandler> handler(new KnobSignalSlotHandler(activated));
            activated->setSignalSlotHandler(handler);
        }

#ifdef NATRON_ROTO_INVERTIBLE
        inverted->setHintToolTip(kRotoInvertedHint);
        inverted->populate();
        inverted->setDefaultValue(false);
        {
            boost::shared_ptr<KnobSignalSlotHandler> handler(new KnobSignalSlotHandler(inverted));
            inverted->setSignalSlotHandler(handler);
        }
#endif

        color->setHintToolTip(kRotoColorHint);
        color->populate();
        color->setDefaultValue(ROTO_DEFAULT_COLOR_R, 0);
        color->setDefaultValue(ROTO_DEFAULT_COLOR_G, 1);
        color->setDefaultValue(ROTO_DEFAULT_COLOR_B, 2);
        {
            boost::shared_ptr<KnobSignalSlotHandler> handler(new KnobSignalSlotHandler(color));
            color->setSignalSlotHandler(handler);
        }
      
        compOperator->setHintToolTip(kRotoCompOperatorHint);
        compOperator->populate();
        std::vector<std::string> operators;
        std::vector<std::string> tooltips;
        getCompositingOperators(&operators, &tooltips);
        compOperator->populateChoices(operators,tooltips);
        compOperator->setDefaultValue((int)CAIRO_OPERATOR_OVER);
        {
            boost::shared_ptr<KnobSignalSlotHandler> handler(new KnobSignalSlotHandler(compOperator));
            compOperator->setSignalSlotHandler(handler);
        }
        
        overlayColor[0] = 0.85164;
        overlayColor[1] = 0.196936;
        overlayColor[2] = 0.196936;
        overlayColor[3] = 1.;
    }

    ~RotoDrawableItemPrivate() {}
};

struct RotoContextPrivate
{
    
    mutable QMutex rotoContextMutex;
    std::list< boost::shared_ptr<RotoLayer> > layers;
    bool autoKeying;
    bool rippleEdit;
    bool featherLink;
    
    Natron::Node* node;
    U64 age;
    
    ///These are knobs that take the value of the selected splines infos.
    ///Their value changes when selection changes.
    boost::shared_ptr<Double_Knob> opacity;
    boost::shared_ptr<Double_Knob> feather;
    boost::shared_ptr<Double_Knob> featherFallOff;
    boost::shared_ptr<Bool_Knob> activated; //<allows to disable a shape on a specific frame range
#ifdef NATRON_ROTO_INVERTIBLE
    boost::shared_ptr<Bool_Knob> inverted;
#endif
    boost::shared_ptr<Color_Knob> colorKnob;
    
    ////For each base item ("Rectangle","Ellipse","Bezier", etc...) a basic countr
    ////to give a unique default name to each shape
    std::map<std::string, int> itemCounters;
    
    
    ///This keeps track  of the items linked to the context knobs
    std::list<boost::shared_ptr<RotoItem> > selectedItems;
    
    boost::shared_ptr<RotoItem> lastInsertedItem;
    boost::shared_ptr<RotoItem> lastLockedItem;
    
    
    QMutex lastRenderArgsMutex; //< protects lastRenderArgs & lastRenderedImage
    U64 lastRenderHash;
    boost::shared_ptr<Natron::Image> lastRenderedImage;
    
    RotoContextPrivate(Natron::Node* n )
    : rotoContextMutex()
    , layers()
    , autoKeying(true)
    , rippleEdit(false)
    , featherLink(true)
    , node(n)
    , age(0)
    {
        
        assert(n && n->getLiveInstance());
        Natron::EffectInstance* effect = n->getLiveInstance();
        opacity = Natron::createKnob<Double_Knob>(effect, kRotoOpacityParamName, 1, false);
        opacity->setHintToolTip(kRotoOpacityHint);
        opacity->setMinimum(0.);
        opacity->setMaximum(1.);
        opacity->setDisplayMinimum(0.);
        opacity->setDisplayMaximum(1.);
        opacity->setDefaultValue(ROTO_DEFAULT_OPACITY);
        opacity->setAllDimensionsEnabled(false);
        opacity->setIsPersistant(false);
        feather = Natron::createKnob<Double_Knob>(effect, kRotoFeatherParamName, 1, false);
        feather->setHintToolTip(kRotoFeatherHint);
        feather->setMinimum(-100);
        feather->setMaximum(100);
        feather->setDisplayMinimum(-100);
        feather->setDisplayMaximum(100);
        feather->setDefaultValue(ROTO_DEFAULT_FEATHER);
        feather->setAllDimensionsEnabled(false);
        feather->setIsPersistant(false);
        featherFallOff = Natron::createKnob<Double_Knob>(effect, kRotoFeatherFallOffParamName, 1, false);
        featherFallOff->setHintToolTip(kRotoFeatherFallOffHint);
        featherFallOff->setMinimum(0.001);
        featherFallOff->setMaximum(5.);
        featherFallOff->setDisplayMinimum(0.2);
        featherFallOff->setDisplayMaximum(5.);
        featherFallOff->setDefaultValue(ROTO_DEFAULT_FEATHERFALLOFF);
        featherFallOff->setAllDimensionsEnabled(false);
        featherFallOff->setIsPersistant(false);
        activated = Natron::createKnob<Bool_Knob>(effect, kRotoActivatedParamName, 1, false);
        activated->setHintToolTip(kRotoActivatedHint);
        activated->turnOffNewLine();
        activated->setDefaultValue(true);
        activated->setAllDimensionsEnabled(false);
        activated->setIsPersistant(false);
#ifdef NATRON_ROTO_INVERTIBLE
        inverted = Natron::createKnob<Bool_Knob>(effect, kRotoInvertedParamName, 1, false);
        inverted->setHintToolTip(kRotoInvertedHint);
        inverted->setDefaultValue(false);
        inverted->setAllDimensionsEnabled(false);
        inverted->setIsPersistant(false);
#endif

        colorKnob = Natron::createKnob<Color_Knob>(effect, kRotoColorParamName, 3, false);
        colorKnob->setHintToolTip(kRotoColorHint);
        colorKnob->setDefaultValue(ROTO_DEFAULT_COLOR_R, 0);
        colorKnob->setDefaultValue(ROTO_DEFAULT_COLOR_G, 1);
        colorKnob->setDefaultValue(ROTO_DEFAULT_COLOR_B, 2);
        colorKnob->setAllDimensionsEnabled(false);
        colorKnob->setIsPersistant(false);

    }
    
    /**
     * @brief Call this after any change to notify the mask has changed for the cache.
     **/
    void incrementRotoAge()
    {
        ///MT-safe: only called on the main-thread
        assert(QThread::currentThread() == qApp->thread());
        
        QMutexLocker l(&rotoContextMutex);
        ++age;
    }
    
   
    void renderInternal(cairo_t* cr,cairo_surface_t* cairoImg,const std::list< boost::shared_ptr<Bezier> >& splines,
                        unsigned int mipmapLevel,int time);
    
    void renderInternalShape(int time,unsigned int mipmapLevel,cairo_t* cr,const BezierCPs& cps);
    
    void applyAndDestroyMask(cairo_t* cr,cairo_pattern_t* mesh);
};


#endif // ROTOCONTEXTPRIVATE_H
