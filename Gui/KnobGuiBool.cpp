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

#include "KnobGuiBool.h"

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

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER
using std::make_pair;

//==========================KnobBool_GUI======================================

Bool_CheckBox::Bool_CheckBox(const KnobGuiPtr& knob,
                             DimSpec dimension,
                             ViewIdx view,
                             QWidget* parent)
    : AnimatedCheckBox(parent)
    , useCustomColor(false)
    , customColor()
    , _dnd( KnobWidgetDnD::create(knob, dimension, view, this) )
{
}

Bool_CheckBox::~Bool_CheckBox()
{
}

void
Bool_CheckBox::getBackgroundColor(double *r,
                                  double *g,
                                  double *b) const
{
    if (useCustomColor) {
        *r = customColor.redF();
        *g = customColor.greenF();
        *b = customColor.blueF();
    } else {
        AnimatedCheckBox::getBackgroundColor(r, g, b);
    }
}

void
Bool_CheckBox::enterEvent(QEvent* e)
{
    _dnd->mouseEnter(e);
    AnimatedCheckBox::enterEvent(e);
}

void
Bool_CheckBox::leaveEvent(QEvent* e)
{
    _dnd->mouseLeave(e);
    AnimatedCheckBox::leaveEvent(e);
}

void
Bool_CheckBox::keyPressEvent(QKeyEvent* e)
{
    _dnd->keyPress(e);
    AnimatedCheckBox::keyPressEvent(e);
}

void
Bool_CheckBox::keyReleaseEvent(QKeyEvent* e)
{
    _dnd->keyRelease(e);
    AnimatedCheckBox::keyReleaseEvent(e);
}

void
Bool_CheckBox::mousePressEvent(QMouseEvent* e)
{
    if ( !_dnd->mousePress(e) ) {
        AnimatedCheckBox::mousePressEvent(e);
    }
}

void
Bool_CheckBox::mouseMoveEvent(QMouseEvent* e)
{
    if ( !_dnd->mouseMove(e) ) {
        AnimatedCheckBox::mouseMoveEvent(e);
    }
}

void
Bool_CheckBox::mouseReleaseEvent(QMouseEvent* e)
{
    _dnd->mouseRelease(e);
    AnimatedCheckBox::mouseReleaseEvent(e);
}

void
Bool_CheckBox::dragEnterEvent(QDragEnterEvent* e)
{
    if ( !_dnd->dragEnter(e) ) {
        AnimatedCheckBox::dragEnterEvent(e);
    }
}

void
Bool_CheckBox::dragMoveEvent(QDragMoveEvent* e)
{
    if ( !_dnd->dragMove(e) ) {
        AnimatedCheckBox::dragMoveEvent(e);
    }
}

void
Bool_CheckBox::dropEvent(QDropEvent* e)
{
    if ( !_dnd->drop(e) ) {
        AnimatedCheckBox::dropEvent(e);
    }
}

void
Bool_CheckBox::focusInEvent(QFocusEvent* e)
{
    _dnd->focusIn();
    AnimatedCheckBox::focusInEvent(e);
}

void
Bool_CheckBox::focusOutEvent(QFocusEvent* e)
{
    _dnd->focusOut();
    AnimatedCheckBox::focusOutEvent(e);
}

KnobGuiBool::KnobGuiBool(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , _checkBox(0)
{
    _knob = toKnobBool(knob->getKnob());
}

void
KnobGuiBool::createWidget(QHBoxLayout* layout)
{
    KnobGuiPtr knobUI = getKnobGui();
    _checkBox = new Bool_CheckBox( knobUI, DimIdx(0), getView(), layout->parentWidget() );
    onLabelChanged();
    //_checkBox->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QObject::connect( _checkBox, SIGNAL(clicked(bool)), this, SLOT(onCheckBoxStateChanged(bool)) );
    QObject::connect( knobUI.get(), SIGNAL(labelClicked(bool)), this, SLOT(onLabelClicked(bool)) );

    ///set the copy/link actions in the right click menu
    KnobGuiWidgets::enableRightClickMenu(knobUI, _checkBox, DimIdx(0), getView());

    layout->addWidget(_checkBox);
}

KnobGuiBool::~KnobGuiBool()
{
}


void
KnobGuiBool::updateGUI()
{
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool checked = knob->getValue(DimIdx(0), getView());
    if (_checkBox->isChecked() == checked) {
        return;
    }
    _checkBox->setChecked(checked);
}

void
KnobGuiBool::reflectAnimationLevel(DimIdx /*dimension*/,
                                   AnimationLevelEnum level)
{
    int value = (int)level;
    if ( value != _checkBox->getAnimation() ) {
        KnobBoolPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        bool isEnabled = knob->isEnabled();
        _checkBox->setReadOnly(level == eAnimationLevelExpression || !isEnabled);
        _checkBox->setAnimation(value);
    }
}

void
KnobGuiBool::onLabelChanged()
{
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    const std::string& label = knob->getLabel();

    if ( (label == "R") || (label == "r") || (label == "red") ) {
        QColor color;
        color.setRgbF(0.851643, 0.196936, 0.196936);
        _checkBox->setCustomColor(color, true);
    } else if ( (label == "G") || (label == "g") || (label == "green") ) {
        QColor color;
        color.setRgbF(0, 0.654707, 0);
        _checkBox->setCustomColor(color, true);
    } else if ( (label == "B") || (label == "b") || (label == "blue") ) {
        QColor color;
        color.setRgbF(0.345293, 0.345293, 1);
        _checkBox->setCustomColor(color, true);
    } else if ( (label == "A") || (label == "a") || (label == "alpha") ) {
        QColor color;
        color.setRgbF(0.398979, 0.398979, 0.398979);
        _checkBox->setCustomColor(color, true);
    } else {
        _checkBox->setCustomColor(Qt::black, false);
    }
}

void
KnobGuiBool::onLabelClicked(bool b)
{
    if ( _checkBox->getReadOnly() ) {
        return;
    }
    _checkBox->setChecked(b);
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    getKnobGui()->pushUndoCommand( new KnobUndoCommand<bool>(knob, knob->getValue(DimIdx(0), getView()), b, DimIdx(0), getView()) );
}

void
KnobGuiBool::onCheckBoxStateChanged(bool b)
{
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    getKnobGui()->pushUndoCommand( new KnobUndoCommand<bool>(knob, knob->getValue(DimIdx(0), getView()), b, DimIdx(0), getView()) );
}

void
KnobGuiBool::setWidgetsVisible(bool visible)
{
    _checkBox->setVisible(visible);
}

void
KnobGuiBool::setEnabled(const std::vector<bool>& perDimEnabled)
{
    _checkBox->setReadOnly(!perDimEnabled[0]);
}

void
KnobGuiBool::reflectMultipleSelection(bool dirty)
{
    _checkBox->setIsSelectedMultipleTimes(dirty);
}

void
KnobGuiBool::reflectSelectionState(bool selected)
{
    _checkBox->setIsSelected(selected);
}

void
KnobGuiBool::reflectLinkedState(DimIdx /*dimension*/, bool linked)
{
    _checkBox->setLinked(linked);
}

void
KnobGuiBool::updateToolTip()
{
    KnobGuiPtr knobgui = getKnobGui();
    if ( knobgui->hasToolTip() ) {
        KnobBoolPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        knobgui->toolTip(_checkBox, getView());
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiBool.cpp"
