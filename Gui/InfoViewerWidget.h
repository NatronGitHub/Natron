//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GUI_INFOVIEWERWIDGET_H_
#define NATRON_GUI_INFOVIEWERWIDGET_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
#include <QtCore/QPoint>
#include <QtGui/QVector4D>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Format.h"

class ViewerGL;
class QLabel;
class QHBoxLayout;

class InfoViewerWidget: public QWidget{
    Q_OBJECT
    
public:
    explicit InfoViewerWidget(ViewerGL* v,QWidget* parent=0);
    virtual ~InfoViewerWidget() OVERRIDE;
    void setColor(float r,float g,float b,float a);
    void setMousePos(QPoint p){mousePos =p;}
    void setUserRect(QPoint p){rectUser=p;}
    bool colorAndMouseVisible(){return _colorAndMouseVisible;}
    void colorAndMouseVisible(bool b){_colorAndMouseVisible=b;}
    
    void setResolution(const Format& f);
    
public slots:
    void updateColor();
    void updateCoordMouse();
    void changeResolution();
    void changeDataWindow();
    void changeUserRect();
    void hideColorAndMouseInfo();
    void showColorAndMouseInfo();
    void setFps(double actualFps,double desiredFps);
    void hideFps();
    
private:
    virtual QSize sizeHint() const OVERRIDE FINAL;
    
    
    
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

#endif /* defined(NATRON_GUI_INFOVIEWERWIDGET_H_) */
