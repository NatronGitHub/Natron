//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. *//*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "InfoViewerWidget.h"

#include <cstdlib>
#include <iostream>
#include <QtGui/QImage>
#include <QHBoxLayout>
#include <QLabel>
#include <QThread>
#include <QCoreApplication>

#include "Engine/ViewerInstance.h"
#include "Engine/Lut.h"
#include "Engine/Image.h"
#include "Gui/ViewerGL.h"

using std::cout; using std::endl;
using namespace Natron;

InfoViewerWidget::InfoViewerWidget(ViewerGL* v,
                                   const QString& description,
                                   QWidget* parent)
: QWidget(parent)
, _comp(ImageComponentNone)
{
    this->viewer = v;
    setObjectName(QString::fromUtf8("infoViewer"));
    setMinimumHeight(20);
    setMaximumHeight(20);
    setStyleSheet(QString("background-color:black"));

    layout = new QHBoxLayout(this);
    
    descriptionLabel = new QLabel(description,this);
    
    imageFormat = new QLabel(this);

    resolution = new QLabel(this);
    resolution->setContentsMargins(0, 0, 0, 0);

    
    _fpsLabel = new QLabel(this);
    
    coordDispWindow = new QLabel(this);
    coordDispWindow->setContentsMargins(0, 0, 0, 0);
    
    coordMouse = new QLabel(this);
    //coordMouse->setText(coord);
    coordMouse->setContentsMargins(0, 0, 0, 0);
    
    rgbaValues = new QLabel(this);
    rgbaValues->setContentsMargins(0, 0, 0, 0);

    color = new QLabel(this);
    color->setMaximumSize(20, 20);
    color->setContentsMargins(0, 0, 0, 0);
    color->setStyleSheet(QString("background-color:black;"));
    
    hvl_lastOption = new QLabel(this);
    hvl_lastOption->setContentsMargins(10, 0, 0, 0);

    layout->addWidget(descriptionLabel);
    layout->addWidget(imageFormat);
    layout->addWidget(resolution);
    layout->addWidget(coordDispWindow);
    layout->addWidget(_fpsLabel);
    layout->addWidget(coordMouse);
    layout->addWidget(rgbaValues);
    layout->addWidget(color);
    layout->addWidget(hvl_lastOption);
    
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);
    
}

QSize
InfoViewerWidget::sizeHint() const
{
    return QSize(0,0);
}

void
InfoViewerWidget::setFps(double actualFps,
                         double desiredFps)
{
    QString colorStr("green");
    if (actualFps < (desiredFps -  desiredFps / 10.f) && actualFps > (desiredFps / 2.f)) {
        colorStr = QString("orange");
    } else if(actualFps < (desiredFps / 2.f)) {
        colorStr = QString("red");
    }
    QString str = QString("<font color='"+colorStr+"'>%1 fps</font>").arg(QString::number(actualFps,'f',1));
    _fpsLabel->setText(str);
    if(!_fpsLabel->isVisible()){
        _fpsLabel->show();
    }
}

void
InfoViewerWidget::hideFps()
{
    if (_fpsLabel->isVisible()) {
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
    //values = QString("<font color='red'>%1</font> <font color='green'>%2</font> <font color='blue'>%3</font> <font color=\"#DBE0E0\">%4</font>")
    // the following three colors have an equal luminance (=0.4), which makes the text easier to read.
    switch (_comp) {
        case Natron::ImageComponentNone:
            values = QString();
            break;
        case Natron::ImageComponentAlpha:
            values = (QString("<font color=\"#DBE0E0\">%4</font>")
                      .arg(a,0,'f',5));
            break;
        case Natron::ImageComponentRGB:
            values = (QString("<font color='#d93232'>%1</font> <font color='#00a700'>%2</font> <font color='#5858ff'>%3</font>")
                      .arg(r,0,'f',5)
                      .arg(g,0,'f',5)
                      .arg(b,0,'f',5));
            break;
        case Natron::ImageComponentRGBA:
            values = (QString("<font color='#d93232'>%1</font> <font color='#00a700'>%2</font> <font color='#5858ff'>%3</font> <font color=\"#DBE0E0\">%4</font>")
                      .arg(r,0,'f',5)
                      .arg(g,0,'f',5)
                      .arg(b,0,'f',5)
                      .arg(a,0,'f',5));
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
    col.setRgbF(Natron::clamp(srgb_r), Natron::clamp(srgb_g), Natron::clamp(srgb_b));
    QPixmap pix(15,15); 
    pix.fill(col);
    color->setPixmap(pix);
    color->repaint();

    
    Color::rgb_to_hsv(srgb_r,srgb_g,srgb_b,&h,&s,&v);
    l = 0.2125*r + 0.7154*g + 0.0721*b; // L according to Rec.709
    QString hsvlValues;
    hsvlValues = QString("<font color=\"#DBE0E0\">H:%1 S:%2 V:%3  L:%4</font>")
        .arg(h,0,'f',0)
        .arg(s,0,'f',2)
        .arg(v,0,'f',2)
        .arg(l,0,'f',5);

    hvl_lastOption->setText(hsvlValues);
    hvl_lastOption->repaint();
}

void
InfoViewerWidget::setMousePos(QPoint p)
{
    QString coord;
    coord = QString("<font color=\"#DBE0E0\">x=%1 y=%2</font>")
    .arg(p.x())
    .arg(p.y());
    coordMouse->setText(coord);
}

void
InfoViewerWidget::setResolution(const Format& f)
{
    assert(QThread::currentThread() == qApp->thread());
    format = f;
    if(format.getName() == std::string("")){
        QString reso;
        reso = QString("<font color=\"#DBE0E0\">%1x%2\t</font>")
        .arg(format.width())
        .arg(format.height());
        resolution->setText(reso);
        resolution->setMaximumWidth(resolution->sizeHint().width());
    }else{
        QString reso("<font color=\"#DBE0E0\">");
        reso.append(format.getName().c_str());
        reso.append("\t");
        reso.append("</font>");
        resolution->setText(reso);
        resolution->setMaximumWidth(resolution->sizeHint().width());
        
    }
}


void
InfoViewerWidget::setDataWindow(const RectD& r)
{
    QString bbox;
    bbox = QString("<font color=\"#DBE0E0\">RoD: %1 %2 %3 %4</font>")
    .arg(r.left())
    .arg(r.bottom())
    .arg(r.right())
    .arg(r.top());
    
    coordDispWindow->setText(bbox);
}


void
InfoViewerWidget::setImageFormat(Natron::ImageComponents comp,
                                 Natron::ImageBitDepth depth)
{
    std::string format = Natron::Image::getFormatString(comp, depth);
    imageFormat->setText(format.c_str());
    _comp = comp;
}
