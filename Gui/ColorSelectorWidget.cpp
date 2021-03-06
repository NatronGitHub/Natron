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

#include "ColorSelectorWidget.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMargins>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/Label.h"
#include "Engine/Lut.h"

NATRON_NAMESPACE_ENTER

ColorSelectorWidget::ColorSelectorWidget(QWidget *parent)
  : QWidget(parent)
  , _spinR(0)
  , _spinG(0)
  , _spinB(0)
  , _spinH(0)
  , _spinS(0)
  , _spinV(0)
  , _spinA(0)
  , _slideR(0)
  , _slideG(0)
  , _slideB(0)
  , _slideH(0)
  , _slideS(0)
  , _slideV(0)
  , _slideA(0)
  , _triangle(0)
  , _hex(0)
  , _button(0)
  , _stack(0)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // colortriangle
    _triangle = new QtColorTriangle(this);
    // position the triangle properly before usage
    _triangle->setColor( QColor::fromHsvF(0.0, 0.0, 0.0, 1.0) );
    _triangle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // sliders
    _slideR = new ScaleSliderQWidget(0.,
                                     1.,
                                     0.,
                                     false,
                                     ScaleSliderQWidget::eDataTypeDouble,
                                     NULL,
                                     eScaleTypeLinear,
                                     this);
    _slideG = new ScaleSliderQWidget(0.,
                                     1.,
                                     0.,
                                     false,
                                     ScaleSliderQWidget::eDataTypeDouble,
                                     NULL,
                                     eScaleTypeLinear,
                                     this);
    _slideB = new ScaleSliderQWidget(0.,
                                     1.,
                                     0.,
                                     false,
                                     ScaleSliderQWidget::eDataTypeDouble,
                                     NULL,
                                     eScaleTypeLinear,
                                     this);
    _slideH = new ScaleSliderQWidget(0.,
                                     1.,
                                     0.,
                                     false,
                                     ScaleSliderQWidget::eDataTypeDouble,
                                     NULL,
                                     eScaleTypeLinear,
                                     this);
    _slideS = new ScaleSliderQWidget(0.,
                                     1.,
                                     0.,
                                     false,
                                     ScaleSliderQWidget::eDataTypeDouble,
                                     NULL,
                                     eScaleTypeLinear,
                                     this);
    _slideV = new ScaleSliderQWidget(0.,
                                     1.,
                                     0.,
                                     false,
                                     ScaleSliderQWidget::eDataTypeDouble,
                                     NULL,
                                     eScaleTypeLinear,
                                     this);
    _slideA = new ScaleSliderQWidget(0.,
                                     1.,
                                     0.,
                                     false,
                                     ScaleSliderQWidget::eDataTypeDouble,
                                     NULL,
                                     eScaleTypeLinear,
                                     this);

    _slideR->setMinimumAndMaximum(0., 1.);
    _slideG->setMinimumAndMaximum(0., 1.);
    _slideB->setMinimumAndMaximum(0., 1.);
    _slideH->setMinimumAndMaximum(0., 1.);
    _slideS->setMinimumAndMaximum(0., 1.);
    _slideV->setMinimumAndMaximum(0., 1.);
    _slideA->setMinimumAndMaximum(0., 1.);

    _slideR->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _slideG->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _slideB->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _slideH->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _slideS->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _slideV->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _slideA->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // set line color to match channel (R/G/B/A)
    _slideR->setUseLineColor(true, QColor(200, 70, 70) );
    _slideG->setUseLineColor(true, QColor(86, 166, 66) );
    _slideB->setUseLineColor(true, QColor(83, 121, 180) );
    _slideA->setUseLineColor(true, QColor(215, 215, 215) );

    // override "knob" color on sliders
    _slideR->setUseSliderColor(true, Qt::white);
    _slideG->setUseSliderColor(true, Qt::white);
    _slideB->setUseSliderColor(true, Qt::white);
    _slideH->setUseSliderColor(true, Qt::white);
    _slideS->setUseSliderColor(true, Qt::white);
    _slideV->setUseSliderColor(true, Qt::white);
    _slideA->setUseSliderColor(true, Qt::white);

    // spinboxes
    _spinR = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinG = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinB = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinH = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinS = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinV = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinA = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);

    _spinR->decimals(3);
    _spinG->decimals(3);
    _spinB->decimals(3);
    _spinH->decimals(3);
    _spinS->decimals(3);
    _spinV->decimals(3);
    _spinA->decimals(3);

    _spinR->setIncrement(0.01);
    _spinG->setIncrement(0.01);
    _spinB->setIncrement(0.01);
    _spinH->setIncrement(0.01);
    _spinS->setIncrement(0.01);
    _spinV->setIncrement(0.01);
    _spinA->setIncrement(0.01);

    _spinR->setMaximum(1.);
    _spinR->setMinimum(0.);
    _spinG->setMaximum(1.);
    _spinG->setMinimum(0.);
    _spinB->setMaximum(1.);
    _spinB->setMinimum(0.);
    _spinH->setMaximum(1.);
    _spinH->setMinimum(0.);
    _spinS->setMaximum(1.);
    _spinS->setMinimum(0.);
    _spinV->setMaximum(1.);
    _spinV->setMinimum(0.);
    _spinA->setMaximum(1.);
    _spinA->setMinimum(0.);

    // set color to match channel (R/G/B/A)
    _spinR->setUseLineColor(true, QColor(200, 70, 70) );
    _spinG->setUseLineColor(true, QColor(86, 166, 66) );
    _spinB->setUseLineColor(true, QColor(83, 121, 180) );
    _spinA->setUseLineColor(true, QColor(215, 215, 215) );

    // hex
    _hex = new LineEdit(this);

    // button
    _button = new Button(QString::fromUtf8("RGB"), this);
    _button->setCheckable(true);

    // set triangle size
    setTriangleSize();

    // set object names (for stylesheet)
    _spinR->setObjectName( QString::fromUtf8("ColorSelectorRed") );
    _spinG->setObjectName( QString::fromUtf8("ColorSelectorGreen") );
    _spinB->setObjectName( QString::fromUtf8("ColorSelectorBlue") );
    _spinA->setObjectName( QString::fromUtf8("ColorSelectorAlpha") );
    _spinH->setObjectName( QString::fromUtf8("ColorSelectorHue") );
    _spinS->setObjectName( QString::fromUtf8("ColorSelectorSat") );
    _spinV->setObjectName( QString::fromUtf8("ColorSelectorVal") );
    _hex->setObjectName( QString::fromUtf8("ColorSelectorHex") );
    _button->setObjectName( QString::fromUtf8("ColorSelectorSwitch") );

    // labels
    Label *labelR = new Label(QString::fromUtf8("R"), this);
    Label *labelG = new Label(QString::fromUtf8("G"), this);
    Label *labelB = new Label(QString::fromUtf8("B"), this);
    Label *labelA = new Label(QString::fromUtf8("A"), this);
    Label *labelH = new Label(QString::fromUtf8("H"), this);
    Label *labelS = new Label(QString::fromUtf8("S"), this);
    Label *labelV = new Label(QString::fromUtf8("V"), this);
    Label *labelHex = new Label(QString::fromUtf8("Hex"), this);

    labelR->setMinimumWidth(10);
    labelG->setMinimumWidth(10);
    labelB->setMinimumWidth(10);
    labelA->setMinimumWidth(10);
    labelH->setMinimumWidth(10);
    labelS->setMinimumWidth(10);
    labelV->setMinimumWidth(10);

    // tooltips
    _spinR->setToolTip( QObject::tr("Red color value") );
    _spinG->setToolTip( QObject::tr("Green color value") );
    _spinB->setToolTip( QObject::tr("Blue color value") );
    _spinA->setToolTip( QObject::tr("Alpha value") );
    _spinH->setToolTip( QObject::tr("Hue color value") );
    _spinS->setToolTip( QObject::tr("Saturation value") );
    _spinV->setToolTip( QObject::tr("Brightness/Intensity value") );
    _hex->setToolTip( QObject::tr("A HTML hexadecimal color is specified with: #RRGGBB, where the RR (red), GG (green) and BB (blue) hexadecimal integers specify the components of the color.") );
    _button->setToolTip( QObject::tr("Switch to HSV") );

    // layout
    _stack = new QStackedWidget(this);

    QWidget *rWidget = new QWidget(this);
    QHBoxLayout *rLayout = new QHBoxLayout(rWidget);

    QWidget *gWidget = new QWidget(this);
    QHBoxLayout *gLayout = new QHBoxLayout(gWidget);

    QWidget *bWidget = new QWidget(this);
    QHBoxLayout *bLayout = new QHBoxLayout(bWidget);

    QWidget *hWidget = new QWidget(this);
    QHBoxLayout *hLayout = new QHBoxLayout(hWidget);

    QWidget *sWidget = new QWidget(this);
    QHBoxLayout *sLayout = new QHBoxLayout(sWidget);

    QWidget *vWidget = new QWidget(this);
    QHBoxLayout *vLayout = new QHBoxLayout(vWidget);

    QWidget *aWidget = new QWidget(this);
    QHBoxLayout *aLayout = new QHBoxLayout(aWidget);

    QWidget *hexWidget = new QWidget(this);
    QHBoxLayout *hexLayout = new QHBoxLayout(hexWidget);

    QWidget *rgbaWidget = new QWidget(this);
    QVBoxLayout *rgbaLayout = new QVBoxLayout(rgbaWidget);

    QWidget *hsvWidget = new QWidget(this);
    QVBoxLayout *hsvLayout = new QVBoxLayout(hsvWidget);

    QWidget *topWidget = new QWidget(this);
    QHBoxLayout *topLayout = new QHBoxLayout(topWidget);

    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);

    QWidget *rightWidget = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);

    QWidget *bottomWidget = new QWidget(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomWidget);

    _stack->setContentsMargins(0, 0, 0, 0);
    hexWidget->setContentsMargins(0, 0, 0, 0);
    topWidget->setContentsMargins(0, 0, 0, 0);
    leftWidget->setContentsMargins(0, 0, 0, 0);
    rightWidget->setContentsMargins(0, 0, 0, 0);
    bottomWidget->setContentsMargins(0, 0, 10, 0);

    _stack->layout()->setContentsMargins(0, 0, 0, 0);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    rLayout->setContentsMargins(0, 0, 0, 0);
    gLayout->setContentsMargins(0, 0, 0, 0);
    bLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setContentsMargins(0, 0, 0, 0);
    sLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setContentsMargins(0, 0, 0, 0);
    hexLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    QMargins aMargin = hsvLayout->contentsMargins();
    aMargin.setTop(0);
    aLayout->setContentsMargins(aMargin);

    _stack->layout()->setSpacing(0);
    mainLayout->setSpacing(0);
    topLayout->setSpacing(0);
    leftLayout->setSpacing(0);
    rightLayout->setSpacing(0);

    rLayout->addWidget(labelR);
    rLayout->addWidget(_spinR);
    rLayout->addWidget(_slideR);
    gLayout->addWidget(labelG);
    gLayout->addWidget(_spinG);
    gLayout->addWidget(_slideG);
    bLayout->addWidget(labelB);
    bLayout->addWidget(_spinB);
    bLayout->addWidget(_slideB);

    hLayout->addWidget(labelH);
    hLayout->addWidget(_spinH);
    hLayout->addWidget(_slideH);
    sLayout->addWidget(labelS);
    sLayout->addWidget(_spinS);
    sLayout->addWidget(_slideS);
    vLayout->addWidget(labelV);
    vLayout->addWidget(_spinV);
    vLayout->addWidget(_slideV);

    aLayout->addWidget(labelA);
    aLayout->addWidget(_spinA);
    aLayout->addWidget(_slideA);

    hexLayout->addWidget(labelHex);
    hexLayout->addWidget(_hex);

    rgbaLayout->addWidget(rWidget);
    rgbaLayout->addWidget(gWidget);
    rgbaLayout->addWidget(bWidget);

    hsvLayout->addWidget(hWidget);
    hsvLayout->addWidget(sWidget);
    hsvLayout->addWidget(vWidget);

    leftLayout->addWidget(_triangle);

    _stack->addWidget(rgbaWidget);
    _stack->addWidget(hsvWidget);

    rightLayout->addWidget(_stack);
    rightLayout->addWidget(aWidget);

    topLayout->addWidget(leftWidget);
    topLayout->addWidget(rightWidget);

    bottomLayout->addWidget(_button);
    bottomLayout->addStretch();
    bottomLayout->addWidget(hexWidget);

    mainLayout->addWidget(topWidget);
    mainLayout->addWidget(bottomWidget);

    // connect the widgets
    QObject::connect( _triangle, SIGNAL( colorChanged(QColor) ),
                      this, SLOT( handleTriangleColorChanged(QColor) ) );

    QObject::connect( _spinR, SIGNAL( valueChanged(double) ),
                      this, SLOT( handleSpinRChanged(double) ) );
    QObject::connect( _spinG, SIGNAL( valueChanged(double) ),
                      this, SLOT( handleSpinGChanged(double) ) );
    QObject::connect( _spinB, SIGNAL( valueChanged(double) ),
                      this, SLOT( handleSpinBChanged(double) ) );
    QObject::connect( _spinH, SIGNAL( valueChanged(double) ),
                      this, SLOT( handleSpinHChanged(double) ) );
    QObject::connect( _spinS, SIGNAL( valueChanged(double) ),
                      this, SLOT( handleSpinSChanged(double) ) );
    QObject::connect( _spinV, SIGNAL( valueChanged(double) ),
                      this, SLOT( handleSpinVChanged(double) ) );
    QObject::connect( _spinA, SIGNAL( valueChanged(double) ),
                      this, SLOT( handleSpinAChanged(double) ) );

    QObject::connect( _hex, SIGNAL( returnPressed() ),
                      this, SLOT( handleHexChanged() ) );

    QObject::connect( _slideR, SIGNAL( positionChanged(double) ),
                      this, SLOT( handleSliderRMoved(double) ) );
    QObject::connect( _slideG, SIGNAL( positionChanged(double) ),
                      this, SLOT( handleSliderGMoved(double) ) );
    QObject::connect( _slideB, SIGNAL( positionChanged(double) ),
                      this, SLOT( handleSliderBMoved(double) ) );
    QObject::connect( _slideH, SIGNAL( positionChanged(double) ),
                      this, SLOT( handleSliderHMoved(double) ) );
    QObject::connect( _slideS, SIGNAL( positionChanged(double) ),
                      this, SLOT( handleSliderSMoved(double) ) );
    QObject::connect( _slideV, SIGNAL( positionChanged(double) ),
                      this, SLOT( handleSliderVMoved(double) ) );
    QObject::connect( _slideA, SIGNAL( positionChanged(double) ),
                      this, SLOT( handleSliderAMoved(double) ) );

    QObject::connect( _button, SIGNAL( clicked(bool) ),
                      this, SLOT( handleButtonClicked(bool) ) );
}

void
ColorSelectorWidget::getColor(float *r,
                              float *g,
                              float *b,
                              float *a)
{
    *r = _spinR->value();
    *g = _spinG->value();
    *b = _spinB->value();
    *a = _spinA->value();
}

void
ColorSelectorWidget::setColor(float r,
                              float g,
                              float b,
                              float a)
{
    float h, s, v;
    Color::rgb_to_hsv(r, g, b, &h, &s, &v);

    setRedChannel(r);
    setGreenChannel(g);
    setBlueChannel(b);
    setHueChannel(h);
    setSaturationChannel(s);
    setValueChannel(v);
    setAlphaChannel(a);
    setTriangle(r, g, b, a);
    setHex( _triangle->color() );
}

void
ColorSelectorWidget::setRedChannel(float value)
{
    _spinR->blockSignals(true);
    _slideR->blockSignals(true);

    _spinR->setValue(value);
    _slideR->seekScalePosition(value);

    _spinR->blockSignals(false);
    _slideR->blockSignals(false);
}

void
ColorSelectorWidget::setGreenChannel(float value)
{
    _spinG->blockSignals(true);
    _slideG->blockSignals(true);

    _spinG->setValue(value);
    _slideG->seekScalePosition(value);

    _spinG->blockSignals(false);
    _slideG->blockSignals(false);
}

void
ColorSelectorWidget::setBlueChannel(float value)
{
    _spinB->blockSignals(true);
    _slideB->blockSignals(true);

    _spinB->setValue(value);
    _slideB->seekScalePosition(value);

    _spinB->blockSignals(false);
    _slideB->blockSignals(false);
}

void
ColorSelectorWidget::setHueChannel(float value)
{
    _spinH->blockSignals(true);
    _slideH->blockSignals(true);

    _spinH->setValue(value);
    _slideH->seekScalePosition(value);

    setSliderHColor();

    _spinH->blockSignals(false);
    _slideH->blockSignals(false);
}

void
ColorSelectorWidget::setSaturationChannel(float value)
{
    _spinS->blockSignals(true);
    _slideS->blockSignals(true);

    _spinS->setValue(value);
    _slideS->seekScalePosition(value);

    setSliderSColor();

    _spinS->blockSignals(false);
    _slideS->blockSignals(false);
}

void
ColorSelectorWidget::setValueChannel(float value)
{
    _spinV->blockSignals(true);
    _slideV->blockSignals(true);

    _spinV->setValue(value);
    _slideV->seekScalePosition(value);

    setSliderVColor();

    _spinV->blockSignals(false);
    _slideV->blockSignals(false);
}

void
ColorSelectorWidget::setAlphaChannel(float value)
{
    _spinA->blockSignals(true);
    _slideA->blockSignals(true);

    _spinA->setValue(value);
    _slideA->seekScalePosition(value);

    _spinA->blockSignals(false);
    _slideA->blockSignals(false);
}

void
ColorSelectorWidget::setTriangle(float r,
                                 float g,
                                 float b,
                                 float a)
{
    QColor color = _triangle->color();
    color.setRgbF( Color::to_func_srgb(r),
                   Color::to_func_srgb(g),
                   Color::to_func_srgb(b) );
    color.setAlphaF(a);

    _triangle->blockSignals(true);
    _triangle->setColor(color);
    _triangle->blockSignals(false);
}

void
ColorSelectorWidget::setHex(const QColor &color)
{
    if ( !color.isValid() ) {
        return;
    }

    _hex->blockSignals(true);
    _hex->setText( color.name() );
    _hex->blockSignals(false);
}

void
ColorSelectorWidget::announceColorChange()
{
    Q_EMIT colorChanged( _spinR->value(),
                         _spinG->value(),
                         _spinB->value(),
                         _spinA->value() );
}

void
ColorSelectorWidget::setTriangleSize()
{
    int triangleSize = 4;
    int padding = 10;

    triangleSize = (_spinR->size().height() * triangleSize) + (triangleSize * padding);

    _triangle->setMinimumSize(triangleSize, triangleSize);
    _triangle->setMaximumSize(triangleSize, triangleSize);
}

void
ColorSelectorWidget::handleTriangleColorChanged(const QColor &color,
                                                bool announce)
{
    setRedChannel( Color::from_func_srgb( color.redF() ) );
    setGreenChannel( Color::from_func_srgb( color.greenF() ) );
    setBlueChannel( Color::from_func_srgb( color.blueF() ) );
    setHueChannel( color.toHsv().hueF() );
    setSaturationChannel( color.toHsv().saturationF() );
    setValueChannel( color.toHsv().valueF() );
    setHex(color);

    if (announce) {
        announceColorChange();
    }
}

void ColorSelectorWidget::manageColorRGBChanged(bool announce)
{
    float r = _spinR->value();
    float g = _spinG->value();
    float b = _spinB->value();
    float a = _spinA->value();
    float h, s, v;
    Color::rgb_to_hsv(r, g, b, &h, &s, &v);

    setHueChannel(h);
    setSaturationChannel(s);
    setValueChannel(v);
    setTriangle(r, g, b, a);
    setHex( _triangle->color() );

    if (announce) {
        announceColorChange();
    }
}

void
ColorSelectorWidget::manageColorHSVChanged(bool announce)
{
    float h = _spinH->value();
    float s = _spinS->value();
    float v = _spinV->value();
    float a = _spinA->value();
    float r, g, b;
    Color::hsv_to_rgb(h, s, v, &r, &g, &b);

    setRedChannel(r);
    setGreenChannel(g);
    setBlueChannel(b);

    QColor color = _triangle->color();
    color.setHsvF(h, s, v);
    color.setAlphaF(a);

    _triangle->blockSignals(true);
    _triangle->setColor(color);
    _triangle->blockSignals(false);

    setHex(color);

    setSliderHColor();
    setSliderSColor();
    setSliderVColor();

    if (announce) {
        announceColorChange();
    }
}

void
ColorSelectorWidget::manageColorAlphaChanged(bool announce)
{
    if (announce) {
        announceColorChange();
    }
}

void
ColorSelectorWidget::handleSpinRChanged(double value)
{
    _slideR->blockSignals(true);
    _slideR->seekScalePosition(value);
    _slideR->blockSignals(false);

    manageColorRGBChanged();
}

void
ColorSelectorWidget::handleSpinGChanged(double value)
{
    _slideG->blockSignals(true);
    _slideG->seekScalePosition(value);
    _slideG->blockSignals(false);

    manageColorRGBChanged();
}

void
ColorSelectorWidget::handleSpinBChanged(double value)
{
    _slideB->blockSignals(true);
    _slideB->seekScalePosition(value);
    _slideB->blockSignals(false);

    manageColorRGBChanged();
}

void
ColorSelectorWidget::handleSpinHChanged(double value)
{
    _slideH->blockSignals(true);
    _slideH->seekScalePosition(value);
    _slideH->blockSignals(false);

    manageColorHSVChanged();
}

void
ColorSelectorWidget::handleSpinSChanged(double value)
{
    _slideS->blockSignals(true);
    _slideS->seekScalePosition(value);
    _slideS->blockSignals(false);

    manageColorHSVChanged();
}

void
ColorSelectorWidget::handleSpinVChanged(double value)
{
    _slideV->blockSignals(true);
    _slideV->seekScalePosition(value);
    _slideV->blockSignals(false);

    manageColorHSVChanged();
}

void
ColorSelectorWidget::handleSpinAChanged(double value)
{
    _slideA->blockSignals(true);
    _slideA->seekScalePosition(value);
    _slideA->blockSignals(false);

    manageColorAlphaChanged();
}

void ColorSelectorWidget::handleHexChanged()
{
    QString value = _hex->text();
    if ( !value.startsWith( QString::fromUtf8("#") ) ) {
        value.prepend( QString::fromUtf8("#") );
    }

    QColor color;
    color.setNamedColor( _hex->text() );
    if ( !color.isValid() ) {
        return;
    }

    _triangle->setColor(color);
}

void
ColorSelectorWidget::handleSliderRMoved(double value)
{
    _spinR->blockSignals(true);
    _spinR->setValue(value);
    _spinR->blockSignals(false);

    manageColorRGBChanged();
}

void
ColorSelectorWidget::handleSliderGMoved(double value)
{
    _spinG->blockSignals(true);
    _spinG->setValue(value);
    _spinG->blockSignals(false);

    manageColorRGBChanged();
}

void
ColorSelectorWidget::handleSliderBMoved(double value)
{
    _spinB->blockSignals(true);
    _spinB->setValue(value);
    _spinB->blockSignals(false);

    manageColorRGBChanged();
}

void
ColorSelectorWidget::handleSliderHMoved(double value)
{
    _spinH->blockSignals(true);
    _spinH->setValue(value);
    _spinH->blockSignals(false);

    manageColorHSVChanged();
}

void
ColorSelectorWidget::handleSliderSMoved(double value)
{
    _spinS->blockSignals(true);
    _spinS->setValue(value);
    _spinS->blockSignals(false);

    manageColorHSVChanged();
}

void
ColorSelectorWidget::handleSliderVMoved(double value)
{
    _spinV->blockSignals(true);
    _spinV->setValue(value);
    _spinV->blockSignals(false);

    manageColorHSVChanged();
}


void
ColorSelectorWidget::handleSliderAMoved(double value)
{
    _spinA->blockSignals(true);
    _spinA->setValue(value);
    _spinA->blockSignals(false);

    manageColorAlphaChanged();
}

void
ColorSelectorWidget::setSliderHColor()
{
    QColor color;
    color.setHsvF( _spinH->value(),
                   1.0,
                   1.0,
                   1.0);
    _spinH->setUseLineColor(true, color);
    _slideH->setUseLineColor(true, color);
}

void
ColorSelectorWidget::setSliderSColor()
{
    QColor color;
    color.setHsvF( _spinH->value(),
                   _spinS->value(),
                   1.0,
                   1.0);
    _spinS->setUseLineColor(true, color);
    _slideS->setUseLineColor(true, color);
}

void
ColorSelectorWidget::setSliderVColor()
{
    QColor color;
    color.setHsvF( _spinH->value(),
                   _spinS->value(),
                   _spinV->value(),
                   1.0);
    _spinV->setUseLineColor(true, color);
    _slideV->setUseLineColor(true, color);
}

void
ColorSelectorWidget::handleButtonClicked(bool checked)
{
    if (checked) {
        _button->setText( QString::fromUtf8("HSV") );
        _button->setToolTip( QObject::tr("Switch to RGB") );
        _stack->setCurrentIndex(1);
    } else {
        _button->setText( QString::fromUtf8("RGB") );
        _button->setToolTip( QObject::tr("Switch to HSV") );
        _stack->setCurrentIndex(0);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ColorSelectorWidget.cpp"
