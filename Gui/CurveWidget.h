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

#ifndef CURVE_WIDGET_H
#define CURVE_WIDGET_H

#include "Gui/ViewerGL.h"

class CurvePath;


class CurveGui {

public:

    CurveGui(boost::shared_ptr<CurvePath> curve,
             const QString& name,
             const QColor& color,
             int thickness = 1)
        : _internalCurve(curve)
        , _name(name)
        , _color(color)
        , _thickness(thickness)
        , _visible(true)
        , _selected(false)
    {

    }


    const QString& getName() const { return _name; }

    void setName(const QString& name) { _name = name; }

    const QColor& getColor() const { return _color; }

    void setColor(const QColor& color) { _color = color; }

    int getThickness() const { return _thickness; }

    void setThickness(int t) { _thickness = t;}

    bool isVisible() const { return _visible; }

    void setVisible(bool visible) { _visible = visible; }

    bool isSelected() const { return _selected; }

    void setSelected(bool s) const { _selected = s; }

    /**
      * @brief Evaluates the curve and returns the y position corresponding to the given x.
      * The coordinates are those of the curve, not of the widget.
    **/
    double evaluate(double x) const;


private:

    boost::shared_ptr<CurvePath> _internalCurve; ///ptr to the internal curve
    QString _name; /// the name of the curve
    QColor _color; /// the color that must be used to draw the curve
    int _thickness; /// its thickness
    bool _visible; /// should we draw this curve ?
    mutable bool _selected; /// is this curve selected

};

class QMenu;
class CurveWidget : public QGLWidget
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
    QColor _scaleColor;
    QColor _selectedCurveColor;
    Natron::TextRenderer _textRenderer;
    QFont* _font;

    typedef std::list<boost::shared_ptr<CurveGui> > Curves;
    Curves _curves;
public:
    
    CurveWidget(QWidget* parent, const QGLWidget* shareWidget = NULL);

    virtual ~CurveWidget();
   
    virtual void initializeGL();
    
    virtual void resizeGL(int width,int height);
    
    virtual void paintGL();
    
    virtual void mousePressEvent(QMouseEvent *event);
    
    virtual void mouseReleaseEvent(QMouseEvent *event);
    
    virtual void mouseMoveEvent(QMouseEvent *event);
    
    virtual void wheelEvent(QWheelEvent *event);
    
    virtual QSize sizeHint() const;
    
    void renderText(double x,double y,const QString& text,const QColor& color,const QFont& font);

    void centerOn(double xmin,double xmax,double ymin,double ymax);

    void addCurve(boost::shared_ptr<CurveGui> curve);

    void removeCurve(boost::shared_ptr<CurveGui> curve);



private:

    /**
     * @brief Returns an iterator pointing to a curve if a curve was nearby the point 'pt' which is
     * widget coordinates.
    **/
    Curves::const_iterator isNearbyCurve(const QPoint& pt) const;

    /**
     * @brief Selects the curve given in parameter and deselects any other curve in the widget.
    **/
    void selectCurve(boost::shared_ptr<CurveGui> curve);

    /**
     *@brief See toImgCoordinates_fast in ViewerGL.h
     **/
    QPointF toImgCoordinates_fast(int x,int y) const;

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPoint toWidgetCoordinates(double x, double y) const;
    

    void drawBaseAxis();
    
    void drawScale();
    
    void drawCurves();
};



#endif // CURVE_WIDGET_H
