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
#include <QThread>
#include <QCoreApplication>

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"

using namespace Natron;
using std::make_pair;
using std::pair;

/******************************INT_KNOB**************************************/
Int_Knob::Int_Knob(KnobHolder *holder, const std::string &description, int dimension):
Knob(holder, description, dimension)
, _dimensionNames(dimension)
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
        
        switch (i) {
            case 0:
                _dimensionNames[i] = "x";
                break;
            case 1:
                _dimensionNames[i] = "y";
                break;
            case 2:
                _dimensionNames[i] = "z";
                break;
            case 3:
                _dimensionNames[i] = "w";
                break;
            default:
                assert(false); //< unsupported dimension
                break;
        }
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

std::pair<int,int> Int_Knob::getMinMaxForCurve(const Curve* curve) const {
    const std::vector< boost::shared_ptr<Curve> >& curves = getCurves();
    for (U32 i = 0; i < curves.size(); ++i) {
        if (curves[i].get() == curve) {
            const std::vector<int>& mins = getMinimums();
            const std::vector<int>& maxs = getMaximums();
            
            assert(mins.size() > i);
            assert(maxs.size() > i);
            
            return std::make_pair(mins[i], maxs[i]);
        }
    }
    throw std::logic_error("Int_Knob::getMinMaxForCurve(): curve not found");
}

void Int_Knob::setDimensionName(int dim,const std::string& name)
{
    assert(dim < (int)_dimensionNames.size());
    _dimensionNames[dim] = name;
}


std::string
Int_Knob::getDimensionName(int dimension) const
{
    assert(dimension < (int)_dimensionNames.size());
    return _dimensionNames[dimension];
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


Double_Knob::Double_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder, description, dimension)
, _dimensionNames(dimension)
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
        
        switch (i) {
            case 0:
                _dimensionNames[i] = "x";
                break;
            case 1:
                _dimensionNames[i] = "y";
                break;
            case 2:
                _dimensionNames[i] = "z";
                break;
            case 3:
                _dimensionNames[i] = "w";
                break;
            default:
                assert(false); //< unsupported dimension
                break;
        }

    }
}

void Double_Knob::setDimensionName(int dim,const std::string& name)
{
    assert(dim < (int)_dimensionNames.size());
    _dimensionNames[dim] = name;
}


std::string
Double_Knob::getDimensionName(int dimension) const
{
    assert(dimension < (int)_dimensionNames.size());
    return _dimensionNames[dimension];
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

std::pair<double,double> Double_Knob::getMinMaxForCurve(const Curve* curve) const {
    const std::vector< boost::shared_ptr<Curve> >& curves = getCurves();
    for (U32 i = 0; i < curves.size(); ++i) {
        if (curves[i].get() == curve) {
            const std::vector<double>& mins = getMinimums();
            const std::vector<double>& maxs = getMaximums();
            
            assert(mins.size() > i);
            assert(maxs.size() > i);
            
            return std::make_pair(mins[i],maxs[i]);
        }
    }
    throw std::logic_error("Double_Knob::getMinMaxForCurve(): curve not found");
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
    setIsPersistant(false);
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
void Choice_Knob::populateChoices(const std::vector<std::string> &entries, const std::vector<std::string> &entriesHelp)
{
    assert(entriesHelp.empty() || entriesHelp.size() == entries.size());
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

const std::string &Choice_Knob::getActiveEntryText() const
{
    int activeIndex = getValue();
    assert(activeIndex < (int)_entries.size());
    return _entries[activeIndex];
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

Color_Knob::Color_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder, description, dimension)
, _allDimensionsEnabled(true)
, _dimensionNames(dimension)
, _minimums(dimension)
, _maximums(dimension)
, _displayMins(dimension)
, _displayMaxs(dimension)
{
    //dimension greater than 4 is not supported. Dimension 2 doesn't make sense.
    assert(dimension <= 4 && dimension != 2);
    for (int i = 0; i < dimension; ++i) {
        _minimums[i] = 0.;
        _maximums[i] = 1.;
        _displayMins[i] = 0.;
        _displayMaxs[i] = 1.;
        
        switch (i) {
            case 0:
                _dimensionNames[i] = "r";
                break;
            case 1:
                _dimensionNames[i] = "g";
                break;
            case 2:
                _dimensionNames[i] = "b";
                break;
            case 3:
                _dimensionNames[i] = "a";
                break;
            default:
                assert(false); //< unsupported dimension
                break;
        }

    }
}

void Color_Knob::onDimensionSwitchToggled(bool b)
{
    _allDimensionsEnabled = b;
}

bool Color_Knob::areAllDimensionsEnabled() const
{
    return _allDimensionsEnabled;
}

void Color_Knob::setDimensionName(int dim,const std::string& dimension)
{
    assert(dim < (int)_dimensionNames.size());
    _dimensionNames[dim] = dimension;
}

// FIXME: the plugin may have set kOfxParamPropDimensionLabel - use this!
std::string
Color_Knob::getDimensionName(int dimension) const
{
    assert(dimension < (int)_dimensionNames.size());
    return _dimensionNames[dimension];
}

bool
Color_Knob::canAnimate() const
{
    return true;
}

const std::string
Color_Knob::_typeNameStr("Color");

const std::string&
Color_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string&
Color_Knob::typeName() const
{
    return typeNameStatic();
}

const std::vector<double> &
Color_Knob::getMinimums() const
{
    return _minimums;
}

const std::vector<double> &
Color_Knob::getMaximums() const
{
    return _maximums;
}

const std::vector<double> &
Color_Knob::getDisplayMinimums() const
{
    return _displayMins;
}

const std::vector<double> &
Color_Knob::getDisplayMaximums() const
{
    return _displayMaxs;
}

void
Color_Knob::setMinimum(double mini, int index)
{
    if (index >= (int)_minimums.size()) {
        throw "Color_Knob::setMinimum , dimension out of range";
    }
    _minimums[index] = mini;
    emit minMaxChanged(mini, _maximums[index], index);
}

void
Color_Knob::setMaximum(double maxi, int index)
{
    if (index >= (int)_maximums.size()) {
        throw "Color_Knob::setMaximum , dimension out of range";
    }
    _maximums[index] = maxi;
    emit minMaxChanged(_minimums[index], maxi, index);
}

void
Color_Knob::setDisplayMinimum(double mini, int index)
{
    if (index >= (int)_displayMins.size()) {
        throw "Color_Knob::setDisplayMinimum , dimension out of range";
    }
    _displayMins[index] = mini;
    emit displayMinMaxChanged(mini, _displayMaxs[index], index);

}

void
Color_Knob::setDisplayMaximum(double maxi, int index)
{
    if (index >= (int)_displayMaxs.size()) {
        throw "Color_Knob::setDisplayMaximum , dimension out of range";
    }
    _displayMaxs[index] = maxi;
    emit displayMinMaxChanged(_displayMins[index], maxi, index);
}

std::pair<double,double>
Color_Knob::getMinMaxForCurve(const Curve* curve) const {
    const std::vector< boost::shared_ptr<Curve> >& curves = getCurves();
    for (U32 i = 0; i < curves.size(); ++i) {
        if (curves[i].get() == curve) {
            const std::vector<double>& mins = getMinimums();
            const std::vector<double>& maxs = getMaximums();

            assert(mins.size() > i);
            assert(maxs.size() > i);

            return std::make_pair(mins[i],maxs[i]);
        }
    }
    throw std::logic_error("Color_Knob::getMinMaxForCurve(): curve not found");
}

/*minis & maxis must have the same size*/
void
Color_Knob::setMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis)
{
    assert(minis.size() == (U32)getDimension() && maxis.size() == (U32)getDimension());
    _minimums = minis;
    _maximums = maxis;
    for (U32 i = 0; i < maxis.size(); ++i) {
        emit minMaxChanged(_minimums[i], _maximums[i], i);
    }
}

void
Color_Knob::setDisplayMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis)
{
    assert(minis.size() == (U32)getDimension() && maxis.size() == (U32)getDimension());
    _displayMins = minis;
    _displayMaxs = maxis;
    for (U32 i = 0; i < maxis.size(); ++i) {
        emit displayMinMaxChanged(_minimums[i], _maximums[i], i);
    }
}

/******************************STRING_KNOB**************************************/


String_Knob::String_Knob(KnobHolder *holder, const std::string &description, int dimension):
AnimatingString_KnobHelper(holder, description, dimension)
, _multiLine(false)
, _isLabel(false)
, _isCustom(false)
{
    
}

String_Knob::~String_Knob() {}

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

void Group_Knob::addKnob(boost::shared_ptr<KnobI> k)
{
    std::vector<boost::shared_ptr<KnobI> >::iterator found = std::find(_children.begin(), _children.end(), k);
    if(found == _children.end()){
        _children.push_back(k);
        boost::shared_ptr<KnobI> thisSharedPtr = getHolder()->getKnobByName(getName());
        assert(thisSharedPtr);
        k->setParentKnob(thisSharedPtr);
    }
    
}

const std::vector< boost::shared_ptr<KnobI> > &Group_Knob::getChildren() const
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



void Tab_Knob::addKnob(boost::shared_ptr<KnobI> k)
{
    std::vector<boost::shared_ptr<KnobI> >::iterator found = std::find(_children.begin(), _children.end(), k);
    if(found == _children.end()){
        _children.push_back(k);
        k->setParentKnob(getHolder()->getKnobByName(getName()));
    }
}


/******************************Parametric_Knob**************************************/


Parametric_Knob::Parametric_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder,description,dimension)
, _curvesMutex()
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
    
    ///only called in the main thread
    assert(QThread::currentThread() == qApp->thread());
    ///Mt-safe as it never changes
    
    assert(dimension < (int)_curvesColor.size());
    _curvesColor[dimension].r = r;
    _curvesColor[dimension].g = g;
    _curvesColor[dimension].b = b;
}

void Parametric_Knob::getCurveColor(int dimension,double* r,double* g,double* b){
    
    ///Mt-safe as it never changes

    assert(dimension < (int)_curvesColor.size());
    *r = _curvesColor[dimension].r ;
    *g = _curvesColor[dimension].g ;
    *b = _curvesColor[dimension].b ;
}

void Parametric_Knob::setCurveLabel(int dimension,const std::string& str){
    
    ///only called in the main thread
    assert(QThread::currentThread() == qApp->thread());
    ///Mt-safe as it never changes

    assert(dimension < (int)_curveLabels.size());
    _curveLabels[dimension] = str;
}

const std::string& Parametric_Knob::getCurveLabel(int dimension) const{
    
    ///Mt-safe as it never changes

    assert(dimension < (int)_curveLabels.size());
    return _curveLabels[dimension];
}

void Parametric_Knob::setParametricRange(double min,double max){
    
    ///only called in the main thread
    assert(QThread::currentThread() == qApp->thread());
    ///Mt-safe as it never changes

    for (U32 i = 0; i < _curves.size(); ++i) {
        _curves[i]->setXRange(min, max);
    }
}

std::pair<double,double> Parametric_Knob::getParametricRange() const
{
    ///Mt-safe as it never changes

    assert(!_curves.empty());
    return _curves.front()->getXRange();
}

std::string Parametric_Knob::getDimensionName(int dimension) const {
    ///Mt-safe as it never changes
    return getCurveLabel(dimension);
}

boost::shared_ptr<Curve> Parametric_Knob::getParametricCurve(int dimension) const{
    ///Mt-safe as Curve is MT-safe and the pointer is never deleted

    assert(dimension < (int)_curves.size());
    return _curves[dimension];
}

Natron::Status Parametric_Knob::addControlPoint(int dimension,double key,double value){
    ///Mt-safe as Curve is MT-safe
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    
    _curves[dimension]->addKeyFrame(KeyFrame(key,value));
    emit curveChanged(dimension);
    return StatOK;
}

Natron::Status Parametric_Knob::getValue(int dimension,double parametricPosition,double *returnValue){
    ///Mt-safe as Curve is MT-safe
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

Natron::Status Parametric_Knob::getNControlPoints(int dimension,int *returnValue)
{
    ///Mt-safe as Curve is MT-safe
    if (dimension >= (int)_curves.size()) {
        return StatFailed;
    }
    *returnValue =  _curves[dimension]->getKeyFramesCount();
    return StatOK;
}

Natron::Status Parametric_Knob::getNthControlPoint(int dimension,
                                  int    nthCtl,
                                  double *key,
                                  double *value){
    ///Mt-safe as Curve is MT-safe
    if(dimension >= (int)_curves.size()){
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

Natron::Status Parametric_Knob::setNthControlPoint(int   dimension,
                                  int   nthCtl,
                                  double key,
                                  double value)
{
    ///Mt-safe as Curve is MT-safe
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    _curves[dimension]->setKeyFrameValueAndTime(key, value, nthCtl);
    emit curveChanged(dimension);
    return StatOK;

}

Natron::Status  Parametric_Knob::deleteControlPoint(int   dimension,int   nthCtl){
    
    ///Mt-safe as Curve is MT-safe
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    
    _curves[dimension]->removeKeyFrameWithIndex(nthCtl);
    emit curveChanged(dimension);
    return StatOK;
}

Natron::Status  Parametric_Knob::deleteAllControlPoints(int   dimension){
    ///Mt-safe as Curve is MT-safe
    if(dimension >= (int)_curves.size()){
        return StatFailed;
    }
    _curves[dimension]->clearKeyFrames();
    emit curveChanged(dimension);
    return StatOK;
}


void Parametric_Knob::cloneExtraData(const boost::shared_ptr<KnobI>& other){
    ///Mt-safe as Curve is MT-safe
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(other.get());
    if (isParametric && isParametric->getDimension() == getDimension()) {
        for (int i = 0; i < getDimension(); ++i) {
            _curves[i]->clone(*isParametric->getParametricCurve(i));
        }
    }
}

void Parametric_Knob::saveParametricCurves(std::list< Curve >* curves) const
{
    for (U32 i = 0; i < _curves.size(); ++i) {
        curves->push_back(*_curves[i]);
    }
}

void Parametric_Knob::loadParametricCurves(const std::list< Curve >& curves)
{ 
    assert(!_curves.empty());
    int i = 0;
    for (std::list< Curve >::const_iterator it = curves.begin(); it!=curves.end(); ++it) {
        _curves[i]->clone(*it);
        ++i;
    }
}
