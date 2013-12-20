//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#include "KnobTypes.h"

#include <cfloat>

#include "Engine/Curve.h"

using namespace Natron;
using std::make_pair;
using std::pair;

/******************************INT_KNOB**************************************/
Int_Knob::Int_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
, _disableSlider(false)
{}

void Int_Knob::disableSlider()
{
    _disableSlider = true;
}

bool Int_Knob::isSliderDisabled() const
{
    return _disableSlider;
}

void Int_Knob::setMinimum(int mini, int index)
{
    if (_minimums.size() > (U32)index) {
        _minimums[index] = mini;
    } else {
        if (index == 0) {
            _minimums.push_back(mini);
        } else {
            while (_minimums.size() <= (U32)index) {
                // kOfxParamPropMin default value is INT_MIN
                _minimums.push_back(INT_MIN);
            }
            _minimums.push_back(mini);
        }
    }
    // kOfxParamPropMax default value is INT_MAX
    int maximum = INT_MAX;
    if (_maximums.size() > (U32)index) {
        maximum = _maximums[index];
    }
    emit minMaxChanged(mini, maximum, index);
}

void Int_Knob::setMaximum(int maxi, int index)
{
    
    if (_maximums.size() > (U32)index) {
        _maximums[index] = maxi;
    } else {
        if (index == 0) {
            _maximums.push_back(maxi);
        } else {
            while (_maximums.size() <= (U32)index) {
                // kOfxParamPropMax default value is INT_MAX
                _maximums.push_back(INT_MAX);
            }
            _maximums.push_back(maxi);
        }
    }
    // kOfxParamPropMin default value is INT_MIN
    int minimum = INT_MIN;
    if (_minimums.size() > (U32)index) {
        minimum = _minimums[index];
    }
    emit minMaxChanged(minimum, maxi, index);
}

void Int_Knob::setDisplayMinimum(int mini, int index)
{
    if (_displayMins.size() > (U32)index) {
        _displayMins[index] = mini;
    } else {
        if (index == 0) {
            _displayMins.push_back(mini);
        } else {
            while (_displayMins.size() <= (U32)index) {
                // kOfxParamPropDisplayMin default value is INT_MIN
                _displayMins.push_back(INT_MIN);
            }
            _displayMins.push_back(mini);
        }
    }
}

void Int_Knob::setDisplayMaximum(int maxi, int index)
{
    
    if (_displayMaxs.size() > (U32)index) {
        _displayMaxs[index] = maxi;
    } else {
        if (index == 0) {
            _displayMaxs.push_back(maxi);
        } else {
            while (_displayMaxs.size() <= (U32)index) {
                // kOfxParamPropDisplayMax default value is INT_MAX
                _displayMaxs.push_back(INT_MAX);
            }
            _displayMaxs.push_back(maxi);
        }
    }
}

void Int_Knob::setIncrement(int incr, int index)
{
    assert(incr > 0);
    /*If _increments is already filled, just replace the existing value*/
    if (_increments.size() > (U32)index) {
        _increments[index] = incr;
    } else {
        /*If it is not filled and it is 0, just push_back the value*/
        if (index == 0) {
            _increments.push_back(incr);
        } else {
            /*Otherwise fill enough values until we  have
             the corresponding index in _increments. Then we
             can push_back the value as the last element of the
             vector.*/
            while (_increments.size() <= (U32)index) {
                // kOfxParamPropIncrement default value is 1
                _increments.push_back(1);
                assert(_increments[_increments.size() - 1] > 0);
            }
            _increments.push_back(incr);
        }
    }
    emit incrementChanged(_increments[index], index);
}

void Int_Knob::setIncrement(const std::vector<int> &incr)
{
    _increments = incr;
    for (U32 i = 0; i < _increments.size(); ++i) {
        assert(_increments[i] > 0);
        emit incrementChanged(_increments[i], i);
    }
}

/*minis & maxis must have the same size*/
void Int_Knob::setMinimumsAndMaximums(const std::vector<int> &minis, const std::vector<int> &maxis)
{
    _minimums = minis;
    _maximums = maxis;
    for (U32 i = 0; i < maxis.size(); ++i) {
        emit minMaxChanged(_minimums[i], _maximums[i], i);
    }
}

void Int_Knob::setDisplayMinimumsAndMaximums(const std::vector<int> &minis, const std::vector<int> &maxis)
{
    _displayMins = minis;
    _displayMaxs = maxis;
}

const std::vector<int> &Int_Knob::getMinimums() const
{
    return _minimums;
}

const std::vector<int> &Int_Knob::getMaximums() const
{
    return _maximums;
}

const std::vector<int> &Int_Knob::getIncrements() const
{
    return _increments;
}

const std::vector<int> &Int_Knob::getDisplayMinimums() const
{
    return _displayMins;
}

const std::vector<int> &Int_Knob::getDisplayMaximums() const
{
    return _displayMaxs;
}


std::string Int_Knob::getDimensionName(int dimension) const
{
    switch (dimension) {
        case 0:
            return "x";
        case 1:
            return "y";
        case 2:
            return "z";
        case 3:
            return "w";
        default:
            return QString::number(dimension).toStdString();
    }
}

bool Int_Knob::canAnimate() const
{
    return true;
}

const std::string Int_Knob::_typeNameStr("Int");

const std::string& Int_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Int_Knob::typeName() const
{
    return typeNameStatic();
}


/******************************BOOL_KNOB**************************************/

Bool_Knob::Bool_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
{}

bool Bool_Knob::canAnimate() const
{
    return canAnimateStatic();
}

const std::string Bool_Knob::_typeNameStr("Bool");

const std::string& Bool_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Bool_Knob::typeName() const
{
    return typeNameStatic();
}


/******************************DOUBLE_KNOB**************************************/


Double_Knob::Double_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
, _disableSlider(false)
{}

std::string Double_Knob::getDimensionName(int dimension) const
{
    switch (dimension) {
        case 0:
            return "x";
        case 1:
            return "y";
        case 2:
            return "z";
        case 3:
            return "w";
        default:
            return QString::number(dimension).toStdString();
    }
}

void Double_Knob::disableSlider()
{
    _disableSlider = true;
}

bool Double_Knob::isSliderDisabled() const
{
    return _disableSlider;
}

bool Double_Knob::canAnimate() const
{
    return true;
}

const std::string Double_Knob::_typeNameStr("Double");

const std::string& Double_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Double_Knob::typeName() const
{
    return typeNameStatic();
}

const std::vector<double> &Double_Knob::getMinimums() const
{
    return _minimums;
}

const std::vector<double> &Double_Knob::getMaximums() const
{
    return _maximums;
}

const std::vector<double> &Double_Knob::getIncrements() const
{
    return _increments;
}

const std::vector<int> &Double_Knob::getDecimals() const
{
    return _decimals;
}

const std::vector<double> &Double_Knob::getDisplayMinimums() const
{
    return _displayMins;
}

const std::vector<double> &Double_Knob::getDisplayMaximums() const
{
    return _displayMaxs;
}

void Double_Knob::setMinimum(double mini, int index)
{
    if (_minimums.size() > (U32)index) {
        _minimums[index] = mini;
    } else {
        if (index == 0) {
            _minimums.push_back(mini);
        } else {
            while (_minimums.size() <= (U32)index) {
                // kOfxParamPropMin default value is -DBL_MAX
                _minimums.push_back(-DBL_MAX);
            }
            _minimums.push_back(mini);
        }
    }
    // kOfxParamPropMax default value is DBL_MAX
    double maximum = DBL_MAX;
    if (_maximums.size() > (U32)index) {
        maximum = _maximums[index];
    }
    emit minMaxChanged(mini, maximum, index);
}

void Double_Knob::setMaximum(double maxi, int index)
{
    if (_maximums.size() > (U32)index) {
        _maximums[index] = maxi;
    } else {
        if (index == 0) {
            _maximums.push_back(maxi);
        } else {
            while (_maximums.size() <= (U32)index) {
                // kOfxParamPropMax default value is DBL_MAX
                _maximums.push_back(DBL_MAX);
            }
            _maximums.push_back(maxi);
        }
    }
    // kOfxParamPropMin default value is -DBL_MAX
    double minimum = -DBL_MAX;
    if (_minimums.size() > (U32)index) {
        minimum = _minimums[index];
    }
    emit minMaxChanged(minimum, maxi, index);
}

void Double_Knob::setDisplayMinimum(double mini, int index)
{
    if (_displayMins.size() > (U32)index) {
        _displayMins[index] = mini;
    } else {
        if (index == 0) {
            _displayMins.push_back(DBL_MIN);
        } else {
            while (_displayMins.size() <= (U32)index) {
                // kOfxParamPropDisplayMax default value is -DBL_MAX
                _displayMins.push_back(-DBL_MAX);
            }
            _displayMins.push_back(mini);
        }
    }
}

void Double_Knob::setDisplayMaximum(double maxi, int index)
{
    
    if (_displayMaxs.size() > (U32)index) {
        _displayMaxs[index] = maxi;
    } else {
        if (index == 0) {
            _displayMaxs.push_back(maxi);
        } else {
            while (_displayMaxs.size() <= (U32)index) {
                // kOfxParamPropDisplayMax default value is DBL_MAX
                _displayMaxs.push_back(DBL_MAX);
            }
            _displayMaxs.push_back(maxi);
        }
    }
}

void Double_Knob::setIncrement(double incr, int index)
{
    assert(incr > 0.);
    if (_increments.size() > (U32)index) {
        _increments[index] = incr;
    } else {
        if (index == 0) {
            _increments.push_back(incr);
        } else {
            while (_increments.size() <= (U32)index) {
                // kOfxParamPropIncrement default value is 1.
                _increments.push_back(1.);
            }
            _increments.push_back(incr);
        }
    }
    emit incrementChanged(_increments[index], index);
}

void Double_Knob::setDecimals(int decis, int index)
{
    if (_decimals.size() > (U32)index) {
        _decimals[index] = decis;
    } else {
        if (index == 0) {
            _decimals.push_back(decis);
        } else {
            while (_decimals.size() <= (U32)index) {
                // default value for kOfxParamPropDigits is 2
                _decimals.push_back(2);
            }
            _decimals.push_back(decis);
        }
    }
    emit decimalsChanged(_decimals[index], index);
}


/*minis & maxis must have the same size*/
void Double_Knob::setMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis)
{
    _minimums = minis;
    _maximums = maxis;
    for (U32 i = 0; i < maxis.size(); ++i) {
        emit minMaxChanged(_minimums[i], _maximums[i], i);
    }
}

void Double_Knob::setDisplayMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis)
{
    _displayMins = minis;
    _displayMaxs = maxis;
}

void Double_Knob::setIncrement(const std::vector<double> &incr)
{
    _increments = incr;
    for (U32 i = 0; i < incr.size(); ++i) {
        emit incrementChanged(_increments[i], i);
    }
}
void Double_Knob::setDecimals(const std::vector<int> &decis)
{
    _decimals = decis;
    for (U32 i = 0; i < decis.size(); ++i) {
        emit decimalsChanged(decis[i], i);
    }
}


/******************************BUTTON_KNOB**************************************/

Button_Knob::Button_Knob(KnobHolder  *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
{
    setPersistent(false);
}

bool Button_Knob::canAnimate() const
{
    return false;
}

const std::string Button_Knob::_typeNameStr("Button");

const std::string& Button_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Button_Knob::typeName() const
{
    return typeNameStatic();
}


/******************************CHOICE_KNOB**************************************/

Choice_Knob::Choice_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
{
    
}

bool Choice_Knob::canAnimate() const
{
    return canAnimateStatic();
}

const std::string Choice_Knob::_typeNameStr("Choice");

const std::string& Choice_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Choice_Knob::typeName() const
{
    return typeNameStatic();
}

/*Must be called right away after the constructor.*/
void Choice_Knob::populate(const std::vector<std::string> &entries, const std::vector<std::string> &entriesHelp)
{
    assert(_entriesHelp.empty() || _entriesHelp.size() == entries.size());
    _entriesHelp = entriesHelp;
    _entries = entries;
    emit populated();
}

const std::vector<std::string> &Choice_Knob::getEntries() const
{
    return _entries;
}

const std::vector<std::string> &Choice_Knob::getEntriesHelp() const
{
    return _entriesHelp;
}

int Choice_Knob::getActiveEntry() const
{
    return getValue<int>();
}

const std::string &Choice_Knob::getActiveEntryText() const
{
    return _entries[getActiveEntry()];
}

/******************************SEPARATOR_KNOB**************************************/

Separator_Knob::Separator_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder, description, dimension)
{
    
}

bool Separator_Knob::canAnimate() const
{
    return false;
}

const std::string Separator_Knob::_typeNameStr("Separator");

const std::string& Separator_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Separator_Knob::typeName() const
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

Color_Knob::Color_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
{
    //dimension greater than 4 is not supported. Dimension 2 doesn't make sense.
    assert(dimension <= 4 && dimension != 2);
}

std::string Color_Knob::getDimensionName(int dimension) const
{
    switch (dimension) {
        case 0:
            return "r";
        case 1:
            return "g";
        case 2:
            return "b";
        case 3:
            return "a";
        default:
            return QString::number(dimension).toStdString();
    }
}

bool Color_Knob::canAnimate() const
{
    return true;
}

const std::string Color_Knob::_typeNameStr("Color");

const std::string& Color_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Color_Knob::typeName() const
{
    return typeNameStatic();
}

/******************************STRING_KNOB**************************************/

String_Knob::String_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
{
    
}

bool String_Knob::canAnimate() const
{
    return canAnimateStatic();
}

const std::string String_Knob::_typeNameStr("String");

const std::string& String_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& String_Knob::typeName() const
{
    return typeNameStatic();
}

std::string String_Knob::getString() const
{
    return getValue<QString>().toStdString();
}

/******************************CUSTOM_KNOB**************************************/

Custom_Knob::Custom_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder, description, dimension)
{
}

bool Custom_Knob::canAnimate() const
{
    return canAnimateStatic();
}

const std::string Custom_Knob::_typeNameStr("Custom");

const std::string& Custom_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Custom_Knob::typeName() const
{
    return typeNameStatic();
}

std::string Custom_Knob::getString() const
{
    return getValue<QString>().toStdString();
}

/******************************GROUP_KNOB**************************************/

Group_Knob::Group_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
{
    
}

bool Group_Knob::canAnimate() const
{
    return false;
}

const std::string Group_Knob::_typeNameStr("Group");

const std::string& Group_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Group_Knob::typeName() const
{
    return typeNameStatic();
}

void Group_Knob::addKnob(boost::shared_ptr<Knob> k)
{
    std::vector<boost::shared_ptr<Knob> >::iterator found = std::find(_children.begin(), _children.end(), k);
    if(found == _children.end()){
        _children.push_back(k);
        k->setParentKnob(getHolder()->getKnobByDescription(getDescription()));
    }
    
}

const std::vector< boost::shared_ptr<Knob> > &Group_Knob::getChildren() const
{
    return _children;
}

/******************************TAB_KNOB**************************************/

Tab_Knob::Tab_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
{
    
}

bool Tab_Knob::canAnimate() const
{
    return false;
}

const std::string Tab_Knob::_typeNameStr("Tab");

const std::string& Tab_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& Tab_Knob::typeName() const
{
    return typeNameStatic();
}



void Tab_Knob::addKnob(boost::shared_ptr<Knob> k)
{
    std::vector<boost::shared_ptr<Knob> >::iterator found = std::find(_children.begin(), _children.end(), k);
    if(found == _children.end()){
        _children.push_back(k);
        k->setParentKnob(getHolder()->getKnobByDescription(getDescription()));
    }
}



/******************************RichText_Knob**************************************/

RichText_Knob::RichText_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
{
    
}

bool RichText_Knob::canAnimate() const
{
    return false;
}

const std::string RichText_Knob::_typeNameStr("RichText");

const std::string& RichText_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& RichText_Knob::typeName() const
{
    return typeNameStatic();
}

std::string RichText_Knob::getString() const
{
    return getValue<QString>().toStdString();
}

/******************************Parametric_Knob**************************************/


Parametric_Knob::Parametric_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder,description,dimension)
, _curves(dimension)
, _curvesColor(dimension)
, _curveLabels(dimension)
{
    for (int i = 0; i < dimension; ++i) {
        RGBAColourF color;
        color.r = color.g = color.b = color.a = 1.;
        _curvesColor[i] = color;
        _curves[i] = boost::shared_ptr<Curve>(new Curve(this));
    }
    _range[0] = 0.;
    _range[1] = 1.;
}

const std::string Parametric_Knob::_typeNameStr("Parametric");

const std::string& Parametric_Knob::typeNameStatic(){
    return _typeNameStr;
}

bool Parametric_Knob::canAnimate() const {
    return false;
}

const std::string& Parametric_Knob::typeName() const {
    return typeNameStatic();
}

void Parametric_Knob::setCurveColor(int dimension,double r,double g,double b){
    assert(dimension < (int)_curvesColor.size());
    _curvesColor[dimension].r = r;
    _curvesColor[dimension].g = g;
    _curvesColor[dimension].b = b;
}

void Parametric_Knob::getCurveColor(int dimension,double* r,double* g,double* b){
    assert(dimension < (int)_curvesColor.size());
    *r = _curvesColor[dimension].r ;
    *g = _curvesColor[dimension].g ;
    *b = _curvesColor[dimension].b ;
}

void Parametric_Knob::setCurveLabel(int dimension,const std::string& str){
    assert(dimension < (int)_curveLabels.size());
    _curveLabels[dimension] = str;
}

const std::string& Parametric_Knob::getCurveLabel(int dimension) const{
    assert(dimension < (int)_curveLabels.size());
    return _curveLabels[dimension];
}

void Parametric_Knob::setParametricRange(double min,double max){
    _range[0] = min;
    _range[1] = max;
}

void Parametric_Knob::getParametricRange(double* min,double* max){
    *min = _range[0];
    *max = _range[1];
}

std::string Parametric_Knob::getDimensionName(int dimension) const{
    return getCurveLabel(dimension);
}

boost::shared_ptr<Curve> Parametric_Knob::getParametricCurve(int dimension) const{
    assert(dimension < (int)_curves.size());
    return _curves[dimension];
}

Natron::Status Parametric_Knob::addControlPoint(int dimension,double key,double value){
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    
    _curves[dimension]->addKeyFrame(KeyFrame(key,value));
    return StatOK;
}

Natron::Status Parametric_Knob::getValue(int dimension,double parametricPosition,double *returnValue){
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    try {
        *returnValue = _curves[dimension]->getValueAt(parametricPosition);
    }catch(...){
        return Natron::StatFailed;
    }
    return Natron::StatOK;
}

Natron::Status Parametric_Knob::getNControlPoints(int dimension,int *returnValue){
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    *returnValue =  _curves[dimension]->keyFramesCount();
    return StatOK;
}

Natron::Status Parametric_Knob::getNthControlPoint(int dimension,
                                  int    nthCtl,
                                  double *key,
                                  double *value){
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    const KeyFrameSet& set = _curves[dimension]->getKeyFrames();
    int index = 0;
    for (KeyFrameSet::const_iterator it = set.begin(); it!=set.end(); ++it) {
        if(index == nthCtl){
            *key = it->getTime();
            *value = it->getValue();
            return StatOK;
        }
        ++index;
    }
    
    return StatFailed;
}

Natron::Status Parametric_Knob::setNthControlPoint(int   dimension,
                                  int   nthCtl,
                                  double key,
                                  double value)
{
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    _curves[dimension]->setKeyFrameValueAndTime(key, value, nthCtl);
    return StatOK;

}

Natron::Status  Parametric_Knob::deleteControlPoint(int   dimension,int   nthCtl){
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    
    _curves[dimension]->removeKeyFrameWithIndex(nthCtl);
    return StatOK;
}

Natron::Status  Parametric_Knob::deleteAllControlPoints(int   dimension){
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    _curves[dimension]->clearKeyFrames();
    return StatOK;
}

void Parametric_Knob::appendExtraDataToHash(std::vector<U64>* hash) const {
    for (U32 i = 0; i < _curves.size(); ++i) {
        const KeyFrameSet& set = _curves[i]->getKeyFrames();
        for (KeyFrameSet::const_iterator it = set.begin(); it!=set.end(); ++it) {
            double k = it->getTime();
            double v = it->getValue();
            double ld = it->getLeftDerivative();
            double rd = it->getRightDerivative();
            hash->push_back(*reinterpret_cast<U64*>(&k));
            hash->push_back(*reinterpret_cast<U64*>(&v));
            hash->push_back(*reinterpret_cast<U64*>(&ld));
            hash->push_back(*reinterpret_cast<U64*>(&rd));
        }
    }
}