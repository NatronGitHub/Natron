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
#include "Gui/AnimationModuleView.h"
#include "Gui/Button.h"
#include "Gui/CurveGui.h"
#include "Gui/ComboBox.h"
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
#include "Engine/Utils.h"
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
    Button* displayViewChoice;

    Label* knobLabel;
    LineEdit* knobExpressionLineEdit;
    Label* expressionResultLabel;


    Splitter *treeAndViewSplitter;


    AnimationModuleTreeView *treeView;

    AnimationModuleView* view;

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
, view(0)
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

    QPixmap pixCurveEditor, pixStacked;
    appPTR->getIcon(NATRON_PIXMAP_CURVE_EDITOR, iconSize, &pixCurveEditor);
    appPTR->getIcon(NATRON_PIXMAP_ANIMATION_MODULE, iconSize, &pixStacked);

    QSize medSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    QSize medIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );

    {
        _imp->displayViewChoice = new Button(_imp->buttonsContainer);
        QIcon ic;
        ic.addPixmap(pixStacked, QIcon::Normal, QIcon::On);
        ic.addPixmap(pixCurveEditor, QIcon::Normal, QIcon::Off);
        _imp->displayViewChoice->setIcon(ic);
        _imp->displayViewChoice->setCheckable(true);
        _imp->displayViewChoice->setChecked(true);
        _imp->displayViewChoice->setDown(true);
        _imp->displayViewChoice->setFixedSize(medSize);
        _imp->displayViewChoice->setIconSize(medIconSize);
        _imp->displayViewChoice->setFocusPolicy(Qt::NoFocus);
        setToolTipWithShortcut(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleStackView, "<p>" + tr("Switch between Dope Sheet + CurveEditor and Curve Editor only").toStdString() + "</p>" + "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->displayViewChoice);
        connect(_imp->displayViewChoice, SIGNAL(clicked(bool)), this, SLOT(onDisplayViewClicked(bool)));
        _imp->buttonsLayout->addWidget(_imp->displayViewChoice);
    }

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
    //_imp->treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    _imp->treeView->setFocusPolicy(Qt::NoFocus);
    _imp->treeAndViewSplitter->addWidget(_imp->treeView);

    _imp->view = new AnimationModuleView(_imp->treeAndViewSplitter);
    _imp->view->initialize(gui, _imp->model);

    _imp->treeAndViewSplitter->addWidget(_imp->view);


    {
        QList<int> sizes;
        sizes.push_back(300);
        sizes.push_back(700);
        _imp->treeAndViewSplitter->setSizes_mt_safe(sizes);
        //_imp->treeAndViewSplitter->setStretchFactor(0, 2);
    }
    _imp->mainLayout->addWidget(_imp->treeAndViewSplitter);

    connect( _imp->model->getSelectionModel().get(), SIGNAL(selectionChanged(bool)), this, SLOT(onSelectionModelSelectionChanged(bool)) );

    connect( _imp->model->getSelectionModel().get(), SIGNAL(selectionChanged(bool)), _imp->view, SLOT(onSelectionModelKeyframeSelectionChanged()) );

    connect( _imp->model.get(), SIGNAL(nodeAdded(NodeAnimPtr)),
             _imp->treeView, SLOT(onNodeAdded(NodeAnimPtr)) );

    connect( _imp->model.get(), SIGNAL(nodeAboutToBeRemoved(NodeAnimPtr)),
             _imp->treeView, SLOT(onNodeAboutToBeRemoved(NodeAnimPtr)) );

    connect( _imp->treeView->verticalScrollBar(), SIGNAL(valueChanged(int)),
             _imp->view, SLOT(onAnimationTreeViewScrollbarMoved(int)) );

    connect( _imp->treeView, SIGNAL(itemExpanded(QTreeWidgetItem*)),
             _imp->view, SLOT(onAnimationTreeViewItemExpandedOrCollapsed(QTreeWidgetItem*)) );

    connect( _imp->treeView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
             _imp->view, SLOT(onAnimationTreeViewItemExpandedOrCollapsed(QTreeWidgetItem*)) );

    onSelectionModelSelectionChanged(false);
}

AnimationModuleEditor::~AnimationModuleEditor()
{}

AnimationModuleEditor::AnimationModuleDisplayViewMode
AnimationModuleEditor::getDisplayMode() const
{
    return _imp->displayViewChoice->isChecked() ? eAnimationModuleDisplayViewModeStacked : eAnimationModuleDisplayViewModeCurveEditor;
}

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
    _imp->view->centerOn(xMin, xMax);
}

void
AnimationModuleEditor::refreshSelectionBboxAndRedrawView()
{
    _imp->view->refreshSelectionBboxAndRedraw();
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

AnimationModuleView*
AnimationModuleEditor::getView() const
{
    return _imp->view;
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
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleStackView, modifiers, key) ) {
        bool checked = _imp->displayViewChoice->isChecked();
        _imp->displayViewChoice->setChecked(!checked);
        onDisplayViewClicked(!checked);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleRemoveKeys, modifiers, key) ) {
        onRemoveSelectedKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleConstant, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeConstant);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleLinear, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeLinear);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleSmooth, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeSmooth);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCatmullrom, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeCatmullRom);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCubic, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeCubic);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleHorizontal, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeHorizontal);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleBreak, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeBroken);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCenterAll, modifiers, key) ) {
        onCenterAllCurvesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCenter, modifiers, key) ) {
        onCenterOnSelectedCurvesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleSelectAll, modifiers, key) ) {
        onSelectAllKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModuleCopy, modifiers, key) ) {
        onCopySelectedKeyFramesToClipBoardActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModulePasteKeyframes, modifiers, key) ) {
        onPasteClipBoardKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutIDActionAnimationModulePasteKeyframesAbsolute, modifiers, key) ) {
        onPasteClipBoardKeyFramesAbsoluteActionTriggered();
    } else if ( key == Qt::Key_Plus ) { // zoom in/out doesn't care about modifiers
        // one wheel click = +-120 delta
        zoomDisplayedView(120);
    } else if ( key == Qt::Key_Minus ) { // zoom in/out doesn't care about modifiers
        // one wheel click = +-120 delta
        zoomDisplayedView(-120);
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
AnimationModuleEditor::saveProjection(SERIALIZATION_NAMESPACE::ViewportData* /*data*/)
{

    /*if (!_imp->view->hasDrawnOnce()) {
        return false;
    }
    _imp->view->getProjection(&data->left, &data->bottom, &data->zoomFactor, &data->par);
*/
    return false;
}

bool
AnimationModuleEditor::loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& /*data*/)
{
    //_imp->view->setProjection(data.left, data.bottom, data.zoomFactor, data.par);
    return false;
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
        throw std::invalid_argument(tr("Cannot set an expression on multiple items").toStdString());
    }
    KnobAnimPtr isKnobAnim = toKnobAnim(curves.front()->getItem());
    if (!isKnobAnim) {
        throw std::invalid_argument(tr("Cannot set an expression on something else than a knob").toStdString());
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

    try {
        setSelectedCurveExpression( _imp->knobExpressionLineEdit->text() );
    } catch(const std::exception& /*e*/) {
        //Dialogs::errorDialog(tr("Animation Module").toStdString(), e.what());
    }
}

void
AnimationModuleEditor::onDisplayViewClicked(bool clicked)
{
    _imp->displayViewChoice->setDown(clicked);
    _imp->view->update();
}

void
AnimationModuleEditor::onSelectionModelSelectionChanged(bool /*recurse*/)
{
    const AnimItemDimViewKeyFramesMap& selectedKeys = _imp->model->getSelectionModel()->getCurrentKeyFramesSelection();
    bool expressionFieldsEnabled = true;
    if (selectedKeys.size() > 1 || selectedKeys.size() == 0) {
        expressionFieldsEnabled = false;
    }
    QString knobLabel, currentExpression;
    if (expressionFieldsEnabled) {
        for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it!=selectedKeys.end(); ++it) {
            KnobAnimPtr knob = toKnobAnim(it->first.item);
            if (!knob) {
                expressionFieldsEnabled = false;
                break;
            } else {
                knobLabel = knob->getViewDimensionLabel(it->first.dim, it->first.view);
                currentExpression = QString::fromUtf8(knob->getInternalKnob()->getExpression(it->first.dim, it->first.view).c_str());
            }
        }
    }
    _imp->knobLabel->setVisible(expressionFieldsEnabled);
    if (expressionFieldsEnabled) {
        _imp->knobLabel->setText(knobLabel);
        _imp->knobExpressionLineEdit->setText(currentExpression);
    }
    _imp->knobExpressionLineEdit->setVisible(expressionFieldsEnabled);
    _imp->expressionResultLabel->setVisible(expressionFieldsEnabled);

}


void
AnimationModuleEditor::onRemoveSelectedKeyFramesActionTriggered()
{
    _imp->model->deleteSelectedKeyframes();
}

void
AnimationModuleEditor::onCopySelectedKeyFramesToClipBoardActionTriggered()
{
    _imp->model->copySelectedKeys();
}

void
AnimationModuleEditor::onPasteClipBoardKeyFramesActionTriggered()
{
    _imp->model->pasteKeys(_imp->model->getKeyFramesClipBoard(), true /*relative*/);
}

void
AnimationModuleEditor::onPasteClipBoardKeyFramesAbsoluteActionTriggered()
{
    _imp->model->pasteKeys(_imp->model->getKeyFramesClipBoard(), false /*relative*/);
}

void
AnimationModuleEditor::onSelectAllKeyFramesActionTriggered()
{
    _imp->model->getSelectionModel()->selectAll();
}

void
AnimationModuleEditor::onSetInterpolationActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    KeyframeTypeEnum type = (KeyframeTypeEnum)action->data().toInt();
    _imp->model->setSelectedKeysInterpolation(type);
}

void
AnimationModuleEditor::onCenterAllCurvesActionTriggered()
{
    _imp->view->centerOnAllItems();
}

void
AnimationModuleEditor::onCenterOnSelectedCurvesActionTriggered()
{
    _imp->view->centerOnSelection();
}

void
AnimationModuleEditor::zoomDisplayedView(int delta)
{
    QWheelEvent e(mapFromGlobal( QCursor::pos() ), delta, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
    _imp->view->wheelEvent(&e);

}

QIcon
AnimationModuleEditor::getIcon() const
{
    int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QPixmap p;
    appPTR->getIcon(NATRON_PIXMAP_ANIMATION_MODULE, iconSize, &p);
    return QIcon(p);
}


NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING
#include "moc_AnimationModuleEditor.cpp"