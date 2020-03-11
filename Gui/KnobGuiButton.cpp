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
#include <QImage>
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

KnobGuiButton::KnobGuiButton(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , _button(0)
{
    _knob = toKnobButton(knob->getKnob());
}

static double overFunctor(double a, double b, double alphaA)
{
    return a + b * (a - alphaA);
}

QPixmap
KnobGuiButton::loadPixmapInternal(bool checked, bool applyColorOverlay, const QColor& overlayColor)
{
    KnobGuiPtr knobUI = getKnobGui();
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return QPixmap();
    }
    EffectInstancePtr isEffect = toEffectInstance( knob->getHolder() );
    KnobTableItemPtr isTableItem = toKnobTableItem(knob->getHolder());
    if (isTableItem) {
        isEffect = isTableItem->getModel()->getNode()->getEffectInstance();
    }

    QString filePath;
    if (knobUI->getLayoutType() == KnobGui::eKnobLayoutTypeViewerUI) {
        filePath = QString::fromUtf8( knob->getInViewerContextIconFilePath(checked).c_str() );
    } else {
        filePath = QString::fromUtf8( knob->getIconLabel(checked).c_str() );
    }
    if ( !filePath.isEmpty() && !QFile::exists(filePath) ) {
        if (isEffect) {
            //Prepend the resources path
            QString resourcesPath = QString::fromUtf8( isEffect->getNode()->getPluginResourcesPath().c_str() );
            if ( !resourcesPath.endsWith( QLatin1Char('/') ) ) {
                resourcesPath += QLatin1Char('/');
            }
            filePath.prepend(resourcesPath);
        }
    }

    if ( !filePath.isEmpty() ) {
#if 0
        QPixmap pix;
        if (pix.load(filePath)) {
            return pix;
        }
#else
        QImage img;
        if ( img.load(filePath) ) {
            if (applyColorOverlay) {

                int depth = img.depth();
                if (depth != 32) {
                    img = img.convertToFormat(QImage::Format_ARGB32);
                }
                depth = img.depth();
                assert(depth == 32);
                for (int y = 0; y < img.height(); ++y) {
                    QRgb* pix = (QRgb*)img.scanLine(y);
                    for (int x = 0; x < img.width(); ++x) {
                        QRgb srcPix = pix[x];
                        double a = qAlpha(srcPix) * (1. / 255);
                        double r = qRed(srcPix) * a * (1. / 255);
                        double g = qGreen(srcPix) * a * (1. / 255);
                        double b = qBlue(srcPix) * a * (1. / 255);

                        r = Image::clamp(overFunctor(overlayColor.redF(), r, overlayColor.alphaF()), 0., 1.);
                        g = Image::clamp(overFunctor(overlayColor.greenF(), g, overlayColor.alphaF()), 0., 1.);
                        b = Image::clamp(overFunctor(overlayColor.blueF(), b, overlayColor.alphaF()), 0., 1.);
                        a = Image::clamp(overFunctor(overlayColor.alphaF(), a, overlayColor.alphaF()) * a, 0., 1.);

                        QRgb p = qRgba(r * 255, g * 255, b * 255, a * 255);
                        img.setPixel(x, y, p);
                    }
                }
            }
            QPixmap pix = QPixmap::fromImage(img);
            return pix;
        }
#endif
    }
    return QPixmap();
} // loadPixmapInternal

void
KnobGuiButton::loadPixmaps(bool applyColorOverlay, const QColor& overlayColor)
{
    QPixmap uncheckedPix = loadPixmapInternal(false, applyColorOverlay, overlayColor);
    QPixmap checkedPix = loadPixmapInternal(true, applyColorOverlay, overlayColor);
    QIcon icon;
    if (!uncheckedPix.isNull()) {
        icon.addPixmap(uncheckedPix, QIcon::Normal, QIcon::Off);
    }
    if (!checkedPix.isNull()) {
        icon.addPixmap(checkedPix, QIcon::Normal, QIcon::On);
    }
    if (!icon.isNull()) {
        _button->setIcon(icon);
    } else {
        KnobButtonPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        QString label = QString::fromUtf8( knob->getLabel().c_str() );
        _button->setText(label);
    }
} // loadPixmap

void
KnobGuiButton::createWidget(QHBoxLayout* layout)
{
    KnobGuiPtr knobUI = getKnobGui();
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    _button = new Button( layout->parentWidget() );
    if (knobUI->getLayoutType() == KnobGui::eKnobLayoutTypeTableItemWidget) {
        _button->setIconSize( QSize(TO_DPIX(NATRON_SMALL_BUTTON_ICON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_ICON_SIZE)) );
    } else {
        _button->setIconSize( QSize(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE)) );
    }

    loadPixmaps(false, QColor());

    if (!_button->icon().isNull()) {
        _button->setFixedSize(TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE));
    }

    bool checkable = knob->getIsCheckable();
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
KnobGuiButton::emitValueChanged(bool clicked)
{
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if ( knob->getIsCheckable() ) {
        _button->setDown(clicked);
        _button->setChecked(clicked);

        getKnobGui()->pushUndoCommand( new KnobUndoCommand<bool>(knob, knob->getValue(DimIdx(0), getView()), clicked, DimIdx(0), getView()) );
    } else {
        knob->trigger();
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
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    if ( knob->getIsCheckable() ) {
        bool checked = knob->getValue();
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
    _button->setEnabled(perDimEnabled[0]);
}

void
KnobGuiButton::reflectMultipleSelection(bool dirty)
{
    QColor c;
    c.setRgbF(0.2, 0.2, 0.2);
    c.setAlphaF(0.2);
    loadPixmaps(dirty, c);
}


void
KnobGuiButton::reflectSelectionState(bool selected)
{
    QColor c;
    double r,g,b;
    appPTR->getCurrentSettings()->getSelectionColor(&r, &g, &b);
    c.setRgbF(r,g,b);
    c.setAlphaF(0.2);
    loadPixmaps(selected, c);
}


void
KnobGuiButton::onLabelChanged()
{
    if (_button) {
        KnobButtonPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        _button->setText(QString::fromUtf8(knob->getLabel().c_str()));
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiButton.cpp"
