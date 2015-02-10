//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#ifndef ROTOWRAPPER_H
#define ROTOWRAPPER_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <list>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/ParameterWrapper.h"

class RotoContext;
class RotoItem;
class Bezier;
class RotoLayer;

class Layer;
class ItemBase
{
    
public:
    
    ItemBase(const boost::shared_ptr<RotoItem>& item);
    
    virtual ~ItemBase();
    
    boost::shared_ptr<RotoItem> getInternalItem() const
    {
        return _item;
    }
    
    void setLabel(const std::string & name);
    std::string getLabel() const;
    
    bool setScriptName(const std::string& name);
    std::string getScriptName() const;
    
    void setLocked(bool locked);
    bool getLocked() const;
    bool getLockedRecursive() const;
    
    void setVisible(bool activated);
    bool getVisible() const;
    
    Layer* getParentLayer() const;
    
private:
    
    boost::shared_ptr<RotoItem> _item;
};

class Layer : public ItemBase
{
public:
    
    Layer(const boost::shared_ptr<RotoItem>& item);
    
    virtual ~Layer();
    
    void addItem(ItemBase* item);
    
    void insertItem(int pos, ItemBase* item);
    
    void removeItem(ItemBase* item);
    
    std::list<ItemBase*> getChildren() const;
  
private:
    
    boost::shared_ptr<RotoLayer> _layer;
    
};

class BezierCurve : public ItemBase
{
public:
    
    enum CairoOperatorEnum {
        CAIRO_OPERATOR_CLEAR,
        
        CAIRO_OPERATOR_SOURCE,
        CAIRO_OPERATOR_OVER,
        CAIRO_OPERATOR_IN,
        CAIRO_OPERATOR_OUT,
        CAIRO_OPERATOR_ATOP,
        
        CAIRO_OPERATOR_DEST,
        CAIRO_OPERATOR_DEST_OVER,
        CAIRO_OPERATOR_DEST_IN,
        CAIRO_OPERATOR_DEST_OUT,
        CAIRO_OPERATOR_DEST_ATOP,
        
        CAIRO_OPERATOR_XOR,
        CAIRO_OPERATOR_ADD,
        CAIRO_OPERATOR_SATURATE,
        
        CAIRO_OPERATOR_MULTIPLY,
        CAIRO_OPERATOR_SCREEN,
        CAIRO_OPERATOR_OVERLAY,
        CAIRO_OPERATOR_DARKEN,
        CAIRO_OPERATOR_LIGHTEN,
        CAIRO_OPERATOR_COLOR_DODGE,
        CAIRO_OPERATOR_COLOR_BURN,
        CAIRO_OPERATOR_HARD_LIGHT,
        CAIRO_OPERATOR_SOFT_LIGHT,
        CAIRO_OPERATOR_DIFFERENCE,
        CAIRO_OPERATOR_EXCLUSION,
        CAIRO_OPERATOR_HSL_HUE,
        CAIRO_OPERATOR_HSL_SATURATION,
        CAIRO_OPERATOR_HSL_COLOR,
        CAIRO_OPERATOR_HSL_LUMINOSITY
    };

    
    BezierCurve(const boost::shared_ptr<RotoItem>& item);
    
    virtual ~BezierCurve();
    
    
    void setCurveFinished(bool finished);
    bool isCurveFinished() const;
    
    void addControlPoint(double x, double y);
    
    void addControlPointOnSegment(int index,double t);
    
    void removeControlPointByIndex(int index);
    
    void movePointByIndex(int index,int time,double dx,double dy);
    
    void moveFeatherByIndex(int index,int time,double dx,double dy);
    
    void moveLeftBezierPoint(int index,int time,double dx,double dy);
    
    void moveRightBezierPoint(int index,int time,double dx,double dy);
    
    void setPointAtIndex(int index,int time,double x,double y,double lx,double ly,double rx,double ry);
    
    void setFeatherPointAtIndex(int index,int time,double x,double y,double lx,double ly,double rx,double ry);
    
    void slavePointToTrack(int index, int trackTime,DoubleParam* trackCenter);
    
    DoubleParam* getPointMasterTrack(int index) const;
    
    int getNumControlPoints() const;
    
    void setActivated(int time, bool activated);
    bool getIsActivated(int time);
    
    void setOpacity(double opacity, int time);
    double getOpacity(int time) const;
    
    ColorTuple getOverlayColor() const;
    void setOverlayColor(double r,double g,double b);
    
    double getFeatherDistance(int time) const;
    void setFeatherDistance(double dist,int time);
    
    double getFeatherFallOff(int time) const;
    void setFeatherFallOff(double falloff,int time);
    
    ColorTuple getColor(int time);    
    void setColor(int time,double r,double g, double b);
    
    void setCompositingOperator(CairoOperatorEnum op);
    CairoOperatorEnum getCompositingOperator() const;
    
    BooleanParam* getActivatedParam() const;
    DoubleParam* getOpacityParam() const;
    DoubleParam* getFeatherDistanceParam() const;
    DoubleParam* getFeatherFallOffParam() const;
    ColorParam* getColorParam() const;
    ChoiceParam* getCompositingOperatorParam() const;
    
private:
    
    boost::shared_ptr<Bezier> _bezier;
    
};

class Roto
{
public:
    
    Roto(const boost::shared_ptr<RotoContext>& ctx);
    
    ~Roto();
        
    Layer* getBaseLayer() const;
    
    ItemBase* getItemByName(const std::string& name) const;
    
    Layer* createLayer();
    
    BezierCurve* createBezier(double x,double y, int time);
    
    BezierCurve* createEllipse(double x,double y,double diameter,bool fromCenter,int time);
    
    BezierCurve* createRectangle(double x,double y,double size, int time);
    
private:
    
    boost::shared_ptr<RotoContext> _ctx;
};

#endif // ROTOWRAPPER_H
