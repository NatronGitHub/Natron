/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/PropertiesBinWrapper.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER
using std::make_pair;


//=============================GROUP_KNOB_GUI===================================

KnobGuiGroup::KnobGuiGroup(KnobIPtr knob,
                           KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , _checked(false)
    , _button(0)
    , _children()
    , _childrenToEnable()
    , _tabGroup(0)
    , _knob( boost::dynamic_pointer_cast<KnobGroup>(knob) )
{
}

KnobGuiGroup::~KnobGuiGroup()
{
}

TabGroup*
KnobGuiGroup::getOrCreateTabWidget()
{
    if (_tabGroup) {
        return _tabGroup;
    }

    _tabGroup = new TabGroup( getContainer()->getContainerWidget() );

    return _tabGroup;
}

void
KnobGuiGroup::removeTabWidget()
{
    delete _tabGroup;
    _tabGroup = 0;
}

void
KnobGuiGroup::removeSpecificGui()
{
    _button->deleteLater();
}

void
KnobGuiGroup::addKnob(const KnobGuiPtr& child)
{
    _children.push_back(child);
}

bool
KnobGuiGroup::isChecked() const
{
    return hasWidgetBeenCreated() ? _button->isChecked() : true;
}

void
KnobGuiGroup::createWidget(QHBoxLayout* layout)
{
    _button = new GroupBoxLabel( layout->parentWidget() );
    if ( hasToolTip() ) {
        _button->setToolTip( toolTip() );
    }
    KnobGroupPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    _checked = knob->getValue();
    _button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _button->setChecked(_checked);
    QObject::connect( _button, SIGNAL(checked(bool)), this, SLOT(onCheckboxChecked(bool)) );
    layout->addWidget(_button);
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
            knob->setValue(checked, ViewSpec::all(), 0, eValueChangedReasonUserEdited, 0);
        }
    }

    ///get the current index of the group knob in the layout, and reinsert
    ///the children back with an offset relative to the group.
    int realIndexInLayout = getActualIndexInLayout();
    int startChildIndex = realIndexInLayout + 1;
    //getGui()->getPropertiesBin()->setUpdatesEnabled(false);
    for (std::list<KnobGuiWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
        KnobGuiPtr knob = it->lock();
        if (!knob) {
            continue;
        }
        if (!checked) {
            knob->hide();
        } else if ( !knob->getKnob()->getIsSecret() ) {
            knob->show(startChildIndex);
            if ( knob->getKnob()->isNewLineActivated() ) {
                ++startChildIndex;
            }
        }
    }
    if (_tabGroup) {
        _tabGroup->setVisible(checked);
    }
    //getGui()->getPropertiesBin()->setUpdatesEnabled(true);
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
KnobGuiGroup::updateGUI(int /*dimension*/)
{
    KnobGroupPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool b = knob->getValue(0);

    setCheckedInternal(b, false);
    if (_button) {
        _button->setChecked(b);
    }
}

void
KnobGuiGroup::_hide()
{
    if (_button) {
        _button->hide();
    }
    for (std::list<KnobGuiWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
        KnobGuiPtr k = it->lock();
        if (!k) {
            continue;
        }
        k->hide();
    }
    if (_tabGroup) {
        _tabGroup->hide();
    }
}

void
KnobGuiGroup::_show()
{
//    if ( _knob->getIsSecret() ) {
//        return;
//    }
    if (_button) {
        _button->show();
    }

    if (_checked) {
        for (std::list<KnobGuiWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
            KnobGuiPtr k = it->lock();
            if (!k) {
                continue;
            }
            k->show();
        }
    }
    if (_tabGroup) {
        _tabGroup->show();
    }
}

void
KnobGuiGroup::setEnabled()
{
    KnobGroupPtr knob = _knob.lock();
    bool enabled = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    if (_button) {
        _button->setEnabled(enabled);
    }
    if (_tabGroup) {
        _tabGroup->setEnabled(enabled);
    }
    if (enabled) {
        for (U32 i = 0; i < _childrenToEnable.size(); ++i) {
            for (U32 j = 0; j < _childrenToEnable[i].second.size(); ++j) {
                KnobGuiPtr k = _childrenToEnable[i].first.lock();
                if (!k) {
                    continue;
                }
                k->getKnob()->setEnabled(_childrenToEnable[i].second[j], true);
            }
        }
    } else {
        _childrenToEnable.clear();
        for (std::list<KnobGuiWPtr>::iterator it = _children.begin(); it != _children.end(); ++it) {
            KnobGuiPtr k = it->lock();
            if (!k) {
                continue;
            }
            std::vector<int> dimensions;
            for (int j = 0; j < k->getKnob()->getDimension(); ++j) {
                if ( k->getKnob()->isEnabled(j) ) {
                    k->getKnob()->setEnabled(j, false);
                    dimensions.push_back(j);
                }
            }
            _childrenToEnable.push_back( std::make_pair(*it, dimensions) );
        }
    }
}

KnobIPtr
KnobGuiGroup::getKnob() const
{
    return _knob.lock();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiGroup.cpp"
