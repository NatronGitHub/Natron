/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <cmath>
#include <locale>
#include <sstream>
#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/Curve.h"
#include "Engine/EffectInstance.h"
#include "Engine/Format.h"
#include "Engine/Image.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/RotoContext.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

//using std::make_pair;
//using std::pair;

/******************************KnobInt**************************************/
KnobInt::KnobInt(KnobHolder* holder,
                 const std::string &label,
                 int dimension,
                 bool declaredByPlugin)
    : KnobIntBase(holder, label, dimension, declaredByPlugin)
    , _increments(dimension, 1)
    , _disableSlider(false)
    , _isRectangle(false)
{
}

KnobInt::KnobInt(KnobHolder* holder,
                 const QString &label,
                 int dimension,
                 bool declaredByPlugin)
    : KnobIntBase(holder, label.toStdString(), dimension, declaredByPlugin)
    , _increments(dimension, 1)
    , _disableSlider(false)
    , _isRectangle(false)
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
                      int index)
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
    assert( (int)incr.size() == getDimension() );
    _increments = incr;
    for (U32 i = 0; i < _increments.size(); ++i) {
        if (_increments[i] <= 0) {
            qDebug() << "Attempting to set the increment of an int param to a value lesser or equal to 0";
            continue;
        }
        Q_EMIT incrementChanged(_increments[i], i);
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

const std::string KnobInt::_typeNameStr("Int");
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

KnobBool::KnobBool(KnobHolder* holder,
                   const std::string &label,
                   int dimension,
                   bool declaredByPlugin)
    : KnobBoolBase(holder, label, dimension, declaredByPlugin)
{
}

KnobBool::KnobBool(KnobHolder* holder,
                   const QString &label,
                   int dimension,
                   bool declaredByPlugin)
    : KnobBoolBase(holder, label.toStdString(), dimension, declaredByPlugin)
{
}

bool
KnobBool::canAnimate() const
{
    return canAnimateStatic();
}

const std::string KnobBool::_typeNameStr("Bool");
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


KnobDouble::KnobDouble(KnobHolder* holder,
                       const std::string &label,
                       int dimension,
                       bool declaredByPlugin)
    : KnobDoubleBase(holder, label, dimension, declaredByPlugin)
    , _spatial(false)
    , _isRectangle(false)
    , _increments(dimension, 1)
    , _decimals(dimension, 2)
    , _disableSlider(false)
    , _autoFoldEnabled(false)
    , _valueIsNormalized(dimension, eValueIsNormalizedNone)
    , _defaultValuesAreNormalized(false)
    , _hasHostOverlayHandle(false)
{
}

KnobDouble::KnobDouble(KnobHolder* holder,
                       const QString &label,
                       int dimension,
                       bool declaredByPlugin)
    : KnobDoubleBase(holder, label.toStdString(), dimension, declaredByPlugin)
    , _spatial(false)
    , _isRectangle(false)
    , _increments(dimension, 1)
    , _decimals(dimension, 2)
    , _disableSlider(false)
    , _autoFoldEnabled(false)
    , _valueIsNormalized(dimension, eValueIsNormalizedNone)
    , _defaultValuesAreNormalized(false)
    , _hasHostOverlayHandle(false)
{
    if (dimension >= 4) {
        disableSlider();
    }
}

void
KnobDouble::setHasHostOverlayHandle(bool handle)
{
    KnobHolder* holder = getHolder();

    if (holder) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
        if (!effect) {
            return;
        }
        if ( !effect->getNode() ) {
            return;
        }
        KnobDoublePtr thisSharedDouble = std::dynamic_pointer_cast<KnobDouble>(shared_from_this());
        assert(thisSharedDouble);
        if (handle) {
            effect->getNode()->addPositionInteract(thisSharedDouble,
                                                   KnobBoolPtr() /*interactive*/);
        } else {
            effect->getNode()->removePositionHostOverlay(this);
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

const std::string KnobDouble::_typeNameStr("Double");
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
                         int index)
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
                        int index)
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
    assert( incr.size() == (U32)getDimension() );
    _increments = incr;
    for (U32 i = 0; i < incr.size(); ++i) {
        Q_EMIT incrementChanged(_increments[i], i);
    }
}

void
KnobDouble::setDecimals(const std::vector<int> &decis)
{
    assert( decis.size() == (U32)getDimension() );
    _decimals = decis;
    for (U32 i = 0; i < decis.size(); ++i) {
        Q_EMIT decimalsChanged(decis[i], i);
    }
}

KnobDouble::~KnobDouble()
{
}

static void
getInputRoD(EffectInstance* effect,
            double /*time*/,
            RectD & rod)
{
#ifdef NATRON_NORMALIZE_SPATIAL_WITH_ROD
    RenderScale scale;
    scale.y = scale.x = 1.;
    bool isProjectFormat;
    Status stat = effect->getRegionOfDefinition_public(effect->getHash(), time, scale, /*view*/ 0, &rod, &isProjectFormat);
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
KnobDouble::denormalize(const int dimension,
                        const double time,
                        const double value) const
{
    EffectInstance* effect = dynamic_cast<EffectInstance*>( getHolder() );

    assert(effect);
    if (!effect) {
        // coverity[dead_error_line]
        return value;
    }
    RectD rod;
    getInputRoD(effect, time, rod);
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
KnobDouble::normalize(const int dimension,
                      const double time,
                      const double value) const
{
    EffectInstance* effect = dynamic_cast<EffectInstance*>( getHolder() );

    assert(effect);
    if (!effect) {
        // coverity[dead_error_line]
        return value;
    }
    RectD rod;
    getInputRoD(effect, time, rod);
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
KnobDouble::computeValuesHaveModifications(int dimension,
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

KnobButton::KnobButton(KnobHolder* holder,
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

KnobButton::KnobButton(KnobHolder* holder,
                       const QString &label,
                       int dimension,
                       bool declaredByPlugin)
    : KnobBoolBase(holder, label.toStdString(), dimension, declaredByPlugin)
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

const std::string KnobButton::_typeNameStr("Button");
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
    return evaluateValueChange(0, getCurrentTime(), ViewIdx(0),  eValueChangedReasonUserEdited);
}

/******************************KnobChoice**************************************/

#define KNOBCHOICE_MAX_ENTRIES_HELP 40 \
    // don't show help in the tootlip if there are more entries that this

KnobChoice::KnobChoice(KnobHolder* holder,
                       const std::string &label,
                       int dimension,
                       bool declaredByPlugin)
    : KnobIntBase(holder, label, dimension, declaredByPlugin)
    , _entriesMutex()
    , _currentEntry()
    , _addNewChoice(false)
    , _isCascading(false)
{
}

KnobChoice::KnobChoice(KnobHolder* holder,
                       const QString &label,
                       int dimension,
                       bool declaredByPlugin)
    : KnobIntBase(holder, label.toStdString(), dimension, declaredByPlugin)
    , _entriesMutex()
    , _currentEntry()
    , _addNewChoice(false)
    , _isCascading(false)
{
}

KnobChoice::~KnobChoice()
{
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

const std::string KnobChoice::_typeNameStr("Choice");
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

void
KnobChoice::cloneExtraData(KnobI* other,
                           int /*dimension*/,
                           int /*otherDimension*/)
{
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(other);

    if (!isChoice) {
        return;
    }

    QMutexLocker k(&_entriesMutex);
    _currentEntry = isChoice->getActiveEntry();
}

bool
KnobChoice::cloneExtraDataAndCheckIfChanged(KnobI* other,
                                            int /*dimension*/,
                                            int /*otherDimension*/)
{
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(other);

    if (!isChoice) {
        return false;
    }

    ChoiceOption otherEntry = isChoice->getActiveEntry();
    QMutexLocker k(&_entriesMutex);
    if (_currentEntry.id != otherEntry.id) {
        _currentEntry = otherEntry;

        return true;
    }

    return false;
}

void
KnobChoice::cloneExtraData(KnobI* other,
                           double /*offset*/,
                           const RangeD* /*range*/,
                           int /*dimension*/,
                           int /*otherDimension*/)
{
    // TODO: Isn't this redundant, as it's implementation is identical to the other cloneExtraData?
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(other);

    if (!isChoice) {
        return;
    }

    QMutexLocker k(&_entriesMutex);
    _currentEntry = isChoice->getActiveEntry();
}


void
KnobChoice::onInternalValueChanged(int dimension,
                                   double time,
                                   ViewSpec /*view*/)
{
    // by-pass any master/slave link here
    int index = getValueAtTime(time, dimension, ViewSpec::current(), true, true);
    QMutexLocker k(&_entriesMutex);

    if ( (index >= 0) && ( index < (int)_entries.size() ) ) {
        _currentEntry = _entries[index];
    }
}

#if 0 // dead code
// replaced by StrUtils::iequals(a, b)
static bool
caseInsensitiveCompare(const std::string& a,
                       const std::string& b)
{
    if ( a.size() != b.size() ) {
        return false;
    }
    std::locale loc;
    for (std::size_t i = 0; i < a.size(); ++i) {
        char aLow = std::tolower(a[i], loc);
        char bLow = std::tolower(b[i], loc);
        if (aLow != bLow) {
            return false;
        }
    }

    return true;
}

#endif


void
KnobChoice::findAndSetOldChoice()
{

    int found = -1;

    {
        QMutexLocker k(&_entriesMutex);

        if ( !_currentEntry.id.empty() ) {
            ChoiceOption matched;
            found = choiceMatch(_currentEntry.id, _entries, &matched);
            if (found != -1) {
                _currentEntry = matched;
            }
        }
    }
    if (found != -1) {
        // Make sure we don't call knobChanged if we found the value
        blockValueChanges();
        beginChanges();
        setValue(found);
        unblockValueChanges();
        endChanges();
    }
}

bool
KnobChoice::populateChoices(const std::vector<ChoiceOption> &entries)
{
    bool hasChanged = false;
    {
        QMutexLocker l(&_entriesMutex);
        _entries = entries;
        for (std::size_t i = 0; i < _entries.size(); ++i) {

            // The ID cannot be empty, this is the only way to uniquely identify the choice.
            assert(!_entries[i].id.empty());

            // If the label is not set, use the ID
            if (_entries[i].label.empty()) {
                _entries[i].label = _entries[i].id;
            }
        }
        hasChanged = true;

    }

    /*
       Try to restore the last choice.
     */
    if (hasChanged) {
        findAndSetOldChoice();


        if (_signalSlotHandler) {
            _signalSlotHandler->s_helpChanged();
        }
        Q_EMIT populated();
    }

    return hasChanged;
} // KnobChoice::populateChoices

void
KnobChoice::resetChoices()
{
    {
        QMutexLocker l(&_entriesMutex);
        _entries.clear();
    }
    findAndSetOldChoice();
    if (_signalSlotHandler) {
        _signalSlotHandler->s_helpChanged();
    }
    Q_EMIT entriesReset();
}

void
KnobChoice::appendChoice(const ChoiceOption& entry)
{
    {
        QMutexLocker l(&_entriesMutex);
        _entries.push_back(entry);
        ChoiceOption& opt = _entries.back();

        // If label is empty, set to the option id
        if (opt.label.empty()) {
            opt.label = opt.id;
        }
    }

    findAndSetOldChoice();
    if (_signalSlotHandler) {
        _signalSlotHandler->s_helpChanged();
    }
    Q_EMIT entryAppended();
}

std::vector<ChoiceOption>
KnobChoice::getEntries_mt_safe() const
{
    QMutexLocker l(&_entriesMutex);

    return _entries;
}

bool
KnobChoice::isActiveEntryPresentInEntries() const
{
    QMutexLocker k(&_entriesMutex);

    if ( _currentEntry.id.empty() ) {
        return true;
    }
    for (std::size_t i = 0; i < _entries.size(); ++i) {
        if (_entries[i].id == _currentEntry.id) {
            return true;
        }
    }

    return false;
}

ChoiceOption
KnobChoice::getEntry(int v) const
{
    if ( (int)_entries.size() <= v || v < 0) {
        throw std::runtime_error( std::string("KnobChoice::getEntry: index out of range") );
    }

    return _entries[v];
}

int
KnobChoice::getNumEntries() const
{
    QMutexLocker l(&_entriesMutex);

    return (int)_entries.size();
}

void
KnobChoice::setActiveEntry(const ChoiceOption& opt)
{
    QMutexLocker l(&_entriesMutex);
    _currentEntry = opt;
}


ChoiceOption
KnobChoice::getActiveEntry()
{
    std::pair<int, KnobIPtr> master = getMaster(0);

    if (master.second) {
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( master.second.get() );
        if (isChoice) {
            return isChoice->getActiveEntry();
        }
    }
    QMutexLocker l(&_entriesMutex);

    if ( !_currentEntry.id.empty() ) {
        return _currentEntry;
    }
    int activeIndex = getValue();
    if ( activeIndex < (int)_entries.size() ) {
        _currentEntry = _entries[activeIndex];
        return _entries[activeIndex];
    }

    return ChoiceOption();
}

#if 0 // dead code
// replaced by StrUtils::trim_copy(str)
static std::string
trim(std::string const & str)
{
    const std::string whitespace = " \t\f\v\n\r";
    std::size_t first = str.find_first_not_of(whitespace);

    // If there is no non-whitespace character, both first and last will be std::string::npos (-1)
    // There is no point in checking both, since if either doesn't work, the
    // other won't work, either.
    if (first == std::string::npos) {
        return "";
    }

    std::size_t last  = str.find_last_not_of(whitespace);

    return str.substr(first, last - first + 1);
}

// replaced by std::replace_if(str.begin(), str.end(), ::isspace, ' ');
static void
whitespacify(std::string & str)
{
    std::replace( str.begin(), str.end(), '\t', ' ');
    std::replace( str.begin(), str.end(), '\f', ' ');
    std::replace( str.begin(), str.end(), '\v', ' ');
    std::replace( str.begin(), str.end(), '\n', ' ');
    std::replace( str.begin(), str.end(), '\r', ' ');
}

#endif

std::string
KnobChoice::getHintToolTipFull() const
{
    assert( QThread::currentThread() == qApp->thread() );

    int gothelp = 0;

    // list values that either have help or have label != id
    if ( !_entries.empty() ) {
        assert( _entries.size() == _entries.size() );
        for (U32 i = 0; i < _entries.size(); ++i) {
            if ( !_entries.empty() && ( (_entries[i].id != _entries[i].label) || !_entries[i].tooltip.empty() ) ) {
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
        ss << StrUtils::trim_copy( getHintToolTip() );
        if (gothelp) {
            // if there are per-option help strings, separate them from main hint
            ss << "\n\n";
        }
    }
    // param may have no hint but still have per-option help
    if (gothelp) {
        for (U32 i = 0; i < _entries.size(); ++i) {
            if ( !_entries[i].tooltip.empty() ) { // no help line is needed if help is unavailable for this option
                std::string entry = StrUtils::trim_copy(_entries[i].label);
                std::replace_if(entry.begin(), entry.end(), ::isspace, ' ');
                if (_entries[i].label != _entries[i].id) {
                    entry += "  (" + _entries[i].id + ")";
                }
                std::string help = StrUtils::trim_copy(_entries[i].tooltip);
                std::replace_if(help.begin(), help.end(), ::isspace, ' ');
                if ( isHintInMarkdown() ) {
                    ss << "* **" << entry << "**";
                } else {
                    ss << entry;
                }
                ss << ": ";
                ss << help;
                if (i < _entries.size() - 1) {
                    ss << '\n';
                }
            }
        }
    }

    return ss.str();
} // KnobChoice::getHintToolTipFull

KnobHelper::ValueChangedReturnCodeEnum
KnobChoice::setValueFromID(const std::string & value,
                              int dimension,
                              bool turnOffAutoKeying)
{
    int i;
    {
        QMutexLocker k(&_entriesMutex);
        i = choiceMatch(value, _entries, &_currentEntry);
    }
    if (i >= 0) {
        return setValue(i, ViewIdx(0), dimension, turnOffAutoKeying);
    }
    /*{
        QMutexLocker k(&_entriesMutex);
        _currentEntryLabel = value;
       }
       return setValue(-1, ViewIdx(0), dimension, turnOffAutoKeying);*/
    return eValueChangedReturnCodeNothingChanged;
}

void
KnobChoice::setDefaultValueFromIDWithoutApplying(const std::string & value,
                                                    int dimension)
{
    int i = choiceMatch(value, _entries, 0);
    if (i >= 0) {
        return setDefaultValueWithoutApplying(i, dimension);
    }
    throw std::runtime_error(std::string("KnobChoice::setDefaultValueFromLabel: unknown label ") + value);
}

void
KnobChoice::setDefaultValueFromID(const std::string & value,
                                     int dimension)
{
    int i = choiceMatch(value, _entries, 0);
    if (i >= 0) {
        return setDefaultValue(i, dimension);
    }
    throw std::runtime_error(std::string("KnobChoice::setDefaultValueFromLabel: unknown label ") + value);
}

// try to match entry id first, then label
static
const std::string&
entryStr(const ChoiceOption& opt, int s)
{
    return s == 0 ? opt.id : opt.label;
}

static
bool BothAreSpaces(char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); }

// Choice restoration tries several options to restore a choice value:
// 1- exact string match, same index
// 2- exact string match, other index
// 3- exact string match before the first '\t', other index
// 4- case-insensistive string match, other index
// 5- paren/bracket-insensitive match (for WriteFFmpeg's format and codecs)
// 6- if the choice ends with " 1" try to match exactly everything before that  (for formats with PAR=1, where the PAR was removed)
// returns index if choice was matched, -1 if not matched
#pragma message WARN("choiceMatch() should be moved into filterKnobChoiceOptionCompat().")
// TODO: choiceMatch() should be moved into filterKnobChoiceOptionCompat()
// TODO: filterKnobChoiceOptionCompat() should be used everywhere instead of choiceMatch()
int
KnobChoice::choiceMatch(const std::string& choice,
                        const std::vector<ChoiceOption>& entries,
                        ChoiceOption* matchedEntry)
{
    if (entries.size() == 0) {
        // no match

        return -1;
    }
    
    // try to match entry id first, then label
    for (int s = 0; s < 2; ++s) {
        // 2- try exact match, other index
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (entryStr(entries[i], s) == choice) {
                if (matchedEntry) {
                    *matchedEntry = entries[i];
                }
                return i;
            }
        }

        // 3- match the part before '\t' with the part before '\t'. This is for value-tab-description options such as in the WriteFFmpeg codec
        std::size_t choicetab = choice.find('\t'); // returns string::npos if no tab was found
        std::string choicemain = choice.substr(0, choicetab); // gives the entire string if no tabs were found
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const ChoiceOption& entry(entries[i]);

            // There is no real reason for an id to contain a tab, but let us search for it anyway
            std::size_t entryidtab = entry.id.find('\t'); // returns string::npos if no tab was found
            std::string entryidmain = entry.id.substr(0, entryidtab); // gives the entire string if no tabs were found

            if (entryidmain == choicemain) {
                if (matchedEntry) {
                    *matchedEntry = entries[i];
                }
                return i;
            }

            // The label may contain a tab
            std::size_t entrytab = entry.label.find('\t'); // returns string::npos if no tab was found
            std::string entrymain = entry.label.substr(0, entrytab); // gives the entire string if no tabs were found

            if (entrymain == choicemain) {
                if (matchedEntry) {
                    *matchedEntry = entries[i];
                }
                return i;
            }
        }

        // 4- case-insensitive match
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if ( StrUtils::iequals(entryStr(entries[i], s), choice) ) {
                if (matchedEntry) {
                    *matchedEntry = entries[i];
                }
                return i;
            }
        }

        // 5- paren/bracket-insensitive match (for WriteFFmpeg's format and codecs, parameter names "format" and "codec" in fr.inria.openfx.WriteFFmpeg)
        std::string choiceparen = choice;
        std::replace( choiceparen.begin(), choiceparen.end(), '[', '(');
        std::replace( choiceparen.begin(), choiceparen.end(), ']', ')');
        for (std::size_t i = 0; i < entries.size(); ++i) {
            std::string entryparen = entryStr(entries[i], s);
            std::replace( entryparen.begin(), entryparen.end(), '[', '(');
            std::replace( entryparen.begin(), entryparen.end(), ']', ')');

            if (choiceparen == entryparen) {
                if (matchedEntry) {
                    *matchedEntry = entries[i];
                }
                return i;
            }
        }

        // 6- handle old format strings, like "square_256  256 x 256  1":
        // - remove duplicate spaces
        // - if the choice ends with " 1" try to match exactly everything before that  (for formats with par=1, where the PAR was removed)
        // - if the choice contains " x ", try to remove one space before and after the x
        // Note: the parameter name is "outputFormat" in project serialization
        {
            bool choiceformatfound = false;
            std::string choiceformat = StrUtils::trim_copy(choice); // trim leading and trailing whitespace
            if (choiceformat != choice) {
                choiceformatfound = true;
            }
            if (choiceformat.find("  ") != std::string::npos) { // remove duplicate spaces
                std::string::iterator new_end = std::unique(choiceformat.begin(), choiceformat.end(), BothAreSpaces);
                choiceformat.erase(new_end, choiceformat.end());
                choiceformatfound = true;
            }
            if (choiceformat.find(" 1", choiceformat.size() - 2) != std::string::npos) { // remove " 1" at the end
                choiceformat.resize(choiceformat.size() - 2);
                choiceformatfound = true;
            }
            if (choiceformat.find(" x ") != std::string::npos) { // remove spaces around 'x'
                StrUtils::replace_first(choiceformat, " x ", "x");
                choiceformatfound = true;
            }
            if (choiceformatfound) {
                for (std::size_t i = 0; i < entries.size(); ++i) {
                    if (entryStr(entries[i], s) == choiceformat) {
                        if (matchedEntry) {
                            *matchedEntry = entries[i];
                        }
                        return i;

                    }
                }
            }
        }
    } // for s

    // no match
    return -1;
}

int
KnobChoice::choiceRestorationId(KnobChoice* knob,
                                const std::string &optionID)
{
    assert(knob);

    int serializedIndex = knob->getValue();
    if ( ( serializedIndex < (int)_entries.size() ) && (_entries[serializedIndex].id == optionID) ) {
        // we're lucky, entry hasn't changed
        return serializedIndex;

    }

    // try to find the same label at some other index
    int i;
    {
        QMutexLocker k(&_entriesMutex);
        i = choiceMatch(optionID, _entries, &_currentEntry);
    }

    if (i >= 0) {
        return i;
    }
    return -1;
}

void
KnobChoice::choiceRestoration(KnobChoice* knob,
                              const std::string &optionID,
                              int id)
{
    assert(knob);


    ///Clone first and then handle restoration of the static value
    clone(knob);
    setSecret( knob->getIsSecret() );
    if ( getDimension() == knob->getDimension() ) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            setEnabled( i, knob->isEnabled(i) );
        }
    }
    

    {
        QMutexLocker k(&_entriesMutex);
        if (id >= 0) {
            // we found a reasonable id
            _currentEntry = _entries[id]; // avoid numerous warnings in the GUI
        } else {
            _currentEntry.id = optionID;
        }
    }

    if (id >= 0) {
        setValue(id, ViewSpec::all(), 0, eValueChangedReasonNatronInternalEdited, NULL, true);
    }
}

void
KnobChoice::onKnobAboutToAlias(const KnobIPtr &slave)
{
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( slave.get() );

    if (isChoice) {
        populateChoices(isChoice->getEntries_mt_safe());
    }
}

void
KnobChoice::onOriginalKnobPopulated()
{
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( sender() );

    if (!isChoice) {
        return;
    }
    populateChoices(isChoice->_entries);
}

void
KnobChoice::onOriginalKnobEntriesReset()
{
    resetChoices();
}

void
KnobChoice::onOriginalKnobEntryAppend()
{
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( sender() );

    if (!isChoice) {
        return;
    }

    populateChoices(isChoice->_entries);
}

void
KnobChoice::handleSignalSlotsForAliasLink(const KnobIPtr& alias,
                                          bool connect)
{
    assert(alias);
    KnobChoice* aliasIsChoice = dynamic_cast<KnobChoice*>( alias.get() );
    if (!aliasIsChoice) {
        return;
    }
    if (connect) {
        QObject::connect( this, SIGNAL(populated()), aliasIsChoice, SLOT(onOriginalKnobPopulated()) );
        QObject::connect( this, SIGNAL(entriesReset()), aliasIsChoice, SLOT(onOriginalKnobEntriesReset()) );
        QObject::connect( this, SIGNAL(entryAppended()), aliasIsChoice,
                          SLOT(onOriginalKnobEntryAppend()) );
    } else {
        QObject::disconnect( this, SIGNAL(populated()), aliasIsChoice, SLOT(onOriginalKnobPopulated()) );
        QObject::disconnect( this, SIGNAL(entriesReset()), aliasIsChoice, SLOT(onOriginalKnobEntriesReset()) );
        QObject::disconnect( this, SIGNAL(entryAppended()), aliasIsChoice,
                             SLOT(onOriginalKnobEntryAppend()) );
    }
}

/******************************KnobSeparator**************************************/

KnobSeparator::KnobSeparator(KnobHolder* holder,
                             const std::string &label,
                             int dimension,
                             bool declaredByPlugin)
    : KnobBoolBase(holder, label, dimension, declaredByPlugin)
{
}

KnobSeparator::KnobSeparator(KnobHolder* holder,
                             const QString &label,
                             int dimension,
                             bool declaredByPlugin)
    : KnobBoolBase(holder, label.toStdString(), dimension, declaredByPlugin)
{
}

bool
KnobSeparator::canAnimate() const
{
    return false;
}

const std::string KnobSeparator::_typeNameStr("Separator");
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

KnobColor::KnobColor(KnobHolder* holder,
                     const std::string &label,
                     int dimension,
                     bool declaredByPlugin)
    : KnobDoubleBase(holder, label, dimension, declaredByPlugin)
    , _allDimensionsEnabled(true)
    , _simplifiedMode(false)
{
    //dimension greater than 4 is not supported. Dimension 2 doesn't make sense.
    assert(dimension == 1 || dimension == 3 ||  dimension == 4);
    if (dimension != 1 && dimension != 3 && dimension != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }
}

KnobColor::KnobColor(KnobHolder* holder,
                     const QString &label,
                     int dimension,
                     bool declaredByPlugin)
    : KnobDoubleBase(holder, label.toStdString(), dimension, declaredByPlugin)
    , _allDimensionsEnabled(true)
    , _simplifiedMode(false)
{
    //dimension greater than 4 is not supported. Dimension 2 doesn't make sense.
    assert(dimension == 1 || dimension == 3 ||  dimension == 4);
    if (dimension != 1 && dimension != 3 && dimension != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }
}

void
KnobColor::onDimensionSwitchToggled(bool b)
{
    _allDimensionsEnabled = b;
}

bool
KnobColor::areAllDimensionsEnabled() const
{
    return _allDimensionsEnabled;
}

bool
KnobColor::canAnimate() const
{
    return true;
}

const std::string
KnobColor::_typeNameStr("Color");
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


KnobString::KnobString(KnobHolder* holder,
                       const std::string &label,
                       int dimension,
                       bool declaredByPlugin)
    : AnimatingKnobStringHelper(holder, label, dimension, declaredByPlugin)
    , _multiLine(false)
    , _richText(false)
    , _customHtmlText(false)
    , _isLabel(false)
    , _isCustom(false)
{
}

KnobString::KnobString(KnobHolder* holder,
                       const QString &label,
                       int dimension,
                       bool declaredByPlugin)
    : AnimatingKnobStringHelper(holder, label.toStdString(), dimension, declaredByPlugin)
    , _multiLine(false)
    , _richText(false)
    , _customHtmlText(false)
    , _isLabel(false)
    , _isCustom(false)
{
}

KnobString::~KnobString()
{
}

bool
KnobString::canAnimate() const
{
    return canAnimateStatic();
}

const std::string KnobString::_typeNameStr("String");
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

void
KnobString::setAsLabel()
{
    setAnimationEnabled(false); //< labels cannot animate
    _isLabel = true;
}

/******************************KnobGroup**************************************/

KnobGroup::KnobGroup(KnobHolder* holder,
                     const std::string &label,
                     int dimension,
                     bool declaredByPlugin)
    : KnobBoolBase(holder, label, dimension, declaredByPlugin)
    , _isTab(false)
    , _isToolButton(false)
    , _isDialog(false)
{
}

KnobGroup::KnobGroup(KnobHolder* holder,
                     const QString &label,
                     int dimension,
                     bool declaredByPlugin)
    : KnobBoolBase(holder, label.toStdString(), dimension, declaredByPlugin)
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

const std::string KnobGroup::_typeNameStr("Group");
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
KnobGroup::removeKnob(KnobI* k)
{
    for (std::vector<KnobIWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->lock().get() == k) {
            _children.erase(it);

            return;
        }
    }
}

bool
KnobGroup::moveOneStepUp(KnobI* k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock().get() == k) {
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
KnobGroup::moveOneStepDown(KnobI* k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock().get() == k) {
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
        std::vector<KnobIWPtr>::iterator it = _children.begin();
        std::advance(it, index);
        _children.insert(it, k);
    }
    k->setParentKnob( shared_from_this() );
}

std::vector<KnobIPtr>
KnobGroup::getChildren() const
{
    std::vector<KnobIPtr> ret;

    for (std::size_t i = 0; i < _children.size(); ++i) {
        KnobIPtr k = _children[i].lock();
        if (k) {
            ret.push_back(k);
        }
    }

    return ret;
}

/******************************PAGE_KNOB**************************************/

KnobPage::KnobPage(KnobHolder* holder,
                   const std::string &label,
                   int dimension,
                   bool declaredByPlugin)
    : KnobBoolBase(holder, label, dimension, declaredByPlugin)
    , _isToolBar(false)
{
    setIsPersistent(false);
}

KnobPage::KnobPage(KnobHolder* holder,
                   const QString &label,
                   int dimension,
                   bool declaredByPlugin)
    : KnobBoolBase(holder, label.toStdString(), dimension, declaredByPlugin)
    , _isToolBar(false)
{
    setIsPersistent(false);
}

bool
KnobPage::canAnimate() const
{
    return false;
}

const std::string KnobPage::_typeNameStr("Page");
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

std::vector<KnobIPtr>
KnobPage::getChildren() const
{
    std::vector<KnobIPtr> ret;

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
        std::vector<KnobIWPtr>::iterator it = _children.begin();
        std::advance(it, index);
        _children.insert(it, k);
    }
    k->setParentKnob( shared_from_this() );
}

void
KnobPage::removeKnob(KnobI* k)
{
    for (std::vector<KnobIWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->lock().get() == k) {
            _children.erase(it);

            return;
        }
    }
}

bool
KnobPage::moveOneStepUp(KnobI* k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock().get() == k) {
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
KnobPage::moveOneStepDown(KnobI* k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock().get() == k) {
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


KnobParametric::KnobParametric(KnobHolder* holder,
                               const std::string &label,
                               int dimension,
                               bool declaredByPlugin)
    : KnobDoubleBase(holder, label, dimension, declaredByPlugin)
    , _curvesMutex()
    , _curves(dimension)
    , _defaultCurves(dimension)
    , _curvesColor(dimension)
{
    for (int i = 0; i < dimension; ++i) {
        RGBAColourD color;
        color.r = color.g = color.b = color.a = 1.;
        _curvesColor[i] = color;
        _curves[i] = std::make_shared<Curve>(this, i);
        _defaultCurves[i] = std::make_shared<Curve>(this, i);
    }
}

KnobParametric::KnobParametric(KnobHolder* holder,
                               const QString &label,
                               int dimension,
                               bool declaredByPlugin)
    : KnobDoubleBase(holder, label.toStdString(), dimension, declaredByPlugin)
    , _curvesMutex()
    , _curves(dimension)
    , _defaultCurves(dimension)
    , _curvesColor(dimension)
{
    for (int i = 0; i < dimension; ++i) {
        RGBAColourD color;
        color.r = color.g = color.b = color.a = 1.;
        _curvesColor[i] = color;
        _curves[i] = std::make_shared<Curve>(this, i);
        _defaultCurves[i] = std::make_shared<Curve>(this, i);
    }
}

void
KnobParametric::setPeriodic(bool periodic)
{
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        _curves[i]->setPeriodic(periodic);
        _defaultCurves[i]->setPeriodic(periodic);
    }
}


const std::string KnobParametric::_typeNameStr("Parametric");
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

void
KnobParametric::setCurveColor(int dimension,
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
KnobParametric::getCurveColor(int dimension,
                              double* r,
                              double* g,
                              double* b)
{
    ///Mt-safe as it never changes

    assert( dimension < (int)_curvesColor.size() );
    std::pair<int, KnobIPtr>  master = getMaster(dimension);
    if (master.second) {
        KnobParametric* m = dynamic_cast<KnobParametric*>( master.second.get() );
        assert(m);

        return m->getCurveColor(dimension, r, g, b);
    } else {
        *r = _curvesColor[dimension].r;
        *g = _curvesColor[dimension].g;
        *b = _curvesColor[dimension].b;
    }
}

void
KnobParametric::setParametricRange(double min,
                                   double max)
{
    ///only called in the main thread
    assert( QThread::currentThread() == qApp->thread() );
    ///Mt-safe as it never changes

    for (U32 i = 0; i < _curves.size(); ++i) {
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
KnobParametric::getDefaultParametricCurve(int dimension) const
{
    assert( dimension >= 0 && dimension < (int)_curves.size() );
    std::pair<int, KnobIPtr>  master = getMaster(dimension);
    if (master.second) {
        KnobParametric* m = dynamic_cast<KnobParametric*>( master.second.get() );
        assert(m);

        return m->getDefaultParametricCurve(dimension);
    } else {
        return _defaultCurves[dimension];
    }
}

CurvePtr KnobParametric::getParametricCurve(int dimension) const
{
    ///Mt-safe as Curve is MT-safe and the pointer is never deleted

    assert( dimension < (int)_curves.size() );
    std::pair<int, KnobIPtr>  master = getMaster(dimension);
    if (master.second) {
        KnobParametric* m = dynamic_cast<KnobParametric*>( master.second.get() );
        assert(m);

        return m->getParametricCurve(dimension);
    } else {
        return _curves[dimension];
    }
}

StatusEnum
KnobParametric::addControlPoint(ValueChangedReasonEnum reason,
                                int dimension,
                                double key,
                                double value,
                                KeyframeTypeEnum interpolation)
{
    ///Mt-safe as Curve is MT-safe
    if ( ( dimension >= (int)_curves.size() ) ||
         std::isnan(key) || // check for NaN
         std::isinf(key) ||
         std::isnan(value) || // check for NaN
         std::isinf(value) ) {
        return eStatusFailed;
    }

    KeyFrame k(key, value);
    k.setInterpolation(interpolation);
    _curves[dimension]->addKeyFrame(k);
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(0, getCurrentTime(), ViewSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::addControlPoint(ValueChangedReasonEnum reason,
                                int dimension,
                                double key,
                                double value,
                                double leftDerivative,
                                double rightDerivative,
                                KeyframeTypeEnum interpolation)
{
    ///Mt-safe as Curve is MT-safe
    if ( ( dimension >= (int)_curves.size() ) ||
         std::isnan(key) || // check for NaN
         std::isinf(key) ||
         std::isnan(value) || // check for NaN
         std::isinf(value) ) {
        return eStatusFailed;
    }

    KeyFrame k(key, value, leftDerivative, rightDerivative);
    k.setInterpolation(interpolation);
    _curves[dimension]->addKeyFrame(k);
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(0, getCurrentTime(), ViewSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::getValue(int dimension,
                         double parametricPosition,
                         double *returnValue) const
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    try {
        *returnValue = getParametricCurve(dimension)->getValueAt(parametricPosition);
    } catch (...) {
        return eStatusFailed;
    }

    return eStatusOK;
}

StatusEnum
KnobParametric::getNControlPoints(int dimension,
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
KnobParametric::getNthControlPoint(int dimension,
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
KnobParametric::getNthControlPoint(int dimension,
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
                                                int dimension,
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
    evaluateValueChange(0, getCurrentTime(), ViewSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::setNthControlPoint(ValueChangedReasonEnum reason,
                                   int dimension,
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
    evaluateValueChange(0, getCurrentTime(), ViewSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::setNthControlPoint(ValueChangedReasonEnum reason,
                                   int dimension,
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
    evaluateValueChange(0, getCurrentTime(), ViewSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::deleteControlPoint(ValueChangedReasonEnum reason,
                                   int dimension,
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
    evaluateValueChange(0, getCurrentTime(), ViewSpec::all(), reason);

    return eStatusOK;
}

StatusEnum
KnobParametric::deleteAllControlPoints(ValueChangedReasonEnum reason,
                                       int dimension)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    _curves[dimension]->clearKeyFrames();
    Q_EMIT curveChanged(dimension);
    evaluateValueChange(0, getCurrentTime(), ViewSpec::all(), reason);

    return eStatusOK;
}

void
KnobParametric::cloneExtraData(KnobI* other,
                               int dimension,
                               int otherDimension)
{
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(other);

    if (!isParametric) {
        return;
    }
    if (dimension == -1) {
        int dimMin = std::min( getDimension(), isParametric->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            _curves[i]->clone(*isParametric->_curves[i]);
        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        assert( dimension >= 0 && dimension < getDimension() && otherDimension >= 0 && otherDimension < other->getDimension() );
        _curves[dimension]->clone(*isParametric->_curves[otherDimension]);
    }
}

bool
KnobParametric::cloneExtraDataAndCheckIfChanged(KnobI* other,
                                                int dimension,
                                                int otherDimension)
{
    ///Mt-safe as Curve is MT-safe
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(other);

    if (!isParametric) {
        return false;
    }
    bool hasChanged = false;
    if (dimension == -1) {
        int dimMin = std::min( getDimension(), isParametric->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            hasChanged |= _curves[i]->cloneAndCheckIfChanged(*isParametric->_curves[i]);
        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        assert( dimension >= 0 && dimension < getDimension() && otherDimension >= 0 && otherDimension < other->getDimension() );
        hasChanged |= _curves[dimension]->cloneAndCheckIfChanged(*isParametric->_curves[otherDimension]);
    }

    return hasChanged;
}

void
KnobParametric::cloneExtraData(KnobI* other,
                               double offset,
                               const RangeD* range,
                               int dimension,
                               int otherDimension)
{
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(other);

    if (!isParametric) {
        return;
    }
    if (dimension == -1) {
        int dimMin = std::min( getDimension(), isParametric->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            _curves[i]->clone(*isParametric->_curves[i], offset, range);
        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        assert( dimension >= 0 && dimension < getDimension() && otherDimension >= 0 && otherDimension < other->getDimension() );
        _curves[dimension]->clone(*isParametric->_curves[otherDimension], offset, range);
    }
}

void
KnobParametric::saveParametricCurves(std::list<Curve >* curves) const
{
    for (U32 i = 0; i < _curves.size(); ++i) {
        curves->push_back(*_curves[i]);
    }
}

void
KnobParametric::loadParametricCurves(const std::list<Curve > & curves)
{
    assert( !_curves.empty() );
    int i = 0;
    for (std::list<Curve >::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        _curves[i]->clone(*it);
        ++i;
    }
}

void
KnobParametric::resetExtraToDefaultValue(int dimension)
{
    StatusEnum s = deleteAllControlPoints(eValueChangedReasonNatronInternalEdited, dimension);

    Q_UNUSED(s);
    _curves[dimension]->clone(*_defaultCurves[dimension]);
    Q_EMIT curveChanged(dimension);
}

void
KnobParametric::setDefaultCurvesFromCurves()
{
    assert( _curves.size() == _defaultCurves.size() );
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        _defaultCurves[i]->clone(*_curves[i]);
    }
}

bool
KnobParametric::hasModificationsVirtual(int dimension) const
{
    assert( dimension >= 0 && dimension < (int)_curves.size() );
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
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>( slave.get() );

    if (isParametric) {
        _defaultCurves.resize( isParametric->_defaultCurves.size() );
        _curvesColor.resize( isParametric->_curvesColor.size() );
        assert( _curvesColor.size() == _defaultCurves.size() );
        for (std::size_t i = 0; i < isParametric->_defaultCurves.size(); ++i) {
            _defaultCurves[i] = std::make_shared<Curve>(this, i);
            _defaultCurves[i]->clone(*isParametric->_defaultCurves[i]);
            _curvesColor[i] = isParametric->_curvesColor[i];
        }
    }
}

/******************************KnobTable**************************************/


KnobTable::KnobTable(KnobHolder* holder,
                     const std::string &label,
                     int dimension,
                     bool declaredByPlugin)
    : KnobStringBase(holder, label, dimension, declaredByPlugin)
{
}

KnobTable::KnobTable(KnobHolder* holder,
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
                KnobHolder* holder = dynamic_cast<KnobHolder*>(getHolder());
                QString knobName;
                if (holder) {
                    EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
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
            KnobHolder* holder = dynamic_cast<KnobHolder*>(getHolder());
            QString knobName;
            if (holder) {
                EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
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
            std::string value = Project::escapeXML( (*it)[c] );
            ss << value;
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
    setValue( encodeToKnobTableFormat(table) );
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

const std::string KnobLayers::_typeNameStr("Layers");
const std::string&
KnobLayers::typeNameStatic()
{
    return _typeNameStr;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobTypes.cpp"
