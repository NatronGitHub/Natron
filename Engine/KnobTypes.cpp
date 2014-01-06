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

#include <QDebug>

#include "Engine/Curve.h"

using namespace Natron;
using std::make_pair;
using std::pair;

/******************************INT_KNOB**************************************/
Int_Knob::Int_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
, _minimums(dimension)
, _maximums(dimension)
, _increments(dimension)
, _displayMins(dimension)
, _displayMaxs(dimension)
, _disableSlider(false)
{
    for (int i = 0; i < dimension; ++i) {
        _minimums[i] = INT_MIN;
        _maximums[i] = INT_MAX;
        _increments[i] = 1;
        _displayMins[i] = INT_MIN;
        _displayMaxs[i] = INT_MAX;
    }
}

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
    if (index >= (int)_minimums.size()) {
        throw "Int_Knob::setMinimum , dimension out of range";
    }
    _minimums[index] = mini;
    emit minMaxChanged(mini, _maximums[index], index);
}

void Int_Knob::setMaximum(int maxi, int index)
{
    
    if (index >= (int)_maximums.size()) {
        throw "Int_Knob::setMaximum , dimension out of range";
    }
    _maximums[index] = maxi;
    emit minMaxChanged(_minimums[index], maxi, index);

}

void Int_Knob::setDisplayMinimum(int mini, int index)
{
    if (index >= (int)_displayMins.size()) {
        throw "Int_Knob::setDisplayMinimum , dimension out of range";
    }
    _displayMins[index] = mini;
    emit displayMinMaxChanged(mini, _displayMaxs[index], index);
}

void Int_Knob::setDisplayMaximum(int maxi, int index)
{
    
    if (index >= (int)_displayMaxs.size()) {
        throw "Int_Knob::setDisplayMaximum , dimension out of range";
    }
    _displayMaxs[index] = maxi;
    emit displayMinMaxChanged(_displayMins[index],maxi, index);
}

void Int_Knob::setIncrement(int incr, int index)
{
    if(incr <= 0){
        qDebug() << "Attempting to set the increment of an int param to a value lesser or equal to 0";
        return;
    }

    if (index >= (int)_increments.size()) {
        throw "Int_Knob::setIncrement , dimension out of range";
    }
    _increments[index] = incr;
    emit incrementChanged(_increments[index], index);
}

void Int_Knob::setIncrement(const std::vector<int> &incr)
{
    assert((int)incr.size() == getDimension());
    _increments = incr;
    for (U32 i = 0; i < _increments.size(); ++i) {
        if(_increments[i] <= 0){
            qDebug() << "Attempting to set the increment of an int param to a value lesser or equal to 0";
            continue;
        }
        emit incrementChanged(_increments[i], i);
    }
}

/*minis & maxis must have the same size*/
void Int_Knob::setMinimumsAndMaximums(const std::vector<int> &minis, const std::vector<int> &maxis)
{
    assert((int)minis.size() == getDimension() && (int)maxis.size() == getDimension());
    _minimums = minis;
    _maximums = maxis;
    for (U32 i = 0; i < maxis.size(); ++i) {
        emit minMaxChanged(_minimums[i], _maximums[i], i);
    }
}

void Int_Knob::setDisplayMinimumsAndMaximums(const std::vector<int> &minis, const std::vector<int> &maxis)
{
    assert((int)minis.size() == getDimension() && (int)maxis.size() == getDimension());
    _displayMins = minis;
    _displayMaxs = maxis;
    for (U32 i = 0; i < maxis.size(); ++i) {
        emit displayMinMaxChanged(_displayMins[i], _displayMaxs[i], i);
    }
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

void Int_Knob::getMinMaxForCurve(const Curve* curve,int* min,int* max){
    const std::vector< boost::shared_ptr<Curve> >& curves = getCurves();
    for (U32 i = 0; i < curves.size(); ++i) {
        if (curves[i].get() == curve) {
            const std::vector<int>& mins = getMinimums();
            const std::vector<int>& maxs = getMaximums();
            
            assert(mins.size() > i);
            assert(maxs.size() > i);
            
            *min = mins[i];
            *max = maxs[i];
        }
    }
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
, _minimums(dimension)
, _maximums(dimension)
, _increments(dimension)
, _displayMins(dimension)
, _displayMaxs(dimension)
, _decimals(dimension)
, _disableSlider(false)

{
    for (int i = 0; i < dimension; ++i) {
        _minimums[i] = -DBL_MAX;
        _maximums[i] = DBL_MAX;
        _increments[i] = 1.;
        _displayMins[i] = -DBL_MAX;
        _displayMaxs[i] = DBL_MAX;
        _decimals[i] = 2;
    }

}

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
    if (index >= (int)_minimums.size()) {
        throw "Double_Knob::setMinimum , dimension out of range";
    }
    _minimums[index] = mini;
    emit minMaxChanged(mini, _maximums[index], index);
}

void Double_Knob::setMaximum(double maxi, int index)
{
    if (index >= (int)_maximums.size()) {
        throw "Double_Knob::setMaximum , dimension out of range";
    }
    _maximums[index] = maxi;
    emit minMaxChanged(_minimums[index], maxi, index);
}

void Double_Knob::setDisplayMinimum(double mini, int index)
{
    if (index >= (int)_displayMins.size()) {
        throw "Double_Knob::setDisplayMinimum , dimension out of range";
    }
    _displayMins[index] = mini;
    emit displayMinMaxChanged(mini, _displayMaxs[index], index);
    
}

void Double_Knob::setDisplayMaximum(double maxi, int index)
{
    if (index >= (int)_displayMaxs.size()) {
        throw "Double_Knob::setDisplayMaximum , dimension out of range";
    }
    _displayMaxs[index] = maxi;
    emit displayMinMaxChanged(_displayMins[index], maxi, index);
}

void Double_Knob::setIncrement(double incr, int index)
{
    if(incr <= 0.){
        qDebug() << "Attempting to set the increment of a double param to a value lesser or equal to 0.";
        return;
    }
    if (index >= (int)_increments.size()) {
        throw "Double_Knob::setIncrement , dimension out of range";
    }
   
    _increments[index] = incr;
    emit incrementChanged(_increments[index], index);
}

void Double_Knob::setDecimals(int decis, int index)
{
    if (index >= (int)_decimals.size()) {
        throw "Double_Knob::setDecimals , dimension out of range";
    }

    _decimals[index] = decis;
    emit decimalsChanged(_decimals[index], index);
}

void Double_Knob::getMinMaxForCurve(const Curve* curve,double* min,double* max){
    const std::vector< boost::shared_ptr<Curve> >& curves = getCurves();
    for (U32 i = 0; i < curves.size(); ++i) {
        if (curves[i].get() == curve) {
            const std::vector<double>& mins = getMinimums();
            const std::vector<double>& maxs = getMaximums();
            
            assert(mins.size() > i);
            assert(maxs.size() > i);
            
            *min = mins[i];
            *max = maxs[i];
        }
    }
}

/*minis & maxis must have the same size*/
void Double_Knob::setMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis)
{
    assert(minis.size() == (U32)getDimension() && maxis.size() == (U32)getDimension());
    _minimums = minis;
    _maximums = maxis;
    for (U32 i = 0; i < maxis.size(); ++i) {
        emit minMaxChanged(_minimums[i], _maximums[i], i);
    }
}

void Double_Knob::setDisplayMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis)
{
    assert(minis.size() == (U32)getDimension() && maxis.size() == (U32)getDimension());
    _displayMins = minis;
    _displayMaxs = maxis;
    for (U32 i = 0; i < maxis.size(); ++i) {
        emit displayMinMaxChanged(_minimums[i], _maximums[i], i);
    }
}

void Double_Knob::setIncrement(const std::vector<double> &incr)
{
    assert(incr.size() == (U32)getDimension());
    _increments = incr;
    for (U32 i = 0; i < incr.size(); ++i) {
        emit incrementChanged(_increments[i], i);
    }
}
void Double_Knob::setDecimals(const std::vector<int> &decis)
{
    assert(decis.size() == (U32)getDimension());
    _decimals = decis;
    for (U32 i = 0; i < decis.size(); ++i) {
        emit decimalsChanged(decis[i], i);
    }
}


/******************************BUTTON_KNOB**************************************/

Button_Knob::Button_Knob(KnobHolder  *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
, _renderButton(false)
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

/******************************TABLE_KNOB**************************************/
//
//Table_Knob::Table_Knob(KnobHolder *holder, const std::string &description, int dimension)
//: Knob(holder,description,dimension)
//{
//    
//}
//
//void Table_Knob::appendRow(const std::string& key,const std::vector<std::string> &choices){
//    if((int)_entries.size() == getDimension()){
//        return;
//    }
//    _entries.insert(std::make_pair(key, choices));
//    if((int)_entries.size() == getDimension()){
//        emit populated();
//    }
//}
//
//const Table_Knob::TableEntries& Table_Knob::getRows() const{
//    return _entries;
//}
//
//void Table_Knob::setRows(const Table_Knob::TableEntries& rows){
//    _entries = rows;
//    emit populated();
//}
//
//bool Table_Knob::canAnimate() const{
//    return canAnimateStatic();
//}
//
//void Table_Knob::setVerticalHeaders(const std::string& keyHeader,const std::string& choicesHeader){
//    _keyHeader = keyHeader;
//    _choicesHeader = choicesHeader;
//}
//
//void Table_Knob::getVerticalHeaders(std::string* keyHeader,std::string* choicesHeader){
//    *keyHeader = _keyHeader;
//    *choicesHeader = _choicesHeader;
//}
//
//
//const std::string Table_Knob::_typeNameStr("Table");
//
//const std::string& Table_Knob::typeNameStatic() {
//    return _typeNameStr;
//}
//
//const std::string& Table_Knob::typeName() const {
//    return  typeNameStatic();
//}
//
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
, _multiLine(false)
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

Natron::Status String_Knob::variantToKeyFrameValue(int time,const Variant& v,double* returnValue) {
    StringKeyFrame k;
    k.time = time;
    k.value = v.toString();
    std::pair<Keyframes::iterator,bool> ret = _keyframes.insert(k);
    if(!ret.second){
        _keyframes.erase(ret.first);
        ret = _keyframes.insert(k);
        assert(ret.second);
    }
    *returnValue = std::distance(_keyframes.begin(), ret.first);
    return StatOK;
}

void String_Knob::variantFromInterpolatedValue(double interpolated,Variant* returnValue) const {
    int index = std::floor(interpolated + 0.5);
    int i = 0;
    for (Keyframes::const_iterator it = _keyframes.begin(); it != _keyframes.end(); ++it) {
        if (i == index) {
            returnValue->setValue<QString>(it->value);
            return;
        }
        ++i;
    }
    ///the index is wrong, something is wrong upstream in the knob class
    assert(false);
}

void String_Knob::cloneExtraData(const Knob& other) {
    const String_Knob& o = dynamic_cast<const String_Knob&>(other);
    _keyframes = o._keyframes;
}


static const QString stringSeparatorTag = QString("__SEP__");
static const QString keyframeSepTag = QString("__,__");

void String_Knob::loadExtraData(const QString& str) {
    if (str.isEmpty()) {
        return;
    }
    int sepIndex = str.indexOf(stringSeparatorTag);
    
    int i = 0;
    while (sepIndex != -1) {
        
        int keyFrameSepIndex = str.indexOf(keyframeSepTag,i);
        assert(keyFrameSepIndex != -1);
        
        QString keyframeTime;
        while (i < keyFrameSepIndex) {
            keyframeTime.push_back(str.at(i));
            ++i;
        }
        
        i+= keyframeSepTag.size();
        
        QString keyframevalue;
        while (i < sepIndex) {
            keyframevalue.push_back(str.at(i));
            ++i;
        }
        
        StringKeyFrame k;
        k.time = keyframeTime.toInt();
        k.value = keyframevalue;
        _keyframes.insert(k);
        
        i+= stringSeparatorTag.size();
        sepIndex = str.indexOf(stringSeparatorTag,sepIndex + 1);
    }
}

QString String_Knob::saveExtraData() const {
    if (_keyframes.empty()) {
        return "";
    }
    QString ret;

    for (Keyframes::const_iterator it = _keyframes.begin();it!=_keyframes.end();++it) {
        ret.push_back(QString::number(it->time));
        ret.push_back(keyframeSepTag);
        ret.push_back(it->value);
        ret.push_back(stringSeparatorTag);
    }
    return ret;
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
        boost::shared_ptr<Knob> thisSharedPtr = getHolder()->getKnobByDescription(getDescription());
        assert(thisSharedPtr);
        k->setParentKnob(thisSharedPtr);
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
    for (U32 i = 0; i < _curves.size(); ++i) {
        _curves[i]->setParametricRange(min, max);
    }
}

void Parametric_Knob::getParametricRange(double* min,double* max){
    assert(!_curves.empty());
    _curves.front()->getParametricRange(min, max);
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
    emit curveChanged(dimension);
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
    emit curveChanged(dimension);
    return StatOK;

}

Natron::Status  Parametric_Knob::deleteControlPoint(int   dimension,int   nthCtl){
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    
    _curves[dimension]->removeKeyFrameWithIndex(nthCtl);
    emit curveChanged(dimension);
    return StatOK;
}

Natron::Status  Parametric_Knob::deleteAllControlPoints(int   dimension){
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    _curves[dimension]->clearKeyFrames();
    emit curveChanged(dimension);
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

void Parametric_Knob::cloneExtraData(const Knob& other){
    assert(other.typeName() == typeNameStatic() && other.getDimension() == getDimension());
    const Parametric_Knob& paramKnob = dynamic_cast<const Parametric_Knob&>(other);
    for (int i = 0; i < getDimension(); ++i) {
        _curves[i]->clone(*(paramKnob.getParametricCurve(i)));
    }
}

static const QString kCurveTag = QString("__C__");
static const QString kControlPointTag  = QString("__CP__");
static const QString kEndControlPointTag = QString("__END_CP__");
static const QString kValueSeparator  = QString("_,_");

void Parametric_Knob::loadExtraData(const QString& str) {
    
    
    if(str.isEmpty()){
        return;
    }
    int curveCursor = str.indexOf(kCurveTag);
    while(curveCursor != -1){
        
        int cpCursor = str.indexOf(kControlPointTag,curveCursor);

        ///i is the index at which the first digit of the dimension of the curve is
        int i = curveCursor + kCurveTag.size();
        assert(str.at(i).isDigit());
        QString curveIndexStr;
        
        while( i < cpCursor ){
            assert(i < str.size());
            curveIndexStr.push_back(str.at(i));
            ++i;
        }
        int curveIndex = curveIndexStr.toUInt();
        
        deleteAllControlPoints(curveIndex);

        
        while(cpCursor != -1){
            
            QString key;
            
            ///i is the index at which the first digit of the key is
            i = cpCursor + kControlPointTag.size();
            assert(str.at(i).isDigit());
            
            ///find the value separator
            int valueSep = str.indexOf(kValueSeparator,i);
            assert(valueSep != -1);
            
            while( i < valueSep){
                assert(i < str.size());
                key.append(str.at(i));
                ++i;
            }
            
            ///we now have the key
            
            ///position i at the first digit of the value
            i = valueSep + kValueSeparator.size();
            assert(str.at(i).isDigit());
            
            int endCp = str.indexOf(kEndControlPointTag,valueSep);
            assert(endCp != -1);
            
            QString value ;
            while( i < endCp ){
                assert(i < str.size());
                value.push_back(str.at(i));
                ++i;
            }
            
            _curves[curveIndex]->addKeyFrame(KeyFrame(key.toDouble(), value.toDouble()));
            cpCursor = str.indexOf(kControlPointTag,cpCursor+1);
            int nextCurveIndex = str.indexOf(kCurveTag,curveCursor+1);
            if(cpCursor > nextCurveIndex && nextCurveIndex != -1){
                break;
            }
        }
        emit curveChanged(curveIndex);

        curveCursor = str.indexOf(kCurveTag,curveCursor+1);
    }
}

QString Parametric_Knob::saveExtraData() const {
    QString ret;
    for (U32 i = 0; i < _curves.size(); ++i) {
        ret.append(kCurveTag);
        ret.append(QString::number(i));
        for (KeyFrameSet::const_iterator it = _curves[i]->begin(); it!= _curves[i]->end(); ++it) {
            ret.append(kControlPointTag);
            ret.append(QString::number(it->getTime()));
            ret.append(kValueSeparator);
            ret.append(QString::number(it->getValue()));
            ret.append(kEndControlPointTag);
        }
    }
    return ret;
}

