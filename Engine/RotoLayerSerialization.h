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

#ifndef Engine_RotoLayerSerialization_h
#define Engine_RotoLayerSerialization_h

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
#include "Engine/BezierSerialization.h"
#include "Engine/CurveSerialization.h"
#include "Engine/KnobSerialization.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoContextPrivate.h"
#include "Engine/RotoItemSerialization.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItemSerialization.h"

#define ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER 2
#define ROTO_LAYER_SERIALIZATION_VERSION ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER


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
            RotoStrokeItemSerialization* isStroke = dynamic_cast<RotoStrokeItemSerialization*>( it->get() );
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
                boost::shared_ptr<RotoStrokeItemSerialization> b(new RotoStrokeItemSerialization);
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

#endif // Engine_RotoLayerSerialization_h
