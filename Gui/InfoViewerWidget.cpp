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

#include "Gui/ViewerGL.h"
#include "Engine/RectI.h"

using std::cout; using std::endl;

InfoViewerWidget::InfoViewerWidget(ViewerGL* v,QWidget* parent) : QWidget(parent),_colorAndMouseVisible(false),
mousePos(0,0),rectUser(0,0),colorUnderMouse(0,0,0,0),_fps(0){
    
    this->viewer = v;
    setObjectName(QString::fromUtf8("infoViewer"));
    setMinimumHeight(20);
    setMaximumHeight(20);
    setStyleSheet(QString("background-color:black"));

    layout = new QHBoxLayout(this);
    QString reso("<font color=\"#DBE0E0\">");
    //char tmp[10];
    reso.append(viewer->getDisplayWindow().getName().c_str());
    reso.append("\t");
    reso.append("</font>");
    resolution = new QLabel(reso,this);
    resolution->setContentsMargins(0, 0, 0, 0);
    resolution->setMaximumWidth(resolution->sizeHint().width());

    
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
    
    QString values;
    values = QString("<font color='red'>%1</font> <font color='green'>%2</font> <font color='blue'>%3</font> <font color=\"#DBE0E0\">%4</font>")
        .arg(colorUnderMouse.x(),0,'f',2)
        .arg(colorUnderMouse.y(),0,'f',2)
        .arg(colorUnderMouse.z(),0,'f',2)
        .arg(colorUnderMouse.w(),0,'f',2);
    
   // rgbaValues->setText(values);
    
    QColor c(0,0,0,0);
    color = new QLabel(this);
    color->setMaximumSize(20, 20);
    color->setContentsMargins(0, 0, 0, 0);
    color->setStyleSheet(QString("background-color:black;"));
   // QPixmap pix(15, 15);
   // pix.fill(c);
   // color->setPixmap(pix);
    
    QColor hsv= c.toHsv();
    QColor hsl= c.toHsl();
    
    QString hsvlValues;
    hsvlValues = QString("<font color=\"#DBE0E0\">H:%1 S:%2 V:%3  L:%4</font>")
        .arg(hsv.hslHue())
        .arg(hsv.hslSaturationF(),0,'f',2)
        .arg(hsv.valueF(),0,'f',2)
        .arg(hsl.lightnessF(),0,'f',2);
    
    hvl_lastOption = new QLabel(this);
  //  hvl_lastOption->setText(hsvlValues);
    hvl_lastOption->setContentsMargins(10, 0, 0, 0);
    
    
    
    layout->addWidget(resolution);
    layout->addWidget(coordDispWindow);
    layout->addWidget(_fpsLabel);
    layout->addWidget(coordMouse);
    layout->addWidget(rgbaValues);
    layout->addWidget(color);
    layout->addWidget(hvl_lastOption);
    
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);
    
}

void InfoViewerWidget::setFps(double v){
    QString str = QString::number(v,'f',1);
    str.append(" fps");
    _fpsLabel->setText(str);
}
void InfoViewerWidget::showColorAndMouseInfo(){

    _colorAndMouseVisible=true;
}
void InfoViewerWidget::hideColorAndMouseInfo(){

    coordMouse->setText("");
    hvl_lastOption->setText("");
    rgbaValues->setText("");
    QPixmap pix(20,20);
    pix.fill(Qt::black);
    color->setPixmap(pix);
    
    _colorAndMouseVisible=false;

}

InfoViewerWidget::~InfoViewerWidget(){
    
}


void InfoViewerWidget::updateColor(){
    QString values;
    values = QString("<font color='red'>%1</font> <font color='green'>%2</font> <font color='blue'>%3</font> <font color=\"#DBE0E0\">%4</font>")
    .arg(colorUnderMouse.x(),0,'f',2)
    .arg(colorUnderMouse.y(),0,'f',2)
    .arg(colorUnderMouse.z(),0,'f',2)
    .arg(colorUnderMouse.w(),0,'f',2);

    rgbaValues->setText(values);
    
    int r = std::max(std::min((int)(colorUnderMouse.x()*255.),255),0);
    int g = std::max(std::min((int)(colorUnderMouse.y()*255.),255),0);
    int b = std::max(std::min((int)(colorUnderMouse.z()*255.),255),0);
    int a = std::max(std::min((int)(colorUnderMouse.w()*255.),255),0);
    QColor c(r,g,b,a);
    QImage img(20,20,QImage::Format_RGB32);
    img.fill(c.rgb());

    QPixmap pix=QPixmap::fromImage(img);
    color->setPixmap(pix);

    QColor hsv= c.toHsv();
    QColor hsl= c.toHsl();

    QString hsvlValues;
    hsvlValues = QString("<font color=\"#DBE0E0\">H:%1 S:%2 V:%3  L:%4</font>")
    .arg(hsv.hslHue())
    .arg(hsv.hslSaturationF(),0,'f',2)
    .arg(hsv.valueF(),0,'f',2)
    .arg(hsl.lightnessF(),0,'f',2);
    hvl_lastOption->setText(hsvlValues);
}

void InfoViewerWidget::updateCoordMouse(){
    QString coord;
    coord = QString("<font color=\"#DBE0E0\">x=%1 y=%2</font>")
    .arg(mousePos.x())
    .arg(mousePos.y());
    coordMouse->setText(coord);
}
void InfoViewerWidget::changeResolution(){
    format = viewer->getDisplayWindow();
    if(format.getName() == std::string("")){
        QString reso;
        reso = QString("<font color=\"#DBE0E0\">%1x%2\t</font>")
        .arg(viewer->getDisplayWindow().width())
        .arg(viewer->getDisplayWindow().height());
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
void InfoViewerWidget::changeDataWindow(){
    QString bbox;
    bbox = QString("<font color=\"#DBE0E0\">RoD: %1 %2 %3 %4</font>")
    .arg(viewer->getRoD().left())
    .arg(viewer->getRoD().bottom())
    .arg(viewer->getRoD().right())
    .arg(viewer->getRoD().top());

    coordDispWindow->setText(bbox);
}
void InfoViewerWidget::changeUserRect(){
    cout << "NOT IMPLEMENTED YET" << endl;
}


