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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AnimationModuleEditor.h"

#include <stdexcept>

#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QKeyEvent>

#include "Serialization/WorkspaceSerialization.h"


#include "Gui/ActionShortcuts.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/Button.h"
#include "Gui/CurveGui.h"
#include "Gui/CurveWidget.h"
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/Label.h"
#include "Gui/KnobAnim.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/LineEdit.h"
#include "Gui/NodeGui.h"
#include "Gui/GuiMacros.h"

#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"

NATRON_NAMESPACE_ENTER;

////////////////////////// AnimationModuleEditor //////////////////////////

class AnimationModuleEditorPrivate
{
public:
    AnimationModuleEditorPrivate(AnimationModuleEditor *qq);

    /* attributes */
    AnimationModuleEditor *publicInterface;
    QVBoxLayout *mainLayout;


    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    Button* curveEditorButton;
    Button* dopeSheetButton;
    Button* displayBothButton;

    Label* knobLabel;
    LineEdit* knobExpressionLineEdit;
    Label* expressionResultLabel;


    QSplitter *treeAndViewSplitter;


    AnimationModuleTreeView *treeView;

    QSplitter *viewsSplitter;
    DopeSheetView* dopeSheetView;
    CurveWidget* curveView;

    AnimationModulePtr model;

};

AnimationModuleEditorPrivate::AnimationModuleEditorPrivate(AnimationModuleEditor *publicInterface)
: publicInterface(publicInterface)
, mainLayout(0)
, splitter(0)
, treeView(0)
, dopeSheetView(0)
, curveView(0)
, model()
{}

/**
 * @brief AnimationModuleEditor::AnimationModuleEditor
 *
 * Creates a AnimationModuleEditor.
 */
AnimationModuleEditor::AnimationModuleEditor(const std::string& scriptName,
                                 Gui *gui,
                                 const TimeLinePtr& timeline,
                                 QWidget *parent)
    : QWidget(parent),
    PanelWidget(scriptName, this, gui),
    _imp( new AnimationModuleEditorPrivate(this) )
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->splitter = new QSplitter(Qt::Horizontal, this);
    _imp->splitter->setHandleWidth(1);

    _imp->model = AnimationModule::create(gui, this, timeline);

    _imp->treeView = new AnimationModuleTreeView(_imp->model, gui, _imp->splitter);

    _imp->splitter->addWidget(_imp->treeView);
    _imp->splitter->setStretchFactor(0, 1);

    _imp->dopeSheetView = new DopeSheetView(_imp->model, _imp->treeView, gui, _imp->splitter);
    _imp->curveView = new CurveWidget(gui, _imp->model, _imp->splitter);
    _imp->splitter->addWidget(_imp->dopeSheetView);
    _imp->splitter->setStretchFactor(1, 5);

    _imp->mainLayout->addWidget(_imp->splitter);

    // Main model -> HierarchyView connections
    connect( _imp->model.get(), SIGNAL(nodeAdded(NodeAnimPtr)),
             _imp->treeView, SLOT(onNodeAdded(NodeAnimPtr)) );

    connect( _imp->model.get(), SIGNAL(nodeAboutToBeRemoved(NodeAnimPtr)),
             _imp->treeView, SLOT(onNodeAboutToBeRemoved(NodeAnimPtr)) );

    connect( _imp->model->getSelectionModel().get(), SIGNAL(selectionChanged(bool)),
             _imp->treeView, SLOT(onSelectionModelKeyframeSelectionChanged(bool)) );

    // Main model -> DopeSheetView connections
    connect( _imp->model.get(), SIGNAL(nodeAdded(NodeAnimPtr)),
             _imp->dopeSheetView, SLOT(onNodeAdded(NodeAnimPtr)) );

    connect( _imp->model.get(), SIGNAL(nodeAboutToBeRemoved(NodeAnimPtr)),
             _imp->dopeSheetView, SLOT(onNodeAboutToBeRemoved(NodeAnimPtr)) );

    connect( _imp->model.get(), SIGNAL(modelChanged()),
             _imp->dopeSheetView, SLOT(redraw()) );

    connect( _imp->model->getSelectionModel().get(), SIGNAL(selectionChanged(bool)),
             _imp->dopeSheetView, SLOT(onKeyframeSelectionChanged()) );

    // HierarchyView -> DopeSheetView connections
    connect( _imp->treeView->verticalScrollBar(), SIGNAL(valueChanged(int)),
             _imp->dopeSheetView, SLOT(onHierarchyViewScrollbarMoved(int)) );

    connect( _imp->treeView, SIGNAL(itemExpanded(QTreeWidgetItem*)),
             _imp->dopeSheetView, SLOT(onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem*)) );

    connect( _imp->treeView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
             _imp->dopeSheetView, SLOT(onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem*)) );
}

AnimationModuleEditor::~AnimationModuleEditor()
{}

void
AnimationModuleEditor::addNode(const NodeGuiPtr& nodeGui)
{
    _imp->model->addNode(nodeGui);
}

void
AnimationModuleEditor::removeNode(const NodeGuiPtr& node)
{
    _imp->model->removeNode(node);
}

void
AnimationModuleEditor::centerOn(double xMin,
                                double xMax)
{
    _imp->dopeSheetView->centerOn(xMin, xMax);
}

void
AnimationModuleEditor::refreshSelectionBboxAndRedrawView()
{
    _imp->dopeSheetView->refreshSelectionBboxAndRedraw();
    _imp->curveView->refreshSelectionBboxAndRedraw();
}

int
AnimationModuleEditor::getTimelineCurrentTime() const
{
    return getGui()->getApp()->getTimeLine()->currentFrame();
}

DopeSheetView*
AnimationModuleEditor::getDopesheetView() const
{
    return _imp->dopeSheetView;
}

CurveWidget*
AnimationModuleEditor::getCurveWidget() const
{
    return _imp->curveView;
}

AnimationModuleTreeView*
AnimationModuleEditor::getTreeView() const
{
    return _imp->treeView;
}

void
AnimationModuleEditor::setTreeWidgetWidth(int width)
{
    _imp->treeView->setCanResizeOtherWidget(false);
    QList<int> sizes;
    sizes << width << _imp->dopeSheetView->width();
    _imp->splitter->setSizes(sizes);
    _imp->treeView->setCanResizeOtherWidget(true);
}

int
AnimationModuleEditor::getTreeWidgetWidth() const
{
    QList<int> sizes = _imp->splitter->sizes();
    assert(sizes.size() > 0);

    return sizes.front();
}

void
AnimationModuleEditor::keyPressEvent(QKeyEvent* e)
{
    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();
    bool accept = true;

    if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRenameNode, modifiers, key) ) {
        _imp->model->renameSelectedNode();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionAnimationModuleEditorDeleteKeys, modifiers, key) ) {
        _imp->dopeSheetView->deleteSelectedKeyframes();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionAnimationModuleEditorFrameSelection, modifiers, key) ) {
        _imp->dopeSheetView->centerOnSelection();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionAnimationModuleEditorSelectAllKeyframes, modifiers, key) ) {
        _imp->dopeSheetView->onSelectedAllTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionCurveEditorConstant, modifiers, key) ) {
        _imp->dopeSheetView->constantInterpSelectedKeyframes();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionCurveEditorLinear, modifiers, key) ) {
        _imp->dopeSheetView->linearInterpSelectedKeyframes();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionCurveEditorSmooth, modifiers, key) ) {
        _imp->dopeSheetView->smoothInterpSelectedKeyframes();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionCurveEditorCatmullrom, modifiers, key) ) {
        _imp->dopeSheetView->catmullRomInterpSelectedKeyframes();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionCurveEditorCubic, modifiers, key) ) {
        _imp->dopeSheetView->cubicInterpSelectedKeyframes();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionCurveEditorHorizontal, modifiers, key) ) {
        _imp->dopeSheetView->horizontalInterpSelectedKeyframes();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionCurveEditorBreak, modifiers, key) ) {
        _imp->dopeSheetView->breakInterpSelectedKeyframes();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionAnimationModuleEditorCopySelectedKeyframes, modifiers, key) ) {
        _imp->dopeSheetView->copySelectedKeyframes();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionAnimationModuleEditorPasteKeyframes, modifiers, key) ) {
        _imp->dopeSheetView->pasteKeyframesRelative();
    } else if ( isKeybind(kShortcutGroupAnimationModuleEditor, kShortcutIDActionAnimationModuleEditorPasteKeyframesAbsolute, modifiers, key) ) {
        _imp->dopeSheetView->pasteKeyframesAbsolute();
    } else {
        accept = false;
    }
    if (accept) {
        takeClickFocus();
        e->accept();
    } else {
        handleUnCaughtKeyPressEvent(e);
        QWidget::keyPressEvent(e);
    }
}

void
AnimationModuleEditor::keyReleaseEvent(QKeyEvent* e)
{
    handleUnCaughtKeyUpEvent(e);
    QWidget::keyReleaseEvent(e);
}

void
AnimationModuleEditor::enterEvent(QEvent *e)
{
    enterEventBase();
    QWidget::enterEvent(e);
}

void
AnimationModuleEditor::leaveEvent(QEvent *e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

void
AnimationModuleEditor::onInputEventCalled()
{
    takeClickFocus();
}

QUndoStack*
AnimationModuleEditor::getUndoStack() const
{
    return _imp->model ? _imp->model->getUndoStack() : 0;
}

bool
AnimationModuleEditor::saveProjection(SERIALIZATION_NAMESPACE::ViewportData* data)
{
    if (!_imp->dopeSheetView->hasDrawnOnce()) {
        return false;
    }
    _imp->dopeSheetView->getProjection(&data->left, &data->bottom, &data->zoomFactor, &data->par);
    return true;
}

bool
AnimationModuleEditor::loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& data)
{
    _imp->dopeSheetView->setProjection(data.left, data.bottom, data.zoomFactor, data.par);
    return true;
}


void
AnimationModuleEditor::setSelectedCurveExpression(const QString& expression)
{
    std::list<CurveGuiPtr> curves = _imp->getSelectedCurves();
    if (curves.empty() || curves.size() > 1) {
        throw std::invalid_argument("Cannot set expression on multiple items");
    }
    KnobAnimPtr isKnobAnim = toKnobAnim(curves.front()->getItem());
    if (!isKnobAnim) {
        throw std::invalid_argument("Cannot set expression on non knob");
    }
    KnobIPtr knob = isKnobAnim->getInternalKnob();
    const CurveGuiPtr& curve = curves.front();
    DimIdx dimension = curve->getDimension();
    ViewIdx view = curve->getView();
    std::string exprResult;
    if ( !expression.isEmpty() ) {
        try {
            knob->validateExpression(expression.toStdString(), dimension, view, false /*hasRetVariable*/, &exprResult);
        } catch (...) {
            _imp->expressionResultLabel->setText( tr("Error") );
            return;
        }
    }
    pushUndoCommand( new SetExpressionCommand(knob, false /*hasRetVariable*/, dimension, view, expression.toStdString()) );
}


NATRON_NAMESPACE_EXIT;
