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


#ifndef SERIALIZATIONCOMPAT_H
#define SERIALIZATIONCOMPAT_H

// This file contains all implementation of old boost serialization involving Engine and Gui classes
#ifdef NATRON_BOOST_SERIALIZATION_COMPAT

#include "Serialization/SerializationBase.h"

#include "Engine/Curve.h"
#include "Engine/ImageComponents.h"
#include "Engine/Format.h"
#include "Engine/RectI.h"

#define FORMAT_SERIALIZATION_CHANGES_TO_RECTD 2
#define FORMAT_SERIALIZATION_CHANGES_TO_RECTI 3
#define FORMAT_SERIALIZATION_VERSION FORMAT_SERIALIZATION_CHANGES_TO_RECTI

template<class Archive>
void NATRON_NAMESPACE::KeyFrame::serialize(Archive & ar,
               const unsigned int version)
{
    ar & ::boost::serialization::make_nvp("Time", _time);
    ar & ::boost::serialization::make_nvp("Value", _value);
    ar & ::boost::serialization::make_nvp("InterpolationMethod", _interpolation);
    ar & ::boost::serialization::make_nvp("LeftDerivative", _leftDerivative);
    ar & ::boost::serialization::make_nvp("RightDerivative", _rightDerivative);
}

template<class Archive>
void NATRON_NAMESPACE::Curve::serialize(Archive & ar, const unsigned int version)
{
    KeyFrameSet keys = getKeyFrames_mt_safe();
    ar & ::boost::serialization::make_nvp("KeyFrameSet", keys);
}


template<class Archive>
void NATRON_NAMESPACE::ImageComponents::serialize(Archive & ar, const unsigned int version)
{
    ar &  boost::serialization::make_nvp("Layer", _layerName);
    ar &  boost::serialization::make_nvp("Components", _componentNames);
    ar &  boost::serialization::make_nvp("CompName", _globalComponentsName);
}

template<class Archive>
void NATRON_NAMESPACE::Format::serialize(Archive & ar,
               const unsigned int version)
{
    if (version < FORMAT_SERIALIZATION_CHANGES_TO_RECTD) {
        RectI r;
        ar & ::boost::serialization::make_nvp("RectI", r);
        x1 = r.x1;
        x2 = r.x2;
        y1 = r.y1;
        y2 = r.y2;
    } else if (version < FORMAT_SERIALIZATION_CHANGES_TO_RECTI) {
        RectD r;
        ar & ::boost::serialization::make_nvp("RectD", r);
        x1 = r.x1;
        x2 = r.x2;
        y1 = r.y1;
        y2 = r.y2;
    } else {
        boost::serialization::void_cast_register<Format, RectI>( static_cast<Format *>(NULL),
                                                                static_cast<RectI *>(NULL) );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RectI);
    }
    ar & ::boost::serialization::make_nvp("Pixel_aspect_ratio", _par);
    ar & ::boost::serialization::make_nvp("Name", _name);
}

template<class Archive>
void
NATRON_NAMESPACE::RectI::serialize(Archive & ar,
                                   const unsigned int /*version*/)
{
    ar & ::boost::serialization::make_nvp("Left", x1);
    ar & ::boost::serialization::make_nvp("Bottom", y1);
    ar & ::boost::serialization::make_nvp("Right", x2);
    ar & ::boost::serialization::make_nvp("Top", y2);
}


template<class Archive>
void
NATRON_NAMESPACE::RectD::serialize(Archive & ar,
                                   const unsigned int /*version*/)
{
    ar & ::boost::serialization::make_nvp("Left", x1);
    ar & ::boost::serialization::make_nvp("Bottom", y1);
    ar & ::boost::serialization::make_nvp("Right", x2);
    ar & ::boost::serialization::make_nvp("Top", y2);
}


BOOST_SERIALIZATION_ASSUME_ABSTRACT(RectD);
BOOST_SERIALIZATION_ASSUME_ABSTRACT(SERIALIZATION_NAMESPACE::RectI);
BOOST_CLASS_VERSION(NATRON_NAMESPACE::Format, FORMAT_SERIALIZATION_VERSION)


#endif // NATRON_BOOST_SERIALIZATION_COMPAT

#endif // SERIALIZATIONCOMPAT_H
