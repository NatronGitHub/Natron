/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "TimeLineGui.h"

#include <cmath>
#include <set>
#include <stdexcept>

#include <QtGui/QFont>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include "Global/GlobalDefines.h"

#include "Engine/Cache.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/TimeLineKeys.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Gui/AnimationModuleEditor.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/TextRenderer.h"
#include "Gui/ViewerTab.h"
#include "Gui/ticks.h"

NATRON_NAMESPACE_ENTER;

#define TICK_HEIGHT 7
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

#define DEFAULT_TIMELINE_LEFT_BOUND 0
#define DEFAULT_TIMELINE_RIGHT_BOUND 100

#define USER_KEYFRAMES_HEIGHT 7


NATRON_NAMESPACE_ANONYMOUS_ENTER

struct TimeLineZoomContext
{
    TimeLineZoomContext()
        : bottom(0.)
        , left(0.)
        , zoomFactor(1.)
    {
    }

    QPoint oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
    double bottom; /// the bottom edge of orthographic projection
    double left; /// the left edge of the orthographic projection
    double zoomFactor; /// the zoom factor applied to the current image
};

struct CachedFrame
{
    SequenceTime time;
    StorageModeEnum mode;

    CachedFrame(SequenceTime t,
                StorageModeEnum m)
        : time(t)
        , mode(m)
    {
    }
};

struct CachedFrame_compare_time
{
    bool operator() (const CachedFrame & lhs,
                     const CachedFrame & rhs) const
    {
        return lhs.time < rhs.time;
    }
};

typedef std::set<CachedFrame, CachedFrame_compare_time> CachedFrames;

NATRON_NAMESPACE_ANONYMOUS_EXIT


struct TimelineGuiPrivate
{
    TimeLineGui *parent;

    // Weakptr because the lifetime of this widget is controlled by the node itself
    ViewerNodeWPtr viewer;
    ViewerTab* viewerTab;
    TimeLinePtr timeline; ///< ptr to the internal timeline
    Gui* gui; ///< ptr to the gui
    bool alphaCursor; ///< should cursor be drawn semi-transparent
    QPoint lastMouseEventWidgetCoord;
    TimelineStateEnum state; ///< state machine for mouse events
    int mousePressX; ///< widget X coordinate of last click
    int mouseMoveX; ///< widget X coordinate of last mousemove position
    TimeLineZoomContext tlZoomCtx;
    TextRenderer textRenderer;
    QFont font;
    bool firstPaint;
    CachedFrames cachedFrames;
    mutable QMutex boundariesMutex;
    SequenceTime leftBoundary, rightBoundary;
    mutable QMutex frameRangeEditedMutex;
    bool isFrameRangeEdited;
    bool seekingTimeline;

    int nRefreshCacheFrameRequests;

    TimelineGuiPrivate(TimeLineGui *qq,
                       const ViewerNodePtr& viewer,
                       Gui* gui,
                       ViewerTab* viewerTab)
        : parent(qq)
        , viewer(viewer)
        , viewerTab(viewerTab)
        , timeline()
        , gui(gui)
        , alphaCursor(false)
        , lastMouseEventWidgetCoord()
        , state(eTimelineStateIdle)
        , mousePressX(0)
        , mouseMoveX(0)
        , tlZoomCtx()
        , textRenderer()
        , font(appFont, appFontSize)
        , firstPaint(true)
        , cachedFrames()
        , boundariesMutex()
        , leftBoundary(0)
        , rightBoundary(0)
        , frameRangeEditedMutex()
        , isFrameRangeEdited(false)
        , seekingTimeline(false)
        , nRefreshCacheFrameRequests(0)
    {
    }

    void updateEditorFrameRanges()
    {
        double zoomRight = parent->toTimeLine(parent->width() - 1);

        gui->getAnimationModuleEditor()->centerOn(tlZoomCtx.left - 5, zoomRight - 5);
    }

    void updateOpenedViewersFrameRanges()
    {
        double zoomRight = parent->toTimeLine(parent->width() - 1);
        const std::list<ViewerTab *> &viewers = gui->getViewersList();

        for (std::list<ViewerTab *>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            if ( (*it) != viewerTab ) {
                (*it)->centerOn(tlZoomCtx.left, zoomRight);
            }
        }
    }


};

TimeLineGui::TimeLineGui(const ViewerNodePtr& viewer,
                         const TimeLinePtr& timeline,
                         Gui* gui,
                         ViewerTab* viewerTab)
    : QGLWidget(viewerTab)
    , _imp( new TimelineGuiPrivate(this, viewer, gui, viewerTab) )
{
    setTimeline(timeline);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);

    connect(this, SIGNAL(refreshCachedFramesLaterReceived()), this, SLOT(onRefreshCachedFramesLaterReceived()));

}

TimeLineGui::~TimeLineGui()
{
}

void
TimeLineGui::setTimeline(const TimeLinePtr& timeline)
{
    GuiAppInstancePtr app = _imp->gui->getApp();

    assert(app);
    if (_imp->timeline) {
        //connect the internal timeline to the gui
        QObject::disconnect( _imp->timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onFrameChanged(SequenceTime,int)) );
    }

    //connect the internal timeline to the gui
    QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onFrameChanged(SequenceTime,int)), Qt::UniqueConnection );

    _imp->timeline = timeline;
}

TimeLinePtr
TimeLineGui::getTimeline() const
{
    return _imp->timeline;
}

QSize
TimeLineGui::sizeHint() const
{
    return QSize( TO_DPIX(1000), TO_DPIY(45) );
}

void
TimeLineGui::initializeGL()
{
    appPTR->initializeOpenGLFunctionsOnce();
}

void
TimeLineGui::resizeGL(int width,
                      int height)
{
    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    if (height == 0) {
        height = 1;
    }
    GL_GPU::Viewport (0, 0, width, height);
}

void
TimeLineGui::discardGuiPointer()
{
    _imp->gui = 0;
}

void
TimeLineGui::paintGL()
{
    if (!_imp->gui) {
        return;
    }
    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }


    glCheckError(GL_GPU);

    SequenceTime leftBound, rightBound;
    {
        QMutexLocker k(&_imp->boundariesMutex);
        leftBound = _imp->leftBoundary;
        rightBound = _imp->rightBoundary;
    }
    const SequenceTime currentTime = _imp->timeline->currentFrame();

    if (_imp->firstPaint) {
        _imp->firstPaint = false;

        if ( (rightBound - leftBound) > 10000 ) {
            centerOn(currentTime - 100, currentTime + 100);
        } else if ( (rightBound - leftBound) < 50 ) {
            centerOn(currentTime - DEFAULT_TIMELINE_LEFT_BOUND, currentTime + DEFAULT_TIMELINE_RIGHT_BOUND);
        } else {
            centerOn(leftBound, rightBound);
        }
    }

    double w = (double)width();
    double h = (double)height();
    //assert(_tlZoomCtx._zoomFactor > 0);
    if (_imp->tlZoomCtx.zoomFactor <= 0) {
        return;
    }
    //assert(_tlZoomCtx._zoomFactor <= 1024);
    double bottom = _imp->tlZoomCtx.bottom;
    double left = _imp->tlZoomCtx.left;
    double top = bottom +  h / (double)_imp->tlZoomCtx.zoomFactor;
    double right = left +  (w / (double)_imp->tlZoomCtx.zoomFactor);
    double clearR, clearG, clearB;
    SettingsPtr settings = appPTR->getCurrentSettings();
    settings->getTimelineBGColor(&clearR, &clearG, &clearB);

    if ( (left == right) || (top == bottom) ) {
        GL_GPU::ClearColor(clearR, clearG, clearB, 1.);
        GL_GPU::Clear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug(GL_GPU);

        return;
    }

    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_SCISSOR_BIT | GL_TRANSFORM_BIT);
        //GLProtectMatrix p(GL_PROJECTION); // no need to protect
        GL_GPU::MatrixMode(GL_PROJECTION);
        GL_GPU::LoadIdentity();
        GL_GPU::Ortho(left, right, bottom, top, 1, -1);
        //GLProtectMatrix m(GL_MODELVIEW); // no need to protect
        GL_GPU::MatrixMode(GL_MODELVIEW);
        GL_GPU::LoadIdentity();

        GL_GPU::ClearColor(clearR, clearG, clearB, 1.);
        GL_GPU::Clear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug(GL_GPU);

        QPointF btmLeft = toTimeLineCoordinates(0, height() - 1);
        QPointF topRight = toTimeLineCoordinates(width() - 1, 0);


        /// change the backgroud color of the portion of the timeline where images are lying
        TimeValue firstFrame, lastFrame;
        if ( !_imp->viewerTab->isFileDialogViewer() ) {
            _imp->gui->getApp()->getProject()->getFrameRange(&firstFrame, &lastFrame);
        } else {
            int f, l;
            _imp->viewerTab->getTimelineBounds(&f, &l);
            firstFrame = TimeValue(f);
            lastFrame = TimeValue(l);
        }
        QPointF firstFrameWidgetPos = toWidgetCoordinates(firstFrame, 0);
        QPointF lastFrameWidgetPos = toWidgetCoordinates(lastFrame, 0);

        GL_GPU::Scissor( firstFrameWidgetPos.x(), 0,
                   lastFrameWidgetPos.x() - firstFrameWidgetPos.x(), height() );

        double bgR, bgG, bgB;
        settings->getBaseColor(&bgR, &bgG, &bgB);

        GL_GPU::Enable(GL_SCISSOR_TEST);
        GL_GPU::ClearColor(bgR, bgG, bgB, 1.);
        GL_GPU::Clear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug(GL_GPU);
        GL_GPU::Disable(GL_SCISSOR_TEST);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (_imp->state == eTimelineStateSelectingZoomRange) {
            // draw timeline selected range
            // https://github.com/MrKepzie/Natron/issues/917
            // draw the select range, from _imp->mousePressX to _imp->mouseMoveX
            GL_GPU::Color4f(1, 1, 1, 0.3);
            GL_GPU::Begin(GL_POLYGON);
            GL_GPU::Vertex2f( toTimeLine(_imp->mousePressX), btmLeft.y() );
            GL_GPU::Vertex2f( toTimeLine(_imp->mousePressX), topRight.y() );
            GL_GPU::Vertex2f( toTimeLine(_imp->mouseMoveX), topRight.y() );
            GL_GPU::Vertex2f( toTimeLine(_imp->mouseMoveX), btmLeft.y() );
            GL_GPU::End();
        }


        QFontMetrics fontM(_imp->font, 0);
        double lineYPosWidget = height() - 1 - fontM.height()  - TO_DPIY(TICK_HEIGHT) / 2.;
        double lineYpos = toTimeLineCoordinates(0, lineYPosWidget).y();
        double cachedLineYPos = toTimeLineCoordinates(0, lineYPosWidget + 1).y();

        /*draw the horizontal axis*/
        double txtR, txtG, txtB;
        settings->getTextColor(&txtR, &txtG, &txtB);
        double kfR, kfG, kfB;
        settings->getKeyframeColor(&kfR, &kfG, &kfB);

        double userkfR, userkfG, userkfB;
        settings->getTrackerKeyframeColor(&userkfR, &userkfG, &userkfB);


        double cursorR, cursorG, cursorB;
        settings->getTimelinePlayheadColor(&cursorR, &cursorG, &cursorB);

        double boundsR, boundsG, boundsB;
        settings->getTimelineBoundsColor(&boundsR, &boundsG, &boundsB);

        double cachedR, cachedG, cachedB;
        settings->getCachedFrameColor(&cachedR, &cachedG, &cachedB);

        double dcR, dcG, dcB;
        settings->getDiskCachedColor(&dcR, &dcG, &dcB);


        GL_GPU::Color4f(txtR / 2., txtG / 2., txtB / 2., 1.);
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2f(btmLeft.x(), lineYpos);
        GL_GPU::Vertex2f(topRight.x(), lineYpos);
        GL_GPU::End();
        glCheckErrorIgnoreOSXBug(GL_GPU);

        double tickBottom = toTimeLineCoordinates( 0, height() - 1 - fontM.height() ).y();
        double tickTop = toTimeLineCoordinates( 0, height() - 1 - fontM.height()  - TO_DPIY(TICK_HEIGHT) ).y();
        const double smallestTickSizePixel = 30.; // tick size (in pixels) for alpha = 0.
        const double largestTickSizePixel = 200.; // tick size (in pixels) for alpha = 1.
        const double rangePixel =  width();
        const double range_min = btmLeft.x();
        const double range_max =  topRight.x();
        const double range = range_max - range_min;
        double smallTickSize;
        bool half_tick;
        ticks_size(range_min, range_max, rangePixel, smallestTickSizePixel, &smallTickSize, &half_tick);
        if (smallTickSize < 1) {
            smallTickSize = 1;
            half_tick = false;
        }
        int m1, m2;
        const int ticks_max = 1000;
        double offset;
        ticks_bounds(range_min, range_max, smallTickSize, half_tick, ticks_max, &offset, &m1, &m2);
        std::vector<int> ticks;
        ticks_fill(half_tick, ticks_max, m1, m2, &ticks);
        const double smallestTickSize = range * smallestTickSizePixel / rangePixel;
        const double largestTickSize = range * largestTickSizePixel / rangePixel;
        const double minTickSizeTextPixel = fontM.width( QLatin1String("00000") ); // AXIS-SPECIFIC
        const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;
        for (int i = m1; i <= m2; ++i) {
            double value = i * smallTickSize + offset;
            const double tickSize = ticks[i - m1] * smallTickSize;
            const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

            // because smallTickSize is at least 1, isFloating can never be true
            bool isFloating = std::abs(std::floor(0.5 + value) - value) != 0.;
            assert(!isFloating);
            if (isFloating) {
                continue;
            }
            GL_GPU::Color4f(txtR, txtG, txtB, alpha);

            GL_GPU::Begin(GL_LINES);
            GL_GPU::Vertex2f(value, tickBottom);
            GL_GPU::Vertex2f(value, tickTop);
            GL_GPU::End();
            glCheckErrorIgnoreOSXBug(GL_GPU);

            if (tickSize > minTickSizeText) {
                const int tickSizePixel = rangePixel * tickSize / range;
                const QString s = QString::number(value);
                const int sSizePixel =  fontM.width(s);
                if (tickSizePixel > sSizePixel) {
                    const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                    double alphaText = 1.0; //alpha;
                    if (tickSizePixel < sSizeFullPixel) {
                        // when the text size is between sSizePixel and sSizeFullPixel,
                        // draw it with a lower alpha
                        alphaText *= (tickSizePixel - sSizePixel) / (double)minTickSizeTextPixel;
                    }
                    alphaText = std::min(alphaText, alpha); // don't draw more opaque than tcks
                    QColor c;
                    c.setRgbF( Image::clamp<qreal>(txtR, 0., 1.),
                               Image::clamp<qreal>(txtG, 0., 1.),
                               Image::clamp<qreal>(txtB, 0., 1.) );
                    c.setAlpha(255 * alphaText);
                    glCheckError(GL_GPU);
                    renderText(value, btmLeft.y(), s, c, _imp->font, Qt::AlignHCenter);
                }
            }
        }
        glCheckError(GL_GPU);

        int cursorWidth = TO_DPIX(CURSOR_WIDTH);
        int cursorHeight = TO_DPIY(CURSOR_HEIGHT);
        QPointF cursorBtm(currentTime, lineYpos);
        QPointF cursorBtmWidgetCoord = toWidgetCoordinates( cursorBtm.x(), cursorBtm.y() );
        QPointF cursorTopLeft = toTimeLineCoordinates(cursorBtmWidgetCoord.x() - cursorWidth / 2.,
                                                      cursorBtmWidgetCoord.y() - cursorHeight);
        QPointF cursorTopRight = toTimeLineCoordinates(cursorBtmWidgetCoord.x() + cursorWidth / 2.,
                                                       cursorBtmWidgetCoord.y() - cursorHeight);
        QPointF leftBoundBtm(leftBound, lineYpos);
        QPointF leftBoundWidgetCoord = toWidgetCoordinates( leftBoundBtm.x(), leftBoundBtm.y() );
        QPointF leftBoundBtmRight = toTimeLineCoordinates( leftBoundWidgetCoord.x() + cursorWidth / 2.,
                                                           leftBoundWidgetCoord.y() );
        QPointF leftBoundTop = toTimeLineCoordinates(leftBoundWidgetCoord.x(),
                                                     leftBoundWidgetCoord.y() - cursorHeight);
        QPointF rightBoundBtm(rightBound, lineYpos);
        QPointF rightBoundWidgetCoord = toWidgetCoordinates( rightBoundBtm.x(), rightBoundBtm.y() );
        QPointF rightBoundBtmLeft = toTimeLineCoordinates( rightBoundWidgetCoord.x() - cursorWidth / 2.,
                                                           rightBoundWidgetCoord.y() );
        QPointF rightBoundTop = toTimeLineCoordinates(rightBoundWidgetCoord.x(),
                                                      rightBoundWidgetCoord.y() - cursorHeight);

        /// pair<time, isUserKey>
        const TimeLineKeysSet& keyframes = _imp->gui->getTimelineGuiKeyframes();

        //draw an alpha cursor if the mouse is hovering the timeline
        GL_GPU::Enable(GL_POLYGON_SMOOTH);
        GL_GPU::Hint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
        glCheckError(GL_GPU);
        if (_imp->alphaCursor) {
            int currentPosBtmWidgetCoordX = _imp->lastMouseEventWidgetCoord.x();
            int currentPosBtmWidgetCoordY = toWidgetCoordinates(0, lineYpos).y();
            QPointF currentPosBtm = toTimeLineCoordinates(currentPosBtmWidgetCoordX, currentPosBtmWidgetCoordY);
            QPointF currentPosTopLeft = toTimeLineCoordinates(currentPosBtmWidgetCoordX - cursorWidth / 2.,
                                                              currentPosBtmWidgetCoordY - cursorHeight);
            QPointF currentPosTopRight = toTimeLineCoordinates(currentPosBtmWidgetCoordX + cursorWidth / 2.,
                                                               currentPosBtmWidgetCoordY - cursorHeight);
            int hoveredTime = std::floor(currentPosBtm.x() + 0.5);
            QString mouseNumber( QString::number(hoveredTime) );
            QPoint mouseNumberWidgetCoord(currentPosBtmWidgetCoordX,
                                          currentPosBtmWidgetCoordY - cursorHeight);
            QPointF mouseNumberPos = toTimeLineCoordinates( mouseNumberWidgetCoord.x(), mouseNumberWidgetCoord.y() );
            TimeLineKeysSet::iterator foundHoveredAsKeyframe = keyframes.find( TimeLineKey(hoveredTime) );
            QColor currentColor;
            if ( foundHoveredAsKeyframe != keyframes.end() ) {
                if (foundHoveredAsKeyframe->isUserKey) {
                    GL_GPU::Color4f(userkfR, userkfG, userkfB, 0.4);
                    currentColor.setRgbF( Image::clamp<qreal>(userkfR, 0., 1.),
                                          Image::clamp<qreal>(userkfG, 0., 1.),
                                          Image::clamp<qreal>(userkfB, 0., 1.) );
                } else {
                    GL_GPU::Color4f(kfR, kfG, kfB, 0.4);
                    currentColor.setRgbF( Image::clamp<qreal>(kfR, 0., 1.),
                                          Image::clamp<qreal>(kfG, 0., 1.),
                                          Image::clamp<qreal>(kfB, 0., 1.) );
                }
            } else {
                GL_GPU::Color4f(cursorR, cursorG, cursorB, 0.4);
                currentColor.setRgbF( Image::clamp<qreal>(cursorR, 0., 1.),
                                      Image::clamp<qreal>(cursorG, 0., 1.),
                                      Image::clamp<qreal>(cursorB, 0., 1.) );
            }
            currentColor.setAlpha(100);


            GL_GPU::Begin(GL_POLYGON);
            GL_GPU::Vertex2f( currentPosBtm.x(), currentPosBtm.y() );
            GL_GPU::Vertex2f( currentPosTopLeft.x(), currentPosTopLeft.y() );
            GL_GPU::Vertex2f( currentPosTopRight.x(), currentPosTopRight.y() );
            GL_GPU::End();
            glCheckErrorIgnoreOSXBug(GL_GPU);

            renderText(mouseNumberPos.x(), mouseNumberPos.y(), mouseNumber, currentColor, _imp->font, Qt::AlignHCenter);
        }

        //draw the bounds and the current time cursor
        TimeLineKeysSet::iterator isCurrentTimeAKeyframe = keyframes.find( TimeLineKey(currentTime) );
        QColor actualCursorColor;
        if ( isCurrentTimeAKeyframe != keyframes.end() ) {
            if (isCurrentTimeAKeyframe->isUserKey) {
                GL_GPU::Color4f(userkfR, userkfG, userkfB, 1.);
                actualCursorColor.setRgbF( Image::clamp<qreal>(userkfR, 0., 1.),
                                           Image::clamp<qreal>(userkfG, 0., 1.),
                                           Image::clamp<qreal>(userkfB, 0., 1.) );
            } else {
                GL_GPU::Color4f(kfR, kfG, kfB, 1.);
                actualCursorColor.setRgbF( Image::clamp<qreal>(kfR, 0., 1.),
                                           Image::clamp<qreal>(kfG, 0., 1.),
                                           Image::clamp<qreal>(kfB, 0., 1.) );
            }
        } else {
            GL_GPU::Color4f(cursorR, cursorG, cursorB, 1.);
            actualCursorColor.setRgbF( Image::clamp<qreal>(cursorR, 0., 1.),
                                       Image::clamp<qreal>(cursorG, 0., 1.),
                                       Image::clamp<qreal>(cursorB, 0., 1.) );
        }

        QString currentFrameStr = QString::number(currentTime);
        double cursorTextXposWidget = cursorBtmWidgetCoord.x();
        double cursorTextPos = toTimeLine(cursorTextXposWidget);
        renderText(cursorTextPos, cursorTopLeft.y(), currentFrameStr, actualCursorColor, _imp->font, Qt::AlignHCenter);
        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f( cursorBtm.x(), cursorBtm.y() );
        GL_GPU::Vertex2f( cursorTopLeft.x(), cursorTopLeft.y() );
        GL_GPU::Vertex2f( cursorTopRight.x(), cursorTopRight.y() );
        GL_GPU::End();
        glCheckErrorIgnoreOSXBug(GL_GPU);

        QColor boundsColor;
        boundsColor.setRgbF( Image::clamp<qreal>(boundsR, 0., 1.),
                             Image::clamp<qreal>(boundsG, 0., 1.),
                             Image::clamp<qreal>(boundsB, 0., 1.) );


        {
            if ( ( leftBoundBtm.x() >= btmLeft.x() ) && ( leftBoundBtmRight.x() <= topRight.x() ) ) {
                if (leftBound != currentTime) {
                    QString leftBoundStr( QString::number(leftBound) );
                    double leftBoundTextXposWidget = toWidgetCoordinates( ( leftBoundBtm.x() + leftBoundBtmRight.x() ) / 2., 0 ).x();
                    double leftBoundTextPos = toTimeLine(leftBoundTextXposWidget);
                    renderText(leftBoundTextPos, leftBoundTop.y(),
                               leftBoundStr, boundsColor, _imp->font, Qt::AlignHCenter);
                }
                GL_GPU::Color4f(boundsR, boundsG, boundsB, 1.);
                GL_GPU::Begin(GL_POLYGON);
                GL_GPU::Vertex2f( leftBoundBtm.x(), leftBoundBtm.y() );
                GL_GPU::Vertex2f( leftBoundBtmRight.x(), leftBoundBtmRight.y() );
                GL_GPU::Vertex2f( leftBoundTop.x(), leftBoundTop.y() );
                GL_GPU::End();
                glCheckErrorIgnoreOSXBug(GL_GPU);
            }

            if ( ( rightBoundBtmLeft.x() >= btmLeft.x() ) && ( rightBoundBtm.x() <= topRight.x() ) ) {
                if ( (rightBound != currentTime) && (rightBound != leftBound) ) {
                    QString rightBoundStr( QString::number( rightBound ) );
                    double rightBoundTextXposWidget = toWidgetCoordinates( ( rightBoundBtm.x() + rightBoundBtmLeft.x() ) / 2., 0 ).x();
                    double rightBoundTextPos = toTimeLine(rightBoundTextXposWidget);
                    renderText(rightBoundTextPos, rightBoundTop.y(),
                               rightBoundStr, boundsColor, _imp->font, Qt::AlignHCenter);
                }
                GL_GPU::Color4f(boundsR, boundsG, boundsB, 1.);
                glCheckError(GL_GPU);
                GL_GPU::Begin(GL_POLYGON);
                GL_GPU::Vertex2f( rightBoundBtm.x(), rightBoundBtm.y() );
                GL_GPU::Vertex2f( rightBoundBtmLeft.x(), rightBoundBtmLeft.y() );
                GL_GPU::Vertex2f( rightBoundTop.x(), rightBoundTop.y() );
                GL_GPU::End();
                glCheckErrorIgnoreOSXBug(GL_GPU);
            }
        }


        //draw cached frames
        GL_GPU::Enable(GL_LINE_SMOOTH);
        GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        glCheckError(GL_GPU);
        GL_GPU::LineWidth(2);
        glCheckError(GL_GPU);
        GL_GPU::Begin(GL_LINES);
        for (CachedFrames::const_iterator i = _imp->cachedFrames.begin(); i != _imp->cachedFrames.end(); ++i) {
            if ( ( i->time >= btmLeft.x() ) && ( i->time <= topRight.x() ) ) {
                if (i->mode == eStorageModeRAM) {
                    GL_GPU::Color4f(cachedR, cachedG, cachedB, 1.);
                } else if (i->mode == eStorageModeDisk) {
                    GL_GPU::Color4f(dcR, dcG, dcB, 1.);
                }
                GL_GPU::Vertex2f(i->time, cachedLineYPos);
                GL_GPU::Vertex2f(i->time + 1, cachedLineYPos);
            }
        }
        GL_GPU::End();

        ///now draw keyframes
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Color4f(kfR, kfG, kfB, 1.);
        std::list<SequenceTime> remainingUserKeys;
        for (TimeLineKeysSet::const_iterator i = keyframes.begin(); i != keyframes.end(); ++i) {
            if ( ( i->frame >= btmLeft.x() ) && ( i->frame <= topRight.x() ) ) {
                if (!i->isUserKey) {
                    GL_GPU::Vertex2f(i->frame, lineYpos);
                    GL_GPU::Vertex2f(i->frame + 1, lineYpos);
                } else {
                    remainingUserKeys.push_back(i->frame);
                }
            }
        }
        GL_GPU::End();

        //const double keyHeight = TO_DPIY(USER_KEYFRAMES_HEIGHT);
        //const double keyTop = toTimeLineCoordinates(0, lineYPosWidget - keyHeight).y();
        //const double keyBtm = toTimeLineCoordinates(0, lineYPosWidget + keyHeight).y();
        GL_GPU::Color4f(userkfR, userkfG, userkfB, 1.);
        QColor userKeyColor;
        userKeyColor.setRgbF( Image::clamp<qreal>(userkfR, 0., 1.),
                              Image::clamp<qreal>(userkfG, 0., 1.),
                              Image::clamp<qreal>(userkfB, 0., 1.) );
        for (std::list<SequenceTime>::iterator it = remainingUserKeys.begin(); it != remainingUserKeys.end(); ++it) {
            if (  /* (*it == currentTime) ||*/
                                              ( *it == leftBound) ||
                                              ( *it == rightBound) ) {
                continue;
            }
            QPointF kfBtm(*it, lineYpos);
            QPointF kfBtmWidgetCoord = toWidgetCoordinates( kfBtm.x(), kfBtm.y() );
            QPointF kfTopLeft = toTimeLineCoordinates(kfBtmWidgetCoord.x() - cursorWidth / 2.,
                                                      kfBtmWidgetCoord.y() - cursorHeight);
            QPointF kfTopRight = toTimeLineCoordinates(kfBtmWidgetCoord.x() + cursorWidth / 2.,
                                                       kfBtmWidgetCoord.y() - cursorHeight);
            QPointF kfBtmLeft = toTimeLineCoordinates(kfBtmWidgetCoord.x() - cursorWidth / 2.,
                                                      kfBtmWidgetCoord.y() + cursorHeight);
            QPointF kfBtmRight = toTimeLineCoordinates(kfBtmWidgetCoord.x() + cursorWidth / 2.,
                                                       kfBtmWidgetCoord.y() + cursorHeight);
            GL_GPU::Begin(GL_POLYGON);
            GL_GPU::Vertex2f( kfBtm.x(), cursorBtm.y() );
            GL_GPU::Vertex2f( kfTopLeft.x(), kfTopLeft.y() );
            GL_GPU::Vertex2f( kfTopRight.x(), kfTopRight.y() );
            GL_GPU::End();
            GL_GPU::Begin(GL_POLYGON);
            GL_GPU::Vertex2f( kfBtm.x(), cursorBtm.y() );
            GL_GPU::Vertex2f( kfBtmLeft.x(), kfBtmLeft.y() );
            GL_GPU::Vertex2f( kfBtmRight.x(), kfBtmRight.y() );
            GL_GPU::End();

            /*QString kfStr = QString::number(*it);
               double kfXposWidget = kfBtmWidgetCoord.x() - fontM.width(kfStr) / 2.;
               double kfTextPos = toTimeLine(kfXposWidget);
               renderText(kfTextPos,kfTopLeft.y(), kfStr, userKeyColor, _imp->font, Qt::AlignHCenter);*/
        }
        glCheckErrorIgnoreOSXBug(GL_GPU);
        GL_GPU::Disable(GL_POLYGON_SMOOTH);
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_SCISSOR_BIT | GL_TRANSFORM_BIT);

    glCheckError(GL_GPU);
} // paintGL

void
TimeLineGui::renderText(double x,
                        double y,
                        const QString & text,
                        const QColor & color,
                        const QFont & font,
                        int flags) const
{
    assert( QGLContext::currentContext() == context() );

    glCheckError(GL_GPU);
    if ( text.isEmpty() ) {
        return;
    }

    double w = (double)width();
    double h = (double)height();
    double bottom = _imp->tlZoomCtx.bottom;
    double left = _imp->tlZoomCtx.left;
    double top = bottom +  h / (double)_imp->tlZoomCtx.zoomFactor;
    double right = left +  (w / (double)_imp->tlZoomCtx.zoomFactor);
    if ( (w <= 0) || (h <= 0) || (right <= left) || (top <= bottom) ) {
        return;
    }
    double scalex = (right - left) / w;
    double scaley = (top - bottom) / h;
    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font, flags);
    glCheckError(GL_GPU);
}

void
TimeLineGui::onFrameChanged(SequenceTime,
                            int reason)
{
    TimelineChangeReasonEnum r = (TimelineChangeReasonEnum)reason;

    if ( (r == eTimelineChangeReasonUserSeek) && _imp->seekingTimeline ) {
        return;
    }
    update();
}

void
TimeLineGui::seek(SequenceTime time)
{
    if ( time != _imp->timeline->currentFrame() ) {
        ViewerNodePtr viewer = _imp->viewer.lock();
        _imp->gui->getApp()->setLastViewerUsingTimeline( viewer->getNode() );
        _imp->seekingTimeline = true;
        _imp->timeline->onFrameChanged(time);
        _imp->seekingTimeline = false;
        update();
    }
}

void
TimeLineGui::mousePressEvent(QMouseEvent* e)
{
    _imp->mousePressX = e->x();
    _imp->mouseMoveX = _imp->mousePressX;
    if ( buttonDownIsMiddle(e) ) {
        _imp->state = eTimelineStatePanning;
    } else if ( buttonDownIsRight(e) ) {
        _imp->state = eTimelineStateSelectingZoomRange;
    } else {
        _imp->lastMouseEventWidgetCoord = e->pos();
        const double t = toTimeLine(_imp->mousePressX);
        SequenceTime tseq = std::floor(t + 0.5);
        if ( modCASIsControl(e) ) {
            int leftBound, rightBound;
            {
                QMutexLocker k(&_imp->boundariesMutex);
                leftBound = _imp->leftBoundary;
                rightBound = _imp->rightBoundary;
            }
            _imp->state = eTimelineStateDraggingBoundary;
            int firstPos = toWidget(leftBound - 1);
            int lastPos = toWidget(rightBound + 1);
            int distFromFirst = std::abs(e->x() - firstPos);
            int distFromLast = std::abs(e->x() - lastPos);
            if (distFromFirst  > distFromLast) {
                setBoundariesInternal(leftBound, tseq, true); // moving last frame anchor
            } else {
                setBoundariesInternal( tseq, rightBound, true );   // moving first frame anchor
            }
        } else {
            _imp->state = eTimelineStateDraggingCursor;
            seek(tseq);
        }
    }
}

void
TimeLineGui::mouseMoveEvent(QMouseEvent* e)
{
    int mouseMoveXprev = _imp->mouseMoveX;

    _imp->lastMouseEventWidgetCoord = e->pos();
    _imp->mouseMoveX = e->x();
    const double t = toTimeLine(_imp->mouseMoveX);
    SequenceTime tseq = std::floor(t + 0.5);
    bool distortViewPort = false;
    bool onEditingFinishedOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
    if (_imp->state == eTimelineStatePanning) {
        _imp->tlZoomCtx.left += toTimeLine(mouseMoveXprev) - toTimeLine(_imp->mouseMoveX);
        update();
        if ( _imp->gui->isTripleSyncEnabled() ) {
            _imp->updateEditorFrameRanges();
            _imp->updateOpenedViewersFrameRanges();
        }
    } else if (_imp->state == eTimelineStateSelectingZoomRange) {
        // https://github.com/MrKepzie/Natron/issues/917
        update();
    } else if ( (_imp->state == eTimelineStateDraggingCursor) && !onEditingFinishedOnly ) {
        if ( tseq != _imp->timeline->currentFrame() ) {
            ViewerNodePtr viewer = _imp->viewer.lock();
            _imp->gui->setDraftRenderEnabled(true);
            _imp->gui->getApp()->setLastViewerUsingTimeline( viewer->getNode() );
            _imp->seekingTimeline = true;
            _imp->timeline->onFrameChanged(tseq);
            _imp->seekingTimeline = false;
        }
        distortViewPort = true;
        _imp->alphaCursor = false;
    } else if (_imp->state == eTimelineStateDraggingBoundary) {
        int leftBound, rightBound;
        {
            QMutexLocker k(&_imp->boundariesMutex);
            leftBound = _imp->leftBoundary;
            rightBound = _imp->rightBoundary;
        }
        int firstPos = toWidget(leftBound - 1);
        int lastPos = toWidget(rightBound + 1);
        int distFromFirst = std::abs(e->x() - firstPos);
        int distFromLast = std::abs(e->x() - lastPos);
        if (distFromFirst  > distFromLast) { // moving last frame anchor
            if (leftBound <= tseq) {
                setBoundariesInternal(leftBound, tseq, true);
            }
        } else { // moving first frame anchor
            if (rightBound >= tseq) {
                setBoundariesInternal(tseq, rightBound, true);
            }
        }
        distortViewPort = true;
        _imp->alphaCursor = false;
    } else {
        _imp->alphaCursor = true;
    }

    if (distortViewPort) {
        double leftMost = toTimeLine(0);
        double rightMost = toTimeLine(width() - 1);
        double delta = (rightMost - leftMost) * 0.02;
        if (tseq < leftMost) {
            centerOn(leftMost - delta, rightMost - delta, 0);
        } else if (tseq > rightMost) {
            centerOn(leftMost + delta, rightMost + delta, 0);
        } else {
            update();
        }
    } else {
        update();
    }
} // TimeLineGui::mouseMoveEvent

void
TimeLineGui::enterEvent(QEvent* e)
{
    _imp->alphaCursor = true;
    update();
    QGLWidget::enterEvent(e);
}

void
TimeLineGui::leaveEvent(QEvent* e)
{
    _imp->alphaCursor = false;
    update();
    QGLWidget::leaveEvent(e);
}

void
TimeLineGui::mouseReleaseEvent(QMouseEvent* e)
{
    if (_imp->state == eTimelineStateSelectingZoomRange) {
        // - if the last selected frame is the same as the first selected frame, zoom on the PROJECT range
        //   (NOT the playback range as in the following, and NOT adding margins as centerOn() does)
        // - if they are different, zoom on that range
        double t = toTimeLine( e->x() );
        int leftBound = std::floor(t + 0.5);
        int rightBound = std::floor(toTimeLine(_imp->mousePressX) + 0.5);
        if (leftBound > rightBound) {
            std::swap(leftBound, rightBound);
        } else if (leftBound == rightBound) {
            if ( !_imp->viewerTab->isFileDialogViewer() ) {
                TimeValue firstFrame, lastFrame;
                _imp->gui->getApp()->getProject()->getFrameRange(&firstFrame, &lastFrame);
                leftBound = std::floor(firstFrame + 0.5);
                rightBound = std::floor(lastFrame + 0.5);
            } else {
                _imp->viewerTab->getTimelineBounds(&leftBound, &rightBound);
            }
        }

        centerOn(leftBound, rightBound, 0);

        if ( _imp->gui->isTripleSyncEnabled() ) {
            _imp->updateEditorFrameRanges();
            _imp->updateOpenedViewersFrameRanges();
        }
    } else if (_imp->state == eTimelineStateDraggingCursor) {
        bool wasScrubbing = false;
        if ( _imp->gui->isDraftRenderEnabled() ) {
            _imp->gui->setDraftRenderEnabled(false);
            wasScrubbing = true;
        }
        _imp->gui->refreshAllPreviews();

        SettingsPtr settings = appPTR->getCurrentSettings();
        bool onEditingFinishedOnly = settings->getRenderOnEditingFinishedOnly();
        bool autoProxyEnabled = settings->isAutoProxyEnabled();


        if (onEditingFinishedOnly) {
            double t = toTimeLine( e->x() );
            SequenceTime tseq = std::floor(t + 0.5);
            if ( ( tseq != _imp->timeline->currentFrame() ) ) {
                ViewerNodePtr viewer = _imp->viewer.lock();
                _imp->gui->getApp()->setLastViewerUsingTimeline( viewer->getNode() );
                _imp->timeline->onFrameChanged(tseq);
            }
        } else if (autoProxyEnabled && wasScrubbing) {
            _imp->gui->getApp()->renderAllViewers();
        }
    }

    _imp->state = eTimelineStateIdle;
    QGLWidget::mouseReleaseEvent(e);
} // TimeLineGui::mouseReleaseEvent

void
TimeLineGui::wheelEvent(QWheelEvent* e)
{
    if (e->orientation() != Qt::Vertical) {
        return;
    }
    const double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    double newZoomFactor = _imp->tlZoomCtx.zoomFactor * scaleFactor;
    if (newZoomFactor <= 0.01) { // 1 pixel for 100 frames
        newZoomFactor = 0.01;
    } else if (newZoomFactor > 100.) { // 100 pixels per frame seems reasonable, see also DopeSheetView::wheelEvent()
        newZoomFactor = 100.;
    }
    QPointF zoomCenter = toTimeLineCoordinates( e->x(), e->y() );
    double zoomRatio =   _imp->tlZoomCtx.zoomFactor / newZoomFactor;
    _imp->tlZoomCtx.left = zoomCenter.x() - (zoomCenter.x() - _imp->tlZoomCtx.left) * zoomRatio;
    _imp->tlZoomCtx.bottom = zoomCenter.y() - (zoomCenter.y() - _imp->tlZoomCtx.bottom) * zoomRatio;

    _imp->tlZoomCtx.zoomFactor = newZoomFactor;

    update();

    if ( _imp->gui->isTripleSyncEnabled() ) {
        _imp->updateEditorFrameRanges();
        _imp->updateOpenedViewersFrameRanges();
    }
}

void
TimeLineGui::setBoundariesInternal(SequenceTime first,
                                   SequenceTime last,
                                   bool emitSignal)
{
    if (first <= last) {
        {
            QMutexLocker k(&_imp->boundariesMutex);
            _imp->leftBoundary = first;
            _imp->rightBoundary = last;
        }
        if (emitSignal) {
            Q_EMIT boundariesChanged(first, last);
        } else {
            update();
        }
        setFrameRangeEdited(true);
    }
}

void
TimeLineGui::setBoundaries(SequenceTime first,
                           SequenceTime last)
{
    setBoundariesInternal(first, last, false);
}

void
TimeLineGui::recenterOnBounds()
{
    SequenceTime first, last;

    getBounds(&first, &last);
    centerOn(first, last);
}

void
TimeLineGui::centerOn_tripleSync(SequenceTime left,
                                 SequenceTime right)
{
    double curveWidth = right - left;
    double w = width();

    _imp->tlZoomCtx.left = left;
    _imp->tlZoomCtx.zoomFactor = w / curveWidth;

    update();
}

void
TimeLineGui::getVisibleRange(SequenceTime* left,
                             SequenceTime* right) const
{
    *left = _imp->tlZoomCtx.left;
    double w = width();
    double curveWidth = w / _imp->tlZoomCtx.zoomFactor;
    *right = *left + curveWidth;
}

void
TimeLineGui::centerOn(SequenceTime left,
                      SequenceTime right,
                      int margin)
{
    double curveWidth = right - left + 2 * margin;
    double w = width();

    _imp->tlZoomCtx.left = left - margin;
    _imp->tlZoomCtx.zoomFactor = w / curveWidth;

    update();
}

SequenceTime
TimeLineGui::leftBound() const
{
    QMutexLocker k(&_imp->boundariesMutex);

    return _imp->leftBoundary;
}

SequenceTime
TimeLineGui::rightBound() const
{
    QMutexLocker k(&_imp->boundariesMutex);

    return _imp->rightBoundary;
}

void
TimeLineGui::getBounds(SequenceTime* left,
                       SequenceTime* right) const
{
    QMutexLocker k(&_imp->boundariesMutex);

    *left = _imp->leftBoundary;
    *right = _imp->rightBoundary;
}

SequenceTime
TimeLineGui::currentFrame() const
{
    return _imp->timeline->currentFrame();
}

double
TimeLineGui::toTimeLine(double x) const
{
    double w = (double)width();
    double left = _imp->tlZoomCtx.left;
    double right = left +  w / _imp->tlZoomCtx.zoomFactor;

    return ( ( (right - left) * x ) / w ) + left;
}

double
TimeLineGui::toWidget(double t) const
{
    double w = (double)width();
    double left = _imp->tlZoomCtx.left;
    double right = left +  w / _imp->tlZoomCtx.zoomFactor;

    return ( (t - left) / (right - left) ) * w;
}

QPointF
TimeLineGui::toTimeLineCoordinates(double x,
                                   double y) const
{
    double h = (double)height();
    double bottom = _imp->tlZoomCtx.bottom;
    double top =  bottom +  h / _imp->tlZoomCtx.zoomFactor;

    return QPointF( toTimeLine(x), ( ( (bottom - top) * y ) / h ) + top );
}

QPointF
TimeLineGui::toWidgetCoordinates(double x,
                                 double y) const
{
    double h = (double)height();
    double bottom = _imp->tlZoomCtx.bottom;
    double top =  bottom +  h / _imp->tlZoomCtx.zoomFactor;

    return QPoint( toWidget(x), ( (y - top) / (bottom - top) ) * h );
}

bool
TimeLineGui::isFrameRangeEdited() const
{
    QMutexLocker k(&_imp->frameRangeEditedMutex);

    return _imp->isFrameRangeEdited;
}

void
TimeLineGui::setFrameRangeEdited(bool edited)
{
    QMutexLocker k(&_imp->frameRangeEditedMutex);

    _imp->isFrameRangeEdited = edited;
}

void
TimeLineGui::refreshCachedFramesNow()
{
    assert(QThread::currentThread() == qApp->thread());

    CachePtr cache = appPTR->getCache();

    std::list<CacheEntryBasePtr> cachedEntries;
    cache->getAllEntriesByKeyIDWithCacheSignalEnabled(kCacheKeyUniqueIDImageTile, &cachedEntries);

    _imp->cachedFrames.clear();
    for (std::list<CacheEntryBasePtr>::const_iterator it = cachedEntries.begin(); it != cachedEntries.end(); ++it) {
        MemoryBufferedCacheEntryBase* isMemoryEntry = dynamic_cast<MemoryBufferedCacheEntryBase*>(it->get());
        assert(isMemoryEntry);
        if (!isMemoryEntry) {
            continue;
        }
        CachedFrame c((SequenceTime)isMemoryEntry->getTime(), isMemoryEntry->getStorageMode());
        _imp->cachedFrames.insert(c);
    }

    update();

} // refreshCachedFramesNow

void
TimeLineGui::onRefreshCachedFramesLaterReceived()
{
    if (!_imp->nRefreshCacheFrameRequests) {
        return;
    }
    _imp->nRefreshCacheFrameRequests = 0;
    refreshCachedFramesNow();
}

void
TimeLineGui::refreshCachedFramesLater()
{
    ++_imp->nRefreshCacheFrameRequests;
    Q_EMIT refreshCachedFramesLaterReceived();
}

void
TimeLineGui::onCacheStatusChanged()
{
    refreshCachedFramesLater();
} // onCacheStatusChanged

void
TimeLineGui::onProjectFrameRangeChanged(int left,
                                        int right)
{
    assert(_imp->viewerTab);
    if ( _imp->viewerTab->isFileDialogViewer() ) {
        return;
    }
    if ( !isFrameRangeEdited() ) {
        setBoundariesInternal(left, right, true);
        setFrameRangeEdited(false);
        centerOn(left, right);
    }
    update();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_TimeLineGui.cpp"
