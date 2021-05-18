/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_RotoDrawableItemSerialization_h
#define Engine_RotoDrawableItemSerialization_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/archive/basic_archive.hpp>
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
#include "Engine/RotoItemSerialization.h"
#include "Engine/EngineFwd.h"


#define ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING 2
#define ROTO_DRAWABLE_ITEM_REMOVES_INVERTED 3
#define ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST 4
#define ROTO_DRAWABLE_ITEM_VERSION ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST

NATRON_NAMESPACE_ENTER

class RotoDrawableItemSerialization
    : public RotoItemSerialization
{
    friend class ::boost::serialization::access;
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
        boost::serialization::void_cast_register<RotoDrawableItemSerialization, RotoItemSerialization>(
            static_cast<RotoDrawableItemSerialization *>(NULL),
            static_cast<RotoItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        int nKnobs = _knobs.size();
        ar & ::boost::serialization::make_nvp("NbItems", nKnobs);
        for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
            ar & ::boost::serialization::make_nvp("Item", **it);
        }

        ar & ::boost::serialization::make_nvp("OC.r", _overlayColor[0]);
        ar & ::boost::serialization::make_nvp("OC.g", _overlayColor[1]);
        ar & ::boost::serialization::make_nvp("OC.b", _overlayColor[2]);
        ar & ::boost::serialization::make_nvp("OC.a", _overlayColor[3]);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoDrawableItemSerialization, RotoItemSerialization>(
            static_cast<RotoDrawableItemSerialization *>(NULL),
            static_cast<RotoItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        if (version < ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST) {
            KnobSerializationPtr activated = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("Activated", *activated);
            _knobs.push_back(activated);
            KnobSerializationPtr opacity = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("Opacity", *opacity);
            _knobs.push_back(opacity);
            KnobSerializationPtr feather = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("Feather", *feather);
            _knobs.push_back(feather);
            KnobSerializationPtr falloff = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("FallOff", *falloff);
            _knobs.push_back(falloff);
            if (version < ROTO_DRAWABLE_ITEM_REMOVES_INVERTED) {
                KnobSerializationPtr inverted = boost::make_shared<KnobSerialization>();
                ar & ::boost::serialization::make_nvp("Inverted", *inverted);
                _knobs.push_back(inverted);
            }
            if (version >= ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING) {
                KnobSerializationPtr color = boost::make_shared<KnobSerialization>();
                ar & ::boost::serialization::make_nvp("Color", *color);
                _knobs.push_back(color);
                KnobSerializationPtr comp = boost::make_shared<KnobSerialization>();
                ar & ::boost::serialization::make_nvp("CompOP", *comp);
                _knobs.push_back(comp);
            }
        } else {
            int nKnobs;
            ar & ::boost::serialization::make_nvp("NbItems", nKnobs);
            for (int i = 0; i < nKnobs; ++i) {
                KnobSerializationPtr k = boost::make_shared<KnobSerialization>();
                ar & ::boost::serialization::make_nvp("Item", *k);
                _knobs.push_back(k);
            }
        }
        ar & ::boost::serialization::make_nvp("OC.r", _overlayColor[0]);
        ar & ::boost::serialization::make_nvp("OC.g", _overlayColor[1]);
        ar & ::boost::serialization::make_nvp("OC.b", _overlayColor[2]);
        ar & ::boost::serialization::make_nvp("OC.a", _overlayColor[3]);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::list<KnobSerializationPtr> _knobs;
    double _overlayColor[4];
};

NATRON_NAMESPACE_EXIT

BOOST_CLASS_VERSION(NATRON_NAMESPACE::RotoDrawableItemSerialization, ROTO_DRAWABLE_ITEM_VERSION)

BOOST_SERIALIZATION_ASSUME_ABSTRACT(RotoDrawableItemSerialization);


#endif // Engine_RotoDrawableItemSerialization_h
