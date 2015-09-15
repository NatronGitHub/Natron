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

#ifndef _Engine_BezierSerialization_h_
#define _Engine_BezierSerialization_h_

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
#include "Engine/RotoDrawableItemSerialization.h"

#define BEZIER_SERIALIZATION_INTRODUCES_ROTO_STROKE 2
#define BEZIER_SERIALIZATION_REMOVES_IS_ROTO_STROKE 3
#define BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER 4
#define BEZIER_SERIALIZATION_VERSION BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER


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


#endif // _Engine_BezierSerialization_h_
