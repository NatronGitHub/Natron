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

#ifndef NATRON_ENGINE_KNOBTYPES_H_
#define NATRON_ENGINE_KNOBTYPES_H_

#include <vector>
#include <QObject>

#include "Engine/Knob.h"

#include "Global/Macros.h"

/******************************INT_KNOB**************************************/

class Int_Knob:public Knob
{
    
    Q_OBJECT
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Int_Knob(holder,description,dimension);
    }
    
    Int_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension)
      ,_disableSlider(false)
    {}

    void disableSlider() { _disableSlider = true;}

    bool isSliderDisabled() const {return _disableSlider;}
    
public:
    void setMinimum(int mini,int index = 0){
        if(_minimums.size() > (U32)index){
            _minimums[index] = mini;
        }else{
            if(index == 0){
                _minimums.push_back(mini);
            }else{
                while(_minimums.size() <= (U32)index){
                    _minimums.push_back(INT_MIN);
                }
                _minimums.push_back(mini);
            }
        }
        int maximum = 99;
        if(_maximums.size() > (U32)index){
            maximum = _maximums[index];
        }
        emit minMaxChanged(mini,maximum,index);
    }
    
    void setMaximum(int maxi,int index = 0){
        
        if(_maximums.size() > (U32)index){
            _maximums[index] = maxi;
        }else{
            if(index == 0){
                _maximums.push_back(maxi);
            }else{
                while(_maximums.size() <= (U32)index){
                    _maximums.push_back(INT_MAX);
                }
                _maximums.push_back(maxi);
            }
        }
        int minimum = 99;
        if(_minimums.size() > (U32)index){
            minimum = _minimums[index];
        }
        emit minMaxChanged(minimum,maxi,index);
    }

    void setDisplayMinimum(int mini,int index = 0){
        if(_displayMins.size() > (U32)index){
            _displayMins[index] = mini;
        }else{
            if(index == 0){
                _displayMins.push_back(mini);
            }else{
                while(_displayMins.size() <= (U32)index){
                    _displayMins.push_back(0);
                }
                _displayMins.push_back(mini);
            }
        }
    }
    
    void setDisplayMaximum(int maxi,int index = 0){
        
        if(_displayMaxs.size() > (U32)index){
            _displayMaxs[index] = maxi;
        }else{
            if(index == 0){
                _displayMaxs.push_back(maxi);
            }else{
                while(_displayMaxs.size() <= (U32)index){
                    _displayMaxs.push_back(99);
                }
                _displayMaxs.push_back(maxi);
            }
        }
    }
    
    void setIncrement(int incr, int index = 0) {
        assert(incr > 0);
        /*If _increments is already filled, just replace the existing value*/
        if (_increments.size() > (U32)index) {
            _increments[index] = incr;
        }else{
            /*If it is not filled and it is 0, just push_back the value*/
            if(index == 0){
                _increments.push_back(incr);
            } else {
                /*Otherwise fill enough values until we  have
                 the corresponding index in _increments. Then we
                 can push_back the value as the last element of the
                 vector.*/
                while (_increments.size() <= (U32)index) {
                    // FIXME: explain what happens here (comment?)
                    _increments.push_back(1);
                    assert(_increments[_increments.size()-1] > 0);
                }
                _increments.push_back(incr);
            }
        }
        emit incrementChanged(_increments[index],index);
    }
    
    void setIncrement(const std::vector<int>& incr){
        _increments = incr;
        for (U32 i = 0; i < _increments.size(); ++i) {
            assert(_increments[i] > 0);
            emit incrementChanged(_increments[i],i);
        }
    }
    
    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<int>& minis,const std::vector<int>& maxis){
        _minimums = minis;
        _maximums = maxis;
        for (U32 i = 0; i < maxis.size(); ++i) {
            emit minMaxChanged(_minimums[i], _maximums[i],i);
        }
    }
    
    void setDisplayMinimumsAndMaximums(const std::vector<int>& minis,const std::vector<int>& maxis){
        _displayMins = minis;
        _displayMaxs = maxis;
    }

    const std::vector<int>& getMinimums() const {return _minimums;}
    
    const std::vector<int>& getMaximums() const {return _maximums;}
    
    const std::vector<int>& getIncrements() const {return _increments;}
    
    const std::vector<int>& getDisplayMinimums() const {return _displayMins;}
    
    const std::vector<int>& getDisplayMaximums() const {return _displayMaxs;}
    
 
signals:
    
    void minMaxChanged(int mini,int maxi,int index = 0);
    
    void incrementChanged(int incr,int index = 0);
    
private:
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL {
        switch(dimension){
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

    virtual bool canAnimate() const OVERRIDE FINAL { return true; }

    virtual std::string typeName() const OVERRIDE FINAL {return "Int";}

private:

    std::vector<int> _minimums,_maximums,_increments,_displayMins,_displayMaxs;
    bool _disableSlider;
    
};

/******************************BOOL_KNOB**************************************/

class Bool_Knob:public Knob
{
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Bool_Knob(holder,description,dimension);
    }
    
    Bool_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension)
    {}
    
    virtual bool canAnimate() const OVERRIDE FINAL { return false; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "Bool";}

   
};

/******************************DOUBLE_KNOB**************************************/

class Double_Knob:public Knob
{
    Q_OBJECT
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Double_Knob(holder,description,dimension);
    }
    
    Double_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension)
      ,_disableSlider(false)
    {}

    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL {
        switch(dimension){
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

    void disableSlider() { _disableSlider = true;}

    bool isSliderDisabled() const {return _disableSlider;}
    
    virtual bool canAnimate() const OVERRIDE FINAL { return true; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "Double";}
    
    const std::vector<double>& getMinimums() const {return _minimums;}
    
    const std::vector<double>& getMaximums() const {return _maximums;}
    
    const std::vector<double>& getIncrements() const {return _increments;}
    
    const std::vector<int>& getDecimals() const {return _decimals;}
    
    const std::vector<double>& getDisplayMinimums() const {return _displayMins;}
    
    const std::vector<double>& getDisplayMaximums() const {return _displayMaxs;}
    
    void setMinimum(double mini,int index = 0){
        if(_minimums.size() > (U32)index){
            _minimums[index] = mini;
        }else{
            if(index == 0){
                _minimums.push_back(mini);
            }else{
                while(_minimums.size() <= (U32)index){
                    _minimums.push_back(0);
                }
                _minimums.push_back(mini);
            }
        }
        double maximum = 99;
        if(_maximums.size() > (U32)index){
            maximum = _maximums[index];
        }
        emit minMaxChanged(mini,maximum,index);
    }
    
    void setMaximum(double maxi,int index = 0){
        if(_maximums.size() > (U32)index){
            _maximums[index] = maxi;
        }else{
            if(index == 0){
                _maximums.push_back(maxi);
            }else{
                while(_maximums.size() <= (U32)index){
                    _maximums.push_back(99);
                }
                _maximums.push_back(maxi);
            }
        }
        double minimum = 99;
        if(_minimums.size() > (U32)index){
            minimum = _minimums[index];
        }
        emit minMaxChanged(minimum,maxi,index);
    }
    
    void setDisplayMinimum(double mini,int index = 0){
        if(_displayMins.size() > (U32)index){
            _displayMins[index] = mini;
        }else{
            if(index == 0){
                _displayMins.push_back(DBL_MIN);
            }else{
                while(_displayMins.size() <= (U32)index){
                    _displayMins.push_back(0);
                }
                _displayMins.push_back(mini);
            }
        }
    }
    
    void setDisplayMaximum(double maxi,int index = 0){
        
        if(_displayMaxs.size() > (U32)index){
            _displayMaxs[index] = maxi;
        }else{
            if(index == 0){
                _displayMaxs.push_back(maxi);
            }else{
                while(_displayMaxs.size() <= (U32)index){
                    _displayMaxs.push_back(DBL_MAX);
                }
                _displayMaxs.push_back(maxi);
            }
        }
    }
    
    void setIncrement(double incr, int index = 0) {
        assert(incr > 0.);
        if (_increments.size() > (U32)index) {
            _increments[index] = incr;
        }else{
            if(index == 0){
                _increments.push_back(incr);
            }else{
                while (_increments.size() <= (U32)index) {
                    _increments.push_back(0.1);  // FIXME: explain with a comment what happens here? why 0.1?
                }
                _increments.push_back(incr);
            }
        }
        emit incrementChanged(_increments[index],index);
    }
    
    void setDecimals(int decis,int index = 0){
        if(_decimals.size() > (U32)index){
            _decimals[index] = decis;
        }else{
            if(index == 0){
                _decimals.push_back(decis);
            }else{
                while(_decimals.size() <= (U32)index){
                    _decimals.push_back(3);
                    _decimals.push_back(decis);
                }
            }
        }
        emit decimalsChanged(_decimals[index],index);
    }
    
    
    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<double>& minis,const std::vector<double>& maxis){
        _minimums = minis;
        _maximums = maxis;
        for (U32 i = 0; i < maxis.size(); ++i) {
            emit minMaxChanged(_minimums[i], _maximums[i],i);
        }
    }
    
    void setDisplayMinimumsAndMaximums(const std::vector<double>& minis,const std::vector<double>& maxis){
        _displayMins = minis;
        _displayMaxs = maxis;
    }
    
    void setIncrement(const std::vector<double>& incr){
        _increments = incr;
        for (U32 i = 0; i < incr.size(); ++i) {
            emit incrementChanged(_increments[i],i);
        }
    }
    void setDecimals(const std::vector<int>& decis){
        _decimals = decis;
        for (U32 i = 0; i < decis.size(); ++i) {
            emit decimalsChanged(decis[i],i);
        }
    }
signals:
    void minMaxChanged(double mini,double maxi,int index = 0);
    
    void incrementChanged(double incr,int index = 0);
    
    void decimalsChanged(int deci,int index = 0);
    
    
private:
    
    std::vector<double> _minimums,_maximums,_increments,_displayMins,_displayMaxs;
    std::vector<int> _decimals;
    bool _disableSlider;
    
};

/******************************BUTTON_KNOB**************************************/

class Button_Knob:public Knob
{
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Button_Knob(holder,description,dimension);
    }
    
    Button_Knob(KnobHolder*  holder, const std::string& description,int dimension):
        Knob(holder,description,dimension){
            setPersistent(false);
        }
    
    virtual bool canAnimate() const OVERRIDE FINAL { return false; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "Button";}
    

};

/******************************COMBOBOX_KNOB**************************************/

class ComboBox_Knob:public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new ComboBox_Knob(holder,description,dimension);
    }
    
    ComboBox_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension)
    {
        
    }

    virtual bool canAnimate() const OVERRIDE FINAL { return false; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "ComboBox";}
    
    /*Must be called right away after the constructor.*/
    void populate(const std::vector<std::string>& entries,const std::vector<std::string>& entriesHelp = std::vector<std::string>()){
        assert(_entriesHelp.empty() || _entriesHelp.size() == entries.size());
        _entriesHelp = entriesHelp;
        _entries = entries;
        emit populated();
    }
    
    const std::vector<std::string>& getEntries() const {return _entries;}
    
    const std::vector<std::string>& getEntriesHelp() const {return _entriesHelp;}
    
    int getActiveEntry() const {return getValue<int>();}
    
    const std::string& getActiveEntryText() const { return _entries[getActiveEntry()]; }
    
signals:
    
    void populated();
private:
    std::vector<std::string> _entries;
    std::vector<std::string> _entriesHelp;
};

/******************************SEPARATOR_KNOB**************************************/

class Separator_Knob:public Knob
{
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Separator_Knob(holder,description,dimension);
    }
    
    Separator_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension){
        
    }

    virtual bool canAnimate() const OVERRIDE FINAL { return false; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "Separator";}
    
};
/******************************RGBA_KNOB**************************************/

/**
 * @brief A color knob with of variable dimension. Each color is a double ranging in [0. , 1.]
 * In dimension 1 the knob will have a single channel being a gray-scale
 * In dimension 3 the knob will have 3 channel R,G,B
 * In dimension 4 the knob will have R,G,B and A channels.
**/
class Color_Knob:public Knob
{
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Color_Knob(holder,description,dimension);
    }
    

    Color_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension)
    {
        //dimension greater than 4 is not supported. Dimension 2 doesn't make sense.
        assert(dimension <= 4 && dimension != 2);
    }

    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL {
        switch(dimension){
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
    
    virtual bool canAnimate() const OVERRIDE FINAL { return true; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "Color";}
    
    
};

/******************************STRING_KNOB**************************************/
class String_Knob:public Knob
{
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new String_Knob(holder,description,dimension);
    }
    
    String_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension){
        
    }
    
    virtual bool canAnimate() const OVERRIDE FINAL { return false; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "String";}
    
    std::string getString() const {return getValue<QString>().toStdString();}

};
/******************************GROUP_KNOB**************************************/
class Group_Knob:public Knob
{
    Q_OBJECT
    
    std::vector<Knob*> _children;
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Group_Knob(holder,description,dimension);
    }
    
    Group_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension)
    {
        
    }

    virtual bool canAnimate() const OVERRIDE FINAL { return false; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "Group";}
    
    void addKnob(Knob* k);
    
    const std::vector<Knob*>& getChildren() const {return _children;}

};
/******************************TAB_KNOB**************************************/

class Tab_Knob:public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Tab_Knob(holder,description,dimension);
    }
    
    Tab_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension){
        
    }
    
    virtual bool canAnimate() const OVERRIDE FINAL { return false; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "Tab";}
    
    void addTab(const std::string& typeName);
    
    void addKnob(const std::string& tabName,Knob* k);
    
    const std::map<std::string,std::vector<Knob*> >& getKnobs() const {return _knobs;}

    
private:
    std::map<std::string,std::vector<Knob*> > _knobs;
};

/******************************RichText_Knob**************************************/
class RichText_Knob:public Knob
{
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new RichText_Knob(holder,description,dimension);
    }
    
    RichText_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension){
        
    }
    
    virtual bool canAnimate() const OVERRIDE FINAL { return false; }

    
    virtual std::string typeName() const OVERRIDE FINAL {return "RichText";}
    
    std::string getString() const {return getValue<QString>().toStdString();}

};

#endif // NATRON_ENGINE_KNOBTYPES_H_
