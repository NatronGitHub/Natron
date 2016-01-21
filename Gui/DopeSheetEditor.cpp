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

#include "DopeSheetEditor.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QKeyEvent>

#include "Gui/ActionShortcuts.h"
#include "Gui/DopeSheet.h"
#include "Gui/DopeSheetHierarchyView.h"
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/NodeGui.h"
#include "Gui/GuiMacros.h"
#include "Engine/TimeLine.h"

NATRON_NAMESPACE_ENTER;

////////////////////////// DopeSheetEditor //////////////////////////

class DopeSheetEditorPrivate
{
public:
    DopeSheetEditorPrivate(DopeSheetEditor *qq);

    /* attributes */
    DopeSheetEditor *q_ptr;

    QVBoxLayout *mainLayout;

    DopeSheet *model;

    QSplitter *splitter;
    HierarchyView *hierarchyView;
    DopeSheetView *dopeSheetView;

};

DopeSheetEditorPrivate::DopeSheetEditorPrivate(DopeSheetEditor *qq)  :
    q_ptr(qq),
    mainLayout(0),
    model(0),
    splitter(0),
    hierarchyView(0),
    dopeSheetView(0)
{}

/**
 * @brief DopeSheetEditor::DopeSheetEditor
 *
 * Creates a DopeSheetEditor.
 */
DopeSheetEditor::DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent) :
    QWidget(parent),
    PanelWidget(this,gui),
    _imp(new DopeSheetEditorPrivate(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->splitter = new QSplitter(Qt::Horizontal, this);
    _imp->splitter->setHandleWidth(1);

    _imp->model = new DopeSheet(gui, this, timeline);

    _imp->hierarchyView = new HierarchyView(_imp->model, gui, _imp->splitter);

    _imp->splitter->addWidget(_imp->hierarchyView);
    _imp->splitter->setStretchFactor(0, 1);

    _imp->dopeSheetView = new DopeSheetView(_imp->model, _imp->hierarchyView, gui, timeline, _imp->splitter);

    _imp->splitter->addWidget(_imp->dopeSheetView);
    _imp->splitter->setStretchFactor(1, 5);

    _imp->mainLayout->addWidget(_imp->splitter);

    // Main model -> HierarchyView connections
    connect(_imp->model, SIGNAL(nodeAdded(DSNode *)),
            _imp->hierarchyView, SLOT(onNodeAdded(DSNode *)));

    connect(_imp->model, SIGNAL(nodeAboutToBeRemoved(DSNode *)),
            _imp->hierarchyView, SLOT(onNodeAboutToBeRemoved(DSNode *)));

    connect(_imp->model, SIGNAL(keyframeSetOrRemoved(DSKnob *)),
            _imp->hierarchyView, SLOT(onKeyframeSetOrRemoved(DSKnob *)));

    connect(_imp->model->getSelectionModel(), SIGNAL(keyframeSelectionChangedFromModel(bool)),
            _imp->hierarchyView, SLOT(onKeyframeSelectionChanged(bool)));

    // Main model -> DopeSheetView connections
    connect(_imp->model, SIGNAL(nodeAdded(DSNode*)),
            _imp->dopeSheetView, SLOT(onNodeAdded(DSNode *)));

    connect(_imp->model, SIGNAL(nodeAboutToBeRemoved(DSNode*)),
            _imp->dopeSheetView, SLOT(onNodeAboutToBeRemoved(DSNode *)));

    connect(_imp->model, SIGNAL(modelChanged()),
            _imp->dopeSheetView, SLOT(redraw()));

    connect(_imp->model->getSelectionModel(), SIGNAL(keyframeSelectionChangedFromModel(bool)),
            _imp->dopeSheetView, SLOT(onKeyframeSelectionChanged()));

    // HierarchyView -> DopeSheetView connections
    connect(_imp->hierarchyView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            _imp->dopeSheetView, SLOT(onHierarchyViewScrollbarMoved(int)));

    connect(_imp->hierarchyView, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            _imp->dopeSheetView, SLOT(onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem*)));

    connect(_imp->hierarchyView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            _imp->dopeSheetView, SLOT(onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem*)));
}

DopeSheetEditor::~DopeSheetEditor()
{}

void DopeSheetEditor::addNode(boost::shared_ptr<NodeGui> nodeGui)
{
    _imp->model->addNode(nodeGui);
}

void DopeSheetEditor::removeNode(NodeGui *node)
{
    _imp->model->removeNode(node);
}

void DopeSheetEditor::centerOn(double xMin, double xMax)
{
    _imp->dopeSheetView->centerOn(xMin, xMax);
}

void
DopeSheetEditor::refreshSelectionBboxAndRedrawView()
{
    _imp->dopeSheetView->refreshSelectionBboxAndRedraw();
}

int
DopeSheetEditor::getTimelineCurrentTime() const
{
    return getGui()->getApp()->getTimeLine()->currentFrame();
}

DopeSheetView*
DopeSheetEditor::getDopesheetView() const
{
    return _imp->dopeSheetView;
}

void
DopeSheetEditor::setTreeWidgetWidth(int width)
{
    _imp->hierarchyView->setCanResizeOtherWidget(false);
    QList<int> sizes;
    sizes << width << _imp->dopeSheetView->width();
    _imp->splitter->setSizes(sizes);
    _imp->hierarchyView->setCanResizeOtherWidget(true);
}

int
DopeSheetEditor::getTreeWidgetWidth() const
{
    QList<int> sizes = _imp->splitter->sizes();
    assert(sizes.size() > 0);
    return sizes.front();
}

void
DopeSheetEditor::keyPressEvent(QKeyEvent* e)
{
    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();
 
    bool accept = true;
    if (isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRenameNode, modifiers, key)) {
        _imp->model->renameSelectedNode();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorDeleteKeys, modifiers, key)) {
        _imp->dopeSheetView->deleteSelectedKeyframes();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorFrameSelection, modifiers, key)) {
        _imp->dopeSheetView->centerOnSelection();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorSelectAllKeyframes, modifiers, key)) {
        _imp->dopeSheetView->onSelectedAllTriggered();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorConstant, modifiers, key)) {
        _imp->dopeSheetView->constantInterpSelectedKeyframes();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorLinear, modifiers, key)) {
        _imp->dopeSheetView->linearInterpSelectedKeyframes();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorSmooth, modifiers, key)) {
        _imp->dopeSheetView->smoothInterpSelectedKeyframes();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorCatmullrom, modifiers, key)) {
        _imp->dopeSheetView->catmullRomInterpSelectedKeyframes();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorCubic, modifiers, key)) {
        _imp->dopeSheetView->cubicInterpSelectedKeyframes();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorHorizontal, modifiers, key)) {
        _imp->dopeSheetView->horizontalInterpSelectedKeyframes();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorBreak, modifiers, key)) {
        _imp->dopeSheetView->breakInterpSelectedKeyframes();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorCopySelectedKeyframes, modifiers, key)) {
        _imp->dopeSheetView->copySelectedKeyframes();
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorPasteKeyframes, modifiers, key)) {
        _imp->dopeSheetView->pasteKeyframes();
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
DopeSheetEditor::keyReleaseEvent(QKeyEvent* e)
{
    handleUnCaughtKeyUpEvent(e);
    QWidget::keyReleaseEvent(e);
}

void DopeSheetEditor::enterEvent(QEvent *e)
{
    enterEventBase();
    QWidget::enterEvent(e);
}

void DopeSheetEditor::leaveEvent(QEvent *e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

void
DopeSheetEditor::onInputEventCalled()
{
    takeClickFocus();
}

NATRON_NAMESPACE_EXIT;
