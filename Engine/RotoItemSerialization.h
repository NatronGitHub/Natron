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

#ifndef Engine_RotoItemSerialization_h
#define Engine_RotoItemSerialization_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

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
#include "Engine/CurveSerialization.h"
#include "Engine/KnobSerialization.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoContextPrivate.h"
#include "Engine/EngineFwd.h"


#define ROTO_ITEM_INTRODUCES_LABEL 2
#define ROTO_ITEM_VERSION ROTO_ITEM_INTRODUCES_LABEL


class RotoItemSerialization
{
    friend class boost::serialization::access;
    friend class RotoItem;

public:

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

private:


    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        Q_UNUSED(version);
        ar & boost::serialization::make_nvp("Name",name);
        ar & boost::serialization::make_nvp("Label",label);
        ar & boost::serialization::make_nvp("Activated",activated);
        ar & boost::serialization::make_nvp("ParentLayer",parentLayerName);
        ar & boost::serialization::make_nvp("Locked",locked);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        ar & boost::serialization::make_nvp("Name",name);
        if ( version >= ROTO_ITEM_INTRODUCES_LABEL) {
            ar & boost::serialization::make_nvp("Label",label);
        }
        ar & boost::serialization::make_nvp("Activated",activated);
        ar & boost::serialization::make_nvp("ParentLayer",parentLayerName);
        ar & boost::serialization::make_nvp("Locked",locked);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::string name,label;
    bool activated;
    std::string parentLayerName;
    bool locked;
};

BOOST_CLASS_VERSION(RotoItemSerialization,ROTO_ITEM_VERSION)


//BOOST_SERIALIZATION_ASSUME_ABSTRACT(RotoItemSerialization);

#endif // Engine_RotoItemSerialization_h
