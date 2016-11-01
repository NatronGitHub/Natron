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
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/OfxOverlayInteract.h"

#include "Gui/AnimationModule.h"
#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/CurveWidget.h"
#include "Gui/KnobGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobAnim.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include "ofxNatron.h"


NATRON_NAMESPACE_ENTER;
using std::make_pair;


//=============================KnobGuiParametric===================================

KnobGuiParametric::KnobGuiParametric(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , OverlaySupport()
    , AnimationModuleBase(knob->getGui())
    , treeColumn(NULL)
    , _curveWidget(NULL)
    , _tree(NULL)
    , _resetButton(NULL)
    , _curves()
{
    _knob = toKnobParametric(knob->getKnob());
    QObject::connect( _knob.lock().get(), SIGNAL(curveColorChanged(DimSpec)), this, SLOT(onColorChanged(DimSpec)) );
}

KnobGuiParametric::~KnobGuiParametric()
{
}

void
KnobGuiParametric::removeSpecificGui()
{
    _curveWidget->deleteLater();
    treeColumn->deleteLater();
}

void
KnobGuiParametric::createWidget(QHBoxLayout* layout)
{
    KnobParametricPtr knob = _knob.lock();
    QObject::connect( knob.get(), SIGNAL(curveChanged(DimSpec)), this, SLOT(onCurveChanged(DimSpec)) );
    OfxParamOverlayInteractPtr interact = knob->getCustomInteract();

    KnobGuiPtr knobUI = getKnobGui();

    treeColumn = new QWidget( layout->parentWidget() );
    QVBoxLayout* treeColumnLayout = new QVBoxLayout(treeColumn);
    treeColumnLayout->setContentsMargins(0, 0, 0, 0);

    _tree = new QTreeWidget( layout->parentWidget() );
    _tree->setSelectionMode(QAbstractItemView::ContiguousSelection);
    _tree->setColumnCount(1);
    _tree->header()->close();
    if ( knobUI->hasToolTip() ) {
        knobUI->toolTip(_tree, getView());
    }
    treeColumnLayout->addWidget(_tree);

    _resetButton = new Button(QString::fromUtf8("Reset"), treeColumn);
    _resetButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Reset the selected curves in the tree to their default shape."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _resetButton, SIGNAL(clicked()), this, SLOT(resetSelectedCurves()) );
    treeColumnLayout->addWidget(_resetButton);

    layout->addWidget(treeColumn);

    AnimationModuleBasePtr thisShared = shared_from_this();
    _selectionModel.reset(new AnimationModuleSelectionModel(thisShared));
    _curveWidget = new CurveWidget(layout->parentWidget());

    _animRoot = KnobAnim::create(thisShared, KnobsHolderAnimBasePtr(), 0 /*parentTreeItem*/, knobUI);
    _curveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if (interact) {
        _curveWidget->setCustomInteract(interact);
    }
    if ( knobUI->hasToolTip() ) {
        knobUI->toolTip(_curveWidget, getView());
    }
    layout->addWidget(_curveWidget);

    ViewIdx view = getView();
    std::vector<CurveGuiPtr > visibleCurves;
    for (int i = 0; i < knob->getNDimensions(); ++i) {
        QString curveName = QString::fromUtf8( knob->getDimensionName(DimIdx(i)).c_str() );
        double color[4];
        knob->getCurveColor(DimIdx(i), &color[0], &color[1], &color[2]);
        color[3] = 1.;
        CurveGuiPtr curve = _animRoot->AnimItemBase::getCurveGui(DimIdx(i), view);
        assert(curve);
        curve->setColor(color);
        QTreeWidgetItem* item = new QTreeWidgetItem(_tree);
        item->setData(0, QT_ROLE_CONTEXT_DIM, QVariant(i));
        item->setData(0, QT_ROLE_CONTEXT_ITEM_POINTER, qVariantFromValue((void*)_animRoot.get()));
        item->setText(0, curveName);
        item->setSelected(true);
        CurveDescriptor desc;
        desc.curve = curve;
        desc.treeItem = item;
        _curves.push_back(desc);
        visibleCurves.push_back(curve);
    }

    _curveWidget->centerOnCurves(visibleCurves, true);
    QObject::connect( _tree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemsSelectionChanged()) );
} // createWidget

TimeLinePtr
KnobGuiParametric::getTimeline() const
{
    return TimeLinePtr();
}

void
KnobGuiParametric::getTopLevelKnobs(std::vector<KnobAnimPtr>* knobs) const
{
    knobs->push_back(_animRoot);
}

void
KnobGuiParametric::refreshSelectionBboxAndUpdateView()
{
    _curveWidget->refreshSelectionBboxAndRedraw();
}

CurveWidget*
KnobGuiParametric::getCurveWidget() const
{
    return _curveWidget;
}

AnimationModuleSelectionModelPtr
KnobGuiParametric::getSelectionModel() const
{
    return _selectionModel;
}

bool
KnobGuiParametric::findItem(QTreeWidgetItem* treeItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* /*isTableItem*/, NodeAnimPtr* /*isNodeItem*/, ViewSetSpec* view, DimSpec* dimension) const
{
    if (!treeItem) {
        return false;
    }
    *dimension = DimSpec(treeItem->data(0, QT_ROLE_CONTEXT_DIM).toInt());
    *view = ViewSetSpec(getView());
    *type = eAnimatedItemTypeKnobDim;
    assert(toKnobAnim(((KnobAnim*)treeItem->data(0, QT_ROLE_CONTEXT_ITEM_POINTER).value<void*>())->shared_from_this()) == _animRoot);
    *isKnob = _animRoot;
    return true;
}

void
KnobGuiParametric::onColorChanged(DimSpec dimension)
{
    KnobParametricPtr knob = _knob.lock();
    for (std::size_t i = 0; i < _curves.size(); ++i) {
        if (dimension.isAll() || (int)i == dimension) {
            double color[4];
            knob->getCurveColor(DimIdx(i), &color[0], &color[1], &color[2]);
            color[3] = 1.;
            CurveDescriptor& found = _curves[dimension];
            found.curve->setColor(color);
        }
    }


    _curveWidget->update();
}

void
KnobGuiParametric::setWidgetsVisible(bool visible)
{
    _curveWidget->setVisible(visible);
    _tree->setVisible(visible);
    _resetButton->setVisible(visible);
}

void
KnobGuiParametric::setEnabled()
{
    KnobParametricPtr knob = _knob.lock();
    bool b = knob->isEnabled(DimIdx(0), getView())  && !knob->isSlave(DimIdx(0), getView()) && knob->getExpression(DimIdx(0), getView()).empty();

    _tree->setEnabled(b);
}

void
KnobGuiParametric::updateGUI(DimSpec /*dimension*/)
{
    _curveWidget->update();
}

void
KnobGuiParametric::onCurveChanged(DimSpec /*dimension*/)
{

    _curveWidget->update();
}

void
KnobGuiParametric::onItemsSelectionChanged()
{
    std::vector<CurveGuiPtr > curves;

    QList<QTreeWidgetItem*> selectedItems = _tree->selectedItems();

    ///find in the _curves map if an item's map the current
    AnimItemDimViewKeyFramesMap selectedKeys;
    std::vector<TableItemAnimPtr> tableItems;
    std::vector<NodeAnimPtr> rangeNodes;

    for (QList<QTreeWidgetItem*>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        for (CurveGuis::iterator it2 = _curves.begin(); it2 != _curves.end(); ++it2) {
            if ( it2->treeItem == *it ) {
                curves.push_back(it2->curve);
                break;
            }
        }
    }

    _selectionModel->makeSelection(selectedKeys, tableItems, rangeNodes, AnimationModuleSelectionModel::SelectionTypeAdd | AnimationModuleSelectionModel::SelectionTypeClear);
    _curveWidget->centerOnCurves(curves, true); //remove this if you don't want the editor to switch to a curve on a selection change
}

void
KnobGuiParametric::resetSelectedCurves()
{
    QList<QTreeWidgetItem*> selected = _tree->selectedItems();
    KnobParametricPtr k = _knob.lock();
    for (int i = 0; i < selected.size(); ++i) {
        //find the items in the curves
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->treeItem == selected.at(i) ) {
                k->resetToDefaultValue( it->curve->getDimension() );
                break;
            }
        }
    }
    k->evaluateValueChange(DimIdx(0), k->getCurrentTime(), ViewIdx(0), eValueChangedReasonUserEdited);
}

void
KnobGuiParametric::refreshDimensionName(DimIdx dim)
{
    if ( (dim < 0) || ( dim >= (int)_curves.size() ) ) {
        return;
    }
    KnobParametricPtr knob = _knob.lock();
    CurveDescriptor& found = _curves[dim];
    QString name = QString::fromUtf8( knob->getDimensionName(dim).c_str() );
    found.treeItem->setText(0, name);
    _curveWidget->update();
}

void
KnobGuiParametric::swapOpenGLBuffers()
{
    _curveWidget->swapOpenGLBuffers();
}

void
KnobGuiParametric::redraw()
{
    _curveWidget->redraw();
}


void
KnobGuiParametric::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
{
    _curveWidget->getOpenGLContextFormat(depthPerComponents, hasAlpha);
}

void
KnobGuiParametric::getViewportSize(double &width,
                                   double &height) const
{
    _curveWidget->getViewportSize(width, height);
}

void
KnobGuiParametric::getPixelScale(double & xScale,
                                 double & yScale) const
{
    _curveWidget->getPixelScale(xScale, yScale);
}

void
KnobGuiParametric::getBackgroundColour(double &r,
                                       double &g,
                                       double &b) const
{
    _curveWidget->getBackgroundColour(r, g, b);
}

void
KnobGuiParametric::saveOpenGLContext()
{
    _curveWidget->saveOpenGLContext();
}

void
KnobGuiParametric::restoreOpenGLContext()
{
    _curveWidget->restoreOpenGLContext();
}

void
KnobGuiParametric::getCursorPosition(double& x, double& y) const
{
    return _curveWidget->getCursorPosition(x, y);
}

RectD
KnobGuiParametric::getViewportRect() const
{
    return _curveWidget->getViewportRect();
}

void
KnobGuiParametric::toCanonicalCoordinates(double *x, double *y) const
{
    return _curveWidget->toCanonicalCoordinates(x, y);
}

void
KnobGuiParametric::toWidgetCoordinates(double *x, double *y) const
{
    return _curveWidget->toWidgetCoordinates(x, y);
}

void
KnobGuiParametric::pushUndoCommand(QUndoCommand *cmd)
{
    getKnobGui()->pushUndoCommand(cmd);
}

int
KnobGuiParametric::getWidgetFontHeight() const
{
    return _curveWidget->getWidgetFontHeight();
}

int
KnobGuiParametric::getStringWidthForCurrentFont(const std::string& string) const
{
    return _curveWidget->getStringWidthForCurrentFont(string);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobGuiParametric.cpp"
