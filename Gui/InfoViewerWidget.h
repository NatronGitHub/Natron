//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef __PowiterOsX__InfoViewerWidget__
#define __PowiterOsX__InfoViewerWidget__

#include <iostream>
#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtCore/QPoint>
#include <QtGui/QVector4D>
#include "Core/displayFormat.h"
class ViewerGL;
class InfoViewerWidget: public QWidget{
    Q_OBJECT
    
public:
    InfoViewerWidget(ViewerGL* v,QWidget* parent=0);
    virtual ~InfoViewerWidget();
    void setColor(QVector4D v){colorUnderMouse = v;}
    void setMousePos(QPoint p){mousePos =p;}
    void setUserRect(QPoint p){rectUser=p;}
    bool colorAndMouseVisible(){return _colorAndMouseVisible;}
    void colorAndMouseVisible(bool b){_colorAndMouseVisible=b;}
    
public slots:
    void updateColor();
    void updateCoordMouse();
    void changeResolution();
    void changeDataWindow();
    void changeUserRect();
    void hideColorAndMouseInfo();
    void showColorAndMouseInfo();
    void setFps(double v);

    
private:
    bool _colorAndMouseVisible;
    
    QHBoxLayout* layout;
    QLabel* resolution;
    Format format;
    QLabel* coordDispWindow;
    QLabel* coordMouse;
    QPoint mousePos;
    QPoint rectUser;
    QLabel* rectUserLabel;
    QVector4D colorUnderMouse;
    QLabel* rgbaValues;
    QLabel* color;
    QLabel* hvl_lastOption;
    bool floatingPoint;
    QLabel* _fpsLabel;
    float _fps;
    ViewerGL* viewer;
    
    
};

#endif /* defined(__PowiterOsX__InfoViewerWidget__) */
