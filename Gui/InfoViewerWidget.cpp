/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "InfoViewerWidget.h"

#include <cstdlib>
#include <stdexcept>

#include <QtGui/QImage>
#include <QHBoxLayout>
#include <QtCore/QThread>
#include <QPaintEvent>
#include <QPainter>
#include <QtCore/QCoreApplication>

#include "Engine/ViewerInstance.h"
#include "Engine/Lut.h"
#include "Engine/Image.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ViewerGL.h"
#include "Gui/Label.h"

using std::cout; using std::endl;
NATRON_NAMESPACE_ENTER

InfoViewerWidget::InfoViewerWidget(const QString & description,
                                   QWidget* parent)
    : QWidget(parent)
    , _comp( ImagePlaneDesc::getNoneComponents() )
    , _colorValid(false)
    , _colorApprox(false)
{
    for (int i = 0; i < 4; ++i) {
        currentColor[i] = 0;
    }

    //Using this constructor of QFontMetrics will respect the DPI of the screen, see http://doc.qt.io/qt-4.8/qfontmetrics.html#QFontMetrics-2
    QFontMetrics fm(font(), 0);
    setFixedHeight( fm.height() );

    setStyleSheet( QString::fromUtf8("background-color: black;\n"
                                     "color : rgba(200,200,200,255);\n"
                                     "QToolTip { background-color: black;}") );

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);


    QString tt = QString( tr("Information, from left to right:<br />"
                             "<br />"
                             "<font color=orange>Input:</font> Specifies whether the information are for the input <b>A</b> or <b>B</b><br />"
                             "<font color=orange>Image format:</font>  An identifier for the pixel components and bitdepth of the displayed image<br />"
                             "<font color=orange>Format:</font>  The resolution of the input (where the image is displayed)<br />"
                             "<font color=orange>RoD:</font>  The region of definition of the displayed image (where the data is defined)<br />"
                             "<font color=orange>Fps:</font>  (Only active during playback) The frame-rate of the play-back sustained by the viewer<br />"
                             "<font color=orange>Coordinates:</font>  The coordinates of the current mouse location<br />"
                             "<font color=orange>RGBA:</font>  The RGBA color of the displayed image. Note that if some <b>?</b> are set instead of colors "
                             "that means the underlying image cannot be accessed internally, you should refresh the viewer to make it available. "
                             "Also sometimes you may notice the tilde '~' before the colors: it indicates whether the color indicated is the true "
                             "color in the image (no tilde) or this is an approximated mipmap that has been filtered with a box filter (tilde), "
                             "in which case this may not reflect exactly the underlying internal image. <br />"
                             "<font color=orange>HSVL:</font>  For convenience the RGBA color is also displayed as HSV(L)"
                             "") );
    setToolTip(tt);

    layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    descriptionLabel = new Label(this);
    {
        const QFont& font = descriptionLabel->font();
        QString descriptionText = QString::fromUtf8("<font color=\"#DBE0E0\" face=\"%1\" size=%2>")
                                  .arg( font.family() )
                                  .arg( font.pixelSize() );
        descriptionText.append(description);
        descriptionText.append( QString::fromUtf8("</font>") );
        descriptionLabel->setText(descriptionText);
        QFontMetrics fm = descriptionLabel->fontMetrics();
        int width = fm.width( QString::fromUtf8("A:") );
        descriptionLabel->setMinimumWidth(width);
    }
    imageFormat = new Label(this);
    {
        QFontMetrics fm = imageFormat->fontMetrics();
        int width = fm.width( QString::fromUtf8("backward.motion32f") );
        imageFormat->setMinimumWidth(width);
    }

    resolution = new Label(this);
    {
        QFontMetrics fm = resolution->fontMetrics();
        int width = fm.width( QString::fromUtf8("2K_Super_35(full-ap) 00000x00000:0.00") );
        resolution->setMinimumWidth(width);
    }


    coordDispWindow = new Label(this);
    {
        QFontMetrics fm = coordDispWindow->fontMetrics();
        int width = fm.width( QString::fromUtf8("RoD: 000 000 0000 0000") );
        coordDispWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        coordDispWindow->setMinimumWidth(width);
    }

    _fpsLabel = new Label(this);
    {
        QFontMetrics fm = _fpsLabel->fontMetrics();
        int width = fm.width( QString::fromUtf8("100 fps") );
        _fpsLabel->setMinimumWidth(width);
        _fpsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        _fpsLabel->hide();
    }

    coordMouse = new Label(this);
    {
        QFontMetrics fm = coordMouse->fontMetrics();
        int width = fm.width( QString::fromUtf8("x=00000 y=00000") );
        coordMouse->setMinimumWidth(width);
    }

    rgbaValues = new Label(this);
    {
        QFontMetrics fm = rgbaValues->fontMetrics();
        int width = fm.width( QString::fromUtf8("0.00000 0.00000 0.00000 ~ ") );
        rgbaValues->setMinimumWidth(width);
    }

    color = new Label(this);
    color->setFixedSize( TO_DPIX(20), TO_DPIY(20) );

    hvl_lastOption = new Label(this);
    {
        QFontMetrics fm = hvl_lastOption->fontMetrics();
        int width = fm.width( QString::fromUtf8("H:000 S:0.00 V:0.00 L:0.00000") );
        hvl_lastOption->setMinimumWidth(width);
    }

    layout->addWidget(descriptionLabel);
    layout->addWidget(imageFormat);
    layout->addWidget(resolution);
    layout->addWidget(coordDispWindow);
    layout->addWidget(_fpsLabel);
    layout->addWidget(coordMouse);
    layout->addWidget(rgbaValues);
    layout->addWidget(color);
    layout->addWidget(hvl_lastOption);
}

QSize
InfoViewerWidget::sizeHint() const
{
    return QWidget::sizeHint();
}

QSize
InfoViewerWidget::minimumSizeHint() const
{
    return QWidget::minimumSizeHint();
}

void
InfoViewerWidget::setFps(double actualFps,
                         double desiredFps)
{
    QString colorStr = QString::fromUtf8("green");
    const QFont& font = _fpsLabel->font();

    if ( ( actualFps < (desiredFps -  desiredFps / 10.f) ) && ( actualFps > (desiredFps / 2.f) ) ) {
        colorStr = QString::fromUtf8("orange");
    } else if ( actualFps < (desiredFps / 2.f) ) {
        colorStr = QString::fromUtf8("red");
    }
    QString str = QString::fromUtf8("<font color=\"") + colorStr + QString::fromUtf8("\" face=\"%2\" size=%3>%1 fps</font>")
                  .arg( QString::number(actualFps, 'f', 1) )
                  .arg( font.family() )
                  .arg( font.pixelSize() );

    _fpsLabel->setText(str);
    if ( !_fpsLabel->isVisible() ) {
        _fpsLabel->show();
    }
}

void
InfoViewerWidget::hideFps()
{
    if ( _fpsLabel->isVisible() ) {
        _fpsLabel->hide();
    }
}

bool
InfoViewerWidget::colorVisible()
{
    return hvl_lastOption->isVisible();
}

void
InfoViewerWidget::showColorInfo()
{
    hvl_lastOption->show();
    rgbaValues->show();
    color->show();
}

void
InfoViewerWidget::hideColorInfo()
{
    hvl_lastOption->hide();
    rgbaValues->hide();
    color->hide();
}

bool
InfoViewerWidget::mouseVisible()
{
    return coordMouse->isVisible();
}

void
InfoViewerWidget::showMouseInfo()
{
    coordMouse->show();
}

void
InfoViewerWidget::hideMouseInfo()
{
    coordMouse->hide();
}

InfoViewerWidget::~InfoViewerWidget()
{
}

void
InfoViewerWidget::setColorApproximated(bool approx)
{
    if (_colorApprox == approx) {
        return;
    }
    _colorApprox = approx;
    setColor(currentColor[0], currentColor[1], currentColor[2], currentColor[3]);
}

void
InfoViewerWidget::setColor(float r,
                           float g,
                           float b,
                           float a)
{
    QString values;
    const QFont& font = rgbaValues->font();

    currentColor[0] = r;
    currentColor[1] = g;
    currentColor[2] = b;
    currentColor[3] = a;

    QString rS = _colorValid ? QString::number(r, 'f', 5) : QString::fromUtf8("?");
    QString gS = _colorValid ? QString::number(g, 'f', 5) : QString::fromUtf8("?");
    QString bS = _colorValid ? QString::number(b, 'f', 5) : QString::fromUtf8("?");
    QString aS = _colorValid ? QString::number(a, 'f', 5) : QString::fromUtf8("?");

    //values = QString("<font color='red'>%1</font> <font color='green'>%2</font> <font color='blue'>%3</font> <font color=\"#DBE0E0\">%4</font>")
    // the following three colors have an equal luminance (=0.4), which makes the text easier to read.

    if ( (_comp.getNumComponents() == 0) || (_comp.getNumComponents() > 4) ) {
        values = QString();
    } else if (_comp.getNumComponents() == 1) {
        values = QString::fromUtf8("<font color=\"#DBE0E0\" face=\"%2\" size=%3>%1</font>")
                 .arg(aS).arg( font.family() ).arg( font.pixelSize() );
    } else if (_comp.getNumComponents() == 2) {
        values = QString::fromUtf8("<font color='#d93232' face=\"%3\" size=%4>%1  </font>"
                                   "<font color='#00a700' face=\"%3\" size=%4>%2  </font>")
                 .arg(rS)
                 .arg(gS)
                 .arg( font.family() )
                 .arg( font.pixelSize() );
    } else if (_comp.getNumComponents() == 3) {
        values = QString::fromUtf8("<font color='#d93232' face=\"%4\" size=%5>%1  </font>"
                                   "<font color='#00a700' face=\"%4\" size=%5>%2  </font>"
                                   "<font color='#5858ff' face=\"%4\" size=%5>%3</font>")
                 .arg(rS)
                 .arg(gS)
                 .arg(bS)
                 .arg( font.family() )
                 .arg( font.pixelSize() );
    } else if (_comp.getNumComponents() == 4) {
        values = QString::fromUtf8("<font color='#d93232' face=\"%5\" size=%6>%1  </font>"
                                   "<font color='#00a700' face=\"%5\" size=%6>%2  </font>"
                                   "<font color='#5858ff' face=\"%5\" size=%6>%3  </font>"
                                   "<font color=\"#DBE0E0\" face=\"%5\" size=%6>%4</font>")
                 .arg(rS)
                 .arg(gS)
                 .arg(bS)
                 .arg(aS)
                 .arg( font.family() )
                 .arg( font.pixelSize() );
    }

    if (_colorApprox) {
        values.prepend( QString::fromUtf8("<b> ~ </b>") );
    }
    rgbaValues->setText(values);
    rgbaValues->update();
    float h, s, v, l;
    // Nuke's HSV display is based on sRGB until Nuke 8, an L is Rec.709.
    // see https://community.foundry.com/discuss/topic/100271
    // This was changed in Nuke 9 to use linear values.
    double srgb_r = Color::to_func_srgb(r);
    double srgb_g = Color::to_func_srgb(g);
    double srgb_b = Color::to_func_srgb(b);
    QColor col;
    col.setRgbF( Image::clamp(srgb_r, 0., 1.), Image::clamp(srgb_g, 0., 1.), Image::clamp(srgb_b, 0., 1.) );
    QPixmap pix(15, 15);
    pix.fill(col);
    color->setPixmap(pix);
    color->update();

    const QFont &hsvFont = hvl_lastOption->font();
    // Nuke 5-8 version used sRGB colors to compute HSV
    //Color::rgb_to_hsv(srgb_r, srgb_g, srgb_b, &h, &s, &v);

    // However, as Alvin Ray Smith said in his paper,
    // "We shall assume that an RGB monitor is a linear device"
    // We thus use linear values (same as Nuke 9 and later).
    // Fixes https://github.com/NatronGitHub/Natron/issues/286
    // Reference:
    // "Color gamut transform pairs", Alvy Ray Smith, Proceeding SIGGRAPH '78
    // https://doi.org/10.1145/800248.807361
    // http://www.icst.pku.edu.cn/F/course/ImageProcessing/2018/resource/Color78.pdf
    Color::rgb_to_hsv(r, g, b, &h, &s, &v);
    l = 0.2125 * r + 0.7154 * g + 0.0721 * b; // L according to Rec.709

    QString lS = _colorValid ? QString::number(l, 'f', 5) : QString::fromUtf8("?");
    assert(NATRON_COLOR_HUE_CIRCLE == 1.);
    QString hS = _colorValid ? QString::number((int)(h * 360 + 0.5)) : QString::fromUtf8("?");
    QString sS = _colorValid ? QString::number(s, 'f', 2) : QString::fromUtf8("?");
    QString vS = _colorValid ? QString::number(v, 'f', 2) : QString::fromUtf8("?");
    QString hsvlValues;
    hsvlValues = QString::fromUtf8("<font color=\"#DBE0E0\" face=\"%5\" size=%6>H:%1 S:%2 V:%3  L:%4</font>")
                 .arg(hS)
                 .arg(sS)
                 .arg(vS)
                 .arg(lS)
                 .arg( hsvFont.family() )
                 .arg( hsvFont.pixelSize() );

    hvl_lastOption->setText(hsvlValues);
    hvl_lastOption->update();
} // setColor

void
InfoViewerWidget::setColorValid(bool valid)
{
    if (_colorValid == valid) {
        return;
    }
    _colorValid = valid;
    if (!valid) {
        setColor(currentColor[0], currentColor[1], currentColor[2], currentColor[3]);
    }
}

void
InfoViewerWidget::setMousePos(QPoint p)
{
    QString coord;
    const QFont& font = coordMouse->font();

    coord = QString::fromUtf8("<font color=\"#DBE0E0\" face=\"%3\" size=%4>x=%1 y=%2</font>")
            .arg( p.x() )
            .arg( p.y() )
            .arg( font.family() )
            .arg( font.pixelSize() );
    coordMouse->setText(coord);
}


void
InfoViewerWidget::setResolution(const QString & f)
{
    assert( QThread::currentThread() == qApp->thread() );
    const QFont& font = resolution->font();
    QString reso;
    reso = QString::fromUtf8("<font color=\"#DBE0E0\" face=\"%2\" size=%3>%1</font>")
    .arg(f)
    .arg( font.family() )
    .arg( font.pixelSize() );
    resolution->setText(reso);

}

void
InfoViewerWidget::setDataWindow(const RectI & r)
{
    QString bbox;
    const QFont& font = coordDispWindow->font();
    QString x1, y1, x2, y2;

    x1.setNum(r.x1);
    y1.setNum(r.y1);
    x2.setNum(r.x2);
    y2.setNum(r.y2);

    bbox = QString::fromUtf8("<font color=\"#DBE0E0\" face=\"%5\" size=%6>RoD: %1 %2 %3 %4</font>")
           .arg(x1)
           .arg(y1)
           .arg(x2)
           .arg(y2)
           .arg( font.family() )
           .arg( font.pixelSize() );

    coordDispWindow->setText(bbox);
}

void
InfoViewerWidget::setImageFormat(const ImagePlaneDesc& comp,
                                 ImageBitDepthEnum depth)
{
    const QFont& font = imageFormat->font();
    QString text = QString::fromUtf8("<font color=\"#DBE0E0\" face=\"%1\" size=%2>")
                   .arg( font.family() )
                   .arg( font.pixelSize() );
    QString format = QString::fromUtf8( Image::getFormatString(comp, depth).c_str() );

    text.append(format);
    text.append( QString::fromUtf8("</font>") );
    imageFormat->setText(text);
    _comp = comp;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_InfoViewerWidget.cpp"
