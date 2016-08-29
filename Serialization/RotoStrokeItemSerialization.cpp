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

#include "RotoLayerSerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/CurveSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RotoStrokeItemSerialization::encode(YAML::Emitter& em) const
{
    RotoDrawableItemSerialization::encode(em);
    em << YAML::BeginMap;
    em << YAML::Key << "Type" << YAML::Value << _brushType;
    em << YAML::Key << "SubStrokes" << YAML::Value;
    em << YAML::BeginSeq;
    for (std::list<PointCurves>::const_iterator it = _subStrokes.begin(); it!= _subStrokes.end(); ++it) {
        em << YAML::BeginMap;
        em << YAML::Key << "x" << YAML::Value;
        it->x->encode(em);
        em << YAML::Key << "y" << YAML::Value;
        it->y->encode(em);
        em << YAML::Key << "pressure" << YAML::Value;
        it->pressure->encode(em);
        em << YAML::EndMap;
    }
    em << YAML::EndSeq;
    em << YAML::EndMap;
   
}

static void decodePointCurve(const YAML::Node& node, RotoStrokeItemSerialization::PointCurves* p)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }
    for (YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
        std::string key = it->first.as<std::string>();

        if (key == "x") {
            p->x.reset(new CurveSerialization);
            p->x->decode(it->second);
        } else if (key == "y") {
            p->y.reset(new CurveSerialization);
            p->y->decode(it->second);
        } else if (key == "pressure") {
            p->pressure.reset(new CurveSerialization);
            p->pressure->decode(it->second);
        }
    }
}

void
RotoStrokeItemSerialization::decode(const YAML::Node& node)
{
    RotoItemSerialization::decode(node);
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }


    for (YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {

        std::string key = it->first.as<std::string>();
        if (key == "Type") {
            _brushType = it->second.as<std::string>();
        } else if (key == "SubStrokes") {
            if (!it->second.IsSequence()) {
                throw YAML::InvalidNode();
            }
            for (YAML::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                PointCurves p;
                decodePointCurve(it2->second, &p);
                _subStrokes.push_back(p);
            }
        }

    }
}

SERIALIZATION_NAMESPACE_EXIT
