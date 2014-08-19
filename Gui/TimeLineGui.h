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

#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>
#include <QtCore/QList>
#include <QtCore/QPointF>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

class Gui;
class ViewerTab;
class QMouseEvent;
class TimeLine;
struct TimelineGuiPrivate;

class TimeLineGui : public QGLWidget {

    Q_OBJECT

public:
    explicit TimeLineGui(boost::shared_ptr<TimeLine> timeLine,Gui* gui, QWidget* parent,const QGLWidget* shareWidget = NULL);

    virtual ~TimeLineGui() OVERRIDE;

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
     *@brief See toZoomCoordinates in ViewerGL.h
     **/
    QPointF toTimeLineCoordinates(double x, double y) const;

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPointF toWidgetCoordinates(double x, double y) const;

    /**
     * @brief Activates the SLOT onViewerCacheFrameAdded() and the SIGNALS removedLRUCachedFrame() and  clearedViewerCache()
     * by connecting them to the ViewerCache emitted signals. They in turn are used by the GUI to refresh the "cached line" on
     * the timeline.
     **/
    void connectSlotsToViewerCache();
    
    /**
     * @brief Since the ViewerCache is global to the application, we don't want
     * a main window (an AppInstance) draw some cached line because another instance is running some playback or rendering something.
     **/
    void disconnectSlotsFromViewerCache();
    
    
public slots:

    void centerOn(SequenceTime left,SequenceTime right);

    void onFrameChanged(SequenceTime,int);
    void onFrameRangeChanged(SequenceTime first, SequenceTime last);
    void onBoundariesChanged(SequenceTime, SequenceTime, int);
    
    void onCachedFrameAdded(SequenceTime time);
    void onCachedFrameRemoved(SequenceTime time,int storage);
    void onCachedFrameStorageChanged(SequenceTime time,int oldStorage,int newStorage);
    void onMemoryCacheCleared();
    void onDiskCacheCleared();

    void clearCachedFrames();
    
    void onKeyframesIndicatorsChanged();
private:

    virtual void initializeGL() OVERRIDE FINAL;
    virtual void resizeGL(int width,int height) OVERRIDE FINAL;
    virtual void paintGL() OVERRIDE FINAL;

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;

    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;


signals:

    void frameChanged(SequenceTime);
    void boundariesChanged(SequenceTime,SequenceTime);

private:

    boost::scoped_ptr<TimelineGuiPrivate> _imp;

};

#endif /* defined(NATRON_GUI_TIMELINE_H_) */
