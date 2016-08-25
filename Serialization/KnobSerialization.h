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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <vector>


#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#ifndef Q_MOC_RUN
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(sign-compare)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/scoped_ptr.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_ON(unused-parameter)
GCC_DIAG_ON(sign-compare)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif
#endif


#include "Serialization/CurveSerialization.h"
#include "Serialization/SerializationBase.h"


#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#define KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS 2
#define KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS_OFFSET 3
#define KNOB_SERIALIZATION_INTRODUCES_CHOICE_LABEL 4
#define KNOB_SERIALIZATION_INTRODUCES_USER_KNOB 5
#define KNOB_SERIALIZATION_NODE_SCRIPT_NAME 6
#define KNOB_SERIALIZATION_INTRODUCES_NATIVE_OVERLAYS 7
#define KNOB_SERIALIZATION_INTRODUCES_CHOICE_HELP_STRINGS 8
#define KNOB_SERIALIZATION_INTRODUCES_DEFAULT_VALUES 9
#define KNOB_SERIALIZATION_INTRODUCES_DISPLAY_MIN_MAX 10
#define KNOB_SERIALIZATION_INTRODUCES_ALIAS 11
#define KNOB_SERIALIZATION_REMOVE_SLAVED_TRACKS 12
#define KNOB_SERIALIZATION_REMOVE_DEFAULT_VALUES 13
#define KNOB_SERIALIZATION_INTRODUCE_VIEWER_UI 14
#define KNOB_SERIALIZATION_INTRODUCE_USER_KNOB_ICON_FILE_PATH 15
#define KNOB_SERIALIZATION_CHANGE_CURVE_SERIALIZATION 16
#define KNOB_SERIALIZATION_VERSION KNOB_SERIALIZATION_CHANGE_CURVE_SERIALIZATION

#define VALUE_SERIALIZATION_INTRODUCES_CHOICE_LABEL 2
#define VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS 3
#define VALUE_SERIALIZATION_REMOVES_EXTRA_DATA 4
#define VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS_RESULTS 5
#define VALUE_SERIALIZATION_REMOVES_EXPRESSIONS_RESULTS 6
#define VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES 7
#define VALUE_SERIALIZATION_CHANGES_CURVE_SERIALIZATION 8
#define VALUE_SERIALIZATION_INTRODUCES_DATA_TYPE 9
#define VALUE_SERIALIZATION_VERSION VALUE_SERIALIZATION_INTRODUCES_DATA_TYPE

#define MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME 2
#define MASTER_SERIALIZATION_VERSION MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME

#define GROUP_KNOB_SERIALIZATION_INTRODUCES_TYPENAME 2
#define GROUP_KNOB_SERIALIZATION_VERSION GROUP_KNOB_SERIALIZATION_INTRODUCES_TYPENAME
#endif

NATRON_NAMESPACE_ENTER;

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
#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & ::boost::serialization::make_nvp("MasterDimension", masterDimension);
        ar & ::boost::serialization::make_nvp("MasterNodeName", masterNodeName);
        ar & ::boost::serialization::make_nvp("MasterKnobName", masterKnobName);
        ar & ::boost::serialization::make_nvp("MasterTrackName", masterTrackName);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        ar & ::boost::serialization::make_nvp("MasterDimension", masterDimension);
        ar & ::boost::serialization::make_nvp("MasterNodeName", masterNodeName);
        ar & ::boost::serialization::make_nvp("MasterKnobName", masterKnobName);

        if (version >= MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME) {
            ar & ::boost::serialization::make_nvp("MasterTrackName", masterTrackName);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif
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

    TypeExtraData() {}

    virtual ~TypeExtraData() {}
};

class ChoiceExtraData
    : public TypeExtraData
{
public:


    ChoiceExtraData()
        : TypeExtraData(), _choiceString(), _entries(), _helpStrings() {}

    virtual ~ChoiceExtraData() {}

     // For choice params, we save the choice string, as it might not necessarily exist in the entries when restoring
    std::string _choiceString;

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
        : TypeExtraData(), label(false),  multiLine(false), richText(false), keyframes() {}

    // User knob only
    bool label;
    bool multiLine;
    bool richText;

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


class DoubleExtraData
: public ValueExtraData
{
public:
    DoubleExtraData()
    : ValueExtraData()
    , useHostOverlayHandle(false)
    {}

    // User knob only
    bool useHostOverlayHandle;

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

/**
 * @brief Contains all info held by a single dimension in the knob
 **/
struct ValueSerializationStorage
{
    enum SerializationValueVariantTypeEnum
    {
        eSerializationValueVariantTypeNone,
        eSerializationValueVariantTypeBoolean,
        eSerializationValueVariantTypeInteger,
        eSerializationValueVariantTypeDouble,
        eSerializationValueVariantTypeString

    };

    // Control which type member holds the current value of the variant
    SerializationValueVariantTypeEnum type;


    SerializationValueVariant value,defaultValue;
    boost::shared_ptr<CurveSerialization> animationCurve;
    std::string expression;
    bool expresionHasReturnVariable;
    MasterSerialization slaveMasterLink;

    // Enabled state is serialized per-dimension
    bool enabled;

    ValueSerializationStorage();
};

class KnobSerializationBase;

/**
 * @brief This class is deprecated and just used for backward compatibility for loading projects older than Natron 2.2.0.
 **/
struct ValueSerialization
{
    int _version;
    KnobSerializationBase* _serialization;
    std::string _typeName;
    int _dimension;
    
    ValueSerializationStorage _value;


    ValueSerialization(KnobSerializationBase* serialization,
                       const std::string& typeName,
                       int dimension = -1);

    // For backward compatibility when the label was not yet in the ChoiceExtraData class
    void setChoiceExtraLabel(const std::string& label);

    const std::string& getKnobName() const;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {

        bool enabled = _value.enabled;
        ar & ::boost::serialization::make_nvp("Enabled", enabled);
        bool hasAnimation = _value.animationCurve.get() != 0;
        ar & ::boost::serialization::make_nvp("HasAnimation", hasAnimation);

        if (hasAnimation) {
            ar & ::boost::serialization::make_nvp( "Curve", *_value.animationCurve);
        }

        ar & ::boost::serialization::make_nvp("DataType", _value.type);

        switch (_value.type) {
            case ValueSerializationStorage::eSerializationValueVariantTypeNone:
                break;
            case ValueSerializationStorage::eSerializationValueVariantTypeInteger:
                ar & ::boost::serialization::make_nvp("Value", _value.value.isInt);
                ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isInt);
                break;
            case ValueSerializationStorage::eSerializationValueVariantTypeDouble:
                ar & ::boost::serialization::make_nvp("Value", _value.value.isDouble);
                ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isDouble);
                break;
            case ValueSerializationStorage::eSerializationValueVariantTypeString:
                ar & ::boost::serialization::make_nvp("Value", _value.value.isString);
                ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isString);
                break;
            case ValueSerializationStorage::eSerializationValueVariantTypeBoolean:
                ar & ::boost::serialization::make_nvp("Value", _value.value.isBool);
                ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isBool);
                break;
        }

        bool hasMaster = _value.slaveMasterLink.masterDimension != -1;
        ar & ::boost::serialization::make_nvp("HasMaster", hasMaster);
        if (hasMaster) {
            ar & ::boost::serialization::make_nvp("Master", _value.slaveMasterLink);
        }

        ar & ::boost::serialization::make_nvp("Expression", _value.expression);
        ar & ::boost::serialization::make_nvp("ExprHasRet", _value.expresionHasReturnVariable);
    } // save

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        _version = version;

        bool isFile = _typeName == KnobFile::typeNameStatic();
        bool isChoice = _typeName == KnobChoice::typeNameStatic();

        ar & ::boost::serialization::make_nvp("Enabled", _value.enabled);

        bool hasAnimation;
        ar & ::boost::serialization::make_nvp("HasAnimation", hasAnimation);
        bool convertOldFileKeyframesToPattern = false;
        if (hasAnimation) {
            _value.animationCurve.reset(new CurveSerialization);
            if (version >= VALUE_SERIALIZATION_CHANGES_CURVE_SERIALIZATION) {
                ar & ::boost::serialization::make_nvp("Curve", *_value.animationCurve);
            } else {
                Curve c;
                ar & ::boost::serialization::make_nvp("Curve", c);
                c.saveSerialization(_value.animationCurve.get());
            }
            convertOldFileKeyframesToPattern = isFile && getKnobName() == kOfxImageEffectFileParamName;
        }

        _value.type = ValueSerializationStorage::eSerializationValueVariantTypeNone;
        if (version >= VALUE_SERIALIZATION_INTRODUCES_DATA_TYPE) {
            ar & ::boost::serialization::make_nvp("DataType", _value.type);
        } else {
            bool isString = _typeName == KnobString::typeNameStatic();
            bool isColor = _typeName == KnobColor::typeNameStatic();
            bool isDouble = _typeName == KnobDouble::typeNameStatic();
            bool isInt = _typeName == KnobInt::typeNameStatic();
            bool isBool = _typeName == KnobBool::typeNameStatic();

            if (isInt) {
                _value.type = ValueSerializationStorage::eSerializationValueVariantTypeInteger;
            } else if (isDouble || isColor) {
                _value.type = ValueSerializationStorage::eSerializationValueVariantTypeDouble;
            } else if (isBool) {
                _value.type = ValueSerializationStorage::eSerializationValueVariantTypeBoolean;
            } else if (isString) {
                _value.type = ValueSerializationStorage::eSerializationValueVariantTypeString;
            }

            if (isChoice) {
                _value.type = ValueSerializationStorage::eSerializationValueVariantTypeInteger;
                ar & ::boost::serialization::make_nvp("Value", _value.value.isInt);
                assert(_value.value.isInt >= 0);
                if (version >= VALUE_SERIALIZATION_INTRODUCES_CHOICE_LABEL) {
                    if (version < VALUE_SERIALIZATION_REMOVES_EXTRA_DATA) {
                        std::string label;
                        ar & ::boost::serialization::make_nvp("Label", label);
                        setChoiceExtraLabel(label);
                    }
                }
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isInt);
                }
            } else if (isFile) {
                _value.type = ValueSerializationStorage::eSerializationValueVariantTypeString;
                ar & ::boost::serialization::make_nvp("Value", _value.value.isString);

                ///Convert the old keyframes stored in the file parameter by analysing one keyframe
                ///and deducing the pattern from it and setting it as a value instead
                if (convertOldFileKeyframesToPattern) {
                    SequenceParsing::FileNameContent content(_value.value.isString);
                    content.generatePatternWithFrameNumberAtIndex(content.getPotentialFrameNumbersCount() - 1,
                                                                  content.getNumPrependingZeroes() + 1,
                                                                  &_value.value.isString);
                }
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isString);
                }
            }

        }

        switch (_value.type) {
            case ValueSerializationStorage::eSerializationValueVariantTypeNone:
                break;

            case ValueSerializationStorage::eSerializationValueVariantTypeInteger:
                ar & ::boost::serialization::make_nvp("Value", _value.value.isInt);
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isInt);
                }
                break;

            case ValueSerializationStorage::eSerializationValueVariantTypeDouble:
                ar & ::boost::serialization::make_nvp("Value", _value.value.isDouble);
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isDouble);
                }
                break;

            case ValueSerializationStorage::eSerializationValueVariantTypeString:
                ar & ::boost::serialization::make_nvp("Value", _value.value.isString);
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isString);
                }
                break;

            case ValueSerializationStorage::eSerializationValueVariantTypeBoolean:
                ar & ::boost::serialization::make_nvp("Value", _value.value.isBool);
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _value.defaultValue.isBool);
                }

                break;

        }


        ///We cannot restore the master yet. It has to be done in another pass.
        bool hasMaster;
        ar & ::boost::serialization::make_nvp("HasMaster", hasMaster);
        if (hasMaster) {
            ar & ::boost::serialization::make_nvp("Master", _value.slaveMasterLink);
        }

        if (version >= VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS) {
            ar & ::boost::serialization::make_nvp("Expression", _value.expression);
            ar & ::boost::serialization::make_nvp("ExprHasRet", _value.expresionHasReturnVariable);
        }

        if ( (version >= VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS_RESULTS) && (version < VALUE_SERIALIZATION_REMOVES_EXPRESSIONS_RESULTS) ) {

            bool isString = _typeName == KnobString::typeNameStatic();
            bool isColor = _typeName == KnobColor::typeNameStatic();
            bool isDouble = _typeName == KnobDouble::typeNameStatic();
            bool isInt = _typeName == KnobInt::typeNameStatic();
            bool isBool = _typeName == KnobBool::typeNameStatic();
            bool isFile = _typeName == KnobFile::typeNameStatic();
            bool isOutFile = _typeName == KnobOutputFile::typeNameStatic();
            bool isPath = _typeName == KnobPath::typeNameStatic();

            bool isDoubleVal = isDouble || isColor;
            bool isIntVal = isChoice || isInt;
            bool isStrVal = isFile || isOutFile || isPath || isString;

            if (isIntVal) {
                std::map<SequenceTime, int> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
            } else if (isBool) {
                std::map<SequenceTime, bool> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
            } else if (isDoubleVal) {
                std::map<SequenceTime, double> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
            } else if (isStrVal) {
                std::map<SequenceTime, std::string> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
            }
        }
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif
};



class KnobSerializationBase : public SerializationObjectBase
{
public:

    KnobSerializationBase() {}

    virtual ~KnobSerializationBase() {}


    virtual const std::string& getName() const = 0;

    virtual const std::string& getTypeName() const = 0;

    // For backward compatibility when the label was not yet in the ChoiceExtraData class
    virtual void setChoiceExtraString(const std::string& /*label*/) {}

};

class KnobSerialization
    : public KnobSerializationBase
{
    Q_DECLARE_TR_FUNCTIONS(KnobSerialization)

public:

    unsigned int _version;
    bool _hasBeenSerialized; // True if this object contains a valid serialization.

    std::string _typeName; // used to re-create the appropriate knob type
    std::string _scriptName; // the unique script-name of the knob
    int _dimension; // the number of dimensions held by the knob
    bool _isSecret; // is this knob secret ?
    bool _masterIsAlias; // is the master/slave link an alias ?
    mutable std::vector<boost::shared_ptr<ValueSerialization> > _values; // serialized value for each dimension
    mutable boost::scoped_ptr<TypeExtraData> _extraData; // holds type specific data other than values

    bool _isUserKnob; // is this knob created by the user or was this created by a plug-in ?
    std::string _label; // only serialized for user knobs
    std::string _iconFilePath[2]; // only serialized for user knobs
    bool _triggerNewLine; // only serialized for user knobs
    bool _evaluatesOnChange; // only serialized for user knobs
    bool _isPersistent; // only serialized for user knobs
    bool _animationEnabled; // only serialized for user knobs
    bool _hasViewerInterface; // does this knob has a user interface ?
    bool _inViewerContextSecret; // is this knob secret in the viewer interface
    int _inViewerContextItemSpacing; // item spacing in viewer, serialized for all knobs if they have a viewer UI
    int _inViewerContextItemLayout; // item layout in viewer, serialized for all knobs if they have a viewer UI
    std::string _inViewerContextIconFilePath[2]; // only serialized for user knobs with a viewer interface
    std::string _inViewerContextLabel; // only serialized for user knobs with a viewer interface
    std::string _tooltip; // serialized for user knobs only


    explicit KnobSerialization()
    : _version(KNOB_SERIALIZATION_VERSION)
    , _hasBeenSerialized(false)
    , _typeName()
    , _scriptName()
    , _dimension(0)
    , _isSecret(false)
    , _masterIsAlias(false)
    , _values()
    , _extraData()
    , _isUserKnob(false)
    , _label()
    , _triggerNewLine(false)
    , _evaluatesOnChange(false)
    , _isPersistent(false)
    , _animationEnabled(false)
    , _hasViewerInterface(false)
    , _inViewerContextSecret(false)
    , _inViewerContextItemSpacing(0)
    , _inViewerContextItemLayout(0)
    , _inViewerContextIconFilePath()
    , _inViewerContextLabel()
    , _tooltip()
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



    // For backward compatibility when the label was not yet in the ChoiceExtraData class
    virtual void setChoiceExtraString(const std::string& label) OVERRIDE FINAL;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {

        ar & ::boost::serialization::make_nvp("Name", _scriptName);
        ar & ::boost::serialization::make_nvp("Type", _typeName);
        ar & ::boost::serialization::make_nvp("Dimension", _dimension);
        ar & ::boost::serialization::make_nvp("Secret", _isSecret);
        ar & ::boost::serialization::make_nvp("MasterIsAlias", _masterIsAlias);

        assert((int)_values.size() == _dimension);
        for (int i = 0; i < _dimension; ++i) {
            ar & ::boost::serialization::make_nvp("item", *(_values[i]));
        }

        // Save extra datas
        ParametricExtraData* parametricData = dynamic_cast<ParametricExtraData*>(_extraData.get());
        TextExtraData* textData = dynamic_cast<TextExtraData*>(_extraData.get());
        ChoiceExtraData* cdata = dynamic_cast<ChoiceExtraData*>(_extraData.get());
        if (parametricData) {
            ar & ::boost::serialization::make_nvp("ParametricCurves", parametricData->parametricCurves);
        } else if (textData) {
            ar & ::boost::serialization::make_nvp("StringsAnimation", textData->keyframes);
        } else if (cdata) {
            ar & ::boost::serialization::make_nvp("ChoiceLabel", cdata->_choiceString);
        }

        ar & ::boost::serialization::make_nvp("UserKnob", _isUserKnob);
        if (_isUserKnob) {
            ar & ::boost::serialization::make_nvp("Label", _label);
            ar & ::boost::serialization::make_nvp("Help", _tooltip);
            ar & ::boost::serialization::make_nvp("NewLine", _triggerNewLine);
            ar & ::boost::serialization::make_nvp("Evaluate", _evaluatesOnChange);
            ar & ::boost::serialization::make_nvp("Animates", _animationEnabled);
            ar & ::boost::serialization::make_nvp("Persistent", _isPersistent);
            ar & ::boost::serialization::make_nvp("UncheckedIcon", _iconFilePath[0]);
            ar & ::boost::serialization::make_nvp("CheckedIcon", _iconFilePath[1]);
            if (_extraData) {
                ValueExtraData* vdata = dynamic_cast<ValueExtraData*>(_extraData.get());
                TextExtraData* tdata = dynamic_cast<TextExtraData*>(_extraData.get());
                FileExtraData* fdata = dynamic_cast<FileExtraData*>(_extraData.get());
                PathExtraData* pdata = dynamic_cast<PathExtraData*>(_extraData.get());

                if (cdata) {
                    ar & ::boost::serialization::make_nvp("Entries", cdata->_entries);
                    ar & ::boost::serialization::make_nvp("Helps", cdata->_helpStrings);
                } else if (vdata) {
                    ar & ::boost::serialization::make_nvp("Min", vdata->min);
                    ar & ::boost::serialization::make_nvp("Max", vdata->max);
                    ar & ::boost::serialization::make_nvp("DMin", vdata->dmin);
                    ar & ::boost::serialization::make_nvp("DMax", vdata->dmax);
                } else if (fdata) {
                    ar & ::boost::serialization::make_nvp("Sequences", fdata->useSequences);
                } else if (pdata) {
                    ar & ::boost::serialization::make_nvp("MultiPath", pdata->multiPath);
                } else if (tdata) {
                    ar & ::boost::serialization::make_nvp("IsLabel", tdata->label);
                    ar & ::boost::serialization::make_nvp("IsMultiLine", tdata->multiLine);
                    ar & ::boost::serialization::make_nvp("UseRichText", tdata->richText);
                }
            }

            bool isDouble = _typeName == KnobDouble::typeNameStatic();
            if ( isDouble ) {
                DoubleExtraData* doubleData = dynamic_cast<DoubleExtraData*>(_extraData.get());
                ar & ::boost::serialization::make_nvp("HasOverlayHandle", doubleData->useHostOverlayHandle);
            }
        }
        ar & ::boost::serialization::make_nvp("HasViewerUI", _hasViewerInterface);
        if (_hasViewerInterface) {
            ar & ::boost::serialization::make_nvp("ViewerUISpacing", _inViewerContextItemSpacing);
            ar & ::boost::serialization::make_nvp("ViewerUILayout", _inViewerContextItemLayout);
            ar & ::boost::serialization::make_nvp("ViewerUISecret", _inViewerContextSecret);
            if (_isUserKnob) {
                ar & ::boost::serialization::make_nvp("ViewerUILabel", _inViewerContextLabel);
                ar & ::boost::serialization::make_nvp("ViewerUIIconUnchecked", _inViewerContextIconFilePath[0]);
                ar & ::boost::serialization::make_nvp("ViewerUIIconChecked", _inViewerContextIconFilePath[1]);
            }
        }
    } // save

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        _version = version;
        
        ar & ::boost::serialization::make_nvp("Name", _scriptName);
        ar & ::boost::serialization::make_nvp("Type", _typeName);
        ar & ::boost::serialization::make_nvp("Dimension", _dimension);
        _values.resize(_dimension);

        ar & ::boost::serialization::make_nvp("Secret", _isSecret);

        if (version >= KNOB_SERIALIZATION_INTRODUCES_ALIAS) {
            ar & ::boost::serialization::make_nvp("MasterIsAlias", _masterIsAlias);
        } else {
            _masterIsAlias = false;
        }

        bool isFile = _typeName == KnobFile::typeNameStatic();
        bool isOutFile = _typeName == KnobOutputFile::typeNameStatic();
        bool isPath = _typeName == KnobPath::typeNameStatic();
        bool isString = _typeName == KnobString::typeNameStatic();
        bool isParametric = _typeName == KnobParametric::typeNameStatic();
        bool isChoice = _typeName == KnobChoice::typeNameStatic();
        bool isDouble = _typeName == KnobDouble::typeNameStatic();
        bool isColor = _typeName == KnobColor::typeNameStatic();
        bool isInt = _typeName == KnobInt::typeNameStatic();
        bool isBool = _typeName == KnobBool::typeNameStatic();

        if (isChoice && !_extraData) {
            _extraData.reset(new ChoiceExtraData);
        }
        if (isParametric && !_extraData) {
            _extraData.reset(new ParametricExtraData);
        }
        if (isString && !_extraData) {
            _extraData.reset(new TextExtraData);
        }

        for (int i = 0; i < _dimension; ++i) {
            _values[i].reset(new ValueSerialization(this, _typeName, i));
            ar & ::boost::serialization::make_nvp("item", *_values[i]);
        }

        ////restore extra datas
        if (isParametric) {
            ParametricExtraData* extraData = dynamic_cast<ParametricExtraData*>(_extraData.get());
            if (version >= KNOB_SERIALIZATION_CHANGE_CURVE_SERIALIZATION) {
                ar & ::boost::serialization::make_nvp("ParametricCurves", extraData->parametricCurves);
            } else {
                std::list<Curve> curves;
                ar & ::boost::serialization::make_nvp("ParametricCurves", curves);
                for (std::list<Curve>::iterator it = curves.begin(); it!=curves.end(); ++it) {
                    CurveSerialization c;
                    it->saveSerialization(&c);
                    extraData->parametricCurves.push_back(c);
                }
            }
            //isParametric->loadParametricCurves(extraData->parametricCurves);
        } else if (isString) {
            TextExtraData* extraData = dynamic_cast<TextExtraData*>(_extraData.get());
            ar & ::boost::serialization::make_nvp("StringsAnimation", extraData->keyframes);
            ///Don't load animation for input image files: they no longer hold keyframes
            // in the Reader context, the script name must be kOfxImageEffectFileParamName, @see kOfxImageEffectContextReader
            /*if ( !isFile || ( isFile && (isFile->getName() != kOfxImageEffectFileParamName) ) ) {
                isStringAnimated->loadAnimation(extraDatas);
            }*/
        }

        // Dead serialization code, just for backward compatibility.
        if ( ( (version >= KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS) && (version < KNOB_SERIALIZATION_REMOVE_SLAVED_TRACKS) ) &&
             isDouble && ( _scriptName == "center") && ( _dimension == 2) ) {
            int count;
            ar & ::boost::serialization::make_nvp("SlavePtsNo", count);
            for (int i = 0; i < count; ++i) {
                std::string rotoNodeName, bezierName;
                int cpIndex;
                bool isFeather;
                int offsetTime;
                ar & ::boost::serialization::make_nvp("SlavePtNodeName", rotoNodeName);
                ar & ::boost::serialization::make_nvp("SlavePtBezier", bezierName);
                ar & ::boost::serialization::make_nvp("SlavePtIndex", cpIndex);
                ar & ::boost::serialization::make_nvp("SlavePtIsFeather", isFeather);
                if (version >= KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS_OFFSET) {
                    ar & ::boost::serialization::make_nvp("OffsetTime", offsetTime);
                }
            }
        }

        if (version >= KNOB_SERIALIZATION_INTRODUCES_USER_KNOB) {
            if (isChoice) {
                assert(_extraData);
                ChoiceExtraData* cData = dynamic_cast<ChoiceExtraData*>(_extraData.get());
                assert(cData);
                if (cData) {
                    ar & ::boost::serialization::make_nvp("ChoiceLabel", cData->_choiceString);
                }
            }


            ar & ::boost::serialization::make_nvp("UserKnob", _isUserKnob);
            if (_isUserKnob) {
                ar & ::boost::serialization::make_nvp("Label", _label);
                ar & ::boost::serialization::make_nvp("Help", _tooltip);
                ar & ::boost::serialization::make_nvp("NewLine", _triggerNewLine);
                ar & ::boost::serialization::make_nvp("Evaluate", _evaluatesOnChange);
                ar & ::boost::serialization::make_nvp("Animates", _animationEnabled);
                ar & ::boost::serialization::make_nvp("Persistent", _isPersistent);
                if (version >= KNOB_SERIALIZATION_INTRODUCE_USER_KNOB_ICON_FILE_PATH) {
                    ar & ::boost::serialization::make_nvp("UncheckedIcon", _iconFilePath[0]);
                    ar & ::boost::serialization::make_nvp("CheckedIcon", _iconFilePath[1]);
                }
                if (isChoice) {
                    assert(_extraData);
                    ChoiceExtraData* data = dynamic_cast<ChoiceExtraData*>(_extraData.get());
                    assert(data);
                    ar & ::boost::serialization::make_nvp("Entries", data->_entries);
                    if (version >= KNOB_SERIALIZATION_INTRODUCES_CHOICE_HELP_STRINGS) {
                        ar & ::boost::serialization::make_nvp("Helps", data->_helpStrings);
                    }
                }

                if (isString) {
                    TextExtraData* tdata = dynamic_cast<TextExtraData*>(_extraData.get());
                    assert(tdata);
                    ar & ::boost::serialization::make_nvp("IsLabel", tdata->label);
                    ar & ::boost::serialization::make_nvp("IsMultiLine", tdata->multiLine);
                    ar & ::boost::serialization::make_nvp("UseRichText", tdata->richText);
                }
                if (isDouble || isInt || isColor) {
                    ValueExtraData* extraData;
                    if (isDouble) {
                        extraData = new DoubleExtraData;
                    } else {
                        extraData = new ValueExtraData;
                    }
                    ar & ::boost::serialization::make_nvp("Min", extraData->min);
                    ar & ::boost::serialization::make_nvp("Max", extraData->max);
                    if (version >= KNOB_SERIALIZATION_INTRODUCES_DISPLAY_MIN_MAX) {
                        ar & ::boost::serialization::make_nvp("DMin", extraData->dmin);
                        ar & ::boost::serialization::make_nvp("DMax", extraData->dmax);
                    }
                    _extraData.reset(extraData);
                }

                if (isFile || isOutFile) {
                    FileExtraData* extraData = new FileExtraData;
                    ar & ::boost::serialization::make_nvp("Sequences", extraData->useSequences);
                    _extraData.reset(extraData);
                }

                if (isPath) {
                    PathExtraData* extraData = new PathExtraData;
                    ar & ::boost::serialization::make_nvp("MultiPath", extraData->multiPath);
                    _extraData.reset(extraData);
                }

                if ( isDouble &&
                    (((version >= KNOB_SERIALIZATION_INTRODUCES_NATIVE_OVERLAYS) && (_dimension == 2)) ||
                     (version >= KNOB_SERIALIZATION_INTRODUCE_VIEWER_UI)) ) {
                    DoubleExtraData* extraData = dynamic_cast<DoubleExtraData*>(_extraData.get());
                    assert(extraData);
                    ar & ::boost::serialization::make_nvp("HasOverlayHandle", extraData->useHostOverlayHandle);
                }

                // Dead serialization code, just for backward compat.
                if (version >= KNOB_SERIALIZATION_INTRODUCES_DEFAULT_VALUES && version < KNOB_SERIALIZATION_REMOVE_DEFAULT_VALUES) {

                    bool isDoubleVal = isDouble || isColor;
                    bool isIntVal = isChoice || isInt;
                    bool isStrVal = isFile || isOutFile || isPath || isString;

                    for (int i = 0; i < _dimension; ++i) {
                        if (isDoubleVal) {
                            double def;
                            ar & ::boost::serialization::make_nvp("DefaultValue", def);
                        } else if (isIntVal) {
                            int def;
                            ar & ::boost::serialization::make_nvp("DefaultValue", def);
                        } else if (isBool) {
                            bool def;
                            ar & ::boost::serialization::make_nvp("DefaultValue", def);
                        } else if (isStrVal) {
                            std::string def;
                            ar & ::boost::serialization::make_nvp("DefaultValue", def);
                        }
                    }
                }
            }
        }

        if (version >= KNOB_SERIALIZATION_INTRODUCE_VIEWER_UI) {
            ar & ::boost::serialization::make_nvp("HasViewerUI", _hasViewerInterface);
            if (_hasViewerInterface) {
                ar & ::boost::serialization::make_nvp("ViewerUISpacing", _inViewerContextItemSpacing);
                ar & ::boost::serialization::make_nvp("ViewerUILayout", _inViewerContextItemLayout);
                ar & ::boost::serialization::make_nvp("ViewerUISecret", _inViewerContextSecret);
                if (_isUserKnob) {
                    ar & ::boost::serialization::make_nvp("ViewerUILabel", _inViewerContextLabel);
                    ar & ::boost::serialization::make_nvp("ViewerUIIconUnchecked", _inViewerContextIconFilePath[0]);
                    ar & ::boost::serialization::make_nvp("ViewerUIIconChecked", _inViewerContextIconFilePath[1]);
                }
            }
        }

        _hasBeenSerialized = true;
        
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif // #ifdef NATRON_BOOST_SERIALIZATION_COMPAT



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


    GroupKnobSerialization(const KnobIPtr& knob = KnobIPtr())
    : _children()
    , _typeName()
    , _name()
    , _label()
    , _secret(false)
    , _isSetAsTab(false)
    , _isOpened(false)
    {
        if (knob) {
            knob->toSerialization(this);
        }
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



#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & ::boost::serialization::make_nvp("TypeName", _typeName);
        ar & ::boost::serialization::make_nvp("Name", _name);
        ar & ::boost::serialization::make_nvp("Label", _label);
        ar & ::boost::serialization::make_nvp("Secret", _secret);
        ar & ::boost::serialization::make_nvp("IsTab", _isSetAsTab);
        ar & ::boost::serialization::make_nvp("IsOpened", _isOpened);
        int nbChildren = (int)_children.size();
        ar & ::boost::serialization::make_nvp("NbChildren", nbChildren);
        for (std::list <boost::shared_ptr<KnobSerializationBase> >::const_iterator it = _children.begin();
             it != _children.end(); ++it) {
            GroupKnobSerializationPtr isGrp = boost::dynamic_pointer_cast<GroupKnobSerialization>(*it);
            KnobSerializationPtr isRegularKnob = boost::dynamic_pointer_cast<KnobSerialization>(*it);
            assert(isGrp || isRegularKnob);

            std::string type;
            if (isGrp) {
                type = "Group";
            } else {
                type = "Regular";
            }
            ar & ::boost::serialization::make_nvp("Type", type);
            if (isGrp) {
                ar & ::boost::serialization::make_nvp("item", *isGrp);
            } else {
                ar & ::boost::serialization::make_nvp("item", *isRegularKnob);
            }
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        if (version >= GROUP_KNOB_SERIALIZATION_INTRODUCES_TYPENAME) {
            ar & ::boost::serialization::make_nvp("TypeName", _typeName);
        }
        ar & ::boost::serialization::make_nvp("Name", _name);
        ar & ::boost::serialization::make_nvp("Label", _label);
        ar & ::boost::serialization::make_nvp("Secret", _secret);
        ar & ::boost::serialization::make_nvp("IsTab", _isSetAsTab);
        ar & ::boost::serialization::make_nvp("IsOpened", _isOpened);
        int nbChildren;
        ar & ::boost::serialization::make_nvp("NbChildren", nbChildren);
        for (int i = 0; i < nbChildren; ++i) {
            std::string type;
            ar & ::boost::serialization::make_nvp("Type", type);

            if (type == "Group") {
                boost::shared_ptr<GroupKnobSerialization> knob(new GroupKnobSerialization);
                ar & ::boost::serialization::make_nvp("item", *knob);
                _children.push_back(knob);
            } else {
                KnobSerializationPtr knob(new KnobSerialization);
                ar & ::boost::serialization::make_nvp("item", *knob);
                _children.push_back(knob);
            }
        }
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif
};

NATRON_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(NATRON_NAMESPACE::KnobSerialization, KNOB_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(NATRON_NAMESPACE::ValueSerialization, VALUE_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(NATRON_NAMESPACE::MasterSerialization, MASTER_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(NATRON_NAMESPACE::GroupKnobSerialization, GROUP_KNOB_SERIALIZATION_VERSION)
#endif

#endif // KNOBSERIALIZATION_H
