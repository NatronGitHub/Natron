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

#ifndef Gui_ColorSelectorWidget_h
#define Gui_ColorSelectorWidget_h

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
#include <QColor>
#include "Gui/QtColorTriangle.h" // from Qt Solutions
#include <QMouseEvent>
#include <QEvent>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/LineEdit.h"

#define COLOR_WHEEL_DEFAULT_SIZE 160

NATRON_NAMESPACE_ENTER

class ColorSelectorWidget : public QWidget
{
    Q_OBJECT

public:

    explicit ColorSelectorWidget(QWidget *parent = NULL,
                                 int colorWheelSize = COLOR_WHEEL_DEFAULT_SIZE);
    void getColor(float *r, float *g, float *b, float *a);

Q_SIGNALS:

    void colorChanged(float r, float g, float b, float a);
    void updateColor();

public Q_SLOTS:

    void setColor(float r, float g, float b, float a);

private:

    SpinBox *_spinR;
    SpinBox *_spinG;
    SpinBox *_spinB;
    SpinBox *_spinH;
    SpinBox *_spinS;
    SpinBox *_spinV;
    SpinBox *_spinA;

    ScaleSliderQWidget *_slideR;
    ScaleSliderQWidget *_slideG;
    ScaleSliderQWidget *_slideB;
    ScaleSliderQWidget *_slideH;
    ScaleSliderQWidget *_slideS;
    ScaleSliderQWidget *_slideV;
    ScaleSliderQWidget *_slideA;

    QtColorTriangle *_triangle;

    LineEdit *_hex;

    void setRedChannel(float value);
    void setGreenChannel(float value);
    void setBlueChannel(float value);
    void setHueChannel(float value);
    void setSaturationChannel(float value);
    void setValueChannel(float value);
    void setAlphaChannel(float value);
    void setTriangle(float r, float g, float b, float a);
    void setHex(const QColor &color);

    void announceColorChange();

private Q_SLOTS:

    void handleTriangleColorChanged(const QColor &color, bool announce = true);

    void manageColorRGBChanged(bool announce = true);
    void manageColorHSVChanged(bool announce = true);
    void manageColorAlphaChanged(bool announce = true);

    void handleSpinRChanged(double value);
    void handleSpinGChanged(double value);
    void handleSpinBChanged(double value);
    void handleSpinHChanged(double value);
    void handleSpinSChanged(double value);
    void handleSpinVChanged(double value);
    void handleSpinAChanged(double value);

    void handleHexChanged();

    void handleSliderRMoved(double value);
    void handleSliderGMoved(double value);
    void handleSliderBMoved(double value);
    void handleSliderHMoved(double value);
    void handleSliderSMoved(double value);
    void handleSliderVMoved(double value);
    void handleSliderAMoved(double value);

    void setSliderHColor();
    void setSliderSColor();
    void setSliderVColor();

    // workaround for QToolButton+QWidgetAction
    // triggered signal(s) are never emitted!?
    bool event(QEvent*e) override
    {
        if (e->type() == QEvent::Show) {
            Q_EMIT updateColor();
        }
        return QWidget::event(e);
    }

    // https://bugreports.qt.io/browse/QTBUG-47406
    void mousePressEvent(QMouseEvent *e) override
    {
        e->accept();
    }
    void mouseReleaseEvent(QMouseEvent *e) override
    {
        e->accept();
    }
};

NATRON_NAMESPACE_EXIT

#endif // Gui_ColorSelectorWidget_h
