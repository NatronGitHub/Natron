//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef KNOBROTO_H
#define KNOBROTO_H

#include <list>
#include <set>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <QObject>

#include "Global/GlobalDefines.h"
/**
 * @class A Bezier is an animated control point of a Bezier. It is the starting point
 * and/or the ending point of a bezier segment. (It would correspond to P0/P3).
 * The left bezier point/right bezier point we refer to in the functions below
 * are respectively the P2 and P1 point.
 * 
 * Note on multi-thread:
 * All getters or const functions can be called in any thread, that is:
 * - The GUI thread (main-thread)
 * - The render thread (VideoEngine)
 * - The serialization thread (when saving)
 *
 * Setters or non-const functions can exclusively be called in the main-thread (Gui thread) to ensure there is no
 * race condition whatsoever.
 
 * More-over the setters must be called ONLY by the Bezier class which is the class handling the thread safety.
 * That's why non-const functions are private.
 **/
namespace Natron {
class Image;
class Node;
}
namespace boost {
    namespace serialization {
        class access;
    }
}

class RectI;
class RectD;

class Bezier;
class BezierSerialization;

struct BezierCPPrivate;
class BezierCP
{
    ///This is the unique class allowed to call the setters.
    friend class Bezier;
    friend class boost::serialization::access;
    
public:
    
    ///Constructore used by the serialization
    BezierCP();
    
    BezierCP(const BezierCP& other);
    
    BezierCP(Bezier* curve);
    
    virtual ~BezierCP();
    
    
    //////Non const functions, meant to be called by the Bezier class which is MT-safe
private:
    
    void clone(const BezierCP& other);
    
    void setPositionAtTime(int time,double x,double y);
    
    void setLeftBezierPointAtTime(int time,double x,double y);
    
    void setRightBezierPointAtTime(int time,double x,double y);
    
    void setStaticPosition(double x,double y);
    
    void setLeftBezierStaticPosition(double x,double y);
    
    void setRightBezierStaticPosition(double x,double y);
    
    void removeKeyframe(int time);

    void cuspPoint(int time,bool autoKeying,bool rippleEdit);
    
    void smoothPoint(int time,bool autoKeying,bool rippleEdit);
    
    
    //////Const functions, fine if called by another class than the Bezier class, as long
    //////as it remains on the main-thread (the setter thread).
public:
    
    virtual bool isFeatherPoint() const { return false; }

    bool equalsAtTime(int time,const BezierCP& other) const;
    
    bool getPositionAtTime(int time,double* x,double* y) const;
    
    bool getLeftBezierPointAtTime(int time,double* x,double* y) const;
    
    bool getRightBezierPointAtTime(int time,double *x,double *y) const;
    
    bool hasKeyFrameAtTime(int time) const;
    
    void getKeyframeTimes(std::set<int>* times) const;
    
    int getKeyframeTime(int index) const;
    
    int getKeyframesCount() const;
    
    Bezier* getCurve() const;
    
    /**
     * @brief Returns whether a tangent handle is nearby the given coordinates.
     * That is, this function a number indicating
     * if the given coordinates are close to the left control point (P2, ret = 0) or the right control point(P1, ret = 1).
     * If it is not close to any tangent, -1 is returned.
     * This function can also return the tangent of a feather point, to find out if the point is a feather point call
     * isFeatherPoint() on the returned control point.
     **/
    int isNearbyTangent(int time,double x,double y,double acceptance) const;
    
private:
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    
    boost::scoped_ptr<BezierCPPrivate> _imp;
    
};

class FeatherPoint : public BezierCP
{
public:
    
    FeatherPoint(Bezier* curve)
    : BezierCP(curve)
    {
    }
    
    virtual bool isFeatherPoint() const { return true; }
};

/**
 * @class This class represents a bezier curve.
 * Note that the bezier also supports feather points.
 * This class is MT-safe
 *
 * The curve supports animation, and by default once the first control point is added the curve
 * has at least a minimum of 1 keyframe.
 **/
class RotoContext;

struct BezierPrivate;
class Bezier
{
    
public:
    
    Bezier(RotoContext* context);
    
    ~Bezier();
    
    
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
    boost::shared_ptr<BezierCP> addControlPoint(double x,double y);
    
    /**
     * @brief Adds a new control point to the curve after the control point at the given index.
     * A feather point will be added, at the same position.
     * If auto keying is enabled, and there's no keyframe a new keyframe will be set at the current time.
     *
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
    int isPointOnCurve(double x,double y,double acceptance,double *t,bool* feather) const;
    
    /**
     * @brief Set whether the curve is finished or not. A finished curve will have an arc between the first
     * and the last control point.
     **/
    void setCurveFinished(bool finished);
    bool isCurveFinished() const;
    
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
    void movePointByIndex(int index,int time,double dx,double dy);
    
    /**
     * @brief Moves the feather point at the given index if any by the given dx and dy.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void moveFeatherByIndex(int index,int time,double dx,double dy);
    
    /**
     * @brief Moves the left bezier point of the control point at the given index by the given deltas
     * and at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void moveLeftBezierPoint(int index,int time,double dx,double dy);
    
    /**
     * @brief Moves the right bezier point of the control point at the given index by the given deltas
     * and at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void moveRightBezierPoint(int index,int time,double dx,double dy);
    
    /**
     * @brief Provided for convenience. It set the left bezier point of the control point at the given index to
     * the position given by (x,y) at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     * This function will also set the point of the feather point at the given x,y.
     **/
    void setLeftBezierPoint(int index,int time,double x,double y);
    
    /**
     * @brief Provided for convenience. It set the right bezier point of the control point at the given index to
     * the position given by (x,y) at the given time.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     * This function will also set the point of the feather point at the given x,y.
     **/
    void setRightBezierPoint(int index,int time,double x,double y);
    
    
    /**
     * @brief This function is a combinaison of setPosition + setLeftBezierPoint / setRightBeziePoint
     * It only works for feather points!
     **/
    void setPointAtIndex(bool feather,int index,int time,double x,double y,double lx,double ly,double rx,double ry);
    
    /**
     * @brief Set the left and right bezier point of the control point.
     **/
    void setPointLeftAndRightIndex(BezierCP& p,int time,double lx,double ly,double rx,double ry);
    
    /**
     * @brief Removes the feather point at the given index by making it equal the "true" control point.
     **/
    void removeFeatherAtIndex(int index);
    
    /**
     * @brief Smooth the curvature of the bezier at the given index by expanding the tangents.
     * This is also applied to the feather points.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void smoothPointAtIndex(int index,int time);
    
    /**
     * @brief Cusps the curvature of the bezier at the given index by reducing the tangents.
     * This is also applied to the feather points.
     * If auto keying is enabled and there's no keyframe at the current time, a new keyframe will be added.
     * If ripple edit is enabled, the point will be moved at the same location at all keyframes.
     **/
    void cuspPointAtIndex(int index,int time);
    
    /**
     * @brief Set a new keyframe at the given time. If a keyframe already exists this function does nothing.
     **/
    void setKeyframe(int time);
    
    /**
     * @brief Removes a keyframe at the given time if any.
     **/
    void removeKeyframe(int time);
    
    /**
     * @brief When deactivated the spline will not be taken into account when rendering, neither will it be visible on the viewer.
     **/
    void setActivated(bool activated);
    bool isActivated() const;
    
    /**
     * @brief Returns the number of keyframes for this spline.
     **/
    int getKeyframesCount() const;
    
    /**
     * @brief Evaluates the spline at the given time and returns the list of all the points on the curve.
     * @param nbPointsPerSegment controls how many points are used to draw one Bezier segment
     **/
    void evaluateAtTime_DeCastelJau(int time,unsigned int mipMapLevel,
                                    int nbPointsPerSegment,std::list<std::pair<double,double> >* points) const;
    
    /**
     * @brief Evaluates the bezier formed by the feather points. Segments which are equal to the control points of the bezier
     * will not be drawn.
     **/
    void evaluateFeatherPointsAtTime_DeCastelJau(int time,int nbPointsPerSegment,std::list<std::pair<double,double> >* points) const;
    
    /**
     * @brief Returns the bounding box of the bezier. The last value computed by evaluateAtTime_DeCastelJau will be returned,
     * otherwise if it has never been called, evaluateAtTime_DeCastelJau will be called to compute the bounding box.
     **/
    const RectD& getBoundingBox(int time) const;
    
    /**
     * @brief Returns a const ref to the control points of the bezier curve. This can only ever be called on the main thread.
     **/
    const std::list< boost::shared_ptr<BezierCP> >& getControlPoints() const;
    
    /**
     * @brief Returns a const ref to the feather points of the bezier curve. This can only ever be called on the main thread.
     **/
    const std::list< boost::shared_ptr<BezierCP> >& getFeatherPoints() const;
    
    /**
     * @brief Returns a pointer to a nearby control point if any. This function  also returns the feather point
     * The first member is the actual point nearby, and the second the counter part (i.e: either the feather point
     * if the first is a control point, or the other way around).
     **/
    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >
    isNearbyControlPoint(double x,double y,double acceptance,int* index) const;
    
    /**
     * @brief Given the control point in parameter, return its index in the curve's control points list.
     * If no such control point could be found, -1 is returned.
     **/
    int getControlPointIndex(const boost::shared_ptr<BezierCP>& cp) const;
    
    /**
     * @brief Given the feather point in parameter, return its index in the curve's feather points list.
     * If no such feather point could be found, -1 is returned.
     **/
    int getFeatherPointIndex(const boost::shared_ptr<BezierCP>& fp) const;
    
    /**
     * @brief Returns the control point at the given index if any, NULL otherwise.
     **/
    boost::shared_ptr<BezierCP> getControlPointAtIndex(int index) const;
    
    /**
     * @brief Returns the feather point at the given index if any, NULL otherwise.
     **/
    boost::shared_ptr<BezierCP> getFeatherPointAtIndex(int index) const;
    
    boost::shared_ptr<BezierCP> getFeatherPointForControlPoint(const boost::shared_ptr<BezierCP>& cp) const;
    
    boost::shared_ptr<BezierCP> getControlPointForFeatherPoint(const boost::shared_ptr<BezierCP>& fp) const;
    
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
    
    static void leftDerivativeAtPoint(int time,const BezierCP& p,const BezierCP& prev,double *dx,double *dy);
    
    static void rightDerivativeAtPoint(int time,const BezierCP& p,const BezierCP& next,double *dx,double *dy);
    
    void save(BezierSerialization* obj) const;
    
    void load(const BezierSerialization& obj);
    
private:
    
    boost::scoped_ptr<BezierPrivate> _imp;
};



/**
 * @class This class is a member of all effects instantiated in the context "paint". It describes internally
 * all the splines data structures and their state.
 * This class is MT-safe.
 **/
class RotoContextSerialization;
struct RotoContextPrivate;
class RotoContext : public QObject
{
    Q_OBJECT
    
public:

    RotoContext(Natron::Node* node);
    
    virtual ~RotoContext();
  
       
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
    
    boost::shared_ptr<Bezier> makeBezier(double x,double y);
    
    void removeBezier(const Bezier* c);
    
    /**
     * @brief Returns a const ref to the bezier curves list. This can only be called from
     * the main thread.
     **/
    const std::list< boost::shared_ptr<Bezier> >& getBeziers() const;
    
    /**
     * @brief Returns a bezier curves nearby the point (x,y) and the parametric value
     * which would be used to find the exact bezier point lying on the curve.
     **/
    boost::shared_ptr<Bezier> isNearbyBezier(double x,double y,double acceptance,int* index,double* t,bool *feather) const;
    
    /**
     * @brief Render the polygon formed by the list of points in the alpha channel of the output image.
     * The output image must be large enough to contain the polygon.
     **/
    void fillPolygon_evenOdd(const RectI& roi,const std::list<std::pair<double,double> >& points,Natron::Image* output);
    
    /**
     * @brief Return the region of definition of all the shapes in the context.
     **/
    void getMaskRegionOfDefinition(int time,int view,RectI* rod) const;
    
    /**
     * @brief Render the mask formed by all the shapes contained in the context within the roi.
     * The image will use the cache if byPassCache is set to true.
     **/
    boost::shared_ptr<Natron::Image> renderMask(const RectI& roi,U64 nodeHash,SequenceTime time,
                                                int view,unsigned int mipmapLevel,bool byPassCache);
    
    /**
     * @brief To be called when a change was made to trigger a new render.
     **/
    void evaluateChange();
    
    void save(RotoContextSerialization* obj) const;
    
    void load(const RotoContextSerialization& obj);
    
public slots:
    
    void onAutoKeyingChanged(bool enabled);
    
    void onFeatherLinkChanged(bool enabled);
    
    void onRippleEditChanged(bool enabled);
    
private:
        
    boost::scoped_ptr<RotoContextPrivate> _imp;
};

#endif // KNOBROTO_H
