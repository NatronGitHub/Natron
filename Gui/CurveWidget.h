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

#include <set>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>
#include <QMetaType>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "Global/GlobalDefines.h"
#include "Gui/CurveEditorUndoRedo.h"


class Variant;
class TimeLine;
class KnobGui;
class CurveWidget;
class CurveGui : public QObject {
    
    Q_OBJECT

public:
    
    enum SelectedDerivative{LEFT_TANGENT = 0,RIGHT_TANGENT = 1};
    
    CurveGui(const CurveWidget *curveWidget,boost::shared_ptr<Curve>  curve,
             KnobGui* knob,
             int dimension,
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
    
    KnobGui* getKnob() const { return _knob; }
    
    int getDimension() const { return _dimension; }

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

    std::pair<KeyFrame,bool> nextPointForSegment(double x1,double* x2);

    boost::shared_ptr<Curve> _internalCurve; ///ptr to the internal curve
    QString _name; /// the name of the curve
    QColor _color; /// the color that must be used to draw the curve
    int _thickness; /// its thickness
    bool _visible; /// should we draw this curve ?
    mutable bool _selected; /// is this curve selected
    const CurveWidget* _curveWidget;
    KnobGui* _knob; //< ptr to the knob holding this curve
    int _dimension; //< which dimension is this curve representing

};

struct SelectedKey {
    CurveGui* curve;
    KeyFrame key;
    std::pair<double,double> leftTan, rightTan;
    
    SelectedKey() : curve(NULL), key(){}

    SelectedKey(CurveGui* c,const KeyFrame& k)
    : curve(c)
    , key(k)
    {
        
    }
    
    SelectedKey(const SelectedKey& o)
    : curve(o.curve)
    , key(o.key)
    , leftTan(o.leftTan)
    , rightTan(o.rightTan)
    {
        
    }
    
};

struct SelectedKey_compare_time{
    bool operator() (const SelectedKey& lhs, const SelectedKey& rhs) const {
        return lhs.key.getTime() < rhs.key.getTime();
    }
};




typedef std::set< SelectedKey,SelectedKey_compare_time > SelectedKeys;

class QMenu;
class CurveWidgetPrivate;

class CurveWidget : public QGLWidget
{
    friend class CurveGui;
    friend class CurveWidgetPrivate;

    Q_OBJECT

public:
    
    /*Pass a null timeline ptr if you don't want interaction with the global timeline. */
    CurveWidget(boost::shared_ptr<TimeLine> timeline = boost::shared_ptr<TimeLine>() ,
                QWidget* parent = NULL, const QGLWidget* shareWidget = NULL);

    virtual ~CurveWidget();

    void centerOn(double xmin,double xmax,double ymin,double ymax);

    CurveGui *createCurve(boost::shared_ptr<Curve> curve,KnobGui* knob,int dimension, const QString &name);

    void removeCurve(CurveGui* curve);

    void centerOn(const std::vector<CurveGui*>& curves);

    void showCurvesAndHideOthers(const std::vector<CurveGui*>& curves);
    
    void getVisibleCurves(std::vector<CurveGui*>* curves) const;

    void addKeyFrame(CurveGui* curve, const KeyFrame& key);

    void removeKeyFrame(CurveGui* curve,const KeyFrame& key);
    
    void moveKeyFrame(CurveGui* curve,const KeyFrame& key,double dt,double dv);
        
    void setSelectedKeys(const SelectedKeys& keys);
    
    double getPixelAspectRatio() const;
    
    void getBackgroundColor(double *r,double *g,double* b) const;
    
public slots:
    
    void refreshDisplayedTangents();

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

    void breakDerivativesForSelectedKeyFrames();

    void frameSelectedCurve();

    void refreshSelectedKeysBbox();

    void onTimeLineFrameChanged(SequenceTime time, int reason);

    void onTimeLineBoundariesChanged(SequenceTime left, SequenceTime right, int);
    
    void onCurveChanged();
    
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

private:

    boost::scoped_ptr<CurveWidgetPrivate> _imp;
        
};

Q_DECLARE_METATYPE(CurveWidget*)


#endif // CURVE_WIDGET_H
