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

class KnobI
{
public:
    
    KnobI(){}
    
    virtual ~KnobI(){}
    
    
    /**
     * @brief Do not call this. It is called right away after the constructor by the factory
     * to initialize curves and values. This is separated from the constructor as we need RTTI
     * for Curve.
     **/
    virtual void populate() = 0;
    
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
    virtual std::string getDimensionName(int dimension) const = 0;
    
    /**
     * @brief Used to bracket calls to setValue. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
     **/
    virtual void beginValueChange(Natron::ValueChangedReason reason) = 0;
    
    /**
     * @brief Called by setValue to indicate that an evaluation is needed. This could be private.
     **/
    virtual void evaluateValueChange(int dimension,Natron::ValueChangedReason reason) = 0;
    
    /**
     * @brief Used to bracket calls to setValue. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
     **/
    virtual void endValueChange() = 0;
    
    /**
     * @brief Called when a keyframe/derivative is modified, indicating that the curve has changed and we must
     * evaluate any change (i.e: force a new render)
     **/
    virtual void evaluateAnimationChange() = 0;
    
    
    /**
     * @brief Called on project save.
     **/
    virtual void save(KnobSerialization* serializationObj) const = 0;
    
    /**
     * @brief Called on project loading.
     **/
    virtual void load(const KnobSerialization& serializationObj) = 0;
    
    /**
     * @brief This cannot be done in load since we need all nodes in the project to be loaded
     * in order to do this.
     **/
    virtual void restoreSlaveMasterState(const KnobSerialization& serializationObj) = 0;

    
protected:
    
    /**
     * @brief Removes all the keyframes in the given dimension.
     **/
    virtual void removeAnimation(int dimension,Natron::ValueChangedReason reason) = 0;

    /**
     * @brief Removes the keyframe at the given time and dimension if it matches any.
     **/
    virtual void deleteValueAtTime(int time,int dimension,Natron::ValueChangedReason reason) = 0;
    
public:
    
    /**
     * @brief Calls deleteValueAtTime with a reason of Natron::PLUGIN_EDITED.
     **/
    void deleteValueAtTime(int time,int dimension);
    
    /**
     * @brief Calls removeAnimation with a reason of Natron::PLUGIN_EDITED.
     **/
    void removeAnimation(int dimension);
    
    /**
     * @brief Calls deleteValueAtTime with a reason of Natron::USER_EDITED
     **/
    void onKeyFrameRemoved(SequenceTime time,int dimension);
    
    /**
     * @brief Calls removeAnimation with a reason of Natron::USER_EDITED
     **/
    void onAnimationRemoved(int dimension);

    /**
     * @brief Called when the current time of the timeline changes. 
     * It must get the value at the given time and notify  the gui it must
     * update the value displayed.
     **/
    virtual void onTimeChanged(SequenceTime time) = 0;
    
    /**
     * @brief Compute the derivative at time as a double
     **/
    virtual double getDerivativeAtTime(double time, int dimension = 0) const = 0;
    
    /**
     * @brief Compute the integral of dimension from time1 to time2 as a double
     **/
    virtual double getIntegrateFromTimeToTime(double time1, double time2, int dimension = 0) const = 0;
    
    /**
     * @brief Places in time the keyframe time at the given index.
     * If it exists the function returns true, false otherwise.
     **/
    virtual bool getKeyFrameTime(int index,int dimension,double* time) const = 0;
    
    /**
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the last
     * keyframe.
     **/
    virtual bool getLastKeyFrameTime(int dimension,double* time) const = 0;
    
    /**
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the first
     * keyframe.
     **/
    virtual bool getFirstKeyFrameTime(int dimension,double* time) const = 0;
    
    /**
     * @brief Returns the count of keyframes in the given dimension.
     **/
    virtual int getKeyFramesCount(int dimension) const = 0;
    
    /**
     * @brief Returns the nearest keyframe time if it was found.
     * Returns true if it succeeded, false otherwise.
     **/
    virtual bool getNearestKeyFrameTime(int dimension,double time,double* nearestTime) const = 0;
    
    /**
     * @brief Returns the keyframe index if there's any keyframe in the curve
     * at the given dimension and the given time. -1 otherwise.
     **/
    virtual int getKeyFrameIndex(int dimension, double time) const = 0;
    
    /**
     * @brief Returns a pointer to the curve in the given dimension. 
     * It cannot be a null pointer.
     **/
    virtual boost::shared_ptr<Curve> getCurve(int dimension = 0) const = 0;
    
    /**
     * @brief Returns true if the dimension is animated with keyframes.
     **/
    virtual bool isAnimated(int dimension) const = 0;
    
    /**
     * @brief Returns true if at least 1 dimension is animated. MT-Safe
     **/
    virtual bool hasAnimation() const = 0;
    
    /**
     * @brief Returns a const ref to the curves held by this knob. This is MT-safe as they're
     * never deleted (except on program exit).
     **/
    virtual const std::vector< boost::shared_ptr<Curve>  >& getCurves() const = 0;
    
    /**
     * @brief Activates or deactivates the animation for this parameter. On the GUI side that means
     * the user can never interact with the animation curves nor can he/she set any keyframe.
     **/
    virtual void setAnimationEnabled(bool val) = 0 ;
    
    /**
     * @brief Returns true if the animation is enabled for this knob. A return value of
     * true doesn't necessarily means that the knob is animated at all.
     **/
    virtual bool isAnimationEnabled() const = 0;
    
    /**
     * @brief Get the knob description, that is the label next to the knob on the user interface.
     * This function is MT-safe as the description NEVER changes throughout the program.
     **/
    virtual const std::string& getDescription() const = 0;
    
    /**
     * @brief Returns a pointer to the holder owning the knob.
     **/
    virtual KnobHolder* getHolder() const = 0;
    
    /**
     * @brief Called by a render task to copy all the knobs values to another separate instance
     * to decorellate the values modified by the gui (i.e the user) and the values used to render.
     * This ensures thread-safety.
     **/
    virtual void cloneValue(const KnobI& other) = 0;
    
    /**
     * @brief Get the knob dimension. MT-safe as it is static and never changes.
     **/
    virtual int getDimension() const = 0;
    
    /**
     * @brief Any GUI representing this parameter should represent the next parameter on the same line as this parameter.
     **/
    virtual void turnOffNewLine() = 0;
    
    /**
     * @brief Any GUI representing this parameter should represent the next parameter on the same line as this parameter.
     **/
    virtual bool isNewLineTurnedOff() const = 0;
    
    /**
     * @brief GUI-related
     **/
    virtual void setSpacingBetweenItems(int spacing) = 0;
    
    /**
     * @brief Enables/disables user interaction with the given dimension.
     **/
    virtual void setEnabled(int dimension,bool b) = 0;
    
    /**
     * @brief Is the dimension enabled ?
     **/
    virtual bool isEnabled(int dimension) const = 0;
    
    /**
     * @brief Convenience function, same as calling setEnabled(int,bool) for all dimensions.
     **/
    virtual void setAllDimensionsEnabled(bool b) = 0;
    
    /**
     * @brief Set the knob visible/invisible on the GUI representing it.
     **/
    virtual void setSecret(bool b) = 0;
    
    /**
     * @brief Is the knob visible to the user ?
     **/
    virtual bool getIsSecret() const = 0;
    
    /**
     * @brief Call this to change the knob name. The name is not the text label displayed on
     * the GUI but what Natron uses internally to identify knobs from each other. By default the
     * name is the same as the description(i.e: the text label).
     */
    virtual void setName(const std::string& name) = 0;
    
    /**
     * @brief Returns the knob name. By default the
     * name is the same as the description(i.e: the text label).
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Set the given knob as the parent of this knob. 
     * @param knob It must be a tab or group knob.
     */
    virtual void setParentKnob(boost::shared_ptr<KnobI> knob) = 0;
    
    /**
     * @brief Returns a pointer to the parent knob if any.
     */
    virtual boost::shared_ptr<KnobI> getParentKnob() const = 0;
    
    /**
     * @brief Returns the hierarchy level of this knob.
     * By default it is 0, however a knob with a parent will
     * have a hierarchy level of 1, etc...
     **/
    virtual int determineHierarchySize() const = 0;

    /**
     * @brief If a knob is evaluating on change, that means
     * everytime a value changes, the knob will call the 
     * evaluate function on the KnobHolder holding it.
     * By default this is set to true.
     **/
    virtual void setEvaluateOnChange(bool b) = 0;
    
    /**
     * @brief Does the knob evaluates on change ?
     **/
    virtual bool getEvaluateOnChange() const = 0;

    
    /**
     * @brief Should the knob be saved in the project ? This is MT-safe
     * because it never changes throughout the object's life-time.
     **/
    virtual bool getIsPersistant() const = 0;
    
    /**
     * @brief Should the knob be saved in the project ? 
     * By default this is set to true.
     **/
    virtual void setIsPersistant(bool b) = 0;
    
    /**
     * @brief If set to false, the knob will not be able to use undo/redo actions.
     * By default this is set to true.
     **/
    virtual void setCanUndo(bool val) = 0;
    
    /**
     * @brief Can the knob use undo/redo actions ?
     **/
    virtual bool getCanUndo() const = 0;
    
    /**
     * @brief Set the text displayed by the tooltip when
     * the user hovers the knob with the mouse.
     **/
    virtual void setHintToolTip(const std::string& hint) = 0;
    
    /**
     * @brief Get the tooltip text.
     **/
    virtual const std::string& getHintToolTip() const = 0;
    
protected:
    
    /**
     * @brief Slaves the value for the given dimension to the curve
     * at the same dimension for the knob 'other'.
     * In case of success, this function returns true, otherwise false.
     **/
    virtual bool slaveTo(int dimension,const boost::shared_ptr<KnobI>&  other,int otherDimension,Natron::ValueChangedReason reason) = 0;
    
    /**
     * @brief Unslaves a previously slaved dimension. The implementation should assert that
     * the dimension was really slaved.
     **/
    virtual void unSlave(int dimension,Natron::ValueChangedReason reason) = 0;
    
public:
    
    /**
     * @brief Calls slaveTo with a value changed reason of Natron::PLUGIN_EDITED.
     **/
    bool slaveTo(int dimension,const boost::shared_ptr<KnobI>& other,int otherDimension);
    
    /**
     * @brief Calls slaveTo with a value changed reason of Natron::USER_EDITED.
     **/
    void onKnobSlavedTo(int dimension,const boost::shared_ptr<KnobI>&  other,int otherDimension);
    
    
    /**
     * @brief Calls unSlave with a value changed reason of Natron::PLUGIN_EDITED.
     **/
    void unSlave(int dimension);
    
    /**
     * @brief Calls unSlave with a value changed reason of Natron::USER_EDITED.
     **/
    void onKnobUnSlaved(int dimension);
    
    /**
     * @brief Returns a valid pointer to a knob if the value at
     * the given dimension is slaved.
     **/
    virtual std::pair<int,boost::shared_ptr<KnobI> > getMaster(int dimension) const = 0;
    
    /**
     * @brief Returns true if the value at the given dimension is slave to another parameter
     **/
    virtual bool isSlave(int dimension) const = 0;
    
    /**
     * @brief Same as getMaster but for all dimensions.
     **/
    virtual std::vector<std::pair<int,boost::shared_ptr<KnobI> > > getMasters_mt_safe() const = 0;
    
    /**
     * @brief Called by the GUI whenever the animation level changes (due to a time change
     * or a value changed).
     **/
    virtual void setAnimationLevel(int dimension,Natron::AnimationLevel level) = 0;
    
    /**
     * @brief Get the current animation level.
     **/
    virtual Natron::AnimationLevel getAnimationLevel(int dimension) const = 0;
    
    /**
     * @brief Restores the default value
     **/
    virtual void resetToDefaultValue(int dimension) = 0;
    
    /**
     * @brief Must return true if the other knobs type can convert to this knob's type.
     **/
    virtual bool isTypeCompatible(const KnobI& other) const = 0;

};

#if 0

class KnobSignalEmitter : public QObject
{
public:
    
};

template <typename T>
class Knob
{
    friend class KnobHolder;
    
public:
    
    enum ValueChangedReturnCode {
        NO_KEYFRAME_ADDED = 0,
        KEYFRAME_MODIFIED,
        KEYFRAME_ADDED
    };

    
    explicit Knob(KnobHolder*  holder,const std::string& description,int dimension = 1);
    
    T getValue(int dimension = 0) const WARN_UNUSED_RETURN;
    
    virtual T getValueAtTime(double time, int dimension = 0) const WARN_UNUSED_RETURN;

private:
    
    ValueChangedReturnCode setValue(const Variant& v,int dimension,Natron::ValueChangedReason reason,KeyFrame* newKey);
    
    bool setValueAtTime(int time,const Variant& v,int dimension,Natron::ValueChangedReason reason,KeyFrame* newKey);

public:
    
    void setValue(const T& value,int dimension,bool turnOffAutoKeying = false);
    
    ValueChangedReturnCode onValueChanged(int dimension,const T& variant,KeyFrame* newKey);
    
    void setValueAtTime(int time,const T& v,int dimension);
    
    void onKeyFrameSet(SequenceTime time,int dimension);
    
    bool getKeyFrameValueByIndex(int dimension,int index,T* value) const WARN_UNUSED_RETURN;

    const std::vector<T>& getValueForEachDimension() const WARN_UNUSED_RETURN;
    
    std::vector<T> getValueForEachDimension_mt_safe() const WARN_UNUSED_RETURN;

    /**
     * @brief Set the default values for the knob. The vector 'values' must have exactly the same size
     * as the dimension of the knob.
     * @see int getDimension()
     **/
    void setDefaultValues(const std::vector<T>& values);
    
    void setDefaultValue(const T& v,int dimension);
    
protected:
    
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
    virtual void processNewValue(Natron::ValueChangedReason /*reason*/){}


};

#endif

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
     * @brief Do not call this. It is called right away after the constructor by the factory
     * to initialize curves and values. This is separated from the constructor as we need RTTI
     * for Curve.
     **/
    void populate();

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
     * @brief This cannot be done in load since we need all nodes in the project to be loaded
     * in order to do this.
     **/
    void restoreSlaveMasterState(const KnobSerialization& serializationObj);

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
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the last
     * keyframe.
     **/
    bool getLastKeyFrameTime(int dimension,double* time) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the first
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
     * @brief Returns true if at least 1 dimension is animated. MT-Safe
     **/
    bool hasAnimation() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns a const ref to the curves held by this knob. This is MT-safe as they're
     * never deleted (except on program exit).
     **/
    const std::vector< boost::shared_ptr<Curve>  >& getCurves() const WARN_UNUSED_RETURN;

    void setAnimationEnabled(bool val) ;

    /**
     * @brief Returns true if the animation is enabled for this knob. A return value of
     * true doesn't necessarily means that the knob is animated at all.
     **/
    bool isAnimationEnabled() const WARN_UNUSED_RETURN;

    /**
     * @brief Get the knob description, that is the label next to the knob on the user interface.
     * This function is MT-safe as the description NEVER changes throughout the program.
     **/
    const std::string& getDescription() const WARN_UNUSED_RETURN;

    KnobHolder* getHolder() const WARN_UNUSED_RETURN;

    /**
     * @brief Called by a render task to copy all the knobs values to another separate instance
     * to decorellate the values modified by the gui (i.e the user) and the values used to render.
     * This ensures thread-safety.
    **/
    void cloneValue(const Knob& other);

    const std::vector<Variant>& getValueForEachDimension() const WARN_UNUSED_RETURN;
    
    std::vector<Variant> getValueForEachDimension_mt_safe() const WARN_UNUSED_RETURN;

    /**
     * @brief Get the knob dimension. MT-safe as it is static and never changes.
     **/
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
    
    /**
     * @brief Should the knob be saved in the project ? This is MT-safe
     * because it never changes throughout the object's life-time.
     **/
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
    bool slaveTo(int dimension,const boost::shared_ptr<Knob>& other,int otherDimension);

    ///Meant to be called by the GUI
    void onKnobSlavedTo(int dimension,const boost::shared_ptr<Knob>&  other,int otherDimension);

    /**
     * @brief Unslave the value at the given dimension if it was previously
     * slaved to another knob using the slaveTo function.
    **/
    void unSlave(int dimension);
    
    ///Meant to be called  by the Gui
    void onKnobUnSlaved(int dimension);

    /**
     * @brief Returns a valid pointer to a knob if the value at
     * the given dimension is slaved.
    **/
    std::pair<int,boost::shared_ptr<Knob> > getMaster(int dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief Returns true if the value at the given dimension is slave to another parameter
     **/
    bool isSlave(int dimension) const WARN_UNUSED_RETURN;
    

    std::vector<std::pair<int,boost::shared_ptr<Knob> > > getMasters_mt_safe() const WARN_UNUSED_RETURN;

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
    
    void onEvaluateValueChangedInOtherThread(int dimension, int reason);
    
signals:
    
    void evaluateValueChangedInMainThread(int dimension,int reason);
    
    ///emitted whenever setAnimationLevel is called. It is meant to notify
    ///openfx params whether it is auto-keying or not.
    void animationLevelChanged(int);
    
    void deleted();
    
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
    
    void knobSlaved(int,bool);
    
private:
    
    void unSlave(int dimension,Natron::ValueChangedReason reason);
    
    bool slaveTo(int dimension,const boost::shared_ptr<Knob>&  other,int otherDimension,Natron::ValueChangedReason reason);
    
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
    virtual void processNewValue(Natron::ValueChangedReason /*reason*/){}
    
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
    bool _knobsInitialized;
    bool _isSlave;
    
public:
    
    /**
     *@brief A holder is a class managing a bunch of knobs and interacting with an appInstance.
     * When appInstance is NULL the holder will be considered "application global" in which case
     * the knob holder will interact directly with the AppManager singleton.
     **/
    KnobHolder(AppInstance* appInstance);
    
    virtual ~KnobHolder();
    
    template<typename K>
    boost::shared_ptr<K> createKnob(const std::string &description, int dimension = 1) const WARN_UNUSED_RETURN;

    /**
     * @brief Clone each knob of "other" into this KnobHolder.
     * WARNING: other must have exactly the same number of knobs.
     **/
    void cloneKnobs(const KnobHolder& other);

    AppInstance* getApp() const WARN_UNUSED_RETURN {return _app;}
    
    boost::shared_ptr<Knob> getKnobByName(const std::string& name) const WARN_UNUSED_RETURN;
    
    const std::vector< boost::shared_ptr<Knob> >& getKnobs() const WARN_UNUSED_RETURN;
    
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
        
    void initializeKnobsPublic();
    
    bool isSlave() const ;
    
    ///Slave all the knobs of this holder to the other holder.
    void slaveAllKnobs(KnobHolder* other);
    
    void unslaveAllKnobs();
    
protected:

    /**
     * @brief Called when the knobHolder is made slave or unslaved.
     * @param master The master knobHolder. When isSlave is false, master
     * will be set to NULL.
     **/
    virtual void onSlaveStateChanged(bool /*isSlave*/,KnobHolder* /*master*/) {}
    
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
