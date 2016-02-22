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
#include "Gui/KnobWidgetDnD.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/Utils.h"

#include "ofxNatron.h"


NATRON_NAMESPACE_ENTER;
using std::make_pair;

//==========================KnobBool_GUI======================================

Bool_CheckBox::Bool_CheckBox(KnobGui* knob, int dimension, QWidget* parent)
: AnimatedCheckBox(parent)
, useCustomColor(false)
, customColor()
, _dnd(new KnobWidgetDnD(knob, dimension, this))
{
}

Bool_CheckBox::~Bool_CheckBox()
{
}

void
Bool_CheckBox::getBackgroundColor(double *r,double *g,double *b) const
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
    if (!_dnd->mousePress(e)) {
        AnimatedCheckBox::mousePressEvent(e);
    }
}

void
Bool_CheckBox::mouseMoveEvent(QMouseEvent* e)
{
    if (!_dnd->mouseMove(e)) {
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
    if (!_dnd->dragEnter(e)) {
        AnimatedCheckBox::dragEnterEvent(e);
    }
}

void
Bool_CheckBox::dragMoveEvent(QDragMoveEvent* e)
{
    if (!_dnd->dragMove(e)) {
        AnimatedCheckBox::dragMoveEvent(e);
    }
}
void
Bool_CheckBox::dropEvent(QDropEvent* e)
{
    if (!_dnd->drop(e)) {
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

KnobGuiBool::KnobGuiBool(KnobPtr knob,
                           DockablePanel *container)
    : KnobGui(knob, container)
    , _checkBox(0)
{
    _knob = boost::dynamic_pointer_cast<KnobBool>(knob);
}

void
KnobGuiBool::createWidget(QHBoxLayout* layout)
{
    _checkBox = new Bool_CheckBox(this, 0,layout->parentWidget() );
    onLabelChangedInternal();
    //_checkBox->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QObject::connect( _checkBox, SIGNAL(clicked(bool)), this, SLOT(onCheckBoxStateChanged(bool)) );
    QObject::connect( this, SIGNAL(labelClicked(bool)), this, SLOT(onLabelClicked(bool)) );

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_checkBox,0);

    layout->addWidget(_checkBox);
}

KnobGuiBool::~KnobGuiBool()
{

}

void KnobGuiBool::removeSpecificGui()
{
    _checkBox->hide();
    _checkBox->deleteLater();
    _checkBox = 0;
    
}

void
KnobGuiBool::updateGUI(int /*dimension*/)
{
    _checkBox->setChecked( _knob.lock()->getValue(0) );
}

void
KnobGuiBool::reflectAnimationLevel(int /*dimension*/,
                                    AnimationLevelEnum level)
{
    int value;
    switch (level) {
        case eAnimationLevelNone:
            value = 0;
            break;
        case eAnimationLevelInterpolatedValue:
            value = 1;
            break;
        case eAnimationLevelOnKeyframe:
            value = 2;
            break;
        default:
            value = 0;
            break;
    }
    if (value != _checkBox->getAnimation()) {
        _checkBox->setAnimation(value);
    }
}

void
KnobGuiBool::onLabelChangedInternal()
{
    const std::string& label = _knob.lock()->getLabel();
    if (label == "R" || label == "r" || label == "red") {
        QColor color;
        color.setRgbF(0.851643,0.196936,0.196936);
        _checkBox->setCustomColor(color, true);
    } else if (label == "G" || label == "g" || label == "green") {
        QColor color;
        color.setRgbF(0,0.654707,0);
        _checkBox->setCustomColor(color, true);
    } else if (label == "B" || label == "b" || label == "blue") {
        QColor color;
        color.setRgbF(0.345293,0.345293,1);
        _checkBox->setCustomColor(color, true);
    } else if (label == "A" || label == "a" || label == "alpha") {
        QColor color;
        color.setRgbF(0.398979,0.398979,0.398979);
        _checkBox->setCustomColor(color, true);
    } else {
        _checkBox->setCustomColor(Qt::black, false);
    }
}

void
KnobGuiBool::onLabelClicked(bool b)
{
    if (_checkBox->getReadOnly()) {
        return;
    }
    _checkBox->setChecked(b);
    pushUndoCommand( new KnobUndoCommand<bool>(this,_knob.lock()->getValue(0),b, 0, false) );
}

void
KnobGuiBool::onCheckBoxStateChanged(bool b)
{
    pushUndoCommand( new KnobUndoCommand<bool>(this,_knob.lock()->getValue(0),b, 0, false) );
}

void
KnobGuiBool::_hide()
{
    _checkBox->hide();
}

void
KnobGuiBool::_show()
{
    _checkBox->show();
}

void
KnobGuiBool::setEnabled()
{
    boost::shared_ptr<KnobBool> knob = _knob.lock();

    bool b = knob->isEnabled(0)  && knob->getExpression(0).empty();

    _checkBox->setReadOnly(!b);
}

void
KnobGuiBool::setReadOnly(bool readOnly,
                          int /*dimension*/)
{
    _checkBox->setReadOnly(readOnly);
}

void
KnobGuiBool::setDirty(bool dirty)
{
    _checkBox->setDirty(dirty);
}

KnobPtr
KnobGuiBool::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiBool::reflectExpressionState(int /*dimension*/,
                                     bool hasExpr)
{
    bool isEnabled = _knob.lock()->isEnabled(0);
    _checkBox->setAnimation(3);
    _checkBox->setReadOnly(hasExpr || !isEnabled);
}

void
KnobGuiBool::updateToolTip()
{
    if ( hasToolTip() ) {
        QString tt = toolTip();
        for (int i = 0; i < _knob.lock()->getDimension(); ++i) {
            _checkBox->setToolTip( tt );
        }
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobGuiBool.cpp"
