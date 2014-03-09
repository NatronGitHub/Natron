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

#include "Engine/Variant.h"
#include "Engine/AppManager.h"

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
    virtual const std::string& typeName() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Must return true if this knob can animate (i.e: if we can set different values depending on the time)
     * Some parameters cannot animate, for example a file selector.
     **/
    virtual bool canAnimate() const WARN_UNUSED_RETURN = 0;

    /**
      * @brief If the parameter is multidimensional, this is the label thats the that will be displayed
      * for a dimension.
    **/
    virtual std::string getDimensionName(int dimension) const WARN_UNUSED_RETURN {(void)dimension; return "";}

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
    void endValueChange() ;

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
    virtual QString saveExtraData() const WARN_UNUSED_RETURN { return ""; }
    

    Variant getValue(int dimension = 0) const WARN_UNUSED_RETURN;

    template <typename T>
    T getValue(int dimension = 0) const WARN_UNUSED_RETURN;

    void setValue(const Variant& value,int dimension,bool turnOffAutoKeying = false);

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
    virtual Variant getValueAtTime(double time, int dimension) const WARN_UNUSED_RETURN;

    template<typename T>
    T getValueAtTime(double time,int dimension = 0) const WARN_UNUSED_RETURN;

    /// compute the derivative at time as a double
    double getDerivativeAtTime(double time, int dimension = 0) const WARN_UNUSED_RETURN;

    /// compute the integral of dimension from time1 to time2 as a double
    double getIntegrateFromTimeToTime(double time1, double time2, int dimension = 0) const WARN_UNUSED_RETURN;

    /**
     * @brief Places in time the keyframe time at the given index.
     * If it exists the function returns true, false otherwise.
     **/
    bool getKeyFrameTime(int index,int dimension,double* time) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Convenience function, does the same as getKeyFrameByIndex but returns the last
     * keyframe.
     **/
    bool getLastKeyFrameTime(int dimension,double* time) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Convenience function, does the same as getKeyFrameByIndex but returns the first
     * keyframe.
     **/
    bool getFirstKeyFrameTime(int dimension,double* time) const WARN_UNUSED_RETURN;
    
    int getKeyFramesCount(int dimension) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Returns the nearest keyframe time if it was found.
     * Returns true if it succeeded, false otherwise.
     **/
    bool getNearestKeyFrameTime(int dimension,double time,double* nearestTime) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Returns the keyframe index if there's any keyframe in the curve 
     * at the given dimension and the given time. -1 otherwise.
     **/
    int getKeyFrameIndex(int dimension, double time) const WARN_UNUSED_RETURN;
    
    bool getKeyFrameValueByIndex(int dimension,int index,Variant* value) const WARN_UNUSED_RETURN;


    boost::shared_ptr<Curve> getCurve(int dimension = 0) const WARN_UNUSED_RETURN;

    /**
     * @brief Returns true if the dimension is animated with keyframes.
     **/
    bool isAnimated(int dimension) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Returns true if at least 1 dimension is animated.
     **/
    bool hasAnimation() const WARN_UNUSED_RETURN;

    const std::vector< boost::shared_ptr<Curve>  >& getCurves() const WARN_UNUSED_RETURN;

    void setAnimationEnabled(bool val) ;

    /**
     * @brief Returns true if the animation is enabled for this knob. A return value of
     * true doesn't necessarily means that the knob is animated at all.
     **/
    bool isAnimationEnabled() const WARN_UNUSED_RETURN;

    const std::string& getDescription() const WARN_UNUSED_RETURN;

    KnobHolder* getHolder() const WARN_UNUSED_RETURN;

    /**
     * @brief Called by a render task to copy all the knobs values to another separate instance
     * to decorellate the values modified by the gui (i.e the user) and the values used to render.
     * This ensures thread-safety.
    **/
    void cloneValue(const Knob& other);

    const std::vector<Variant>& getValueForEachDimension() const WARN_UNUSED_RETURN;

    int getDimension() const WARN_UNUSED_RETURN;

    void turnOffNewLine();
    
    bool isNewLineTurnedOff() const WARN_UNUSED_RETURN;
    
    void setSpacingBetweenItems(int spacing);
    
    void setEnabled(int dimension,bool b);
    
    void setAllDimensionsEnabled(bool b);
    
    void setSecret(bool b);
    
    /*Call this to change the knob name. The name is not the text label displayed on
     the GUI but what is passed to the valueChangedByUser signal. By default the
     name is the same as the description(i.e: the text label).*/
    void setName(const std::string& name);
    
    std::string getName() const WARN_UNUSED_RETURN;
    
    void setParentKnob(boost::shared_ptr<Knob> knob);
    
    boost::shared_ptr<Knob> getParentKnob() const WARN_UNUSED_RETURN;
    
    int determineHierarchySize() const WARN_UNUSED_RETURN;
    
    bool getIsSecret() const WARN_UNUSED_RETURN;
    
    bool isEnabled(int dimension) const WARN_UNUSED_RETURN;
    
    void setEvaluateOnChange(bool b);
    
    bool getIsPersistant() const WARN_UNUSED_RETURN;

    void setIsPersistant(bool b);

    void setCanUndo(bool val);
    
    bool getCanUndo() const WARN_UNUSED_RETURN;
    
    bool getEvaluateOnChange() const WARN_UNUSED_RETURN;
    
    void setHintToolTip(const std::string& hint);
    
    const std::string& getHintToolTip() const WARN_UNUSED_RETURN;

    /**
     * @brief Slaves the value for the given dimension to the curve
     * at the same dimension for the knob 'other'.
     * In case of success, this function returns true, otherwise false.
    **/
    bool slaveTo(int dimension,boost::shared_ptr<Knob> other,int otherDimension);


    /**
     * @brief Unslave the value at the given dimension if it was previously
     * slaved to another knob using the slaveTo function.
    **/
    void unSlave(int dimension);

    /**
     * @brief Returns a valid pointer to a knob if the value at
     * the given dimension is slaved.
    **/
    std::pair<int,boost::shared_ptr<Knob> > getMaster(int dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief Returns true if the value at the given dimension is slave to another parameter
     **/
    bool isSlave(int dimension) const WARN_UNUSED_RETURN;
    

    const std::vector<std::pair<int,boost::shared_ptr<Knob> > > &getMasters() const WARN_UNUSED_RETURN;

    /**
     * @brief Called by the GUI whenever the animation level changes (due to a time change
     * or a value changed).
     **/
    void setAnimationLevel(int dimension,Natron::AnimationLevel level);
    
    Natron::AnimationLevel getAnimationLevel(int dimension) const WARN_UNUSED_RETURN;

    
    /*Set the value of the knob but does NOT emit the valueChanged signal.
     This is called by the GUI.*/
    ValueChangedReturnCode onValueChanged(int dimension,const Variant& variant,KeyFrame* newKey);
    
    /**
     * @brief Set the default values for the knob. The vector 'values' must have exactly the same size
     * as the dimension of the knob. 
     * @see int getDimension()
     **/
    void setDefaultValues(const std::vector<Variant>& values);
    
    
    void setDefaultValue(const Variant& v,int dimension);
    
    template <typename T>
    void setDefaultValue(const T& v,int dimension = 0) {
        setDefaultValue(Variant(v),dimension);
    }
    
    /**
     * @brief Restores the default value
     **/
    void resetToDefaultValue(int dimension);
    
    
    virtual bool isTypeCompatible(const Knob& other) const WARN_UNUSED_RETURN = 0;
public slots:
    
  
    /*Set a keyframe for the knob but does NOT emit the keyframeSet signal.
         This is called by the GUI .*/
    void onKeyFrameSet(SequenceTime time,int dimension);

    void onKeyFrameRemoved(SequenceTime time,int dimension);

    void onTimeChanged(SequenceTime);
    
    void onAnimationRemoved(int dimension);

    void triggerUpdateSlaves(int dimension) { emit updateSlaves(dimension); }
    
    void onMasterChanged(int);
    
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
    
    void updateSlaves(int dimension);
    
    void readOnlyChanged(bool,int);
    
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
    
    
    /**
     * @brief Called when a keyframe is removed.
     **/
    virtual void keyframeRemoved_virtual(int /*dimension*/, double /*time*/) {}
    
    /**
     * @brief Called when all keyframes are removed
     **/
    virtual void animationRemoved_virtual(int /*dimension*/) {}
    
    /** @brief This function is called right after that the _value has changed
     * but before any signal notifying that it has changed. It can be useful
     * to do some processing to create new informations.
     * e.g: The File_Knob parses the files list to create a mapping of
     * <time,file> .
    **/
    virtual void processNewValue(){}
    
    /**
     * @brief Override to return the real value to store in a keyframe for the variant passed in parameter.
     * This function is almost only addressing the problem of animating strings. The string knob just returns
     * in the return value the index of the string from the variant's current value.
     * In case you implement this function, make sure to return Natron::StatOk, otherwise this function call 
     * will get ignored.
     **/
    virtual Natron::Status variantToKeyFrameValue(int /*time*/,const Variant& /*v*/,double* /*returnValue*/) {
        return Natron::StatReplyDefault;
    }
    
    /**
     * @brief Override to return the real value of the variant (which should have the same type as
     * all other elements in this knob (i.e: String for a string knob).
     * This function is almost only addressing the problem of animating strings. The string knob just returns
     * in the return value the string at the given index contained in interpolated.
     **/
    virtual void variantFromInterpolatedValue(double interpolated,Variant* returnValue) const;

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
    bool _knobsInitialized;
public:
    
    /**
     *@brief A holder is a class managing a bunch of knobs and interacting with an appInstance.
     * When appInstance is NULL the holder will be considered "application global" in which case
     * the knob holder will interact directly with the AppManager singleton.
     **/
    KnobHolder(AppInstance* appInstance);
    
    virtual ~KnobHolder();
    
    bool isClone() const WARN_UNUSED_RETURN { return _isClone; }

    void setClone() { _isClone = true; }
    
    template<typename K>
    boost::shared_ptr<K> createKnob(const std::string &description, int dimension = 1) const WARN_UNUSED_RETURN;

    /**
     * @brief Clone each knob of "other" into this KnobHolder.
     * WARNING: other must have exactly the same number of knobs.
     **/
    void cloneKnobs(const KnobHolder& other);

    AppInstance* getApp() const WARN_UNUSED_RETURN {return _app;}

    int getAppAge() const WARN_UNUSED_RETURN;
    
    boost::shared_ptr<Knob> getKnobByDescription(const std::string& desc) const WARN_UNUSED_RETURN;
    
    const std::vector< boost::shared_ptr<Knob> >& getKnobs() const WARN_UNUSED_RETURN { return _knobs; }

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
    void notifyProjectEndKnobsValuesChanged();
    
    
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
    
    void invalidateHash();
    
    
    void initializeKnobsPublic();
    
private:

    
    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() = 0;


   };

template <typename T>
T Knob::getValue(int dimension) const
{
    return getValue(dimension).value<T>();
}

template<typename T>
T Knob::getValueAtTime(double time,int dimension) const
{
    return getValueAtTime(time,dimension).value<T>();
}

template<typename K>
boost::shared_ptr<K> KnobHolder::createKnob(const std::string &description, int dimension) const
{
    return Natron::createKnob<K>(this, description,dimension);
}

#endif // NATRON_ENGINE_KNOB_H_
