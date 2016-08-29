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

#ifndef Engine_RotoLayerSerialization_h
#define Engine_RotoLayerSerialization_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"


#include "Serialization/BezierSerialization.h"
#include "Serialization/CurveSerialization.h"
#include "Serialization/KnobSerialization.h"
#include "Serialization/RotoItemSerialization.h"
#include "Serialization/RotoStrokeItemSerialization.h"

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#define ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER 2
#define ROTO_LAYER_SERIALIZATION_VERSION ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER
#endif

SERIALIZATION_NAMESPACE_ENTER;

class RotoLayerSerialization
    : public RotoItemSerialization
{

public:

    RotoLayerSerialization()
        : RotoItemSerialization()
    {
    }

    virtual ~RotoLayerSerialization()
    {
    }



    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        throw std::runtime_error("Saving with boost is no longer supported");
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoLayerSerialization, RotoItemSerialization>(
            static_cast<RotoLayerSerialization *>(NULL),
            static_cast<RotoItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        int numChildren;
        ar & ::boost::serialization::make_nvp("NumChildren", numChildren);
        for (int i = 0; i < numChildren; ++i) {
            int type;
            if (version < ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER) {
                bool bezier;
                ar & ::boost::serialization::make_nvp("IsBezier", bezier);
                type = bezier ? 0 : 2;
            } else {
                ar & ::boost::serialization::make_nvp("Type", type);
            }
            if (type == 0) {
                boost::shared_ptr<BezierSerialization> b(new BezierSerialization);
                ar & ::boost::serialization::make_nvp("Item", *b);
                children.push_back(b);
            } else if (type == 1) {
                boost::shared_ptr<RotoStrokeItemSerialization> b(new RotoStrokeItemSerialization);
                ar & ::boost::serialization::make_nvp("Item", *b);
                children.push_back(b);
            } else {
                boost::shared_ptr<RotoLayerSerialization> l(new RotoLayerSerialization);
                ar & ::boost::serialization::make_nvp("Item", *l);
                children.push_back(l);
            }
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

    std::list <RotoItemSerializationPtr> children;
};

SERIALIZATION_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::RotoLayerSerialization, ROTO_LAYER_SERIALIZATION_VERSION)
#endif

#endif // Engine_RotoLayerSerialization_h
