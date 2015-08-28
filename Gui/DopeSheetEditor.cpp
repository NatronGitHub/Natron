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

#include "Gui/DopeSheet.h"
#include "Gui/DopeSheetHierarchyView.h"
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeGui.h"
#include "Engine/TimeLine.h"

////////////////////////// DopeSheetEditor //////////////////////////

class DopeSheetEditorPrivate
{
public:
    DopeSheetEditorPrivate(DopeSheetEditor *qq, Gui *gui);

    /* attributes */
    DopeSheetEditor *q_ptr;
    Gui *gui;

    QVBoxLayout *mainLayout;
    QHBoxLayout *helpersLayout;

    DopeSheet *model;

    QSplitter *splitter;
    HierarchyView *hierarchyView;
    DopeSheetView *dopeSheetView;

    QPushButton *toggleTripleSyncBtn;
};

DopeSheetEditorPrivate::DopeSheetEditorPrivate(DopeSheetEditor *qq, Gui *gui)  :
    q_ptr(qq),
    gui(gui),
    mainLayout(0),
    helpersLayout(0),
    model(0),
    splitter(0),
    hierarchyView(0),
    dopeSheetView(0),
    toggleTripleSyncBtn(0)
{}

/**
 * @brief DopeSheetEditor::DopeSheetEditor
 *
 * Creates a DopeSheetEditor.
 */
DopeSheetEditor::DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent) :
    QWidget(parent),
    ScriptObject(),
    _imp(new DopeSheetEditorPrivate(this, gui))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->helpersLayout = new QHBoxLayout();
    _imp->helpersLayout->setContentsMargins(0, 0, 0, 0);

    _imp->toggleTripleSyncBtn = new QPushButton(this);

    QPixmap tripleSyncBtnPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_UNLOCKED, &tripleSyncBtnPix);

    _imp->toggleTripleSyncBtn->setIcon(QIcon(tripleSyncBtnPix));
    _imp->toggleTripleSyncBtn->setToolTip(tr("Toggle triple synchronization"));
    _imp->toggleTripleSyncBtn->setCheckable(true);
    _imp->toggleTripleSyncBtn->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    
    connect(_imp->toggleTripleSyncBtn, SIGNAL(toggled(bool)),
            this, SLOT(toggleTripleSync(bool)));

    _imp->helpersLayout->addWidget(_imp->toggleTripleSyncBtn);
    _imp->helpersLayout->addStretch();

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
    _imp->mainLayout->addLayout(_imp->helpersLayout);

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

void DopeSheetEditor::toggleTripleSync(bool enabled)
{
    Natron::PixmapEnum tripleSyncPixmapValue = (enabled)
            ? Natron::NATRON_PIXMAP_LOCKED
            : Natron::NATRON_PIXMAP_UNLOCKED;

    QPixmap tripleSyncBtnPix;
    appPTR->getIcon(tripleSyncPixmapValue, &tripleSyncBtnPix);

    _imp->toggleTripleSyncBtn->setIcon(QIcon(tripleSyncBtnPix));
    _imp->toggleTripleSyncBtn->setDown(enabled);
    _imp->gui->setTripleSyncEnabled(enabled);
    if (enabled) {
        QList<int> sizes = _imp->splitter->sizes();
        assert(sizes.size() > 0);
        _imp->gui->setCurveEditorTreeWidth(sizes[0]);
    }
}

void
DopeSheetEditor::refreshSelectionBboxAndRedrawView()
{
    _imp->dopeSheetView->refreshSelectionBboxAndRedraw();
}

int
DopeSheetEditor::getTimelineCurrentTime() const
{
    return _imp->gui->getApp()->getTimeLine()->currentFrame();
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