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

#ifndef AnimationModuleEditor_H
#define AnimationModuleEditor_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)

#if QT_VERSION >= 0x050000
#include <QtWidgets/QWidget>
#else
#include <QtGui/QWidget>
#endif

CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"
#include "Gui/AnimItemBase.h"
#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_ENTER

class AnimationModuleEditorPrivate;

/**
 * @class AnimationModuleEditor
 *
 * The AnimationModuleEditor class provides several widgets to edit keyframe animations
 * and video clips.
 *
 * It contains two main widgets : at left, the hierarchy view provides a tree
 * representation of the animated parameters (knobs) of each opened node.
 * At right, the dope sheet view is an OpenGL widget displaying horizontally the
 * keyframes of these knobs.
 *
 * Visually, the editor looks like a single tool with horizontally layered
 * datas.
 *
 * The user can select, move, delete the keyframes and set their interpolation.
 * He can move and trim clips too.
 *
 * The AnimationModuleEditor class acts like a facade. It's the only class visible
 * from the outside. It means that if you want to add a feature which require
 * some interaction with the other tools of Natron, you must add its "entry
 * point" here.
 *
 * For example the 'centerOn' function is called from the CurveWidget and
 * the TimeLineGui classes, and calls the 'centerOn' method of the
 * DopeSheetView class.
 *
 * Internally, this class is a composition of the following classes :
 * - DopeSheet -> it's the main model of the dope sheet editor
 * - HierarchyView -> describes the hierarchy view
 * - DopeSheetView -> describes the dope sheet view
 *
 * These two views query the model to display and modify data.
 */
class AnimationModuleEditor
    : public QWidget, public PanelWidget
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    AnimationModuleEditor(const std::string& scriptName,
                          Gui *gui,
                          const TimeLinePtr& timeline,
                          QWidget *parent = 0);

    virtual ~AnimationModuleEditor();

    enum AnimationModuleDisplayViewMode
    {
        eAnimationModuleDisplayViewModeStacked,
        eAnimationModuleDisplayViewModeCurveEditor,
    };

    AnimationModuleEditor::AnimationModuleDisplayViewMode getDisplayMode() const;

    /**
     * @brief Tells to the dope sheet model to add 'nodeGui' to the dope sheet
     * editor.
     *
     * The model will decide if the node should be accepted or discarded.
     */
    void addNode(const NodeGuiPtr& nodeGui);

    /**
     * @brief Tells to the dope sheet model to remove 'node' from the dope
     * sheet editor.
     */
    void removeNode(const NodeGuiPtr& node);

    /**
     * @brief Center the content of the dope sheet view on the timeline range
     * ['xMin', 'xMax'].
     */
    void centerOn(double xMin, double xMax);

    void refreshSelectionBboxAndRedrawView();

    TimeValue getTimelineCurrentTime() const;

    AnimationModuleView* getView() const;

    AnimationModuleTreeView* getTreeView() const;

    AnimationModulePtr getModel() const;

    void onInputEventCalled();

    bool isOnlyAnimatedItemsVisibleButtonChecked() const;

    virtual bool saveProjection(SERIALIZATION_NAMESPACE::ViewportData* data) OVERRIDE FINAL;

    virtual bool loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& data) OVERRIDE FINAL;

    void setSelectedCurveExpression(const QString& expression, ExpressionLanguageEnum lang);

    void zoomDisplayedView(int delta);

    virtual QIcon getIcon() const OVERRIDE FINAL;

    void refreshKeyframeWidgetsFromSelection();

    AnimItemDimViewKeyFrame getCurrentlySelectedKeyFrame() const;

    /**
     * @brief Set all tree items in the model to the given visibility except for the item passed
     * in argument.
     **/
    void setOtherItemsVisibility(const std::list<QTreeWidgetItem*>& items, bool visibile);

    /**
     * @brief Same as  setOtherItemsVisibility except that the visibility of other items
     * is computed from the first item different than the given item.
     **/
    void setOtherItemsVisibilityAuto(const std::list<QTreeWidgetItem*>& items);

    /**
     * @brief Set the given item visible
     **/
    void setItemVisibility(QTreeWidgetItem* item, bool visible, bool recurseOnParent);
    void setItemsVisibility(const std::list<QTreeWidgetItem*>& items, bool visible, bool recurseOnParent);
    void invertItemVisibility(QTreeWidgetItem* item);

Q_SIGNALS:

    void mustRefreshExpressionResultsLater();
    
public Q_SLOTS:

    void onMustRefreshExpressionResultsLaterReceived();

    void onExprLineEditFinished();

    void onDisplayViewClicked(bool clicked);

    void onShowOnlyAnimatedButtonClicked(bool clicked);

    void onSelectionModelSelectionChanged(bool recurse);

    void onRemoveSelectedKeyFramesActionTriggered();

    void onCopySelectedKeyFramesToClipBoardActionTriggered();

    void onPasteClipBoardKeyFramesActionTriggered();

    void onPasteClipBoardKeyFramesAbsoluteActionTriggered();

    void onSelectAllKeyFramesActionTriggered();

    void onSetInterpolationActionTriggered();

    void onCenterAllCurvesActionTriggered();

    void onCenterOnSelectedCurvesActionTriggered();

    void onKeyfameInterpolationChoiceMenuChanged(int index);

    void onLeftSlopeSpinBoxValueChanged(double value);

    void onRightSlopeSpinBoxValueChanged(double value);

    void onKeyframeValueLineEditEditFinished();

    void onKeyframeTimeSpinBoxValueChanged(double value);

    void onKeyframeValueSpinBoxValueChanged(double value);

    void onTimelineTimeChanged(SequenceTime time, int reason);

    void onTreeItemClicked(QTreeWidgetItem* item ,int col);

    void onTreeItemRightClicked(const QPoint& globalPos, QTreeWidgetItem* item);

    void onShowHideOtherItemsRightClickMenuActionTriggered();

    void onExprLanguageCurrentIndexChanged(int index);

private:

    bool setItemVisibilityRecursive(QTreeWidgetItem* item, bool visible, const std::list<QTreeWidgetItem*>& itemsToIgnore);
    void refreshKeyFrameWidgetsEnabledNess();

    virtual void notifyGuiClosing() OVERRIDE FINAL;
    virtual void enterEvent(QEvent *e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent *e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual boost::shared_ptr<QUndoStack> getUndoStack() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    boost::scoped_ptr<AnimationModuleEditorPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // AnimationModuleEditor_H
