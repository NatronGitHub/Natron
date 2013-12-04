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

#ifndef KNOB_H
#define KNOB_H

#include <vector>
#include <string>
#include <map>
#include <cfloat>

#include <QtCore/QMutex>

#include "Global/GlobalDefines.h"
#include "Engine/Curve.h"

class Knob;
class holder;
class DockablePanel;
class AppInstance;

namespace Natron {
class LibraryBinary;
}

/******************************KNOB_FACTORY**************************************/
//Maybe the factory should move to a separate file since it is used to create KnobGui aswell
class KnobGui;
class KnobHolder;
class KnobFactory{
    
    std::map<std::string,Natron::LibraryBinary*> _loadedKnobs;
    
public:
    KnobFactory();
    
    ~KnobFactory();
    
    const std::map<std::string,Natron::LibraryBinary*>& getLoadedKnobs() const {return _loadedKnobs;}
    
    Knob* createKnob(const std::string& id, KnobHolder* holder, const std::string& description,int dimension = 1) const;
    
    KnobGui* createGuiForKnob(Knob* knob,DockablePanel* container) const;
    
private:
    
    void loadKnobPlugins();
    
    void loadBultinKnobs();
    
};


/******************************KNOB_BASE**************************************/

struct KnobPrivate;
class Knob : public QObject, public AnimatingParam
{
    Q_OBJECT
    
public:

    
    explicit Knob(KnobHolder*  holder,const std::string& description,int dimension = 1);
    
    /**
     * @brief Never delete a knob on your ownn. If you need to delete a knob pre-emptivly
     * see Knob::remove() , otherwise the KnobFactory will take care of allocation / deallocation
     * for you;
    **/
    virtual ~Knob();
    
    /**
     * @brief Call this if you want to remove the Knob dynamically (for example in response to
     * another knob value changed). You must not call the delete operator as the factory is
     * responsible for allocation/deallocation.
     **/
    void remove();
    
        
    /**
     * @brief Must return the type name of the knob. This name will be used by the KnobFactory
     * to create an instance of this knob.
    **/
    virtual const std::string typeName()=0;

    /**
     * @brief Must return true if this knob can animate (i.e: if we can set different values depending on the time)
     * Some parameters cannot animate, for example a file selector.
     **/
    virtual bool canAnimate() const = 0;

    /**
      * @brief If the parameter is multidimensional, this is the label thats the that will be displayed
      * for a dimension.
    **/
    virtual std::string getDimensionName(int dimension) const{(void)dimension; return "";}

    /**
      * @brief Must be implemented to evaluate a value that has changed at the given dimension.
      * The reason can be of any type : time changed, user edited or plugin edited.
    **/
    virtual void evaluateValueChange(int dimension,AnimatingParam::ValueChangedReason reason) OVERRIDE FINAL;

    /**
     * @brief Used to bracket calls to evaluateValueChange. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
    **/
    virtual void beginValueChange(AnimatingParam::ValueChangedReason reason) OVERRIDE FINAL;

    /**
     * @brief Used to bracket calls to evaluateValueChange. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
    **/
    virtual void endValueChange(AnimatingParam::ValueChangedReason reason) OVERRIDE FINAL;

    virtual void evaluateAnimationChange() OVERRIDE FINAL;

    /**
     * @brief Called on project loading. This copies the value from AnimatingParam to the knob.
    **/
    void onStartupRestoration(const AnimatingParam& other);

    void turnOffAnimation() ;
    
    bool isAnimationEnabled() const;

    const std::string& getDescription() const;

    const std::vector<U64>& getHashVector() const;

    KnobHolder* getHolder() const;

    /**
     * @brief Called by a render task to copy all the knobs values to another separate instance
     * to decorellate the values modified by the gui (i.e the user) and the values used to render.
     * This ensures thread-safety.
    **/
    void cloneValue(const Knob& other);
    
    void turnOffNewLine();
    
    void setSpacingBetweenItems(int spacing);
    
    void setEnabled(bool b);
    
    void setSecret(bool b);
    
    /*Call this to change the knob name. The name is not the text label displayed on
     the GUI but what is passed to the valueChangedByUser signal. By default the
     name is the same as the description(i.e: the text label).*/
    void setName(const std::string& name);
    
    std::string getName() const;
    
    void setParentKnob(Knob* knob);
    
    Knob* getParentKnob() const ;
    
    int determineHierarchySize() const;
    
    bool isSecret() const ;
    
    bool isEnabled() const ;
    
    void setIsInsignificant(bool b);
    
    bool isPersistent() const;
    
    void setPersistent(bool b);
    
    void turnOffUndoRedo() ;
    
    bool canBeUndone() const ;
    
    bool isInsignificant() const;
    
    void setHintToolTip(const std::string& hint);
    
    const std::string& getHintToolTip() const ;


public slots:
    
    /*Set the value of the knob but does NOT emit the valueChanged signal.
     This is called by the GUI hence does not change the value of any
     render thread storage.*/
    void onValueChanged(int dimension,const Variant& variant);
    
//    void onKnobUndoneChange();
    
//    void onKnobRedoneChange();

    void onTimeChanged(SequenceTime);
        

signals:
    
    void deleted();
    
    /*Emitted when the value is changed internally by a call to setValue*/
    void valueChanged(int dimension);

    void secret(bool);
    
    void enabled(bool);
    
    void restorationComplete();
    
protected:
    
    
    /** @brief This function can be implemented if you want to clone more data than just the value
     * of the knob. Cloning happens when a render request is made: all knobs values of the GUI
     * are cloned into small copies in order to be sure they will not be modified further on.
     * This function is useful if you need to copy for example an extra bit of information.
     * e.g: the File_Knob has to copy not only the QStringList containing the file names, but
     * also the sequence it has parsed.
    **/
    virtual void cloneExtraData(const Knob& other){(void)other;}
    
    /** @brief This function is called right after that the _value has changed
     * but before any signal notifying that it has changed. It can be useful
     * to do some processing to create new informations.
     * e.g: The File_Knob parses the files list to create a mapping of
     * <time,file> .
    **/
    virtual void processNewValue(){}


private:
          
    boost::scoped_ptr<KnobPrivate> _imp;
};

/**
 * @brief A Knob holder is a class that stores Knobs and interact with them in some way.
 * It serves 2 purpose:
 * 1) It automatically deletes the knobs, you don't have to manually call delete
 * 2) It calls a set of begin/end valueChanged whenever a knob value changed. It also
 * calls evaluate() which should then trigger an evaluation of the freshly changed value
 * (i.e force a new render).
 **/
class KnobHolder {
    
    AppInstance* _app;
    std::vector< boost::shared_ptr<Knob> > _knobs;
    bool _betweenBeginEndParamChanged;
    bool _insignificantChange;
    std::vector <Knob*> _knobsChanged;
public:

    friend class Knob;
    
    KnobHolder(AppInstance* appInstance);
    
    virtual ~KnobHolder();
    
    /**
     * @brief Clone each knob of "other" into this KnobHolder.
     * WARNING: other must have exactly the same number of knobs.
     **/
    void cloneKnobs(const KnobHolder& other);
    
    AppInstance* getApp() const {return _app;}
    
    bool wasBeginCalled() const { return _betweenBeginEndParamChanged; }

    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() = 0;
    
    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    virtual void evaluate(Knob* knob,bool isSignificant) = 0;
    
    /**
     * @brief Should be implemented by any deriving class that maintains
     * a hash value based on the knobs.
     **/
    void invalidateHash();
    
    int getAppAge() const;
    
    const std::vector< boost::shared_ptr<Knob> >& getKnobs() const { return _knobs; }
    
    void beginValuesChanged(AnimatingParam::ValueChangedReason reason, bool isSignificant);
    
    void endValuesChanged(AnimatingParam::ValueChangedReason reason);
    
    void onValueChanged(Knob* k,AnimatingParam::ValueChangedReason reason,bool isSignificant);
    
    void refreshAfterTimeChange(SequenceTime time);
    
protected:
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(AnimatingParam::ValueChangedReason reason){(void)reason;}
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(AnimatingParam::ValueChangedReason reason){(void)reason;}
    
    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void onKnobValueChanged(Knob* k,AnimatingParam::ValueChangedReason reason){(void)k;(void)reason;}

    
private:
    
    void triggerAutoSave();
    
    /*Add a knob to the vector. This is called by the
     Knob class.*/
    void addKnob(boost::shared_ptr<Knob> k){ _knobs.push_back(k); }
    
    /*Removes a knob to the vector. This is called by the
     Knob class.*/
    void removeKnob(Knob* k);

};

/******************************FILE_KNOB**************************************/

class File_Knob:public Knob
{
    Q_OBJECT
    
    mutable QMutex _fileSequenceLock;
    std::map<int,QString> _filesSequence;///mapping <frameNumber,fileName>
public:
    
    static Knob* BuildKnob(KnobHolder*  holder, const std::string& description,int dimension){
        return new File_Knob(holder,description,dimension);
    }
    
    File_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension)
    {}
    
    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "InputFile";}
    
    void openFile(){
        emit shouldOpenFile();
    }
    
    /**
     * @brief firstFrame
     * @return Returns the index of the first frame in the sequence held by this Reader.
     */
    int firstFrame() const;
    
    /**
     * @brief lastFrame
     * @return Returns the index of the last frame in the sequence held by this Reader.
     */
    int lastFrame() const;
    
    int frameCount() const{return _filesSequence.size();}
    
    /**
     * @brief nearestFrame
     * @return Returns the index of the nearest frame in the Range [ firstFrame() - lastFrame( ].
     * @param f The index of the frame to modify.
     */
    int nearestFrame(int f) const;
    
    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    QString getRandomFrameName(int f,bool loadNearestIfNotFound) const;
    
    virtual void cloneExtraData(const Knob& other);
    
    virtual void processNewValue();
    
signals:
    void shouldOpenFile();
    

};

/******************************OUTPUT_FILE_KNOB**************************************/

class OutputFile_Knob:public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new OutputFile_Knob(holder,description,dimension);
    }
    
    OutputFile_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension)
    {}
    
    virtual bool canAnimate() const { return false; }
    
    std::string getFileName() const;
    
    virtual const std::string typeName(){return "OutputFile";}
    
    void openFile(){
        emit shouldOpenFile();
    }
    
    
signals:
    
    void shouldOpenFile();

};

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

    virtual std::string getDimensionName(int dimension) const{
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
    
    virtual bool canAnimate() const { return true; }
    
    virtual const std::string typeName(){return "Int";}
    
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
    
    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "Bool";}

   
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

    virtual std::string getDimensionName(int dimension) const{
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
    
    virtual bool canAnimate() const { return true; }
    
    virtual const std::string typeName(){return "Double";}
    
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
    
    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "Button";}
    

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

    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "ComboBox";}
    
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

    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "Separator";}
    
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

    virtual std::string getDimensionName(int dimension) const{
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
    
    virtual bool canAnimate() const { return true; }
    
    virtual const std::string typeName(){return "Color";}
    
    
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
    
    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "String";}
    
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

    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "Group";}
    
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
    
    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "Tab";}
    
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
    
    virtual bool canAnimate() const { return false; }

    
    virtual const std::string typeName(){return "RichText";}
    
    std::string getString() const {return getValue<QString>().toStdString();}

};

#endif // KNOB_H
