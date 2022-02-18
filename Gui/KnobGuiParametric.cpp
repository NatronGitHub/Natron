/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER
using std::make_pair;


//=============================KnobGuiParametric===================================

KnobGuiParametric::KnobGuiParametric(KnobIPtr knob,
                                     KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , treeColumn(NULL)
    , _curveWidget(NULL)
    , _tree(NULL)
    , _resetButton(NULL)
    , _curves()
{
    _knob = boost::dynamic_pointer_cast<KnobParametric>(knob);
    QObject::connect( _knob.lock().get(), SIGNAL(curveColorChanged(int)), this, SLOT(onColorChanged(int)) );
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
    QObject::connect( knob.get(), SIGNAL(curveChanged(int)), this, SLOT(onCurveChanged(int)) );
    OfxParamOverlayInteractPtr interact = knob->getCustomInteract();

    //layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    treeColumn = new QWidget( layout->parentWidget() );
    QVBoxLayout* treeColumnLayout = new QVBoxLayout(treeColumn);
    treeColumnLayout->setContentsMargins(0, 0, 0, 0);

    _tree = new QTreeWidget( layout->parentWidget() );
    _tree->setSelectionMode(QAbstractItemView::ContiguousSelection);
    _tree->setColumnCount(1);
    _tree->header()->close();
    if ( hasToolTip() ) {
        _tree->setToolTip( toolTip() );
    }
    treeColumnLayout->addWidget(_tree);

    _resetButton = new Button(QString::fromUtf8("Reset"), treeColumn);
    _resetButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Reset the selected curves in the tree to their default shape."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _resetButton, SIGNAL(clicked()), this, SLOT(resetSelectedCurves()) );
    treeColumnLayout->addWidget(_resetButton);

    layout->addWidget(treeColumn);

    _curveWidget = new CurveWidget( getGui(), this, TimeLinePtr(), layout->parentWidget() );
    _curveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if (interact) {
        _curveWidget->setCustomInteract(interact);
    }
    if ( hasToolTip() ) {
        _curveWidget->setToolTip( toolTip() );
    }
    layout->addWidget(_curveWidget);

    KnobGuiPtr thisShared = shared_from_this();
    std::vector<CurveGuiPtr> visibleCurves;
    for (int i = 0; i < knob->getDimension(); ++i) {
        QString curveName = QString::fromUtf8( knob->getDimensionName(i).c_str() );
        KnobCurveGuiPtr curve( new KnobCurveGui(_curveWidget, knob->getParametricCurve(i), thisShared, i, curveName, QColor(255, 255, 255), 1.) );
        _curveWidget->addCurveAndSetColor(curve);
        QColor color;
        double r, g, b;
        knob->getCurveColor(i, &r, &g, &b);
        color.setRedF(r);
        color.setGreenF(g);
        color.setBlueF(b);
        curve->setColor(color);
        QTreeWidgetItem* item = new QTreeWidgetItem(_tree);
        item->setText(0, curveName);
        item->setSelected(true);
        CurveDescriptor desc;
        desc.curve = curve;
        desc.treeItem = item;
        _curves.push_back(desc);
        if ( curve->isVisible() ) {
            visibleCurves.push_back(curve);
        }
    }

    _curveWidget->centerOn(visibleCurves, true);
    QObject::connect( _tree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemsSelectionChanged()) );
} // createWidget

void
KnobGuiParametric::onColorChanged(int dimension)
{
    if ( (dimension < 0) || ( dimension >= (int)_curves.size() ) ) {
        return;
    }
    double r, g, b;
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->getCurveColor(dimension, &r, &g, &b);

    CurveDescriptor& found = _curves[dimension];
    QColor c;
    c.setRgbF(r, g, b);
    found.curve->setColor(c);

    _curveWidget->update();
}

void
KnobGuiParametric::_hide()
{
    _curveWidget->hide();
    _tree->hide();
    _resetButton->hide();
}

void
KnobGuiParametric::_show()
{
    _curveWidget->show();
    _tree->show();
    _resetButton->show();
}

void
KnobGuiParametric::setEnabled()
{
    KnobParametricPtr knob = _knob.lock();
    bool b = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    _tree->setEnabled(b);
}

void
KnobGuiParametric::updateGUI(int /*dimension*/)
{
    _curveWidget->update();
}

void
KnobGuiParametric::onCurveChanged(int dimension)
{
    if ( (dimension < 0) || ( dimension >= (int)_curves.size() ) ) {
        return;
    }
    CurveDescriptor& found = _curves[dimension];

    // even when there is only one keyframe, there may be tangents!
    if ( (found.curve->getInternalCurve()->getKeyFramesCount() > 0) && found.treeItem->isSelected() ) {
        found.curve->setVisible(true);
    } else {
        found.curve->setVisible(false);
    }
    _curveWidget->update();
}

void
KnobGuiParametric::onItemsSelectionChanged()
{
    std::vector<CurveGuiPtr> curves;

    QList<QTreeWidgetItem*> selectedItems = _tree->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i) {
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->treeItem == selectedItems.at(i) ) {
                if ( it->curve->getInternalCurve()->isAnimated() ) {
                    curves.push_back(it->curve);
                }
                break;
            }
        }
    }

    ///find in the _curves map if an item's map the current

    _curveWidget->showCurvesAndHideOthers(curves);
    _curveWidget->centerOn(curves, true); //remove this if you don't want the editor to switch to a curve on a selection change
}

void
KnobGuiParametric::getSelectedCurves(std::vector<CurveGuiPtr>* selection)
{
    QList<QTreeWidgetItem*> selected = _tree->selectedItems();
    for (int i = 0; i < selected.size(); ++i) {
        //find the items in the curves
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->treeItem == selected.at(i) ) {
                selection->push_back(it->curve);
            }
        }
    }
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
    k->evaluateValueChange(0, k->getCurrentTime(), ViewIdx(0), eValueChangedReasonUserEdited);
}

KnobIPtr
KnobGuiParametric::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiParametric::refreshDimensionName(int dim)
{
    if ( (dim < 0) || ( dim >= (int)_curves.size() ) ) {
        return;
    }
    KnobParametricPtr knob = _knob.lock();
    CurveDescriptor& found = _curves[dim];
    QString name = QString::fromUtf8( knob->getDimensionName(dim).c_str() );
    found.curve->setName(name);
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

#ifdef OFX_EXTENSIONS_NATRON
double
KnobGuiParametric::getScreenPixelRatio() const
{
    return _curveWidget->getScreenPixelRatio();
}
#endif

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

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiParametric.cpp"
