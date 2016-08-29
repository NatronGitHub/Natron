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

#ifndef Engine_RotoItemSerialization_h
#define Engine_RotoItemSerialization_h


#include "Serialization/SerializationBase.h"

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#define ROTO_ITEM_INTRODUCES_LABEL 2
#define ROTO_ITEM_VERSION ROTO_ITEM_INTRODUCES_LABEL
#endif

SERIALIZATION_NAMESPACE_ENTER;

class RotoItemSerialization
: public SerializationObjectBase
{

public:

    std::string name, label;
    bool activated;
    std::string parentLayerName;
    bool locked;


    RotoItemSerialization()
        : name()
        , activated(false)
        , parentLayerName()
        , locked(false)
    {
    }

    virtual ~RotoItemSerialization()
    {
    }


    virtual void encode(YAML_NAMESPACE::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML_NAMESPACE::Node& node) OVERRIDE;

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
        ar & ::boost::serialization::make_nvp("Name", name);
        if (version >= ROTO_ITEM_INTRODUCES_LABEL) {
            ar & ::boost::serialization::make_nvp("Label", label);
        }
        ar & ::boost::serialization::make_nvp("Activated", activated);
        ar & ::boost::serialization::make_nvp("ParentLayer", parentLayerName);
        ar & ::boost::serialization::make_nvp("Locked", locked);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif


};

SERIALIZATION_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::RotoItemSerialization, ROTO_ITEM_VERSION)
#endif


#endif // Engine_RotoItemSerialization_h
