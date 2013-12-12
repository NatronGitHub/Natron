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

#ifndef NATRON_GUI_TIMELINE_H_
#define NATRON_GUI_TIMELINE_H_

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <QtCore/QList>
#include <QtCore/QPointF>

#include "Global/GlobalDefines.h"


class ViewerTab;
class QMouseEvent;
class TimeLine;
struct TimelineGuiPrivate;

class TimeLineGui : public QGLWidget{

    Q_OBJECT

public:


    
    explicit TimeLineGui(boost::shared_ptr<TimeLine> timeLine, QWidget* parent,const QGLWidget* shareWidget = NULL);

    virtual ~TimeLineGui();

    /*initialises the boundaries on the timeline*/
    void setBoundaries(SequenceTime first, SequenceTime last);
    
    SequenceTime firstFrame() const;
    SequenceTime lastFrame() const;
    SequenceTime leftBound() const;
    SequenceTime rightBound() const;
    SequenceTime currentFrame() const;

    void setCursorColor(const QColor& cursorColor);
    void setBoundsColor(const QColor& boundsColor);
    void setCachedLineColor(const QColor& cachedLineColor);
    void setTicksColor(const QColor& ticksColor);
    void setBackGroundColor(const QColor& backgroundColor);
    void setScaleColor(const QColor& scaleColor);
    
    void seek(SequenceTime time);

    void renderText(double x,double y,const QString& text,const QColor& color,const QFont& font) const;


    /**
     *@brief See toImgCoordinates_fast in ViewerGL.h
     **/
    QPointF toTimeLineCoordinates(double x, double y) const;

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPointF toWidgetCoordinates(double x, double y) const;

public slots:

    void centerOn(SequenceTime left,SequenceTime right);

    void onFrameChanged(SequenceTime,int);
    void onFrameRangeChanged(SequenceTime first, SequenceTime last);
    void onBoundariesChanged(SequenceTime, SequenceTime, int);
    void onCachedFrameAdded(SequenceTime time);
    void onCachedFrameRemoved(SequenceTime time);
    void onLRUCachedFrameRemoved();
    void onCachedFramesCleared();

protected:


    virtual void initializeGL();
    virtual void resizeGL(int width,int height);
    virtual void paintGL();

    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void wheelEvent(QWheelEvent *event);

    virtual void enterEvent(QEvent* e);
    virtual void leaveEvent(QEvent* e);
    virtual QSize sizeHint() const;


signals:

    void frameChanged(SequenceTime);
    void boundariesChanged(SequenceTime,SequenceTime);

private:

    boost::scoped_ptr<TimelineGuiPrivate> _imp;

};

#endif /* defined(NATRON_GUI_TIMELINE_H_) */
