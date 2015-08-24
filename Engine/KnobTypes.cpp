//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/Transform.h"
#include "Engine/AppInstance.h"
#include "Engine/RotoContext.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Format.h"
using namespace Natron;
using std::make_pair;
using std::pair;

/******************************INT_KNOB**************************************/
Int_Knob::Int_Knob(KnobHolder* holder,
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
Int_Knob::disableSlider()
{
    _disableSlider = true;
}

bool
Int_Knob::isSliderDisabled() const
{
    return _disableSlider;
}


void
Int_Knob::setIncrement(int incr,
                       int index)
{
    if (incr <= 0) {
        qDebug() << "Attempting to set the increment of an int param to a value lesser or equal to 0";
        
        return;
    }
    
    if ( index >= (int)_increments.size() ) {
        throw std::runtime_error("Int_Knob::setIncrement , dimension out of range");
    }
    _increments[index] = incr;
    Q_EMIT incrementChanged(_increments[index], index);
}

void
Int_Knob::setIncrement(const std::vector<int> &incr)
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
Int_Knob::getIncrements() const
{
    return _increments;
}

bool
Int_Knob::canAnimate() const
{
    return true;
}

const std::string Int_Knob::_typeNameStr("Int");
const std::string &
Int_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
Int_Knob::typeName() const
{
    return typeNameStatic();
}

/******************************BOOL_KNOB**************************************/

Bool_Knob::Bool_Knob(KnobHolder* holder,
                     const std::string &description,
                     int dimension,
                     bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
{
}

bool
Bool_Knob::canAnimate() const
{
    return canAnimateStatic();
}

const std::string Bool_Knob::_typeNameStr("Bool");
const std::string &
Bool_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
Bool_Knob::typeName() const
{
    return typeNameStatic();
}

/******************************DOUBLE_KNOB**************************************/


Double_Knob::Double_Knob(KnobHolder* holder,
                         const std::string &description,
                         int dimension,
                         bool declaredByPlugin)
: Knob<double>(holder, description, dimension,declaredByPlugin)
, _spatial(false)
, _increments(dimension)
, _decimals(dimension)
, _disableSlider(false)
, _normalizationXY()
, _defaultStoredNormalized(false)
, _hasNativeOverlayHandle(false)
{
    _normalizationXY.first = eNormalizedStateNone;
    _normalizationXY.second = eNormalizedStateNone;
    
    for (int i = 0; i < dimension; ++i) {
        _increments[i] = 1.;
        _decimals[i] = 2;
    }
}

void
Double_Knob::setHasNativeOverlayHandle(bool handle)
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
        boost::shared_ptr<Double_Knob> thisSharedDouble = boost::dynamic_pointer_cast<Double_Knob>(thisShared);
        assert(thisSharedDouble);
        if (handle) {
            effect->getNode()->addDefaultPositionOverlay(thisSharedDouble);
        } else {
            effect->getNode()->removeDefaultOverlay(this);
        }
       _hasNativeOverlayHandle = handle;
    }
    
}

bool
Double_Knob::getHasNativeOverlayHandle() const
{
    return _hasNativeOverlayHandle;
}

void
Double_Knob::disableSlider()
{
    _disableSlider = true;
}

bool
Double_Knob::isSliderDisabled() const
{
    return _disableSlider;
}

bool
Double_Knob::canAnimate() const
{
    return true;
}

const std::string Double_Knob::_typeNameStr("Double");
const std::string &
Double_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
Double_Knob::typeName() const
{
    return typeNameStatic();
}

const std::vector<double> &
Double_Knob::getIncrements() const
{
    return _increments;
}

const std::vector<int> &
Double_Knob::getDecimals() const
{
    return _decimals;
}

void
Double_Knob::setIncrement(double incr,
                          int index)
{
    if (incr <= 0.) {
        qDebug() << "Attempting to set the increment of a double param to a value lesser or equal to 0.";
        
        return;
    }
    if ( index >= (int)_increments.size() ) {
        throw std::runtime_error("Double_Knob::setIncrement , dimension out of range");
    }
    
    _increments[index] = incr;
    Q_EMIT incrementChanged(_increments[index], index);
}

void
Double_Knob::setDecimals(int decis,
                         int index)
{
    if ( index >= (int)_decimals.size() ) {
        throw std::runtime_error("Double_Knob::setDecimals , dimension out of range");
    }
    
    _decimals[index] = decis;
    Q_EMIT decimalsChanged(_decimals[index], index);
}


void
Double_Knob::setIncrement(const std::vector<double> &incr)
{
    assert( incr.size() == (U32)getDimension() );
    _increments = incr;
    for (U32 i = 0; i < incr.size(); ++i) {
        Q_EMIT incrementChanged(_increments[i], i);
    }
}

void
Double_Knob::setDecimals(const std::vector<int> &decis)
{
    assert( decis.size() == (U32)getDimension() );
    _decimals = decis;
    for (U32 i = 0; i < decis.size(); ++i) {
        Q_EMIT decimalsChanged(decis[i], i);
    }
}

void
Double_Knob::onNodeDeactivated()
{
    ///unslave all roto control points that would be slaved
    for (std::list< boost::shared_ptr<BezierCP> >::iterator it = _slavedTracks.begin(); it != _slavedTracks.end(); ++it) {
        (*it)->unslave();
    }
}

void
Double_Knob::onNodeActivated()
{
    ///get a shared_ptr to this
    assert( getHolder() );
    boost::shared_ptr<Double_Knob> thisShared;
    const std::vector<boost::shared_ptr<KnobI> > & knobs = getHolder()->getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i].get() == this) {
            thisShared = boost::dynamic_pointer_cast<Double_Knob>(knobs[i]);
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
Double_Knob::removeSlavedTrack(const boost::shared_ptr<BezierCP> & cp)
{
    std::list< boost::shared_ptr<BezierCP> >::iterator found = std::find(_slavedTracks.begin(),_slavedTracks.end(),cp);
    
    if ( found != _slavedTracks.end() ) {
        _slavedTracks.erase(found);
    }
}

void
Double_Knob::serializeTracks(std::list<SerializedTrack>* tracks)
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
Double_Knob::restoreTracks(const std::list <SerializedTrack> & tracks,
                           const std::list<boost::shared_ptr<Node> > & activeNodes)
{
    ///get a shared_ptr to this
    assert( getHolder() );
    boost::shared_ptr<Double_Knob> thisShared;
    const std::vector<boost::shared_ptr<KnobI> > & knobs = getHolder()->getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i].get() == this) {
            thisShared = boost::dynamic_pointer_cast<Double_Knob>(knobs[i]);
            break;
        }
    }
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

Double_Knob::~Double_Knob()
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
Double_Knob::setSpatial(bool spatial)
{
    _spatial = spatial;
}

bool
Double_Knob::getIsSpatial() const
{
    return _spatial;
}

void
Double_Knob::setDefaultValuesNormalized(int dims,
                                        double defaults[])
{
    assert( dims == getDimension() );
    _defaultStoredNormalized = true;
    for (int i = 0; i < dims; ++i) {
        setDefaultValue(defaults[i],i);
    }
}

bool
Double_Knob::areDefaultValuesNormalized() const
{
    return _defaultStoredNormalized;
}

void
Double_Knob::denormalize(int dimension,
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
    if (dimension == 0) {
        *value *= rod.width();
    } else if (dimension == 1) {
        *value *= rod.height();
    }
}

void
Double_Knob::normalize(int dimension,
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
    if (dimension == 0) {
        *value /= rod.width();
    } else if (dimension == 1) {
        *value /= rod.height();
    }
}

/******************************BUTTON_KNOB**************************************/

Button_Knob::Button_Knob(KnobHolder*  holder,
                         const std::string &description,
                         int dimension,
                         bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
, _renderButton(false)
{
    setIsPersistant(false);
}

bool
Button_Knob::canAnimate() const
{
    return false;
}

const std::string Button_Knob::_typeNameStr("Button");
const std::string &
Button_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
Button_Knob::typeName() const
{
    return typeNameStatic();
}

/******************************CHOICE_KNOB**************************************/

Choice_Knob::Choice_Knob(KnobHolder* holder,
                         const std::string &description,
                         int dimension,
                         bool declaredByPlugin)
: Knob<int>(holder, description, dimension,declaredByPlugin)
, _entriesMutex()
, _addNewChoice(false)
, _isCascading(false)
{
}

Choice_Knob::~Choice_Knob()
{
}

void
Choice_Knob::setHostCanAddOptions(bool add)
{
    _addNewChoice = add;
}

bool
Choice_Knob::getHostCanAddOptions() const
{
    return _addNewChoice;
}

bool
Choice_Knob::canAnimate() const
{
    return canAnimateStatic();
}

const std::string Choice_Knob::_typeNameStr("Choice");
const std::string &
Choice_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
Choice_Knob::typeName() const
{
    return typeNameStatic();
}

/*Must be called right away after the constructor.*/
void
Choice_Knob::populateChoices(const std::vector<std::string> &entries,
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
            if (entries[i] == curEntry) {
                blockValueChanges();
                setValue(cur_i, 0);
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

std::vector<std::string>
Choice_Knob::getEntries_mt_safe() const
{
    QMutexLocker l(&_entriesMutex);
    return _entries;
}

const std::string&
Choice_Knob::getEntry(int v) const
{
    if (v < 0 || (int)_entries.size() <= v) {
        throw std::runtime_error(std::string("Choice_Knob::getEntry: index out of range"));
    }
    return _entries[v];
}

int
Choice_Knob::getNumEntries() const
{
    QMutexLocker l(&_entriesMutex);
    return (int)_entries.size();
}

std::vector<std::string>
Choice_Knob::getEntriesHelp_mt_safe() const
{
    QMutexLocker l(&_entriesMutex);
    return _entriesHelp;
}

std::string
Choice_Knob::getActiveEntryText_mt_safe() const
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

std::string
Choice_Knob::getHintToolTipFull() const
{
    assert(QThread::currentThread() == qApp->thread());
    
    bool gothelp = false;
    
    if ( !_entriesHelp.empty() ) {
        assert( _entriesHelp.size() == _entries.size() );
        for (U32 i = 0; i < _entries.size(); ++i) {
            if ( !_entriesHelp.empty() && !_entriesHelp[i].empty() ) {
                gothelp = true;
            }
        }
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
                ss << trim(_entries[i]);
                ss << ": ";
                ss << trim(_entriesHelp[i]);
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
Choice_Knob::setValueFromLabel(const std::string & value,
                               int dimension,
                               bool turnOffAutoKeying)
{
    for (std::size_t i = 0; i < _entries.size(); ++i) {
        if (caseInsensitiveCompare(_entries[i], value)) {
            return setValue(i, dimension, turnOffAutoKeying);
        }
    }
    return KnobHelper::eValueChangedReturnCodeNothingChanged;
    //throw std::runtime_error(std::string("Choice_Knob::setValueFromLabel: unknown label ") + value);
}

void
Choice_Knob::setDefaultValueFromLabel(const std::string & value,
                                      int dimension)
{
    for (std::size_t i = 0; i < _entries.size(); ++i) {
        if (caseInsensitiveCompare(_entries[i], value)) {
            return setDefaultValue(i, dimension);
        }
    }
    //throw std::runtime_error(std::string("Choice_Knob::setDefaultValueFromLabel: unknown label ") + value);
}

void
Choice_Knob::choiceRestoration(Choice_Knob* knob,const ChoiceExtraData* data)
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
/******************************SEPARATOR_KNOB**************************************/

Separator_Knob::Separator_Knob(KnobHolder* holder,
                               const std::string &description,
                               int dimension,
                               bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
{
}

bool
Separator_Knob::canAnimate() const
{
    return false;
}

const std::string Separator_Knob::_typeNameStr("Separator");
const std::string &
Separator_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
Separator_Knob::typeName() const
{
    return typeNameStatic();
}

/******************************COLOR_KNOB**************************************/

/**
 * @brief A color knob with of variable dimension. Each color is a double ranging in [0. , 1.]
 * In dimension 1 the knob will have a single channel being a gray-scale
 * In dimension 3 the knob will have 3 channel R,G,B
 * In dimension 4 the knob will have R,G,B and A channels.
 **/

Color_Knob::Color_Knob(KnobHolder* holder,
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
Color_Knob::onDimensionSwitchToggled(bool b)
{
    _allDimensionsEnabled = b;
}

bool
Color_Knob::areAllDimensionsEnabled() const
{
    return _allDimensionsEnabled;
}



bool
Color_Knob::canAnimate() const
{
    return true;
}

const std::string
Color_Knob::_typeNameStr("Color");
const std::string &
Color_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
Color_Knob::typeName() const
{
    return typeNameStatic();
}


void
Color_Knob::setSimplified(bool simp)
{
    _simplifiedMode = simp;
}

bool
Color_Knob::isSimplified() const
{
    return _simplifiedMode;
}

/******************************STRING_KNOB**************************************/


String_Knob::String_Knob(KnobHolder* holder,
                         const std::string &description,
                         int dimension,
                         bool declaredByPlugin)
: AnimatingString_KnobHelper(holder, description, dimension,declaredByPlugin)
, _multiLine(false)
, _richText(false)
, _isLabel(false)
, _isCustom(false)
{
}

String_Knob::~String_Knob()
{
}

bool
String_Knob::canAnimate() const
{
    return canAnimateStatic();
}

const std::string String_Knob::_typeNameStr("String");
const std::string &
String_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
String_Knob::typeName() const
{
    return typeNameStatic();
}

bool
String_Knob::hasContentWithoutHtmlTags() const
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

/******************************GROUP_KNOB**************************************/

Group_Knob::Group_Knob(KnobHolder* holder,
                       const std::string &description,
                       int dimension,
                       bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
, _isTab(false)
{
}

void
Group_Knob::setAsTab()
{
    _isTab = true;
}

bool
Group_Knob::isTab() const
{
    return _isTab;
}

bool
Group_Knob::canAnimate() const
{
    return false;
}

const std::string Group_Knob::_typeNameStr("Group");
const std::string &
Group_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
Group_Knob::typeName() const
{
    return typeNameStatic();
}

void
Group_Knob::addKnob(boost::shared_ptr<KnobI> k)
{
    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }
    
    
    boost::shared_ptr<KnobI> parent= k->getParentKnob();
    if (parent) {
        Group_Knob* isParentGrp = dynamic_cast<Group_Knob*>(parent.get());
        Page_Knob* isParentPage = dynamic_cast<Page_Knob*>(parent.get());
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
Group_Knob::removeKnob(KnobI* k)
{
    for (std::vector<boost::weak_ptr<KnobI> >::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->lock().get() == k) {
            _children.erase(it);
            return;
        }
    }
}

void
Group_Knob::moveOneStepUp(KnobI* k)
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
Group_Knob::moveOneStepDown(KnobI* k)
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
Group_Knob::insertKnob(int index, const boost::shared_ptr<KnobI>& k)
{
    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }
    
    boost::shared_ptr<KnobI> parent= k->getParentKnob();
    if (parent) {
        Group_Knob* isParentGrp = dynamic_cast<Group_Knob*>(parent.get());
        Page_Knob* isParentPage = dynamic_cast<Page_Knob*>(parent.get());
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
Group_Knob::getChildren() const
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

Page_Knob::Page_Knob(KnobHolder* holder,
                     const std::string &description,
                     int dimension,
                     bool declaredByPlugin)
: Knob<bool>(holder, description, dimension,declaredByPlugin)
{
}

bool
Page_Knob::canAnimate() const
{
    return false;
}

const std::string Page_Knob::_typeNameStr("Page");
const std::string &
Page_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
Page_Knob::typeName() const
{
    return typeNameStatic();
}


std::vector< boost::shared_ptr<KnobI> >
Page_Knob::getChildren() const
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
Page_Knob::addKnob(const boost::shared_ptr<KnobI> &k)
{
    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }
    
    
    boost::shared_ptr<KnobI> parent= k->getParentKnob();
    if (parent) {
        Group_Knob* isParentGrp = dynamic_cast<Group_Knob*>(parent.get());
        Page_Knob* isParentPage = dynamic_cast<Page_Knob*>(parent.get());
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
Page_Knob::insertKnob(int index, const boost::shared_ptr<KnobI>& k)
{
    for (std::size_t i = 0; i < _children.size(); ++i) {
        if (_children[i].lock() == k) {
            return;
        }
    }
    
    boost::shared_ptr<KnobI> parent= k->getParentKnob();
    if (parent) {
        Group_Knob* isParentGrp = dynamic_cast<Group_Knob*>(parent.get());
        Page_Knob* isParentPage = dynamic_cast<Page_Knob*>(parent.get());
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
Page_Knob::removeKnob(KnobI* k)
{
    for (std::vector<boost::weak_ptr<KnobI> >::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (it->lock().get() == k) {
            _children.erase(it);
            return;
        }
    }
}

void
Page_Knob::moveOneStepUp(KnobI* k)
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
Page_Knob::moveOneStepDown(KnobI* k)
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


/******************************Parametric_Knob**************************************/


Parametric_Knob::Parametric_Knob(KnobHolder* holder,
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

const std::string Parametric_Knob::_typeNameStr("Parametric");
const std::string &
Parametric_Knob::typeNameStatic()
{
    return _typeNameStr;
}

bool
Parametric_Knob::canAnimate() const
{
    return false;
}

const std::string &
Parametric_Knob::typeName() const
{
    return typeNameStatic();
}

void
Parametric_Knob::setCurveColor(int dimension,
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
Parametric_Knob::getCurveColor(int dimension,
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
Parametric_Knob::setParametricRange(double min,
                                    double max)
{
    ///only called in the main thread
    assert( QThread::currentThread() == qApp->thread() );
    ///Mt-safe as it never changes
    
    for (U32 i = 0; i < _curves.size(); ++i) {
        _curves[i]->setXRange(min, max);
    }
}

std::pair<double,double> Parametric_Knob::getParametricRange() const
{
    ///Mt-safe as it never changes
    assert( !_curves.empty() );
    
    return _curves.front()->getXRange();
}

boost::shared_ptr<Curve>
Parametric_Knob::getDefaultParametricCurve(int dimension) const
{
    assert(dimension >= 0 && dimension < (int)_curves.size());
    std::pair<int,boost::shared_ptr<KnobI> >  master = getMaster(dimension);
    if (master.second) {
        Parametric_Knob* m = dynamic_cast<Parametric_Knob*>(master.second.get());
        assert(m);
        return m->getDefaultParametricCurve(dimension);
    } else {
        return _defaultCurves[dimension];
    }

}

boost::shared_ptr<Curve> Parametric_Knob::getParametricCurve(int dimension) const
{
    ///Mt-safe as Curve is MT-safe and the pointer is never deleted
    
    assert( dimension < (int)_curves.size() );
    std::pair<int,boost::shared_ptr<KnobI> >  master = getMaster(dimension);
    if (master.second) {
        Parametric_Knob* m = dynamic_cast<Parametric_Knob*>(master.second.get());
        assert(m);
        return m->getParametricCurve(dimension);
    } else {
        return _curves[dimension];
    }
}

Natron::StatusEnum
Parametric_Knob::addControlPoint(int dimension,
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
Parametric_Knob::addHorizontalControlPoint(int dimension,double key,double value)
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
Parametric_Knob::getValue(int dimension,
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
Parametric_Knob::getNControlPoints(int dimension,
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
Parametric_Knob::getNthControlPoint(int dimension,
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
Parametric_Knob::getNthControlPoint(int dimension,
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
Parametric_Knob::setNthControlPoint(int dimension,
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
Parametric_Knob::setNthControlPoint(int dimension,
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
Parametric_Knob::deleteControlPoint(int dimension,
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
Parametric_Knob::deleteAllControlPoints(int dimension)
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
Parametric_Knob::cloneExtraData(KnobI* other,int dimension )
{
    ///Mt-safe as Curve is MT-safe
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(other);
    
    if ( isParametric && ( isParametric->getDimension() == getDimension() ) ) {
        for (int i = 0; i < getDimension(); ++i) {
            if (i == dimension || dimension == -1) {
                _curves[i]->clone( *isParametric->getParametricCurve(i) );
            }
        }
    }
}

bool
Parametric_Knob::cloneExtraDataAndCheckIfChanged(KnobI* other,int dimension) {
    bool hasChanged = false;
    ///Mt-safe as Curve is MT-safe
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(other);
    
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
Parametric_Knob::cloneExtraData(KnobI* other,
                                SequenceTime offset,
                                const RangeD* range,
                                int dimension)
{
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(other);
    
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
Parametric_Knob::saveParametricCurves(std::list< Curve >* curves) const
{
    for (U32 i = 0; i < _curves.size(); ++i) {
        curves->push_back(*_curves[i]);
    }
}

void
Parametric_Knob::loadParametricCurves(const std::list< Curve > & curves)
{
    assert( !_curves.empty() );
    int i = 0;
    for (std::list< Curve >::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        _curves[i]->clone(*it);
        ++i;
    }
}

void
Parametric_Knob::resetExtraToDefaultValue(int dimension)
{
    Natron::StatusEnum s = deleteAllControlPoints(dimension);
    Q_UNUSED(s);
    _curves[dimension]->clone(*_defaultCurves[dimension]);
    Q_EMIT curveChanged(dimension);

}

void
Parametric_Knob::setDefaultCurvesFromCurves()
{
    assert(_curves.size() == _defaultCurves.size());
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        _defaultCurves[i]->clone(*_curves[i]);
    }
}

bool
Parametric_Knob::hasModificationsVirtual(int dimension) const
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
