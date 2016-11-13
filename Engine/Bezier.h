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

#ifndef Engine_Bezier_h
#define Engine_Bezier_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <set>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated-declarations)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated-declarations)

#include "Global/GlobalDefines.h"

#include "Engine/RotoDrawableItem.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


#define ROTO_BEZIER_EVAL_ITERATIVE

NATRON_NAMESPACE_ENTER;


/**
 * @class A base class for all items made by the roto context
 **/


/**
 * @class This class represents a bezier curve.
 * Note that the bezier also supports feather points.
 * This class is MT-safe
 *
 * The curve supports animation, and by default once the first control point is added the curve
 * has at least a minimum of 1 keyframe.
 **/

struct ParametricPoint
{
    double x,y,t;
};

struct BezierPrivate;
class Bezier
    : public RotoDrawableItem
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    Bezier(const KnobItemsTablePtr& model,
           const std::string& baseName,
           bool isOpenBezier);

    Bezier(const Bezier& other);

    virtual ~Bezier();

    virtual void copyItem(const KnobTableItemPtr& other) OVERRIDE FINAL;

    virtual bool getCanAnimateUserKeyframes() const OVERRIDE FINAL { return true; }
    
    static double bezierEval(double p0,
                      double p1,
                      double p2,
                      double p3,
                      double t);

    static void bezierFullPoint(const Point & p0,
                                const Point & p1,
                                const Point & p2,
                                const Point & p3,
                                double t,
                                Point *p0p1,
                                Point *p1p2,
                                Point *p2p3,
                                Point *p0p1_p1p2,
                                Point *p1p2_p2p3,
                                Point *dest);

    static void bezierPoint(const Point & p0,
                            const Point & p1,
                            const Point & p2,
                            const Point & p3,
                            double t,
                            Point *dest);

    static void bezierPointBboxUpdate(const Point & p0,
                                      const Point & p1,
                                      const Point & p2,
                                      const Point & p3,
                                      RectD *bbox,
                                      bool* bboxSet);

    bool isOpenBezier() const;

    /**
     * @brief Used to differentiate real shapes with feather of paint strokes which does not have a feather
     **/
    virtual bool useFeatherPoints() const { return true; }


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
    BezierCPPtr addControlPoint(double x, double y, double time, ViewSetSpec view);

private:

    BezierCPPtr addControlPointInternal(double x, double y, double time, ViewIdx view);

public:

    /**
     * @brief Adds a new control point to the curve after the control point at the given index.
     * A feather point will be added, at the same position.
     * If auto keying is enabled, and there's no keyframe a new keyframe will be set at the current time.
     *
     * If index is -1 then the point will be added as the first point of the curve.
     * If index is invalid an invalid argument exception will be thrown.
     **/
    BezierCPPtr addControlPointAfterIndex(int index, double t, ViewSetSpec view);

private:

    BezierCPPtr addControlPointAfterIndexInternal(int index, double t, ViewIdx view);

public:

    /**
     * @brief Returns the number of control points of the bezier
     **/
    int getControlPointsCount(ViewGetSpec view) const;

    /**
     * @brief Given the (x,y) coordinates of a point, this function returns whether a point lies on
     * the cubic bezier curve of the feather points or of the control points or not.
     * @returns The index of the starting control point (P0) of the bezier segment on which the given
     * point lies. If the point doesn't belong to any bezier segment of this curve then it will return -1.
     **/
    int isPointOnCurve(double x, double y, double acceptance, double time, ViewGetSpec view, double *t, bool* feather) const;

    /**
     * @brief Set whether the curve is finished or not. A finished curve will have an arc between the first
     * and the last control point.
     **/
    void setCurveFinished(bool finished, ViewSetSpec view);
    bool isCurveFinished(ViewGetSpec view) const;


    /**
     * @brief Removes the control point at the given index if any. The feather point will also be removed , at the same position.
     * If auto keying is enabled, and there's no keyframe a new keyframe will be set at the current time.
     **/
    void removeControlPointByIndex(int index, ViewSetSpec view);

private:

    void removeControlPointByIndexInternal(int index, ViewIdx view);

public:

    /**
     * @brief Move the control point at the given index by the given dx and dy.
     * If feather link is enabled, the feather point will also be moved by the same deltas.
     * If  there's no keyframe at the current time, a new keyframe will be added.
     * This function asserts that  auto-keying is  enabled.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void movePointByIndex(int index, double time, ViewSetSpec view, double dx, double dy);

    /**
     * @brief Set the control point at the given index to x,y.
     * If feather link is enabled, the feather point will also be moved by the same deltas.
     * If  there's no keyframe at the current time, a new keyframe will be added.
     * This function asserts that  auto-keying is  enabled.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void setPointByIndex(int index, double time, ViewSetSpec view, double x, double y);

    /**
     * @brief Moves the feather point at the given index if any by the given dx and dy.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void moveFeatherByIndex(int index, double time, ViewSetSpec view, double dx, double dy);

private:

    void movePointByIndexInternal(int index, double time, ViewSetSpec view, double dx, double dy, bool onlyFeather);
    void setPointByIndexInternal(int index, double time, ViewSetSpec view, double dx, double dy);

    void movePointByIndexInternalForView(int index, double time, ViewIdx view, double dx, double dy, bool onlyFeather);
    void setPointByIndexInternalForView(int index, double time, ViewIdx view, double dx, double dy);

public:


    /**
     * @brief Moves the left bezier point of the control point at the given index by the given deltas
     * and at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void moveLeftBezierPoint(int index, double time, ViewSetSpec view, double dx, double dy);

    /**
     * @brief Moves the right bezier point of the control point at the given index by the given deltas
     * and at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void moveRightBezierPoint(int index, double time, ViewSetSpec view, double dx, double dy);

private:

    void moveBezierPointInternal(BezierCP* cpParam,
                                 BezierCP* fpParam,
                                 int index,
                                 double time,
                                 ViewSetSpec view,
                                 double lx, double ly, double rx, double ry,
                                 double flx, double fly, double frx, double fry,
                                 bool isLeft,
                                 bool moveBoth,
                                 bool onlyFeather);

    void moveBezierPointInternalForView(BezierCP* cpParam,
                                 BezierCP* fpParam,
                                 int index,
                                 double time,
                                 ViewIdx view,
                                 double lx, double ly, double rx, double ry,
                                 double flx, double fly, double frx, double fry,
                                 bool isLeft,
                                 bool moveBoth,
                                 bool onlyFeather);

public:


    /**
     * @brief Transforms the given point at the given time by the given matrix.
     **/
    void transformPoint(const BezierCPPtr & point, double time, ViewSetSpec view, Transform::Matrix3x3* matrix);

private:

    void transformPointInternal(const BezierCPPtr & point, double time, ViewIdx view, Transform::Matrix3x3* matrix);

public:
    
    /**
     * @brief Provided for convenience. It set the left bezier point of the control point at the given index to
     * the position given by (x,y) at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     * This function will also set the point of the feather point at the given x,y.
     **/
    void setLeftBezierPoint(int index, double time, ViewSetSpec view, double x, double y);

    /**
     * @brief Provided for convenience. It set the right bezier point of the control point at the given index to
     * the position given by (x,y) at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     * This function will also set the point of the feather point at the given x,y.
     **/
    void setRightBezierPoint(int index, double time, ViewSetSpec view, double x, double y);


    /**
     * @brief This function is a combinaison of setPosition + setLeftBezierPoint / setRightBeziePoint
     **/
    void setPointAtIndex(bool feather, int index, double time, ViewSetSpec view, double x, double y, double lx, double ly, double rx, double ry);

private:

    void setPointAtIndexInternal(bool setLeft, bool setRight, bool setPoint, bool feather, bool featherAndCp, int index, double time, ViewSetSpec view, double x, double y, double lx, double ly, double rx, double ry);

    void setPointAtIndexInternalForView(bool setLeft, bool setRight, bool setPoint, bool feather, bool featherAndCp, int index, double time, ViewIdx view, double x, double y, double lx, double ly, double rx, double ry);

public:


    /**
     * @brief Set the left and right bezier point of the control point.
     **/
    void movePointLeftAndRightIndex(BezierCP & cp,
                                    BezierCP & fp,
                                    double time,
                                    ViewSetSpec view,
                                    double lx, double ly, double rx, double ry,
                                    double flx, double fly, double frx, double fry,
                                    bool onlyFeather);


    /**
     * @brief Removes the feather point at the given index by making it equal the "true" control point.
     **/
    void removeFeatherAtIndex(int index, ViewSetSpec view);

private:

    void removeFeatherAtIndexForView(int index, ViewIdx view);

public:

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
    void smoothPointAtIndex(int index, double time, ViewSetSpec view, const std::pair<double, double>& pixelScale);

    /**
     * @brief Cusps the curvature of the bezier at the given index by reducing the tangents.
     * This is also applied to the feather points.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void cuspPointAtIndex(int index, double time, ViewSetSpec view, const std::pair<double, double>& pixelScale);

    void getMotionBlurSettings(const double time,
                               ViewGetSpec view,
                               double* startTime,
                               double* endTime,
                               double* timeStep) const;

private:

    void smoothOrCuspPointAtIndex(bool isSmooth, int index, double time, ViewSetSpec view, const std::pair<double, double>& pixelScale);

    void smoothOrCuspPointAtIndexInternal(bool isSmooth, int index, double time, ViewIdx view, const std::pair<double, double>& pixelScale);

public:



    static void deCastelJau(const std::list<BezierCPPtr >& cps, double time, unsigned int mipMapLevel,
                            bool finished,
                            int nBPointsPerSegment,
                            const Transform::Matrix3x3& transform,
                            std::vector<std::vector<ParametricPoint> >* points,
                            std::vector<ParametricPoint >* pointsSingleList,
                            RectD* bbox);
    static void point_line_intersection(const Point &p1,
                                        const Point &p2,
                                        const Point &pos,
                                        int *winding);

    /**
     * @brief Evaluates the spline at the given time and returns the list of all the points on the curve.
     * @param nbPointsPerSegment controls how many points are used to draw one Bezier segment
     **/
    void evaluateAtTime_DeCasteljau(double time,
                                    ViewGetSpec view,
                                    unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                    int nbPointsPerSegment,
#else
                                    double errorScale,
#endif
                                    std::vector<std::vector<ParametricPoint> >* points,
                                    RectD* bbox) const;

    void evaluateAtTime_DeCasteljau(double time,
                                    ViewGetSpec view,
                                    unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                    int nbPointsPerSegment,
#else
                                    double errorScale,
#endif
                                    std::vector<ParametricPoint >* points,
                                    RectD* bbox) const;

private:

    void evaluateAtTime_DeCasteljau_internal(double time,
                                             ViewGetSpec view,
                                             unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                             int nbPointsPerSegment,
#else
                                             double errorScale,
#endif
                                             std::vector<std::vector<ParametricPoint> >* points,
                                             std::vector<ParametricPoint >* pointsSingleList,
                                             RectD* bbox) const;

public:

    /**
     * @brief Same as evaluateAtTime_DeCasteljau but nbPointsPerSegment is approximated automatically
     **/
    void evaluateAtTime_DeCasteljau_autoNbPoints(double time,
                                                 ViewGetSpec view,
                                                 unsigned int mipMapLevel,
                                                 std::vector<std::vector<ParametricPoint> >* points,
                                                 RectD* bbox) const;

    /**
     * @brief Evaluates the bezier formed by the feather points. Segments which are equal to the control points of the bezier
     * will not be drawn.
     **/
    void evaluateFeatherPointsAtTime_DeCasteljau(double time,
                                                 ViewGetSpec view,
                                                 unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                 int nbPointsPerSegment,
#else
                                                 double errorScale,
#endif
                                                 bool evaluateIfEqual,
                                                 std::vector<std::vector<ParametricPoint>  >* points,
                                                 RectD* bbox) const;

    void evaluateFeatherPointsAtTime_DeCasteljau(double time,
                                                 ViewGetSpec view,
                                                 unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                 int nbPointsPerSegment,
#else
                                                 double errorScale,
#endif
                                                 bool evaluateIfEqual,
                                                 std::vector<ParametricPoint >* points,
                                                 RectD* bbox) const;

private:

    void evaluateFeatherPointsAtTime_DeCasteljau_internal(double time,
                                                          ViewGetSpec view,
                                                          unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                          int nbPointsPerSegment,
#else
                                                          double errorScale,
#endif
                                                          bool evaluateIfEqual,
                                                          std::vector<std::vector<ParametricPoint>  >* points,
                                                          std::vector<ParametricPoint >* pointsSingleList,
                                                          RectD* bbox) const;

public:

    /**
     * @brief Returns the bounding box of the bezier.
     **/
    virtual RectD getBoundingBox(double time,ViewGetSpec view) const OVERRIDE;
    
    static void bezierSegmentListBboxUpdate(const std::list<BezierCPPtr > & points,
                                            bool finished,
                                            bool isOpenBezier,
                                            double time,
                                            unsigned int mipMapLevel,
                                            const Transform::Matrix3x3& transform,
                                            RectD* bbox);

    /**
     * @brief Returns the control points of the bezier curve. This can only ever be called on the main thread.
     **/
    std::list< BezierCPPtr >  getControlPoints(ViewGetSpec view) const;

protected:

    std::list< BezierCPPtr >  getControlPoints_internal(ViewGetSpec view);

public:

    /**
     * @brief Returns the feather points of the bezier curve. This can only ever be called on the main thread.
     **/
    std::list< BezierCPPtr > getFeatherPoints(ViewGetSpec view) const;

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
    std::pair<BezierCPPtr, BezierCPPtr >isNearbyControlPoint(double x, double y, double acceptance, double time, ViewGetSpec view, ControlPointSelectionPrefEnum pref, int* index) const;

    /**
     * @brief Given the control point in parameter, return its index in the curve's control points list.
     * If no such control point could be found, -1 is returned.
     **/
    int getControlPointIndex(const BezierCPPtr & cp, ViewGetSpec view) const;
    int getControlPointIndex(const BezierCP* cp, ViewGetSpec view) const;

    /**
     * @brief Given the feather point in parameter, return its index in the curve's feather points list.
     * If no such feather point could be found, -1 is returned.
     **/
    int getFeatherPointIndex(const BezierCPPtr & fp, ViewGetSpec view) const;

    /**
     * @brief Returns the control point at the given index if any, NULL otherwise.
     **/
    BezierCPPtr getControlPointAtIndex(int index, ViewGetSpec view) const;

    /**
     * @brief Returns the feather point at the given index if any, NULL otherwise.
     **/
    BezierCPPtr getFeatherPointAtIndex(int index, ViewGetSpec view) const;
    BezierCPPtr getFeatherPointForControlPoint(const BezierCPPtr & cp, ViewGetSpec view) const;
    BezierCPPtr getControlPointForFeatherPoint(const BezierCPPtr & fp, ViewGetSpec view) const;

    /**
     * @brief Returns all the control points/feather points within the rectangle
     * given by (l,r,b,t). Each control point is paired with its counter part.
     * The first member is always "the selected point" and the second the counter part.
     * @param mode An integer representing what the function should do.
     * mode == 0: Add all the cp/fp within the rect and their counter parts in the return value
     * mode == 1: Add only the cp within the rect and their respective feather points counter parts
     * mode == 2: Add only the fp within the rect and their repsective control points counter parts
     **/
    std::list< std::pair<BezierCPPtr, BezierCPPtr > > controlPointsWithinRect(double time, ViewGetSpec view, double l, double r, double b, double t, double acceptance, int mode) const;

    static void leftDerivativeAtPoint(double time, const BezierCP & p, const BezierCP & prev, const Transform::Matrix3x3& transform, double *dx, double *dy);
    static void rightDerivativeAtPoint(double time, const BezierCP & p, const BezierCP & next, const Transform::Matrix3x3& transform, double *dx, double *dy);

    /**
     * @brief Computes the location of the feather extent relative to the current feather point position and
     * the given feather distance.
     * In the case the control point and the feather point of the bezier are distinct, this function just makes use
     * of Thales theorem.
       >     * If the feather point and the control point are equal then this function computes the left and right derivative
     * of the bezier at that point to determine the direction in which the extent is.
     * @returns The delta from the given feather point to apply to find out the extent position.
     *
     * Note that the delta will be applied to fp.
     **/
    static Point expandToFeatherDistance(const Point & cp,         //< the point
                                         Point* fp,         //< the feather point
                                         double featherDistance,         //< feather distance
                                         double time,         //< time
                                         bool clockWise,         //< is the bezier  clockwise oriented or not
                                         const Transform::Matrix3x3& transform,
                                         std::list<BezierCPPtr >::const_iterator prevFp,         //< iterator pointing to the feather before curFp
                                         std::list<BezierCPPtr >::const_iterator curFp,         //< iterator pointing to fp
                                         std::list<BezierCPPtr >::const_iterator nextFp);         //< iterator pointing after curFp
    enum FillRuleEnum
    {
        eFillRuleOddEven,
        eFillRuleWinding
    };


    bool isFeatherPolygonClockwiseOriented(double time, ViewGetSpec view) const;

    /**
     * @brief Refresh the polygon orientation for a specific keyframe or for all keyframes. Auto polygon orientation must be set to true
     * so make sure setAutoOrientationComputation(true) has been called before.
     **/
    void refreshPolygonOrientation(double time, ViewSetSpec view);
    void refreshPolygonOrientation(ViewSetSpec view);
    void refreshPolygonOrientationForView(ViewIdx view);

    void setAutoOrientationComputation(bool autoCompute);

    KnobDoublePtr getFeatherKnob() const;
    KnobDoublePtr getFeatherFallOffKnob() const;
    KnobChoicePtr getFallOffRampTypeKnob() const;
    KnobDoublePtr getMotionBlurAmountKnob() const;
    KnobDoublePtr getShutterOffsetKnob() const;
    KnobDoublePtr getShutterKnob() const;
    KnobChoicePtr getShutterTypeKnob() const;

    virtual std::string getBaseItemName() const OVERRIDE FINAL;

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return true;
    }
    
private:

    virtual void onTransformSet(double time, ViewSetSpec view) OVERRIDE FINAL;

    bool isFeatherPolygonClockwiseOrientedInternal(double time, ViewGetSpec view) const;

    void computePolygonOrientation(double time, ViewSetSpec view, bool isStatic) const;

    void computePolygonOrientationForView(double time, ViewIdx view, bool isStatic) const;

    void evaluateCurveModified();

public:



    bool isAutoKeyingEnabled() const;
    bool isFeatherLinkEnabled() const;
    bool isRippleEditEnabled() const;
    
    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)  OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj) OVERRIDE;


    virtual void appendToHash(double time, ViewIdx view, Hash64* hash) OVERRIDE FINAL;


private:


    virtual void onKeyFrameSet(double time, ViewSetSpec view) OVERRIDE FINAL;

    virtual void onKeyFrameRemoved(double time, ViewSetSpec view) OVERRIDE FINAL;

    void onKeyFrameSetForView(double time, ViewIdx view);

    void onKeyFrameRemovedForView(double time, ViewIdx view);

    virtual void initializeKnobs() OVERRIDE;

    boost::scoped_ptr<BezierPrivate> _imp;
};

inline BezierPtr
toBezier(const KnobHolderPtr& item)
{
    return boost::dynamic_pointer_cast<Bezier>(item);
}


NATRON_NAMESPACE_EXIT;


#endif // Engine_Bezier_h
