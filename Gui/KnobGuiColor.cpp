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

#include "KnobGuiColor.h"

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
#include "Engine/Utils.h" // convertFromPlainText
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
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/SpinBoxValidator.h"
#include "Gui/TabGroup.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER


ColorPickerLabel::ColorPickerLabel(bool simplified,
                                   KnobGui::KnobLayoutTypeEnum layout,
                                   KnobGuiColor* knob,
                                   QWidget* parent)
    : Label(parent)
    , _simplified(simplified)
    , _layout(layout)
    , _pickingEnabled(false)
    , _currentColor()
    , _knob(knob)
{
    setMouseTracking(true);
}

void
ColorPickerLabel::mousePressEvent(QMouseEvent*)
{
    if (_layout == KnobGui::eKnobLayoutTypeTableItemWidget) {
        _knob->showColorDialog();
        return;
    }
    if (_simplified) {
        return;
    }
    _pickingEnabled = !_pickingEnabled;
    Q_EMIT pickingEnabled(_pickingEnabled);
    setColor(_currentColor); //< refresh the icon
}

void
ColorPickerLabel::enterEvent(QEvent*)
{
    QToolTip::showText( QCursor::pos(), toolTip() );
}

void
ColorPickerLabel::leaveEvent(QEvent*)
{
    QToolTip::hideText();
}

void
ColorPickerLabel::setPickingEnabled(bool enabled)
{
    if ( !isEnabled() ) {
        return;
    }
    _pickingEnabled = enabled;
    setColor(_currentColor);
}

void
ColorPickerLabel::setEnabledMode(bool enabled)
{
    setEnabled(enabled);
    if (!enabled && _pickingEnabled) {
        _pickingEnabled = false;
        setColor(_currentColor);
        if (_knob) {
            ViewIdx view = _knob->getView();
            KnobGuiPtr knobUI = _knob->getKnobGui();
            knobUI->getGui()->removeColorPicker( toKnobColor( knobUI->getKnob() ), view );
        }
    }
}

void
ColorPickerLabel::setColor(const QColor & color)
{
    _currentColor = color;

    if (_pickingEnabled && _knob) {
        //draw the picker on top of the label
        QPixmap pickerIcon;
        appPTR->getIcon(NATRON_PIXMAP_COLOR_PICKER, &pickerIcon);
        QImage pickerImg = pickerIcon.toImage();
        QImage img(pickerIcon.width(), pickerIcon.height(), QImage::Format_ARGB32);
        img.fill( color.rgb() );


        int h = pickerIcon.height();
        int w = pickerIcon.width();
        for (int i = 0; i < h; ++i) {
            const QRgb* data = (QRgb*)pickerImg.scanLine(i);
            if ( (i == 0) || (i == h - 1) ) {
                for (int j = 0; j < w; ++j) {
                    img.setPixel( j, i, qRgb(0, 0, 0) );
                }
            } else {
                for (int j = 0; j < w; ++j) {
                    if ( (j == 0) || (j == w - 1) ) {
                        img.setPixel( j, i, qRgb(0, 0, 0) );
                    } else {
                        int alpha = qAlpha(data[j]);
                        if (alpha > 0) {
                            img.setPixel(j, i, data[j]);
                        }
                    }
                }
            }
        }
        QPixmap pix = QPixmap::fromImage(img); //.scaled(20, 20);
        setPixmap(pix);
    } else {
        QImage img(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE, QImage::Format_ARGB32);
        int h = img.height();
        int w = img.width();
        QRgb c = color.rgb();
        for (int i = 0; i < h; ++i) {
            if ( (i == 0) || (i == h - 1) ) {
                for (int j = 0; j < w; ++j) {
                    img.setPixel( j, i, qRgb(0, 0, 0) );
                }
            } else {
                for (int j = 0; j < w; ++j) {
                    if ( (j == 0) || (j == w - 1) ) {
                        img.setPixel( j, i, qRgb(0, 0, 0) );
                    } else {
                        img.setPixel(j, i, c);
                    }
                }
            }
        }
        QPixmap pix = QPixmap::fromImage(img);
        setPixmap(pix);
    }
} // ColorPickerLabel::setColor

KnobGuiColor::KnobGuiColor(const KnobGuiPtr& knobUI, ViewIdx view)
    : KnobGuiValue(knobUI, view)
    , _knob( toKnobColor(knobUI->getKnob()) )
    , _colorLabel(0)
    , _colorDialogButton(0)
    , _useSimplifiedUI(false)
    , _uiColorspaceLut(0)
    , _internalColorspaceLut(0)
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        throw std::logic_error(__func__);
    }
    int nDims = knob->getNDimensions();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    _useSimplifiedUI = knob && knob->isSimplified();
    if (!_useSimplifiedUI) {
        DimIdx singleDim;
        bool singleDimEnabled = knobUI->isSingleDimensionalEnabled(&singleDim);
        if (knobUI->getLayoutType() == KnobGui::eKnobLayoutTypeViewerUI && !singleDimEnabled) {
            _useSimplifiedUI = true;
        }
    }
    const std::string& uiName = knob->getUIColorspaceName();
    const std::string& internalName = knob->getInternalColorspaceName();
    _uiColorspaceLut = Color::LutManager::findLut(uiName);
    _internalColorspaceLut = Color::LutManager::findLut(internalName);
}

void
KnobGuiColor::connectKnobSignalSlots()
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    QObject::connect( knob.get(), SIGNAL(pickingEnabled(ViewSetSpec,bool)), this, SLOT(onInternalKnobPickingEnabled(ViewSetSpec,bool)) );
}

void
KnobGuiColor::getIncrements(std::vector<double>* increments) const
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    int nDims = knob->getNDimensions();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    increments->resize(nDims);
    for (int i = 0; i < nDims; ++i) {
        (*increments)[i] = 0.001;
    }
}

void
KnobGuiColor::getDecimals(std::vector<int>* decimals) const
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    int nDims = knob->getNDimensions();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    decimals->resize(nDims);
    for (int i = 0; i < nDims; ++i) {
        (*decimals)[i] = 6;
    }
}

void
KnobGuiColor::addExtraWidgets(QHBoxLayout* containerLayout)
{
    containerLayout->addSpacing( TO_DPIX(10) );
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    KnobGuiPtr knobUI = getKnobGui();
    if (!knobUI) {
        return;
    }
    _colorLabel = new ColorPickerLabel( _useSimplifiedUI, knobUI->getLayoutType(), this, containerLayout->widget() );
    if (!_useSimplifiedUI && knobUI->getLayoutType() != KnobGui::eKnobLayoutTypeTableItemWidget) {
        _colorLabel->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("To pick a color on a viewer, click this and then press control + left click on any viewer.\n"
                                                                   "You can also pick the average color of a given rectangle by holding control + shift + left click\n. "
                                                                   "To deselect the picker left click anywhere."
                                                                   "Note that by default %1 converts to linear the color picked\n"
                                                                   "because all the processing pipeline is linear, but you can turn this off in the\n"
                                                                   "preferences panel.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ), NATRON_NAMESPACE::WhiteSpaceNormal) );
    }

    QSize medSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    QSize medIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );

    _colorLabel->setFixedSize(medSize);
    QObject::connect( _colorLabel, SIGNAL(pickingEnabled(bool)), this, SLOT(onColorLabelPickingEnabled(bool)) );
    containerLayout->addWidget(_colorLabel);

    if (_useSimplifiedUI) {
        containerLayout->addSpacing( TO_DPIX(5) );
    }

    if (knobUI->getLayoutType() != KnobGui::eKnobLayoutTypeTableItemWidget) {
        QPixmap buttonPix;
        appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, NATRON_MEDIUM_BUTTON_ICON_SIZE, &buttonPix);
        _colorDialogButton = new Button( QIcon(buttonPix), QString(), containerLayout->widget() );
        _colorDialogButton->setFixedSize(medSize);
        _colorDialogButton->setIconSize(medIconSize);
        _colorDialogButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Open the color dialog."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _colorDialogButton->setFocusPolicy(Qt::NoFocus);
        QObject::connect( _colorDialogButton, SIGNAL(clicked()), this, SLOT(showColorDialog()) );
        containerLayout->addWidget(_colorDialogButton);
    }
    if (_useSimplifiedUI) {
        setWidgetsVisibleInternal(false);
        KnobGuiWidgets::enableRightClickMenu(getKnobGui(), _colorLabel, DimSpec::all(), getView());
    }
}

void
KnobGuiColor::setAllDimensionsVisible(bool visible)
{
    if (_useSimplifiedUI) {
        return;
    }
    KnobGuiValue::setAllDimensionsVisible(visible);
}

void
KnobGuiColor::updateLabel(double r,
                          double g,
                          double b,
                          double a)
{
    QColor color;
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    convertFromInternalToUIColorspace(&r, &g, &b);
    color.setRgbF( Image::clamp<qreal>(r, 0., 1.),
                   Image::clamp<qreal>(g, 0., 1.),
                   Image::clamp<qreal>(b, 0., 1.),
                   Image::clamp<qreal>(a, 0., 1.) );
    _colorLabel->setColor(color);
}

void
KnobGuiColor::onInternalKnobPickingEnabled(ViewSetSpec view, bool enabled)
{
    if (!view.isAll() && (int)view != (int)getView()) {
        return;
    }
    if (!_colorLabel) {
        return;
    }
    if (_colorLabel->isPickingEnabled() == enabled) {
        return;
    }
    _colorLabel->setPickingEnabled(enabled);
    setPickingEnabledInternal(enabled);
}

void
KnobGuiColor::onColorLabelPickingEnabled(bool enabled)
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    KnobGuiPtr knobUI = getKnobGui();
    if ( knob->getHolder()->getApp() ) {
        if (enabled) {
            knobUI->getGui()->registerNewColorPicker( _knob.lock(), getView() );
        } else {
            knobUI->getGui()->removeColorPicker( _knob.lock(), getView());
        }
    }
}

void
KnobGuiColor::setPickingEnabledInternal(bool enabled)
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if ( knob->getHolder()->getApp() ) {
        if (enabled) {
            getKnobGui()->getGui()->registerNewColorPicker( knob, getView() );
        } else {
            getKnobGui()->getGui()->removeColorPicker( knob, getView() );
        }
    }
}

void
KnobGuiColor::setWidgetsVisible(bool visible)
{
    if (!_useSimplifiedUI) {
        KnobGuiValue::setWidgetsVisible(visible);
    }
    _colorLabel->setVisible(visible);
    if (_colorDialogButton) {
        _colorDialogButton->setVisible(visible);
    }

}


void
KnobGuiColor::updateExtraGui(const std::vector<double>& values)
{
    assert(values.size() > 0);
    double r = values[0];
    double g = r;
    double b = r;
    double a = r;
    if (values.size() > 1) {
        g = values[1];
    }
    if (values.size() > 2) {
        b = values[2];
    }
    if (values.size() > 3) {
        a = values[3];
    }
    updateLabel(r, g, b, a);
}

void
KnobGuiColor::onDimensionsMadeVisible(bool visible)
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    int nDims = knob->getNDimensions();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    QColor colors[4];
    colors[0].setRgbF(0.851643, 0.196936, 0.196936);
    colors[1].setRgbF(0, 0.654707, 0);
    colors[2].setRgbF(0.345293, 0.345293, 1);
    colors[3].setRgbF(0.398979, 0.398979, 0.398979);


    for (int i = 0; i < nDims; ++i) {
        SpinBox* sb = 0;
        getSpinBox(DimIdx(i), &sb);
        assert(sb);
        if (!visible) {
            sb->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredUnderlinedText, false);
        } else {
            sb->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredUnderlinedText, true, colors[i]);
        }
    }


}

    void
    KnobGuiColor::setEnabledExtraGui(bool enabled)
{
    if (_colorDialogButton) {
        _colorDialogButton->setEnabled(enabled);
    }
    _colorLabel->setEnabledMode(enabled);
}

void
KnobGuiColor::onDialogCurrentColorChanged(const QColor & color)
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    int nDims = knob->getNDimensions();
    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    std::vector<double> values(nDims);
    values[0] = color.redF();
    convertFromUIToInternalColorspace(&values[0]);
    if (nDims > 1) {
        values[1] =  color.greenF();
        convertFromUIToInternalColorspace(&values[1]);
    }
    if (nDims > 2) {
        values[2] = color.blueF();
        convertFromUIToInternalColorspace(&values[2]);
    }
    if (nDims > 3) {
        values[3] = color.alphaF();
    }

    KnobGuiPtr knobUI = getKnobGui();
    knob->setValueAcrossDimensions(values, DimIdx(0), getView(), eValueChangedReasonUserEdited);
    if ( knobUI->getGui() ) {
        knobUI->getGui()->setDraftRenderEnabled(true);
    }
}

void
KnobGuiColor::convertFromUIToInternalColorspace(double *value)
{
    if (_uiColorspaceLut) {
        *value = _uiColorspaceLut->fromColorSpaceFloatToLinearFloat(*value);
    }
    if (_internalColorspaceLut) {
        *value = _internalColorspaceLut->toColorSpaceFloatFromLinearFloat(*value);
    }
}

void
KnobGuiColor::convertFromInternalToUIColorspace(double *value)
{
    if (_internalColorspaceLut) {
        *value = _internalColorspaceLut->fromColorSpaceFloatToLinearFloat(*value);
    }
    if (_uiColorspaceLut) {
        *value = _uiColorspaceLut->toColorSpaceFloatFromLinearFloat(*value);
    }
}

void
KnobGuiColor::convertFromUIToInternalColorspace(double *r, double *g, double* b)
{
    convertFromUIToInternalColorspace(r);
    convertFromUIToInternalColorspace(g);
    convertFromUIToInternalColorspace(b);
}

void
KnobGuiColor::convertFromInternalToUIColorspace(double *r, double *g, double* b)
{
    convertFromInternalToUIColorspace(r);
    convertFromInternalToUIColorspace(g);
    convertFromInternalToUIColorspace(b);
}

void
KnobGuiColor::showColorDialog()
{
    QColorDialog dialog( _colorLabel->parentWidget() );

    dialog.setOption(QColorDialog::DontUseNativeDialog);
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    const int nDims = knob->getNDimensions();
    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    ViewIdx view = getView();
    double curR = knob->getValue(DimIdx(0), view, false /*clampToMinmax*/);

    _lastColor.resize(nDims);
    _lastColor[0] = curR;
    double curG = curR;
    double curB = curR;
    double curA = 1.;
    if (nDims > 1) {
        curG = knob->getValue(DimIdx(1), view, false /*clampToMinmax*/);
        _lastColor[1] =  curG;
        curB = knob->getValue(DimIdx(2), view, false /*clampToMinmax*/);
        _lastColor[2] = curB;
    }
    if (nDims > 3) {
        dialog.setOption(QColorDialog::ShowAlphaChannel);
        curA = knob->getValue(DimIdx(3), view, false /*clampToMinmax*/);
        _lastColor[3] = curA;
    }

    convertFromInternalToUIColorspace(&curR, &curG, &curB);


    QColor curColor;
    curColor.setRgbF( Image::clamp<qreal>(curR, 0., 1.),
                      Image::clamp<qreal>(curG, 0., 1.),
                      Image::clamp<qreal>(curB, 0., 1.),
                      Image::clamp<qreal>(curA, 0., 1.) );
    dialog.setCurrentColor(curColor);
    QObject::connect( &dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onDialogCurrentColorChanged(QColor)) );
    if ( !dialog.exec() ) {
        knob->setValueAcrossDimensions(_lastColor, DimIdx(0), view, eValueChangedReasonUserEdited);
    } else {
        QColor userColor = dialog.currentColor();
        std::vector<double> color(nDims);
        color[0] = userColor.redF();
        convertFromUIToInternalColorspace(&color[0]);
        if (nDims > 1) {
            color[1] =  userColor.greenF();
            convertFromUIToInternalColorspace(&color[1]);
        }
        if (nDims > 2) {
            color[2] = userColor.blueF();
            convertFromUIToInternalColorspace(&color[2]);
        }
        if (nDims > 3) {
            color[3] = userColor.alphaF();
        }



        for (int i = 0; i < 3; ++i) {
            SpinBox* sb = 0;
            getSpinBox(DimIdx(i), &sb);
            if (sb) {
                sb->setValue(color[i]);
            }
        }

        std::vector<double> oldColor(nDims);
        for (int i = 0; i < nDims; ++i) {
            oldColor[i] = _lastColor[i];
        }
        KnobGuiPtr knobUI = getKnobGui();
        knobUI->pushUndoCommand( new KnobUndoCommand<double>(knob, oldColor, color, getView()) );

    }
    KnobGuiPtr knobUI = getKnobGui();
    if ( knobUI->getGui() ) {
        knobUI->getGui()->setDraftRenderEnabled(false);
    }
} // showColorDialog


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_KnobGuiColor.cpp"
