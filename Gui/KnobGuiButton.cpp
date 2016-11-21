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
#include "Engine/KnobItemsTable.h"
#include "Engine/TimeLine.h"

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

//=============================BUTTON_KNOB_GUI===================================

KnobGuiButton::KnobGuiButton(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , _button(0)
{
    _knob = toKnobButton(knob->getKnob());
}

void
KnobGuiButton::createWidget(QHBoxLayout* layout)
{
    KnobGuiPtr knobUI = getKnobGui();
    KnobButtonPtr knob = _knob.lock();
    QString label = QString::fromUtf8( knob->getLabel().c_str() );
    QString onIconFilePath, offIconFilePath;
    if (knobUI->getLayoutType() == KnobGui::eKnobLayoutTypeViewerUI) {
        onIconFilePath = QString::fromUtf8( knob->getInViewerContextIconFilePath(true).c_str() );
        offIconFilePath = QString::fromUtf8( knob->getInViewerContextIconFilePath(false).c_str() );
    } else {
        onIconFilePath = QString::fromUtf8( knob->getIconLabel(true).c_str() );
        offIconFilePath = QString::fromUtf8( knob->getIconLabel(false).c_str() );
    }

    EffectInstancePtr isEffect = toEffectInstance( knob->getHolder() );
    KnobTableItemPtr isTableItem = toKnobTableItem(knob->getHolder());
    if (isTableItem) {
        isEffect = isTableItem->getModel()->getNode()->getEffectInstance();
    }

    // If the icon path is not absolute, prepend the plug-in resource path
    if ( !onIconFilePath.isEmpty() && !QFile::exists(onIconFilePath) ) {
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
        _button->setFixedSize(TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE));
        _button->setIconSize( QSize(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE)) );
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
    if ( knobUI->hasToolTip() ) {
        knobUI->toolTip(_button, getView());
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
KnobGuiButton::disableButtonBorder()
{
    _button->setStyleSheet(QString::fromUtf8("QPushButton {border: none;}"));
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

        KnobButtonPtr knob = _knob.lock();
        getKnobGui()->pushUndoCommand( new KnobUndoCommand<bool>(knob, knob->getValue(DimIdx(0), getView()), clicked, DimIdx(0), getView()) );
    } else {
        k->trigger();
    }
}

void
KnobGuiButton::setWidgetsVisible(bool visible)
{
    if (_button) {
        _button->setVisible(visible);
    }
}

void
KnobGuiButton::updateGUI()
{
    KnobButtonPtr k = _knob.lock();

    if ( k->getIsCheckable() ) {
        bool checked = k->getValue();
        if (_button->isChecked() == checked) {
            return;
        }
        _button->setDown(checked);
        _button->setChecked(checked);
    }
}

void
KnobGuiButton::setEnabled(const std::vector<bool>& perDimEnabled)
{
    KnobButtonPtr knob = _knob.lock();

    _button->setEnabled(perDimEnabled[0]);
}


void
KnobGuiButton::onLabelChanged()
{
    if (_button) {
        _button->setText(QString::fromUtf8(_knob.lock()->getLabel().c_str()));
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobGuiButton.cpp"
