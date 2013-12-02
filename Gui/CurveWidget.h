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

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>
#include <boost/shared_ptr.hpp>
#include "Gui/TextRenderer.h"

class Curve;
class KeyFrame;
class CurveWidget;
class CurveGui : public QObject {
    
    Q_OBJECT

public:
    

    CurveGui(const CurveWidget *curveWidget,boost::shared_ptr<Curve>  curve,
             const QString& name,
             const QColor& color,
             int thickness = 1);

    virtual ~CurveGui();
    
    const QString& getName() const { return _name; }

    void setName(const QString& name) { _name = name; }

    const QColor& getColor() const { return _color; }

    void setColor(const QColor& color) { _color = color; }

    int getThickness() const { return _thickness; }

    void setThickness(int t) { _thickness = t;}

    bool isVisible() const { return _visible; }

    void setVisibleAndRefresh(bool visible) ;

    /**
      * @brief same as setVisibleAndRefresh() but doesn't emit any signal for a repaint
    **/
    void setVisible(bool visible);

    bool isSelected() const { return _selected; }

    void setSelected(bool s) const { _selected = s; }

    /**
      * @brief Evaluates the curve and returns the y position corresponding to the given x.
      * The coordinates are those of the curve, not of the widget.
    **/
    double evaluate(double x) const;

    boost::shared_ptr<Curve>  getInternalCurve() const { return _internalCurve; }

    void drawCurve();

signals:

    void curveChanged();
    
private:

    boost::shared_ptr<Curve> _internalCurve; ///ptr to the internal curve
    QString _name; /// the name of the curve
    QColor _color; /// the color that must be used to draw the curve
    int _thickness; /// its thickness
    bool _visible; /// should we draw this curve ?
    mutable bool _selected; /// is this curve selected
    const CurveWidget* _curveWidget;

};

class QMenu;
class CurveWidget : public QGLWidget
{
    
    enum EventState{
        DRAGGING_VIEW = 0,
        DRAGGING_KEYS = 1,
        NONE = 2
    };
    
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
    EventState _state;
    QMenu* _rightClickMenu;
    QColor _clearColor;
    QColor _baseAxisColor;
    QColor _scaleColor;
    QColor _selectedCurveColor;
    QColor _nextCurveAddedColor;
    Natron::TextRenderer _textRenderer;
    QFont* _font;

    typedef std::list<CurveGui* > Curves;
    Curves _curves;
    
    typedef std::list< std::pair<CurveGui*,KeyFrame*> > SelectedKeys;
    SelectedKeys _selectedKeyFrames;
    bool _hasOpenGLVAOSupport;
    
    bool _mustSetDragOrientation;
    QPoint _mouseDragOrientation; ///used to drag a key frame in only 1 direction (horizontal or vertical)
                                  ///the value is either (1,0) or (0,1)

public:
    
    CurveWidget(QWidget* parent, const QGLWidget* shareWidget = NULL);

    virtual ~CurveWidget();
   
    virtual void initializeGL();
    
    virtual void resizeGL(int width,int height);
    
    virtual void paintGL();
    
    virtual QSize sizeHint() const;
    
    void renderText(double x,double y,const QString& text,const QColor& color,const QFont& font) const;

    void centerOn(double xmin,double xmax,double ymin,double ymax);

    CurveGui *createCurve(boost::shared_ptr<Curve> curve, const QString &name);

    void removeCurve(CurveGui* curve);
    
    void centerOn(const std::vector<CurveGui*>& curves);
    
    void showCurvesAndHideOthers(const std::vector<CurveGui*>& curves);
    
    /**
     *@brief See toImgCoordinates_fast in ViewerGL.h
     **/
    QPointF toScaleCoordinates(int x,int y) const;

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPoint toWidgetCoordinates(double x, double y) const;

    const QColor& getSelectedCurveColor() const { return _selectedCurveColor; }

    const QFont& getFont() const { return *_font; }

    const SelectedKeys& getSelectedKeyFrames() const { return _selectedKeyFrames; }

    bool isSupportingOpenGLVAO() const { return _hasOpenGLVAOSupport; }


protected:
    virtual void mousePressEvent(QMouseEvent *event);

    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);

    virtual void wheelEvent(QWheelEvent *event);

    virtual void keyPressEvent(QKeyEvent *event);
private:

    /**
     * @brief Returns an iterator pointing to a curve if a curve lies nearby the point 'pt' which is
     * widget coordinates.
    **/
    Curves::const_iterator isNearbyCurve(const QPoint& pt) const;
    
    /**
     * @brief Returns a pointer to a keyframe if a keyframe lies nearby the point 'pt' which is
     * widget coordinates.
     **/
    std::pair<CurveGui*,KeyFrame*> isNearbyKeyFrame(const QPoint& pt) const;

    /**
     * @brief Selects the curve given in parameter and deselects any other curve in the widget.
    **/
    void selectCurve(CurveGui *curve);

    void drawBaseAxis();
    
    void drawScale();
    
    void drawCurves();
};



#endif // CURVE_WIDGET_H
