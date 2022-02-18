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

#include "KnobGuiButton.h"

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

#include <ofxNatron.h>

NATRON_NAMESPACE_ENTER
using std::make_pair;

//=============================BUTTON_KNOB_GUI===================================

KnobGuiButton::KnobGuiButton(KnobIPtr knob,
                             KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , _button(0)
{
    _knob = boost::dynamic_pointer_cast<KnobButton>(knob);
}

void
KnobGuiButton::createWidget(QHBoxLayout* layout)
{
    KnobButtonPtr knob = _knob.lock();
    QString label = QString::fromUtf8( knob->getLabel().c_str() );
    QString onIconFilePath = QString::fromUtf8( knob->getIconLabel(true).c_str() );
    QString offIconFilePath = QString::fromUtf8( knob->getIconLabel(false).c_str() );


    if ( !onIconFilePath.isEmpty() && !QFile::exists(onIconFilePath) ) {
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>( knob->getHolder() );
        if (isEffect) {
            //Prepend the resources path
            QString resourcesPath = QString::fromUtf8( isEffect->getNode()->getPluginResourcesPath().c_str() );
            if ( !resourcesPath.endsWith( QLatin1Char('/') ) ) {
                resourcesPath += QLatin1Char('/');
            }
            onIconFilePath.prepend(resourcesPath);
        }
    }
    if ( !offIconFilePath.isEmpty() && !QFile::exists(offIconFilePath) ) {
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>( knob->getHolder() );
        if (isEffect) {
            //Prepend the resources path
            QString resourcesPath = QString::fromUtf8( isEffect->getNode()->getPluginResourcesPath().c_str() );
            if ( !resourcesPath.endsWith( QLatin1Char('/') ) ) {
                resourcesPath += QLatin1Char('/');
            }
            offIconFilePath.prepend(resourcesPath);
        }
    }

    bool checkable = knob->getIsCheckable();
    QIcon icon;
    QPixmap pixChecked, pixUnchecked;
    if ( !offIconFilePath.isEmpty() ) {
        if ( pixUnchecked.load(offIconFilePath) ) {
            icon.addPixmap(pixUnchecked, QIcon::Normal, QIcon::Off);
        }
    }
    if ( !onIconFilePath.isEmpty() ) {
        if ( pixChecked.load(onIconFilePath) ) {
            icon.addPixmap(pixChecked, QIcon::Normal, QIcon::On);
        }
    }

    if ( !icon.isNull() ) {
        _button = new Button( icon, QString(), layout->parentWidget() );
        _button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _button->setIconSize( QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE) );
    } else {
        _button = new Button( label, layout->parentWidget() );
    }
    if (checkable) {
        _button->setCheckable(true);
        bool checked = knob->getValue();
        _button->setChecked(checked);
        _button->setDown(checked);
    }
    QObject::connect( _button, SIGNAL(clicked(bool)), this, SLOT(emitValueChanged(bool)) );
    if ( hasToolTip() ) {
        _button->setToolTip( toolTip() );
    }
    layout->addWidget(_button);
} // KnobGuiButton::createWidget

std::string
KnobGuiButton::getDescriptionLabel() const
{
    return std::string();
}

KnobGuiButton::~KnobGuiButton()
{
}

void
KnobGuiButton::removeSpecificGui()
{
    _button->deleteLater();
}

void
KnobGuiButton::emitValueChanged(bool clicked)
{
    KnobButtonPtr k = _knob.lock();

    assert(k);

    if ( k->getIsCheckable() ) {
        _button->setDown(clicked);
        _button->setChecked(clicked);

        pushUndoCommand( new KnobUndoCommand<bool>(shared_from_this(), k->getValue(), clicked, 0, false) );
    } else {
        k->trigger();
    }
}

void
KnobGuiButton::_hide()
{
    if (_button) {
        _button->hide();
    }
}

void
KnobGuiButton::_show()
{
    if (_button) {
        _button->show();
    }
}

void
KnobGuiButton::updateGUI(int /*dimension*/)
{
    KnobButtonPtr k = _knob.lock();

    if ( k->getIsCheckable() ) {
        bool checked = k->getValue();
        _button->setDown(checked);
        _button->setChecked(checked);
    }
}

void
KnobGuiButton::setEnabled()
{
    KnobButtonPtr knob = _knob.lock();
    bool b = knob->isEnabled(0);

    _button->setEnabled(b);
}

void
KnobGuiButton::setReadOnly(bool readOnly,
                           int /*dimension*/)
{
    _button->setEnabled(!readOnly);
}

KnobIPtr
KnobGuiButton::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiButton::onLabelChangedInternal()
{
    if (_button) {
        _button->setText(QString::fromUtf8(getKnob()->getLabel().c_str()));
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiButton.cpp"
