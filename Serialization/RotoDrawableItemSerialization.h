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

#ifndef Engine_RotoDrawableItemSerialization_h
#define Engine_RotoDrawableItemSerialization_h

#include "Serialization/RotoItemSerialization.h"

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#define ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING 2
#define ROTO_DRAWABLE_ITEM_REMOVES_INVERTED 3
#define ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST 4
#define ROTO_DRAWABLE_ITEM_VERSION ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST
#endif

SERIALIZATION_NAMESPACE_ENTER;

class RotoDrawableItemSerialization
    : public RotoItemSerialization
{

public:

    RotoDrawableItemSerialization()
        : RotoItemSerialization()
    {
    }

    virtual ~RotoDrawableItemSerialization()
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
        boost::serialization::void_cast_register<RotoDrawableItemSerialization, RotoItemSerialization>(
            static_cast<RotoDrawableItemSerialization *>(NULL),
            static_cast<RotoItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        if (version < ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST) {
            KnobSerializationPtr activated(new KnobSerialization);
            ar & ::boost::serialization::make_nvp("Activated", *activated);
            _knobs.push_back(activated);
            KnobSerializationPtr opacity(new KnobSerialization);
            ar & ::boost::serialization::make_nvp("Opacity", *opacity);
            _knobs.push_back(opacity);
            KnobSerializationPtr feather(new KnobSerialization);
            ar & ::boost::serialization::make_nvp("Feather", *feather);
            _knobs.push_back(feather);
            KnobSerializationPtr falloff(new KnobSerialization);
            ar & ::boost::serialization::make_nvp("FallOff", *falloff);
            _knobs.push_back(falloff);
            if (version < ROTO_DRAWABLE_ITEM_REMOVES_INVERTED) {
                KnobSerializationPtr inverted(new KnobSerialization);
                ar & ::boost::serialization::make_nvp("Inverted", *inverted);
                _knobs.push_back(inverted);
            }
            if (version >= ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING) {
                KnobSerializationPtr color(new KnobSerialization);
                ar & ::boost::serialization::make_nvp("Color", *color);
                _knobs.push_back(color);
                KnobSerializationPtr comp(new KnobSerialization);
                ar & ::boost::serialization::make_nvp("CompOP", *comp);
                _knobs.push_back(comp);
            }
        } else {
            int nKnobs;
            ar & ::boost::serialization::make_nvp("NbItems", nKnobs);
            for (int i = 0; i < nKnobs; ++i) {
                KnobSerializationPtr k(new KnobSerialization);
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
#endif

    std::list<KnobSerializationPtr> _knobs;
    double _overlayColor[4];
};

SERIALIZATION_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::RotoDrawableItemSerialization, ROTO_DRAWABLE_ITEM_VERSION)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(SERIALIZATION_NAMESPACE::RotoDrawableItemSerialization);
#endif


#endif // Engine_RotoDrawableItemSerialization_h
