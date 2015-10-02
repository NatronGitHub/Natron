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

#ifndef Engine_BezierCPSerialization_h
#define Engine_BezierCPSerialization_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/BezierCP.h"
#include "Engine/BezierCPPrivate.h"

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

#define BEZIER_CP_INTRODUCES_OFFSET 2
#define BEZIER_CP_FIX_BUG_CURVE_POINTER 3
#define BEZIER_CP_VERSION BEZIER_CP_FIX_BUG_CURVE_POINTER

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

#endif // Engine_BezierCPSerialization_h
