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
#include <string>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(sign-compare)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
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

#include <ofxImageEffect.h>

#include <SequenceParsing.h>

#include "Engine/Variant.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/CurveSerialization.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


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
#define KNOB_SERIALIZATION_CHANGE_PLANES_SERIALIZATION 14
#define KNOB_SERIALIZATION_VERSION KNOB_SERIALIZATION_CHANGE_PLANES_SERIALIZATION

#define VALUE_SERIALIZATION_INTRODUCES_CHOICE_LABEL 2
#define VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS 3
#define VALUE_SERIALIZATION_REMOVES_EXTRA_DATA 4
#define VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS_RESULTS 5
#define VALUE_SERIALIZATION_REMOVES_EXPRESSIONS_RESULTS 6
#define VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES 7
#define VALUE_SERIALIZATION_VERSION VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES

#define MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME 2
#define MASTER_SERIALIZATION_VERSION MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME

NATRON_NAMESPACE_ENTER

struct MasterSerialization
{
    int masterDimension;
    std::string masterNodeName;
    std::string masterTrackName;
    std::string masterKnobName;

    MasterSerialization()
        : masterDimension(-1)
        , masterNodeName()
        , masterTrackName()
        , masterKnobName()
    {
    }

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
};

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

    std::string _choiceString;
    std::vector<std::string> _entries;
    std::vector<std::string> _helpStrings;
};

class FileExtraData
    : public TypeExtraData
{
public:
    FileExtraData()
        : TypeExtraData(), useSequences(false) {}

    bool useSequences;
};

class PathExtraData
    : public TypeExtraData
{
public:
    PathExtraData()
        : TypeExtraData(), multiPath(false) {}

    bool multiPath;
};

class TextExtraData
    : public TypeExtraData
{
public:
    TextExtraData()
        : TypeExtraData(), label(false),  multiLine(false), richText(false) {}

    bool label;
    bool multiLine;
    bool richText;
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

    double min;
    double max;
    double dmin;
    double dmax;
};

/*
 * @brief Maps a knob name that existed in a specific version of Natron to the actual knob name in the current version.
 * @param pluginID The plugin ID of the plug-in being loaded.
 * @param natronVersionMajor/Minor/Revision The version of Natron that saved the project being loaded. This is -1 if the version of Natron could not be determined.
 * @param pluginVersionMajor/Minor The version of the plug-in that saved the project being loaded. This is -1 if the version of the plug-in could not be determined.
 * @param name[in][out] In input and output the name of the knob which may be modified by this function
 * @returns true if a filter was applied, false otherwise
 */
bool filterKnobNameCompat(const std::string& pluginID,
                          int pluginVersionMajor, int pluginVersionMinor,
                          int natronVersionMajor, int natronVersionMinor, int natronVersionRevision,
                          std::string* name);

/*
 * @brief Maps a KnobChoice option ID that existed in a specific version of Natron to the actual option in the current version.
 * @param pluginID The plugin ID of the plug-in being loaded
 * @param natronVersionMajor/Minor/Revision The version of Natron that saved the project being loaded
 * @param pluginVersionMajor/Minor The version of the plug-in that saved the project being loaded
 * @param paramName The name of the parameter being loaded. Note that this is the name of the parameter in the saved project, prior to being passed to filterKnobNameCompat
 * dynamic choice parameters: we used to store the value in a string parameter with the suffix "Choice".
 * @param optionID[in][out] In input and output the optionID which may be modified by this function
 * @returns true if a filter was applied, false otherwise
 */
bool filterKnobChoiceOptionCompat(const std::string& pluginID,
                            int pluginVersionMajor, int pluginVersionMinor,
                            int natronVersionMajor, int natronVersionMinor, int natronVersionRevision,
                            const std::string& paramName,
                            std::string* optionID);

class KnobSerializationBase;
struct ValueSerialization
{
    KnobSerializationBase* _serialization;
    KnobIPtr _knob;
    int _dimension;
    MasterSerialization _master;
    std::string _expression;
    bool _exprHasRetVar;

    ValueSerialization();

    ///Load
    ValueSerialization(KnobSerializationBase* serialization,
                       const KnobIPtr & knob,
                       int dimension);
    void initForLoad(KnobSerializationBase* serialization,
                     const KnobIPtr & knob,
                     int dimension);

    ///Save
    ValueSerialization(const KnobIPtr & knob,
                       int dimension,
                       bool exprHasRetVar,
                       const std::string& expr);
    void initForSave(const KnobIPtr & knob,
                     int dimension,
                     bool exprHasRetVar,
                     const std::string& expr);

    void setChoiceExtraLabel(const std::string& label);


    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( _knob.get() );
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( _knob.get() );
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( _knob.get() );
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>( _knob.get() );
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>( _knob.get() );
        KnobPage* isPage = dynamic_cast<KnobPage*>( _knob.get() );
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>( _knob.get() );
        KnobSeparator* isSep = dynamic_cast<KnobSeparator*>( _knob.get() );
        KnobButton* btn = dynamic_cast<KnobButton*>( _knob.get() );
        bool enabled = _knob->isEnabled(_dimension);
        ar & ::boost::serialization::make_nvp("Enabled", enabled);
        bool hasAnimation = _knob->isAnimated(_dimension);
        ar & ::boost::serialization::make_nvp("HasAnimation", hasAnimation);

        if (hasAnimation) {
            ar & ::boost::serialization::make_nvp( "Curve", *( _knob->getCurve(ViewIdx(0), _dimension, true) ) );
        }

        if (isInt && !isChoice) {
            int v = isInt->getValue(_dimension);
            int defV = isInt->getDefaultValue(_dimension);
            ar & ::boost::serialization::make_nvp("Value", v);
            ar & ::boost::serialization::make_nvp("Default", defV);
        } else if (isBool && !isPage && !isGrp && !isSep && !btn) {
            bool v = isBool->getValue(_dimension);
            bool defV = isBool->getDefaultValue(_dimension);
            ar & ::boost::serialization::make_nvp("Value", v);
            ar & ::boost::serialization::make_nvp("Default", defV);
        } else if (isDouble && !isParametric) {
            double v = isDouble->getValue(_dimension);
            double defV = isDouble->getDefaultValue(_dimension);
            ar & ::boost::serialization::make_nvp("Value", v);
            ar & ::boost::serialization::make_nvp("Default", defV);
        } else if (isChoice) {
            int v = isChoice->getValue(_dimension);
            int defV = isChoice->getDefaultValue(_dimension);
            std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();
            std::string label;
            if ( ( v < (int)entries.size() ) && (v >= 0) ) {
                label = entries[v].id;
            }
            ar & ::boost::serialization::make_nvp("Value", v);
            ar & ::boost::serialization::make_nvp("Default", defV);
        } else if (isString) {
            std::string v = isString->getValue(_dimension);
            std::string defV = isString->getDefaultValue(_dimension);
            ar & ::boost::serialization::make_nvp("Value", v);
            ar & ::boost::serialization::make_nvp("Default", defV);
        }

        bool hasMaster = _knob->isSlave(_dimension);
        ar & ::boost::serialization::make_nvp("HasMaster", hasMaster);
        if (hasMaster) {
            ar & ::boost::serialization::make_nvp("Master", _master);
        }

        ar & ::boost::serialization::make_nvp("Expression", _expression);
        ar & ::boost::serialization::make_nvp("ExprHasRet", _exprHasRetVar);
    } // save

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( _knob.get() );
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( _knob.get() );
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( _knob.get() );
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>( _knob.get() );
        KnobFile* isFile = dynamic_cast<KnobFile*>( _knob.get() );
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>( _knob.get() );
        KnobPage* isPage = dynamic_cast<KnobPage*>( _knob.get() );
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>( _knob.get() );
        KnobSeparator* isSep = dynamic_cast<KnobSeparator*>( _knob.get() );
        KnobButton* btn = dynamic_cast<KnobButton*>( _knob.get() );
        bool enabled;
        ar & ::boost::serialization::make_nvp("Enabled", enabled);

        _knob->setEnabled(_dimension, enabled);

        bool hasAnimation;
        ar & ::boost::serialization::make_nvp("HasAnimation", hasAnimation);
        bool convertOldFileKeyframesToPattern = false;
        if (hasAnimation) {
            assert( _knob->canAnimate() );
            Curve c;
            ar & ::boost::serialization::make_nvp("Curve", c);
            ///This is to overcome the change to the animation of file params: They no longer hold keyframes
            ///Don't try to load keyframes
            convertOldFileKeyframesToPattern = isFile && isFile->getName() == kOfxImageEffectFileParamName;
            if (!convertOldFileKeyframesToPattern) {
                CurvePtr curve = _knob->getCurve(ViewIdx(0), _dimension);
                assert(curve);
                if (curve) {
                    _knob->getCurve(ViewIdx(0), _dimension)->clone(c);
                }
            }
        }

        if (isInt && !isChoice) {
            int v;
            ar & ::boost::serialization::make_nvp("Value", v);
            isInt->setValue(v, ViewSpec::all(), _dimension);
            if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                int defV;
                ar & ::boost::serialization::make_nvp("Default", defV);
                isInt->setDefaultValueWithoutApplying(defV, _dimension);
            }
        } else if (isBool && !isGrp && !isPage && !isSep && !btn) {
            bool v;
            ar & ::boost::serialization::make_nvp("Value", v);
            isBool->setValue(v, ViewSpec::all(), _dimension);
            if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                bool defV;
                ar & ::boost::serialization::make_nvp("Default", defV);
                isBool->setDefaultValueWithoutApplying(defV, _dimension);
            }
        } else if (isDouble && !isParametric) {
            double v;
            ar & ::boost::serialization::make_nvp("Value", v);
            isDouble->setValue(v, ViewSpec::all(), _dimension);
            if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                double defV;
                ar & ::boost::serialization::make_nvp("Default", defV);
                isDouble->setDefaultValueWithoutApplying(defV, _dimension);
            }
        } else if (isChoice) {
            int v;
            ar & ::boost::serialization::make_nvp("Value", v);
            assert(v >= 0);
            if (version >= VALUE_SERIALIZATION_INTRODUCES_CHOICE_LABEL) {
                if (version < VALUE_SERIALIZATION_REMOVES_EXTRA_DATA) {
                    std::string label;
                    ar & ::boost::serialization::make_nvp("Label", label);
                    setChoiceExtraLabel(label);
                }
            }
            isChoice->setValue(v, ViewIdx(0), _dimension);
            if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                int defV;
                ar & ::boost::serialization::make_nvp("Default", defV);
                isChoice->setDefaultValueWithoutApplying(defV, _dimension);
            }
        } else if (isString && !isFile) {
            std::string v;
            ar & ::boost::serialization::make_nvp("Value", v);
            isString->setValue(v, ViewIdx(0), _dimension);
            if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                std::string defV;
                ar & ::boost::serialization::make_nvp("Default", defV);
                isString->setDefaultValueWithoutApplying(defV, _dimension);
            }
        } else if (isFile) {
            std::string v;
            ar & ::boost::serialization::make_nvp("Value", v);

            ///Convert the old keyframes stored in the file parameter by analysing one keyframe
            ///and deducing the pattern from it and setting it as a value instead
            if (convertOldFileKeyframesToPattern) {
                SequenceParsing::FileNameContent content(v);
                content.generatePatternWithFrameNumberAtIndex(content.getPotentialFrameNumbersCount() - 1,
                                                              content.getLeadingZeroes() + 1,
                                                              &v);
            }
            isFile->setValue(v, ViewIdx(0), _dimension);
            if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                std::string defV;
                ar & ::boost::serialization::make_nvp("Default", defV);
                isFile->setDefaultValueWithoutApplying(defV, _dimension);
            }
        }

        ///We cannot restore the master yet. It has to be done in another pass.
        bool hasMaster;
        ar & ::boost::serialization::make_nvp("HasMaster", hasMaster);
        if (hasMaster) {
            ar & ::boost::serialization::make_nvp("Master", _master);
        }

        if (version >= VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS) {
            ar & ::boost::serialization::make_nvp("Expression", _expression);
            ar & ::boost::serialization::make_nvp("ExprHasRet", _exprHasRetVar);
        }

        if ( (version >= VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS_RESULTS) && (version < VALUE_SERIALIZATION_REMOVES_EXPRESSIONS_RESULTS) ) {
            if (isInt) {
                std::map<SequenceTime, int> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
            } else if (isBool) {
                std::map<SequenceTime, bool> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
            } else if (isDouble) {
                std::map<SequenceTime, double> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
            } else if (isString) {
                std::map<SequenceTime, std::string> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
            }
        }
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


class KnobSerializationBase
{
public:

    KnobSerializationBase() {}

    virtual ~KnobSerializationBase() {}


    virtual const std::string& getName() const = 0;
    virtual KnobIPtr getKnob() const = 0;
    virtual void setChoiceExtraString(const std::string& /*label*/) {}
};

class KnobSerialization
    : public KnobSerializationBase
{
    KnobIPtr _knob; //< used when serializing
    std::string _typeName;
    int _dimension;
    std::list<MasterSerialization> _masters; //< used when deserializating, we can't restore it before all knobs have been restored.
    bool _masterIsAlias;
    std::vector<std::pair<std::string, bool> > _expressions; //< used when deserializing, we can't restore it before all knobs have been restored.
    std::list<Curve > parametricCurves;
    mutable TypeExtraData* _extraData;
    bool _isUserKnob;
    std::string _label;
    bool _triggerNewLine;
    bool _evaluatesOnChange;
    bool _isPersistent;
    bool _animationEnabled;
    std::string _tooltip;
    bool _useHostOverlay;
    mutable std::vector<ValueSerialization> _values;

public:

    unsigned int _version;
private:
    
    virtual void setChoiceExtraString(const std::string& label) OVERRIDE FINAL;

    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {

        assert(_knob);
        AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>( _knob.get() );
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>( _knob.get() );
        KnobDouble* isDouble = dynamic_cast<KnobDouble*>( _knob.get() );
        std::string name = _knob->getName();
        ar & ::boost::serialization::make_nvp("Name", name);
        ar & ::boost::serialization::make_nvp("Type", _typeName);
        ar & ::boost::serialization::make_nvp("Dimension", _dimension);
        bool secret = _knob->getIsSecret();
        ar & ::boost::serialization::make_nvp("Secret", secret);
        ar & ::boost::serialization::make_nvp("MasterIsAlias", _masterIsAlias);

        assert((int)_values.size() == _dimension);
        for (int i = 0; i < _knob->getDimension(); ++i) {
            ar & ::boost::serialization::make_nvp("item", _values[i]);
        }

        ////restore extra datas
        if (isParametric) {
            std::list<Curve > curves;
            isParametric->saveParametricCurves(&curves);
            ar & ::boost::serialization::make_nvp("ParametricCurves", curves);
        } else if (isString) {
            std::map<int, std::string> extraDatas;
            isString->getAnimation().save(&extraDatas);
            ar & ::boost::serialization::make_nvp("StringsAnimation", extraDatas);
        }
        ChoiceExtraData* cdata = dynamic_cast<ChoiceExtraData*>(_extraData);
        if (cdata) {
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
            if (_extraData) {
                ValueExtraData* vdata = dynamic_cast<ValueExtraData*>(_extraData);
                TextExtraData* tdata = dynamic_cast<TextExtraData*>(_extraData);
                FileExtraData* fdata = dynamic_cast<FileExtraData*>(_extraData);
                PathExtraData* pdata = dynamic_cast<PathExtraData*>(_extraData);

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

            if ( isDouble && (isDouble->getDimension() == 2) ) {
                bool useOverlay = isDouble->getHasHostOverlayHandle();
                ar & ::boost::serialization::make_nvp("HasOverlayHandle", useOverlay);
            }
        }
    } // save

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        _version = version;
        assert(!_knob);
        std::string name;
        ar & ::boost::serialization::make_nvp("Name", name);
        ar & ::boost::serialization::make_nvp("Type", _typeName);
        ar & ::boost::serialization::make_nvp("Dimension", _dimension);
        _values.resize(_dimension);
        KnobIPtr created = createKnob(_typeName, _dimension);
        if (!created) {
            return;
        } else {
            _knob = created;
        }
        _knob->setName(name);

        bool secret;
        ar & ::boost::serialization::make_nvp("Secret", secret);
        _knob->setSecret(secret);

        if (version >= KNOB_SERIALIZATION_INTRODUCES_ALIAS) {
            ar & ::boost::serialization::make_nvp("MasterIsAlias", _masterIsAlias);
        } else {
            _masterIsAlias = false;
        }

        AnimatingKnobStringHelper* isStringAnimated = dynamic_cast<AnimatingKnobStringHelper*>( _knob.get() );
        KnobFile* isFile = dynamic_cast<KnobFile*>( _knob.get() );
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>( _knob.get() );
        KnobDouble* isDouble = dynamic_cast<KnobDouble*>( _knob.get() );
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
        if (isChoice && !_extraData) {
            _extraData = new ChoiceExtraData;
        }

        for (int i = 0; i < _knob->getDimension(); ++i) {
            _values[i].initForLoad(this, _knob, i);
            ar & ::boost::serialization::make_nvp("item", _values[i]);
            _masters.push_back(_values[i]._master);
            _expressions.push_back( std::make_pair(_values[i]._expression, _values[i]._exprHasRetVar) );
        }

        ////restore extra datas
        if (isParametric) {
            std::list<Curve > curves;
            ar & ::boost::serialization::make_nvp("ParametricCurves", curves);
            isParametric->loadParametricCurves(curves);
        } else if (isStringAnimated) {
            std::map<int, std::string> extraDatas;
            ar & ::boost::serialization::make_nvp("StringsAnimation", extraDatas);
            ///Don't load animation for input image files: they no longer hold keyframes
            // in the Reader context, the script name must be kOfxImageEffectFileParamName, @see kOfxImageEffectContextReader
            if ( !isFile || ( isFile && (isFile->getName() != kOfxImageEffectFileParamName) ) ) {
                isStringAnimated->loadAnimation(extraDatas);
            }
        }
        if ( ( (version >= KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS) && (version < KNOB_SERIALIZATION_REMOVE_SLAVED_TRACKS) ) &&
             isDouble && ( isDouble->getName() == "center") && ( isDouble->getDimension() == 2) ) {
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
            KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
            if (isChoice) {
                //ChoiceExtraData* cData = new ChoiceExtraData;
                assert(_extraData);
                ChoiceExtraData* cData = dynamic_cast<ChoiceExtraData*>(_extraData);
                assert(cData);
                if (cData) {
                    ar & ::boost::serialization::make_nvp("ChoiceLabel", cData->_choiceString);
                    /*if (version < KNOB_SERIALIZATION_CHANGE_PLANES_SERIALIZATION) {
                        // In Natron 2.2.6 we changed the encoding of planes: they no longer are planeLabel + "." + channels
                        // but planeID + "." + channels
                        // Hard-code the mapping
                        filterKnobChoiceOption(getPluginID(), projectInfos.vMajor, projectInfos.vMinor, isChoice->getName(), &cData->_choiceString);
                    }*/
                    //_extraData = cData;
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

                if (isChoice) {
                    assert(_extraData);
                    ChoiceExtraData* data = dynamic_cast<ChoiceExtraData*>(_extraData);
                    assert(data);
                    ar & ::boost::serialization::make_nvp("Entries", data->_entries);
                    if (version >= KNOB_SERIALIZATION_INTRODUCES_CHOICE_HELP_STRINGS) {
                        ar & ::boost::serialization::make_nvp("Helps", data->_helpStrings);
                    }
                }

                KnobString* isString = dynamic_cast<KnobString*>( _knob.get() );
                if (isString) {
                    TextExtraData* tdata = new TextExtraData;
                    ar & ::boost::serialization::make_nvp("IsLabel", tdata->label);
                    ar & ::boost::serialization::make_nvp("IsMultiLine", tdata->multiLine);
                    ar & ::boost::serialization::make_nvp("UseRichText", tdata->richText);
                    _extraData = tdata;
                }
                KnobDouble* isDbl = dynamic_cast<KnobDouble*>( _knob.get() );
                KnobInt* isInt = dynamic_cast<KnobInt*>( _knob.get() );
                KnobColor* isColor = dynamic_cast<KnobColor*>( _knob.get() );
                if (isDbl || isInt || isColor) {
                    ValueExtraData* extraData = new ValueExtraData;
                    ar & ::boost::serialization::make_nvp("Min", extraData->min);
                    ar & ::boost::serialization::make_nvp("Max", extraData->max);
                    if (version >= KNOB_SERIALIZATION_INTRODUCES_DISPLAY_MIN_MAX) {
                        ar & ::boost::serialization::make_nvp("DMin", extraData->dmin);
                        ar & ::boost::serialization::make_nvp("DMax", extraData->dmax);
                    }
                    _extraData = extraData;
                }

                KnobFile* isFile = dynamic_cast<KnobFile*>( _knob.get() );
                KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>( _knob.get() );
                if (isFile || isOutFile) {
                    FileExtraData* extraData = new FileExtraData;
                    ar & ::boost::serialization::make_nvp("Sequences", extraData->useSequences);
                    _extraData = extraData;
                }

                KnobPath* isPath = dynamic_cast<KnobPath*>( _knob.get() );
                if (isPath) {
                    PathExtraData* extraData = new PathExtraData;
                    ar & ::boost::serialization::make_nvp("MultiPath", extraData->multiPath);
                    _extraData = extraData;
                }


                if ( isDbl && (version >= KNOB_SERIALIZATION_INTRODUCES_NATIVE_OVERLAYS) && (isDbl->getDimension() == 2) ) {
                    ar & ::boost::serialization::make_nvp("HasOverlayHandle", _useHostOverlay);
                }

                if (version >= KNOB_SERIALIZATION_INTRODUCES_DEFAULT_VALUES && version < KNOB_SERIALIZATION_REMOVE_DEFAULT_VALUES) {
                    KnobDoubleBase* isDoubleVal = dynamic_cast<KnobDoubleBase*>( _knob.get() );
                    KnobIntBase* isIntVal = dynamic_cast<KnobIntBase*>( _knob.get() );
                    KnobBool* isBool = dynamic_cast<KnobBool*>( _knob.get() );
                    KnobStringBase* isStr = dynamic_cast<KnobStringBase*>( _knob.get() );

                    for (int i = 0; i < _knob->getDimension(); ++i) {
                        if (isDoubleVal) {
                            double def;
                            ar & ::boost::serialization::make_nvp("DefaultValue", def);
                        } else if (isIntVal) {
                            int def;
                            ar & ::boost::serialization::make_nvp("DefaultValue", def);
                        } else if (isBool) {
                            bool def;
                            ar & ::boost::serialization::make_nvp("DefaultValue", def);
                        } else if (isStr) {
                            std::string def;
                            ar & ::boost::serialization::make_nvp("DefaultValue", def);
                        }
                    }
                }
            }
        }
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()

public:

    ///Constructor used to serialize
    explicit KnobSerialization(const KnobIPtr & knob)
        : _knob()
        , _dimension(0)
        , _masterIsAlias(false)
        , _extraData(NULL)
        , _isUserKnob(false)
        , _label()
        , _triggerNewLine(false)
        , _evaluatesOnChange(false)
        , _isPersistent(false)
        , _animationEnabled(false)
        , _tooltip()
        , _useHostOverlay(false)
        , _version(KNOB_SERIALIZATION_VERSION)
    {

        initialize(knob);
    }


    ///Doing the empty param constructor + this function is the same
    ///as calling the constructore above
    void initialize(const KnobIPtr & knob)
    {
        _knob = knob;

        _typeName = knob->typeName();
        _dimension = knob->getDimension();

        _values.resize(_dimension);

        for (int i = 0; i < _dimension; ++i) {
            _expressions.push_back( std::make_pair( knob->getExpression(i), knob->isExpressionUsingRetVariable(i) ) );
            _values[i].initForSave(_knob, i , _expressions[i].second, _expressions[i].first);
            _masters.push_back(_values[i]._master);
        }



        _masterIsAlias = knob->getAliasMaster().get() != 0;

        _isUserKnob = knob->isUserKnob();
        _label = knob->getLabel();
        _triggerNewLine = knob->isNewLineActivated();
        _evaluatesOnChange = knob->getEvaluateOnChange();
        _isPersistent = knob->getIsPersistent();
        _animationEnabled = knob->isAnimationEnabled();
        _tooltip = knob->getHintToolTip();

        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
        if (isChoice) {
            ChoiceExtraData* extraData = new ChoiceExtraData;
            std::vector<ChoiceOption> options = isChoice->getEntries_mt_safe();
            extraData->_entries.resize(options.size());
            extraData->_helpStrings.resize(options.size());
            for (std::size_t i = 0; i < options.size(); ++i) {
                extraData->_entries[i] = options[i].id;
                extraData->_helpStrings[i] = options[i].tooltip;
            }
            int idx = isChoice->getValue();
            if ( (idx >= 0) && ( idx < (int)extraData->_entries.size() ) ) {
                extraData->_choiceString = extraData->_entries[idx];
            }
            _extraData = extraData;
        }
        if (_isUserKnob) {
            KnobString* isString = dynamic_cast<KnobString*>( _knob.get() );
            if (isString) {
                TextExtraData* extraData = new TextExtraData;
                extraData->label = isString->isLabel();
                extraData->multiLine = isString->isMultiLine();
                extraData->richText = isString->usesRichText();
                _extraData = extraData;
            }
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>( _knob.get() );
            KnobInt* isInt = dynamic_cast<KnobInt*>( _knob.get() );
            KnobColor* isColor = dynamic_cast<KnobColor*>( _knob.get() );
            if (isDbl || isInt || isColor) {
                ValueExtraData* extraData = new ValueExtraData;
                if (isDbl) {
                    extraData->min = isDbl->getMinimum();
                    extraData->max = isDbl->getMaximum();
                    extraData->dmin = isDbl->getDisplayMinimum();
                    extraData->dmax = isDbl->getDisplayMaximum();
                } else if (isInt) {
                    extraData->min = isInt->getMinimum();
                    extraData->max = isInt->getMaximum();
                    extraData->dmin = isInt->getDisplayMinimum();
                    extraData->dmax = isInt->getDisplayMaximum();
                } else if (isColor) {
                    extraData->min = isColor->getMinimum();
                    extraData->max = isColor->getMaximum();
                    extraData->dmin = isColor->getDisplayMinimum();
                    extraData->dmax = isColor->getDisplayMaximum();
                }
                _extraData = extraData;
            }

            KnobFile* isFile = dynamic_cast<KnobFile*>( _knob.get() );
            KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>( _knob.get() );
            if (isFile || isOutFile) {
                FileExtraData* extraData = new FileExtraData;
                extraData->useSequences = isFile ? isFile->isInputImageFile() : isOutFile->isOutputImageFile();
                _extraData = extraData;
            }

            KnobPath* isPath = dynamic_cast<KnobPath*>( _knob.get() );
            if (isPath) {
                PathExtraData* extraData = new PathExtraData;
                extraData->multiPath = isPath->isMultiPath();
                _extraData = extraData;
            }
        }
    } // initialize

    ///Constructor used to deserialize: It will try to deserialize the next knob in the archive
    ///into a knob of the holder. If it couldn't find a knob with the same name as it was serialized
    ///this the deserialization will not succeed.
    KnobSerialization()
        : _knob()
        , _dimension(0)
        , _masters()
        , _masterIsAlias(false)
        , _extraData(NULL)
        , _isUserKnob(false)
        , _label()
        , _triggerNewLine(false)
        , _evaluatesOnChange(false)
        , _isPersistent(false)
        , _animationEnabled(false)
        , _tooltip()
        , _useHostOverlay(false)
        , _version(KNOB_SERIALIZATION_VERSION)
    {
    }

    ~KnobSerialization() { delete _extraData; }

    /**
     * @brief This function cannot be called until all knobs of the project have been created.
     **/
    void restoreKnobLinks(const KnobIPtr & knob,
                          const NodesList & allNodes,
                          const std::map<std::string, std::string>& oldNewScriptNamesMapping);

    /**
     * @brief This function cannot be called until all knobs of the project have been created.
     **/
    void restoreExpressions(const KnobIPtr & knob,
                            const std::map<std::string, std::string>& oldNewScriptNamesMapping);

    virtual KnobIPtr getKnob() const OVERRIDE FINAL
    {
        return _knob;
    }

    virtual const std::string& getName() const OVERRIDE FINAL
    {
        return _knob->getName();
    }

    static KnobIPtr createKnob(const std::string & typeName, int dimension);
    const TypeExtraData* getExtraData() const { return _extraData; }

    bool isPersistent() const
    {
        return _isPersistent;
    }

    bool getEvaluatesOnChange() const
    {
        return _evaluatesOnChange;
    }

    bool isAnimationEnabled() const
    {
        return _animationEnabled;
    }

    bool triggerNewLine() const
    {
        return _triggerNewLine;
    }

    bool isUserKnob() const
    {
        return _isUserKnob;
    }

    std::string getLabel() const
    {
        return _label;
    }

    std::string getHintToolTip() const
    {
        return _tooltip;
    }

    bool getUseHostOverlayHandle() const
    {
        return _useHostOverlay;
    }

private:
};


template <typename T>
KnobSerializationPtr
createDefaultValueForParam(const std::string& paramName,
                           const T& value0,
                           const T& value1)
{
    boost::shared_ptr< Knob<T> > knob = boost::make_shared<Knob<T> >(NULL, paramName, 2, false);

    knob->populate();
    knob->setName(paramName);
    knob->setValues(value0, value1, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
    KnobSerializationPtr ret = boost::make_shared<KnobSerialization>(knob);

    return ret;
}

template <typename T>
KnobSerializationPtr
createDefaultValueForParam(const std::string& paramName,
                           const T& value)
{
    boost::shared_ptr< Knob<T> > knob = boost::make_shared<Knob<T> >(NULL, paramName, 1, false);

    knob->populate();
    knob->setName(paramName);
    knob->setValue(value);
    KnobSerializationPtr ret = boost::make_shared<KnobSerialization>(knob);

    return ret;
}

///Used by Groups and Pages for serialization of User knobs, to maintain their layout.
class GroupKnobSerialization
    : public KnobSerializationBase
{
    KnobIPtr _knob;
    std::list<KnobSerializationBasePtr> _children;
    std::string _name, _label;
    bool _secret;
    bool _isSetAsTab; //< only valid for groups
    bool _isOpened; //< only for groups

public:

    GroupKnobSerialization(const KnobIPtr& knob)
        : _knob(knob)
        , _children()
        , _name()
        , _label()
        , _secret(false)
        , _isSetAsTab(false)
        , _isOpened(false)
    {
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>( knob.get() );
        KnobPage* isPage = dynamic_cast<KnobPage*>( knob.get() );

        assert(isGrp || isPage);

        _name = knob->getName();
        _label = knob->getLabel();
        _secret = _knob->getIsSecret();

        if (isGrp) {
            _isSetAsTab = isGrp->isTab();
            _isOpened = isGrp->getValue();
        }

        KnobsVec children;

        if (isGrp) {
            children = isGrp->getChildren();
        } else if (isPage) {
            children = isPage->getChildren();
        }
        for (U32 i = 0; i < children.size(); ++i) {
            if (isPage) {
                ///If page, check that the child is a top level child and not child of a sub-group
                ///otherwise let the sub group register the child
                KnobIPtr parent = children[i]->getParentKnob();
                if (parent.get() != isPage) {
                    continue;
                }
            }
            KnobGroupPtr isGrp = boost::dynamic_pointer_cast<KnobGroup>(children[i]);
            if (isGrp) {
                GroupKnobSerializationPtr serialisation = boost::make_shared<GroupKnobSerialization>(isGrp);
                _children.push_back(serialisation);
            } else {
                //KnobChoice* isChoice = dynamic_cast<KnobChoice*>(children[i].get());
                //bool copyKnob = false;//isChoice != NULL;
                KnobSerializationPtr serialisation = boost::make_shared<KnobSerialization>(children[i]);
                _children.push_back(serialisation);
            }
        }
    }

    GroupKnobSerialization()
        : _knob()
        , _children()
        , _name()
        , _label()
        , _secret(false)
        , _isSetAsTab(false)
        , _isOpened(false)
    {
    }

    ~GroupKnobSerialization()
    {
    }

    const std::list<KnobSerializationBasePtr>& getChildren() const
    {
        return _children;
    }

    virtual KnobIPtr getKnob() const OVERRIDE FINAL
    {
        return _knob;
    }

    virtual const std::string& getName() const OVERRIDE FINAL
    {
        return _name;
    }

    const std::string& getLabel() const
    {
        return _label;
    }

    bool isSecret() const
    {
        return _secret;
    }

    bool isOpened() const
    {
        return _isOpened;
    }

    bool isSetAsTab() const
    {
        return _isSetAsTab;
    }

private:

    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        assert(_knob);
        ar & ::boost::serialization::make_nvp("Name", _name);
        ar & ::boost::serialization::make_nvp("Label", _label);
        ar & ::boost::serialization::make_nvp("Secret", _secret);
        ar & ::boost::serialization::make_nvp("IsTab", _isSetAsTab);
        ar & ::boost::serialization::make_nvp("IsOpened", _isOpened);
        int nbChildren = (int)_children.size();
        ar & ::boost::serialization::make_nvp("NbChildren", nbChildren);
        for (std::list<KnobSerializationBasePtr>::const_iterator it = _children.begin();
             it != _children.end(); ++it) {
            GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>( it->get() );
            KnobSerialization* isRegularKnob = dynamic_cast<KnobSerialization*>( it->get() );
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
              const unsigned int /*version*/)
    {
        assert(!_knob);
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
                GroupKnobSerializationPtr knob = boost::make_shared<GroupKnobSerialization>();
                ar & ::boost::serialization::make_nvp("item", *knob);
                _children.push_back(knob);
            } else {
                KnobSerializationPtr knob = boost::make_shared<KnobSerialization>();
                ar & ::boost::serialization::make_nvp("item", *knob);
                _children.push_back(knob);
            }
        }
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

NATRON_NAMESPACE_EXIT

BOOST_CLASS_VERSION(NATRON_NAMESPACE::KnobSerialization, KNOB_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(NATRON_NAMESPACE::ValueSerialization, VALUE_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(NATRON_NAMESPACE::MasterSerialization, MASTER_SERIALIZATION_VERSION)

#endif // KNOBSERIALIZATION_H
