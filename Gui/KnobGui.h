/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef NATRON_GUI_KNOBGUI_H
#define NATRON_GUI_KNOBGUI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cfloat> // DBL_MAX
#include <climits> // INT_MAX

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Global/Enums.h"
#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/KnobGuiI.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"
#include "Engine/Markdown.h"

#include "Gui/KnobGuiContainerI.h"
#include "Gui/GuiFwd.h"


#define SLIDER_MAX_RANGE 100000

//Define this if you want the spinbox to clamp to the plugin defined range
//#define SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT

NATRON_NAMESPACE_ENTER

struct KnobGuiPrivate;

class KnobGui
    : public QObject
    , public KnobGuiI
    , public boost::enable_shared_from_this<KnobGui>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum KnobWarningEnum
    {
        eKnobWarningExpressionInvalid = 0,
        eKnobWarningChoiceMenuOutOfDate
    };

public:
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobGui(const KnobIPtr& knob,
            KnobGuiContainerI* container);

public:
    virtual ~KnobGui() OVERRIDE;

    void initialize();

    KnobGuiContainerI* getContainer();

    void removeGui();

    void setGuiRemoved();

protected:
    /**
     * @brief Override this to delete all user interface created for this knob
     **/
    virtual void removeSpecificGui() = 0;

public:

    /**
     * @brief This function must return a pointer to the internal knob.
     * This is virtual as it is easier to hold the knob in the derived class
     * avoiding many dynamic_cast in the deriving class.
     **/
    virtual KnobIPtr getKnob() const = 0;

    bool isViewerUIKnob() const;

    bool triggerNewLine() const;

    void turnOffNewLine();

    /*Set the spacing between items in the layout*/
    void setSpacingBetweenItems(int spacing);

    int getSpacingBetweenItems() const;

    bool hasWidgetBeenCreated() const;

    void createGUI(QWidget* fieldContainer,
                   QWidget* labelContainer,
                   KnobClickableLabel* label,
                   Label* warningIndicator,
                   QHBoxLayout* layout,
                   bool isOnNewLine,
                   int lastKnobSpacing,
                   const std::vector<KnobIPtr> & knobsOnSameLine);

    virtual bool shouldAddStretch() const { return true; }

    void pushUndoCommand(QUndoCommand* cmd);
    const QUndoCommand* getLastUndoCommand() const;


    void setKeyframe(double time, int dimension, ViewSpec view);
    void setKeyframe(double time, const KeyFrame& key, int dimension, ViewSpec view);
    void removeKeyFrame(double time, int dimension, ViewSpec view);

    void setKeyframes(const std::vector<KeyFrame>& keys, int dimension, ViewSpec view);
    void removeKeyframes(const std::vector<KeyFrame>& keys, int dimension, ViewSpec view);

    QString getScriptNameHtml() const;

    QString toolTip() const;

    bool hasToolTip() const;

    Gui* getGui() const;

    void enableRightClickMenu(QWidget* widget, int dimension);

    virtual bool shouldCreateLabel() const;
    virtual bool isLabelOnSameColumn() const;
    virtual bool isLabelBold() const;
    virtual std::string getDescriptionLabel() const;
    QWidget* getFieldContainer() const;

    /**
     * @brief Returns the row index of the knob in the layout.
     * The knob MUST be in the layout for this function to work,
     * that is, it must be visible.
     **/
    int getActualIndexInLayout() const;

    bool isOnNewLine() const;

    ////calls setReadOnly and also set the label black
    void setReadOnly_(bool readOnly, int dimension);

    int getKnobsCountOnSameLine() const;


    void removeAllKeyframeMarkersOnTimeline(int dim);
    void setAllKeyframeMarkersOnTimeline(int dim);
    void setKeyframeMarkerOnTimeline(double time);

    /*This function is used by KnobUndoCommand. Calling this in a onInternalValueChanged/valueChanged
       signal/slot sequence can cause an infinite loop.*/
    template<typename T>
    int setValue(int dimension,
                 const T & v,
                 KeyFrame* newKey,
                 bool refreshGui,
                 ValueChangedReasonEnum reason)
    {
        KnobHelper::ValueChangedReturnCodeEnum ret = KnobHelper::eValueChangedReturnCodeNothingChanged;

        Knob<T>* knob = dynamic_cast<Knob<T>*>( getKnob().get() );
        assert(knob);
        if (knob) {
            ret = knob->setValue(v, ViewSpec::current(), dimension, reason, newKey);
        }
        if ( (ret > 0) && (ret != KnobHelper::eValueChangedReturnCodeNothingChanged) && (reason == eValueChangedReasonUserEdited) ) {
            assert(newKey != nullptr);
            if (ret == KnobHelper::eValueChangedReturnCodeKeyframeAdded && newKey != nullptr) {
                setKeyframeMarkerOnTimeline( newKey->getTime() );
            }
            Q_EMIT keyFrameSet();
        }
        if (refreshGui) {
            updateGUI(dimension);
        }

        return (int)ret;
    }

    /*This function is used by KnobUndoCommand. Calling this in a onInternalValueChanged/valueChanged
       signal/slot sequence can cause an infinite loop.*/
    template<typename T>
    bool setValueAtTime(int dimension,
                        const T & v,
                        double time,
                        ViewSpec view,
                        KeyFrame* newKey,
                        bool refreshGui,
                        ValueChangedReasonEnum reason)
    {
        Knob<T>* knob = dynamic_cast<Knob<T>*>( getKnob().get() );
        assert(knob);
        KnobHelper::ValueChangedReturnCodeEnum addedKey = KnobHelper::eValueChangedReturnCodeNothingChanged;
        if (knob) {
            addedKey = knob->setValueAtTime(time, v, view, dimension, reason, newKey);
        }
        if ( (knob) && (reason == eValueChangedReasonUserEdited) ) {
            assert(newKey);
            setKeyframeMarkerOnTimeline( newKey->getTime() );
        }
        if (refreshGui) {
            updateGUI(dimension);
        }

        return ( (addedKey != KnobHelper::eValueChangedReturnCodeNoKeyframeAdded) &&
                 (addedKey != KnobHelper::eValueChangedReturnCodeNothingChanged) );
    }

    virtual RectD getViewportRect() const OVERRIDE FINAL;
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL;
    /**
     * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
     **/
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE FINAL;

    /**
     * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
     **/
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE FINAL;


    /**
     * @brief Returns the font height, i.e: the height of the highest letter for this font
     **/
    virtual int getWidgetFontHeight() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns for a string the estimated pixel size it would take on the widget
     **/
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void swapOpenGLBuffers() OVERRIDE;
    virtual void redraw() OVERRIDE;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE;
#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE;
#endif
   virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE;
    virtual void saveOpenGLContext() OVERRIDE;
    virtual void restoreOpenGLContext() OVERRIDE;

    ///Should set to the underlying knob the gui ptr
    virtual void setKnobGuiPointer() OVERRIDE FINAL;
    virtual bool isGuiFrozenForPlayback() const OVERRIDE FINAL;
    virtual void copyAnimationToClipboard(int dimension = -1) const OVERRIDE FINAL;
    virtual void copyValuesToClipboard(int dimension = -1) const OVERRIDE FINAL;
    virtual void copyLinkToClipboard(int dimension = -1) const OVERRIDE FINAL;
    virtual CurvePtr getCurve(ViewSpec view, int dimension) const OVERRIDE FINAL;

    /**
     * @brief Check if the knob is secret by also checking the parent group visibility
     **/
    bool isSecretRecursive() const;

    KnobIPtr createDuplicateOnNode(EffectInstance* effect,
                                  bool makeAlias,
                                  const KnobPagePtr& page,
                                  const KnobGroupPtr& group,
                                  int indexInParent);


    static bool shouldSliderBeVisible(int sliderMin,
                                      int sliderMax)
    {
        return (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < INT_MAX) && (sliderMin > INT_MIN);
    }

    static bool shouldSliderBeVisible(double sliderMin,
                                      double sliderMax)
    {
        return (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < DBL_MAX) && (sliderMin > -DBL_MAX);
    }

    virtual bool getAllDimensionsVisible() const OVERRIDE { return true; }

    void setWarningValue(KnobWarningEnum warn, const QString& value);

public Q_SLOTS:

    void onRemoveAliasLinkActionTriggered();

    void onUnlinkActionTriggered();

    void onRedrawGuiCurve(int reason, ViewSpec view, int dimension);

    void onDimensionNameChanged(int dimension);

    /**
     * @brief Called when the internal value held by the knob is changed. It calls updateGUI().
     **/
    void onInternalValueChanged(ViewSpec view, int dimension, int reason);

    void onInternalKeySet(double time, ViewSpec view, int dimension, int reason, bool added);

    void onInternalKeyRemoved(double time, ViewSpec view, int dimension, int reason);

    void onMultipleKeySet(const std::list<double>& keys, ViewSpec view, int dimension, int reason);

    void onMultipleKeyRemoved(const std::list<double>& keys, ViewSpec view, int dimension, int reason);

    void onInternalAnimationAboutToBeRemoved(ViewSpec view, int dimension);

    void onInternalAnimationRemoved();

    ///Handler when a keyframe is moved in the curve editor/dope sheet
    void onKeyFrameMoved(ViewSpec view, int dimension, double oldTime, double newTime);

    void setSecret();

    void onViewerContextSecretChanged();

    void onRightClickClicked(const QPoint & pos);

    void showRightClickMenuForDimension(const QPoint & pos, int dimension);

    void setEnabledSlot();

    void hide();

    ///if index is != -1 then it will reinsert the knob at this index in the layout
    void show(int index = -1);

    void onSetKeyActionTriggered();

    void onRemoveKeyActionTriggered();

    void onShowInCurveEditorActionTriggered();

    void onRemoveAnimationActionTriggered();

    void onConstantInterpActionTriggered();

    void onLinearInterpActionTriggered();

    void onSmoothInterpActionTriggered();

    void onCatmullromInterpActionTriggered();

    void onCubicInterpActionTriggered();

    void onHorizontalInterpActionTriggered();

    void onCopyValuesActionTriggered();

    void onCopyAnimationActionTriggered();

    void onCopyLinksActionTriggered();

    void onPasteActionTriggered();

    void onLinkToActionTriggered();
    void linkTo(int dimension);

    void onResetDefaultValuesActionTriggered();

    ///Actually restores all dimensions, the parameter is disregarded.
    void resetDefault(int dimension);

    void onKnobSlavedChanged(int dimension, bool b);

    void onSetValueUsingUndoStack(const Variant & v, ViewSpec view, int dim);

    void onSetDirty(bool d);

    void onAnimationLevelChanged(ViewSpec view, int dim);

    void onAppendParamEditChanged(int reason, const Variant & v, ViewSpec view, int dim, double time, bool createNewCommand, bool setKeyFrame);

    void onFrozenChanged(bool frozen);

    void updateCurveEditorKeyframes();

    void onSetExprActionTriggered();

    void onClearExprActionTriggered();

    void onEditExprDialogFinished();

    void onExprChanged(int dimension);

    void onHelpChanged();

    void onHasModificationsChanged();

    void onLabelChanged();

    void onCreateAliasOnGroupActionTriggered();

Q_SIGNALS:

    void knobUndoneChange();

    void knobRedoneChange();

    void refreshCurveEditor();

    void refreshDopeSheet();

    void expressionChanged();

    /**
     *@brief Emitted whenever a keyframe is set by the user or by the plugin.
     **/
    void keyFrameSet();

    /**
     *@brief Emitted whenever a keyframe is removed by the user or by the plugin.
     **/
    void keyFrameRemoved();

    /**
     *@brief Emitted whenever a keyframe's interpolation method is changed by the user or by the plugin.
     **/
    void keyInterpolationChanged();

    ///emitted when the description label is clicked
    void labelClicked(bool);

protected:

    /**
     * @brief Called when the internal value held by the knob is changed, you must implement
     * it to update the interface to reflect the new value. You can query the new value
     * by calling knob->getValue()
     * The dimension is either -1 indicating that all dimensions should be updated or the dimension index.
     **/
    virtual void updateGUI(int dimension) = 0;
    virtual void addRightClickMenuEntries(QMenu* /*menu*/) {}

    virtual void reflectModificationsState() {}

private:

    void refreshKnobWarningIndicatorVisibility();

    void updateGuiInternal(int dimension);

    void copyToClipBoard(KnobClipBoardType type, int dimension) const;

    void pasteClipBoard(int targetDimension);

    virtual void _hide() = 0;
    virtual void _show() = 0;
    virtual void setEnabled() = 0;
    virtual void setReadOnly(bool readOnly, int dimension) = 0;
    virtual void setDirty(bool dirty) = 0;
    virtual void onLabelChangedInternal() {}

    virtual void refreshDimensionName(int /*dim*/) {}

    /**
     * @brief Must fill the horizontal layout with all the widgets composing the knob.
     **/
    virtual void createWidget(QHBoxLayout* layout) = 0;


    /*Called right away after updateGUI(). Depending in the animation level
       the widget for the knob could display its gui a bit differently.
     */
    virtual void reflectAnimationLevel(int /*dimension*/,
                                       AnimationLevelEnum /*level*/)
    {
    }

    virtual void reflectExpressionState(int /*dimension*/,
                                        bool /*hasExpr*/) {}

    virtual void updateToolTip() {}

    void createAnimationMenu(QMenu* menu, int dimension);
    Menu* createInterpolationMenu(QMenu* menu, int dimension, bool isEnabled);

    void setInterpolationForDimensions(QAction* action, KeyframeTypeEnum interp);

private:

    boost::scoped_ptr<KnobGuiPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_KNOBGUI_H
