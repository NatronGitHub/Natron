/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <map>
#include <list>
#include "Serialization/KnobTableItemSerialization.h"
#include "Serialization/BezierCPSerialization.h"
#include "Serialization/SerializationFwd.h"


SERIALIZATION_NAMESPACE_ENTER

class BezierSerialization
    : public KnobTableItemSerialization
{

public:

    BezierSerialization(bool openedBezier)
        : KnobTableItemSerialization()
        , _isOpenBezier(openedBezier)
    {
        _emitMap = false;
    }

    virtual ~BezierSerialization()
    {
    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;
    

    struct ControlPoint
    {
        BezierCPSerialization innerPoint;

        // Serialize feather only if different
        boost::shared_ptr<BezierCPSerialization> featherPoint;
    };

    struct Shape
    {
        std::list<ControlPoint> controlPoints;
        bool closed;
    };

    typedef std::map<std::string, Shape> PerViewShapeMap;

    PerViewShapeMap _shapes;
    bool _isOpenBezier;
};


SERIALIZATION_NAMESPACE_EXIT

#endif // Engine_BezierSerialization_h
