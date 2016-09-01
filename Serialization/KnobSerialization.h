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

#ifndef KNOBSERIALIZATION_H
#define KNOBSERIALIZATION_H

#include <map>
#include <vector>


#include "Serialization/CurveSerialization.h"
#include "Serialization/SerializationBase.h"


#define kInViewerContextItemLayoutSpacing "Spacing"
#define kInViewerContextItemLayoutStretchBefore "StretchB"
#define kInViewerContextItemLayoutStretchAfter "StretchA"
#define kInViewerContextItemLayoutNewLine "NewLine"
#define kInViewerContextItemLayoutAddSeparator "Separator"

#define kKnobIntTypeName "Int"
#define kKnobDoubleTypeName "Double"
#define kKnobColorTypeName "Color"
#define kKnobBoolTypeName "Bool"
#define kKnobStringTypeName "String"
#define kKnobButtonTypeName "Button"
#define kKnobChoiceTypeName "Choice"
#define kKnobSeparatorTypeName "Separator"
#define kKnobGroupTypeName "Group"
#define kKnobPageTypeName "Page"
#define kKnobParametricTypeName "Parametric"
#define kKnobLayersTypeName "Layers"
#define kKnobFileTypeName "InputFile"
#define kKnobOutputFileTypeName "OutputFile"
#define kKnobPathTypeName "Path"

#define kKnobInViewerContextDefaultItemSpacing 5
#define kKnobStringDefaultFontSize 11

SERIALIZATION_NAMESPACE_ENTER;

/**
 User knobs are serialized in a different manner. For them we also need to remember all layouts bits as well
 as the page/group hierarchy. In order to do so, we just serialize the top level user pages using the
 GroupKnobSerialization class which will in turn serialize by itself all its knobs recursively.
 To deserialize user knobs, the caller should loop over the user page script-names and call
 for each GroupKnobSerialization corresponding to a user page. When deserializing a user
 page or group knob, it will in turn create all knobs recursively.
 
 Finally to restore knob links (slave/master and expression) this should be done once all nodes in a group
 have been created (since links require master knobs to be created). 
 Once all nodes have been created, loop over all plug-in knobs as well as all user pages and call
 restoreKnobLinks, passing it in parameter a list of all nodes in the group (including the group node itself if this is a Group node).
 **/


/**
 * @brief Small struct encapsulating the serialization needed to restore master/slave links for knobs.
 **/
struct MasterSerialization
{
    int masterDimension; // The dimension the knob is slaved to in the master knob
    std::string masterNodeName; // the node script-name holding this knob
    std::string masterTrackName; // if the master knob is part of a track this is the track name
    std::string masterKnobName; // the script-name of the master knob

    MasterSerialization()
        : masterDimension(-1)
        , masterNodeName()
        , masterTrackName()
        , masterKnobName()
    {
    }
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};

/**
 * @brief A small variant class to serialize the internal data type of knobs without using templates
 **/
struct SerializationValueVariant
{

    bool isBool;
    int isInt;
    double isDouble;
    std::string isString;

    SerializationValueVariant()
    : isBool(false)
    , isInt(0)
    , isDouble(0.)
    , isString()
    {}
};

/**
 * @brief Base class for any type-specific extra data
 **/
class TypeExtraData
{
public:

    // User knob only
    bool useHostOverlayHandle;

    TypeExtraData()
    : useHostOverlayHandle(false)
    {}

    virtual ~TypeExtraData() {}
};

class ChoiceExtraData
    : public TypeExtraData
{
public:


    ChoiceExtraData()
        : TypeExtraData(), _entries(), _helpStrings() {}

    virtual ~ChoiceExtraData() {}

    // Entries and help strings are only serialized for user knobs
    std::vector<std::string> _entries;
    std::vector<std::string> _helpStrings;
};


class FileExtraData
    : public TypeExtraData
{
public:
    FileExtraData()
        : TypeExtraData(), useSequences(false) {}

    // User knob only
    bool useSequences;
};

class PathExtraData
    : public TypeExtraData
{
public:
    PathExtraData()
        : TypeExtraData(), multiPath(false) {}

    // User knob only
    bool multiPath;
};

class TextExtraData
    : public TypeExtraData
{
public:
    TextExtraData()
    : TypeExtraData()
    , label(false)
    , multiLine(false)
    , richText(false)
    , fontSize(kKnobStringDefaultFontSize)
    , fontFamily(NATRON_FONT)
    , italicActivated(false)
    , boldActivated(false)
    , fontColor()
    , keyframes()
    {
        fontColor[0] = fontColor[1] = fontColor[2] = 0.;
    }

    // User knob only
    bool label;
    bool multiLine;
    bool richText;

    // Non user knob only
    int fontSize;
    std::string fontFamily;
    bool italicActivated;
    bool boldActivated;
    double fontColor[3];


    // Animation keyframes
    std::map<int, std::string> keyframes;

};

class ValueExtraData
    : public TypeExtraData
{
public:
    ValueExtraData()
        : TypeExtraData()
        , min(0.)
        , max(0.)
        , dmin(0.)
        , dmax(0.)
    {
    }

    // User knob only
    double min;
    double max;
    double dmin;
    double dmax;
};


class ParametricExtraData
: public TypeExtraData
{
public:

    ParametricExtraData()
    : TypeExtraData()
    , parametricCurves()
    {

    }

    std::list<CurveSerialization> parametricCurves;
};


class KnobSerializationBase;

/**
 * @brief This class is used by KnobSerialization itself
 **/
struct ValueSerialization
{
    // Enum indicating what's the data-type saved by the Knob
    enum SerializationValueVariantTypeEnum
    {
        eSerializationValueVariantTypeNone,
        eSerializationValueVariantTypeBoolean,
        eSerializationValueVariantTypeInteger,
        eSerializationValueVariantTypeDouble,
        eSerializationValueVariantTypeString

    };

    // If true, at least one of the fields below needs to be serialized
    bool _mustSerialize;

    int _dimension;

    // Pointer to the actual KnobSerialization
    KnobSerializationBase* _serialization;

    // Control which type member holds the current value of the variant
    SerializationValueVariantTypeEnum _type;

    // Serialization of the value and default value as a variant
    SerializationValueVariant _value, _defaultValue;

    // Whether we should save the value and default value
    bool _serializeValue, _serializeDefaultValue;

    // Serialization of the animation curve
    CurveSerialization _animationCurve;

    // Expression
    std::string _expression;
    bool _expresionHasReturnVariable;

    // Slave/Master link
    MasterSerialization _slaveMasterLink;

    // Enabled state is serialized per-dimension
    bool _enabledChanged;

    // Just used for backward compatibility when loading with boost, do not use
    std::string _typeName;

    ValueSerialization()
    : _mustSerialize(false)
    , _dimension(-1)
    , _serialization(0)
    , _type(eSerializationValueVariantTypeNone)
    , _value()
    , _defaultValue()
    , _serializeValue(false)
    , _serializeDefaultValue(false)
    , _animationCurve()
    , _expression()
    , _expresionHasReturnVariable(false)
    , _slaveMasterLink()
    , _enabledChanged(false)
    , _typeName()
    {
        
    }


    const std::string& getKnobName() const;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};



class KnobSerializationBase : public SerializationObjectBase
{
public:

    KnobSerializationBase() {}

    virtual ~KnobSerializationBase() {}


    virtual const std::string& getName() const = 0;

    virtual const std::string& getTypeName() const = 0;


};

class KnobSerialization
    : public KnobSerializationBase
{

public:

    std::string _typeName; // used for user knobs only to re-create the appropriate knob type
    std::string _scriptName; // the unique script-name of the knob
    int _dimension; // the number of dimensions held by the knob
    bool _visibilityChanged; // true if the secretness of the knob is different than default
    bool _masterIsAlias; // is the master/slave link an alias ?
    std::vector<ValueSerialization> _values; // serialized value for each dimension
    boost::scoped_ptr<TypeExtraData> _extraData; // holds type specific data other than values

    bool _isUserKnob; // is this knob created by the user or was this created by a plug-in ?
    std::string _label; // only serialized for user knobs
    std::string _iconFilePath[2]; // only serialized for user knobs
    bool _triggerNewLine; // only serialized for user knobs
    bool _evaluatesOnChange; // only serialized for user knobs
    bool _isPersistent; // only serialized for user knobs
    bool _animatesChanged; // only serialized for user knobs
    bool _hasViewerInterface; // does this knob has a user interface ?
    bool _inViewerContextSecret; // is this knob secret in the viewer interface
    int _inViewerContextItemSpacing; // item spacing in viewer, serialized for all knobs if they have a viewer UI
    std::string _inViewerContextItemLayout; // item layout in viewer, serialized for all knobs if they have a viewer UI
    std::string _inViewerContextIconFilePath[2]; // only serialized for user knobs with a viewer interface
    std::string _inViewerContextLabel; // only serialized for user knobs with a viewer interface
    std::string _tooltip; // serialized for user knobs only

    // True if the knob has been changed since its default state or if this is a user knob
    bool _mustSerialize;


    explicit KnobSerialization()
    : _typeName()
    , _scriptName()
    , _dimension(0)
    , _visibilityChanged(false)
    , _masterIsAlias(false)
    , _values()
    , _extraData()
    , _isUserKnob(false)
    , _label()
    , _triggerNewLine(false)
    , _evaluatesOnChange(false)
    , _isPersistent(true)
    , _animatesChanged(false)
    , _hasViewerInterface(false)
    , _inViewerContextSecret(false)
    , _inViewerContextItemSpacing(kKnobInViewerContextDefaultItemSpacing)
    , _inViewerContextItemLayout()
    , _inViewerContextIconFilePath()
    , _inViewerContextLabel()
    , _tooltip()
    , _mustSerialize(false)
    {

    }

    virtual ~KnobSerialization() {}

    virtual const std::string& getName() const OVERRIDE FINAL
    {
        return _scriptName;
    }

    virtual const std::string& getTypeName() const OVERRIDE FINAL
    {
        return _typeName;
    }

    virtual void encode(YAML_NAMESPACE::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML_NAMESPACE::Node& node) OVERRIDE;


    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};


/**
 * @brief Used by Groups and Pages for serialization of User knobs, to maintain their layout.
 **/
class GroupKnobSerialization
    : public KnobSerializationBase
{
public:

    std::list <boost::shared_ptr<KnobSerializationBase> > _children;
    std::string _typeName;
    std::string _name, _label;
    bool _secret;
    bool _isSetAsTab; //< only valid for groups
    bool _isOpened; //< only for groups


    GroupKnobSerialization()
    : _children()
    , _typeName()
    , _name()
    , _label()
    , _secret(false)
    , _isSetAsTab(false)
    , _isOpened(false)
    {

    }

    virtual ~GroupKnobSerialization()
    {
    }

    virtual const std::string& getName() const OVERRIDE FINAL
    {
        return _name;
    }

    virtual const std::string& getTypeName() const OVERRIDE FINAL
    {
        return _typeName;
    }

    virtual void encode(YAML_NAMESPACE::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML_NAMESPACE::Node& node) OVERRIDE;


    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

SERIALIZATION_NAMESPACE_EXIT;


#endif // KNOBSERIALIZATION_H
