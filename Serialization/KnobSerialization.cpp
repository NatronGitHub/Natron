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


#include <cfloat>
#include <climits>
#include <iostream>
#include <sstream> // stringstream

#include <yaml-cpp/yaml.h>

#include "KnobSerialization.h"

#include "Serialization/CurveSerialization.h"

#define kKnobSerializationDataTypeKeyBool "Bool"
#define kKnobSerializationDataTypeKeyInt "Int"
#define kKnobSerializationDataTypeKeyDouble "Float"
#define kKnobSerializationDataTypeKeyString "String"
#define kKnobSerializationDataTypeKeyNone "Value"

SERIALIZATION_NAMESPACE_ENTER;

const std::string&
ValueSerialization::getKnobName() const
{
    return _serialization->getName();
}

void
ValueSerialization::setEnabledChanged(bool b)
{
    KnobSerialization* isKnob = dynamic_cast<KnobSerialization*>(_serialization);
    if (isKnob) {
        isKnob->_disabled = !b;
    }
}

static std::string dataTypeToString(SerializationValueVariantTypeEnum type)
{
    switch (type) {
        case eSerializationValueVariantTypeBoolean:
            return kKnobSerializationDataTypeKeyBool;
        case eSerializationValueVariantTypeInteger:
            return kKnobSerializationDataTypeKeyInt;
        case eSerializationValueVariantTypeDouble:
            return kKnobSerializationDataTypeKeyDouble;
        case eSerializationValueVariantTypeString:
            return kKnobSerializationDataTypeKeyString;
        case eSerializationValueVariantTypeNone:
            return kKnobSerializationDataTypeKeyNone;
    }
    return kKnobSerializationDataTypeKeyNone;
}

static SerializationValueVariantTypeEnum dataTypeFromString(const std::string& type)
{
    if (type == kKnobSerializationDataTypeKeyBool) {
        return eSerializationValueVariantTypeBoolean;
    } else if (type == kKnobSerializationDataTypeKeyInt) {
        return eSerializationValueVariantTypeInteger;
    } else if (type == kKnobSerializationDataTypeKeyDouble) {
        return eSerializationValueVariantTypeDouble;
    } else if (type == kKnobSerializationDataTypeKeyString) {
        return eSerializationValueVariantTypeString;
    } else if (type == kKnobSerializationDataTypeKeyNone) {
        return eSerializationValueVariantTypeNone;
    }
    throw std::invalid_argument("Unknown value type " + type);
}

void
KnobSerialization::encode(YAML::Emitter& em) const
{
    if (!_mustSerialize || _values.empty()) {
        return;
    }
    em << YAML::BeginMap;
    em << YAML::Key << "Name" << YAML::Value << _scriptName;

    // Instead of inserting in the map each boolean
    // we serialize strings, meaning that
    // if they are here, their value is true otherwise it goes to default
    std::list<std::string> propNames;

    int nDimsToSerialize = 0;
    int nDimsWithValue = 0;
    int nDimsWithDefValue = 0;
    for (PerViewValueSerializationMap::const_iterator it = _values.begin(); it != _values.end(); ++it) {
        for (PerDimensionValueSerializationVec::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if (it2->_mustSerialize && _isPersistent) {
                ++nDimsToSerialize;
                if (it2->_serializeValue || !it2->_animationCurve.keys.empty() || it2->_slaveMasterLink.hasLink || !it2->_expression.empty()) {
                    ++nDimsWithValue;
                }
            }
        }
    }
    for (std::size_t i = 0; i < _defaultValues.size(); ++i) {
        if (_defaultValues[i].serializeDefaultValue) {
            ++nDimsWithDefValue;
        }
    }


    if (nDimsToSerialize > 0) {
        // We serialize a value, we need to know which type of knob this is because at the time of deserialization the
        // application may not have created the corresponding knob already and may not know to what type it corresponds
        if (nDimsWithValue > 0) {
            em << YAML::Key;
            em << dataTypeToString(_dataType);
            em << YAML::Value;


            if (_values.size() > 1) {
                // Map against views
                em << YAML::BeginMap;
            }
            for (PerViewValueSerializationMap::const_iterator it = _values.begin(); it != _values.end(); ++it) {

                if (_values.size() > 1) {
                    em << YAML::Key << it->first << YAML::Value;
                }

                // Starting dimensions, make a sequence if multiple dimensions are serialized
                if (it->second.size() > 1) {
                    em << YAML::Flow << YAML::BeginSeq;
                }
                for (PerDimensionValueSerializationVec::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {

                    const ValueSerialization& val = *it2;

                    if (val._slaveMasterLink.hasLink) {
                        // Wrap the link in a sequence of 1 element to distinguish with regular string knobs values
                        em << YAML::Flow << YAML::BeginMap;
                        std::stringstream ss;
                        // Encode the hard-link into a single string
                        if (!val._slaveMasterLink.masterNodeName.empty()) {
                            em << YAML::Key << "N";
                            em << YAML::Value << val._slaveMasterLink.masterNodeName;
                        }
                        if (!val._slaveMasterLink.masterTableItemName.empty()) {
                            em << YAML::Key << "T";
                            em << YAML::Value << val._slaveMasterLink.masterTableItemName;
                        }
                        if (!val._slaveMasterLink.masterKnobName.empty()) {
                            em << YAML::Key << "K";
                            em << YAML::Value << val._slaveMasterLink.masterKnobName;
                        }
                        if (!val._slaveMasterLink.masterDimensionName.empty()) {
                            em << YAML::Key << "D";
                            em << YAML::Value << val._slaveMasterLink.masterDimensionName;
                        }
                        if (!val._slaveMasterLink.masterViewName.empty()) {
                            em << YAML::Key << "V";
                            em << YAML::Value << val._slaveMasterLink.masterViewName;
                        }
                        // Also serialize the curve in case the expression fails to restore correctly
                        if (!val._animationCurve.keys.empty()) {
                            em << YAML::Key << "Curve" << YAML::Value;
                            val._animationCurve.encode(em);
                        }

                        em << YAML::EndMap;
                    } else if (!val._expression.empty()) {
                        // Wrap the expression in a sequence of 1 element to distinguish with regular string knobs values
                        em << YAML::Flow << YAML::BeginMap;
                        if (val._expresionHasReturnVariable) {
                            // Multi-line expr
                            em << YAML::Key << "MultiExpr";
                        } else {
                            // single line expr
                            em << YAML::Key << "Expr";
                        }
                        em << YAML::Value << val._expression;

                        // Also serialize the curve in case the expression fails to restore correctly
                        if (!val._animationCurve.keys.empty()) {
                            em << YAML::Key << "Curve" << YAML::Value;
                            val._animationCurve.encode(em);
                        }

                        em << YAML::EndMap;
                    } else if (!val._animationCurve.keys.empty()) {
                        em << YAML::Flow << YAML::BeginMap;
                        em << YAML::Key << "Curve" << YAML::Value;
                        val._animationCurve.encode(em);
                        em << YAML::EndMap;
                    } else {
                        // No animation or link, directly write the value without a map
                        // This is the general case so we keep it tight
                        assert(_dataType != eSerializationValueVariantTypeNone);
                        switch (_dataType) {
                            case eSerializationValueVariantTypeBoolean:
                                em << val._value.isBool;
                                break;
                            case eSerializationValueVariantTypeInteger:
                                em << val._value.isInt;
                                break;
                            case eSerializationValueVariantTypeDouble:
                                em << val._value.isDouble;
                                break;
                            case eSerializationValueVariantTypeString:
                                em << val._value.isString;
                                break;
                            default:
                                break;
                        }
                    }

                } // for each dimension
                if (it->second.size() > 1) {
                    em << YAML::EndSeq;
                }
            } // for each view


            if (_values.size() > 1) {
                em << YAML::EndMap;
            }
        } // nDimsWithValue > 0


        if (nDimsWithDefValue > 0) {
            {
                em << YAML::Key;
                std::string defaultKey("Default");
                defaultKey += dataTypeToString(_dataType);
                em << defaultKey;
                em << YAML::Value;
            }

            // Starting dimensions
            if (_defaultValues.size() > 1) {
                em << YAML::Flow << YAML::BeginSeq;
            }
            for (std::size_t i = 0; i < _defaultValues.size(); ++i) {

                switch (_dataType) {
                    case eSerializationValueVariantTypeBoolean:
                        em << _defaultValues[i].value.isBool;
                        break;
                    case eSerializationValueVariantTypeInteger:
                        em << _defaultValues[i].value.isInt;
                        break;
                    case eSerializationValueVariantTypeDouble:
                        em << _defaultValues[i].value.isDouble;
                        break;
                    case eSerializationValueVariantTypeString:
                        em << _defaultValues[i].value.isString;
                        break;
                    default:
                        break;
                }

            } // for all dimensions
            if (_defaultValues.size() > 1) {
                em << YAML::EndSeq;
            }


        } // nDimsWithDefValue > 0

    } // nDimsToSerialize > 0

    TextExtraData* textData = dynamic_cast<TextExtraData*>(_extraData.get());
    if (_extraData && _isPersistent) {
        ParametricExtraData* parametricData = dynamic_cast<ParametricExtraData*>(_extraData.get());
        if (parametricData) {
            if (!parametricData->parametricCurves.empty()) {
                em << YAML::Key << "ParametricCurves" << YAML::Value;
                if (parametricData->parametricCurves.size() > 1) {
                    // Multi-view
                    em << YAML::BeginMap;
                }
                for (std::map<std::string,std::list<CurveSerialization> >::const_iterator it = parametricData->parametricCurves.begin(); it!= parametricData->parametricCurves.end(); ++it) {
                    if (parametricData->parametricCurves.size() > 1) {
                        // Multi-view
                        em << YAML::Key << it->first << YAML::Value;
                    }


                    em << YAML::BeginSeq;
                    for (std::list<CurveSerialization>::const_iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
                        it2->encode(em);
                    }
                    em << YAML::EndSeq;


                }
                if (parametricData->parametricCurves.size() > 1) {
                    // Multi-view
                    em << YAML::EndMap;
                }


            }
        } else if (textData) {
            if (!textData->keyframes.empty()) {

                bool hasDeclaredTextAnim = false;
                for (std::map<std::string,std::map<double, std::string> >::const_iterator it = textData->keyframes.begin(); it != textData->keyframes.end(); ++it) {
                    if (it->second.empty()) {
                        continue;
                    }
                    if (!hasDeclaredTextAnim) {
                        hasDeclaredTextAnim = true;

                        em << YAML::Key << "TextAnim" << YAML::Value;

                        if (textData->keyframes.size() > 1) {
                            // Multi-view: start a map
                            em << YAML::BeginMap;
                        }
                    }
                    if (textData->keyframes.size() > 1) {
                        em << YAML::Key << it->first << YAML::Value;
                    }
                    em << YAML::Flow;
                    em << YAML::BeginSeq;
                    for (std::map<double, std::string>::const_iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
                        em << it2->first << it2->second;
                    }
                    em << YAML::EndSeq;
                }
                if (hasDeclaredTextAnim && textData->keyframes.size() > 1) {
                    em << YAML::EndMap;
                }


            }
            if (std::abs(textData->fontColor[0] - 0.) > 0.01 || std::abs(textData->fontColor[1] - 0.) > 0.01 || std::abs(textData->fontColor[2] - 0.) > 0.01) {
                em << YAML::Key << "FontColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << textData->fontColor[0] << textData->fontColor[1] << textData->fontColor[2] << YAML::EndSeq;
            }
            if (textData->fontSize != kKnobStringDefaultFontSize) {
                em << YAML::Key << "FontSize" << YAML::Value << textData->fontSize;
            }
            if (textData->fontFamily != NATRON_FONT) {
                em << YAML::Key << "Font" << YAML::Value << textData->fontFamily;
            }
            if (textData->italicActivated) {
                propNames.push_back("Italic");
            }
            if (textData->boldActivated) {
                propNames.push_back("Bold");
            }
        }
    }

    if (_isUserKnob) {
        // Num of dimensions can be figured out for knobs created by plug-in
        // Knobs created by user need to know it
        em << YAML::Key << "NDims" << YAML::Value << _dimension;
        em << YAML::Key << "TypeName" << YAML::Value << _typeName;

        if (_label != _scriptName) {
            em << YAML::Key << "Label" << YAML::Value << _label;
        }
        if (!_tooltip.empty()) {
            em << YAML::Key << "Hint" << YAML::Value << _tooltip;
        }
        if (_isSecret) {
            propNames.push_back("Secret");
        }

        if (_disabled) {
            propNames.push_back("Disabled");
        }

        if (!_triggerNewLine) {
            propNames.push_back("NoNewLine");
        }
        if (!_evaluatesOnChange) {
            propNames.push_back("NoEval");
        }
        if (_animatesChanged) {
            propNames.push_back("AnimatesChanged");
        }
        if (!_isPersistent) {
            propNames.push_back("Volatile");
        }
        if (!_iconFilePath[0].empty()) {
            em << YAML::Key << "UncheckedIcon" << YAML::Value << _iconFilePath[0];
        }
        if (!_iconFilePath[1].empty()) {
            em << YAML::Key << "CheckedIcon" << YAML::Value << _iconFilePath[1];
        }

        TypeExtraData* typeData = _extraData.get();
        ChoiceExtraData* cdata = dynamic_cast<ChoiceExtraData*>(_extraData.get());
        ValueExtraData* vdata = dynamic_cast<ValueExtraData*>(_extraData.get());
        TextExtraData* tdata = dynamic_cast<TextExtraData*>(_extraData.get());
        FileExtraData* fdata = dynamic_cast<FileExtraData*>(_extraData.get());
        PathExtraData* pdata = dynamic_cast<PathExtraData*>(_extraData.get());

        if (cdata) {
            if (!cdata->_entries.empty()) {
                em << YAML::Key << "Entries" << YAML::Value;
                em << YAML::Flow << YAML::BeginSeq;
                for (std::size_t i = 0; i < cdata->_entries.size(); ++i) {
                    em << cdata->_entries[i];
                }
                em << YAML::EndSeq;
            }
            if (!cdata->_helpStrings.empty()) {
                bool hasHelp = false;
                for (std::size_t i = 0; i < cdata->_helpStrings.size(); ++i) {
                    if (!cdata->_helpStrings[i].empty()) {
                        hasHelp = true;
                        break;
                    }
                }
                if (hasHelp) {
                    em << YAML::Key << "Hints" << YAML::Value;
                    em <<  YAML::Flow << YAML::BeginSeq;
                    for (std::size_t i = 0; i < cdata->_helpStrings.size(); ++i) {
                        em << cdata->_helpStrings[i];
                    }
                    em << YAML::EndSeq;
                }
            }
        } else if (vdata) {
            if (vdata->min != INT_MIN && vdata->min != -DBL_MAX) {
                em << YAML::Key << "Min" << YAML::Value << vdata->min;
            }
            if (vdata->min != INT_MAX && vdata->min != DBL_MAX) {
                em << YAML::Key << "Max" << YAML::Value << vdata->max;
            }
            if (vdata->min != INT_MIN && vdata->min != -DBL_MAX) {
                em << YAML::Key << "DisplayMin" << YAML::Value << vdata->dmin;
            }
            if (vdata->min != INT_MAX && vdata->min != DBL_MAX) {
                em << YAML::Key << "DisplayMax" << YAML::Value << vdata->dmax;
            }
        } else if (tdata) {
            if (tdata->label) {
                propNames.push_back("IsLabel");
            }
            if (tdata->multiLine) {
                propNames.push_back("MultiLine");
            }
            if (tdata->richText) {
                propNames.push_back("RichText");
            }
        } else if (pdata) {
            if (pdata->multiPath) {
                propNames.push_back("MultiPath");
            }
        } else if (fdata) {
            if (fdata->useSequences) {
                propNames.push_back("Sequences");
            }
            if (fdata->useExistingFiles) {
                propNames.push_back("ExistingFiles");
            }
            if (!fdata->filters.empty()) {
                em << YAML::Key << "FileTypes" << YAML::Value << YAML::Flow << YAML::BeginSeq;
                for (std::size_t i = 0; i < fdata->filters.size(); ++i) {
                    em << fdata->filters[i];
                }
                em << YAML::EndSeq;
            }
        }

        if (typeData) {
            if (typeData->useHostOverlayHandle) {
                propNames.push_back("UseOverlay");
            }
        }

    } // if (_isUserKnob) {

    if (_hasViewerInterface) {
        if (!_inViewerContextItemLayout.empty() && _inViewerContextItemLayout != kInViewerContextItemLayoutSpacing) {
            em << YAML::Key << "InViewerLayout" << YAML::Value << _inViewerContextItemLayout;
        } else {
            if (_inViewerContextItemSpacing != kKnobInViewerContextDefaultItemSpacing) {
                em << YAML::Key << "InViewerSpacing" << YAML::Value << _inViewerContextItemSpacing;
            }
        }
        if (_isUserKnob) {
            if (!_inViewerContextLabel.empty()) {
                em << YAML::Key << "InViewerLabel" << YAML::Value << _inViewerContextLabel;
            }
            if (!_inViewerContextIconFilePath[0].empty()) {
                em << YAML::Key << "InViewerIconUnchecked" << YAML::Value << _inViewerContextIconFilePath[0];
            }
            if (!_inViewerContextIconFilePath[1].empty()) {
                em << YAML::Key << "InViewerIconChecked" << YAML::Value << _inViewerContextIconFilePath[1];
            }
        }
    }

    if (!propNames.empty()) {
        em << YAML::Key << "Props" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (std::list<std::string>::const_iterator it2 = propNames.begin(); it2 != propNames.end(); ++it2) {
            em << *it2;
        }
        em << YAML::EndSeq;
    }

    em << YAML::EndMap;
} // KnobSerialization::encode

template <typename T>
static T* getOrCreateExtraData(boost::scoped_ptr<TypeExtraData>& extraData)
{
    T* data = 0;
    if (extraData.get()) {
        data = dynamic_cast<T*>(extraData.get());
    } else {
        data = new T;
        extraData.reset(data);
    }
    assert(data);
    if (!data) {
        throw YAML::InvalidNode();
    }
    return data;
}

static void decodeValueFromNode(const YAML::Node& node,
                                SerializationValueVariant& variant,
                                SerializationValueVariantTypeEnum type)
{
    switch (type) {
        case eSerializationValueVariantTypeBoolean:
            variant.isBool = node.as<bool>();
            break;
        case eSerializationValueVariantTypeDouble:
            variant.isDouble = node.as<double>();
            break;
        case eSerializationValueVariantTypeInteger:
            variant.isInt = node.as<int>();
            break;
        case eSerializationValueVariantTypeString:
            variant.isString = node.as<std::string>();
            break;
        case eSerializationValueVariantTypeNone:
            break;
    }
}

static void initValuesVec(KnobSerialization* knob, KnobSerialization::PerDimensionValueSerializationVec* serialization, int nDims)
{
    if ((int)serialization->size() != nDims) {
        knob->_dimension = nDims;
        serialization->resize(nDims);
        for (std::size_t i = 0; i < serialization->size(); ++i) {
            (*serialization)[i]._serialization = knob;
            (*serialization)[i]._mustSerialize = true;
            (*serialization)[i]._dimension = i;
        }
    }
}

void
KnobSerialization::decodeValueNode(const std::string& viewName, const YAML::Node& node)
{

    int nDims = node.IsSequence() ? node.size() : 1;

    PerDimensionValueSerializationVec& dimVec = _values[viewName];
    initValuesVec(this, &dimVec, nDims);

    for (int i = 0; i < nDims; ++i) {

        YAML::Node dimNode = node.IsSequence() ? node[i] : node;

        if (!dimNode.IsMap()) {
            // This is a value
            decodeValueFromNode(dimNode, dimVec[i]._value, _dataType);
            dimVec[i]._serializeValue = true;
        } else { // !isMap

            // Always look for an animation curve
            if (dimNode["Curve"]) {
                // Curve
                dimVec[i]._animationCurve.decode(dimNode["Curve"]);
            }

            // Look for a link or expression
            if (dimNode["MultiExpr"]) {
                dimVec[i]._expression = dimNode["MultiExpr"].as<std::string>();
                dimVec[i]._expresionHasReturnVariable = true;
            } else if (dimNode["Expr"]) {
                dimVec[i]._expression = dimNode["Expr"].as<std::string>();
            } else {
                // This is most likely a regular slavr/master link
                bool gotLink = false;
                if (dimNode["N"]) {
                    dimVec[i]._slaveMasterLink.masterNodeName = dimNode["N"].as<std::string>();
                    gotLink = true;
                }
                if (dimNode["T"]) {
                    dimVec[i]._slaveMasterLink.masterTableItemName = dimNode["T"].as<std::string>();
                    gotLink = true;
                }
                if (dimNode["K"]) {
                    dimVec[i]._slaveMasterLink.masterKnobName = dimNode["K"].as<std::string>();
                    gotLink = true;
                }
                if (dimNode["D"]) {
                    dimVec[i]._slaveMasterLink.masterDimensionName = dimNode["D"].as<std::string>();
                    gotLink = true;
                }
                if (dimNode["V"]) {
                    dimVec[i]._slaveMasterLink.masterViewName = dimNode["V"].as<std::string>();
                    gotLink = true;
                }
                dimVec[i]._slaveMasterLink.hasLink = gotLink;
            }

        } // isMap
    }

} //decodeValueNode

bool
KnobSerialization::checkForValueNode(const YAML::Node& node, const std::string& nodeType)
{

    if (!node[nodeType]) {
        return false;
    }
        // We need to figure out of the knob is multi-view and if multi-dimensional
    YAML::Node valueNode = node[nodeType];

    _dataType = dataTypeFromString(nodeType);

    // If the "Value" is a map, this can be either a multi-view knob or a single-view
    // and single-dimensional knob with animation.
    // Check to find any of the keys of a single dimension map. If we find it, that means
    // this is not the multi-view map and that this is a single-dimensional knob
    if (!valueNode.IsMap()) {
        decodeValueNode("Main", valueNode);
    } else {
        if (valueNode["Curve"] || valueNode["MultiExpr"] || valueNode["Expr"] || valueNode["N"] || valueNode["T"] ||
            valueNode["K"] || valueNode["D"] || valueNode["V"]) {
            decodeValueNode("Main", valueNode);
        } else {
            // Multi-view
            for (YAML::const_iterator it = valueNode.begin(); it != valueNode.end(); ++it) {
                decodeValueNode(it->first.as<std::string>(), it->second);
            }
        }
    }
    return true;
}

bool
KnobSerialization::checkForDefaultValueNode(const YAML::Node& node, const std::string& nodeType, bool dataTypeSet)
{
    std::string defaultString("Default");
    std::string defaultTypeName = defaultString + nodeType;
    if (!node[defaultTypeName]) {
        return false;
    }

    // If the _dataType member was set in checkForValueNode, ensure that the data type of the value is the same as
    // the default value.
    if (dataTypeSet) {
        SerializationValueVariantTypeEnum type = dataTypeFromString(nodeType);
        if (type != _dataType) {
            throw std::invalid_argument(_scriptName + ": Default value and value type differ!");
        }
    } else {
        _dataType = dataTypeFromString(nodeType);
    }


    YAML::Node defNode = node[defaultTypeName];
    int nDims = defNode.IsSequence() ? defNode.size() : 1;
    _defaultValues.resize(nDims);
    for (int i = 0; i < nDims; ++i) {
        _defaultValues[i].serializeDefaultValue = true;
        YAML::Node dimNode = defNode.IsSequence() ? defNode[i] : defNode;
        decodeValueFromNode(dimNode, _defaultValues[i].value, _dataType);
    }

    return true;
}

void
KnobSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        return;
    }

    // Set the flag to true if the user use this object directly to encode after
    _mustSerialize = true;

    _scriptName = node["Name"].as<std::string>();

    // Check for nodes
    bool dataTypeSet = false;
    static const std::string typesToCheck[5] = { kKnobSerializationDataTypeKeyBool, kKnobSerializationDataTypeKeyInt, kKnobSerializationDataTypeKeyDouble, kKnobSerializationDataTypeKeyString, kKnobSerializationDataTypeKeyNone };
    for (int i = 0; i < 5; ++i) {
        if (checkForValueNode(node, typesToCheck[i])) {
            dataTypeSet = true;
            break;
        }
    }
    for (int i = 0; i < 5; ++i) {
        if (checkForDefaultValueNode(node, typesToCheck[i], dataTypeSet)) {
            break;
        }
    }


    if (node["ParametricCurves"]) {
        YAML::Node curveNode = node["ParametricCurves"];
        ParametricExtraData *data = getOrCreateExtraData<ParametricExtraData>(_extraData);
        if (curveNode.IsMap()) {
            for (YAML::const_iterator it = curveNode.begin(); it!=curveNode.end(); ++it) {
                std::string viewName = it->first.as<std::string>();
                YAML::Node curvesViewNode = it->second;

                std::list<CurveSerialization>& curvesList = data->parametricCurves[viewName];
                for (std::size_t i = 0; i < curvesViewNode.size(); ++i) {
                    CurveSerialization s;
                    s.decode(curvesViewNode[i]);
                    curvesList.push_back(s);
                }

            }
        } else {
            std::list<CurveSerialization>& curvesList = data->parametricCurves["Main"];
            for (std::size_t i = 0; i < curveNode.size(); ++i) {
                CurveSerialization s;
                s.decode(curveNode[i]);
                curvesList.push_back(s);
            }
        }

    }
    if (node["TextAnim"]) {
        YAML::Node curveNode = node["TextAnim"];
        TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
        if (curveNode.IsMap()) {
            // Multi-view
            for (YAML::const_iterator it = curveNode.begin(); it!=curveNode.end(); ++it) {
                std::string viewName = it->first.as<std::string>();
                YAML::Node keysForViewNode = it->second;

                std::map<double, std::string>& keysForView = data->keyframes[viewName];
                // If type = 0 we expect a int, otherwise a string
                int type = 0;
                std::pair<double, std::string> p;
                for (std::size_t i = 0; i < keysForViewNode.size(); ++i) {
                    if (type == 0) {
                        p.first = keysForViewNode[i].as<double>();
                        type = 1;
                    } else if (type == 1) {
                        type = 0;
                        p.second = keysForViewNode[i].as<std::string>();
                        keysForView.insert(p);
                    }
                }
            }
        } else {
            // If type = 0 we expect a int, otherwise a string
            std::map<double, std::string>& keysForView = data->keyframes["Main"];
            int type = 0;
            std::pair<double, std::string> p;
            for (std::size_t i = 0; i < curveNode.size(); ++i) {
                if (type == 0) {
                    p.first = curveNode[i].as<double>();
                    type = 1;
                } else if (type == 1) {
                    type = 0;
                    p.second = curveNode[i].as<std::string>();
                    keysForView.insert(p);
                }
            }
        }

    }

    if (node["FontColor"]) {
        YAML::Node n = node["FontColor"];
        if (n.size() != 3) {
            throw YAML::InvalidNode();
        }
        TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
        data->fontColor[0] = n[0].as<double>();
        data->fontColor[1] = n[1].as<double>();
        data->fontColor[2] = n[2].as<double>();

    }
    if (node["FontSize"]) {
        TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
        data->fontSize = node["FontSize"].as<int>();
    }

    if (node["Font"]) {
        TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
        data->fontFamily = node["Font"].as<std::string>();
    }

    if (node["NDims"]) {

        // This is a user knob
        _isUserKnob = true;

        _typeName = node["TypeName"].as<std::string>();

        _dimension = node["NDims"].as<int>();

        if (node["Label"]) {
            _label = node["Label"].as<std::string>();
        } else {
            _label = _scriptName;
        }
        if (node["Hint"]) {
            _tooltip = node["Hint"].as<std::string>();
        }

        if (node["Persistent"]) {
            _isPersistent = node["Persistent"].as<bool>();
        } else {
            _isPersistent = true;
        }
        if (node["UncheckedIcon"]) {
            _iconFilePath[0] = node["UncheckedIcon"].as<std::string>();
        }
        if (node["CheckedIcon"]) {
            _iconFilePath[1] = node["CheckedIcon"].as<std::string>();
        }

        // User specific data
        if (node["Entries"]) {
            // This is a choice
            ChoiceExtraData *data = new ChoiceExtraData;
            _extraData.reset(data);
            YAML::Node entriesNode = node["Entries"];
            for (std::size_t i = 0; i < entriesNode.size(); ++i) {
                data->_entries.push_back(entriesNode[i].as<std::string>());
            }

            // Also look for hints...
            if (node["Hints"]) {
                YAML::Node hintsNode = node["Hints"];
                for (std::size_t i = 0; i < hintsNode.size(); ++i) {
                    data->_helpStrings.push_back(hintsNode[i].as<std::string>());
                }
            }
        }

        if (node["Min"]) {
            ValueExtraData* data = getOrCreateExtraData<ValueExtraData>(_extraData);
            data->min = node["Min"].as<double>();
        }
        if (node["Max"]) {
            ValueExtraData* data = getOrCreateExtraData<ValueExtraData>(_extraData);
            data->max = node["Max"].as<double>();
        }
        if (node["DisplayMin"]) {
            ValueExtraData* data = getOrCreateExtraData<ValueExtraData>(_extraData);
            data->dmin = node["DisplayMin"].as<double>();
        }
        if (node["DisplayMax"]) {
            ValueExtraData* data = getOrCreateExtraData<ValueExtraData>(_extraData);
            data->dmax = node["DisplayMax"].as<double>();
        }
        if (node["FileTypes"]) {
            FileExtraData* data = getOrCreateExtraData<FileExtraData>(_extraData);
            YAML::Node fileTypesNode = node["FileTypes"];
            for (std::size_t i = 0; i < fileTypesNode.size(); ++i) {
                data->filters.push_back(fileTypesNode[i].as<std::string>());
            }
        }
    } // isUserKnob

    if (node["InViewerLayout"]) {
        _inViewerContextItemLayout = node["InViewerLayout"].as<std::string>();
        _hasViewerInterface = true;
    }

    if (node["InViewerSpacing"]) {
        _inViewerContextItemSpacing = node["InViewerSpacing"].as<int>();
        _hasViewerInterface = true;
    }

    if (_isUserKnob) {
        if (node["InViewerLabel"]) {
            _inViewerContextLabel = node["InViewerLabel"].as<std::string>();
            _hasViewerInterface = true;
        }
        if (node["InViewerIconUnchecked"]) {
            _inViewerContextIconFilePath[0] = node["InViewerIconUnchecked"].as<std::string>();
            _hasViewerInterface = true;
        }
        if (node["InViewerIconChecked"]) {
            _inViewerContextIconFilePath[1] = node["InViewerIconChecked"].as<std::string>();
            _hasViewerInterface = true;
        }
    }

    if (node["Props"]) {
        YAML::Node propsNode = node["Props"];
        for (std::size_t i = 0; i < propsNode.size(); ++i) {
            std::string prop = propsNode[i].as<std::string>();
            if (prop == "Secret") {
                _isSecret = true;
            } else if (prop == "Disabled") {
                _disabled = true;
            } else if (prop == "NoNewLine") {
                _triggerNewLine = false;
            } else if (prop == "NoEval") {
                _evaluatesOnChange = false;
            } else if (prop == "AnimatesChanged") {
                _animatesChanged = true;
            } else if (prop == "Volatile") {
                _isPersistent = false;
            } else if (prop == "IsLabel") {
                TextExtraData* data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->label = true;
                data->multiLine = false;
                data->richText = false;
            } else if (prop == "MultiLine") {
                TextExtraData* data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->label = false;
                data->multiLine = true;
                data->richText = false;
            } else if (prop == "RichText") {
                TextExtraData* data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->richText = true;
            } else if (prop == "MultiPath") {
                PathExtraData* data = getOrCreateExtraData<PathExtraData>(_extraData);
                data->multiPath = node["MultiPath"].as<bool>();
            } else if (prop == "Sequences") {
                FileExtraData* data = getOrCreateExtraData<FileExtraData>(_extraData);
                data->useSequences = node["Sequences"].as<bool>();
            } else if (prop == "ExistingFiles") {
                FileExtraData* data = getOrCreateExtraData<FileExtraData>(_extraData);
                data->useExistingFiles = node["ExistingFiles"].as<bool>();
            } else if (prop == "UseOverlay") {
                TypeExtraData* data = getOrCreateExtraData<TypeExtraData>(_extraData);
                data->useHostOverlayHandle = true;
            } else if (prop == "Italic") {
                TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->italicActivated = true;
            } else if (prop == "Bold") {
                TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->boldActivated = true;
            } else {
                std::cerr << "WARNING: Unrecognized parameter property " << prop << std::endl;
            }


        }

    }
    
} // KnobSerialization::decode


void
GroupKnobSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;

    em << YAML::Key << "TypeName" << YAML::Value << _typeName;
    em << YAML::Key << "Name" << YAML::Value << _name;
    if (_label != _name) {
        em << YAML::Key << "Label" << YAML::Value << _label;
    }

    if (!_children.empty()) {
        em << YAML::Key << "Params" << YAML::Value << YAML::BeginSeq;
        for (std::list <boost::shared_ptr<KnobSerializationBase> >::const_iterator it = _children.begin(); it!=_children.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }

    
    
    std::list<std::string> props;
    if (_isOpened) {
        props.push_back("Opened");
    }
    if (_isSetAsTab) {
        props.push_back("IsTab");
    }
    if (_secret) {
        props.push_back("Secret");
    }
    if (!props.empty()) {
        em << YAML::Key << "Props" << YAML::Value << YAML::BeginSeq;
        for (std::list<std::string>::const_iterator it = props.begin(); it!=props.end(); ++it) {
            em << *it;
        }
        em << YAML::EndSeq;
    }
    em << YAML::EndMap;
} // GroupKnobSerialization::encode

void
GroupKnobSerialization::decode(const YAML::Node& node)
{
    _typeName = node["TypeName"].as<std::string>();
    _name = node["Name"].as<std::string>();
    if (node["Label"]) {
        _label = node["Label"].as<std::string>();
    } else {
        _label = _name;
    }

    if (node["Params"]) {
        YAML::Node paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {

            std::string typeName;
            if (paramsNode[i]["TypeName"]) {
                typeName = paramsNode[i]["TypeName"].as<std::string>();
            }

            if (typeName == kKnobPageTypeName || typeName == kKnobGroupTypeName) {
                GroupKnobSerializationPtr s(new GroupKnobSerialization);
                s->decode(paramsNode[i]);
                _children.push_back(s);
            } else {
                KnobSerializationPtr s (new KnobSerialization);
                s->decode(paramsNode[i]);
                _children.push_back(s);
            }
        }
    }

    if (node["Props"]) {
        YAML::Node propsNode = node["Props"];
        for (std::size_t i = 0; i < propsNode.size(); ++i) {
            std::string prop = propsNode[i].as<std::string>();
            if (prop == "Opened") {
                _isOpened = true;
            } else if (prop == "Secret") {
                _secret = true;
            } else if (prop == "IsTab") {
                _isSetAsTab = true;
            }
        }
    }
} // GroupKnobSerialization::decode

SERIALIZATION_NAMESPACE_EXIT;
