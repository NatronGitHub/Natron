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

#ifndef _Engine_RotoDrawableItemSerialization_h_
#define _Engine_RotoDrawableItemSerialization_h_

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
#include "Engine/RotoContextPrivate.h"
#include "Engine/RotoItemSerialization.h"

#define ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING 2
#define ROTO_DRAWABLE_ITEM_REMOVES_INVERTED 3
#define ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST 4
#define ROTO_DRAWABLE_ITEM_VERSION ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST


class RotoDrawableItemSerialization
    : public RotoItemSerialization
{
    friend class boost::serialization::access;
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
        boost::serialization::void_cast_register<RotoDrawableItemSerialization,RotoItemSerialization>(
            static_cast<RotoDrawableItemSerialization *>(NULL),
            static_cast<RotoItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        int nKnobs = _knobs.size();
        ar & boost::serialization::make_nvp("NbItems",nKnobs);
        for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
            ar & boost::serialization::make_nvp("Item",**it);
        }
        
        ar & boost::serialization::make_nvp("OC.r",_overlayColor[0]);
        ar & boost::serialization::make_nvp("OC.g",_overlayColor[1]);
        ar & boost::serialization::make_nvp("OC.b",_overlayColor[2]);
        ar & boost::serialization::make_nvp("OC.a",_overlayColor[3]);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoDrawableItemSerialization,RotoItemSerialization>(
                                                                                                      static_cast<RotoDrawableItemSerialization *>(NULL),
                                                                                                      static_cast<RotoItemSerialization *>(NULL)
                                                                                                      );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        if (version < ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST) {
            
            boost::shared_ptr<KnobSerialization> activated(new KnobSerialization);
            ar & boost::serialization::make_nvp("Activated",*activated);
            _knobs.push_back(activated);
            boost::shared_ptr<KnobSerialization> opacity(new KnobSerialization);
            ar & boost::serialization::make_nvp("Opacity",*opacity);
            _knobs.push_back(opacity);
            boost::shared_ptr<KnobSerialization> feather(new KnobSerialization);
            ar & boost::serialization::make_nvp("Feather",*feather);
            _knobs.push_back(feather);
            boost::shared_ptr<KnobSerialization> falloff(new KnobSerialization);
            ar & boost::serialization::make_nvp("FallOff",*falloff);
            _knobs.push_back(falloff);
            if (version < ROTO_DRAWABLE_ITEM_REMOVES_INVERTED) {
                boost::shared_ptr<KnobSerialization> inverted(new KnobSerialization);
                ar & boost::serialization::make_nvp("Inverted",*inverted);
                _knobs.push_back(inverted);
            }
            if (version >= ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING) {
                boost::shared_ptr<KnobSerialization> color(new KnobSerialization);
                ar & boost::serialization::make_nvp("Color",*color);
                _knobs.push_back(color);
                boost::shared_ptr<KnobSerialization> comp(new KnobSerialization);
                ar & boost::serialization::make_nvp("CompOP",*comp);
                _knobs.push_back(comp);
            }
        } else {
            int nKnobs;
            ar & boost::serialization::make_nvp("NbItems",nKnobs);
            for (int i = 0; i < nKnobs; ++i) {
                boost::shared_ptr<KnobSerialization> k(new KnobSerialization);
                ar & boost::serialization::make_nvp("Item",*k);
                _knobs.push_back(k);
            }
        }
        ar & boost::serialization::make_nvp("OC.r",_overlayColor[0]);
        ar & boost::serialization::make_nvp("OC.g",_overlayColor[1]);
        ar & boost::serialization::make_nvp("OC.b",_overlayColor[2]);
        ar & boost::serialization::make_nvp("OC.a",_overlayColor[3]);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    std::list<boost::shared_ptr<KnobSerialization> > _knobs;
    double _overlayColor[4];
};

BOOST_CLASS_VERSION(RotoDrawableItemSerialization,ROTO_DRAWABLE_ITEM_VERSION)

BOOST_SERIALIZATION_ASSUME_ABSTRACT(RotoDrawableItemSerialization);


#endif // _Engine_RotoDrawableItemSerialization_h_
