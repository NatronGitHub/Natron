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
#include "Gui/ViewerGL.h"

using std::cout; using std::endl;
using namespace Natron;

InfoViewerWidget::InfoViewerWidget(ViewerGL* v,QWidget* parent)
: QWidget(parent)
, mousePos(0,0)
, rectUser(0,0)
, colorUnderMouse(0,0,0,0)
, _fps(0)
{
    this->viewer = v;
    setObjectName(QString::fromUtf8("infoViewer"));
    setMinimumHeight(20);
    setMaximumHeight(20);
    setStyleSheet(QString("background-color:black"));

    layout = new QHBoxLayout(this);
    QString reso("<font color=\"#DBE0E0\">");
    reso.append(viewer->getDisplayWindow().getName().c_str());
    reso.append("\t");
    reso.append("</font>");
    resolution = new QLabel(reso,this);
    resolution->setContentsMargins(0, 0, 0, 0);

    
    QString bbox;
    bbox = QString("<font color=\"#DBE0E0\">RoD: %1 %2 %3 %4</font>")
        .arg(viewer->getRoD().left())
        .arg(viewer->getRoD().bottom())
        .arg(viewer->getRoD().right())
        .arg(viewer->getRoD().top());
    
    _fpsLabel = new QLabel(this);
    
    coordDispWindow = new QLabel(bbox,this);
    coordDispWindow->setContentsMargins(0, 0, 0, 0);
    
    QString coord;
    coord = QString("<font color=\"#DBE0E0\">x=%1 y=%2</font>")
        .arg(mousePos.x())
        .arg(mousePos.y());
    
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

QSize InfoViewerWidget::sizeHint() const { return QSize(0,0); }

void InfoViewerWidget::setFps(double actualFps,double desiredFps){
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

void InfoViewerWidget::hideFps(){
    _fpsLabel->hide();
}

bool InfoViewerWidget::colorAndMouseVisible()
{
    return coordMouse->isVisible();
}

void InfoViewerWidget::showColorAndMouseInfo(){
    coordMouse->show();
    hvl_lastOption->show();
    rgbaValues->show();
    color->show();
}
void InfoViewerWidget::hideColorAndMouseInfo(){

    coordMouse->hide();
    hvl_lastOption->hide();
    rgbaValues->hide();
    color->hide();
}

InfoViewerWidget::~InfoViewerWidget(){
    
}

static double clamp(double v)
{
    return std::max(std::min(1.,v), 0.);
}

void InfoViewerWidget::updateColor()
{
    float r = colorUnderMouse.x();
    float g = colorUnderMouse.y();
    float b = colorUnderMouse.z();
    float a = colorUnderMouse.w();

    
    QString values;
    //values = QString("<font color='red'>%1</font> <font color='green'>%2</font> <font color='blue'>%3</font> <font color=\"#DBE0E0\">%4</font>")
    // the following three colors have an equal luminance (=0.4), which makes the text easier to read.
    values = QString("<font color='#d93232'>%1</font> <font color='#00a700'>%2</font> <font color='#5858ff'>%3</font> <font color=\"#DBE0E0\">%4</font>")
        .arg(r,0,'f',5)
        .arg(g,0,'f',5)
        .arg(b,0,'f',5)
        .arg(a,0,'f',5);

    rgbaValues->setText(values);
    rgbaValues->repaint();
    float h,s,v,l;
    // Nuke's HSV display is based on sRGB, an L is Rec.709.
    // see http://forums.thefoundry.co.uk/phpBB2/viewtopic.php?t=2283
    
    double srgb_r = Color::to_func_srgb(r);
    double srgb_g = Color::to_func_srgb(g);
    double srgb_b = Color::to_func_srgb(b);
    
    QColor col;
    col.setRgbF(clamp(srgb_r), clamp(srgb_g), clamp(srgb_b));
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

void InfoViewerWidget::updateCoordMouse(){
    QString coord;
    coord = QString("<font color=\"#DBE0E0\">x=%1 y=%2</font>")
    .arg(mousePos.x())
    .arg(mousePos.y());
    coordMouse->setText(coord);
    //coordMouse->repaint();
}

void InfoViewerWidget::setResolution(const Format& f)
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

void InfoViewerWidget::changeResolution()
{
    Format f = viewer->getDisplayWindow();
    unsigned int mmLvl = viewer->getInternalNode()->getMipMapLevel();
    if (mmLvl != 0) {
        f = f.downscalePowerOfTwoLargestEnclosed(mmLvl);
    }
    setResolution(f);
}

void InfoViewerWidget::changeDataWindow()
{
    QString bbox;
    RectI rod = viewer->getRoD();
    bbox = QString("<font color=\"#DBE0E0\">RoD: %1 %2 %3 %4</font>")
    .arg(rod.left())
    .arg(rod.bottom())
    .arg(rod.right())
    .arg(rod.top());

    coordDispWindow->setText(bbox);
}
void InfoViewerWidget::changeUserRect(){
    cout << "NOT IMPLEMENTED YET" << endl;
}
void InfoViewerWidget::setColor(float r,float g,float b,float a){
    colorUnderMouse.setX(r);
    colorUnderMouse.setY(g);
    colorUnderMouse.setZ(b);
    colorUnderMouse.setW(a);
}


