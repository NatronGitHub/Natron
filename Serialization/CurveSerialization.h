/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_CURVESERIALIZATION_H
#define NATRON_ENGINE_CURVESERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <map>

#include "Serialization/SerializationBase.h"
#include "Serialization/SerializationFwd.h"

SERIALIZATION_NAMESPACE_ENTER

#define kKeyframeSerializationTypeConstant "K"
#define kKeyframeSerializationTypeLinear "L"
#define kKeyframeSerializationTypeSmooth "S"
#define kKeyframeSerializationTypeCatmullRom "R"
#define kKeyframeSerializationTypeCubic "C"
#define kKeyframeSerializationTypeHorizontal "H"
#define kKeyframeSerializationTypeFree "F"
#define kKeyframeSerializationTypeBroken "X"

#define kKeyFramePropertyVariantTypeBool "Bool"
#define kKeyFramePropertyVariantTypeInt "Int"
#define kKeyFramePropertyVariantTypeDouble "Double"
#define kKeyFramePropertyVariantTypeString "String"



struct KeyFramePropertyVariant
{
    double scalarValue;
    std::string stringValue;
};

struct KeyFrameProperty
{
    std::string type;
    std::string name;
    std::vector<KeyFramePropertyVariant> values;
};

/**
 * @brief Basically just the same as a Keyframe but without all member functions and extracted to remove any dependency to Natron.
 * This class i contained into CurveSerialization
 **/
struct KeyFrameSerialization
{

    // The frame in the timeline. This is not necessarily an integer if the user applied retiming operations
    double time;

    // The value of the keyframe
    double value;

    // Letter that corresponds to one value of KeyframeTypeEnum
    // eKeyframeTypeConstant = kKeyframeSerializationTypeConstant,
    // eKeyframeTypeLinear = kKeyframeSerializationTypeLinear
    // eKeyframeTypeSmooth = kKeyframeSerializationTypeSmooth
    // eKeyframeTypeCatmullRom = kKeyframeSerializationTypeCatmullRom
    // eKeyframeTypeCubic = kKeyframeSerializationTypeCubic
    // eKeyframeTypeHorizontal = kKeyframeSerializationTypeHorizontal
    // eKeyframeTypeFree = kKeyframeSerializationTypeFree
    // eKeyframeTypeBroken = kKeyframeSerializationTypeBroken
    // eKeyframeTypeNone = empty string, meaning we do not serialize the value
    //
    // This value is only serialized if this is the first keyframe in the curve OR if the interpolation type changes
    // at this keyframe.
    std::string interpolation;

    // This is only needed in eKeyframeTypeFree mode where the user controls the interpolation of both tangents at once
    // or in eKeyframeTypeBroken mode
    double rightDerivative;

    // This is only needed in eKeyframeTypeBroken mode when user can have set different right/left tangents
    double leftDerivative;

    // List of properties associated to the keyframe. Properties do not have any interpolation, they are
    // assumed to have a constant interpolation.
    std::list<KeyFrameProperty> properties;


};

enum CurveSerializationTypeEnum
{
    // The curve contains for each keyframe a scalar value
    eCurveSerializationTypeScalar,

    // The curve contains for each keyframe a single property. It can only be of constant interpolation type.
    eCurveSerializationTypeString,

    // The curve contains for each keyframe a list of properties of different types.
    // This is a generic version of eCurveSerializationTypeString where a keyframe may hold more data.
    // Note that if a keyframe doesn't have any property, it will still save the time marker.
    // It can only be of constant interpolation type.
    eCurveSerializationTypePropertiesOnly,
};

class CurveSerialization
: public SerializationObjectBase
{
public:

    CurveSerializationTypeEnum curveType;

    // We don't need a set complicated data structure here because we trust that the Curve itself
    // gave us a list of keyframes with a correct ordering
    std::list<KeyFrameSerialization> keys;

    virtual void encode(YAML::Emitter& em) const OVERRIDE FINAL;

    virtual void decode(const YAML::Node& node) OVERRIDE FINAL;

};

SERIALIZATION_NAMESPACE_EXIT

#endif // NATRON_ENGINE_CURVESERIALIZATION_H
