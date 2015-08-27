/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#ifndef _Engine_RotoContext_h_
#define _Engine_RotoContext_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <set>
#include <string>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"

CLANG_DIAG_OFF(deprecated-declarations)
#include <QObject>
#include <QMutex>
#include <QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#define kRotoLayerBaseName "Layer"
#define kRotoBezierBaseName "Bezier"
#define kRotoOpenBezierBaseName "Pencil"
#define kRotoBSplineBaseName "BSpline"
#define kRotoEllipseBaseName "Ellipse"
#define kRotoRectangleBaseName "Rectangle"
#define kRotoPaintBrushBaseName "Brush"
#define kRotoPaintEraserBaseName "Eraser"
#define kRotoPaintBlurBaseName "Blur"
#define kRotoPaintSmearBaseName "Smear"
#define kRotoPaintSharpenBaseName "Sharpen"
#define kRotoPaintCloneBaseName "Clone"
#define kRotoPaintRevealBaseName "Reveal"
#define kRotoPaintDodgeBaseName "Dodge"
#define kRotoPaintBurnBaseName "Burn"

namespace Natron {
class Image;
class ImageComponents;
class Node;
}
namespace boost { namespace serialization { class access; } }

class RectI;
class RectD;
class KnobI;
class Bool_Knob;
class Double_Knob;
class Int_Knob;
class Choice_Knob;
class Color_Knob;
typedef struct _cairo_pattern cairo_pattern_t;

class Curve;
class Bezier;
class RotoItemSerialization;
class BezierSerialization;

/**
 * @class A Bezier is an animated control point of a Bezier. It is the starting point
 * and/or the ending point of a bezier segment. (It would correspond to P0/P3).
 * The left bezier point/right bezier point we refer to in the functions below
 * are respectively the P2 and P1 point.
 *
 * Note on multi-thread:
 * All getters or const functions can be called in any thread, that is:
 * - The GUI thread (main-thread)
 * - The render thread 
 * - The serialization thread (when saving)
 *
 * Setters or non-const functions can exclusively be called in the main-thread (Gui thread) to ensure there is no
 * race condition whatsoever.

 * More-over the setters must be called ONLY by the Bezier class which is the class handling the thread safety.
 * That's why non-const functions are private.
 **/
struct BezierCPPrivate;
class BezierCP
{
    ///This is the unique class allowed to call the setters.
    friend class Bezier;
    friend class boost::serialization::access;

public:

    ///Constructore used by the serialization
    BezierCP();

    BezierCP(const BezierCP & other);

    BezierCP(const boost::shared_ptr<Bezier>& curve);

    virtual ~BezierCP();
    
    boost::shared_ptr<Curve> getXCurve() const;
    boost::shared_ptr<Curve> getYCurve() const;
    boost::shared_ptr<Curve> getLeftXCurve() const;
    boost::shared_ptr<Curve> getLeftYCurve() const;
    boost::shared_ptr<Curve> getRightXCurve() const;
    boost::shared_ptr<Curve> getRightYCurve() const;

    void clone(const BezierCP & other);

    void setPositionAtTime(bool useGuiCurves,double time,double x,double y);

    void setLeftBezierPointAtTime(bool useGuiCurves,double time,double x,double y);

    void setRightBezierPointAtTime(bool useGuiCurves,double time,double x,double y);

    void setStaticPosition(bool useGuiCurves,double x,double y);

    void setLeftBezierStaticPosition(bool useGuiCurves,double x,double y);

    void setRightBezierStaticPosition(bool useGuiCurves,double x,double y);

    void removeKeyframe(bool useGuiCurves,double time);
    
    void removeAnimation(bool useGuiCurves,int currentTime);

    ///returns true if a keyframe was set
    bool cuspPoint(bool useGuiCurves,double time,bool autoKeying,bool rippleEdit,const std::pair<double,double>& pixelScale);

    ///returns true if a keyframe was set
    bool smoothPoint(bool useGuiCurves,double time,bool autoKeying,bool rippleEdit,const std::pair<double,double>& pixelScale);


    virtual bool isFeatherPoint() const
    {
        return false;
    }

    bool equalsAtTime(bool useGuiCurves,double time,const BezierCP & other) const;

    bool getPositionAtTime(bool useGuiCurves,double time,double* x,double* y,bool skipMasterOrRelative = false) const;

    bool getLeftBezierPointAtTime(bool useGuiCurves,double time,double* x,double* y,bool skipMasterOrRelative = false) const;

    bool getRightBezierPointAtTime(bool useGuiCurves,double time,double *x,double *y,bool skipMasterOrRelative = false) const;

    bool hasKeyFrameAtTime(bool useGuiCurves,double time) const;

    void getKeyframeTimes(bool useGuiCurves,std::set<int>* times) const;
    
    void getKeyFrames(bool useGuiCurves,std::list<std::pair<int,Natron::KeyframeTypeEnum> >* keys) const;
    
    int getKeyFrameIndex(bool useGuiCurves,double time) const;
    
    void setKeyFrameInterpolation(bool useGuiCurves,Natron::KeyframeTypeEnum interp,int index);

    int getKeyframeTime(bool useGuiCurves,int index) const;

    int getKeyframesCount(bool useGuiCurves) const;

    int getControlPointsCount() const;

    /**
     * @brief Pointer to the bezier holding this control point. This is not protected by a mutex
     * since it never changes.
     **/
    boost::shared_ptr<Bezier> getBezier() const;

    /**
     * @brief Returns whether a tangent handle is nearby the given coordinates.
     * That is, this function a number indicating
     * if the given coordinates are close to the left control point (P2, ret = 0) or the right control point(P1, ret = 1).
     * If it is not close to any tangent, -1 is returned.
     * This function can also return the tangent of a feather point, to find out if the point is a feather point call
     * isFeatherPoint() on the returned control point.
     **/
    int isNearbyTangent(bool useGuiCurves,double time,double x,double y,double acceptance) const;

    /**
     * The functions below are to slave/unslave a control point to a track
     **/
    void slaveTo(SequenceTime offsetTime,const boost::shared_ptr<Double_Knob> & track);
    boost::shared_ptr<Double_Knob> isSlaved() const;
    void unslave();

    SequenceTime getOffsetTime() const;
    
    void cloneInternalCurvesToGuiCurves();
    
    void cloneGuiCurvesToInternalCurves();

private:

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const;

    template<class Archive>
    void load(Archive & ar, const unsigned int version);
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    boost::scoped_ptr<BezierCPPrivate> _imp;
};

class FeatherPoint
    : public BezierCP
{
public:
    
    FeatherPoint(const BezierCP& other)
    : BezierCP(other)
    {
    }


    FeatherPoint(const boost::shared_ptr<Bezier>& curve)
        : BezierCP(curve)
    {
    }

    virtual bool isFeatherPoint() const
    {
        return true;
    }
};


/**
 * @class A base class for all items made by the roto context
 **/
class RotoContext;
class RotoLayer;

namespace Transform {
struct Matrix3x3;
}

struct RotoItemPrivate;
class RotoItem
: public QObject, public boost::enable_shared_from_this<RotoItem>
{
public:
    
    enum SelectionReasonEnum
    {
        eSelectionReasonOverlayInteract = 0, ///when the user presses an interact
        eSelectionReasonSettingsPanel, ///when the user interacts with the settings panel
        eSelectionReasonOther ///when the project loader restores the selection
    };


    RotoItem(const boost::shared_ptr<RotoContext>& context,
             const std::string & name,
             boost::shared_ptr<RotoLayer> parent = boost::shared_ptr<RotoLayer>());

    virtual ~RotoItem();

    virtual void clone(const RotoItem*  other);

    ///only callable on the main-thread
    bool setScriptName(const std::string & name);

    std::string getScriptName() const;
    
    std::string getFullyQualifiedName() const;
    
    std::string getLabel() const;
    
    void setLabel(const std::string& label);

    ///only callable on the main-thread
    void setParentLayer(boost::shared_ptr<RotoLayer> layer);

    ///MT-safe
    boost::shared_ptr<RotoLayer> getParentLayer() const;

    ///only callable from the main-thread
    void setGloballyActivated(bool a, bool setChildren);

    ///MT-safe
    bool isGloballyActivated() const;

    bool isDeactivatedRecursive() const;

    void setLocked(bool l,bool lockChildren,RotoItem::SelectionReasonEnum reason);
    bool getLocked() const;

    bool isLockedRecursive() const;

    /**
     * @brief Returns at which hierarchy level the item is.
     * The base layer is 0.
     * All items into that base layer are on level 1.
     * etc...
     **/
    int getHierarchyLevel() const;

    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void save(RotoItemSerialization* obj) const;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void load(const RotoItemSerialization & obj);

    /**
     * @brief Returns the name of the node holding this item
     **/
    std::string getRotoNodeName() const;

    boost::shared_ptr<RotoContext> getContext() const;
    
    boost::shared_ptr<RotoItem> getPreviousItemInLayer() const;

protected:


    ///This mutex protects every-member this class and the derived class might have.
    ///That is for the RotoItem class:
    ///  - name
    ///  - globallyActivated
    ///  - locked
    ///  - parentLayer
    ///
    ///For the RotoDrawableItem:
    ///  - overlayColor
    ///
    ///For the RotoLayer class:
    ///  - items
    ///
    ///For the Bezier class:
    ///  - points
    ///  - featherPoints
    ///  - finished
    ///  - pointsAtDistance
    ///  - featherPointsAtDistance
    ///  - featherPointsAtDistanceVal
    mutable QMutex itemMutex;

private:

    void setGloballyActivated_recursive(bool a);
    void setLocked_recursive(bool locked,RotoItem::SelectionReasonEnum reason);

    boost::scoped_ptr<RotoItemPrivate> _imp;
};

Q_DECLARE_METATYPE(RotoItem*);


/**
 * @brief Base class for all drawable items
 **/
struct RotoDrawableItemPrivate;
class RotoDrawableItem
    : public RotoItem
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    RotoDrawableItem(const boost::shared_ptr<RotoContext>& context,
                     const std::string & name,
                     const boost::shared_ptr<RotoLayer>& parent,
                     bool isStroke);

    virtual ~RotoDrawableItem();
    
    void createNodes(bool connectNodes = true);
    
    void incrementNodesAge();
    
    void refreshNodesConnections();

    virtual void clone(const RotoItem*  other) OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void save(RotoItemSerialization* obj) const OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void load(const RotoItemSerialization & obj) OVERRIDE;

    /**
     * @brief When deactivated the spline will not be taken into account when rendering, neither will it be visible on the viewer.
     * If isGloballyActivated() returns false, this function will return false aswell.
     **/
    bool isActivated(double time) const;
    void setActivated(bool a, double time);

    /**
     * @brief The opacity of the curve
     **/
    double getOpacity(double time) const;
    void setOpacity(double o,double time);

    /**
     * @brief The distance of the feather is the distance from the control point to the feather point plus
     * the feather distance returned by this function.
     **/
    double getFeatherDistance(double time) const;
    void setFeatherDistance(double d,double time);
    int getNumKeyframesFeatherDistance() const;

    /**
     * @brief The fall-off rate: 0.5 means half color is faded at half distance.
     **/
    double getFeatherFallOff(double time) const;
    void setFeatherFallOff(double f,double time);

    /**
     * @brief The color that the GUI should use to draw the overlay of the shape
     **/
    void getOverlayColor(double* color) const;
    void setOverlayColor(const double* color);

    bool getInverted(double time) const;

    void getColor(double time,double* color) const;
    void setColor(double time,double r,double g,double b);
    
    int getCompositingOperator() const;
    
    void setCompositingOperator(int op);

    std::string getCompositingOperatorToolTip() const;
    boost::shared_ptr<Bool_Knob> getActivatedKnob() const;
    boost::shared_ptr<Double_Knob> getFeatherKnob() const;
    boost::shared_ptr<Double_Knob> getFeatherFallOffKnob() const;
    boost::shared_ptr<Double_Knob> getOpacityKnob() const;
#ifdef NATRON_ROTO_INVERTIBLE
    boost::shared_ptr<Bool_Knob> getInvertedKnob() const;
#endif
    boost::shared_ptr<Choice_Knob> getOperatorKnob() const;
    boost::shared_ptr<Color_Knob> getColorKnob() const;
    boost::shared_ptr<Double_Knob> getCenterKnob() const;
    boost::shared_ptr<Int_Knob> getLifeTimeFrameKnob() const;
    boost::shared_ptr<Double_Knob> getBrushSizeKnob() const;
    boost::shared_ptr<Double_Knob> getBrushHardnessKnob() const;
    boost::shared_ptr<Double_Knob> getBrushSpacingKnob() const;
    boost::shared_ptr<Double_Knob> getBrushEffectKnob() const;
    boost::shared_ptr<Double_Knob> getBrushVisiblePortionKnob() const;
    boost::shared_ptr<Bool_Knob> getPressureOpacityKnob() const;
    boost::shared_ptr<Bool_Knob> getPressureSizeKnob() const;
    boost::shared_ptr<Bool_Knob> getPressureHardnessKnob() const;
    boost::shared_ptr<Bool_Knob> getBuildupKnob() const;
    boost::shared_ptr<Int_Knob> getTimeOffsetKnob() const;
    boost::shared_ptr<Choice_Knob> getTimeOffsetModeKnob() const;
    boost::shared_ptr<Choice_Knob> getBrushSourceTypeKnob() const;
    boost::shared_ptr<Double_Knob> getBrushCloneTranslateKnob() const;
    
    void setKeyframeOnAllTransformParameters(double time);

    const std::list<boost::shared_ptr<KnobI> >& getKnobs() const;
    
    virtual RectD getBoundingBox(double time) const = 0;

    void getTransformAtTime(double time,Transform::Matrix3x3* matrix) const;
    
    /**
     * @brief Set the transform at the given time
     **/
    void setTransform(double time, double tx, double ty, double sx, double sy, double centerX, double centerY, double rot, double skewX, double skewY);
    
    boost::shared_ptr<Natron::Node> getEffectNode() const;
    boost::shared_ptr<Natron::Node> getMergeNode() const;
    boost::shared_ptr<Natron::Node> getTimeOffsetNode() const;
    boost::shared_ptr<Natron::Node> getFrameHoldNode() const;
    
    void resetNodesThreadSafety();
    void deactivateNodes();
    void activateNodes();
    void disconnectNodes();

    
Q_SIGNALS:

#ifdef NATRON_ROTO_INVERTIBLE
    void invertedStateChanged();
#endif

    void overlayColorChanged();

    void shapeColorChanged();

    void compositingOperatorChanged(int,int);

public Q_SLOTS:
    
    void onRotoOutputChannelsChanged();
    void onRotoKnobChanged(int);
    
protected:
    
    void rotoKnobChanged(const boost::shared_ptr<KnobI>& knob);
    
    virtual void onTransformSet(double /*time*/) {}
    
    void addKnob(const boost::shared_ptr<KnobI>& knob);

private:
    
    void resetCloneTransformCenter();
    
    RotoDrawableItem* findPreviousInHierarchy();


    boost::scoped_ptr<RotoDrawableItemPrivate> _imp;
};


/**
 * @class A RotoLayer is a group of RotoItem. This allows the context to sort
 * and build hierarchies of layers.
 **/
struct RotoLayerPrivate;
class RotoLayer
    : public RotoItem
{
public:

    RotoLayer(const boost::shared_ptr<RotoContext>& context,
              const std::string & name,
              const boost::shared_ptr<RotoLayer>& parent);

    explicit RotoLayer(const RotoLayer & other);

    virtual ~RotoLayer();

    virtual void clone(const RotoItem* other) OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void save(RotoItemSerialization* obj) const OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void load(const RotoItemSerialization & obj) OVERRIDE;

    ///only callable on the main-thread
    ///No check is done to figure out if the item already exists in this layer
    ///this is up to the caller responsability
    void addItem(const boost::shared_ptr<RotoItem>& item,bool declareToPython = true);

    ///Inserts the item into the layer before the indicated index.
    ///The same restrictions as addItem are applied.
    void insertItem(const boost::shared_ptr<RotoItem>& item,int index);

    ///only callable on the main-thread
    void removeItem(const boost::shared_ptr<RotoItem>& item);

    ///Returns the index of the given item in the layer, or -1 if not found
    int getChildIndex(const boost::shared_ptr<RotoItem>& item) const;

    ///only callable on the main-thread
    const std::list< boost::shared_ptr<RotoItem> >& getItems() const;

    ///MT-safe
    std::list< boost::shared_ptr<RotoItem> > getItems_mt_safe() const;

    
private:

    boost::scoped_ptr<RotoLayerPrivate> _imp;
};

/**
 * @class This class represents a bezier curve.
 * Note that the bezier also supports feather points.
 * This class is MT-safe
 *
 * The curve supports animation, and by default once the first control point is added the curve
 * has at least a minimum of 1 keyframe.
 **/


struct BezierPrivate;
class Bezier
    : public RotoDrawableItem
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    Bezier(const boost::shared_ptr<RotoContext>& context,
           const std::string & name,
           const boost::shared_ptr<RotoLayer>& parent,
           bool isOpenBezier);

    Bezier(const Bezier & other,
           const boost::shared_ptr<RotoLayer>& parent);

    virtual ~Bezier();

    
    static void
    bezierPoint(const Natron::Point & p0,
                const Natron::Point & p1,
                const Natron::Point & p2,
                const Natron::Point & p3,
                double t,
                Natron::Point *dest);

    
    bool isOpenBezier() const;
    
    /**
     * @brief Used to differentiate real shapes with feather of paint strokes which does not have a feather
     **/
    virtual bool useFeatherPoints() const { return true; }
    
    
    
    virtual void clone(const RotoItem* other) OVERRIDE;

    
    void clearAllPoints();
    
    /**
     * @brief Adds a new control point to the curve. A feather point will be added, at the same position.
     * If auto keying is enabled and this is the first point and there's no keyframe a new keyframe will be set at the current time.
     *
     * This function can only be called when the curved is not yet "finished". Once setCurveFinished has
     * been called you can no longer call this function either. The only way to make it finished again
     * is to call setCurveFinished. You can then add points normally with addControlPointAfterIndex
     * This function is used to build-up the curve as opposed to addControlPointAfterIndex which is there to
     * edit an already fully shaped spline.
     **/
    boost::shared_ptr<BezierCP> addControlPoint(double x,double y,double time);

    /**
     * @brief Adds a new control point to the curve after the control point at the given index.
     * A feather point will be added, at the same position.
     * If auto keying is enabled, and there's no keyframe a new keyframe will be set at the current time.
     *
     * If index is -1 then the point will be added as the first point of the curve.
     * If index is invalid an invalid argument exception will be thrown.
     **/
    boost::shared_ptr<BezierCP> addControlPointAfterIndex(int index,double t);

    /**
     * @brief Returns the number of control points of the bezier
     **/
    int getControlPointsCount() const;
    
    /**
     * @brief Given the (x,y) coordinates of a point, this function returns whether a point lies on
     * the cubic bezier curve of the feather points or of the control points or not.
     * @returns The index of the starting control point (P0) of the bezier segment on which the given
     * point lies. If the point doesn't belong to any bezier segment of this curve then it will return -1.
     **/
    int isPointOnCurve(double x, double y, double acceptance, double *t, bool* feather) const;

    /**
     * @brief Set whether the curve is finished or not. A finished curve will have an arc between the first
     * and the last control point.
     **/
    void setCurveFinished(bool finished);
    bool isCurveFinished() const;
    
    void resetCenterKnob();

    /**
     * @brief Removes the control point at the given index if any. The feather point will also be removed , at the same position.
     * If auto keying is enabled, and there's no keyframe a new keyframe will be set at the current time.
     **/
    void removeControlPointByIndex(int index);

    /**
     * @brief Move the control point at the given index by the given dx and dy.
     * If feather link is enabled, the feather point will also be moved by the same deltas.
     * If  there's no keyframe at the current time, a new keyframe will be added.
     * This function asserts that  auto-keying is  enabled.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void movePointByIndex(int index, double time, double dx, double dy);

    /**
     * @brief Set the control point at the given index to x,y.
     * If feather link is enabled, the feather point will also be moved by the same deltas.
     * If  there's no keyframe at the current time, a new keyframe will be added.
     * This function asserts that  auto-keying is  enabled.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void setPointByIndex(int index, double time, double x, double y);

    /**
     * @brief Moves the feather point at the given index if any by the given dx and dy.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void moveFeatherByIndex(int index, double time, double dx, double dy);
    
private:
    
    void movePointByIndexInternal(bool useGuiCurve,int index, double time, double dx, double dy, bool onlyFeather);
    void setPointByIndexInternal(bool useGuiCurve,int index, double time, double dx, double dy);

public:
    

    /**
     * @brief Moves the left bezier point of the control point at the given index by the given deltas
     * and at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void moveLeftBezierPoint(int index,double time,double dx,double dy);

    /**
     * @brief Moves the right bezier point of the control point at the given index by the given deltas
     * and at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void moveRightBezierPoint(int index,double time,double dx,double dy);
    
private:
    
    void moveBezierPointInternal(BezierCP* cpParam,int index,double time, double lx, double ly, double rx, double ry, bool isLeft, bool moveBoth);
    
public:

    
    /**
     * @brief Transforms the given point at the given time by the given matrix.
     **/
    void transformPoint(const boost::shared_ptr<BezierCP> & point,double time,Transform::Matrix3x3* matrix);

    /**
     * @brief Provided for convenience. It set the left bezier point of the control point at the given index to
     * the position given by (x,y) at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     * This function will also set the point of the feather point at the given x,y.
     **/
    void setLeftBezierPoint(int index,double time,double x,double y);

    /**
     * @brief Provided for convenience. It set the right bezier point of the control point at the given index to
     * the position given by (x,y) at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     * This function will also set the point of the feather point at the given x,y.
     **/
    void setRightBezierPoint(int index,double time,double x,double y);


    /**
     * @brief This function is a combinaison of setPosition + setLeftBezierPoint / setRightBeziePoint
     **/
    void setPointAtIndex(bool feather,int index,double time,double x,double y,double lx,double ly,double rx,double ry);
    
private:
    
    void setPointAtIndexInternal(bool setLeft,bool setRight,bool setPoint,bool feather,bool featherAndCp,int index,double time,double x,double y,double lx,double ly,double rx,double ry);
    
public:
    

    /**
     * @brief Set the left and right bezier point of the control point.
     **/
    void movePointLeftAndRightIndex(BezierCP & p,double time,double lx,double ly,double rx,double ry);


    /**
     * @brief Removes the feather point at the given index by making it equal the "true" control point.
     **/
    void removeFeatherAtIndex(int index);
    
    /**
     * @brief Expand the feather point in the direction of the feather distance by the given distance.
     **/
    //void expandFeatherAtIndex(int index,double distance);

    /**
     * @brief Smooth the curvature of the bezier at the given index by expanding the tangents.
     * This is also applied to the feather points.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void smoothPointAtIndex(int index,double time,const std::pair<double,double>& pixelScale);

    /**
     * @brief Cusps the curvature of the bezier at the given index by reducing the tangents.
     * This is also applied to the feather points.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void cuspPointAtIndex(int index,double time,const std::pair<double,double>& pixelScale);

private:
    
    void smoothOrCuspPointAtIndex(bool isSmooth,int index,double time,const std::pair<double,double>& pixelScale);
    
public:
    
    
    /**
     * @brief Set a new keyframe at the given time. If a keyframe already exists this function does nothing.
     **/
    void setKeyframe(double time);

    /**
     * @brief Removes a keyframe at the given time if any.
     **/
    void removeKeyframe(double time);
        
    /**
     * @brief Removes all animation
     **/
    void removeAnimation();
    
    /**
     * @brief Moves a keyframe
     **/
    void moveKeyframe(int oldTime,int newTime);


    /**
     * @brief Returns the number of keyframes for this spline.
     **/
    int getKeyframesCount() const;

    static void deCastelJau(bool useGuiCurves,
                            const std::list<boost::shared_ptr<BezierCP> >& cps, double time, unsigned int mipMapLevel,
                            bool finished,
                            int nBPointsPerSegment,
                            const Transform::Matrix3x3& transform,
                            std::list<Natron::Point>* points,
                            RectD* bbox);
    
    static void point_line_intersection(const Natron::Point &p1,
                            const Natron::Point &p2,
                            const Natron::Point &pos,
                            int *winding);
    
    /**
     * @brief Evaluates the spline at the given time and returns the list of all the points on the curve.
     * @param nbPointsPerSegment controls how many points are used to draw one Bezier segment
     **/
    void evaluateAtTime_DeCasteljau(bool useGuiCurves,
                                    double time,
                                    unsigned int mipMapLevel,
                                    int nbPointsPerSegment,
                                    std::list<Natron::Point>* points,
                                    RectD* bbox) const;
    
    /**
     * @brief Same as evaluateAtTime_DeCasteljau but nbPointsPerSegment is approximated automatically
     **/
    void evaluateAtTime_DeCasteljau_autoNbPoints(bool useGuiCurves,
                                                 double time,
                                                 unsigned int mipMapLevel,
                                                 std::list<Natron::Point>* points,
                                                 RectD* bbox) const;

    /**
     * @brief Evaluates the bezier formed by the feather points. Segments which are equal to the control points of the bezier
     * will not be drawn.
     **/
    void evaluateFeatherPointsAtTime_DeCasteljau(bool useGuiCurves,
                                                 double time,
                                                 unsigned int mipMapLevel,
                                                 int nbPointsPerSegment,
                                                 bool evaluateIfEqual,
                                                 std::list<Natron::Point >* points,
                                                 RectD* bbox) const;

    /**
     * @brief Returns the bounding box of the bezier. The last value computed by evaluateAtTime_DeCasteljau will be returned,
     * otherwise if it has never been called, evaluateAtTime_DeCasteljau will be called to compute the bounding box.
     **/
    virtual RectD getBoundingBox(double time) const OVERRIDE;
    
    static void
    bezierSegmentListBboxUpdate(bool useGuiCurves,
                                const std::list<boost::shared_ptr<BezierCP> > & points,
                                bool finished,
                                bool isOpenBezier,
                                double time,
                                unsigned int mipMapLevel,
                                const Transform::Matrix3x3& transform,
                                RectD* bbox);

    /**
     * @brief Returns a const ref to the control points of the bezier curve. This can only ever be called on the main thread.
     **/
    const std::list< boost::shared_ptr<BezierCP> > & getControlPoints() const;
protected:
    
    std::list< boost::shared_ptr<BezierCP> > & getControlPoints_internal();
    
public:
    
    
    std::list< boost::shared_ptr<BezierCP> > getControlPoints_mt_safe() const;

    /**
     * @brief Returns a const ref to the feather points of the bezier curve. This can only ever be called on the main thread.
     **/
    const std::list< boost::shared_ptr<BezierCP> > & getFeatherPoints() const;
    std::list< boost::shared_ptr<BezierCP> > getFeatherPoints_mt_safe() const;
    
    enum ControlPointSelectionPrefEnum
    {
        eControlPointSelectionPrefFeatherFirst = 0,
        eControlPointSelectionPrefControlPointFirst,
        eControlPointSelectionPrefWhateverFirst
    };

    /**
     * @brief Returns a pointer to a nearby control point if any. This function  also returns the feather point
     * The first member is the actual point nearby, and the second the counter part (i.e: either the feather point
     * if the first is a control point, or the other way around).
     **/
    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >
    isNearbyControlPoint(double x,double y,double acceptance,ControlPointSelectionPrefEnum pref,int* index) const;

    /**
     * @brief Given the control point in parameter, return its index in the curve's control points list.
     * If no such control point could be found, -1 is returned.
     **/
    int getControlPointIndex(const boost::shared_ptr<BezierCP> & cp) const;
    int getControlPointIndex(const BezierCP* cp) const;

    /**
     * @brief Given the feather point in parameter, return its index in the curve's feather points list.
     * If no such feather point could be found, -1 is returned.
     **/
    int getFeatherPointIndex(const boost::shared_ptr<BezierCP> & fp) const;

    /**
     * @brief Returns the control point at the given index if any, NULL otherwise.
     **/
    boost::shared_ptr<BezierCP> getControlPointAtIndex(int index) const;

    /**
     * @brief Returns the feather point at the given index if any, NULL otherwise.
     **/
    boost::shared_ptr<BezierCP> getFeatherPointAtIndex(int index) const;
    boost::shared_ptr<BezierCP> getFeatherPointForControlPoint(const boost::shared_ptr<BezierCP> & cp) const;
    boost::shared_ptr<BezierCP> getControlPointForFeatherPoint(const boost::shared_ptr<BezierCP> & fp) const;

    /**
     * @brief Returns all the control points/feather points within the rectangle
     * given by (l,r,b,t). Each control point is paired with its counter part.
     * The first member is always "the selected point" and the second the counter part.
     * @param mode An integer representing what the function should do.
     * mode == 0: Add all the cp/fp within the rect and their counter parts in the return value
     * mode == 1: Add only the cp within the rect and their respective feather points counter parts
     * mode == 2: Add only the fp within the rect and their repsective control points counter parts
     **/
    std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >
    controlPointsWithinRect(double l,double r,double b,double t,double acceptance,int mode) const;
    static void leftDerivativeAtPoint(bool useGuiCurves,double time,const BezierCP & p,const BezierCP & prev, const Transform::Matrix3x3& transform,double *dx,double *dy);
    static void rightDerivativeAtPoint(bool useGuiCurves,double time,const BezierCP & p,const BezierCP & next, const Transform::Matrix3x3& transform, double *dx,double *dy);

    /**
     * @brief Computes the location of the feather extent relative to the current feather point position and
     * the given feather distance.
     * In the case the control point and the feather point of the bezier are distinct, this function just makes use
     * of Thales theorem.
     * If the feather point and the control point are equal then this function computes the left and right derivative
     * of the bezier at that point to determine the direction in which the extent is.
     * @returns The delta from the given feather point to apply to find out the extent position.
     *
     * Note that the delta will be applied to fp.
     **/
    static Natron::Point expandToFeatherDistance(bool useGuiCurve,
                                                 const Natron::Point & cp, //< the point
                                                 Natron::Point* fp, //< the feather point
                                                 double featherDistance, //< feather distance
                                                 //const std::list<Natron::Point> & featherPolygon, //< the polygon of the bezier
                                                 //const RectD & featherPolyBBox, //< helper to speed-up pointInPolygon computations
                                                 double time, //< time
                                                 bool clockWise, //< is the bezier  clockwise oriented or not
                                                 const Transform::Matrix3x3& transform,
                                                 std::list<boost::shared_ptr<BezierCP> >::const_iterator prevFp, //< iterator pointing to the feather before curFp
                                                 std::list<boost::shared_ptr<BezierCP> >::const_iterator curFp, //< iterator pointing to fp
                                                 std::list<boost::shared_ptr<BezierCP> >::const_iterator nextFp); //< iterator pointing after curFp
    enum FillRuleEnum
    {
        eFillRuleOddEven,
        eFillRuleWinding
    };

    
    bool isFeatherPolygonClockwiseOriented(bool useGuiCurve,double time) const;
    
    /**
     * @brief Refresh the polygon orientation for a specific keyframe or for all keyframes. Auto polygon orientation must be set to true
     * so make sure setAutoOrientationComputation(true) has been called before.
     **/
    void refreshPolygonOrientation(bool useGuiCurve,double time);
    void refreshPolygonOrientation(bool useGuiCurve);

    void setAutoOrientationComputation(bool autoCompute);
    
    bool dequeueGuiActions();
    
private:
    
    virtual void onTransformSet(double time) OVERRIDE FINAL;
    
    bool isFeatherPolygonClockwiseOrientedInternal(bool useGuiCurve,double time) const;
    
    void computePolygonOrientation(bool useGuiCurves,double time,bool isStatic) const;
    
    /*
     * @brief If the node is currently involved in a render, returns false, otherwise returns true
     */
    bool canSetInternalPoints() const;
    
    void copyInternalPointsToGuiPoints();
    
public:
    

    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void save(RotoItemSerialization* obj) const OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void load(const RotoItemSerialization & obj) OVERRIDE;

    void getKeyframeTimes(std::set<int> *times) const;
    
    void getKeyframeTimesAndInterpolation(std::list<std::pair<int,Natron::KeyframeTypeEnum> > *keys) const;

    /**
     * @brief Get the nearest previous keyframe from the given time.
     * If nothing was found INT_MIN is returned.
     **/
    int getPreviousKeyframeTime(double time) const;

    /**
     * @brief Get the nearest next keyframe from the given time.
     * If nothing was found INT_MAX is returned.
     **/
    int getNextKeyframeTime(double time) const;
    
    int getKeyFrameIndex(double time) const;
    
    void setKeyFrameInterpolation(Natron::KeyframeTypeEnum interp,int index);


Q_SIGNALS:

    void aboutToClone();

    void cloned();

    void keyframeSet(double time);

    void keyframeRemoved(double time);
    
    void animationRemoved();
        
    void controlPointRemoved();

private:

    boost::scoped_ptr<BezierPrivate> _imp;
};


struct RotoPoint
{
    Natron::Point pos;
    double pressure;
    double timestamp;
    
    RotoPoint() : pos(), pressure(0), timestamp(0) {}

    RotoPoint(const Natron::Point &pos_, double pressure_, double timestamp_)
    : pos(pos_), pressure(pressure_), timestamp(timestamp_) {}

    RotoPoint(double x, double y, double pressure_, double timestamp_)
    : pressure(pressure_), timestamp(timestamp_) { pos.x = x; pos.y = y; }
};

/**
 * @class Base class for all strokes
 **/
struct RotoStrokeItemPrivate;
class RotoStrokeItem :
public RotoDrawableItem
{
public:
    
    RotoStrokeItem(Natron::RotoStrokeType type,
                   const boost::shared_ptr<RotoContext>& context,
                   const std::string & name,
                   const boost::shared_ptr<RotoLayer>& parent);
    
    virtual ~RotoStrokeItem();
    
    
    Natron::RotoStrokeType getBrushType() const;
    
    /**
     * @brief Appends to the paint stroke the raw points list.
     * @returns True if the number of points is > 1
     **/
    bool appendPoint(const RotoPoint& p);
    
    std::vector<cairo_pattern_t*> getPatternCache() const;
    void updatePatternCache(const std::vector<cairo_pattern_t*>& cache);
    
    double renderSingleStroke(const boost::shared_ptr<RotoStrokeItem>& stroke,
                              const RectD& rod,
                              const std::list<std::pair<Natron::Point,double> >& points,
                              unsigned int mipmapLevel,
                              double par,
                              const Natron::ImageComponents& components,
                              Natron::ImageBitDepthEnum depth,
                              double distToNext,
                              boost::shared_ptr<Natron::Image> *wholeStrokeImage);

    
    
    bool getMostRecentStrokeChangesSinceAge(int lastAge, std::list<std::pair<Natron::Point,double> >* points, RectD* pointsBbox,
                                             int* newAge);
    
    
    void setStrokeFinished();
    
    
    virtual void clone(const RotoItem* other) OVERRIDE FINAL;
    
    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void save(RotoItemSerialization* obj) const OVERRIDE FINAL;
    
    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void load(const RotoItemSerialization & obj) OVERRIDE FINAL;
    
    virtual RectD getBoundingBox(double time) const OVERRIDE FINAL;
    
    
    ///bbox is in canonical coords
    void evaluateStroke(unsigned int mipMapLevel, double time, std::list<std::pair<Natron::Point,double> >* points,
                        RectD* bbox = 0) const;
    
    const Curve& getXControlPoints() const;
    const Curve& getYControlPoints() const;    
private:
    
    RectD computeBoundingBox(double time) const;
    
    
    boost::scoped_ptr<RotoStrokeItemPrivate> _imp;
};


/**
 * @class This class is a member of all effects instantiated in the context "paint". It describes internally
 * all the splines data structures and their state.
 * This class is MT-safe.
 **/
class RotoContextSerialization;
struct RotoContextPrivate;
class RotoContext
: public QObject, public boost::enable_shared_from_this<RotoContext>
{
    Q_OBJECT

public:

    
    RotoContext(const boost::shared_ptr<Natron::Node>& node);

    virtual ~RotoContext();
    
    /**
     * @brief We have chosen to disable rotopainting and roto shapes from the same RotoContext because the rendering techniques are
     * very much differents. The rotopainting systems requires an entire compositing tree held inside whereas the rotoshapes
     * are rendered and optimized by Cairo internally.
     **/
    bool isRotoPaint() const;
    
    void createBaseLayer();
    
    boost::shared_ptr<RotoLayer> getOrCreateBaseLayer() ;

    /**
     * @brief Returns true when the context is empty (it has no shapes)
     **/
    bool isEmpty() const;

    /**
     * @brief Turn on/off auto-keying
     **/
    void setAutoKeyingEnabled(bool enabled);
    bool isAutoKeyingEnabled() const;

    /**
     * @brief Turn on/off feather link. When enabled the feather point will move by the same delta as the corresponding spline point
     * is moved by.
     **/
    void setFeatherLinkEnabled(bool enabled);
    bool isFeatherLinkEnabled() const;

    /**
     * @brief Turn on/off ripple edit. When enabled moving a point at a given keyframe will move it to the same position
     * at all keyframes.
     **/
    void setRippleEditEnabled(bool enabled);
    bool isRippleEditEnabled() const;

    int getTimelineCurrentTime() const;

    /**
     * @brief Create a new layer to the currently selected layer.
     **/
    boost::shared_ptr<RotoLayer> addLayer();
private:
    
    boost::shared_ptr<RotoLayer> addLayerInternal(bool declarePython);
public:
    
    
    /**
     * @brief Add an existing layer to the layers
     **/
    void addLayer(const boost::shared_ptr<RotoLayer> & layer);
    

    /**
     * @brief Make a new bezier curve and append it into the currently selected layer.
     * @param baseName A hint to name the item. It can be something like "Bezier", "Ellipse", "Rectangle" , etc...
     **/
    boost::shared_ptr<Bezier> makeBezier(double x,double y,const std::string & baseName,double time, bool isOpenBezier);
    boost::shared_ptr<Bezier> makeEllipse(double x,double y,double diameter,bool fromCenter,double time);
    boost::shared_ptr<Bezier> makeSquare(double x,double y,double initialSize,double time);
    
    
    boost::shared_ptr<RotoStrokeItem> makeStroke(Natron::RotoStrokeType type,
                                                 const std::string& baseName,
                                                 bool clearSel);
    
    std::string generateUniqueName(const std::string& baseName);
    
    /**
     * @brief Removes the given item from the context. This also removes the item from the selection
     * if it was selected. If the item has children, this will also remove all the children.
     **/
    void removeItem(const boost::shared_ptr<RotoItem>& item, RotoItem::SelectionReasonEnum reason = RotoItem::eSelectionReasonOther);

    ///This is here for undo/redo purpose. Do not call this
    void addItem(const boost::shared_ptr<RotoLayer>& layer, int indexInLayer, const boost::shared_ptr<RotoItem> & item,RotoItem::SelectionReasonEnum reason);
    /**
     * @brief Returns a const ref to the layers list. This can only be called from
     * the main thread.
     **/
    const std::list< boost::shared_ptr<RotoLayer> > & getLayers() const;

    /**
     * @brief Returns a bezier curves nearby the point (x,y) and the parametric value
     * which would be used to find the exact bezier point lying on the curve.
     **/
    boost::shared_ptr<Bezier> isNearbyBezier(double x,double y,double acceptance,int* index,double* t,bool *feather) const;


    /**
     * @brief Returns the region of definition of the shape unioned to the region of definition of the node
     * or the project format.
     **/
    void getMaskRegionOfDefinition(double time,
                                   int view,
                                   RectD* rod) const; //!< rod in canonical coordinates

    
   
    boost::shared_ptr<Natron::Image> renderMaskFromStroke(const boost::shared_ptr<RotoDrawableItem>& stroke,
                                                          const RectI& roi,
                                                          U64 rotoAge,
                                                          U64 nodeHash,
                                                          const Natron::ImageComponents& components,
                                                          SequenceTime time,
                                                          int view,
                                                          Natron::ImageBitDepthEnum depth,
                                                          unsigned int mipmapLevel);
    
    double renderSingleStroke(const boost::shared_ptr<RotoStrokeItem>& stroke,
                            const RectD& rod,
                            const std::list<std::pair<Natron::Point,double> >& points,
                            unsigned int mipmapLevel,
                            double par,
                            const Natron::ImageComponents& components,
                            Natron::ImageBitDepthEnum depth,
                            double distToNext,
                            boost::shared_ptr<Natron::Image> *wholeStrokeImage);
    
private:
    
    boost::shared_ptr<Natron::Image> renderMaskInternal(const boost::shared_ptr<RotoDrawableItem>& stroke,
                                                        const RectI & roi,
                                                        const Natron::ImageComponents& components,
                                                        SequenceTime time,
                                                        Natron::ImageBitDepthEnum depth,
                                                        unsigned int mipmapLevel,
                                                        const std::list<std::pair<Natron::Point,double> >& points,
                                                        const boost::shared_ptr<Natron::Image> &image);
    
public:
    


    /**
     * @brief To be called when a change was made to trigger a new render.
     **/
    void evaluateChange();
    void evaluateChange_noIncrement();
    
    void incrementAge();
    
    void clearViewersLastRenderedStrokes();

    /**
     *@brief Returns the age of the roto context
     **/
    U64 getAge();

    ///Serialization
    void save(RotoContextSerialization* obj) const;

    ///Deserialization
    void load(const RotoContextSerialization & obj);


    /**
     * @brief This must be called by the GUI whenever an item is selected. This is recursive for layers.
     **/
    void select(const boost::shared_ptr<RotoItem> & b, RotoItem::SelectionReasonEnum reason);

    ///for convenience
    void select(const std::list<boost::shared_ptr<RotoDrawableItem> > & beziers, RotoItem::SelectionReasonEnum reason);
    void select(const std::list<boost::shared_ptr<RotoItem> > & items, RotoItem::SelectionReasonEnum reason);

    /**
     * @brief This must be called by the GUI whenever an item is deselected. This is recursive for layers.
     **/
    void deselect(const boost::shared_ptr<RotoItem> & b, RotoItem::SelectionReasonEnum reason);

    ///for convenience
    void deselect(const std::list<boost::shared_ptr<Bezier> > & beziers, RotoItem::SelectionReasonEnum reason);
    void deselect(const std::list<boost::shared_ptr<RotoItem> > & items, RotoItem::SelectionReasonEnum reason);

    void clearAndSelectPreviousItem(const boost::shared_ptr<RotoItem>& item,RotoItem::SelectionReasonEnum reason);
    
    void clearSelection(RotoItem::SelectionReasonEnum reason);

    ///only callable on main-thread
    void setKeyframeOnSelectedCurves();

    ///only callable on main-thread
    void removeKeyframeOnSelectedCurves();
    
    void removeAnimationOnSelectedCurves();

    ///only callable on main-thread
    void goToPreviousKeyframe();

    ///only callable on main-thread
    void goToNextKeyframe();

    /**
     * @brief Returns a list of the currently selected curves. Can only be called on the main-thread.
     **/
    std::list< boost::shared_ptr<RotoDrawableItem> > getSelectedCurves() const;


    /**
     * @brief Returns a const ref to the selected items. This can only be called on the main thread.
     **/
    const std::list< boost::shared_ptr<RotoItem> > & getSelectedItems() const;

    /**
     * @brief Returns a list of all the curves in the order in which they should be rendered.
     * Non-active curves will not be inserted into the list.
     * MT-safe
     **/
    std::list< boost::shared_ptr<RotoDrawableItem> > getCurvesByRenderOrder(bool onlyActivated = true) const;
    
    int getNCurves() const;
    
    boost::shared_ptr<Natron::Node> getNode() const;
    
    boost::shared_ptr<RotoLayer> getLayerByName(const std::string & n) const;
    boost::shared_ptr<RotoItem> getItemByName(const std::string & n) const;
    boost::shared_ptr<RotoItem> getLastInsertedItem() const;

#ifdef NATRON_ROTO_INVERTIBLE
    boost::shared_ptr<Bool_Knob> getInvertedKnob() const;
#endif
    boost::shared_ptr<Color_Knob> getColorKnob() const;


    boost::shared_ptr<RotoItem> getLastItemLocked() const;
    boost::shared_ptr<RotoLayer> getDeepestSelectedLayer() const;

    void onItemLockedChanged(const boost::shared_ptr<RotoItem>& item,RotoItem::SelectionReasonEnum reason);

    void emitRefreshViewerOverlays();

    void getBeziersKeyframeTimes(std::list<int> *times) const;

    /**
     * @brief Returns the name of the node holding this item
     **/
    std::string getRotoNodeName() const;
    
    void onItemScriptNameChanged(const boost::shared_ptr<RotoItem>& item);
    void onItemLabelChanged(const boost::shared_ptr<RotoItem>& item);
    
    void onItemKnobChanged();

    void declarePythonFields();
    
    void changeItemScriptName(const std::string& oldFullyQualifiedName,const std::string& newFullyQUalifiedName);
    
    void declareItemAsPythonField(const boost::shared_ptr<RotoItem>& item);
    void removeItemAsPythonField(const boost::shared_ptr<RotoItem>& item);
    
    /**
     * @brief Rebuilds the connection between nodes used internally by the rotopaint tree
     * To be called whenever changes position in the hierarchy or when one gets removed/inserted
     **/
    void refreshRotoPaintTree();
    
    void onRotoPaintInputChanged(const boost::shared_ptr<Natron::Node>& node);
        
    void getRotoPaintTreeNodes(std::list<boost::shared_ptr<Natron::Node> >* nodes) const;
    
    void setStrokeBeingPainted(const boost::shared_ptr<RotoStrokeItem>& stroke);
    boost::shared_ptr<RotoStrokeItem> getStrokeBeingPainted() const;
    
    /**
     * @brief First searches through the selected layer which one is the deepest in the hierarchy.
     * If nothing is found, it searches through the selected items and find the deepest selected item's layer
     **/
    boost::shared_ptr<RotoLayer> findDeepestSelectedLayer() const;
    
    void dequeueGuiActions();
    
Q_SIGNALS:

    /**
     * Emitted when the selection is changed. The integer corresponds to the
     * RotoItem::SelectionReasonEnum enum.
     **/
    void selectionChanged(int);

    void restorationComplete();

    void itemInserted(int);

    void itemRemoved(const boost::shared_ptr<RotoItem>&,int);

    void refreshViewerOverlays();

    void itemLockedChanged(int reason);
    
    void itemScriptNameChanged(const boost::shared_ptr<RotoItem>&);
    void itemLabelChanged(const boost::shared_ptr<RotoItem>&);

public Q_SLOTS:

    void onAutoKeyingChanged(bool enabled);

    void onFeatherLinkChanged(bool enabled);

    void onRippleEditChanged(bool enabled);
    
    void onSelectedKnobCurveChanged();
    
    void onLifeTimeKnobValueChanged(int, int);

private:
    
    void selectInternal(const boost::shared_ptr<RotoItem>& b);
    void deselectInternal(boost::shared_ptr<RotoItem> b);

    void removeItemRecursively(const boost::shared_ptr<RotoItem>& item,RotoItem::SelectionReasonEnum reason);

   
    boost::scoped_ptr<RotoContextPrivate> _imp;
};

#endif // _Engine_RotoContext_h_
