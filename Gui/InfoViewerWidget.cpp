//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. *//*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#include <cstdlib>
#include <QtGui/QImage>
#include "Gui/InfoViewerWidget.h"
#include "Gui/GLViewer.h"
#include "Core/Box.h"
using namespace std;
InfoViewerWidget::InfoViewerWidget(ViewerGL* v,QWidget* parent) : QWidget(parent),_colorAndMouseVisible(false),
mousePos(0,0),rectUser(0,0),colorUnderMouse(0,0,0,0),_fps(0){
    
    this->viewer = v;
    setObjectName(QString::fromUtf8("infoViewer"));
    setMinimumHeight(20);
    setMaximumHeight(20);
    setStyleSheet(QString("background-color:black"));

    layout = new QHBoxLayout(this);
    QString reso("<font color=\"#DBE0E0\">");
    char tmp[10];
    reso.append(viewer->displayWindow().name().c_str());
    reso.append("\t");
    reso.append("</font>");
    resolution = new QLabel(reso,this);
    resolution->setContentsMargins(0, 0, 0, 0);
    resolution->setMaximumWidth(resolution->sizeHint().width());

    
    QString bbox("<font color=\"#DBE0E0\">bbox: ");
    sprintf(tmp, "%i",viewer->dataWindow().x());
    bbox.append(tmp);
    bbox.append(" ");
    sprintf(tmp, "%i",viewer->dataWindow().y());
    bbox.append(tmp);
    bbox.append(" ");
    sprintf(tmp, "%i",viewer->dataWindow().right());
    bbox.append(tmp);
    bbox.append(" ");
    sprintf(tmp, "%i",viewer->dataWindow().top());
    bbox.append(tmp);
    bbox.append("</font>");
    
    _fpsLabel = new QLabel(this);
    
    coordDispWindow = new QLabel(bbox,this);
    coordDispWindow->setContentsMargins(0, 0, 0, 0);
    
    QString coord("<font color=\"#DBE0E0\">x= ");
    sprintf(tmp, "%i",mousePos.x());
    coord.append(tmp);
    coord.append(" y= ");
    sprintf(tmp, "%i",mousePos.y());
    coord.append(tmp);
    coord.append("</font>");
    
    coordMouse = new QLabel(this);
    //coordMouse->setText(coord);
    coordMouse->setContentsMargins(0, 0, 0, 0);
    
    rgbaValues = new QLabel(this);
    rgbaValues->setContentsMargins(0, 0, 0, 0);
    
    QString values;
    QString red;
    sprintf(tmp, "%.2f",colorUnderMouse.x());
    red.append(tmp);
    values.append("<font color='red'>");
    values.append(red);
    values.append("</font> ");
    QString green;
    sprintf(tmp, "%.2f",colorUnderMouse.y());
    green.append(tmp);
    values.append("<font color='green'>");
    values.append(green);
    values.append("</font> ");
    QString blue;
    sprintf(tmp, "%.2f",colorUnderMouse.z());
    blue.append(tmp);
    values.append("<font color='blue'>");
    values.append(blue);
    values.append("</font> ");
    QString alpha;
    sprintf(tmp, "%.2f",colorUnderMouse.z());
    alpha.append(tmp);
    values.append("<font color=\"#DBE0E0\">");
    values.append(alpha);
    values.append("</font>");
    
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
    
    QString hsvlValues("<font color=\"#DBE0E0\">H:");
    sprintf(tmp, "%i",hsv.hslHue());
    hsvlValues.append(tmp);
    hsvlValues.append(" S:");
    sprintf(tmp, "%.2f",hsv.hslSaturationF());
    hsvlValues.append(tmp);
    hsvlValues.append(" V:");
    sprintf(tmp, "%.2f",hsv.valueF());
    hsvlValues.append(tmp);
    hsvlValues.append("   L:");
    sprintf(tmp, "%.2f",hsl.lightnessF());
    hsvlValues.append(tmp);
    hsvlValues.append("</font>");
    
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
    
    
    char tmp[10];
    QString values;
    QString red;
    sprintf(tmp, "%.2f",colorUnderMouse.x());
    red.append(tmp);
    values.append("<font color='red'>");
    values.append(red);
    values.append("</font> ");
    QString green;
    sprintf(tmp, "%.2f",colorUnderMouse.y());
    green.append(tmp);
    values.append("<font color='green'>");
    values.append(green);
    values.append("</font> ");
    QString blue;
    sprintf(tmp, "%.2f",colorUnderMouse.z());
    blue.append(tmp);
    values.append("<font color='blue'>");
    values.append(blue);
    values.append("</font> ");
    QString alpha;
    sprintf(tmp, "%.2f",colorUnderMouse.w());
    alpha.append(tmp);
    values.append("<font color=\"#DBE0E0\">");
    values.append(alpha);
    values.append("</font>");
    
    rgbaValues->setText(values);
    
    int r = colorUnderMouse.x()*255;
    if(r>255) r = 255;
    else if(r<0) r= 0;
    int g = colorUnderMouse.y()*255;
    if(g>255) g = 255;
    else if(g<0) g= 0;
    int b = colorUnderMouse.z()*255;
    if(b>255) b = 255;
    else if(b<0) b= 0;
    int a = colorUnderMouse.w()*255;
    if(a>255) a = 255;
    else if(a<0) a= 0;
    QColor c(r,g,b,a);
   
    
    QImage img(20,20,QImage::Format_RGB32);
    img.fill(c.rgb());

    QPixmap pix=QPixmap::fromImage(img);
    color->setPixmap(pix);
    
    QColor hsv= c.toHsv();
    QColor hsl= c.toHsl();
    
    QString hsvlValues("<font color=\"#DBE0E0\">    H:");
    sprintf(tmp, "%.2f",hsv.hslHueF());
    hsvlValues.append(tmp);
    hsvlValues.append(" S:");
    sprintf(tmp, "%.2f",hsv.hslSaturationF());
    hsvlValues.append(tmp);
    hsvlValues.append(" V:");
    sprintf(tmp, "%.2f",hsv.valueF());
    hsvlValues.append(tmp);
    hsvlValues.append("   L:");
    sprintf(tmp, "%.2f",hsl.lightnessF());
    hsvlValues.append(tmp);
    hsvlValues.append("</font>");
    hvl_lastOption->setText(hsvlValues);
}
void InfoViewerWidget::updateCoordMouse(){
    char tmp[10];
    QString coord("<font color=\"#DBE0E0\">x= ");
    sprintf(tmp, "%i",mousePos.x());
    coord.append(tmp);
    coord.append(" y= ");
    sprintf(tmp, "%i",mousePos.y());
    coord.append(tmp);
    coord.append("</font>");
    coordMouse->setText(coord);

}
void InfoViewerWidget::changeResolution(){
    format = viewer->displayWindow();
    if(format.name() == string("")){
        QString reso("<font color=\"#DBE0E0\">");
        char tmp[10];
        sprintf(tmp, "%i",viewer->displayWindow().w());
        reso.append(tmp);
        reso.append("x");
        sprintf(tmp, "%i",viewer->displayWindow().h());
        reso.append(tmp);
        reso.append("\t");
        reso.append("</font>");
        resolution->setText(reso);
        resolution->setMaximumWidth(resolution->sizeHint().width());
    }else{
        QString reso("<font color=\"#DBE0E0\">");
        reso.append(format.name().c_str());
        reso.append("\t");
        reso.append("</font>");
        resolution->setText(reso);
        resolution->setMaximumWidth(resolution->sizeHint().width());

    }
}
void InfoViewerWidget::changeDataWindow(){
    char tmp[10];
    QString bbox("<font color=\"#DBE0E0\">bbox: ");
    sprintf(tmp, "%i",viewer->dataWindow().x());
    bbox.append(tmp);
    bbox.append(" ");
    sprintf(tmp, "%i",viewer->dataWindow().y());
    bbox.append(tmp);
    bbox.append(" ");
    sprintf(tmp, "%i",viewer->dataWindow().w());
    bbox.append(tmp);
    bbox.append(" ");
    sprintf(tmp, "%i",viewer->dataWindow().h());
    bbox.append(tmp);
    bbox.append("</font>");

    coordDispWindow->setText(bbox);
}
void InfoViewerWidget::changeUserRect(){
    cout << "NOT IMPLEMENTED YET" << endl;
}


