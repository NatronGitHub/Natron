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

#include "ColorSelectorWidget.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMargins>
#include <QSettings>
#include <QShortcut>
#include <QKeySequence>
#include <QPixmap>
#include <QIcon>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Lut.h"
#include "Gui/Label.h"

#define COLOR_SELECTOR_BUTTON_PROPERTY "ColorButtonType"
#define COLOR_SELECTOR_BUTTON_RGB 0
#define COLOR_SELECTOR_BUTTON_HSV 1

NATRON_NAMESPACE_ENTER

ColorSelectorPaletteButton::ColorSelectorPaletteButton(QWidget *parent)
    : Button(parent)
    , _r(COLOR_SELECTOR_PALETTE_DEFAULT_COLOR)
    , _g(COLOR_SELECTOR_PALETTE_DEFAULT_COLOR)
    , _b(COLOR_SELECTOR_PALETTE_DEFAULT_COLOR)
    , _a(1.0)
    , _modified(false)
{
    initButton();
}

ColorSelectorPaletteButton::ColorSelectorPaletteButton(float r,
                                                       float g,
                                                       float b,
                                                       float a,
                                                       QWidget *parent)
    : Button(parent)
    , _r(r)
    , _g(g)
    , _b(b)
    , _a(a)
    , _modified(true)
{
    initButton();
}

bool
ColorSelectorPaletteButton::isModified()
{
    return _modified;
}

void
ColorSelectorPaletteButton::clearModified()
{
    _modified = false;
}

void
ColorSelectorPaletteButton::getColor(float *r,
                                     float *g,
                                     float *b,
                                     float *a)
{
    *r = _r;
    *g = _g;
    *b = _b;
    *a = _a;
}

void
ColorSelectorPaletteButton::clearColor()
{
    _r = COLOR_SELECTOR_PALETTE_DEFAULT_COLOR;
    _g = COLOR_SELECTOR_PALETTE_DEFAULT_COLOR;
    _b = COLOR_SELECTOR_PALETTE_DEFAULT_COLOR;
    _a = 1.0;
    _modified = false;
    updateColor(false);
}

void
ColorSelectorPaletteButton::setColor(float r,
                                     float g,
                                     float b,
                                     float a)
{
    _r = r;
    _g = g;
    _b = b;
    _a = a;
    _modified = true;
    updateColor();
}

void
ColorSelectorPaletteButton::updateColor(bool signal)
{
    QColor color;
    color.setRgbF(Color::to_func_srgb(_r),
                  Color::to_func_srgb(_g),
                  Color::to_func_srgb(_b),
                  _a);
    QPixmap pixColor(COLOR_SELECTOR_PALETTE_ICON_SIZE,
                     COLOR_SELECTOR_PALETTE_ICON_SIZE);
    pixColor.fill(color);
    setIcon( QIcon(pixColor) );
    setToolTip( QString::fromUtf8("R: %1\nG: %2\nB: %3\nA: %4\n\n%5")
                .arg(_r)
                .arg(_g)
                .arg(_b)
                .arg(_a)
                .arg( tr("Clear color with right-click.") ) );

    if (signal) {
        Q_EMIT colorChanged(_r, _g, _b, _a);
    }
}

void
ColorSelectorPaletteButton::initButton()
{
    setIconSize( QSize(COLOR_SELECTOR_PALETTE_ICON_SIZE,
                       COLOR_SELECTOR_PALETTE_ICON_SIZE) );
    setStyleSheet( QString::fromUtf8("QPushButton { border: 0; background-color: transparent; }") );

    QObject::connect( this, SIGNAL( clicked() ),
                      this, SLOT( buttonClicked() ) );

    updateColor(false);
}

void
ColorSelectorPaletteButton::buttonClicked()
{
    Q_EMIT colorPicked(_r, _g, _b, _a);
}

void
ColorSelectorPaletteButton::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        clearColor();
    }
    Button::mouseReleaseEvent(e);
}

ColorSelectorWidget::ColorSelectorWidget(bool withAlpha, QWidget *parent)
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
  , _buttonColorGroup(0)
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
    if (withAlpha) {
        _slideA = new ScaleSliderQWidget(0.,
                                         1.,
                                         0.,
                                         false,
                                         ScaleSliderQWidget::eDataTypeDouble,
                                         NULL,
                                         eScaleTypeLinear,
                                         this);
    }
    _slideR->setMinimumAndMaximum(0., 1.);
    _slideG->setMinimumAndMaximum(0., 1.);
    _slideB->setMinimumAndMaximum(0., 1.);
    _slideH->setMinimumAndMaximum(0., 1.);
    _slideS->setMinimumAndMaximum(0., 1.);
    _slideV->setMinimumAndMaximum(0., 1.);
    if (_slideA) {
        _slideA->setMinimumAndMaximum(0., 1.);
    }

    _slideR->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _slideG->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _slideB->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _slideH->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _slideS->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _slideV->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (_slideA) {
        _slideA->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    // set line color to match channel (R/G/B/A)
    _slideR->setUseLineColor(true, QColor(200, 70, 70) );
    _slideG->setUseLineColor(true, QColor(86, 166, 66) );
    _slideB->setUseLineColor(true, QColor(83, 121, 180) );
    if (_slideA) {
        _slideA->setUseLineColor(true, QColor(215, 215, 215) );
    }

    // override "knob" color on sliders
    _slideR->setUseSliderColor(true, Qt::white);
    _slideG->setUseSliderColor(true, Qt::white);
    _slideB->setUseSliderColor(true, Qt::white);
    _slideH->setUseSliderColor(true, Qt::white);
    _slideS->setUseSliderColor(true, Qt::white);
    _slideV->setUseSliderColor(true, Qt::white);
    if (_slideA) {
        _slideA->setUseSliderColor(true, Qt::white);
    }

    // spinboxes
    _spinR = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinG = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinB = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinH = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinS = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _spinV = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    if (_slideA) {
        _spinA = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    }

    _spinR->decimals(3);
    _spinG->decimals(3);
    _spinB->decimals(3);
    _spinH->decimals(3);
    _spinS->decimals(3);
    _spinV->decimals(3);
    if (_slideA) {
        _spinA->decimals(3);
    }

    _spinR->setIncrement(0.01);
    _spinG->setIncrement(0.01);
    _spinB->setIncrement(0.01);
    _spinH->setIncrement(0.01);
    _spinS->setIncrement(0.01);
    _spinV->setIncrement(0.01);
    if (_slideA) {
        _spinA->setIncrement(0.01);
    }

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
    if (_slideA) {
        _spinA->setMaximum(1.);
        _spinA->setMinimum(0.);
    }

    // set color to match channel (R/G/B/A)
    _spinR->setUseLineColor(true, QColor(200, 70, 70) );
    _spinG->setUseLineColor(true, QColor(86, 166, 66) );
    _spinB->setUseLineColor(true, QColor(83, 121, 180) );
    if (_slideA) {
        _spinA->setUseLineColor(true, QColor(215, 215, 215) );
    }

    // hex
    _hex = new LineEdit(this);
    _hex->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // color buttons
    _buttonColorGroup = new QButtonGroup(this);

    Button *buttonRGB = new Button(QString::fromUtf8("RGB"), this);
    Button *buttonHSV = new Button(QString::fromUtf8("HSV"), this);

    buttonRGB->setProperty(COLOR_SELECTOR_BUTTON_PROPERTY, COLOR_SELECTOR_BUTTON_RGB);
    buttonHSV->setProperty(COLOR_SELECTOR_BUTTON_PROPERTY, COLOR_SELECTOR_BUTTON_HSV);

    buttonRGB->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    buttonHSV->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    buttonRGB->setCheckable(true);
    buttonRGB->setChecked(true);

    buttonHSV->setCheckable(true);
    buttonHSV->setChecked(false);

    _buttonColorGroup->setExclusive(true);
    _buttonColorGroup->addButton(buttonRGB);
    _buttonColorGroup->addButton(buttonHSV);

    // palette
    Button *paletteAddColorButton = new Button(QObject::tr("Add"), this);
    Button *paletteClearButton = new Button(QObject::tr("Clear"), this);

    QShortcut *addPaletteColorShortcut = new QShortcut(QKeySequence( QObject::tr("Shift+A") ), this);
    QShortcut *clearPaletteColorsShortcut = new QShortcut(QKeySequence( QObject::tr("Shift+C") ), this);

    // set triangle size
    setTriangleSize();

    // set object names (for stylesheet)
    _spinR->setObjectName( QString::fromUtf8("ColorSelectorRed") );
    _spinG->setObjectName( QString::fromUtf8("ColorSelectorGreen") );
    _spinB->setObjectName( QString::fromUtf8("ColorSelectorBlue") );
    if (_slideA) {
        _spinA->setObjectName( QString::fromUtf8("ColorSelectorAlpha") );
    }
    _spinH->setObjectName( QString::fromUtf8("ColorSelectorHue") );
    _spinS->setObjectName( QString::fromUtf8("ColorSelectorSat") );
    _spinV->setObjectName( QString::fromUtf8("ColorSelectorVal") );
    _hex->setObjectName( QString::fromUtf8("ColorSelectorHex") );

    // labels
    Label *labelR = new Label(QString::fromUtf8("R"), this);
    Label *labelG = new Label(QString::fromUtf8("G"), this);
    Label *labelB = new Label(QString::fromUtf8("B"), this);
    Label *labelA;
    if (_slideA) {
        labelA = new Label(QString::fromUtf8("A"), this);
    }
    Label *labelH = new Label(QString::fromUtf8("H"), this);
    Label *labelS = new Label(QString::fromUtf8("S"), this);
    Label *labelV = new Label(QString::fromUtf8("V"), this);
    Label *labelHex = new Label(QString::fromUtf8("Hex"), this);

    labelR->setMinimumWidth(10);
    labelG->setMinimumWidth(10);
    labelB->setMinimumWidth(10);
    if (_slideA) {
        labelA->setMinimumWidth(10);
    }
    labelH->setMinimumWidth(10);
    labelS->setMinimumWidth(10);
    labelV->setMinimumWidth(10);

    labelR->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    labelG->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    labelB->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (_slideA) {
        labelA->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    labelH->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    labelS->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    labelV->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // tooltips
    _spinR->setToolTip( QObject::tr("Red color value") );
    _spinG->setToolTip( QObject::tr("Green color value") );
    _spinB->setToolTip( QObject::tr("Blue color value") );
    if (_slideA) {
        _spinA->setToolTip( QObject::tr("Alpha value") );
    }
    _spinH->setToolTip( QObject::tr("Hue color value") );
    _spinS->setToolTip( QObject::tr("Saturation value") );
    _spinV->setToolTip( QObject::tr("Brightness/Intensity value") );
    _hex->setToolTip( QObject::tr("A HTML hexadecimal color is specified with: #RRGGBB, where the RR (red), GG (green) and BB (blue) hexadecimal integers specify the components of the color.") );
    paletteAddColorButton->setToolTip( QObject::tr("Add current color to palette (Shift+A)") );
    paletteClearButton->setToolTip( QObject::tr("Clear colors in palette (Shift+C)") );

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

    QWidget *aWidget;
    QHBoxLayout *aLayout;
    if (_slideA) {
        aWidget = new QWidget(this);
        aLayout = new QHBoxLayout(aWidget);
    }

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

    QWidget *paletteWidget = new QWidget(this);
    QHBoxLayout *paletteLayout = new QHBoxLayout(paletteWidget);

    QWidget *paletteButtonsWidget = new QWidget(this);

    QWidget *paletteOptionsWidget = new QWidget(this);
    QVBoxLayout *paletteOptionsLayout = new QVBoxLayout(paletteOptionsWidget);

    _stack->setContentsMargins(0, 0, 0, 0);
    hexWidget->setContentsMargins(0, 0, 0, 0);
    topWidget->setContentsMargins(0, 0, 0, 0);
    leftWidget->setContentsMargins(0, 0, 0, 0);
    rightWidget->setContentsMargins(0, 0, 0, 0);
    bottomWidget->setContentsMargins(10, 5, 10, 0);
    paletteOptionsWidget->setContentsMargins(0, 0, 0, 0);

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
    paletteLayout->setContentsMargins(0, 5, 10, 0);
    paletteOptionsLayout->setContentsMargins(0, 0, 0, 0);

    if (_slideA) {
        QMargins aMargin = hsvLayout->contentsMargins();
        aMargin.setTop(0);
        aLayout->setContentsMargins(aMargin);
    }

    _stack->layout()->setSpacing(0);
    mainLayout->setSpacing(0);
    topLayout->setSpacing(0);
    leftLayout->setSpacing(0);
    rightLayout->setSpacing(0);
    paletteOptionsLayout->setSpacing(2);

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

    if (_slideA) {
        aLayout->addWidget(labelA);
        aLayout->addWidget(_spinA);
        aLayout->addWidget(_slideA);
    }

    hexLayout->addWidget(labelHex);
    hexLayout->addWidget(_hex);

    rgbaLayout->addWidget(rWidget);
    rgbaLayout->addWidget(gWidget);
    rgbaLayout->addWidget(bWidget);

    hsvLayout->addWidget(hWidget);
    hsvLayout->addWidget(sWidget);
    hsvLayout->addWidget(vWidget);

    paletteLayout->addWidget(paletteOptionsWidget);
    paletteLayout->addWidget(paletteButtonsWidget);

    paletteOptionsLayout->addWidget(paletteAddColorButton);
    paletteOptionsLayout->addStretch();
    paletteOptionsLayout->addWidget(paletteClearButton);

    leftLayout->addWidget(_triangle);

    _stack->addWidget(rgbaWidget);
    _stack->addWidget(hsvWidget);

    rightLayout->addWidget(_stack);
    if (_slideA) {
        rightLayout->addWidget(aWidget);
    }

    topLayout->addWidget(leftWidget);
    topLayout->addWidget(rightWidget);

    bottomLayout->addWidget(buttonRGB);
    bottomLayout->addWidget(buttonHSV);
    bottomLayout->addWidget(hexWidget);

    mainLayout->addWidget(topWidget);
    mainLayout->addWidget(paletteWidget);
    rightLayout->addWidget(bottomWidget);

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
    if (_slideA) {
        QObject::connect( _spinA, SIGNAL( valueChanged(double) ),
                         this, SLOT( handleSpinAChanged(double) ) );
    }

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
    if (_slideA) {
        QObject::connect( _slideA, SIGNAL( positionChanged(double) ),
                         this, SLOT( handleSliderAMoved(double) ) );
    }

    QObject::connect( _buttonColorGroup, SIGNAL( buttonClicked(int) ),
                      this, SLOT( handleButtonColorClicked(int) ) );

    QObject::connect( paletteAddColorButton, SIGNAL( clicked(bool) ),
                      this, SLOT( setPaletteButtonColor(bool) ) );
    QObject::connect( paletteClearButton, SIGNAL( clicked(bool) ),
                      this, SLOT( clearPaletteButtons(bool) ) );
    QObject::connect( addPaletteColorShortcut, SIGNAL( activated() ),
                      this, SLOT( setPaletteButtonColor() ) );
    QObject::connect( clearPaletteColorsShortcut, SIGNAL( activated() ),
                      this, SLOT( clearPaletteButtons() ) );

    // setup palette
    initPaletteButtons(paletteButtonsWidget);
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
    *a = _slideA ? _spinA->value() : 1.f;
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
    if (_slideA) {
        setAlphaChannel(a);
    }
    setTriangle(r, g, b, _slideA ? a : 1.);
    setHex( _triangle->color() );
}

void
ColorSelectorWidget::setColorFromPalette(float r,
                                         float g,
                                         float b,
                                         float a)
{
    setColor(r, g, b, a);
    announceColorChange();
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
    if (!_slideA) {
        return;
    }
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
    color.setAlphaF(_slideA ? a : 1.);

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
                         _slideA ? _spinA->value() : 1. );
}

void
ColorSelectorWidget::setTriangleSize()
{
    int triangleSize = _spinA ? 5 : 4;
    int padding = 10;

    triangleSize = (_spinR->size().height() * triangleSize) + (triangleSize * padding);

    _triangle->setMinimumSize(triangleSize, triangleSize);
    _triangle->setMaximumSize(triangleSize, triangleSize);
}

void
ColorSelectorWidget::initPaletteButtons(QWidget *widget,
                                        int rows,
                                        int cols)
{
    if (!widget) {
        return;
    }

    _paletteButtons.clear();
    QVBoxLayout *widgetLayout = new QVBoxLayout(widget);
    widget->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setSpacing(0);

    for (int x = 0; x < rows; ++x) {
        QWidget *colWidget = new QWidget(widget);
        colWidget->setContentsMargins(0, 0, 0, 0);
        QHBoxLayout *colLayout = new QHBoxLayout(colWidget);
        colLayout->setContentsMargins(0, 0, 0, 0);
        colLayout->setSpacing(0);
        widgetLayout->addWidget(colWidget);
        for (int y = 0; y < cols; ++y) {
            ColorSelectorPaletteButton *button = new ColorSelectorPaletteButton(colWidget);
            QObject::connect( button, SIGNAL( colorPicked(float,float,float,float) ),
                              this, SLOT( setColorFromPalette(float,float,float,float) ) );
            colLayout->addWidget(button);
            _paletteButtons << button;
        }
    }
}

void
ColorSelectorWidget::updatePaletteButtons()
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME),
                        QString::fromUtf8(NATRON_APPLICATION_NAME) );
    settings.beginGroup( QString::fromUtf8(COLOR_SELECTOR_PALETTE_SETTINGS) );

    for (int i = 0; i < _paletteButtons.size(); ++i) {
        if ( !settings.value( QString::number(i) ).isValid() ) {
            continue;
        }
        QStringList colorValues = settings.value( QString::number(i) ).toStringList();
        if (colorValues.size() > 3) {
            _paletteButtons.at(i)->setColor( colorValues.at(0).toFloat(),
                                             colorValues.at(1).toFloat(),
                                             colorValues.at(2).toFloat(),
                                             colorValues.at(3).toFloat() );
        }
    }

    settings.endGroup();
}

void
ColorSelectorWidget::savePaletteButton(int id)
{
    if (_paletteButtons.size() < 1 || id >= _paletteButtons.size() || id < 0) {
        return;
    }

    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME),
                        QString::fromUtf8(NATRON_APPLICATION_NAME) );
    settings.beginGroup( QString::fromUtf8(COLOR_SELECTOR_PALETTE_SETTINGS) );

    QStringList colorValues;
    float r, g, b, a;
    _paletteButtons.at(id)->getColor(&r, &g, &b, &a);
    colorValues << QString::number(r) << QString::number(g);
    colorValues << QString::number(b) << QString::number(a);
    settings.setValue(QString::number(id), colorValues);

    settings.endGroup();
}

void
ColorSelectorWidget::savePaletteButtons()
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME),
                        QString::fromUtf8(NATRON_APPLICATION_NAME) );
    settings.beginGroup( QString::fromUtf8(COLOR_SELECTOR_PALETTE_SETTINGS) );

    for (int i = 0; i < _paletteButtons.size(); ++i) {
        QStringList colorValues;
        float r, g, b, a;
        _paletteButtons.at(i)->getColor(&r, &g, &b, &a);
        colorValues << QString::number(r) << QString::number(g);
        colorValues << QString::number(b) << QString::number(a);
        settings.setValue(QString::number(i), colorValues);
    }

    settings.endGroup();
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
    float a = _slideA ? _spinA->value() : 1.;
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
    float a = _slideA ? _spinA->value() : 1.;
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
ColorSelectorWidget::handleButtonColorClicked(int /*id*/)
{
    QVariant var = _buttonColorGroup->checkedButton()->property(COLOR_SELECTOR_BUTTON_PROPERTY);
    if ( var.isValid() ) {
        _stack->setCurrentIndex( var.toInt() );
    }
}

void
ColorSelectorWidget::setPaletteButtonColor(bool /*clicked*/)
{
    if (_paletteButtons.size() < 1) {
        return;
    }

    int buttonID = 0;
    bool resetModified = false;
    for (int i = 0; i < _paletteButtons.size(); ++i) {
        if ( _paletteButtons.at(i)->isModified() ) {
            if (i == (_paletteButtons.size() - 1) ) {
                resetModified = true;
            }
            continue;
        }
        buttonID = i;
        break;
    }

    if (resetModified) {
        for (int i = 0; i < _paletteButtons.size(); ++i) {
            _paletteButtons.at(i)->clearModified();
        }
    }

    if ( buttonID < _paletteButtons.size() ) {
        _paletteButtons.at(buttonID)->setColor( _spinR->value(),
                                                _spinG->value(),
                                                _spinB->value(),
                                                _spinA ? _spinA->value() : 1.0 );
        savePaletteButton(buttonID);
    }
}

void
ColorSelectorWidget::clearPaletteButtons(bool /*clicked*/)
{
    for (int i = 0; i < _paletteButtons.size(); ++i) {
        _paletteButtons.at(i)->clearColor();
    }
    savePaletteButtons();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ColorSelectorWidget.cpp"
