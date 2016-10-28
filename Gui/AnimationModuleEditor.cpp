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
#include "Gui/ComboBox.h"
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
#include "Gui/Splitter.h"
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
    ComboBox* displayViewChoice;

    Label* knobLabel;
    LineEdit* knobExpressionLineEdit;
    Label* expressionResultLabel;


    Splitter *treeAndViewSplitter;


    AnimationModuleTreeView *treeView;

    Splitter *viewsSplitter;
    DopeSheetView* dopeSheetView;
    CurveWidget* curveView;

    AnimationModulePtr model;

};

AnimationModuleEditorPrivate::AnimationModuleEditorPrivate(AnimationModuleEditor *publicInterface)
: publicInterface(publicInterface)
, mainLayout(0)
, buttonsContainer(0)
, buttonsLayout(0)
, displayViewChoice(0)
, knobLabel(0)
, knobExpressionLineEdit(0)
, expressionResultLabel(0)
, treeAndViewSplitter(0)
, treeView(0)
, viewsSplitter(0)
, dopeSheetView(0)
, curveView(0)
, model()
{
}

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

    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    _imp->buttonsLayout->setContentsMargins(0, 0, 0, 0);
    _imp->buttonsLayout->setSpacing(0);

    int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);

    QPixmap pixCurveEditor, pixDopeSheet, pixSplit;
    appPTR->getIcon(NATRON_PIXMAP_CURVE_EDITOR, iconSize, &pixCurveEditor);
    appPTR->getIcon(NATRON_PIXMAP_DOPE_SHEET, iconSize, &pixDopeSheet);

    _imp->displayViewChoice = new ComboBox(_imp->buttonsContainer);
    _imp->displayViewChoice->addItem(tr("Curve Editor"), pixCurveEditor, QKeySequence());
    _imp->displayViewChoice->addItem(tr("Dope Sheet"), pixDopeSheet, QKeySequence());
    _imp->displayViewChoice->addItem(tr("Split"), QIcon(), QKeySequence());
    _imp->displayViewChoice->setCurrentIndex_no_emit(0);
    connect(_imp->displayViewChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(onViewCurrentIndexChanged(int)));
    _imp->buttonsLayout->addWidget(_imp->displayViewChoice);

    _imp->buttonsLayout->addSpacing(TO_DPIX(10));

    _imp->knobLabel = new Label(_imp->buttonsContainer);
    _imp->knobLabel->setAltered(true);
    _imp->knobLabel->setText( tr("No curve selected") );
    _imp->knobExpressionLineEdit = new LineEdit(_imp->buttonsContainer);
    _imp->knobExpressionLineEdit->setReadOnly_NoFocusRect(true);
    QObject::connect( _imp->knobExpressionLineEdit, SIGNAL(editingFinished()), this, SLOT(onExprLineEditFinished()) );
    _imp->expressionResultLabel = new Label(_imp->buttonsContainer);
    _imp->expressionResultLabel->setAltered(true);
    _imp->expressionResultLabel->setText( QString::fromUtf8("= ") );


    _imp->buttonsLayout->addWidget(_imp->knobLabel);
    _imp->buttonsLayout->addSpacing(TO_DPIX(5));
    _imp->buttonsLayout->addWidget(_imp->knobExpressionLineEdit);
    _imp->buttonsLayout->addSpacing(TO_DPIX(5));
    _imp->buttonsLayout->addWidget(_imp->expressionResultLabel);
    _imp->buttonsLayout->addStretch();

    _imp->mainLayout->addWidget(_imp->buttonsContainer);

    _imp->treeAndViewSplitter = new Splitter(Qt::Horizontal, gui, this);

    _imp->model = AnimationModule::create(gui, this, timeline);
    _imp->treeView = new AnimationModuleTreeView(_imp->model, gui, _imp->treeAndViewSplitter);

    _imp->treeAndViewSplitter->addWidget(_imp->treeView);

    _imp->viewsSplitter = new Splitter(Qt::Horizontal, gui, _imp->treeAndViewSplitter);

    _imp->dopeSheetView = new DopeSheetView(_imp->viewsSplitter);
    _imp->curveView = new CurveWidget(_imp->viewsSplitter);

    _imp->dopeSheetView->initialize(gui, _imp->model);
    _imp->curveView->initialize(gui, _imp->model);
    
    _imp->viewsSplitter->addWidget(_imp->dopeSheetView);
    _imp->viewsSplitter->addWidget(_imp->curveView);
    _imp->dopeSheetView->hide();

    _imp->treeAndViewSplitter->addWidget(_imp->viewsSplitter);
    _imp->mainLayout->addWidget(_imp->treeAndViewSplitter);

    connect( _imp->model->getSelectionModel().get(), SIGNAL(selectionChanged(bool)), this, SLOT(onSelectionModelSelectionChanged(bool)) );

    connect( _imp->model->getSelectionModel().get(), SIGNAL(selectionChanged(bool)), _imp->treeView, SLOT(onSelectionModelKeyframeSelectionChanged(bool)) );

    connect( _imp->model.get(), SIGNAL(nodeAdded(NodeAnimPtr)),
             _imp->treeView, SLOT(onNodeAdded(NodeAnimPtr)) );

    connect( _imp->model.get(), SIGNAL(nodeAboutToBeRemoved(NodeAnimPtr)),
             _imp->treeView, SLOT(onNodeAboutToBeRemoved(NodeAnimPtr)) );

    connect( _imp->model.get(), SIGNAL(nodeAdded(NodeAnimPtr)),
             _imp->dopeSheetView, SLOT(onNodeAdded(NodeAnimPtr)) );

    connect( _imp->model.get(), SIGNAL(nodeAboutToBeRemoved(NodeAnimPtr)),
             _imp->dopeSheetView, SLOT(onNodeAboutToBeRemoved(NodeAnimPtr)) );

    connect( _imp->treeView->verticalScrollBar(), SIGNAL(valueChanged(int)),
             _imp->dopeSheetView, SLOT(onAnimationTreeViewScrollbarMoved(int)) );

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
    _imp->curveView->centerOn(xMin, xMax);
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

AnimationModulePtr
AnimationModuleEditor::getModel() const
{
    return _imp->model;
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
AnimationModuleEditor::keyPressEvent(QKeyEvent* e)
{
    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();
    bool accept = true;

    if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRenameNode, modifiers, key) ) {
        _imp->model->renameSelectedNode();
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

    if (!_imp->curveView->hasDrawnOnce()) {
        return false;
    }
    _imp->curveView->getProjection(&data->left, &data->bottom, &data->zoomFactor, &data->par);

    return true;
}

bool
AnimationModuleEditor::loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& data)
{
    _imp->curveView->setProjection(data.left, data.bottom, data.zoomFactor, data.par);
    return true;
}


void
AnimationModuleEditor::setSelectedCurveExpression(const QString& expression)
{
    const AnimItemDimViewKeyFramesMap& selectedKeys = _imp->model->getSelectionModel()->getCurrentKeyFramesSelection();
    std::list<CurveGuiPtr> curves;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it != selectedKeys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (guiCurve) {
            curves.push_back(guiCurve);
        }
    }

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

void
AnimationModuleEditor::onExprLineEditFinished()
{
    setSelectedCurveExpression( _imp->knobExpressionLineEdit->text() );
}

void
AnimationModuleEditor::onViewCurrentIndexChanged(int index)
{
    if (index == 0) {
        // CurveEditor
        _imp->curveView->show();
        _imp->dopeSheetView->hide();
    } else if (index == 1) {
        // DopeSheet
        _imp->curveView->hide();
        _imp->dopeSheetView->show();
    } else if (index == 2) {
        // Split
        _imp->curveView->show();
        _imp->dopeSheetView->show();
    }
}

void
AnimationModuleEditor::onSelectionModelSelectionChanged(bool /*recurse*/)
{
    const AnimItemDimViewKeyFramesMap& selectedKeys = _imp->model->getSelectionModel()->getCurrentKeyFramesSelection();
    bool expressionFieldsEnabled = true;
    if (selectedKeys.size() > 1) {
        expressionFieldsEnabled = false;
    }
    if (expressionFieldsEnabled) {
        for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it!=selectedKeys.end(); ++it) {
            KnobAnimPtr knob = toKnobAnim(it->first.item);
            if (!knob) {
                expressionFieldsEnabled = false;
                break;
            }
        }
    }
    _imp->knobLabel->setVisible(expressionFieldsEnabled);
    _imp->expressionResultLabel->setVisible(expressionFieldsEnabled);
    _imp->expressionResultLabel->setVisible(expressionFieldsEnabled);

}

NATRON_NAMESPACE_EXIT;
