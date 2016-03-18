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

#ifndef ROTOWRAPPER_H
#define ROTOWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/PyParameter.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

class Layer; // defined below

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
    
    Param* getParam(const std::string& name) const;
    
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
    
    void movePointByIndex(int index,double time,double dx,double dy);
    
    void moveFeatherByIndex(int index,double time,double dx,double dy);
    
    void moveLeftBezierPoint(int index,double time,double dx,double dy);
    
    void moveRightBezierPoint(int index,double time,double dx,double dy);
    
    void setPointAtIndex(int index,double time,double x,double y,double lx,double ly,double rx,double ry);
    
    void setFeatherPointAtIndex(int index,double time,double x,double y,double lx,double ly,double rx,double ry);
    
    void slavePointToTrack(int index, double trackTime,DoubleParam* trackCenter);
    
    DoubleParam* getPointMasterTrack(int index) const;
    
    int getNumControlPoints() const;
    
    void setActivated(double time, bool activated);
    bool getIsActivated(double time);
    
    void setOpacity(double opacity, double time);
    double getOpacity(double time) const;
    
    ColorTuple getOverlayColor() const;
    void setOverlayColor(double r,double g,double b);
    
    double getFeatherDistance(double time) const;
    void setFeatherDistance(double dist,double time);
    
    double getFeatherFallOff(double time) const;
    void setFeatherFallOff(double falloff,double time);
    
    ColorTuple getColor(double time);
    void setColor(double time,double r,double g, double b);
    
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
    
    BezierCurve* createBezier(double x,double y, double time);
    
    BezierCurve* createEllipse(double x,double y,double diameter,bool fromCenter,double time);
    
    BezierCurve* createRectangle(double x,double y,double size, double time);
    
private:
    
    boost::shared_ptr<RotoContext> _ctx;
};

NATRON_NAMESPACE_EXIT;

#endif // ROTOWRAPPER_H
