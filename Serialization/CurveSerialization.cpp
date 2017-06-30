/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "CurveSerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER

void
CurveSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::Flow;
    em << YAML::BeginSeq;

    std::string prevInterpolation; // No valid interpolation set yet
    for (std::list<KeyFrameSerialization>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {

        if (prevInterpolation.empty() ||
            prevInterpolation != it->interpolation) {
            // serialize interpolation
            em << it->interpolation;
        }
        prevInterpolation = it->interpolation;


        em << it->time;
        em << it->value;

        if (it->interpolation == kKeyframeSerializationTypeFree || it->interpolation == kKeyframeSerializationTypeBroken) {
            // When user set tangents manually, we need the right tangents.
            em << it->rightDerivative;
        }

        if (it->interpolation == kKeyframeSerializationTypeBroken) {
            // In broken mode, both tangents can be different so serialize both
            em << it->leftDerivative;
        }

    }
    em << YAML::EndSeq;
}

enum CurveDecodeStateEnum
{
    eCurveDecodeStateExpectTime,
    eCurveDecodeStateExpectValue,
    eCurveDecodeStateMayExpectInterpolation,
    eCurveDecodeStateMayExpectRightDerivative,
    eCurveDecodeStateMayExpectLeftDerivative

};

void
CurveSerialization::decode(const YAML::Node& node)
{
    if (!node.IsSequence()) {
        return;
    }
    if (node.size() == 0) {
        return;
    }
    CurveDecodeStateEnum state = eCurveDecodeStateMayExpectInterpolation;
    std::string interpolation;
    KeyFrameSerialization keyframe;
    bool pushKeyFrame = false;
    for (std::size_t i = 0; i < node.size(); ++i) {

        YAML::Node keyNode = node[i];

        switch (state) {
            case eCurveDecodeStateMayExpectInterpolation:
                if (pushKeyFrame) {
                    keys.push_back(keyframe);

                    // Reset keyframe
                    keyframe.interpolation.clear();
                    keyframe.time = keyframe.value = keyframe.leftDerivative = keyframe.rightDerivative = 0.;

                }
                try {
                    // First try to get a time. Conversion of a string to double always fails but string to double does not
                    keyframe.time = keyNode.as<double>();

                    // OK we read a valid time, assume the interpolation is the same as the previous

                    // No interpolation, use the interpolation set previously.
                    // If interpolation is not set, set interpolation to linear
                    if (interpolation.empty()) {
                        interpolation = kKeyframeSerializationTypeLinear;
                    }
                    keyframe.interpolation = interpolation;
                    state = eCurveDecodeStateExpectValue;
                } catch (const YAML::BadConversion& /*e*/) {
                    // OK we read an interpolation and set the curve interpolation so far if needed
                    keyframe.interpolation = keyNode.as<std::string>();
                    // No interpolation, use the interpolation set previously.
                    // If interpolation is not set, set interpolation to linear
                    if (interpolation.empty()) {
                        interpolation = keyframe.interpolation;
                    }
                    state = eCurveDecodeStateExpectTime;
                }

                break;
            case eCurveDecodeStateExpectTime:
                keyframe.time = keyNode.as<double>();
                state  = eCurveDecodeStateExpectValue;
                break;
            case eCurveDecodeStateExpectValue:
                keyframe.value = keyNode.as<double>();
                // Depending on interpolation we may expect derivatives
                if (keyframe.interpolation == kKeyframeSerializationTypeFree || keyframe.interpolation == kKeyframeSerializationTypeBroken) {
                    state = eCurveDecodeStateMayExpectRightDerivative;
                } else {
                    state = eCurveDecodeStateMayExpectInterpolation;
                }
                break;
            case eCurveDecodeStateMayExpectRightDerivative:
                keyframe.rightDerivative = keyNode.as<double>();
                if (keyframe.interpolation == kKeyframeSerializationTypeBroken) {
                    state = eCurveDecodeStateMayExpectLeftDerivative;
                } else {
                    state = eCurveDecodeStateMayExpectInterpolation;
                }
                break;
            case eCurveDecodeStateMayExpectLeftDerivative:
                keyframe.leftDerivative = keyNode.as<double>();
                state = eCurveDecodeStateMayExpectInterpolation;
                break;
        }
        pushKeyFrame = true;
    }
    if (pushKeyFrame) {
        keys.push_back(keyframe);
    }
}


SERIALIZATION_NAMESPACE_EXIT
