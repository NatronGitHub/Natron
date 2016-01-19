/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "HostOverlay.h"

#include <list>
#include <cmath>

#include <boost/weak_ptr.hpp>

#include "Gui/NodeGui.h"
#include "Gui/TextRenderer.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"

#include "Engine/Curve.h"
#include "Engine/EffectInstance.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"
#include "Engine/Transform.h"

#include "Global/KeySymbols.h"

#include "Global/Macros.h"
#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
CLANG_DIAG_OFF(deprecated)
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)
#include <QPointF>
#include <QThread>
#include <QFont>
#include <QColor>
#include <QApplication>


#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288419717
#endif

NATRON_NAMESPACE_ENTER;

namespace {

enum PositionInteractState
{
    ePositionInteractStateInactive,
    ePositionInteractStatePoised,
    ePositionInteractStatePicked
};


struct PositionInteract
{
    boost::weak_ptr<KnobDouble> param;
    QPointF dragPos;
    PositionInteractState state;
    
    
    double pointSize() const
    {
        return 5;
    }
    
    double pointTolerance() const
    {
        return 6.;
    }
    
    
};


struct TransformInteract
{
    
    enum DrawStateEnum {
        eInActive = 0, //< nothing happening
        eCircleHovered, //< the scale circle is hovered
        eLeftPointHovered, //< the left point of the circle is hovered
        eRightPointHovered, //< the right point of the circle is hovered
        eBottomPointHovered, //< the bottom point of the circle is hovered
        eTopPointHovered, //< the top point of the circle is hovered
        eCenterPointHovered, //< the center point of the circle is hovered
        eRotationBarHovered, //< the rotation bar is hovered
        eSkewXBarHoverered, //< the skew bar is hovered
        eSkewYBarHoverered //< the skew bar is hovered
    };
    
    enum MouseStateEnum {
        eReleased = 0,
        eDraggingCircle,
        eDraggingLeftPoint,
        eDraggingRightPoint,
        eDraggingTopPoint,
        eDraggingBottomPoint,
        eDraggingTranslation,
        eDraggingCenter,
        eDraggingRotationBar,
        eDraggingSkewXBar,
        eDraggingSkewYBar
    };
    
    enum OrientationEnum
    {
        eOrientationAllDirections = 0,
        eOrientationNotSet,
        eOrientationHorizontal,
        eOrientationVertical
    };
    
    boost::weak_ptr<KnobDouble> translate;
    boost::weak_ptr<KnobDouble> scale;
    boost::weak_ptr<KnobBool> scaleUniform;
    boost::weak_ptr<KnobDouble> rotate;
    boost::weak_ptr<KnobDouble> center;
    boost::weak_ptr<KnobDouble> skewX;
    boost::weak_ptr<KnobDouble> skewY;
    boost::weak_ptr<KnobChoice> skewOrder;
    
    DrawStateEnum _drawState;
    MouseStateEnum _mouseState;
    int _modifierStateCtrl;
    int _modifierStateShift;
    OrientationEnum _orientation;
    
    Point _centerDrag;
    Point _translateDrag;
    Point _scaleParamDrag;
    bool _scaleUniformDrag;
    double _rotateDrag;
    double _skewXDrag;
    double _skewYDrag;
    int _skewOrderDrag;
    bool _interactiveDrag;
    
    
    TransformInteract()
    : translate()
    , scale()
    , scaleUniform()
    , rotate()
    , center()
    , skewX()
    , skewY()
    , skewOrder()
    , _drawState(eInActive)
    , _mouseState(eReleased)
    , _modifierStateCtrl(0)
    , _modifierStateShift(0)
    , _orientation(eOrientationAllDirections)
    , _centerDrag()
    , _translateDrag()
    , _scaleParamDrag()
    , _scaleUniformDrag(false)
    , _rotateDrag(0)
    , _skewXDrag(0)
    , _skewYDrag(0)
    , _skewOrderDrag(0)
    , _interactiveDrag(false)
    {
        
    }
    
    static double circleRadiusBase() { return 30.; }
    static double circleRadiusMin() { return 15.; }
    static double circleRadiusMax() { return 300.; }
    static double pointSize() { return 7.; }
    static double ellipseNPoints() { return 50.; }

    
    void getTranslate(double time, double* tx, double* ty) const
    {
        boost::shared_ptr<KnobDouble> knob = translate.lock();
        assert(knob);
        *tx = knob->getValueAtTime(time,0);
        *ty = knob->getValueAtTime(time,1);
    }
    
    void getCenter(double time, double* cx, double* cy) const
    {
        boost::shared_ptr<KnobDouble> knob = center.lock();
        assert(knob);
        *cx = knob->getValueAtTime(time,0);
        *cy = knob->getValueAtTime(time,1);
    }
    
    void getScale(double time, double* sx, double* sy) const
    {
        boost::shared_ptr<KnobDouble> knob = scale.lock();
        assert(knob);
        *sx = knob->getValueAtTime(time,0);
        *sy = knob->getValueAtTime(time,1);
    }
    
    void getRotate(double time, double* rot) const
    {
        boost::shared_ptr<KnobDouble> knob = rotate.lock();
        assert(knob);
        *rot = knob->getValueAtTime(time,0);
    }
    
    void getSkewX(double time, double* x) const
    {
        boost::shared_ptr<KnobDouble> knob = skewX.lock();
        assert(knob);
        *x = knob->getValueAtTime(time,0);
    }
    
    void getSkewY(double time, double* y) const
    {
        boost::shared_ptr<KnobDouble> knob = skewY.lock();
        assert(knob);
        *y = knob->getValueAtTime(time,0);
    }
    
    void getSkewOrder(double time, int* order) const
    {
        boost::shared_ptr<KnobChoice> knob = skewOrder.lock();
        assert(knob);
        *order = knob->getValueAtTime(time,0);
    }
    
    void getScaleUniform(double time, bool* uniform) const
    {
        boost::shared_ptr<KnobBool> knob = scaleUniform.lock();
        assert(knob);
        *uniform = knob->getValueAtTime(time,0);
    }

};

// round to the closest int, 1/10 int, etc
// this make parameter editing easier
// pscale is args.pixelScale.x / args.renderScale.x;
// pscale10 is the power of 10 below pscale
inline double fround(double val,
                         double pscale)
{
    double pscale10 = std::pow( 10.,std::floor( std::log10(pscale) ) );
    
    return pscale10 * std::floor(val / pscale10 + 0.5);
}
    
typedef std::list<PositionInteract> PositionInteracts;
typedef std::list<TransformInteract> TransformInteracts;
    
}

struct HostOverlayPrivate
{
    HostOverlay* _publicInterface;
    PositionInteracts positions;
    TransformInteracts transforms;
    boost::weak_ptr<NodeGui> node;
    
    QPointF lastPenPos;
    
    Natron::TextRenderer textRenderer;
    
    bool interactiveDrag;
    
    HostOverlayPrivate(HostOverlay* publicInterface, const boost::shared_ptr<NodeGui>& node)
    : _publicInterface(publicInterface)
    , positions()
    , transforms()
    , node(node)
    , lastPenPos()
    , textRenderer()
    , interactiveDrag(false)
    {
        
    }
    
    void requestRedraw()
    {
        node.lock()->getNode()->getApp()->queueRedrawForAllViewers();
    }
    
    void drawPosition(const PositionInteract& p,
                      double time,
                      const OfxPointD& pscale,
                      const OfxRGBColourD& color,
                      const OfxPointD& shadow,
                      const QFont& font,
                      const QFontMetrics& fm);
    
    void drawTransform(const TransformInteract& p,
                       double time,
                       const OfxPointD& pscale,
                       const OfxRGBColourD& color,
                       const OfxPointD& shadow,
                       const QFont& font,
                       const QFontMetrics& fm);
    
    ////////Position interacts
    bool penMotion(double time,
                   const RenderScale &renderScale,
                   const QPointF &penPos,
                   const QPoint &penPosViewport,
                   double pressure,
                   PositionInteract* it);
    
    bool penUp(double time,
               const RenderScale &renderScale,
               const QPointF &penPos,
               const QPoint &penPosViewport,
               double  pressure,
               PositionInteract* it);
    
    
    bool penDown(double time,
                 const RenderScale &renderScale,
                 const QPointF &penPos,
                 const QPoint &penPosViewport,
                 double  pressure,
                 PositionInteract* it);
    
    
    bool keyDown(double time,
                 const RenderScale &renderScale,
                 int     key,
                 char*   keyString,
                 PositionInteract* it);
    
    
    bool keyUp(double time,
               const RenderScale &renderScale,
               int     key,
               char*   keyString,
               PositionInteract* it);
    
    
    bool keyRepeat(double time,
                   const RenderScale &renderScale,
                   int     key,
                   char*   keyString,
                   PositionInteract* it);
    
    
    bool gainFocus(double time,
                   const RenderScale &renderScale,
                   PositionInteract* it);
    
    
    bool loseFocus(double  time,
                   const RenderScale &renderScale,
                   PositionInteract* it);
    
    
    /////Transform interacts
    
    bool penMotion(double time,
                   const RenderScale &renderScale,
                   const QPointF &penPos,
                   const QPoint &penPosViewport,
                   double pressure,
                   TransformInteract* it);
    
    bool penUp(double time,
               const RenderScale &renderScale,
               const QPointF &penPos,
               const QPoint &penPosViewport,
               double  pressure,
               TransformInteract* it);
    
    
    bool penDown(double time,
                 const RenderScale &renderScale,
                 const QPointF &penPos,
                 const QPoint &penPosViewport,
                 double  pressure,
                 TransformInteract* it);
    
    
    bool keyDown(double time,
                 const RenderScale &renderScale,
                 int     key,
                 char*   keyString,
                 TransformInteract* it);
    
    
    bool keyUp(double time,
               const RenderScale &renderScale,
               int     key,
               char*   keyString,
               TransformInteract* it);
    
    
    bool keyRepeat(double time,
                   const RenderScale &renderScale,
                   int     key,
                   char*   keyString,
                   TransformInteract* it);
    
    
    bool gainFocus(double time,
                   const RenderScale &renderScale,
                   TransformInteract* it);
    
    
    bool loseFocus(double  time,
                   const RenderScale &renderScale,
                   TransformInteract* it);

    
};

HostOverlay::HostOverlay(const boost::shared_ptr<NodeGui>& node)
: _imp(new HostOverlayPrivate(this, node))
{
}

HostOverlay::~HostOverlay()
{
    
}

boost::shared_ptr<NodeGui>
HostOverlay::getNode() const
{
    return _imp->node.lock();
}

bool
HostOverlay::addPositionParam(const boost::shared_ptr<KnobDouble>& position)
{
    assert(QThread::currentThread() == qApp->thread());
    
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        if (it->param.lock() == position) {
            return false;
        }
    }
    
    PositionInteract p;
    p.param = position;
    p.state = ePositionInteractStateInactive;
    _imp->positions.push_back(p);
    return true;
}

bool
HostOverlay::addTransformInteract(const boost::shared_ptr<KnobDouble>& translate,
                                  const boost::shared_ptr<KnobDouble>& scale,
                                  const boost::shared_ptr<KnobBool>& scaleUniform,
                                  const boost::shared_ptr<KnobDouble>& rotate,
                                  const boost::shared_ptr<KnobDouble>& skewX,
                                  const boost::shared_ptr<KnobDouble>& skewY,
                                  const boost::shared_ptr<KnobChoice>& skewOrder,
                                  const boost::shared_ptr<KnobDouble>& center)
{
    assert(QThread::currentThread() == qApp->thread());

    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        if (it->translate.lock() == translate) {
            return false;
        }
    }
    TransformInteract p;
    p.translate = translate;
    p.scale = scale;
    p.scaleUniform = scaleUniform;
    p.rotate = rotate;
    p.skewX = skewX;
    p.skewY = skewY;
    p.skewOrder = skewOrder;
    p.center = center;
    p._drawState = TransformInteract::eInActive;
    p._mouseState = TransformInteract::eReleased;
    _imp->transforms.push_back(p);
    return true;
}

void
HostOverlayPrivate::drawPosition(const PositionInteract& p,
                                 double time,
                                 const OfxPointD& pscale,
                                 const OfxRGBColourD& color,
                                 const OfxPointD& shadow,
                                 const QFont& font,
                                 const QFontMetrics& fm)
{
    boost::shared_ptr<KnobDouble> knob = p.param.lock();
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if (!knob || knob->getIsSecretRecursive() || !knob->isEnabled(0) || !knob->isEnabled(1)) {
        return;
    }
    
    float pR = 1.f;
    float pG = 1.f;
    float pB = 1.f;
    switch (p.state) {
        case ePositionInteractStateInactive:
            pR = (float)color.r; pG = (float)color.g; pB = (float)color.b; break;
        case ePositionInteractStatePoised:
            pR = 0.f; pG = 1.0f; pB = 0.0f; break;
        case ePositionInteractStatePicked:
            pR = 0.f; pG = 1.0f; pB = 0.0f; break;
    }
    
    QPointF pos;
    if (p.state == ePositionInteractStatePicked) {
        pos = lastPenPos;
    } else {
        double p[2];
        for (int i = 0; i < 2; ++i) {
            p[i] = knob->getValueAtTime(time, i);
            if (knob->getValueIsNormalized(i) != KnobDouble::eValueIsNormalizedNone) {
                knob->denormalize(i, time, &p[i]);
            }
        }
        pos.setX(p[0]);
        pos.setY(p[1]);
    }
    //glPushAttrib(GL_ALL_ATTRIB_BITS); // caller is responsible for protecting attribs
    glPointSize( (GLfloat)p.pointSize() );
    // Draw everything twice
    // l = 0: shadow
    // l = 1: drawing
    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        glMatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        glTranslated(direction * shadow.x, -direction * shadow.y, 0);
        glMatrixMode(GL_MODELVIEW); // Modelview should be used on Nuke
        
        glColor3f(pR * l, pG * l, pB * l);
        glBegin(GL_POINTS);
        glVertex2d(pos.x(), pos.y());
        glEnd();
        QColor c;
        c.setRgbF(pR * l, pG * l, pB * l);
        
        textRenderer.renderText(pos.x(), pos.y() - (fm.height() + p.pointSize()) * pscale.y,
                                      pscale.x, pscale.y, QString(knob->getLabel().c_str()), c, font);
    }
}

static void getTargetCenter(const OfxPointD &center, const OfxPointD &translate, OfxPointD *targetCenter)
{
    targetCenter->x = center.x + translate.x;
    targetCenter->y = center.y + translate.y;
}

static void getTargetRadius(const OfxPointD& scale, const OfxPointD& pixelScale, OfxPointD* targetRadius)
{
    targetRadius->x = scale.x * TransformInteract::circleRadiusBase();
    targetRadius->y = scale.y * TransformInteract::circleRadiusBase();
    // don't draw too small. 15 pixels is the limit
    if (std::fabs(targetRadius->x) < TransformInteract::circleRadiusMin() && std::fabs(targetRadius->y) < TransformInteract::circleRadiusMin()) {
        targetRadius->x = targetRadius->x >= 0 ? TransformInteract::circleRadiusMin() : -TransformInteract::circleRadiusMin();
        targetRadius->y = targetRadius->y >= 0 ? TransformInteract::circleRadiusMin() : -TransformInteract::circleRadiusMin();
    } else if (std::fabs(targetRadius->x) > TransformInteract::circleRadiusMax() && std::fabs(targetRadius->y) > TransformInteract::circleRadiusMax()) {
        targetRadius->x = targetRadius->x >= 0 ? TransformInteract::circleRadiusMax() : -TransformInteract::circleRadiusMax();
        targetRadius->y = targetRadius->y >= 0 ? TransformInteract::circleRadiusMax() : -TransformInteract::circleRadiusMax();
    } else {
        if (std::fabs(targetRadius->x) < TransformInteract::circleRadiusMin()) {
            if (targetRadius->x == 0. && targetRadius->y != 0.) {
                targetRadius->y = targetRadius->y > 0 ? TransformInteract::circleRadiusMax() : -TransformInteract::circleRadiusMax();
            } else {
                targetRadius->y *= std::fabs(TransformInteract::circleRadiusMin()/targetRadius->x);
            }
            targetRadius->x = targetRadius->x >= 0 ? TransformInteract::circleRadiusMin() : -TransformInteract::circleRadiusMin();
        }
        if (std::fabs(targetRadius->x) > TransformInteract::circleRadiusMax()) {
            targetRadius->y *= std::fabs(TransformInteract::circleRadiusMax()/targetRadius->x);
            targetRadius->x = targetRadius->x > 0 ? TransformInteract::circleRadiusMax() : -TransformInteract::circleRadiusMax();
        }
        if (std::fabs(targetRadius->y) < TransformInteract::circleRadiusMin()) {
            if (targetRadius->y == 0. && targetRadius->x != 0.) {
                targetRadius->x = targetRadius->x > 0 ? TransformInteract::circleRadiusMax() : -TransformInteract::circleRadiusMax();
            } else {
                targetRadius->x *= std::fabs(TransformInteract::circleRadiusMin()/targetRadius->y);
            }
            targetRadius->y = targetRadius->y >= 0 ? TransformInteract::circleRadiusMin() : -TransformInteract::circleRadiusMin();
        }
        if (std::fabs(targetRadius->y) > TransformInteract::circleRadiusMax()) {
            targetRadius->x *= std::fabs(TransformInteract::circleRadiusMax()/targetRadius->x);
            targetRadius->y = targetRadius->y > 0 ? TransformInteract::circleRadiusMax() : -TransformInteract::circleRadiusMax();
        }
    }
    // the circle axes are not aligned with the images axes, so we cannot use the x and y scales separately
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    targetRadius->x *= meanPixelScale;
    targetRadius->y *= meanPixelScale;
}

static void getTargetPoints(const OfxPointD& targetCenter,
                            const OfxPointD& targetRadius,
                            OfxPointD *left,
                            OfxPointD *bottom,
                            OfxPointD *top,
                            OfxPointD *right)
{
    left->x = targetCenter.x - targetRadius.x ;
    left->y = targetCenter.y;
    right->x = targetCenter.x + targetRadius.x ;
    right->y = targetCenter.y;
    top->x = targetCenter.x;
    top->y = targetCenter.y + targetRadius.y ;
    bottom->x = targetCenter.x;
    bottom->y = targetCenter.y - targetRadius.y ;
}

static void ofxsTransformGetScale(const OfxPointD &scaleParam, bool scaleUniform, OfxPointD* scale)
{
    const double SCALE_MIN = 0.0001;
    scale->x = scaleParam.x;
    if (std::fabs(scale->x) < SCALE_MIN) {
        scale->x = (scale->x >= 0) ? SCALE_MIN : -SCALE_MIN;
    }
    if (scaleUniform) {
        scale->y = scaleParam.x;
    } else {
        scale->y = scaleParam.y;
    }
    if (std::fabs(scale->y) < SCALE_MIN) {
        scale->y = (scale->y >= 0) ? SCALE_MIN : -SCALE_MIN;
    }
}
static void
drawSquare(const OfxRGBColourD& color,
           const OfxPointD& center,
           const OfxPointD& pixelScale,
           bool hovered,
           bool althovered,
           int l)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    if (hovered) {
        if (althovered) {
            glColor3f(0.f*l, 1.f*l, 0.f*l);
        } else {
            glColor3f(1.f*l, 0.f*l, 0.f*l);
        }
    } else {
        glColor3f((float)color.r*l, (float)color.g*l, (float)color.b*l);
    }
    double halfWidth = (TransformInteract::pointSize() / 2.) * meanPixelScale;
    double halfHeight = (TransformInteract::pointSize() / 2.) * meanPixelScale;
    glPushMatrix();
    glTranslated(center.x, center.y, 0.);
    glBegin(GL_POLYGON);
    glVertex2d(- halfWidth, - halfHeight); // bottom left
    glVertex2d(- halfWidth, + halfHeight); // top left
    glVertex2d(+ halfWidth, + halfHeight); // bottom right
    glVertex2d(+ halfWidth, - halfHeight); // top right
    glEnd();
    glPopMatrix();
    
}

static void
drawEllipse(const OfxRGBColourD& color,
            const OfxPointD& center,
            const OfxPointD& targetRadius,
            bool hovered,
            int l)
{
    if (hovered) {
        glColor3f(1.f*l, 0.f*l, 0.f*l);
    } else {
        glColor3f((float)color.r*l, (float)color.g*l, (float)color.b*l);
    }
    
    glPushMatrix();
    //  center the oval at x_center, y_center
    glTranslatef((float)center.x, (float)center.y, 0.f);
    //  draw the oval using line segments
    glBegin(GL_LINE_LOOP);
    // we don't need to be pixel-perfect here, it's just an interact!
    // 40 segments is enough.
    for (int i = 0; i < 40; ++i) {
        double theta = i * 2 * M_PI / 40.;
        glVertex2d(targetRadius.x * std::cos(theta), targetRadius.y * std::sin(theta));
    }
    glEnd();
    
    glPopMatrix();
}

static void
drawSkewBar(const OfxRGBColourD& color,
            const OfxPointD &center,
            const OfxPointD& pixelScale,
            double targetRadiusY,
            bool hovered,
            double angle,
            int l)
{
    if (hovered) {
        glColor3f(1.f*l, 0.f*l, 0.f*l);
    } else {
        glColor3f((float)color.r*l, (float)color.g*l, (float)color.b*l);
    }
    
    // we are not axis-aligned: use the mean pixel scale
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barHalfSize = targetRadiusY + 20. * meanPixelScale;
    
    glPushMatrix();
    glTranslatef((float)center.x, (float)center.y, 0.f);
    glRotated(angle, 0, 0, 1);
    
    glBegin(GL_LINES);
    glVertex2d(0., - barHalfSize);
    glVertex2d(0., + barHalfSize);
    
    if (hovered) {
        double arrowYPosition = targetRadiusY + 10. * meanPixelScale;
        double arrowXHalfSize = 10 * meanPixelScale;
        double arrowHeadOffsetX = 3 * meanPixelScale;
        double arrowHeadOffsetY = 3 * meanPixelScale;
        
        ///draw the central bar
        glVertex2d(- arrowXHalfSize, - arrowYPosition);
        glVertex2d(+ arrowXHalfSize, - arrowYPosition);
        
        ///left triangle
        glVertex2d(- arrowXHalfSize, -  arrowYPosition);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, - arrowYPosition + arrowHeadOffsetY);
        
        glVertex2d(- arrowXHalfSize,- arrowYPosition);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, - arrowYPosition - arrowHeadOffsetY);
        
        ///right triangle
        glVertex2d(+ arrowXHalfSize,- arrowYPosition);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, - arrowYPosition + arrowHeadOffsetY);
        
        glVertex2d(+ arrowXHalfSize,- arrowYPosition);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, - arrowYPosition - arrowHeadOffsetY);
    }
    glEnd();
    glPopMatrix();
}


static void
drawRotationBar(const OfxRGBColourD& color,
                const OfxPointD& pixelScale,
                double targetRadiusX,
                bool hovered,
                bool inverted,
                int l)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    if (hovered) {
        glColor3f(1.f*l, 0.f*l, 0.f*l);
    } else {
        glColor3f(color.r*l, color.g*l, color.b*l);
    }
    
    double barExtra = 30. * meanPixelScale;
    glBegin(GL_LINES);
    glVertex2d(0., 0.);
    glVertex2d(0. + targetRadiusX + barExtra, 0.);
    glEnd();
    
    if (hovered) {
        double arrowCenterX = targetRadiusX + barExtra / 2.;
        
        ///draw an arrow slightly bended. This is an arc of circle of radius 5 in X, and 10 in Y.
        OfxPointD arrowRadius;
        arrowRadius.x = 5. * meanPixelScale;
        arrowRadius.y = 10. * meanPixelScale;
        
        glPushMatrix();
        //  center the oval at x_center, y_center
        glTranslatef((float)arrowCenterX, 0.f, 0);
        //  draw the oval using line segments
        glBegin(GL_LINE_STRIP);
        glVertex2d(0, arrowRadius.y);
        glVertex2d(arrowRadius.x, 0.);
        glVertex2d(0, -arrowRadius.y);
        glEnd();
        
        
        glBegin(GL_LINES);
        ///draw the top head
        glVertex2d(0., arrowRadius.y);
        glVertex2d(0., arrowRadius.y - 5. * meanPixelScale);
        
        glVertex2d(0., arrowRadius.y);
        glVertex2d(4. * meanPixelScale, arrowRadius.y - 3. * meanPixelScale); // 5^2 = 3^2+4^2
        
        ///draw the bottom head
        glVertex2d(0., -arrowRadius.y);
        glVertex2d(0., -arrowRadius.y + 5. * meanPixelScale);
        
        glVertex2d(0., -arrowRadius.y);
        glVertex2d(4. * meanPixelScale, -arrowRadius.y + 3. * meanPixelScale); // 5^2 = 3^2+4^2
        
        glEnd();
        
        glPopMatrix();
    }
    if (inverted) {
        double arrowXPosition = targetRadiusX + barExtra * 1.5;
        double arrowXHalfSize = 10 * meanPixelScale;
        double arrowHeadOffsetX = 3 * meanPixelScale;
        double arrowHeadOffsetY = 3 * meanPixelScale;
        
        glPushMatrix();
        glTranslatef((float)arrowXPosition, 0, 0);
        
        glBegin(GL_LINES);
        ///draw the central bar
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize, 0.);
        
        ///left triangle
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, arrowHeadOffsetY);
        
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, -arrowHeadOffsetY);
        
        ///right triangle
        glVertex2d(+ arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, arrowHeadOffsetY);
        
        glVertex2d(+ arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, -arrowHeadOffsetY);
        glEnd();
        
        glRotated(90., 0., 0., 1.);
        
        glBegin(GL_LINES);
        ///draw the central bar
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize, 0.);
        
        ///left triangle
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, arrowHeadOffsetY);
        
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, -arrowHeadOffsetY);
        
        ///right triangle
        glVertex2d(+ arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, arrowHeadOffsetY);
        
        glVertex2d(+ arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, -arrowHeadOffsetY);
        glEnd();
        
        glPopMatrix();
    }
    
}

void
HostOverlayPrivate::drawTransform(const TransformInteract& p,
                                  double time,
                                  const OfxPointD& pscale,
                                  const OfxRGBColourD& color,
                                  const OfxPointD& shadow,
                                  const QFont& /*font*/,
                                  const QFontMetrics& /*fm*/)
{
    
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    boost::shared_ptr<KnobDouble> translateKnob = p.translate.lock();
    if (!translateKnob || translateKnob->getIsSecretRecursive() || !translateKnob->isEnabled(0)) {
        return;
    }
    
    OfxPointD center;
    OfxPointD translate;
    OfxPointD scaleParam;
    bool scaleUniform;
    double rotate;
    double skewX, skewY;
    int skewOrder;
    bool inverted = false;
    
    if (p._mouseState == TransformInteract::eReleased) {
        p.getCenter(time, &center.x, &center.y);
        p.getTranslate(time, &translate.x, &translate.y);
        p.getScale(time, &scaleParam.x, &scaleParam.y);
        p.getScaleUniform(time, &scaleUniform);
        p.getRotate(time, &rotate);
        p.getSkewX(time, &skewX);
        p.getSkewY(time, &skewY);
        p.getSkewOrder(time, &skewOrder);
    } else {
        center = p._centerDrag;
        translate = p._translateDrag;
        scaleParam = p._scaleParamDrag;
        scaleUniform = p._scaleUniformDrag;
        rotate = p._rotateDrag;
        skewX = p._skewXDrag;
        skewY = p._skewYDrag;
        skewOrder = p._skewOrderDrag;
    }
    
    OfxPointD targetCenter;
    getTargetCenter(center, translate, &targetCenter);
    
    OfxPointD scale;
    ofxsTransformGetScale(scaleParam, scaleUniform, &scale);
    
    OfxPointD targetRadius;
    getTargetRadius(scale, pscale, &targetRadius);
    
    OfxPointD left, right, bottom, top;
    getTargetPoints(targetCenter, targetRadius, &left, &bottom, &top, &right);
    
    
    GLdouble skewMatrix[16];
    skewMatrix[0] = (skewOrder ? 1. : (1.+skewX*skewY)); skewMatrix[1] = skewY; skewMatrix[2] = 0.; skewMatrix[3] = 0;
    skewMatrix[4] = skewX; skewMatrix[5] = (skewOrder ? (1.+skewX*skewY) : 1.); skewMatrix[6] = 0.; skewMatrix[7] = 0;
    skewMatrix[8] = 0.; skewMatrix[9] = 0.; skewMatrix[10] = 1.; skewMatrix[11] = 0;
    skewMatrix[12] = 0.; skewMatrix[13] = 0.; skewMatrix[14] = 0.; skewMatrix[15] = 1.;
    
    //glPushAttrib(GL_ALL_ATTRIB_BITS); // caller is responsible for protecting attribs
    
    glDisable(GL_LINE_STIPPLE);
    glEnable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
    glLineWidth(1.5f);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw everything twice
    // l = 0: shadow
    // l = 1: drawing
    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        glMatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        glTranslated(direction * shadow.x, -direction * shadow.y, 0);
        glMatrixMode(GL_MODELVIEW); // Modelview should be used on Nuke
        
        glColor3f(color.r*l, color.g*l, color.b*l);
        
        glPushMatrix();
        glTranslated(targetCenter.x, targetCenter.y, 0.);
        
        glRotated(rotate, 0, 0., 1.);
        drawRotationBar(color, pscale, targetRadius.x, p._mouseState == TransformInteract::eDraggingRotationBar || p._drawState == TransformInteract::eRotationBarHovered, inverted, l);
        glMultMatrixd(skewMatrix);
        glTranslated(-targetCenter.x, -targetCenter.y, 0.);
        
        drawEllipse(color, targetCenter, targetRadius, p._mouseState == TransformInteract::eDraggingCircle || p._drawState == TransformInteract::eCircleHovered, l);
        
        // add 180 to the angle to draw the arrows on the other side. unfortunately, this requires knowing
        // the mouse position in the ellipse frame
        double flip = 0.;
        if (p._drawState == TransformInteract::eSkewXBarHoverered || p._drawState == TransformInteract::eSkewYBarHoverered) {
            double rot = Transform::toRadians(rotate);
            Transform::Matrix3x3 transformscale;
            transformscale = Transform::matInverseTransformCanonical(0., 0., scale.x, scale.y, skewX, skewY, (bool)skewOrder, rot, targetCenter.x, targetCenter.y);
            
            Transform::Point3D previousPos;
            previousPos.x = lastPenPos.x();
            previousPos.y = lastPenPos.y();
            previousPos.z = 1.;
            previousPos = Transform::matApply(transformscale, previousPos);
            if (previousPos.z != 0) {
                previousPos.x /= previousPos.z;
                previousPos.y /= previousPos.z;
            }
            if ((p._drawState == TransformInteract::eSkewXBarHoverered && previousPos.y > targetCenter.y) ||
                (p._drawState == TransformInteract::eSkewYBarHoverered && previousPos.x > targetCenter.x)) {
                flip = 180.;
            }
        }
        drawSkewBar(color, targetCenter, pscale, targetRadius.y, p._mouseState == TransformInteract::eDraggingSkewXBar || p._drawState == TransformInteract::eSkewXBarHoverered, flip, l);
        drawSkewBar(color, targetCenter, pscale, targetRadius.x, p._mouseState == TransformInteract::eDraggingSkewYBar || p._drawState == TransformInteract::eSkewYBarHoverered, flip - 90., l);
        
        
        drawSquare(color, targetCenter, pscale, p._mouseState == TransformInteract::eDraggingTranslation || p._mouseState == TransformInteract::eDraggingCenter || p._drawState == TransformInteract::eCenterPointHovered, p._modifierStateCtrl, l);
        drawSquare(color, left, pscale, p._mouseState == TransformInteract::eDraggingLeftPoint || p._drawState == TransformInteract::eLeftPointHovered, false, l);
        drawSquare(color, right, pscale, p._mouseState == TransformInteract::eDraggingRightPoint || p._drawState == TransformInteract::eRightPointHovered, false, l);
        drawSquare(color, top, pscale, p._mouseState == TransformInteract::eDraggingTopPoint || p._drawState == TransformInteract::eTopPointHovered, false, l);
        drawSquare(color, bottom, pscale, p._mouseState == TransformInteract::eDraggingBottomPoint || p._drawState == TransformInteract::eBottomPointHovered, false, l);
        
        glPopMatrix();
    }
    //glPopAttrib();
}

void
HostOverlay::draw(double time,const RenderScale & /*renderScale*/)
{
    OfxRGBColourD color;
    if (!getNode()->getOverlayColor(&color.r, &color.g, &color.b)) {
        color.r = color.g = color.b = 0.8;
    }
    OfxPointD pscale;
    n_getPixelScale(pscale.x, pscale.y);
    double w,h;
    n_getViewportSize(w, h);
    
    
    GLdouble projection[16];
    glGetDoublev( GL_PROJECTION_MATRIX, projection);
    OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
    shadow.x = 2./(projection[0] * w);
    shadow.y = 2./(projection[5] * h);
    
    QFont font(appFont,appFontSize);
    QFontMetrics fm(font);
    // draw in reverse order
    for (PositionInteracts::reverse_iterator it = _imp->positions.rbegin(); it != _imp->positions.rend(); ++it) {
        _imp->drawPosition(*it, time, pscale, color, shadow, font, fm);
    }
    for (TransformInteracts::reverse_iterator it = _imp->transforms.rbegin(); it != _imp->transforms.rend(); ++it) {
        _imp->drawTransform(*it, time, pscale, color, shadow, font, fm);
    }

}

bool
HostOverlayPrivate::penMotion(double time,
                          const RenderScale &/*renderScale*/,
                          const QPointF &penPos,
                          const QPoint &/*penPosViewport*/,
                          double /*pressure*/,
                          PositionInteract* it)
{
    boost::shared_ptr<KnobDouble> knob = it->param.lock();
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if (!knob || knob->getIsSecretRecursive() || !knob->isEnabled(0) || !knob->isEnabled(1)) {
        return false;
    }

    OfxPointD pscale;
    _publicInterface->n_getPixelScale(pscale.x, pscale.y);

    QPointF pos;
    if (it->state == ePositionInteractStatePicked) {
        pos = lastPenPos;
    } else {
        boost::shared_ptr<KnobDouble> knob = it->param.lock();
        if (knob) {
            double p[2];
            for (int i = 0; i < 2; ++i) {
                p[i] = knob->getValueAtTime(time, i);
                if (knob->getValueIsNormalized(i) != KnobDouble::eValueIsNormalizedNone) {
                    knob->denormalize(i, time, &p[i]);
                }
            }
            pos.setX(p[0]);
            pos.setY(p[1]);
        }
    }

    bool didSomething = false;
    bool valuesChanged = false;

    switch (it->state) {
        case ePositionInteractStateInactive:
        case ePositionInteractStatePoised: {
            // are we in the box, become 'poised'
            PositionInteractState newState;
            if ( ( std::fabs(penPos.x() - pos.x()) <= it->pointTolerance() * pscale.x) &&
                ( std::fabs(penPos.y() - pos.y()) <= it->pointTolerance() * pscale.y) ) {
                newState = ePositionInteractStatePoised;
            } else   {
                newState = ePositionInteractStateInactive;
            }

            if (it->state != newState) {
                // state changed, must redraw
                requestRedraw();
            }
            it->state = newState;
            //}
        }
            break;

        case ePositionInteractStatePicked: {
            valuesChanged = true;
        }
            break;
    }
    didSomething = (it->state == ePositionInteractStatePoised) || (it->state == ePositionInteractStatePicked);

    if (it->state != ePositionInteractStateInactive && interactiveDrag && valuesChanged) {
        boost::shared_ptr<KnobDouble> knob = it->param.lock();
        if (knob) {
            double p[2];
            p[0] = fround(lastPenPos.x(), pscale.x);
            p[1] = fround(lastPenPos.y(), pscale.y);
            for (int i = 0; i < 2; ++i) {
                if (knob->getValueIsNormalized(i) != KnobDouble::eValueIsNormalizedNone) {
                    knob->normalize(i, time, &p[i]);
                }
            }

            knob->setValues(p[0], p[1], eValueChangedReasonNatronGuiEdited);
        }
    }
    return (didSomething || valuesChanged);
}



static bool squareContains(const Transform::Point3D& pos,
                           const OfxRectD& rect,
                           double toleranceX= 0.,
                           double toleranceY = 0.)
{
    return (pos.x >= (rect.x1 - toleranceX) && pos.x < (rect.x2 + toleranceX)
            && pos.y >= (rect.y1 - toleranceY) && pos.y < (rect.y2 + toleranceY));
}

static bool isOnEllipseBorder(const Transform::Point3D& pos,
                              const OfxPointD& targetRadius,
                              const OfxPointD& targetCenter,
                              double epsilon = 0.1)
{
    
    double v = ((pos.x - targetCenter.x) * (pos.x - targetCenter.x) / (targetRadius.x * targetRadius.x) +
                (pos.y - targetCenter.y) * (pos.y - targetCenter.y) / (targetRadius.y * targetRadius.y));
    if (v <= (1. + epsilon) && v >= (1. - epsilon)) {
        return true;
    }
    return false;
}

static bool isOnSkewXBar(const Transform::Point3D& pos,
                         double targetRadiusY,
                         const OfxPointD& center,
                         const OfxPointD& pixelScale,
                         double tolerance)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barHalfSize = targetRadiusY + (20. * meanPixelScale);
    if (pos.x >= (center.x - tolerance) && pos.x <= (center.x + tolerance) &&
        pos.y >= (center.y - barHalfSize - tolerance) && pos.y <= (center.y + barHalfSize + tolerance)) {
        return true;
    }
    
    return false;
}

static bool isOnSkewYBar(const Transform::Point3D& pos,
                         double targetRadiusX,
                         const OfxPointD& center,
                         const OfxPointD& pixelScale,
                         double tolerance)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barHalfSize = targetRadiusX + (20. * meanPixelScale);
    if (pos.y >= (center.y - tolerance) && pos.y <= (center.y + tolerance) &&
        pos.x >= (center.x - barHalfSize - tolerance) && pos.x <= (center.x + barHalfSize + tolerance)) {
        return true;
    }
    
    return false;
}

static bool isOnRotationBar(const  Transform::Point3D& pos,
                            double targetRadiusX,
                            const OfxPointD& center,
                            const OfxPointD& pixelScale,
                            double tolerance)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barExtra = 30. * meanPixelScale;
    if (pos.x >= (center.x - tolerance) && pos.x <= (center.x + targetRadiusX + barExtra + tolerance) &&
        pos.y >= (center.y  - tolerance) && pos.y <= (center.y + tolerance)) {
        return true;
    }
    
    return false;
}

static OfxRectD rectFromCenterPoint(const OfxPointD& center,
                                    const OfxPointD& pixelScale)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    OfxRectD ret;
    ret.x1 = center.x - (TransformInteract::pointSize() / 2.) * meanPixelScale;
    ret.x2 = center.x + (TransformInteract::pointSize() / 2.) * meanPixelScale;
    ret.y1 = center.y - (TransformInteract::pointSize() / 2.) * meanPixelScale;
    ret.y2 = center.y + (TransformInteract::pointSize() / 2.) * meanPixelScale;
    return ret;
}



bool
HostOverlayPrivate::penMotion(double time,
                              const RenderScale &/*renderScale*/,
                              const QPointF &penPosParam,
                              const QPoint &/*penPosViewport*/,
                              double /*pressure*/,
                              TransformInteract* it)
{
    
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    boost::shared_ptr<KnobDouble> translateKnob = it->translate.lock();
    if (!translateKnob || translateKnob->getIsSecretRecursive() || !translateKnob->isEnabled(0)) {
        return false;
    }
    OfxPointD pscale;
    _publicInterface->n_getPixelScale(pscale.x, pscale.y);
    
    OfxPointD center;
    OfxPointD translate;
    OfxPointD scaleParam;
    bool scaleUniform;
    double rotate;
    double skewX, skewY;
    int skewOrder;
    //bool inverted = false;
    
    if (it->_mouseState == TransformInteract::eReleased) {
        it->getCenter(time, &center.x, &center.y);
        it->getTranslate(time, &translate.x, &translate.y);
        it->getScale(time, &scaleParam.x, &scaleParam.y);
        it->getScaleUniform(time, &scaleUniform);
        it->getRotate(time, &rotate);
        it->getSkewX(time, &skewX);
        it->getSkewY(time, &skewY);
        it->getSkewOrder(time, &skewOrder);
    } else {
        center = it->_centerDrag;
        translate = it->_translateDrag;
        scaleParam = it->_scaleParamDrag;
        scaleUniform = it->_scaleUniformDrag;
        rotate = it->_rotateDrag;
        skewX = it->_skewXDrag;
        skewY = it->_skewYDrag;
        skewOrder = it->_skewOrderDrag;
    }
    
    bool didSomething = false;
    bool centerChanged = false;
    bool translateChanged = false;
    bool scaleChanged = false;
    bool rotateChanged = false;
    bool skewXChanged = false;
    bool skewYChanged = false;
    
    OfxPointD targetCenter;
    getTargetCenter(center, translate, &targetCenter);
    
    OfxPointD scale;
    ofxsTransformGetScale(scaleParam, scaleUniform, &scale);
    
    OfxPointD targetRadius;
    getTargetRadius(scale, pscale, &targetRadius);
    
    OfxPointD left, right, bottom, top;
    getTargetPoints(targetCenter, targetRadius, &left, &bottom, &top, &right);
    
    OfxRectD centerPoint = rectFromCenterPoint(targetCenter, pscale);
    OfxRectD leftPoint = rectFromCenterPoint(left, pscale);
    OfxRectD rightPoint = rectFromCenterPoint(right, pscale);
    OfxRectD topPoint = rectFromCenterPoint(top, pscale);
    OfxRectD bottomPoint = rectFromCenterPoint(bottom, pscale);
    
    
    //double dx = args.penPosition.x - _lastMousePos.x;
    //double dy = args.penPosition.y - _lastMousePos.y;
    
    double rot = Transform::toRadians(rotate);
    
    
    Transform::Point3D penPos, prevPenPos, rotationPos, transformedPos, previousPos, currentPos;
    penPos.x = penPosParam.x();
    penPos.y = penPosParam.y();
    penPos.z = 1.;
    prevPenPos.x = lastPenPos.x();
    prevPenPos.y = lastPenPos.y();
    prevPenPos.z = 1.;
    
    Transform::Matrix3x3 rotation, transform, transformscale;
    ////for the rotation bar/translation/center dragging we dont use the same transform, we don't want to undo the rotation transform
    if (it->_mouseState != TransformInteract::eDraggingTranslation && it->_mouseState != TransformInteract::eDraggingCenter) {
        ///undo skew + rotation to the current position
        rotation = Transform::matInverseTransformCanonical(0., 0., 1., 1., 0., 0., false, rot, targetCenter.x, targetCenter.y);
        transform = Transform::matInverseTransformCanonical(0., 0., 1., 1., skewX, skewY, (bool)skewOrder, rot, targetCenter.x, targetCenter.y);
        transformscale = Transform::matInverseTransformCanonical(0., 0., scale.x, scale.y, skewX, skewY, (bool)skewOrder, rot, targetCenter.x, targetCenter.y);
    } else {
        rotation = Transform::matInverseTransformCanonical(0., 0., 1., 1., 0., 0., false, 0., targetCenter.x, targetCenter.y);
        transform = Transform::matInverseTransformCanonical(0., 0., 1., 1., skewX, skewY, (bool)skewOrder, 0., targetCenter.x, targetCenter.y);
        transformscale = Transform::matInverseTransformCanonical(0., 0., scale.x, scale.y, skewX, skewY, (bool)skewOrder, 0., targetCenter.x, targetCenter.y);
    }
    
    rotationPos = Transform::matApply(rotation, penPos);
    if (rotationPos.z != 0) {
        rotationPos.x /= rotationPos.z;
        rotationPos.y /= rotationPos.z;
    }
    
    transformedPos = Transform::matApply(transform, penPos);
    if (transformedPos.z != 0) {
        transformedPos.x /= transformedPos.z;
        transformedPos.y /= transformedPos.z;
    }
    
    previousPos = Transform::matApply(transformscale , prevPenPos);
    if (previousPos.z != 0) {
        previousPos.x /= previousPos.z;
        previousPos.y /= previousPos.z;
    }
    
    currentPos = Transform::matApply(transformscale, penPos);
    if (currentPos.z != 0) {
        currentPos.x /= currentPos.z;
        currentPos.y /= currentPos.z;
    }
    
    double minX,minY,maxX,maxY;
    
    boost::shared_ptr<KnobDouble> scaleKnob = it->scale.lock();
    assert(scaleKnob);
    
    minX = scaleKnob->getMinimum(0);
    minY = scaleKnob->getMinimum(1);
    maxX = scaleKnob->getMaximum(0);
    maxY = scaleKnob->getMaximum(1);

    if (it->_mouseState == TransformInteract::eReleased) {
        // we are not axis-aligned
        double meanPixelScale = (pscale.x + pscale.y) / 2.;
        double hoverTolerance = (TransformInteract::pointSize() / 2.) * meanPixelScale;
        if (squareContains(transformedPos, centerPoint)) {
            it->_drawState = TransformInteract::eCenterPointHovered;
            didSomething = true;
        } else if (squareContains(transformedPos, leftPoint)) {
            it->_drawState = TransformInteract::eLeftPointHovered;
            didSomething = true;
        } else if (squareContains(transformedPos, rightPoint)) {
            it->_drawState = TransformInteract::eRightPointHovered;
            didSomething = true;
        } else if (squareContains(transformedPos, topPoint)) {
            it->_drawState = TransformInteract::eTopPointHovered;
            didSomething = true;
        } else if (squareContains(transformedPos, bottomPoint)) {
            it->_drawState = TransformInteract::eBottomPointHovered;
            didSomething = true;
        } else if (isOnEllipseBorder(transformedPos, targetRadius, targetCenter)) {
            it->_drawState = TransformInteract::eCircleHovered;
            didSomething = true;
        } else if (isOnRotationBar(rotationPos, targetRadius.x, targetCenter, pscale, hoverTolerance)) {
            it->_drawState = TransformInteract::eRotationBarHovered;
            didSomething = true;
        } else if (isOnSkewXBar(transformedPos, targetRadius.y, targetCenter, pscale, hoverTolerance)) {
            it->_drawState = TransformInteract::eSkewXBarHoverered;
            didSomething = true;
        } else if (isOnSkewYBar(transformedPos, targetRadius.x, targetCenter, pscale, hoverTolerance)) {
            it->_drawState = TransformInteract::eSkewYBarHoverered;
            didSomething = true;
        } else {
            it->_drawState = TransformInteract::eInActive;
        }
    } else if (it->_mouseState == TransformInteract::eDraggingCircle) {
        
        // we need to compute the backtransformed points with the scale
        
        // the scale ratio is the ratio of distances to the center
        double prevDistSq = (targetCenter.x - previousPos.x)*(targetCenter.x - previousPos.x) + (targetCenter.y - previousPos.y)*(targetCenter.y - previousPos.y);
        if (prevDistSq != 0.) {
            const double distSq = (targetCenter.x - currentPos.x)*(targetCenter.x - currentPos.x) + (targetCenter.y - currentPos.y)*(targetCenter.y - currentPos.y);
            const double distRatio = std::sqrt(distSq/prevDistSq);
            scale.x *= distRatio;
            scale.y *= distRatio;
            //_scale->setValue(scale.x, scale.y);
            scaleChanged = true;
        }
    } else if (it->_mouseState == TransformInteract::eDraggingLeftPoint || it->_mouseState == TransformInteract::eDraggingRightPoint) {
        // avoid division by zero
        if (targetCenter.x != previousPos.x) {
            const double scaleRatio = (targetCenter.x - currentPos.x)/(targetCenter.x - previousPos.x);
            OfxPointD newScale;
            newScale.x = scale.x * scaleRatio;
            newScale.x = std::max(minX, std::min(newScale.x, maxX));
            newScale.y = scaleUniform ? newScale.x : scale.y;
            scale = newScale;
            //_scale->setValue(scale.x, scale.y);
            scaleChanged = true;
        }
    } else if (it->_mouseState == TransformInteract::eDraggingTopPoint || it->_mouseState == TransformInteract::eDraggingBottomPoint) {
        // avoid division by zero
        if (targetCenter.y != previousPos.y) {
            const double scaleRatio = (targetCenter.y - currentPos.y)/(targetCenter.y - previousPos.y);
            OfxPointD newScale;
            newScale.y = scale.y * scaleRatio;
            newScale.y = std::max(minY, std::min(newScale.y, maxY));
            newScale.x = scaleUniform ? newScale.y : scale.x;
            scale = newScale;
            //_scale->setValue(scale.x, scale.y);
            scaleChanged = true;
        }
    } else if (it->_mouseState == TransformInteract::eDraggingTranslation) {
        double dx = penPosParam.x() - lastPenPos.x();
        double dy = penPosParam.y() - lastPenPos.y();
        
        if (it->_orientation == TransformInteract::eOrientationNotSet && it->_modifierStateShift > 0) {
            it->_orientation = std::abs(dx) > std::abs(dy) ? TransformInteract::eOrientationHorizontal : TransformInteract::eOrientationVertical;
        }
        
        dx = it->_orientation == TransformInteract::eOrientationVertical ? 0 : dx;
        dy = it->_orientation == TransformInteract::eOrientationHorizontal ? 0 : dy;
        double newx = translate.x + dx;
        double newy = translate.y + dy;
        // round newx/y to the closest int, 1/10 int, etc
        // this make parameter editing easier
        newx = fround(newx, pscale.x);
        newy = fround(newy, pscale.y);
        translate.x = newx;
        translate.y = newy;
        //_translate->setValue(translate.x, translate.y);
        translateChanged = true;
    } else if (it->_mouseState == TransformInteract::eDraggingCenter) {
        OfxPointD currentCenter = center;
        Transform::Matrix3x3 R = Transform::matMul(Transform::matMul(Transform::matScale(1. / scale.x, 1. / scale.y), Transform::matSkewXY(-skewX, -skewY, !skewOrder)), Transform::matRotation(rot));
        
        double dx = penPosParam.x() - lastPenPos.x();
        double dy = penPosParam.y() - lastPenPos.y();
        
        if (it->_orientation == TransformInteract::eOrientationNotSet && it->_modifierStateShift > 0) {
            it->_orientation = std::abs(dx) > std::abs(dy) ? TransformInteract::eOrientationHorizontal : TransformInteract::eOrientationVertical;
        }
        
        dx = it->_orientation == TransformInteract::eOrientationVertical ? 0 : dx;
        dy = it->_orientation == TransformInteract::eOrientationHorizontal ? 0 : dy;
        
        Transform::Point3D dRot;
        dRot.x = dx;
        dRot.y = dy;
        dRot.z = 1.;
        dRot = Transform::matApply(R, dRot);
        if (dRot.z != 0) {
            dRot.x /= dRot.z;
            dRot.y /= dRot.z;
        }
        double dxrot = dRot.x;
        double dyrot = dRot.y;
        double newx = currentCenter.x + dxrot;
        double newy = currentCenter.y + dyrot;
        // round newx/y to the closest int, 1/10 int, etc
        // this make parameter editing easier
        newx = fround(newx, pscale.x);
        newy = fround(newy, pscale.y);
        center.x = newx;
        center.y = newy;
        //_effect->beginEditBlock("setCenter");
        //_center->setValue(center.x, center.y);
        centerChanged = true;
        // recompute dxrot,dyrot after rounding
        double det = Transform::matDeterminant(R);
        if (det != 0.) {
            Transform::Matrix3x3 Rinv = Transform::matInverse(R, det);
            
            dxrot = newx - currentCenter.x;
            dyrot = newy - currentCenter.y;
            dRot.x = dxrot;
            dRot.y = dyrot;
            dRot.z = 1;
            dRot = Transform::matApply(Rinv, dRot);
            if (dRot.z != 0) {
                dRot.x /= dRot.z;
                dRot.y /= dRot.z;
            }
            dx = dRot.x;
            dy = dRot.y;
            OfxPointD newTranslation;
            newTranslation.x = translate.x + dx - dxrot;
            newTranslation.y = translate.y + dy - dyrot;
            translate = newTranslation;
            //_translate->setValue(translate.x, translate.y);
            translateChanged = true;
        }
        //_effect->endEditBlock();
    } else if (it->_mouseState == TransformInteract::eDraggingRotationBar) {
        OfxPointD diffToCenter;
        ///the current mouse position (untransformed) is doing has a certain angle relative to the X axis
        ///which can be computed by : angle = arctan(opposite / adjacent)
        diffToCenter.y = rotationPos.y - targetCenter.y;
        diffToCenter.x = rotationPos.x - targetCenter.x;
        double angle = std::atan2(diffToCenter.y, diffToCenter.x);
        double angledegrees = rotate + Transform::toDegrees(angle);
        double closest90 = 90. * std::floor((angledegrees + 45.)/90.);
        if (std::fabs(angledegrees - closest90) < 5.) {
            // snap to closest multiple of 90.
            angledegrees = closest90;
        }
        rotate = angledegrees;
        //_rotate->setValue(rotate);
        rotateChanged = true;
        
    } else if (it->_mouseState == TransformInteract::eDraggingSkewXBar) {
        // avoid division by zero
        if (scale.y != 0. && targetCenter.y != previousPos.y) {
            const double addSkew = (scale.x/scale.y)*(currentPos.x - previousPos.x)/(currentPos.y - targetCenter.y);
            skewX = skewX + addSkew;
            //_skewX->setValue(skewX);
            skewXChanged = true;
        }
    } else if (it->_mouseState == TransformInteract::eDraggingSkewYBar) {
        // avoid division by zero
        if (scale.x != 0. && targetCenter.x != previousPos.x) {
            const double addSkew = (scale.y/scale.x)*(currentPos.y - previousPos.y)/(currentPos.x - targetCenter.x);
            skewY = skewY + addSkew;
            //_skewY->setValue(skewY + addSkew);
            skewYChanged = true;
        }
    } else {
        assert(false);
    }
    
    it->_centerDrag = center;
    it->_translateDrag = translate;
    it->_scaleParamDrag = scale;
    it->_scaleUniformDrag = scaleUniform;
    it->_rotateDrag = rotate;
    it->_skewXDrag = skewX;
    it->_skewYDrag = skewY;
    it->_skewOrderDrag = skewOrder;
    
    bool valuesChanged = (centerChanged || translateChanged || scaleChanged || rotateChanged || skewXChanged || skewYChanged);
    
    if (it->_mouseState != TransformInteract::eReleased && it->_interactiveDrag && valuesChanged) {
        // no need to redraw overlay since it is slave to the paramaters
        KnobHolder* holder = node.lock()->getNode()->getLiveInstance();
        holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
        if (centerChanged) {
            boost::shared_ptr<KnobDouble> knob = it->center.lock();
            knob->setValues(center.x, center.y, eValueChangedReasonNatronGuiEdited);
        }
        if (translateChanged) {
            boost::shared_ptr<KnobDouble> knob = it->translate.lock();
            knob->setValues(translate.x, translate.y, eValueChangedReasonNatronGuiEdited);
        }
        if (scaleChanged) {
            boost::shared_ptr<KnobDouble> knob = it->scale.lock();
            knob->setValues(scale.x, scale.y, eValueChangedReasonNatronGuiEdited);
        }
        if (rotateChanged) {
            boost::shared_ptr<KnobDouble> knob = it->rotate.lock();
            knob->setValue(rotate, 0, eValueChangedReasonNatronGuiEdited);
        }
        if (skewXChanged) {
            boost::shared_ptr<KnobDouble> knob = it->skewX.lock();
            knob->setValue(skewX, 0, eValueChangedReasonNatronGuiEdited);
        }
        if (skewYChanged) {
            boost::shared_ptr<KnobDouble> knob = it->skewY.lock();
            knob->setValue(skewY, 0, eValueChangedReasonNatronGuiEdited);
        }
        holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    } else if (didSomething || valuesChanged) {
        requestRedraw();
    }
    
    return didSomething || valuesChanged;
}

bool
HostOverlay::penMotion(double time,
                          const RenderScale &renderScale,
                          const QPointF &penPos,
                          const QPoint &penPosViewport,
                          double pressure)
{
    OfxPointD pscale;
    n_getPixelScale(pscale.x, pscale.y);
    
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        if (_imp->penMotion(time, renderScale, penPos, penPosViewport, pressure, &(*it))) {
            _imp->lastPenPos = penPos;
            return true;
        }
    }
    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        if (_imp->penMotion(time, renderScale, penPos, penPosViewport, pressure, &(*it))) {
            _imp->lastPenPos = penPos;
            return true;
        }
    }
    _imp->lastPenPos = penPos;
    return false;
}


bool
HostOverlayPrivate::penUp(double time,
                      const RenderScale &renderScale,
                      const QPointF &penPos,
                      const QPoint &penPosViewport,
                      double  pressure,
                      PositionInteract* it)
{
    boost::shared_ptr<KnobDouble> knob = it->param.lock();
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if (!knob || knob->getIsSecretRecursive() || !knob->isEnabled(0) || !knob->isEnabled(1)) {
        return false;
    }

    OfxPointD pscale;
    _publicInterface->n_getPixelScale(pscale.x, pscale.y);
    
    bool didSomething = false;
    if (it->state == ePositionInteractStatePicked) {
        if (!interactiveDrag) {
            boost::shared_ptr<KnobDouble> knob = it->param.lock();
            if (knob) {
                double p[2];
                p[0] = fround(lastPenPos.x(), pscale.x);
                p[1] = fround(lastPenPos.y(), pscale.y);
                for (int i = 0; i < 2; ++i) {
                    if (knob->getValueIsNormalized(i) != KnobDouble::eValueIsNormalizedNone) {
                        knob->normalize(i, time, &p[i]);
                    }
                }

                knob->setValues(p[0], p[1], eValueChangedReasonNatronGuiEdited);
            }
        }

        it->state = ePositionInteractStateInactive;
        bool motion = penMotion(time, renderScale, penPos, penPosViewport, pressure, it);
        Q_UNUSED(motion);
        didSomething = true;
    }

    return didSomething;
}

bool
HostOverlayPrivate::penUp(double /*time*/,
                          const RenderScale &/*renderScale*/,
                          const QPointF &/*penPos*/,
                          const QPoint &/*penPosViewport*/,
                          double  /*pressure*/,
                          TransformInteract* it)
{

    
    bool ret = it->_mouseState != TransformInteract::eReleased;
    
    if (!it->_interactiveDrag && it->_mouseState != TransformInteract::eReleased) {
        // no need to redraw overlay since it is slave to the paramaters
        
        /*
         Give eValueChangedReasonPluginEdited reason so that the command uses the undo/redo stack
         see Knob::setValue
         */
        KnobHolder* holder = node.lock()->getNode()->getLiveInstance();
        holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
        {
            boost::shared_ptr<KnobDouble> knob = it->center.lock();
            knob->setValues(it->_centerDrag.x, it->_centerDrag.y, eValueChangedReasonPluginEdited);
        }
        {
            boost::shared_ptr<KnobDouble> knob = it->translate.lock();
            knob->setValues(it->_translateDrag.x, it->_translateDrag.y, eValueChangedReasonPluginEdited);
        }
        {
            boost::shared_ptr<KnobDouble> knob = it->scale.lock();
            knob->setValues(it->_scaleParamDrag.x, it->_scaleParamDrag.y, eValueChangedReasonPluginEdited);
        }
        {
            boost::shared_ptr<KnobDouble> knob = it->rotate.lock();
            KeyFrame k;
            knob->setValue(it->_rotateDrag, 0, eValueChangedReasonPluginEdited, &k);
        }
        {
            boost::shared_ptr<KnobDouble> knob = it->skewX.lock();
            KeyFrame k;
            knob->setValue(it->_skewXDrag, 0, eValueChangedReasonPluginEdited, &k);
        }
        {
            boost::shared_ptr<KnobDouble> knob = it->skewY.lock();
            KeyFrame k;
            knob->setValue(it->_skewYDrag, 0, eValueChangedReasonPluginEdited,&k);
        }
        holder->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
        

    } else if (it->_mouseState != TransformInteract::eReleased) {
        requestRedraw();
    }
    
    it->_mouseState = TransformInteract::eReleased;
    
    return ret;

}

bool
HostOverlay::penUp(double time,
                      const RenderScale &renderScale,
                      const QPointF &penPos,
                      const QPoint &penPosViewport,
                      double  pressure)
{
    OfxPointD pscale;
    n_getPixelScale(pscale.x, pscale.y);

    bool didSomething = false;
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        didSomething |= _imp->penUp(time, renderScale, penPos, penPosViewport, pressure, &(*it));
    }
    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        didSomething |= _imp->penUp(time, renderScale, penPos, penPosViewport, pressure, &(*it));
    }
    return didSomething;
}


bool
HostOverlayPrivate::penDown(double time,
                        const RenderScale &renderScale,
                        const QPointF &penPos,
                        const QPoint &penPosViewport,
                        double  pressure,
                        PositionInteract* it)
{
    boost::shared_ptr<KnobDouble> knob = it->param.lock();
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if (!knob || knob->getIsSecretRecursive() || !knob->isEnabled(0) || !knob->isEnabled(1)) {
        return false;
    }

    bool motion = penMotion(time,renderScale,penPos,penPosViewport,pressure, it);
    Q_UNUSED(motion);
    if (it->state == ePositionInteractStatePoised) {
        it->state = ePositionInteractStatePicked;
        if (interactiveDrag) {
            interactiveDrag = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
        }
        return true;
    }
    return false;
}

bool
HostOverlayPrivate::penDown(double time,
                            const RenderScale &/*renderScale*/,
                            const QPointF &penPosParam,
                            const QPoint &/*penPosViewport*/,
                            double  /*pressure*/,
                            TransformInteract* it)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    boost::shared_ptr<KnobDouble> translateKnob = it->translate.lock();
    if (!translateKnob || translateKnob->getIsSecretRecursive() || !translateKnob->isEnabled(0)) {
        return false;
    }
    OfxPointD pscale;
    _publicInterface->n_getPixelScale(pscale.x, pscale.y);
    
    OfxPointD center;
    OfxPointD translate;
    OfxPointD scaleParam;
    bool scaleUniform;
    double rotate;
    double skewX, skewY;
    int skewOrder;
    //bool inverted = false;
    
    if (it->_mouseState == TransformInteract::eReleased) {
        it->getCenter(time, &center.x, &center.y);
        it->getTranslate(time, &translate.x, &translate.y);
        it->getScale(time, &scaleParam.x, &scaleParam.y);
        it->getScaleUniform(time, &scaleUniform);
        it->getRotate(time, &rotate);
        it->getSkewX(time, &skewX);
        it->getSkewY(time, &skewY);
        it->getSkewOrder(time, &skewOrder);
    } else {
        center = it->_centerDrag;
        translate = it->_translateDrag;
        scaleParam = it->_scaleParamDrag;
        scaleUniform = it->_scaleUniformDrag;
        rotate = it->_rotateDrag;
        skewX = it->_skewXDrag;
        skewY = it->_skewYDrag;
        skewOrder = it->_skewOrderDrag;
    }

    
    OfxPointD targetCenter;
    getTargetCenter(center, translate, &targetCenter);
    
    OfxPointD scale;
    ofxsTransformGetScale(scaleParam, scaleUniform, &scale);
    
    OfxPointD targetRadius;
    getTargetRadius(scale, pscale, &targetRadius);
    
    OfxPointD left, right, bottom, top;
    getTargetPoints(targetCenter, targetRadius, &left, &bottom, &top, &right);
    
    OfxRectD centerPoint = rectFromCenterPoint(targetCenter, pscale);
    OfxRectD leftPoint = rectFromCenterPoint(left, pscale);
    OfxRectD rightPoint = rectFromCenterPoint(right, pscale);
    OfxRectD topPoint = rectFromCenterPoint(top, pscale);
    OfxRectD bottomPoint = rectFromCenterPoint(bottom, pscale);
    
    
    Transform::Point3D transformedPos, rotationPos;
    transformedPos.x = penPosParam.x();
    transformedPos.y = penPosParam.y();
    transformedPos.z = 1.;
    
    double rot = Transform::toRadians(rotate);
    
    ///now undo skew + rotation to the current position
    Transform::Matrix3x3 rotation, transform;
    rotation = Transform::matInverseTransformCanonical(0., 0., 1., 1., 0., 0., false, rot, targetCenter.x, targetCenter.y);
    transform = Transform::matInverseTransformCanonical(0., 0., 1., 1., skewX, skewY, (bool)skewOrder, rot, targetCenter.x, targetCenter.y);
    
    rotationPos = Transform::matApply(rotation, transformedPos);
    if (rotationPos.z != 0) {
        rotationPos.x /= rotationPos.z;
        rotationPos.y /= rotationPos.z;
    }
    transformedPos = Transform::matApply(transform, transformedPos);
    if (transformedPos.z != 0) {
        transformedPos.x /= transformedPos.z;
        transformedPos.y /= transformedPos.z;
    }
    
    it->_orientation = TransformInteract::eOrientationAllDirections;
    
    double pressToleranceX = 5 * pscale.x;
    double pressToleranceY = 5 * pscale.y;
    
    bool didSomething = false;
    if (squareContains(transformedPos, centerPoint,pressToleranceX,pressToleranceY)) {
        it->_mouseState = it->_modifierStateCtrl ? TransformInteract::eDraggingCenter : TransformInteract::eDraggingTranslation;
        if (it->_modifierStateShift > 0) {
            it->_orientation = TransformInteract::eOrientationNotSet;
        }
        didSomething = true;
        
    } else if (squareContains(transformedPos, leftPoint,pressToleranceX,pressToleranceY)) {
        it->_mouseState = TransformInteract::eDraggingLeftPoint;
        didSomething = true;
        
    } else if (squareContains(transformedPos, rightPoint,pressToleranceX,pressToleranceY)) {
        it->_mouseState = TransformInteract::eDraggingRightPoint;
        didSomething = true;
        
    } else if (squareContains(transformedPos, topPoint,pressToleranceX,pressToleranceY)) {
        it->_mouseState = TransformInteract::eDraggingTopPoint;
        didSomething = true;
        
    } else if (squareContains(transformedPos, bottomPoint,pressToleranceX,pressToleranceY)) {
        it->_mouseState = TransformInteract::eDraggingBottomPoint;
        didSomething = true;
        
    } else if (isOnEllipseBorder(transformedPos, targetRadius, targetCenter)) {
        it->_mouseState = TransformInteract::eDraggingCircle;
        didSomething = true;
        
    } else if (isOnRotationBar(rotationPos, targetRadius.x, targetCenter, pscale, pressToleranceY)) {
        it->_mouseState = TransformInteract::eDraggingRotationBar;
        didSomething = true;
        
    } else if (isOnSkewXBar(transformedPos, targetRadius.y, targetCenter, pscale, pressToleranceY)) {
        it->_mouseState = TransformInteract::eDraggingSkewXBar;
        didSomething = true;
        
    } else if (isOnSkewYBar(transformedPos, targetRadius.x, targetCenter, pscale, pressToleranceX)) {
        it->_mouseState = TransformInteract::eDraggingSkewYBar;
        didSomething = true;
        
    } else {
        it->_mouseState = TransformInteract::eReleased;
    }
    
    it->_centerDrag = center;
    it->_translateDrag = translate;
    it->_scaleParamDrag = scaleParam;
    it->_scaleUniformDrag = scaleUniform;
    it->_rotateDrag = rotate;
    it->_skewXDrag = skewX;
    it->_skewYDrag = skewY;
    it->_skewOrderDrag = skewOrder;
    //it->_invertedDrag = inverted;
    
    if (didSomething) {
        requestRedraw();
    }
    
    return didSomething;
}

bool
HostOverlay::penDown(double time,
                        const RenderScale &renderScale,
                        const QPointF &penPos,
                        const QPoint &penPosViewport,
                        double  pressure)
{
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        if (_imp->penDown(time, renderScale, penPos, penPosViewport, pressure, &(*it))) {
            _imp->lastPenPos = penPos;
            return true;
        }
        
    }
    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        if (_imp->penDown(time, renderScale, penPos, penPosViewport, pressure, &(*it))) {
            _imp->lastPenPos = penPos;
            return true;
        }
        
    }
    _imp->lastPenPos = penPos;
    return false;
}


bool
HostOverlayPrivate::keyDown(double /*time*/,
                        const RenderScale &/*renderScale*/,
                        int     /*key*/,
                        char*   /*keyString*/,
                        PositionInteract* /*it*/)
{
    return false;
}

bool
HostOverlayPrivate::keyDown(double /*time*/,
                            const RenderScale &/*renderScale*/,
                            int     key,
                            char*   /*keyString*/,
                            TransformInteract* it)
{
    // Always process, even if interact is not open, since this concerns modifiers
    //if (!_interactOpen->getValueAtTime(args.time)) {
    //    return false;
    //}
    
    // Note that on the Mac:
    // cmd/apple/cloverleaf is kOfxKey_Control_L
    // ctrl is kOfxKey_Meta_L
    // alt/option is kOfxKey_Alt_L
    bool mustRedraw = false;
    
    // the two control keys may be pressed consecutively, be aware about this
    if (key == kOfxKey_Control_L || key == kOfxKey_Control_R) {
        mustRedraw = it->_modifierStateCtrl == 0;
        ++it->_modifierStateCtrl;
    }
    if (key == kOfxKey_Shift_L || key == kOfxKey_Shift_R) {
        mustRedraw = it->_modifierStateShift == 0;
        ++it->_modifierStateShift;
        if (it->_modifierStateShift > 0) {
            it->_orientation = TransformInteract::eOrientationNotSet;
        }
    }
    if (mustRedraw) {
        requestRedraw();
    }
    //std::cout << std::hex << args.keySymbol << std::endl;
    // modifiers are not "caught"
    return false;
}

bool
HostOverlay::keyDown(double time,
                        const RenderScale &renderScale,
                        int     key,
                        char*   keyString)
{
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        if (_imp->keyDown(time, renderScale, key, keyString, &(*it))) {
            return true;
        }

    }
    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        if (_imp->keyDown(time, renderScale, key, keyString, &(*it))) {
            return true;
        }
        
    }
    return false;
}


bool
HostOverlayPrivate::keyUp(double /*time*/,
                      const RenderScale &/*renderScale*/,
                      int     /*key*/,
                      char*   /*keyString*/,
                      PositionInteract* /*it*/)
{
    return false;
}

bool
HostOverlayPrivate::keyUp(double /*time*/,
                          const RenderScale &/*renderScale*/,
                          int     key,
                          char*   /*keyString*/,
                          TransformInteract* it)
{
    
    bool mustRedraw = false;
    
    if (key == kOfxKey_Control_L || key == kOfxKey_Control_R) {
        // we may have missed a keypress
        if (it->_modifierStateCtrl > 0) {
            --it->_modifierStateCtrl;
            mustRedraw = it->_modifierStateCtrl == 0;
        }
    }
    if (key == kOfxKey_Shift_L || key == kOfxKey_Shift_R) {
        if (it->_modifierStateShift > 0) {
            --it->_modifierStateShift;
            mustRedraw = it->_modifierStateShift == 0;
        }
        if (it->_modifierStateShift == 0) {
            it->_orientation = TransformInteract::eOrientationAllDirections;
        }
    }
    if (mustRedraw) {
        requestRedraw();
    }
    // modifiers are not "caught"
    return false;
}

bool
HostOverlay::keyUp(double time,
                      const RenderScale &renderScale,
                      int     key,
                      char*   keyString)
{
    bool didSomething = false;
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        didSomething |= _imp->keyUp(time, renderScale, key, keyString, &(*it));
    }
    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        didSomething |= _imp->keyUp(time, renderScale, key, keyString, &(*it));
    }
    return didSomething;
}


bool
HostOverlayPrivate::keyRepeat(double /*time*/,
                          const RenderScale &/*renderScale*/,
                          int     /*key*/,
                          char*   /*keyString*/,
                          PositionInteract* /*it*/)
{
    return false;
}

bool
HostOverlayPrivate::keyRepeat(double /*time*/,
                              const RenderScale &/*renderScale*/,
                              int     /*key*/,
                              char*   /*keyString*/,
                              TransformInteract* /*it*/)
{
    return false;
}

bool
HostOverlay::keyRepeat(double time,
               const RenderScale &renderScale,
               int     key,
               char*   keyString)
{
    bool didSomething = false;
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        didSomething |= _imp->keyRepeat(time, renderScale, key, keyString, &(*it));
    }
    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        didSomething |= _imp->keyRepeat(time, renderScale, key, keyString, &(*it));
    }
    return didSomething;
}


bool
HostOverlayPrivate::gainFocus(double /*time*/,
                          const RenderScale &/*renderScale*/,
                          PositionInteract* /*it*/)
{
    return false;
}

bool
HostOverlayPrivate::gainFocus(double /*time*/,
                              const RenderScale &/*renderScale*/,
                              TransformInteract* /*it*/)
{
    return false;
}

bool
HostOverlay::gainFocus(double time,
                          const RenderScale &renderScale)
{
    bool didSomething = false;
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        didSomething |= _imp->gainFocus(time, renderScale, &(*it));
    }
    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        didSomething |= _imp->gainFocus(time, renderScale, &(*it));
    }
    return didSomething;
}


bool
HostOverlayPrivate::loseFocus(double  /*time*/,
                          const RenderScale &/*renderScale*/,
                          PositionInteract* it)
{
    boost::shared_ptr<KnobDouble> knob = it->param.lock();
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if (!knob || knob->getIsSecretRecursive() || !knob->isEnabled(0) || !knob->isEnabled(1)) {
        return false;
    }

    if (it->state != ePositionInteractStateInactive) {
        it->state = ePositionInteractStateInactive;
        // state changed, must redraw
        requestRedraw();
    }
    return false;
}

bool
HostOverlayPrivate::loseFocus(double  /*time*/,
                              const RenderScale &/*renderScale*/,
                              TransformInteract* it)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    boost::shared_ptr<KnobDouble> translateKnob = it->translate.lock();
    if (!translateKnob || translateKnob->getIsSecretRecursive() || !translateKnob->isEnabled(0)) {
        return false;
    }
    // reset the modifiers state
    it->_modifierStateCtrl = 0;
    it->_modifierStateShift = 0;
    it->_interactiveDrag = false;
    it->_mouseState = TransformInteract::eReleased;
    it->_drawState = TransformInteract::eInActive;

    return false;
}

bool
HostOverlay::loseFocus(double  time,
                          const RenderScale &renderScale)
{
    bool didSomething = false;
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        didSomething |= _imp->loseFocus(time, renderScale, &(*it));
    }
    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        didSomething |= _imp->loseFocus(time, renderScale, &(*it));
    }
    return didSomething;
}

bool
HostOverlay::hasHostOverlayForParam(const KnobI* param)
{
    assert(QThread::currentThread() == qApp->thread());
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        if (it->param.lock().get() == param) {
            return true;
        }
    }
    for (TransformInteracts::iterator it = _imp->transforms.begin(); it != _imp->transforms.end(); ++it) {
        if (it->translate.lock().get() == param ||
            it->scale.lock().get() == param ||
            it->scaleUniform.lock().get() == param ||
            it->rotate.lock().get() == param ||
            it->center.lock().get() == param ||
            it->skewOrder.lock().get() == param ||
            it->skewX.lock().get() == param ||
            it->skewY.lock().get() == param) {
            return true;
        }
    }
    return false;
}

void
HostOverlay::removePositionHostOverlay(KnobI* knob)
{
    for (PositionInteracts::iterator it = _imp->positions.begin(); it != _imp->positions.end(); ++it) {
        if (it->param.lock().get() == knob) {
            _imp->positions.erase(it);
            return;
        }
    }
}

bool
HostOverlay::isEmpty() const
{
    if (_imp->positions.empty() && _imp->transforms.empty()) {
        return true;
    }
    return false;
}

NATRON_NAMESPACE_EXIT;
