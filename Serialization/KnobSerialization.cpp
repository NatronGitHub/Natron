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


#include <cfloat>
#include <climits>
#include <iostream>

#include "KnobSerialization.h"

#include "Serialization/CurveSerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON


SERIALIZATION_NAMESPACE_ENTER;

const std::string&
ValueSerialization::getKnobName() const
{
    return _serialization->getName();
}


void
KnobSerialization::encode(YAML::Emitter& em) const
{
    if (!_mustSerialize) {
        return;
    }
    em << YAML::BeginMap;
    em << YAML::Key << "ScriptName" << YAML::Value << _scriptName;

    // Instead of inserting in the map each boolean
    // we serialize strings, meaning that
    // if they are here, their value is true otherwise it goes to default
    std::list<std::string> propNames;
    if (_visibilityChanged && _isPersistent) {
        propNames.push_back("VisibilityChanged");
    }
    if (_masterIsAlias) {
        propNames.push_back("MasterIsAlias");
    }
    assert((int)_values.size() == _dimension);

    bool hasDimensionToSerialize = false;
    for (std::size_t i = 0; i < _values.size(); ++i) {
        if (_values[i]._mustSerialize && _isPersistent) {
            hasDimensionToSerialize = true;
            break;
        }
    }
    if (hasDimensionToSerialize) {
        // We serialize a value, we need to know which type of knob this is because at the time of deserialization the
        // application may not have created the corresponding knob already and may not know to what type it corresponds
        em << YAML::Key << "Dimensions" << YAML::Value;
        // Starting dimensions
        em << YAML::BeginSeq;
        for (std::size_t i = 0; i < _values.size(); ++i) {
            const ValueSerialization& s = _values[i];

            if (!s._mustSerialize) {
                continue;
            }

            em << YAML::Flow << YAML::BeginMap;

            std::list<std::string> dimensionProps;
            em << YAML::Key << "Index" << YAML::Value << s._dimension;

            if (s._serializeValue && s._animationCurve.keys.empty()) {
                em << YAML::Key << "Value" << YAML::Value;
                switch (s._type) {
                    case ValueSerialization::eSerializationValueVariantTypeBoolean:
                        em << s._value.isBool;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeInteger:
                        em << s._value.isInt;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeDouble:
                        em << s._value.isDouble;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeString:
                        em << s._value.isString;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeNone:
                        break;
                }
            }
            if (s._serializeDefaultValue) {
                em << YAML::Key << "Default" << YAML::Value;
                switch (s._type) {
                    case ValueSerialization::eSerializationValueVariantTypeBoolean:
                        em << s._defaultValue.isBool;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeInteger:
                        em << s._defaultValue.isInt;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeDouble:
                        em << s._defaultValue.isDouble;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeString:
                        em << s._defaultValue.isString;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeNone:
                        break;
                }
            }
            if (!s._animationCurve.keys.empty()) {
                // Serialize only if non empty
                em << YAML::Key << "Curve" << YAML::Value;
                s._animationCurve.encode(em);
            }
            if (!s._expression.empty()) {
                // Serialize only if non empty
                em << YAML::Key << "Expr" << YAML::Value << s._expression;
                if (s._expresionHasReturnVariable) {
                    // Serialize only if true because otherwise we assume false by default
                    dimensionProps.push_back("ExprHasRet");
                }
            }
            if (s._enabledChanged) {
                dimensionProps.push_back("EnabledChanged");
            }
            if (!s._slaveMasterLink.masterKnobName.empty()) {
                em << YAML::Key << "Master" << YAML::Value << YAML::Flow << YAML::BeginSeq;
                em << s._slaveMasterLink.masterKnobName;
                em << s._slaveMasterLink.masterDimension;
                em << s._slaveMasterLink.masterNodeName;
                if (!s._slaveMasterLink.masterTrackName.empty()) {
                    em << s._slaveMasterLink.masterTrackName;
                }
                em << YAML::EndSeq;
            }
            if (!dimensionProps.empty()) {
                em << YAML::Key << "DimProps" << YAML::Value << YAML::Flow << YAML::BeginSeq;
                for (std::list<std::string>::const_iterator it2 = dimensionProps.begin(); it2 != dimensionProps.end(); ++it2) {
                    em << *it2;
                }
                em << YAML::EndSeq;
            }

            em << YAML::EndMap;
        } // for each dimension
        em << YAML::EndSeq;
    } // hasDimensionToSerialize

    TextExtraData* textData = dynamic_cast<TextExtraData*>(_extraData.get());
    if (_extraData && _isPersistent) {
        ParametricExtraData* parametricData = dynamic_cast<ParametricExtraData*>(_extraData.get());
        if (parametricData) {
            if (!parametricData->parametricCurves.empty()) {
                em << YAML::Key << "ParametricCurves" << YAML::Value;
                em << YAML::BeginSeq;
                for (std::list<CurveSerialization>::const_iterator it = parametricData->parametricCurves.begin(); it!=parametricData->parametricCurves.end(); ++it) {
                    it->encode(em);
                }
                em << YAML::EndSeq;
            }
        } else if (textData) {
            if (!textData->keyframes.empty()) {
                em << YAML::Key << "TextAnim" << YAML::Value;
                em << YAML::Flow;
                em << YAML::BeginSeq;
                for (std::map<int, std::string>::const_iterator it = textData->keyframes.begin(); it!=textData->keyframes.end(); ++it) {
                    em << it->first << it->second;
                }
                em << YAML::EndSeq;
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
                em << YAML::BeginSeq;
                for (std::size_t i = 0; i < cdata->_entries.size(); ++i) {
                    em << cdata->_entries[i];
                }
                em << YAML::EndSeq;
            }
            if (!cdata->_helpStrings.empty()) {
                em << YAML::Key << "Hints" << YAML::Value;
                em << YAML::BeginSeq;
                for (std::size_t i = 0; i < cdata->_helpStrings.size(); ++i) {
                    em << cdata->_helpStrings[i];
                }
                em << YAML::EndSeq;
            }
        } else if (vdata) {
            if (vdata->min != INT_MIN && vdata->min != -DBL_MAX) {
                em << YAML::Key << "Min" << YAML::Value;
            }
            if (vdata->min != INT_MAX && vdata->min != DBL_MAX) {
                em << YAML::Key << "Max" << YAML::Value;
            }
            if (vdata->min != INT_MIN && vdata->min != -DBL_MAX) {
                em << YAML::Key << "DisplayMin" << YAML::Value;
            }
            if (vdata->min != INT_MAX && vdata->min != DBL_MAX) {
                em << YAML::Key << "DisplayMax" << YAML::Value;
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
        }

        if (typeData) {
            if (typeData->useHostOverlayHandle) {
                propNames.push_back("UseOverlay");
            }
        }

    } // if (_isUserKnob) {

    if (_hasViewerInterface) {
        if (_inViewerContextItemLayout != kInViewerContextItemLayoutSpacing) {
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
                                SerializationValueVariant& variant)
{
    /*
     For non user knobs, we do not need to store the typename of the Knob. It's either a string
     or a double (which can be cast away to int or bool).
     */
    try {
        variant.isString = node.as<std::string>();
    } catch (const YAML::BadConversion& /*e*/) {
        try {
            variant.isDouble = node.as<double>();
            variant.isInt = (int)variant.isDouble;
            variant.isBool = (bool)variant.isInt;
        } catch (const YAML::BadConversion& /*e*/) {
            try {
                variant.isInt = node.as<int>();
                variant.isBool = (bool)variant.isInt;
                variant.isDouble = (bool)variant.isInt;
            } catch (const YAML::BadConversion& /*e*/) {
                variant.isBool = node.as<bool>();
                variant.isInt = (int)variant.isBool;
                variant.isDouble = (double)variant.isInt;
            }
        }
    }

}

void
KnobSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        return;
    }

    _scriptName = node["ScriptName"].as<std::string>();


    if (node["Dimensions"]) {

        YAML::Node dimensionsNode = node["Dimensions"];
        if (!dimensionsNode.IsSequence()) {
            throw YAML::InvalidNode();
        }
        _values.resize(dimensionsNode.size());
        for (std::size_t i = 0; i < _values.size(); ++i) {
            _values[i]._serialization = this;
            _values[i]._dimension = i;
        }
        for (YAML::const_iterator it = dimensionsNode.begin(); it!=dimensionsNode.end(); ++it) {

            if (!it->second.IsMap()) {
                throw YAML::InvalidNode();
            }
            int dim = it->second["Index"].as<int>();
            if (dim < 0 || dim >= (int)_values.size()) {
                throw YAML::BadSubscript();
            }
            ValueSerialization& s = _values[dim];
            s._dimension = dim;
            if (it->second["Value"]) {
                YAML::Node valueNode = it->second["Value"];
                decodeValueFromNode(valueNode, s._value);
            }
            if (it->second["Default"]) {
                YAML::Node valueNode = it->second["Default"];
                decodeValueFromNode(valueNode, s._defaultValue);
            }
            if (it->second["Curve"]) {
                YAML::Node curveNode = it->second["Curve"];
                s._animationCurve.decode(curveNode);
            }
            if (it->second["Expr"]) {
                s._expression = it->second["Expr"].as<std::string>();
        
            }
            if (it->second["Master"]) {
                YAML::Node masterNode = it->second["Master"];
                if (!masterNode.IsSequence() || (masterNode.size() != 3 && masterNode.size() != 4)) {
                    throw YAML::InvalidNode();
                }

                s._slaveMasterLink.masterKnobName = masterNode[0].as<std::string>();
                s._slaveMasterLink.masterDimension = masterNode[1].as<int>();
                s._slaveMasterLink.masterNodeName = masterNode[2].as<std::string>();
                if (masterNode.size() == 4) {
                    s._slaveMasterLink.masterTrackName = masterNode[3].as<std::string>();
                }


            }

            if (it->second["DimProps"]) {
                YAML::Node propsNode = it->second["DimProps"];
                for (YAML::const_iterator it2 = propsNode.begin(); it2 != propsNode.end(); ++it2) {
                    std::string prop = it2->second.as<std::string>();
                    if (prop == "EnabledChanged") {
                        s._enabledChanged = true;
                    } else if (prop == "ExprHasRet") {
                        s._expresionHasReturnVariable = true;
                    } else {
                        assert(false);
                        std::cerr << "WARNING: Unknown parameter property " << prop << std::endl;
                    }
                }
            }

        } // for all serialized dimensions
    } // has dimension serialized

    if (node["ParametricCurves"]) {
        YAML::Node curveNode = node["ParametricCurves"];
        ParametricExtraData *data = new ParametricExtraData;
        _extraData.reset(data);
        for (YAML::const_iterator it = curveNode.begin(); it!=curveNode.end(); ++it) {
            CurveSerialization s;
            s.decode(it->second);
            data->parametricCurves.push_back(s);
        }
    }
    if (node["TextAnim"]) {
        YAML::Node curveNode = node["TextAnim"];
        TextExtraData *data = new TextExtraData;
        _extraData.reset(data);
        // If type = 0 we expect a int, otherwise a string
        int type = 0;
        std::pair<int, std::string> p;
        for (YAML::const_iterator it = curveNode.begin(); it!=curveNode.end(); ++it) {
            if (type == 0) {
                p.first = it->second.as<int>();
                type = 1;
            } else if (type == 1) {
                type = 0;
                p.second = it->second.as<std::string>();
                data->keyframes.insert(p);
            }
        }
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
            for (YAML::const_iterator it = entriesNode.begin(); it!=entriesNode.end(); ++it) {
                data->_entries.push_back(it->second.as<std::string>());
            }

            // Also look for hints...
            if (node["Hints"]) {
                YAML::Node hintsNode = node["Hints"];
                for (YAML::const_iterator it = hintsNode.begin(); it!=hintsNode.end(); ++it) {
                    data->_helpStrings.push_back(it->second.as<std::string>());
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
        for (YAML::const_iterator it2 = propsNode.begin(); it2 != propsNode.end(); ++it2) {
            std::string prop = it2->second.as<std::string>();
            if (prop == "VisibilityChanged") {
                _visibilityChanged = true;
            } else if (prop == "MasterIsAlias") {
                _masterIsAlias = true;
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
            } else if (prop == "UseOverlay") {
                TypeExtraData* data = getOrCreateExtraData<TypeExtraData>(_extraData);
                data->useHostOverlayHandle = true;
            } else {
                assert(false);
                std::cerr << "WARNING: Unrecognized parameter property " << prop << std::endl;
            }
        }

    }
    
} // KnobSerialization::decode


void
GroupKnobSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::VerbatimTag("GroupParam");
    em << YAML::BeginMap;

    em << YAML::Key << "TypeName" << YAML::Value << _typeName;
    em << YAML::Key << "ScriptName" << YAML::Value << _name;
    if (_label != _name) {
        em << YAML::Key << "Label" << YAML::Value << _label;
    }

    em << YAML::Key << "Params" << YAML::Value << YAML::BeginSeq;
    for (std::list <boost::shared_ptr<KnobSerializationBase> >::const_iterator it = _children.begin(); it!=_children.end(); ++it) {
        (*it)->encode(em);
    }
    em << YAML::EndSeq;



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
    _name = node["ScriptName"].as<std::string>();
    if (node["Label"]) {
        _label = node["Label"].as<std::string>();
    } else {
        _label = _name;
    }

    YAML::Node paramsNode = node["Params"];
    for (YAML::const_iterator it = paramsNode.begin(); it!=paramsNode.end(); ++it) {
        if (it->second.Tag() != "GroupParam") {
            GroupKnobSerializationPtr s(new GroupKnobSerialization);
            s->decode(it->second);
            _children.push_back(s);
        } else {
            KnobSerializationPtr s (new KnobSerialization);
            s->decode(it->second);
            _children.push_back(s);
        }
    }

    if (node["Props"]) {
        YAML::Node propsNode = node["Props"];
        for (YAML::const_iterator it2 = propsNode.begin(); it2 != propsNode.end(); ++it2) {
            std::string prop = it2->second.as<std::string>();
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
