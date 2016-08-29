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

#ifndef Engine_BezierCPSerialization_h
#define Engine_BezierCPSerialization_h


#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#include "Engine/BezierCP.h"
#include "Engine/BezierCPPrivate.h"
#endif

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#define BEZIER_CP_INTRODUCES_OFFSET 2
#define BEZIER_CP_FIX_BUG_CURVE_POINTER 3
#define BEZIER_CP_REMOVE_OFFSET 4
#define BEZIER_CP_VERSION BEZIER_CP_REMOVE_OFFSET
#endif

#include "Serialization/CurveSerialization.h"

SERIALIZATION_NAMESPACE_ENTER;

class BezierCPSerialization
: public SerializationObjectBase
{
public:

    // Animation of the point
    CurveSerialization xCurve, yCurve, leftCurveX, rightCurveX, leftCurveY, rightCurveY;

    // If the point is not animated, this is its static value
    double x,y,leftX,rightX,leftY,rightY;


    BezierCPSerialization()
    : xCurve()
    , yCurve()
    , leftCurveX()
    , rightCurveX()
    , leftCurveY()
    , rightCurveY()
    , x(0)
    , y(0)
    , leftX(0)
    , rightX(0)
    , leftY(0)
    , rightY(0)
    {

    }

    virtual ~BezierCPSerialization()
    {

    }

    virtual void encode(YAML_NAMESPACE::Emitter& em) const OVERRIDE FINAL;

    virtual void decode(const YAML_NAMESPACE::Node& node) OVERRIDE FINAL;
};


#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
SERIALIZATION_NAMESPACE_EXIT
// Everything below is deprecated and maintained for projects prior to Natron 2.2
template<class Archive>
void
NATRON_NAMESPACE::BezierCP::serialize(Archive &ar,
                    const unsigned int file_version)
{
    boost::serialization::split_member(ar, *this, file_version);
}

template<class Archive>
void
NATRON_NAMESPACE::BezierCP::load(Archive & ar,
               const unsigned int version)
{
    bool createdDuringToRC2Or3 = appPTR->wasProjectCreatedDuringRC2Or3();

    if ( (version >= BEZIER_CP_FIX_BUG_CURVE_POINTER) || !createdDuringToRC2Or3 ) {
        ar & ::boost::serialization::make_nvp("X", _imp->x);
        NATRON_NAMESPACE::Curve xCurve;
        ar & ::boost::serialization::make_nvp("X_animation", xCurve);
        _imp->curveX->clone(xCurve);

        if (version < BEZIER_CP_FIX_BUG_CURVE_POINTER) {
            NATRON_NAMESPACE::Curve curveBug;
            ar & ::boost::serialization::make_nvp("Y", curveBug);
        } else {
            ar & ::boost::serialization::make_nvp("Y", _imp->y);
        }

        NATRON_NAMESPACE::Curve yCurve;
        ar & ::boost::serialization::make_nvp("Y_animation", yCurve);
        _imp->curveY->clone(yCurve);

        ar & ::boost::serialization::make_nvp("Left_X", _imp->leftX);
        NATRON_NAMESPACE::Curve leftCurveX, leftCurveY, rightCurveX, rightCurveY;
        ar & ::boost::serialization::make_nvp("Left_X_animation", leftCurveX);
        ar & ::boost::serialization::make_nvp("Left_Y", _imp->leftY);
        ar & ::boost::serialization::make_nvp("Left_Y_animation", leftCurveY);
        ar & ::boost::serialization::make_nvp("Right_X", _imp->rightX);
        ar & ::boost::serialization::make_nvp("Right_X_animation", rightCurveX);
        ar & ::boost::serialization::make_nvp("Right_Y", _imp->rightY);
        ar & ::boost::serialization::make_nvp("Right_Y_animation", rightCurveY);

        _imp->curveLeftBezierX->clone(leftCurveX);
        _imp->curveLeftBezierY->clone(leftCurveY);
        _imp->curveRightBezierX->clone(rightCurveX);
        _imp->curveRightBezierY->clone(rightCurveY);
    } else {
        ar & ::boost::serialization::make_nvp("X", _imp->x);
        NATRON_NAMESPACE::CurvePtr xCurve, yCurve, leftCurveX, leftCurveY, rightCurveX, rightCurveY;
        ar & ::boost::serialization::make_nvp("X_animation", xCurve);
        _imp->curveX->clone(*xCurve);

        NATRON_NAMESPACE::CurvePtr curveBug;
        ar & ::boost::serialization::make_nvp("Y", curveBug);
        ar & ::boost::serialization::make_nvp("Y_animation", yCurve);
        _imp->curveY->clone(*yCurve);

        ar & ::boost::serialization::make_nvp("Left_X", _imp->leftX);
        ar & ::boost::serialization::make_nvp("Left_X_animation", leftCurveX);
        ar & ::boost::serialization::make_nvp("Left_Y", _imp->leftY);
        ar & ::boost::serialization::make_nvp("Left_Y_animation", leftCurveY);
        ar & ::boost::serialization::make_nvp("Right_X", _imp->rightX);
        ar & ::boost::serialization::make_nvp("Right_X_animation", rightCurveX);
        ar & ::boost::serialization::make_nvp("Right_Y", _imp->rightY);
        ar & ::boost::serialization::make_nvp("Right_Y_animation", rightCurveY);

        _imp->curveLeftBezierX->clone(*leftCurveX);
        _imp->curveLeftBezierY->clone(*leftCurveY);
        _imp->curveRightBezierX->clone(*rightCurveX);
        _imp->curveRightBezierY->clone(*rightCurveY);
    }
    if ( (version >= BEZIER_CP_INTRODUCES_OFFSET) && (version < BEZIER_CP_REMOVE_OFFSET) ) {
        int offsetTime;
        ar & ::boost::serialization::make_nvp("OffsetTime", offsetTime);
    }
} // BezierCP::load

SERIALIZATION_NAMESPACE_ENTER
#endif // NATRON_BOOST_SERIALIZATION_COMPAT

SERIALIZATION_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(NATRON_NAMESPACE::BezierCP, BEZIER_CP_VERSION)
#endif


#endif // Engine_BezierCPSerialization_h
