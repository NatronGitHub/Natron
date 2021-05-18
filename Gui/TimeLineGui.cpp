/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"

#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/TextRenderer.h"
#include "Gui/ViewerTab.h"
#include "Gui/ticks.h"

NATRON_NAMESPACE_ENTER

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

static
QString
timecodeString(double value, double fps)
{
    QString sign;
    if (value < 0) {
        value = -value;
        sign = QLatin1Char('-');
    }
    long rvalue = (long)value;
    int rfps = (int)std::ceil(fps);

    int f = rvalue % rfps;
    rvalue /= rfps;
    int s = rvalue % 60;
    rvalue /= 60;
    int m = rvalue % 60;
    int h = rvalue / 60;
    return sign + QString::fromUtf8("%1:%2:%3:%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(f, 2, 10, QLatin1Char('0'));
}

NATRON_NAMESPACE_ANONYMOUS_EXIT


struct TimelineGuiPrivate
{
    TimeLineGui *parent;
    ViewerInstance* viewer;
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
    bool isTimeFormatFrames;

    // Use a timer to refresh the timeline on a timed basis rather than for each change
    // so that we limit the amount of redraws
    QTimer keyframeChangesUpdateTimer;

    TimelineGuiPrivate(TimeLineGui *qq,
                       ViewerInstance* viewer,
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
        , isTimeFormatFrames(true)
        , keyframeChangesUpdateTimer()
    {
    }

    void updateEditorFrameRanges()
    {
        double zoomRight = parent->toTimeLine(parent->width() - 1);

        gui->getCurveEditor()->getCurveWidget()->centerOn(tlZoomCtx.left - 5, zoomRight - 5);
        gui->getDopeSheetEditor()->centerOn(tlZoomCtx.left - 5, zoomRight - 5);
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

    void startKeyframeChangesTimer()
    {
        if (!keyframeChangesUpdateTimer.isActive()) {
            keyframeChangesUpdateTimer.start(200);
        }
    }
};

TimeLineGui::TimeLineGui(ViewerInstance* viewer,
                         TimeLinePtr timeline,
                         Gui* gui,
                         ViewerTab* viewerTab)
    : QGLWidget(viewerTab)
    , _imp( new TimelineGuiPrivate(this, viewer, gui, viewerTab) )
{
    setTimeline(timeline);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);

    _imp->keyframeChangesUpdateTimer.setSingleShot(true);
    connect(&_imp->keyframeChangesUpdateTimer, SIGNAL(timeout()), this, SLOT(onKeyframeChangesUpdateTimerTimeout()));
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

        //connect the gui to the internal timeline
        QObject::disconnect( app.get(), SIGNAL(keyframeIndicatorsChanged()), this, SLOT(onKeyframesIndicatorsChanged()) );
    }

    //connect the internal timeline to the gui
    QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onFrameChanged(SequenceTime,int)), Qt::UniqueConnection );
    QObject::connect( app.get(), SIGNAL(keyframeIndicatorsChanged()), this, SLOT(onKeyframesIndicatorsChanged()), Qt::UniqueConnection );

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

    if (width == 0) {
        width = 1;
    }
    if (height == 0) {
        height = 1;
    }
    glViewport (0, 0, width, height);
}

void
TimeLineGui::discardGuiPointer()
{
    _imp->gui = 0;
}

struct TimeLineKey
{
    SequenceTime time;
    bool isUserKey;

    explicit TimeLineKey(SequenceTime t,
                         bool isUserKey = false)
        : time(t)
        , isUserKey(isUserKey)
    {
    }

    explicit TimeLineKey()
        : time(0)
        , isUserKey(false)
    {
    }
};

struct TimeLineKey_compare
{
    bool operator() (const TimeLineKey& lhs,
                     const TimeLineKey& rhs) const
    {
        return lhs.time < rhs.time;
    }
};

typedef std::set<TimeLineKey, TimeLineKey_compare> TimeLineKeysSet;

void
TimeLineGui::paintGL()
{
    if (!_imp->gui) {
        return;
    }
    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }


    glCheckError();

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
        glClearColor(clearR, clearG, clearB, 1.);
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug();

        return;
    }

    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_SCISSOR_BIT | GL_TRANSFORM_BIT);
        //GLProtectMatrix p(GL_PROJECTION); // no need to protect
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(left, right, bottom, top, 1, -1);
        //GLProtectMatrix m(GL_MODELVIEW); // no need to protect
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glClearColor(clearR, clearG, clearB, 1.);
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug();

        QPointF btmLeft = toTimeLineCoordinates(0, height() - 1);
        QPointF topRight = toTimeLineCoordinates(width() - 1, 0);


        /// change the background color of the portion of the timeline where images are lying
        double firstFrame, lastFrame;
        if ( !_imp->viewerTab->isFileDialogViewer() ) {
            _imp->gui->getApp()->getFrameRange(&firstFrame, &lastFrame);
        } else {
            int f, l;
            _imp->viewerTab->getTimelineBounds(&f, &l);
            firstFrame = (double)f;
            lastFrame = (double)l;
        }
        QPointF firstFrameWidgetPos = toWidgetCoordinates(firstFrame, 0);
        QPointF lastFrameWidgetPos = toWidgetCoordinates(lastFrame, 0);

        glScissor( firstFrameWidgetPos.x(), 0,
                   lastFrameWidgetPos.x() - firstFrameWidgetPos.x(), height() );

        double bgR, bgG, bgB;
        settings->getBaseColor(&bgR, &bgG, &bgB);

        glEnable(GL_SCISSOR_TEST);
        glClearColor(bgR, bgG, bgB, 1.);
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug();
        glDisable(GL_SCISSOR_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (_imp->state == eTimelineStateSelectingZoomRange) {
            // draw timeline selected range
            // https://github.com/MrKepzie/Natron/issues/917
            // draw the select range, from _imp->mousePressX to _imp->mouseMoveX
            glColor4f(1, 1, 1, 0.3);
            glBegin(GL_POLYGON);
            glVertex2f( toTimeLine(_imp->mousePressX), btmLeft.y() );
            glVertex2f( toTimeLine(_imp->mousePressX), topRight.y() );
            glVertex2f( toTimeLine(_imp->mouseMoveX), topRight.y() );
            glVertex2f( toTimeLine(_imp->mouseMoveX), btmLeft.y() );
            glEnd();
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


        glColor4f(txtR / 2., txtG / 2., txtB / 2., 1.);
        glBegin(GL_LINES);
        glVertex2f(btmLeft.x(), lineYpos);
        glVertex2f(topRight.x(), lineYpos);
        glEnd();
        glCheckErrorIgnoreOSXBug();

#if (__GNUC__ == 8 && __GNUC_MINOR__ >= 1 && __GNUC_MINOR__ < 3) && !defined(ENFORCE_GCC8)
#error "Timeline GUI is wrong with GCC 8.1 (and maybe GCC 8.2), see https://github.com/NatronGitHub/Natron/issues/279"
#endif
        double tickBottom = toTimeLineCoordinates( 0, height() - 1 - fontM.height() ).y();
        double tickTop = toTimeLineCoordinates( 0, height() - 1 - fontM.height()  - TO_DPIY(TICK_HEIGHT) ).y();
        const double smallestTickSizePixel = 30.; // tick size (in pixels) for alpha = 0.
        const double largestTickSizePixel = 200.; // tick size (in pixels) for alpha = 1.
        const double rangePixel =  width();
        const double range_min = btmLeft.x();
        const double range_max =  topRight.x();
        const double range = range_max - range_min;
        const double fps = _imp->gui->getApp()->getProjectFrameRate();
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
        const double minTickSizeTextPixel = _imp->isTimeFormatFrames ? fontM.width( QLatin1String("00000") ) : fontM.width( QLatin1String("00:00:00:00") ); // AXIS-SPECIFIC
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
            glColor4f(txtR, txtG, txtB, alpha);

            glBegin(GL_LINES);
            glVertex2f(value, tickBottom);
            glVertex2f(value, tickTop);
            glEnd();
            glCheckErrorIgnoreOSXBug();

            if (tickSize > minTickSizeText) {
                const int tickSizePixel = rangePixel * tickSize / range;
                const QString s = _imp->isTimeFormatFrames ? QString::number(value) : timecodeString(value, fps);
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
                    glCheckError();
                    renderText(value, btmLeft.y(), s, c, _imp->font, Qt::AlignHCenter);
                }
            }
        }
        glCheckError();

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
        TimeLineKeysSet keyframes;

        {
            std::list<SequenceTime> userKeys;
            _imp->gui->getApp()->getUserKeyframes(&userKeys);
            for (std::list<SequenceTime>::iterator it = userKeys.begin(); it != userKeys.end(); ++it) {
                TimeLineKey k(*it, true);
                keyframes.insert(k);
            }

            ///THere may be duplicates in this list
            std::list<SequenceTime> keyframesList;
            _imp->gui->getApp()->getKeyframes(&keyframesList);
            for (std::list<SequenceTime>::iterator it = keyframesList.begin(); it != keyframesList.end(); ++it) {
                TimeLineKey k(*it, false);
                keyframes.insert(k);
            }
        }


        //draw an alpha cursor if the mouse is hovering the timeline
        glEnable(GL_POLYGON_SMOOTH);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
        glCheckError();
        if (_imp->alphaCursor) {
            int currentPosBtmWidgetCoordX = _imp->lastMouseEventWidgetCoord.x();
            int currentPosBtmWidgetCoordY = toWidgetCoordinates(0, lineYpos).y();
            QPointF currentPosBtm = toTimeLineCoordinates(currentPosBtmWidgetCoordX, currentPosBtmWidgetCoordY);
            int hoveredTime = std::floor(currentPosBtm.x() + 0.5);
            QPointF currentBtm(hoveredTime, lineYpos);
            QPointF currentBtmWidgetCoord = toWidgetCoordinates( currentBtm.x(), currentBtm.y() );
            QPointF currentTopLeft = toTimeLineCoordinates(currentBtmWidgetCoord.x() - cursorWidth / 2.,
                                                           currentBtmWidgetCoord.y() - cursorHeight);
            QPointF currentTopRight = toTimeLineCoordinates(currentBtmWidgetCoord.x() + cursorWidth / 2.,
                                                            currentBtmWidgetCoord.y() - cursorHeight);

            QString currentNumber( _imp->isTimeFormatFrames ? QString::number(hoveredTime) : timecodeString(hoveredTime, fps) );
            TimeLineKeysSet::iterator foundHoveredAsKeyframe = keyframes.find( TimeLineKey(hoveredTime) );
            QColor currentColor;
            if ( foundHoveredAsKeyframe != keyframes.end() ) {
                if (foundHoveredAsKeyframe->isUserKey) {
                    glColor4f(userkfR, userkfG, userkfB, 0.4);
                    currentColor.setRgbF( Image::clamp<qreal>(userkfR, 0., 1.),
                                          Image::clamp<qreal>(userkfG, 0., 1.),
                                          Image::clamp<qreal>(userkfB, 0., 1.) );
                } else {
                    glColor4f(kfR, kfG, kfB, 0.4);
                    currentColor.setRgbF( Image::clamp<qreal>(kfR, 0., 1.),
                                          Image::clamp<qreal>(kfG, 0., 1.),
                                          Image::clamp<qreal>(kfB, 0., 1.) );
                }
            } else {
                glColor4f(cursorR, cursorG, cursorB, 0.4);
                currentColor.setRgbF( Image::clamp<qreal>(cursorR, 0., 1.),
                                      Image::clamp<qreal>(cursorG, 0., 1.),
                                      Image::clamp<qreal>(cursorB, 0., 1.) );
            }
            currentColor.setAlpha(100);


            glBegin(GL_POLYGON);
            glVertex2f( currentBtm.x(), currentBtm.y() );
            glVertex2f( currentTopLeft.x(), currentTopLeft.y() );
            glVertex2f( currentTopRight.x(), currentTopRight.y() );
            glEnd();
            glCheckErrorIgnoreOSXBug();

            renderText(currentBtm.x(), currentTopRight.y(), currentNumber, currentColor, _imp->font, Qt::AlignHCenter);
        }

        //draw the bounds and the current time cursor
        TimeLineKeysSet::iterator isCurrentTimeAKeyframe = keyframes.find( TimeLineKey(currentTime) );
        QColor actualCursorColor;
        if ( isCurrentTimeAKeyframe != keyframes.end() ) {
            if (isCurrentTimeAKeyframe->isUserKey) {
                glColor4f(userkfR, userkfG, userkfB, 1.);
                actualCursorColor.setRgbF( Image::clamp<qreal>(userkfR, 0., 1.),
                                           Image::clamp<qreal>(userkfG, 0., 1.),
                                           Image::clamp<qreal>(userkfB, 0., 1.) );
            } else {
                glColor4f(kfR, kfG, kfB, 1.);
                actualCursorColor.setRgbF( Image::clamp<qreal>(kfR, 0., 1.),
                                           Image::clamp<qreal>(kfG, 0., 1.),
                                           Image::clamp<qreal>(kfB, 0., 1.) );
            }
        } else {
            glColor4f(cursorR, cursorG, cursorB, 1.);
            actualCursorColor.setRgbF( Image::clamp<qreal>(cursorR, 0., 1.),
                                       Image::clamp<qreal>(cursorG, 0., 1.),
                                       Image::clamp<qreal>(cursorB, 0., 1.) );
        }

        QString currentFrameStr = _imp->isTimeFormatFrames ? QString::number(currentTime) : timecodeString(currentTime, fps);
        double cursorTextXposWidget = cursorBtmWidgetCoord.x();
        double cursorTextPos = toTimeLine(cursorTextXposWidget);
        renderText(cursorTextPos, cursorTopLeft.y(), currentFrameStr, actualCursorColor, _imp->font, Qt::AlignHCenter);
        glBegin(GL_POLYGON);
        glVertex2f( cursorBtm.x(), cursorBtm.y() );
        glVertex2f( cursorTopLeft.x(), cursorTopLeft.y() );
        glVertex2f( cursorTopRight.x(), cursorTopRight.y() );
        glEnd();
        glCheckErrorIgnoreOSXBug();

        QColor boundsColor;
        boundsColor.setRgbF( Image::clamp<qreal>(boundsR, 0., 1.),
                             Image::clamp<qreal>(boundsG, 0., 1.),
                             Image::clamp<qreal>(boundsB, 0., 1.) );


        {
            if ( ( leftBoundBtm.x() >= btmLeft.x() ) && ( leftBoundBtmRight.x() <= topRight.x() ) ) {
                if (leftBound != currentTime) {
                    QString leftBoundStr( _imp->isTimeFormatFrames ? QString::number(leftBound) : timecodeString(leftBound, fps) );
                    double leftBoundTextXposWidget = toWidgetCoordinates( ( leftBoundBtm.x() + leftBoundBtmRight.x() ) / 2., 0 ).x();
                    double leftBoundTextPos = toTimeLine(leftBoundTextXposWidget);
                    renderText(leftBoundTextPos, leftBoundTop.y(),
                               leftBoundStr, boundsColor, _imp->font, Qt::AlignHCenter);
                }
                glColor4f(boundsR, boundsG, boundsB, 1.);
                glBegin(GL_POLYGON);
                glVertex2f( leftBoundBtm.x(), leftBoundBtm.y() );
                glVertex2f( leftBoundBtmRight.x(), leftBoundBtmRight.y() );
                glVertex2f( leftBoundTop.x(), leftBoundTop.y() );
                glEnd();
                glCheckErrorIgnoreOSXBug();
            }

            if ( ( rightBoundBtmLeft.x() >= btmLeft.x() ) && ( rightBoundBtm.x() <= topRight.x() ) ) {
                if ( (rightBound != currentTime) && (rightBound != leftBound) ) {
                    QString rightBoundStr( _imp->isTimeFormatFrames ? QString::number(rightBound) : timecodeString(rightBound, fps) );
                    double rightBoundTextXposWidget = toWidgetCoordinates( ( rightBoundBtm.x() + rightBoundBtmLeft.x() ) / 2., 0 ).x();
                    double rightBoundTextPos = toTimeLine(rightBoundTextXposWidget);
                    renderText(rightBoundTextPos, rightBoundTop.y(),
                               rightBoundStr, boundsColor, _imp->font, Qt::AlignHCenter);
                }
                glColor4f(boundsR, boundsG, boundsB, 1.);
                glCheckError();
                glBegin(GL_POLYGON);
                glVertex2f( rightBoundBtm.x(), rightBoundBtm.y() );
                glVertex2f( rightBoundBtmLeft.x(), rightBoundBtmLeft.y() );
                glVertex2f( rightBoundTop.x(), rightBoundTop.y() );
                glEnd();
                glCheckErrorIgnoreOSXBug();
            }
        }


        //draw cached frames
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        glCheckError();
        glLineWidth(2);
        glCheckError();
        glBegin(GL_LINES);
        for (CachedFrames::const_iterator i = _imp->cachedFrames.begin(); i != _imp->cachedFrames.end(); ++i) {
            if ( ( i->time >= btmLeft.x() ) && ( i->time <= topRight.x() ) ) {
                if (i->mode == eStorageModeRAM) {
                    glColor4f(cachedR, cachedG, cachedB, 1.);
                } else if (i->mode == eStorageModeDisk) {
                    glColor4f(dcR, dcG, dcB, 1.);
                }
                glVertex2f(i->time, cachedLineYPos);
                glVertex2f(i->time + 1, cachedLineYPos);
            }
        }
        glEnd();

        ///now draw keyframes
        glBegin(GL_LINES);
        glColor4f(kfR, kfG, kfB, 1.);
        std::list<SequenceTime> remainingUserKeys;
        for (TimeLineKeysSet::const_iterator i = keyframes.begin(); i != keyframes.end(); ++i) {
            if ( ( i->time >= btmLeft.x() ) && ( i->time <= topRight.x() ) ) {
                if (!i->isUserKey) {
                    glVertex2f(i->time, lineYpos);
                    glVertex2f(i->time + 1, lineYpos);
                } else {
                    remainingUserKeys.push_back(i->time);
                }
            }
        }
        glEnd();

        //const double keyHeight = TO_DPIY(USER_KEYFRAMES_HEIGHT);
        //const double keyTop = toTimeLineCoordinates(0, lineYPosWidget - keyHeight).y();
        //const double keyBtm = toTimeLineCoordinates(0, lineYPosWidget + keyHeight).y();
        glColor4f(userkfR, userkfG, userkfB, 1.);
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
            glBegin(GL_POLYGON);
            glVertex2f( kfBtm.x(), cursorBtm.y() );
            glVertex2f( kfTopLeft.x(), kfTopLeft.y() );
            glVertex2f( kfTopRight.x(), kfTopRight.y() );
            glEnd();
            glBegin(GL_POLYGON);
            glVertex2f( kfBtm.x(), cursorBtm.y() );
            glVertex2f( kfBtmLeft.x(), kfBtmLeft.y() );
            glVertex2f( kfBtmRight.x(), kfBtmRight.y() );
            glEnd();

            /*QString kfStr = QString::number(*it);
               double kfXposWidget = kfBtmWidgetCoord.x() - fontM.width(kfStr) / 2.;
               double kfTextPos = toTimeLine(kfXposWidget);
               renderText(kfTextPos,kfTopLeft.y(), kfStr, userKeyColor, _imp->font, Qt::AlignHCenter);*/
        }
        glCheckErrorIgnoreOSXBug();
        glDisable(GL_POLYGON_SMOOTH);
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_SCISSOR_BIT | GL_TRANSFORM_BIT);

    glCheckError();
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

    glCheckError();
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
    glCheckError();
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
        _imp->gui->getApp()->setLastViewerUsingTimeline( _imp->viewer->getNode() );
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
            _imp->gui->setDraftRenderEnabled(true);
            _imp->gui->getApp()->setLastViewerUsingTimeline( _imp->viewer->getNode() );
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
                double firstFrame, lastFrame;
                _imp->gui->getApp()->getFrameRange(&firstFrame, &lastFrame);
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
                _imp->gui->getApp()->setLastViewerUsingTimeline( _imp->viewer->getNode() );
                _imp->timeline->onFrameChanged(tseq);
            }
        } else if (autoProxyEnabled && wasScrubbing) {
            _imp->gui->getApp()->renderAllViewers(true);
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
    const double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() ); // no need to use ipow() here, because the result is not cast to int
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

void
TimeLineGui::onKeyframesIndicatorsChanged()
{
    _imp->startKeyframeChangesTimer();
}

void
TimeLineGui::onKeyframeChangesUpdateTimerTimeout()
{
    update();
}

void
TimeLineGui::connectSlotsToViewerCache()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    CacheSignalEmitterPtr emitter = appPTR->getOrActivateViewerCacheSignalEmitter();
    QObject::connect( emitter.get(), SIGNAL(addedEntry(SequenceTime)), this, SLOT(onCachedFrameAdded(SequenceTime)) );
    QObject::connect( emitter.get(), SIGNAL(removedEntry(SequenceTime,int)), this, SLOT(onCachedFrameRemoved(SequenceTime,int)) );
    QObject::connect( emitter.get(), SIGNAL(entryStorageChanged(SequenceTime,int,int)), this,
                      SLOT(onCachedFrameStorageChanged(SequenceTime,int,int)) );
    QObject::connect( emitter.get(), SIGNAL(clearedDiskPortion()), this, SLOT(onDiskCacheCleared()) );
    QObject::connect( emitter.get(), SIGNAL(clearedInMemoryPortion()), this, SLOT(onMemoryCacheCleared()) );
}

void
TimeLineGui::disconnectSlotsFromViewerCache()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    CacheSignalEmitterPtr emitter = appPTR->getOrActivateViewerCacheSignalEmitter();
    QObject::disconnect( emitter.get(), SIGNAL(addedEntry(SequenceTime)), this, SLOT(onCachedFrameAdded(SequenceTime)) );
    QObject::disconnect( emitter.get(), SIGNAL(removedEntry(SequenceTime,int)), this, SLOT(onCachedFrameRemoved(SequenceTime,int)) );
    QObject::disconnect( emitter.get(), SIGNAL(entryStorageChanged(SequenceTime,int,int)), this,
                         SLOT(onCachedFrameStorageChanged(SequenceTime,int,int)) );
    QObject::disconnect( emitter.get(), SIGNAL(clearedDiskPortion()), this, SLOT(onDiskCacheCleared()) );
    QObject::disconnect( emitter.get(), SIGNAL(clearedInMemoryPortion()), this, SLOT(onMemoryCacheCleared()) );
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
TimeLineGui::setTimeFormatFrames(bool value)
{
    _imp->isTimeFormatFrames = value;
}

void
TimeLineGui::onCachedFrameAdded(SequenceTime time)
{
    _imp->cachedFrames.insert( CachedFrame(time, eStorageModeRAM) );
}

void
TimeLineGui::onCachedFrameRemoved(SequenceTime time,
                                  int /*storage*/)
{
    for (CachedFrames::iterator it = _imp->cachedFrames.begin(); it != _imp->cachedFrames.end(); ++it) {
        if (it->time == time) {
            _imp->cachedFrames.erase(it);
            break;
        }
    }
    _imp->startKeyframeChangesTimer();
}

void
TimeLineGui::onCachedFrameStorageChanged(SequenceTime time,
                                         int /*oldStorage*/,
                                         int newStorage)
{
    for (CachedFrames::iterator it = _imp->cachedFrames.begin(); it != _imp->cachedFrames.end(); ++it) {
        if (it->time == time) {
            _imp->cachedFrames.erase(it);
            _imp->cachedFrames.insert( CachedFrame(time, (StorageModeEnum)newStorage) );
            break;
        }
    }
}

void
TimeLineGui::onMemoryCacheCleared()
{
    CachedFrames copy;

    for (CachedFrames::iterator it = _imp->cachedFrames.begin(); it != _imp->cachedFrames.end(); ++it) {
        if (it->mode == eStorageModeDisk) {
            copy.insert(*it);
        }
    }
    _imp->cachedFrames = copy;
    _imp->startKeyframeChangesTimer();
}

void
TimeLineGui::onDiskCacheCleared()
{
    CachedFrames copy;

    for (CachedFrames::iterator it = _imp->cachedFrames.begin(); it != _imp->cachedFrames.end(); ++it) {
        if (it->mode == eStorageModeRAM) {
            copy.insert(*it);
        }
    }
    _imp->cachedFrames = copy;
    _imp->startKeyframeChangesTimer();
}

void
TimeLineGui::clearCachedFrames()
{
    _imp->cachedFrames.clear();
    _imp->startKeyframeChangesTimer();
}

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


void
TimeLineGui::onTimeFormatChanged(int value)
{
    setTimeFormatFrames(value == 1);
    update();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_TimeLineGui.cpp"
