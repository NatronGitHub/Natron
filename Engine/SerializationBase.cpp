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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "SerializationBase.h"


#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Engine/CurveSerialization.h"
#include "Engine/DockablePanelI.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Project.h"
#include "Engine/ProjectPrivate.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/PyPanelI.h"
#include "Engine/SerializableWindow.h"

NATRON_NAMESPACE_ENTER;


static void
initializeValueSerializationStorage(const KnobIPtr& knob, const int dimension, ValueSerializationStorage* serialization)
{
    serialization->expression = knob->getExpression(dimension);
    serialization->expresionHasReturnVariable = knob->isExpressionUsingRetVariable(dimension);
    CurvePtr curve = knob->getCurve(ViewSpec::current(), dimension);
    if (curve) {
        serialization->animationCurve.reset(new CurveSerialization());
        curve->saveSerialization(serialization->animationCurve.get());
    }

    std::pair< int, KnobIPtr > master = knob->getMaster(dimension);

    if ( master.second && !knob->isMastersPersistenceIgnored() ) {
        serialization->slaveMasterLink.masterDimension = master.first;

        NamedKnobHolderPtr holder = boost::dynamic_pointer_cast<NamedKnobHolder>( master.second->getHolder() );
        assert(holder);

        TrackMarkerPtr isMarker = toTrackMarker(holder);
        if (isMarker) {
            serialization->slaveMasterLink.masterTrackName = isMarker->getScriptName_mt_safe();
            serialization->slaveMasterLink.masterNodeName = isMarker->getContext()->getNode()->getScriptName_mt_safe();
        } else {
            // coverity[dead_error_line]
            serialization->slaveMasterLink.masterNodeName = holder ? holder->getScriptName_mt_safe() : "";
        }
        serialization->slaveMasterLink.masterKnobName = master.second->getName();
    } else {
        serialization->slaveMasterLink.masterDimension = -1;
    }

    serialization->enabled = knob->isEnabled(dimension);

    KnobIntBasePtr isInt = toKnobIntBase(knob);
    KnobBoolBasePtr isBool = toKnobBoolBase(knob);
    KnobDoubleBasePtr isDouble = toKnobDoubleBase(knob);
    KnobChoicePtr isChoice = toKnobChoice(knob);
    KnobStringBasePtr isString = toKnobStringBase(knob);
    KnobParametricPtr isParametric = toKnobParametric(knob);
    KnobPagePtr isPage = toKnobPage(knob);
    KnobGroupPtr isGrp = toKnobGroup(knob);
    KnobSeparatorPtr isSep = toKnobSeparator(knob);
    KnobButtonPtr btn = toKnobButton(knob);

    if (isInt) {
        serialization->value.isInt = isInt->getValue(dimension);
        serialization->type = ValueSerializationStorage::eSerializationValueVariantTypeInteger;
        serialization->defaultValue.isInt = isInt->getDefaultValue(dimension);
    } else if (isBool && !isPage && !isGrp && !isSep) {
        serialization->value.isBool = isBool->getValue(dimension);
        serialization->type = ValueSerializationStorage::eSerializationValueVariantTypeBoolean;
        serialization->defaultValue.isBool = isBool->getDefaultValue(dimension);
    } else if (isDouble && !isParametric) {
        serialization->value.isDouble = isDouble->getValue(dimension);
        serialization->type = ValueSerializationStorage::eSerializationValueVariantTypeDouble;
        serialization->defaultValue.isDouble = isDouble->getDefaultValue(dimension);
    } else if (isString) {
        serialization->value.isString = isString->getValue(dimension);
        serialization->type = ValueSerializationStorage::eSerializationValueVariantTypeString;
        serialization->defaultValue.isString = isString->getDefaultValue(dimension);
    }

} // initializeValueSerializationStorage


void
KnobHelper::toSerialization(SerializationObjectBase* serializationBase)
{

    KnobSerialization* serialization = dynamic_cast<KnobSerialization*>(serializationBase);
    GroupKnobSerialization* groupSerialization = dynamic_cast<GroupKnobSerialization*>(serializationBase);
    assert(serialization || groupSerialization);
    if (!serialization && !groupSerialization) {
        return;
    }

    if (groupSerialization) {
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
        KnobPage* isPage = dynamic_cast<KnobPage*>(this);

        assert(isGrp || isPage);

        groupSerialization->_typeName = typeName();
        groupSerialization->_name = getName();
        groupSerialization->_label = getLabel();
        groupSerialization->_secret = getIsSecret();

        if (isGrp) {
            groupSerialization->_isSetAsTab = isGrp->isTab();
            groupSerialization->_isOpened = isGrp->getValue();
        }

        KnobsVec children;

        if (isGrp) {
            children = isGrp->getChildren();
        } else if (isPage) {
            children = isPage->getChildren();
        }
        for (std::size_t i = 0; i < children.size(); ++i) {
            if (isPage) {
                // If page, check that the child is a top level child and not child of a sub-group
                // otherwise let the sub group register the child
                KnobIPtr parent = children[i]->getParentKnob();
                if (parent.get() != isPage) {
                    continue;
                }
            }
            KnobGroupPtr isGrp = toKnobGroup(children[i]);
            if (isGrp) {
                boost::shared_ptr<GroupKnobSerialization> serialisation( new GroupKnobSerialization(isGrp) );
                groupSerialization->_children.push_back(serialisation);
            } else {
                //KnobChoicePtr isChoice = toKnobChoice(children[i].get());
                //bool copyKnob = false;//isChoice != NULL;
                KnobSerializationPtr serialisation( new KnobSerialization(children[i]) );
                groupSerialization->_children.push_back(serialisation);
            }
        }
    } else {


        KnobIPtr thisShared = shared_from_this();

        serialization->_typeName = typeName();
        serialization->_dimension = getDimension();
        serialization->_scriptName = getName();
        serialization->_isSecret = getIsSecret();



        // Values bits
        serialization->_values.resize(serialization->_dimension);
        for (int i = 0; i < serialization->_dimension; ++i) {
            serialization->_values[i].reset(new ValueSerialization(serialization, serialization->_typeName, i));
            initializeValueSerializationStorage(thisShared, i, &serialization->_values[i]->_value);
        }

        serialization->_masterIsAlias = getAliasMaster().get() != 0;

        // User knobs bits
        serialization->_isUserKnob = isUserKnob();
        serialization->_label = getLabel();
        serialization->_triggerNewLine = isNewLineActivated();
        serialization->_evaluatesOnChange = getEvaluateOnChange();
        serialization->_isPersistent = getIsPersistent();
        serialization->_animationEnabled = isAnimationEnabled();
        serialization->_tooltip = getHintToolTip();
        serialization->_iconFilePath[0] = getIconLabel(false);
        serialization->_iconFilePath[1] = getIconLabel(true);

        // Viewer UI context bits
        if (getHolder()) {
            if (getHolder()->getInViewerContextKnobIndex(thisShared) != -1) {
                serialization->_hasViewerInterface = true;
                serialization->_inViewerContextItemSpacing = getInViewerContextItemSpacing();
                serialization->_inViewerContextItemLayout = getInViewerContextLayoutType();
                serialization->_inViewerContextSecret = getInViewerContextSecret();
                if (serialization->_isUserKnob) {
                    serialization->_inViewerContextLabel = getInViewerContextLabel();
                    serialization->_inViewerContextIconFilePath[0] = getInViewerContextIconFilePath(false);
                    serialization->_inViewerContextIconFilePath[1] = getInViewerContextIconFilePath(true);

                }
            }
        }

        // Per-type specific data
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
        if (isChoice) {
            ChoiceExtraData* extraData = new ChoiceExtraData;
            extraData->_entries = isChoice->getEntries_mt_safe();
            extraData->_helpStrings = isChoice->getEntriesHelp_mt_safe();
            int idx = isChoice->getValue();
            if ( (idx >= 0) && ( idx < (int)extraData->_entries.size() ) ) {
                extraData->_choiceString = extraData->_entries[idx];
            }
            serialization->_extraData.reset(extraData);
        }
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);
        if (isParametric) {
            ParametricExtraData* extraData = new ParametricExtraData;
            isParametric->saveParametricCurves(&extraData->parametricCurves);
            serialization->_extraData.reset(extraData);
        }
        KnobString* isString = dynamic_cast<KnobString*>(this);
        if (isString) {
            TextExtraData* extraData = new TextExtraData;
            isString->getAnimation().save(&extraData->keyframes);
            serialization->_extraData.reset(extraData);
        }
        if (serialization->_isUserKnob) {
            if (isString) {
                TextExtraData* extraData = dynamic_cast<TextExtraData*>(serialization->_extraData.get());
                assert(extraData);
                extraData->label = isString->isLabel();
                extraData->multiLine = isString->isMultiLine();
                extraData->richText = isString->usesRichText();
            }
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>(this);
            KnobInt* isInt = dynamic_cast<KnobInt*>(this);
            KnobColor* isColor = dynamic_cast<KnobColor*>(this);
            if (isDbl || isInt || isColor) {

                ValueExtraData* extraData;
                if (isDbl) {
                    DoubleExtraData* dblData = new DoubleExtraData;
                    dblData->useHostOverlayHandle = serialization->_dimension == 2 && isDbl->getHasHostOverlayHandle();
                    extraData = dblData;
                } else {
                    extraData = new ValueExtraData;
                }
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
                serialization->_extraData.reset(extraData);
            }

            KnobFile* isFile = dynamic_cast<KnobFile*>(this);
            KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>(this);
            if (isFile || isOutFile) {
                FileExtraData* extraData = new FileExtraData;
                extraData->useSequences = isFile ? isFile->isInputImageFile() : isOutFile->isOutputImageFile();
                serialization->_extraData.reset(extraData);
            }
            
            KnobPath* isPath = dynamic_cast<KnobPath*>(this);
            if (isPath) {
                PathExtraData* extraData = new PathExtraData;
                extraData->multiPath = isPath->isMultiPath();
                serialization->_extraData.reset(extraData);
            }
        }
        
        serialization->_hasBeenSerialized = true;
    } // groupSerialization
} // KnobHelper::toSerialization


void
KnobHelper::fromSerialization(const SerializationObjectBase& serializationBase)
{
    // We allow non persistent knobs to be loaded if we found a valid serialization for them
    const KnobSerialization* serialization = dynamic_cast<const KnobSerialization*>(&serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    // Safety checks
    if (!serialization->_hasBeenSerialized) {
        throw std::invalid_argument("Invalid serialization object");
    }
    if (typeName() != serialization->_typeName) {
        throw std::invalid_argument("KnobSerialization::restoreKnobFromSerialization: typename mis-match");
    }

    // Block any instance change action call when loading a knob
    blockValueChanges();
    beginChanges();

    // Restore visibility
    setSecret(serialization->_isSecret);

    // There is a case where the dimension of a parameter might have changed between versions, e.g:
    // the size parameter of the Blur node was previously a Double1D and has become a Double2D to control
    // both dimensions.
    // For compatibility, we do not load only the first dimension, otherwise the result wouldn't be the same,
    // instead we replicate the last dimension of the serialized knob to all other remaining dimensions to fit the
    // knob's dimensions.

    int nDims = std::min( getDimension(), serialization->_dimension );
    for (int i = 0; i < nDims; ++i) {

        // Restore enabled state
        setEnabled(i, serialization->_values[i]->_value.enabled);

        // Clone animation
        if (serialization->_values[i]->_value.animationCurve) {
            CurvePtr curve = getCurve(ViewIdx(0), serialization->_dimension);
            if (curve) {
                curve->loadSerialization(*serialization->_values[i]->_value.animationCurve);
            }
        }

        // Restore value and default value
        switch (serialization->_values[i]->_value.type) {
            case ValueSerializationStorage::eSerializationValueVariantTypeBoolean: {
                KnobBoolBase* isBoolean = dynamic_cast<KnobBoolBase*>(this);
                if (isBoolean) {
                    if (serialization->_values[i]->_version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                        isBoolean->setDefaultValueWithoutApplying(serialization->_values[i]->_value.defaultValue.isBool, i);
                    }
                    isBoolean->setValue(serialization->_values[i]->_value.value.isBool, ViewSpec::all(), i);
                }
            }   break;
            case ValueSerializationStorage::eSerializationValueVariantTypeInteger: {
                // Choice parameters have a custom deserialization for the value because the entry text might no longer
                // exist in the parameter
                KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
                if (isChoice) {
                    const ChoiceExtraData* choiceData = dynamic_cast<const ChoiceExtraData*>(serialization->_extraData.get());
                    assert(choiceData);
                    if (choiceData) {
                        bool found = false;
                        std::vector<std::string> entries = isChoice->getEntries_mt_safe();
                        for (std::size_t i = 0; i < entries.size(); ++i) {
                            if ( boost::iequals(entries[i], choiceData->_choiceString) ) {
                                isChoice->setValue(i);
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            // Fallback on index, but set the string
                            isChoice->setValue(serialization->_values[i]->_value.defaultValue.isInt);
                            isChoice->setActiveEntry(choiceData->_choiceString);
                        }
                    }
                } else {
                    KnobIntBase* isInteger = dynamic_cast<KnobIntBase*>(this);
                    assert(isInteger);
                    if (isInteger) {
                        if (serialization->_values[i]->_version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                            isInteger->setDefaultValueWithoutApplying(serialization->_values[i]->_value.defaultValue.isInt, i);
                        }
                        isInteger->setValue(serialization->_values[i]->_value.value.isInt, ViewSpec::all(), i);
                    }
                }
            }   break;
            case ValueSerializationStorage::eSerializationValueVariantTypeDouble: {
                KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(this);
                assert(isDouble);
                if (isDouble) {
                    if (serialization->_values[i]->_version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                        isDouble->setDefaultValueWithoutApplying(serialization->_values[i]->_value.defaultValue.isDouble, i);
                    }
                    isDouble->setValue(serialization->_values[i]->_value.value.isDouble, ViewSpec::all(), i);
                }
            }   break;
            case ValueSerializationStorage::eSerializationValueVariantTypeString: {
                KnobStringBase* isString = dynamic_cast<KnobStringBase*>(this);
                assert(isString);
                if (isString) {
                    if (serialization->_values[i]->_version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                        isString->setDefaultValueWithoutApplying(serialization->_values[i]->_value.defaultValue.isString, i);
                    }
                    isString->setValue(serialization->_values[i]->_value.value.isString, ViewSpec::all(), i);
                }
            }   break;
            case ValueSerializationStorage::eSerializationValueVariantTypeNone:
                break;
        } // type
    } // for all dims

    // Restore extra datas
    KnobFile* isInFile = dynamic_cast<KnobFile*>(this);
    AnimatingKnobStringHelper* animatedStringKnob = dynamic_cast<AnimatingKnobStringHelper*>(this);
    if (animatedStringKnob) {

        // Don't load animation for input image files: they no longer hold keyframes since Natron 1.0
        // In the Reader context, the script name must be kOfxImageEffectFileParamName, @see kOfxImageEffectContextReader
        if ( !isInFile || ( isInFile && (isInFile->getName() != kOfxImageEffectFileParamName) ) ) {
            const TextExtraData* data = dynamic_cast<const TextExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data) {
                animatedStringKnob->loadAnimation(data->keyframes);
            }
        }
    }

    // Load parametric parameter's curves
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);
    if (isParametric) {
        const ParametricExtraData* data = dynamic_cast<const ParametricExtraData*>(serialization->_extraData.get());
        assert(data);
        if (data) {
            isParametric->loadParametricCurves(data->parametricCurves);
        }
    }


    // Restore user knobs bits
    if (serialization->_isUserKnob) {
        setAsUserKnob(true);
        setIsPersistent(serialization->_isPersistent);
        setAnimationEnabled(serialization->_animationEnabled);
        setEvaluateOnChange(serialization->_evaluatesOnChange);
        setName(serialization->_scriptName);
        setHintToolTip(serialization->_tooltip);
        setAddNewLine(serialization->_triggerNewLine);
        setIconLabel(serialization->_iconFilePath[0], false);
        setIconLabel(serialization->_iconFilePath[1], true);

        KnobInt* isInt = dynamic_cast<KnobInt*>(this);
        KnobDouble* isDouble = dynamic_cast<KnobDouble*>(this);
        KnobColor* isColor = dynamic_cast<KnobColor*>(this);
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
        KnobString* isString = dynamic_cast<KnobString*>(this);
        KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>(this);
        KnobPath* isPath = dynamic_cast<KnobPath*>(this);

        if (isInt) {
            const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data) {
                std::vector<int> minimums, maximums, dminimums, dmaximums;
                for (int i = 0; i < nDims; ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                isInt->setMinimumsAndMaximums(minimums, maximums);
                isInt->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
            }
        } else if (isDouble) {
            const DoubleExtraData* data = dynamic_cast<const DoubleExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data) {
                std::vector<double> minimums, maximums, dminimums, dmaximums;
                for (int i = 0; i < nDims; ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                isDouble->setMinimumsAndMaximums(minimums, maximums);
                isDouble->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                if (data->useHostOverlayHandle) {
                    isDouble->setHasHostOverlayHandle(true);
                }
            }

        } else if (isChoice) {
            const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data) {
                isChoice->populateChoices(data->_entries, data->_helpStrings);
            }
        } else if (isColor) {
            const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data) {
                std::vector<double> minimums, maximums, dminimums, dmaximums;
                for (int i = 0; i < nDims; ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                isColor->setMinimumsAndMaximums(minimums, maximums);
                isColor->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
            }
        } else if (isString) {
            const TextExtraData* data = dynamic_cast<const TextExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data) {
                if (data->label) {
                    isString->setAsLabel();
                } else if (data->multiLine) {
                    isString->setAsMultiLine();
                    if (data->richText) {
                        isString->setUsesRichText(true);
                    }
                }
            }

        } else if (isInFile || isOutFile) {
            const FileExtraData* data = dynamic_cast<const FileExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data && data->useSequences) {
                if (isInFile) {
                    isInFile->setAsInputImage();
                } else if (isOutFile) {
                    isOutFile->setAsOutputImageFile();
                }
            }
        } else if (isPath) {
            const PathExtraData* data = dynamic_cast<const PathExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data && data->multiPath) {
                isPath->setMultiPath(true);
            }
        }

    } // isUserKnob

    // Restore viewer UI context
    if (serialization->_hasViewerInterface) {
        setInViewerContextItemSpacing(serialization->_inViewerContextItemSpacing);
        setInViewerContextLayoutType((ViewerContextLayoutTypeEnum)serialization->_inViewerContextItemLayout);
        setInViewerContextSecret(serialization->_inViewerContextSecret);
        if (isUserKnob()) {
            setInViewerContextLabel(QString::fromUtf8(serialization->_inViewerContextLabel.c_str()));
            setInViewerContextIconFilePath(serialization->_inViewerContextIconFilePath[0], false);
            setInViewerContextIconFilePath(serialization->_inViewerContextIconFilePath[1], true);
        }
    }
    
    //allow changes again
    endChanges();
    unblockValueChanges();
} // KnobHelper::fromSerialization


void
Project::toSerialization(SerializationObjectBase* serializationBase)
{
    // All the code in this function is MT-safe and run in the serialization thread
    ProjectSerialization* serialization = dynamic_cast<ProjectSerialization*>(serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    // Serialize nodes
    {
        NodesList nodes;
        getActiveNodes(&nodes);
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if ( !(*it)->getParentMultiInstance() && (*it)->isPartOfProject() ) {
                NodeSerializationPtr state( new NodeSerialization(*it) );
                serialization->_nodes.push_back(state);
            }
        }
    }

    // Get user additional formats
    std::list<Format> formats;
    getAdditionalFormats(&formats);
    for (std::list<Format>::iterator it = formats.begin(); it!=formats.end(); ++it) {
        FormatSerialization s;
        s.x1 = it->x1;
        s.y1 = it->y1;
        s.x2 = it->x2;
        s.y2 = it->y2;
        s.par = it->getPixelAspectRatio();
        s.name = it->getName();
        serialization->_additionalFormats.push_back(s);
    }

    // Serialize project settings
    std::vector< KnobIPtr > knobs = getKnobs_mt_safe();
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobGroupPtr isGroup = toKnobGroup(knobs[i]);
        KnobPagePtr isPage = toKnobPage(knobs[i]);
        KnobButtonPtr isButton = toKnobButton(knobs[i]);
        if ( knobs[i]->getIsPersistent() &&
            !isGroup && !isPage && !isButton &&
            knobs[i]->hasModificationsForSerialization() ) {
            KnobSerializationPtr newKnobSer( new KnobSerialization(knobs[i]) );
            serialization->_projectKnobs.push_back(newKnobSer);
        }
    }

    // Timeline's current frame
    serialization->_timelineCurrent = currentFrame();

    // Remember project creation date
    serialization->_creationDate = getProjectCreationTime();

    // Serialize workspace
    serialization->_projectWorkspace.reset(new ProjectWorkspaceSerialization);

    // Floating windows
    std::list<SerializableWindow*> floatingWindows = getApp()->getFloatingWindowsSerialization();
    for (std::list<SerializableWindow*>::const_iterator it = floatingWindows.begin(); it != floatingWindows.end(); ++it) {
        boost::shared_ptr<ProjectWindowSerialization> s(new ProjectWindowSerialization());
        (*it)->toSerialization(s.get());
        serialization->_projectWorkspace->_floatingWindowsSerialization.push_back(s);
    }

    getApp()->saveApplicationWorkspace(serialization->_projectWorkspace.get());
   
    // Save opened panels
    std::list<DockablePanelI*> openedPanels = getApp()->getOpenedSettingsPanels();
    for (std::list<DockablePanelI*>::iterator it = openedPanels.begin(); it!=openedPanels.end(); ++it) {
        serialization->_openedPanelsOrdered.push_back((*it)->getHolderFullyQualifiedScriptName());
    }

    // Save viewports
    getApp()->getViewportsProjection(&serialization->_viewportsData);
    
} // Project::toSerialization


void
Project::fromSerialization(const SerializationObjectBase& serializationBase)
{

    // All the code in this function is MT-safe and run in the serialization thread
    const ProjectSerialization* serialization = dynamic_cast<const ProjectSerialization*>(&serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    {
        CreatingNodeTreeFlag_RAII creatingNodeTreeFlag(getApp());

        // Restore project creation date
        _imp->projectCreationTime = QDateTime::fromMSecsSinceEpoch( serialization->_creationDate );

        getApp()->updateProjectLoadStatus( tr("Restoring project settings...") );

        // Restore project formats
        // We must restore the entries in the combobox before restoring the value
        std::vector<std::string> entries;
        for (std::list<Format>::const_iterator it = _imp->builtinFormats.begin(); it != _imp->builtinFormats.end(); ++it) {
            QString formatStr = ProjectPrivate::generateStringFromFormat(*it);
            entries.push_back( formatStr.toStdString() );
        }

        {
            _imp->additionalFormats.clear();
            const std::list<FormatSerialization> & objAdditionalFormats = serialization->getAdditionalFormats();
            for (std::list<FormatSerialization>::const_iterator it = objAdditionalFormats.begin(); it != objAdditionalFormats.end(); ++it) {
                Format f;
                f.setName(it->name);
                f.setPixelAspectRatio(it->par);
                f.x1 = it->x1;
                f.y1 = it->y1;
                f.x2 = it->x2;
                f.y2 = it->y2;
                _imp->additionalFormats.push_back(f);
            }
            for (std::list<Format>::const_iterator it = _imp->additionalFormats.begin(); it != _imp->additionalFormats.end(); ++it) {
                QString formatStr = ProjectPrivate::generateStringFromFormat(*it);
                entries.push_back( formatStr.toStdString() );
            }
        }

        _imp->formatKnob->populateChoices(entries);
        _imp->autoSetProjectFormat = false;

        const KnobsVec& projectKnobs = getKnobs();

        // Restore project's knobs
        for (std::size_t i = 0; i < projectKnobs.size(); ++i) {
            // Try to find a serialized value for this knob
            for (std::list<KnobSerializationPtr>::const_iterator it = serialization->_projectKnobs.begin(); it != serialization->_projectKnobs.end(); ++it) {
                if ( (*it)->getName() == projectKnobs[i]->getName() ) {
                    projectKnobs[i]->fromSerialization(**it);
                    break;
                }
            }
            if (projectKnobs[i] == _imp->envVars) {
                onOCIOConfigPathChanged(appPTR->getOCIOConfigPath(), false);
            } else if (projectKnobs[i] == _imp->natronVersion) {
                std::string v = _imp->natronVersion->getValue();
                if (v == "Natron v1.0.0") {
                    getApp()->setProjectWasCreatedWithLowerCaseIDs(true);
                }
            }
        }

        // Restore the timeline
        _imp->timeline->seekFrame(serialization->_timelineCurrent, false, OutputEffectInstancePtr(), eTimelineChangeReasonOtherSeek);


        // Restore the nodes
        std::map<std::string, bool> processedModules;
        ProjectPrivate::restoreGroupFromSerialization(serialization->_nodes, shared_from_this(), true, &processedModules);
        getApp()->updateProjectLoadStatus( tr("Restoring graph stream preferences...") );
    } // CreatingNodeTreeFlag_RAII creatingNodeTreeFlag(_publicInterface->getApp());


    // Recompute all meta-datas and stuff depending on the trees now
    forceComputeInputDependentDataOnAllTrees();

    QDateTime time = QDateTime::currentDateTime();
    _imp->hasProjectBeenSavedByUser = true;
    _imp->ageSinceLastSave = time;
    _imp->lastAutoSave = time;
    getApp()->setProjectWasCreatedWithLowerCaseIDs(false);

    if (serialization->_version < PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS) {
        recomputeFrameRangeFromReaders();
    }

} // Project::fromSerialization


NATRON_NAMESPACE_EXIT;