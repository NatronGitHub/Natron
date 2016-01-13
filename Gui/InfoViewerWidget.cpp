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

#include "InfoViewerWidget.h"

#include <cstdlib>
#include <iostream>
#include <QtGui/QImage>
#include <QHBoxLayout>
#include <QThread>
#include <QPaintEvent>
#include <QPainter>
#include <QCoreApplication>

#include "Engine/ViewerInstance.h"
#include "Engine/Lut.h"
#include "Engine/Image.h"
#include "Gui/ViewerGL.h"
#include "Gui/Label.h"

using std::cout; using std::endl;
using namespace Natron;

InfoViewerWidget::InfoViewerWidget(ViewerGL* v,
                                   const QString & description,
                                   QWidget* parent)
: QWidget(parent)
, viewer(v)
, _comp(ImageComponents::getNoneComponents())
, _colorValid(false)
, _colorApprox(false)
{
    for (int i = 0; i < 4; ++i) {
        currentColor[i] = 0;
    }

    //Using this constructor of QFontMetrics will respect the DPI of the screen, see http://doc.qt.io/qt-4.8/qfontmetrics.html#QFontMetrics-2
    QFontMetrics fm(font(),0);
    setFixedHeight(fm.height());

    setStyleSheet( QString("background-color: black;\n"
                           "color : rgba(200,200,200,255);\n"
                           "QToolTip { background-color: black;}") );
    
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    
    QString tt = QString(tr("Informations from left to right:<br/>"
                            "<br/>"
                            "<br><font color=orange>Input:</font> Specifies whether the information are for the input <b>A</b> or <b>B</b></br>"
                            "<br/>"
                            "<br><font color=orange>Image format:</font>  An identifier for the pixel components and bitdepth of the displayed image</br>"
                            "<br/>"
                            "<br><font color=orange>Format:</font>  The resolution of the project's format</br>"
                            "<br/>"
                            "<br><font color=orange>RoD:</font>  The region of definition of the displayed image</br>"
                            "<br/>"
                            "<br><font color=orange>Fps:</font>  (Only active during playback) The frame-rate of the play-back sustained by the viewer</br>"
                            "<br/>"
                            "<br><font color=orange>Coordinates:</font>  The coordinates of the current mouse location</br>"
                            "<br/>"
                            "<br><font color=orange>RGBA:</font>  The RGBA color of the displayed image. Note that if some <b>?</b> are set instead of colors "
                            "that means the underlying image cannot be accessed internally, you should refresh the viewer to make it available. "
                            "Also sometimes you may notice the tild '~' before the colors: it indicates whether the color indicated is the true "
                            "color in the image (no tild) or this is an approximated mipmap that has been filtered with a box filter (tild), "
                            "in which case this may not reflect exactly the underlying internal image. </br>"
                            "<br/>"
                            "<br><font color=orange>HSVL:</font>  For convenience the RGBA color is also displayed as HSV(L)</br>"
                            "<br/>"
                            ""));
    setToolTip(tt);

    layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    descriptionLabel = new Natron::Label(this);
    {
        const QFont& font = descriptionLabel->font();

        QString descriptionText = QString("<font color=\"#DBE0E0\" face=\"%1\" size=%2>")
        .arg(font.family())
        .arg(font.pixelSize());
        descriptionText.append(description);
        descriptionText.append("</font>");
        descriptionLabel->setText(descriptionText);
        QFontMetrics fm = descriptionLabel->fontMetrics();
        int width = fm.width("A:");
        descriptionLabel->setMinimumWidth(width);
    }
    imageFormat = new Natron::Label(this);
    {
        QFontMetrics fm = imageFormat->fontMetrics();
        int width = fm.width("backward.motion32f");
        imageFormat->setMinimumWidth(width);
    }

    resolution = new Natron::Label(this);
    {
        QFontMetrics fm = resolution->fontMetrics();
        int width = fm.width("00000x00000");
        resolution->setMinimumWidth(width);
    }


    coordDispWindow = new Natron::Label(this);
    {
        QFontMetrics fm = coordDispWindow->fontMetrics();
        int width = fm.width("RoD: 000 000 0000 0000");
        coordDispWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        coordDispWindow->setMinimumWidth(width);
    }
   
    _fpsLabel = new Natron::Label(this);
    {
        QFontMetrics fm = _fpsLabel->fontMetrics();
        int width = fm.width("100 fps");
        _fpsLabel->setMinimumWidth(width);
        _fpsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        _fpsLabel->hide();
    }
    
    coordMouse = new Natron::Label(this);
    {
        QFontMetrics fm = coordMouse->fontMetrics();
        int width = fm.width("x=00000 y=00000");
        coordMouse->setMinimumWidth(width);
    }

    rgbaValues = new Natron::Label(this);
    {
        QFontMetrics fm = rgbaValues->fontMetrics();
        int width = fm.width("0.00000 0.00000 0.00000 ~ ");
        rgbaValues->setMinimumWidth(width);
    }

    color = new Natron::Label(this);
    color->setFixedSize(20, 20);
    
    hvl_lastOption = new Natron::Label(this);
    {
        QFontMetrics fm = hvl_lastOption->fontMetrics();
        int width = fm.width("H:000 S:0.00 V:0.00 L:0.00000");
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
    QString colorStr("green");
    
    const QFont& font = _fpsLabel->font();
    
    if ( ( actualFps < (desiredFps -  desiredFps / 10.f) ) && ( actualFps > (desiredFps / 2.f) ) ) {
        colorStr = QString("orange");
    } else if ( actualFps < (desiredFps / 2.f) ) {
        colorStr = QString("red");
    }
    QString str = QString("<font color=\""+ colorStr + "\" face=\"%2\" size=%3>%1 fps</font>")
    .arg( QString::number(actualFps,'f',1) )
    .arg(font.family())
    .arg(font.pixelSize());

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
InfoViewerWidget::colorAndMouseVisible()
{
    return coordMouse->isVisible();
}

void
InfoViewerWidget::showColorAndMouseInfo()
{
    coordMouse->show();
    hvl_lastOption->show();
    rgbaValues->show();
    color->show();
}

void
InfoViewerWidget::hideColorAndMouseInfo()
{
    coordMouse->hide();
    hvl_lastOption->hide();
    rgbaValues->hide();
    color->hide();
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
    
    QString rS = _colorValid ? QString::number(r,'f',5) : "?";
    QString gS = _colorValid ? QString::number(g,'f',5) : "?";
    QString bS = _colorValid ? QString::number(b,'f',5) : "?";
    QString aS = _colorValid ? QString::number(a,'f',5) : "?";
    
    //values = QString("<font color='red'>%1</font> <font color='green'>%2</font> <font color='blue'>%3</font> <font color=\"#DBE0E0\">%4</font>")
    // the following three colors have an equal luminance (=0.4), which makes the text easier to read.

    if (_comp.getNumComponents() == 0 || _comp.getNumComponents() > 4) {
        values = QString();
    } else if (_comp.getNumComponents() == 1) {
        values = QString("<font color=\"#DBE0E0\" face=\"%2\" size=%3>%1</font>")
            .arg(aS).arg(font.family()).arg(font.pixelSize());
    } else if (_comp.getNumComponents() == 2) {
        values = QString("<font color='#d93232' face=\"%3\" size=%4>%1  </font>"
                         "<font color='#00a700' face=\"%3\" size=%4>%2  </font>")
        .arg(rS)
        .arg(gS)
        .arg(font.family())
        .arg(font.pixelSize());
    } else if (_comp.getNumComponents() == 3) {
        values = QString("<font color='#d93232' face=\"%4\" size=%5>%1  </font>"
                           "<font color='#00a700' face=\"%4\" size=%5>%2  </font>"
                           "<font color='#5858ff' face=\"%4\" size=%5>%3</font>")
                   .arg(rS)
                   .arg(gS)
                   .arg(bS)
                   .arg(font.family())
                   .arg(font.pixelSize());
    } else if (_comp.getNumComponents() == 4) {
        values = QString("<font color='#d93232' face=\"%5\" size=%6>%1  </font>"
                           "<font color='#00a700' face=\"%5\" size=%6>%2  </font>"
                           "<font color='#5858ff' face=\"%5\" size=%6>%3  </font>"
                           "<font color=\"#DBE0E0\" face=\"%5\" size=%6>%4</font>")
                   .arg(rS)
                   .arg(gS)
                   .arg(bS)
                   .arg(aS)
                   .arg(font.family())
                   .arg(font.pixelSize());
    }
    
    if (_colorApprox) {
        values.prepend("<b> ~ </b>");
    }
    rgbaValues->setText(values);
    rgbaValues->update();
    float h,s,v,l;
    // Nuke's HSV display is based on sRGB, an L is Rec.709.
    // see http://forums.thefoundry.co.uk/phpBB2/viewtopic.php?t=2283
    double srgb_r = Color::to_func_srgb(r);
    double srgb_g = Color::to_func_srgb(g);
    double srgb_b = Color::to_func_srgb(b);
    QColor col;
    col.setRgbF( Natron::clamp(srgb_r, 0., 1.), Natron::clamp(srgb_g, 0., 1.), Natron::clamp(srgb_b, 0., 1.) );
    QPixmap pix(15,15);
    pix.fill(col);
    color->setPixmap(pix);
    color->update();

    const QFont &hsvFont = hvl_lastOption->font();

    
    Color::rgb_to_hsv(srgb_r,srgb_g,srgb_b,&h,&s,&v);
    l = 0.2125 * r + 0.7154 * g + 0.0721 * b; // L according to Rec.709
    
    QString lS = _colorValid ? QString::number(l,'f',5) : "?";
    QString hS = _colorValid ? QString::number(h,'f',0) : "?";
    QString sS = _colorValid ? QString::number(s,'f',2) : "?";
    QString vS = _colorValid ? QString::number(v,'f',2) : "?";
    
    QString hsvlValues;
    hsvlValues = QString("<font color=\"#DBE0E0\" face=\"%5\" size=%6>H:%1 S:%2 V:%3  L:%4</font>")
                 .arg(hS)
                 .arg(sS)
                 .arg(vS)
                 .arg(lS)
                 .arg(hsvFont.family())
                 .arg(hsvFont.pixelSize());

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
    coord = QString("<font color=\"#DBE0E0\" face=\"%3\" size=%4>x=%1 y=%2</font>")
            .arg( p.x() )
            .arg( p.y() )
            .arg(font.family())
            .arg(font.pixelSize());
    coordMouse->setText(coord);
}

void
InfoViewerWidget::removeTrailingZeroes(QString& str)
{
    int dot = str.lastIndexOf('.');
    if (dot != -1) {
        int i = str.size() - 1;
        while (i > dot) {
            if (str[i] == QChar('0')) {
                --i;
            } else {
                break;
            }
        }
        if (i != (str.size() -1)) {
            str.remove(i, str.size() - i);
        }
    }
}

void
InfoViewerWidget::setResolution(const Format & f)
{
    assert( QThread::currentThread() == qApp->thread() );
    format = f;
    
    const QFont& font = resolution->font();
    if ( format.getName().empty() ) {
        
        QString w,h;
        w.setNum(format.width());
        h.setNum(format.height());
        removeTrailingZeroes(w);
        removeTrailingZeroes(h);
        
        QString reso;
        reso = QString("<font color=\"#DBE0E0\" face=\"%3\" size=%4>%1x%2</font>")
        .arg(w)
        .arg(h)
        .arg(font.family())
        .arg(font.pixelSize());
        resolution->setText(reso);
    } else {
        QString reso = QString("<font color=\"#DBE0E0\" face=\"%1\" size=%2>")
        .arg(font.family())
        .arg(font.pixelSize());
        reso.append( format.getName().c_str() );
      //  reso.append("\t");
        reso.append("</font>");
        resolution->setText(reso);
    }
}

void
InfoViewerWidget::setDataWindow(const RectI & r)
{
    QString bbox;
    const QFont& font = coordDispWindow->font();
    QString left,btm,right,top;
    left.setNum(r.left());
    btm.setNum(r.bottom());
    right.setNum(r.right());
    top.setNum(r.top());
    
    bbox = QString("<font color=\"#DBE0E0\" face=\"%5\" size=%6>RoD: %1 %2 %3 %4</font>")
    .arg(left)
    .arg(btm)
    .arg(right)
    .arg(top)
    .arg(font.family())
    .arg(font.pixelSize());
    
    coordDispWindow->setText(bbox);
}

void
InfoViewerWidget::setImageFormat(const Natron::ImageComponents& comp,
                                 Natron::ImageBitDepthEnum depth)
{

    const QFont& font = imageFormat->font();
    QString text = QString("<font color=\"#DBE0E0\" face=\"%1\" size=%2>")
    .arg(font.family())
    .arg(font.pixelSize());
    QString format(Natron::Image::getFormatString(comp, depth).c_str());
    text.append(format);
    text.append("</font>");
    imageFormat->setText(text);
    _comp = comp;
}

