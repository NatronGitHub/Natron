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

#ifndef _Engine_RotoContextSerialization_h_
#define _Engine_RotoContextSerialization_h_

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
#include "Engine/CurveSerialization.h"
#include "Engine/KnobSerialization.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoLayerSerialization.h"
#include "Engine/RotoContextPrivate.h"

#define ROTO_CTX_REMOVE_COUNTERS 2
#define ROTO_CTX_VERSION ROTO_CTX_REMOVE_COUNTERS


class RotoContextSerialization
{
    friend class boost::serialization::access;
    friend class RotoContext;

public:

    RotoContextSerialization()
        : _baseLayer()
        , _selectedItems()
        , _autoKeying(false)
        , _rippleEdit(false)
        , _featherLink(false)
    {
    }

    ~RotoContextSerialization()
    {
    }

private:


    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        Q_UNUSED(version);

        ar & boost::serialization::make_nvp("BaseLayer",_baseLayer);
        ar & boost::serialization::make_nvp("AutoKeying",_autoKeying);
        ar & boost::serialization::make_nvp("RippleEdit",_rippleEdit);
        ar & boost::serialization::make_nvp("FeatherLink",_featherLink);
        ar & boost::serialization::make_nvp("Selection",_selectedItems);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {

        ar & boost::serialization::make_nvp("BaseLayer",_baseLayer);
        ar & boost::serialization::make_nvp("AutoKeying",_autoKeying);
        ar & boost::serialization::make_nvp("RippleEdit",_rippleEdit);
        ar & boost::serialization::make_nvp("FeatherLink",_featherLink);
        ar & boost::serialization::make_nvp("Selection",_selectedItems);
        if (version < ROTO_CTX_REMOVE_COUNTERS) {
            std::map<std::string,int> _itemCounters;
            ar & boost::serialization::make_nvp("Counters",_itemCounters);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    RotoLayerSerialization _baseLayer;
    std::list< std::string > _selectedItems;
    bool _autoKeying;
    bool _rippleEdit;
    bool _featherLink;
};

BOOST_CLASS_VERSION(RotoContextSerialization,ROTO_CTX_VERSION)


#endif // _Engine_RotoContextSerialization_h_
