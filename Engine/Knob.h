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
#include <QtCore/QStringList>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/GlobalDefines.h"
#include "Global/AppManager.h"

#include "Engine/Variant.h"
#include "Engine/Rect.h"
#include "Engine/Interpolation.h"

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


/**
 * @brief A KeyFrame is a pair <time,value>. These are the value that are used
 * to interpolate an AnimationCurve. The _leftTangent and _rightTangent can be
 * used by the interpolation method of the curve.
**/
class KeyFrame  {
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Time",_time);
        ar & boost::serialization::make_nvp("Value",_value);
        ar & boost::serialization::make_nvp("LeftTangent",_leftTangent);
        ar & boost::serialization::make_nvp("RightTangent",_rightTangent);
    }
    
public:

    typedef std::pair<double,Variant> Tangent;

    KeyFrame()
    : _value()
    , _time(0)
    , _interpolation(Natron::KEYFRAME_LINEAR)
    {}
    
    KeyFrame(double time,const Variant& initialValue);
    
    KeyFrame(const KeyFrame& other)
    : _value(other._value)
    , _time(other._time)
    , _leftTangent(other._leftTangent)
    , _rightTangent(other._rightTangent)
    , _interpolation(Natron::KEYFRAME_LINEAR)
    {
        
    }

    ~KeyFrame(){}
    
    const Variant& getValue() const { return _value; }
    
    double getTime() const { return _time; }
    
    const Tangent& getLeftTangent() const { return _leftTangent; }
    
    const Tangent& getRightTangent() const { return _rightTangent; }

    void setLeftTangent(double time,const Variant& v){
        _leftTangent.first = time;
        _leftTangent.second = v;
    }
    
    void setRightTangent(double time,const Variant& v){
        _rightTangent.first = time;
        _rightTangent.second = v;
    }
    
    void setValue(const Variant& v){
        _value = v;
    }
    
    void setTime(double time){
        _time = time;
    }

    void setInterpolation(Natron::KeyframeType interp) { _interpolation = interp; }
    
    Natron::KeyframeType getInterpolation() const { return _interpolation; }
    
    bool operator==(const KeyFrame& other) const {
        return _value == other._value &&
        _time == other._time &&
        _leftTangent.first == other._leftTangent.first &&
        _leftTangent.second == other._leftTangent.second &&
        _rightTangent.first == other._rightTangent.first &&
        _rightTangent.second == other._rightTangent.second &&
        _interpolation == other._interpolation;
    }
    
private:
    
    
    Variant _value; /// the value held by the key
    double _time; /// a value ranging between 0 and 1
    
    Tangent _leftTangent,_rightTangent;
    Natron::KeyframeType _interpolation;

};

/**
  * @brief A CurvePath is a list of chained curves. Each curve is a set of 2 keyFrames and has its
  * own interpolation method (that can differ from other curves).
**/
class CurvePath : public QObject {
    
    Q_OBJECT
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("KeyFrames",_keyFrames);
    }
    
public:
    
    
    //each segment of curve between a keyframe and the next can have a different interpolation method
    typedef std::list< KeyFrame > KeyFrames;


    enum InterpolableType{
        INT = 0,
        DOUBLE = 1
    };
    
    CurvePath(){}
    
    CurvePath(const CurvePath& other):
     _keyFrames(other._keyFrames)
    ,_bbox()
    ,_betweendBeginAndEndRecord(false)
    {}

    CurvePath(const KeyFrame& cp);

    ~CurvePath(){}

    bool operator==(const CurvePath& other) const{
        const KeyFrames& otherKeys = other.getKeyFrames();
        KeyFrames::const_iterator itOther = otherKeys.begin();
        for(KeyFrames::const_iterator it = _keyFrames.begin();it!=_keyFrames.end();++it){
            if(!((*it) == (*itOther))){
                return false;
            }
            if(itOther == otherKeys.end())
                return false;
            ++itOther;
        }
    }

    bool isAnimated() const { return _keyFrames.size() > 1; }
    
    void addControlPoint(const KeyFrame& cp);

    void setStart(const KeyFrame& cp);

    void setEnd(const KeyFrame& cp);

    int getControlPointsCount() const { return (int)_keyFrames.size(); }

    double getMinimumTimeCovered() const;

    double getMaximumTimeCovered() const;

    const KeyFrame& getStart() const { assert(!_keyFrames.empty()); return _keyFrames.front(); }

    const KeyFrame& getEnd() const { assert(!_keyFrames.empty()); return _keyFrames.back(); }
    
    void beginRecordBoundingBox() const { _betweendBeginAndEndRecord = true; _bbox.clear(); }

    Variant getValueAt(double t) const;
    
    void endRecordBoundingBox() const { _betweendBeginAndEndRecord = false; }
    
    const RectD& getBoundingBox() const { return _bbox; }
    
    const KeyFrames& getKeyFrames() const { return _keyFrames; }

    void refreshTangents(KeyFrames::iterator key);
    
signals:
    
    void keyFrameAdded();
    
    void keyFrameRemoved();
    
    void keyFrameChanged();
    
private:


    template <typename T>
    T getValueAtInternal(double t) const {

        if(_keyFrames.size() == 1){
            //if there's only 1 keyframe, don't bother interpolating
            return (*_keyFrames.begin()).getValue().value<T>();
        }

        KeyFrames::const_iterator upper = _keyFrames.begin();
        for(KeyFrames::const_iterator it = _keyFrames.begin();it!=_keyFrames.end();++it){
            if((*it).getTime() > t){
                upper = it;
                break;
            }else if((*it).getTime() == t){
                //if the time is exactly the time of a keyframe, return its value
                return (*it).getValue().value<T>();
            }
        }

        //if we found no key that has a greater time (i.e: we search before the 1st keyframe)
        if(upper == _keyFrames.begin()){
            if((*upper).getInterpolation() == Natron::KEYFRAME_CONSTANT || (*upper).getInterpolation() == Natron::KEYFRAME_LINEAR){
                return (*upper).getValue().value<T>();
            }else{
                const KeyFrame& key = (*upper);
                //for all other methods, interpolate linearly in the direction of the tangent
                const KeyFrame::Tangent& tangent = key.getLeftTangent();
                T keyFrameValue = key.getValue().value<T>();
                return ((std::abs(tangent.second.value<T>() - keyFrameValue) * std::abs(key.getTime() - t)) /
                        (std::abs(key.getTime() - tangent.first))) + keyFrameValue;
            }
        }


        //if all keys have a greater time (i.e: we search after the last keyframe)
        KeyFrames::const_iterator prev = upper;
        --prev;
        assert(prev != _keyFrames.end());

        if(upper == _keyFrames.end()){

            if((*prev).getInterpolation() == Natron::KEYFRAME_CONSTANT || (*prev).getInterpolation() == Natron::KEYFRAME_LINEAR){
                return (*prev).getValue().value<T>();
            }else{
                const KeyFrame& key = (*prev);
                //for all other methods, interpolate linearly in the direction of the tangent
                const KeyFrame::Tangent& tangent = key.getRightTangent();
                T keyFrameValue = key.getValue().value<T>();
                return ((std::abs(tangent.second.value<T>() - keyFrameValue) * std::abs(key.getTime() - t)) /
                        (std::abs(key.getTime() - tangent.first))) + keyFrameValue;
            }
        }
        
        double t0,t3;
        T v0,v1,v2,v3;
        t0 = (*prev).getTime();
        t3 = (*upper).getTime();
        
        v0 = (*prev).getValue().value<T>();
        v1 = (*prev).getRightTangent().second.value<T>();
        v2 = (*upper).getLeftTangent().second.value<T>();
        v3 = (*upper).getValue().value<T>();
        
        //normalize t relatively to t0, t3
        t = (t - t0) / (t3 - t0);
        t0 = 0.;
        t3 = 1.;
        
        return Natron::interpolate<T>(t0,v0,v1,v2,t3,v3,t,(*prev).getInterpolation());
        
        
    }
    
    KeyFrames _keyFrames;
    mutable RectD _bbox;
    mutable bool _betweendBeginAndEndRecord;
};



/******************************KNOB_BASE**************************************/

/**
 * @brief A class based on Variant that can store a value across multiple dimension
 * and also have specific "keyframes".
 **/
class MultidimensionalValue {
    
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        (void)version;
        ar & boost::serialization::make_nvp("Values",_value);
        ar & boost::serialization::make_nvp("Curves",_curves);
    }
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Values",_value);
        ar & boost::serialization::make_nvp("Curves",_curves);
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    
    
public:
    /// for each dimension,there's a list of chained curve. The list is always sorted
    /// which means that the curve at index i-1 is linked to the curve at index i.
    /// Note that 2 connected curves share a pointer to the same keyframe.
    typedef std::map<int, CurvePath > CurvesMap;
    
    MultidimensionalValue(int dimension = 1)
    : _dimension(dimension)
    {
        //default initialize the values map
        for(int i = 0; i < dimension ; ++i){
            _value.insert(std::make_pair(i,Variant()));
            _curves.insert(std::make_pair(i,CurvePath()));
        }
    }
    
    MultidimensionalValue(const MultidimensionalValue& other):
    _value(other._value)
    ,_dimension(other._dimension)
    ,_curves(other._curves)
    {
        
    }
    
    ~MultidimensionalValue() { _value.clear(); _curves.clear(); }
    
    const std::map<int,Variant>& getValueForEachDimension() const {return _value;}
    
    int getDimension() const {return _dimension;}
    
    void setValue(const Variant& v,int dimension);
    
    const Variant& getValue(int dimension) const;
    
    const CurvePath& getCurve(int dimension) const;
    
    void setValueAtTime(double time,const Variant& v,int dimension);
    
    Variant getValueAtTime(double time, int dimension) const;
    
    RectD getCurvesBoundingBox() const;
private:
    
  
    
    /* A variant storing all the values of the knob in any dimension. <dimension,value>*/
    std::map<int,Variant> _value;
    
    int _dimension;
    
    /*A map storing for each dimension the keys of the value. For instance a Double_Knob of dimension 2
     would have 2 Keys in its map, 1 for dimension 0 and 1 for dimension 1.
     The Keys are an ordered map of <time,value> where value is the type requested for the knob (i.e: double
     for a double knob).*/
    CurvesMap _curves; /// the keys for a specific dimension
};

class Knob : public QObject
{
    Q_OBJECT
    
public:
    

    enum ValueChangedReason{USER_EDITED = 0,PLUGIN_EDITED = 1};
    
    Knob(KnobHolder*  holder,const std::string& description,int dimension = 1);
    
    virtual ~Knob();
    
    const std::string& getDescription() const { return _description; }
    
    const std::vector<U64>& getHashVector() const { return _hashVector; }
    
    KnobHolder*  getHolder() const { return _holder; }
    
    int getDimension() const {return _value.getDimension();}
    
    /*Must return the type name of the knob. This name will be used by the KnobFactory
     to create an instance of this knob.*/
    virtual const std::string typeName()=0;
    
    void onStartupRestoration(const MultidimensionalValue& other);

    
    template<typename T>
    void setValue(T variant[],int count){
        for(int i = 0; i < count; ++i){
            setValueInternal(Variant(variant[i]),i);
        }
    }
    
    template<typename T>
    void setValue(const T &value,int dimension = 0) {
        setValueInternal(Variant(value),dimension);
    }
    
    /*Used to extract the value held by the knob.
     Derived classes should provide a more appropriate
     way to retrieve results in the expected type.*/
    template<typename T>
    T getValue(int dimension = 0) const {
        return _value.getValue(dimension).value<T>();
    }
    
    const MultidimensionalValue& getValue() const { return _value; }
    
    const std::map<int,Variant>& getValueForEachDimension() const { return _value.getValueForEachDimension(); }

    RectD getCurvesBoundingBox() const { return _value.getCurvesBoundingBox(); }
    
    /**
     * @brief Must return true if this knob can animate (i.e: if we can set different values depending on the time)
     * Some parameters cannot animate, for example a file selector.
     **/
    virtual bool canAnimate() const = 0;
    
    void ifAnimatingTurnOffAnimation() { _isAnimationEnabled = false; }
    
    bool isAnimationEnabled() const { return canAnimate() && _isAnimationEnabled; }
    
    /**
     * @brief Set the value of a knob for a specific dimension at a specific time. By default dimension
     * is 0. If the knob has only 1 dimension, it will set the dimension 0 regardless of the parameter dimension.
     * Otherwise, it will attempt to set a key for only the dimension 'dimensionIndex' of the knob.
     **/
    template<typename T>
    void setValueAtTime(double time,const T& value,int dimensionIndex = 0){
        assert(dimensionIndex < getDimension());
        assert(canAnimate());
        _value.setValueAtTime(time,Variant(value),dimensionIndex);
    }

    /**
     * @brief Set the value for the knob in all dimensions at time 'time'.
     **/
    template<typename T>
    void setValueAtTime(double time,T variant[],int count){
        assert(canAnimate());
        for(int i = 0; i < count; ++i){
            _value.setValueAtTime(time,Variant(variant[i]),i);
        }
    }

    /**
     * @brief Returns the value of the knob in a specific dimension at a specific time. If
     * the knob has no keys in this dimension it will return the value at the requested dimension
     * stored in _value. Type type parameter must be the type of the knob, i.e: double for a Double_Knob.
     * The type of the Variant can never be cast to QList<QVariant>.
     **/
    template<typename T>
    T getValueAtTime(double time,int dimension = 0){
        assert(canAnimate());
        return _value.getValueAtTime(time,dimension).value<T>();
    }

    /**
     * @brief Returns an ordered map of all the keys at a specific dimension.
     **/
    const CurvePath&  getCurve(int dimension = 0) const;
    
    /*other must have exactly the same name*/
    void cloneValue(const Knob& other);
    
    void turnOffNewLine();
    
    void setSpacingBetweenItems(int spacing);
    
    void setEnabled(bool b);
    
    void setVisible(bool b);
    
    /*Call this to change the knob name. The name is not the text label displayed on
     the GUI but what is passed to the valueChangedByUser signal. By default the
     name is the same as the description(i.e: the text label).*/
    void setName(const std::string& name){_name = QString(name.c_str());}
    
    std::string getName() const {return _name.toStdString();}
    
    void setParentKnob(Knob* knob){_parentKnob = knob;}
    
    Knob* getParentKnob() const {return _parentKnob;}
    
    int determineHierarchySize() const;
    
    bool isVisible() const {return _visible;}
    
    bool isEnabled() const {return _enabled;}
    
    void setIsInsignificant(bool b){_isInsignificant = b;}
    
    void turnOffUndoRedo() {_canUndo = false;}
    
    bool canBeUndone() const {return _canUndo;}
    
    bool isInsignificant() const {return _isInsignificant;}
    
    void setHintToolTip(const std::string& hint){_tooltipHint = hint;}
    
    const std::string& getHintToolTip() const {return _tooltipHint;}

    /**
      * @brief If the parameter is multidimensional, this is the label thats the that will be displayed
      * for a dimension.
    **/
    virtual std::string getDimensionName(int dimension) const{(void)dimension; return "";}

public slots:
    
    /*Set the value of the knob but does NOT emit the valueChanged signal.
     This is called by the GUI hence does not change the value of any
     render thread storage.*/
    void onValueChanged(int dimension,const Variant& variant);
    
    void onKnobUndoneChange();
    
    void onKnobRedoneChange();
    
signals:
    
    void deleted();
    
    /*Emitted when the value is changed internally by a call to setValue*/
    void valueChanged(int dimension,const Variant&);

    void visible(bool);
    
    void enabled(bool);
    
    
protected:
    
    
    /*This function can be implemented if you want to clone more data than just the value
     of the knob. Cloning happens when a render request is made: all knobs values of the GUI
     are cloned into small copies in order to be sure they will not be modified further on.
     This function is useful if you need to copy for example an extra bit of information.
     e.g: the File_Knob has to copy not only the QStringList containing the file names, but
     also the sequence it has parsed.*/
    virtual void cloneExtraData(const Knob& other){(void)other;}
    
    /*This function is called right after that the _value has changed
     but before any signal notifying that it has changed. It can be useful
     to do some processing to create new informations.
     e.g: The File_Knob parses the files list to create a mapping of
     <time,file> .*/
    virtual void processNewValue(){}


private:
    
    void fillHashVector(); // function to add the specific values of the knob to the values vector.

    void updateHash();
    
    void setValueInternal(const Variant& v,int dimension);
    
    
    KnobHolder*  _holder;
    std::vector<U64> _hashVector;
    MultidimensionalValue _value;
    std::string _description;//< the text label that will be displayed  on the GUI
    QString _name;//< the knob can have a name different than the label displayed on GUI.
    //By default this is the same as _description but can be set by calling setName().
    bool _newLine;
    int _itemSpacing;
    
    Knob* _parentKnob;
    bool _visible;
    bool _enabled;
    bool _canUndo;
    bool _isInsignificant; //< if true, a value change will never trigger an evaluation
    std::string _tooltipHint;
    bool _isAnimationEnabled;

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

public:
    friend class Knob;
    
    KnobHolder(AppInstance* appInstance):
        _app(appInstance)
      , _knobs()
      , _betweenBeginEndParamChanged(false){}
    
    virtual ~KnobHolder(){
        _knobs.clear();
    }
    
    /**
     * @brief Clone each knob of "other" into this KnobHolder.
     * WARNING: other must have exactly the same number of knobs.
     **/
    void cloneKnobs(const KnobHolder& other);
    
    AppInstance* getApp() const {return _app;}
    
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
    
    void beginValuesChanged(Knob::ValueChangedReason reason);
    
    void endValuesChanged(Knob::ValueChangedReason reason);
    
    void onValueChanged(Knob* k,Knob::ValueChangedReason reason);
    
protected:
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(Knob::ValueChangedReason reason){(void)reason;}
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(Knob::ValueChangedReason reason){(void)reason;}
    
    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void onKnobValueChanged(Knob* k,Knob::ValueChangedReason reason){(void)k;(void)reason;}

    
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
        for (U32 i = 0; i < incr.size(); ++i) {
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
        Knob(holder,description,dimension){}
    
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
    
private:
    QStringList _entries;
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
