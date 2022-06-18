/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef Engine_Knob_h
#define Engine_Knob_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <string>
#include <set>
#include <map>

#if (!defined(Q_MOC_RUN) && !defined(SBK_RUN)) || defined(SBK2_RUN)
#include <boost/enable_shared_from_this.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <QtCore/QReadWriteLock>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QtCore/QRecursiveMutex>
#endif

#include "Engine/Variant.h"
#include "Engine/AppManager.h" // for AppManager::createKnob
#include "Engine/OverlaySupport.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


#define NATRON_USER_MANAGED_KNOBS_PAGE_LABEL "User"
#define NATRON_USER_MANAGED_KNOBS_PAGE "userNatron"

NATRON_NAMESPACE_ENTER

class KnobSignalSlotHandler
    : public QObject
{
    Q_OBJECT

    KnobIWPtr k;

public:

    KnobSignalSlotHandler(const KnobIPtr &knob);

    KnobIPtr getKnob() const
    {
        return k.lock();
    }

    void s_animationLevelChanged(ViewSpec view,
                                 int dim)
    {
        Q_EMIT animationLevelChanged(view, dim);
    }

    void s_valueChanged(ViewSpec view,
                        int dimension,
                        int reason)
    {
        Q_EMIT valueChanged(view, dimension, reason);
    }

    void s_secretChanged()
    {
        Q_EMIT secretChanged();
    }

    void s_viewerContextSecretChanged()
    {
        Q_EMIT viewerContextSecretChanged();
    }

    void s_enabledChanged()
    {
        Q_EMIT enabledChanged();
    }

    void s_keyFrameSet(double time,
                       ViewSpec view,
                       int dimension,
                       int reason,
                       bool added)
    {
        Q_EMIT keyFrameSet(time, view, dimension, reason, added);
    }

    void s_keyFrameRemoved(double time,
                           ViewSpec view,
                           int dimension,
                           int reason)
    {
        Q_EMIT keyFrameRemoved(time, view, dimension, reason);
    }

    void s_multipleKeyFramesSet(const std::list<double>& keys,
                                ViewSpec view,
                                int dimension,
                                int reason)
    {
        Q_EMIT multipleKeyFramesSet(keys, view,  dimension, reason);
    }

    void s_multipleKeyFramesRemoved(const std::list<double>& keys,
                                    ViewSpec view,
                                    int dimension,
                                    int reason)
    {
        Q_EMIT multipleKeyFramesRemoved(keys, view,  dimension, reason);
    }

    void s_animationAboutToBeRemoved(ViewSpec view,
                                     int dimension)
    {
        Q_EMIT animationAboutToBeRemoved(view, dimension);
    }

    void s_animationRemoved(ViewSpec view,
                            int dimension)
    {
        Q_EMIT animationRemoved(view, dimension);
    }

    void s_knobSlaved(int dim,
                      bool slaved)
    {
        Q_EMIT knobSlaved(dim, slaved);
    }

    void s_setValueWithUndoStack(const Variant& v,
                                 ViewSpec view,
                                 int dim)
    {
        Q_EMIT setValueWithUndoStack(v, view, dim);
    }

    void s_appendParamEditChange(NATRON_ENUM::ValueChangedReasonEnum reason,
                                 Variant v,
                                 ViewSpec view,
                                 int dim,
                                 double time,
                                 bool createNewCommand,
                                 bool setKeyFrame)
    {
        Q_EMIT appendParamEditChange( (int)reason, v, view,  dim, time, createNewCommand, setKeyFrame );
    }

    void s_setDirty(bool b)
    {
        Q_EMIT dirty(b);
    }

    void s_setFrozen(bool f)
    {
        Q_EMIT frozenChanged(f);
    }

    void s_keyFrameMoved(ViewSpec view,
                         int dimension,
                         double oldTime,
                         double newTime)
    {
        Q_EMIT keyFrameMoved(view, dimension, oldTime, newTime);
    }

    void s_redrawGuiCurve(NATRON_ENUM::CurveChangeReason reason,
                          ViewSpec view,
                          int dimension)
    {
        Q_EMIT redrawGuiCurve( (int)reason, view, dimension );
    }

    void s_minMaxChanged(double mini,
                         double maxi,
                         int index)
    {
        Q_EMIT minMaxChanged(mini, maxi, index);
    }

    void s_displayMinMaxChanged(double mini,
                                double maxi,
                                int index)
    {
        Q_EMIT displayMinMaxChanged(mini, maxi, index);
    }

    void s_helpChanged()
    {
        Q_EMIT helpChanged();
    }

    void s_expressionChanged(int dimension)
    {
        Q_EMIT expressionChanged(dimension);
    }

    void s_derivativeMoved(double time,
                           ViewSpec view,
                           int dimension)
    {
        Q_EMIT derivativeMoved(time, view, dimension);
    }

    void s_keyFrameInterpolationChanged(double time,
                                        ViewSpec view,
                                        int dimension)
    {
        Q_EMIT keyFrameInterpolationChanged(time, view, dimension);
    }

    void s_hasModificationsChanged()
    {
        Q_EMIT hasModificationsChanged();
    }

    void s_labelChanged()
    {
        Q_EMIT labelChanged();
    }

    void s_inViewerContextLabelChanged()
    {
        Q_EMIT inViewerContextLabelChanged();
    }

    void s_evaluateOnChangeChanged(bool value)
    {
        Q_EMIT evaluateOnChangeChanged(value);
    }

    void s_dimensionNameChanged(int dimension)
    {
        Q_EMIT dimensionNameChanged(dimension);
    }

public Q_SLOTS:

    /**
     * @brief Calls KnobI::onAnimationRemoved
     **/
    void onAnimationRemoved(ViewSpec view, int dimension);

    void onMasterKeyFrameSet(double time, ViewSpec view, int dimension, int reason, bool added);

    void onMasterKeyFrameRemoved(double time, ViewSpec view, int dimension, int reason);

    void onMasterKeyFrameMoved(ViewSpec view, int dimension, double oldTime, double newTime);

    void onMasterAnimationRemoved(ViewSpec view, int dimension);


Q_SIGNALS:

    void evaluateOnChangeChanged(bool value);

    ///emitted whenever setAnimationLevel is called. It is meant to notify
    ///openfx params whether it is auto-keying or not.
    void animationLevelChanged(ViewSpec view, int dimension);

    ///Emitted when the value is changed with a reason different than eValueChangedReasonUserEdited
    ///This can happen as the result of a setValue() call from the plug-in or by
    ///a slaved knob whose master's value changed. The reason is passed in parameter.
    void valueChanged(ViewSpec view, int dimension, int reason);

    ///Emitted when the secret state of the knob changed
    void secretChanged();

    ///Emitted when the secret state of the knob changed in the viewer context
    void viewerContextSecretChanged();

    ///Emitted when a dimension enabled state changed
    void enabledChanged();

    ///This is called to notify the gui that the knob shouldn't be editable.
    ///Basically this is used when rendering with a Writer or for Trackers while tracking is active.
    void frozenChanged(bool frozen);

    ///Emitted whenever a keyframe is set with a reason different of eValueChangedReasonUserEdited
    ///@param added True if this is the first time that the keyframe was set
    void keyFrameSet(double time, ViewSpec view, int dimension, int reason, bool added);

    void multipleKeyFramesSet(std::list<double>, ViewSpec view, int dimension, int reason);

    void multipleKeyFramesRemoved(std::list<double>, ViewSpec view, int dimension, int reason);

    ///Emitted whenever a keyframe is removed with a reason different of eValueChangedReasonUserEdited
    void keyFrameRemoved(double time,  ViewSpec view, int dimension, int reason);

    void keyFrameMoved( ViewSpec view, int dimension, double oldTime, double newTime);

    void derivativeMoved(double time, ViewSpec view, int dimension);

    void keyFrameInterpolationChanged(double time, ViewSpec view, int dimension);

    /// Emitted when the gui curve has been cloned
    void redrawGuiCurve(int reason,  ViewSpec view, int dimension);

    ///Emitted whenever all keyframes of a dimension are about removed with a reason different of eValueChangedReasonUserEdited
    void animationAboutToBeRemoved(ViewSpec view, int dimension);

    ///Emitted whenever all keyframes of a dimension are effectively removed
    void animationRemoved(ViewSpec view, int dimension);

    ///Emitted whenever a knob is slaved via the slaveTo function with a reason of eValueChangedReasonPluginEdited.
    void knobSlaved(int dimension, bool slaved);

    ///Emitted whenever the GUI should set the value using the undo stack. This is
    ///only to address the problem of interacts that should use the undo/redo stack.
    void setValueWithUndoStack(Variant v,  ViewSpec view, int dim);

    ///Same as setValueWithUndoStack except that the value change will be compressed
    ///in a multiple edit undo/redo action
    void appendParamEditChange(int reason, Variant v, ViewSpec view, int dim, double time, bool createNewCommand, bool setKeyFrame);

    ///Emitted whenever the knob is dirty, @see KnobI::setDirty(bool)
    void dirty(bool);

    void minMaxChanged(double mini, double maxi, int index);

    void displayMinMaxChanged(double mini, double maxi, int index);

    void helpChanged();

    void expressionChanged(int dimension);

    void hasModificationsChanged();

    void labelChanged();

    void inViewerContextLabelChanged();

    void dimensionNameChanged(int dimension);
};

struct KnobChange
{
    KnobIPtr knob;
    NATRON_ENUM::ValueChangedReasonEnum reason, originalReason;
    bool originatedFromMainThread;
    bool refreshGui;
    double time;
    ViewSpec view;
    std::set<int> dimensionChanged;
    bool valueChangeBlocked;
};

typedef std::list<KnobChange> KnobChanges;


class KnobI
    : public OverlaySupport
    , public boost::enable_shared_from_this<KnobI>
{
    friend class KnobHolder;

public:
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobI()
    {
    }

public:

    // dtor
    virtual ~KnobI()
    {
    }

protected:
    /**
     * @brief Deletes this knob permanently
     **/
    virtual void deleteKnob() = 0;

public:

    struct ListenerDim
    {
        bool isExpr;
        bool isListening;
        int targetDim;
        ListenerDim()
            : isExpr(false), isListening(false), targetDim(-1) {}
    };

    typedef std::map<KnobIWPtr, std::vector<ListenerDim> > ListenerDimsMap;

    /**
     * @brief Do not call this. It is called right away after the constructor by the factory
     * to initialize curves and values. This is separated from the constructor as we need RTTI
     * for Curve.
     **/
    virtual void populate() = 0;
    virtual void setKnobGuiPointer(const boost::shared_ptr<KnobGuiI>& ptr) = 0;
    virtual boost::shared_ptr<KnobGuiI> getKnobGuiPointer() const = 0;
    virtual bool getAllDimensionVisible() const = 0;
    static bool areTypesCompatibleForSlave(KnobI* lhs, KnobI* rhs);

    /**
     * @brief Returns the knob was created by a plugin or added automatically by Natron (e.g like mask knobs)
     **/
    virtual bool isDeclaredByPlugin() const = 0;

    /**
     * @brief Must flag that the knob was dynamically created to warn the gui it should handle it correctly
     **/
    virtual void setDynamicallyCreated() = 0;
    virtual bool isDynamicallyCreated() const = 0;

    /**
     * @brief A user knob is a knob created by the user by the gui
     **/
    virtual void setAsUserKnob(bool b) = 0;
    virtual bool isUserKnob() const = 0;


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
     * @brief Returns true if the knob has had modifications
     **/
    virtual bool hasModifications() const = 0;
    virtual bool hasModifications(int dimension) const = 0;
    virtual bool hasModificationsForSerialization() const = 0;
    virtual void computeHasModifications() = 0;
    virtual void checkAnimationLevel(ViewSpec view, int dimension) = 0;

    /**
     * @brief If the parameter is multidimensional, this is the label that will be displayed for a dimension.
     **/
    virtual std::string getDimensionName(int dimension) const = 0;
    virtual void setDimensionName(int dim, const std::string & name) = 0;


    /**
     * @brief When set to true the evaluate (render) action will not be called
     * when issuing value changes. Internally it maintains a counter, when it reaches 0 the evaluation is unblocked.
     **/
    virtual void beginChanges() = 0;

    /**
     * @brief To be called to reactivate evaluation. Internally it maintains a counter, when it reaches 0 the evaluation is unblocked.
     **/
    virtual void endChanges() = 0;
    virtual void blockValueChanges() = 0;
    virtual void unblockValueChanges() = 0;
    virtual bool isValueChangesBlocked() const = 0;
    virtual void blockListenersNotification() = 0;
    virtual void unblockListenersNotification() = 0;
    virtual bool isListenersNotificationBlocked() const = 0;

    /**
     * @brief Called by setValue to refresh the GUI, call the instanceChanged action on the plugin and
     * evaluate the new value (cause a render).
     * @returns true if the knobChanged handler was called once for this knob
     **/
    virtual bool evaluateValueChange(int dimension, double time, ViewSpec view, NATRON_ENUM::ValueChangedReasonEnum reason) = 0;

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
    virtual void clone(KnobI* other, int dimension = -1, int otherDimension = -1) = 0;
    virtual void clone(const KnobIPtr & other,
                       int dimension = -1,
                       int otherDimension = -1)
    {
        clone( other.get(), dimension, otherDimension );
    }

    /**
     * @brief Performs the same as clone but also refresh any gui it has.
     **/
    virtual void cloneAndUpdateGui(KnobI* other, int dimension = -1, int otherDimension = -1) = 0;
    virtual void cloneDefaultValues(KnobI* other) = 0;

    /**
     * @brief Same as clone but returns whether the knob state changed as the result of the clone operation
     **/
    virtual bool cloneAndCheckIfChanged(KnobI* other, int dimension = -1, int otherDimension = -1) = 0;

    /**
     * @brief Same as clone(const KnobIPtr& ) except that the given offset is applied
     * on the keyframes time and only the keyframes within the given range are copied.
     * If the range is NULL everything will be copied.
     *
     * Note that unlike the other version of clone, this version is more relaxed and accept parameters
     * with different dimensions, but only the intersection of the dimension of the 2 parameters will be copied.
     * The restriction on types still apply.
     **/
    virtual void clone(KnobI* other, double offset, const RangeD* range, int dimension = -1, int otherDimension = -1) = 0;
    virtual void clone(const KnobIPtr & other,
                       double offset,
                       const RangeD* range,
                       int dimension = -1,
                       int otherDimension = -1)
    {
        clone(other.get(), offset, range, dimension, otherDimension);
    }

    /**
     * @brief Must return the curve used by the GUI of the parameter
     **/
    virtual boost::shared_ptr<Curve> getGuiCurve(ViewSpec view, int dimension, bool byPassMaster = false) const = 0;
    virtual double random(double min, double max, double time, unsigned int seed = 0) const = 0;
    virtual double random(double min = 0., double max = 1.) const = 0;
    virtual int randomInt(int min, int max, double time, unsigned int seed = 0) const = 0;
    virtual int randomInt(int min, int max) const = 0;

    /**
     * @brief Evaluates the curve at the given dimension and at the given time. This returns the value of the curve directly.
     * If the knob is holding a string, it will return the index.
     **/
    virtual double getRawCurveValueAt(double time, ViewSpec view, int dimension)  = 0;

    /**
     * @brief Same as getRawCurveValueAt, but first check if an expression is present. The expression should return a PoD.
     **/
    virtual double getValueAtWithExpression(double time, ViewSpec view, int dimension) = 0;

protected:


    /**
     * @brief Removes all the keyframes in the given dimension.
     **/
    virtual void removeAnimationWithReason(ViewSpec view, int dimension, NATRON_ENUM::ValueChangedReasonEnum reason) = 0;

public:

    /**
     * @brief Removes the keyframe at the given time and dimension if it matches any.
     **/
    virtual void deleteValueAtTime(NATRON_ENUM::CurveChangeReason curveChangeReason, double time, ViewSpec view, int dimension, bool copyCurveValueAtTimeToInternalValue) = 0;
    virtual void deleteValuesAtTime(NATRON_ENUM::CurveChangeReason curveChangeReason, const std::list<double>& times, ViewSpec view, int dimension, bool copyCurveValueAtTimeToInternalValue) = 0;


    /**
     * @brief Moves a keyframe by a given delta and emits the signal keyframeMoved
     **/
    virtual bool moveValueAtTime(NATRON_ENUM::CurveChangeReason reason, double time, ViewSpec view,  int dimension, double dt, double dv, KeyFrame* newKey) = 0;
    virtual bool moveValuesAtTime(NATRON_ENUM::CurveChangeReason reason, ViewSpec view,  int dimension, double dt, double dv, std::vector<KeyFrame>* keyframes) = 0;

    /**
     * @brief Transforms a keyframe by a given matrix. The matrix must not contain any skew or rotation.
     **/
    virtual bool transformValueAtTime(NATRON_ENUM::CurveChangeReason curveChangeReason, double time, ViewSpec view,  int dimension, const Transform::Matrix3x3& matrix, KeyFrame* newKey) = 0;
    virtual bool transformValuesAtTime(NATRON_ENUM::CurveChangeReason curveChangeReason, ViewSpec view,  int dimension, const Transform::Matrix3x3& matrix, std::vector<KeyFrame>* keyframes) = 0;

    /**
     * @brief Copies all the animation of *curve* into the animation curve at the given dimension.
     **/
    virtual void cloneCurve(ViewSpec view, int dimension, const Curve& curve) = 0;

    /**
     * @brief Changes the interpolation type for the given keyframe
     **/
    virtual bool setInterpolationAtTime(NATRON_ENUM::CurveChangeReason reason, ViewSpec view, int dimension, double time, NATRON_ENUM::KeyframeTypeEnum interpolation, KeyFrame* newKey) = 0;

    /**
     * @brief Set the left/right derivatives of the control point at the given time.
     **/
    virtual bool moveDerivativesAtTime(NATRON_ENUM::CurveChangeReason reason, ViewSpec view, int dimension, double time, double left, double right) = 0;
    virtual bool moveDerivativeAtTime(NATRON_ENUM::CurveChangeReason reason, ViewSpec view, int dimension, double time, double derivative, bool isLeft) = 0;

    /**
     * @brief Removes animation before the given time and dimension. If the reason is different than eValueChangedReasonUserEdited
     * a signal will be emitted
     **/
    virtual void deleteAnimationBeforeTime(double time, ViewSpec view, int dimension, NATRON_ENUM::ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Removes animation before the given time and dimension. If the reason is different than eValueChangedReasonUserEdited
     * a signal will be emitted
     **/
    virtual void deleteAnimationAfterTime(double time, ViewSpec view, int dimension, NATRON_ENUM::ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Calls removeAnimation with a reason of eValueChangedReasonNatronInternalEdited.
     **/
    void removeAnimation(ViewSpec view, int dimension);

    /**
     * @brief Calls deleteValueAtTime with a reason of eValueChangedReasonUserEdited
     **/
    virtual void onKeyFrameRemoved(double time, ViewSpec view, int dimension, bool copyCurveValueAtTimeToInternalValue) = 0;

    /**
     * @brief Calls removeAnimation with a reason of eValueChangedReasonUserEdited
     **/
    void onAnimationRemoved(ViewSpec view, int dimension);

    /**
     * @brief Set an expression on the knob. If this expression is invalid, this function throws an excecption with the error from the
     * Python interpreter.
     * @param hasRetVariable If true the expression is expected to be multi-line and have its return value set to the variable "ret", otherwise
     * the expression is expected to be single-line.
     * @param force If set to true, this function will not check if the expression is valid nor will it attempt to compile/evaluate it, it will
     * just store it. This flag is used for serialisation, you should always pass false
     **/

protected:
    virtual void setExpressionInternal(int dimension, const std::string& expression, bool hasRetVariable, bool clearResults, bool failIfInvalid) = 0;

public:

    void restoreExpression(int dimension,
                           const std::string& expression,
                           bool hasRetVariable)
    {
        setExpressionInternal(dimension, expression, hasRetVariable, false, false);
    }

    void setExpression(int dimension,
                       const std::string& expression,
                       bool hasRetVariable,
                       bool failIfInvalid)
    {
        setExpressionInternal(dimension, expression, hasRetVariable, true, failIfInvalid);
    }

    /**
     * @brief Tries to re-apply invalid expressions, returns true if they are all valid
     **/
    virtual bool checkInvalidExpressions() = 0;
    virtual bool isExpressionValid(int dimension, std::string* error) const = 0;
    virtual void setExpressionInvalid(int dimension, bool valid, const std::string& error) = 0;


    /**
     * @brief For each dimension, try to find in the expression, if set, the node name "oldName" and replace
     * it by "newName"
     **/
    virtual void replaceNodeNameInExpression(int dimension,
                                             const std::string& oldName,
                                             const std::string& newName) = 0;
    virtual void clearExpressionsResults(int dimension) = 0;
    virtual void clearExpression(int dimension, bool clearResults) = 0;
    virtual std::string getExpression(int dimension) const = 0;

    /**
     * @brief Checks that the given expr for the given dimension will produce a correct behaviour.
     * On success this function returns correctly, otherwise an exception is thrown with the error.
     * This function also declares some extra python variables via the declareCurrentKnobVariable_Python function.
     * The new expression is returned.
     * @param resultAsString[out] The result of the execution of the expression will be written to the string.
     * @returns A new string containing the modified expression with the 'ret' variable declared if it wasn't already declared
     * by the user.
     **/
    virtual std::string validateExpression(const std::string& expression, int dimension, bool hasRetVariable,
                                           std::string* resultAsString) = 0;

protected:

    virtual void refreshListenersAfterValueChange(ViewSpec view, NATRON_ENUM::ValueChangedReasonEnum reason, int dimension) = 0;

public:

    /**
     * @brief Returns whether the expr at the given dimension uses the ret variable to assign to the return value or not
     **/
    virtual bool isExpressionUsingRetVariable(int dimension = 0) const = 0;

    /**
     * @brief Returns in dependencies a list of all the knobs used in the expression at the given dimension
     * @returns True on success, false if no expression is set.
     **/
    virtual bool getExpressionDependencies(int dimension, std::list<std::pair<KnobIWPtr, int> >& dependencies) const = 0;


    /**
     * @brief Calls setValueAtTime with a reason of eValueChangedReasonUserEdited.
     **/
    virtual bool onKeyFrameSet(double time, ViewSpec view, int dimension) = 0;
    virtual bool onKeyFrameSet(double time, ViewSpec view, const KeyFrame& key, int dimension) = 0;
    virtual bool setKeyFrame(const KeyFrame& key, ViewSpec view,  int dimension, NATRON_ENUM::ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Called when the current time of the timeline changes.
     * It must get the value at the given time and notify  the gui it must
     * update the value displayed.
     **/
    virtual void onTimeChanged(bool isPlayback, double time) = 0;

    /**
     * @brief Compute the derivative at time as a double
     **/
    virtual double getDerivativeAtTime(double time, ViewSpec view, int dimension = 0)  = 0;

    /**
     * @brief Compute the integral of dimension from time1 to time2 as a double
     **/
    virtual double getIntegrateFromTimeToTime(double time1, double time2, ViewSpec view, int dimension = 0)  = 0;

    /**
     * @brief Places in time the keyframe time at the given index.
     * If it exists the function returns true, false otherwise.
     **/
    virtual bool getKeyFrameTime(ViewSpec view, int index, int dimension, double* time) const = 0;

    /**
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the last
     * keyframe.
     **/
    virtual bool getLastKeyFrameTime(ViewSpec view, int dimension, double* time) const = 0;

    /**
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the first
     * keyframe.
     **/
    virtual bool getFirstKeyFrameTime(ViewSpec view, int dimension, double* time) const = 0;

    /**
     * @brief Returns the count of keyframes in the given dimension.
     **/
    virtual int getKeyFramesCount(ViewSpec view, int dimension) const = 0;

    /**
     * @brief Returns the nearest keyframe time if it was found.
     * Returns true if it succeeded, false otherwise.
     **/
    virtual bool getNearestKeyFrameTime(ViewSpec view, int dimension, double time, double* nearestTime) const = 0;

    /**
     * @brief Returns the keyframe index if there's any keyframe in the curve
     * at the given dimension and the given time. -1 otherwise.
     **/
    virtual int getKeyFrameIndex(ViewSpec view, int dimension, double time) const = 0;

    /**
     * @brief Returns a pointer to the curve in the given dimension.
     * It cannot be a null pointer.
     **/
    virtual boost::shared_ptr<Curve> getCurve(ViewSpec view, int dimension, bool byPassMaster = false) const = 0;

    /**
     * @brief Returns true if the dimension is animated with keyframes.
     **/
    virtual bool isAnimated( int dimension, ViewSpec view = ViewSpec::current() ) const = 0;

    /**
     * @brief Returns true if at least 1 dimension is animated. MT-Safe
     **/
    virtual bool hasAnimation() const = 0;

    /**
     * @brief Returns a const ref to the curves held by this knob. This is MT-safe as they're
     * never deleted (except on program exit).
     **/
    virtual const std::vector<boost::shared_ptr<Curve>  > & getCurves() const = 0;

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
     * @brief Get the knob label, that is the label next to the knob on the user interface.
     * This function is MT-safe as it the label can only be changed by the main thread.
     **/
    virtual std::string getLabel() const = 0;
    virtual void setLabel(const std::string& label) = 0;

    void setLabel(const QString & label)
    {
        setLabel( label.toStdString() );
    }

    /**
     * @brief Set an icon instead of the text label for this knob
     * @param modeOff If true, this icon will be used when the parameter is an unchecked state (only relevant for
     * buttons/booleans parameters), otherwise the icon will be used when the parameter is in a checked state
     **/
    virtual void setIconLabel(const std::string& iconFilePath, bool checked = false) = 0;
    virtual const std::string& getIconLabel(bool checked = false) const = 0;

    /**
     * @brief Returns a pointer to the holder owning the knob.
     **/
    virtual KnobHolder* getHolder() const = 0;
    virtual void setHolder(KnobHolder* holder) = 0;

    /**
     * @brief Get the knob dimension. MT-safe as it is static and never changes.
     **/
    virtual int getDimension() const = 0;

    /**
     * @brief Any GUI representing this parameter should represent the next parameter on the same line as this parameter.
     **/
    virtual void setAddNewLine(bool newLine) = 0;
    virtual void setAddSeparator(bool addSep) = 0;

    /**
     * @brief Any GUI representing this parameter should represent the next parameter on the same line as this parameter.
     **/
    virtual bool isNewLineActivated() const = 0;
    virtual bool isSeparatorActivated() const = 0;

    /**
     * @brief GUI-related
     **/
    virtual void setSpacingBetweenItems(int spacing) = 0;
    virtual int getSpacingBetweenitems() const = 0;

    /**
     * @brief Set the label of the knob on the viewer
     **/
    virtual std::string getInViewerContextLabel() const = 0;
    virtual void setInViewerContextLabel(const QString& label) = 0;

    /**
     * @brief Determines whether this knob can be assigned a shortcut or not via the shortcut editor.
     * If true, Natron will look for a shortcut in the shortcuts database with an ID matching the name of this
     * parameter. To set default values for shortcuts, implement EffectInstance::getPluginShortcuts(...)
     **/
    virtual void setInViewerContextCanHaveShortcut(bool haveShortcut) = 0;
    virtual bool getInViewerContextHasShortcut() const = 0;

    /**
     * @brief Returns whether this type of knob can be instantiated in the viewer UI
     **/
    virtual bool supportsInViewerContext() const
    {
        return false;
    }

    /**
     * @brief Set how much space (in pixels) to leave between the current parameter and the next parameter in horizontal layouts.
     **/
    virtual void setInViewerContextItemSpacing(int spacing) = 0;
    virtual int  getInViewerContextItemSpacing() const = 0;

    /**
     * @brief Set whether the knob should have a vertical separator after or not in the viewer
     **/
    virtual void setInViewerContextAddSeparator(bool addSeparator) = 0;
    virtual bool  getInViewerContextAddSeparator() const = 0;

    /**
     * @brief Set whether the viewer UI should create a new line after this parameter or not
     **/
    virtual void setInViewerContextNewLineActivated(bool activated) = 0;
    virtual bool  getInViewerContextNewLineActivated() const = 0;

    /**
     * @brief Set whether the knob should have its viewer GUI secret or not
     **/
    virtual void setInViewerContextSecret(bool secret) = 0;
    virtual bool  getInViewerContextSecret() const = 0;

    /**
     * @brief Enables/disables user interaction with the given dimension.
     **/
    virtual void setEnabled(int dimension, bool b) = 0;
    virtual void setDefaultEnabled(int dimension, bool b) = 0;

    /**
     * @brief Is the dimension enabled ?
     **/
    virtual bool isEnabled(int dimension) const = 0;
    virtual bool isDefaultEnabled(int dimension) const = 0;

    /**
     * @brief Convenience function, same as calling setEnabled(int,bool) for all dimensions.
     **/
    virtual void setAllDimensionsEnabled(bool b) = 0;
    virtual void setDefaultAllDimensionsEnabled(bool b) = 0;

    /**
     * @brief Set the knob visible/invisible on the GUI representing it.
     **/
    virtual void setSecret(bool b) = 0;
    virtual void setSecretByDefault(bool b) = 0;

    /**
     * @brief Is the knob visible to the user ?
     **/
    virtual bool getIsSecret() const = 0;
    virtual bool getDefaultIsSecret() const = 0;

    /**
     * @brief Returns true if a knob is secret because it is either itself secret or one of its parent, recursively
     **/
    virtual bool getIsSecretRecursive() const = 0;

    /**
     * @biref This is called to notify the gui that the knob shouldn't be editable.
     * Basically this is used when rendering with a Writer or for Trackers while tracking is active.
     **/
    virtual void setIsFrozen(bool frozen) = 0;
    virtual bool evaluateValueChangeOnTimeChange() const { return false; }

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
     * name is the same as the getLabel(i.e: the text label).
     */
    virtual void setName(const std::string & name, bool throwExceptions = false) = 0;

    /**
     * @brief Returns the knob name. By default the
     * name is the same as the getLabel(i.e: the text label).
     */
    virtual const std::string & getName() const = 0;

    /**
     * @brief Returns the name passed to setName(). This might be different than getName() if
     * the name passed to setName() was not python compliant.
     **/
    virtual const std::string & getOriginalName() const = 0;

    /**
     * @brief Set the given knob as the parent of this knob.
     * @param knob It must be a tab or group knob.
     */
    virtual void setParentKnob(KnobIPtr knob) = 0;
    virtual void resetParent() = 0;

    /**
     * @brief Returns a pointer to the parent knob if any.
     */
    virtual KnobIPtr getParentKnob() const = 0;

    /**
     * @brief Returns the hierarchy level of this knob.
     * By default it is 0, however a knob with a parent will
     * have a hierarchy level of 1, etc...
     **/
    virtual int determineHierarchySize() const = 0;

    /**
     * @brief If a knob is evaluating on change, that means
     * every time a value changes, the knob will call the
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
    virtual bool getIsPersistent() const = 0;

    /**
     * @brief Should the knob be saved in the project ?
     * By default this is set to true.
     **/
    virtual void setIsPersistent(bool b) = 0;

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
     * @brief When a knob is clip preferences slaves, when its value changes, checkOFXClipPreferences will be called on the
     * holder effect
     **/
    virtual void setIsMetadataSlave(bool slave) = 0;
    virtual bool getIsMetadataSlave() const = 0;

    /**
     * @brief Set the text displayed by the tooltip when
     * the user hovers the knob with the mouse.
     **/
    virtual void setHintToolTip(const std::string & hint) = 0;

    void setHintToolTip(const QString & hint)
    {
        setHintToolTip( hint.toStdString() );
    }

    //void setHintToolTip(const char* hint)
    //{
    //    setHintToolTip(std::string(hint));
    //}

    /**
     * @brief Get the tooltip text.
     **/
    virtual const std::string & getHintToolTip() const = 0;

    /**
     * @brief Returns whether the hint is encoded in markdown or not
     **/
    virtual bool isHintInMarkdown() const = 0;
    virtual void setHintIsMarkdown(bool b) = 0;

    /**
     * @brief Call this to set a custom interact entry point, replacing any existing gui.
     **/
    virtual void setCustomInteract(const boost::shared_ptr<OfxParamOverlayInteract> & interactDesc) = 0;
    virtual boost::shared_ptr<OfxParamOverlayInteract> getCustomInteract() const = 0;
    virtual void swapOpenGLBuffers() OVERRIDE = 0;
    virtual void redraw() OVERRIDE = 0;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE = 0;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE = 0;
#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE = 0;
#endif
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE = 0;
    virtual unsigned int getCurrentRenderScale() const OVERRIDE FINAL { return 0; }

    virtual RectD getViewportRect() const OVERRIDE = 0;
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE = 0;
    virtual void saveOpenGLContext() OVERRIDE = 0;
    virtual void restoreOpenGLContext() OVERRIDE = 0;

    /**
     * @brief Returns whether any interact associated to this knob should be drawn or not
     **/
    bool shouldDrawOverlayInteract() const;

    /**
     * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
     **/
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE = 0;

    /**
     * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
     **/
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE = 0;

    /**
     * @brief Returns the font height, i.e: the height of the highest letter for this font
     **/
    virtual int getWidgetFontHeight() const OVERRIDE = 0;

    /**
     * @brief Returns for a string the estimated pixel size it would take on the widget
     **/
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE = 0;

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
     * @returns true if any value was dequeued
     **/
    virtual bool dequeueValuesSet(bool disableEvaluation) = 0;

    /**
     * @brief Returns the current time if attached to a timeline or the time being rendered
     **/
    virtual double getCurrentTime() const = 0;

    /**
     * @brief Returns the current view being rendered
     **/
    virtual ViewIdx getCurrentView() const = 0;
    virtual boost::shared_ptr<KnobSignalSlotHandler> getSignalSlotHandler() const = 0;

    /**
     * @brief Adds a new listener to this knob. This is just a pure notification about the fact that the given knob
     * is now listening to the values/keyframes of "this" either via a hard-link (slave/master) or via its expressions
     * @param isExpression Is this listener listening through expressions or via a slave/master link
     * @param listenerDimension The dimension of the listener that is listening to this knob
     * @param listenedDimension The dimension of this knob that is listened to by the listener
     **/
    virtual void addListener(const bool isExpression,
                             const int listenerDimension,
                             const int listenedToDimension,
                             const KnobIPtr& listener) = 0;
    virtual void getAllExpressionDependenciesRecursive(std::set<NodePtr>& nodes) const = 0;

private:
    virtual void removeListener(KnobI* listener, int listenerDimension) = 0;

public:

    virtual bool useHostOverlayHandle() const { return false; }

protected:

    virtual bool setHasModifications(int dimension, bool value, bool lock) = 0;

    /**
     * @brief Slaves the value for the given dimension to the curve
     * at the same dimension for the knob 'other'.
     * In case of success, this function returns true, otherwise false.
     **/
    virtual bool slaveToInternal(int dimension, const KnobIPtr &  other, int otherDimension, NATRON_ENUM::ValueChangedReasonEnum reason,
                                 bool ignoreMasterPersistence) = 0;

    /**
     * @brief Unslaves a previously slaved dimension. The implementation should assert that
     * the dimension was really slaved.
     **/
    virtual void unSlaveInternal(int dimension, NATRON_ENUM::ValueChangedReasonEnum reason, bool copyState) = 0;

public:

    virtual void onKnobAboutToAlias(const KnobIPtr& /*slave*/) {}


    /**
     * @brief Calls slaveTo with a value changed reason of eValueChangedReasonNatronInternalEdited.
     * @param ignoreMasterPersistence If true the master will not be serialized.
     **/
    bool slaveTo(int dimension, const KnobIPtr & other, int otherDimension, bool ignoreMasterPersistence = false);


    void storeLink(int slaveDimension,
                   const std::string& nodeNameFull,
                   const std::string& nodeName,
                   const std::string& trackName,
                   const std::string& knobName,
                   int dimension)
    {
        _links.push_back(Link(slaveDimension, nodeNameFull, nodeName, trackName, knobName, dimension));
    }

    void restoreLinks(const NodesList & allNodes,
                      const std::map<std::string, std::string>& oldNewScriptNamesMapping,
                      bool throwOnFailure);

private:
    struct Link {
        int slaveDimension;
        std::string nodeNameFull;
        std::string nodeName;
        std::string trackName;
        std::string knobName;
        int dimension; // -1 means alias

        Link(int slaveDimension_,
             const std::string& nodeNameFull_,
             const std::string& nodeName_,
             const std::string& trackName_,
             const std::string& knobName_,
             int dimension_)
        : slaveDimension(slaveDimension_)
        , nodeNameFull(nodeNameFull_)
        , nodeName(nodeName_)
        , trackName(trackName_)
        , knobName(knobName_)
        , dimension(dimension_)
        {}
    };

    std::vector<Link> _links;

public:

    virtual bool isMastersPersistenceIgnored() const = 0;
    virtual KnobIPtr createDuplicateOnHolder(KnobHolder* otherHolder,
                                            const boost::shared_ptr<KnobPage>& page,
                                            const boost::shared_ptr<KnobGroup>& group,
                                            int indexInParent,
                                            bool makeAlias,
                                            const std::string& newScriptName,
                                            const std::string& newLabel,
                                            const std::string& newToolTip,
                                            bool refreshParams,
                                            bool isUserKnob) = 0;

    /**
     * @brief If a knob was created using createDuplicateOnHolder(effect,true), this function will return true
     **/
    virtual KnobIPtr getAliasMaster() const = 0;
    virtual bool setKnobAsAliasOfThis(const KnobIPtr& master, bool doAlias) = 0;

    /**
     * @brief Calls slaveTo with a value changed reason of eValueChangedReasonUserEdited.
     **/
    void onKnobSlavedTo(int dimension, const KnobIPtr& other, int otherDimension);


    /**
     * @brief Calls unSlave with a value changed reason of eValueChangedReasonNatronInternalEdited.
     **/
    void unSlave(int dimension, bool copyState);

    /**
     * @brief Returns a list of all the knobs whose value depends upon this knob.
     **/
    virtual void getListeners(ListenerDimsMap & listeners) const = 0;

    /**
     * @brief Calls unSlave with a value changed reason of eValueChangedReasonUserEdited.
     **/
    void onKnobUnSlaved(int dimension);

    /**
     * @brief Returns a valid pointer to a knob if the value at
     * the given dimension is slaved.
     **/
    virtual std::pair<int, KnobIPtr> getMaster(int dimension) const = 0;

    /**
     * @brief Returns true if the value at the given dimension is slave to another parameter
     **/
    virtual bool isSlave(int dimension) const = 0;

    /**
     * @brief Get the current animation level.
     **/
    virtual NATRON_ENUM::AnimationLevelEnum getAnimationLevel(int dimension) const = 0;

    /**
     * @brief Restores the default value
     **/
    virtual void resetToDefaultValue(int dimension) = 0;
    virtual void resetToDefaultValueWithoutSecretNessAndEnabledNess(int dimension) = 0;

    /**
     * @brief Must return true if this Lnob holds a POD (plain old data) type, i.e. int, bool, or double.
     **/
    virtual bool isTypePOD() const = 0;

    /**
     * @brief Must return true if the other knobs type can convert to this knob's type.
     **/
    virtual bool isTypeCompatible(const KnobIPtr & other) const = 0;
    boost::shared_ptr<KnobPage> getTopLevelPage() const;
};


///Skins the API of KnobI by implementing most of the functions in a non templated manner.
struct KnobHelperPrivate;
class KnobHelper
    : public KnobI
{
    Q_DECLARE_TR_FUNCTIONS(KnobHelper)

    // friends
    friend class KnobHolder;

public:

    enum ValueChangedReturnCodeEnum
    {
        eValueChangedReturnCodeNoKeyframeAdded = 0,
        eValueChangedReturnCodeKeyframeModified,
        eValueChangedReturnCodeKeyframeAdded,
        eValueChangedReturnCodeNothingChanged,
    };

    /**
     * @brief Creates a new Knob that belongs to the given holder, with the given label.
     * The name of the knob will be equal to the label, you can change it by calling setName()
     * The dimension parameter indicates how many dimension the knob should have.
     * If declaredByPlugin is false then Natron will never call onKnobValueChanged on the holder.
     **/
    KnobHelper(KnobHolder*  holder,
               const std::string &label,
               int dimension = 1,
               bool declaredByPlugin = true);

    virtual ~KnobHelper();

private:
    virtual void deleteKnob() OVERRIDE FINAL;

public:

    struct KnobTLSData
    {
        int expressionRecursionLevel;

        KnobTLSData()
            : expressionRecursionLevel(0)
        {
        }
    };

    typedef boost::shared_ptr<KnobTLSData> KnobDataTLSPtr;

    virtual void setKnobGuiPointer(const boost::shared_ptr<KnobGuiI>& ptr) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobGuiI> getKnobGuiPointer() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getAllDimensionVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    /**
     * @brief Returns the knob was created by a plugin or added automatically by Natron (e.g like mask knobs)
     **/
    virtual bool isDeclaredByPlugin() const OVERRIDE FINAL;
    virtual void setAsInstanceSpecific() OVERRIDE FINAL;
    virtual bool isInstanceSpecific() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setDynamicallyCreated() OVERRIDE FINAL;
    virtual bool isDynamicallyCreated() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAsUserKnob(bool b) OVERRIDE FINAL;
    virtual bool isUserKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN;
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
    virtual void beginChanges() OVERRIDE FINAL;
    virtual void endChanges() OVERRIDE FINAL;
    virtual void blockValueChanges() OVERRIDE FINAL;
    virtual void unblockValueChanges() OVERRIDE FINAL;
    virtual bool isValueChangesBlocked() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void blockListenersNotification() OVERRIDE FINAL;
    virtual void unblockListenersNotification() OVERRIDE FINAL;
    virtual bool isListenersNotificationBlocked() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool evaluateValueChange(int dimension, double time, ViewSpec view,  NATRON_ENUM::ValueChangedReasonEnum reason) OVERRIDE FINAL;

protected:
    // Returns true if the knobChanged handler was called
    bool evaluateValueChangeInternal(int dimension,
                                     double time,
                                     ViewSpec view,
                                     NATRON_ENUM::ValueChangedReasonEnum reason,
                                     NATRON_ENUM::ValueChangedReasonEnum originalReason);

    virtual void onInternalValueChanged(int /*dimension*/,
                                        double /*time*/,
                                        ViewSpec /*view*/)
    {
    }

public:


    virtual double random(double min, double max, double time, unsigned int seed = 0) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double random(double min = 0., double max = 1.) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int randomInt(int min, int max, double time, unsigned int seed = 0) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int randomInt(int min, int max) const OVERRIDE FINAL WARN_UNUSED_RETURN;

protected:


    void randomSeed(double time, unsigned int seed) const;

#ifdef DEBUG
    //Helper to set breakpoints in templated code
    void debugHook();
#endif

private:


    virtual void removeAnimationWithReason(ViewSpec view, int dimension, NATRON_ENUM::ValueChangedReasonEnum reason) OVERRIDE FINAL;
    virtual void deleteValueAtTime(NATRON_ENUM::CurveChangeReason curveChangeReason, double time, ViewSpec view,  int dimension,bool copyCurveValueAtTimeToInternalValue) OVERRIDE FINAL;
    virtual void deleteValuesAtTime(NATRON_ENUM::CurveChangeReason curveChangeReason, const std::list<double>& times, ViewSpec view, int dimension, bool copyCurveValueAtTimeToInternalValue) OVERRIDE FINAL;

public:

    virtual void onKeyFrameRemoved(double time, ViewSpec view, int dimension, bool copyCurveValueAtTimeToInternalValue) OVERRIDE FINAL;
    virtual bool moveValueAtTime(NATRON_ENUM::CurveChangeReason reason, double time, ViewSpec view, int dimension, double dt, double dv, KeyFrame* newKey) OVERRIDE FINAL;
    virtual bool moveValuesAtTime(NATRON_ENUM::CurveChangeReason reason, ViewSpec view,  int dimension, double dt, double dv, std::vector<KeyFrame>* keyframes) OVERRIDE FINAL;

private:
    bool moveValueAtTimeInternal(bool useGuiCurve, double time, ViewSpec view, int dimension, double dt, double dv, KeyFrame* newKey);

public:


    virtual bool transformValueAtTime(NATRON_ENUM::CurveChangeReason curveChangeReason, double time, ViewSpec view, int dimension, const Transform::Matrix3x3& matrix, KeyFrame* newKey) OVERRIDE FINAL;
    virtual bool transformValuesAtTime(NATRON_ENUM::CurveChangeReason curveChangeReason, ViewSpec view,  int dimension, const Transform::Matrix3x3& matrix, std::vector<KeyFrame>* keyframes) OVERRIDE FINAL;

private:
    bool transformValueAtTimeInternal(bool useGuiCurve, double time, ViewSpec view, int dimension, const Transform::Matrix3x3& matrix, KeyFrame* newKey);

public:

    virtual void cloneCurve(ViewSpec view, int dimension, const Curve& curve) OVERRIDE FINAL;
    virtual bool setInterpolationAtTime(NATRON_ENUM::CurveChangeReason reason, ViewSpec view, int dimension, double time, NATRON_ENUM::KeyframeTypeEnum interpolation, KeyFrame* newKey) OVERRIDE FINAL;
    virtual bool moveDerivativesAtTime(NATRON_ENUM::CurveChangeReason reason, ViewSpec view, int dimension, double time, double left, double right)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool moveDerivativeAtTime(NATRON_ENUM::CurveChangeReason reason, ViewSpec view, int dimension, double time, double derivative, bool isLeft) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void deleteAnimationBeforeTime(double time, ViewSpec view, int dimension, NATRON_ENUM::ValueChangedReasonEnum reason) OVERRIDE FINAL;
    virtual void deleteAnimationAfterTime(double time, ViewSpec view, int dimension, NATRON_ENUM::ValueChangedReasonEnum reason) OVERRIDE FINAL;

private:

    void deleteAnimationConditional(double time, ViewSpec view, int dimension, NATRON_ENUM::ValueChangedReasonEnum reason, bool before);

public:


    virtual bool getKeyFrameTime(ViewSpec view, int index, int dimension, double* time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getLastKeyFrameTime(ViewSpec view, int dimension, double* time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getFirstKeyFrameTime(ViewSpec view, int dimension, double* time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getKeyFramesCount(ViewSpec view, int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getNearestKeyFrameTime(ViewSpec view, int dimension, double time, double* nearestTime) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getKeyFrameIndex(ViewSpec view, int dimension, double time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual boost::shared_ptr<Curve> getCurve(ViewSpec view, int dimension, bool byPassMaster = false) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isAnimated( int dimension, ViewSpec view = ViewSpec::current() ) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasAnimation() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool checkInvalidExpressions() OVERRIDE FINAL;
    virtual bool isExpressionValid(int dimension, std::string* error) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setExpressionInvalid(int dimension, bool valid, const std::string& error) OVERRIDE FINAL;

protected:

public:

    virtual void setExpressionInternal(int dimension, const std::string& expression, bool hasRetVariable, bool clearResults, bool failIfInvalid) OVERRIDE FINAL;
    virtual void replaceNodeNameInExpression(int dimension,
                                             const std::string& oldName,
                                             const std::string& newName) OVERRIDE FINAL;
    virtual void clearExpression(int dimension, bool clearResults) OVERRIDE FINAL;
    virtual std::string validateExpression(const std::string& expression, int dimension, bool hasRetVariable,
                                           std::string* resultAsString) OVERRIDE FINAL WARN_UNUSED_RETURN;

protected:

    template <typename T>
    static T pyObjectToType(PyObject* o);

    virtual void refreshListenersAfterValueChange(ViewSpec view, NATRON_ENUM::ValueChangedReasonEnum reason, int dimension) OVERRIDE FINAL;

public:

    virtual bool isExpressionUsingRetVariable(int dimension = 0) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getExpressionDependencies(int dimension, std::list<std::pair<KnobIWPtr, int> >& dependencies) const OVERRIDE FINAL;
    virtual std::string getExpression(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual const std::vector<boost::shared_ptr<Curve>  > & getCurves() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAnimationEnabled(bool val) OVERRIDE FINAL;
    virtual bool isAnimationEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string  getLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    void setLabel(const std::string& label) OVERRIDE FINAL;
    void setLabel(const QString & label) { setLabel( label.toStdString() ); }

    virtual void setIconLabel(const std::string& iconFilePath, bool checked = false) OVERRIDE FINAL;
    virtual const std::string& getIconLabel(bool checked = false) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual KnobHolder* getHolder() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setHolder(KnobHolder* holder) OVERRIDE FINAL;
    virtual int getDimension() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAddNewLine(bool newLine) OVERRIDE FINAL;
    virtual void setAddSeparator(bool addSep) OVERRIDE FINAL;
    virtual bool isNewLineActivated() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isSeparatorActivated() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setSpacingBetweenItems(int spacing) OVERRIDE FINAL;
    virtual int getSpacingBetweenitems() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getInViewerContextLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextLabel(const QString& label) OVERRIDE FINAL;
    virtual void setInViewerContextCanHaveShortcut(bool haveShortcut) OVERRIDE FINAL;
    virtual bool getInViewerContextHasShortcut() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextItemSpacing(int spacing) OVERRIDE FINAL;
    virtual int  getInViewerContextItemSpacing() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextAddSeparator(bool addSeparator) OVERRIDE FINAL;
    virtual bool  getInViewerContextAddSeparator() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextNewLineActivated(bool activated) OVERRIDE FINAL;
    virtual bool  getInViewerContextNewLineActivated() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextSecret(bool secret) OVERRIDE FINAL;
    virtual bool  getInViewerContextSecret() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setEnabled(int dimension, bool b) OVERRIDE FINAL;
    virtual void setDefaultEnabled(int dimension, bool b) OVERRIDE FINAL;
    virtual bool isEnabled(int dimension) const OVERRIDE FINAL;
    virtual bool isDefaultEnabled(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAllDimensionsEnabled(bool b) OVERRIDE FINAL;
    virtual void setDefaultAllDimensionsEnabled(bool b) OVERRIDE FINAL;
    virtual void setSecret(bool b) OVERRIDE FINAL;
    virtual void setSecretByDefault(bool b) OVERRIDE FINAL;
    virtual bool getIsSecret() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getIsSecretRecursive() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getDefaultIsSecret() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setIsFrozen(bool frozen) OVERRIDE FINAL;
    virtual void setDirty(bool d) OVERRIDE FINAL;
    virtual void setName(const std::string & name, bool throwExceptions = false) OVERRIDE FINAL;
    virtual const std::string & getName() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual const std::string & getOriginalName() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setParentKnob(KnobIPtr knob) OVERRIDE FINAL;
    virtual void resetParent() OVERRIDE FINAL;
    virtual KnobIPtr getParentKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int determineHierarchySize() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setEvaluateOnChange(bool b) OVERRIDE FINAL;
    virtual bool getEvaluateOnChange() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getIsPersistent() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setIsPersistent(bool b) OVERRIDE FINAL;
    virtual void setCanUndo(bool val) OVERRIDE FINAL;
    virtual bool getCanUndo() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setIsMetadataSlave(bool slave) OVERRIDE FINAL;
    virtual bool getIsMetadataSlave() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setHintToolTip(const std::string & hint) OVERRIDE FINAL;
    void setHintToolTip(const QString & hint) { setHintToolTip( hint.toStdString() ); }

    //void setHintToolTip(const char* hint) { setHintToolTip(std::string(hint)); }
    virtual const std::string & getHintToolTip() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isHintInMarkdown() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setHintIsMarkdown(bool b) OVERRIDE FINAL;
    virtual void setCustomInteract(const boost::shared_ptr<OfxParamOverlayInteract> & interactDesc) OVERRIDE FINAL;
    virtual boost::shared_ptr<OfxParamOverlayInteract> getCustomInteract() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;
    virtual void redraw() OVERRIDE FINAL;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;
#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE FINAL;
#endif
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual RectD getViewportRect() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL;
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE FINAL;
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE FINAL;
    virtual int getWidgetFontHeight() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setOfxParamHandle(void* ofxParamHandle) OVERRIDE FINAL;
    virtual void* getOfxParamHandle() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isMastersPersistenceIgnored() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void copyAnimationToClipboard() const OVERRIDE FINAL;
    virtual double getCurrentTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ViewIdx getCurrentView() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setDimensionName(int dim, const std::string & name) OVERRIDE FINAL;
    virtual bool hasModifications() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasModifications(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasModificationsForSerialization() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void checkAnimationLevel(ViewSpec view, int dimension) OVERRIDE FINAL;
    virtual KnobIPtr createDuplicateOnHolder(KnobHolder* otherHolder,
                                            const boost::shared_ptr<KnobPage>& page,
                                            const boost::shared_ptr<KnobGroup>& group,
                                            int indexInParent,
                                            bool makeAlias,
                                            const std::string& newScriptName,
                                            const std::string& newLabel,
                                            const std::string& newToolTip,
                                            bool refreshParams,
                                            bool isUserKnob) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual KnobIPtr getAliasMaster() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool setKnobAsAliasOfThis(const KnobIPtr& master, bool doAlias) OVERRIDE FINAL;

private:


    virtual bool slaveToInternal(int dimension, const KnobIPtr &  other, int otherDimension, NATRON_ENUM::ValueChangedReasonEnum reason
                                 , bool ignoreMasterPersistence) OVERRIDE FINAL WARN_UNUSED_RETURN;

protected:

    virtual bool setHasModifications(int dimension, bool value, bool lock) OVERRIDE FINAL;

    virtual bool hasDefaultValueChanged(int dimension) const = 0;
    /**
     * @brief Protected so the implementation of unSlave can actually use this to reset the master pointer
     **/
    void resetMaster(int dimension);

    ///The return value must be Py_DECRREF
    /// The Python GIL must be held before calling this, so the the PyObject remains valid.
    bool executeExpression(double time, ViewIdx view, int dimension, PyObject** ret, std::string* error) const;

public:

    /// The return value must be Py_DECRREF
    /// The expression must put its result in the Python variable named "ret"
    /// The Python GIL must be held before calling this, so the the PyObject remains valid.
    static bool executeExpression(const std::string& expr, PyObject** ret, std::string* error);

    virtual std::pair<int, KnobIPtr> getMaster(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isSlave(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual NATRON_ENUM::AnimationLevelEnum getAnimationLevel(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isTypeCompatible(const KnobIPtr & other) const OVERRIDE WARN_UNUSED_RETURN = 0;

    /**
     * @brief Adds a new listener to this knob. This is just a pure notification about the fact that the given knob
     * is listening to the values/keyframes of "this". It could be call addSlave but it will also be use for expressions.
     **/
    virtual void addListener(bool isFromExpr, int fromExprDimension, int thisDimension, const KnobIPtr& knob) OVERRIDE FINAL;
    virtual void removeListener(KnobI* listener, int listenerDimension) OVERRIDE FINAL;
    virtual void getAllExpressionDependenciesRecursive(std::set<NodePtr>& nodes) const OVERRIDE FINAL;
    virtual void getListeners(KnobI::ListenerDimsMap& listeners) const OVERRIDE FINAL;
    virtual void clearExpressionsResults(int /*dimension*/) OVERRIDE {}

    void incrementExpressionRecursionLevel() const;

    void decrementExpressionRecursionLevel() const;

    int getExpressionRecursionLevel() const;

    class ExprRecursionLevel_RAII
    {
        const KnobHelper* _k;

public:

        ExprRecursionLevel_RAII(const KnobHelper* k)
            : _k(k)
        {
            k->incrementExpressionRecursionLevel();
        }

        ~ExprRecursionLevel_RAII()
        {
            _k->decrementExpressionRecursionLevel();
        }
    };

protected:

    virtual void copyValuesFromCurve(int /*dim*/) {}


    virtual void handleSignalSlotsForAliasLink(const KnobIPtr& /*alias*/,
                                               bool /*connect*/)
    {
    }

    /**
     * @brief Called when you must copy any extra data you maintain from the other knob.
     * The other knob is guaranteed to be of the same type.
     **/
    virtual void cloneExtraData(KnobI* /*other*/,
                                int dimension = -1,
                                int otherDimension = -1)
    {
        Q_UNUSED(dimension);
        Q_UNUSED(otherDimension);
    }

    virtual bool cloneExtraDataAndCheckIfChanged(KnobI* /*other*/,
                                                 int dimension = -1,
                                                 int otherDimension = -1)
    {
        Q_UNUSED(dimension);
        Q_UNUSED(otherDimension);

        return false;
    }

    virtual void cloneExtraData(KnobI* /*other*/,
                                double /*offset*/,
                                const RangeD* /*range*/,
                                int dimension = -1,
                                int otherDimension = -1)
    {
        Q_UNUSED(dimension);
        Q_UNUSED(otherDimension);
    }

    void cloneExpressions(KnobI* other, int dimension = -1, int otherDimension = -1);
    bool cloneExpressionsAndCheckIfChanged(KnobI* other, int dimension = -1, int otherDimension = -1);

    virtual void cloneExpressionsResults(KnobI* /*other*/,
                                         int /*dimension = -1*/,
                                         int /*otherDimension = -1*/) {}

    void cloneOneCurve(KnobI* other, int offset, const RangeD* range, int dimension, int otherDimension);
    bool cloneOneCurveAndCheckIfChanged(KnobI* other, bool updateGui, int dimension, int otherDimension);

    void cloneCurves(KnobI* other, int offset, const RangeD* range, int dimension = -1, int otherDimension = -1);
    bool cloneCurvesAndCheckIfChanged(KnobI* other, bool updateGui, int dimension = -1, int otherDimension = -1);


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

    void cloneGuiCurvesIfNeeded(std::map<int, NATRON_ENUM::ValueChangedReasonEnum>& modifiedDimensions);

    void cloneInternalCurvesIfNeeded(std::map<int, NATRON_ENUM::ValueChangedReasonEnum>& modifiedDimensions);

    void setInternalCurveHasChanged(ViewSpec view, int dimension, bool changed);

    void guiCurveCloneInternalCurve(NATRON_ENUM::CurveChangeReason curveChangeReason, ViewSpec view, int dimension, NATRON_ENUM::ValueChangedReasonEnum reason);

    virtual boost::shared_ptr<Curve> getGuiCurve(ViewSpec view, int dimension, bool byPassMaster = false) const OVERRIDE FINAL;

    void setGuiCurveHasChanged(ViewSpec view, int dimension, bool changed);
    bool hasGuiCurveChanged(ViewSpec view, int dimension) const;
    void clearExpressionsResultsIfNeeded(std::map<int, NATRON_ENUM::ValueChangedReasonEnum>& modifiedDimensions);


    boost::shared_ptr<KnobSignalSlotHandler> _signalSlotHandler;

private:

    void expressionChanged(int dimension);

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


    /*
       For each dimension, the results of the expressions at a given pair <frame, time> is stored so
       that we're able to get the same value again for the same render.
       Of course, this saved in the project to retrieve the same values between 2 runs of the project.
     */
    typedef std::map<double, T> FrameValueMap;
    typedef std::vector<FrameValueMap> ExprResults;


    /**
     * @brief Make a knob for the given KnobHolder with the given label (the label displayed on
     * its interface) and with the given dimension. The dimension parameter is used for example for the
     * KnobColor which has 4 doubles (r,g,b,a), hence 4 dimensions.
     **/
    Knob(KnobHolder*  holder,
         const std::string & label,
         int dimension = 1,
         bool declaredByPlugin = true);

    virtual ~Knob();


    virtual void computeHasModifications() OVERRIDE FINAL;

protected:

    virtual bool computeValuesHaveModifications(int dimension,
                                                const T& value,
                                                const T& defaultValue) const;
    virtual bool hasModificationsVirtual(int /*dimension*/) const { return false; }

public:

    /**
     * @brief Get the current value of the knob for the given dimension.
     * If it is animated, it will return the value at the current time.
     **/
    T getValue(int dimension = 0, ViewSpec view = ViewSpec::current(), bool clampToMinMax = true) WARN_UNUSED_RETURN;


    /**
     * @brief Returns the value of the knob at the given time and for the given dimension.
     * If it is not animated, it will call getValue for that dimension and return the result.
     *
     * This function is overloaded by the KnobString which can have its custom interpolation
     * but this should be the only knob which should ever need to overload it.
     **/
    T getValueAtTime(double time, int dimension = 0, ViewSpec view = ViewSpec::current(), bool clampToMinMax = true, bool byPassMaster = false)  WARN_UNUSED_RETURN;

    virtual double getRawCurveValueAt(double time, ViewSpec view,  int dimension)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double getValueAtWithExpression(double time, ViewSpec view, int dimension)  OVERRIDE FINAL WARN_UNUSED_RETURN;

private:


    virtual void unSlaveInternal(int dimension,
                                 NATRON_ENUM::ValueChangedReasonEnum reason,
                                 bool copyState) OVERRIDE FINAL;

public:


    /**
     * @brief Set the value of the knob at the given time and for the given dimension with the given reason.
     * @param newKey[out] The keyframe that was added if the return value is true.
     * @returns True if a keyframe was successfully added, false otherwise.
     **/
    ValueChangedReturnCodeEnum setValueAtTime(double time,
                                              const T & v,
                                              ViewSpec view,
                                              int dimension,
                                              NATRON_ENUM::ValueChangedReasonEnum reason,
                                              KeyFrame* newKey,
                                              bool hasChanged = false); //!< set to true if any previous dimension of the same knob have changed

    virtual bool setKeyFrame(const KeyFrame& key, ViewSpec view, int dimension, NATRON_ENUM::ValueChangedReasonEnum reason) OVERRIDE FINAL;

    /**
     * @brief Set the value of the knob in the given dimension with the given reason.
     * @param newKey If not NULL and the animation level of the knob is eAnimationLevelInterpolatedValue
     * then a new keyframe will be set at the current time.
     **/
    ValueChangedReturnCodeEnum setValue(const T & v,
                                        ViewSpec view,
                                        int dimension,
                                        NATRON_ENUM::ValueChangedReasonEnum reason,
                                        KeyFrame* newKey,
                                        bool hasChanged = false); //!< set to true if any previous dimension of the same knob have changed


    /**
     * @brief Calls setValue with a reason of eValueChangedReasonNatronInternalEdited.
     * @param turnOffAutoKeying If set to true, the underlying call to setValue will
     * not set a new keyframe.
     **/
    ValueChangedReturnCodeEnum setValue(const T & value,
                                        ViewSpec view = ViewSpec::all(),
                                        int dimension = 0,
                                        bool turnOffAutoKeying = false);

    void setValues(const T& value0,
                   const T& value1,
                   ViewSpec view,
                   NATRON_ENUM::ValueChangedReasonEnum reason);

    void setValues(const T& value0,
                   const T& value1,
                   ViewSpec view,
                   NATRON_ENUM::ValueChangedReasonEnum reason,
                   int dimensionOffset);

    void setValues(const T& value0,
                   const T& value1,
                   const T& value2,
                   ViewSpec view,
                   NATRON_ENUM::ValueChangedReasonEnum reason);

    void setValues(const T& value0,
                   const T& value1,
                   const T& value2,
                   ViewSpec view,
                   NATRON_ENUM::ValueChangedReasonEnum reason,
                   int dimensionOffset);


    void setValues(const T& value0,
                   const T& value1,
                   const T& value2,
                   const T& value3,
                   ViewSpec view,
                   NATRON_ENUM::ValueChangedReasonEnum reason);

    /**
     * @brief Calls setValue
     * @param reason Can either be eValueChangedReasonUserEdited or eValueChangedReasonNatronGuiEdited
     * @param newKey[out] The keyframe that was added if the return value is true.
     * @returns A status according to the operation that was made to the keyframe.
     * @see ValueChangedReturnCodeEnum
     **/
    ValueChangedReturnCodeEnum onValueChanged(const T & v,
                                              ViewSpec view,
                                              int dimension,
                                              NATRON_ENUM::ValueChangedReasonEnum reason,
                                              KeyFrame* newKey);

    /**
     * @brief Calls setValue with a reason of eValueChangedReasonPluginEdited.
     **/
    ValueChangedReturnCodeEnum setValueFromPlugin(const T & value,
                                                  ViewSpec view,
                                                  int dimension);

    /**
     * @brief This is called by the plugin when a set value call would happen during  an interact action.
     **/
    void requestSetValueOnUndoStack(const T & value,
                                    ViewSpec view,
                                    int dimension);

    /**
     * @brief Calls setValueAtTime with a reason of eValueChangedReasonNatronInternalEdited.
     **/
    void setValueAtTime(double time,
                        const T & v,
                        ViewSpec view,
                        int dimension);

    /**
     * @brief Calls setValueAtTime with a reason of eValueChangedReasonPluginEdited.
     **/
    void setValueAtTimeFromPlugin(double time,
                                  const T & v,
                                  ViewSpec view,
                                  int dimension);

    void setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         ViewSpec view,
                         NATRON_ENUM::ValueChangedReasonEnum reason);

    void setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         ViewSpec view,
                         NATRON_ENUM::ValueChangedReasonEnum reason,
                         int dimensionOffset);

    void setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         const T& value2,
                         ViewSpec view,
                         NATRON_ENUM::ValueChangedReasonEnum reason);

    void setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         const T& value2,
                         ViewSpec view,
                         NATRON_ENUM::ValueChangedReasonEnum reason,
                         int dimensionOffset);

    void setValuesAtTime(double time,
                         const T& value0,
                         const T& value1,
                         const T& value2,
                         const T& value3,
                         ViewSpec view,
                         NATRON_ENUM::ValueChangedReasonEnum reason);

    /**
     * @brief Unlike getValueAtTime this function doesn't interpolate the values.
     * Instead the true value of the keyframe at the given index will be returned.
     * @returns True if a keyframe was found at the given index and for the given dimension,
     * false otherwise.
     **/
    T getKeyFrameValueByIndex(ViewSpec view,
                              int dimension,
                              int index,
                              bool* ok) const WARN_UNUSED_RETURN;

    /**
     * @brief Same as getValueForEachDimension() but MT thread-safe.
     **/
    std::vector<T> getValueForEachDimension_mt_safe_vector() const WARN_UNUSED_RETURN;
    std::list<T> getValueForEachDimension_mt_safe() const WARN_UNUSED_RETURN;

    T getRawValue(int dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief Get Default values
     **/
    std::vector<T> getDefaultValues_mt_safe() const WARN_UNUSED_RETURN;
    T getDefaultValue(int dimension) const WARN_UNUSED_RETURN;

    bool isDefaultValueSet(int dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief Set a default value and set the knob value to it for the particular dimension.
     **/
    void setDefaultValue(const T & v, int dimension = 0);
    void setDefaultValueWithoutApplying(const T& v, int dimension = 0);
    void setDefaultValuesWithoutApplying(const T& v1, const T& v2);
    void setDefaultValuesWithoutApplying(const T& v1, const T& v2, const T& v3);
    void setDefaultValuesWithoutApplying(const T& v1, const T& v2, const T& v3, const T& v4);


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
    virtual bool isTypePOD() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isTypeCompatible(const KnobIPtr & other) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    ///Cannot be overloaded by KnobHelper as it requires setValueAtTime
    virtual bool onKeyFrameSet(double time, ViewSpec view, int dimension) OVERRIDE FINAL;
    virtual bool onKeyFrameSet(double time, ViewSpec view, const KeyFrame& key, int dimension) OVERRIDE FINAL;

    ///Cannot be overloaded by KnobHelper as it requires setValue
    virtual void onTimeChanged(bool isPlayback, double time) OVERRIDE FINAL;

    ///Cannot be overloaded by KnobHelper as it requires the value member
    virtual double getDerivativeAtTime(double time, ViewSpec view, int dimension = 0)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double getIntegrateFromTimeToTime(double time1, double time2, ViewSpec view, int dimension = 0)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    double getIntegrateFromTimeToTimeSimpson(double time1, double time2, ViewSpec view, int dimension = 0);

    ///Cannot be overloaded by KnobHelper as it requires setValue
    virtual void resetToDefaultValue(int dimension) OVERRIDE FINAL;
    virtual void resetToDefaultValueWithoutSecretNessAndEnabledNess(int dimension) OVERRIDE FINAL;
    virtual void clone(KnobI* other, int dimension = -1, int otherDimension = -1)  OVERRIDE FINAL;
    virtual void clone(KnobI* other, double offset, const RangeD* range, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL;
    virtual void cloneAndUpdateGui(KnobI* other, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL;
    virtual void cloneDefaultValues(KnobI* other) OVERRIDE FINAL;
    virtual bool cloneAndCheckIfChanged(KnobI* other, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool dequeueValuesSet(bool disableEvaluation) OVERRIDE FINAL;

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

    void getExpressionResults(int dim,
                              FrameValueMap& map)
    {
        QMutexLocker k(&_valueMutex);

        map = _exprRes[dim];
    }

    T getValueFromMasterAt(double time, ViewSpec view, int dimension, KnobI* master);
    T getValueFromMaster(ViewSpec view, int dimension, KnobI* master, bool clamp);

    bool getValueFromCurve(double time, ViewSpec view, int dimension, bool useGuiCurve, bool byPassMaster, bool clamp, T* ret);

protected:

    virtual void resetExtraToDefaultValue(int /*dimension*/) {}

private:

    virtual void copyValuesFromCurve(int dim) OVERRIDE FINAL;

    virtual bool hasDefaultValueChanged(int dimension) const OVERRIDE FINAL;

    void initMinMax();

    T clampToMinMax(const T& value, int dimension) const;

    void signalMinMaxChanged(const T& mini, const T& maxi, int dimension);
    void signalDisplayMinMaxChanged(const T& mini, const T& maxi, int dimension);

    template <typename OTHERTYPE>
    void copyValueForType(Knob<OTHERTYPE>* other, int dimension, int otherDimension);

    template <typename OTHERTYPE>
    bool copyValueForTypeAndCheckIfChanged(Knob<OTHERTYPE>* other, int dimension, int otherDimension);

    void cloneValues(KnobI* other, int dimension, int otherDimension);

    bool cloneValuesAndCheckIfChanged(KnobI* other, int dimension, int otherDimension);

    virtual void cloneExpressionsResults(KnobI* other, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL;

    void valueToVariant(const T & v, Variant* vari);

    int get_SetValueRecursionLevel() const
    {
        QMutexLocker l(&_setValueRecursionLevelMutex);

        return _setValueRecursionLevel;
    }

    void makeKeyFrame(Curve* curve, double time, ViewSpec view, const T& v, KeyFrame* key);

    void queueSetValue(const T& v, ViewSpec view, int dimension);

    virtual void clearExpressionsResults(int dimension) OVERRIDE FINAL
    {
        QMutexLocker k(&_valueMutex);

        _exprRes[dimension].clear();
    }


public:
    /// This static publicly-available function is useful to evaluate simple python expressions that evaluate to a double, int or string value.
    /// The expression must put its result in the Python variable named "ret"
    static bool evaluateExpression(const std::string& expr, T* ret, std::string* error);

private:

    bool evaluateExpression(double time, ViewIdx view, int dimension, T* ret, std::string* error);

    /*
     * @brief Same as evaluateExpression but expects it to return a PoD
     */
    bool evaluateExpression_pod(double time, ViewIdx view, int dimension, double* value, std::string* error);


    bool getValueFromExpression(double time, ViewIdx view, int dimension, bool clamp, T* ret);

    bool getValueFromExpression_pod(double time, ViewIdx view, int dimension, bool clamp, double* ret);

    //////////////////////////////////////////////////////////////////////
    /////////////////////////////////// End implementation of KnobI
    //////////////////////////////////////////////////////////////////////
    struct QueuedSetValuePrivate;
    class QueuedSetValue
    {
public:
        QueuedSetValue(ViewSpec view,
                       int dimension,
                       const T& value,
                       const KeyFrame& key,
                       bool useKey,
                       NATRON_ENUM::ValueChangedReasonEnum reason,
                       bool valueChangesBlocked);

        virtual bool isSetValueAtTime() const { return false; }

        virtual ~QueuedSetValue();

        int dimension() const;

        ViewSpec view() const;

        const T& value() const;
        const KeyFrame& key() const;

        bool useKey() const;

        NATRON_ENUM::ValueChangedReasonEnum reason() const;

        bool valueChangesBlocked() const;

private:
        boost::scoped_ptr<QueuedSetValuePrivate> _imp;
    };

    typedef boost::shared_ptr<QueuedSetValue> QueuedSetValuePtr;

    class QueuedSetValueAtTime
        : public QueuedSetValue
    {
public:
        QueuedSetValueAtTime(double time,
                             ViewSpec view,
                             int dimension,
                             const T& value,
                             const KeyFrame& key,
                             NATRON_ENUM::ValueChangedReasonEnum reason,
                             bool valueChangesBlocked)
            : QueuedSetValue(view, dimension, value, key, true, reason, valueChangesBlocked)
            , _time(time)
        {
        }

        virtual bool isSetValueAtTime() const { return true; }

        virtual ~QueuedSetValueAtTime() {}

        double time() const { return _time; };

private:
        double _time;
    };

    typedef boost::shared_ptr<QueuedSetValueAtTime> QueuedSetValueAtTimePtr;

    ///Here is all the stuff we couldn't get rid of the template parameter
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    mutable QRecursiveMutex _valueMutex;
#else
    mutable QMutex _valueMutex; //< protects _values & _guiValues & _defaultValues & ExprResults
#endif
    std::vector<T> _values, _guiValues;

    struct DefaultValue
    {
        T initialValue,value;
        bool defaultValueSet;
    };
    std::vector<DefaultValue> _defaultValues;
    mutable ExprResults _exprRes;

    //Only for double and int
    mutable QReadWriteLock _minMaxMutex;
    std::vector<T>  _minimums, _maximums, _displayMins, _displayMaxs;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    mutable QRecursiveMutex _setValuesQueueMutex;
#else
    mutable QMutex _setValuesQueueMutex;
#endif
    std::list<boost::shared_ptr<QueuedSetValue> > _setValuesQueue;

    ///this flag is to avoid recursive setValue calls
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    mutable QRecursiveMutex _setValueRecursionLevelMutex;
#else
    mutable QMutex _setValueRecursionLevelMutex;
#endif
    int _setValueRecursionLevel;
};


typedef Knob<bool> KnobBoolBase;
typedef Knob<double> KnobDoubleBase;
typedef Knob<int> KnobIntBase;
typedef Knob<std::string> KnobStringBase;

typedef boost::shared_ptr<KnobBoolBase> KnobBoolBasePtr;
typedef boost::shared_ptr<KnobDoubleBase> KnobDoubleBasePtr;
typedef boost::shared_ptr<KnobIntBase> KnobIntBasePtr;
typedef boost::shared_ptr<KnobStringBase> KnobStringBasePtr;

typedef boost::weak_ptr<KnobBoolBase> KnobBoolBaseWPtr;
typedef boost::weak_ptr<KnobDoubleBase> KnobDoubleBaseWPtr;
typedef boost::weak_ptr<KnobIntBase> KnobIntBaseWPtr;
typedef boost::weak_ptr<KnobStringBase> KnobStringBaseWPtr;

class AnimatingKnobStringHelper
    : public KnobStringBase
{
public:

    AnimatingKnobStringHelper(KnobHolder* holder,
                              const std::string &label,
                              int dimension,
                              bool declaredByPlugin = true);

    virtual ~AnimatingKnobStringHelper();

    void stringToKeyFrameValue(double time, ViewSpec view, const std::string & v, double* returnValue);

    //for integration of openfx custom params
    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle inArgsRaw,
                                                           OfxPropertySetHandle outArgsRaw);


    const StringAnimationManager & getAnimation() const
    {
        return *_animation;
    }

    void loadAnimation(const std::map<int, std::string> & keyframes);

    void saveAnimation(std::map<int, std::string>* keyframes) const;

    void setCustomInterpolation(customParamInterpolationV1Entry_t func, void* ofxParamHandle);

    void stringFromInterpolatedValue(double interpolated, ViewSpec view, std::string* returnValue) const;

    std::string getStringAtTime(double time, ViewSpec view, int dimension);

protected:

    virtual void cloneExtraData(KnobI* other, int dimension = -1, int otherDimension = -1) OVERRIDE;
    virtual bool cloneExtraDataAndCheckIfChanged(KnobI* other, int dimension = -1, int otherDimension = -1) OVERRIDE;
    virtual void cloneExtraData(KnobI* other, double offset, const RangeD* range, int dimension = -1, int otherDimension = -1) OVERRIDE;
    virtual void keyframeRemoved_virtual(int dimension, double time) OVERRIDE;
    virtual void animationRemoved_virtual(int dimension) OVERRIDE;

private:
    boost::scoped_ptr<StringAnimationManager> _animation;
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
    : public QObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    friend class RecursionLevelRAII;
    struct KnobHolderPrivate;
    boost::scoped_ptr<KnobHolderPrivate> _imp;

public:

    enum MultipleParamsEditEnum
    {
        eMultipleParamsEditOff = 0, //< The knob should not use multiple edits command
        eMultipleParamsEditOnCreateNewCommand, //< The knob should use multiple edits command and create a new one that will not merge with others
        eMultipleParamsEditOn //< The knob should use multiple edits command and merge it with priors command (if any)
    };

    /**
     *@brief A holder is a class managing a bunch of knobs and interacting with an appInstance.
     * When appInstance is NULL the holder will be considered "application global" in which case
     * the knob holder will interact directly with the AppManager singleton.
     **/
    KnobHolder(const AppInstancePtr& appInstance);

    KnobHolder(const KnobHolder& other);

    virtual ~KnobHolder();

    void setPanelPointer(DockablePanelI* gui);

    void discardPanelPointer();

    /**
     * @brief Wipe all user-knobs GUI and re-create it to synchronize it to the current internal state.
     * This may only be called on the main-thread. This is an expensive operation.
     **/
    void recreateUserKnobs(bool keepCurPageIndex);

    /**
     * @brief Wipe all knobs GUI and re-create it to synchronize it to the current internal state.
     * This may only be called on the main-thread. This is an expensive operation.
     **/
    void recreateKnobs(bool keepCurPageIndex);

    /**
     * @brief Dynamically removes a knob. Can only be called on the main-thread.
     * @param knob The knob to remove
     * @param alsoDeleteGui If true, then the knob gui will be removed before returning from this function
     * In case of removing multiple knobs, it may be more efficient to set alsoDeleteGui to false and to call
     * recreateKnobs() when finished
     **/
    void deleteKnob(KnobI* knob, bool alsoDeleteGui);

    //To re-arrange user knobs only, does nothing if knob->isUserKnob() returns false
    bool moveKnobOneStepUp(KnobI* knob);
    bool moveKnobOneStepDown(KnobI* knob);


    template<typename K>
    boost::shared_ptr<K> createKnob(const std::string &label, int dimension = 1) const WARN_UNUSED_RETURN;
    AppInstancePtr getApp() const WARN_UNUSED_RETURN;
    KnobIPtr getKnobByName(const std::string & name) const WARN_UNUSED_RETURN;
    KnobIPtr getOtherKnobByName(const std::string & name, const KnobI* caller) const WARN_UNUSED_RETURN;

    template <typename TYPE>
    boost::shared_ptr<TYPE> getKnobByNameAndType(const std::string & name) const
    {
        const KnobsVec& knobs = getKnobs();

        for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            if ( (*it)->getName() == name ) {
                boost::shared_ptr<TYPE> isType = boost::dynamic_pointer_cast<TYPE>(*it);
                if (isType) {
                    return isType;
                }
                break;
            }
        }

        return boost::shared_ptr<TYPE>();
    }

    const std::vector<KnobIPtr> & getKnobs() const WARN_UNUSED_RETURN;
    std::vector<KnobIPtr>  getKnobs_mt_safe() const WARN_UNUSED_RETURN;

    void refreshAfterTimeChange(bool isPlayback, double time);

    /**
     * @brief Same as refreshAfterTimeChange but refreshes only the knobs
     * whose function evaluateValueChangeOnTimeChange() return true so that
     * after a playback their state is correct
     **/
    void refreshAfterTimeChangeOnlyKnobsWithTimeEvaluation(double time);

    void setIsInitializingKnobs(bool b);
    bool isInitializingKnobs() const;

    void addKnobToViewerUI(const KnobIPtr& knob);
    bool isInViewerUIKnob(const KnobIPtr& knob) const;
    KnobsVec getViewerUIKnobs() const;

protected:

    virtual void refreshExtraStateAfterTimeChanged(bool /*isPlayback*/,
                                                   double /*time*/) {}

public:

    void refreshInstanceSpecificKnobsOnly(bool isPlayback, double time);
    KnobHolder::MultipleParamsEditEnum getMultipleParamsEditLevel() const;

    void setMultipleParamsEditLevel(KnobHolder::MultipleParamsEditEnum level);

    virtual bool isProject() const
    {
        return false;
    }

    /**
     * @brief If false, all knobs within this container cannot animate
     **/
    virtual bool canKnobsAnimate() const
    {
        return true;
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

    bool areKnobsFrozen() const;

    /**
     * @brief Makes all output nodes connected downstream to this node abort any rendering.
     * Should be called prior to changing the state of the user interface that could impact the
     * final image.
     **/
    virtual void abortAnyEvaluation(bool keepOldestRender = true) { Q_UNUSED(keepOldestRender); }

    /**
     * @brief Dequeues all values set in the queues for all knobs
     **/
    bool dequeueValuesSet();

    bool isDequeueingValuesSet() const;

    /**
     * @brief Returns true if at least a parameter is animated
     **/
    bool getHasAnimation() const;

    /**
     * @brief Set the hasAnimation flag
     **/
    void setHasAnimation(bool hasAnimation);

    /**
     * @brief Auto-compute the animation flag by checking the animation state of all curves of all parameters held
     **/
    void updateHasAnimation();

    //////////////////////////////////////////////////////////////////////////////////////////
    boost::shared_ptr<KnobPage> getOrCreateUserPageKnob();
    boost::shared_ptr<KnobPage> getUserPageKnob() const;

    /**
     * @brief These functions below are dynamic in a sense that they can be called at any time (on the main-thread)
     * to create knobs on the fly. Their gui will be properly created. In order to notify the GUI that new parameters were
     * created, you must call refreshKnobs() that will re-scan for new parameters
     **/
    boost::shared_ptr<KnobInt> createIntKnob(const std::string& name, const std::string& label, int dimension, bool userKnob = true);
    boost::shared_ptr<KnobDouble> createDoubleKnob(const std::string& name, const std::string& label, int dimension, bool userKnob = true);
    boost::shared_ptr<KnobColor> createColorKnob(const std::string& name, const std::string& label, int dimension, bool userKnob = true);
    boost::shared_ptr<KnobBool> createBoolKnob(const std::string& name, const std::string& label, bool userKnob = true);
    boost::shared_ptr<KnobChoice> createChoiceKnob(const std::string& name, const std::string& label, bool userKnob = true);
    boost::shared_ptr<KnobButton> createButtonKnob(const std::string& name, const std::string& label, bool userKnob = true);
    boost::shared_ptr<KnobSeparator> createSeparatorKnob(const std::string& name, const std::string& label, bool userKnob = true);

    //Type corresponds to the Type enum defined in StringParamBase in Parameter.h
    boost::shared_ptr<KnobString> createStringKnob(const std::string& name, const std::string& label, bool userKnob = true);
    boost::shared_ptr<KnobFile> createFileKnob(const std::string& name, const std::string& label, bool userKnob = true);
    boost::shared_ptr<KnobOutputFile> createOuptutFileKnob(const std::string& name, const std::string& label, bool userKnob = true);
    boost::shared_ptr<KnobPath> createPathKnob(const std::string& name, const std::string& label, bool userKnob = true);
    boost::shared_ptr<KnobPage> createPageKnob(const std::string& name, const std::string& label, bool userKnob = true);
    boost::shared_ptr<KnobGroup> createGroupKnob(const std::string& name, const std::string& label, bool userKnob = true);
    boost::shared_ptr<KnobParametric> createParametricKnob(const std::string& name, const std::string& label, int nbCurves, bool userKnob = true);
    /**
     * @brief Returns whether the onKnobValueChanged can be called by a separate thread
     **/
    virtual bool canHandleEvaluateOnChangeInOtherThread() const { return false; }

    virtual bool isDoingInteractAction() const { return false; }

    bool isEvaluationBlocked() const;

    void appendValueChange(const KnobIPtr& knob,
                           int dimension,
                           bool refreshGui,
                           double time,
                           ViewSpec view,
                           NATRON_ENUM::ValueChangedReasonEnum originalReason,
                           NATRON_ENUM::ValueChangedReasonEnum reason);

    bool isSetValueCurrentlyPossible() const;

    void getAllExpressionDependenciesRecursive(std::set<NodePtr>& nodes) const;

    
protected:

    //////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Can be overridden to prevent values to be set directly.
     * Instead the setValue/setValueAtTime actions are queued up
     * They will be dequeued when dequeueValuesSet will be called.
     **/
    virtual bool canSetValue() const { return true; }


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
     *
     * We also allow recursion in some other actions such as getRegionOfDefinition or isIdentity
     **/
    virtual void assertActionIsNotRecursive() const {}

    /**
     * @brief Should be called in the beginning of an action.
     * Right after assertActionIsNotRecursive() for non recursive actions.
     **/
    virtual void incrementRecursionLevel() {}

    /**
     * @brief Should be called at the end of an action.
     **/
    virtual void decrementRecursionLevel() {}

public:

    virtual int getRecursionLevel() const { return 0; }


    /**
     * @brief Use this to bracket setValue() calls, this will actually trigger the evaluate() and instanceChanged()
     * if needed when endChanges() is called
     **/
    void beginChanges();

    // Returns true if at least 1 knob changed handler was called
    bool endChanges(bool discardEverything = false);


    /**
     * @brief The virtual portion of notifyProjectBeginValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to prepare yourself to a lot of value changes.
     **/
    void beginKnobsValuesChanged_public(NATRON_ENUM::ValueChangedReasonEnum reason);

    /**
     * @brief The virtual portion of notifyProjectEndKnobsValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to finish a serie of value changes, thus limiting the amount of changes to do.
     **/
    void endKnobsValuesChanged_public(NATRON_ENUM::ValueChangedReasonEnum reason);


    /**
     * @brief The virtual portion of notifyProjectEvaluationRequested(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to do things when a value is changed. Bear in mind that you can compress
     * the change by using the begin/end[ValueChanges] to optimize the changes.
     **/
    virtual bool onKnobValueChanged_public(KnobI* k, NATRON_ENUM::ValueChangedReasonEnum reason, double time, ViewSpec view,
                                           bool originatedFromMainThread);


    /**
     * @brief To be called after each function that modifies actionsRecursionLevel that is not
     * onKnobChanged or begin/endKnobValueChange.
     * If actionsRecursionLevel drops to 1 and there was some evaluate requested, it
     * will call evaluate_public
     **/
    void checkIfRenderNeeded();

    /**
     * Call this if getRecursionLevel > 1 to compress overlay redraws
     **/
    void incrementRedrawNeededCounter();

    /**
     * @brief Returns true if the overlay should be redrawn. This should be called once the recursion level reaches 0
     * This will reset the overlay redraw needed counter to 0.
     **/
    bool checkIfOverlayRedrawNeeded();


    /*Add a knob to the vector. This is called by the
       Knob class. Don't call this*/
    void addKnob(const KnobIPtr& k);

    void insertKnob(int idx, const KnobIPtr& k);
    void removeKnobFromList(const KnobI* knob);


    void initializeKnobsPublic();

    bool isSlave() const;

    ///Slave all the knobs of this holder to the other holder.
    void slaveAllKnobs(KnobHolder* other, bool restore);

    void unslaveAllKnobs();

    /**
     * @brief Returns the local current time of the timeline
     **/
    virtual double getCurrentTime() const;

    /**
     * @brief Returns the local current view being rendered or 0
     **/
    virtual ViewIdx getCurrentView() const
    {
        return ViewIdx(0);
    }

    int getPageIndex(const KnobPage* page) const;


    //Calls onSignificantEvaluateAboutToBeCalled + evaluate
    void incrHashAndEvaluate(bool isSignificant, bool refreshMetadata);


    /**
     * @brief Return a list of all the internal pages which were created by the user
     **/
    void getUserPages(std::list<KnobPage*>& userPages) const;
    
protected:


    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    virtual void evaluate(bool /*isSignificant*/,
                          bool /*refreshMetadata*/) {}

    /**
     * @brief The virtual portion of notifyProjectBeginValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to prepare yourself to a lot of value changes.
     **/
    virtual void beginKnobsValuesChanged(NATRON_ENUM::ValueChangedReasonEnum reason)
    {
        Q_UNUSED(reason);
    }

    /**
     * @brief The virtual portion of notifyProjectEndKnobsValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to finish a serie of value changes, thus limiting the amount of changes to do.
     **/
    virtual void endKnobsValuesChanged(NATRON_ENUM::ValueChangedReasonEnum reason)
    {
        Q_UNUSED(reason);
    }

    /**
     * @brief The virtual portion of notifyProjectEvaluationRequested(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to do things when a value is changed. Bear in mind that you can compress
     * the change by using the begin/end[ValueChanges] to optimize the changes.
     **/
    virtual bool onKnobValueChanged(KnobI* /*k*/,
                                    NATRON_ENUM::ValueChangedReasonEnum /*reason*/,
                                    double /*time*/,
                                    ViewSpec /*view*/,
                                    bool /*originatedFromMainThread*/)
    {
        return false;
    }

    virtual void onSignificantEvaluateAboutToBeCalled(KnobI* /*knob*/) {}

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
    virtual void onKnobSlaved(const KnobIPtr& /*slave*/,
                              const KnobIPtr& /*master*/,
                              int /*dimension*/,
                              bool /*isSlave*/)
    {
    }

public Q_SLOTS:

    void onDoEndChangesOnMainThreadTriggered();

    void onDoEvaluateOnMainThread(bool significant, bool refreshMetadata);

    void onDoValueChangeOnMainThread(KnobI* knob, int reason, double time, ViewSpec view, bool originatedFromMT);

Q_SIGNALS:

    void doEndChangesOnMainThread();

    void doEvaluateOnMainThread(bool significant, bool refreshMetadata);

    void doValueChangeOnMainThread(KnobI* knob, int reason, double time, ViewSpec view, bool originatedFromMT);

private:


    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() = 0;
};


/**
 * @brief A small class to help managing the recursion level
 * that can also that an action is not recursive.
 **/
class RecursionLevelRAII
{
    KnobHolder* effect;

public:

    RecursionLevelRAII(KnobHolder* effect,
                       bool assertNotRecursive)
        : effect(effect)
    {
        if (assertNotRecursive) {
            effect->assertActionIsNotRecursive();
        }
        effect->incrementRecursionLevel();
    }

    ~RecursionLevelRAII()
    {
        effect->decrementRecursionLevel();
    }
};

/**
 * @macro This special macro creates an object that will increment the recursion level in the constructor and decrement it in the
 * destructor.
 * @param assertNonRecursive If true then it will check that the recursion level is 0 and otherwise print a warning indicating that
 * this action was called recursively.
 **/
#define MANAGE_RECURSION(assertNonRecursive) RecursionLevelRAII actionRecursionManager(this, assertNonRecursive)

/**
 * @brief Should be called in the beginning of any action that cannot be called recursively.
 **/
#define NON_RECURSIVE_ACTION() MANAGE_RECURSION(true)

/**
 * @brief Should be called in the beginning of any action that can be called recursively
 **/
#define RECURSIVE_ACTION() MANAGE_RECURSION(false)


class InitializeKnobsFlag_RAII
{
    KnobHolder* _h;

public:

    InitializeKnobsFlag_RAII(KnobHolder* h)
        : _h(h)
    {
        h->setIsInitializingKnobs(true);
    }

    ~InitializeKnobsFlag_RAII()
    {
        _h->setIsInitializingKnobs(false);
    }
};

////Common interface for EffectInstance and backdrops
class NamedKnobHolder
    : public KnobHolder
{
public:

    NamedKnobHolder(const AppInstancePtr& instance)
        : KnobHolder(instance)
    {
    }

    NamedKnobHolder(const NamedKnobHolder& other)
    : KnobHolder(other)
    {
    }


    virtual ~NamedKnobHolder()
    {
    }

    virtual std::string getScriptName_mt_safe() const = 0;

    virtual std::string getFullyQualifiedName() const = 0;
};


template<typename K>
boost::shared_ptr<K> KnobHolder::createKnob(const std::string &label,
                                            int dimension) const
{
    return AppManager::createKnob<K>(this, label, dimension);
}

NATRON_NAMESPACE_EXIT

#endif // Engine_Knob_h
