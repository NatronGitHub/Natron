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

#ifndef NATRON_ENGINE_KNOB_H_
#define NATRON_ENGINE_KNOB_H_

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/GlobalDefines.h"
#include "Engine/Variant.h"

class Curve;
class KeyFrame;
class KnobHolder;
class AppInstance;
class KnobSerialization;
/******************************KNOB_BASE**************************************/

class Knob : public QObject
{
    Q_OBJECT
    
    friend class KnobHolder;
    
public:
    
    enum ValueChangedReturnCode {
        NO_KEYFRAME_ADDED = 0,
        KEYFRAME_MODIFIED,
        KEYFRAME_ADDED
    };

    explicit Knob(KnobHolder*  holder,const std::string& description,int dimension = 1);
    
    virtual ~Knob();
    
    /**
     * @brief To be called if you want to remove the knob prematurely from the gui.
     **/
    void remove();

    /**
     * @brief Must return the type name of the knob. This name will be used by the KnobFactory
     * to create an instance of this knob.
    **/
    virtual const std::string& typeName() const = 0;

    /**
     * @brief Must return true if this knob can animate (i.e: if we can set different values depending on the time)
     * Some parameters cannot animate, for example a file selector.
     **/
    virtual bool canAnimate() const = 0;

    /**
      * @brief If the parameter is multidimensional, this is the label thats the that will be displayed
      * for a dimension.
    **/
    virtual std::string getDimensionName(int dimension) const {(void)dimension; return "";}

    /**
     * @brief Used to bracket calls to setValue. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
    **/
    void beginValueChange(Natron::ValueChangedReason reason);

    /**
     * @brief Called by setValue to indicate that an evaluation is needed. This could be private.
     **/
    void evaluateValueChange(int dimension,Natron::ValueChangedReason reason);

    /**
     * @brief Used to bracket calls to setValue. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
    **/
    void endValueChange(Natron::ValueChangedReason reason) ;

    /**
     * @brief Called when a keyframe/derivative is modified, indicating that the curve has changed and we must
     * evaluate any change (i.e: force a new render)
    **/
    void evaluateAnimationChange();

    /**
     * @brief Called on project loading.
    **/
    void load(const KnobSerialization& serializationObj);

    /**
     * @brief Called by load(const KnobSerialization*&) to deserialize your own extra data.
     * You must implement the parsing yourself.
     **/
    virtual void loadExtraData(const QString& /*str*/) {}
    
    /**
     * @brief Called on project save.
    **/
    void save(KnobSerialization* serializationObj) const;
    
    /**
     * @brief Called by save(KnobSerialization*) if you want to serialize more data.
     * You must return a string with your serialized data
     **/
    virtual QString saveExtraData() const { return ""; }
    

    const Variant& getValue(int dimension = 0) const;

    template <typename T>
    T getValue(int dimension = 0) const {
        return getValue(dimension).value<T>();
    }


    void setValue(const Variant& value,int dimension);

    template<typename T>
    void setValue(const T &value,int dimension = 0) {
        setValue(Variant(value),dimension);
    }

    template<typename T>
    void setValue(T variant[],int count){
        for(int i = 0; i < count; ++i){
            setValue(Variant(variant[i]),i);
        }
    }

    /**
     * @brief Set the value for a specific dimension at a specific time. By default dimension
     * is 0. If there's a single dimension, it will set the dimension 0 regardless of the parameter dimension.
     * Otherwise, it will attempt to set a key for only the dimension 'dimensionIndex'.
     **/
    void setValueAtTime(int time,const Variant& v,int dimension);

    template<typename T>
    void setValueAtTime(int time,const T& value,int dimensionIndex = 0){
        assert(dimensionIndex < getDimension());
        setValueAtTime(time,Variant(value),dimensionIndex);
    }

    template<typename T>
    void setValueAtTime(int time,T variant[],int count){
        for(int i = 0; i < count; ++i){
            setValueAtTime(time,Variant(variant[i]),i);
        }
    }

    void deleteValueAtTime(int time,int dimension);

    void removeAnimation(int dimension);
    
    /**
     * @brief Returns the value  in a specific dimension at a specific time. If
     * there is no key in this dimension it will return the value at the requested dimension
     **/
    Variant getValueAtTime(double time, int dimension) const;

    template<typename T>
    T getValueAtTime(double time,int dimension = 0) const {
        return getValueAtTime(time,dimension).value<T>();
    }


    boost::shared_ptr<Curve> getCurve(int dimension = 0) const;

    bool isAnimated(int dimension) const;

    const std::vector< boost::shared_ptr<Curve>  >& getCurves() const;

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

    const std::vector<Variant>& getValueForEachDimension() const ;

    int getDimension() const ;

    void turnOffNewLine();
    
    void setSpacingBetweenItems(int spacing);
    
    void setEnabled(bool b);
    
    void setSecret(bool b);
    
    /*Call this to change the knob name. The name is not the text label displayed on
     the GUI but what is passed to the valueChangedByUser signal. By default the
     name is the same as the description(i.e: the text label).*/
    void setName(const std::string& name);
    
    std::string getName() const;
    
    void setParentKnob(boost::shared_ptr<Knob> knob);
    
    boost::shared_ptr<Knob> getParentKnob() const;
    
    int determineHierarchySize() const;
    
    bool isSecret() const ;
    
    bool isEnabled() const;
    
    void setInsignificant(bool b);
    
    bool isPersistent() const;

    void setPersistent(bool b);

    void turnOffUndoRedo();
    
    bool canBeUndone() const;
    
    bool isInsignificant() const;
    
    void setHintToolTip(const std::string& hint);
    
    const std::string& getHintToolTip() const;

    /**
     * @brief Slaves the curve for the given dimension to the curve
     * at the same dimension for the knob 'other'.
     * In case of success, this function returns true, otherwise false.
    **/
    bool slaveTo(int dimension,boost::shared_ptr<Knob> other);


    /**
     * @brief Unslave the curve at the given dimension if it was previously
     * slaved to another knob using the slaveTo function.
    **/
    void unSlave(int dimension);

    /**
     * @brief Returns a valid pointer to a knob if the curve at
     * the given dimension is slaved.
    **/
    boost::shared_ptr<Knob> isCurveSlave(int dimension) const;


    const std::vector<boost::shared_ptr<Knob> > &getMasters() const;

    /**
     * @brief Called by the GUI whenever the animation level changes (due to a time change
     * or a value changed).
     **/
    void setAnimationLevel(int dimension,Natron::AnimationLevel level);
    
    Natron::AnimationLevel getAnimationLevel(int dimension) const;

    
    /*Set the value of the knob but does NOT emit the valueChanged signal.
     This is called by the GUI.*/
    ValueChangedReturnCode onValueChanged(int dimension,const Variant& variant,KeyFrame* newKey);

public slots:
    
  
    /*Set a keyframe for the knob but does NOT emit the keyframeSet signal.
         This is called by the GUI .*/
    void onKeyFrameSet(SequenceTime time,int dimension);

    void onKeyFrameRemoved(SequenceTime time,int dimension);

    void onTimeChanged(SequenceTime);
    
    void onAnimationRemoved(int dimension);

signals:
    
    ///emitted whenever setAnimationLevel is called. It is meant to notify
    ///openfx params whether it is auto-keying or not.
    void animationLevelChanged(int);
    
    void deleted(Knob*);
    
    /*Emitted when the value is changed internally by a call to setValue*/
    void valueChanged(int dimension);

    void secretChanged();
    
    void enabledChanged();
    
    void restorationComplete();

    void keyFrameSet(SequenceTime,int);

    void keyFrameRemoved(SequenceTime,int);
    
    void animationRemoved(int);
    
    
private:
    //private because it emits a signal
    ValueChangedReturnCode setValue(const Variant& v,int dimension,Natron::ValueChangedReason reason,KeyFrame* newKey);

     //private because it emits a signal
    bool setValueAtTime(int time,const Variant& v,int dimension,Natron::ValueChangedReason reason,KeyFrame* newKey);

     //private because it emits a signal
    void deleteValueAtTime(int time,int dimension,Natron::ValueChangedReason reason);
    
    void removeAnimation(int dimension,Natron::ValueChangedReason reason);
    
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
    
    /** @brief This function is called when a value is changed a the hash for this knob is recomputed.
     * It lets the derived class a chance to add any extra info that would be needed to differentiate 
     * 2 values from each other.
    **/
    virtual void appendExtraDataToHash(std::vector<U64>* /*hash*/) const {}


private:
    struct KnobPrivate;
    boost::scoped_ptr<KnobPrivate> _imp;
};

Q_DECLARE_METATYPE(Knob*)

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
    bool _isClone;
public:
    
    /**
     *@brief A holder is a class managing a bunch of knobs and interacting with an appInstance.
     * When appInstance is NULL the holder will be considered "application global" in which case
     * the knob holder will interact directly with the AppManager singleton.
     **/
    KnobHolder(AppInstance* appInstance);
    
    virtual ~KnobHolder();
    
    bool isClone() const { return _isClone; }

    void setClone() { _isClone = true; }
    
    /**
     * @brief Clone each knob of "other" into this KnobHolder.
     * WARNING: other must have exactly the same number of knobs.
     **/
    void cloneKnobs(const KnobHolder& other);

    AppInstance* getApp() const {return _app;}

    int getAppAge() const;
    
    boost::shared_ptr<Knob> getKnobByDescription(const std::string& desc) const WARN_UNUSED_RETURN;
    
    const std::vector< boost::shared_ptr<Knob> >& getKnobs() const { return _knobs; }

    void refreshAfterTimeChange(SequenceTime time);
    
    /**
     * @brief Used to bracket a series of calls to setValue(...) in case many complex changes are done
     * at once. If not called, notifyProjectEvaluationRequested() will  automatically bracket its call by a begin/end
     * but this can lead to worse performance.
     **/
    void notifyProjectBeginKnobsValuesChanged(Natron::ValueChangedReason reason);
    
    /**
     * @brief Called whenever a value changes. It brakcets the call by a begin/end if it was
     * not done already and requests an evaluation (i.e: probably a render).
     **/
    void notifyProjectEvaluationRequested(Natron::ValueChangedReason reason,Knob* k,bool significant);

    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    void notifyProjectEndKnobsValuesChanged(Natron::ValueChangedReason reason);
    
    
    /**
     * @brief The virtual portion of notifyProjectBeginValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to prepare yourself to a lot of value changes.
     **/
    virtual void beginKnobsValuesChanged(Natron::ValueChangedReason reason){(void)reason;}

    /**
     * @brief The virtual portion of notifyProjectEndKnobsValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to finish a serie of value changes, thus limiting the amount of changes to do.
     **/
    virtual void endKnobsValuesChanged(Natron::ValueChangedReason reason){(void)reason;}

    
    /**
     * @brief The virtual portion of notifyProjectEvaluationRequested(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to do things when a value is changed. Bear in mind that you can compress
     * the change by using the begin/end[ValueChanges] to optimize the changes.
     **/
    virtual void onKnobValueChanged(Knob* k,Natron::ValueChangedReason reason){(void)k;(void)reason;}


    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    virtual void evaluate(Knob* knob,bool isSignificant) = 0;
    
    /*Add a knob to the vector. This is called by the
     Knob class. Don't call this*/
    void addKnob(boost::shared_ptr<Knob> k){ _knobs.push_back(k); }
    
    
    /*Removes a knob to the vector. This is called by the
     Knob class. Don't call this*/
    void removeKnob(Knob* k);
    

    /**
     * @brief Should be implemented by any deriving class that maintains
     * a hash value based on the knobs.
     **/
    void invalidateHash();
    
   
    
private:

    
    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() = 0;


   };


#endif // NATRON_ENGINE_KNOB_H_
