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


    // Note that the curve type is not serialized, it has to be somehow figured out externally
    switch (curveType) {
        case eCurveSerializationTypePropertiesOnly: {
            for (std::list<KeyFrameSerialization>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
                em << it->time;
                if (!it->properties.empty()) {
                    em << YAML::BeginMap;
                    for (std::list<KeyFrameProperty>::const_iterator it2 = it->properties.begin(); it2 != it->properties.end(); ++it2) {

                        if (it2->values.size() == 0) {
                            continue;
                        }
                        em << YAML::Key << it2->type << YAML::Value;
                        em << YAML::BeginMap;

                        em << YAML::Key << "Name" << YAML::Value << it2->name;


                        em << YAML::Key << "Value" << YAML::Value;
                        if (it2->values.size() > 1) {
                            em << YAML::Flow << YAML::BeginSeq;
                        }
                        for (std::size_t i = 0; i < it2->values.size(); ++i) {
                            if (it2->type == kKeyFramePropertyVariantTypeString) {
                                em << it2->values[i].stringValue;
                            } else if (it2->type == kKeyFramePropertyVariantTypeDouble) {
                                em << it2->values[i].scalarValue;
                            } else if (it2->type == kKeyFramePropertyVariantTypeInt) {
                                em << (int)it2->values[i].scalarValue;
                            } else if (it2->type == kKeyFramePropertyVariantTypeBool) {
                                em << (bool)it2->values[i].scalarValue;
                            }
                        }
                        if (it2->values.size() > 1) {
                            em << YAML::EndSeq;
                        }


                        em << YAML::EndMap;
                    }
                    em << YAML::EndMap;
                }

            }
        }   break;
        case eCurveSerializationTypeString: {
            for (std::list<KeyFrameSerialization>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
                em << it->time;
                assert(it->properties.size() == 1 && it->properties.front().type == kKeyFramePropertyVariantTypeString);
                assert(it->properties.front().values.size() == 1);
                em << it->properties.front().values[0].stringValue;
            }
        }   break;

        case eCurveSerializationTypeScalar: {
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
        }   break;
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

    switch (curveType) {
        case eCurveSerializationTypePropertiesOnly: {
            for (std::size_t i = 0; i < node.size(); ++i) {
                KeyFrameSerialization keyframe;
                keyframe.time = node[i].as<double>();
                if (i+1 < node.size() && node[i+1].IsMap()) {
                    ++i;
                    // We have properties
                    const YAML::Node& propertiesNode = node[i];
                    for (YAML::Node::const_iterator it = propertiesNode.begin(); it != propertiesNode.end(); ++it) {
                        KeyFrameProperty prop;
                        prop.type = it->first.as<std::string>();

                        const YAML::Node& propNode = it->second;
                        if (!propNode.IsMap()) {
                            throw YAML::InvalidNode();
                        }
                        prop.name = propNode["Name"].as<std::string>();
                        const YAML::Node& valuesNode = propNode["Value"];
                        if (valuesNode.IsSequence()) {
                            for (std::size_t j = 0; j < valuesNode.size(); ++j) {
                                KeyFramePropertyVariant v;
                                if (prop.type == kKeyFramePropertyVariantTypeString) {
                                    v.stringValue = valuesNode[j].as<std::string>();
                                } else if (prop.type == kKeyFramePropertyVariantTypeInt) {
                                    v.scalarValue = valuesNode[j].as<int>();
                                } else if (prop.type == kKeyFramePropertyVariantTypeBool) {
                                    v.scalarValue = valuesNode[j].as<bool>();
                                } else if (prop.type == kKeyFramePropertyVariantTypeDouble) {
                                    v.scalarValue = valuesNode[j].as<double>();
                                }
                                prop.values.push_back(v);


                            }
                        } else {
                            KeyFramePropertyVariant v;
                            if (prop.type == kKeyFramePropertyVariantTypeString) {
                                v.stringValue = valuesNode.as<std::string>();
                            } else if (prop.type == kKeyFramePropertyVariantTypeInt) {
                                v.scalarValue = valuesNode.as<int>();
                            } else if (prop.type == kKeyFramePropertyVariantTypeBool) {
                                v.scalarValue = valuesNode.as<bool>();
                            } else if (prop.type == kKeyFramePropertyVariantTypeDouble) {
                                v.scalarValue = valuesNode.as<double>();
                            }
                            prop.values.push_back(v);
                        }
                        keyframe.properties.push_back(prop);
                    }
                }
                keys.push_back(keyframe);
            }
        }   break;
        case eCurveSerializationTypeString: {
            if ((node.size() % 2) != 0) {
                return;
            }
            for (std::size_t i = 0; i < node.size(); i+=2) {
                KeyFrameSerialization keyframe;

                keyframe.time = node[i].as<double>();

                KeyFrameProperty prop;
                prop.type = kKeyFramePropertyVariantTypeString;
                KeyFramePropertyVariant v;
                v.stringValue = node[i+1].as<std::string>();
                prop.values.push_back(v);
                keyframe.properties.push_back(prop);
                keys.push_back(keyframe);
            }
        }   break;

        case eCurveSerializationTypeScalar: {
            CurveDecodeStateEnum state = eCurveDecodeStateMayExpectInterpolation;
            std::string interpolation;
            KeyFrameSerialization keyframe;
            bool pushKeyFrame = false;
            for (std::size_t i = 0; i < node.size(); ++i) {

                const YAML::Node& keyNode = node[i];

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

        }   break;

    };

}


SERIALIZATION_NAMESPACE_EXIT
