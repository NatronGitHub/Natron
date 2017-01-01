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

#include "KnobTypes.h"

#include <cfloat>
#include <locale>
#include <sstream>
#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/Curve.h"
#include "Engine/ColorParser.h"
#include "Engine/EffectInstance.h"
#include "Engine/Format.h"
#include "Engine/Image.h"
#include "Engine/KnobFile.h"
#include "Engine/Hash64.h"
#include "Engine/Node.h"
#include "Engine/RenderValuesCache.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"

#include "Serialization/CurveSerialization.h"
#include "Serialization/KnobSerialization.h"

NATRON_NAMESPACE_ENTER;

//using std::make_pair;
//using std::pair;

/******************************KnobInt**************************************/
KnobInt::KnobInt(const KnobHolderPtr& holder,
                 const std::string &label,
                 int dimension,
                 bool declaredByPlugin)
    : KnobIntBase(holder, label, dimension, declaredByPlugin)
    , _increments(dimension, 1)
    , _disableSlider(false)
    , _isRectangle(false)
    , _isValueCenteredInSpinbox(false)
    , _isShortcutKnob(false)
{
}

void
KnobInt::disableSlider()
{
    _disableSlider = true;
}

bool
KnobInt::isSliderDisabled() const
{
    return _disableSlider;
}

void
KnobInt::setIncrement(int incr,
                      DimIdx index)
{
    if (incr <= 0) {
        qDebug() << "Attempting to set the increment of an int param to a value lesser or equal to 0";

        return;
    }

    if ( index >= (int)_increments.size() ) {
        throw std::runtime_error("KnobInt::setIncrement , dimension out of range");
    }
    _increments[index] = incr;
    Q_EMIT incrementChanged(_increments[index], index);
}

void
KnobInt::setIncrement(const std::vector<int> &incr)
{
    assert( (int)incr.size() == getNDimensions() );
    _increments = incr;
    for (U32 i = 0; i < _increments.size(); ++i) {
        if (_increments[i] <= 0) {
            qDebug() << "Attempting to set the increment of an int param to a value lesser or equal to 0";
            continue;
        }
        Q_EMIT incrementChanged(_increments[i], DimIdx(i));
    }
}

const std::vector<int> &
KnobInt::getIncrements() const
{
    return _increments;
}

bool
KnobInt::canAnimate() const
{
    return true;
}

const std::string KnobInt::_typeNameStr(kKnobIntTypeName);
const std::string &
KnobInt::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobInt::typeName() const
{
    return typeNameStatic();
}

/******************************KnobBool**************************************/

KnobBool::KnobBool(const KnobHolderPtr& holder,
                   const std::string &label,
                   int dimension,
                   bool declaredByPlugin)
    : KnobBoolBase(holder, label, dimension, declaredByPlugin)
{
}

bool
KnobBool::canAnimate() const
{
    return canAnimateStatic();
}

const std::string KnobBool::_typeNameStr(kKnobBoolTypeName);
const std::string &
KnobBool::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobBool::typeName() const
{
    return typeNameStatic();
}

/******************************KnobDouble**************************************/


KnobDouble::KnobDouble(const KnobHolderPtr& holder,
                       const std::string &label,
                       int dimension,
                       bool declaredByPlugin)
    : KnobDoubleBase(holder, label, dimension, declaredByPlugin)
    , _spatial(false)
    , _isRectangle(false)
    , _increments(dimension, 1)
    , _decimals(dimension, 2)
    , _disableSlider(false)
    , _valueIsNormalized(dimension, eValueIsNormalizedNone)
    , _defaultValuesAreNormalized(false)
    , _hasHostOverlayHandle(false)
{
}

void
KnobDouble::setHasHostOverlayHandle(bool handle)
{
    KnobHolderPtr holder = getHolder();

    if (holder) {
        EffectInstancePtr effect = toEffectInstance(holder);
        if (!effect) {
            return;
        }
        if ( !effect->getNode() ) {
            return;
        }
        KnobDoublePtr thisSharedDouble = toKnobDouble(shared_from_this());
        assert(thisSharedDouble);
        if (handle) {
            effect->getNode()->addPositionInteract(thisSharedDouble,
                                                   KnobBoolPtr() /*interactive*/);
        } else {
            effect->getNode()->removePositionHostOverlay( shared_from_this() );
        }
        _hasHostOverlayHandle = handle;
    }
}

bool
KnobDouble::getHasHostOverlayHandle() const
{
    return _hasHostOverlayHandle;
}

void
KnobDouble::disableSlider()
{
    _disableSlider = true;
}

bool
KnobDouble::isSliderDisabled() const
{
    return _disableSlider;
}

bool
KnobDouble::canAnimate() const
{
    return true;
}

void
KnobDouble::setValueIsNormalized(DimIdx dimension,
                          ValueIsNormalizedEnum state)
{
    if (dimension < 0 || dimension >= (int)_valueIsNormalized.size()) {
        throw std::invalid_argument("KnobDouble::setValueIsNormalized: dimension out of range");
    }
    _valueIsNormalized[dimension] = state;

}

void
KnobDouble::setDefaultValuesAreNormalized(bool normalized)
{
    _defaultValuesAreNormalized = normalized;
}

void
KnobDouble::setSpatial(bool spatial)
{
    _spatial = spatial;
}


const std::string KnobDouble::_typeNameStr(kKnobDoubleTypeName);
const std::string &
KnobDouble::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobDouble::typeName() const
{
    return typeNameStatic();
}

const std::vector<double> &
KnobDouble::getIncrements() const
{
    return _increments;
}

const std::vector<int> &
KnobDouble::getDecimals() const
{
    return _decimals;
}

void
KnobDouble::setIncrement(double incr,
                         DimIdx index)
{
    if (incr <= 0.) {
        qDebug() << "Attempting to set the increment of a double param to a value lesser or equal to 0.";

        return;
    }
    if ( index >= (int)_increments.size() ) {
        throw std::runtime_error("KnobDouble::setIncrement , dimension out of range");
    }

    _increments[index] = incr;
    Q_EMIT incrementChanged(_increments[index], index);
}

void
KnobDouble::setDecimals(int decis,
                        DimIdx index)
{
    if ( index >= (int)_decimals.size() ) {
        throw std::runtime_error("KnobDouble::setDecimals , dimension out of range");
    }

    _decimals[index] = decis;
    Q_EMIT decimalsChanged(_decimals[index], index);
}

void
KnobDouble::setIncrement(const std::vector<double> &incr)
{
    assert( incr.size() == (U32)getNDimensions() );
    _increments = incr;
    for (U32 i = 0; i < incr.size(); ++i) {
        Q_EMIT incrementChanged(_increments[i], DimIdx(i));
    }
}

void
KnobDouble::setDecimals(const std::vector<int> &decis)
{
    assert( decis.size() == (U32)getNDimensions() );
    _decimals = decis;
    for (U32 i = 0; i < decis.size(); ++i) {
        Q_EMIT decimalsChanged(decis[i], DimIdx(i));
    }
}

KnobDouble::~KnobDouble()
{
}

static void
getNormalizeRect(const EffectInstancePtr& effect,
            double /*time*/,
            RectD & rod)
{
#ifdef NATRON_NORMALIZE_SPATIAL_WITH_ROD
    RenderScale scale;
    scale.y = scale.x = 1.;
    Status stat = effect->getRegionOfDefinition_public(0, time, scale, /*view*/ 0, &rod);
    if ( (stat == StatFailed) || ( (rod.x1 == 0) && (rod.y1 == 0) && (rod.x2 == 1) && (rod.y2 == 1) ) ) {
        Format f;
        effect->getRenderFormat(&f);
        rod = f;
    }
#else
    Format f;
    effect->getApp()->getProject()->getProjectDefaultFormat(&f);
    rod = f.toCanonicalFormat();
#endif
}

double
KnobDouble::denormalize(DimIdx dimension,
                        double time,
                        double value) const
{
    EffectInstancePtr effect = toEffectInstance( getHolder() );
    if (!effect) {
        // coverity[dead_error_line]
        return value;
    }
    RectD rod;
    getNormalizeRect(effect, time, rod);
    ValueIsNormalizedEnum e = getValueIsNormalized(dimension);
    // the second expression (with e == eValueIsNormalizedNone) is used when denormalizing default values
    if ( (e == eValueIsNormalizedX) || ( (e == eValueIsNormalizedNone) && (dimension == 0) ) ) {
        return value * rod.width();
    } else if ( (e == eValueIsNormalizedY) || ( (e == eValueIsNormalizedNone) && (dimension == 1) ) ) {
        return value * rod.height();
    }

    return value;
}

double
KnobDouble::normalize(DimIdx dimension,
                     double time,
                     double value) const
{
    EffectInstancePtr effect = toEffectInstance( getHolder() );

    assert(effect);
    if (!effect) {
        // coverity[dead_error_line]
        return value;
    }
    RectD rod;
    getNormalizeRect(effect, time, rod);
    ValueIsNormalizedEnum e = getValueIsNormalized(dimension);
    // the second expression (with e == eValueIsNormalizedNone) is used when normalizing default values
    if ( (e == eValueIsNormalizedX) || ( (e == eValueIsNormalizedNone) && (dimension == 0) ) ) {
        return value / rod.width();
    } else if ( (e == eValueIsNormalizedY) || ( (e == eValueIsNormalizedNone) && (dimension == 1) ) ) {
        return value / rod.height();
    }

    return value;
}

bool
KnobDouble::hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const
{
    if (Knob<double>::hasModificationsVirtual(data, dimension)) {
        return true;
    }

    ValueKnobDimView<double>* doubleData = dynamic_cast<ValueKnobDimView<double>*>(data.get());
    assert(doubleData);

    double defaultValue = getDefaultValue(dimension);
    if (_defaultValuesAreNormalized) {
        double denormalizedDefaultValue = denormalize(dimension, 0, defaultValue);
        QMutexLocker k(&doubleData->valueMutex);
        return doubleData->value != denormalizedDefaultValue;
    } else {
        QMutexLocker k(&doubleData->valueMutex);
        return doubleData->value != defaultValue;
    }
}


/******************************KnobButton**************************************/

KnobButton::KnobButton(const KnobHolderPtr& holder,
                       const std::string &label,
                       int dimension,
                       bool declaredByPlugin)
    : KnobBoolBase(holder, label, dimension, declaredByPlugin)
    , _renderButton(false)
    , _checkable(false)
    , _isToolButtonAction(false)
{
    //setIsPersistent(false);
}

bool
KnobButton::canAnimate() const
{
    return false;
}

const std::string KnobButton::_typeNameStr(kKnobButtonTypeName);
const std::string &
KnobButton::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobButton::typeName() const
{
    return typeNameStatic();
}

bool
KnobButton::trigger()
{
    return evaluateValueChange(DimSpec(0), getCurrentTime(), ViewSetSpec(0),  eValueChangedReasonUserEdited);
}

/******************************KnobChoice**************************************/

// don't show help in the tootlip if there are more entries that this
#define KNOBCHOICE_MAX_ENTRIES_HELP 40

ChoiceKnobDimView::ChoiceKnobDimView()
: ValueKnobDimView<int>()
, menuOptions()
, menuOptionTooltips()
, activeEntry()
, separators()
, shortcuts()
, menuIcons()
, addNewChoiceCallback(0)
, textToFitHorizontally()
, isCascading(false)
, showMissingEntryWarning(true)
, menuColors()
{

}

bool
ChoiceKnobDimView::setValueAndCheckIfChanged(const int& v)
{
    ValueKnobDimView<int>::setValueAndCheckIfChanged(v);

    QMutexLocker k(&valueMutex);
    std::string newChoice;
    if (v >= 0 && v < (int)menuOptions.size()) {
        newChoice = menuOptions[v];
    } else {
        // No current value, assume they are different
        return true;
    }
    if (activeEntry != newChoice) {
        activeEntry = newChoice;
        return true;
    }
    return false;
}

bool
ChoiceKnobDimView::copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs)
{
    bool hasChanged = ValueKnobDimView<int>::copy(inArgs, outArgs);

    const ChoiceKnobDimView* otherType = dynamic_cast<const ChoiceKnobDimView*>(inArgs.other);
    assert(otherType);

    QMutexLocker k(&valueMutex);
    QMutexLocker k2(&inArgs.other->valueMutex);

    menuOptions = otherType->menuOptions;
    menuOptionTooltips = otherType->menuOptionTooltips;
    separators = otherType->separators;
    shortcuts = otherType->shortcuts;
    menuIcons = otherType->menuIcons;
    addNewChoiceCallback = otherType->addNewChoiceCallback;
    textToFitHorizontally = otherType->textToFitHorizontally;
    isCascading = otherType->isCascading;
    showMissingEntryWarning = otherType->showMissingEntryWarning;
    menuColors = otherType->menuColors;

    if (activeEntry != otherType->activeEntry) {
        activeEntry = otherType->activeEntry;
        hasChanged = true;
    }

    return hasChanged;
}

KnobChoice::KnobChoice(const KnobHolderPtr& holder,
                       const std::string &label,
                       int nDims,
                       bool declaredByPlugin)
: KnobIntBase(holder, label, nDims, declaredByPlugin)
{
}

KnobChoice::~KnobChoice()
{
}

void
KnobChoice::setMissingEntryWarningEnabled(bool enabled)
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    data->showMissingEntryWarning = enabled;
}

bool
KnobChoice::isMissingEntryWarningEnabled() const
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    return data->showMissingEntryWarning;
}

void
KnobChoice::setColorForIndex(int index, const RGBAColourD& color)
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    data->menuColors[index] = color;
}

bool
KnobChoice::getColorForIndex(int index, RGBAColourD* color) const
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    std::map<int, RGBAColourD>::const_iterator found = data->menuColors.find(index);
    if (found == data->menuColors.end()) {
        return false;
    }
    *color = found->second;
    return true;
}

void
KnobChoice::setTextToFitHorizontally(const std::string& text)
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    data->textToFitHorizontally = text;
}

std::string
KnobChoice::getTextToFitHorizontally() const
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    return data->textToFitHorizontally;
}

void
KnobChoice::setNewOptionCallback(ChoiceKnobDimView::KnobChoiceNewItemCallback callback)
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    data->addNewChoiceCallback = callback;
}

ChoiceKnobDimView::KnobChoiceNewItemCallback
KnobChoice::getNewOptionCallback() const
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    return data->addNewChoiceCallback;
}

void
KnobChoice::setCascading(bool cascading)
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    data->isCascading = cascading;
}

bool
KnobChoice::isCascading() const
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    return data->isCascading;
}

void
KnobChoice::onLinkChanged()
{
    // We changed data, refresh the menu
    Q_EMIT populated();
}

bool
KnobChoice::canLinkWith(const KnobIPtr & other, DimIdx thisDimension, ViewIdx thisView, DimIdx otherDim, ViewIdx otherView, std::string* error) const
{
    if (!Knob<int>::canLinkWith(other, thisDimension, thisView, otherDim, otherView, error)) {
        return false;
    }
    KnobChoice* otherIsChoice = dynamic_cast<KnobChoice*>(other.get());
    if (!otherIsChoice) {
        if (error) {
            *error = tr("You can only copy/paste between parameters of the same type. To overcome this, use an expression instead.").toStdString();
        }
        return false;
    }
    ChoiceKnobDimViewPtr otherData = toChoiceKnobDimView(otherIsChoice->getDataForDimView(otherDim, otherView));
    ChoiceKnobDimViewPtr thisData = toChoiceKnobDimView(getDataForDimView(thisDimension, thisView));
    assert(otherData && thisData);
    if ( !otherData || !thisData) {
        return false;
    }

    // Choice parameters with different menus cannot be linked
    QString menuDifferentError = tr("You cannot link choice parameters with different menus. To overcome this, use an expression instead.");
    std::vector<std::string> thisOptions, otherOptions;
    {
        QMutexLocker k(&thisData->valueMutex);
        thisOptions = thisData->menuOptions;
    }
    {
        QMutexLocker k(&otherData->valueMutex);
        otherOptions = otherData->menuOptions;
    }
    if (thisOptions.size() != otherOptions.size()) {
        *error = menuDifferentError.toStdString();
        return false;
    }
    for (std::size_t i = 0; i < thisOptions.size(); ++i) {
        if (thisOptions[i] != otherOptions[i]) {
            *error = menuDifferentError.toStdString();
            return false;
        }
    }
    return true;
} // canLinkWith


bool
KnobChoice::canAnimate() const
{
    return canAnimateStatic();
}

const std::string KnobChoice::_typeNameStr(kKnobChoiceTypeName);
const std::string &
KnobChoice::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobChoice::typeName() const
{
    return typeNameStatic();
}


bool
KnobChoice::hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const
{
    if (Knob<int>::hasModificationsVirtual(data, dimension)) {
        return true;
    }
    int def_i = getDefaultValue(dimension);
    std::string defaultVal;

    ChoiceKnobDimViewPtr choiceData = toChoiceKnobDimView(data);
    assert(choiceData);
    QMutexLocker k(&data->valueMutex);

    if (def_i >= 0 && def_i < (int)choiceData->menuOptions.size()) {
        defaultVal = choiceData->menuOptions[def_i];
    }

    if (choiceData->activeEntry != defaultVal) {
        return true;
    }

    return false;
}



static bool
stringEqualFunctor(const std::string& a,
                   const std::string& b,
                   KnobChoiceMergeEntriesData* /*data*/)
{
    return a == b;
}

void
KnobChoice::findAndSetOldChoice(MergeMenuEqualityFunctor mergingFunctor,
                                KnobChoiceMergeEntriesData* mergingData)
{
    std::list<ViewIdx> views = getViewsList();
    if (views.empty()) {
        return;
    }

    // Make sure we don't call knobChanged if we found the value
    blockValueChanges();
    beginChanges();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {

        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), *it));
        assert(data);

        QMutexLocker k(&data->valueMutex);

        if ( !data->activeEntry.empty() ) {
            if (mergingFunctor) {
                assert(mergingData);
                mergingData->clear();
            } else {
                mergingFunctor = stringEqualFunctor;
            }
            int found = -1;
            {
                for (std::size_t i = 0; i < data->menuOptions.size(); ++i) {
                    if ( mergingFunctor(data->menuOptions[i], data->activeEntry, mergingData) ) {
                        found = i;

                        // Update the label if different
                        data->activeEntry = data->menuOptions[i];
                        break;
                    }
                }
            }
            if (found != -1) {
                k.unlock();
                setValue(found, ViewSetSpec(*it));
            }
        }
    } // for all views
    unblockValueChanges();
    endChanges();

}

bool
KnobChoice::populateChoices(const std::vector<std::string> &entries,
                            const std::vector<std::string> &entriesHelp,
                            MergeMenuEqualityFunctor mergingFunctor,
                            KnobChoiceMergeEntriesData* mergingData)
{
    assert( entriesHelp.empty() || entriesHelp.size() == entries.size() );
    bool hasChanged = false;
    KnobDimViewKeySet sharedKnobs;
    {
        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
        assert(data);

        QMutexLocker k(&data->valueMutex);
        sharedKnobs = data->sharedKnobs;
        if (!mergingFunctor) {
            // No merging functor, replace
            data->menuOptions = entries;
            data->menuOptionTooltips = entriesHelp;
            hasChanged = true;
        } else {
            assert(mergingData);
            // If there is a merging functor to merge current entries with new entries, do the merging

            // For all new entries, check if one of the merged entry matches and then merge
            // otherwise add to the merged entries
            for (std::size_t i = 0; i < entries.size(); ++i) {
                mergingData->clear();
                bool found = false;
                for (std::size_t j = 0; j < data->menuOptions.size(); ++j) {
                    if ( mergingFunctor(data->menuOptions[j], entries[i], mergingData) ) {
                        if (data->menuOptions[j] != entries[i]) {
                            hasChanged = true;
                            data->menuOptions[j] = entries[i];
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    hasChanged = true;
                    if ( i < data->menuOptionTooltips.size() ) {
                        data->menuOptionTooltips.push_back(entriesHelp[i]);
                    }
                    data->menuOptions.push_back(entries[i]);
                }
            }
        }
    } // QMutexLocker

    if (hasChanged) {

        //  Try to restore the last choice.
        findAndSetOldChoice(mergingFunctor, mergingData);

        for (KnobDimViewKeySet::const_iterator it = sharedKnobs.begin(); it!=sharedKnobs.end(); ++it) {
            KnobChoicePtr sharedKnob = toKnobChoice(it->knob.lock());
            assert(sharedKnob);
            if (!sharedKnob) {
                continue;
            }
            // Notify tooltip changed because we changed the menu entries
            sharedKnob->_signalSlotHandler->s_helpChanged();

            Q_EMIT sharedKnob->populated();
        }

    }

    return hasChanged;
} // KnobChoice::populateChoices


void
KnobChoice::setShortcuts(const std::map<int, std::string>& shortcuts)
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    data->shortcuts = shortcuts;
}

std::map<int, std::string>
KnobChoice::getShortcuts() const
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    return data->shortcuts;
}

void
KnobChoice::setIcons(const std::map<int, std::string>& icons)
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    data->menuIcons = icons;

}

std::map<int, std::string>
KnobChoice::getIcons() const
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    return data->menuIcons;
}

void
KnobChoice::setSeparators(const std::vector<int>& separators)
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    data->separators = separators;
}

std::vector<int>
KnobChoice::getSeparators() const
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);
    return data->separators;
}


void
KnobChoice::resetChoices(ViewSetSpec view)
{
    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
            if (view_i != *it) {
                continue;
            }
        }


        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), *it));
        if (!data) {
            continue;
        }
        KnobDimViewKeySet sharedKnobs;
        {
            QMutexLocker k(&data->valueMutex);
            sharedKnobs = data->sharedKnobs;
            data->menuOptions.clear();
            data->menuOptionTooltips.clear();

        }

        for (KnobDimViewKeySet::const_iterator it = sharedKnobs.begin(); it!=sharedKnobs.end(); ++it) {
            KnobChoicePtr sharedKnob = toKnobChoice(it->knob.lock());
            assert(sharedKnob);
            if (!sharedKnob) {
                continue;
            }
            // Notify tooltip changed because we changed the menu entries
            sharedKnob->_signalSlotHandler->s_helpChanged();

            Q_EMIT sharedKnob->entriesReset();
        }

    }

    // Refresh active entry state
    findAndSetOldChoice();


}

void
KnobChoice::appendChoice(const std::string& entry,
                         const std::string& help,
                         ViewSetSpec view)
{
    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
            if (view_i != *it) {
                continue;
            }
        }

        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), *it));
        if (!data) {
            continue;
        }
        KnobDimViewKeySet sharedKnobs;
        {
            QMutexLocker k(&data->valueMutex);
            data->menuOptions.push_back(entry);
            data->menuOptionTooltips.push_back(help);
            sharedKnobs = data->sharedKnobs;
        }
        for (KnobDimViewKeySet::const_iterator it = sharedKnobs.begin(); it!=sharedKnobs.end(); ++it) {
            KnobChoicePtr sharedKnob = toKnobChoice(it->knob.lock());
            assert(sharedKnob);
            if (!sharedKnob) {
                continue;
            }
            // Notify tooltip changed because we changed the menu entries
            sharedKnob->_signalSlotHandler->s_helpChanged();

            Q_EMIT sharedKnob->entryAppended( QString::fromUtf8( entry.c_str() ), QString::fromUtf8( help.c_str() ) );
        }
    }

    // Refresh active entry state
    findAndSetOldChoice();
}

std::vector<std::string>
KnobChoice::getEntries(ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    {
        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), view_i));
        if (!data) {
            return std::vector<std::string>();
        }
        QMutexLocker k(&data->valueMutex);
        return data->menuOptions;
    }

}

std::vector<std::string>
KnobChoice::getEntriesHelp(ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    {
        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), view_i));
        if (!data) {
            return std::vector<std::string>();
        }
        QMutexLocker k(&data->valueMutex);
        return data->menuOptionTooltips;
    }
}

bool
KnobChoice::isActiveEntryPresentInEntries(ViewIdx view) const
{

    {
        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), view));
        if (!data) {
            return false;
        }
        QMutexLocker k(&data->valueMutex);
        for (std::size_t i = 0; i < data->menuOptions.size(); ++i) {
            if (data->menuOptions[i] == data->activeEntry) {
                return true;
            }
        }

    }
    return false;
}

std::string
KnobChoice::getEntry(int v, ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    {
        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), view_i));
        if (!data) {
            return std::string();
        }
        QMutexLocker k(&data->valueMutex);
        if (v < 0 || (int)data->menuOptions.size() <= v ) {
            throw std::invalid_argument( std::string("KnobChoice::getEntry: index out of range") );
        }
        return data->menuOptions[v];
    }
}

int
KnobChoice::getNumEntries(ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    {
        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), view_i));
        if (!data) {
            return false;
        }
        QMutexLocker k(&data->valueMutex);
        return data->menuOptions.size();
    }
}



void
KnobChoice::setActiveEntryText(const std::string& entry, ViewSetSpec view)
{

    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
            if (view_i != *it) {
                continue;
            }
        }

        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), *it));
        if (!data) {
            continue;
        }
        KnobDimViewKeySet sharedKnobs;
        {
            QMutexLocker k(&data->valueMutex);
            sharedKnobs = data->sharedKnobs;
            data->activeEntry = entry;
        }
        for (KnobDimViewKeySet::const_iterator it = sharedKnobs.begin(); it!=sharedKnobs.end(); ++it) {
            KnobChoicePtr sharedKnob = toKnobChoice(it->knob.lock());
            assert(sharedKnob);
            if (!sharedKnob) {
                continue;
            }

            Q_EMIT sharedKnob->populated();
        }

    }
}

std::string
KnobChoice::getActiveEntryText(ViewGetSpec view)
{
    ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    {
        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), view_i));
        if (!data) {
            return false;
        }
        {
            QMutexLocker k(&data->valueMutex);
            if (!data->activeEntry.empty()) {
                return data->activeEntry;
            }
        }

        // Active entry was not set yet, give something based on the index and set the active entry
        int activeIndex = getValue(DimIdx(0), view_i);
        {
            QMutexLocker k(&data->valueMutex);
            if ( activeIndex >= 0 && activeIndex < (int)data->menuOptions.size() ) {
                data->activeEntry = data->menuOptions[activeIndex];
                return data->activeEntry;
            }

        }
    }

    return std::string();
}


std::string
KnobChoice::getHintToolTipFull() const
{
    ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    QMutexLocker k(&data->valueMutex);

    int gothelp = 0;

    if ( !data->menuOptionTooltips.empty() ) {
        for (std::size_t i = 0; i < data->menuOptionTooltips.size(); ++i) {
            if ( !data->menuOptionTooltips[i].empty() ) {
                ++gothelp;
            }
        }
    }

    if (gothelp > KNOBCHOICE_MAX_ENTRIES_HELP) {
        // too many entries
        gothelp = 0;
    }
    std::stringstream ss;
    if ( !getHintToolTip().empty() ) {
        ss << boost::trim_copy( getHintToolTip() );
        if (gothelp) {
            // if there are per-option help strings, separate them from main hint
            ss << "\n\n";
        }
    }
    // param may have no hint but still have per-option help
    if (gothelp) {
        for (std::size_t i = 0; i < data->menuOptionTooltips.size(); ++i) {
            if ( !data->menuOptionTooltips[i].empty() ) { // no help line is needed if help is unavailable for this option
                std::string entry = boost::trim_copy(data->menuOptions[i]);
                std::replace_if(entry.begin(), entry.end(), ::isspace, ' ');
                std::string help = boost::trim_copy(data->menuOptionTooltips[i]);
                std::replace_if(help.begin(), help.end(), ::isspace, ' ');
                if ( isHintInMarkdown() ) {
                    ss << "* **" << entry << "**";
                } else {
                    ss << entry;
                }
                ss << ": ";
                ss << help;
                if (i < data->menuOptionTooltips.size() - 1) {
                    ss << '\n';
                }
            }
        }
    }

    return ss.str();
} // KnobChoice::getHintToolTipFull

ValueChangedReturnCodeEnum
KnobChoice::setValueFromLabel(const std::string & value, ViewSetSpec view)
{
    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
            if (view_i != *it) {
                continue;
            }
        }

        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), *it));
        if (!data) {
            continue;
        }
        int index = -1;
        {
            QMutexLocker k(&data->valueMutex);
            int i = choiceMatch(value, data->menuOptions);
        }
        if (index != -1) {
            return setValue(index, view);
        }
    }

    throw std::runtime_error(std::string("KnobChoice::setValueFromLabel: unknown label ") + value);
}


// Choice restoration tries several options to restore a choice value:
// 1- exact string match, same index
// 2- exact string match, other index
// 3- exact string match before the first '\t', other index
// 4- case-insensistive string match, other index
// returns index if choice was matched, -1 if not matched
int
KnobChoice::choiceMatch(const std::string& choice,
                        const std::vector<std::string>& entries)
{
    // first, try exact match
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i] == choice) {
            return i;
        }
    }

    // second, match the part before '\t' with the part before '\t'. This is for value-tab-description options such as in the WriteFFmpeg codec
    std::size_t choicetab = choice.find('\t'); // returns string::npos if no tab was found
    std::string choicemain = choice.substr(0, choicetab); // gives the entire string if no tabs were found
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const std::string& entry(entries[i]);
        std::size_t entrytab = entry.find('\t'); // returns string::npos if no tab was found
        std::string entrymain = entry.substr(0, entrytab); // gives the entire string if no tabs were found

        if (entrymain == choicemain) {
            return i;
        }
    }

    // third, case-insensitive match
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if ( boost::iequals(entries[i], choice) ) {
            return i;
        }
    }

    // no match
    return -1;
}

void
KnobChoice::setDefaultValueFromLabelWithoutApplying(const std::string & value)
{
    int index = -1;
    {
        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
        assert(data);
        QMutexLocker k(&data->valueMutex);
        index = choiceMatch(value, data->menuOptions);
    }
    if (index != -1) {
        return setDefaultValueWithoutApplying(index, DimSpec(0));
    }
    throw std::runtime_error(std::string("KnobChoice::setDefaultValueFromLabelWithoutApplying: unknown label ") + value);
}

void
KnobChoice::setDefaultValueFromLabel(const std::string & value)
{
    int index = -1;
    {
        ChoiceKnobDimViewPtr data = toChoiceKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
        assert(data);
        QMutexLocker k(&data->valueMutex);
        for (std::size_t i = 0; i < data->menuOptions.size(); ++i) {
            if ( boost::iequals(data->menuOptions[i], value) ) {
                index = i;
                break;
            }
        }
    }
    if (index != -1) {
        return setDefaultValue(index, DimSpec(0));
    }
    throw std::runtime_error(std::string("KnobChoice::setDefaultValueFromLabel: unknown label ") + value);
}

KnobDimViewBasePtr
KnobChoice::createDimViewData() const
{
    ChoiceKnobDimViewPtr ret(new ChoiceKnobDimView);
    return ret;
}


/******************************KnobSeparator**************************************/

KnobSeparator::KnobSeparator(const KnobHolderPtr& holder,
                             const std::string &label,
                             int dimension,
                             bool declaredByPlugin)
    : KnobBoolBase(holder, label, dimension, declaredByPlugin)
{
}

bool
KnobSeparator::canAnimate() const
{
    return false;
}

const std::string KnobSeparator::_typeNameStr(kKnobSeparatorTypeName);
const std::string &
KnobSeparator::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobSeparator::typeName() const
{
    return typeNameStatic();
}

/******************************KnobColor**************************************/

/**
 * @brief A color knob with of variable dimension. Each color is a double ranging in [0. , 1.]
 * In dimension 1 the knob will have a single channel being a gray-scale
 * In dimension 3 the knob will have 3 channel R,G,B
 * In dimension 4 the knob will have R,G,B and A channels.
 **/

KnobColor::KnobColor(const KnobHolderPtr& holder,
                     const std::string &label,
                     int dimension,
                     bool declaredByPlugin)
    : KnobDoubleBase(holder, label, dimension, declaredByPlugin)
    , _simplifiedMode(false)
{
    //dimension greater than 4 is not supported. Dimension 2 doesn't make sense.
    assert(dimension <= 4 && dimension != 2);
}


bool
KnobColor::canAnimate() const
{
    return true;
}

const std::string
KnobColor::_typeNameStr(kKnobColorTypeName);
const std::string &
KnobColor::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobColor::typeName() const
{
    return typeNameStatic();
}

void
KnobColor::setSimplified(bool simp)
{
    _simplifiedMode = simp;
}

bool
KnobColor::isSimplified() const
{
    return _simplifiedMode;
}

/******************************KnobString**************************************/


KnobString::KnobString(const KnobHolderPtr& holder,
                       const std::string &label,
                       int dimension,
                       bool declaredByPlugin)
    : AnimatingKnobStringHelper(holder, label, dimension, declaredByPlugin)
    , _multiLine(false)
    , _richText(false)
    , _customHtmlText(false)
    , _isLabel(false)
    , _isCustom(false)
    , _fontSize(getDefaultFontPointSize())
    , _boldActivated(false)
    , _italicActivated(false)
    , _fontFamily(NATRON_FONT)
    , _fontColor()
{
    _fontColor[0] = _fontColor[1] = _fontColor[2] = 0.;
}

KnobString::~KnobString()
{
}

int
KnobString::getDefaultFontPointSize()
{
    return kKnobStringDefaultFontSize;
}

bool
KnobString::canAnimate() const
{
    return canAnimateStatic();
}

const std::string KnobString::_typeNameStr(kKnobStringTypeName);
const std::string &
KnobString::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobString::typeName() const
{
    return typeNameStatic();
}

bool
KnobString::parseFont(const QString & label, int* fontSize, QString* fontFamily, bool* isBold, bool* isItalic, double* r, double *g, double* b)
{
    assert(isBold && isItalic && r && g && b && fontFamily && fontSize);

    *isBold = false;
    *isItalic = false;
    *fontSize = 0;
    *r = *g = *b = 0.;

    QString toFind = QString::fromUtf8(kFontSizeTag);
    int startFontTag = label.indexOf(toFind);

    assert(startFontTag != -1);
    if (startFontTag == -1) {
        return false;
    }
    startFontTag += toFind.size();
    int j = startFontTag;
    QString sizeStr;
    while ( j < label.size() && label.at(j).isDigit() ) {
        sizeStr.push_back( label.at(j) );
        ++j;
    }

    toFind = QString::fromUtf8(kFontFaceTag);
    startFontTag = label.indexOf(toFind, startFontTag);
    assert(startFontTag != -1);
    if (startFontTag == -1) {
        return false;
    }
    startFontTag += toFind.size();
    j = startFontTag;
    QString faceStr;
    while ( j < label.size() && label.at(j) != QLatin1Char('"') ) {
        faceStr.push_back( label.at(j) );
        ++j;
    }

    *fontSize = sizeStr.toInt();
    *fontFamily = faceStr;

    {
        toFind = QString::fromUtf8(kBoldStartTag);
        int foundBold = label.indexOf(toFind);
        if (foundBold != -1) {
            *isBold = true;
        }
    }

    {
        toFind = QString::fromUtf8(kItalicStartTag);
        int foundItalic = label.indexOf(toFind);
        if (foundItalic != -1) {
            *isItalic = true;
        }
    }
    {
        toFind = QString::fromUtf8(kFontColorTag);
        int foundColor = label.indexOf(toFind);
        if (foundColor != -1) {
            foundColor += toFind.size();
            QString currentColor;
            int j = foundColor;
            while ( j < label.size() && label.at(j) != QLatin1Char('"') ) {
                currentColor.push_back( label.at(j) );
                ++j;
            }
            int red, green, blue;
            ColorParser::parseColor(currentColor, &red, &green, &blue);
            *r = red / 255.0;
            *g = green / 255.0;
            *b = blue / 255.0;
        }
    }
    return true;
} // KnobString::parseFont

bool
KnobString::hasContentWithoutHtmlTags()
{
    std::string str = getValue();

    if ( str.empty() ) {
        return false;
    }

    //First remove content in the NATRON_CUSTOM_HTML tags
    const std::string customTagStart(NATRON_CUSTOM_HTML_TAG_START);
    const std::string customTagEnd(NATRON_CUSTOM_HTML_TAG_END);
    std::size_t foundNatronCustomDataTag = str.find(customTagStart, 0);
    if (foundNatronCustomDataTag != std::string::npos) {
        ///remove the current custom data
        int foundNatronEndTag = str.find(customTagEnd, foundNatronCustomDataTag);
        assert(foundNatronEndTag != (int)std::string::npos);

        foundNatronEndTag += customTagEnd.size();
        str.erase(foundNatronCustomDataTag, foundNatronEndTag - foundNatronCustomDataTag);
    }

    std::size_t foundOpen = str.find("<");
    if (foundOpen == std::string::npos) {
        return true;
    }
    while (foundOpen != std::string::npos) {
        std::size_t foundClose = str.find(">", foundOpen);
        if (foundClose == std::string::npos) {
            return true;
        }

        if ( foundClose + 1 < str.size() ) {
            if (str[foundClose + 1] == '<') {
                foundOpen = foundClose + 1;
            } else {
                return true;
            }
        } else {
            return false;
        }
    }

    return true;
}


QString
KnobString::removeNatronHtmlTag(QString text)
{
    // We also remove any custom data added by natron so the user doesn't see it
    int startCustomData = text.indexOf( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START) );

    if (startCustomData != -1) {

        // Found start tag, now find end tag and remove what's in-between
        QString endTag( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_END) );
        int endCustomData = text.indexOf(endTag, startCustomData);
        assert(endCustomData != -1);
        if (endCustomData == -1) {
            return text;
        }
        endCustomData += endTag.size();
        text.remove(startCustomData, endCustomData - startCustomData);
    }

    return text;
}

QString
KnobString::getNatronHtmlTagContent(QString text)
{
    QString label = removeAutoAddedHtmlTags(text, false);
    QString startTag = QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START);
    int startCustomData = label.indexOf(startTag);

    if (startCustomData != -1) {

        // Found start tag, now find end tag and get what's in-between
        QString endTag = QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_END);
        int endCustomData = label.indexOf(endTag, startCustomData);
        assert(endCustomData != -1);
        if (endCustomData == -1) {
            return label;
        }
        label = label.remove( endCustomData, endTag.size() );
        label = label.remove( startCustomData, startTag.size() );
    }

    return label;
}

QString
KnobString::removeAutoAddedHtmlTags(QString text,
                                       bool removeNatronTag)
{
    // Find font start tag
    QString toFind = QString::fromUtf8(kFontSizeTag);
    int i = text.indexOf(toFind);
    bool foundFontStart = i != -1;

    // Remove bold tag
    QString boldStr = QString::fromUtf8(kBoldStartTag);
    int foundBold = text.lastIndexOf(boldStr, i);

    // Assert removed: the knob might be linked from elsewhere and the button might not have been pressed.
    //assert((foundBold == -1 && !_boldActivated) || (foundBold != -1 && _boldActivated));

    if (foundBold != -1) {
        // We found bold, remove it
        text.remove( foundBold, boldStr.size() );
        boldStr = QString::fromUtf8(kBoldEndTag);
        foundBold = text.lastIndexOf(boldStr);
        assert(foundBold != -1);
        if (foundBold == -1) {
            return text;
        }
        text.remove( foundBold, boldStr.size() );
    }

    // Refresh the index of the font start tag
    i = text.indexOf(toFind);

    // Remove italic tag
    QString italStr = QString::fromUtf8(kItalicStartTag);
    int foundItal = text.lastIndexOf(italStr, i);

    // Assert removed: the knob might be linked from elsewhere and the button might not have been pressed.
    // assert((_italicActivated && foundItal != -1) || (!_italicActivated && foundItal == -1));

    if (foundItal != -1) {
        // We found italic, remove it
        text.remove( foundItal, italStr.size() );
        italStr = QString::fromUtf8(kItalicEndTag);
        foundItal = text.lastIndexOf(italStr);
        assert(foundItal != -1);
        text.remove( foundItal, italStr.size() );
    }

    // Refresh the index of the font start tag
    i = text.indexOf(toFind);

    // Find the end of the font declaration start tag
    QString endTag = QString::fromUtf8("\">");
    int foundEndTag = text.indexOf(endTag, i);
    foundEndTag += endTag.size();
    if (foundFontStart) {
        //Remove the whole font declaration tag
        text.remove(i, foundEndTag - i);
    }

    // Find the font end tag
    endTag = QString::fromUtf8(kFontEndTag);
    foundEndTag = text.lastIndexOf(endTag);
    assert( (foundEndTag != -1 && foundFontStart) || !foundFontStart );
    if (foundEndTag != -1) {
        // Remove the font end tag
        text.remove( foundEndTag, endTag.size() );
    }

    // We also remove any custom data added by natron so the user doesn't see it
    if (removeNatronTag) {
        return removeNatronHtmlTag(text);
    } else {
        return text;
    }
} // removeAutoAddedHtmlTags

QString
KnobString::makeFontTag(const QString& family,
                        int fontSize,
                        double r, double g, double b)
{
    QString colorName = ColorParser::getColorName(Image::clamp(r, 0., 1.) * 255.0, Image::clamp(g, 0., 1.) * 255.0, Image::clamp(b, 0., 1.) * 255.0);
    return QString::fromUtf8(kFontSizeTag "%1\" " kFontColorTag "%2\" " kFontFaceTag "%3\">")
    .arg(fontSize)
    .arg(colorName)
    .arg(family);
}

QString
KnobString::decorateTextWithFontTag(const QString& family,
                                    int fontSize,
                                    double r, double g, double b,
                                    bool isBold, bool isItalic,
                                    const QString& text)
{
    QString ret = makeFontTag(family, fontSize, r, g, b);
    if (isBold) {
        ret += QString::fromUtf8(kBoldStartTag);
    }
    if (isItalic) {
        ret += QString::fromUtf8(kItalicStartTag);
    }
    ret += text;
    if (isBold) {
        ret += QString::fromUtf8(kBoldEndTag);
    }
    if (isItalic) {
        ret += QString::fromUtf8(kItalicEndTag);
    }

    ret += QString::fromUtf8(kFontEndTag);
    return ret;
}

QString
KnobString::decorateStringWithCurrentState(const QString& str)
{
    QString ret = str;
    if (!_richText) {
        return ret;
    }
    ret = decorateTextWithFontTag(QString::fromUtf8(_fontFamily.c_str()), _fontSize, _fontColor[0], _fontColor[1], _fontColor[2], _boldActivated, _italicActivated, ret);
    return ret;
}

QString
KnobString::getValueDecorated(double time, ViewGetSpec view)
{
    QString ret;
    if (isAnimated(DimIdx(0), view)) {
        ret = QString::fromUtf8(getValueAtTime(time, DimIdx(0) , view).c_str());
    } else {
        ret = QString::fromUtf8(getValue(DimIdx(0), view).c_str());
    }
    return decorateStringWithCurrentState(ret);
}

void
KnobString::setAsLabel()
{
    setAnimationEnabled(false); //< labels cannot animate
    _isLabel = true;
}

/******************************KnobGroup**************************************/

KnobGroup::KnobGroup(const KnobHolderPtr& holder,
                     const std::string &label,
                     int dimension,
                     bool declaredByPlugin)
    : KnobBoolBase(holder, label, dimension, declaredByPlugin)
    , _isTab(false)
    , _isToolButton(false)
    , _isDialog(false)
{
}

void
KnobGroup::setAsTab()
{
    _isTab = true;
}

bool
KnobGroup::isTab() const
{
    return _isTab;
}

void
KnobGroup::setAsToolButton(bool b)
{
    _isToolButton = b;
}

bool
KnobGroup::getIsToolButton() const
{
    return _isToolButton;
}

void
KnobGroup::setAsDialog(bool b)
{
    _isDialog = b;
}

bool
KnobGroup::getIsDialog() const
{
    return _isDialog;
}

bool
KnobGroup::canAnimate() const
{
    return false;
}

const std::string KnobGroup::_typeNameStr(kKnobGroupTypeName);
const std::string &
KnobGroup::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobGroup::typeName() const
{
    return typeNameStatic();
}

void
KnobGroup::addKnob(const KnobIPtr& k)
{
    if ( !isUserKnob() && k->isUserKnob() ) {
        return;
    }

    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }

    k->resetParent();

    _children.push_back(k);
    k->setParentKnob( shared_from_this() );
}

void
KnobGroup::removeKnob(const KnobIPtr& k)
{
    for (std::vector<KnobIWPtr >::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->lock() == k) {
            _children.erase(it);

            return;
        }
    }
}

bool
KnobGroup::moveOneStepUp(const KnobIPtr& k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            if (i == 0) {
                return false;
            }
            KnobIWPtr tmp = _children[i - 1];
            _children[i - 1] = _children[i];
            _children[i] = tmp;

            return true;
        }
    }
    throw std::invalid_argument("Given knob does not belong to this group");
}

bool
KnobGroup::moveOneStepDown(const KnobIPtr& k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            if (i == _children.size() - 1) {
                return false;
            }
            KnobIWPtr tmp = _children[i + 1];
            _children[i + 1] = _children[i];
            _children[i] = tmp;

            return true;
        }
    }
    throw std::invalid_argument("Given knob does not belong to this group");
}

void
KnobGroup::insertKnob(int index,
                      const KnobIPtr& k)
{
    if ( !isUserKnob() && k->isUserKnob() ) {
        return;
    }

    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }

    k->resetParent();

    if ( index >= (int)_children.size() ) {
        _children.push_back(k);
    } else {
        std::vector<KnobIWPtr >::iterator it = _children.begin();
        std::advance(it, index);
        _children.insert(it, k);
    }
    k->setParentKnob( shared_from_this() );
}

std::vector< KnobIPtr >
KnobGroup::getChildren() const
{
    std::vector< KnobIPtr > ret;

    for (std::size_t i = 0; i < _children.size(); ++i) {
        KnobIPtr k = _children[i].lock();
        if (k) {
            ret.push_back(k);
        }
    }

    return ret;
}

/******************************PAGE_KNOB**************************************/

KnobPage::KnobPage(const KnobHolderPtr& holder,
                   const std::string &label,
                   int dimension,
                   bool declaredByPlugin)
    : KnobBoolBase(holder, label, dimension, declaredByPlugin)
    , _isToolBar(false)
{
    setIsPersistent(false);
}

bool
KnobPage::canAnimate() const
{
    return false;
}

const std::string KnobPage::_typeNameStr(kKnobPageTypeName);
const std::string &
KnobPage::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobPage::typeName() const
{
    return typeNameStatic();
}

std::vector< KnobIPtr >
KnobPage::getChildren() const
{
    std::vector< KnobIPtr > ret;

    for (std::size_t i = 0; i < _children.size(); ++i) {
        KnobIPtr k = _children[i].lock();
        if (k) {
            ret.push_back(k);
        }
    }

    return ret;
}

void
KnobPage::addKnob(const KnobIPtr &k)
{
    if ( !isUserKnob() && k->isUserKnob() ) {
        return;
    }
    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }


    k->resetParent();

    _children.push_back(k);
    k->setParentKnob( shared_from_this() );
}

void
KnobPage::insertKnob(int index,
                     const KnobIPtr& k)
{
    if ( !isUserKnob() && k->isUserKnob() ) {
        return;
    }

    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }

    k->resetParent();

    if ( index >= (int)_children.size() ) {
        _children.push_back(k);
    } else {
        std::vector<KnobIWPtr >::iterator it = _children.begin();
        std::advance(it, index);
        _children.insert(it, k);
    }
    k->setParentKnob( shared_from_this() );
}

void
KnobPage::removeKnob(const KnobIPtr& k)
{
    for (std::vector<KnobIWPtr >::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->lock() == k) {
            _children.erase(it);

            return;
        }
    }
}

bool
KnobPage::moveOneStepUp(const KnobIPtr& k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            if (i == 0) {
                return false;
            }
            KnobIWPtr tmp = _children[i - 1];
            _children[i - 1] = _children[i];
            _children[i] = tmp;

            return true;
        }
    }
    throw std::invalid_argument("Given knob does not belong to this page");
}

bool
KnobPage::moveOneStepDown(const KnobIPtr& k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            if (i == _children.size() - 1) {
                return false;
            }
            KnobIWPtr tmp = _children[i + 1];
            _children[i + 1] = _children[i];
            _children[i] = tmp;

            return true;
        }
    }
    throw std::invalid_argument("Given knob does not belong to this page");
}

/******************************KnobParametric**************************************/

bool
ParametricKnobDimView::copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs)
{
    bool hasChanged = ValueKnobDimView<double>::copy(inArgs, outArgs);
    const ParametricKnobDimView* otherType = dynamic_cast<const ParametricKnobDimView*>(inArgs.other);
    assert(otherType);

    QMutexLocker k(&valueMutex);
    QMutexLocker k2(&inArgs.other->valueMutex);

    if (otherType->parametricCurve) {
        if (!parametricCurve) {
            parametricCurve.reset(new Curve(otherType->parametricCurve->getType()));
        }
        hasChanged |= parametricCurve->cloneAndCheckIfChanged(*otherType->parametricCurve, 0 /*offset*/, 0 /*range*/);
    }
    return hasChanged;
}

KnobParametric::KnobParametric(const KnobHolderPtr& holder,
                               const std::string &label,
                               int dimension,
                               bool declaredByPlugin)
    : KnobDoubleBase(holder, label, dimension, declaredByPlugin)
    , _curvesMutex()
    , _defaultCurves(dimension)
    , _curvesColor(dimension)
{
    setCanAutoFoldDimensions(false);
}

KnobDimViewBasePtr
KnobParametric::createDimViewData() const
{
    ParametricKnobDimViewPtr ret(new ParametricKnobDimView);
    ret->parametricCurve.reset(new Curve(Curve::eCurveTypeParametric));
    return ret;
}

void
KnobParametric::populate()
{
    KnobDoubleBase::populate();
    for (int i = 0; i < getNDimensions(); ++i) {
        RGBAColourD color;
        color.r = color.g = color.b = color.a = 1.;
        _curvesColor[i] = color;
        _defaultCurves[i].reset(new Curve(Curve::eCurveTypeParametric));
    }
}

const std::string KnobParametric::_typeNameStr(kKnobParametricTypeName);

void
KnobParametric::setPeriodic(bool periodic)
{
    for (std::size_t i = 0; i < _defaultCurves.size(); ++i) {
        ParametricKnobDimViewPtr data = toParametricKnobDimView(getDataForDimView(DimIdx(i), ViewIdx(0)));
        assert(data);
        data->parametricCurve->setPeriodic(periodic);
        _defaultCurves[i]->setPeriodic(periodic);
    }
}


const std::string &
KnobParametric::typeNameStatic()
{
    return _typeNameStr;
}


bool
KnobParametric::canAnimate() const
{
    return false;
}

const std::string &
KnobParametric::typeName() const
{
    return typeNameStatic();
}

CurvePtr
KnobParametric::getAnimationCurve(ViewGetSpec idx, DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_defaultCurves.size()) {
        throw std::invalid_argument("KnobParametric::getAnimationCurve dimension out of range");
    }
    ViewIdx view_i = getViewIdxFromGetSpec(idx);
    ParametricKnobDimViewPtr data = toParametricKnobDimView(getDataForDimView(dimension, view_i));
    if (!data) {
        return CurvePtr();
    }
    return data->parametricCurve;
}

void
KnobParametric::setCurveColor(DimIdx dimension,
                              double r,
                              double g,
                              double b)
{
    ///only called in the main thread
    assert( QThread::currentThread() == qApp->thread() );
    ///Mt-safe as it never changes

    assert( dimension < (int)_curvesColor.size() );
    _curvesColor[dimension].r = r;
    _curvesColor[dimension].g = g;
    _curvesColor[dimension].b = b;

    Q_EMIT curveColorChanged(dimension);
}

void
KnobParametric::getCurveColor(DimIdx dimension,
                              double* r,
                              double* g,
                              double* b)
{
    ///Mt-safe as it never changes

    if (dimension < 0 || dimension >= (int)_defaultCurves.size()) {
        throw std::invalid_argument("KnobParametric::getCurveColor dimension out of range");
    }

    *r = _curvesColor[dimension].r;
    *g = _curvesColor[dimension].g;
    *b = _curvesColor[dimension].b;

}

void
KnobParametric::setParametricRange(double min,
                                   double max)
{
    ///only called in the main thread
    assert( QThread::currentThread() == qApp->thread() );
    ///Mt-safe as it never changes

    for (std::size_t i = 0; i < _defaultCurves.size(); ++i) {
        ParametricKnobDimViewPtr data = toParametricKnobDimView(getDataForDimView(DimIdx(i), ViewIdx(0)));
        assert(data);
        data->parametricCurve->setXRange(min, max);
    }
}

std::pair<double, double> KnobParametric::getParametricRange() const
{
    ///Mt-safe as it never changes
    ParametricKnobDimViewPtr data = toParametricKnobDimView(getDataForDimView(DimIdx(0), ViewIdx(0)));
    assert(data);
    return data->parametricCurve->getXRange();
}

CurvePtr
KnobParametric::getDefaultParametricCurve(DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_defaultCurves.size()) {
        throw std::invalid_argument("KnobParametric::getDefaultParametricCurve dimension out of range");
    }
    return _defaultCurves[dimension];

}

CurvePtr
KnobParametric::getParametricCurveInternal(DimIdx dimension, ViewGetSpec view, ParametricKnobDimViewPtr* outData) const
{
    ///Mt-safe as Curve is MT-safe and the pointer is never deleted
    if (dimension < 0 || dimension >= (int)_defaultCurves.size()) {
        throw std::invalid_argument("KnobParametric::getParametricCurve dimension out of range");
    }
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    ParametricKnobDimViewPtr data = toParametricKnobDimView(getDataForDimView(dimension, view_i));
    if (!data) {
        return CurvePtr();
    }
    if (outData) {
        *outData = data;
    }

    EffectInstancePtr holder = toEffectInstance(getHolder());
    if (holder) {
        RenderValuesCachePtr cache = holder->getRenderValuesCacheTLS();
        if (cache) {
            KnobParametricPtr thisShared = boost::const_pointer_cast<KnobParametric>(boost::dynamic_pointer_cast<const KnobParametric>(shared_from_this()));
            return cache->getOrCreateCachedParametricKnobCurve(thisShared, data->parametricCurve, dimension);
        }
    }
    return data->parametricCurve;
}

CurvePtr KnobParametric::getParametricCurve(DimIdx dimension, ViewGetSpec view) const
{
    return getParametricCurveInternal(dimension, view, 0);
}

void
KnobParametric::signalCurveChanged(DimSpec dimension, const KnobDimViewBasePtr& data)
{
    KnobDimViewKeySet sharedKnobs;
    {
        QMutexLocker k(&data->valueMutex);
        sharedKnobs = data->sharedKnobs;
    }
    for (KnobDimViewKeySet::const_iterator it = sharedKnobs.begin(); it!=sharedKnobs.end(); ++it) {
        KnobParametricPtr sharedKnob = toKnobParametric(it->knob.lock());
        assert(sharedKnob);
        if (!sharedKnob) {
            continue;
        }

        Q_EMIT sharedKnob->curveChanged(dimension);
    }
}

StatusEnum
KnobParametric::addControlPoint(ValueChangedReasonEnum reason,
                                DimIdx dimension,
                                double key,
                                double value,
                                KeyframeTypeEnum interpolation)
{
    ///Mt-safe as Curve is MT-safe
    if ( ( dimension >= (int)_defaultCurves.size() ) ||
         ( key != key) || // check for NaN
         boost::math::isinf(key) ||
         ( value != value) || // check for NaN
         boost::math::isinf(value) ) {
        return eStatusFailed;
    }

    KeyFrame k(key, value);
    k.setInterpolation(interpolation);

    ParametricKnobDimViewPtr data;
    CurvePtr curve = getParametricCurveInternal(dimension, ViewIdx(0), &data);
    assert(curve);
    curve->addKeyFrame(k);
    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);
    signalCurveChanged(dimension, data);
    return eStatusOK;
}

StatusEnum
KnobParametric::addControlPoint(ValueChangedReasonEnum reason,
                                DimIdx dimension,
                                double key,
                                double value,
                                double leftDerivative,
                                double rightDerivative,
                                KeyframeTypeEnum interpolation)
{
    ///Mt-safe as Curve is MT-safe
    if ( ( dimension >= (int)_defaultCurves.size() ) ||
         ( key != key) || // check for NaN
         boost::math::isinf(key) ||
         ( value != value) || // check for NaN
         boost::math::isinf(value) ) {
        return eStatusFailed;
    }

    KeyFrame k(key, value, leftDerivative, rightDerivative);
    k.setInterpolation(interpolation);
    ParametricKnobDimViewPtr data;
    CurvePtr curve = getParametricCurveInternal(dimension, ViewIdx(0), &data);
    assert(curve);
    curve->addKeyFrame(k);
    signalCurveChanged(dimension, data);
    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::evaluateCurve(DimIdx dimension,
                         ViewGetSpec view,
                         double parametricPosition,
                         double *returnValue) const
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_defaultCurves.size() ) {
        return eStatusFailed;
    }
    CurvePtr curve = getParametricCurve(dimension, view);
    if (!curve) {
        return eStatusFailed;
    }
    *returnValue = curve->getValueAt(parametricPosition);
    return eStatusOK;
}

StatusEnum
KnobParametric::getNControlPoints(DimIdx dimension,
                                  ViewGetSpec view,
                                  int *returnValue) const
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_defaultCurves.size() ) {
        return eStatusFailed;
    }
    CurvePtr curve = getParametricCurve(dimension, view);
    if (!curve) {
        return eStatusFailed;
    }
    *returnValue =  curve->getKeyFramesCount();

    return eStatusOK;
}

StatusEnum
KnobParametric::getNthControlPoint(DimIdx dimension,
                                   ViewGetSpec view,
                                   int nthCtl,
                                   double *key,
                                   double *value) const
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_defaultCurves.size() ) {
        return eStatusFailed;
    }
    CurvePtr curve = getParametricCurve(dimension, view);
    if (!curve) {
        return eStatusFailed;
    }

    KeyFrame kf;
    bool ret = curve->getKeyFrameWithIndex(nthCtl, &kf);
    if (!ret) {
        return eStatusFailed;
    }
    *key = kf.getTime();
    *value = kf.getValue();

    return eStatusOK;
}

StatusEnum
KnobParametric::getNthControlPoint(DimIdx dimension,
                                   ViewGetSpec view,
                                   int nthCtl,
                                   double *key,
                                   double *value,
                                   double *leftDerivative,
                                   double *rightDerivative) const
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_defaultCurves.size() ) {
        return eStatusFailed;
    }
    CurvePtr curve = getParametricCurve(dimension, view);
    if (!curve) {
        return eStatusFailed;
    }
    KeyFrame kf;
    bool ret = curve->getKeyFrameWithIndex(nthCtl, &kf);
    if (!ret) {
        return eStatusFailed;
    }
    *key = kf.getTime();
    *value = kf.getValue();
    *leftDerivative = kf.getLeftDerivative();
    *rightDerivative = kf.getRightDerivative();

    return eStatusOK;
}

StatusEnum
KnobParametric::setNthControlPointInterpolation(ValueChangedReasonEnum reason,
                                                DimIdx dimension,
                                                ViewSetSpec view,
                                                int nThCtl,
                                                KeyframeTypeEnum interpolation)
{

    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_defaultCurves.size() ) {
        return eStatusFailed;
    }
    std::list<ViewIdx> views = getViewsList();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        ParametricKnobDimViewPtr data;
        CurvePtr curve = getParametricCurveInternal(dimension, *it, &data);
        if (!curve) {
            return eStatusFailed;
        }

        try {
            curve->setKeyFrameInterpolation(interpolation, nThCtl);
        } catch (...) {
            return eStatusFailed;
        }
        signalCurveChanged(dimension, data);

    }



    evaluateValueChange(dimension, getCurrentTime(), view, reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::setNthControlPoint(ValueChangedReasonEnum reason,
                                   DimIdx dimension,
                                   ViewSetSpec view,
                                   int nthCtl,
                                   double key,
                                   double value)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_defaultCurves.size() ) {
        return eStatusFailed;
    }
    std::list<ViewIdx> views = getViewsList();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        ParametricKnobDimViewPtr data;
        CurvePtr curve = getParametricCurveInternal(dimension, *it, &data);
        if (!curve) {
            return eStatusFailed;
        }
        try {
            curve->setKeyFrameValueAndTime(key, value, nthCtl);
        } catch (...) {
            return eStatusFailed;
        }
        signalCurveChanged(dimension, data);
    }


    evaluateValueChange(dimension, getCurrentTime(), view, reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::setNthControlPoint(ValueChangedReasonEnum reason,
                                   DimIdx dimension,
                                   ViewSetSpec view,
                                   int nthCtl,
                                   double key,
                                   double value,
                                   double leftDerivative,
                                   double rightDerivative)
{
    if ( dimension >= (int)_defaultCurves.size() ) {
        return eStatusFailed;
    }
    std::list<ViewIdx> views = getViewsList();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        ParametricKnobDimViewPtr data;
        CurvePtr curve = getParametricCurveInternal(dimension, *it, &data);
        if (!curve) {
            return eStatusFailed;
        }
        int newIdx;
        try {
            curve->setKeyFrameValueAndTime(key, value, nthCtl, &newIdx);
        } catch (...) {
            return eStatusFailed;
        }
        curve->setKeyFrameDerivatives(leftDerivative, rightDerivative, newIdx);
        signalCurveChanged(dimension, data);
    }

    evaluateValueChange(dimension, getCurrentTime(), view, reason);

    return eStatusOK;
} // setNthControlPoint

StatusEnum
KnobParametric::deleteControlPoint(ValueChangedReasonEnum reason,
                                   DimIdx dimension,
                                   ViewSetSpec view,
                                   int nthCtl)
{
    if ( dimension >= (int)_defaultCurves.size() ) {
        return eStatusFailed;
    }
    std::list<ViewIdx> views = getViewsList();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        ParametricKnobDimViewPtr data;
        CurvePtr curve = getParametricCurveInternal(dimension, *it, &data);
        if (!curve) {
            return eStatusFailed;
        }
        try {
            curve->removeKeyFrameWithIndex(nthCtl);
        } catch (...) {
            return eStatusFailed;
        }
        signalCurveChanged(dimension, data);
    }

    evaluateValueChange(dimension, getCurrentTime(), view, reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::deleteAllControlPoints(ValueChangedReasonEnum reason,
                                       DimIdx dimension,
                                       ViewSetSpec view)
{
    if ( dimension >= (int)_defaultCurves.size() ) {
        return eStatusFailed;
    }
    std::list<ViewIdx> views = getViewsList();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        ParametricKnobDimViewPtr data;
        CurvePtr curve = getParametricCurveInternal(dimension, *it, &data);
        if (!curve) {
            return eStatusFailed;
        }
        curve->clearKeyFrames();
        signalCurveChanged(dimension, data);

    }

    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);

    return eStatusOK;
}


void
KnobParametric::saveParametricCurves(std::map<std::string,std::list< SERIALIZATION_NAMESPACE::CurveSerialization > >* curves) const
{
    AppInstancePtr app = getHolder()->getApp();
    assert(app);
    std::vector<std::string> projectViews = app->getProject()->getProjectViewNames();
    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        std::string viewName;
        if (*it >= 0 && *it < (int)projectViews.size()) {
            viewName = projectViews[*it];
        }
        std::list< SERIALIZATION_NAMESPACE::CurveSerialization >& curveList = (*curves)[viewName];
        for (int i = 0; i < getNDimensions(); ++i) {
            CurvePtr curve = getParametricCurve(DimIdx(i), *it);
            assert(curve);
            SERIALIZATION_NAMESPACE::CurveSerialization c;
            if (curve) {
                curve->toSerialization(&c);
            }
            curveList.push_back(c);
        }
    }
} // saveParametricCurves

void
KnobParametric::loadParametricCurves(const std::map<std::string,std::list< SERIALIZATION_NAMESPACE::CurveSerialization > > & curves)
{
    AppInstancePtr app = getHolder()->getApp();
    assert(app);
    std::vector<std::string> projectViews = app->getProject()->getProjectViewNames();

    for (std::map<std::string,std::list< SERIALIZATION_NAMESPACE::CurveSerialization > >::const_iterator viewsIt = curves.begin(); viewsIt != curves.end(); ++viewsIt) {

        ViewIdx view_i(0);
        Project::getViewIndex(projectViews, viewsIt->first, &view_i);

        int i = 0;
        for (std::list< SERIALIZATION_NAMESPACE::CurveSerialization >::const_iterator it2 = viewsIt->second.begin(); it2 != viewsIt->second.end(); ++it2, ++i) {
            CurvePtr curve = getParametricCurve(DimIdx(i), view_i);
            if (!curve) {
                continue;
            }
            curve->fromSerialization(*it2);
        }
    }
} // loadParametricCurves

void
KnobParametric::resetExtraToDefaultValue(DimSpec dimension, ViewSetSpec view)
{

    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }

            ParametricKnobDimViewPtr data;
            CurvePtr curve = getParametricCurveInternal(DimIdx(i), *it, &data);
            if (!curve) {
                continue;
            }

            curve->clone(*_defaultCurves[DimIdx(i)]);
            signalCurveChanged(dimension, data);

        }

    }
}

void
KnobParametric::setDefaultCurvesFromCurves()
{
    for (std::size_t i = 0; i < _defaultCurves.size(); ++i) {
        CurvePtr curve = getParametricCurve(DimIdx(i), ViewIdx(0));
        assert(curve);
        _defaultCurves[i]->clone(*curve);
    }
    computeHasModifications();
}

bool
KnobParametric::hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const
{

    KeyFrameSet defKeys = _defaultCurves[dimension]->getKeyFrames_mt_safe();
    ParametricKnobDimViewPtr parametricData = toParametricKnobDimView(data);
    assert(parametricData);
    assert(parametricData->parametricCurve);

    KeyFrameSet keys = parametricData->parametricCurve->getKeyFrames_mt_safe();
    if ( defKeys.size() != keys.size() ) {
        return true;
    }
    KeyFrameSet::iterator itO = defKeys.begin();
    for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it, ++itO) {
        if (*it != *itO) {
            return true;
        }
    }

    return false;
}


void
KnobParametric::appendToHash(double /*time*/, ViewIdx view, Hash64* hash)
{

    for (std::size_t i = 0; i < _defaultCurves.size(); ++i) {
        // Parametric params are a corner case:
        // The plug-in will try to call getValue at many parametric times, which are unknown.
        // The only way to identify uniquely the curve as a hash is to append all control points
        // of the curve to the hash.
        CurvePtr curve = getParametricCurve(DimIdx(i), view);
        if (curve) {
            Hash64::appendCurve(curve, hash);
        }

    }

}

bool
KnobParametric::cloneCurve(ViewIdx view, DimIdx dimension, const Curve& curve, double offset, const RangeD* range, const StringAnimationManager* /*stringAnimation*/)
{
    if (dimension < 0 || dimension >= (int)_defaultCurves.size()) {
        throw std::invalid_argument("KnobParametric: dimension out of range");
    }
    ParametricKnobDimViewPtr data;
    CurvePtr thisCurve = getParametricCurveInternal(dimension, view, &data);
    if (!thisCurve) {
        return false;
    }

    bool ret = thisCurve->cloneAndCheckIfChanged(curve, offset, range);
    if (ret) {
        signalCurveChanged(dimension, data);
        evaluateValueChange(dimension, getCurrentTime(), ViewSetSpec(view), eValueChangedReasonUserEdited);
    }
    return ret;
}

void
KnobParametric::deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason)
{
    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }

            ParametricKnobDimViewPtr data;
            CurvePtr curve = getParametricCurveInternal(DimIdx(i), *it, &data);
            if (!curve) {
                continue;
            }

            for (std::list<double>::const_iterator it2 = times.begin(); it2!=times.end(); ++it2) {
                curve->removeKeyFrameWithTime(*it2);
            }
            signalCurveChanged(dimension, data);

        }
        
    }


    evaluateValueChange(dimension, getCurrentTime(), view, reason);
}

bool
KnobParametric::warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes)
{
    bool ok = false;

    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }

            ParametricKnobDimViewPtr data;
            CurvePtr curve = getParametricCurveInternal(DimIdx(i), *it, &data);
            if (!curve) {
                continue;
            }

            ok |= curve->transformKeyframesValueAndTime(times, warp, keyframes);
            signalCurveChanged(dimension, data);
        }

    }


    if (ok) {
        evaluateValueChange(dimension, getCurrentTime(), view, eValueChangedReasonUserEdited);
        return true;
    }
    return false;
}

void
KnobParametric::removeAnimation(ViewSetSpec view, DimSpec dim, ValueChangedReasonEnum reason)
{
    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dim.isAll() && dim != i) {
                continue;
            }

            ParametricKnobDimViewPtr data;
            CurvePtr curve = getParametricCurveInternal(DimIdx(i), *it, &data);
            if (!curve) {
                continue;
            }

            curve->clearKeyFrames();
            signalCurveChanged(dim, data);
        }
        
    }


    evaluateValueChange(dim, getCurrentTime(), view, reason);

}

void
KnobParametric::deleteAnimationBeforeTime(double time, ViewSetSpec view, DimSpec dimension)
{
    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }

            ParametricKnobDimViewPtr data;
            CurvePtr curve = getParametricCurveInternal(DimIdx(i), *it, &data);
            if (!curve) {
                continue;
            }

            curve->removeKeyFramesAfterTime(time, 0);
            signalCurveChanged(dimension, data);
        }
        
    }

    evaluateValueChange(dimension, getCurrentTime(), view, eValueChangedReasonUserEdited);

}

void
KnobParametric::deleteAnimationAfterTime(double time, ViewSetSpec view, DimSpec dimension)
{
    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }

            ParametricKnobDimViewPtr data;
            CurvePtr curve = getParametricCurveInternal(DimIdx(i), *it, &data);
            if (!curve) {
                continue;
            }
            curve->removeKeyFramesAfterTime(time, 0);
            signalCurveChanged(dimension, data);
        }
        
    }


    evaluateValueChange(dimension, getCurrentTime(), view, eValueChangedReasonUserEdited);
}

void
KnobParametric::setInterpolationAtTimes(ViewSetSpec view, DimSpec dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys)
{

    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }

            ParametricKnobDimViewPtr data;
            CurvePtr curve = getParametricCurveInternal(DimIdx(i), *it, &data);
            if (!curve) {
                continue;
            }
            for (std::list<double>::const_iterator it2 = times.begin(); it2!=times.end(); ++it2) {
                KeyFrame k;
                if (curve->setKeyFrameInterpolation(interpolation, *it2, &k)) {
                    if (newKeys) {
                        newKeys->push_back(k);
                    }
                }
            }
            signalCurveChanged(dimension, data);
        }
    }

    evaluateValueChange(dimension, getCurrentTime(), view, eValueChangedReasonUserEdited);
}

bool
KnobParametric::setLeftAndRightDerivativesAtTime(ViewSetSpec view, DimSpec dimension, double time, double left, double right)
{
    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }

            ParametricKnobDimViewPtr data;
            CurvePtr curve = getParametricCurveInternal(DimIdx(i), *it, &data);
            if (!curve) {
                continue;
            }

            int keyIndex = curve->keyFrameIndex(time);
            if (keyIndex == -1) {
                return false;
            }

            curve->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
            curve->setKeyFrameDerivatives(left, right, keyIndex);

            signalCurveChanged(dimension, data);
        }
        
    }

    evaluateValueChange(dimension, getCurrentTime(), view, eValueChangedReasonUserEdited);
    return true;
}

bool
KnobParametric::setDerivativeAtTime(ViewSetSpec view, DimSpec dimension, double time, double derivative, bool isLeft)
{

    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }

            ParametricKnobDimViewPtr data;
            CurvePtr curve = getParametricCurveInternal(DimIdx(i), *it, &data);
            if (!curve) {
                continue;
            }

            int keyIndex = curve->keyFrameIndex(time);
            if (keyIndex == -1) {
                return false;
            }

            curve->setKeyFrameInterpolation(eKeyframeTypeBroken, keyIndex);
            if (isLeft) {
                curve->setKeyFrameLeftDerivative(derivative, keyIndex);
            } else {
                curve->setKeyFrameRightDerivative(derivative, keyIndex);
            }

            signalCurveChanged(dimension, data);

        }
    }
    evaluateValueChange(dimension, getCurrentTime(), view, eValueChangedReasonUserEdited);
    return true;
}

ValueChangedReturnCodeEnum
KnobParametric::setKeyFrameInternal(double time, double value, DimIdx dimension, ViewIdx view, KeyFrame* newKey)
{
    ParametricKnobDimViewPtr data;
    CurvePtr curve = getParametricCurveInternal(dimension, view, &data);
    if (!curve) {
        return eValueChangedReturnCodeNothingChanged;
    }

    ValueChangedReturnCodeEnum ret = curve->setOrAddKeyframe(KeyFrame(time, value));
    if (newKey) {
        (void)curve->getKeyFrameWithTime(time, newKey);
    }
    signalCurveChanged(dimension, data);
    return ret;
}

ValueChangedReturnCodeEnum
KnobParametric::setDoubleValueAtTime(double time, double value, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNothingChanged;
    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }

            ret = setKeyFrameInternal(time, value, DimIdx(i), *it, newKey);
            
        }
    }

    if (ret != eValueChangedReturnCodeNothingChanged) {
        evaluateValueChange(dimension, getCurrentTime(), view, reason);
    }
    return ret;
}

void
KnobParametric::setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& keys, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKey)
{
    if (keys.empty()) {
        return;
    }
    if (newKey) {
        newKey->clear();
    }

    std::list<ViewIdx> views = getViewsList();
    int nDims = getNDimensions();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (int i = 0; i < nDims; ++i) {
            if (!dimension.isAll() && dimension != i) {
                continue;
            }
            KeyFrame key;
            for (std::list<DoubleTimeValuePair>::const_iterator it2 = keys.begin(); it2!=keys.end(); ++it2) {
                setKeyFrameInternal(it2->time, it2->value, DimIdx(i), *it, newKey ? &key : 0);
                if (newKey) {
                    newKey->push_back(key);
                }
            }

        }
    }

    evaluateValueChange(dimension, getCurrentTime(), view, reason);
}

void
KnobParametric::setDoubleValueAtTimeAcrossDimensions(double time, const std::vector<double>& values, DimIdx dimensionStartIndex, ViewSetSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    if (values.empty()) {
        return;
    }
    
    if (dimensionStartIndex < 0 || dimensionStartIndex + values.size() > _defaultCurves.size()) {
        throw std::invalid_argument("KnobParametric: dimension out of range");
    }
    std::list<ViewIdx> views = getViewsList();
    ViewIdx view_i;
    if (!view.isAll()) {
        view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
    }
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        if (!view.isAll()) {
            if (view_i != *it) {
                continue;
            }
        }
        for (std::size_t i = 0; i < values.size(); ++i) {
            ValueChangedReturnCodeEnum ret = setKeyFrameInternal(time, values[i], DimIdx(dimensionStartIndex + i), *it, 0);
            if (retCodes) {
                retCodes->push_back(ret);
            }
        }
    }

    evaluateValueChange(DimSpec::all(), getCurrentTime(), view, reason);

}

void
KnobParametric::setMultipleDoubleValueAtTimeAcrossDimensions(const PerCurveDoubleValuesList& keysPerDimension, ValueChangedReasonEnum reason)
{
    if (keysPerDimension.empty()) {
        return;
    }
    for (std::size_t i = 0; i < keysPerDimension.size(); ++i) {
        if (keysPerDimension[i].second.empty()) {
            continue;
        }
        for (std::list<DoubleTimeValuePair>::const_iterator it2 = keysPerDimension[i].second.begin(); it2!=keysPerDimension[i].second.end(); ++it2) {
            setKeyFrameInternal(it2->time, it2->value, keysPerDimension[i].first.dimension, keysPerDimension[i].first.view, 0);
        }

    }

    evaluateValueChange(DimSpec::all(), getCurrentTime(), ViewSetSpec(0), reason);
}

bool
KnobParametric::canLinkWith(const KnobIPtr & other, DimIdx /*thisDimension*/, ViewIdx /*thisView*/,  DimIdx /*otherDim*/, ViewIdx /*otherView*/, std::string* error) const
{
    KnobParametric* otherIsParametric = dynamic_cast<KnobParametric*>(other.get());
    if (!otherIsParametric) {
        if (error) {
            *error = tr("Can only link with another parametric curve").toStdString();
        }
        return false;
    }
    return true;
}

void
KnobParametric::onLinkChanged()
{
    Q_EMIT curveChanged(DimSpec::all());
}

/******************************KnobTable**************************************/


KnobTable::KnobTable(const KnobHolderPtr& holder,
                     const std::string &label,
                     int dimension,
                     bool declaredByPlugin)
    : KnobStringBase(holder, label, dimension, declaredByPlugin)
{
}

KnobTable::KnobTable(const KnobHolderPtr& holder,
                     const QString &label,
                     int dimension,
                     bool declaredByPlugin)
    : KnobStringBase(holder, label.toStdString(), dimension, declaredByPlugin)
{
}

KnobTable::~KnobTable()
{
}

void
KnobTable::getTableSingleCol(std::list<std::string>* table)
{
    std::list<std::vector<std::string> > tmp;

    getTable(&tmp);
    for (std::list<std::vector<std::string> >::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        table->push_back( (*it)[0] );
    }
}

void
KnobTable::getTable(std::list<std::vector<std::string> >* table)
{
    decodeFromKnobTableFormat(getValue(), table);
}

void
KnobTable::decodeFromKnobTableFormat(const std::string& value,
                                     std::list<std::vector<std::string> >* table)
{
    QString raw = QString::fromUtf8( value.c_str() );

    if ( raw.isEmpty() ) {
        return;
    }
    const int colsCount = getColumnsCount();
    assert(colsCount > 0);


    QString startTag = QString::fromUtf8("<%1>");
    QString endTag = QString::fromUtf8("</%1>");
    int lastFoundIndex = 0;

    for (;; ) {
        int colIndex = 0;
        std::vector<std::string> row;
        bool mustStop = false;
        while (colIndex < colsCount) {
            QString colLabel = QString::fromUtf8( getColumnLabel(colIndex).c_str() );
            const QString startToFind = startTag.arg(colLabel);
            const QString endToFind = endTag.arg(colLabel);

            lastFoundIndex = raw.indexOf(startToFind, lastFoundIndex);
            if (lastFoundIndex == -1) {
                mustStop = true;
                break;
            }

            lastFoundIndex += startToFind.size();
            assert( lastFoundIndex < raw.size() );

            int endNamePos = raw.indexOf(endToFind, lastFoundIndex);
            assert( endNamePos != -1 && endNamePos < raw.size() );

            if ( (endNamePos == -1) || ( endNamePos >= raw.size() ) ) {
                KnobHolderPtr holder = getHolder();
                QString knobName;
                if (holder) {
                    EffectInstancePtr effect = toEffectInstance(holder);
                    if (effect) {
                        knobName += QString::fromUtf8(effect->getNode()->getFullyQualifiedName().c_str());
                        knobName += QString::fromUtf8(".");
                    }
                }
                knobName += QString::fromUtf8(getName().c_str());
                QString err = tr("%1 table is wrongly encoded, check your project file or report an issue to the developers").arg(knobName);
                throw std::logic_error(err.toStdString());
            }

            std::string value = raw.mid(lastFoundIndex, endNamePos - lastFoundIndex).toStdString();
            lastFoundIndex += (endNamePos - lastFoundIndex);


            // In order to use XML tags, the text inside the tags has to be unescaped.
            value = Project::unescapeXML(value);

            row.push_back(value);

            ++colIndex;
        }

        if (mustStop) {
            break;
        }

        if ( (int)row.size() == colsCount ) {
            table->push_back(row);
        } else {
            KnobHolderPtr holder = getHolder();
            QString knobName;
            if (holder) {
                EffectInstancePtr effect = toEffectInstance(holder);
                if (effect) {
                    knobName += QString::fromUtf8(effect->getNode()->getFullyQualifiedName().c_str());
                    knobName += QString::fromUtf8(".");
                }
            }
            knobName += QString::fromUtf8(getName().c_str());
            QString err = tr("%1 table is wrongly encoded, check your project file or report an issue to the developers").arg(knobName);
            throw std::logic_error(err.toStdString());

        }
    }
} // KnobTable::decodeFromKnobTableFormat

std::string
KnobTable::encodeToKnobTableFormatSingleCol(const std::list<std::string>& table)
{
    std::list<std::vector<std::string> > tmp;

    for (std::list<std::string>::const_iterator it = table.begin(); it != table.end(); ++it) {
        std::vector<std::string> vec;
        vec.push_back(*it);
        tmp.push_back(vec);
    }

    return encodeToKnobTableFormat(tmp);
}

std::string
KnobTable::encodeToKnobTableFormat(const std::list<std::vector<std::string> >& table)
{
    std::stringstream ss;


    for (std::list<std::vector<std::string> >::const_iterator it = table.begin(); it != table.end(); ++it) {
        // In order to use XML tags, the text inside the tags has to be escaped.
        for (std::size_t c = 0; c < it->size(); ++c) {
            std::string label = getColumnLabel(c);
            ss << "<" << label << ">";
            ss << Project::escapeXML( (*it)[c] );
            ss << "</" << label << ">";
        }
    }

    return ss.str();
}

void
KnobTable::setTableSingleCol(const std::list<std::string>& table)
{
    std::list<std::vector<std::string> > tmp;

    for (std::list<std::string>::const_iterator it = table.begin(); it != table.end(); ++it) {
        std::vector<std::string> vec;
        vec.push_back(*it);
        tmp.push_back(vec);
    }
    setTable(tmp);
}

void
KnobTable::setTable(const std::list<std::vector<std::string> >& table)
{
    setValue(encodeToKnobTableFormat(table));
}

void
KnobTable::appendRowSingleCol(const std::string& row)
{
    std::vector<std::string> tmp;

    tmp.push_back(row);
    appendRow(tmp);
}

void
KnobTable::appendRow(const std::vector<std::string>& row)
{
    std::list<std::vector<std::string> > table;

    getTable(&table);
    table.push_back(row);
    setTable(table);
}

void
KnobTable::insertRowSingleCol(int index,
                              const std::string& row)
{
    std::vector<std::string> tmp;

    tmp.push_back(row);
    insertRow(index, tmp);
}

void
KnobTable::insertRow(int index,
                     const std::vector<std::string>& row)
{
    std::list<std::vector<std::string> > table;

    getTable(&table);
    if ( (index < 0) || ( index >= (int)table.size() ) ) {
        table.push_back(row);
    } else {
        std::list<std::vector<std::string> >::iterator pos = table.begin();
        std::advance(pos, index);
        table.insert(pos, row);
    }

    setTable(table);
}

void
KnobTable::removeRow(int index)
{
    std::list<std::vector<std::string> > table;

    getTable(&table);
    if ( (index < 0) || ( index >= (int)table.size() ) ) {
        return;
    }
    std::list<std::vector<std::string> >::iterator pos = table.begin();
    std::advance(pos, index);
    table.erase(pos);
    setTable(table);
}

const std::string KnobLayers::_typeNameStr(kKnobLayersTypeName);
const std::string&
KnobLayers::typeNameStatic()
{
    return _typeNameStr;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobTypes.cpp"
