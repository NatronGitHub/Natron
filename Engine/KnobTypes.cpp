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
                _minimums.push_back(INT_MIN);
            }
            _minimums.push_back(mini);
        }
    }
    int maximum = 99;
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
                _maximums.push_back(INT_MAX);
            }
            _maximums.push_back(maxi);
        }
    }
    int minimum = 99;
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
                _displayMins.push_back(0);
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
                _displayMaxs.push_back(99);
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
                // FIXME: explain what happens here (comment?)
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

std::string Int_Knob::typeName() const
{
    return "Int";
}


/******************************BOOL_KNOB**************************************/

Bool_Knob::Bool_Knob(KnobHolder *holder, const std::string &description, int dimension):
    Knob(holder, description, dimension)
{}

bool Bool_Knob::canAnimate() const
{
    return false;
}

std::string Bool_Knob::typeName() const
{
    return "Bool";
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

std::string Double_Knob::typeName() const
{
    return "Double";
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
                _minimums.push_back(0);
            }
            _minimums.push_back(mini);
        }
    }
    double maximum = 99;
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
                _maximums.push_back(99);
            }
            _maximums.push_back(maxi);
        }
    }
    double minimum = 99;
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
                _displayMins.push_back(0);
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
                _increments.push_back(0.1);  // FIXME: explain with a comment what happens here? why 0.1?
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
                _decimals.push_back(3);
                _decimals.push_back(decis);
            }
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

std::string Button_Knob::typeName() const
{
    return "Button";
}


/******************************COMBOBOX_KNOB**************************************/

ComboBox_Knob::ComboBox_Knob(KnobHolder *holder, const std::string &description, int dimension):
    Knob(holder, description, dimension)
{

}

bool ComboBox_Knob::canAnimate() const
{
    return false;
}

std::string ComboBox_Knob::typeName() const
{
    return "ComboBox";
}

/*Must be called right away after the constructor.*/
void ComboBox_Knob::populate(const std::vector<std::string> &entries, const std::vector<std::string> &entriesHelp)
{
    assert(_entriesHelp.empty() || _entriesHelp.size() == entries.size());
    _entriesHelp = entriesHelp;
    _entries = entries;
    emit populated();
}

const std::vector<std::string> &ComboBox_Knob::getEntries() const
{
    return _entries;
}

const std::vector<std::string> &ComboBox_Knob::getEntriesHelp() const
{
    return _entriesHelp;
}

int ComboBox_Knob::getActiveEntry() const
{
    return getValue<int>();
}

const std::string &ComboBox_Knob::getActiveEntryText() const
{
    return _entries[getActiveEntry()];
}

/******************************SEPARATOR_KNOB**************************************/


Separator_Knob::Separator_Knob(KnobHolder *holder, const std::string &description, int dimension):
    Knob(holder, description, dimension)
{

}

bool Separator_Knob::canAnimate() const
{
    return false;
}

std::string Separator_Knob::typeName() const
{
    return "Separator";
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

std::string Color_Knob::typeName() const
{
    return "Color";
}




String_Knob::String_Knob(KnobHolder *holder, const std::string &description, int dimension):
    Knob(holder, description, dimension)
{

}

bool String_Knob::canAnimate() const
{
    return false;
}

std::string String_Knob::typeName() const
{
    return "String";
}

std::string String_Knob::getString() const
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

std::string Group_Knob::typeName() const
{
    return "Group";
}

void Group_Knob::addKnob(Knob *k)
{
    _children.push_back(k);
}

const std::vector<Knob *> &Group_Knob::getChildren() const
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

std::string Tab_Knob::typeName() const
{
    return "Tab";
}

void Tab_Knob::addTab(const std::string &name)
{
    _knobs.insert(make_pair(name, std::vector<Knob *>()));
}

void Tab_Knob::addKnob(const std::string &tabName, Knob *k)
{
    std::map<std::string, std::vector<Knob *> >::iterator it = _knobs.find(tabName);
    if (it == _knobs.end()) {
        pair<std::map<std::string, std::vector<Knob *> >::iterator, bool> ret = _knobs.insert(make_pair(tabName, std::vector<Knob *>()));
        ret.first->second.push_back(k);
    } else {
        it->second.push_back(k);
    }
}


const std::map<std::string, std::vector<Knob *> > &Tab_Knob::getKnobs() const
{
    return _knobs;
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


std::string RichText_Knob::typeName() const
{
    return "RichText";
}

std::string RichText_Knob::getString() const
{
    return getValue<QString>().toStdString();
}

