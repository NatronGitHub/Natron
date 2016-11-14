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

    // Knobs with default values that are normalized are positions hence disable auto dimensions folding
    //setAutoAllDimensionsVisibleSwitchEnabled(false);
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
KnobDouble::computeValuesHaveModifications(DimIdx dimension,
                                           const double& value,
                                           const double& defaultValue) const
{
    if (_defaultValuesAreNormalized) {
        double tmp = denormalize(dimension, 0, defaultValue);

        return value != tmp;
    } else {
        return value != defaultValue;
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


KnobChoice::KnobChoice(const KnobHolderPtr& holder,
                       const std::string &label,
                       int nDims,
                       bool declaredByPlugin)
    : KnobIntBase(holder, label, nDims, declaredByPlugin)
    , _entriesMutex()
    , _activeEntryMap()
    , _addNewChoice(false)
    , _isCascading(false)
    , _showMissingEntryWarning(true)
    , _isDisplayChannelKnob(false)
{
    _activeEntryMap.insert(std::make_pair(ViewIdx(0), std::string()));
}

KnobChoice::~KnobChoice()
{
}

void
KnobChoice::setMissingEntryWarningEnabled(bool enabled)
{
    _showMissingEntryWarning = enabled;
}

bool
KnobChoice::isMissingEntryWarningEnabled() const
{
    return _showMissingEntryWarning;
}

void
KnobChoice::setIsDisplayChannelsKnob(bool b)
{
    _isDisplayChannelKnob = b;
}

bool
KnobChoice::isDisplayChannelsKnob() const
{
    return _isDisplayChannelKnob;
}

void
KnobChoice::setTextToFitHorizontally(const std::string& text)
{
    _textToFitHorizontally = text;
}

std::string
KnobChoice::getTextToFitHorizontally() const
{
    return _textToFitHorizontally;
}

void
KnobChoice::setHostCanAddOptions(bool add)
{
    _addNewChoice = add;
}

bool
KnobChoice::getHostCanAddOptions() const
{
    return _addNewChoice;
}

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
KnobChoice::cloneExtraData(const KnobIPtr& other,
                           ViewSetSpec view,
                           ViewSetSpec otherView,
                           DimSpec /*dimension*/,
                           DimSpec /*otherDimension*/,
                           double /*offset*/,
                           const RangeD* /*range*/)
{
    assert((view.isAll() && otherView.isAll()) || (view.isViewIdx() && view.isViewIdx()));

    if (!other) {
        return false;
    }

    KnobChoicePtr isChoice = toKnobChoice(other);
    if (!isChoice) {
        return false;
    }

    bool hasChanged = false;
    if (view.isAll()) {
        std::list<ViewIdx> views = other->getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
            std::string otherChoice = isChoice->getActiveEntryText(*it);
            std::string& thisChoice = _activeEntryMap[*it];
            if (otherChoice != thisChoice) {
                thisChoice = otherChoice;
                hasChanged = true;
            }
        }
    } else {
        std::string otherChoice = isChoice->getActiveEntryText(ViewIdx(otherView));
        std::string& thisChoice = _activeEntryMap[ViewIdx(view)];
        if (otherChoice != thisChoice) {
            thisChoice = otherChoice;
            hasChanged = true;
        }
    }
    return hasChanged;
}




bool
KnobChoice::hasModificationsVirtual(DimIdx dimension, ViewIdx view) const
{
    int def_i = getDefaultValue(dimension);
    std::string defaultVal;

    QMutexLocker k(&_entriesMutex);
    if (def_i >= 0 && def_i < (int)_entries.size()) {
        defaultVal = _entries[def_i];
    }
    PerViewActiveEntryMap::const_iterator foundView = _activeEntryMap.find(view);
    if (foundView != _activeEntryMap.end()) {
        if (foundView->second != defaultVal) {
            return true;
        }
    }
    return false;
}

bool
KnobChoice::checkIfValueChanged(const int& a, DimIdx /*dimension*/, ViewIdx view) const
{
    std::string aStr;
    QMutexLocker k(&_entriesMutex);
    if (a >= 0 && a < (int)_entries.size()) {
        aStr = _entries[a];
    } else {
        // No current value, assume they are different
        return true;
    }
    PerViewActiveEntryMap::const_iterator it = _activeEntryMap.find(view);
    if (it == _activeEntryMap.end()) {
        // No current value, assume they are different
        return true;
    }
    return it->second != aStr;

}

void
KnobChoice::onInternalValueChanged(DimSpec /*dimension*/,
                                   double time,
                                   ViewSetSpec view)
{
    // by-pass any master/slave link here
    std::list<ViewIdx> views = getViewsList();
    if (view.isAll()) {
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            int index = getValueAtTime(time, DimIdx(0), *it, true, true);
            QMutexLocker k(&_entriesMutex);
            if ( (index >= 0) && ( index < (int)_entries.size() ) ) {
                _activeEntryMap[*it] = _entries[index];
            }
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view.value()));
        int index = getValueAtTime(time, DimIdx(0), view_i, true, true);
        QMutexLocker k(&_entriesMutex);
        if ( (index >= 0) && ( index < (int)_entries.size() ) ) {
            _activeEntryMap[view_i] = _entries[index];
        }
    }

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

        std::string curEntry;
        {
            QMutexLocker k(&_entriesMutex);
            PerViewActiveEntryMap::const_iterator foundView = _activeEntryMap.find(*it);
            if (foundView == _activeEntryMap.end()) {
                continue;
            }
            curEntry = foundView->second;
        }

        if ( !curEntry.empty() ) {
            if (mergingFunctor) {
                assert(mergingData);
                mergingData->clear();
            } else {
                mergingFunctor = stringEqualFunctor;
            }
            int found = -1;
            {
                QMutexLocker k(&_entriesMutex);
                for (std::size_t i = 0; i < _entries.size(); ++i) {
                    if ( mergingFunctor(_entries[i], curEntry, mergingData) ) {
                        found = i;

                        // Update the label if different
                        _activeEntryMap[*it] = _entries[i];
                        break;
                    }
                }
            }
            if (found != -1) {
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
    {
        QMutexLocker l(&_entriesMutex);

        if (!mergingFunctor) {
            // No merging functor, replace
            _entries = entries;
            _entriesHelp = entriesHelp;
            hasChanged = true;
        } else {
            assert(mergingData);
            // If there is a merging functor to merge current entries with new entries, do the merging

            // For all new entries, check if one of the merged entry matches and then merge
            // otherwise add to the merged entries
            for (std::size_t i = 0; i < entries.size(); ++i) {
                mergingData->clear();
                bool found = false;
                for (std::size_t j = 0; j < _entries.size(); ++j) {
                    if ( mergingFunctor(_entries[j], entries[i], mergingData) ) {
                        if (_entries[j] != entries[i]) {
                            hasChanged = true;
                            _entries[j] = entries[i];
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    hasChanged = true;
                    if ( i < _entriesHelp.size() ) {
                        _entriesHelp.push_back(entriesHelp[i]);
                    }
                    _entries.push_back(entries[i]);
                }
            }
        }
    } // QMutexLocker

    if (hasChanged) {

        //  Try to restore the last choice.
        findAndSetOldChoice(mergingFunctor, mergingData);

        // Notify tooltip changed because we changed the menu entries
        _signalSlotHandler->s_helpChanged();

        Q_EMIT populated();
    }

    return hasChanged;
} // KnobChoice::populateChoices


void
KnobChoice::setShortcuts(const std::map<int, std::string>& shortcuts)
{
    _shortcuts = shortcuts;
}

const std::map<int, std::string>&
KnobChoice::getShortcuts() const
{
    return _shortcuts;
}

void
KnobChoice::setIcons(const std::map<int, std::string>& icons)
{
    _menuIcons = icons;
}

const std::map<int, std::string>&
KnobChoice::getIcons() const
{
    return _menuIcons;
}

void
KnobChoice::setSeparators(const std::vector<int>& separators)
{
    _separators = separators;
}

const std::vector<int>&
KnobChoice::getSeparators() const
{
    return _separators;
}


void
KnobChoice::resetChoices()
{
    {
        QMutexLocker l(&_entriesMutex);
        _entries.clear();
        _entriesHelp.clear();
    }

    // Refresh active entry state
    findAndSetOldChoice();
    _signalSlotHandler->s_helpChanged();
    Q_EMIT entriesReset();
}

void
KnobChoice::appendChoice(const std::string& entry,
                         const std::string& help)
{
    {
        QMutexLocker l(&_entriesMutex);
        _entries.push_back(help);
        _entriesHelp.push_back(entry);
    }

    // Refresh active entry state
    findAndSetOldChoice();
    _signalSlotHandler->s_helpChanged();
    Q_EMIT entryAppended( QString::fromUtf8( entry.c_str() ), QString::fromUtf8( help.c_str() ) );
}

std::vector<std::string>
KnobChoice::getEntries() const
{
    QMutexLocker l(&_entriesMutex);
    return _entries;
}

bool
KnobChoice::isActiveEntryPresentInEntries(ViewIdx view) const
{
    QMutexLocker k(&_entriesMutex);
    PerViewActiveEntryMap::const_iterator foundView = _activeEntryMap.find(view);
    if (foundView == _activeEntryMap.end()) {
        return false;
    }
    if ( foundView->second.empty() ) {
        return true;
    }
    for (std::size_t i = 0; i < _entries.size(); ++i) {
        if (_entries[i] == foundView->second) {
            return true;
        }
    }

    return false;
}

bool
KnobChoice::splitView(ViewIdx view)
{
    if (!KnobIntBase::splitView(view)) {
        return false;
    }
    {
        QMutexLocker k(&_entriesMutex);
        const std::string& mainViewEntry = _activeEntryMap[ViewIdx(0)];
        std::string& viewEntry = _activeEntryMap[view];
        viewEntry = mainViewEntry;
    }
    return true;
}

bool
KnobChoice::unSplitView(ViewIdx view)
{
    if (!KnobIntBase::unSplitView(view)) {
        return false;
    }
    {
        QMutexLocker k(&_entriesMutex);
        PerViewActiveEntryMap::iterator found = _activeEntryMap.find(view);
        if (found != _activeEntryMap.end()) {
            _activeEntryMap.erase(found);
        }
    }
    return true;
}

std::string
KnobChoice::getEntry(int v) const
{
    QMutexLocker k(&_entriesMutex);
    if (v < 0 || (int)_entries.size() <= v ) {
        throw std::invalid_argument( std::string("KnobChoice::getEntry: index out of range") );
    }
    return _entries[v];
}

int
KnobChoice::getNumEntries() const
{
    QMutexLocker l(&_entriesMutex);
    return (int)_entries.size();
}

std::vector<std::string>
KnobChoice::getEntriesHelp() const
{
    QMutexLocker l(&_entriesMutex);
    return _entriesHelp;
}

void
KnobChoice::setActiveEntryText(const std::string& entry, ViewSetSpec view)
{

    {
        QMutexLocker l(&_entriesMutex);
        if (view.isAll()) {
            for (PerViewActiveEntryMap::iterator it = _activeEntryMap.begin(); it!=_activeEntryMap.end(); ++it) {
                it->second = entry;
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view.value()));
            PerViewActiveEntryMap::iterator foundView = _activeEntryMap.find(view_i);
            if (foundView == _activeEntryMap.end()) {
                return;
            }
            foundView->second = entry;
        }
    }
    Q_EMIT populated();
}

std::string
KnobChoice::getActiveEntryText(ViewGetSpec view)
{
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    MasterKnobLink linkData;
    if (getMaster(DimIdx(0), view_i, &linkData)) {
        KnobIPtr masterKnob = linkData.masterKnob.lock();
        KnobChoicePtr isChoice = toKnobChoice(masterKnob);
        if (isChoice) {
            return isChoice->getActiveEntryText(linkData.masterView);
        }
    }

    {
        QMutexLocker l(&_entriesMutex);
        PerViewActiveEntryMap::const_iterator foundView = _activeEntryMap.find(view_i);
        if ( !foundView->second.empty() ) {
            return foundView->second;
        }
    }
    int activeIndex = getValue(DimIdx(0), view_i);

    QMutexLocker l(&_entriesMutex);
    if ( activeIndex >= 0 && activeIndex < (int)_entries.size() ) {
        return _entries[activeIndex];
    }

    return std::string();
}


std::string
KnobChoice::getHintToolTipFull() const
{
    QMutexLocker l(&_entriesMutex);

    int gothelp = 0;

    if ( !_entriesHelp.empty() ) {
        assert( _entriesHelp.size() == _entriesHelp.size() );
        for (std::size_t i = 0; i < _entriesHelp.size(); ++i) {
            if ( !_entriesHelp.empty() && !_entriesHelp[i].empty() ) {
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
        for (std::size_t i = 0; i < _entriesHelp.size(); ++i) {
            if ( !_entriesHelp[i].empty() ) { // no help line is needed if help is unavailable for this option
                std::string entry = boost::trim_copy(_entries[i]);
                std::replace_if(entry.begin(), entry.end(), ::isspace, ' ');
                std::string help = boost::trim_copy(_entriesHelp[i]);
                std::replace_if(help.begin(), help.end(), ::isspace, ' ');
                if ( isHintInMarkdown() ) {
                    ss << "* **" << entry << "**";
                } else {
                    ss << entry;
                }
                ss << ": ";
                ss << help;
                if (i < _entriesHelp.size() - 1) {
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
    int index = -1;
    {
        QMutexLocker l(&_entriesMutex);
        for (std::size_t i = 0; i < _entries.size(); ++i) {
            if ( boost::iequals(_entries[i], value) ) {
                index = i;
                break;
            }
        }
    }
    if (index != -1) {
        return setValue(index, view);
    }
    throw std::runtime_error(std::string("KnobChoice::setValueFromLabel: unknown label ") + value);
}

void
KnobChoice::setDefaultValueFromLabelWithoutApplying(const std::string & value)
{
    int index = -1;
    {
        QMutexLocker l(&_entriesMutex);
        for (std::size_t i = 0; i < _entries.size(); ++i) {
            if ( boost::iequals(_entries[i], value) ) {
                index = i;
                break;
            }
        }
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
        QMutexLocker l(&_entriesMutex);
        for (std::size_t i = 0; i < _entries.size(); ++i) {
            if ( boost::iequals(_entries[i], value) ) {
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


void
KnobChoice::onKnobAboutToAlias(const KnobIPtr &slave)
{
    KnobChoicePtr isChoice = toKnobChoice(slave);

    if (isChoice) {
        populateChoices(isChoice->getEntries(),
                        isChoice->getEntriesHelp(),
                        0,
                        0);
    }
}

void
KnobChoice::onOriginalKnobPopulated()
{
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( sender() );
    if (!isChoice) {
        return;
    }
    populateChoices(isChoice->_entries, isChoice->_entriesHelp, 0, 0);
}

void
KnobChoice::onOriginalKnobEntriesReset()
{
    resetChoices();
}

void
KnobChoice::onOriginalKnobEntryAppend(const QString& text,
                                      const QString& help)
{
    appendChoice( text.toStdString(), help.toStdString() );
}

void
KnobChoice::handleSignalSlotsForAliasLink(const KnobIPtr& alias,
                                          bool connect)
{
    assert(alias);
    KnobChoicePtr aliasIsChoice = toKnobChoice(alias);
    if (!aliasIsChoice) {
        return;
    }
    if (connect) {
        QObject::connect( this, SIGNAL(populated()), aliasIsChoice.get(), SLOT(onOriginalKnobPopulated()) );
        QObject::connect( this, SIGNAL(entriesReset()), aliasIsChoice.get(), SLOT(onOriginalKnobEntriesReset()) );
        QObject::connect( this, SIGNAL(entryAppended(QString,QString)), aliasIsChoice.get(),
                          SLOT(onOriginalKnobEntryAppend(QString,QString)) );
    } else {
        QObject::disconnect( this, SIGNAL(populated()), aliasIsChoice.get(), SLOT(onOriginalKnobPopulated()) );
        QObject::disconnect( this, SIGNAL(entriesReset()), aliasIsChoice.get(), SLOT(onOriginalKnobEntriesReset()) );
        QObject::disconnect( this, SIGNAL(entryAppended(QString,QString)), aliasIsChoice.get(),
                             SLOT(onOriginalKnobEntryAppend(QString,QString)) );
    }
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


KnobParametric::KnobParametric(const KnobHolderPtr& holder,
                               const std::string &label,
                               int dimension,
                               bool declaredByPlugin)
    : KnobDoubleBase(holder, label, dimension, declaredByPlugin)
    , _curvesMutex()
    , _curves(dimension)
    , _defaultCurves(dimension)
    , _curvesColor(dimension)
{

}

void
KnobParametric::populate()
{
    KnobDoubleBase::populate();
    for (int i = 0; i < getNDimensions(); ++i) {
        RGBAColourD color;
        color.r = color.g = color.b = color.a = 1.;
        _curvesColor[i] = color;
        _curves[i] = CurvePtr( new Curve(shared_from_this(), DimIdx(i), ViewIdx(0)) );
        _defaultCurves[i] = CurvePtr( new Curve(shared_from_this(), DimIdx(i), ViewIdx(0)) );
    }
}

const std::string KnobParametric::_typeNameStr(kKnobParametricTypeName);

void
KnobParametric::setPeriodic(bool periodic)
{
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        _curves[i]->setPeriodic(periodic);
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
KnobParametric::getAnimationCurve(ViewGetSpec /*idx*/, DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_curves.size()) {
        throw std::invalid_argument("KnobParametric::getAnimationCurve dimension out of range");
    }
    return _curves[dimension];
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

    if (dimension < 0 || dimension >= (int)_curves.size()) {
        throw std::invalid_argument("KnobParametric::getCurveColor dimension out of range");
    }
    MasterKnobLink linkData;
    if (getMaster(dimension, ViewIdx(0), &linkData)) {
        KnobParametricPtr masterKnob = toKnobParametric(linkData.masterKnob.lock());
        if (masterKnob) {
            return masterKnob->getCurveColor(dimension, r, g, b);
        }
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

    for (std::size_t i = 0; i < _curves.size(); ++i) {
        _curves[i]->setXRange(min, max);
    }
}

std::pair<double, double> KnobParametric::getParametricRange() const
{
    ///Mt-safe as it never changes
    assert( !_curves.empty() );

    return _curves.front()->getXRange();
}

CurvePtr
KnobParametric::getDefaultParametricCurve(DimIdx dimension) const
{
    if (dimension < 0 || dimension >= (int)_curves.size()) {
        throw std::invalid_argument("KnobParametric::getDefaultParametricCurve dimension out of range");
    }

    MasterKnobLink linkData;
    if (getMaster(dimension, ViewIdx(0), &linkData)) {
        KnobParametricPtr masterKnob = toKnobParametric(linkData.masterKnob.lock());
        if (masterKnob) {
            return masterKnob->getDefaultParametricCurve(dimension);
        }
    }

    return _defaultCurves[dimension];

}

CurvePtr KnobParametric::getParametricCurve(DimIdx dimension) const
{
    ///Mt-safe as Curve is MT-safe and the pointer is never deleted
    if (dimension < 0 || dimension >= (int)_curves.size()) {
        throw std::invalid_argument("KnobParametric::getParametricCurve dimension out of range");
    }

    MasterKnobLink linkData;
    if (getMaster(dimension, ViewIdx(0), &linkData)) {
        KnobParametricPtr masterKnob = toKnobParametric(linkData.masterKnob.lock());
        if (masterKnob) {
            return masterKnob->getParametricCurve(dimension);
        }
    }


    return _curves[dimension];

}

StatusEnum
KnobParametric::addControlPoint(ValueChangedReasonEnum reason,
                                DimIdx dimension,
                                double key,
                                double value,
                                KeyframeTypeEnum interpolation)
{
    ///Mt-safe as Curve is MT-safe
    if ( ( dimension >= (int)_curves.size() ) ||
         ( key != key) || // check for NaN
         boost::math::isinf(key) ||
         ( value != value) || // check for NaN
         boost::math::isinf(value) ) {
        return eStatusFailed;
    }

    KeyFrame k(key, value);
    k.setInterpolation(interpolation);
    _curves[dimension]->addKeyFrame(k);
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);

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
    if ( ( dimension >= (int)_curves.size() ) ||
         ( key != key) || // check for NaN
         boost::math::isinf(key) ||
         ( value != value) || // check for NaN
         boost::math::isinf(value) ) {
        return eStatusFailed;
    }

    KeyFrame k(key, value, leftDerivative, rightDerivative);
    k.setInterpolation(interpolation);
    _curves[dimension]->addKeyFrame(k);
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::getValue(DimIdx dimension,
                         double parametricPosition,
                         double *returnValue) const
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    try {
        *returnValue = getParametricCurve(dimension)->getValueAt(parametricPosition);
    }catch (...) {
        return eStatusFailed;
    }

    return eStatusOK;
}

StatusEnum
KnobParametric::getNControlPoints(DimIdx dimension,
                                  int *returnValue) const
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    *returnValue =  getParametricCurve(dimension)->getKeyFramesCount();

    return eStatusOK;
}

StatusEnum
KnobParametric::getNthControlPoint(DimIdx dimension,
                                   int nthCtl,
                                   double *key,
                                   double *value) const
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    KeyFrame kf;
    bool ret = getParametricCurve(dimension)->getKeyFrameWithIndex(nthCtl, &kf);
    if (!ret) {
        return eStatusFailed;
    }
    *key = kf.getTime();
    *value = kf.getValue();

    return eStatusOK;
}

StatusEnum
KnobParametric::getNthControlPoint(DimIdx dimension,
                                   int nthCtl,
                                   double *key,
                                   double *value,
                                   double *leftDerivative,
                                   double *rightDerivative) const
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    KeyFrame kf;
    bool ret = getParametricCurve(dimension)->getKeyFrameWithIndex(nthCtl, &kf);
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
                                                int nThCtl,
                                                KeyframeTypeEnum interpolation)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    try {
        _curves[dimension]->setKeyFrameInterpolation(interpolation, nThCtl);
    } catch (...) {
        return eStatusFailed;
    }

    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::setNthControlPoint(ValueChangedReasonEnum reason,
                                   DimIdx dimension,
                                   int nthCtl,
                                   double key,
                                   double value)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    try {
        _curves[dimension]->setKeyFrameValueAndTime(key, value, nthCtl);
    } catch (...) {
        return eStatusFailed;
    }

    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::setNthControlPoint(ValueChangedReasonEnum reason,
                                   DimIdx dimension,
                                   int nthCtl,
                                   double key,
                                   double value,
                                   double leftDerivative,
                                   double rightDerivative)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    int newIdx;
    try {
        _curves[dimension]->setKeyFrameValueAndTime(key, value, nthCtl, &newIdx);
    } catch (...) {
        return eStatusFailed;
    }

    _curves[dimension]->setKeyFrameDerivatives(leftDerivative, rightDerivative, newIdx);
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::deleteControlPoint(ValueChangedReasonEnum reason,
                                   DimIdx dimension,
                                   int nthCtl)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    try {
        _curves[dimension]->removeKeyFrameWithIndex(nthCtl);
    } catch (...) {
        return eStatusFailed;
    }
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::deleteAllControlPoints(ValueChangedReasonEnum reason,
                                       DimIdx dimension)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    _curves[dimension]->clearKeyFrames();
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec::all(), reason);

    return eStatusOK;
}

bool
KnobParametric::cloneExtraData(const KnobIPtr& other,
                               ViewSetSpec /*view*/,
                               ViewSetSpec /*otherView*/,
                               DimSpec dimension,
                               DimSpec otherDimension,
                               double offset,
                               const RangeD* range)
{
    assert((dimension.isAll() && otherDimension.isAll()) || (!dimension.isAll() && !otherDimension.isAll()));
    ///Mt-safe as Curve is MT-safe
    KnobParametricPtr isParametric = toKnobParametric(other);

    if (!isParametric) {
        return false;
    }
    bool hasChanged = false;
    if (dimension.isAll()) {
        int dimMin = std::min( getNDimensions(), isParametric->getNDimensions() );
        for (int i = 0; i < dimMin; ++i) {
            hasChanged |= _curves[i]->cloneAndCheckIfChanged(*isParametric->_curves[i], offset /*offset*/, range /*range*/);
        }
    } else {
        assert( dimension >= 0 && dimension < getNDimensions() && otherDimension >= 0 && otherDimension < getNDimensions() );
        hasChanged |= _curves[dimension]->cloneAndCheckIfChanged(*isParametric->_curves[otherDimension], offset /*offset*/, range /*range*/);
    }

    return hasChanged;
}

void
KnobParametric::saveParametricCurves(std::list< SERIALIZATION_NAMESPACE::CurveSerialization >* curves) const
{
    for (U32 i = 0; i < _curves.size(); ++i) {
        SERIALIZATION_NAMESPACE::CurveSerialization c;
        _curves[i]->toSerialization(&c);
        curves->push_back(c);
    }
}

void
KnobParametric::loadParametricCurves(const std::list< SERIALIZATION_NAMESPACE::CurveSerialization > & curves)
{
    assert( !_curves.empty() );
    int i = 0;
    for (std::list< SERIALIZATION_NAMESPACE::CurveSerialization >::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        _curves[i]->fromSerialization(*it);
        ++i;
    }
}

void
KnobParametric::resetExtraToDefaultValue(DimSpec dimension, ViewSetSpec view)
{
    assert( _curves.size() == _defaultCurves.size() );
    removeAnimation(view, dimension);
    if (dimension.isAll()) {
        for (std::size_t i = 0; i < _curves.size(); ++i) {
            _curves[i]->clone(*_defaultCurves[i]);
        }
    } else {
        if  (dimension < 0 || dimension >= (int)_curves.size()) {
            throw std::invalid_argument("KnobParametric::resetExtraToDefaultValue: dimension out of range");
        }
        _curves[dimension]->clone(*_defaultCurves[dimension]);
    }
    Q_EMIT curveChanged(dimension);
}

void
KnobParametric::setDefaultCurvesFromCurves()
{
    assert( _curves.size() == _defaultCurves.size() );
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        _defaultCurves[i]->clone(*_curves[i]);
    }
    computeHasModifications();
}

bool
KnobParametric::hasModificationsVirtual(DimIdx dimension, ViewIdx /*view*/) const
{
    if  (dimension < 0 || dimension >= (int)_curves.size()) {
        throw std::invalid_argument("KnobParametric::hasModificationsVirtual: dimension out of range");
    }
    KeyFrameSet defKeys = _defaultCurves[dimension]->getKeyFrames_mt_safe();
    KeyFrameSet keys = _curves[dimension]->getKeyFrames_mt_safe();
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
KnobParametric::onKnobAboutToAlias(const KnobIPtr& slave)
{
    KnobParametricPtr isParametric = toKnobParametric(slave);

    if (isParametric) {
        _defaultCurves.resize( isParametric->_defaultCurves.size() );
        _curvesColor.resize( isParametric->_curvesColor.size() );
        assert( _curvesColor.size() == _defaultCurves.size() );
        for (std::size_t i = 0; i < isParametric->_defaultCurves.size(); ++i) {
            _defaultCurves[i].reset( new Curve(shared_from_this(), DimIdx(i), ViewIdx(0)) );
            _defaultCurves[i]->clone(*isParametric->_defaultCurves[i]);
            _curvesColor[i] = isParametric->_curvesColor[i];
        }
    }
}

void
KnobParametric::appendToHash(double /*time*/, ViewIdx /*view*/, Hash64* hash)
{

    for (std::size_t i = 0; i < _curves.size(); ++i) {
        // Parametric params are a corner case: the only way to know the value
        // is to serialize the whole curve
        CurvePtr curve = _curves[i];
        if (curve) {
            Hash64::appendCurve(curve, hash);
        }

    }

}

bool
KnobParametric::cloneCurve(ViewIdx view, DimIdx dimension, const Curve& curve, double offset, const RangeD* range, const StringAnimationManager* /*stringAnimation*/)
{
    if (dimension < 0 || dimension >= (int)_curves.size()) {
        throw std::invalid_argument("KnobParametric: dimension out of range");
    }
    bool ret = _curves[dimension]->cloneAndCheckIfChanged(curve, offset, range);
    if (ret) {
        Q_EMIT curveChanged(dimension);
        evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec(view), eValueChangedReasonNatronInternalEdited);
    }
    return ret;
}

void
KnobParametric::deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec dimension)
{
    if (dimension.isAll()) {
        for (std::size_t i = 0; i < _curves.size(); ++i) {
            for (std::list<double>::const_iterator it = times.begin(); it!=times.end(); ++it) {
                _curves[i]->removeKeyFrameWithTime(*it);
            }
        }
    } else {
        if (dimension < 0 || dimension >= (int)_curves.size()) {
            throw std::invalid_argument("KnobParametric: dimension out of range");
        }
        for (std::list<double>::const_iterator it = times.begin(); it!=times.end(); ++it) {
            _curves[dimension]->removeKeyFrameWithTime(*it);
        }
    }


    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);
}

bool
KnobParametric::warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes)
{
    bool ok = false;
    if (dimension.isAll()) {
        for (std::size_t i = 0; i < _curves.size(); ++i) {
            ok |= _curves[i]->transformKeyframesValueAndTime(times, warp, keyframes);
        }
    } else {
        if (dimension < 0 || dimension >= (int)_curves.size()) {
            throw std::invalid_argument("KnobParametric: dimension out of range");
        }
        ok |= _curves[dimension]->transformKeyframesValueAndTime(times, warp, keyframes);

    }


    if (ok) {
        Q_EMIT curveChanged(dimension);
        evaluateValueChange(DimIdx(0), getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);
        return true;
    }
    return false;
}

void
KnobParametric::removeAnimation(ViewSetSpec view, DimSpec dim)
{
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        if (!dim.isAll() && dim != (int)i) {
            continue;
        }
        _curves[i]->clearKeyFrames();
    }
    Q_EMIT curveChanged(dim);
    evaluateValueChange(DimIdx(0), getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);

}

void
KnobParametric::deleteAnimationBeforeTime(double time, ViewSetSpec view, DimSpec dimension)
{
    if (dimension.isAll()) {
        for (int i = 0; i < (int)_curves.size(); ++i) {
            _curves[i]->removeKeyFramesAfterTime(time, 0);
        }
    } else {
        DimIdx dim_i(dimension.value());
        if (dim_i < 0 || dim_i >= (int)_curves.size()) {
            throw std::invalid_argument("KnobParametric: dimension out of range");
        }
        _curves[dim_i]->removeKeyFramesAfterTime(time, 0);

    }
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);

}

void
KnobParametric::deleteAnimationAfterTime(double time, ViewSetSpec view, DimSpec dimension)
{
    if (dimension.isAll()) {
        for (int i = 0; i < (int)_curves.size(); ++i) {
            _curves[i]->removeKeyFramesAfterTime(time, 0);
        }
    } else {
        DimIdx dim_i(dimension.value());
        if (dim_i < 0 || dim_i >= (int)_curves.size()) {
            throw std::invalid_argument("KnobParametric: dimension out of range");
        }
        _curves[dim_i]->removeKeyFramesAfterTime(time, 0);

    }
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);
}

void
KnobParametric::setInterpolationAtTimes(ViewSetSpec view, DimSpec dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys)
{
    if (dimension.isAll()) {
        for (int i = 0; i < (int)_curves.size(); ++i) {
            for (std::list<double>::const_iterator it = times.begin(); it!=times.end(); ++it) {
                KeyFrame k;
                if (_curves[i]->setKeyFrameInterpolation(interpolation, *it, &k)) {
                    if (newKeys && i == 0) {
                        newKeys->push_back(k);
                    }
                }
            }

        }
    } else {
        DimIdx dim_i(dimension.value());
        if (dim_i < 0 || dim_i >= (int)_curves.size()) {
            throw std::invalid_argument("KnobParametric: dimension out of range");
        }
        for (std::list<double>::const_iterator it = times.begin(); it!=times.end(); ++it) {
            KeyFrame k;
            if (_curves[dim_i]->setKeyFrameInterpolation(interpolation, *it, &k)) {
                if (newKeys) {
                    newKeys->push_back(k);
                }
            }
        }

    }
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);
}

bool
KnobParametric::setLeftAndRightDerivativesAtTime(ViewSetSpec view, DimSpec dimension, double time, double left, double right)
{
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        if (!dimension.isAll() && dimension != (int)i) {
            continue;
        }
        int keyIndex = _curves[i]->keyFrameIndex(time);
        if (keyIndex == -1) {
            return false;
        }

        _curves[i]->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
        _curves[i]->setKeyFrameDerivatives(left, right, keyIndex);
    }


    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);
    return true;
}

bool
KnobParametric::setDerivativeAtTime(ViewSetSpec view, DimSpec dimension, double time, double derivative, bool isLeft)
{
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        if (!dimension.isAll() && dimension != (int)i) {
            continue;
        }
        int keyIndex = _curves[i]->keyFrameIndex(time);
        if (keyIndex == -1) {
            return false;
        }

        _curves[dimension]->setKeyFrameInterpolation(eKeyframeTypeBroken, keyIndex);
        if (isLeft) {
            _curves[dimension]->setKeyFrameLeftDerivative(derivative, keyIndex);
        } else {
            _curves[dimension]->setKeyFrameRightDerivative(derivative, keyIndex);
        }    }


    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);
    return true;
}

ValueChangedReturnCodeEnum
KnobParametric::setKeyFrameInternal(double time, double value, const CurvePtr& curve, KeyFrame* newKey)
{
    ValueChangedReturnCodeEnum ret = curve->setOrAddKeyframe(KeyFrame(time, value));
    if (newKey) {
        (void)curve->getKeyFrameWithTime(time, newKey);
    }
    return ret;
}

ValueChangedReturnCodeEnum
KnobParametric::setDoubleValueAtTime(double time, double value, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason, KeyFrame* newKey)
{
    ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNothingChanged;
    if (dimension.isAll()) {
        for (std::size_t i = 0; i < _curves.size(); ++i) {
            ret = setKeyFrameInternal(time, value, _curves[i], newKey);
        }
    } else {
        if (dimension < 0 || dimension >= (int)_curves.size()) {
            throw std::invalid_argument("KnobParametric: dimension out of range");
        }
        ret = setKeyFrameInternal(time, value, _curves[dimension], newKey);
    }

    if (ret != eValueChangedReturnCodeNothingChanged) {
        Q_EMIT curveChanged(dimension);
        evaluateValueChange(DimIdx(0), getCurrentTime(), view, reason);
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
    if (dimension.isAll()) {
        for (std::size_t i = 0; i < _curves.size(); ++i) {
            KeyFrame key;
            for (std::list<DoubleTimeValuePair>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
                setKeyFrameInternal(it->time, it->value, _curves[i], newKey ? &key : 0);
                if (newKey) {
                    newKey->push_back(key);
                }
            }
        }
    } else {
        if (dimension < 0 || dimension >= (int)_curves.size()) {
            throw std::invalid_argument("KnobParametric: dimension out of range");
        }

        KeyFrame key;
        for (std::list<DoubleTimeValuePair>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            setKeyFrameInternal(it->time, it->value, _curves[dimension], newKey ? &key : 0);
            if (newKey) {
                newKey->push_back(key);
            }
        }
    }

    Q_EMIT curveChanged(dimension);
    evaluateValueChange(DimIdx(0), getCurrentTime(), view, reason);
}

void
KnobParametric::setDoubleValueAtTimeAcrossDimensions(double time, const std::vector<double>& values, DimIdx dimensionStartIndex, ViewSetSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    if (values.empty()) {
        return;
    }
    if (dimensionStartIndex < 0 || dimensionStartIndex + values.size() > _curves.size()) {
        throw std::invalid_argument("KnobParametric: dimension out of range");
    }
    for (std::size_t i = 0; i < values.size(); ++i) {
        ValueChangedReturnCodeEnum ret = setKeyFrameInternal(time, values[i], _curves[dimensionStartIndex + i], 0);
        if (retCodes) {
            retCodes->push_back(ret);
        }
    }
    Q_EMIT curveChanged(DimSpec::all());
    evaluateValueChange(DimIdx(0), getCurrentTime(), view, reason);

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
            setKeyFrameInternal(it2->time, it2->value, _curves[keysPerDimension[i].first.dimension], 0);
        }

    }
    Q_EMIT curveChanged(DimSpec::all());

    evaluateValueChange(DimIdx(0), getCurrentTime(), ViewSetSpec(0), reason);
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
