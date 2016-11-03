/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/enable_shared_from_this.hpp>
#endif

#include <QtCore/QReadWriteLock>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>

#include "Engine/AnimatingObjectI.h"
#include "Engine/DimensionIdx.h"
#include "Engine/Variant.h"
#include "Engine/AppManager.h"
#include "Engine/KnobGuiI.h"
#include "Engine/HashableObject.h"
#include "Engine/OverlaySupport.h"
#include "Serialization/SerializationBase.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


#define NATRON_USER_MANAGED_KNOBS_PAGE_LABEL "User"
#define NATRON_USER_MANAGED_KNOBS_PAGE "userNatron"

NATRON_NAMESPACE_ENTER;

/**
 * @brief A class that reports changes happening to a knob
 **/
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

    void s_animationLevelChanged(ViewSetSpec view, DimSpec dim)
    {
        Q_EMIT animationLevelChanged(view, dim);
    }

    void s_mustRefreshKnobGui(ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason)
    {
        Q_EMIT mustRefreshKnobGui(view, dimension, reason);
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

    void s_curveAnimationChanged(const std::list<double>& keysAdded,
                                 const std::list<double>& keysRemoved,
                                 ViewIdx view,
                                 DimIdx dimension)
    {
        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, view,  dimension);
    }

    void s_knobSlaved(DimIdx dim,
                      ViewIdx view,
                      bool slaved)
    {
        Q_EMIT knobSlaved(dim, view, slaved);
    }

    void s_appendParamEditChange(ValueChangedReasonEnum reason,
                                 ValueChangedReturnCodeEnum setValueRetCode,
                                 Variant v,
                                 ViewSetSpec view,
                                 DimSpec dim,
                                 double time,
                                 bool setKeyFrame)
    {
        Q_EMIT appendParamEditChange(reason, setValueRetCode, v, view,  dim, time, setKeyFrame);
    }

    void s_setDirty(bool b)
    {
        Q_EMIT dirty(b);
    }

    void s_setFrozen(bool f)
    {
        Q_EMIT frozenChanged(f);
    }


    void s_minMaxChanged(DimSpec index)
    {
        Q_EMIT minMaxChanged(index);
    }

    void s_displayMinMaxChanged(DimSpec index)
    {
        Q_EMIT displayMinMaxChanged(index);
    }

    void s_helpChanged()
    {
        Q_EMIT helpChanged();
    }

    void s_expressionChanged(DimIdx dimension, ViewIdx view)
    {
        Q_EMIT expressionChanged(dimension, view);
    }

    void s_derivativeMoved(double time,
                           ViewIdx view,
                           DimIdx dimension)
    {
        Q_EMIT derivativeMoved(time, view, dimension);
    }

    void s_keyFrameInterpolationChanged(double time,
                                        ViewIdx view,
                                        DimIdx dimension)
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

    void s_dimensionNameChanged(DimIdx dimension)
    {
        Q_EMIT dimensionNameChanged(dimension);
    }

    void s_availableViewsChanged()
    {
        Q_EMIT availableViewsChanged();
    }


Q_SIGNALS:

    // Called whenever the evaluateOnChange property changed
    void evaluateOnChangeChanged(bool value);

    // Emitted whenever setAnimationLevel is called to update the user interface
    void animationLevelChanged(ViewSetSpec view, DimSpec dimension);

    // Emitted when the value should be updated on the GUI at the current timeline's time
    void mustRefreshKnobGui(ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason);

    // Emitted when the secret state of the knob changed
    void secretChanged();

    // Emitted when the secret state of the knob changed in the viewer context
    void viewerContextSecretChanged();

    // Emitted when a dimension enabled state changed
    void enabledChanged();

    // This is called to notify the gui that the knob shouldn't be editable.
    // Basically this is used when rendering with a Writer or for Trackers while tracking is active.
    void frozenChanged(bool frozen);

    // Emitted when the animation of a curve has changed. The added list contains keyframes that were added and removed list
    // keys that were removed
    void curveAnimationChanged(std::list<double> added, std::list<double> removed, ViewIdx view, DimIdx dimension);

    // Emitted when a derivative is changed on a keyframe
    void derivativeMoved(double time, ViewIdx view, DimIdx dimension);

    // Emitted when interpolation on a key is changed
    void keyFrameInterpolationChanged(double time, ViewIdx view, DimIdx dimension);

    // Emitted whenever a knob is slaved via the slaveTo function with a reason of eValueChangedReasonPluginEdited.
    void knobSlaved(DimIdx dimension, ViewIdx, bool slaved);

    // Same as setValueWithUndoStack except that the value change will be compressed
    // in a multiple edit undo/redo action
    void appendParamEditChange(ValueChangedReasonEnum reason, ValueChangedReturnCodeEnum setValueRetCode, Variant v, ViewSetSpec view, DimSpec dim, double time, bool setKeyFrame);

    // Emitted whenever the knob is dirty, @see KnobI::setDirty(bool)
    void dirty(bool);

    // Emitted when min/max changes for a value knob
    void minMaxChanged(DimSpec index);

    // Emitted when display min/max changes for a value knob
    void displayMinMaxChanged(DimSpec index);

    // Emitted when tooltip is changed
    void helpChanged();

    // Emitted when expr changed
    void expressionChanged(DimIdx dimension, ViewIdx view);

    // Emitted when the modification state of the knob has changed
    void hasModificationsChanged();

    // Emitted when the label has changed
    void labelChanged();

    // Emitted when the label has changed in the viewer
    void inViewerContextLabelChanged();

    // Emitted when a dimension label hash changed
    void dimensionNameChanged(DimIdx dimension);

    // Called when knob split/unsplit views
    void availableViewsChanged();
};

struct KnobChange
{
    KnobIPtr knob;
    ValueChangedReasonEnum reason, originalReason;
    bool originatedFromMainThread;
    double time;
    ViewSetSpec view;
    std::set<DimIdx> dimensionChanged;
    bool valueChangeBlocked;
};

typedef std::list<KnobChange> KnobChanges;

struct MasterKnobLink
{
    KnobIWPtr masterKnob;
    DimIdx masterDimension;
    ViewIdx masterView;
};

class KnobI
    : public AnimatingObjectI
    , public OverlaySupport
    , public boost::enable_shared_from_this<KnobI>
    , public SERIALIZATION_NAMESPACE::SerializableObjectBase
    , public HashableObject
{
    friend class KnobHolder;

protected: // parent of KnobHelper
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
     * @brief Deletes this knob permanantly
     **/
    virtual void deleteKnob() = 0;

public:

    struct ListenerLink
    {
        // Is the link made with an expression?
        bool isExpr;

        // The view of the listener that is listening
        ViewIdx listenerView;

        // The dimension of the listener that is listening
        DimIdx listenerDimension;

        // The dimension of the knob listened to
        DimIdx targetDim;

        // The view of the knob listened to
        ViewIdx targetView;

        ListenerLink()
        : isExpr(false)
        , listenerView()
        , listenerDimension()
        , targetDim()
        , targetView()
        {}
    };

    typedef std::list<ListenerLink> ListenerLinkList;
    typedef std::map<KnobIWPtr, ListenerLinkList> ListenerDimsMap;


    struct Expr
    {
        std::string expression; //< the one modified by Natron
        std::string originalExpression; //< the one input by the user
        std::string exprInvalid;
        bool hasRet;

        ///The list of pair<knob, dimension> dpendencies for an expression
        struct Dependency
        {
            KnobIWPtr knob;
            DimIdx dimension;
            ViewIdx view;
        };
        std::list<Dependency> dependencies;

        //PyObject* code;

        Expr()
        : expression(), originalExpression(), exprInvalid(), hasRet(false) /*, code(0)*/ {}
    };

    /**
     * @brief Do not call this. It is called right away after the constructor by the factory
     * to initialize curves and values. This is separated from the constructor as we need RTTI
     * for Curve.
     **/
    virtual void populate() = 0;
    virtual void setKnobGuiPointer(const KnobGuiIPtr& ptr) = 0;
    virtual KnobGuiIPtr getKnobGuiPointer() const = 0;
    virtual bool getAllDimensionVisible(ViewIdx view) const = 0;

    /**
     * @brief Given the dimension and view in input, if the knob has all its dimensions folded or is not multi-view,
     * adjust the arguments so that they best fit PasteKnobClipBoardUndoCommand
     **/
    virtual void convertDimViewArgAccordingToKnobState(DimSpec dimIn, ViewSetSpec viewIn, DimSpec* dimOut, ViewSetSpec* viewOut) const = 0;

    /**
     * @brief Returns the knob was created by a plugin or added automatically by Natron (e.g like mask knobs)
     **/
    virtual bool isDeclaredByPlugin() const = 0;
    virtual void setDeclaredByPlugin(bool b) = 0;

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
     * @brief Returns true if by default this knob has the animated flag on
     **/
    virtual bool isAnimatedByDefault() const = 0;

    /**
     * @brief Returns true if the knob has had modifications
     **/
    virtual bool hasModifications() const = 0;
    virtual bool hasModifications(DimIdx dimension) const = 0;
    virtual void computeHasModifications() = 0;
    virtual void refreshAnimationLevel(ViewSetSpec view, DimSpec dimension) = 0;

    /**
     * @brief If the parameter is multidimensional, this is the label thats the that will be displayed
     * for a dimension.
     **/
    virtual std::string getDimensionName(DimIdx dimension) const = 0;
    virtual void setDimensionName(DimIdx dim, const std::string & name) = 0;

    /**
     * @brief Set the value of the knob in the given dimension with the given reason.
     * @param dimension If set to all, all dimensions will receive the modification otherwise just the dimension given by the index
     * @param view If set to all, all views on the knob will receive the given value.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey If this knob has auto-keying enabled and its animation level is currently interpolated, then it may
     * attempt to set a keyframe automatically. If this parameter is non NULL the new keyframe (or modified keyframe) will
     * contain it.
     * @param forceHandlerEvenIfNoChange If true, even if the value of the knob did not change, the knobValueChanged handler of the
     * knob holder will be called and an evaluation will be triggered.
     * @return Returns the kind of changed that the knob has had
     **/
    virtual ValueChangedReturnCodeEnum setIntValue(int value,
                                                   ViewSetSpec view = ViewSetSpec::current(),
                                                   DimSpec dimension = DimSpec(0),
                                                   ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                   KeyFrame* newKey = 0,
                                                   bool forceHandlerEvenIfNoChange = false) = 0;

    /**
     * @brief Set multiple dimensions at once
     * This efficiently set values on all dimensions instead of
     * calling setValue for each dimension.
     * @param values The values to set, the vector must correspond to a contiguous set of dimensions
     * that are contained in the dimension count of the knob
     * @param dimensionStartIndex If the values do not represent all the dimension of the knob, this is the
     * dimension to start from
     * @param view If set to all, all views on the knob will receive the given values
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param reason The change reason
     **/
    virtual void setIntValueAcrossDimensions(const std::vector<int>& values,
                                             DimIdx dimensionStartIndex = DimIdx(0),
                                             ViewSetSpec view = ViewSetSpec::current(),
                                             ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                             std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) = 0;

    /**
     * @brief Set the value of the knob in the given dimension with the given reason.
     * @param dimension If set to all, all dimensions will receive the modification otherwise just the dimension given by the index
     * @param view If set to all, all views on the knob will receive the given value.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey If this knob has auto-keying enabled and its animation level is currently interpolated, then it may
     * attempt to set a keyframe automatically. If this parameter is non NULL the new keyframe (or modified keyframe) will
     * contain it.
     * @param forceHandlerEvenIfNoChange If true, even if the value of the knob did not change, the knobValueChanged handler of the
     * knob holder will be called and an evaluation will be triggered.
     * @return Returns the kind of changed that the knob has had
     **/
    virtual ValueChangedReturnCodeEnum setDoubleValue(double value,
                                                      ViewSetSpec view = ViewSetSpec::current(),
                                                      DimSpec dimension = DimSpec(0),
                                                      ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                      KeyFrame* newKey = 0,
                                                      bool forceHandlerEvenIfNoChange = false) = 0;

    /**
     * @brief Set multiple dimensions at once
     * This efficiently set values on all dimensions instead of
     * calling setValue for each dimension.
     * @param values The values to set, the vector must correspond to a contiguous set of dimensions
     * that are contained in the dimension count of the knob
     * @param dimensionStartIndex If the values do not represent all the dimension of the knob, this is the
     * dimension to start from
     * @param view If set to all, all views on the knob will receive the given values
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param reason The change reason
     **/
    virtual void setDoubleValueAcrossDimensions(const std::vector<double>& values,
                                                DimIdx dimensionStartIndex = DimIdx(0),
                                                ViewSetSpec view = ViewSetSpec::current(),
                                                ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) = 0;

    /**
     * @brief Set the value of the knob in the given dimension with the given reason.
     * @param dimension If set to all, all dimensions will receive the modification otherwise just the dimension given by the index
     * @param view If set to all, all views on the knob will receive the given value.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey If this knob has auto-keying enabled and its animation level is currently interpolated, then it may
     * attempt to set a keyframe automatically. If this parameter is non NULL the new keyframe (or modified keyframe) will
     * contain it.
     * @param forceHandlerEvenIfNoChange If true, even if the value of the knob did not change, the knobValueChanged handler of the
     * knob holder will be called and an evaluation will be triggered.
     * @return Returns the kind of changed that the knob has had
     **/
    virtual ValueChangedReturnCodeEnum setBoolValue(bool value,
                                                    ViewSetSpec view = ViewSetSpec::current(),
                                                    DimSpec dimension = DimSpec(0),
                                                    ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                    KeyFrame* newKey = 0,
                                                    bool forceHandlerEvenIfNoChange = false) = 0;

    /**
     * @brief Set multiple dimensions at once
     * This efficiently set values on all dimensions instead of
     * calling setValue for each dimension.
     * @param values The values to set, the vector must correspond to a contiguous set of dimensions
     * that are contained in the dimension count of the knob
     * @param dimensionStartIndex If the values do not represent all the dimension of the knob, this is the
     * dimension to start from
     * @param view If set to all, all views on the knob will receive the given values
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param reason The change reason
     **/
    virtual void setBoolValueAcrossDimensions(const std::vector<bool>& values,
                                              DimIdx dimensionStartIndex = DimIdx(0),
                                              ViewSetSpec view = ViewSetSpec::current(),
                                              ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                              std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) = 0;

    /**
     * @brief Set the value of the knob in the given dimension with the given reason.
     * @param dimension If set to all, all dimensions will receive the modification otherwise just the dimension given by the index
     * @param view If set to all, all views on the knob will receive the given value.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey If this knob has auto-keying enabled and its animation level is currently interpolated, then it may
     * attempt to set a keyframe automatically. If this parameter is non NULL the new keyframe (or modified keyframe) will
     * contain it.
     * @param forceHandlerEvenIfNoChange If true, even if the value of the knob did not change, the knobValueChanged handler of the
     * knob holder will be called and an evaluation will be triggered.
     * @return Returns the kind of changed that the knob has had
     **/
    virtual ValueChangedReturnCodeEnum setStringValue(const std::string& value,
                                                      ViewSetSpec view = ViewSetSpec::current(),
                                                      DimSpec dimension = DimSpec(0),
                                                      ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                      KeyFrame* newKey = 0,
                                                      bool forceHandlerEvenIfNoChange = false) = 0;


    /**
     * @brief Set multiple dimensions at once
     * This efficiently set values on all dimensions instead of
     * calling setValue for each dimension.
     * @param values The values to set, the vector must correspond to a contiguous set of dimensions
     * that are contained in the dimension count of the knob
     * @param dimensionStartIndex If the values do not represent all the dimension of the knob, this is the
     * dimension to start from
     * @param view If set to all, all views on the knob will receive the given values
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param reason The change reason
     **/
    virtual void setStringValueAcrossDimensions(const std::vector<std::string>& values,
                                                DimIdx dimensionStartIndex = DimIdx(0),
                                                ViewSetSpec view = ViewSetSpec::current(),
                                                ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) = 0;
    
    /**
     * @brief When in-between a begin/endChanges bracket, evaluate (render) action will not be called
     * when issuing value changes. Internally it maintains a counter, when it reaches 0 the evaluation is unblocked.
     **/
    virtual void beginChanges() = 0;

    /**
     * @brief To be called to reactivate evaluation. Internally it maintains a counter, when it reaches 0 the evaluation is unblocked.
     **/
    virtual void endChanges() = 0;

    // Prevent knobValueChanged handler calls until unblock is called
    virtual void blockValueChanges() = 0;
    virtual void unblockValueChanges() = 0;
    virtual bool isValueChangesBlocked() const = 0;

    // Prevent knob listening to this knob to be refreshed while under the block/unblock bracket
    virtual void blockListenersNotification() = 0;
    virtual void unblockListenersNotification() = 0;
    virtual bool isListenersNotificationBlocked() const = 0;

    // Prevent knob gui refreshing while under the block/unblock bracket
    virtual void blockGuiRefreshing() = 0;
    virtual void unblockGuiRefreshing() = 0;
    virtual bool isGuiRefreshingBlocked() const = 0;

    // Prevent autokeying
    virtual void setAutoKeyingEnabled(bool enabled) = 0;

    /**
     * @brief Returns the value previously set by setAutoKeyingEnabled
     **/
    virtual bool isAutoKeyingEnabled(DimSpec dimension, ViewSetSpec view, ValueChangedReasonEnum reason) const = 0;

    /**
     * @brief Called by setValue to refresh the GUI, call the instanceChanged action on the plugin and
     * evaluate the new value (cause a render).
     * @returns true if the knobChanged handler was called once for this knob
     **/
    virtual bool evaluateValueChange(DimSpec dimension, double time, ViewSetSpec view, ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Copies all the values, animations and extra data the other knob might have
     * to this knob. This function calls cloneExtraData.
     * The evaluateValueChange function will not be called as a result of the clone.
     * However a valueChanged signal will be emitted by the KnobSignalSlotHandler if there's any.
     *
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     *
     * @param dimension If -1 all dimensions will be cloned, otherwise you can clone only a specific dimension
     *
     * Note: This knob and 'other' MUST have the same dimension but not necessarily the same data type (e.g KnobInt can convert to KnobDouble
     * etc..)
     * @return True if this function succeeded and the knob has had its state changed, false otherwise
     **/
    virtual bool copyKnob(const KnobIPtr& other, ViewSetSpec view = ViewSetSpec::all(), DimSpec dimension = DimSpec::all(), ViewSetSpec otherView = ViewSetSpec::all(), DimSpec otherDimension = DimSpec::all(), const RangeD* range = 0, double offset = 0) = 0;

    virtual void cloneDefaultValues(const KnobIPtr& other) = 0;

    virtual double random(double time, unsigned int seed) const = 0;
    virtual double random(double min = 0., double max = 1.) const = 0;
    virtual int randomInt(double time, unsigned int seed) const = 0;
    virtual int randomInt(int min = INT_MIN, int max = INT_MAX) const = 0;

    /**
     * @brief Evaluates the curve at the given dimension and at the given time. This returns the value of the curve directly.
     * If the knob is holding a string, it will return the index.
     **/
    virtual double getRawCurveValueAt(double time, ViewGetSpec view, DimIdx dimension)  = 0;

    /**
     * @brief Same as getRawCurveValueAt, but first check if an expression is present. The expression should return a PoD.
     **/
    virtual double getValueAtWithExpression(double time, ViewGetSpec view, DimIdx dimension) = 0;

    /**
     * @brief Set an expression on the knob. If this expression is invalid, this function throws an excecption with the error from the
     * Python interpreter.
     * @param hasRetVariable If true the expression is expected to be multi-line and have its return value set to the variable "ret", otherwise
     * the expression is expected to be single-line.
     * @param force If set to true, this function will not check if the expression is valid nor will it attempt to compile/evaluate it, it will
     * just store it. This flag is used for serialisation, you should always pass false
     **/

protected:

    virtual void setExpressionCommon(DimSpec dimension, ViewSetSpec view, const std::string& expression, bool hasRetVariable, bool clearResults, bool failIfInvalid) = 0;

public:

    void restoreExpression(DimSpec dimension,
                           ViewSetSpec view,
                           const std::string& expression,
                           bool hasRetVariable)
    {
        setExpressionCommon(dimension, view, expression, hasRetVariable, false, false);
    }

    void setExpression(DimSpec dimension,
                       ViewSetSpec view,
                       const std::string& expression,
                       bool hasRetVariable,
                       bool failIfInvalid)
    {
        setExpressionCommon(dimension, view, expression, hasRetVariable, true, failIfInvalid);
    }

    /**
     * @brief Tries to re-apply invalid expressions, returns true if they are all valid
     **/
    virtual bool checkInvalidExpressions() = 0;
    virtual bool isExpressionValid(DimIdx dimension, ViewGetSpec view, std::string* error) const = 0;
    virtual void setExpressionInvalid(DimSpec dimension, ViewSetSpec view, bool valid, const std::string& error) = 0;


    /**
     * @brief For each dimension, try to find in the expression, if set, the node name "oldName" and replace
     * it by "newName"
     **/
    virtual void replaceNodeNameInExpression(DimSpec dimension,
                                             ViewSetSpec view,
                                             const std::string& oldName,
                                             const std::string& newName) = 0;
    virtual void clearExpressionsResults(DimSpec dimension, ViewSetSpec view) = 0;
    virtual void clearExpression(DimSpec dimension, ViewSetSpec view, bool clearResults) = 0;
    virtual std::string getExpression(DimIdx dimension, ViewGetSpec view = ViewGetSpec::current()) const = 0;

    /**
     * @brief Checks that the given expr for the given dimension will produce a correct behaviour.
     * On success this function returns correctly, otherwise an exception is thrown with the error.
     * This function also declares some extra python variables via the declareCurrentKnobVariable_Python function.
     * The new expression is returned.
     * @param resultAsString[out] The result of the execution of the expression will be written to the string.
     * @returns A new string containing the modified expression with the 'ret' variable declared if it wasn't already declared
     * by the user.
     **/
    virtual std::string validateExpression(const std::string& expression, DimIdx dimension, ViewIdx view, bool hasRetVariable, std::string* resultAsString) = 0;

protected:

    virtual void refreshListenersAfterValueChange(ViewSetSpec view, ValueChangedReasonEnum reason, DimSpec dimension) = 0;

public:

    /**
     * @brief Returns whether the expr at the given dimension uses the ret variable to assign to the return value or not
     **/
    virtual bool isExpressionUsingRetVariable(ViewGetSpec view, DimIdx dimension) const = 0;

    /**
     * @brief Returns in dependencies a list of all the knobs used in the expression at the given dimension
     * @returns True on sucess, false if no expression is set.
     **/
    virtual bool getExpressionDependencies(DimIdx dimension, ViewGetSpec view, std::list<KnobI::Expr::Dependency>& dependencies) const = 0;

    /**
     * @brief Called when the current time of the timeline changes.
     * It must get the value at the given time and notify  the gui it must
     * update the value displayed.
     **/
    virtual void onTimeChanged(bool isPlayback, double time) = 0;

    /**
     * @brief Compute the derivative at time as a double
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual double getDerivativeAtTime(double time, ViewGetSpec view, DimIdx dimension) = 0;

    /**
     * @brief Compute the integral of dimension from time1 to time2 as a double
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual double getIntegrateFromTimeToTime(double time1, double time2, ViewGetSpec view, DimIdx dimension)  = 0;

    /**
     * @brief Places in time the keyframe time at the given index.
     * If it exists the function returns true, false otherwise.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual bool getKeyFrameTime(ViewGetSpec view, int index, DimIdx dimension, double* time) const = 0;

    /**
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the last
     * keyframe.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual bool getLastKeyFrameTime(ViewGetSpec view, DimIdx dimension, double* time) const = 0;

    /**
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the first
     * keyframe.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual bool getFirstKeyFrameTime(ViewGetSpec view, DimIdx dimension, double* time) const = 0;

    /**
     * @brief Returns the count of keyframes in the given dimension.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual int getKeyFramesCount(ViewGetSpec view, DimIdx dimension) const = 0;

    /**
     * @brief Returns the nearest keyframe time if it was found.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     * @return true if it succeeded, false otherwise.
     **/
    virtual bool getNearestKeyFrameTime(ViewGetSpec view, DimIdx dimension, double time, double* nearestTime) const = 0;

    /**
     * @brief Returns the keyframe index if there's any keyframe in the curve
     * at the given dimension and the given time. -1 otherwise.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual int getKeyFrameIndex(ViewGetSpec view, DimIdx dimension, double time) const = 0;

    /**
     * @brief Returns a pointer to the curve in the given dimension and view.
     **/
    virtual CurvePtr getCurve(ViewGetSpec view, DimIdx dimension, bool byPassMaster = false) const = 0;

    /**
     * @brief Returns true if the curve corresponding to the dimension/view is animated with keyframes.
     * @param dimension The dimension index of the corresponding curve
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index. 
     **/
    virtual bool isAnimated( DimIdx dimension, ViewGetSpec view ) const = 0;

    /**
     * @brief Returns true if at least 1 dimension/view is animated. MT-Safe
     **/
    virtual bool hasAnimation() const = 0;

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
    virtual void setIconLabel(const std::string& iconFilePath, bool checked = false, bool alsoSetViewerUIIcon = true) = 0;
    virtual const std::string& getIconLabel(bool checked = false) const = 0;

    /**
     * @brief Returns a pointer to the holder owning the knob.
     **/
    virtual KnobHolderPtr getHolder() const = 0;
    virtual void setHolder(const KnobHolderPtr& holder) = 0;


    /**
     * @brief Controls the knob hashing stragey. @See KnobFrameViewHashingStrategyEnum
     **/
    virtual void setHashingStrategy(KnobFrameViewHashingStrategyEnum strategty) = 0;
    virtual KnobFrameViewHashingStrategyEnum getHashingStrategy() const = 0;

    virtual void appendToHash(double time, ViewIdx view, Hash64* hash) OVERRIDE = 0;

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
     * @brief Set the icon instead of the label for the viewer GUI
     **/
    virtual std::string getInViewerContextIconFilePath(bool checked) const = 0;
    virtual void setInViewerContextIconFilePath(const std::string& icon, bool checked = true) = 0;

    /**
     * @brief Determines whether this knob can be assigned a shortcut or not via the shortcut editor.
     * If true, Natron will look for a shortcut in the shortcuts database with an ID matching the name of this
     * parameter. To set default values for shortcuts, implement EffectInstance::getPluginShortcuts(...)
     **/
    virtual void setInViewerContextCanHaveShortcut(bool haveShortcut) = 0;
    virtual bool getInViewerContextHasShortcut() const = 0;

    /**
     * @brief If this knob has a viewer UI and it has an associated shortcut, the tooltip
     * will indicate to the viewer the shortcuts. The plug-in may also want to reference
     * other action shorcuts via this tooltip, and can add them here.
     * e.g: The Refresh button of the viewer shortcut is SHIFT+U but SHIFT+CTRL+U can also
     * be used to refresh but also enable in-depth render statistics
     * In the hint text, each additional shortcut must be reference with a %2, %3, %4, starting
     * from 2 since 1 is reserved for this knob's own shortcut.
     **/
    virtual void addInViewerContextShortcutsReference(const std::string& actionID) = 0;
    virtual const std::list<std::string>& getInViewerContextAdditionalShortcuts() const = 0;

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
     * @brief Controls whether to add horizontal stretch before or after (or none) stretch
     **/
    virtual void setInViewerContextLayoutType(ViewerContextLayoutTypeEnum type) = 0;
    virtual ViewerContextLayoutTypeEnum getInViewerContextLayoutType() const = 0;

    /**
     * @brief Set whether the knob should have its viewer GUI secret or not
     **/
    virtual void setInViewerContextSecret(bool secret) = 0;
    virtual bool  getInViewerContextSecret() const = 0;

    /**
     * @brief Enables/disables user interaction with the given dimension.
     **/
    virtual void setEnabled(bool b, DimSpec dimension = DimSpec::all(), ViewSetSpec view = ViewSetSpec(0)) = 0;

    /**
     * @brief Is the dimension enabled ?
     **/
    virtual bool isEnabled(DimIdx dimension = DimIdx(0), ViewGetSpec view = ViewGetSpec(0)) const = 0;

    /**
     * @brief Set the knob visible/invisible on the GUI representing it.
     **/
    virtual void setSecret(bool b) = 0;

    /**
     * @brief Is the knob visible to the user ?
     **/
    virtual bool getIsSecret() const = 0;

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
    virtual void setCustomInteract(const OfxParamOverlayInteractPtr & interactDesc) = 0;
    virtual OfxParamOverlayInteractPtr getCustomInteract() const = 0;
    virtual void swapOpenGLBuffers() OVERRIDE = 0;
    virtual void redraw() OVERRIDE = 0;
    virtual void getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const OVERRIDE = 0;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE = 0;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE = 0;
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
                             const DimIdx listenerDimension,
                             const DimIdx listenedToDimension,
                             const ViewIdx listenerView,
                             const ViewIdx listenedToView,
                             const KnobIPtr& listener) = 0;
    virtual void getAllExpressionDependenciesRecursive(std::set<NodePtr >& nodes) const = 0;

    /**
     * @brief Implement to save the content of the object to the serialization object
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE = 0;

    /**
     * @brief Implement to load the content of the serialization object onto this object
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase&  serializationBase) OVERRIDE = 0;

    virtual void restoreValueFromSerialization(const SERIALIZATION_NAMESPACE::ValueSerialization& obj, DimIdx targetDimension, ViewIdx view, bool restoreDefaultValue) = 0;

private:

    virtual void removeListener(const KnobIPtr& listener, DimIdx listenerDimension, ViewIdx listenerView) = 0;

public:

    virtual bool useHostOverlayHandle() const { return false; }

protected:

    virtual bool setHasModifications(DimIdx dimension, ViewIdx view, bool value, bool lock) = 0;

public:

    virtual void onKnobAboutToAlias(const KnobIPtr& /*slave*/) {}


    /**
     * @brief Slaves this Knob given dimension to the other knob dimension. 
     * @param otherKnob The knob to slave to
     * @param thisDimension If set to all, each dimension will be respectively slaved to the other knob
     * assuming they have a matching dimension in the other knob.
     * @param thisView If set to all, then all views
     * will be slaved, it is then expected that view == otherView == ViewSpec::all(). If not set to all valid view index should be given
     * @return True on success, false otherwise
     * Note that if this parameter is already slaved to another parameter this function will fail.
     **/
    virtual bool slaveTo(const KnobIPtr & otherKnob, DimSpec thisDimension = DimSpec::all(), DimSpec otherDimension = DimSpec::all(), ViewSetSpec view = ViewSetSpec::all(), ViewSetSpec otherView = ViewSetSpec::all()) = 0;

    /**
     * @brief Unslave the given dimension(s)/view(s) if they are slaved to another knob
     * @param copyState if true then the knob will copy the state of the master knob before removing the link, otherwise
     * it will revert to the state it had prior to slaving to the master knob.
     **/
    virtual void unSlave(DimSpec dimension, ViewSetSpec view, bool copyState) = 0;

    virtual bool isMastersPersistenceIgnored() const = 0;

    virtual void setMastersPersistenceIgnore(bool ignored) = 0;

    virtual KnobIPtr createDuplicateOnHolder(const KnobHolderPtr& otherHolder,
                                            const KnobPagePtr& page,
                                            const KnobGroupPtr& group,
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
     * @brief Returns a list of all the knobs whose value depends upon this knob.
     **/
    virtual void getListeners(ListenerDimsMap & listeners) const = 0;

    /**
     * @brief Returns a valid pointer to a knob if the value at
     * the given dimension is slaved.
     **/
    virtual bool getMaster(DimIdx dimension, ViewIdx view, MasterKnobLink* masterLink) const = 0;

    /**
     * @brief Returns true if the value at the given dimension is slave to another parameter
     **/
    virtual bool isSlave(DimIdx dimension, ViewIdx view) const = 0;

    /**
     * @brief Get the current animation level.
     **/
    virtual AnimationLevelEnum getAnimationLevel(DimIdx dimension, ViewGetSpec view) const = 0;

    /**
     * @brief Restores the default value
     **/
    virtual void resetToDefaultValue(DimSpec dimension = DimSpec::all(), ViewSetSpec view = ViewSetSpec::all()) = 0;

    /**
     * @brief Must return true if this Lnob holds a POD (plain old data) type, i.e. int, bool, or double.
     **/
    virtual bool isTypePOD() const = 0;

    /**
     * @brief Must return true if the other knobs type can convert to this knob's type.
     **/
    virtual bool isTypeCompatible(const KnobIPtr & other) const = 0;
    KnobPagePtr getTopLevelPage() const;

    /**
     * @brief Get list of views that are split off in the knob
     **/
    virtual std::list<ViewIdx> getViewsList() const = 0;

    /**
     * @brief Split the given view in the storage off the main view so that the user can give it different values
     **/
    virtual void splitView(ViewIdx view) = 0;
    virtual void unSplitView(ViewIdx view) = 0;
    virtual ViewIdx getViewIdxFromGetSpec(ViewGetSpec view) const = 0;

    /**
     * @brief Returns true if this knob supports multi-view
     **/
    virtual bool canSplitViews() const = 0;
};


///Skins the API of KnobI by implementing most of the functions in a non templated manner.
struct KnobHelperPrivate;
class KnobHelper
    : public KnobI
{
    Q_DECLARE_TR_FUNCTIONS(KnobHelper)

    // friends
    friend class KnobHolder;


protected: // derives from KnobI, parent of Knob
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    /**
     * @brief Creates a new Knob that belongs to the given holder, with the given label.
     * The name of the knob will be equal to the label, you can change it by calling setName()
     * The dimension parameter indicates how many dimension the knob should have.
     * If declaredByPlugin is false then Natron will never call onKnobValueChanged on the holder.
     **/
    KnobHelper(const KnobHolderPtr& holder,
               const std::string &label,
               int nDims = 1,
               bool declaredByPlugin = true);

public:
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

    virtual void setKnobGuiPointer(const KnobGuiIPtr& ptr) OVERRIDE FINAL;
    virtual KnobGuiIPtr getKnobGuiPointer() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getAllDimensionVisible(ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void convertDimViewArgAccordingToKnobState(DimSpec dimIn, ViewSetSpec viewIn, DimSpec* dimOut, ViewSetSpec* viewOut) const OVERRIDE FINAL;
    /**
     * @brief Returns the knob was created by a plugin or added automatically by Natron (e.g like mask knobs)
     **/
    virtual bool isDeclaredByPlugin() const OVERRIDE FINAL;
    virtual void setDeclaredByPlugin(bool b) OVERRIDE FINAL;
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
    virtual void blockGuiRefreshing() OVERRIDE FINAL;
    virtual void unblockGuiRefreshing() OVERRIDE FINAL;
    virtual bool isGuiRefreshingBlocked() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAutoKeyingEnabled(bool enabled) OVERRIDE FINAL;
    virtual bool isAutoKeyingEnabled(DimSpec dimension, ViewSetSpec view, ValueChangedReasonEnum reason) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool evaluateValueChange(DimSpec dimension, double time, ViewSetSpec view,  ValueChangedReasonEnum reason) OVERRIDE FINAL;

private:

    bool isAutoKeyingEnabledInternal(DimIdx dimension, ViewIdx view) const WARN_UNUSED_RETURN;

protected:
    // Returns true if the knobChanged handler was called
    bool evaluateValueChangeInternal(DimSpec dimension,
                                     double time,
                                     ViewSetSpec view,
                                     ValueChangedReasonEnum reason,
                                     ValueChangedReasonEnum originalReason);

    virtual void onInternalValueChanged(DimSpec /*dimension*/,
                                        double /*time*/,
                                        ViewSetSpec /*view*/)
    {
    }

public:


    virtual double random(double time, unsigned int seed) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double random(double min = 0., double max = 1.) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int randomInt(double time, unsigned int seed) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int randomInt(int min = 0, int max = INT_MAX) const OVERRIDE FINAL WARN_UNUSED_RETURN;

protected:


    void randomSeed(double time, unsigned int seed) const;

#ifdef DEBUG
    //Helper to set breakpoints in templated code
    void debugHook();
#endif


public:

    //////////// Overriden from AnimatingObjectI
    virtual bool cloneCurve(ViewIdx view, DimIdx dimension, const Curve& curve, double offset, const RangeD* range, const StringAnimationManager* stringAnimation) OVERRIDE;
    virtual void deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec dimension) OVERRIDE;
    virtual bool warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes = 0) OVERRIDE ;
    virtual void removeAnimation(ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void deleteAnimationBeforeTime(double time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void deleteAnimationAfterTime(double time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void setInterpolationAtTimes(ViewSetSpec view, DimSpec dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys = 0) OVERRIDE ;
    virtual bool setLeftAndRightDerivativesAtTime(ViewSetSpec view, DimSpec dimension, double time, double left, double right)  OVERRIDE WARN_UNUSED_RETURN;
    virtual bool setDerivativeAtTime(ViewSetSpec view, DimSpec dimension, double time, double derivative, bool isLeft) OVERRIDE WARN_UNUSED_RETURN;
    virtual CurvePtr getAnimationCurve(ViewGetSpec idx, DimIdx dimension) const OVERRIDE ;
    //////////// End from AnimatingObjectI

private:

    void removeAnimationInternal(ViewIdx view, DimIdx dimension);
    void deleteValuesAtTimeInternal(const std::list<double>& times, ViewIdx view, DimIdx dimension);
    void deleteAnimationConditional(double time, ViewSetSpec view, DimSpec dimension, bool before);
    void deleteAnimationConditionalInternal(double time, ViewIdx view, DimIdx dimension, bool before);
    bool warpValuesAtTimeInternal(const std::list<double>& times, ViewIdx view,  DimIdx dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes);
    void setInterpolationAtTimesInternal(ViewIdx view, DimIdx dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys);
    bool setLeftAndRightDerivativesAtTimeInternal(ViewIdx view, DimIdx dimension, double time, double left, double right);
    bool setDerivativeAtTimeInternal(ViewIdx view, DimIdx dimension, double time, double derivative, bool isLeft);

public:


    virtual bool getKeyFrameTime(ViewGetSpec view, int index, DimIdx dimension, double* time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getLastKeyFrameTime(ViewGetSpec view, DimIdx dimension, double* time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getFirstKeyFrameTime(ViewGetSpec view, DimIdx dimension, double* time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getKeyFramesCount(ViewGetSpec view, DimIdx dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getNearestKeyFrameTime(ViewGetSpec view, DimIdx dimension, double time, double* nearestTime) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getKeyFrameIndex(ViewGetSpec view, DimIdx dimension, double time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual CurvePtr getCurve(ViewGetSpec view, DimIdx dimension, bool byPassMaster = false) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isAnimated( DimIdx dimension, ViewGetSpec view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasAnimation() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool checkInvalidExpressions() OVERRIDE FINAL;
    virtual bool isExpressionValid(DimIdx dimension, ViewGetSpec view, std::string* error) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setExpressionInvalid(DimSpec dimension, ViewSetSpec view, bool valid, const std::string& error) OVERRIDE FINAL;

protected:

    void setExpressionInternal(DimIdx dimension, ViewIdx view, const std::string& expression, bool hasRetVariable, bool clearResults, bool failIfInvalid);

private:

    void setExpressionInvalidInternal(DimIdx dimension, ViewIdx view, bool valid, const std::string& error);

    void replaceNodeNameInExpressionInternal(DimIdx dimension,
                                             ViewIdx view,
                                             const std::string& oldName,
                                             const std::string& newName);

    void clearExpressionInternal(DimIdx dimension, ViewIdx view);
public:

    virtual void setExpressionCommon(DimSpec dimension, ViewSetSpec view, const std::string& expression, bool hasRetVariable, bool clearResults, bool failIfInvalid) OVERRIDE FINAL;

    virtual void replaceNodeNameInExpression(DimSpec dimension,
                                             ViewSetSpec view,
                                             const std::string& oldName,
                                             const std::string& newName) OVERRIDE FINAL;
    virtual void clearExpression(DimSpec dimension, ViewSetSpec view, bool clearResults) OVERRIDE FINAL;
    virtual std::string validateExpression(const std::string& expression, DimIdx dimension, ViewIdx view, bool hasRetVariable, std::string* resultAsString) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool slaveTo(const KnobIPtr & otherKnob, DimSpec thisDimension = DimSpec::all(), DimSpec otherDimension = DimSpec::all(), ViewSetSpec view = ViewSetSpec::all(), ViewSetSpec otherView = ViewSetSpec::all()) OVERRIDE FINAL;
    virtual void unSlave(DimSpec dimension, ViewSetSpec view, bool copyState) OVERRIDE FINAL;

protected:

    virtual void unSlaveInternal(DimIdx dimension, ViewIdx view, bool copyState) = 0;

private:

    bool slaveToInternal(const KnobIPtr & otherKnob, DimIdx thisDimension, DimIdx otherDimension, ViewIdx view, ViewIdx otherView) WARN_UNUSED_RETURN;


protected:

    template <typename T>
    T pyObjectToType(PyObject* o, ViewIdx view) const;

    virtual void refreshListenersAfterValueChange(ViewSetSpec view, ValueChangedReasonEnum reason, DimSpec dimension) OVERRIDE FINAL;

public:

    virtual bool isExpressionUsingRetVariable(ViewGetSpec view, DimIdx dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getExpressionDependencies(DimIdx dimension, ViewGetSpec view, std::list<KnobI::Expr::Dependency>& dependencies) const OVERRIDE FINAL;
    virtual std::string getExpression(DimIdx dimension, ViewGetSpec view = ViewGetSpec::current()) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAnimationEnabled(bool val) OVERRIDE FINAL;
    virtual bool isAnimationEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string  getLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    void setLabel(const std::string& label) OVERRIDE FINAL;
    void setLabel(const QString & label) { setLabel( label.toStdString() ); }

    virtual void setIconLabel(const std::string& iconFilePath, bool checked = false, bool alsoSetViewerUIIcon = true) OVERRIDE FINAL;
    virtual const std::string& getIconLabel(bool checked = false) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual KnobHolderPtr getHolder() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setHolder(const KnobHolderPtr& holder) OVERRIDE FINAL;
    virtual void setHashingStrategy(KnobFrameViewHashingStrategyEnum strategty) OVERRIDE FINAL;
    virtual KnobFrameViewHashingStrategyEnum getHashingStrategy() const OVERRIDE FINAL;
    virtual int getNDimensions() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAddNewLine(bool newLine) OVERRIDE FINAL;
    virtual void setAddSeparator(bool addSep) OVERRIDE FINAL;
    virtual bool isNewLineActivated() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isSeparatorActivated() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setSpacingBetweenItems(int spacing) OVERRIDE FINAL;
    virtual int getSpacingBetweenitems() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getInViewerContextLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextLabel(const QString& label) OVERRIDE FINAL;
    virtual std::string getInViewerContextIconFilePath(bool checked) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextIconFilePath(const std::string& icon, bool checked = false) OVERRIDE FINAL;
    virtual void setInViewerContextCanHaveShortcut(bool haveShortcut) OVERRIDE FINAL;
    virtual bool getInViewerContextHasShortcut() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void addInViewerContextShortcutsReference(const std::string& actionID) OVERRIDE FINAL;
    virtual const std::list<std::string>& getInViewerContextAdditionalShortcuts() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextItemSpacing(int spacing) OVERRIDE FINAL;
    virtual int  getInViewerContextItemSpacing() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextLayoutType(ViewerContextLayoutTypeEnum type) OVERRIDE FINAL;
    virtual ViewerContextLayoutTypeEnum getInViewerContextLayoutType() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setInViewerContextSecret(bool secret) OVERRIDE FINAL;
    virtual bool  getInViewerContextSecret() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setEnabled(bool b, DimSpec dimension = DimSpec::all(), ViewSetSpec view = ViewSetSpec(0)) OVERRIDE FINAL;
    virtual bool isEnabled(DimIdx dimension = DimIdx(0), ViewGetSpec view = ViewGetSpec(0)) const OVERRIDE FINAL;
    virtual void setSecret(bool b) OVERRIDE FINAL;
    virtual bool getIsSecret() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getIsSecretRecursive() const OVERRIDE FINAL WARN_UNUSED_RETURN;
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
    virtual void setCustomInteract(const OfxParamOverlayInteractPtr & interactDesc) OVERRIDE FINAL;
    virtual OfxParamOverlayInteractPtr getCustomInteract() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;
    virtual void redraw() OVERRIDE FINAL;
    virtual void getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const OVERRIDE FINAL;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;
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
    virtual void setMastersPersistenceIgnore(bool ignored) OVERRIDE FINAL;
    virtual double getCurrentTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ViewIdx getCurrentView() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getDimensionName(DimIdx dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setDimensionName(DimIdx dim, const std::string & name) OVERRIDE FINAL;
    virtual bool hasModifications() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasModifications(DimIdx dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void refreshAnimationLevel(ViewSetSpec view, DimSpec dimension) OVERRIDE FINAL;
    virtual KnobIPtr createDuplicateOnHolder(const KnobHolderPtr& otherHolder,
                                            const KnobPagePtr& page,
                                            const KnobGroupPtr& group,
                                            int indexInParent,
                                            bool makeAlias,
                                            const std::string& newScriptName,
                                            const std::string& newLabel,
                                            const std::string& newToolTip,
                                            bool refreshParams,
                                            bool isUserKnob) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual KnobIPtr getAliasMaster() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool setKnobAsAliasOfThis(const KnobIPtr& master, bool doAlias) OVERRIDE FINAL;

    virtual bool hasDefaultValueChanged(DimIdx dimension) const = 0;

private:

    void refreshAnimationLevelInternal(ViewIdx view, DimIdx dimension);

protected:

    virtual bool setHasModifications(DimIdx dimension, ViewIdx view, bool value, bool lock) OVERRIDE FINAL;

    /**
     * @brief Protected so the implementation of unSlave can actually use this to reset the master pointer
     **/
    void resetMaster(DimIdx dimension, ViewIdx view);

    ///The return value must be Py_DECRREF
    bool executeExpression(double time, ViewIdx view, DimIdx dimension, PyObject** ret, std::string* error) const;

public:

    virtual bool getMaster(DimIdx dimension, ViewIdx view, MasterKnobLink* masterLink) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isSlave(DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual AnimationLevelEnum getAnimationLevel(DimIdx dimension, ViewGetSpec view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isTypeCompatible(const KnobIPtr & other) const OVERRIDE WARN_UNUSED_RETURN = 0;


    /**
     * @brief Adds a new listener to this knob. This is just a pure notification about the fact that the given knob
     * is listening to the values/keyframes of "this". It could be call addSlave but it will also be use for expressions.
     **/
    virtual void addListener(bool isFromExpr,
                             DimIdx fromExprDimension,
                             DimIdx thisDimension,
                             const ViewIdx listenerView,
                             const ViewIdx listenedToView, const KnobIPtr& knob) OVERRIDE FINAL;
    virtual void removeListener(const KnobIPtr& listener, DimIdx listenerDimension, ViewIdx listenerView) OVERRIDE FINAL;
    virtual void getAllExpressionDependenciesRecursive(std::set<NodePtr >& nodes) const OVERRIDE FINAL;
    virtual void getListeners(KnobI::ListenerDimsMap& listeners) const OVERRIDE FINAL;
    virtual void clearExpressionsResults(DimSpec /*dimension*/, ViewSetSpec /*view*/) OVERRIDE {}

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

    /**
     * @brief Implement to save the content of the object to the serialization object
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    /**
     * @brief Implement to load the content of the serialization object onto this object
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;

    virtual void restoreValueFromSerialization(const SERIALIZATION_NAMESPACE::ValueSerialization& obj, DimIdx targetDimension, ViewIdx view, bool restoreDefaultValue) OVERRIDE FINAL;


    virtual std::list<ViewIdx> getViewsList() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void splitView(ViewIdx view) OVERRIDE;

    virtual void unSplitView(ViewIdx view) OVERRIDE;

    virtual ViewIdx getViewIdxFromGetSpec(ViewGetSpec view) const OVERRIDE FINAL WARN_UNUSED_RETURN;

protected:


    virtual void copyValuesFromCurve(DimSpec /*dim*/, ViewSetSpec /*view*/) {}


    virtual void handleSignalSlotsForAliasLink(const KnobIPtr& /*alias*/,
                                               bool /*connect*/)
    {
    }

    /**
     * @brief Called when you must copy any extra data you maintain from the other knob.
     * The other knob is guaranteed to be of the same type.
     **/
    virtual bool cloneExtraData(const KnobIPtr& /*other*/,
                                ViewSetSpec /*view*/,
                                ViewSetSpec /*otherView*/,
                                DimSpec /*dimension*/,
                                DimSpec /*otherDimension*/,
                                double /*offset*/,
                                const RangeD* /*range*/)
    {

        return false;
    }

    bool cloneExpressions(const KnobIPtr& other, ViewSetSpec view, ViewSetSpec otherView, DimSpec dimension, DimSpec otherDimension);

private:

    bool cloneExpressionInternal(const KnobIPtr& other, ViewIdx view, ViewIdx otherView, DimIdx dimension, DimIdx otherDimension);

public:


    virtual void cloneExpressionsResults(const KnobIPtr& /*other*/,
                                         ViewSetSpec /*view*/,
                                         ViewSetSpec /*otherView*/,
                                         DimSpec /*dimension */,
                                         DimSpec /*otherDimension */) {}

    bool cloneCurves(const KnobIPtr& other, ViewSetSpec view, ViewSetSpec otherView, DimSpec dimension, DimSpec otherDimension, double offset, const RangeD* range);


    /**
     * @brief Called when a curve animation is changed.
     * Derived knobs can use it to refresh any data structure related to keyframes it may have.
     **/
    virtual void onKeyframesRemoved(const std::list<double>& /*keysRemoved*/,
                                    ViewSetSpec /*view*/,
                                    DimSpec /*dimension*/)
    {
    }




    boost::shared_ptr<KnobSignalSlotHandler> _signalSlotHandler;

private:

    void expressionChanged(DimIdx dimension, ViewIdx view);

    boost::scoped_ptr<KnobHelperPrivate> _imp;
};

inline KnobHelperPtr
toKnobHelper(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobHelper>(knob);
}

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
    typedef std::map<ViewIdx, FrameValueMap> PerViewFrameValueMap;
    typedef std::vector<PerViewFrameValueMap> PerDimensionFrameValueMap;


    // For each dimension a map of all views that are split-off
    typedef std::map<ViewIdx, T> PerViewValueMap;
    typedef std::vector<PerViewValueMap> PerDimensionValuesVec;

protected: // derives from KnobI, parent of KnobInt, KnobBool
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    /**
     * @brief Make a knob for the given KnobHolder with the given label (the label displayed on
     * its interface) and with the given dimension. The dimension parameter is used for example for the
     * KnobColor which has 4 doubles (r,g,b,a), hence 4 dimensions.
     **/
    Knob(const KnobHolderPtr& holder,
         const std::string & label,
         int nDims = 1,
         bool declaredByPlugin = true);

public:
    virtual ~Knob();


    virtual void computeHasModifications() OVERRIDE FINAL;

protected:

    virtual bool computeValuesHaveModifications(DimIdx dimension,
                                                const T& value,
                                                const T& defaultValue) const;
    virtual bool hasModificationsVirtual(DimIdx /*dimension*/, ViewIdx /*view*/) const { return false; }

public:

    /**
     * @brief Get the current value of the knob for the given dimension and view.
     * If it is animated, it will return the value at the current time.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    T getValue(DimIdx dimension = DimIdx(0), ViewGetSpec view = ViewGetSpec::current(), bool clampToMinMax = true) WARN_UNUSED_RETURN;


    /**
     * @brief Returns the value of the knob at the given time and for the given dimension and view.
     * If it is not animated, it will call getValue for that dimension and return the result.
     *
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     *
     * Note: This function is overloaded by the KnobString which can have its custom interpolation
     * but this should be the only knob which should ever need to overload it.
     *
     **/
    T getValueAtTime(double time, DimIdx dimension = DimIdx(0), ViewGetSpec view = ViewGetSpec::current(), bool clampToMinMax = true, bool byPassMaster = false)  WARN_UNUSED_RETURN;

    /**
     * @brief Same as getValueAtTime excepts that it ignores expression, hard-links (slave/master) and doesn't clamp to min/max.
     * This is useful to display the internal curve on the Curve Editor
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual double getRawCurveValueAt(double time, ViewGetSpec view,  DimIdx dimension)  OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Same as getValueAtTime excepts that the expression is evaluated to return a double value, mainly to display the curve corresponding to an expression
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual double getValueAtWithExpression(double time, ViewGetSpec view, DimIdx dimension)  OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns a vector of the internal values held by the knob (not the animation)
     * This is used only internally for the implementation and should not be used, instead use getValue()
     **/
    PerDimensionValuesVec getRawValues() const;
    T getRawValue(DimIdx dimension, ViewIdx view) const;

private:


    virtual void unSlaveInternal(DimIdx dimension, ViewIdx view, bool copyState) OVERRIDE FINAL;

public:

    virtual void appendToHash(double time, ViewIdx view, Hash64* hash) OVERRIDE;

    //////////// Overriden from AnimatingObjectI
    virtual KeyframeDataTypeEnum getKeyFrameDataType() const OVERRIDE FINAL;

    virtual ValueChangedReturnCodeEnum setIntValueAtTime(double time, int value, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, KeyFrame* newKey = 0) OVERRIDE ;
    virtual void setMultipleIntValueAtTime(const std::list<IntTimeValuePair>& keys, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<KeyFrame>* newKey = 0) OVERRIDE ;
    virtual void setIntValueAtTimeAcrossDimensions(double time, const std::vector<int>& values, DimIdx dimensionStartIndex = DimIdx(0), ViewSetSpec view = ViewSetSpec::current(), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE ;
    virtual void setMultipleIntValueAtTimeAcrossDimensions(const PerCurveIntValuesList& keysPerDimension,  ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited) OVERRIDE ;

    virtual ValueChangedReturnCodeEnum setDoubleValueAtTime(double time, double value, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, KeyFrame* newKey = 0) OVERRIDE ;
    virtual void setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& keys, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<KeyFrame>* newKey = 0) OVERRIDE ;
    virtual void setDoubleValueAtTimeAcrossDimensions(double time, const std::vector<double>& values, DimIdx dimensionStartIndex = DimIdx(0), ViewSetSpec view = ViewSetSpec::current(), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE ;
    virtual void setMultipleDoubleValueAtTimeAcrossDimensions(const PerCurveDoubleValuesList& keysPerDimension, ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited) OVERRIDE ;

    virtual ValueChangedReturnCodeEnum setBoolValueAtTime(double time, bool value, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, KeyFrame* newKey = 0) OVERRIDE ;
    virtual void setMultipleBoolValueAtTime(const std::list<BoolTimeValuePair>& keys, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<KeyFrame>* newKey = 0) OVERRIDE ;
    virtual void setBoolValueAtTimeAcrossDimensions(double time, const std::vector<bool>& values, DimIdx dimensionStartIndex = DimIdx(0), ViewSetSpec view = ViewSetSpec::current(), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE;
    virtual void setMultipleBoolValueAtTimeAcrossDimensions(const PerCurveBoolValuesList& keysPerDimension, ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited) OVERRIDE;

    virtual ValueChangedReturnCodeEnum setStringValueAtTime(double time, const std::string& value, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, KeyFrame* newKey = 0) OVERRIDE ;
    virtual void setMultipleStringValueAtTime(const std::list<StringTimeValuePair>& keys, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<KeyFrame>* newKey = 0) OVERRIDE ;
    virtual void setStringValueAtTimeAcrossDimensions(double time, const std::vector<std::string>& values, DimIdx dimensionStartIndex = DimIdx(0), ViewSetSpec view = ViewSetSpec::current(), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE;
    virtual void setMultipleStringValueAtTimeAcrossDimensions(const PerCurveStringValuesList& keysPerDimension, ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited) OVERRIDE;
    //////////// end overriden from AnimatingObjectI

    /**
     * @brief Set the value of the knob at a specific time (keyframe) in the given dimension with the given reason.
     * @param view If set to all, all views on the knob will receive the given value.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey If this knob has auto-keying enabled and its animation level is currently interpolated, then it may
     * attempt to set a keyframe automatically. If this parameter is non NULL the new keyframe (or modified keyframe) will
     * contain it.
     * @param forceHandlerEvenIfNoChange If true, even if the value of the knob did not change, the knobValueChanged handler of the
     * knob holder will be called and an evaluation will be triggered.
     * @return Returns the kind of changed that the knob has had
     **/
    ValueChangedReturnCodeEnum setValueAtTime(double time,
                                              const T & v,
                                              ViewSetSpec view = ViewSetSpec::current(),
                                              DimSpec dimension = DimSpec(0),
                                              ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                              KeyFrame* newKey = 0,
                                              bool forceHandlerEvenIfNoChange = false);

    /**
     * @brief Wraps multiple calls to  setValueAtTime on the curve at the given view and dimension.
     * This is efficient to set many keyframes on the given curve.
     * @param newKey[out] If non null, this will be set to the new keyframe in return
     **/
    void setMultipleValueAtTime(const std::list<TimeValuePair<T> >& keys, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<KeyFrame>* newKey = 0);


    /**
     * @brief Set a keyframe on multiple dimensions at once. This efficiently set values on all dimensions instead of 
     * calling setValueAtTime for each dimension.
     * @param values The values to set, the vector must correspond to a contiguous set of dimensions
     * that are contained in the dimension count of the knob
     * @param dimensionStartIndex If the values do not represent all the dimension of the knob, this is the
     * dimension to start from
     * @param view If set to all, all views on the knob will receive the given values
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param reason The change reason
     **/
    void setValueAtTimeAcrossDimensions(double time,
                                        const std::vector<T>& values,
                                        DimIdx dimensionStartIndex = DimIdx(0),
                                        ViewSetSpec view = ViewSetSpec::current(),
                                        ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                        std::vector<ValueChangedReturnCodeEnum>* retCodes = 0);

    /**
     * @brief Wraps multiple calls to  setValueAtTimeAcrossDimensions on the curve at the given view and dimension
     **/
    void setMultipleValueAtTimeAcrossDimensions(const std::vector<std::pair<DimensionViewPair, std::list<TimeValuePair<T> > > >& keysPerDimension, ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited);


    virtual ValueChangedReturnCodeEnum setIntValue(int value,
                                                   ViewSetSpec view = ViewSetSpec::current(),
                                                   DimSpec dimension = DimSpec(0),
                                                   ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                   KeyFrame* newKey = 0,
                                                   bool forceHandlerEvenIfNoChange = false) OVERRIDE FINAL;

    virtual void setIntValueAcrossDimensions(const std::vector<int>& values,
                                             DimIdx dimensionStartIndex,
                                             ViewSetSpec view,
                                             ValueChangedReasonEnum reason,
                                             std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE FINAL;

    virtual ValueChangedReturnCodeEnum setDoubleValue(double value,
                                                      ViewSetSpec view = ViewSetSpec::current(),
                                                      DimSpec dimension = DimSpec(0),
                                                      ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                      KeyFrame* newKey = 0,
                                                      bool forceHandlerEvenIfNoChange = false) OVERRIDE FINAL;

    virtual void setDoubleValueAcrossDimensions(const std::vector<double>& values,
                                                DimIdx dimensionStartIndex,
                                                ViewSetSpec view,
                                                ValueChangedReasonEnum reason,
                                                std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE FINAL;

    virtual ValueChangedReturnCodeEnum setBoolValue(bool value,
                                                    ViewSetSpec view = ViewSetSpec::current(),
                                                    DimSpec dimension = DimSpec(0),
                                                    ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                    KeyFrame* newKey = 0,
                                                    bool forceHandlerEvenIfNoChange = false) OVERRIDE FINAL;

    virtual void setBoolValueAcrossDimensions(const std::vector<bool>& values,
                                              DimIdx dimensionStartIndex,
                                              ViewSetSpec view,
                                              ValueChangedReasonEnum reason,
                                              std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE FINAL;

    virtual ValueChangedReturnCodeEnum setStringValue(const std::string& value,
                                                      ViewSetSpec view = ViewSetSpec::current(),
                                                      DimSpec dimension = DimSpec(0),
                                                      ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                                      KeyFrame* newKey = 0,
                                                      bool forceHandlerEvenIfNoChange = false) OVERRIDE FINAL;

    virtual void setStringValueAcrossDimensions(const std::vector<std::string>& values,
                                                DimIdx dimensionStartIndex,
                                                ViewSetSpec view,
                                                ValueChangedReasonEnum reason,
                                                std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE FINAL;


    /**
     * @brief Set the value of the knob in the given dimension with the given reason.
     * @param view If set to all, all views on the knob will receive the given value.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey If this knob has auto-keying enabled and its animation level is currently interpolated, then it may
     * attempt to set a keyframe automatically. If this parameter is non NULL the new keyframe (or modified keyframe) will 
     * contain it. 
     * @param forceHandlerEvenIfNoChange If true, even if the value of the knob did not change, the knobValueChanged handler of the
     * knob holder will be called and an evaluation will be triggered.
     * @return Returns the kind of changed that the knob has had
     **/
    ValueChangedReturnCodeEnum setValue(const T & v,
                                        ViewSetSpec view = ViewSetSpec::current(),
                                        DimSpec dimension = DimSpec(0),
                                        ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                        KeyFrame* newKey = 0,
                                        bool forceHandlerEvenIfNoChange = false);



    /**
     * @brief Set multiple dimensions at once
     * This efficiently set values on all dimensions instead of
     * calling setValue for each dimension.
     * @param values The values to set, the vector must correspond to a contiguous set of dimensions
     * that are contained in the dimension count of the knob
     * @param dimensionStartIndex If the values do not represent all the dimension of the knob, this is the
     * dimension to start from
     * @param view If set to all, all views on the knob will receive the given values
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param reason The change reason
     **/
    void setValueAcrossDimensions(const std::vector<T>& values,
                                  DimIdx dimensionStartIndex = DimIdx(0),
                                  ViewSetSpec view = ViewSetSpec::current(),
                                  ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited,
                                  std::vector<ValueChangedReturnCodeEnum>* retCodes = 0);



    /**
     * @brief Get the current default values
     **/
    T getDefaultValue(DimIdx dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief If the default value was changed more than once with setDefaultValue, this is the initial value that was passed to setDefaultValue
     **/
    T getInitialDefaultValue(DimIdx dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief Returns true if setDefaultValue was never called yet
     **/
    bool isDefaultValueSet(DimIdx dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief Set the current default value as if it was the initial default value passed to the knob
     **/
    void setCurrentDefaultValueAsInitialValue();

    /**
     * @brief Set a default value for a specific dimension
     * Note that it will also restore the knob to its defaults
     **/
    void setDefaultValue(const T & v, DimSpec dimension = DimSpec(0));
    void setDefaultValues(const std::vector<T>& values, DimIdx dimensionStartOffset);

    /**
     * @brief Same as setDefaultValue except that it does not restore the knob to its defaults
     **/
    void setDefaultValueWithoutApplying(const T& v, DimSpec dimension = DimSpec(0));
    void setDefaultValuesWithoutApplying(const std::vector<T>& values, DimIdx dimensionStartOffset = DimIdx(0));


    //////////////////////////////////////////////////////////////////////
    ///////////////////////////////////
    /////////////////////////////////// Implementation of KnobI
    //////////////////////////////////////////////////////////////////////

    ///Populates for each dimension: the default value and the value member. The implementation MUST call
    /// the KnobHelper version.
    virtual void populate() OVERRIDE;

    /// You must implement it
    virtual const std::string & typeName() const OVERRIDE;

    /// You must implement it
    virtual bool canAnimate() const OVERRIDE;
    virtual bool isTypePOD() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isTypeCompatible(const KnobIPtr & other) const OVERRIDE WARN_UNUSED_RETURN;

    ///Cannot be overloaded by KnobHelper as it requires setValue
    virtual void onTimeChanged(bool isPlayback, double time) OVERRIDE FINAL;

    ///Cannot be overloaded by KnobHelper as it requires the value member
    virtual double getDerivativeAtTime(double time, ViewGetSpec view, DimIdx dimension)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double getIntegrateFromTimeToTime(double time1, double time2, ViewGetSpec view, DimIdx dimension)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    double getIntegrateFromTimeToTimeSimpson(double time1, double time2, ViewGetSpec view, DimIdx dimension);

    ///Cannot be overloaded by KnobHelper as it requires setValue
    virtual void resetToDefaultValue(DimSpec dimension = DimSpec::all(), ViewSetSpec view = ViewSetSpec::all()) OVERRIDE FINAL;
    virtual bool copyKnob(const KnobIPtr& other, ViewSetSpec view = ViewSetSpec::all(), DimSpec dimension = DimSpec::all(), ViewSetSpec otherView = ViewSetSpec::all(), DimSpec otherDimension = DimSpec::all(), const RangeD* range = 0, double offset = 0) OVERRIDE FINAL;
    virtual void cloneDefaultValues(const KnobIPtr& other) OVERRIDE FINAL;

    ///MT-safe
    void setRange(const T& mini, const T& maxi, DimSpec dimension = DimSpec::all());
    void setDisplayRange(const T& mini, const T& maxi, DimSpec dimension = DimSpec::all());
    void setRangeAcrossDimensions(const std::vector<T> &minis, const std::vector<T> &maxis);
    void setDisplayRangeAcrossDimensions(const std::vector<T> &minis, const std::vector<T> &maxis);


    ///Not MT-SAFE, can only be called from main thread
    const std::vector<T> &getMinimums() const;
    const std::vector<T> &getMaximums() const;
    const std::vector<T> &getDisplayMinimums() const;
    const std::vector<T> &getDisplayMaximums() const;

    /// MT-SAFE
    T getMinimum(DimIdx dimension = DimIdx(0)) const;
    T getMaximum(DimIdx dimension = DimIdx(0)) const;
    T getDisplayMinimum(DimIdx dimension = DimIdx(0)) const;
    T getDisplayMaximum(DimIdx dimension = DimIdx(0)) const;

    void getExpressionResults(DimIdx dim, ViewGetSpec view, FrameValueMap& map) const;

    T getValueFromMasterAt(double time, ViewGetSpec view, DimIdx dimension, const KnobIPtr& master);
    T getValueFromMaster(ViewGetSpec view, DimIdx dimension, const KnobIPtr& master, bool clamp);

    bool getValueFromCurve(double time, ViewGetSpec view, DimIdx dimension, bool byPassMaster, bool clamp, T* ret);

    virtual bool hasDefaultValueChanged(DimIdx dimension) const OVERRIDE FINAL;


    virtual void splitView(ViewIdx view) OVERRIDE;

    virtual void unSplitView(ViewIdx view) OVERRIDE;

protected:


    virtual void resetExtraToDefaultValue(DimSpec /*dimension*/, ViewSetSpec /*view*/) {}

    virtual bool checkIfValueChanged(const T& a, DimIdx dimension, ViewIdx view) const;


private:

    void setValueOnCurveInternal(double time, const T& v, DimIdx dimension, ViewIdx view, KeyFrame* newKey, ValueChangedReturnCodeEnum* ret);

    void addSetValueToUndoRedoStackIfNeeded(const T& value, ValueChangedReasonEnum reason, ValueChangedReturnCodeEnum setValueRetCode, ViewSetSpec view, DimSpec dimension, double time, bool setKeyFrame);

    virtual void copyValuesFromCurve(DimSpec dim, ViewSetSpec view) OVERRIDE FINAL;

    void initMinMax();

    T clampToMinMax(const T& value, DimIdx dimension) const;


    template <typename OTHERTYPE>
    bool copyValueForType(const boost::shared_ptr<Knob<OTHERTYPE> >& other, ViewIdx view, ViewIdx otherView, DimIdx dimension, DimIdx otherDimension);

    bool cloneValues(const KnobIPtr& other, ViewSetSpec view, ViewSetSpec otherView, DimSpec dimension, DimSpec otherDimension);

    virtual void cloneExpressionsResults(const KnobIPtr& other,
                                         ViewSetSpec view,
                                         ViewSetSpec otherView,
                                         DimSpec dimension,
                                         DimSpec otherDimension) OVERRIDE FINAL;

    void valueToVariant(const T & v, Variant* vari);


    void makeKeyFrame(double time, const T& v, ViewIdx view, KeyFrame* key);

    void queueSetValue(const T& v, ViewSetSpec view, DimSpec dimension);

    virtual void clearExpressionsResults(DimSpec dimension, ViewSetSpec view) OVERRIDE FINAL;

    bool evaluateExpression(double time, ViewIdx view, DimIdx dimension, T* ret, std::string* error);

    /*
     * @brief Same as evaluateExpression but expects it to return a PoD
     */
    bool evaluateExpression_pod(double time, ViewIdx view, DimIdx dimension, double* value, std::string* error);


    bool getValueFromExpression(double time, ViewGetSpec view, DimIdx dimension, bool clamp, T* ret);

    bool getValueFromExpression_pod(double time, ViewGetSpec view, DimIdx dimension, bool clamp, double* ret);

    //////////////////////////////////////////////////////////////////////
    /////////////////////////////////// End implementation of KnobI
    //////////////////////////////////////////////////////////////////////


    ///Here is all the stuff we couldn't get rid of the template parameter
    mutable QMutex _valueMutex; //< protects _values & _guiValues & _defaultValues & ExprResults

    PerDimensionValuesVec _values;

    struct DefaultValue
    {
        T initialValue,value;
        bool defaultValueSet;
    };
    std::vector<DefaultValue> _defaultValues;
    mutable PerDimensionFrameValueMap _exprRes;

    //Only for double and int
    mutable QReadWriteLock _minMaxMutex;
    std::vector<T>  _minimums, _maximums, _displayMins, _displayMaxs;

};



typedef Knob<bool> KnobBoolBase;
typedef Knob<double> KnobDoubleBase;
typedef Knob<int> KnobIntBase;
typedef Knob<std::string> KnobStringBase;

typedef boost::shared_ptr<KnobBoolBase> KnobBoolBasePtr;
typedef boost::shared_ptr<KnobDoubleBase> KnobDoubleBasePtr;
typedef boost::shared_ptr<KnobIntBase> KnobIntBasePtr;
typedef boost::shared_ptr<KnobStringBase> KnobStringBasePtr;



class AnimatingKnobStringHelper
    : public KnobStringBase
{
public:

    AnimatingKnobStringHelper(const KnobHolderPtr& holder,
                              const std::string &label,
                              int nDims,
                              bool declaredByPlugin = true);

    virtual ~AnimatingKnobStringHelper();

    void stringToKeyFrameValue(double time, ViewIdx view, const std::string & v, double* returnValue);

    //for integration of openfx custom params
    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle inArgsRaw,
                                                           OfxPropertySetHandle outArgsRaw);


    virtual StringAnimationManagerPtr getStringAnimation() const OVERRIDE FINAL
    {
        return _animation;
    }

    void loadAnimation(const std::map<std::string,std::map<double, std::string> > & keyframes);

    void saveAnimation(std::map<std::string,std::map<double, std::string> >* keyframes) const;

    void setCustomInterpolation(customParamInterpolationV1Entry_t func, void* ofxParamHandle);

    void stringFromInterpolatedValue(double interpolated, ViewGetSpec view, std::string* returnValue) const;

    std::string getStringAtTime(double time, ViewGetSpec view);

    virtual void splitView(ViewIdx view) OVERRIDE;

    virtual void unSplitView(ViewIdx view) OVERRIDE;

protected:



    virtual bool cloneExtraData(const KnobIPtr& other,
                                ViewSetSpec view,
                                ViewSetSpec otherView,
                                DimSpec dimension,
                                DimSpec otherDimension,
                                double offset,
                                const RangeD* range) OVERRIDE;
    virtual void onKeyframesRemoved(const std::list<double>& keysRemoved,
                                    ViewSetSpec view,
                                    DimSpec dimension) OVERRIDE FINAL;
    virtual void populate() OVERRIDE;
private:
    

    StringAnimationManagerPtr _animation;
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
    , public boost::enable_shared_from_this<KnobHolder>
    , public HashableObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    friend class KnobHelper;
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

protected: // parent of NamedKnobHolder, Project, Settings
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    /**
     *@brief A holder is a class managing a bunch of knobs and interacting with an appInstance.
     * When appInstance is NULL the holder will be considered "application global" in which case
     * the knob holder will interact directly with the AppManager singleton.
     **/
    KnobHolder(const AppInstancePtr& appInstance);

    KnobHolder(const KnobHolder& other);

public:

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
    void deleteKnob(const KnobIPtr& knob, bool alsoDeleteGui);


    /**
     * @brief Flag that the overlays should be redrawn when this knob changes.
     **/
    void addOverlaySlaveParam(const KnobIPtr& knob);

    bool isOverlaySlaveParam(const KnobIConstPtr& knob) const;

    virtual void redrawOverlayInteract();
    

    //To re-arrange user knobs only, does nothing if knob->isUserKnob() returns false
    bool moveKnobOneStepUp(const KnobIPtr& knob);
    bool moveKnobOneStepDown(const KnobIPtr& knob);


    template<typename K>
    boost::shared_ptr<K> createKnob(const std::string &label, int nDims = 1) const WARN_UNUSED_RETURN;
    AppInstancePtr getApp() const WARN_UNUSED_RETURN;
    KnobIPtr getKnobByName(const std::string & name) const WARN_UNUSED_RETURN;
    KnobIPtr getOtherKnobByName(const std::string & name, const KnobIConstPtr& caller) const WARN_UNUSED_RETURN;

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

    const std::vector< KnobIPtr > & getKnobs() const WARN_UNUSED_RETURN;
    std::vector< KnobIPtr >  getKnobs_mt_safe() const WARN_UNUSED_RETURN;

    void refreshAfterTimeChange(bool isPlayback, double time);

    /**
     * @brief Same as refreshAfterTimeChange but refreshes only the knobs
     * whose function evaluateValueChangeOnTimeChange() return true so that
     * after a playback their state is correct
     **/
    void refreshAfterTimeChangeOnlyKnobsWithTimeEvaluation(double time);

    void setIsInitializingKnobs(bool b);
    bool isInitializingKnobs() const;

    void setViewerUIKnobs(const KnobsVec& knobs);
    void addKnobToViewerUI(const KnobIPtr& knob);
    void insertKnobToViewerUI(const KnobIPtr& knob, int index);
    void removeKnobViewerUI(const KnobIPtr& knob);
    bool moveViewerUIKnobOneStepUp(const KnobIPtr& knob);
    bool moveViewerUIOneStepDown(const KnobIPtr& knob);

    int getInViewerContextKnobIndex(const KnobIConstPtr& knob) const;
    KnobsVec getViewerUIKnobs() const;

protected:

    virtual void refreshExtraStateAfterTimeChanged(bool /*isPlayback*/,
                                                   double /*time*/) {}


public:

    /**
     * @brief Calls beginChanges() and flags that all subsequent calls to setValue/setValueAtTime should be 
     * gathered under the same undo/redo action in the GUI. To stop actions to be in this undo/redo action
     * call endMultipleEdits(). 
     * Note that each beginMultipleEdits must have a corresponding call to endMultipleEdits. No render will be
     * issued until endMultipleEdits is called. 
     * Note that however, the knobChanged handler IS called at each setValue/setValueAtTime call made in-between
     * the begin/endMultipleEdits.
     * If beginMultipleEdits is called while another call to beginMultipleEdits is on, then it will create a new
     * undo/redo action that will be different from the first one. Note that when calling recursively
     * begin/end, the render is issued once the last endMultipleEdits function is called (i.e: when recursion reaches its toplevel).
     * @param commandName The name of the command that should appear under the Edit menu of the application.
     * Note: Undo/Redo only works if knob have a gui, otherwise this function does nothing more than beginChanges().
     **/
    void beginMultipleEdits(const std::string& commandName);

    /**
     * @brief When in-between beginMultipleEdits/endMultipleEdits, returns whether a knob should create a new command
     * or append it to an existing one.
     **/
    KnobHolder::MultipleParamsEditEnum getMultipleEditsLevel() const;

    /**
     * @brief Returns the name of the current command during a beginMultipleEdits/endMultipleEdits bracket.
     * If not in-between the 2 functions called, the returned string is empty.
     **/
    std::string getCurrentMultipleEditsCommandName() const;

    /**
     * @brief To be called when changes applied after a beginMultipleEdits group are finished. If this call
     * to endMultipleEdits matches the last beginMultipleEdits function call in the recursion stack then 
     * if a significant knob was modified, a render is triggered.
     **/
    void endMultipleEdits();


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
    KnobPagePtr getOrCreateUserPageKnob();
    KnobPagePtr getUserPageKnob() const;

    /**
     * @brief These functions below are dynamic in a sense that they can be called at any time (on the main-thread)
     * to create knobs on the fly. Their gui will be properly created. In order to notify the GUI that new parameters were
     * created, you must call refreshKnobs() that will re-scan for new parameters
     **/
    KnobIntPtr createIntKnob(const std::string& name, const std::string& label, int nDims, bool userKnob = true);
    KnobDoublePtr createDoubleKnob(const std::string& name, const std::string& label, int nDims, bool userKnob = true);
    KnobColorPtr createColorKnob(const std::string& name, const std::string& label, int nDims, bool userKnob = true);
    KnobBoolPtr createBoolKnob(const std::string& name, const std::string& label, bool userKnob = true);
    KnobChoicePtr createChoiceKnob(const std::string& name, const std::string& label, bool userKnob = true);
    KnobButtonPtr createButtonKnob(const std::string& name, const std::string& label, bool userKnob = true);
    KnobSeparatorPtr createSeparatorKnob(const std::string& name, const std::string& label, bool userKnob = true);

    //Type corresponds to the Type enum defined in StringParamBase in Parameter.h
    KnobStringPtr createStringKnob(const std::string& name, const std::string& label, bool userKnob = true);
    KnobFilePtr createFileKnob(const std::string& name, const std::string& label, bool userKnob = true);
    KnobPathPtr createPathKnob(const std::string& name, const std::string& label, bool userKnob = true);
    KnobPagePtr createPageKnob(const std::string& name, const std::string& label, bool userKnob = true);
    KnobGroupPtr createGroupKnob(const std::string& name, const std::string& label, bool userKnob = true);
    KnobParametricPtr createParametricKnob(const std::string& name, const std::string& label, int nbCurves, bool userKnob = true);
    /**
     * @brief Returns whether the onKnobValueChanged can be called by a separate thread
     **/
    virtual bool canHandleEvaluateOnChangeInOtherThread() const { return false; }

    virtual bool isDoingInteractAction() const { return false; }

    bool isEvaluationBlocked() const;

    void appendValueChange(const KnobIPtr& knob,
                           DimSpec dimension,
                           double time,
                           ViewSetSpec view,
                           ValueChangedReasonEnum originalReason,
                           ValueChangedReasonEnum reason);

    void getAllExpressionDependenciesRecursive(std::set<NodePtr >& nodes) const;

    /**
     * @brief To implement if you need to make the hash vary at a specific time/view
     **/

    virtual void appendToHash(double time, ViewIdx view, Hash64* hash) OVERRIDE;


    /**
     * @brief Set the holder to have the given table to display in its settings panel.
     * The KnobHolder takes ownership of the table.
     * @param paramScriptNameBefore The script-name of the knob right before where the table should be inserted in the layout or the script-name of a page.
     **/
    void setItemsTable(const KnobItemsTablePtr& table, const std::string& paramScriptNameBefore);
    KnobItemsTablePtr getItemsTable() const;
    std::string getItemsTablePreviousKnobScriptName() const;

protected:

    void onUserKnobCreated(const KnobIPtr& knob, bool isUserKnob);

    //////////////////////////////////////////////////////////////////////////////////////////



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
     * @brief Should be called in the begining of an action.
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
     * @brief Use this to bracket setValue() calls, this will actually trigger the evaluate() (i.e: render) function
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
    void beginKnobsValuesChanged_public(ValueChangedReasonEnum reason);

    /**
     * @brief The virtual portion of notifyProjectEndKnobsValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to finish a serie of value changes, thus limiting the amount of changes to do.
     **/
    void endKnobsValuesChanged_public(ValueChangedReasonEnum reason);

    /**
     * @brief The virtual portion of notifyProjectEvaluationRequested(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to do things when a value is changed. Bear in mind that you can compress
     * the change by using the begin/end[ValueChanges] to optimize the changes.
     **/
    virtual bool onKnobValueChanged_public(const KnobIPtr& k, ValueChangedReasonEnum reason, double time, ViewSetSpec view, bool originatedFromMainThread);


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
    void removeKnobFromList(const KnobIConstPtr& knob);


    void initializeKnobsPublic();

    bool isSlave() const;

    ///Slave all the knobs of this holder to the other holder.
    void slaveAllKnobs(const KnobHolderPtr& other);

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

    int getPageIndex(const KnobPagePtr page) const;


    //Calls onSignificantEvaluateAboutToBeCalled + evaluate
    void invalidateCacheHashAndEvaluate(bool isSignificant, bool refreshMetadatas);


    /**
     * @brief Return a list of all the internal pages which were created by the user
     **/
    void getUserPages(std::list<KnobPagePtr>& userPages) const;
    
protected:


    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    virtual void evaluate(bool /*isSignificant*/,
                          bool /*refreshMetadatas*/) {}

    /**
     * @brief The virtual portion of notifyProjectBeginValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to prepare yourself to a lot of value changes.
     **/
    virtual void beginKnobsValuesChanged(ValueChangedReasonEnum reason)
    {
        Q_UNUSED(reason);
    }

    /**
     * @brief The virtual portion of notifyProjectEndKnobsValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to finish a serie of value changes, thus limiting the amount of changes to do.
     **/
    virtual void endKnobsValuesChanged(ValueChangedReasonEnum reason)
    {
        Q_UNUSED(reason);
    }

    /**
     * @brief The virtual portion of notifyProjectEvaluationRequested(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to do things when a value is changed. Bear in mind that you can compress
     * the change by using the begin/end[ValueChanges] to optimize the changes.
     **/
    virtual bool onKnobValueChanged(const KnobIPtr& /*k*/,
                                    ValueChangedReasonEnum /*reason*/,
                                    double /*time*/,
                                    ViewSetSpec /*view*/,
                                    bool /*originatedFromMainThread*/)
    {
        return false;
    }

    virtual void onSignificantEvaluateAboutToBeCalled(const KnobIPtr& /*knob*/, ValueChangedReasonEnum /*reason*/, DimSpec /*dimension*/, double /*time*/, ViewSetSpec /*view*/) {}

    /**
     * @brief Called when the knobHolder is made slave or unslaved.
     * @param master The master knobHolder.
     * @param isSlave Whether this KnobHolder is now slaved or not.
     * If false, master will be NULL.
     **/
    virtual void onAllKnobsSlaved(bool /*isSlave*/,
                                  const KnobHolderPtr& /*master*/)
    {
    }

public:

    /**
     * @brief Same as onAllKnobsSlaved but called when only 1 knob is slaved
     **/
    virtual void onKnobSlaved(const KnobIPtr& /*slave*/,
                              const KnobIPtr& /*master*/,
                              DimIdx /*dimension*/,
                              ViewIdx /*view*/,
                              bool /*isSlave*/)
    {
    }

public Q_SLOTS:

    void onDoEndChangesOnMainThreadTriggered();

    void onDoEvaluateOnMainThread(bool significant, bool refreshMetadata);

    void onDoValueChangeOnMainThread(const KnobIPtr& knob, ValueChangedReasonEnum reason, double time, ViewSetSpec view, bool originatedFromMT);

    void onDoBeginKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum);

    void onDoEndKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum);

Q_SIGNALS:

    void doBeginKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum);

    void doEndKnobsValuesChangedActionOnMainThread(ValueChangedReasonEnum);

    void doEndChangesOnMainThread();

    void doEvaluateOnMainThread(bool significant, bool refreshMetadata);

    void doValueChangeOnMainThread(const KnobIPtr& knob, ValueChangedReasonEnum reason, double time, ViewSetSpec view, bool originatedFromMT);

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
    KnobHolderPtr effect;

public:

    RecursionLevelRAII(const KnobHolderPtr& effect,
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
#define MANAGE_RECURSION(assertNonRecursive) RecursionLevelRAII actionRecursionManager(shared_from_this(), assertNonRecursive)

/**
 * @brief Should be called in the begining of any action that cannot be called recursively.
 **/
#define NON_RECURSIVE_ACTION() MANAGE_RECURSION(true)

/**
 * @brief Should be called in the begining of any action that can be called recursively
 **/
#define RECURSIVE_ACTION() MANAGE_RECURSION(false)


class InitializeKnobsFlag_RAII
{
    KnobHolderPtr _h;

public:

    InitializeKnobsFlag_RAII(const KnobHolderPtr& h)
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
protected: // derives from KnobHolder, parent of EffectInstance, TrackMarker, DialogParamHolder
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    NamedKnobHolder(const AppInstancePtr& instance)
        : KnobHolder(instance)
    {
    }

    NamedKnobHolder(const NamedKnobHolder& other)
    : KnobHolder(other)
    {
    }

public:
    virtual ~NamedKnobHolder()
    {
    }

    virtual std::string getScriptName_mt_safe() const = 0;
};


template<typename K>
boost::shared_ptr<K> KnobHolder::createKnob(const std::string &label,
                                            int nDims) const
{
    return AppManager::createKnob<K>(shared_from_this(), label, nDims);
}

NATRON_NAMESPACE_EXIT;

#endif // Engine_Knob_h
