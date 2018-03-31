/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <boost/scoped_ptr.hpp>
#endif

#include <QtCore/QReadWriteLock>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>

#include "Engine/AnimatingObjectI.h"
#include "Engine/AppManager.h" // for AppManager::createKnob
#include "Engine/Cache.h" // CacheEntryLockerPtr - could we put this in EngineFwd.h?
#include "Engine/DimensionIdx.h"
#include "Engine/HashableObject.h"
#include "Engine/KnobFactory.h"
#include "Engine/Variant.h"
#include "Engine/ViewIdx.h"

#include "Serialization/SerializationBase.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#define NATRON_USER_MANAGED_KNOBS_PAGE_LABEL "User"
#define NATRON_USER_MANAGED_KNOBS_PAGE "userNatron"


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

    void s_curveAnimationChanged(ViewSetSpec view, DimSpec dimension)
    {
        Q_EMIT curveAnimationChanged(view,  dimension);
    }


    void s_selectedMultipleTimes(bool b)
    {
        Q_EMIT selectedMultipleTimes(b);
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

    void s_dimensionsVisibilityChanged(ViewSetSpec view)
    {
        Q_EMIT dimensionsVisibilityChanged(view);
    }

    void s_linkChanged()
    {
        Q_EMIT linkChanged();
    }
Q_SIGNALS:

    // Called whenever the evaluateOnChange property changed
    void evaluateOnChangeChanged(bool value);

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
    void curveAnimationChanged(ViewSetSpec view, DimSpec dimension);

    void selectedMultipleTimes(bool);

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

    // Called when all dimensions are folded/unfolded
    void dimensionsVisibilityChanged(ViewSetSpec views);

    // Called when the links of the knob changed (either hard-link or expression)
    void linkChanged();
};



// A small knob/dimension/view tuple that identifies the owner
// of a KnobDimViewBase
struct KnobDimViewKey
{
    KnobIWPtr knob;
    DimIdx dimension;
    ViewIdx view;

    KnobDimViewKey()
    : knob()
    , dimension()
    , view()
    {

    }

    KnobDimViewKey(const KnobIPtr& knob, DimIdx dimension, ViewIdx view)
    : knob(knob)
    , dimension(dimension)
    , view(view)
    {

    }
};

struct KnobDimViewKey_Compare
{
    bool operator() (const KnobDimViewKey& lhs, const KnobDimViewKey& rhs) const
    {
        KnobIPtr lhsKnob = lhs.knob.lock();
        KnobIPtr rhsKnob = rhs.knob.lock();
        if (lhsKnob.get() < rhsKnob.get()) {
            return true;
        } else if (lhsKnob.get() > rhsKnob.get()) {
            return false;
        } else {
            if (lhs.dimension < rhs.dimension) {
                return true;
            } else if (lhs.dimension > rhs.dimension) {
                return false;
            } else {
                return lhs.view < rhs.view;
            }
        }
    }
};

typedef std::set<KnobDimViewKey, KnobDimViewKey_Compare> KnobDimViewKeySet;

class KnobI
    : public AnimatingObjectI
    , public boost::enable_shared_from_this<KnobI>
    , public SERIALIZATION_NAMESPACE::SerializableObjectBase
    , public HashableObject
{
    friend class KnobHolder;

protected: // parent of KnobHelper
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobI();

public:

    // dtor
    virtual ~KnobI();

protected:
    /**
     * @brief Deletes this knob permanently
     **/
    virtual void deleteKnob() = 0;

public:

   

    /**
     * @brief Do not call this. It is called right away after the constructor by the factory
     * to initialize curves and values. This is separated from the constructor as we need RTTI
     * for Curve.
     **/
    virtual void populate() = 0;

    /**
     * @brief Return whether all dimensions are folded or not, when folded all dimensions have the same value/animation
     **/
    virtual bool getAllDimensionsVisible(ViewIdx view) const = 0;

    /**
     * @brief Set whether all dimensions are folded or not, when folded all dimensions have the same value/animation
     **/
    virtual void setAllDimensionsVisible(ViewSetSpec view, bool visible) = 0;

    /**
     * @brief When enabled, the knob can be automatically folded if 2 dimensions have equal values.
     **/
    virtual void setCanAutoFoldDimensions(bool enabled) = 0;
    virtual bool isAutoFoldDimensionsEnabled() const = 0;

    /**
     * @brief When enabled, the knob can be automatically expanded folded dimensions
     **/
    virtual void setAdjustFoldExpandStateAutomatically(bool enabled) = 0;
    virtual bool isAdjustFoldExpandStateAutomaticallyEnabled() const = 0;


    /**
     * @brief Check the knob state across all dimensions and if they have the same values
     * it folds the dimensions.
     **/
    virtual void autoFoldDimensions(ViewIdx view) = 0;

    /**
     * @brief Returns whether 2 dimensions are equal.
     * It checks their expression, animation curve and value.
     * This can be potentially expensive as it compares all animation if needed.
     **/
    virtual bool areDimensionsEqual(ViewIdx view) = 0;

    /**
     * @brief Calls areDimensionsEqual to figure out if all dimensions should be made visible or not.
     **/
    virtual void autoAdjustFoldExpandDimensions(ViewIdx view) = 0;

    /**
     * @brief Given the dimension and view in input, if the knob has all its dimensions folded or is not multi-view,
     * adjust the arguments so that they best fit PasteKnobClipBoardUndoCommand
     **/
    virtual void convertDimViewArgAccordingToKnobState(DimSpec dimIn, ViewSetSpec viewIn, DimSpec* dimOut, ViewSetSpec* viewOut) const = 0;

    enum KnobDeclarationTypeEnum
    {
        // The knob was described by the plug-in. No extended properties such as min/max/default values etc... will
        // be serialized.
        // This is the default.
        eKnobDeclarationTypePlugin,

        // The knob is described by the host, it is the same than eKnobDeclarationTypePlugin except that:
        // - It will not show up in the documentation
        // - The knobChanged action will not be called on the plug-in for this parameter since it is unknown by the plug-in.
        eKnobDeclarationTypeHost,

        // The knob was created by the user. The serialization will include extended properties such as min/max/default values etc...
        // This is used for example by user parameters when describing a PyPlug.
        eKnobDeclarationTypeUser
    };

    /**
     * @brief Returns the knob was created by a plugin or added automatically by Natron (e.g like mask knobs)
     **/
    virtual KnobDeclarationTypeEnum getKnobDeclarationType() const = 0;
    virtual void setKnobDeclarationType(KnobDeclarationTypeEnum b) = 0;


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

    /**
     * @brief Return true if the default value has changed.
     **/
    bool hasDefaultValueChanged() const;
    virtual bool hasDefaultValueChanged(DimIdx dimension) const = 0;

    /**
     * @brief If the parameter is multidimensional, this is the label thats the that will be displayed
     * for a dimension.
     **/
    virtual std::string getDimensionName(DimIdx dimension) const = 0;
    virtual void setDimensionName(DimIdx dim, const std::string & name) = 0;
    
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

    // Prevent autokeying
    virtual void setAutoKeyingEnabled(bool enabled) = 0;

    /**
     * @brief Returns the value previously set by setAutoKeyingEnabled
     **/
    virtual bool isAutoKeyingEnabled(DimSpec dimension, TimeValue time, ViewSetSpec view, ValueChangedReasonEnum reason) const = 0;

    /**
     * @brief Called by setValue to refresh the GUI, call the instanceChanged action on the plugin and
     * evaluate the new value (cause a render).
     * @returns true if the knobChanged handler was called once for this knob
     **/
    virtual bool evaluateValueChange(DimSpec dimension, TimeValue time, ViewSetSpec view, ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Copies the value and animation of the knob "other" to this knob.
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

    virtual double random(TimeValue time, unsigned int seed) const = 0;
    virtual double random(double min = 0., double max = 1.) const = 0;
    virtual int randomInt(TimeValue time, unsigned int seed) const = 0;
    virtual int randomInt(int min = INT_MIN, int max = INT_MAX) const = 0;


    /**
     * @brief Same as getRawCurveValueAt, but first check if an expression is present. The expression should return a PoD.
     **/
    virtual double getValueAtWithExpression(TimeValue time, ViewIdx view, DimIdx dimension) = 0;

    /**
     * @brief Set an expression on the knob. If this expression is invalid, this function throws an excecption with the error from the
     * Python interpreter or exprTK compiler
     * @param Language (Python or ExprTk)
     * @param hasRetVariable (Python only) If true the expression is expected to be multi-line and have its return value set to the variable "ret", otherwise
     * the expression is expected to be single-line.
     * @param force If set to true, this function will not check if the expression is valid nor will it attempt to compile/evaluate it, it will
     * just store it. This flag is used for serialisation, you should always pass false
     **/

protected:

    virtual void setExpressionCommon(DimSpec dimension, ViewSetSpec view, const std::string& expression, ExpressionLanguageEnum language, bool hasRetVariable, bool failIfInvalid) = 0;

public:

    void restoreExpression(DimSpec dimension,
                           ViewSetSpec view,
                           const std::string& expression,
                           ExpressionLanguageEnum language,
                           bool hasRetVariable)
    {
        setExpressionCommon(dimension, view, expression, language, hasRetVariable, false);
    }

    void setExpression(DimSpec dimension,
                       ViewSetSpec view,
                       const std::string& expression,
                       ExpressionLanguageEnum language,
                       bool hasRetVariable,
                       bool failIfInvalid)
    {
        setExpressionCommon(dimension, view, expression, language, hasRetVariable, failIfInvalid);
    }

    /**
     * @brief Restores all links for the given knob if it has masters or expressions.
     * This function cannot be called until all knobs of the node group have been created because it needs to reference other knobs
     * from other nodes.
     * This function throws an exception if no serialization is valid in the object
     **/
    virtual void restoreKnobLinks(const SERIALIZATION_NAMESPACE::KnobSerializationBasePtr& serialization,
                                  const std::map<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr>& allCreatedNodesInGroup) = 0;

    /**
     * @brief Tries to re-apply invalid expressions, returns true if they are all valid
     **/
    virtual bool checkInvalidLinks() = 0;
    virtual bool isLinkValid(DimIdx dimension, ViewIdx view, std::string* error) const = 0;
    virtual void setLinkStatus(DimSpec dimension, ViewSetSpec view, bool valid, const std::string& error) = 0;


    /**
     * @brief For each dimension, try to find in the expression, if set, the node name "oldName" and replace
     * it by "newName"
     **/
    virtual void replaceNodeNameInExpression(DimSpec dimension,
                                             ViewSetSpec view,
                                             const std::string& oldName,
                                             const std::string& newName) = 0;
    virtual void clearExpression(DimSpec dimension, ViewSetSpec view) = 0;
    virtual std::string getExpression(DimIdx dimension, ViewIdx view) const = 0;

    virtual bool hasExpression(DimIdx dimension, ViewIdx view) const = 0;

    bool hasAnyExpression() const;

    virtual void clearExpressionsResults(DimSpec dimension, ViewSetSpec view) = 0;

    /**
     * @brief When enabled, results of expressions are cached. By default this is enabled.
     * This can be turned off in case the expression depends on external stuff that the caching
     * system cannot detect.
     **/
    virtual void setExpressionsResultsCachingEnabled(bool enabled) = 0;
    virtual bool isExpressionsResultsCachingEnabled() const = 0;

    /**
     * @brief Checks that the given expr for the given dimension will produce a correct behaviour.
     * On success this function returns correctly, otherwise an exception is thrown with the error.
     * This function also declares some extra python variables via the declareCurrentKnobVariable_Python function.
     * The new expression is returned.
     * @param resultAsString[out] The result of the execution of the expression will be written to the string.
     * @returns A new string containing the modified expression with the 'ret' variable declared if it wasn't already declared
     * by the user.
     **/
    virtual void validateExpression(const std::string& expression, ExpressionLanguageEnum language, DimIdx dimension, ViewIdx view, bool hasRetVariable, std::string* resultAsString) = 0;

public:

    /**
     * @brief Returns whether the expr at the given dimension uses the ret variable to assign to the return value or not
     **/
    virtual bool isExpressionUsingRetVariable(ViewIdx view, DimIdx dimension) const = 0;

    /**
     * @brief Returns the language used by an expression
     **/
    virtual ExpressionLanguageEnum getExpressionLanguage(ViewIdx view, DimIdx dimension) const = 0;

    /**
     * @brief Returns in dependencies a list of all the knobs used in the expression at the given dimension
     * @returns True on sucess, false if no expression is set.
     **/
    virtual bool getExpressionDependencies(DimIdx dimension, ViewIdx view, KnobDimViewKeySet& dependencies) const = 0;

    /**
     * @brief Called when the current time of the timeline changes.
     * It must get the value at the given time and notify  the gui it must
     * update the value displayed.
     **/
    virtual void onTimeChanged(bool isPlayback, TimeValue time) = 0;

    /**
     * @brief Returns the keyframe object at the given time. This by-passes any expression, but not hard-links
     **/
    virtual bool getCurveKeyFrame(TimeValue time, DimIdx dimension, ViewIdx view, bool clampToMinMax, KeyFrame* key) = 0;


    /**
     * @brief Compute the derivative at time as a double
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual double getDerivativeAtTime(TimeValue time, ViewIdx view, DimIdx dimension) = 0;

    /**
     * @brief Compute the integral of dimension from time1 to time2 as a double
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual double getIntegrateFromTimeToTime(TimeValue time1, TimeValue time2, ViewIdx view, DimIdx dimension)  = 0;


    /**
     * @brief Returns true if the curve corresponding to the dimension/view is animated with keyframes.
     * @param dimension The dimension index of the corresponding curve
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index. 
     **/
    virtual bool isAnimated( DimIdx dimension, ViewIdx view ) const = 0;

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
     * @brief When enabled, if the knob has a gui, whenever it will be visible eithre in the settings panel or in
     * the viewer gui its keyframes will
     * also be displayed on the timeline.
     * By default this is enabled.
     **/
    virtual void setKeyFrameTrackingEnabled(bool enabled) = 0;
    virtual bool isKeyFrameTrackingEnabled() const = 0;

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

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash) OVERRIDE = 0;

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
    virtual void setEnabled(bool b) = 0;

    /**
     * @brief Is the parameter enabled ?
     **/
    virtual bool isEnabled() const = 0;

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
     * @brief For a master knob in a KnobItemsTable, it indicates to the gui that multiple
     * items in the table are selected and the knob should be displayed with a specific state
     * to reflect the user that modifying the value will impact all selected rows of the 
     * KnobItemsTable.
     **/
    virtual void setKnobSelectedMultipleTimes(bool d) = 0;

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
    virtual void setCustomInteract(const OverlayInteractBasePtr & interactDesc) = 0;
    virtual OverlayInteractBasePtr getCustomInteract() const = 0;

    /**
     * @brief Returns whether any interact associated to this knob should be drawn or not
     **/
    bool shouldDrawOverlayInteract() const;

    /**
     * @brief Returns the current time if attached to a timeline or the time being rendered
     **/
    virtual TimeValue getCurrentRenderTime() const = 0;

    virtual boost::shared_ptr<KnobSignalSlotHandler> getSignalSlotHandler() const = 0;


    /**
     * @brief Adds a new listener to this knob. This is just a pure notification about the fact that the given knob
     * is now listening to the values/keyframes of "this" either via a hard-link (slave/master) or via its expressions
     * @param isExpression Is this listener listening through expressions or via a slave/master link
     * @param listenerDimension The dimension of the listener that is listening to this knob
     * @param listenedDimension The dimension of this knob that is listened to by the listener
     **/
    virtual void addListener(const DimIdx listenerDimension,
                             const DimIdx listenedToDimension,
                             const ViewIdx listenerView,
                             const ViewIdx listenedToView,
                             const KnobIPtr& listener,
                             ExpressionLanguageEnum language) = 0;

    /**
     * @brief Implement to save the content of the object to the serialization object
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE = 0;

    /**
     * @brief Implement to load the content of the serialization object onto this object
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase&  serializationBase) OVERRIDE = 0;

    virtual void restoreValueFromSerialization(const SERIALIZATION_NAMESPACE::ValueSerialization& obj, DimIdx targetDimension, ViewIdx view) = 0;

    virtual void restoreDefaultValueFromSerialization(const SERIALIZATION_NAMESPACE::DefaultValueSerialization& defObj,
                                                      bool applyDefaultValue,
                                                      DimIdx targetDimension) = 0;

    virtual void clearRenderValuesCache() = 0;

    template <typename K>
    boost::shared_ptr<K> getCloneForHolder(const KnobHolderPtr& holder) const
    {
        KnobIPtr k = getCloneForHolderInternal(holder);
        if (!k) {
            return boost::shared_ptr<K>();
        }
        return boost::dynamic_pointer_cast<K>(k);
    }

    virtual KnobIPtr getCloneForHolderInternal(const KnobHolderPtr& holder) const = 0;


    /**
     * @brief Hack for the meta Read/Write nodes
     **/
    virtual void setActualCloneForHolder(const KnobHolderPtr& holder) = 0;

    /**
     * @brief Returns the main-instance knob if this is a render  clone, otherwise NULL
     **/
    virtual KnobIPtr getMainInstance() const = 0;

protected:


protected:

    virtual bool setHasModifications(DimIdx dimension, ViewIdx view, bool value, bool lock) = 0;
    
public:



    /**
     * @brief The value given by thisDimension and thisView will point to
     * the values of the otherKnob of the given otherDimension and otherView.
     * @param otherKnob The knob to point to
     * @param thisDimension If set to all, each dimension will be respectively redirected to the other knob
     * assuming they have a matching dimension in the other knob.(e.g: 0 to 0, 1 to 1, etc...)
     * @param thisView If set to all, then all views
     * will be redirected, it is then expected that view == otherView == ViewSpec::all(). If not set to all valid view index should be given
     * @return True on success, false otherwise
     * Note that if this parameter shares already a value of another parameter this function will fail, you must call removeRedirection first.
     **/
    virtual bool linkTo(const KnobIPtr & otherKnob, DimSpec thisDimension = DimSpec::all(), DimSpec otherDimension = DimSpec::all(), ViewSetSpec thisView = ViewSetSpec::all(), ViewSetSpec otherView = ViewSetSpec::all()) = 0;

    /**
     * @brief Removes any link for the given dimension(s)/view(s): after this call the values will no longer be shared with any other knob.
     * @param copyState If true, if the knob was redirected to another knob then the knob will copy the state of the knob to which it is pointing to before removing the link, otherwise
     * it will revert to the state it had prior to linking.
     **/
    virtual void unlink(DimSpec dimension, ViewSetSpec view, bool copyState) = 0;


    enum DuplicateKnobTypeEnum
    {
        // The new knob will act as an alias of this parameter
        eDuplicateKnobTypeAlias,

        // The new knob will be expression linked
        eDuplicateKnobTypeExprLinked,

        // The new knob will just be a copy of this knob
        // but will not be linked
        eDuplicateKnobTypeCopy
    };

    virtual KnobIPtr createDuplicateOnHolder(const KnobHolderPtr& otherHolder,
                                            const KnobPagePtr& page,
                                            const KnobGroupPtr& group,
                                            int indexInParent,
                                            DuplicateKnobTypeEnum duplicateType,
                                            const std::string& newScriptName,
                                            const std::string& newLabel,
                                            const std::string& newToolTip,
                                            bool refreshParams,
                                            KnobI::KnobDeclarationTypeEnum isUserKnob) = 0;


    enum ListenersTypeEnum
    {
        eListenersTypeAll = 0x0,
        eListenersTypeExpression = 0x1,
        eListenersTypeSharedValue = 0x2
    };
    
    DECLARE_FLAGS(ListenersTypeFlags, ListenersTypeEnum)
    
    /**
     * @brief Returns a list of all the knobs whose value depends upon this knob.
     **/
    virtual void getListeners(KnobDimViewKeySet &listeners, ListenersTypeFlags flags = ListenersTypeFlags(eListenersTypeAll)) const = 0;

    /**
     * @brief If the value is shared for the given dimension/view, returns the original owner of the value.
     * If this knob is the original owner, this returns false, otherwise it returns true.
     **/
    virtual bool getSharingMaster(DimIdx dimension, ViewIdx view, KnobDimViewKey* linkData) const = 0;

    /**
     * @brief Return a set of all knob/dim/view tuples that share the same value that the given dimension/view of
     * this knob.
     **/
    virtual void getSharedValues(DimIdx dimension, ViewIdx view, KnobDimViewKeySet* sharedKnobs) const = 0;

    /**
     * @brief Get the current animation level.
     **/
    virtual AnimationLevelEnum getAnimationLevel(DimIdx dimension, TimeValue time, ViewIdx view) const = 0;

    /**
     * @brief Restores the default value
     **/
    virtual void resetToDefaultValue(DimSpec dimension = DimSpec::all(), ViewSetSpec view = ViewSetSpec::all()) = 0;

    /**
     * @brief Must return true if this Lnob holds a POD (plain old data) type, i.e. int, bool, or double.
     **/
    virtual bool isTypePOD() const = 0;

    /**
     * @brief Must return true if this knob can link with the other knob
     **/
    virtual bool canLinkWith(const KnobIPtr & other, DimIdx thisDimension, ViewIdx thisView, DimIdx otherDim, ViewIdx otherView, std::string* error) const = 0;
    KnobPagePtr getTopLevelPage() const;

    virtual ValueChangedReturnCodeEnum setValueFromKeyFrame(const SetKeyFrameArgs& args, const KeyFrame & k) = 0;


};



/**
 * @brief The internal class that holds a value in a knob.
 * These are not directly members of the knob class so that it is possible
 * to make bi-directional knobs that share a value but not the decoration properties.
 **/
class KnobDimViewBase {

public:

    // Protects all fields of this class and derivatives
    mutable QMutex valueMutex;

    // A set of all knob/dimension/view sharing this value
    KnobDimViewKeySet sharedKnobs;

    // The animation curve if it can animate
    CurvePtr animationCurve;

    // Counter used to know when shared knobs are being refreshed.
    // This is only used on the main-thread.
    int isRefreshingSharedKnobs;

    KnobDimViewBase()
    : valueMutex()
    , sharedKnobs()
    , animationCurve()
    {

    }

    virtual ~KnobDimViewBase()
    {

    }

    struct CopyInArgs
    {
        // Pointer to the other dim/view value to copy.
        // If NULL, otherCurve must be set
        const KnobDimViewBase* other;

        // Can be provided if there's no KnboDimViewBase object to copy from
        // but just a curve object. Generally this should be left to NULL and the curve
        // from the "other" KnboDimViewBase is used, and properly protected by a mutex.
        const Curve* otherCurve;

        // A range of keyframes to copy
        const RangeD* keysToCopyRange;
        
        // A offset to copy keyframes
        double keysToCopyOffset;

        CopyInArgs(const Curve& otherCurve)
        : other(0)
        , otherCurve(&otherCurve)
        , keysToCopyRange(0)
        , keysToCopyOffset(0)
        {

        }
        
        CopyInArgs(const KnobDimViewBase& other)
        : other(&other)
        , otherCurve(0)
        , keysToCopyRange(0)
        , keysToCopyOffset(0)
        {
            
        }
    };
    
    struct CopyOutArgs
    {
        
        // If the copy operation should report keyframes removed, this is a pointer
        // to the keyframes removed list
        KeyFrameSet* keysRemoved;
        
        // If the copy operation should report keyframes added, this is a pointer
        // to the keyframes added list
        KeyFrameSet* keysAdded;
        
        CopyOutArgs()
        : keysRemoved(0)
        , keysAdded(0)
        {
            
        }
        
    };

    /**
     * @brief Set a keyframe, or modify it
     **/
    virtual ValueChangedReturnCodeEnum setKeyFrame(const KeyFrame& key, SetKeyFrameFlags flags);

    /**
     * @brief Copy the other dimension/view value with the given args.
     * @returns Returns true if something changed, false otherwise.
     **/
    virtual bool copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs);

    /**
     * @brief Delete the following keyframes (given by their timeline's time) from the animation curve
     **/
    virtual void deleteValuesAtTime(const std::list<double>& times);

    /**
     * @brief Remove all keyframes before or after the given timeline's time, excluding any keyframe at the 
     * given time.
     **/
    virtual void deleteAnimationConditional(TimeValue time, bool before);

    /**
     * @brief Remove all animation on the curve
     **/
    virtual void removeAnimation();

    /**
     * @brief Warps the following keyframes time and returns optionally their modified version
     **/
    virtual bool warpValuesAtTime(const std::list<double>& times, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* outKeys);

    /**
     * @brief Set the interpolator at the given keyframes time and returns optionally their modified version
     **/
    virtual void setInterpolationAtTimes(const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys);

    /**
     * @brief Set the left and right derivative at the given keyframe time
     **/
    virtual bool setLeftAndRightDerivativesAtTime(TimeValue time, double left, double right);

    /**
     * @brief Set the left or right derivative independently at the given keyframe time
     **/
    virtual bool setDerivativeAtTime(TimeValue time, double derivative, bool isLeft);


protected:

    /**
     * @brief Emits the curveAnimationChanged signal on all knobs referencing this value.
     **/
    void notifyCurveChanged();

};


template <typename T>
class ValueKnobDimView : public KnobDimViewBase
{
public:

    // The static value if not animated
    T value;

    ValueKnobDimView();

    virtual ~ValueKnobDimView()
    {
        
    }

    virtual KeyFrame makeKeyFrame(TimeValue time, const T& value);

    virtual T getValueFromKeyFrame(const KeyFrame& k);

    virtual bool setValueAndCheckIfChanged(const T& value);

    virtual bool copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs) OVERRIDE;
};


///Skins the API of KnobI by implementing most of the functions in a non templated manner.
struct KnobHelperPrivate;
class KnobHelper
    : public KnobI
{
    Q_DECLARE_TR_FUNCTIONS(KnobHelper)

    // friends
    friend class KnobHolder;
    friend class ExprRecursionLevel_RAII;

protected: // derives from KnobI, parent of Knob
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    /**
     * @brief Creates a new Knob that belongs to the given holder, with the given label.
     * The name of the knob will be equal to the label, you can change it by calling setName()
     * The dimension parameter indicates how many dimension the knob should have.
     * If declaredByPlugin is false then Natron will never call onKnobValueChanged on the holder.
     **/
    KnobHelper(const KnobHolderPtr& holder, const std::string &scriptName, int nDims = 1);

    /**
     * @brief Creates a knob that is a render clone of the other knob.
     * Everything non render related is forwarded to otherKnob
     **/
    KnobHelper(const KnobHolderPtr& holder, const KnobIPtr& mainKnob);

public:
    virtual ~KnobHelper();

private:
    virtual void deleteKnob() OVERRIDE FINAL;

public:


    virtual bool getAllDimensionsVisible(ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAllDimensionsVisible(ViewSetSpec view, bool visible)  OVERRIDE FINAL;
    virtual void setCanAutoFoldDimensions(bool enabled) OVERRIDE FINAL ;
    virtual bool isAutoFoldDimensionsEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAdjustFoldExpandStateAutomatically(bool enabled) OVERRIDE FINAL;
    virtual bool isAdjustFoldExpandStateAutomaticallyEnabled() const OVERRIDE FINAL;

private:
    void setAllDimensionsVisibleInternal(ViewIdx view, bool visible);


public:

    KnobDimViewBasePtr getDataForDimView(DimIdx dimension, ViewIdx view) const;

    virtual KnobIPtr getCloneForHolderInternal(const KnobHolderPtr& holder) const OVERRIDE FINAL WARN_UNUSED_RETURN;


    virtual void setActualCloneForHolder(const KnobHolderPtr& holder) OVERRIDE FINAL;

    virtual KnobIPtr getMainInstance() const OVERRIDE FINAL;

    virtual void autoFoldDimensions(ViewIdx view) OVERRIDE FINAL;


    virtual void autoAdjustFoldExpandDimensions(ViewIdx view) OVERRIDE FINAL;

    virtual void convertDimViewArgAccordingToKnobState(DimSpec dimIn, ViewSetSpec viewIn, DimSpec* dimOut, ViewSetSpec* viewOut) const OVERRIDE FINAL;
    /**
     * @brief Returns the knob was created by a plugin or added automatically by Natron (e.g like mask knobs)
     **/
    virtual KnobDeclarationTypeEnum getKnobDeclarationType() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setKnobDeclarationType(KnobDeclarationTypeEnum b) OVERRIDE FINAL;

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
    virtual void setAutoKeyingEnabled(bool enabled) OVERRIDE FINAL;
    virtual bool isAutoKeyingEnabled(DimSpec dimension, TimeValue time, ViewSetSpec view, ValueChangedReasonEnum reason) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool evaluateValueChange(DimSpec dimension, TimeValue time, ViewSetSpec view,  ValueChangedReasonEnum reason) OVERRIDE FINAL;
    virtual void onTimeChanged(bool isPlayback, TimeValue time) OVERRIDE FINAL;

private:

    bool isAutoKeyingEnabledInternal(DimIdx dimension, TimeValue time, ViewIdx view) const WARN_UNUSED_RETURN;

protected:

    virtual KnobDimViewBasePtr createDimViewData() const = 0;

    // Returns true if the knobChanged handler was called
    bool evaluateValueChangeInternal(DimSpec dimension,
                                     TimeValue time,
                                     ViewSetSpec view,
                                     ValueChangedReasonEnum reason,
                                     KnobDimViewKeySet* evaluatedKnobs);

public:


    virtual double random(TimeValue time, unsigned int seed) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double random(double min = 0., double max = 1.) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int randomInt(TimeValue time, unsigned int seed) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int randomInt(int min = 0, int max = INT_MAX) const OVERRIDE FINAL WARN_UNUSED_RETURN;

protected:


    void randomSeed(TimeValue time, unsigned int seed) const;

#ifdef DEBUG
    //Helper to set breakpoints in templated code
    void debugHook();
#endif


public:

    //////////// Overriden from AnimatingObjectI
    virtual bool cloneCurve(ViewIdx view, DimIdx dimension, const Curve& curve, double offset, const RangeD* range) OVERRIDE;
    virtual void deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason) OVERRIDE;
    virtual bool warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes = 0) OVERRIDE ;
    virtual void removeAnimation(ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason) OVERRIDE ;
    virtual void deleteAnimationBeforeTime(TimeValue time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void deleteAnimationAfterTime(TimeValue time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void setInterpolationAtTimes(ViewSetSpec view, DimSpec dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys = 0) OVERRIDE ;
    virtual bool setLeftAndRightDerivativesAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, double left, double right)  OVERRIDE WARN_UNUSED_RETURN;
    virtual bool setDerivativeAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, double derivative, bool isLeft) OVERRIDE WARN_UNUSED_RETURN;
    virtual CurvePtr getAnimationCurve(ViewIdx idx, DimIdx dimension) const OVERRIDE ;
    //////////// End from AnimatingObjectI

private:

    bool removeAnimationInternal(ViewIdx view, DimIdx dimension);
    void deleteValuesAtTimeInternal(const std::list<double>& times, ViewIdx view, DimIdx dimension);
    void deleteAnimationConditional(TimeValue time, ViewSetSpec view, DimSpec dimension, bool before);
    void deleteAnimationConditionalInternal(TimeValue time, ViewIdx view, DimIdx dimension, bool before);
    bool warpValuesAtTimeInternal(const std::list<double>& times, ViewIdx view,  DimIdx dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes);
    void setInterpolationAtTimesInternal(ViewIdx view, DimIdx dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys);
    bool setLeftAndRightDerivativesAtTimeInternal(ViewIdx view, DimIdx dimension, TimeValue time, double left, double right);
    bool setDerivativeAtTimeInternal(ViewIdx view, DimIdx dimension, TimeValue time, double derivative, bool isLeft);

public:


    
    virtual bool isAnimated( DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasAnimation() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool checkInvalidLinks() OVERRIDE FINAL;
    virtual bool isLinkValid(DimIdx dimension, ViewIdx view, std::string* error) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ExpressionLanguageEnum getExpressionLanguage(ViewIdx view, DimIdx dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setLinkStatus(DimSpec dimension, ViewSetSpec view, bool valid, const std::string& error) OVERRIDE FINAL;

protected:

    void setExpressionInternal(DimIdx dimension, ViewIdx view, const std::string& expression, ExpressionLanguageEnum language, bool hasRetVariable, bool failIfInvalid);

private:

    void setLinkStatusInternal(DimIdx dimension, ViewIdx view, bool valid, const std::string& error);

    void replaceNodeNameInExpressionInternal(DimIdx dimension,
                                             ViewIdx view,
                                             const std::string& oldName,
                                             const std::string& newName);

    bool clearExpressionInternal(DimIdx dimension, ViewIdx view);
public:

    virtual void setExpressionCommon(DimSpec dimension, ViewSetSpec view, const std::string& expression, ExpressionLanguageEnum language, bool hasRetVariable, bool failIfInvalid) OVERRIDE FINAL;

    virtual void restoreKnobLinks(const SERIALIZATION_NAMESPACE::KnobSerializationBasePtr& serialization,
                                  const std::map<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr>& allCreatedNodesInGroup) OVERRIDE FINAL;

private:

    KnobIPtr findMasterKnob(const std::string& masterKnobName,
                            const std::string& masterNodeName,
                            const std::string& masterTableName,
                            const std::string& masterItemName,
                            const std::map<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr>& allCreatedNodesInGroup);


public:


    virtual void replaceNodeNameInExpression(DimSpec dimension,
                                             ViewSetSpec view,
                                             const std::string& oldName,
                                             const std::string& newName) OVERRIDE FINAL;
    virtual void clearExpression(DimSpec dimension, ViewSetSpec view) OVERRIDE FINAL;

    virtual void setExpressionsResultsCachingEnabled(bool enabled) OVERRIDE FINAL;
    virtual bool isExpressionsResultsCachingEnabled() const OVERRIDE FINAL;

    virtual void validateExpression(const std::string& expression, ExpressionLanguageEnum language, DimIdx dimension, ViewIdx view, bool hasRetVariable, std::string* resultAsString) OVERRIDE FINAL ;

    virtual bool linkTo(const KnobIPtr & otherKnob, DimSpec thisDimension = DimSpec::all(), DimSpec otherDimension = DimSpec::all(), ViewSetSpec thisView = ViewSetSpec::all(), ViewSetSpec otherView = ViewSetSpec::all()) OVERRIDE FINAL;
    virtual void unlink(DimSpec dimension, ViewSetSpec view, bool copyState) OVERRIDE FINAL;

protected:

  
    /*
     * @brief Callback called when linked or unlinked to refresh extra state
     */
    virtual void onLinkChanged() {

    }

    void unlinkInternal(DimIdx dimension, ViewIdx view, bool copyState);

    bool linkToInternal(const KnobIPtr & otherKnob, DimIdx thisDimension, DimIdx otherDimension, ViewIdx view, ViewIdx otherView) WARN_UNUSED_RETURN;

    template <typename T>
    static T pyObjectToType(PyObject* o);

    // the following is specialized only for T=std::string
    template <typename T>
    T pyObjectToType(PyObject* o, DimIdx dim, ViewIdx view) const { (void)view; (void)dim;  return pyObjectToType<T>(o); }

    void refreshListenersAfterValueChangeInternal(TimeValue time, ViewIdx view, ValueChangedReasonEnum reason, DimIdx dimension, KnobDimViewKeySet* evaluatedKnobs);

    void refreshListenersAfterValueChange(TimeValue time, ViewSetSpec view, ValueChangedReasonEnum reason, DimSpec dimension, KnobDimViewKeySet* evaluatedKnobs) ;

public:

    virtual bool isExpressionUsingRetVariable(ViewIdx view, DimIdx dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getExpressionDependencies(DimIdx dimension, ViewIdx view, KnobDimViewKeySet& dependencies) const OVERRIDE FINAL;
    virtual std::string getExpression(DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasExpression(DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setAnimationEnabled(bool val) OVERRIDE FINAL;
    virtual bool isAnimationEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setKeyFrameTrackingEnabled(bool enabled) OVERRIDE FINAL;
    virtual bool isKeyFrameTrackingEnabled() const OVERRIDE FINAL;
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
    virtual void setEnabled(bool b) OVERRIDE FINAL;
    virtual bool isEnabled() const OVERRIDE FINAL;
    virtual void setSecret(bool b) OVERRIDE FINAL;
    virtual bool getIsSecret() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getIsSecretRecursive() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setIsFrozen(bool frozen) OVERRIDE FINAL;
    virtual void setKnobSelectedMultipleTimes(bool d) OVERRIDE FINAL;
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
    virtual void setCustomInteract(const OverlayInteractBasePtr & interactDesc) OVERRIDE FINAL;
    virtual OverlayInteractBasePtr getCustomInteract() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual TimeValue getCurrentRenderTime() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ViewIdx getCurrentRenderView() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getDimensionName(DimIdx dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setDimensionName(DimIdx dim, const std::string & name) OVERRIDE FINAL;
    virtual bool hasModifications() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasModifications(DimIdx dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual KnobIPtr createDuplicateOnHolder(const KnobHolderPtr& otherHolder,
                                            const KnobPagePtr& page,
                                            const KnobGroupPtr& group,
                                            int indexInParent,
                                            DuplicateKnobTypeEnum duplicateType,
                                            const std::string& newScriptName,
                                            const std::string& newLabel,
                                            const std::string& newToolTip,
                                            bool refreshParams,
                                            KnobI::KnobDeclarationTypeEnum isUserKnob) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool copyKnob(const KnobIPtr& other, ViewSetSpec view = ViewSetSpec::all(), DimSpec dimension = DimSpec::all(), ViewSetSpec otherView = ViewSetSpec::all(), DimSpec otherDimension = DimSpec::all(), const RangeD* range = 0, double offset = 0) OVERRIDE FINAL;

private:

    virtual bool invalidateHashCacheInternal(std::set<HashableObject*>* invalidatedObjects) OVERRIDE FINAL;


    bool cloneValueInternal(const KnobIPtr& other, ViewIdx view, ViewIdx otherView, DimIdx dimension, DimIdx otherDimension, const RangeD* range, double offset);
    bool cloneValues(const KnobIPtr& other, ViewSetSpec view, ViewSetSpec otherView, DimSpec dimension, DimSpec otherDimension, const RangeD* range, double offset);
    
protected:

    virtual bool setHasModifications(DimIdx dimension, ViewIdx view, bool value, bool lock) OVERRIDE FINAL;

    /**
     * @brief Protected so the implementation of unSlave can actually use this to reset the master pointer
     **/
    void resetMaster(DimIdx dimension, ViewIdx view);

    ///The return value must be Py_DECRREF
    bool executePythonExpression(TimeValue time, ViewIdx view, DimIdx dimension, PyObject** ret, std::string* error) const;

public:

    enum ExpressionReturnValueTypeEnum
    {
        eExpressionReturnValueTypeScalar,
        eExpressionReturnValueTypeString,
        eExpressionReturnValueTypeError
    };
protected:
    
    ExpressionReturnValueTypeEnum executeExprTkExpression(TimeValue time, ViewIdx view, DimIdx dimension, double* retValueIsScalar, std::string* retValueIsString, std::string* error);

    /// The return value must be Py_DECRREF
    /// The expression must put its result in the Python variable named "ret"
    static bool executePythonExpression(const std::string& expr, PyObject** ret, std::string* error);
    static ExpressionReturnValueTypeEnum executeExprTkExpression(const std::string& expr, double* retValueIsScalar, std::string* retValueIsString, std::string* error);

public:


    /// This static publicly-available function is useful to evaluate simple python expressions that evaluate to a double, int or string value.
    /// The expression must put its result in the Python variable named "ret"
    static ExpressionReturnValueTypeEnum evaluateExpression(const std::string& expr, ExpressionLanguageEnum language, double* retIsScalar, std::string* retIsString, std::string* error);


    virtual bool getSharingMaster(DimIdx dimension, ViewIdx view, KnobDimViewKey* linkData) const OVERRIDE FINAL;
    virtual void getSharedValues(DimIdx dimension, ViewIdx view, KnobDimViewKeySet* sharedKnobs) const OVERRIDE FINAL;

    virtual AnimationLevelEnum getAnimationLevel(DimIdx dimension, TimeValue time, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Adds a new listener to this knob. This is just a pure notification about the fact that the given knob
     * is listening to the values/keyframes of "this". It could be call addSlave but it will also be use for expressions.
     **/
    virtual void addListener(DimIdx fromExprDimension,
                             DimIdx thisDimension,
                             const ViewIdx listenerView,
                             const ViewIdx listenedToView,
                             const KnobIPtr& knob,
                             ExpressionLanguageEnum language) OVERRIDE FINAL;

    virtual void getListeners(KnobDimViewKeySet& listeners, ListenersTypeFlags flags = ListenersTypeFlags(eListenersTypeExpression | eListenersTypeSharedValue)) const OVERRIDE FINAL;

private:

    void incrementExpressionRecursionLevel() const;

    void decrementExpressionRecursionLevel() const;

protected:


    int getExpressionRecursionLevel() const;



public:

    /**
     * @brief Implement to save the content of the object to the serialization object
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    /**
     * @brief Implement to load the content of the serialization object onto this object
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;

    virtual void restoreValueFromSerialization(const SERIALIZATION_NAMESPACE::ValueSerialization& obj,
                                               DimIdx targetDimension,
                                               ViewIdx view) OVERRIDE;

    virtual void restoreDefaultValueFromSerialization(const SERIALIZATION_NAMESPACE::DefaultValueSerialization& defObj, bool applyDefaultValue, DimIdx targetDimension) OVERRIDE FINAL;

    virtual bool splitView(ViewIdx view) OVERRIDE;

    virtual bool unSplitView(ViewIdx view) OVERRIDE;

    virtual bool canSplitViews() const OVERRIDE;

protected:

    virtual void refreshCurveMinMaxInternal(ViewIdx view, DimIdx dimension) = 0;

    void refreshCurveMinMax(ViewSetSpec view, DimSpec dimension);

    virtual void copyValuesFromCurve(DimIdx /*dim*/, ViewIdx /*view*/) {}


    bool cloneExpressions(const KnobIPtr& other, ViewSetSpec view, ViewSetSpec otherView, DimSpec dimension, DimSpec otherDimension);

private:

    bool cloneExpressionInternal(const KnobIPtr& other, ViewIdx view, ViewIdx otherView, DimIdx dimension, DimIdx otherDimension);

protected:


    boost::shared_ptr<KnobSignalSlotHandler> _signalSlotHandler;

private:

    void expressionChanged(DimIdx dimension, ViewIdx view);

    friend struct KnobHelperPrivate;
    boost::scoped_ptr<KnobHelperPrivate> _imp;
};

inline KnobHelperPtr
toKnobHelper(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobHelper>(knob);
}

struct DimTimeView
{
    TimeValue time;
    DimIdx dimension;
    ViewIdx view;
};

struct ValueDimTimeViewCompareLess
{

    bool operator() (const DimTimeView& lhs,
                     const DimTimeView& rhs) const
    {
        if (lhs.view < rhs.view) {
            return true;
        } else if (lhs.view > rhs.view) {
            return false;
        } else {
            if (lhs.dimension < rhs.dimension) {
                return true;
            } else if (lhs.dimension > rhs.dimension) {
                return false;
            } else {
                return lhs.time < rhs.time;
            }
        }
    }
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

protected: // derives from KnobI, parent of KnobInt, KnobBool
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    /**
     * @brief Make a knob for the given KnobHolder with the given label (the label displayed on
     * its interface) and with the given dimension. The dimension parameter is used for example for the
     * KnobColor which has 4 doubles (r,g,b,a), hence 4 dimensions.
     **/
    Knob(const KnobHolderPtr& holder,
         const std::string & name,
         int nDims = 1);

    Knob(const KnobHolderPtr& holder, const KnobIPtr& mainKnob);

public:
    virtual ~Knob();


    virtual void computeHasModifications() OVERRIDE FINAL;

protected:

    virtual KnobDimViewBasePtr createDimViewData() const OVERRIDE;
    
    virtual bool hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const;

public:

    /**
     * @brief Get the current value of the knob for the given dimension and view.
     * If it is animated, it will return the value at the current time.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual T getValue(DimIdx dimension = DimIdx(0), ViewIdx view = ViewIdx(0), bool clampToMinMax = true) WARN_UNUSED_RETURN;


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
    virtual T getValueAtTime(TimeValue time, DimIdx dimension = DimIdx(0), ViewIdx view = ViewIdx(0), bool clampToMinMax = true)  WARN_UNUSED_RETURN;

    virtual bool getCurveKeyFrame(TimeValue time, DimIdx dimension, ViewIdx view, bool clampToMinMax, KeyFrame* key) OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Same as getValueAtTime excepts that the expression is evaluated to return a double value, mainly to display the curve corresponding to an expression
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual double getValueAtWithExpression(TimeValue time, ViewIdx view, DimIdx dimension)  OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Creates a keyframe object and setup its properties given the value.
     **/
    KeyFrame makeKeyFrame(TimeValue time, const T& value, DimIdx dimension, ViewIdx view) const;

    T getValueFromKeyFrame(const KeyFrame& key, DimIdx dimension, ViewIdx view) const;

public:

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash) OVERRIDE;


    virtual T getValueForHash(DimIdx dim, ViewIdx view);

    //////////// Overriden from AnimatingObjectI
    virtual CurveTypeEnum getKeyFrameDataType() const OVERRIDE;

    virtual ValueChangedReturnCodeEnum setKeyFrame(const SetKeyFrameArgs& args, const KeyFrame& key) OVERRIDE;
    virtual void setMultipleKeyFrames(const SetKeyFrameArgs& args, const std::list<KeyFrame>& keys) OVERRIDE;
    virtual void setKeyFramesAcrossDimensions(const SetKeyFrameArgs& args, const std::vector<KeyFrame>& values, DimIdx dimensionStartIndex = DimIdx(0), std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE;

    //////////// end overriden from AnimatingObjectI


    // Convenience function, calls setKeyFrame
    ValueChangedReturnCodeEnum setValueAtTime(TimeValue time,
                                              const T & v,
                                              ViewSetSpec view = ViewSetSpec::all(),
                                              DimSpec dimension = DimSpec(0),
                                              ValueChangedReasonEnum reason = eValueChangedReasonUserEdited,
                                              bool forceHandlerEvenIfNoChange = false);


    // Convenience function, calls setKeyFramesAcrossDimensions
    void setValueAtTimeAcrossDimensions(TimeValue time,
                                        const std::vector<T>& values,
                                        DimIdx dimensionStartIndex = DimIdx(0),
                                        ViewSetSpec view = ViewSetSpec::all(),
                                        ValueChangedReasonEnum reason = eValueChangedReasonUserEdited,
                                        std::vector<ValueChangedReturnCodeEnum>* retCodes = 0);



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
                                        ViewSetSpec view = ViewSetSpec::all(),
                                        DimSpec dimension = DimSpec(0),
                                        ValueChangedReasonEnum reason = eValueChangedReasonUserEdited,
                                        bool forceHandlerEvenIfNoChange = false);


    virtual ValueChangedReturnCodeEnum setValueFromKeyFrame(const SetKeyFrameArgs& args, const KeyFrame & k) OVERRIDE;


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
                                  ViewSetSpec view = ViewSetSpec::all(),
                                  ValueChangedReasonEnum reason = eValueChangedReasonUserEdited,
                                  std::vector<ValueChangedReturnCodeEnum>* retCodes = 0);



    /**
     * @brief Get the current default values
     **/
    virtual T getDefaultValue(DimIdx dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief If the default value was changed more than once with setDefaultValue, this is the initial value that was passed to setDefaultValue
     **/
    virtual T getInitialDefaultValue(DimIdx dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief Returns true if setDefaultValue was never called yet
     **/
    bool isDefaultValueSet(DimIdx dimension) const WARN_UNUSED_RETURN;

    /**
     * @brief Set the current default value as if it was the initial default value passed to the knob
     **/
    virtual void setCurrentDefaultValueAsInitialValue();

    /**
     * @brief Set a default value for a specific dimension
     * Note that it will also restore the knob to its defaults
     **/
    virtual void setDefaultValue(const T & v, DimSpec dimension = DimSpec(0));
    virtual void setDefaultValues(const std::vector<T>& values, DimIdx dimensionStartOffset = DimIdx(0));

    /**
     * @brief Same as setDefaultValue except that it does not restore the knob to its defaults
     **/
    virtual void setDefaultValueWithoutApplying(const T& v, DimSpec dimension = DimSpec(0));
    virtual void setDefaultValuesWithoutApplying(const std::vector<T>& values, DimIdx dimensionStartOffset = DimIdx(0));

protected:

    virtual void onDefaultValueChanged(DimSpec dimension)
    {
        Q_UNUSED(dimension);
    }

public:

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
    virtual bool canLinkWith(const KnobIPtr & other, DimIdx thisDimension, ViewIdx thisView, DimIdx otherDim, ViewIdx otherView, std::string* error) const OVERRIDE WARN_UNUSED_RETURN;

    ///Cannot be overloaded by KnobHelper as it requires the value member
    virtual double getDerivativeAtTime(TimeValue time, ViewIdx view, DimIdx dimension)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double getIntegrateFromTimeToTime(TimeValue time1, TimeValue time2, ViewIdx view, DimIdx dimension)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    double getIntegrateFromTimeToTimeSimpson(TimeValue time1, TimeValue time2, ViewIdx view, DimIdx dimension);

    ///Cannot be overloaded by KnobHelper as it requires setValue
    virtual void resetToDefaultValue(DimSpec dimension = DimSpec::all(), ViewSetSpec view = ViewSetSpec::all()) OVERRIDE FINAL;


    virtual void cloneDefaultValues(const KnobIPtr& other) OVERRIDE FINAL;

    ///MT-safe
    void setMinimum(const T& mini, DimSpec dimension = DimSpec::all());
    void setMaximum(const T& maxi, DimSpec dimension = DimSpec::all());
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


    virtual bool hasDefaultValueChanged(DimIdx dimension) const OVERRIDE ;


    /**
     * @brief Returns whether 2 dimensions are equal.
     * It checks their expression, animation curve and value.
     **/
    virtual bool areDimensionsEqual(ViewIdx view) OVERRIDE FINAL;

    virtual void clearRenderValuesCache() OVERRIDE
    {
        if (_valuesCache) {
            _valuesCache->clear();
        }
    }

    virtual void clearExpressionsResults(DimSpec dimension, ViewSetSpec view) OVERRIDE FINAL;

protected:


    virtual void refreshCurveMinMaxInternal(ViewIdx view, DimIdx dimension) OVERRIDE FINAL;


    virtual void resetExtraToDefaultValue(DimSpec /*dimension*/, ViewSetSpec /*view*/) {}


private:

    
    T getValueInternal(TimeValue currentTime,
                       DimIdx dimension,
                       ViewIdx view,
                       bool clamp);

    void addSetValueToUndoRedoStackIfNeeded(const T& oldValue, const T& value, ValueChangedReasonEnum reason, ValueChangedReturnCodeEnum setValueRetCode, ViewSetSpec view, DimSpec dimension, TimeValue time, bool setKeyFrame);

    virtual void copyValuesFromCurve(DimIdx dim, ViewIdx view) OVERRIDE FINAL;

    void initMinMax();

    T clampToMinMax(const T& value, DimIdx dimension) const;

private:

    bool evaluateExpression(TimeValue time, ViewIdx view, DimIdx dimension, T* ret, std::string* error);

    bool handleExprtkReturnValue(KnobHelper::ExpressionReturnValueTypeEnum type, double valueAsDouble, const std::string& valueAsString, T* finalValue, std::string* error);

    /*
     * @brief Same as evaluateExpression but expects it to return a PoD
     */
    bool evaluateExpression_pod(TimeValue time, ViewIdx view, DimIdx dimension, double* value, std::string* error);

    bool getValueFromExpression(TimeValue time, ViewIdx view, DimIdx dimension, bool clamp, T* ret);

    bool getValueFromExpression_pod(TimeValue time, ViewIdx view, DimIdx dimension, bool clamp, double* ret);

    //////////////////////////////////////////////////////////////////////
    /////////////////////////////////// End implementation of KnobI
    //////////////////////////////////////////////////////////////////////
    struct DefaultValue
    {
        T initialValue,value;
        bool defaultValueSet;
    };


    struct TimeViewPair
    {
        TimeValue time;
        ViewIdx view;
    };


    struct TimeView_compare_less
    {
        bool operator() (const TimeViewPair & lhs,
                         const TimeViewPair & rhs) const
        {

            if (std::abs(lhs.time - rhs.time) < 1e-6) {
                if (lhs.view == -1 || rhs.view == -1 || lhs.view == rhs.view) {
                    return false;
                }
                if (lhs.view < rhs.view) {
                    return true;
                } else {
                    // lhs.view > rhs.view
                    return false;
                }
            } else if (lhs.time < rhs.time) {
                return true;
            } else {
                assert(lhs.time > rhs.time);
                return false;
            }
        }


    };

    typedef std::map<TimeViewPair, T, TimeView_compare_less> ExpressionCache;
    typedef std::map<ViewIdx, ExpressionCache> PerViewExpressionCache;
    typedef std::vector<PerViewExpressionCache> PerDimensionExpressionCache;

    typedef std::map<DimTimeView, T, ValueDimTimeViewCompareLess> ValuesCacheMap;


    struct Data
    {
        ///Here is all the stuff we couldn't get rid of the template parameter
        mutable QMutex defaultValueMutex; //< protects  _defaultValues and _exprRes


        std::vector<DefaultValue> defaultValues;

        // Only for double and int
        mutable QMutex minMaxMutex;
        std::vector<T>  minimums, maximums, displayMins, displayMaxs;

        mutable QMutex expressionResultsMutex;
        PerDimensionExpressionCache expressionResults;

        Data(int nDims)
        : defaultValueMutex()
        , defaultValues(nDims)
        , minMaxMutex(QMutex::Recursive)
        , minimums(nDims)
        , maximums(nDims)
        , displayMins(nDims)
        , displayMaxs(nDims)
        , expressionResultsMutex()
        , expressionResults()
        {

        }
    };

    // Used only on render clones to cache the result of getValue/getValueAtTime so it stays
    // consistant throughout a render.
    boost::scoped_ptr<ValuesCacheMap> _valuesCache;

    // The Data pointer is shared accross the "main" instance and the render clones.
    boost::shared_ptr<Data> _data;


};



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
 * @brief Helper class that helps in the implementation of all setValue-like functions to determine if we need to add a value to the undo/redo stack.
 **/
template <typename T>
class AddToUndoRedoStackHelper
{
    Knob<T>* _knob;
    KnobHolderPtr _holder;
    bool _mustEndEdits;
    bool _isUndoRedoStackOpened;

    struct ValueToPush {
        PerDimViewKeyFramesMap oldValues;
        ViewSetSpec view;
        DimSpec dimension;
        bool setKeyframe;
        ValueChangedReasonEnum reason;
    };

    std::list<ValueToPush> _valuesToPushQueue;

public:

    AddToUndoRedoStackHelper(Knob<T>* k);

    ~AddToUndoRedoStackHelper();

    bool canAddValueToUndoStack() const;

    /**
     * @brief Must be called to prepare the old value per dimension/view map before a call to addSetValueToUndoRedoStackIfNeeded.
     * each call to this function must match a following call of addSetValueToUndoRedoStackIfNeeded.
     **/
    void prepareOldValueToUndoRedoStack(ViewSetSpec view, DimSpec dimension, TimeValue time, ValueChangedReasonEnum reason, bool setKeyFrame);

    /**
     * @brief Must be called after prepareOldValueToUndoRedoStack to finalize the value in the undo/redo stack.
     **/
    void addSetValueToUndoRedoStackIfNeeded(const KeyFrame& value, ValueChangedReturnCodeEnum setValueRetCode);
};

typedef Knob<bool> KnobBoolBase;
typedef Knob<double> KnobDoubleBase;
typedef Knob<int> KnobIntBase;
typedef Knob<std::string> KnobStringBase;

typedef boost::shared_ptr<KnobBoolBase> KnobBoolBasePtr;
typedef boost::shared_ptr<KnobDoubleBase> KnobDoubleBasePtr;
typedef boost::shared_ptr<KnobIntBase> KnobIntBasePtr;
typedef boost::shared_ptr<KnobStringBase> KnobStringBasePtr;


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

    // The constructor to create a render clone
    KnobHolder(const KnobHolderPtr& mainInstance, const FrameViewRenderKey& key);

public:

    virtual ~KnobHolder();

    /**
     * @brief If this KnobHolder is a render clone, return the "main instance" one.
     **/
    KnobHolderPtr getMainInstance() const;

    bool isRenderClone() const;

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
    void addOverlaySlaveParam(const KnobIPtr& knob, OverlaySlaveParamFlags type = eOverlaySlaveViewport);

    bool isOverlaySlaveParam(const KnobIConstPtr& knob, OverlaySlaveParamFlags type = eOverlaySlaveViewport) const;

    //To re-arrange user knobs only, does nothing if this is not a user knob
    bool moveKnobOneStepUp(const KnobIPtr& knob);
    bool moveKnobOneStepDown(const KnobIPtr& knob);

    AppInstancePtr getApp() const WARN_UNUSED_RETURN;
    KnobIPtr getKnobByName(const std::string & name) const WARN_UNUSED_RETURN;

    template <typename K>
    boost::shared_ptr<K> getKnobByNameAndType(const std::string& name) const 
    {
        KnobIPtr ret = getKnobByName(name);
        if (!ret) {
            return boost::shared_ptr<K>();
        }
        return boost::dynamic_pointer_cast<K>(ret);
    }


    const std::vector<KnobIPtr> & getKnobs() const WARN_UNUSED_RETURN;
    std::vector<KnobIPtr> getKnobs_mt_safe() const WARN_UNUSED_RETURN;

    void refreshAfterTimeChange(bool isPlayback, TimeValue time);

    virtual TimeValue getTimelineCurrentTime() const;

    /**
     * @brief Same as refreshAfterTimeChange but refreshes only the knobs
     * whose function evaluateValueChangeOnTimeChange() return true so that
     * after a playback their state is correct
     **/
    void refreshAfterTimeChangeOnlyKnobsWithTimeEvaluation(TimeValue time);

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
                                                   TimeValue /*time*/) {}


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


    /**
     * @brief Push a new undo command to the undo/redo stack associated to this node.
     * The stack takes ownership of the shared pointer, so you should not hold a strong reference to the passed pointer.
     * If no undo/redo stack is present, the command will just be redone once then destroyed.
     **/
    void pushUndoCommand(const UndoCommandPtr& command);

    // Same as the version above, do NOT derefence command after this call as it will be destroyed already, usually this call is
    // made as such: pushUndoCommand(new MyCommand(...))
    void pushUndoCommand(UndoCommand* command);


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
     * @brief Creates a new knob. If another knob with the same name already exists, its name
     * will be modified so it is unique. If you don't want the name to be modified you may use
     * the getOrCreateKnob function instead.
     **/
    template <typename K>
    boost::shared_ptr<K> createKnob(const std::string& name, int dimension = 1)
    {
        return appPTR->getKnobFactory().createKnob<K>(shared_from_this(), name, dimension);
    }

    /**
     * @brief Same as createKnob() except that it does not create a knob if another knob already
     * exists with the same name.
     **/
    template <typename K>
    boost::shared_ptr<K> getOrCreateKnob(const std::string& name, int dimension = 1)
    {
        return appPTR->getKnobFactory().getOrCreateKnob<K>(shared_from_this(), name, dimension);
    }

    /**
     * @brief These functions below are dynamic in a sense that they can be called at any time (on the main-thread)
     * to create knobs on the fly. Their gui will be properly created. In order to notify the GUI that new parameters were
     * created, you must call refreshKnobs() that will re-scan for new parameters
     **/
    KnobIntPtr createIntKnob(const std::string& name, const std::string& label, int nDims, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobDoublePtr createDoubleKnob(const std::string& name, const std::string& label, int nDims, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobColorPtr createColorKnob(const std::string& name, const std::string& label, int nDims, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobBoolPtr createBoolKnob(const std::string& name, const std::string& label, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobChoicePtr createChoiceKnob(const std::string& name, const std::string& label, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobButtonPtr createButtonKnob(const std::string& name, const std::string& label, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobSeparatorPtr createSeparatorKnob(const std::string& name, const std::string& label, KnobI::KnobDeclarationTypeEnum userKnob);

    //Type corresponds to the Type enum defined in StringParamBase in Parameter.h
    KnobStringPtr createStringKnob(const std::string& name, const std::string& label, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobFilePtr createFileKnob(const std::string& name, const std::string& label, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobPathPtr createPathKnob(const std::string& name, const std::string& label, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobPagePtr createPageKnob(const std::string& name, const std::string& label, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobGroupPtr createGroupKnob(const std::string& name, const std::string& label, KnobI::KnobDeclarationTypeEnum userKnob);
    KnobParametricPtr createParametricKnob(const std::string& name, const std::string& label, int nbCurves, KnobI::KnobDeclarationTypeEnum userKnob);

    virtual bool isDoingInteractAction() const { return false; }

    bool isEvaluationBlocked() const;

    bool onKnobValueChangedInternal(const KnobIPtr& knob,
                                    TimeValue time,
                                    ViewSetSpec view,
                                    ValueChangedReasonEnum reason);
    
    /**
     * @brief To implement if you need to make the hash vary at a specific time/view
     **/

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash) OVERRIDE;

    /**
     * @brief If this function returns true, all knobs of thie KnobHolder will have their curve appended to their hash
     * regardless of their hashing strategy.
     **/
    virtual bool isFullAnimationToHashEnabled() const;

    enum KnobItemsTablePositionEnum
    {
        // The table will be placed at the bottom of all pages
        eKnobItemsTablePositionBottomOfAllPages,

        // The table will be placed after the named knob in the named knob page
        eKnobItemsTablePositionAfterKnob,
    };

    /**
     * @brief Set the holder to have the given table to display in its settings panel.
     * The KnobHolder takes ownership of the table.
     * @param paramScriptNameBefore The script-name of the knob right before where the table should be inserted in the layout or the script-name of a page.
     **/
    void addItemsTable(const KnobItemsTablePtr& table, KnobItemsTablePositionEnum positioning, const std::string& paramScriptNameBefore);
    KnobItemsTablePtr getItemsTable(const std::string& tableIdentifier) const;
    std::list<KnobItemsTablePtr> getAllItemsTables() const;
    KnobItemsTablePositionEnum getItemsTablePosition(const std::string& tableIdentifier) const;
    std::string getItemsTablePreviousKnobScriptName(const std::string& tableIdentifier) const;

    virtual TimeValue getCurrentRenderTime() const;
    virtual ViewIdx getCurrentRenderView() const;

    TreeRenderPtr getCurrentRender() const;
    

protected:

    void onUserKnobCreated(const KnobIPtr& knob, KnobI::KnobDeclarationTypeEnum isUserKnob);

public:



    /**
     * @brief Use this to bracket setValue() calls, this will actually trigger the evaluate() (i.e: render) function
     * if needed when endChanges() is called
     **/
    void beginChanges();

    void endChanges(bool discardEverything = false);


    /**
     * @brief The virtual portion of notifyProjectBeginValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to prepare yourself to a lot of value changes.
     **/
    virtual void beginKnobsValuesChanged_public(ValueChangedReasonEnum reason);

    /**
     * @brief The virtual portion of notifyProjectEndKnobsValuesChanged(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to finish a serie of value changes, thus limiting the amount of changes to do.
     **/
    virtual void endKnobsValuesChanged_public(ValueChangedReasonEnum reason);

    /**
     * @brief The virtual portion of notifyProjectEvaluationRequested(). This is called by the project
     * You should NEVER CALL THIS YOURSELF as it would break the bracketing system.
     * You can overload this to do things when a value is changed. Bear in mind that you can compress
     * the change by using the begin/end[ValueChanges] to optimize the changes.
     **/
    virtual bool onKnobValueChanged_public(const KnobIPtr& k, ValueChangedReasonEnum reason, TimeValue time, ViewSetSpec view);



    /*Add a knob to the vector. This is called by the
       Knob class. Don't call this*/
    void addKnob(const KnobIPtr& k);

    void insertKnob(int idx, const KnobIPtr& k);
    bool removeKnobFromList(const KnobIConstPtr& knob);


    void initializeKnobsPublic();


    int getPageIndex(const KnobPagePtr page) const;


    //Calls onSignificantEvaluateAboutToBeCalled + evaluate
    void invalidateCacheHashAndEvaluate(bool isSignificant, bool refreshMetadata);


    /**
     * @brief Return a list of all the internal pages which were created by the user
     **/
    void getUserPages(std::list<KnobPagePtr>& userPages) const;

    /**
     * @brief Return true if a clone of this knob holder is needed when rendering 
     **/
    virtual bool isRenderCloneNeeded() const
    {
        return false;
    }


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
                                    TimeValue /*time*/,
                                    ViewSetSpec /*view*/)
    {
        return false;
    }

public:
    /**
     * @brief Create a new clone for rendering.
     * Can only be called on the main instance!
     **/
    KnobHolderPtr createRenderClone(const FrameViewRenderKey& key) const;

    /**
     * @brief Remove a clone previously created with createRenderClone
     * Can only be called on the main instance!
     **/
    bool removeRenderClone(const TreeRenderPtr& key);

    /**
     * @brief Returns a clone created for the given render
     * Can only be called on the main instance!
     **/
    KnobHolderPtr getRenderClone(const FrameViewRenderKey& key) const;

protected:


    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() = 0;

    /**
     * @brief Create a shallow render copy of this holder. 
     * If this is implemented, also implement isRenderCloneNeeded()
     **/
    virtual KnobHolderPtr createRenderCopy(const FrameViewRenderKey& key) const
    {
        Q_UNUSED(key);
        return KnobHolderPtr();
    }

    /**
     * @brief Implement to fetch knobs needed during any action called on a render thread.
     * Derived implementation should call base-class version
     **/
    virtual void fetchRenderCloneKnobs();
};


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

    NamedKnobHolder(const boost::shared_ptr<NamedKnobHolder>& other, const FrameViewRenderKey& key)
    : KnobHolder(other, key)
    {
    }

public:
    virtual ~NamedKnobHolder()
    {
    }

    virtual std::string getScriptName_mt_safe() const = 0;
};

class ScopedChanges_RAII
{
    KnobHolder* _holder;
    bool _discard;
public:

    ScopedChanges_RAII(KnobHolder* holder, bool discard = false)
    : _holder(holder)
    , _discard(discard)
    {
        if (_holder) {
            _holder->beginChanges();
        }
    }

    ScopedChanges_RAII(KnobI* knob, bool discard = false)
    : _holder()
    , _discard(discard)
    {
        _holder = knob->getHolder().get();
        if (_holder) {
            _holder->beginChanges();
        }
    }

    ~ScopedChanges_RAII()
    {
        if (_holder) {
            _holder->endChanges(_discard);
        }
    }
};


NATRON_NAMESPACE_EXIT

DECLARE_OPERATORS_FOR_FLAGS(NATRON_NAMESPACE::KnobI::ListenersTypeFlags);


#endif // Engine_Knob_h
