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
#include <QDebug>
#include <QFontComboBox>
#include <QDialogButtonBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

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
#include "Gui/Utils.h"

#include "ofxNatron.h"


using namespace Natron;
using std::make_pair;


//=============================KnobGuiParametric===================================

KnobGuiParametric::KnobGuiParametric(boost::shared_ptr<KnobI> knob,
                                       DockablePanel *container)
: KnobGui(knob, container)
, treeColumn(NULL)
, _curveWidget(NULL)
, _tree(NULL)
, _resetButton(NULL)
, _curves()
{
    _knob = boost::dynamic_pointer_cast<KnobParametric>(knob);
    QObject::connect(_knob.lock().get(), SIGNAL(curveColorChanged(int)), this, SLOT(onColorChanged(int)));
}

KnobGuiParametric::~KnobGuiParametric()
{
    
}

void KnobGuiParametric::removeSpecificGui()
{
    delete _curveWidget;
    delete treeColumn;
}

void
KnobGuiParametric::createWidget(QHBoxLayout* layout)
{
    boost::shared_ptr<KnobParametric> knob = _knob.lock();
    QObject::connect( knob.get(), SIGNAL( curveChanged(int) ), this, SLOT( onCurveChanged(int) ) );

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

    _resetButton = new Button("Reset",treeColumn);
    _resetButton->setToolTip( Natron::convertFromPlainText(tr("Reset the selected curves in the tree to their default shape."), Qt::WhiteSpaceNormal) );
    QObject::connect( _resetButton, SIGNAL( clicked() ), this, SLOT( resetSelectedCurves() ) );
    treeColumnLayout->addWidget(_resetButton);

    layout->addWidget(treeColumn);

    _curveWidget = new CurveWidget( getGui(),this, boost::shared_ptr<TimeLine>(),layout->parentWidget() );
    _curveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if ( hasToolTip() ) {
        _curveWidget->setToolTip( toolTip() );
    }
    layout->addWidget(_curveWidget);


    std::vector<boost::shared_ptr<CurveGui> > visibleCurves;
    for (int i = 0; i < knob->getDimension(); ++i) {
        QString curveName = knob->getDimensionName(i).c_str();
        boost::shared_ptr<KnobCurveGui> curve(new KnobCurveGui(_curveWidget,knob->getParametricCurve(i),this,i,curveName,QColor(255,255,255),1.));
        _curveWidget->addCurveAndSetColor(curve);
        QColor color;
        double r,g,b;
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
        _curves.insert( std::make_pair(i, desc) );
        if ( curve->isVisible() ) {
            visibleCurves.push_back(curve);
        }
    }

    _curveWidget->centerOn(visibleCurves);
    QObject::connect( _tree, SIGNAL( itemSelectionChanged() ),this,SLOT( onItemsSelectionChanged() ) );
} // createWidget

void
KnobGuiParametric::onColorChanged(int dimension)
{
    double r, g, b;
    _knob.lock()->getCurveColor(dimension, &r, &g, &b);
    CurveGuis::iterator found = _curves.find(dimension);
    if (found != _curves.end()) {
        QColor c;
        c.setRgbF(r, g, b);
        found->second.curve->setColor(c);
    }
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
    boost::shared_ptr<KnobParametric> knob = _knob.lock();
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
    CurveGuis::iterator it = _curves.find(dimension);

    assert( it != _curves.end() );

    // even when there is only one keyframe, there may be tangents!
    if ( (it->second.curve->getInternalCurve()->getKeyFramesCount() > 0) && it->second.treeItem->isSelected() ) {
        it->second.curve->setVisible(true);
    } else {
        it->second.curve->setVisible(false);
    }
    _curveWidget->update();
}

void
KnobGuiParametric::onItemsSelectionChanged()
{
    std::vector<boost::shared_ptr<CurveGui> > curves;

    QList<QTreeWidgetItem*> selectedItems = _tree->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i) {
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->second.treeItem == selectedItems.at(i) ) {
                if ( it->second.curve->getInternalCurve()->isAnimated() ) {
                    curves.push_back(it->second.curve);
                }
                break;
            }
        }
    }

    ///find in the _curves map if an item's map the current

    _curveWidget->showCurvesAndHideOthers(curves);
    _curveWidget->centerOn(curves); //remove this if you don't want the editor to switch to a curve on a selection change
}

void
KnobGuiParametric::getSelectedCurves(std::vector<boost::shared_ptr<CurveGui> >* selection)
{
    QList<QTreeWidgetItem*> selected = _tree->selectedItems();
    for (int i = 0; i < selected.size(); ++i) {
        //find the items in the curves
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->second.treeItem == selected.at(i) ) {
                selection->push_back(it->second.curve);
            }
        }
    }
}

void
KnobGuiParametric::resetSelectedCurves()
{
    QList<QTreeWidgetItem*> selected = _tree->selectedItems();
    boost::shared_ptr<KnobParametric> k = _knob.lock();
    for (int i = 0; i < selected.size(); ++i) {
        //find the items in the curves
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->second.treeItem == selected.at(i) ) {
                k->resetToDefaultValue(it->second.curve->getDimension());
                break;
            }
        }
    }
    k->evaluateValueChange(0, k->getCurrentTime(), Natron::eValueChangedReasonUserEdited);
}

boost::shared_ptr<KnobI> KnobGuiParametric::getKnob() const
{
    return _knob.lock();
}

