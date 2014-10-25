//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. *//*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "InfoViewerWidget.h"

#include <cstdlib>
#include <iostream>
#include <QtGui/QImage>
#include <QHBoxLayout>
#include <QLabel>
#include <QThread>
#include <QPaintEvent>
#include <QPainter>
#include <QCoreApplication>

#include "Engine/ViewerInstance.h"
#include "Engine/Lut.h"
#include "Engine/Image.h"
#include "Gui/ViewerGL.h"

using std::cout; using std::endl;
using namespace Natron;

InfoViewerWidget::InfoViewerWidget(ViewerGL* v,
                                   const QString & description,
                                   QWidget* parent)
    : QWidget(parent)
      , viewer(v)
      , _comp(eImageComponentNone)
{

    setFixedHeight(20);
    setStyleSheet( QString("background :black") );
    
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    descriptionLabel = new QLabel(this);
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
    imageFormat = new QLabel(this);
    {
        QFontMetrics fm = imageFormat->fontMetrics();
        int width = fm.width("RGBA32f");
        imageFormat->setMinimumWidth(width);
    }

    resolution = new QLabel(this);
    {
        QFontMetrics fm = resolution->fontMetrics();
        int width = fm.width("00000x00000");
        resolution->setMinimumWidth(width);
    }


    coordDispWindow = new QLabel(this);
    {
        QFontMetrics fm = coordDispWindow->fontMetrics();
        int width = fm.width("RoD: 000 000 0000 0000");
        coordDispWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        coordDispWindow->setMinimumWidth(width);
    }
   
    _fpsLabel = new QLabel(this);
    {
        QFontMetrics fm = _fpsLabel->fontMetrics();
        int width = fm.width("100 fps");
        _fpsLabel->setMinimumWidth(width);
        _fpsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        _fpsLabel->hide();
    }
    
    coordMouse = new QLabel(this);
    {
        QFontMetrics fm = coordMouse->fontMetrics();
        int width = fm.width("x=00000 y=00000");
        coordMouse->setMinimumWidth(width);
    }

    rgbaValues = new QLabel(this);
    {
        QFontMetrics fm = rgbaValues->fontMetrics();
        int width = fm.width("0.00000 0.00000 0.00000");
        rgbaValues->setMinimumWidth(width);
    }

    color = new QLabel(this);
    color->setFixedSize(20, 20);
    
    hvl_lastOption = new QLabel(this);
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
InfoViewerWidget::setColor(float r,
                           float g,
                           float b,
                           float a)
{
    QString values;
    const QFont& font = rgbaValues->font();
    //values = QString("<font color='red'>%1</font> <font color='green'>%2</font> <font color='blue'>%3</font> <font color=\"#DBE0E0\">%4</font>")
    // the following three colors have an equal luminance (=0.4), which makes the text easier to read.
    switch (_comp) {
    case Natron::eImageComponentNone:
        values = QString();
        break;
    case Natron::eImageComponentAlpha:
        values = QString("<font color=\"#DBE0E0\" face=\"%2\" size=%3>%1</font>")
            .arg(a,0,'f',5).arg(font.family()).arg(font.pixelSize());
        break;
    case Natron::eImageComponentRGB:
        values = QString("<font color='#d93232' face=\"%4\" size=%5>%1  </font>"
                           "<font color='#00a700' face=\"%4\" size=%5>%2  </font>"
                           "<font color='#5858ff' face=\"%4\" size=%5>%3</font>")
                   .arg(r,0,'f',5)
                   .arg(g,0,'f',5)
                   .arg(b,0,'f',5)
                   .arg(font.family())
                   .arg(font.pixelSize());
        break;
    case Natron::eImageComponentRGBA:
        values = QString("<font color='#d93232' face=\"%5\" size=%6>%1  </font>"
                           "<font color='#00a700' face=\"%5\" size=%6>%2  </font>"
                           "<font color='#5858ff' face=\"%5\" size=%6>%3  </font>"
                           "<font color=\"#DBE0E0\" face=\"%5\" size=%6>%4</font>")
                   .arg(r,0,'f',5)
                   .arg(g,0,'f',5)
                   .arg(b,0,'f',5)
                   .arg(a,0,'f',5)
                   .arg(font.family())
                   .arg(font.pixelSize());
        break;
    }
    rgbaValues->setText(values);
    rgbaValues->repaint();
    float h,s,v,l;
    // Nuke's HSV display is based on sRGB, an L is Rec.709.
    // see http://forums.thefoundry.co.uk/phpBB2/viewtopic.php?t=2283
    double srgb_r = Color::to_func_srgb(r);
    double srgb_g = Color::to_func_srgb(g);
    double srgb_b = Color::to_func_srgb(b);
    QColor col;
    col.setRgbF( Natron::clamp(srgb_r), Natron::clamp(srgb_g), Natron::clamp(srgb_b) );
    QPixmap pix(15,15);
    pix.fill(col);
    color->setPixmap(pix);
    color->repaint();

    const QFont &hsvFont = hvl_lastOption->font();

    Color::rgb_to_hsv(srgb_r,srgb_g,srgb_b,&h,&s,&v);
    l = 0.2125 * r + 0.7154 * g + 0.0721 * b; // L according to Rec.709
    QString hsvlValues;
    hsvlValues = QString("<font color=\"#DBE0E0\" face=\"%5\" size=%6>H:%1 S:%2 V:%3  L:%4</font>")
                 .arg(h,0,'f',0)
                 .arg(s,0,'f',2)
                 .arg(v,0,'f',2)
                 .arg(l,0,'f',5)
                 .arg(hsvFont.family())
                 .arg(hsvFont.pixelSize());

    hvl_lastOption->setText(hsvlValues);
    hvl_lastOption->repaint();
} // setColor

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
InfoViewerWidget::setResolution(const Format & f)
{
    assert( QThread::currentThread() == qApp->thread() );
    format = f;
    
    const QFont& font = resolution->font();
    if ( format.getName() == std::string("") ) {
        QString reso;
        reso = QString("<font color=\"#DBE0E0\" face=\"%3\" size=%4>%1x%2</font>")
        .arg( std::ceil(format.width()) )
        .arg( std::ceil(format.height()) )
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
InfoViewerWidget::setDataWindow(const RectD & r)
{
    QString bbox;
    const QFont& font = coordDispWindow->font();
    bbox = QString("<font color=\"#DBE0E0\" face=\"%5\" size=%6>RoD: %1 %2 %3 %4</font>")
           .arg( std::ceil( r.left() ) )
           .arg( std::ceil( r.bottom() ) )
           .arg( std::floor( r.right() ) )
           .arg( std::floor( r.top() ) )
           .arg(font.family())
           .arg(font.pixelSize());

    coordDispWindow->setText(bbox);
}

void
InfoViewerWidget::setImageFormat(Natron::ImageComponentsEnum comp,
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

void
InfoViewerWidget::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
}

