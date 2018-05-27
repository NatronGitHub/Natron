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
#include <QFontMetrics>
#include "Global/GlobalDefines.h"

#include "Engine/Cache.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/OverlayInteractBase.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/TimeLineKeys.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"


#include "Gui/AnimationModuleEditor.h"
#include "Gui/Gui.h"
#include "Gui/CachedFramesThread.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/TextRenderer.h"
#include "Gui/ViewerTab.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/ticks.h"
#include "Gui/ZoomContext.h"


NATRON_NAMESPACE_ENTER


#define TICK_HEIGHT 7
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

#define DEFAULT_TIMELINE_LEFT_BOUND 0
#define DEFAULT_TIMELINE_RIGHT_BOUND 100

#define USER_KEYFRAMES_HEIGHT 7


NATRON_NAMESPACE_ANONYMOUS_ENTER



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
    ZoomContext zoomCtx;
    TextRenderer textRenderer;
    QFont font;
    bool firstPaint;
    mutable QMutex boundariesMutex;
    SequenceTime leftBoundary, rightBoundary;
    mutable QMutex frameRangeEditedMutex;
    bool isFrameRangeEdited;
    bool seekingTimeline;
    bool isTimeFormatFrames;

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
        , zoomCtx()
        , textRenderer()
        , font(appFont, appFontSize)
        , firstPaint(true)
        , boundariesMutex()
        , leftBoundary(0)
        , rightBoundary(0)
        , frameRangeEditedMutex()
        , isFrameRangeEdited(false)
        , seekingTimeline(false)
        , isTimeFormatFrames(true)
    {
    }

    void updateEditorFrameRanges()
    {
        gui->getAnimationModuleEditor()->centerOn(zoomCtx.left() - 5, zoomCtx.right() - 5);
    }

    void updateOpenedViewersFrameRanges()
    {
        const std::list<ViewerTab *> &viewers = gui->getViewersList();

        for (std::list<ViewerTab *>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            if ( (*it) != viewerTab ) {
                (*it)->centerOn(zoomCtx.left(), zoomCtx.right());
            }
        }
    }


    void drawOverlays(SequenceTime currentFrame);
    bool penDown(SequenceTime currentFrame, PenType pen, const QPointF & viewportPos, const QPointF & pos);
    bool penMotion(SequenceTime currentFrame, const QPointF & viewportPos, const QPointF & pos);
    bool penUp(SequenceTime currentFrame, const QPointF & viewportPos, const QPointF & pos);
    bool keyDown(SequenceTime currentFrame, Key k, KeyboardModifiers km);
    bool keyUp(SequenceTime currentFrame, Key k, KeyboardModifiers km);
    bool keyRepeat(SequenceTime currentFrame, Key k, KeyboardModifiers km);
    bool focusLost(SequenceTime currentFrame);
    bool focusGain(SequenceTime currentFrame);
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

    if (width == 0) {
        width = 1;
    }
    if (height == 0) {
        height = 1;
    }
    _imp->zoomCtx.setScreenSize(width, height, /*alignTop=*/ true, /*alignRight=*/ false);
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

    if (_imp->zoomCtx.factor() <= 0) {
        return;
    }
    //assert(_tlZoomCtx._zoomFactor <= 1024);
    double bottom = _imp->zoomCtx.bottom();
    double left = _imp->zoomCtx.left();
    double top = _imp->zoomCtx.top();
    double right = _imp->zoomCtx.right();
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

        GL_GPU::Color4f(txtR / 2., txtG / 2., txtB / 2., 1.);
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2f(btmLeft.x(), lineYpos);
        GL_GPU::Vertex2f(topRight.x(), lineYpos);
        GL_GPU::End();
        glCheckErrorIgnoreOSXBug(GL_GPU);

#if (__GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ >= 1))
#error "Timeline GUI is wrong with GCC 8.1, either fix or check that it is OK with a later version and adjust the error, see https://github.com/NatronGitHub/Natron/issues/279"
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
            GL_GPU::Color4f(txtR, txtG, txtB, alpha);

            GL_GPU::Begin(GL_LINES);
            GL_GPU::Vertex2f(value, tickBottom);
            GL_GPU::Vertex2f(value, tickTop);
            GL_GPU::End();
            glCheckErrorIgnoreOSXBug(GL_GPU);

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
            QString mouseNumber( _imp->isTimeFormatFrames ? QString::number(hoveredTime) : timecodeString(hoveredTime, fps) );
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

        QString currentFrameStr = _imp->isTimeFormatFrames ? QString::number(currentTime) : timecodeString(currentTime, fps);
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
                    QString leftBoundStr( _imp->isTimeFormatFrames ? QString::number(leftBound) : timecodeString(leftBound, fps) );
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
                    QString rightBoundStr( _imp->isTimeFormatFrames ? QString::number(rightBound) : timecodeString(rightBound, fps) );
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

        std::list<TimeValue> cachedFrames;
        _imp->viewerTab->getTimeLineCachedFrames(&cachedFrames);
        for (std::list<TimeValue>::const_iterator i = cachedFrames.begin(); i != cachedFrames.end(); ++i) {
            if ( ( *i >= btmLeft.x() ) && ( *i <= topRight.x() ) ) {
                GL_GPU::Color4f(cachedR, cachedG, cachedB, 1.);

                GL_GPU::Vertex2f((int)*i, cachedLineYPos);
                GL_GPU::Vertex2f((int)*i + 1, cachedLineYPos);
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
        _imp->drawOverlays(currentTime);

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
    double bottom = _imp->zoomCtx.bottom();
    double left = _imp->zoomCtx.left();
    double top = _imp->zoomCtx.top();
    double right = _imp->zoomCtx.right();
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


    PenType pen = ePenTypeLMB;
    if ( (e->buttons() == Qt::LeftButton) && !(e->modifiers() &  Qt::MetaModifier) ) {
        pen = ePenTypeLMB;
    } else if ( (e->buttons() == Qt::RightButton) || (e->buttons() == Qt::LeftButton  && (e->modifiers() &  Qt::MetaModifier)) ) {
        pen = ePenTypeRMB;
    } else if ( (e->buttons() == Qt::MiddleButton)  || (e->buttons() == Qt::LeftButton  && (e->modifiers() &  Qt::AltModifier)) ) {
        pen = ePenTypeMMB;
    }
    QPointF zoomPos = _imp->zoomCtx.toZoomCoordinates(e->x(), e->y());
    if (_imp->penDown(toTimeLine(_imp->mouseMoveX), pen, e->pos(), zoomPos)) {
        update();
        return;
    }

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

    QPointF timelinePos = _imp->zoomCtx.toZoomCoordinates(e->x(), e->y());
    const double t = timelinePos.x();
    SequenceTime tseq = std::floor(t + 0.5);
    bool distortViewPort = false;
    bool onEditingFinishedOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
    if (_imp->state == eTimelineStatePanning) {
        _imp->zoomCtx.translate(toTimeLine(mouseMoveXprev) - toTimeLine(_imp->mouseMoveX), 0);
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
    } else if (_imp->penMotion(t, e->pos(), timelinePos)) {
        update();
        return;
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
TimeLineGui::focusInEvent(QFocusEvent* e)
{

    if (_imp->focusGain(_imp->timeline->currentFrame())) {
        update();
    }
    QGLWidget::focusInEvent(e);
}

void
TimeLineGui::focusOutEvent(QFocusEvent* e)
{
    if (_imp->focusLost(_imp->timeline->currentFrame())) {
        update();
    }
    QGLWidget::focusOutEvent(e);
}

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
    QPointF timelinePos = _imp->zoomCtx.toZoomCoordinates(e->x(), e->y());
    double t = timelinePos.x();

    if (_imp->state == eTimelineStateSelectingZoomRange) {
        // - if the last selected frame is the same as the first selected frame, zoom on the PROJECT range
        //   (NOT the playback range as in the following, and NOT adding margins as centerOn() does)
        // - if they are different, zoom on that range
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
    } else {
        if (_imp->penUp(t, e->pos(), timelinePos)) {
            update();
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


    const double par_min = 0.0001;
    const double par_max = 10000.;

    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );

    QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );


    // Alt + Wheel: zoom time only, keep point under mouse
    double par = _imp->zoomCtx.aspectRatio() * scaleFactor;
    if (par <= par_min) {
        par = par_min;
        scaleFactor = par / _imp->zoomCtx.aspectRatio();
    } else if (par > par_max) {
        par = par_max;
        scaleFactor = par / _imp->zoomCtx.factor();
    }


    _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);


    update();

    if ( _imp->gui->isTripleSyncEnabled() ) {
        _imp->updateEditorFrameRanges();
        _imp->updateOpenedViewersFrameRanges();
    }
}

void
TimeLineGui::keyPressEvent(QKeyEvent* e)
{
    Qt::KeyboardModifiers qMods = e->modifiers();
    Qt::Key qKey = (Qt::Key)Gui::handleNativeKeys( e->key(), e->nativeScanCode(), e->nativeVirtualKey() );



    Key natronKey = QtEnumConvert::fromQtKey(qKey);
    KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers(qMods);

    if (e->isAutoRepeat()) {
        if (_imp->keyRepeat(_imp->timeline->currentFrame(), natronKey, natronMod)) {
            update();
            return;
        }
    } else {
        if (_imp->keyDown(_imp->timeline->currentFrame(), natronKey, natronMod)) {
            update();
            return;
        }
    }

    QGLWidget::keyPressEvent(e);

}

void
TimeLineGui::keyReleaseEvent(QKeyEvent* e)
{
    Qt::KeyboardModifiers qMods = e->modifiers();
    Qt::Key qKey = (Qt::Key)Gui::handleNativeKeys( e->key(), e->nativeScanCode(), e->nativeVirtualKey() );

    Key natronKey = QtEnumConvert::fromQtKey(qKey );
    KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers(qMods);


    if (_imp->keyUp(_imp->timeline->currentFrame(), natronKey, natronMod)) {
        update();
        return;
    }

    QGLWidget::keyReleaseEvent(e);

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

    _imp->zoomCtx.fill( left, right, _imp->zoomCtx.bottom(), _imp->zoomCtx.top() );

    update();
}

void
TimeLineGui::getVisibleRange(SequenceTime* left,
                             SequenceTime* right) const
{
    *left = _imp->zoomCtx.left();
    *right = _imp->zoomCtx.right();
}

void
TimeLineGui::centerOn(SequenceTime left,
                      SequenceTime right,
                      int margin)
{
    _imp->zoomCtx.fill( left - margin, right + margin, _imp->zoomCtx.bottom(), _imp->zoomCtx.top() );

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
    return _imp->zoomCtx.toZoomCoordinates(x, 0).x();
}

double
TimeLineGui::toWidget(double t) const
{
    return _imp->zoomCtx.toWidgetCoordinates(t, 0).x();
}

QPointF
TimeLineGui::toTimeLineCoordinates(double x,
                                   double y) const
{
    return _imp->zoomCtx.toZoomCoordinates(x, y);

}

QPointF
TimeLineGui::toWidgetCoordinates(double x,
                                 double y) const
{
    return _imp->zoomCtx.toWidgetCoordinates(x, y);
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


void TimeLineGui::toWidgetCoordinates(double *x, double *y) const {
    QPointF ret = _imp->zoomCtx.toWidgetCoordinates(*x, *y);
    *x = ret.x();
    *y = ret.y();
}

void TimeLineGui::toCanonicalCoordinates(double *x, double *y) const {
    QPointF ret = _imp->zoomCtx.toZoomCoordinates(*x, *y);
    *x = ret.x();
    *y = ret.y();
}

int TimeLineGui::getWidgetFontHeight() const {
    return fontMetrics().height();
}

int TimeLineGui::getStringWidthForCurrentFont(const std::string& string) const {
    return fontMetrics().width(QString::fromUtf8(string.c_str()));
}

RectD TimeLineGui::getViewportRect() const {
    RectD bbox;
    {
        bbox.x1 = _imp->zoomCtx.left();
        bbox.y1 = _imp->zoomCtx.bottom();
        bbox.x2 = _imp->zoomCtx.right();
        bbox.y2 = _imp->zoomCtx.top();
    }

    return bbox;
}

void TimeLineGui::getCursorPosition(double& x, double& y) const {
    QPoint p = QCursor::pos();

    p = mapFromGlobal(p);
    QPointF mappedPos = _imp->zoomCtx.toZoomCoordinates(p.x(), p.y());
    x = mappedPos.x();
    y = mappedPos.y();
}

RangeD TimeLineGui::getFrameRange() const {
    RangeD ret;
    SequenceTime min,max;
    getBounds(&min, &max);
    ret.min = min;
    ret.max = max;
    return ret;
}

bool TimeLineGui::renderText(double x,
                        double y,
                        const std::string &string,
                        double r,
                        double g,
                        double b,
                        double a,
                        int flags) {
    QColor c;
    c.setRgbF( Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.) );
    c.setAlphaF(Image::clamp(a, 0., 1.));
    renderText(x, y, QString::fromUtf8(string.c_str()), c, font(), flags);
    return true;
}

void TimeLineGui::swapOpenGLBuffers() {
    swapBuffers();
}

void TimeLineGui::redraw() {
    update();
}

void TimeLineGui::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const {
    QGLFormat f = format();
    *hasAlpha = f.alpha();
    int r = f.redBufferSize();
    if (r == -1) {
        r = 8;// taken from qgl.h
    }
    int g = f.greenBufferSize();
    if (g == -1) {
        g = 8;// taken from qgl.h
    }
    int b = f.blueBufferSize();
    if (b == -1) {
        b = 8;// taken from qgl.h
    }
    int size = r;
    size = std::min(size, g);
    size = std::min(size, b);
    *depthPerComponents = size;

}

void TimeLineGui::getViewportSize(double &width, double &height) const {
    width = _imp->zoomCtx.screenWidth();
    height = _imp->zoomCtx.screenHeight();
}

void TimeLineGui::getPixelScale(double & xScale, double & yScale) const  {
    xScale = _imp->zoomCtx.screenPixelWidth();
    yScale = _imp->zoomCtx.screenPixelHeight();
}


void
TimeLineGui::setTimeFormatFrames(bool value)
{
    _imp->isTimeFormatFrames = value;
}


#ifdef OFX_EXTENSIONS_NATRON
double
TimeLineGui::getScreenPixelRatio() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return windowHandle()->devicePixelRatio()
#else
    return 1.;
#endif
}
#endif

void TimeLineGui::getBackgroundColour(double &r, double &g, double &b) const {
    SettingsPtr settings = appPTR->getCurrentSettings();
    settings->getTimelineBGColor(&r, &g, &b);
}

void TimeLineGui::saveOpenGLContext() {
    assert( QThread::currentThread() == qApp->thread() );

    //glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&_imp->activeTexture);
    glCheckAttribStack(GL_GPU);
    GL_GPU::PushAttrib(GL_ALL_ATTRIB_BITS);
    glCheckClientAttribStack(GL_GPU);
    GL_GPU::PushClientAttrib(GL_ALL_ATTRIB_BITS);
    GL_GPU::MatrixMode(GL_PROJECTION);
    glCheckProjectionStack(GL_GPU);
    GL_GPU::PushMatrix();
    GL_GPU::MatrixMode(GL_MODELVIEW);
    glCheckModelviewStack(GL_GPU);
    GL_GPU::PushMatrix();

    // set defaults to work around OFX plugin bugs
    GL_GPU::Enable(GL_BLEND); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glEnable(GL_TEXTURE_2D);					//Activate texturing
    //glActiveTexture (GL_TEXTURE0);
    GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // or TuttleHistogramKeyer doesn't work - maybe other OFX plugins rely on this
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // GL_MODULATE is the default, set it

}

void TimeLineGui::restoreOpenGLContext() {
    //glActiveTexture(_imp->activeTexture);
    GL_GPU::MatrixMode(GL_PROJECTION);
    GL_GPU::PopMatrix();
    GL_GPU::MatrixMode(GL_MODELVIEW);
    GL_GPU::PopMatrix();
    GL_GPU::PopClientAttrib();
    GL_GPU::PopAttrib();

}

unsigned int TimeLineGui::getCurrentRenderScale() const {
    return 0;
}

void
TimelineGuiPrivate::drawOverlays(SequenceTime time)
{
    NodesList nodes;
    viewerTab->getNodesEntitledForOverlays(TimeValue(time), ViewIdx(0), nodes);

    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeTimeline, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            (*it)->drawOverlay_public(parent, TimeValue(time), RenderScale(1.), ViewIdx(0));
        }
    }
} // drawOverlays

bool
TimelineGuiPrivate::penDown(SequenceTime time, PenType pen, const QPointF & viewportPos, const QPointF & pos)
{
    NodesList nodes;
    viewerTab->getNodesEntitledForOverlays(TimeValue(time), ViewIdx(0), nodes);


    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeTimeline, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            bool caught = (*it)->onOverlayPenDown_public(parent, TimeValue(time), RenderScale(1.), ViewIdx(0), viewportPos, pos, 1., TimeValue(0), pen);
            if (caught) {
                return true;
            }
        }
    }
    return false;
}

bool
TimelineGuiPrivate::penMotion(SequenceTime time, const QPointF & viewportPos, const QPointF & pos)
{
    NodesList nodes;
    viewerTab->getNodesEntitledForOverlays(TimeValue(time), ViewIdx(0), nodes);


    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeTimeline, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            bool caught = (*it)->onOverlayPenMotion_public(parent, TimeValue(time), RenderScale(1.), ViewIdx(0), viewportPos, pos, 1., TimeValue(0));
            if (caught) {
                return true;
            }
        }
    }
    return false;
}

bool
TimelineGuiPrivate::penUp(SequenceTime time, const QPointF & viewportPos, const QPointF & pos)
{
    NodesList nodes;
    viewerTab->getNodesEntitledForOverlays(TimeValue(time), ViewIdx(0), nodes);


    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeTimeline, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            bool caught = (*it)->onOverlayPenUp_public(parent, TimeValue(time), RenderScale(1.), ViewIdx(0), viewportPos, pos, 1., TimeValue(0));
            if (caught) {
                return true;
            }
        }
    }
    return false;
}

bool
TimelineGuiPrivate::keyDown(SequenceTime currentFrame, Key k, KeyboardModifiers km)
{
    NodesList nodes;
    viewerTab->getNodesEntitledForOverlays(TimeValue(currentFrame), ViewIdx(0), nodes);


    /*
     Modifiers key down/up should be passed to all active interacts always so that they can properly figure out
     whether they are up or down
     */
    const bool isModifier = k == Key_Control_L || k == Key_Control_R || k == Key_Alt_L || k == Key_Alt_R || k == Key_Shift_L || k == Key_Shift_R || k == Key_Meta_L || k == Key_Meta_R;

    bool caught = false;

    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeTimeline, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            caught |= (*it)->onOverlayKeyDown_public(parent, TimeValue(currentFrame), RenderScale(1.), ViewIdx(0), k, km);
            if (!isModifier && caught) {
                return true;
            }
        }
    }
    return caught;

}

bool
TimelineGuiPrivate::keyUp(SequenceTime currentFrame, Key k, KeyboardModifiers km)
{
    NodesList nodes;
    viewerTab->getNodesEntitledForOverlays(TimeValue(currentFrame), ViewIdx(0), nodes);


    /*
     Modifiers key down/up should be passed to all active interacts always so that they can properly figure out
     whether they are up or down
     */
    const bool isModifier = k == Key_Control_L || k == Key_Control_R || k == Key_Alt_L || k == Key_Alt_R || k == Key_Shift_L || k == Key_Shift_R || k == Key_Meta_L || k == Key_Meta_R;

    bool caught = false;
    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeTimeline, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            caught |= (*it)->onOverlayKeyUp_public(parent, TimeValue(currentFrame), RenderScale(1.), ViewIdx(0), k, km);
            if (!isModifier && caught) {
                return true;
            }
        }
    }
    return caught;
}

bool
TimelineGuiPrivate::keyRepeat(SequenceTime currentFrame, Key k, KeyboardModifiers km)
{
    NodesList nodes;
    viewerTab->getNodesEntitledForOverlays(TimeValue(currentFrame), ViewIdx(0), nodes);
    /*
     Modifiers key down/up should be passed to all active interacts always so that they can properly figure out
     whether they are up or down
     */
    const bool isModifier = k == Key_Control_L || k == Key_Control_R || k == Key_Alt_L || k == Key_Alt_R || k == Key_Shift_L || k == Key_Shift_R || k == Key_Meta_L || k == Key_Meta_R;

    bool caught = false;

    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeTimeline, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            caught |= (*it)->onOverlayKeyRepeat_public(parent, TimeValue(currentFrame), RenderScale(1.), ViewIdx(0), k, km);
            if (!isModifier && caught) {
                return true;
            }
        }
    }
    return caught;
}

bool
TimelineGuiPrivate::focusLost(SequenceTime currentFrame)
{
    NodesList nodes;
    viewerTab->getNodesEntitledForOverlays(TimeValue(currentFrame), ViewIdx(0), nodes);

    bool caught = false;

    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeTimeline, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            caught |= (*it)->onOverlayFocusLost_public(parent, TimeValue(currentFrame), RenderScale(1.), ViewIdx(0));

        }
    }
    return caught;
}

bool
TimelineGuiPrivate::focusGain(SequenceTime currentFrame)
{
    NodesList nodes;
    viewerTab->getNodesEntitledForOverlays(TimeValue(currentFrame), ViewIdx(0), nodes);


    bool caught = false;

    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeTimeline, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            caught |= (*it)->onOverlayFocusGained_public(parent, TimeValue(currentFrame), RenderScale(1.), ViewIdx(0));

        }
    }
    return caught;
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
