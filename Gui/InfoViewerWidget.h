//
//  InfoViewerWidget.h
//  PowiterOsX
//
//  Created by Alexandre on 2/14/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#ifndef __PowiterOsX__InfoViewerWidget__
#define __PowiterOsX__InfoViewerWidget__

#include <iostream>
#include <QtGui/QWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtCore/qpoint.h>
#include <QtGui/qvector4d.h>
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
    void changeDisplayWindow();
    void changeUserRect();
    void hideColorAndMouseInfo();
    void showColorAndMouseInfo();
    
    
private:
    bool _colorAndMouseVisible;
    
    QHBoxLayout* layout;
    QLabel* resolution;
    DisplayFormat format;
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
    ViewerGL* viewer;
    
    
};

#endif /* defined(__PowiterOsX__InfoViewerWidget__) */
