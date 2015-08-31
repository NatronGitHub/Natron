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

#ifndef ROTOSERIALIZATION_H
#define ROTOSERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/RotoContext.h"
#include "Engine/RotoContextPrivate.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/map.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/AppManager.h"
#include "Engine/CurveSerialization.h"
#include "Engine/KnobSerialization.h"

#define ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING 2
#define ROTO_DRAWABLE_ITEM_REMOVES_INVERTED 3
#define ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST 4
#define ROTO_DRAWABLE_ITEM_VERSION ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST

#define BEZIER_CP_INTRODUCES_OFFSET 2
#define BEZIER_CP_FIX_BUG_CURVE_POINTER 3
#define BEZIER_CP_VERSION BEZIER_CP_FIX_BUG_CURVE_POINTER

#define ROTO_ITEM_INTRODUCES_LABEL 2
#define ROTO_ITEM_VERSION ROTO_ITEM_INTRODUCES_LABEL

#define ROTO_CTX_REMOVE_COUNTERS 2
#define ROTO_CTX_VERSION ROTO_CTX_REMOVE_COUNTERS

#define ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER 2
#define ROTO_LAYER_SERIALIZATION_VERSION ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER

#define BEZIER_SERIALIZATION_INTRODUCES_ROTO_STROKE 2
#define BEZIER_SERIALIZATION_REMOVES_IS_ROTO_STROKE 3
#define BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER 4
#define BEZIER_SERIALIZATION_VERSION BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER

#define ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES 2
#define ROTO_STROKE_SERIALIZATION_VERSION ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES

//BOOST_SERIALIZATION_SPLIT_MEMBER()
// split member function serialize funcition into save/load
template<class Archive>
void
BezierCP::serialize(Archive &ar,
                   const unsigned int file_version)
{
    boost::serialization::split_member(ar, *this, file_version);
}

template<class Archive>
void
BezierCP::save(Archive & ar,
                    const unsigned int version) const
{
    Q_UNUSED(version);
    ar & boost::serialization::make_nvp("X",_imp->x);
    ar & boost::serialization::make_nvp("X_animation",*_imp->curveX);
    ar & boost::serialization::make_nvp("Y",_imp->y);
    ar & boost::serialization::make_nvp("Y_animation",*_imp->curveY);
    ar & boost::serialization::make_nvp("Left_X",_imp->leftX);
    ar & boost::serialization::make_nvp("Left_X_animation",*_imp->curveLeftBezierX);
    ar & boost::serialization::make_nvp("Left_Y",_imp->leftY);
    ar & boost::serialization::make_nvp("Left_Y_animation",*_imp->curveLeftBezierY);
    ar & boost::serialization::make_nvp("Right_X",_imp->rightX);
    ar & boost::serialization::make_nvp("Right_X_animation",*_imp->curveRightBezierX);
    ar & boost::serialization::make_nvp("Right_Y",_imp->rightY);
    ar & boost::serialization::make_nvp("Right_Y_animation",*_imp->curveRightBezierY);
    if (version >= BEZIER_CP_INTRODUCES_OFFSET) {
        QWriteLocker l(&_imp->masterMutex);
        ar & boost::serialization::make_nvp("OffsetTime",_imp->offsetTime);
    }
}

template<class Archive>
void
BezierCP::load(Archive & ar,
               const unsigned int version) 
{
    bool createdDuringToRC2Or3 = appPTR->wasProjectCreatedDuringRC2Or3();
    if (version >= BEZIER_CP_FIX_BUG_CURVE_POINTER || !createdDuringToRC2Or3) {
    
        ar & boost::serialization::make_nvp("X",_imp->x);
        
        Curve xCurve;
        ar & boost::serialization::make_nvp("X_animation",xCurve);
        _imp->curveX->clone(xCurve);
        
        if (version < BEZIER_CP_FIX_BUG_CURVE_POINTER) {
            Curve curveBug;
            ar & boost::serialization::make_nvp("Y",curveBug);
        } else {
            ar & boost::serialization::make_nvp("Y",_imp->y);
        }
        
        Curve yCurve;
        ar & boost::serialization::make_nvp("Y_animation",yCurve);
        _imp->curveY->clone(yCurve);
        
        ar & boost::serialization::make_nvp("Left_X",_imp->leftX);
        
        Curve leftCurveX,leftCurveY,rightCurveX,rightCurveY;
        
        ar & boost::serialization::make_nvp("Left_X_animation",leftCurveX);
        ar & boost::serialization::make_nvp("Left_Y",_imp->leftY);
        ar & boost::serialization::make_nvp("Left_Y_animation",leftCurveY);
        ar & boost::serialization::make_nvp("Right_X",_imp->rightX);
        ar & boost::serialization::make_nvp("Right_X_animation",rightCurveX);
        ar & boost::serialization::make_nvp("Right_Y",_imp->rightY);
        ar & boost::serialization::make_nvp("Right_Y_animation",rightCurveY);
        
        _imp->curveLeftBezierX->clone(leftCurveX);
        _imp->curveLeftBezierY->clone(leftCurveY);
        _imp->curveRightBezierX->clone(rightCurveX);
        _imp->curveRightBezierY->clone(rightCurveY);
        
    } else {
        ar & boost::serialization::make_nvp("X",_imp->x);
        
        boost::shared_ptr<Curve> xCurve,yCurve,leftCurveX,leftCurveY,rightCurveX,rightCurveY;
        ar & boost::serialization::make_nvp("X_animation",xCurve);
        _imp->curveX->clone(*xCurve);
        
        boost::shared_ptr<Curve> curveBug;
        ar & boost::serialization::make_nvp("Y",curveBug);
     
        
        ar & boost::serialization::make_nvp("Y_animation",yCurve);
        _imp->curveY->clone(*yCurve);
        
        ar & boost::serialization::make_nvp("Left_X",_imp->leftX);
        
        
        ar & boost::serialization::make_nvp("Left_X_animation",leftCurveX);
        ar & boost::serialization::make_nvp("Left_Y",_imp->leftY);
        ar & boost::serialization::make_nvp("Left_Y_animation",leftCurveY);
        ar & boost::serialization::make_nvp("Right_X",_imp->rightX);
        ar & boost::serialization::make_nvp("Right_X_animation",rightCurveX);
        ar & boost::serialization::make_nvp("Right_Y",_imp->rightY);
        ar & boost::serialization::make_nvp("Right_Y_animation",rightCurveY);
        
        _imp->curveLeftBezierX->clone(*leftCurveX);
        _imp->curveLeftBezierY->clone(*leftCurveY);
        _imp->curveRightBezierX->clone(*rightCurveX);
        _imp->curveRightBezierY->clone(*rightCurveY);
    }
    if (version >= BEZIER_CP_INTRODUCES_OFFSET) {
        QWriteLocker l(&_imp->masterMutex);
        ar & boost::serialization::make_nvp("OffsetTime",_imp->offsetTime);
    }
}

BOOST_CLASS_VERSION(BezierCP,BEZIER_CP_VERSION)

class RotoItemSerialization
{
    friend class boost::serialization::access;
    friend class RotoItem;

public:

    RotoItemSerialization()
    : name()
    , activated(false)
    , parentLayerName()
    , locked(false)
    {
    }

    virtual ~RotoItemSerialization()
    {
    }

private:


    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        Q_UNUSED(version);
        ar & boost::serialization::make_nvp("Name",name);
        ar & boost::serialization::make_nvp("Label",label);
        ar & boost::serialization::make_nvp("Activated",activated);
        ar & boost::serialization::make_nvp("ParentLayer",parentLayerName);
        ar & boost::serialization::make_nvp("Locked",locked);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        ar & boost::serialization::make_nvp("Name",name);
        if ( version >= ROTO_ITEM_INTRODUCES_LABEL) {
            ar & boost::serialization::make_nvp("Label",label);
        }
        ar & boost::serialization::make_nvp("Activated",activated);
        ar & boost::serialization::make_nvp("ParentLayer",parentLayerName);
        ar & boost::serialization::make_nvp("Locked",locked);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::string name,label;
    bool activated;
    std::string parentLayerName;
    bool locked;
};

BOOST_CLASS_VERSION(RotoItemSerialization,ROTO_ITEM_VERSION)


//BOOST_SERIALIZATION_ASSUME_ABSTRACT(RotoItemSerialization);

class RotoDrawableItemSerialization
    : public RotoItemSerialization
{
    friend class boost::serialization::access;
    friend class RotoDrawableItem;

public:

    RotoDrawableItemSerialization()
        : RotoItemSerialization()
    {
    }

    virtual ~RotoDrawableItemSerialization()
    {
    }

private:


    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoDrawableItemSerialization,RotoItemSerialization>(
            static_cast<RotoDrawableItemSerialization *>(NULL),
            static_cast<RotoItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        int nKnobs = _knobs.size();
        ar & boost::serialization::make_nvp("NbItems",nKnobs);
        for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
            ar & boost::serialization::make_nvp("Item",**it);
        }
        
        ar & boost::serialization::make_nvp("OC.r",_overlayColor[0]);
        ar & boost::serialization::make_nvp("OC.g",_overlayColor[1]);
        ar & boost::serialization::make_nvp("OC.b",_overlayColor[2]);
        ar & boost::serialization::make_nvp("OC.a",_overlayColor[3]);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoDrawableItemSerialization,RotoItemSerialization>(
                                                                                                      static_cast<RotoDrawableItemSerialization *>(NULL),
                                                                                                      static_cast<RotoItemSerialization *>(NULL)
                                                                                                      );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        if (version < ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST) {
            
            boost::shared_ptr<KnobSerialization> activated(new KnobSerialization);
            ar & boost::serialization::make_nvp("Activated",*activated);
            _knobs.push_back(activated);
            boost::shared_ptr<KnobSerialization> opacity(new KnobSerialization);
            ar & boost::serialization::make_nvp("Opacity",*opacity);
            _knobs.push_back(opacity);
            boost::shared_ptr<KnobSerialization> feather(new KnobSerialization);
            ar & boost::serialization::make_nvp("Feather",*feather);
            _knobs.push_back(feather);
            boost::shared_ptr<KnobSerialization> falloff(new KnobSerialization);
            ar & boost::serialization::make_nvp("FallOff",*falloff);
            _knobs.push_back(falloff);
            if (version < ROTO_DRAWABLE_ITEM_REMOVES_INVERTED) {
                boost::shared_ptr<KnobSerialization> inverted(new KnobSerialization);
                ar & boost::serialization::make_nvp("Inverted",*inverted);
                _knobs.push_back(inverted);
            }
            if (version >= ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING) {
                boost::shared_ptr<KnobSerialization> color(new KnobSerialization);
                ar & boost::serialization::make_nvp("Color",*color);
                _knobs.push_back(color);
                boost::shared_ptr<KnobSerialization> comp(new KnobSerialization);
                ar & boost::serialization::make_nvp("CompOP",*comp);
                _knobs.push_back(comp);
            }
        } else {
            int nKnobs;
            ar & boost::serialization::make_nvp("NbItems",nKnobs);
            for (int i = 0; i < nKnobs; ++i) {
                boost::shared_ptr<KnobSerialization> k(new KnobSerialization);
                ar & boost::serialization::make_nvp("Item",*k);
                _knobs.push_back(k);
            }
        }
        ar & boost::serialization::make_nvp("OC.r",_overlayColor[0]);
        ar & boost::serialization::make_nvp("OC.g",_overlayColor[1]);
        ar & boost::serialization::make_nvp("OC.b",_overlayColor[2]);
        ar & boost::serialization::make_nvp("OC.a",_overlayColor[3]);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::list<boost::shared_ptr<KnobSerialization> > _knobs;
    double _overlayColor[4];
};

BOOST_CLASS_VERSION(RotoDrawableItemSerialization,ROTO_DRAWABLE_ITEM_VERSION)

BOOST_SERIALIZATION_ASSUME_ABSTRACT(RotoDrawableItemSerialization);


struct StrokePoint {
    
    double x,y,pressure;
    
    template<class Archive>
    void serialize(Archive & ar,
              const unsigned int /*version*/)
    {
        ar & boost::serialization::make_nvp("X",x);
        ar & boost::serialization::make_nvp("Y",y);
        ar & boost::serialization::make_nvp("Press",pressure);
    }
};


class BezierSerialization
    : public RotoDrawableItemSerialization
{
    friend class boost::serialization::access;
    friend class Bezier;
    
public:
    
    BezierSerialization()
    : RotoDrawableItemSerialization()
    , _controlPoints()
    , _featherPoints()
    , _closed(false)
    , _isOpenBezier(false)
    {
    }

    virtual ~BezierSerialization()
    {
    }

private:


    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<BezierSerialization,RotoDrawableItemSerialization>(
            static_cast<BezierSerialization *>(NULL),
            static_cast<RotoDrawableItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);
        int numPoints = (int)_controlPoints.size();
        ar & boost::serialization::make_nvp("NumPoints",numPoints);
        std::list< BezierCP >::const_iterator itF = _featherPoints.begin();
        for (std::list< BezierCP >::const_iterator it = _controlPoints.begin(); it != _controlPoints.end(); ++it) {
            ar & boost::serialization::make_nvp("CP",*it);
            ar & boost::serialization::make_nvp("FP",*itF);
            ++itF;
            
        }
        ar & boost::serialization::make_nvp("Closed",_closed);
        ar & boost::serialization::make_nvp("OpenBezier",_isOpenBezier);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<BezierSerialization,RotoDrawableItemSerialization>(
            static_cast<BezierSerialization *>(NULL),
            static_cast<RotoDrawableItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);
        int numPoints;
        ar & boost::serialization::make_nvp("NumPoints",numPoints);
        if (version >= BEZIER_SERIALIZATION_INTRODUCES_ROTO_STROKE && version < BEZIER_SERIALIZATION_REMOVES_IS_ROTO_STROKE) {
            bool isStroke;
            ar & boost::serialization::make_nvp("IsStroke",isStroke);
        }
        for (int i = 0; i < numPoints; ++i) {
            BezierCP cp;
            ar & boost::serialization::make_nvp("CP",cp);
            _controlPoints.push_back(cp);
            
            BezierCP fp;
            ar & boost::serialization::make_nvp("FP",fp);
            _featherPoints.push_back(fp);
            
        }
        ar & boost::serialization::make_nvp("Closed",_closed);
        if (version >= BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER) {
            ar & boost::serialization::make_nvp("OpenBezier",_isOpenBezier);
        } else {
            _isOpenBezier = false;
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::list< BezierCP > _controlPoints,_featherPoints;
    bool _closed;
    bool _isOpenBezier;
};

BOOST_CLASS_VERSION(BezierSerialization,BEZIER_SERIALIZATION_VERSION)

class RotoStrokeSerialization : public RotoDrawableItemSerialization
{
    friend class boost::serialization::access;
    friend class RotoStrokeItem;
    
public:
    
    
    RotoStrokeSerialization()
    : RotoDrawableItemSerialization()
    , _brushType()
    , _xCurves()
    , _yCurves()
    , _pressureCurves()
    {
        
    }
    
    virtual ~RotoStrokeSerialization()
    {
        
    }
    
    int getType() const
    {
        return _brushType;
    }
    
private:
    
    
    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoStrokeSerialization,RotoDrawableItemSerialization>(
                                                                                                        static_cast<RotoStrokeSerialization *>(NULL),
                                                                                                        static_cast<RotoDrawableItemSerialization *>(NULL)
                                                                                                        );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);

        assert(_xCurves.size() == _yCurves.size() && _xCurves.size() == _pressureCurves.size());
        std::list<boost::shared_ptr<Curve> >::const_iterator itY = _yCurves.begin();
        std::list<boost::shared_ptr<Curve> >::const_iterator itP = _pressureCurves.begin();
        int nb = (int)_xCurves.size();
        ar & boost::serialization::make_nvp("BrushType",_brushType);
        ar & boost::serialization::make_nvp("NbItems",nb);
        for (std::list<boost::shared_ptr<Curve> >::const_iterator it = _xCurves.begin(); it!= _xCurves.end(); ++it, ++itY, ++itP) {
            ar & boost::serialization::make_nvp("CurveX",**it);
            ar & boost::serialization::make_nvp("CurveY",**itY);
            ar & boost::serialization::make_nvp("CurveP",**itP);
        }
        
        
    }
    
    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoStrokeSerialization,RotoDrawableItemSerialization>(
                                                                                                        static_cast<RotoStrokeSerialization *>(NULL),
                                                                                                        static_cast<RotoDrawableItemSerialization *>(NULL)
                                                                                                        );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);
        if (version < ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES) {
            ar & boost::serialization::make_nvp("BrushType",_brushType);
            
            boost::shared_ptr<Curve> x(new Curve),y(new Curve),p(new Curve);
            ar & boost::serialization::make_nvp("CurveX",*x);
            ar & boost::serialization::make_nvp("CurveY",*y);
            ar & boost::serialization::make_nvp("CurveP",*p);
            _xCurves.push_back(x);
            _yCurves.push_back(y);
            _pressureCurves.push_back(p);
            
        } else {
            int nb;
            ar & boost::serialization::make_nvp("BrushType",_brushType);
            ar & boost::serialization::make_nvp("NbItems",nb);
            for (int i = 0; i < nb ;++i) {
                boost::shared_ptr<Curve> x(new Curve),y(new Curve),p(new Curve);
                ar & boost::serialization::make_nvp("CurveX",*x);
                ar & boost::serialization::make_nvp("CurveY",*y);
                ar & boost::serialization::make_nvp("CurveP",*p);
                _xCurves.push_back(x);
                _yCurves.push_back(y);
                _pressureCurves.push_back(p);

            }
        }
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    
    int _brushType;
    std::list<boost::shared_ptr<Curve> > _xCurves,_yCurves,_pressureCurves;
};

BOOST_CLASS_VERSION(RotoStrokeSerialization, ROTO_STROKE_SERIALIZATION_VERSION)

class RotoLayerSerialization
    : public RotoItemSerialization
{
    friend class boost::serialization::access;
    friend class RotoLayer;

public:

    RotoLayerSerialization()
        : RotoItemSerialization()
    {
    }

    virtual ~RotoLayerSerialization()
    {
    }

private:


    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        Q_UNUSED(version);

        boost::serialization::void_cast_register<RotoLayerSerialization,RotoItemSerialization>(
            static_cast<RotoLayerSerialization *>(NULL),
            static_cast<RotoItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        int numChildren = (int)children.size();
        ar & boost::serialization::make_nvp("NumChildren",numChildren);
        for (std::list < boost::shared_ptr<RotoItemSerialization> >::const_iterator it = children.begin(); it != children.end(); ++it) {
            BezierSerialization* isBezier = dynamic_cast<BezierSerialization*>( it->get() );
            RotoStrokeSerialization* isStroke = dynamic_cast<RotoStrokeSerialization*>( it->get() );
            RotoLayerSerialization* isLayer = dynamic_cast<RotoLayerSerialization*>( it->get() );
            int type = 0;
            if (isBezier && !isStroke) {
                type = 0;
            } else if (isStroke) {
                type = 1;
            } else if (isLayer) {
                type = 2;
            }
            ar & boost::serialization::make_nvp("Type",type);
            if (isBezier && !isStroke) {
                ar & boost::serialization::make_nvp("Item",*isBezier);
            } else if (isStroke) {
                ar & boost::serialization::make_nvp("Item",*isStroke);
            } else {
                assert(isLayer);
                ar & boost::serialization::make_nvp("Item",*isLayer);
            }
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoLayerSerialization,RotoItemSerialization>(
            static_cast<RotoLayerSerialization *>(NULL),
            static_cast<RotoItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        int numChildren;
        ar & boost::serialization::make_nvp("NumChildren",numChildren);
        for (int i = 0; i < numChildren; ++i) {
            
            int type;
            if (version < ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER) {
                bool bezier;
                ar & boost::serialization::make_nvp("IsBezier",bezier);
                type = bezier ? 0 : 2;
            } else {
                ar & boost::serialization::make_nvp("Type",type);
            }
            if (type == 0) {
                boost::shared_ptr<BezierSerialization> b(new BezierSerialization);
                ar & boost::serialization::make_nvp("Item",*b);
                children.push_back(b);
            } else if (type == 1) {
                boost::shared_ptr<RotoStrokeSerialization> b(new RotoStrokeSerialization);
                ar & boost::serialization::make_nvp("Item",*b);
                children.push_back(b);
            } else {
                boost::shared_ptr<RotoLayerSerialization> l(new RotoLayerSerialization);
                ar & boost::serialization::make_nvp("Item",*l);
                children.push_back(l);
            }
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::list < boost::shared_ptr<RotoItemSerialization> > children;
};

BOOST_CLASS_VERSION(RotoLayerSerialization,ROTO_LAYER_SERIALIZATION_VERSION)

class RotoContextSerialization
{
    friend class boost::serialization::access;
    friend class RotoContext;

public:

    RotoContextSerialization()
        : _baseLayer()
        , _selectedItems()
        , _autoKeying(false)
        , _rippleEdit(false)
        , _featherLink(false)
    {
    }

    ~RotoContextSerialization()
    {
    }

private:


    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        Q_UNUSED(version);

        ar & boost::serialization::make_nvp("BaseLayer",_baseLayer);
        ar & boost::serialization::make_nvp("AutoKeying",_autoKeying);
        ar & boost::serialization::make_nvp("RippleEdit",_rippleEdit);
        ar & boost::serialization::make_nvp("FeatherLink",_featherLink);
        ar & boost::serialization::make_nvp("Selection",_selectedItems);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {

        ar & boost::serialization::make_nvp("BaseLayer",_baseLayer);
        ar & boost::serialization::make_nvp("AutoKeying",_autoKeying);
        ar & boost::serialization::make_nvp("RippleEdit",_rippleEdit);
        ar & boost::serialization::make_nvp("FeatherLink",_featherLink);
        ar & boost::serialization::make_nvp("Selection",_selectedItems);
        if (version < ROTO_CTX_REMOVE_COUNTERS) {
            std::map<std::string,int> _itemCounters;
            ar & boost::serialization::make_nvp("Counters",_itemCounters);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    RotoLayerSerialization _baseLayer;
    std::list< std::string > _selectedItems;
    bool _autoKeying;
    bool _rippleEdit;
    bool _featherLink;
};

BOOST_CLASS_VERSION(RotoContextSerialization,ROTO_CTX_VERSION)


#endif // ROTOSERIALIZATION_H
