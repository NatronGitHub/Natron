/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <cfloat> // DBL_MAX
#include <climits> // INT_MAX

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/KnobGuiI.h"

// Qt
class QUndoCommand; //used by KnobGui
class QVBoxLayout; //used by KnobGui
class QHBoxLayout; //used by KnobGui
class QGridLayout;
class QMenu;
namespace Natron {
class ClickableLabel;
class Label;
class EffectInstance;
}
class QString;

// Engine
class Variant; //used by KnobGui
class KeyFrame;

// Gui
class ComboBox;
class Button;
class AnimationButton; //used by KnobGui
class DockablePanel; //used by KnobGui
class Gui;
class NodeGui;

#define SLIDER_MAX_RANGE 100000

//Define this if you want the spinbox to clamp to the plugin defined range
//#define SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT

struct KnobGuiPrivate;

class KnobGui
    : public QObject,public KnobGuiI
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    KnobGui(const boost::shared_ptr<KnobI>& knob,
            DockablePanel* container);

    virtual ~KnobGui() OVERRIDE;
    
    DockablePanel* getContainer();
    
    void removeGui();
    
    void callDeleteLater();
    
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
    virtual boost::shared_ptr<KnobI> getKnob() const = 0;

    bool triggerNewLine() const;

    void turnOffNewLine();

    /*Set the spacing between items in the layout*/
    void setSpacingBetweenItems(int spacing);

    int getSpacingBetweenItems() const;

    bool hasWidgetBeenCreated() const;

    void createGUI(QGridLayout* containerLayout,
                   QWidget* fieldContainer,
                   Natron::ClickableLabel* label,
                   QHBoxLayout* layout,
                   bool isOnNewLine,
                   const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine);

    virtual bool shouldAddStretch() const { return true; }

    void pushUndoCommand(QUndoCommand* cmd);

    const QUndoCommand* getLastUndoCommand() const;


    void setKeyframe(double time,int dimension);
    void setKeyframe(double time,const KeyFrame& key,int dimension);
    void removeKeyFrame(double time,int dimension);
    
    void setKeyframes(const std::vector<KeyFrame>& keys, int dimension);
    void removeKeyframes(const std::vector<KeyFrame>& keys, int dimension);

    QString getScriptNameHtml() const;

    QString toolTip() const;

    bool hasToolTip() const;

    Gui* getGui() const;

    void enableRightClickMenu(QWidget* widget,int dimension);

    virtual bool isLabelVisible() const;

    QWidget* getFieldContainer() const;

    /**
     * @brief Returns the row index of the knob in the layout.
     * The knob MUST be in the layout for this function to work,
     * that is, it must be visible.
     **/
    int getActualIndexInLayout() const;

    bool isOnNewLine() const;

    ////calls setReadOnly and also set the label black
    void setReadOnly_(bool readOnly,int dimension);

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
                 Natron::ValueChangedReasonEnum reason)
    {
        KnobHelper::ValueChangedReturnCodeEnum ret = KnobHelper::eValueChangedReturnCodeNothingChanged;
        Knob<T>* knob = dynamic_cast<Knob<T>*>( getKnob().get() );
        assert(knob);
        if (knob) {
            ret = knob->setValue(v,dimension,reason,newKey);
        }
        if (ret > 0 && ret != KnobHelper::eValueChangedReturnCodeNothingChanged && reason == Natron::eValueChangedReasonUserEdited) {
            assert(newKey);
            if (ret == KnobHelper::eValueChangedReturnCodeKeyframeAdded) {
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
    void setValueAtTime(int dimension,
                        const T & v,
                        double time,
                        KeyFrame* newKey,
                        bool refreshGui,
                        Natron::ValueChangedReasonEnum reason)
    {
        
        Knob<T>* knob = dynamic_cast<Knob<T>*>( getKnob().get() );
        assert(knob);
        bool addedKey  = false;
        if (knob) {
            addedKey = knob->setValueAtTime(time,v,dimension,reason,newKey);
        }
        if ((knob) && reason == Natron::eValueChangedReasonUserEdited) {
            assert(newKey);
            setKeyframeMarkerOnTimeline( newKey->getTime() );
        }
        if (refreshGui) {
            updateGUI(dimension);
        }
    }

    virtual void swapOpenGLBuffers() OVERRIDE FINAL;
    virtual void redraw() OVERRIDE FINAL;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;
    
    ///Should set to the underlying knob the gui ptr
    virtual void setKnobGuiPointer() OVERRIDE FINAL;
    
    virtual void onKnobDeletion() OVERRIDE FINAL;

    virtual bool isGuiFrozenForPlayback() const OVERRIDE FINAL;

    virtual void copyAnimationToClipboard() const OVERRIDE FINAL;

    void copyValuesToCliboard();

    void pasteValuesFromClipboard();
    
    virtual boost::shared_ptr<Curve> getCurve(int dimension) const OVERRIDE FINAL;

    /**
     * @brief Check if the knob is secret by also checking the parent group visibility
     **/
    bool isSecretRecursive() const;
    
    void createDuplicateOnNode(Natron::EffectInstance* effect,bool linkExpression);


    static bool shouldSliderBeVisible(int sliderMin, int sliderMax)
    {
        return (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < INT_MAX) && (sliderMin > INT_MIN);
    }

    static bool shouldSliderBeVisible(double sliderMin, double sliderMax)
    {
        return (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < DBL_MAX) && (sliderMin > -DBL_MAX);
    }

public Q_SLOTS:
    
    void onUnlinkActionTriggered();

    void onRedrawGuiCurve(int reason, int dimension);
    
    /**
     * @brief Called when the internal value held by the knob is changed. It calls updateGUI().
     **/
    void onInternalValueChanged(int dimension,int reason);

    void onInternalKeySet(double time,int dimension,int reason,bool added);

    void onInternalKeyRemoved(double time,int dimension,int reason);
    
    void onMultipleKeySet(const std::list<double>& keys,int dimension, int reason);

    void onInternalAnimationAboutToBeRemoved(int dimension);
    
    void onInternalAnimationRemoved();
    
    ///Handler when a keyframe is moved in the curve editor/dope sheet
    void onKeyFrameMoved(int dimension,double oldTime,double newTime);

    void setSecret();

    void showAnimationMenu();

    void onRightClickClicked(const QPoint & pos);

    void showRightClickMenuForDimension(const QPoint & pos,int dimension);

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

    void onPasteValuesActionTriggered();

    void onCopyAnimationActionTriggered();

    void onPasteAnimationActionTriggered();

    void onLinkToActionTriggered();
    void linkTo(int dimension);

    void onResetDefaultValuesActionTriggered();

    ///Actually restores all dimensions, the parameter is disregarded.
    void resetDefault(int dimension);

    void onKnobSlavedChanged(int dimension,bool b);

    void onSetValueUsingUndoStack(const Variant & v,int dim);

    void onSetDirty(bool d);

    void onAnimationLevelChanged(int dim,int level);

    void onAppendParamEditChanged(int reason,const Variant & v,int dim,double time,bool createNewCommand,bool setKeyFrame);

    void onFrozenChanged(bool frozen);

    void updateCurveEditorKeyframes();

    void onSetExprActionTriggered();
    
    void onClearExprActionTriggered();
    
    void onEditExprDialogFinished();
    
    void onExprChanged(int dimension);
    
    void onHelpChanged();
    
    void onHasModificationsChanged();
    
    void onLabelChanged();
    
    void onCreateMasterOnGroupActionTriggered();
    
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

    void updateGuiInternal(int dimension);

    void copyToClipBoard(bool copyAnimation) const;

    void pasteClipBoard();

    virtual void _hide() = 0;
    virtual void _show() = 0;
    virtual void setEnabled() = 0;
    virtual void setReadOnly(bool readOnly,int dimension) = 0;
    virtual void setDirty(bool dirty) = 0;
    
    virtual void onLabelChangedInternal() {}

    /**
     * @brief Must fill the horizontal layout with all the widgets composing the knob.
     **/
    virtual void createWidget(QHBoxLayout* layout) = 0;


    /*Called right away after updateGUI(). Depending in the animation level
       the widget for the knob could display its gui a bit differently.
     */
    virtual void reflectAnimationLevel(int /*dimension*/,
                                       Natron::AnimationLevelEnum /*level*/)
    {
    }
    
    virtual void reflectExpressionState(int /*dimension*/,bool /*hasExpr*/) {}

    virtual void updateToolTip() {}
    
    void createAnimationMenu(QMenu* menu,int dimension);

    void createAnimationButton(QHBoxLayout* layout);


    void setInterpolationForDimensions(const std::vector<int> & dimensions,Natron::KeyframeTypeEnum interp);

private:

    boost::scoped_ptr<KnobGuiPrivate> _imp;
};


#endif // NATRON_GUI_KNOBGUI_H
