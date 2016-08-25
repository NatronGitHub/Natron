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

#ifndef Engine_RotoStrokeItemSerialization_h
#define Engine_RotoStrokeItemSerialization_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "Serialization/CurveSerialization.h"
#include "Serialization/KnobSerialization.h"
#include "Serialization/RotoDrawableItemSerialization.h"

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#define ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES 2
#define ROTO_STROKE_SERIALIZATION_VERSION ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES
#endif

NATRON_NAMESPACE_ENTER;
struct StrokePoint
{
    double x, y, pressure;

    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("X", x);
        ar & ::boost::serialization::make_nvp("Y", y);
        ar & ::boost::serialization::make_nvp("Press", pressure);
    }
};


class RotoStrokeItemSerialization
    : public RotoDrawableItemSerialization
{

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

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoStrokeItemSerialization, RotoDrawableItemSerialization>(
            static_cast<RotoStrokeItemSerialization *>(NULL),
            static_cast<RotoDrawableItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);
        if (version < ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES) {
            ar & ::boost::serialization::make_nvp("BrushType", _brushType);
            CurvePtr x(new Curve), y(new Curve), p(new Curve);
            ar & ::boost::serialization::make_nvp("CurveX", *x);
            ar & ::boost::serialization::make_nvp("CurveY", *y);
            ar & ::boost::serialization::make_nvp("CurveP", *p);
            _xCurves.push_back(x);
            _yCurves.push_back(y);
            _pressureCurves.push_back(p);
        } else {
            int nb;
            ar & ::boost::serialization::make_nvp("BrushType", _brushType);
            ar & ::boost::serialization::make_nvp("NbItems", nb);
            for (int i = 0; i < nb; ++i) {
                CurvePtr x(new Curve), y(new Curve), p(new Curve);
                ar & ::boost::serialization::make_nvp("CurveX", *x);
                ar & ::boost::serialization::make_nvp("CurveY", *y);
                ar & ::boost::serialization::make_nvp("CurveP", *p);
                _xCurves.push_back(x);
                _yCurves.push_back(y);
                _pressureCurves.push_back(p);
            }
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

    int _brushType;
    std::list<CurvePtr > _xCurves, _yCurves, _pressureCurves;
};

NATRON_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(NATRON_NAMESPACE::RotoStrokeItemSerialization, ROTO_STROKE_SERIALIZATION_VERSION)
#endif

#endif // Engine_RotoStrokeItemSerialization_h
