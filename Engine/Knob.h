//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_KNOB_H_
#define NATRON_ENGINE_KNOB_H_

#include <vector>
#include <string>
#include <set>
#include <QtCore/QReadWriteLock>
#include <QtCore/QMutex>

#include "Engine/Variant.h"
#include "Engine/AppManager.h"
#include "Engine/KnobGuiI.h"
#include "Engine/OverlaySupport.h"


class Curve;
class KeyFrame;
class KnobHolder;
class AppInstance;
class KnobSerialization;
class StringAnimationManager;

namespace Natron {
class OfxParamOverlayInteract;
}


class KnobI;
class KnobSignalSlotHandler
: public QObject
{
    Q_OBJECT
    
    boost::shared_ptr<KnobI> k;
    
public:
    
    KnobSignalSlotHandler(boost::shared_ptr<KnobI> knob);
    
    boost::shared_ptr<KnobI> getKnob() const
    {
        return k;
    }
    
    void s_evaluateValueChangedInMainThread(int dimension,
                                            int reason)
    {
        emit evaluateValueChangedInMainThread(dimension,reason);
    }
    
    void s_animationLevelChanged(int dim,int level)
    {
        emit animationLevelChanged(dim,level);
    }
    
    void s_deleted()
    {
        emit deleted();
    }
    
    void s_valueChanged(int dimension,
                        int reason)
    {
        emit valueChanged(dimension,reason);
    }
    
    void s_secretChanged()
    {
        emit secretChanged();
    }
    
    void s_enabledChanged()
    {
        emit enabledChanged();
    }
    
    void s_keyFrameSet(SequenceTime time,
                       int dimension,
                       int reason,
                       bool added)
    {
        emit keyFrameSet(time,dimension,reason,added);
    }
    
    void s_keyFrameRemoved(SequenceTime time,
                           int dimension,
                           int reason)
    {
        emit keyFrameRemoved(time,dimension,reason);
    }
    
    void s_animationAboutToBeRemoved(int dimension)
    {
        emit animationAboutToBeRemoved(dimension);
    }
    
    void s_animationRemoved(int dimension)
    {
        emit animationRemoved(dimension);
    }
    
    void s_updateSlaves(int dimension)
    {
        emit updateSlaves(dimension);
    }
    
    void s_knobSlaved(int dim,
                      bool slaved)
    {
        emit knobSlaved(dim,slaved);
    }
    
    void s_setValueWithUndoStack(Variant v,
                                 int dim)
    {
        emit setValueWithUndoStack(v, dim);
    }
    
    void s_appendParamEditChange(Variant v,
                                 int dim,
                                 int time,
                                 bool createNewCommand,
                                 bool setKeyFrame)
    {
        emit appendParamEditChange(v, dim,time,createNewCommand,setKeyFrame);
    }
    
    void s_setDirty(bool b)
    {
        emit dirty(b);
    }
    
    void s_setFrozen(bool f)
    {
        emit frozenChanged(f);
    }
    
    void s_keyFrameMoved(int dimension,int oldTime,int newTime)
    {
        emit keyFrameMoved(dimension, oldTime, newTime);
    }
    
    void s_refreshGuiCurve(int dimension)
    {
        emit refreshGuiCurve(dimension);
    }
    
    void s_minMaxChanged(double mini, double maxi, int index)
    {
        emit minMaxChanged(mini,maxi,index);
    }
    
    void s_displayMinMaxChanged(double mini,double maxi,int index)
    {
        emit displayMinMaxChanged(mini,maxi,index);
    }
    
public slots:

    /**
     * @brief Calls KnobI::onAnimationRemoved
     **/
    void onAnimationRemoved(int dimension);
    
    /**
     * @brief Calls KnobI::evaluateValueChange with a reason of Natron::eValueChangedReasonPluginEdited
     **/
    void onMasterChanged(int);
    
    void onMasterKeyFrameSet(SequenceTime time,int dimension,int reason,bool added);
    
    void onMasterKeyFrameRemoved(SequenceTime time,int dimension,int reason);
    
    void onMasterKeyFrameMoved(int dimension,int oldTime,int newTime);
    
    void onMasterAnimationRemoved(int dimension);
    
    /**
     * @brief Calls KnobI::evaluateValueChange and assert that this function is run in the main thread.
     **/
    void onEvaluateValueChangedInOtherThread(int dimension, int reason);
    
signals:
    
    ///emitted whenever evaluateValueChanged is called in another thread than the main thread
    void evaluateValueChangedInMainThread(int dimension,int reason);
    
    ///emitted whenever setAnimationLevel is called. It is meant to notify
    ///openfx params whether it is auto-keying or not.
    void animationLevelChanged(int,int);
    
    ///emitted when the destructor is entered
    void deleted();
    
    ///Emitted when the value is changed with a reason different than eValueChangedReasonUserEdited
    ///This can happen as the result of a setValue() call from the plug-in or by
    ///a slaved knob whose master's value changed. The reason is passed in parameter.
    void valueChanged(int dimension,int reason);
    
    ///Emitted when the secret state of the knob changed
    void secretChanged();
    
    ///Emitted when a dimension enabled state changed
    void enabledChanged();
    
    ///This is called to notify the gui that the knob shouldn't be editable.
    ///Basically this is used when rendering with a Writer or for Trackers while tracking is active.
    void frozenChanged(bool frozen);
    
    ///Emitted whenever a keyframe is set with a reason different of eValueChangedReasonUserEdited
    ///@param added True if this is the first time that the keyframe was set
    void keyFrameSet(SequenceTime time,int dimension,int reason,bool added);
    
    void refreshGuiCurve(int dimension);

    
    ///Emitted whenever a keyframe is removed with a reason different of eValueChangedReasonUserEdited
    void keyFrameRemoved(SequenceTime,int dimension,int reason);
    
    void keyFrameMoved(int dimension,int oldTime,int newTime);
    
    ///Emitted whenever all keyframes of a dimension are about removed with a reason different of eValueChangedReasonUserEdited
    void animationAboutToBeRemoved(int);
    
    ///Emitted whenever all keyframes of a dimension are effectively removed
    void animationRemoved(int);
    
    ///Emitted whenever setValueAtTime,setValue or deleteValueAtTime is called. It notifies slaves
    ///of the changes that occured in this knob, letting them a chance to update their interface.
    void updateSlaves(int dimension);
    
    ///Emitted whenever a knob is slaved via the slaveTo function with a reason of eValueChangedReasonPluginEdited.
    void knobSlaved(int,bool);
    
    ///Emitted whenever the GUI should set the value using the undo stack. This is
    ///only to address the problem of interacts that should use the undo/redo stack.
    void setValueWithUndoStack(Variant v,int dim);
    
    ///Same as setValueWithUndoStack except that the value change will be compressed
    ///in a multiple edit undo/redo action
    void appendParamEditChange(Variant v,int dim,int time,bool createNewCommand,bool setKeyFrame);
    
    ///Emitted whenever the knob is dirty, @see KnobI::setDirty(bool)
    void dirty(bool);
    
    void minMaxChanged(double mini, double maxi, int index);
    
    void displayMinMaxChanged(double mini,double maxi,int index);
};

class KnobI
    : public OverlaySupport
{
public:

    KnobI()
    {
    }

    virtual ~KnobI()
    {
    }

    /**
     * @brief Do not call this. It is called right away after the constructor by the factory
     * to initialize curves and values. This is separated from the constructor as we need RTTI
     * for Curve.
     **/
    virtual void populate() = 0;
    virtual void setKnobGuiPointer(KnobGuiI* ptr) = 0;
    virtual KnobGuiI* getKnobGuiPointer() const = 0;

    /**
     * @brief Returns the knob was created by a plugin or added automatically by Natron (e.g like mask knobs)
     **/
    virtual bool isDeclaredByPlugin() const = 0;


    /**
     * @brief Must return the type name of the knob. This name will be used by the KnobFactory
     * to create an instance of this knob.
     **/
    virtual const std::string & typeName() const = 0;

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
    virtual void setDimensionName(int dim,const std::string & name) = 0;


    /**
     * @brief When set to true the instanceChanged action on the plugin and evaluate (render) will not be called
     * when issuing value changes. Internally it maintains a counter, when it reaches 0 the evaluation is unblocked.
     **/
    virtual void blockEvaluation() = 0;

    /**
     * @brief To be called to reactivate evaluation. Internally it maintains a counter, when it reaches 0 the evaluation is unblocked.
     **/
    virtual void unblockEvaluation() = 0;

    /**
     * @brief Called by setValue to refresh the GUI, call the instanceChanged action on the plugin and
     * evaluate the new value (cause a render).
     **/
    virtual void evaluateValueChange(int dimension,Natron::ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Copies all the values, animations and extra data the other knob might have
     * to this knob. This function calls cloneExtraData.
     * The evaluateValueChange function will not be called as a result of the clone.
     * However a valueChanged signal will be emitted by the KnobSignalSlotHandler if there's any.
     *
     * @param dimension If -1 all dimensions will be cloned, otherwise you can clone only a specific dimension
     *
     * WARNING: This knob and 'other' MUST have the same dimension as well as the same type.
     **/
    virtual void clone(KnobI* other,int dimension = -1) = 0;
    virtual void clone(const boost::shared_ptr<KnobI> & other,int dimension = -1)
    {
        clone( other.get(), dimension );
    }

    /**
     * @brief Performs the same as clone but also refresh any gui it has.
     **/
    virtual void cloneAndUpdateGui(KnobI* other,int dimension = -1) = 0;

    /**
     * @brief Performs the same as cloneAndUpdateGui, but also copies the properties of the knob such as whether it is enabled, secret,
     * the name of the knob, etc...
     **/
    virtual void deepClone(KnobI* other) = 0;
    
    /**
     * @brief Same as clone(const boost::shared_ptr<KnobI>& ) except that the given offset is applied
     * on the keyframes time and only the keyframes withing the given range are copied.
     * If the range is NULL everything will be copied.
     *
     * Note that unlike the other version of clone, this version is more relaxed and accept parameters
     * with different dimensions, but only the intersection of the dimension of the 2 parameters will be copied.
     * The restriction on types still apply.
     **/
    virtual void clone(KnobI* other, SequenceTime offset, const RangeD* range,int dimension = -1) = 0;
    virtual void clone(const boost::shared_ptr<KnobI> & other,
                       SequenceTime offset,
                       const RangeD* range,
                       int dimension = -1)
    {
        clone(other.get(),offset,range,dimension);
    }

protected:

    /**
     * @brief Removes all the keyframes in the given dimension.
     **/
    virtual void removeAnimation(int dimension,Natron::ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Removes the keyframe at the given time and dimension if it matches any.
     **/
    virtual void deleteValueAtTime(int time,int dimension,Natron::ValueChangedReasonEnum reason) = 0;

public:

    /**
     * @brief Calls deleteValueAtTime with a reason of Natron::eValueChangedReasonPluginEdited.
     **/
    void deleteValueAtTime(int time,int dimension);
    
    /**
     * @brief Moves a keyframe by a given delta and emits the signal keyframeMoved
     **/
    virtual bool moveValueAtTime(int time,int dimension,double dt,double dv,KeyFrame* newKey) = 0;
    
    /**
     * @brief Changes the interpolation type for the given keyframe
     **/
    virtual bool setInterpolationAtTime(int dimension,int time,Natron::KeyframeTypeEnum interpolation,KeyFrame* newKey) = 0;
    
    /**
     * @brief Set the left/right derivatives of the control point at the given time.
     **/
    virtual bool moveDerivativesAtTime(int dimension,int time,double left,double right) = 0;
    virtual bool moveDerivativeAtTime(int dimension,int time,double derivative,bool isLeft) = 0;

    /**
     * @brief Removes animation before the given time and dimension. If the reason is different than Natron::eValueChangedReasonUserEdited
     * a signal will be emitted
     **/
    virtual void deleteAnimationBeforeTime(int time,int dimension,Natron::ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Removes animation before the given time and dimension. If the reason is different than Natron::eValueChangedReasonUserEdited
     * a signal will be emitted
     **/
    virtual void deleteAnimationAfterTime(int time,int dimension,Natron::ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Calls removeAnimation with a reason of Natron::eValueChangedReasonPluginEdited.
     **/
    void removeAnimation(int dimension);

    /**
     * @brief Calls deleteValueAtTime with a reason of Natron::eValueChangedReasonUserEdited
     **/
    virtual void onKeyFrameRemoved(SequenceTime time,int dimension) = 0;

    /**
     * @brief Calls removeAnimation with a reason of Natron::eValueChangedReasonUserEdited
     **/
    void onAnimationRemoved(int dimension);

    /**
     * @brief Called when the master knob has changed its values or keyframes.
     * @param masterDimension The dimension of the master which has changed
     **/
    virtual void onMasterChanged(KnobI* master,int masterDimension) = 0;

    /**
     * @brief Calls setValueAtTime with a reason of Natron::eValueChangedReasonUserEdited.
     **/
    virtual bool onKeyFrameSet(SequenceTime time,int dimension) = 0;
    virtual bool onKeyFrameSet(SequenceTime time,const KeyFrame& key,int dimension) = 0;

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
    virtual boost::shared_ptr<Curve> getCurve(int dimension = 0,bool byPassMaster = false) const = 0;

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
    virtual const std::vector< boost::shared_ptr<Curve>  > & getCurves() const = 0;

    /**
     * @brief Activates or deactivates the animation for this parameter. On the GUI side that means
     * the user can never interact with the animation curves nor can he/she set any keyframe.
     **/
    virtual void setAnimationEnabled(bool val) = 0;

    /**
     * @brief Returns true if the animation is enabled for this knob. A return value of
     * true doesn't necessarily means that the knob is animated at all.
     **/
    virtual bool isAnimationEnabled() const = 0;

    /**
     * @brief Get the knob description, that is the label next to the knob on the user interface.
     * This function is MT-safe as the description NEVER changes throughout the program.
     **/
    virtual const std::string & getDescription() const = 0;
    
    /**
     * @brief Hide the description label on the GUI on the left of the knob. This is not dynamic
     * and must be called upon the knob creation.
     **/
    virtual void hideDescription() = 0;
    virtual bool isDescriptionVisible() const = 0;

    /**
     * @brief Returns a pointer to the holder owning the knob.
     **/
    virtual KnobHolder* getHolder() const = 0;

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
     * @biref This is called to notify the gui that the knob shouldn't be editable.
     * Basically this is used when rendering with a Writer or for Trackers while tracking is active.
     **/
    virtual void setIsFrozen(bool frozen) = 0;

    /**
     * @brief When dirty, the knob is actually representing several elements that do not hold the same value.
     * For example for the roto node, a knob actually represent the value of the opacity of the selected curve.
     * If they're multiple curves selected,then the value of the knob is dirty
     * since we have no means to know the specifics values for each curve.
     * This is purely a GUI stuff and should only be called when the GUI is created. This is here just for means to
     * notify the GUI.
     **/
    virtual void setDirty(bool d) = 0;

    /**
     * @brief Call this to change the knob name. The name is not the text label displayed on
     * the GUI but what Natron uses internally to identify knobs from each other. By default the
     * name is the same as the getDescription(i.e: the text label).
     */
    virtual void setName(const std::string & name) = 0;

    /**
     * @brief Returns the knob name. By default the
     * name is the same as the getDescription(i.e: the text label).
     */
    virtual const std::string & getName() const = 0;

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
    virtual void setHintToolTip(const std::string & hint) = 0;

    /**
     * @brief Get the tooltip text.
     **/
    virtual const std::string & getHintToolTip() const = 0;

    /**
     * @brief Call this to set a custom interact entry point, replacing any existing gui.
     **/
    virtual void setCustomInteract(const boost::shared_ptr<Natron::OfxParamOverlayInteract> & interactDesc) = 0;
    virtual boost::shared_ptr<Natron::OfxParamOverlayInteract> getCustomInteract() const = 0;
    virtual void swapOpenGLBuffers() = 0;
    virtual void redraw() = 0;
    virtual void getViewportSize(double &width, double &height) const = 0;
    virtual void getPixelScale(double & xScale, double & yScale) const  = 0;
    virtual void getBackgroundColour(double &r, double &g, double &b) const = 0;
    virtual void saveOpenGLContext() = 0;
    virtual void restoreOpenGLContext() = 0;
    
    
    /**
     * @brief If this is an openfx param, this is the pointer to the handle.
     **/
    virtual void setOfxParamHandle(void* ofxParamHandle) = 0;
    virtual void* getOfxParamHandle() const = 0;

    /**
     * @brief For MultiInstance effects, flagging a knob as instance specific will make it appear in the table of the instances
     * instead as the regular way of showing knobs.
     * Not MT-safe, should be called right away after creation.
     **/
    virtual void setAsInstanceSpecific() = 0;
    virtual bool isInstanceSpecific() const = 0;

    /**
     * @brief If this knob has a gui view, then the gui view should set the animation of this knob on the application's knob clipboard.
     **/
    virtual void copyAnimationToClipboard() const = 0;

    /**
     * @brief Dequeues any setValue/setValueAtTime calls that were queued in the queue.
     **/
    virtual void dequeueValuesSet(bool disableEvaluation) = 0;
    
    /**
     * @brief Returns the current time if attached to a timeline
     **/
    virtual SequenceTime getCurrentTime() const = 0;
    
    
    virtual boost::shared_ptr<KnobSignalSlotHandler> getSignalSlotHandler() const = 0;

protected:

    /**
     * @brief Slaves the value for the given dimension to the curve
     * at the same dimension for the knob 'other'.
     * In case of success, this function returns true, otherwise false.
     **/
    virtual bool slaveTo(int dimension,const boost::shared_ptr<KnobI> &  other,int otherDimension,Natron::ValueChangedReasonEnum reason,
                         bool ignoreMasterPersistence) = 0;

    /**
     * @brief Unslaves a previously slaved dimension. The implementation should assert that
     * the dimension was really slaved.
     **/
    virtual void unSlave(int dimension,Natron::ValueChangedReasonEnum reason,bool copyState) = 0;

public:

    /**
     * @brief Calls slaveTo with a value changed reason of Natron::eValueChangedReasonPluginEdited.
     * @param ignoreMasterPersistence If true the master will not be serialized.
     **/
    bool slaveTo(int dimension,const boost::shared_ptr<KnobI> & other,int otherDimension,bool ignoreMasterPersistence = false);
    virtual bool isMastersPersistenceIgnored() const = 0;

    /**
     * @brief Calls slaveTo with a value changed reason of Natron::eValueChangedReasonUserEdited.
     **/
    void onKnobSlavedTo(int dimension,const boost::shared_ptr<KnobI> &  other,int otherDimension);


    /**
     * @brief Calls unSlave with a value changed reason of Natron::eValueChangedReasonPluginEdited.
     **/
    void unSlave(int dimension,bool copyState);

    /**
     * @brief Returns a list of all the knobs whose value depends upon this knob.
     **/
    virtual void getListeners(std::list<KnobI*> & listeners) const = 0;

    /**
     * @brief Calls unSlave with a value changed reason of Natron::eValueChangedReasonUserEdited.
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
    virtual void setAnimationLevel(int dimension,Natron::AnimationLevelEnum level) = 0;

    /**
     * @brief Get the current animation level.
     **/
    virtual Natron::AnimationLevelEnum getAnimationLevel(int dimension) const = 0;

    /**
     * @brief Restores the default value
     **/
    virtual void resetToDefaultValue(int dimension) = 0;

    /**
     * @brief Must return true if the other knobs type can convert to this knob's type.
     **/
    virtual bool isTypeCompatible(const boost::shared_ptr<KnobI> & other) const = 0;
};



///Skins the API of KnobI by implementing most of the functions in a non templated manner.
class KnobHelper
    : public KnobI
{
    friend class KnobHolder;

public:

    enum ValueChangedReturnCode
    {
        NO_KEYFRAME_ADDED = 0,
        KEYFRAME_MODIFIED,
        KEYFRAME_ADDED
    };

    /**
     * @brief Creates a new Knob that belongs to the given holder, with the given description.
     * The name of the knob will be equal to the description, you can change it by calling setName()
     * The dimension parameter indicates how many dimension the knob should have.
     * If declaredByPlugin is false then Natron will never call onKnobValueChanged on the holder.
     **/
    KnobHelper(KnobHolder*  holder,
               const std::string & description,
               int dimension = 1,
               bool declaredByPlugin = true);

    virtual ~KnobHelper();

    virtual void setKnobGuiPointer(KnobGuiI* ptr) OVERRIDE FINAL;
    virtual KnobGuiI* getKnobGuiPointer() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    /**
     * @brief Returns the knob was created by a plugin or added automatically by Natron (e.g like mask knobs)
     **/
    virtual bool isDeclaredByPlugin() const OVERRIDE FINAL;
    virtual void setAsInstanceSpecific() OVERRIDE FINAL;
    virtual bool isInstanceSpecific() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Set a shared ptr to the signal slot handler, that will live as long as the knob lives.
     * It is set by the factory, do not call it yourself.
     **/
    void setSignalSlotHandler(const boost::shared_ptr<KnobSignalSlotHandler> & handler);

    virtual boost::shared_ptr<KnobSignalSlotHandler> getSignalSlotHandler() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return _signalSlotHandler;
    }

    //////////////////////////////////////////////////////////////////////
    ///////////////////////////////////
    /////////////////////////////////// Implementation of KnobI
    //////////////////////////////////////////////////////////////////////

    ///Populates for each dimension: the enabled state, the curve and the animation level
    virtual void populate() OVERRIDE;
    virtual void blockEvaluation() OVERRIDE FINAL;
    virtual void unblockEvaluation() OVERRIDE FINAL;
    virtual void evaluateValueChange(int dimension,Natron::ValueChangedReasonEnum reason) OVERRIDE FINAL;

private:

    
    
    virtual void removeAnimation(int dimension,Natron::ValueChangedReasonEnum reason) OVERRIDE FINAL;
    virtual void deleteValueAtTime(int time,int dimension,Natron::ValueChangedReasonEnum reason) OVERRIDE FINAL;

public:

    virtual void onKeyFrameRemoved(SequenceTime time,int dimension) OVERRIDE FINAL;
    virtual bool moveValueAtTime(int time,int dimension,double dt,double dv,KeyFrame* newKey) OVERRIDE FINAL;
    virtual bool setInterpolationAtTime(int dimension,int time,Natron::KeyframeTypeEnum interpolation,KeyFrame* newKey) OVERRIDE FINAL;
    virtual bool moveDerivativesAtTime(int dimension,int time,double left,double right)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool moveDerivativeAtTime(int dimension,int time,double derivative,bool isLeft) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void onMasterChanged(KnobI* master,int masterDimension) OVERRIDE FINAL;
    virtual void deleteAnimationBeforeTime(int time,int dimension,Natron::ValueChangedReasonEnum reason) OVERRIDE FINAL;
    virtual void deleteAnimationAfterTime(int time,int dimension,Natron::ValueChangedReasonEnum reason) OVERRIDE FINAL;
    virtual double getDerivativeAtTime(double time, int dimension = 0) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getKeyFrameTime(int index,int dimension,double* time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getLastKeyFrameTime(int dimension,double* time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getFirstKeyFrameTime(int dimension,double* time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getKeyFramesCount(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getNearestKeyFrameTime(int dimension,double time,double* nearestTime) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getKeyFrameIndex(int dimension, double time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual boost::shared_ptr<Curve> getCurve(int dimension = 0,bool byPassMaster = false) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isAnimated(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasAnimation() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual const std::vector< boost::shared_ptr<Curve>  > & getCurves() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAnimationEnabled(bool val) OVERRIDE FINAL;
    virtual bool isAnimationEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual const std::string & getDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void hideDescription()  OVERRIDE FINAL;
    virtual bool isDescriptionVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual KnobHolder* getHolder() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getDimension() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void turnOffNewLine() OVERRIDE FINAL;
    virtual bool isNewLineTurnedOff() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setSpacingBetweenItems(int spacing) OVERRIDE FINAL;
    virtual void setEnabled(int dimension,bool b) OVERRIDE FINAL;
    virtual bool isEnabled(int dimension) const OVERRIDE FINAL;
    virtual void setAllDimensionsEnabled(bool b) OVERRIDE FINAL;
    virtual void setSecret(bool b) OVERRIDE FINAL;
    virtual bool getIsSecret() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setIsFrozen(bool frozen) OVERRIDE FINAL;
    virtual void setDirty(bool d) OVERRIDE FINAL;
    virtual void setName(const std::string & name) OVERRIDE FINAL;
    virtual const std::string & getName() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setParentKnob(boost::shared_ptr<KnobI> knob) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getParentKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int determineHierarchySize() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setEvaluateOnChange(bool b) OVERRIDE FINAL;
    virtual bool getEvaluateOnChange() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getIsPersistant() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setIsPersistant(bool b) OVERRIDE FINAL;
    virtual void setCanUndo(bool val) OVERRIDE FINAL;
    virtual bool getCanUndo() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setHintToolTip(const std::string & hint) OVERRIDE FINAL;
    virtual const std::string & getHintToolTip() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setCustomInteract(const boost::shared_ptr<Natron::OfxParamOverlayInteract> & interactDesc) OVERRIDE FINAL;
    virtual boost::shared_ptr<Natron::OfxParamOverlayInteract> getCustomInteract() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;
    virtual void redraw() OVERRIDE FINAL;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;
    virtual void setOfxParamHandle(void* ofxParamHandle) OVERRIDE FINAL;
    virtual void* getOfxParamHandle() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isMastersPersistenceIgnored() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void copyAnimationToClipboard() const OVERRIDE FINAL;
    virtual SequenceTime getCurrentTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setDimensionName(int dim,const std::string & name) OVERRIDE FINAL;
    
private:

    virtual bool slaveTo(int dimension,const boost::shared_ptr<KnobI> &  other,int otherDimension,Natron::ValueChangedReasonEnum reason
                         ,bool ignoreMasterPersistence) OVERRIDE FINAL WARN_UNUSED_RETURN;

protected:

    /**
     * @brief Protected so the implementation of unSlave can actually use this to reset the master pointer
     **/
    void resetMaster(int dimension);

public:

    virtual std::pair<int,boost::shared_ptr<KnobI> > getMaster(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isSlave(int dimension) const;
    virtual std::vector<std::pair<int,boost::shared_ptr<KnobI> > > getMasters_mt_safe() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual Natron::AnimationLevelEnum getAnimationLevel(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isTypeCompatible(const boost::shared_ptr<KnobI> & other) const = 0;


    /**
     * @brief Adds a new listener to this knob. This is just a pure notification about the fact that the given knob
     * is listening to the values/keyframes of "this". It could be call addSlave but it will also be use for expressions.
     **/
    void addListener(KnobI* knob);
    void removeListener(KnobI* knob);

    virtual void getListeners(std::list<KnobI*> & listeners) const OVERRIDE FINAL;

protected:


    /**
     * @brief Called when you must copy any extra data you maintain from the other knob.
     * The other knob is guaranteed to be of the same type.
     **/
    virtual void cloneExtraData(KnobI* /*other*/,int dimension = -1)
    {
        (void)dimension;
    }

    virtual void cloneExtraData(KnobI* /*other*/,
                                SequenceTime /*offset*/,
                                const RangeD* /*range*/,
                                int dimension = -1)
    {
        (void)dimension;
    }

    /**
     * @brief Override to copy extra properties, such as the entries for a combobox for example.
     **/
    virtual void deepCloneExtraData(KnobI* /*other*/) {}
    
    /**
     * @brief Called when a keyframe is removed.
     * Derived knobs can use it to refresh any data structure related to keyframes it may have.
     **/
    virtual void keyframeRemoved_virtual(int /*dimension*/,
                                         double /*time*/)
    {
    }

    /**
     * @brief Called when all keyframes are removed
     * Derived knobs can use it to refresh any data structure related to keyframes it may have.
     **/
    virtual void animationRemoved_virtual(int /*dimension*/)
    {
    }
    
    void cloneGuiCurvesIfNeeded(std::set<int>& modifiedDimensions);
    
    void cloneInternalCurvesIfNeeded(std::set<int>& modifiedDimensions);
    
    void setInternalCurveHasChanged(int dimension, bool changed);
    
    void guiCurveCloneInternalCurve(int dimension);
    
    boost::shared_ptr<Curve> getGuiCurve(int dimension) const;
    
    void setGuiCurveHasChanged(int dimension,bool changed);

    void checkAnimationLevel(int dimension);
    boost::shared_ptr<KnobSignalSlotHandler> _signalSlotHandler;

private:

    struct KnobHelperPrivate;
    boost::scoped_ptr<KnobHelperPrivate> _imp;
};

/**
 * @brief A Knob is a parameter, templated by a data type.
 * The template parameter is meant to be used with POD types or strings,
 * not complex classes or structures.
 **/
template <typename T>
class Knob
    : public KnobHelper
{
public:

    typedef T DataType;

    /**
     * @brief Make a knob for the given KnobHolder with the given description (the label displayed on
     * its interface) and with the given dimension. The dimension parameter is used for example for the
     * Color_Knob which has 4 doubles (r,g,b,a), hence 4 dimensions.
     **/
    Knob(KnobHolder*  holder,
         const std::string & description,
         int dimension = 1,
         bool declaredByPlugin = true);

    virtual ~Knob();

public:

    /**
     * @brief Get the current value of the knob for the given dimension.
     * If it is animated, it will return the value at the current time.
     **/
    T getValue(int dimension = 0,bool clampToMinMax = true) const WARN_UNUSED_RETURN;

    /**
     * @brief Returns the value of the knob at the given time and for the given dimension.
     * If it is not animated, it will call getValue for that dimension and return the result.
     *
     * This function is overloaded by the String_Knob which can have its custom interpolation
     * but this should be the only knob which should ever need to overload it.
     **/
    T getValueAtTime(double time, int dimension = 0,bool clampToMinMax = true,bool byPassMaster = false) const WARN_UNUSED_RETURN;

private:

   
    virtual void unSlave(int dimension,Natron::ValueChangedReasonEnum reason,bool copyState) OVERRIDE FINAL;

    
    /**
     * @brief Set the value of the knob in the given dimension with the given reason.
     * @param newKey If not NULL and the animation level of the knob is Natron::eAnimationLevelInterpolatedValue
     * then a new keyframe will be set at the current time.
     **/
    ValueChangedReturnCode setValue(const T & v,int dimension,Natron::ValueChangedReasonEnum reason,
                                    KeyFrame* newKey) WARN_UNUSED_RETURN;
    /**
     * @brief Set the value of the knob at the given time and for the given dimension with the given reason.
     * @param newKey[out] The keyframe that was added if the return value is true.
     * @returns True if a keyframe was successfully added, false otherwise.
     **/
    bool setValueAtTime(int time,const T & v,int dimension,Natron::ValueChangedReasonEnum reason,KeyFrame* newKey) WARN_UNUSED_RETURN;

public:

    
   
    
    /**
     * @brief Calls setValue with a reason of Natron::eValueChangedReasonNatronInternalEdited.
     * @param turnOffAutoKeying If set to true, the underlying call to setValue will
     * not set a new keyframe.
     **/
    ValueChangedReturnCode setValue(const T & value,int dimension,bool turnOffAutoKeying = false);

    /**
     * @brief Calls setValue 
     * @param reason Can either be Natron::eValueChangedReasonUserEdited or Natron::eValueChangedReasonNatronGuiEdited
     * @param newKey[out] The keyframe that was added if the return value is true.
     * @returns A status according to the operation that was made to the keyframe.
     * @see ValueChangedReturnCode
     **/
    ValueChangedReturnCode onValueChanged(const T & v,int dimension,Natron::ValueChangedReasonEnum reason,KeyFrame* newKey);
    
    /**
     * @brief Calls setValue with a reason of Natron::eValueChangedReasonPluginEdited.
     **/
    ValueChangedReturnCode setValueFromPlugin(const T & value,int dimension);

    /**
     * @brief This is called by the plugin when a set value call would happen during  an interact action.
     **/
    void requestSetValueOnUndoStack(const T & value,int dimension);

    /**
     * @brief Calls setValueAtTime with a reason of Natron::eValueChangedReasonNatronInternalEdited.
     **/
    void setValueAtTime(int time,const T & v,int dimension);
    
    /**
     * @brief Calls setValueAtTime with a reason of Natron::eValueChangedReasonPluginEdited.
     **/
    void setValueAtTimeFromPlugin(int time,const T & v,int dimension);

    /**
     * @brief Unlike getValueAtTime this function doesn't interpolate the values.
     * Instead the true value of the keyframe at the given index will be returned.
     * @returns True if a keyframe was found at the given index and for the given dimension,
     * false otherwise.
     **/
    T getKeyFrameValueByIndex(int dimension,int index,bool* ok) const WARN_UNUSED_RETURN;

    /**
     * @brief Same as getValueForEachDimension() but MT thread-safe.
     **/
    std::vector<T> getValueForEachDimension_mt_safe_vector() const WARN_UNUSED_RETURN;
    std::list<T> getValueForEachDimension_mt_safe() const WARN_UNUSED_RETURN;

    /**
     * @brief Get Default values
     **/
    std::vector<T> getDefaultValues_mt_safe() const WARN_UNUSED_RETURN;

    /**
     * @brief Set a default value for the particular dimension.
     **/
    void setDefaultValue(const T & v,int dimension = 0);


    //////////////////////////////////////////////////////////////////////
    ///////////////////////////////////
    /////////////////////////////////// Implementation of KnobI
    //////////////////////////////////////////////////////////////////////

    ///Populates for each dimension: the default value and the value member. The implementation MUST call
    /// the KnobHelper version.
    virtual void populate() OVERRIDE FINAL;

    /// You must implement it
    virtual const std::string & typeName() const OVERRIDE;

    /// You must implement it
    virtual bool canAnimate() const OVERRIDE;
    virtual bool isTypeCompatible(const boost::shared_ptr<KnobI> & other) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    ///Cannot be overloaded by KnobHelper as it requires setValueAtTime
    virtual bool onKeyFrameSet(SequenceTime time,int dimension) OVERRIDE FINAL;
    virtual bool onKeyFrameSet(SequenceTime time,const KeyFrame& key,int dimension) OVERRIDE FINAL;

    ///Cannot be overloaded by KnobHelper as it requires setValue
    virtual void onTimeChanged(SequenceTime time) OVERRIDE FINAL;

    ///Cannot be overloaded by KnobHelper as it requires the value member
    virtual double getIntegrateFromTimeToTime(double time1, double time2, int dimension = 0) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    ///Cannot be overloaded by KnobHelper as it requires setValue
    virtual void resetToDefaultValue(int dimension) OVERRIDE FINAL;
    virtual void clone(KnobI* other,int dimension = -1)  OVERRIDE FINAL;
    virtual void clone(KnobI* other,SequenceTime offset, const RangeD* range,int dimension = -1) OVERRIDE FINAL;
    virtual void cloneAndUpdateGui(KnobI* other,int dimension = -1) OVERRIDE FINAL;
    virtual void deepClone(KnobI* other)  OVERRIDE FINAL;
    
    virtual void dequeueValuesSet(bool disableEvaluation) OVERRIDE FINAL;
    
    ///MT-safe
    void setMinimum(const T& mini, int dimension = 0);
    void setMaximum(const T& maxi, int dimension = 0);
    void setDisplayMinimum(const T& mini, int dimension = 0);
    void setDisplayMaximum(const T& maxi, int dimension = 0);
    void setMinimumsAndMaximums(const std::vector<T> &minis, const std::vector<T> &maxis);
    void setDisplayMinimumsAndMaximums(const std::vector<T> &minis, const std::vector<T> &maxis);

    
    ///Not MT-SAFE, can only be called from main thread
    const std::vector<T> &getMinimums() const;
    const std::vector<T> &getMaximums() const;
    const std::vector<T> &getDisplayMinimums() const;
    const std::vector<T> &getDisplayMaximums() const;
    
    /// MT-SAFE
    T getMinimum(int dimension = 0) const;
    T getMaximum(int dimension = 0) const;
    T getDisplayMinimum(int dimension = 0) const;
    T getDisplayMaximum(int dimension = 0) const;
    
protected:
    
    virtual void resetExtraToDefaultValue(int /*dimension*/) {}

private:
    
    void initMinMax();
    
    T clampToMinMax(const T& value,int dimension) const;
    
    void signalMinMaxChanged(const T& mini,const T& maxi,int dimension);
    void signalDisplayMinMaxChanged(const T& mini,const T& maxi,int dimension);

    void cloneValues(KnobI* other);

    T getValueFromMaster(int dimension);

    void valueToVariant(const T & v,Variant* vari);

    int get_SetValueRecursionLevel() const
    {
        QMutexLocker l(&_setValueRecursionLevelMutex);

        return _setValueRecursionLevel;
    }
    
    void makeKeyFrame(Curve* curve,double time,const T& v,KeyFrame* key);
    
    void queueSetValue(const T& v,int dimension);

    //////////////////////////////////////////////////////////////////////
    /////////////////////////////////// End implementation of KnobI
    //////////////////////////////////////////////////////////////////////
    struct QueuedSetValuePrivate;
    struct QueuedSetValue
    {
        
        boost::scoped_ptr<QueuedSetValuePrivate> _imp;
  
        
        QueuedSetValue(int dimension,const T& value,const KeyFrame& key,bool useKey);
        
        virtual bool isSetValueAtTime() const { return false; }
        
        virtual ~QueuedSetValue();
    };
    
    struct QueuedSetValueAtTime : public QueuedSetValue
    {
        double time;
        
        QueuedSetValueAtTime(double time,int dimension,const T& value,const KeyFrame& key)
        : QueuedSetValue(dimension,value,key,true)
        , time(time)
        {
            
        }
        
        virtual bool isSetValueAtTime() const { return true; }
        
        virtual ~QueuedSetValueAtTime() {}
    };
    

    ///Here is all the stuff we couldn't get rid of the template parameter

    mutable QReadWriteLock _valueMutex; //< protects _values
    std::vector<T> _values;
    std::vector<T> _defaultValues;
    
    //Only for double and int
    mutable QReadWriteLock _minMaxMutex;
    std::vector<T>  _minimums,_maximums,_displayMins,_displayMaxs;
    
    ///this flag is to avoid recursive setValue calls
    int _setValueRecursionLevel;
    mutable QMutex _setValueRecursionLevelMutex;
    
    mutable QMutex _setValuesQueueMutex;
    std::list< boost::shared_ptr<QueuedSetValue> > _setValuesQueue;
};


class AnimatingString_KnobHelper
    : public Knob<std::string>
{
public:

    AnimatingString_KnobHelper(KnobHolder* holder,
                               const std::string &description,
                               int dimension,
                               bool declaredByPlugin = true);

    virtual ~AnimatingString_KnobHelper();

    void stringToKeyFrameValue(int time,const std::string & v,double* returnValue);

    //for integration of openfx custom params
    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle inArgsRaw,
                                                           OfxPropertySetHandle outArgsRaw);


    const StringAnimationManager & getAnimation() const
    {
        return *_animation;
    }

    void loadAnimation(const std::map<int,std::string> & keyframes);

    void saveAnimation(std::map<int,std::string>* keyframes) const;

    void setCustomInterpolation(customParamInterpolationV1Entry_t func,void* ofxParamHandle);

    void stringFromInterpolatedValue(double interpolated,std::string* returnValue) const;

    std::string getStringAtTime(double time, int dimension) const;

protected:

    virtual void cloneExtraData(KnobI* other,int dimension = -1) OVERRIDE;
    virtual void cloneExtraData(KnobI* other, SequenceTime offset, const RangeD* range,int dimension = -1) OVERRIDE;
    virtual void keyframeRemoved_virtual(int dimension, double time) OVERRIDE;
    virtual void animationRemoved_virtual(int dimension) OVERRIDE;

private:


    StringAnimationManager* _animation;
};


/**
 * @brief A Knob holder is a class that stores Knobs and interact with them in some way.
 * It serves 2 purpose:
 * 1) It is a "factory" for knobs: it creates and destroys them.
 * 2) It calls a set of begin/end valueChanged whenever a knob value changed. It also
 * calls evaluate() which should then trigger an evaluation of the freshly changed value
 * (i.e force a new render).
 **/
class KnobHolder
{
    struct KnobHolderPrivate;
    boost::scoped_ptr<KnobHolderPrivate> _imp;

public:

    enum MultipleParamsEditLevel
    {
        PARAM_EDIT_OFF = 0, //< The knob should not use multiple edits command
        PARAM_EDIT_ON_CREATE_NEW_COMMAND, //< The knob should use multiple edits command and create a new one that will not merge with others
        PARAM_EDIT_ON //< The knob should use multiple edits command and merge it with priors command (if any)
    };

    /**
     *@brief A holder is a class managing a bunch of knobs and interacting with an appInstance.
     * When appInstance is NULL the holder will be considered "application global" in which case
     * the knob holder will interact directly with the AppManager singleton.
     **/
    KnobHolder(AppInstance* appInstance);

    virtual ~KnobHolder();

    template<typename K>
    boost::shared_ptr<K> createKnob(const std::string &description, int dimension = 1) const WARN_UNUSED_RETURN;
    AppInstance* getApp() const WARN_UNUSED_RETURN;
    boost::shared_ptr<KnobI> getKnobByName(const std::string & name) const WARN_UNUSED_RETURN;
    const std::vector< boost::shared_ptr<KnobI> > & getKnobs() const WARN_UNUSED_RETURN;

    void onGuiFrozenChange(bool frozen);
    
    void refreshAfterTimeChange(SequenceTime time);

    void refreshInstanceSpecificKnobsOnly(SequenceTime time);

    KnobHolder::MultipleParamsEditLevel getMultipleParamsEditLevel() const;

    void setMultipleParamsEditLevel(KnobHolder::MultipleParamsEditLevel level);

    virtual bool isProject() const
    {
        return false;
    }

    /**
     * @brief Restore all knobs to their default values
     **/
    void restoreDefaultValues();
    virtual void aboutToRestoreDefaultValues()
    {
    }

    /**
     * @brief When frozen is true all the knobs of this effect read-only so the user can't interact with it.
     **/
    void setKnobsFrozen(bool frozen);

    /**
     * @brief Can be overriden to prevent values to be set directly.
     * Instead the setValue/setValueAtTime actions are queued up
     * They will be dequeued when dequeueValuesSet will be called.
     **/
    virtual bool canSetValue() const { return true; }
    
    /**
     * @brief Dequeues all values set in the queues for all knobs
     **/
    void dequeueValuesSet();
    
    void discardAppPointer();
    
protected:

    bool isEvaluationBlocked() const;


    /**
     * @brief Equivalent to assert(actionsRecursionLevel == 0).
     * In release mode an exception is thrown instead.
     * This should be called in all actions except in the following recursive actions...
     *
     * kOfxActionBeginInstanceChanged
     * kOfxActionInstanceChanged
     * kOfxActionEndInstanceChanged
     * kOfxImageEffectActionGetClipPreferences
     * kOfxInteractActionDraw
     **/
    void assertActionIsNotRecursive() const;

    /**
     * @brief Should be called in the begining of an action.
     * Right after assertActionIsNotRecursive() for non recursive actions.
     **/
    void incrementRecursionLevel();

    /**
     * @brief Should be called at the end of an action.
     **/
    void decrementRecursionLevel();

        
    /**
     * @brief A small class to help managing the recursion level
     * that can also that an action is not recursive.
     **/
    class RecursionLevelManager
    {
        KnobHolder* _holder;

public:

        RecursionLevelManager(KnobHolder* holder,
                              bool assertNotRecursive)
            : _holder(holder)
        {
            if (assertNotRecursive) {
                _holder->assertActionIsNotRecursive();
            }
            _holder->incrementRecursionLevel();
        }

        ~RecursionLevelManager()
        {
            _holder->decrementRecursionLevel();
        }
    };

public:

    int getRecursionLevel() const;

    /**
     * @brief When set to true any change which would trigger an instanceChanged action or cause a new
     * evaluation will not cause them to be called. Internally maintains a counter, when 0 evaluation
     * is enabled.
     **/
    void blockEvaluation();
    void unblockEvaluation();

    /**
     * @brief The virtual portion of notifyProjectBeginValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to prepare yourself to a lot of value changes.
     **/
    void beginKnobsValuesChanged_public(Natron::ValueChangedReasonEnum reason);

    /**
     * @brief The virtual portion of notifyProjectEndKnobsValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to finish a serie of value changes, thus limiting the amount of changes to do.
     **/
    void endKnobsValuesChanged_public(Natron::ValueChangedReasonEnum reason);


    /**
     * @brief The virtual portion of notifyProjectEvaluationRequested(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to do things when a value is changed. Bear in mind that you can compress
     * the change by using the begin/end[ValueChanges] to optimize the changes.
     **/
    virtual void onKnobValueChanged_public(KnobI* k,Natron::ValueChangedReasonEnum reason,SequenceTime time);


    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    void evaluate_public(KnobI* knob,bool isSignificant,Natron::ValueChangedReasonEnum reason);

    /**
     * @brief To be called after each function that modifies actionsRecursionLevel that is not
     * onKnobChanged or begin/endKnobValueChange.
     * If actionsRecursionLevel drops to 1 and there was some evaluate requested, it
     * will call evaluate_public
     **/
    void checkIfRenderNeeded();

    /*Add a knob to the vector. This is called by the
       Knob class. Don't call this*/
    void addKnob(boost::shared_ptr<KnobI> k);


    /*Removes a knob to the vector. This is called by the
       Knob class. Don't call this*/
    void removeKnob(KnobI* k);

    void initializeKnobsPublic();

    bool isSlave() const;

    ///Slave all the knobs of this holder to the other holder.
    void slaveAllKnobs(KnobHolder* other);

    void unslaveAllKnobs();
    
    /**
     * @brief Returns the local current time of the timeline
     **/
    virtual SequenceTime getCurrentTime() const;

protected:


    /**
     * @brief The virtual portion of notifyProjectBeginValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to prepare yourself to a lot of value changes.
     **/
    virtual void beginKnobsValuesChanged(Natron::ValueChangedReasonEnum reason)
    {
        (void)reason;
    }

    /**
     * @brief The virtual portion of notifyProjectEndKnobsValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to finish a serie of value changes, thus limiting the amount of changes to do.
     **/
    virtual void endKnobsValuesChanged(Natron::ValueChangedReasonEnum reason)
    {
        (void)reason;
    }

    /**
     * @brief The virtual portion of notifyProjectEvaluationRequested(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to do things when a value is changed. Bear in mind that you can compress
     * the change by using the begin/end[ValueChanges] to optimize the changes.
     **/
    virtual void onKnobValueChanged(KnobI* /*k*/,
                                    Natron::ValueChangedReasonEnum /*reason*/,
                                    SequenceTime /*time*/)
    {
    }

    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    virtual void evaluate(KnobI* knob,bool isSignificant,Natron::ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Called when the knobHolder is made slave or unslaved.
     * @param master The master knobHolder.
     * @param isSlave Whether this KnobHolder is now slaved or not.
     * If false, master will be NULL.
     **/
    virtual void onAllKnobsSlaved(bool /*isSlave*/,
                                  KnobHolder* /*master*/)
    {
    }

public:

    /**
     * @brief Same as onAllKnobsSlaved but called when only 1 knob is slaved
     **/
    virtual void onKnobSlaved(const boost::shared_ptr<KnobI> & /*knob*/,
                              int /*dimension*/,
                              bool /*isSlave*/,
                              KnobHolder* /*master*/)
    {
    }

private:


    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() = 0;
};

/**
 * @macro This special macro creates an object that will increment the recursion level in the constructor and decrement it in the
 * destructor.
 * @param assertNonRecursive If true then it will check that the recursion level is 0 and otherwise print a warning indicating that
 * this action was called recursively.
 **/
#define MANAGE_RECURSION(assertNonRecursive) KnobHolder::RecursionLevelManager actionRecursionManager(this,assertNonRecursive)

/**
 * @brief Should be called in the begining of any action that cannot be called recursively.
 **/
#define NON_RECURSIVE_ACTION() MANAGE_RECURSION(true)

/**
 * @brief Should be called in the begining of any action that can be called recursively
 **/
#define RECURSIVE_ACTION() MANAGE_RECURSION(false)

////Common interface for EffectInstance and backdrops
class NamedKnobHolder
    : public KnobHolder
{
public:

    NamedKnobHolder(AppInstance* instance)
        : KnobHolder(instance)
    {
    }

    virtual ~NamedKnobHolder()
    {
    }

    virtual std::string getName_mt_safe() const = 0;
};


template<typename K>
boost::shared_ptr<K> KnobHolder::createKnob(const std::string &description,
                                            int dimension) const
{
    return Natron::createKnob<K>(this, description,dimension);
}

#endif // NATRON_ENGINE_KNOB_H_
