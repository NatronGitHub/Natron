//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

class QString;
class QColor;
class QFont;

class Gui;
class ViewerGL;
/**
 * @class An histogram view in the histograms gui.
 **/
struct HistogramPrivate;
class Histogram : public QGLWidget
{
    Q_OBJECT
    
public:
    
    enum DisplayMode {
        RGB = 0,
        A,
        Y,
        R,
        G,
        B
    };
    
    Histogram(Gui* gui, const QGLWidget* shareWidget = NULL);
    
    virtual ~Histogram();

    
    QPointF toHistogramCoordinates(double x,double y) const;
    
    QPointF toWidgetCoordinates(double x, double y) const;
    
    void centerOn(double xmin,double xmax,double ymin,double ymax);
    
    
    void renderText(double x,double y,const QString& text,const QColor& color,const QFont& font) const;
    
    
public slots:
    
#ifndef NATRON_HISTOGRAM_USING_OPENGL

    void onCPUHistogramComputed();
    
    void computeHistogramAndRefresh(bool forceEvenIfNotVisible = false);

    void populateViewersChoices();
    
#endif
    
    void onDisplayModeChanged(QAction*);
    
    void onFilterChanged(QAction*);
    
    void onCurrentViewerChanged(QAction*);

    void onViewerImageChanged(ViewerGL* viewer);
    
private:
    
    virtual void initializeGL() OVERRIDE FINAL; 
    
    virtual void paintGL() OVERRIDE FINAL;
    
    virtual void resizeGL(int w,int h) OVERRIDE FINAL;
    
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    
    virtual void wheelEvent(QWheelEvent *event) OVERRIDE FINAL;
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    
    virtual void showEvent(QShowEvent* e) OVERRIDE FINAL;
    
    virtual QSize sizeHint() const OVERRIDE FINAL;
    
    boost::scoped_ptr<HistogramPrivate> _imp;
};

#endif // HISTOGRAM_H
