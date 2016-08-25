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

#ifndef Engine_BezierSerialization_h
#define Engine_BezierSerialization_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"



#include "Serialization/RotoDrawableItemSerialization.h"
#include "Serialization/BezierCPSerialization.h"

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#define BEZIER_SERIALIZATION_INTRODUCES_ROTO_STROKE 2
#define BEZIER_SERIALIZATION_REMOVES_IS_ROTO_STROKE 3
#define BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER 4
#define BEZIER_SERIALIZATION_VERSION BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER
#endif

NATRON_NAMESPACE_ENTER;

class BezierSerialization
    : public RotoDrawableItemSerialization
{

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


#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<BezierSerialization, RotoDrawableItemSerialization>(
            static_cast<BezierSerialization *>(NULL),
            static_cast<RotoDrawableItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);
        int numPoints;
        ar & ::boost::serialization::make_nvp("NumPoints", numPoints);
        if ( (version >= BEZIER_SERIALIZATION_INTRODUCES_ROTO_STROKE) && (version < BEZIER_SERIALIZATION_REMOVES_IS_ROTO_STROKE) ) {
            bool isStroke;
            ar & ::boost::serialization::make_nvp("IsStroke", isStroke);
        }
        for (int i = 0; i < numPoints; ++i) {
            BezierCP cp;
            ar & ::boost::serialization::make_nvp("CP", cp);
            _controlPoints.push_back(cp);

            BezierCP fp;
            ar & ::boost::serialization::make_nvp("FP", fp);
            _featherPoints.push_back(fp);
        }
        ar & ::boost::serialization::make_nvp("Closed", _closed);
        if (version >= BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER) {
            ar & ::boost::serialization::make_nvp("OpenBezier", _isOpenBezier);
        } else {
            _isOpenBezier = false;
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

    std::list< BezierCP > _controlPoints, _featherPoints;
    bool _closed;
    bool _isOpenBezier;
};


NATRON_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(NATRON_NAMESPACE::BezierSerialization, BEZIER_SERIALIZATION_VERSION)
#endif

#endif // Engine_BezierSerialization_h
