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

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "Global/GlobalDefines.h"



class Curve;
class Variant;
class KeyFrame;
class TimeLine;
class CurveWidget;
class CurveGui : public QObject {
    
    Q_OBJECT

public:
    
    enum SelectedTangent{LEFT_TANGENT = 0,RIGHT_TANGENT = 1};
    
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

    boost::shared_ptr<KeyFrame> nextPointForSegment(double x1,double* x2);

    boost::shared_ptr<Curve> _internalCurve; ///ptr to the internal curve
    QString _name; /// the name of the curve
    QColor _color; /// the color that must be used to draw the curve
    int _thickness; /// its thickness
    bool _visible; /// should we draw this curve ?
    mutable bool _selected; /// is this curve selected
    const CurveWidget* _curveWidget;

};

struct SelectedKey;
typedef std::list< boost::shared_ptr<SelectedKey> > SelectedKeys;

class QMenu;
class CurveWidgetPrivate;

class CurveWidget : public QGLWidget
{
    friend class CurveGui;
    friend class CurveWidgetPrivate;

    Q_OBJECT

public:
    
    /*Pass a null timeline ptr if you don't want interaction with the global timeline. */
    CurveWidget(boost::shared_ptr<TimeLine> timeline = boost::shared_ptr<TimeLine>() ,QWidget* parent = NULL, const QGLWidget* shareWidget = NULL);

    virtual ~CurveWidget();

    void centerOn(double xmin,double xmax,double ymin,double ymax);

    CurveGui *createCurve(boost::shared_ptr<Curve> curve, const QString &name);

    void removeCurve(CurveGui* curve);

    void centerOn(const std::vector<CurveGui*>& curves);

    void showCurvesAndHideOthers(const std::vector<CurveGui*>& curves);

    void addKeyFrame(CurveGui* curve, boost::shared_ptr<KeyFrame> key);

    boost::shared_ptr<KeyFrame> addKeyFrame(CurveGui* curve,const Variant& y, int x);

    void removeKeyFrame(CurveGui* curve,boost::shared_ptr<KeyFrame> key);

    void setKeyPos(boost::shared_ptr<KeyFrame> key,double x,const Variant& y);
    
    void setKeyInterpolation(boost::shared_ptr<KeyFrame> key,Natron::KeyframeType interpolation);

    void refreshDisplayedTangents(); //< need to be called if you call setKeyPos() externally
    
public slots:

    void deleteSelectedKeyFrames();

    void copySelectedKeyFrames();

    void pasteKeyFramesFromClipBoardToSelectedCurve();

    void selectAllKeyFrames();

    void constantInterpForSelectedKeyFrames();

    void linearInterpForSelectedKeyFrames();

    void smoothForSelectedKeyFrames();

    void catmullromInterpForSelectedKeyFrames();

    void cubicInterpForSelectedKeyFrames();

    void horizontalInterpForSelectedKeyFrames();

    void breakTangentsForSelectedKeyFrames();

    void frameSelectedCurve();

    void refreshSelectedKeysBbox();

    void onTimeLineFrameChanged(SequenceTime time, int reason);

    void onTimeLineBoundariesChanged(SequenceTime left, SequenceTime right);

private:
    
    virtual void initializeGL() OVERRIDE FINAL;

    virtual void resizeGL(int width,int height) OVERRIDE FINAL;

    virtual void paintGL() OVERRIDE FINAL;

    virtual QSize sizeHint() const OVERRIDE FINAL;

    virtual void mousePressEvent(QMouseEvent *event) OVERRIDE FINAL;

    virtual void mouseReleaseEvent(QMouseEvent *event) OVERRIDE FINAL;

    virtual void mouseMoveEvent(QMouseEvent *event) OVERRIDE FINAL;

    virtual void wheelEvent(QWheelEvent *event) OVERRIDE FINAL;

    virtual void keyPressEvent(QKeyEvent *event) OVERRIDE FINAL;

    virtual void enterEvent(QEvent *event) OVERRIDE FINAL;

    void renderText(double x,double y,const QString& text,const QColor& color,const QFont& font) const;

    /**
     *@brief See toImgCoordinates_fast in ViewerGL.h
     **/
    QPointF toScaleCoordinates(double x, double y) const;

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPointF toWidgetCoordinates(double x, double y) const;

    const QColor& getSelectedCurveColor() const ;

    const QFont& getFont() const;

    const SelectedKeys& getSelectedKeyFrames() const ;

    bool isSupportingOpenGLVAO() const ;

    std::pair<SelectedKeys::const_iterator,bool> isKeySelected(boost::shared_ptr<KeyFrame> key) const;


private:

    boost::scoped_ptr<CurveWidgetPrivate> _imp;
        
};


#endif // CURVE_WIDGET_H
