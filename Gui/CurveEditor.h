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

#ifndef CURVEEDITOR_H
#define CURVEEDITOR_H

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>

class CurveEditor : public QGLWidget
{
public:
    CurveEditor();

    virtual ~CurveEditor();
   
    virtual void initializeGL();
    
    virtual void resizeGL(int width,int height);
    
    virtual void paintGL();
    
    virtual void mousePressEvent(QMouseEvent *event);
    
    virtual void mouseReleaseEvent(QMouseEvent *event);
    
    virtual void mouseMoveEvent(QMouseEvent *event);
    
    virtual void wheelEvent(QWheelEvent *event);
    
    
};

#endif // CURVEEDITOR_H
