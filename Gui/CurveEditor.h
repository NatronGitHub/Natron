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


#include "Gui/ViewerGL.h"


class QMenu;
class CurveEditor : public QGLWidget
{
    
    class ZoomContext{
        
    public:
        
        ZoomContext():
        _bottom(0.)
        ,_left(0.)
        ,_zoomFactor(1.)
        {}
        
        QPoint _oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
        double _bottom; /// the bottom edge of orthographic projection
        double _left; /// the left edge of the orthographic projection
        double _zoomFactor; /// the zoom factor applied to the current image
        
        double _lastOrthoLeft,_lastOrthoBottom,_lastOrthoRight,_lastOrthoTop; //< remembers the last values passed to the glOrtho call
        
        /*!< the level of zoom used to display the frame*/
        void setZoomFactor(double f){assert(f>0.); _zoomFactor = f;}
        
        double getZoomFactor() const {return _zoomFactor;}
    };
    
    ZoomContext _zoomCtx;
    bool _dragging; /// true when the user is dragging (panning)
    QMenu* _rightClickMenu;
    QColor _clearColor;
    QColor _baseAxisColor;
    QColor _majorAxisColor;
    QColor _minorAxisColor;
    QColor _scaleColor;
    Natron::TextRenderer _textRenderer;
    QFont* _font;
    
public:
    
    CurveEditor(QWidget* parent, const QGLWidget* shareWidget = NULL);

    virtual ~CurveEditor();
   
    virtual void initializeGL();
    
    virtual void resizeGL(int width,int height);
    
    virtual void paintGL();
    
    virtual void mousePressEvent(QMouseEvent *event);
    
    virtual void mouseReleaseEvent(QMouseEvent *event);
    
    virtual void mouseMoveEvent(QMouseEvent *event);
    
    virtual void wheelEvent(QWheelEvent *event);
    
    virtual QSize sizeHint() const;
    
    void renderText(double x,double y,const QString& text,const QColor& color,const QFont& font);
    
private:
    
    /**
     *@brief See toImgCoordinates_fast in ViewerGL.h
     **/
    QPointF toImgCoordinates_fast(int x,int y);
    
    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPoint toWidgetCoordinates(double x, double y);

    void drawBaseAxis();
    
    void drawScale();
    
};

#endif // CURVEEDITOR_H
