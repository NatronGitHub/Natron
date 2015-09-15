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

#ifndef _Engine_RotoStrokeItemSerialization_h_
#define _Engine_RotoStrokeItemSerialization_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoContextPrivate.h"

#define ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES 2
#define ROTO_STROKE_SERIALIZATION_VERSION ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES




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



class RotoStrokeItemSerialization : public RotoDrawableItemSerialization
{
    friend class boost::serialization::access;
    friend class RotoStrokeItem;
    
public:
    
    
    RotoStrokeItemSerialization()
    : RotoDrawableItemSerialization()
    , _brushType()
    , _xCurves()
    , _yCurves()
    , _pressureCurves()
    {
        
    }
    
    virtual ~RotoStrokeItemSerialization()
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
        boost::serialization::void_cast_register<RotoStrokeItemSerialization,RotoDrawableItemSerialization>(
                                                                                                        static_cast<RotoStrokeItemSerialization *>(NULL),
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
        boost::serialization::void_cast_register<RotoStrokeItemSerialization,RotoDrawableItemSerialization>(
                                                                                                        static_cast<RotoStrokeItemSerialization *>(NULL),
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

BOOST_CLASS_VERSION(RotoStrokeItemSerialization, ROTO_STROKE_SERIALIZATION_VERSION)

#endif // _Engine_RotoStrokeItemSerialization_h_
