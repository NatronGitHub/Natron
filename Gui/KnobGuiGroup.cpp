/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "KnobGuiGroup.h"

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
#include "Engine/KnobUndoCommand.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/KnobGui.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/PropertiesBinWrapper.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER


KnobGuiGroup::KnobGuiGroup(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , _checked(false)
    , _button(0)
    , _children()
    , _knob( toKnobGroup(knob->getKnob()) )
{
}

KnobGuiGroup::~KnobGuiGroup()
{
}

void
KnobGuiGroup::addKnob(const KnobGuiPtr& child)
{
    _children.push_back(child);
}

bool
KnobGuiGroup::isChecked() const
{
    return getKnobGui()->hasWidgetBeenCreated() ? _button->isChecked() : true;
}

void
KnobGuiGroup::createWidget(QHBoxLayout* layout)
{
    _button = new GroupBoxLabel( layout->parentWidget() );
    KnobGuiPtr knobUI = getKnobGui();
    if ( knobUI->hasToolTip() ) {
        knobUI->toolTip(_button, getView());
    }
    KnobGroupPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    _checked = knob->getValue(DimIdx(0), getView());
    _button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _button->setChecked(_checked);
    QObject::connect( _button, SIGNAL(checked(bool)), this, SLOT(onCheckboxChecked(bool)) );
    layout->addWidget(_button);
}

static void setVisibilityRecursive(const KnobGuiPtr& knob, bool checked)
{
    if (!checked) {
        knob->hide();
    } else if ( !knob->getKnob()->getIsSecret() ) {
        knob->show();
    }
    KnobGuiGroup* isGroup = dynamic_cast<KnobGuiGroup*>(knob.get());
    if (isGroup) {
        const std::list<KnobGuiWPtr>& children = isGroup->getChildren();
        for (std::list<KnobGuiWPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
            KnobGuiPtr child = it->lock();
            if (!child) {
                continue;
            }
            setVisibilityRecursive(child, checked);
        }
    }
}

void
KnobGuiGroup::setCheckedInternal(bool checked,
                                 bool userRequested)
{
    if (checked == _checked) {
        return;
    }
    _checked = checked;

    if (userRequested) {
        KnobGroupPtr knob = _knob.lock();
        if (knob) {
            knob->setValue(checked);
        }
    }

    //getGui()->getPropertiesBin()->setUpdatesEnabled(false);
    for (std::list<KnobGuiWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
        KnobGuiPtr knob = it->lock();
        if (!knob) {
            continue;
        }
        setVisibilityRecursive(knob, checked);

    }
}

void
KnobGuiGroup::onCheckboxChecked(bool b)
{
    setCheckedInternal(b, true);
}

bool
KnobGuiGroup::eventFilter(QObject */*target*/,
                          QEvent* /*event*/)
{
    //if(e->type() == QEvent::Paint){

    ///discard the paint event
    return true;
    // }
    //return QObject::eventFilter(target, event);
}

void
KnobGuiGroup::updateGUI()
{
    KnobGroupPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool b = knob->getValue();

    setCheckedInternal(b, false);
    if (_button) {
        if (_button->isChecked() == b) {
            return;
        }
        _button->setChecked(b);
    }
}

void
KnobGuiGroup::setWidgetsVisible(bool visible)
{
    if (_button) {
        _button->setVisible(visible);
    }
    for (std::list<KnobGuiWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
        KnobGuiPtr k = it->lock();
        if (!k) {
            continue;
        }
        if (visible) {
            k->show();
        } else {
            k->hide();
        }
    }

}

void
KnobGuiGroup::setEnabled(const std::vector<bool>& perDimEnabled)
{
    if (_button) {
        _button->setEnabled(perDimEnabled[0]);
    }

    if (perDimEnabled[0]) {
        for (std::list<KnobGuiWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
            KnobGuiPtr k = it->lock();
            if (!k) {
                continue;
            }
            k->setEnabledSlot();
        }

    } else {
        for (std::list<KnobGuiWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
            KnobGuiPtr k = it->lock();
            if (!k) {
                continue;
            }
            k->onFrozenChanged(true);
        }
    }
}



NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiGroup.cpp"
