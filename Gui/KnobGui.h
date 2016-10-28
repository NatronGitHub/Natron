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
#include "Engine/DimensionIdx.h"
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

NATRON_NAMESPACE_ENTER;

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
                   const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine);

    virtual bool shouldAddStretch() const { return true; }

    void pushUndoCommand(QUndoCommand* cmd);
    const QUndoCommand* getLastUndoCommand() const;

    QString getScriptNameHtml() const;

    // Apply the tooltip on the widget if non-null, otherwise return it
    QString toolTip(QWidget* w, ViewIdx view) const;

    bool hasToolTip() const;

    Gui* getGui() const;

    void enableRightClickMenu(QWidget* widget, DimSpec dimension, ViewIdx view);

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
    void setReadOnly_(bool readOnly, DimSpec dimension, ViewIdx view);

    int getKnobsCountOnSameLine() const;


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

    virtual void getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const OVERRIDE;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE;
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE;
    virtual void saveOpenGLContext() OVERRIDE;
    virtual void restoreOpenGLContext() OVERRIDE;

    ///Should set to the underlying knob the gui ptr
    virtual void setKnobGuiPointer() OVERRIDE FINAL;
    virtual bool isGuiFrozenForPlayback() const OVERRIDE FINAL;
    virtual void copyAnimationToClipboard(DimSpec dimension, ViewIdx view) const OVERRIDE FINAL;
    virtual void copyValuesToClipboard(DimSpec dimension, ViewIdx view) const OVERRIDE FINAL;
    virtual void copyLinkToClipboard(DimSpec dimension, ViewIdx view) const OVERRIDE FINAL;

    /**
     * @brief Check if the knob is secret by also checking the parent group visibility
     **/
    bool isSecretRecursive() const;

    KnobIPtr createDuplicateOnNode(const EffectInstancePtr& effect,
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

    void onDimensionNameChanged(int dimension);

    void onCurveAnimationChangedInternally(const std::list<double>& keysAdded,
                                           const std::list<double>& keysRemoved,
                                           ViewIdx view,
                                           DimIdx dimension);

    /**
     * @brief Called when the internal value held by the knob is changed. It calls updateGUI().
     **/
    void onMustRefreshGuiActionTriggered(ViewSetSpec view ,DimSpec dimension ,ValueChangedReasonEnum reason);

    void setSecret();

    void onViewerContextSecretChanged();

    void onRightClickClicked(const QPoint & pos);

    void showRightClickMenuForDimension(const QPoint & pos, DimSpec dimension, ViewIdx view);

    void setEnabledSlot();

    void hide();

    ///if index is != -1 then it will reinsert the knob at this index in the layout
    void show(int index = -1);

    void onSetKeyActionTriggered();

    void onRemoveKeyActionTriggered();

    void onShowInCurveEditorActionTriggered();

    void onRemoveAnimationActionTriggered();

    void onInterpolationActionTriggered();

    void onCopyValuesActionTriggered();

    void onCopyAnimationActionTriggered();

    void onCopyLinksActionTriggered();

    void onPasteActionTriggered();

    void onLinkToActionTriggered();
    void linkTo(DimSpec dimension, ViewIdx view);

    void onResetDefaultValuesActionTriggered();

    ///Actually restores all dimensions, the parameter is disregarded.
    void resetDefault(DimSpec dimension, ViewIdx view);

    void onSetDirty(bool d);

    void onAnimationLevelChanged(ViewSetSpec view, DimSpec dimension);

    void onAppendParamEditChanged(ValueChangedReasonEnum reason,
                              ValueChangedReturnCodeEnum setValueRetCode,
                              Variant v,
                              ViewSetSpec view,
                              DimSpec dim,
                              double time,
                              bool setKeyFrame);

    void onFrozenChanged(bool frozen);

    void onSetExprActionTriggered();

    void onClearExprActionTriggered();

    void onEditExprDialogFinished(bool accepted);

    void onExprChanged(DimIdx dimension, ViewIdx view);

    void onHelpChanged();

    void onHasModificationsChanged();

    void onLabelChanged();

    void onCreateAliasOnGroupActionTriggered();

Q_SIGNALS:

    // Emitted when the description label is clicked
    void labelClicked(bool);

protected:

    /**
     * @brief Called when the internal value held by the knob is changed, you must implement
     * it to update the interface to reflect the new value. You can query the new value
     * by calling knob->getValue()
     **/
    virtual void updateGUI(DimSpec dimension, ViewSetSpec view) = 0;
    virtual void addRightClickMenuEntries(QMenu* /*menu*/) {}

    virtual void reflectModificationsState() {}

private:

    static void getDimViewFromActionData(const QAction* action, ViewIdx* view, DimSpec* dimension);

    void refreshKnobWarningIndicatorVisibility();

    void updateGuiInternal(DimSpec dimension, ViewSetSpec view);

    void copyToClipBoard(KnobClipBoardType type, DimSpec dimension, ViewIdx view) const;

    void pasteClipBoard(DimSpec dimension, ViewIdx view);

    virtual void _hide() = 0;
    virtual void _show() = 0;
    virtual void setEnabled() = 0;
    virtual void setReadOnly(bool readOnly, DimSpec dimension, ViewIdx view) = 0;
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
    virtual void reflectAnimationLevel(DimIdx /*dimension*/, ViewIdx /*view*/, AnimationLevelEnum /*level*/)
    {
    }

    virtual void reflectExpressionState(DimIdx /*dimension*/,
                                        ViewIdx /*view*/,
                                        bool /*hasExpr*/) {}

    virtual void updateToolTip() {}

    void createAnimationMenu(QMenu* menu, DimSpec dimension, ViewIdx view);
    Menu* createInterpolationMenu(QMenu* menu, DimSpec dimension, ViewIdx view, bool isEnabled);


private:

    boost::scoped_ptr<KnobGuiPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_GUI_KNOBGUI_H
