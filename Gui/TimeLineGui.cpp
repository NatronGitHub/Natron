//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "TimeLineGui.h"

#include <cmath>
#include <set>
#include <QtGui/QFont>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QMouseEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QCoreApplication>
#include <QThread>
#include "Global/GlobalDefines.h"

#include "Engine/Cache.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"

#include "Gui/ViewerTab.h"
#include "Gui/TextRenderer.h"
#include "Gui/ticks.h"
#include "Gui/Gui.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

using namespace Natron;

#define TICK_HEIGHT 7
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8

#define DEFAULT_TIMELINE_LEFT_BOUND 0
#define DEFAULT_TIMELINE_RIGHT_BOUND 100

namespace { // protect local classes in anonymous namespace

struct ZoomContext {
    ZoomContext()
        : bottom(0.)
        , left(0.)
        , zoomFactor(1.)
    {}

    QPoint oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
    double bottom; /// the bottom edge of orthographic projection
    double left; /// the left edge of the orthographic projection
    double zoomFactor; /// the zoom factor applied to the current image

    double lastOrthoLeft, lastOrthoBottom, lastOrthoRight, lastOrthoTop; //< remembers the last values passed to the glOrtho call
};

struct CachedFrame {
    SequenceTime time;
    StorageMode mode;
    
    CachedFrame(SequenceTime t,StorageMode m)
    : time(t)
    , mode(m)
    {
        
    }
};
    
struct CachedFrame_compare_time {
    bool operator() (const CachedFrame& lhs, const CachedFrame& rhs) const {
        return lhs.time < rhs.time;
    }
};
    
typedef std::set<CachedFrame,CachedFrame_compare_time> CachedFrames;
}

struct TimelineGuiPrivate{

    boost::shared_ptr<TimeLine> _timeline; //ptr to the internal timeline
    Gui* _gui; //< ptr to the gui
    bool _alphaCursor; // should cursor be drawn semi-transparant
    QPoint _lastMouseEventWidgetCoord;
    Natron::TIMELINE_STATE _state; //state machine for mouse events

    ZoomContext _zoomCtx;
    Natron::TextRenderer _textRenderer;

    QColor _cursorColor;
    QColor _boundsColor;
    QColor _cachedLineColor;
    QColor _diskCachedLineColor;
    QColor _keyframesColor;
    QColor _clearColor;
    QColor _backgroundColor;
    QColor _ticksColor;
    QColor _scaleColor;
    QFont _font;
    bool _firstPaint;
    
    CachedFrames cachedFrames;

    TimelineGuiPrivate(boost::shared_ptr<TimeLine> timeline,Gui* gui):
        _timeline(timeline)
      , _gui(gui)
      , _alphaCursor(false)
      , _lastMouseEventWidgetCoord()
      , _state(IDLE)
      , _zoomCtx()
      , _textRenderer()
      , _cursorColor(243,149,0)
      , _boundsColor(207,69,6)
      , _cachedLineColor(143,201,103)
      , _diskCachedLineColor(69,96,63)
      , _keyframesColor(21,97,248)
      , _clearColor(0,0,0,255)
      , _backgroundColor(50,50,50)
      , _ticksColor(200,200,200)
      , _scaleColor(100,100,100)
      , _font(NATRON_FONT_ALT, NATRON_FONT_SIZE_10)
      , _firstPaint(true)
      , cachedFrames()
    {}



};

TimeLineGui::TimeLineGui(boost::shared_ptr<TimeLine> timeline,Gui* gui, QWidget* parent, const QGLWidget *shareWidget):
    QGLWidget(parent,shareWidget),
    _imp(new TimelineGuiPrivate(timeline,gui))
{
    
    //connect the internal timeline to the gui
    QObject::connect(timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onFrameChanged(SequenceTime,int)));
    QObject::connect(timeline.get(), SIGNAL(frameRangeChanged(SequenceTime,SequenceTime)),
                     this, SLOT(onFrameRangeChanged(SequenceTime,SequenceTime)));
    QObject::connect(timeline.get(), SIGNAL(boundariesChanged(SequenceTime,SequenceTime,int)),
                     this, SLOT(onBoundariesChanged(SequenceTime,SequenceTime,int)));


    //connect the gui to the internal timeline
    QObject::connect(this, SIGNAL(frameChanged(SequenceTime)), timeline.get(), SLOT(onFrameChanged(SequenceTime)));
    QObject::connect(this, SIGNAL(boundariesChanged(SequenceTime,SequenceTime)),
                     timeline.get(), SLOT(onBoundariesChanged(SequenceTime,SequenceTime)));
    
    QObject::connect(timeline.get(), SIGNAL(keyframeIndicatorsChanged()), this, SLOT(onKeyframesIndicatorsChanged()));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);
}

TimeLineGui::~TimeLineGui(){}

QSize TimeLineGui::sizeHint() const{
    return QSize(1000,45);
}

void TimeLineGui::initializeGL(){

}

void TimeLineGui::resizeGL(int width,int height){
    if(height == 0)
        height = 1;
    glViewport (0, 0, width , height);
}

void TimeLineGui::paintGL()
{
    glCheckError();
    if(_imp->_firstPaint){
        _imp->_firstPaint = false;
        centerOn(DEFAULT_TIMELINE_LEFT_BOUND,DEFAULT_TIMELINE_RIGHT_BOUND);
    }

    double w = (double)width();
    double h = (double)height();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    //assert(_zoomCtx._zoomFactor > 0);
    if(_imp->_zoomCtx.zoomFactor <= 0){
        return;
    }
    //assert(_zoomCtx._zoomFactor <= 1024);
    double bottom = _imp->_zoomCtx.bottom;
    double left = _imp->_zoomCtx.left;
    double top = bottom +  h / (double)_imp->_zoomCtx.zoomFactor ;
    double right = left +  (w / (double)_imp->_zoomCtx.zoomFactor);
    if(left == right || top == bottom){
        glClearColor(_imp->_clearColor.redF(),_imp->_clearColor.greenF(),_imp->_clearColor.blueF(),_imp->_clearColor.alphaF());
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    _imp->_zoomCtx.lastOrthoLeft = left;
    _imp->_zoomCtx.lastOrthoRight = right;
    _imp->_zoomCtx.lastOrthoBottom = bottom;
    _imp->_zoomCtx.lastOrthoTop = top;
    glOrtho(left , right, bottom, top, -1, 1);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(_imp->_clearColor.redF(),_imp->_clearColor.greenF(),_imp->_clearColor.blueF(),_imp->_clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);
    glCheckErrorIgnoreOSXBug();

    QPointF btmLeft = toTimeLineCoordinates(0,height()-1);
    QPointF topRight = toTimeLineCoordinates(width()-1, 0);


    /// change the backgroud color of the portion of the timeline where images are lying
    QPointF firstFrameWidgetPos = toWidgetCoordinates(_imp->_timeline->firstFrame(),0);
    QPointF lastFrameWidgetPos = toWidgetCoordinates(_imp->_timeline->lastFrame(),0);

    // glPushAttrib(GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glScissor(firstFrameWidgetPos.x(),0,
              lastFrameWidgetPos.x() - firstFrameWidgetPos.x(),height());

    glEnable(GL_SCISSOR_TEST);
    glClearColor(_imp->_backgroundColor.redF(),_imp->_backgroundColor.greenF(),_imp->_backgroundColor.blueF(),_imp->_backgroundColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);
    glCheckErrorIgnoreOSXBug();
    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    QFontMetrics fontM(_imp->_font);

    double lineYpos = toTimeLineCoordinates(0,height() -1 - fontM.height()  - TICK_HEIGHT/2).y();


    /*draw the horizontal axis*/
    glColor4f(_imp->_scaleColor.redF(), _imp->_scaleColor.greenF(), _imp->_scaleColor.blueF(), _imp->_scaleColor.alphaF());
    glBegin(GL_LINES);
    glVertex2f(btmLeft.x(), lineYpos);
    glVertex2f(topRight.x(), lineYpos);
    glEnd();
    glCheckErrorIgnoreOSXBug();

    double tickBottom = toTimeLineCoordinates(0,height() -1 - fontM.height() ).y();
    double tickTop = toTimeLineCoordinates(0,height() -1 - fontM.height()  - TICK_HEIGHT).y();
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);
    const double rangePixel =  width();
    const double range_min = btmLeft.x() ;
    const double range_max =  topRight.x() ;
    const double range = range_max - range_min;
    double smallTickSize;
    bool half_tick;
    ticks_size(range_min, range_max, rangePixel, smallestTickSizePixel, &smallTickSize, &half_tick);
    int m1, m2;
    const int ticks_max = 1000;
    double offset;
    ticks_bounds(range_min, range_max, smallTickSize, half_tick, ticks_max, &offset, &m1, &m2);
    std::vector<int> ticks;
    ticks_fill(half_tick, ticks_max, m1, m2, &ticks);
    const double smallestTickSize = range * smallestTickSizePixel / rangePixel;
    const double largestTickSize = range * largestTickSizePixel / rangePixel;
    const double minTickSizeTextPixel = fontM.width(QString("00")) ; // AXIS-SPECIFIC
    const double minTickSizeText = range * minTickSizeTextPixel/rangePixel;
    for(int i = m1 ; i <= m2; ++i) {
        double value = i * smallTickSize + offset;
        const double tickSize = ticks[i-m1]*smallTickSize;
        const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

        glColor4f(_imp->_ticksColor.redF(), _imp->_ticksColor.greenF(), _imp->_ticksColor.blueF(), alpha);

        glBegin(GL_LINES);
        glVertex2f(value, tickBottom);
        glVertex2f(value, tickTop);
        glEnd();
        glCheckErrorIgnoreOSXBug();

        if (tickSize > minTickSizeText) {
            const int tickSizePixel = rangePixel * tickSize/range;
            const QString s = QString::number(value);
            const int sSizePixel =  fontM.width(s);
            if (tickSizePixel > sSizePixel) {
                const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                double alphaText = 1.0;//alpha;
                if (tickSizePixel < sSizeFullPixel) {
                    // when the text size is between sSizePixel and sSizeFullPixel,
                    // draw it with a lower alpha
                    alphaText *= (tickSizePixel - sSizePixel)/(double)minTickSizeTextPixel;
                }
                QColor c = _imp->_ticksColor;
                c.setAlpha(255*alphaText);
                glCheckError();
                renderText(value, btmLeft.y(), s, c, _imp->_font);
            }
        }
    }
    glCheckError();

    QPointF cursorBtm(_imp->_timeline->currentFrame(),lineYpos);
    QPointF cursorBtmWidgetCoord = toWidgetCoordinates(cursorBtm.x(),cursorBtm.y());
    QPointF cursorTopLeft = toTimeLineCoordinates(cursorBtmWidgetCoord.x() - CURSOR_WIDTH /2,
                                                  cursorBtmWidgetCoord.y() - CURSOR_HEIGHT);
    QPointF cursorTopRight = toTimeLineCoordinates(cursorBtmWidgetCoord.x() + CURSOR_WIDTH /2,
                                                   cursorBtmWidgetCoord.y() - CURSOR_HEIGHT);

    QPointF leftBoundBtm(_imp->_timeline->leftBound(),lineYpos);
    QPointF leftBoundWidgetCoord = toWidgetCoordinates(leftBoundBtm.x(),leftBoundBtm.y());
    QPointF leftBoundBtmRight = toTimeLineCoordinates(leftBoundWidgetCoord.x() + CURSOR_WIDTH /2,
                                                      leftBoundWidgetCoord.y());
    QPointF leftBoundTop = toTimeLineCoordinates(leftBoundWidgetCoord.x(),
                                                 leftBoundWidgetCoord.y() - CURSOR_HEIGHT);

    QPointF rightBoundBtm(_imp->_timeline->rightBound(),lineYpos);
    QPointF rightBoundWidgetCoord = toWidgetCoordinates(rightBoundBtm.x(),rightBoundBtm.y());
    QPointF rightBoundBtmLeft = toTimeLineCoordinates(rightBoundWidgetCoord.x() - CURSOR_WIDTH /2,
                                                      rightBoundWidgetCoord.y());
    QPointF rightBoundTop = toTimeLineCoordinates(rightBoundWidgetCoord.x(),
                                                  rightBoundWidgetCoord.y() - CURSOR_HEIGHT);

    std::list<SequenceTime> keyframes;
    _imp->_timeline->getKeyframes(&keyframes);
    
    //draw an alpha cursor if the mouse is hovering the timeline
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_DONT_CARE);
    if(_imp->_alphaCursor){
        int currentPosBtmWidgetCoordX = _imp->_lastMouseEventWidgetCoord.x();
        int currentPosBtmWidgetCoordY = toWidgetCoordinates(0,lineYpos).y();
        QPointF currentPosBtm = toTimeLineCoordinates(currentPosBtmWidgetCoordX,currentPosBtmWidgetCoordY);
        QPointF currentPosTopLeft = toTimeLineCoordinates(currentPosBtmWidgetCoordX - CURSOR_WIDTH /2,
                                                          currentPosBtmWidgetCoordY - CURSOR_HEIGHT);
        QPointF currentPosTopRight = toTimeLineCoordinates(currentPosBtmWidgetCoordX + CURSOR_WIDTH /2,
                                                           currentPosBtmWidgetCoordY - CURSOR_HEIGHT);
        
        int hoveredTime = std::floor(currentPosBtm.x() + 0.5);
        QString mouseNumber(QString::number(hoveredTime));
        QPoint mouseNumberWidgetCoord(currentPosBtmWidgetCoordX - fontM.width(mouseNumber)/2 ,
                                      currentPosBtmWidgetCoordY - CURSOR_HEIGHT - 2);
        QPointF mouseNumberPos = toTimeLineCoordinates(mouseNumberWidgetCoord.x(),mouseNumberWidgetCoord.y());
        
        
        QColor currentColor;
        std::list<SequenceTime>::iterator foundHoveredAsKeyframe = std::find(keyframes.begin(),keyframes.end(),hoveredTime);
        if (foundHoveredAsKeyframe != keyframes.end()) {
            currentColor = _imp->_keyframesColor;
        } else {
            currentColor = _imp->_cursorColor;
        }
        currentColor.setAlpha(100);

        glColor4f(currentColor.redF(),currentColor.greenF(),currentColor.blueF(),currentColor.alphaF());
        glBegin(GL_POLYGON);
        glVertex2f(currentPosBtm.x(),currentPosBtm.y());
        glVertex2f(currentPosTopLeft.x(),currentPosTopLeft.y());
        glVertex2f(currentPosTopRight.x(),currentPosTopRight.y());
        glEnd();
        glCheckError();


        

        renderText(mouseNumberPos.x(),mouseNumberPos.y(), mouseNumber, currentColor, _imp->_font);
    }

    //draw the bounds and the current time cursor
    QColor actualCursorColor;
    std::list<SequenceTime>::iterator isCurrentTimeAKeyframe = std::find(keyframes.begin(),keyframes.end(),_imp->_timeline->currentFrame());
    if (isCurrentTimeAKeyframe != keyframes.end()) {
        actualCursorColor = _imp->_keyframesColor;
    } else {
        actualCursorColor = _imp->_cursorColor;
    }
    
    QString currentFrameStr(QString::number(_imp->_timeline->currentFrame()));
    double cursorTextXposWidget = cursorBtmWidgetCoord.x() - fontM.width(currentFrameStr)/2;
    double cursorTextPos = toTimeLineCoordinates(cursorTextXposWidget,0).x();
    renderText(cursorTextPos ,cursorTopLeft.y(), currentFrameStr, actualCursorColor, _imp->_font);
    glColor4f(actualCursorColor.redF(),actualCursorColor.greenF(),actualCursorColor.blueF(),actualCursorColor.alphaF());
    glBegin(GL_POLYGON);
    glVertex2f(cursorBtm.x(),cursorBtm.y());
    glVertex2f(cursorTopLeft.x(),cursorTopLeft.y());
    glVertex2f(cursorTopRight.x(),cursorTopRight.y());
    glEnd();
    glCheckErrorIgnoreOSXBug();

    if(_imp->_timeline->leftBound() != _imp->_timeline->currentFrame()){
        QString leftBoundStr(QString::number(_imp->_timeline->leftBound()));
        double leftBoundTextXposWidget = toWidgetCoordinates((leftBoundBtm.x() + leftBoundBtmRight.x())/2,0).x() - fontM.width(leftBoundStr)/2;
        double leftBoundTextPos = toTimeLineCoordinates(leftBoundTextXposWidget,0).x();
        renderText(leftBoundTextPos,leftBoundTop.y(),
                   leftBoundStr, _imp->_boundsColor, _imp->_font);
    }
    glColor4f(_imp->_boundsColor.redF(),_imp->_boundsColor.greenF(),_imp->_boundsColor.blueF(),_imp->_boundsColor.alphaF());
    glBegin(GL_POLYGON);
    glVertex2f(leftBoundBtm.x(),leftBoundBtm.y());
    glVertex2f(leftBoundBtmRight.x(),leftBoundBtmRight.y());
    glVertex2f(leftBoundTop.x(),leftBoundTop.y());
    glEnd();
    glCheckErrorIgnoreOSXBug();

    if(_imp->_timeline->rightBound() != _imp->_timeline->currentFrame()){
        QString rightBoundStr(QString::number(_imp->_timeline->rightBound()));
        double rightBoundTextXposWidget = toWidgetCoordinates((rightBoundBtm.x() + rightBoundBtmLeft.x())/2,0).x() - fontM.width(rightBoundStr)/2;
        double rightBoundTextPos = toTimeLineCoordinates(rightBoundTextXposWidget,0).x();
        renderText(rightBoundTextPos,rightBoundTop.y(),
                   rightBoundStr, _imp->_boundsColor, _imp->_font);
    }
    glColor4f(_imp->_boundsColor.redF(),_imp->_boundsColor.greenF(),_imp->_boundsColor.blueF(),_imp->_boundsColor.alphaF());
    glCheckError();
    glBegin(GL_POLYGON);
    glVertex2f(rightBoundBtm.x(),rightBoundBtm.y());
    glVertex2f(rightBoundBtmLeft.x(),rightBoundBtmLeft.y());
    glVertex2f(rightBoundTop.x(),rightBoundTop.y());
    glEnd();
    glCheckErrorIgnoreOSXBug();

    glDisable(GL_POLYGON_SMOOTH);

    //draw cached frames
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
    glCheckError();
    glLineWidth(2);
    glCheckError();
    glBegin(GL_LINES);
    for(CachedFrames::const_iterator i = _imp->cachedFrames.begin();i!= _imp->cachedFrames.end();++i) {
        if (i->mode == RAM) {
            glColor4f(_imp->_cachedLineColor.redF(),_imp->_cachedLineColor.greenF(),
                      _imp->_cachedLineColor.blueF(),_imp->_cachedLineColor.alphaF());
        } else if (i->mode == DISK) {
            glColor4f(_imp->_diskCachedLineColor.redF(),_imp->_diskCachedLineColor.greenF(),
                      _imp->_diskCachedLineColor.blueF(),_imp->_diskCachedLineColor.alphaF());

        }
        glVertex2f(i->time - 0.5,lineYpos);
        glVertex2f(i->time + 0.5,lineYpos);
    }
    glEnd();
    
    ///now draw keyframes
    glColor4f(_imp->_keyframesColor.redF(),_imp->_keyframesColor.greenF(),_imp->_keyframesColor.blueF(),_imp->_keyframesColor.alphaF());
    std::set<SequenceTime> alreadyDrawnKeyframes;
    glBegin(GL_LINES);
    for (std::list<SequenceTime>::const_iterator i = keyframes.begin(); i != keyframes.end(); ++i) {
        std::pair<std::set<SequenceTime>::iterator,bool> success = alreadyDrawnKeyframes.insert(*i);
        if (success.second) {
            glVertex2f(*i - 0.5,lineYpos);
            glVertex2f(*i + 0.5,lineYpos);
        }
    }
    glEnd();
    
    glCheckErrorIgnoreOSXBug();
    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.);

    glDisable(GL_BLEND);
    glColor4f(1.,1.,1.,1.);
    glPopAttrib();
    glCheckError();
}



void TimeLineGui::renderText(double x,double y,const QString& text,const QColor& color,const QFont& font) const
{
    assert(QGLContext::currentContext() == context());

    glCheckError();
    if(text.isEmpty())
        return;
    glMatrixMode (GL_PROJECTION);
    glPushMatrix(); // save GL_PROJECTION
    glCheckError();
    glLoadIdentity();
    double h = (double)height();
    double w = (double)width();
    /*we put the ortho proj to the widget coords, draw the elements and revert back to the old orthographic proj.*/
    glOrtho(0,w,0,h,-1,1);
    glMatrixMode(GL_MODELVIEW);
    QPointF pos = toWidgetCoordinates(x, y);
    glCheckError();
    _imp->_textRenderer.renderText(pos.x(),h-pos.y(),text,color,font);
    glCheckError();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glPopMatrix(); // restore GL_PROJECTION
    glMatrixMode(GL_MODELVIEW);
    glCheckError();
}


void TimeLineGui::onFrameChanged(SequenceTime ,int ){
    update();
}

void TimeLineGui::seek(SequenceTime time){
    emit frameChanged(time);
    update();

}


void TimeLineGui::mousePressEvent(QMouseEvent* e){
    _imp->_lastMouseEventWidgetCoord = e->pos();
    double c = toTimeLineCoordinates(e->x(),0).x();
    if(e->modifiers().testFlag(Qt::ControlModifier)){
        _imp->_state = DRAGGING_BOUNDARY;
        int firstPos = toWidgetCoordinates(_imp->_timeline->leftBound()-1,0).x();
        int lastPos = toWidgetCoordinates(_imp->_timeline->rightBound()+1,0).x();
        int distFromFirst = std::abs(e->x() - firstPos);
        int distFromLast = std::abs(e->x() - lastPos);
        if(distFromFirst  > distFromLast ){
            setBoundaries(_imp->_timeline->leftBound(),c); // moving last frame anchor
        }else{
            setBoundaries(c,_imp->_timeline->rightBound());   // moving first frame anchor
        }
    }else{
        _imp->_state = DRAGGING_CURSOR;
        _imp->_gui->setUserScrubbingTimeline(true);
        seek(std::floor(c + 0.5));
    }
    


}
void TimeLineGui::mouseMoveEvent(QMouseEvent* e){
    _imp->_lastMouseEventWidgetCoord = e->pos();
    double c = toTimeLineCoordinates(e->x(),0).x();
    bool distortViewPort = false;
    if(_imp->_state == DRAGGING_CURSOR){

        emit frameChanged(c);
        distortViewPort = true;
        _imp->_alphaCursor = false;
    }else if(_imp->_state == DRAGGING_BOUNDARY){

        int firstPos = toWidgetCoordinates(_imp->_timeline->leftBound()-1,0).x();
        int lastPos = toWidgetCoordinates(_imp->_timeline->rightBound()+1,0).x();
        int distFromFirst = std::abs(e->x() - firstPos);
        int distFromLast = std::abs(e->x() - lastPos);
        if( distFromFirst  > distFromLast  ){ // moving last frame anchor
            if(_imp->_timeline->leftBound() <= c){
                emit boundariesChanged(_imp->_timeline->leftBound(),c);
            }
        }else{ // moving first frame anchor
            if(_imp->_timeline->rightBound() >= c){
                emit boundariesChanged(c,_imp->_timeline->rightBound());
            }
        }
        distortViewPort = true;
        _imp->_alphaCursor = false;
    } else {
        _imp->_alphaCursor = true;
    }

    if(distortViewPort){
        double leftMost = toTimeLineCoordinates(0,0).x();
        double rightMost = toTimeLineCoordinates(width()-1,0).x();
        if(c < leftMost){
            centerOn(c,rightMost);
        }else if(c > rightMost){
            centerOn(leftMost,c);
        }else{
            update();
        }
    }else{
        update();
    }

}
void TimeLineGui::enterEvent(QEvent* e){
    _imp->_alphaCursor = true;
    update();
    QGLWidget::enterEvent(e);
}
void TimeLineGui::leaveEvent(QEvent* e){
    _imp->_alphaCursor = false;
    update();
    QGLWidget::leaveEvent(e);
}


void TimeLineGui::mouseReleaseEvent(QMouseEvent* e){
    if (_imp->_state == DRAGGING_CURSOR) {
        _imp->_gui->setUserScrubbingTimeline(false);
        _imp->_gui->refreshAllPreviews();
    }
    _imp->_state = IDLE;
    QGLWidget::mouseReleaseEvent(e);
}

void TimeLineGui::wheelEvent(QWheelEvent *event){
    if (event->orientation() != Qt::Vertical) {
        return;
    }
    const double scaleFactor = std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, event->delta());
    double newZoomFactor = _imp->_zoomCtx.zoomFactor * scaleFactor;
    if (newZoomFactor <= 0.01) {
        newZoomFactor = 0.01;
    } else if (newZoomFactor > 1024.) {
        newZoomFactor = 1024.;
    }
    QPointF zoomCenter = toTimeLineCoordinates(event->x(), event->y());
    double zoomRatio =   _imp->_zoomCtx.zoomFactor / newZoomFactor;
    _imp->_zoomCtx.left = zoomCenter.x() - (zoomCenter.x() - _imp->_zoomCtx.left)*zoomRatio ;
    _imp->_zoomCtx.bottom = zoomCenter.y() - (zoomCenter.y() - _imp->_zoomCtx.bottom)*zoomRatio;

    _imp->_zoomCtx.zoomFactor = newZoomFactor;

    update();

}

void TimeLineGui::onFrameRangeChanged(SequenceTime first , SequenceTime last ){
    if(first == last)
        return;
    centerOn(first,last);
}

void TimeLineGui::setBoundaries(SequenceTime first,SequenceTime last){
    if(first <= last){
        emit boundariesChanged(first,last);
        update();
    }
}

void TimeLineGui::onBoundariesChanged(SequenceTime ,SequenceTime,int reason){
    if(reason == Natron::PLUGIN_EDITED){
        update();
    }
}

void TimeLineGui::centerOn(SequenceTime left,SequenceTime right){
    double curveWidth = right - left + 10;
    double w = width();
    _imp->_zoomCtx.left = left - 5;
    _imp->_zoomCtx.zoomFactor = w / curveWidth;

    update();
}

SequenceTime TimeLineGui::firstFrame() const { return _imp->_timeline->firstFrame(); }

SequenceTime TimeLineGui::lastFrame() const { return _imp->_timeline->lastFrame(); }

SequenceTime TimeLineGui::leftBound() const { return _imp->_timeline->leftBound(); }

SequenceTime TimeLineGui::rightBound() const { return _imp->_timeline->rightBound(); }

SequenceTime TimeLineGui::currentFrame() const { return _imp->_timeline->currentFrame(); }



QPointF TimeLineGui::toTimeLineCoordinates(double x,double y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->_zoomCtx.bottom;
    double left = _imp->_zoomCtx.left;
    double top =  bottom +  h / _imp->_zoomCtx.zoomFactor;
    double right = left +  w / _imp->_zoomCtx.zoomFactor;
    return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
}

QPointF TimeLineGui::toWidgetCoordinates(double x, double y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->_zoomCtx.bottom;
    double left = _imp->_zoomCtx.left;
    double top =  bottom +  h / _imp->_zoomCtx.zoomFactor ;
    double right = left +  w / _imp->_zoomCtx.zoomFactor;
    return QPoint(((x - left)/(right - left))*w,((y - top)/(bottom - top))*h);
}

void TimeLineGui::onKeyframesIndicatorsChanged()
{
    repaint();
}

void TimeLineGui::connectSlotsToViewerCache()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    
    Natron::CacheSignalEmitter* emitter = appPTR->getOrActivateViewerCacheSignalEmitter();
    QObject::connect(emitter, SIGNAL(addedEntry(SequenceTime)), this, SLOT(onCachedFrameAdded(SequenceTime)));
    QObject::connect(emitter, SIGNAL(removedEntry(SequenceTime,int)), this, SLOT(onCachedFrameRemoved(SequenceTime,int)));
    QObject::connect(emitter, SIGNAL(entryStorageChanged(SequenceTime,int,int)), this,
                     SLOT(onCachedFrameStorageChanged(SequenceTime,int,int)));
    QObject::connect(emitter, SIGNAL(clearedDiskPortion()), this, SLOT(onDiskCacheCleared()));
    QObject::connect(emitter, SIGNAL(clearedInMemoryPortion()), this, SLOT(onMemoryCacheCleared()));
}

void TimeLineGui::disconnectSlotsFromViewerCache()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    
    Natron::CacheSignalEmitter* emitter = appPTR->getOrActivateViewerCacheSignalEmitter();
    QObject::disconnect(emitter, SIGNAL(addedEntry(SequenceTime)), this, SLOT(onCachedFrameAdded(SequenceTime)));
    QObject::disconnect(emitter, SIGNAL(removedEntry(SequenceTime,int)), this, SLOT(onCachedFrameRemoved(SequenceTime,int)));
    QObject::disconnect(emitter, SIGNAL(entryStorageChanged(SequenceTime,int,int)), this,
                        SLOT(onCachedFrameStorageChanged(SequenceTime,int,int)));
    QObject::disconnect(emitter, SIGNAL(clearedDiskPortion()), this, SLOT(onDiskCacheCleared()));
    QObject::disconnect(emitter, SIGNAL(clearedInMemoryPortion()), this, SLOT(onMemoryCacheCleared()));}

void TimeLineGui::onCachedFrameAdded(SequenceTime time)
{
    _imp->cachedFrames.insert(CachedFrame(time,RAM));
}

void TimeLineGui::onCachedFrameRemoved(SequenceTime time,int /*storage*/)
{
    for (CachedFrames::iterator it = _imp->cachedFrames.begin();it!=_imp->cachedFrames.end();++it) {
        if (it->time == time) {
            _imp->cachedFrames.erase(it);
            break;
        }
    }
    update();
}

void TimeLineGui::onCachedFrameStorageChanged(SequenceTime time,int /*oldStorage*/,int newStorage)
{
    for (CachedFrames::iterator it = _imp->cachedFrames.begin();it!=_imp->cachedFrames.end();++it) {
        if (it->time == time) {
            _imp->cachedFrames.erase(it);
            _imp->cachedFrames.insert(CachedFrame(time,(StorageMode)newStorage));
            break;
        }
    }
}

void TimeLineGui::onMemoryCacheCleared()
{
    CachedFrames copy;
    for (CachedFrames::iterator it = _imp->cachedFrames.begin();it!=_imp->cachedFrames.end();++it) {
        if (it->mode == DISK) {
            copy.insert(*it);
        }
    }
    _imp->cachedFrames = copy;
    update();
}

void TimeLineGui::onDiskCacheCleared()
{
    CachedFrames copy;
    for (CachedFrames::iterator it = _imp->cachedFrames.begin();it!=_imp->cachedFrames.end();++it) {
        if (it->mode == RAM) {
            copy.insert(*it);
        }
    }
    _imp->cachedFrames = copy;
    update();
}

void TimeLineGui::clearCachedFrames()
{
    _imp->cachedFrames.clear();
    update();
}
