/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "KnobGuiParametric.h"

#include <cfloat>
#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QColorDialog>
#include <QToolTip>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QApplication>
#include <QScrollArea>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QDebug>
#include <QFontComboBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/KnobUndoCommand.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/OfxOverlayInteract.h"

#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobAnim.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER


class KnobGuiParametricAnimationModel;
struct KnobGuiParametricPrivate
{
    KnobGuiParametric* _publicInterface;

    QWidget* treeColumn;
    AnimationModuleView* curveWidget;
    QTreeWidget* tree;

    AnimationModuleSelectionModelPtr selectionModel;
    KnobAnimPtr animRoot;
    Button* resetButton;
    struct CurveDescriptor
    {
        CurveGuiPtr curve;
        QTreeWidgetItem* treeItem;
    };

    typedef std::vector<CurveDescriptor> CurveGuis;
    boost::shared_ptr<AnimationModuleBase> model;
    CurveGuis curves;
    KnobParametricWPtr knob;
    int nRefreshRequests;

    KnobGuiParametricPrivate(KnobGuiParametric* publicInterface)
    : _publicInterface(publicInterface)
    , treeColumn(0)
    , curveWidget(0)
    , tree(0)
    , selectionModel()
    , animRoot()
    , resetButton(0)
    , model()
    , curves()
    , knob()
    , nRefreshRequests(0)
    {

    }


};

class KnobGuiParametricAnimationModel : public AnimationModuleBase
{
    KnobGuiParametricPrivate* _imp;

public:

    KnobGuiParametricAnimationModel(Gui* gui, KnobGuiParametricPrivate* knob)
    : AnimationModuleBase(gui)
    , _imp(knob)
    {

    }

    virtual ~KnobGuiParametricAnimationModel()
    {

    }


    // Overriden from AnimationModuleBase
    virtual void pushUndoCommand(QUndoCommand *cmd) OVERRIDE FINAL;
    virtual TimeLinePtr getTimeline() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getTopLevelKnobs(bool onlyVisible, std::vector<KnobAnimPtr>* knobs) const OVERRIDE FINAL;
    virtual void refreshSelectionBboxAndUpdateView() OVERRIDE FINAL;
    virtual AnimationModuleView* getView() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual AnimationModuleSelectionModelPtr getSelectionModel() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool findItem(QTreeWidgetItem* treeItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* isTableItem, NodeAnimPtr* isNodeItem, ViewSetSpec* view, DimSpec* dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isCurveVisible(const AnimItemBasePtr& item, DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getTreeColumnsCount() const OVERRIDE FINAL WARN_UNUSED_RETURN;
};


KnobGuiParametric::KnobGuiParametric(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , _imp(new KnobGuiParametricPrivate(this))
{
    _imp->knob = toKnobParametric(knob->getKnob());
    QObject::connect( _imp->knob.lock().get(), SIGNAL(curveColorChanged(DimSpec)), this, SLOT(onColorChanged(DimSpec)) );
    QObject::connect(this, SIGNAL(mustUpdateCurveWidgetLater()), this, SLOT(onUpdateCurveWidgetLaterReceived()), Qt::QueuedConnection);
}

KnobGuiParametric::~KnobGuiParametric()
{
}


void
KnobGuiParametric::createWidget(QHBoxLayout* layout)
{
    KnobParametricPtr knob = _imp->knob.lock();
    QObject::connect( knob.get(), SIGNAL(curveChanged(DimSpec)), this, SLOT(onCurveChanged(DimSpec)) );
    OverlayInteractBasePtr interact = knob->getCustomInteract();

    KnobGuiPtr knobUI = getKnobGui();

    _imp->treeColumn = new QWidget( layout->parentWidget() );
    QVBoxLayout* treeColumnLayout = new QVBoxLayout(_imp->treeColumn);
    treeColumnLayout->setContentsMargins(0, 0, 0, 0);

    _imp->tree = new QTreeWidget( layout->parentWidget() );
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    _imp->tree->setSelectionMode(QAbstractItemView::ContiguousSelection);
    _imp->tree->setColumnCount(1);
    _imp->tree->header()->close();
    QObject::connect( _imp->tree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemsSelectionChanged()) );

    if ( knobUI->hasToolTip() ) {
        knobUI->toolTip(_imp->tree, getView());
    }
    treeColumnLayout->addWidget(_imp->tree);

    _imp->resetButton = new Button(tr("Reset"), _imp->treeColumn);
    _imp->resetButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Reset the selected curves in the tree to their default shape."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->resetButton, SIGNAL(clicked()), this, SLOT(resetSelectedCurves()) );
    treeColumnLayout->addWidget(_imp->resetButton);

    layout->addWidget(_imp->treeColumn);

    _imp->model = boost::make_shared<KnobGuiParametricAnimationModel>(knobUI->getGui(), _imp.get());
    _imp->selectionModel = boost::make_shared<AnimationModuleSelectionModel>(_imp->model);
    _imp->curveWidget = new AnimationModuleView(layout->parentWidget());

    _imp->animRoot = KnobAnim::create(_imp->model, KnobsHolderAnimBasePtr(), 0 /*parentTreeItem*/, knob);
    _imp->curveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _imp->curveWidget->initialize(knobUI->getGui(), _imp->model);
    if (interact) {
        _imp->curveWidget->setCustomInteract(interact);
    }
    if ( knobUI->hasToolTip() ) {
        knobUI->toolTip(_imp->curveWidget, getView());
    }
    layout->addWidget(_imp->curveWidget);

    ViewIdx view = getView();

    for (int i = 0; i < knob->getNDimensions(); ++i) {
        QString curveName = QString::fromUtf8( knob->getDimensionName(DimIdx(i)).c_str() );
        double color[4];
        knob->getCurveColor(DimIdx(i), &color[0], &color[1], &color[2]);
        color[3] = 1.;
        QTreeWidgetItem* treeItem = _imp->animRoot->getTreeItem(DimIdx(i), view);
        assert(treeItem);
        CurveGuiPtr curve = _imp->animRoot->getCurveGui(DimIdx(i), view);
        assert(curve);
        curve->setColor(color);
        treeItem->setText(0, curveName);
        treeItem->setSelected(true);
        KnobGuiParametricPrivate::CurveDescriptor desc;
        desc.curve = curve;
        desc.treeItem = treeItem;
        QTreeWidgetItem* curParent = treeItem->parent();
        if (curParent) {
            curParent->removeChild(treeItem);
        }
        _imp->tree->addTopLevelItem(treeItem);
        _imp->curves.push_back(desc);
    }
    _imp->tree->selectAll();
    _imp->animRoot->emit_s_refreshKnobVisibilityLater();

} // createWidget

TimeLinePtr
KnobGuiParametricAnimationModel::getTimeline() const
{
    return TimeLinePtr();
}

void
KnobGuiParametricAnimationModel::getTopLevelKnobs(bool /*onlyVisible*/, std::vector<KnobAnimPtr>* knobs) const
{
    knobs->push_back(_imp->animRoot);
}

void
KnobGuiParametricAnimationModel::refreshSelectionBboxAndUpdateView()
{
    _imp->curveWidget->refreshSelectionBboxAndRedraw();
}

AnimationModuleView*
KnobGuiParametricAnimationModel::getView() const
{
    return _imp->curveWidget;
}

AnimationModuleSelectionModelPtr
KnobGuiParametricAnimationModel::getSelectionModel() const
{
    return _imp->selectionModel;
}

bool
KnobGuiParametricAnimationModel::findItem(QTreeWidgetItem* treeItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* /*isTableItem*/, NodeAnimPtr* /*isNodeItem*/, ViewSetSpec* view, DimSpec* dimension) const
{
    if (!treeItem) {
        return false;
    }
    *dimension = DimSpec(treeItem->data(0, QT_ROLE_CONTEXT_DIM).toInt());
    *view = ViewSetSpec(_imp->_publicInterface->getView());
    *type = eAnimatedItemTypeKnobDim;
    assert(toKnobAnim(((KnobAnim*)treeItem->data(0, QT_ROLE_CONTEXT_ITEM_POINTER).value<void*>())->shared_from_this()) == _imp->animRoot);
    *isKnob = _imp->animRoot;
    return true;
}


void
KnobGuiParametricAnimationModel::pushUndoCommand(QUndoCommand *cmd)
{
    _imp->_publicInterface->getKnobGui()->pushUndoCommand(cmd);
}

bool
KnobGuiParametricAnimationModel::isCurveVisible(const AnimItemBasePtr& item, DimIdx dimension, ViewIdx view) const
{
    QTreeWidgetItem* treeItem = item->getTreeItem(dimension, view);
    if (!treeItem) {
        return false;
    }
    return _imp->tree->isItemSelected(treeItem);
}

int
KnobGuiParametricAnimationModel::getTreeColumnsCount() const
{
    return _imp->tree->columnCount();
}

void
KnobGuiParametric::onColorChanged(DimSpec dimension)
{
    KnobParametricPtr knob = _imp->knob.lock();
    for (std::size_t i = 0; i < _imp->curves.size(); ++i) {
        if (dimension.isAll() || (int)i == dimension) {
            double color[4];
            knob->getCurveColor(DimIdx(i), &color[0], &color[1], &color[2]);
            color[3] = 1.;
            KnobGuiParametricPrivate::CurveDescriptor& found = _imp->curves[dimension];
            found.curve->setColor(color);
        }
    }


    _imp->curveWidget->update();
}

void
KnobGuiParametric::setWidgetsVisible(bool visible)
{
    _imp->curveWidget->setVisible(visible);
    _imp->tree->setVisible(visible);
    _imp->resetButton->setVisible(visible);
}

void
KnobGuiParametric::setEnabled(const std::vector<bool>& perDimEnabled)
{
    _imp->tree->setEnabled(perDimEnabled[0]);
}

void
KnobGuiParametric::updateGUI()
{
    _imp->curveWidget->update();
}

void
KnobGuiParametric::onUpdateCurveWidgetLaterReceived()
{
    if (!_imp->nRefreshRequests) {
        return;
    }
    _imp->nRefreshRequests = 0;
    _imp->curveWidget->update();

}

void
KnobGuiParametric::onCurveChanged(DimSpec /*dimension*/)
{
    ++_imp->nRefreshRequests;
    Q_EMIT mustUpdateCurveWidgetLater();
}

void
KnobGuiParametric::onItemsSelectionChanged()
{
    std::vector<CurveGuiPtr> curves;

    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();

    ///find in the _curves map if an item's map the current
    AnimItemDimViewKeyFramesMap selectedKeys;
    std::vector<TableItemAnimPtr> tableItems;
    std::vector<NodeAnimPtr> rangeNodes;

    for (QList<QTreeWidgetItem*>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        for (KnobGuiParametricPrivate::CurveGuis::iterator it2 = _imp->curves.begin(); it2 != _imp->curves.end(); ++it2) {
            if ( it2->treeItem == *it ) {
                curves.push_back(it2->curve);
                AnimItemDimViewIndexID id(_imp->animRoot, it2->curve->getView(), it2->curve->getDimension());
                selectedKeys.insert(std::make_pair(id, KeyFrameSet()));
                break;
            }
        }
    }

    _imp->selectionModel->makeSelection(selectedKeys, tableItems, rangeNodes, AnimationModuleSelectionModel::SelectionTypeAdd | AnimationModuleSelectionModel::SelectionTypeClear);
    _imp->curveWidget->centerOnCurves(curves, true); //remove this if you don't want the editor to switch to a curve on a selection change
}

void
KnobGuiParametric::resetSelectedCurves()
{
    QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
    KnobParametricPtr k = _imp->knob.lock();
    for (int i = 0; i < selected.size(); ++i) {
        //find the items in the curves
        for (KnobGuiParametricPrivate::CurveGuis::iterator it = _imp->curves.begin(); it != _imp->curves.end(); ++it) {
            if ( it->treeItem == selected.at(i) ) {
                k->resetToDefaultValue( it->curve->getDimension() );
                break;
            }
        }
    }
    k->evaluateValueChange(DimIdx(0), k->getHolder()->getTimelineCurrentTime(), ViewIdx(0), eValueChangedReasonUserEdited);
}

void
KnobGuiParametric::refreshDimensionName(DimIdx dim)
{
    if ( (dim < 0) || ( dim >= (int)_imp->curves.size() ) ) {
        return;
    }
    KnobParametricPtr knob = _imp->knob.lock();
    KnobGuiParametricPrivate::CurveDescriptor& found = _imp->curves[dim];
    QString name = QString::fromUtf8( knob->getDimensionName(dim).c_str() );
    found.treeItem->setText(0, name);
    _imp->curveWidget->update();
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_KnobGuiParametric.cpp"
