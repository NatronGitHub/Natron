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
#include "KnobTypes.h"

#include <cfloat>
#include <sstream>
#include <boost/math/special_functions/fpclassify.hpp>

#include <QDebug>
#include <QThread>
#include <QCoreApplication>

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
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
        throw "Int_Knob::setIncrement , dimension out of range";
    }
    _increments[index] = incr;
    emit incrementChanged(_increments[index], index);
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
        emit incrementChanged(_increments[i], i);
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
, _increments(dimension)
, _decimals(dimension)
, _disableSlider(false)
, _normalizationXY()
, _defaultStoredNormalized(false)

{
    _normalizationXY.first = NORMALIZATION_NONE;
    _normalizationXY.second = NORMALIZATION_NONE;
    
    for (int i = 0; i < dimension; ++i) {
        _increments[i] = 1.;
        _decimals[i] = 2;
    }
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
        throw "Double_Knob::setIncrement , dimension out of range";
    }
    
    _increments[index] = incr;
    emit incrementChanged(_increments[index], index);
}

void
Double_Knob::setDecimals(int decis,
                         int index)
{
    if ( index >= (int)_decimals.size() ) {
        throw "Double_Knob::setDecimals , dimension out of range";
    }
    
    _decimals[index] = decis;
    emit decimalsChanged(_decimals[index], index);
}


void
Double_Knob::setIncrement(const std::vector<double> &incr)
{
    assert( incr.size() == (U32)getDimension() );
    _increments = incr;
    for (U32 i = 0; i < incr.size(); ++i) {
        emit incrementChanged(_increments[i], i);
    }
}

void
Double_Knob::setDecimals(const std::vector<int> &decis)
{
    assert( decis.size() == (U32)getDimension() );
    _decimals = decis;
    for (U32 i = 0; i < decis.size(); ++i) {
        emit decimalsChanged(decis[i], i);
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
        s.bezierName = (*it)->getBezier()->getName_mt_safe();
        s.isFeather = (*it)->isFeatherPoint();
        s.cpIndex = !s.isFeather ? (*it)->getBezier()->getControlPointIndex(*it) : (*it)->getBezier()->getFeatherPointIndex(*it);
        s.rotoNodeName = (*it)->getBezier()->getRotoNodeName();
        s.offsetTime = (*it)->getOffsetTime();
        tracks->push_back(s);
    }
}

void
Double_Knob::restoreTracks(const std::list <SerializedTrack> & tracks,
                           const std::vector<boost::shared_ptr<Node> > & activeNodes)
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
        if (it->rotoNodeName == lastNodeName) {
            roto = lastRoto;
        } else {
            for (U32 i = 0; i < activeNodes.size(); ++i) {
                if (activeNodes[i]->getName() == it->rotoNodeName) {
                    lastNodeName = activeNodes[i]->getName();
                    boost::shared_ptr<RotoContext> rotoCtx = activeNodes[i]->getRotoContext();
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
                qDebug() << "Failed to restore slaved track " << it->bezierName.c_str();
                break;
            }
            Bezier* isBezier = dynamic_cast<Bezier*>( item.get() );
            assert(isBezier);
            
            boost::shared_ptr<BezierCP> point = it->isFeather ?
            isBezier->getFeatherPointAtIndex(it->cpIndex)
            : isBezier->getControlPointAtIndex(it->cpIndex);
            
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
            double time,
            RectD & rod)
{
    RenderScale scale;
    scale.y = scale.x = 1.;
    bool isProjectFormat;
    Status stat = effect->getRegionOfDefinition_public(effect->getHash(),time, scale, /*view*/0, &rod, &isProjectFormat);
    if ( (stat == StatFailed) || ( (rod.x1 == 0) && (rod.y1 == 0) && (rod.x2 == 1) && (rod.y2 == 1) ) ) {
        Format f;
        effect->getRenderFormat(&f);
        rod = f;
    }
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
{
}

Choice_Knob::~Choice_Knob()
{
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
    {
        QMutexLocker l(&_entriesMutex);
        _entriesHelp = entriesHelp;
        _entries = entries;
    }
    emit populated();
}

std::vector<std::string>
Choice_Knob::getEntries_mt_safe() const
{
    QMutexLocker l(&_entriesMutex);
    return _entries;
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

void
Choice_Knob::deepCloneExtraData(KnobI* other)
{
    Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(other);
    if (!isChoice) {
        return;
    }
    
    
    QMutexLocker l(&_entriesMutex);
    _entries = isChoice->getEntries_mt_safe();
    _entriesHelp = isChoice->getEntriesHelp_mt_safe();
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
        std::vector<std::string>::iterator it = std::find(_entries.begin(), _entries.end(), data->_choiceString);
        if ( it != _entries.end() ) {
            setValue(std::distance(_entries.begin(), it), 0);
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
Color_Knob::setValues(double r,
                      double g,
                      double b)
{
    assert(getDimension() == 3);
    KeyFrame k;
    blockEvaluation();
    onValueChanged(r, 0, Natron::NATRON_GUI_EDITED, &k);
    onValueChanged(g, 1, Natron::NATRON_GUI_EDITED, &k);
    unblockEvaluation();
    onValueChanged(b, 2, Natron::NATRON_GUI_EDITED, &k);
}

void
Color_Knob::setValues(double r,
                      double g,
                      double b,
                      double a)
{
    assert(getDimension() == 4);
    KeyFrame k;
    blockEvaluation();
    onValueChanged(r, 0, Natron::NATRON_GUI_EDITED, &k);
    onValueChanged(g, 1, Natron::NATRON_GUI_EDITED, &k);
    onValueChanged(b, 2, Natron::NATRON_GUI_EDITED, &k);
    unblockEvaluation();
    onValueChanged(a, 3, Natron::NATRON_GUI_EDITED, &k);
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
    std::vector<boost::shared_ptr<KnobI> >::iterator found = std::find(_children.begin(), _children.end(), k);
    
    if ( found == _children.end() ) {
        _children.push_back(k);
        boost::shared_ptr<KnobI> thisSharedPtr = getHolder()->getKnobByName( getName() );
        assert(thisSharedPtr);
        k->setParentKnob(thisSharedPtr);
    }
}

const std::vector< boost::shared_ptr<KnobI> > &
Group_Knob::getChildren() const
{
    return _children;
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

void
Page_Knob::addKnob(boost::shared_ptr<KnobI> k)
{
    std::vector<boost::shared_ptr<KnobI> >::iterator found = std::find(_children.begin(), _children.end(), k);
    
    if ( found == _children.end() ) {
        _children.push_back(k);
        if ( !k->getParentKnob() ) {
            k->setParentKnob( getHolder()->getKnobByName( getName() ) );
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
, _curvesColor(dimension)
{
    for (int i = 0; i < dimension; ++i) {
        RGBAColourF color;
        color.r = color.g = color.b = color.a = 1.;
        _curvesColor[i] = color;
        _curves[i] = boost::shared_ptr<Curve>( new Curve(this,i) );
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


boost::shared_ptr<Curve> Parametric_Knob::getParametricCurve(int dimension) const
{
    ///Mt-safe as Curve is MT-safe and the pointer is never deleted
    
    assert( dimension < (int)_curves.size() );
    
    return _curves[dimension];
}

Natron::Status
Parametric_Knob::addControlPoint(int dimension,
                                 double key,
                                 double value)
{
    ///Mt-safe as Curve is MT-safe
    if (dimension >= (int)_curves.size() ||
        boost::math::isnan(key) ||
        boost::math::isinf(key) ||
        boost::math::isnan(value) ||
        boost::math::isinf(value)) {
        return StatFailed;
    }
    
    KeyFrame k(key,value);
    k.setInterpolation(Natron::KEYFRAME_CUBIC);
    _curves[dimension]->addKeyFrame(k);
    emit curveChanged(dimension);
    
    return StatOK;
}

Natron::Status
Parametric_Knob::getValue(int dimension,
                          double parametricPosition,
                          double *returnValue)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return StatFailed;
    }
    try {
        *returnValue = _curves[dimension]->getValueAt(parametricPosition);
    }catch (...) {
        return Natron::StatFailed;
    }
    
    return Natron::StatOK;
}

Natron::Status
Parametric_Knob::getNControlPoints(int dimension,
                                   int *returnValue)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return StatFailed;
    }
    *returnValue =  _curves[dimension]->getKeyFramesCount();
    
    return StatOK;
}

Natron::Status
Parametric_Knob::getNthControlPoint(int dimension,
                                    int nthCtl,
                                    double *key,
                                    double *value)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return StatFailed;
    }
    KeyFrame kf;
    bool ret = _curves[dimension]->getKeyFrameWithIndex(nthCtl, &kf);
    if (!ret) {
        return StatFailed;
    }
    *key = kf.getTime();
    *value = kf.getValue();
    
    return StatOK;
}

Natron::Status
Parametric_Knob::setNthControlPoint(int dimension,
                                    int nthCtl,
                                    double key,
                                    double value)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return StatFailed;
    }
    _curves[dimension]->setKeyFrameValueAndTime(key, value, nthCtl);
    emit curveChanged(dimension);
    
    return StatOK;
}

Natron::Status
Parametric_Knob::deleteControlPoint(int dimension,
                                    int nthCtl)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return StatFailed;
    }
    
    _curves[dimension]->removeKeyFrameWithIndex(nthCtl);
    emit curveChanged(dimension);
    
    return StatOK;
}

Natron::Status
Parametric_Knob::deleteAllControlPoints(int dimension)
{
    ///Mt-safe as Curve is MT-safe
    if ( dimension >= (int)_curves.size() ) {
        return StatFailed;
    }
    _curves[dimension]->clearKeyFrames();
    emit curveChanged(dimension);
    
    return StatOK;
}

void
Parametric_Knob::cloneExtraData(KnobI* other)
{
    ///Mt-safe as Curve is MT-safe
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(other);
    
    if ( isParametric && ( isParametric->getDimension() == getDimension() ) ) {
        for (int i = 0; i < getDimension(); ++i) {
            _curves[i]->clone( *isParametric->getParametricCurve(i) );
        }
    }
}

void
Parametric_Knob::cloneExtraData(KnobI* other,
                                SequenceTime offset,
                                const RangeD* range)
{
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(other);
    
    if (isParametric) {
        int dimMin = std::min( getDimension(), isParametric->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            _curves[i]->clone(*isParametric->getParametricCurve(i), offset, range);
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
    QVector<int> dimensions(1);
    dimensions[0] = dimension;
    emit mustResetToDefault(dimensions);
}

