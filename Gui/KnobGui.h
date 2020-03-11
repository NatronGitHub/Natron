/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

    enum KnobLayoutTypeEnum
    {
        // The knob widgets will be used in a paged layout such as the property panel
        eKnobLayoutTypePage,

        // The knob widgets will be used in an horizontal layout such as the viewer top toolbar
        eKnobLayoutTypeViewerUI,

        // The knob widgets will be used in a table such as the RotoPaint items table. Ideally,
        // there should be a single widget so the table is not cluttered
        eKnobLayoutTypeTableItemWidget
    };

private:
    struct MakeSharedEnabler;

    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    // used by make_shared
    KnobGui(const KnobIPtr& knob, KnobLayoutTypeEnum layoutType, KnobGuiContainerI* container);

    void initialize();


public:
    static KnobGuiPtr create(const KnobIPtr& knob, KnobLayoutTypeEnum layoutType, KnobGuiContainerI* container);

    virtual ~KnobGui() OVERRIDE;

    KnobGuiContainerI* getContainer();

    /**
     * @brief Calls deleteLater on all widgets owned by this knob. If the knob is on a same layout line of the other,
     * it will not re-adjust the layout. If removing a knob it is advised to wipe all knobs in the grid layout and
     * reconstruct them.
     **/
    void destroyWidgetsLater();

    void removeTabWidget();

    void setGuiRemoved();

    KnobIPtr getKnob() const;

    KnobLayoutTypeEnum getLayoutType() const;


    bool hasWidgetBeenCreated() const;

    /**
     * @brief When enabled, only the given dimension will be created for the knob.
     * This should be called before calling createGUI otherwise this will not be taken
     * into account.
     **/
    void setSingleDimensionalEnabled(bool enabled, DimIdx dimension);
    bool isSingleDimensionalEnabled(DimIdx* dimension) const;

    void createGUI(QWidget* parentWidget);


    void pushUndoCommand(const UndoCommandPtr& command);

    // Takes ownership, command is deleted when returning call
    void pushUndoCommand(UndoCommand* command);
    void pushUndoCommand(QUndoCommand* cmd);
    const QUndoCommand* getLastUndoCommand() const;

    QString getScriptNameHtml() const;

    // Apply the tooltip on the widget if non-null, otherwise return it
    QString toolTip(QWidget* w, ViewIdx view) const;

    bool hasToolTip() const;

    Gui* getGui() const;


    QWidget* getFieldContainer() const;
    QWidget* getLabelContainer() const;

    KnobGuiWidgetsPtr getWidgetsForView(ViewIdx view);

    bool isLabelOnSameColumn() const;

    /**
     * @brief Returns the row index of the knob in the layout.
     * The knob MUST be in the layout for this function to work,
     * that is, it must be visible.
     **/
   // int getActualIndexInLayout() const;

    bool isOnNewLine() const;


    int getKnobsCountOnSameLine() const;


    virtual bool isGuiFrozenForPlayback() const OVERRIDE FINAL;
    virtual void copyAnimationToClipboard(DimSpec dimension, ViewSetSpec view) const OVERRIDE FINAL;
    virtual void copyValuesToClipboard(DimSpec dimension, ViewSetSpec view) const OVERRIDE FINAL;
    virtual void copyLinkToClipboard(DimSpec dimension, ViewSetSpec view) const OVERRIDE FINAL;

    bool getAllViewsVisible() const;

    /**
     * @brief Check if the knob is secret by also checking the parent group visibility
     **/
    bool isSecretRecursive() const;

    KnobIPtr createDuplicateOnNode(const EffectInstancePtr& effect,
                                  bool makeAlias,
                                  const KnobPagePtr& page,
                                  const KnobGroupPtr& group,
                                  int indexInParent);



    void setWarningValue(KnobWarningEnum warn, const QString& value);



    /**
     * @brief Check if the given clipboard copy operation can be processed. 
     * @param showErrorDialog If true, show an error dialog upon failure.
     **/
    bool canPasteKnob(const KnobIPtr& fromKnob, KnobClipBoardType type, DimSpec fromDim, ViewSetSpec fromView, DimSpec targetDimension, ViewSetSpec targetView, bool showErrorDialog);

    /**
     * @brief Copy from the given fromKnob according to the given copy type.
     **/
    bool pasteKnob(const KnobIPtr& fromKnob, KnobClipBoardType type, DimSpec fromDim, ViewSetSpec fromView, DimSpec targetDimension, ViewSetSpec targetView);

    TabGroup* getTabWidget() const;
    TabGroup* getOrCreateTabWidget();

    void addSpacerItemAtEndOfLine();
    void removeSpacerItemAtEndOfLine();

    void reflectKnobSelectionState(bool selected);

    void removeAnimation(DimSpec dimension, ViewSetSpec view);

    void removeKeyAtCurrentTime(DimSpec dimension, ViewSetSpec view);

public Q_SLOTS:

    void onProjectViewsChanged();

    void onUnlinkActionTriggered();

    void onDimensionNameChanged(DimIdx dimension);

    void onInternalKnobLinksChanged();

    void onInternalKnobDimensionsVisibilityChanged(ViewSetSpec view);


    /**
     * @brief Called when the internal value held by the knob is changed. It calls updateGUI().
     **/
    void onMustRefreshGuiActionTriggered(ViewSetSpec view ,DimSpec dimension ,ValueChangedReasonEnum reason);

    void setSecret();

    void onViewerContextSecretChanged();

    void onRightClickClicked(const QPoint & pos);

    void showRightClickMenuForDimension(const QPoint & pos, DimSpec dimension, ViewSetSpec view);

    void setEnabledSlot();

    void hide();

    void show();

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
    void linkTo(DimSpec dimension, ViewSetSpec view);

    void onResetDefaultValuesActionTriggered();

    ///Actually restores all dimensions, the parameter is disregarded.
    void resetDefault(DimSpec dimension, ViewSetSpec view);

    void onKnobMultipleSelectionChanged(bool d);

    void onFrozenChanged(bool frozen);
    
    void onSetExprActionTriggered();

    void onClearExprActionTriggered();

    void onEditExprDialogFinished(bool accepted);

    void onExprChanged(DimIdx dimension, ViewIdx view);

    void onHelpChanged();

    void onHasModificationsChanged();

    void onLabelChanged();

    void onCreateAliasOnGroupActionTriggered();

    void onMultiViewUnfoldClicked(bool expanded);

    void onDoUpdateGuiLaterReceived();

    void onSplitViewActionTriggered();

    void onUnSplitViewActionTriggered();

    void onInternalKnobAvailableViewsChanged();

    void onDoUpdateModificationsStateLaterReceived();

    void onRefreshDimensionsVisibilityLaterReceived();

Q_SIGNALS:

    // Emitted when the description label is clicked
    void labelClicked(bool);

    void s_doUpdateGuiLater();

    void s_updateModificationsStateLater();

    void s_refreshDimensionsVisibilityLater();

private:

    KnobAnimPtr findKnobAnim() const;

    void setViewEnabledInternal(const std::vector<bool>& perDimEnabled, ViewIdx view);

    void refreshModificationsStateNow();

    void refreshGuiNow();

    void createViewContainers(ViewIdx view);

    static void getDimViewFromActionData(const QAction* action, ViewSetSpec* view, DimSpec* dimension);

    void refreshKnobWarningIndicatorVisibility();

    void copyToClipBoard(KnobClipBoardType type, DimSpec dimension, ViewSetSpec view) const;

    void pasteClipBoard(DimSpec dimension, ViewSetSpec view);

    void createAnimationMenu(QMenu* menu, DimSpec dimension, ViewSetSpec view);
    Menu* createInterpolationMenu(QMenu* menu, DimSpec dimension, ViewSetSpec view, bool isEnabled);


private:

    friend struct KnobGuiPrivate;
    boost::scoped_ptr<KnobGuiPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_KNOBGUI_H
