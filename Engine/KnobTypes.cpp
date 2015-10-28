/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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
#include <stdexcept>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include <QDebug>
#include <QThread>
#include <QCoreApplication>

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
#include "Engine/RotoContext.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"

using namespace Natron;
using std::make_pair;
using std::pair;

/******************************KnobInt**************************************/
KnobInt::KnobInt(KnobHolder* holder,
                   const std::string &description,
                   int dimension,
                   bool declaredByPlugin)
: Knob<int>(holder, description, dimension,declaredByPlugin)
, _increments(dimension)
, _disableSlider(false)
{
    for (int i = 0; i < dimension; ++i) {
        _increments[i] = 1;
    }
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
                     const std::string &description,
                     int dimension,
                     bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
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
                         const std::string &description,
                         int dimension,
                         bool declaredByPlugin)
: Knob<double>(holder, description, dimension,declaredByPlugin)
, _spatial(false)
, _increments(dimension)
, _decimals(dimension)
, _disableSlider(false)
, _valueIsNormalized(dimension)
, _defaultValuesAreNormalized(false)
, _hasHostOverlayHandle(false)
{
    for (int i = 0; i < dimension; ++i) {
        _increments[i] = 1.;
        _decimals[i] = 2;
        _valueIsNormalized[i] = eValueIsNormalizedNone;
    }
}

void
KnobDouble::setHasHostOverlayHandle(bool handle)
{
    KnobHolder* holder = getHolder();
    if (holder) {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
        if (!effect) {
            return;
        }
        if (!effect->getNode()) {
            return;
        }
        boost::shared_ptr<KnobI> thisShared = holder->getKnobByName(getName());
        assert(thisShared);
        boost::shared_ptr<KnobDouble> thisSharedDouble = boost::dynamic_pointer_cast<KnobDouble>(thisShared);
        assert(thisSharedDouble);
        if (handle) {
            effect->getNode()->addDefaultPositionOverlay(thisSharedDouble);
        } else {
            effect->getNode()->removeHostOverlay(this);
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

void
KnobDouble::onNodeDeactivated()
{
    ///unslave all roto control points that would be slaved
    for (std::list< boost::shared_ptr<BezierCP> >::iterator it = _slavedTracks.begin(); it != _slavedTracks.end(); ++it) {
        (*it)->unslave();
    }
}

void
KnobDouble::onNodeActivated()
{
    ///get a shared_ptr to this
    assert( getHolder() );
    boost::shared_ptr<KnobDouble> thisShared;
    const std::vector<boost::shared_ptr<KnobI> > & knobs = getHolder()->getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i].get() == this) {
            thisShared = boost::dynamic_pointer_cast<KnobDouble>(knobs[i]);
            break;
        }
    }
    assert(thisShared);
    SequenceTime time = getHolder()->getApp()->getTimeLine()->currentFrame();
    ///reslave all tracks that where slaved
    for (std::list< boost::shared_ptr<BezierCP> >::iterator it = _slavedTracks.begin(); it != _slavedTracks.end(); ++it) {
        (*it)->slaveTo(time,thisShared);
    }
}

void
KnobDouble::removeSlavedTrack(const boost::shared_ptr<BezierCP> & cp)
{
    std::list< boost::shared_ptr<BezierCP> >::iterator found = std::find(_slavedTracks.begin(),_slavedTracks.end(),cp);
    
    if ( found != _slavedTracks.end() ) {
        _slavedTracks.erase(found);
    }
}

void
KnobDouble::serializeTracks(std::list<SerializedTrack>* tracks)
{
    for (std::list< boost::shared_ptr<BezierCP> >::iterator it = _slavedTracks.begin(); it != _slavedTracks.end(); ++it) {
        SerializedTrack s;
        s.bezierName = (*it)->getBezier()->getScriptName();
        s.isFeather = (*it)->isFeatherPoint();
        s.cpIndex = !s.isFeather ? (*it)->getBezier()->getControlPointIndex(*it) : (*it)->getBezier()->getFeatherPointIndex(*it);
        s.rotoNodeName = (*it)->getBezier()->getRotoNodeName();
        s.offsetTime = (*it)->getOffsetTime();
        tracks->push_back(s);
    }
}

void
KnobDouble::restoreTracks(const std::list <SerializedTrack> & tracks,
                           const std::list<boost::shared_ptr<Node> > & activeNodes)
{
    ///get a shared_ptr to this
    assert( getHolder() );
    boost::shared_ptr<KnobDouble> thisShared = boost::dynamic_pointer_cast<KnobDouble>(shared_from_this());
    assert(thisShared);
    
    std::string lastNodeName;
    RotoContext* lastRoto = 0;
    for (std::list< SerializedTrack >::const_iterator it = tracks.begin(); it != tracks.end(); ++it) {
        RotoContext* roto = 0;
        ///speed-up by remembering the last one
        std::string scriptFriendlyRoto = Natron::makeNameScriptFriendly(it->rotoNodeName);
        if (it->rotoNodeName == lastNodeName || scriptFriendlyRoto == lastNodeName) {
            roto = lastRoto;
        } else {
            for (std::list<boost::shared_ptr<Node> >::const_iterator it2 = activeNodes.begin(); it2 != activeNodes.end(); ++it2) {
                if ((*it2)->getScriptName() == it->rotoNodeName || (*it2)->getScriptName() == scriptFriendlyRoto) {
                    lastNodeName = (*it2)->getScriptName();
                    boost::shared_ptr<RotoContext> rotoCtx = (*it2)->getRotoContext();
                    assert(rotoCtx);
                    lastRoto = rotoCtx.get();
                    roto = rotoCtx.get();
                    break;
                }
            }
        }
        if (roto) {
            
            boost::shared_ptr<RotoItem> item = roto->getItemByName(it->bezierName);
            if (!item) {
                std::string scriptFriendlyBezier = Natron::makeNameScriptFriendly(it->bezierName);
                item = roto->getItemByName(scriptFriendlyBezier);
                if (!item) {
                    qDebug() << "Failed to restore slaved track " << it->bezierName.c_str();
                    break;
                }
            }
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(item);
            assert(isBezier);
            
            boost::shared_ptr<BezierCP> point = (it->isFeather ?
                                                 isBezier->getFeatherPointAtIndex(it->cpIndex)
                                                 : isBezier->getControlPointAtIndex(it->cpIndex));
            
            if (!point) {
                qDebug() << "Failed to restore slaved track " << it->bezierName.c_str();
                break;
            }
            point->slaveTo(it->offsetTime,thisShared);
            _slavedTracks.push_back(point);
        }
    }
} // restoreTracks

KnobDouble::~KnobDouble()
{
    for (std::list< boost::shared_ptr<BezierCP> >::iterator it = _slavedTracks.begin(); it != _slavedTracks.end(); ++it) {
        (*it)->unslave();
    }
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
    Status stat = effect->getRegionOfDefinition_public(effect->getHash(),time, scale, /*view*/0, &rod, &isProjectFormat);
    if ( (stat == StatFailed) || ( (rod.x1 == 0) && (rod.y1 == 0) && (rod.x2 == 1) && (rod.y2 == 1) ) ) {
        Format f;
        effect->getRenderFormat(&f);
        rod = f;
    }
#else
    Format f;
    effect->getRenderFormat(&f);
    rod = f.toCanonicalFormat();
#endif
}


void
KnobDouble::denormalize(int dimension,
                        double time,
                        double* value) const
{
    EffectInstance* effect = dynamic_cast<EffectInstance*>( getHolder() );
    
    assert(effect);
    if (!effect) {
        // coverity[dead_error_line]
        return;
    }
    RectD rod;
    getInputRoD(effect,time,rod);
    ValueIsNormalizedEnum e = getValueIsNormalized(dimension);
    // the second expression (with e == eValueIsNormalizedNone) is used when denormalizing default values
    if (e == eValueIsNormalizedX || (e == eValueIsNormalizedNone && dimension == 0)) {
        *value *= rod.width();
    } else if (e == eValueIsNormalizedY || (e == eValueIsNormalizedNone && dimension == 1)) {
        *value *= rod.height();
    }
}

void
KnobDouble::normalize(int dimension,
                       double time,
                       double* value) const
{
    EffectInstance* effect = dynamic_cast<EffectInstance*>( getHolder() );
    
    assert(effect);
    if (!effect) {
        // coverity[dead_error_line]
        return;
    }
    RectD rod;
    getInputRoD(effect,time,rod);
    ValueIsNormalizedEnum e = getValueIsNormalized(dimension);
    // the second expression (with e == eValueIsNormalizedNone) is used when normalizing default values
    if (e == eValueIsNormalizedX || (e == eValueIsNormalizedNone && dimension == 0)) {
        *value /= rod.width();
    } else if (e == eValueIsNormalizedY || (e == eValueIsNormalizedNone && dimension == 1)) {
        *value /= rod.height();
    }
}

/******************************KnobButton**************************************/

KnobButton::KnobButton(KnobHolder*  holder,
                         const std::string &description,
                         int dimension,
                         bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
, _renderButton(false)
{
    setIsPersistant(false);
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

void
KnobButton::trigger()
{
    evaluateValueChange(0, getCurrentTime(),  Natron::eValueChangedReasonUserEdited);
}

/******************************KnobChoice**************************************/

#define KNOBCHOICE_MAX_ENTRIES_HELP 40 // don't show help in the tootlip if there are more entries that this

KnobChoice::KnobChoice(KnobHolder* holder,
                         const std::string &description,
                         int dimension,
                         bool declaredByPlugin)
: Knob<int>(holder, description, dimension,declaredByPlugin)
, _entriesMutex()
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

/*Must be called right away after the constructor.*/
void
KnobChoice::populateChoices(const std::vector<std::string> &entries,
                             const std::vector<std::string> &entriesHelp)
{
    assert( entriesHelp.empty() || entriesHelp.size() == entries.size() );
    std::vector<std::string> curEntries;
    {
        QMutexLocker l(&_entriesMutex);
        curEntries = _entries;
        _entriesHelp = entriesHelp;
        _entries = entries;
    }
    int cur_i = getValue();
    std::string curEntry;
    if (cur_i >= 0 && cur_i < (int)curEntries.size()) {
        curEntry = curEntries[cur_i];
    }
    if (!curEntry.empty()) {
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (entries[i] == curEntry && cur_i != (int)i) {
                blockValueChanges();
                setValue((int)i, 0);
                unblockValueChanges();
                break;
            }
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_helpChanged();
    }
    Q_EMIT populated();
}

void
KnobChoice::resetChoices()
{
    {
        QMutexLocker l(&_entriesMutex);
        _entries.clear();
        _entriesHelp.clear();
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_helpChanged();
    }
    Q_EMIT entriesReset();
}

void
KnobChoice::appendChoice(const std::string& entry, const std::string& help)
{
    {
        QMutexLocker l(&_entriesMutex);
        if (!_entriesHelp.empty()) {
            _entriesHelp.push_back(help);
        }
        _entries.push_back(entry);
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_helpChanged();
    }
    Q_EMIT entryAppended(QString(entry.c_str()), QString(help.c_str()));
}

std::vector<std::string>
KnobChoice::getEntries_mt_safe() const
{
    QMutexLocker l(&_entriesMutex);
    return _entries;
}

const std::string&
KnobChoice::getEntry(int v) const
{
    if (v < 0 || (int)_entries.size() <= v) {
        throw std::runtime_error(std::string("KnobChoice::getEntry: index out of range"));
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
KnobChoice::getEntriesHelp_mt_safe() const
{
    QMutexLocker l(&_entriesMutex);
    return _entriesHelp;
}

std::string
KnobChoice::getActiveEntryText_mt_safe() const
{
    int activeIndex = getValue();
    
    QMutexLocker l(&_entriesMutex);
    assert( activeIndex < (int)_entries.size() );
    
    return _entries[activeIndex];
}



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

static void
whitespacify(std::string & str)
{
    std::replace( str.begin(), str.end(), '\t', ' ');
    std::replace( str.begin(), str.end(), '\f', ' ');
    std::replace( str.begin(), str.end(), '\v', ' ');
    std::replace( str.begin(), str.end(), '\n', ' ');
    std::replace( str.begin(), str.end(), '\r', ' ');
}

std::string
KnobChoice::getHintToolTipFull() const
{
    assert(QThread::currentThread() == qApp->thread());
    
    int gothelp = 0;
    
    if ( !_entriesHelp.empty() ) {
        assert( _entriesHelp.size() == _entries.size() );
        for (U32 i = 0; i < _entries.size(); ++i) {
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
        ss << trim( getHintToolTip() );
        if (gothelp) {
            // if there are per-option help strings, separate them from main hint
            ss << "\n\n";
        }
    }
    // param may have no hint but still have per-option help
    if (gothelp) {
        for (U32 i = 0; i < _entriesHelp.size(); ++i) {
            if ( !_entriesHelp[i].empty() ) { // no help line is needed if help is unavailable for this option
                std::string entry = trim(_entries[i]);
                whitespacify(entry);
                std::string help = trim(_entriesHelp[i]);
                whitespacify(help);
                ss << entry;
                ss << ": ";
                ss << help;
                if (i < _entriesHelp.size() - 1) {
                    ss << '\n';
                }
            }
        }
    }
    
    return ss.str();
}

static bool caseInsensitiveCompare(const std::string& a,const std::string& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    std::locale loc;
    for (std::size_t i = 0; i < a.size(); ++i) {
        char aLow = std::tolower(a[i],loc);
        char bLow = std::tolower(b[i],loc);
        if (aLow != bLow) {
            return false;
        }
    }
    return true;
}

KnobHelper::ValueChangedReturnCodeEnum
KnobChoice::setValueFromLabel(const std::string & value,
                               int dimension,
                               bool turnOffAutoKeying)
{
    for (std::size_t i = 0; i < _entries.size(); ++i) {
        if (caseInsensitiveCompare(_entries[i], value)) {
            return setValue(i, dimension, turnOffAutoKeying);
        }
    }
    return KnobHelper::eValueChangedReturnCodeNothingChanged;
    //throw std::runtime_error(std::string("KnobChoice::setValueFromLabel: unknown label ") + value);
}

void
KnobChoice::setDefaultValueFromLabel(const std::string & value,
                                      int dimension)
{
    for (std::size_t i = 0; i < _entries.size(); ++i) {
        if (caseInsensitiveCompare(_entries[i], value)) {
            return setDefaultValue(i, dimension);
        }
    }
    //throw std::runtime_error(std::string("KnobChoice::setDefaultValueFromLabel: unknown label ") + value);
}

void
KnobChoice::choiceRestoration(KnobChoice* knob,const ChoiceExtraData* data)
{
    assert(knob && data);
    
    
    ///Clone first and then handle restoration of the static value
    clone(knob);
    setSecret( knob->getIsSecret() );
    if ( getDimension() == knob->getDimension() ) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            setEnabled( i, knob->isEnabled(i) );
        }
    }
    
    int serializedIndex = knob->getValue();
    if ( ( serializedIndex < (int)_entries.size() ) && (_entries[serializedIndex] == data->_choiceString) ) {
        // we're lucky, entry hasn't changed
        setValue(serializedIndex, 0);
    } else {
        // try to find the same label at some other index
        for (std::size_t i = 0; i < _entries.size(); ++i) {
            if (caseInsensitiveCompare(_entries[i], data->_choiceString)) {
                setValue(i, 0);
                return;
            }
        }
    }
    
}
/******************************KnobSeparator**************************************/

KnobSeparator::KnobSeparator(KnobHolder* holder,
                               const std::string &description,
                               int dimension,
                               bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
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
                       const std::string &description,
                       int dimension,
                       bool declaredByPlugin)
: Knob<double>(holder, description, dimension,declaredByPlugin)
, _allDimensionsEnabled(true)
, _simplifiedMode(false)
{
    //dimension greater than 4 is not supported. Dimension 2 doesn't make sense.
    assert(dimension <= 4 && dimension != 2);
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
                         const std::string &description,
                         int dimension,
                         bool declaredByPlugin)
: AnimatingKnobStringHelper(holder, description, dimension,declaredByPlugin)
, _multiLine(false)
, _richText(false)
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
KnobString::hasContentWithoutHtmlTags() const
{
    std::string str = getValue();
    if (str.empty()) {
        return false;
    }
    
    std::size_t foundOpen = str.find("<");
    if (foundOpen == std::string::npos) {
        return true;
    }
    while (foundOpen != std::string::npos) {
        std::size_t foundClose = str.find(">",foundOpen);
        if (foundClose == std::string::npos) {
            return true;
        }
        
        if (foundClose + 1 < str.size()) {
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

/******************************KnobGroup**************************************/

KnobGroup::KnobGroup(KnobHolder* holder,
                       const std::string &description,
                       int dimension,
                       bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
, _isTab(false)
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
KnobGroup::addKnob(boost::shared_ptr<KnobI> k)
{
    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }
    
    
    boost::shared_ptr<KnobI> parent= k->getParentKnob();
    if (parent) {
        KnobGroup* isParentGrp = dynamic_cast<KnobGroup*>(parent.get());
        KnobPage* isParentPage = dynamic_cast<KnobPage*>(parent.get());
        if (isParentGrp) {
            isParentGrp->removeKnob(k.get());
        } else if (isParentPage) {
            isParentPage->removeKnob(k.get());
        }
        k->setParentKnob(boost::shared_ptr<KnobI>());
    }
    
    
    _children.push_back(k);
    k->setParentKnob(shared_from_this());
    
}

void
KnobGroup::removeKnob(KnobI* k)
{
    for (std::vector<boost::weak_ptr<KnobI> >::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->lock().get() == k) {
            _children.erase(it);
            return;
        }
    }
}

void
KnobGroup::moveOneStepUp(KnobI* k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock().get() == k) {
            if (i == 0) {
                break;
            }
            boost::weak_ptr<KnobI> tmp = _children[i - 1];
            _children[i - 1] = _children[i];
            _children[i] = tmp;
            break;
        }
    }
}

void
KnobGroup::moveOneStepDown(KnobI* k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock().get() == k) {
            if (i == _children.size() - 1) {
                break;
            }
            boost::weak_ptr<KnobI> tmp = _children[i + 1];
            _children[i + 1] = _children[i];
            _children[i] = tmp;
            break;
        }
    }
}

void
KnobGroup::insertKnob(int index, const boost::shared_ptr<KnobI>& k)
{
    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }
    
    boost::shared_ptr<KnobI> parent= k->getParentKnob();
    if (parent) {
        KnobGroup* isParentGrp = dynamic_cast<KnobGroup*>(parent.get());
        KnobPage* isParentPage = dynamic_cast<KnobPage*>(parent.get());
        if (isParentGrp) {
            isParentGrp->removeKnob(k.get());
        } else if (isParentPage) {
            isParentPage->removeKnob(k.get());
        }
        k->setParentKnob(boost::shared_ptr<KnobI>());
    }

    if (index >= (int)_children.size()) {
        _children.push_back(k);
    } else {
        std::vector<boost::weak_ptr<KnobI> >::iterator it = _children.begin();
        std::advance(it, index);
        _children.insert(it, k);
    }
    k->setParentKnob(shared_from_this());
}

std::vector< boost::shared_ptr<KnobI> >
KnobGroup::getChildren() const
{
    std::vector< boost::shared_ptr<KnobI> > ret;
    for (std::size_t i = 0; i < _children.size(); ++i) {
        boost::shared_ptr<KnobI> k = _children[i].lock();
        if (k) {
            ret.push_back(k);
        }
    }
    return ret;
}

/******************************PAGE_KNOB**************************************/

KnobPage::KnobPage(KnobHolder* holder,
                     const std::string &description,
                     int dimension,
                     bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
{
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


std::vector< boost::shared_ptr<KnobI> >
KnobPage::getChildren() const
{
    std::vector< boost::shared_ptr<KnobI> > ret;
    for (std::size_t i = 0; i < _children.size(); ++i) {
        boost::shared_ptr<KnobI> k = _children[i].lock();
        if (k) {
            ret.push_back(k);
        }
    }
    return ret;
}

void
KnobPage::addKnob(const boost::shared_ptr<KnobI> &k)
{
    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }
    
    
    boost::shared_ptr<KnobI> parent= k->getParentKnob();
    if (parent) {
        KnobGroup* isParentGrp = dynamic_cast<KnobGroup*>(parent.get());
        KnobPage* isParentPage = dynamic_cast<KnobPage*>(parent.get());
        if (isParentGrp) {
            isParentGrp->removeKnob(k.get());
        } else if (isParentPage) {
            isParentPage->removeKnob(k.get());
        }
        k->setParentKnob(boost::shared_ptr<KnobI>());
    }
    _children.push_back(k);
    k->setParentKnob(shared_from_this());
    
}

void
KnobPage::insertKnob(int index, const boost::shared_ptr<KnobI>& k)
{
    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }
    
    boost::shared_ptr<KnobI> parent= k->getParentKnob();
    if (parent) {
        KnobGroup* isParentGrp = dynamic_cast<KnobGroup*>(parent.get());
        KnobPage* isParentPage = dynamic_cast<KnobPage*>(parent.get());
        if (isParentGrp) {
            isParentGrp->removeKnob(k.get());
        } else if (isParentPage) {
            isParentPage->removeKnob(k.get());
        }
        k->setParentKnob(boost::shared_ptr<KnobI>());
    }
    
    
    if (index >= (int)_children.size()) {
        _children.push_back(k);
    } else {
        std::vector<boost::weak_ptr<KnobI> >::iterator it = _children.begin();
        std::advance(it, index);
        _children.insert(it, k);
    }
    k->setParentKnob(shared_from_this());
    
}

void
KnobPage::removeKnob(KnobI* k)
{
    for (std::vector<boost::weak_ptr<KnobI> >::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->lock().get() == k) {
            _children.erase(it);
            return;
        }
    }
}

void
KnobPage::moveOneStepUp(KnobI* k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock().get() == k) {
            if (i == 0) {
                break;
            }
            boost::weak_ptr<KnobI> tmp = _children[i - 1];
            _children[i - 1] = _children[i];
            _children[i] = tmp;
            break;
        }
    }
}

void
KnobPage::moveOneStepDown(KnobI* k)
{
    for (U32 i = 0; i < _children.size(); ++i) {
        if (_children[i].lock().get() == k) {
            if (i == _children.size() - 1) {
                break;
            }
            boost::weak_ptr<KnobI> tmp = _children[i + 1];
            _children[i + 1] = _children[i];
            _children[i] = tmp;
            break;
        }
    }
}


/******************************KnobParametric**************************************/


KnobParametric::KnobParametric(KnobHolder* holder,
                                 const std::string &description,
                                 int dimension,
                                 bool declaredByPlugin)
: Knob<double>(holder,description,dimension,declaredByPlugin)
, _curvesMutex()
, _curves(dimension)
, _defaultCurves(dimension)
, _curvesColor(dimension)
{
    for (int i = 0; i < dimension; ++i) {
        RGBAColourF color;
        color.r = color.g = color.b = color.a = 1.;
        _curvesColor[i] = color;
        _curves[i] = boost::shared_ptr<Curve>( new Curve(this,i) );
        _defaultCurves[i] = boost::shared_ptr<Curve>( new Curve(this,i) );
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
    
    for (U32 i = 0; i < _curves.size(); ++i) {
        _curves[i]->setXRange(min, max);
    }
}

std::pair<double,double> KnobParametric::getParametricRange() const
{
    ///Mt-safe as it never changes
    assert( !_curves.empty() );
    
    return _curves.front()->getXRange();
}

boost::shared_ptr<Curve>
KnobParametric::getDefaultParametricCurve(int dimension) const
{
    assert(dimension >= 0 && dimension < (int)_curves.size());
    std::pair<int,boost::shared_ptr<KnobI> >  master = getMaster(dimension);
    if (master.second) {
        KnobParametric* m = dynamic_cast<KnobParametric*>(master.second.get());
        assert(m);
        return m->getDefaultParametricCurve(dimension);
    } else {
        return _defaultCurves[dimension];
    }

}

boost::shared_ptr<Curve> KnobParametric::getParametricCurve(int dimension) const
{
    ///Mt-safe as Curve is MT-safe and the pointer is never deleted
    
    assert( dimension < (int)_curves.size() );
    std::pair<int,boost::shared_ptr<KnobI> >  master = getMaster(dimension);
    if (master.second) {
        KnobParametric* m = dynamic_cast<KnobParametric*>(master.second.get());
        assert(m);
        return m->getParametricCurve(dimension);
    } else {
        return _curves[dimension];
    }
}

Natron::StatusEnum
KnobParametric::addControlPoint(int dimension,
                                 double key,
                                 double value)
{
    ///Mt-safe as Curve is MT-safe
    if (dimension >= (int)_curves.size() ||
        key != key || // check for NaN
        boost::math::isinf(key) ||
        value != value || // check for NaN
        boost::math::isinf(value)) {
        return eStatusFailed;
    }
    
    KeyFrame k(key,value);
    k.setInterpolation(Natron::eKeyframeTypeCubic);
    _curves[dimension]->addKeyFrame(k);
    Q_EMIT curveChanged(dimension);
    
    return eStatusOK;
}

Natron::StatusEnum
KnobParametric::addHorizontalControlPoint(int dimension,double key,double value)
{
    ///Mt-safe as Curve is MT-safe
    if (dimension >= (int)_curves.size() ||
        key != key || // check for NaN
        boost::math::isinf(key) ||
        value != value || // check for NaN
        boost::math::isinf(value)) {
        return eStatusFailed;
    }
    
    KeyFrame k(key,value);
    k.setInterpolation(Natron::eKeyframeTypeBroken);
    k.setLeftDerivative(0);
    k.setRightDerivative(0);
    _curves[dimension]->addKeyFrame(k);
    Q_EMIT curveChanged(dimension);
    
    return eStatusOK;
 
}

Natron::StatusEnum
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
    }catch (...) {
        return Natron::eStatusFailed;
    }
    
    return Natron::eStatusOK;
}

Natron::StatusEnum
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

Natron::StatusEnum
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

Natron::StatusEnum
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

Natron::StatusEnum
KnobParametric::setNthControlPoint(int dimension,
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
    
    return eStatusOK;
}

Natron::StatusEnum
KnobParametric::setNthControlPoint(int dimension,
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
        _curves[dimension]->setKeyFrameValueAndTime(key, value, nthCtl,&newIdx);
    } catch (...) {
        return eStatusFailed;
    }
    _curves[dimension]->setKeyFrameDerivatives(leftDerivative, rightDerivative, newIdx);
    Q_EMIT curveChanged(dimension);
    
    return eStatusOK;

}

Natron::StatusEnum
KnobParametric::deleteControlPoint(int dimension,
                                    int nthCtl)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    
    _curves[dimension]->removeKeyFrameWithIndex(nthCtl);
    Q_EMIT curveChanged(dimension);
    
    return eStatusOK;
}

Natron::StatusEnum
KnobParametric::deleteAllControlPoints(int dimension)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return eStatusFailed;
    }
    _curves[dimension]->clearKeyFrames();
    Q_EMIT curveChanged(dimension);
    
    return eStatusOK;
}

void
KnobParametric::cloneExtraData(KnobI* other,int dimension )
{
    ///Mt-safe as Curve is MT-safe
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(other);
    
    if ( isParametric && ( isParametric->getDimension() == getDimension() ) ) {
        for (int i = 0; i < getDimension(); ++i) {
            if (i == dimension || dimension == -1) {
                _curves[i]->clone( *isParametric->getParametricCurve(i) );
            }
        }
    }
}

bool
KnobParametric::cloneExtraDataAndCheckIfChanged(KnobI* other,int dimension) {
    bool hasChanged = false;
    ///Mt-safe as Curve is MT-safe
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(other);
    
    if ( isParametric && ( isParametric->getDimension() == getDimension() ) ) {
        for (int i = 0; i < getDimension(); ++i) {
            if (i == dimension || dimension == -1) {
                hasChanged |= _curves[i]->cloneAndCheckIfChanged( *isParametric->getParametricCurve(i) );
            }
        }
    }
    return hasChanged;
}

void
KnobParametric::cloneExtraData(KnobI* other,
                                SequenceTime offset,
                                const RangeD* range,
                                int dimension)
{
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(other);
    
    if (isParametric) {
        int dimMin = std::min( getDimension(), isParametric->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            if (i == dimension || dimension == -1) {
                _curves[i]->clone(*isParametric->getParametricCurve(i), offset, range);
            }
        }
    }
}

void
KnobParametric::saveParametricCurves(std::list< Curve >* curves) const
{
    for (U32 i = 0; i < _curves.size(); ++i) {
        curves->push_back(*_curves[i]);
    }
}

void
KnobParametric::loadParametricCurves(const std::list< Curve > & curves)
{
    assert( !_curves.empty() );
    int i = 0;
    for (std::list< Curve >::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        _curves[i]->clone(*it);
        ++i;
    }
}

void
KnobParametric::resetExtraToDefaultValue(int dimension)
{
    Natron::StatusEnum s = deleteAllControlPoints(dimension);
    Q_UNUSED(s);
    _curves[dimension]->clone(*_defaultCurves[dimension]);
    Q_EMIT curveChanged(dimension);

}

void
KnobParametric::setDefaultCurvesFromCurves()
{
    assert(_curves.size() == _defaultCurves.size());
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        _defaultCurves[i]->clone(*_curves[i]);
    }
}

bool
KnobParametric::hasModificationsVirtual(int dimension) const
{
    assert(dimension >= 0 && dimension < (int)_curves.size());
    KeyFrameSet defKeys = _defaultCurves[dimension]->getKeyFrames_mt_safe();
    KeyFrameSet keys = _curves[dimension]->getKeyFrames_mt_safe();
    if (defKeys.size() != keys.size()) {
        return true;
    }
    KeyFrameSet::iterator itO = defKeys.begin();
    for (KeyFrameSet::iterator it = keys.begin(); it!=keys.end(); ++it,++itO) {
        if (*it != *itO) {
            return true;
        }
    }
    return false;
}
