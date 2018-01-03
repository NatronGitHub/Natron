/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_GUI_TIMELINE_H
#define NATRON_GUI_TIMELINE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include "Global/GLIncludes.h" //!<must be included before QGLWidget because of gl.h and glew.h
#include <QtOpenGL/QGLWidget>
#include "Global/GLObfuscate.h" //!<must be included after QGLWidget
#include <QtCore/QList>
#include <QtCore/QPointF>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#include "Engine/OverlaySupport.h"

#include "Global/GlobalDefines.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct TimelineGuiPrivate;

class TimeLineGui
    : public QGLWidget
    , public OverlaySupport
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    explicit TimeLineGui(const ViewerNodePtr& viewer,
                         const TimeLinePtr& timeLine,
                         Gui* gui,
                         ViewerTab* viewerTab);

    virtual ~TimeLineGui() OVERRIDE;

    void discardGuiPointer();

    void setTimeline(const TimeLinePtr& timeline);
    TimeLinePtr getTimeline() const;

    /*initialises the boundaries on the timeline*/
    void setBoundaries(SequenceTime first, SequenceTime last);


    SequenceTime leftBound() const;
    SequenceTime rightBound() const;
    SequenceTime currentFrame() const;

    void getBounds(SequenceTime* left, SequenceTime* right) const;

    void setCursorColor(const QColor & cursorColor);
    void setBoundsColor(const QColor & boundsColor);
    void setCachedLineColor(const QColor & cachedLineColor);
    void setTicksColor(const QColor & ticksColor);
    void setBackGroundColor(const QColor & backgroundColor);
    void setScaleColor(const QColor & scaleColor);

    void seek(SequenceTime time);

    void renderText(double x,
                    double y,
                    const QString & text,
                    const QColor & color,
                    const QFont & font,
                    int flags = 0) const; //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10


    /**
     *@brief See toZoomCoordinates in ViewerGL.h
     **/
    QPointF toTimeLineCoordinates(double x, double y) const;
    double toTimeLine(double x) const;

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPointF toWidgetCoordinates(double x, double y) const;
    double toWidget(double t) const;

    /**
     * @brief Activates the SLOT onViewerCacheFrameAdded() and the SIGNALS removedLRUCachedFrame() and  clearedViewerCache()
     * by connecting them to the ViewerCache emitted signals. They in turn are used by the GUI to refresh the "cached line" on
     * the timeline.
     **/
    void connectSlotsToViewerCache();

    bool isFrameRangeEdited() const;

    void setFrameRangeEdited(bool edited);

    void centerOn_tripleSync(SequenceTime left, SequenceTime right);

    void getVisibleRange(SequenceTime* left, SequenceTime* right) const;


    // Overriden from OverlaySupport
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE FINAL;
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE FINAL;
    virtual int getWidgetFontHeight() const OVERRIDE FINAL;
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE FINAL;
    virtual RectD getViewportRect() const OVERRIDE FINAL;
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL;
    virtual RangeD getFrameRange() const OVERRIDE FINAL;
    virtual bool renderText(double x,
                            double y,
                            const std::string &string,
                            double r,
                            double g,
                            double b,
                            double a,
                            int flags = 0) OVERRIDE FINAL;
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;
    virtual void redraw() OVERRIDE FINAL;
    virtual void getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const OVERRIDE FINAL;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    virtual void getPixelScale(double & xScale, double & yScale) const  OVERRIDE FINAL;
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;
    virtual unsigned int getCurrentRenderScale() const OVERRIDE FINAL;

    ////

public Q_SLOTS:


    void recenterOnBounds();

    void centerOn(SequenceTime left, SequenceTime right, int margin = 5);

    void onFrameChanged(SequenceTime, int);

    void onProjectFrameRangeChanged(int, int);



private:

    void setBoundariesInternal(SequenceTime first, SequenceTime last, bool emitSignal);

    virtual void initializeGL() OVERRIDE FINAL;
    virtual void resizeGL(int width, int height) OVERRIDE FINAL;
    virtual void paintGL() OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;

Q_SIGNALS:

    
    void boundariesChanged(SequenceTime, SequenceTime);

private:

    boost::scoped_ptr<TimelineGuiPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif /* defined(NATRON_GUI_TIMELINE_H_) */
