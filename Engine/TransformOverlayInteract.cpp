/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "TransformOverlayInteract.h"

#include "Engine/AppManager.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/EffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"
#include "Engine/Utils.h"

NATRON_NAMESPACE_ENTER



struct TransformOverlayInteractPrivate
{

    enum DrawStateEnum
    {
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

    enum MouseStateEnum
    {
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


    KnobDoubleWPtr translate;
    KnobDoubleWPtr scale;
    KnobBoolWPtr scaleUniform;
    KnobDoubleWPtr rotate;
    KnobDoubleWPtr center;
    KnobDoubleWPtr skewX;
    KnobDoubleWPtr skewY;
    KnobChoiceWPtr skewOrder;
    KnobBoolWPtr invert;
    KnobBoolWPtr interactive;

    QPointF lastPenPos;

    DrawStateEnum drawState;
    MouseStateEnum mouseState;
    int modifierStateCtrl;
    int modifierStateShift;
    OrientationEnum orientation;
    Point centerDrag;
    Point translateDrag;
    Point scaleParamDrag;
    bool scaleUniformDrag;
    double rotateDrag;
    double skewXDrag;
    double skewYDrag;
    int skewOrderDrag;
    bool invertedDrag;
    bool interactiveDrag;

    TransformOverlayInteractPrivate()
    : lastPenPos()
    , drawState(eInActive)
    , mouseState(eReleased)
    , modifierStateCtrl(0)
    , modifierStateShift(0)
    , orientation(eOrientationAllDirections)
    , centerDrag()
    , translateDrag()
    , scaleParamDrag()
    , scaleUniformDrag(false)
    , rotateDrag(0)
    , skewXDrag(0)
    , skewYDrag(0)
    , skewOrderDrag(0)
    , invertedDrag(false)
    , interactiveDrag(false)
    {

    }


    static double circleRadiusBase() { return 30.; }

    static double circleRadiusMin() { return 15.; }

    static double circleRadiusMax() { return 300.; }

    static double pointSize() { return 7.; }

    static double ellipseNPoints() { return 50.; }


    void getTranslate(TimeValue time,
                      double* tx,
                      double* ty) const
    {
        KnobDoublePtr knob = translate.lock();

        assert(knob);
        *tx = knob->getValueAtTime(time, DimIdx(0));
        *ty = knob->getValueAtTime(time, DimIdx(1));
    }

    void getCenter(TimeValue time,
                   double* cx,
                   double* cy) const
    {
        KnobDoublePtr knob = center.lock();

        assert(knob);
        *cx = knob->getValueAtTime(time, DimIdx(0));
        *cy = knob->getValueAtTime(time, DimIdx(1));
    }

    void getScale(TimeValue time,
                  double* sx,
                  double* sy) const
    {
        KnobDoublePtr knob = scale.lock();

        assert(knob);
        *sx = knob->getValueAtTime(time, DimIdx(0));
        *sy = knob->getValueAtTime(time, DimIdx(1));
    }

    void getRotate(TimeValue time,
                   double* rot) const
    {
        KnobDoublePtr knob = rotate.lock();

        assert(knob);
        *rot = knob->getValueAtTime(time, DimIdx(0));
    }

    void getSkewX(TimeValue time,
                  double* x) const
    {
        KnobDoublePtr knob = skewX.lock();

        assert(knob);
        *x = knob->getValueAtTime(time, DimIdx(0));
    }

    void getSkewY(TimeValue time,
                  double* y) const
    {
        KnobDoublePtr knob = skewY.lock();

        assert(knob);
        *y = knob->getValueAtTime(time, DimIdx(0));
    }

    void getSkewOrder(TimeValue time,
                      int* order) const
    {
        KnobChoicePtr knob = skewOrder.lock();

        assert(knob);
        *order = knob->getValueAtTime(time, DimIdx(0));
    }

    void getScaleUniform(TimeValue time,
                         bool* uniform) const
    {
        KnobBoolPtr knob = scaleUniform.lock();

        assert(knob);
        *uniform = knob->getValueAtTime(time, DimIdx(0));
    }

    bool getInvert(TimeValue time) const
    {
        KnobBoolPtr knob = invert.lock();

        if (knob) {
            return knob->getValueAtTime(time);
        } else {
            return false;
        }
    }

    bool getInteractive() const
    {
        if ( interactive.lock() ) {
            return interactive.lock()->getValue();
        } else {
            return !appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
        }
    }

};

TransformOverlayInteract::TransformOverlayInteract()
: OverlayInteractBase()
, _imp(new TransformOverlayInteractPrivate())
{
    
}

TransformOverlayInteract::~TransformOverlayInteract()
{

}

void
TransformOverlayInteract::describeKnobs(OverlayInteractBase::KnobsDescMap* desc) const
{
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["translate"];
        knob.type = KnobDouble::typeNameStatic();
        knob.nDims = 2;
        knob.optional = false;
    }
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["scale"];
        knob.type = KnobDouble::typeNameStatic();
        knob.nDims = 2;
        knob.optional = false;
    }
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["scaleUniform"];
        knob.type = KnobBool::typeNameStatic();
        knob.nDims = 1;
        knob.optional = false;
    }
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["rotate"];
        knob.type = KnobDouble::typeNameStatic();
        knob.nDims = 1;
        knob.optional = false;
    }
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["center"];
        knob.type = KnobDouble::typeNameStatic();
        knob.nDims = 2;
        knob.optional = false;
    }
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["skewX"];
        knob.type = KnobDouble::typeNameStatic();
        knob.nDims = 1;
        knob.optional = false;
    }
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["skewY"];
        knob.type = KnobDouble::typeNameStatic();
        knob.nDims = 1;
        knob.optional = false;
    }
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["skewOrder"];
        knob.type = KnobChoice::typeNameStatic();
        knob.nDims = 1;
        knob.optional = false;
    }
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["invert"];
        knob.type = KnobBool::typeNameStatic();
        knob.nDims = 1;
        knob.optional = true;
    }
    {

        OverlayInteractBase::KnobDesc& knob = (*desc)["interactive"];
        knob.type = KnobBool::typeNameStatic();
        knob.nDims = 1;
        knob.optional = true;
    }

}

void
TransformOverlayInteract::fetchKnobs(const std::map<std::string, std::string>& knobs)
{
    _imp->translate = getEffectKnobByRole<KnobDouble>(knobs, "translate", 2, false);
    _imp->scale = getEffectKnobByRole<KnobDouble>(knobs, "scale", 2, false);
    _imp->scaleUniform = getEffectKnobByRole<KnobBool>(knobs, "scaleUniform", 1, false);
    _imp->center = getEffectKnobByRole<KnobDouble>(knobs, "center", 2, false);
    _imp->rotate = getEffectKnobByRole<KnobDouble>(knobs, "rotate", 1, false);
    _imp->skewX = getEffectKnobByRole<KnobDouble>(knobs, "skewX", 1, false);
    _imp->skewY = getEffectKnobByRole<KnobDouble>(knobs, "skewY", 1, false);
    _imp->skewOrder = getEffectKnobByRole<KnobChoice>(knobs, "skewOrder", 1, false);
    _imp->invert = getEffectKnobByRole<KnobBool>(knobs, "invert", 1, true);
    _imp->interactive = getEffectKnobByRole<KnobBool>(knobs, "interactive", 1, true);
}


static void
getTargetCenter(const OfxPointD &center,
                const OfxPointD &translate,
                OfxPointD *targetCenter)
{
    targetCenter->x = center.x + translate.x;
    targetCenter->y = center.y + translate.y;
}

static void
getTargetRadius(const OfxPointD& scale,
                const OfxPointD& pixelScale,
                OfxPointD* targetRadius)
{
    targetRadius->x = scale.x * TransformOverlayInteractPrivate::circleRadiusBase();
    targetRadius->y = scale.y * TransformOverlayInteractPrivate::circleRadiusBase();
    // don't draw too small. 15 pixels is the limit
    if ( ( std::fabs(targetRadius->x) < TransformOverlayInteractPrivate::circleRadiusMin() ) && ( std::fabs(targetRadius->y) < TransformOverlayInteractPrivate::circleRadiusMin() ) ) {
        targetRadius->x = targetRadius->x >= 0 ? TransformOverlayInteractPrivate::circleRadiusMin() : -TransformOverlayInteractPrivate::circleRadiusMin();
        targetRadius->y = targetRadius->y >= 0 ? TransformOverlayInteractPrivate::circleRadiusMin() : -TransformOverlayInteractPrivate::circleRadiusMin();
    } else if ( ( std::fabs(targetRadius->x) > TransformOverlayInteractPrivate::circleRadiusMax() ) && ( std::fabs(targetRadius->y) > TransformOverlayInteractPrivate::circleRadiusMax() ) ) {
        targetRadius->x = targetRadius->x >= 0 ? TransformOverlayInteractPrivate::circleRadiusMax() : -TransformOverlayInteractPrivate::circleRadiusMax();
        targetRadius->y = targetRadius->y >= 0 ? TransformOverlayInteractPrivate::circleRadiusMax() : -TransformOverlayInteractPrivate::circleRadiusMax();
    } else {
        if ( std::fabs(targetRadius->x) < TransformOverlayInteractPrivate::circleRadiusMin() ) {
            if ( (targetRadius->x == 0.) && (targetRadius->y != 0.) ) {
                targetRadius->y = targetRadius->y > 0 ? TransformOverlayInteractPrivate::circleRadiusMax() : -TransformOverlayInteractPrivate::circleRadiusMax();
            } else {
                targetRadius->y *= std::fabs(TransformOverlayInteractPrivate::circleRadiusMin() / targetRadius->x);
            }
            targetRadius->x = targetRadius->x >= 0 ? TransformOverlayInteractPrivate::circleRadiusMin() : -TransformOverlayInteractPrivate::circleRadiusMin();
        }
        if ( std::fabs(targetRadius->x) > TransformOverlayInteractPrivate::circleRadiusMax() ) {
            targetRadius->y *= std::fabs(TransformOverlayInteractPrivate::circleRadiusMax() / targetRadius->x);
            targetRadius->x = targetRadius->x > 0 ? TransformOverlayInteractPrivate::circleRadiusMax() : -TransformOverlayInteractPrivate::circleRadiusMax();
        }
        if ( std::fabs(targetRadius->y) < TransformOverlayInteractPrivate::circleRadiusMin() ) {
            if ( (targetRadius->y == 0.) && (targetRadius->x != 0.) ) {
                targetRadius->x = targetRadius->x > 0 ? TransformOverlayInteractPrivate::circleRadiusMax() : -TransformOverlayInteractPrivate::circleRadiusMax();
            } else {
                targetRadius->x *= std::fabs(TransformOverlayInteractPrivate::circleRadiusMin() / targetRadius->y);
            }
            targetRadius->y = targetRadius->y >= 0 ? TransformOverlayInteractPrivate::circleRadiusMin() : -TransformOverlayInteractPrivate::circleRadiusMin();
        }
        if ( std::fabs(targetRadius->y) > TransformOverlayInteractPrivate::circleRadiusMax() ) {
            targetRadius->x *= std::fabs(TransformOverlayInteractPrivate::circleRadiusMax() / targetRadius->x);
            targetRadius->y = targetRadius->y > 0 ? TransformOverlayInteractPrivate::circleRadiusMax() : -TransformOverlayInteractPrivate::circleRadiusMax();
        }
    }
    // the circle axes are not aligned with the images axes, so we cannot use the x and y scales separately
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    targetRadius->x *= meanPixelScale;
    targetRadius->y *= meanPixelScale;
}

static void
getTargetPoints(const OfxPointD& targetCenter,
                const OfxPointD& targetRadius,
                OfxPointD *left,
                OfxPointD *bottom,
                OfxPointD *top,
                OfxPointD *right)
{
    left->x = targetCenter.x - targetRadius.x;
    left->y = targetCenter.y;
    right->x = targetCenter.x + targetRadius.x;
    right->y = targetCenter.y;
    top->x = targetCenter.x;
    top->y = targetCenter.y + targetRadius.y;
    bottom->x = targetCenter.x;
    bottom->y = targetCenter.y - targetRadius.y;
}

static void
ofxsTransformGetScale(const OfxPointD &scaleParam,
                      bool scaleUniform,
                      OfxPointD* scale)
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
            GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
        } else {
            GL_GPU::Color3f(1.f * l, 0.f * l, 0.f * l);
        }
    } else {
        GL_GPU::Color3f( (float)color.r * l, (float)color.g * l, (float)color.b * l );
    }
    double halfWidth = (TransformOverlayInteractPrivate::pointSize() / 2.) * meanPixelScale;
    double halfHeight = (TransformOverlayInteractPrivate::pointSize() / 2.) * meanPixelScale;
    GL_GPU::PushMatrix();
    GL_GPU::Translated(center.x, center.y, 0.);
    GL_GPU::Begin(GL_POLYGON);
    GL_GPU::Vertex2d(-halfWidth, -halfHeight);   // bottom left
    GL_GPU::Vertex2d(-halfWidth, +halfHeight);   // top left
    GL_GPU::Vertex2d(+halfWidth, +halfHeight);   // bottom right
    GL_GPU::Vertex2d(+halfWidth, -halfHeight);   // top right
    GL_GPU::End();
    GL_GPU::PopMatrix();
}

static void
drawEllipse(const OfxRGBColourD& color,
            const OfxPointD& center,
            const OfxPointD& targetRadius,
            bool hovered,
            int l)
{
    if (hovered) {
        GL_GPU::Color3f(1.f * l, 0.f * l, 0.f * l);
    } else {
        GL_GPU::Color3f( (float)color.r * l, (float)color.g * l, (float)color.b * l );
    }

    GL_GPU::PushMatrix();
    //  center the oval at x_center, y_center
    GL_GPU::Translatef( (float)center.x, (float)center.y, 0.f );
    //  draw the oval using line segments
    GL_GPU::Begin(GL_LINE_LOOP);
    // we don't need to be pixel-perfect here, it's just an interact!
    // 40 segments is enough.
    for (int i = 0; i < 40; ++i) {
        double theta = i * 2 * M_PI / 40.;
        GL_GPU::Vertex2d( targetRadius.x * std::cos(theta), targetRadius.y * std::sin(theta) );
    }
    GL_GPU::End();

    GL_GPU::PopMatrix();
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
        GL_GPU::Color3f(1.f * l, 0.f * l, 0.f * l);
    } else {
        GL_GPU::Color3f( (float)color.r * l, (float)color.g * l, (float)color.b * l );
    }

    // we are not axis-aligned: use the mean pixel scale
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barHalfSize = targetRadiusY + 20. * meanPixelScale;

    GL_GPU::PushMatrix();
    GL_GPU::Translatef( (float)center.x, (float)center.y, 0.f );
    GL_GPU::Rotated(angle, 0, 0, 1);

    GL_GPU::Begin(GL_LINES);
    GL_GPU::Vertex2d(0., -barHalfSize);
    GL_GPU::Vertex2d(0., +barHalfSize);

    if (hovered) {
        double arrowYPosition = targetRadiusY + 10. * meanPixelScale;
        double arrowXHalfSize = 10 * meanPixelScale;
        double arrowHeadOffsetX = 3 * meanPixelScale;
        double arrowHeadOffsetY = 3 * meanPixelScale;

        ///draw the central bar
        GL_GPU::Vertex2d(-arrowXHalfSize, -arrowYPosition);
        GL_GPU::Vertex2d(+arrowXHalfSize, -arrowYPosition);

        ///left triangle
        GL_GPU::Vertex2d(-arrowXHalfSize, -arrowYPosition);
        GL_GPU::Vertex2d(-arrowXHalfSize + arrowHeadOffsetX, -arrowYPosition + arrowHeadOffsetY);

        GL_GPU::Vertex2d(-arrowXHalfSize, -arrowYPosition);
        GL_GPU::Vertex2d(-arrowXHalfSize + arrowHeadOffsetX, -arrowYPosition - arrowHeadOffsetY);

        ///right triangle
        GL_GPU::Vertex2d(+arrowXHalfSize, -arrowYPosition);
        GL_GPU::Vertex2d(+arrowXHalfSize - arrowHeadOffsetX, -arrowYPosition + arrowHeadOffsetY);

        GL_GPU::Vertex2d(+arrowXHalfSize, -arrowYPosition);
        GL_GPU::Vertex2d(+arrowXHalfSize - arrowHeadOffsetX, -arrowYPosition - arrowHeadOffsetY);
    }
    GL_GPU::End();
    GL_GPU::PopMatrix();
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
        GL_GPU::Color3f(1.f * l, 0.f * l, 0.f * l);
    } else {
        GL_GPU::Color3f(color.r * l, color.g * l, color.b * l);
    }

    double barExtra = 30. * meanPixelScale;
    GL_GPU::Begin(GL_LINES);
    GL_GPU::Vertex2d(0., 0.);
    GL_GPU::Vertex2d(0. + targetRadiusX + barExtra, 0.);
    GL_GPU::End();

    if (hovered) {
        double arrowCenterX = targetRadiusX + barExtra / 2.;

        ///draw an arrow slightly bended. This is an arc of circle of radius 5 in X, and 10 in Y.
        OfxPointD arrowRadius;
        arrowRadius.x = 5. * meanPixelScale;
        arrowRadius.y = 10. * meanPixelScale;

        GL_GPU::PushMatrix();
        //  center the oval at x_center, y_center
        GL_GPU::Translatef( (float)arrowCenterX, 0.f, 0 );
        //  draw the oval using line segments
        GL_GPU::Begin(GL_LINE_STRIP);
        GL_GPU::Vertex2d(0, arrowRadius.y);
        GL_GPU::Vertex2d(arrowRadius.x, 0.);
        GL_GPU::Vertex2d(0, -arrowRadius.y);
        GL_GPU::End();


        GL_GPU::Begin(GL_LINES);
        ///draw the top head
        GL_GPU::Vertex2d(0., arrowRadius.y);
        GL_GPU::Vertex2d(0., arrowRadius.y - 5. * meanPixelScale);

        GL_GPU::Vertex2d(0., arrowRadius.y);
        GL_GPU::Vertex2d(4. * meanPixelScale, arrowRadius.y - 3. * meanPixelScale); // 5^2 = 3^2+4^2

        ///draw the bottom head
        GL_GPU::Vertex2d(0., -arrowRadius.y);
        GL_GPU::Vertex2d(0., -arrowRadius.y + 5. * meanPixelScale);

        GL_GPU::Vertex2d(0., -arrowRadius.y);
        GL_GPU::Vertex2d(4. * meanPixelScale, -arrowRadius.y + 3. * meanPixelScale); // 5^2 = 3^2+4^2

        GL_GPU::End();

        GL_GPU::PopMatrix();
    }
    if (inverted) {
        double arrowXPosition = targetRadiusX + barExtra * 1.5;
        double arrowXHalfSize = 10 * meanPixelScale;
        double arrowHeadOffsetX = 3 * meanPixelScale;
        double arrowHeadOffsetY = 3 * meanPixelScale;

        GL_GPU::PushMatrix();
        GL_GPU::Translatef( (float)arrowXPosition, 0, 0 );

        GL_GPU::Begin(GL_LINES);
        ///draw the central bar
        GL_GPU::Vertex2d(-arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(+arrowXHalfSize, 0.);

        ///left triangle
        GL_GPU::Vertex2d(-arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(-arrowXHalfSize + arrowHeadOffsetX, arrowHeadOffsetY);

        GL_GPU::Vertex2d(-arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(-arrowXHalfSize + arrowHeadOffsetX, -arrowHeadOffsetY);

        ///right triangle
        GL_GPU::Vertex2d(+arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(+arrowXHalfSize - arrowHeadOffsetX, arrowHeadOffsetY);

        GL_GPU::Vertex2d(+arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(+arrowXHalfSize - arrowHeadOffsetX, -arrowHeadOffsetY);
        GL_GPU::End();

        GL_GPU::Rotated(90., 0., 0., 1.);

        GL_GPU::Begin(GL_LINES);
        ///draw the central bar
        GL_GPU::Vertex2d(-arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(+arrowXHalfSize, 0.);

        ///left triangle
        GL_GPU::Vertex2d(-arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(-arrowXHalfSize + arrowHeadOffsetX, arrowHeadOffsetY);

        GL_GPU::Vertex2d(-arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(-arrowXHalfSize + arrowHeadOffsetX, -arrowHeadOffsetY);

        ///right triangle
        GL_GPU::Vertex2d(+arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(+arrowXHalfSize - arrowHeadOffsetX, arrowHeadOffsetY);
        
        GL_GPU::Vertex2d(+arrowXHalfSize, 0.);
        GL_GPU::Vertex2d(+arrowXHalfSize - arrowHeadOffsetX, -arrowHeadOffsetY);
        GL_GPU::End();
        
        GL_GPU::PopMatrix();
    }
} // drawRotationBar

static bool
squareContains(const Transform::Point3D& pos,
               const OfxRectD& rect,
               double toleranceX = 0.,
               double toleranceY = 0.)
{
    return ( pos.x >= (rect.x1 - toleranceX) && pos.x < (rect.x2 + toleranceX)
            && pos.y >= (rect.y1 - toleranceY) && pos.y < (rect.y2 + toleranceY) );
}

static bool
isOnEllipseBorder(const Transform::Point3D& pos,
                  const OfxPointD& targetRadius,
                  const OfxPointD& targetCenter,
                  double epsilon = 0.1)
{
    double v = ( (pos.x - targetCenter.x) * (pos.x - targetCenter.x) / (targetRadius.x * targetRadius.x) +
                (pos.y - targetCenter.y) * (pos.y - targetCenter.y) / (targetRadius.y * targetRadius.y) );

    if ( ( v <= (1. + epsilon) ) && ( v >= (1. - epsilon) ) ) {
        return true;
    }

    return false;
}

static bool
isOnSkewXBar(const Transform::Point3D& pos,
             double targetRadiusY,
             const OfxPointD& center,
             const OfxPointD& pixelScale,
             double tolerance)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barHalfSize = targetRadiusY + (20. * meanPixelScale);

    if ( ( pos.x >= (center.x - tolerance) ) && ( pos.x <= (center.x + tolerance) ) &&
        ( pos.y >= (center.y - barHalfSize - tolerance) ) && ( pos.y <= (center.y + barHalfSize + tolerance) ) ) {
        return true;
    }

    return false;
}

static bool
isOnSkewYBar(const Transform::Point3D& pos,
             double targetRadiusX,
             const OfxPointD& center,
             const OfxPointD& pixelScale,
             double tolerance)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barHalfSize = targetRadiusX + (20. * meanPixelScale);

    if ( ( pos.y >= (center.y - tolerance) ) && ( pos.y <= (center.y + tolerance) ) &&
        ( pos.x >= (center.x - barHalfSize - tolerance) ) && ( pos.x <= (center.x + barHalfSize + tolerance) ) ) {
        return true;
    }

    return false;
}

static bool
isOnRotationBar(const Transform::Point3D& pos,
                double targetRadiusX,
                const OfxPointD& center,
                const OfxPointD& pixelScale,
                double tolerance)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barExtra = 30. * meanPixelScale;

    if ( ( pos.x >= (center.x - tolerance) ) && ( pos.x <= (center.x + targetRadiusX + barExtra + tolerance) ) &&
        ( pos.y >= (center.y  - tolerance) ) && ( pos.y <= (center.y + tolerance) ) ) {
        return true;
    }

    return false;
}

static OfxRectD
rectFromCenterPoint(const OfxPointD& center,
                    const OfxPointD& pixelScale)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    OfxRectD ret;

    ret.x1 = center.x - (TransformOverlayInteractPrivate::pointSize() / 2.) * meanPixelScale;
    ret.x2 = center.x + (TransformOverlayInteractPrivate::pointSize() / 2.) * meanPixelScale;
    ret.y1 = center.y - (TransformOverlayInteractPrivate::pointSize() / 2.) * meanPixelScale;
    ret.y2 = center.y + (TransformOverlayInteractPrivate::pointSize() / 2.) * meanPixelScale;
    
    return ret;
}

void
TransformOverlayInteract::drawOverlay(TimeValue time,
                                      const RenderScale & /*renderScale*/,
                                      ViewIdx /*view*/)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    KnobDoublePtr translateKnob = _imp->translate.lock();

    if ( !translateKnob || !translateKnob->shouldDrawOverlayInteract() ) {
        return;
    }

    OfxRGBColourD color;
    if ( !getOverlayColor(color.r, color.g, color.b) ) {
        color.r = color.g = color.b = 0.8;
    }
    
    RenderScale pscale;
    getPixelScale(pscale.x, pscale.y);

    OfxPointD center;
    OfxPointD translate;
    OfxPointD scaleParam;
    bool scaleUniform;
    double rotate;
    double skewX, skewY;
    int skewOrder;
    bool inverted = false;

    if (_imp->mouseState == TransformOverlayInteractPrivate::eReleased) {
        _imp->getCenter(time, &center.x, &center.y);
        _imp->getTranslate(time, &translate.x, &translate.y);
        _imp->getScale(time, &scaleParam.x, &scaleParam.y);
        _imp->getScaleUniform(time, &scaleUniform);
        _imp->getRotate(time, &rotate);
        _imp->getSkewX(time, &skewX);
        _imp->getSkewY(time, &skewY);
        _imp->getSkewOrder(time, &skewOrder);
        inverted = _imp->getInvert(time);
    } else {
        center = _imp->centerDrag;
        translate = _imp->translateDrag;
        scaleParam = _imp->scaleParamDrag;
        scaleUniform = _imp->scaleUniformDrag;
        rotate = _imp->rotateDrag;
        skewX = _imp->skewXDrag;
        skewY = _imp->skewYDrag;
        skewOrder = _imp->skewOrderDrag;
        inverted = _imp->invertedDrag;
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
    skewMatrix[0] = ( skewOrder ? 1. : (1. + skewX * skewY) ); skewMatrix[1] = skewY; skewMatrix[2] = 0.; skewMatrix[3] = 0;
    skewMatrix[4] = skewX; skewMatrix[5] = (skewOrder ? (1. + skewX * skewY) : 1.); skewMatrix[6] = 0.; skewMatrix[7] = 0;
    skewMatrix[8] = 0.; skewMatrix[9] = 0.; skewMatrix[10] = 1.; skewMatrix[11] = 0;
    skewMatrix[12] = 0.; skewMatrix[13] = 0.; skewMatrix[14] = 0.; skewMatrix[15] = 1.;

    //glPushAttrib(GL_ALL_ATTRIB_BITS); // caller is responsible for protecting attribs

    GL_GPU::Disable(GL_LINE_STIPPLE);
    GL_GPU::Enable(GL_LINE_SMOOTH);
    GL_GPU::Disable(GL_POINT_SMOOTH);
    GL_GPU::Enable(GL_BLEND);
    GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    GL_GPU::LineWidth(1.5f);
    GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double w, h;
    getViewportSize(w, h);

    GLdouble projection[16];
    GL_GPU::GetDoublev( GL_PROJECTION_MATRIX, projection);
    OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
    shadow.x = 2. / (projection[0] * w);
    shadow.y = 2. / (projection[5] * h);
    


    // Draw everything twice
    // l = 0: shadow
    // l = 1: drawing
    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        GL_GPU::MatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        GL_GPU::Translated(direction * shadow.x, -direction * shadow.y, 0);
        GL_GPU::MatrixMode(GL_MODELVIEW); // Modelview should be used on Nuke

        GL_GPU::Color3f(color.r * l, color.g * l, color.b * l);

        GL_GPU::PushMatrix();
        GL_GPU::Translated(targetCenter.x, targetCenter.y, 0.);

        GL_GPU::Rotated(rotate, 0, 0., 1.);
        drawRotationBar(color, pscale, targetRadius.x, _imp->mouseState == TransformOverlayInteractPrivate::eDraggingRotationBar || _imp->drawState == TransformOverlayInteractPrivate::eRotationBarHovered, inverted, l);
        GL_GPU::MultMatrixd(skewMatrix);
        GL_GPU::Translated(-targetCenter.x, -targetCenter.y, 0.);

        drawEllipse(color, targetCenter, targetRadius, _imp->mouseState == TransformOverlayInteractPrivate::eDraggingCircle || _imp->drawState == TransformOverlayInteractPrivate::eCircleHovered, l);

        // add 180 to the angle to draw the arrows on the other side. unfortunately, this requires knowing
        // the mouse position in the ellipse frame
        double flip = 0.;
        if ( (_imp->drawState == TransformOverlayInteractPrivate::eSkewXBarHoverered) || (_imp->drawState == TransformOverlayInteractPrivate::eSkewYBarHoverered) ) {
            double rot = Transform::toRadians(rotate);
            Transform::Matrix3x3 transformscale;
            transformscale = Transform::matInverseTransformCanonical(0., 0., scale.x, scale.y, skewX, skewY, (bool)skewOrder, rot, targetCenter.x, targetCenter.y);

            Transform::Point3D previousPos;
            previousPos.x = _imp->lastPenPos.x();
            previousPos.y = _imp->lastPenPos.y();
            previousPos.z = 1.;
            previousPos = Transform::matApply(transformscale, previousPos);
            if (previousPos.z != 0) {
                previousPos.x /= previousPos.z;
                previousPos.y /= previousPos.z;
            }
            if ( ( (_imp->drawState == TransformOverlayInteractPrivate::eSkewXBarHoverered) && (previousPos.y > targetCenter.y) ) ||
                ( ( _imp->drawState == TransformOverlayInteractPrivate::eSkewYBarHoverered) && ( previousPos.x > targetCenter.x) ) ) {
                flip = 180.;
            }
        }
        drawSkewBar(color, targetCenter, pscale, targetRadius.y, _imp->mouseState == TransformOverlayInteractPrivate::eDraggingSkewXBar || _imp->drawState == TransformOverlayInteractPrivate::eSkewXBarHoverered, flip, l);
        drawSkewBar(color, targetCenter, pscale, targetRadius.x, _imp->mouseState == TransformOverlayInteractPrivate::eDraggingSkewYBar || _imp->drawState == TransformOverlayInteractPrivate::eSkewYBarHoverered, flip - 90., l);


        drawSquare(color, targetCenter, pscale, _imp->mouseState == TransformOverlayInteractPrivate::eDraggingTranslation || _imp->mouseState == TransformOverlayInteractPrivate::eDraggingCenter || _imp->drawState == TransformOverlayInteractPrivate::eCenterPointHovered, _imp->modifierStateCtrl, l);
        drawSquare(color, left, pscale, _imp->mouseState == TransformOverlayInteractPrivate::eDraggingLeftPoint || _imp->drawState == TransformOverlayInteractPrivate::eLeftPointHovered, false, l);
        drawSquare(color, right, pscale, _imp->mouseState == TransformOverlayInteractPrivate::eDraggingRightPoint || _imp->drawState == TransformOverlayInteractPrivate::eRightPointHovered, false, l);
        drawSquare(color, top, pscale, _imp->mouseState == TransformOverlayInteractPrivate::eDraggingTopPoint || _imp->drawState == TransformOverlayInteractPrivate::eTopPointHovered, false, l);
        drawSquare(color, bottom, pscale, _imp->mouseState == TransformOverlayInteractPrivate::eDraggingBottomPoint || _imp->drawState == TransformOverlayInteractPrivate::eBottomPointHovered, false, l);
        
        GL_GPU::PopMatrix();
    }
    //glPopAttrib();

} // drawOverlay



// round to the closest int, 1/10 int, etc
// this make parameter editing easier
// pscale is args.pixelScale.x / args.renderScale.x;
// pscale10 is the power of 10 below pscale
inline double
fround(double val,
       double pscale)
{
    double pscale10 = ipow( 10, (int)std::floor( std::log10(pscale) ) );

    return pscale10 * std::floor(val / pscale10 + 0.5);
}


bool
TransformOverlayInteract::onOverlayPenDown(TimeValue time,
                                    const RenderScale & /*renderScale*/,
                                    ViewIdx /*view*/,
                                    const QPointF & /*viewportPos*/,
                                    const QPointF & penPosParam,
                                    double /*pressure*/,
                                    TimeValue /*timestamp*/,
                                    PenType /*pen*/)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    KnobDoublePtr translateKnob = _imp->translate.lock();

    if ( !translateKnob || !translateKnob->shouldDrawOverlayInteract() ) {
        return false;
    }

    RenderScale pscale;
    getPixelScale(pscale.x, pscale.y);

    OfxPointD center;
    OfxPointD translate;
    OfxPointD scaleParam;
    bool scaleUniform;
    double rotate;
    double skewX, skewY;
    int skewOrder;
    bool inverted = false;

    if (_imp->mouseState == TransformOverlayInteractPrivate::eReleased) {
        _imp->getCenter(time, &center.x, &center.y);
        _imp->getTranslate(time, &translate.x, &translate.y);
        _imp->getScale(time, &scaleParam.x, &scaleParam.y);
        _imp->getScaleUniform(time, &scaleUniform);
        _imp->getRotate(time, &rotate);
        _imp->getSkewX(time, &skewX);
        _imp->getSkewY(time, &skewY);
        _imp->getSkewOrder(time, &skewOrder);
        inverted = _imp->getInvert(time);
    } else {
        center = _imp->centerDrag;
        translate = _imp->translateDrag;
        scaleParam = _imp->scaleParamDrag;
        scaleUniform = _imp->scaleUniformDrag;
        rotate = _imp->rotateDrag;
        skewX = _imp->skewXDrag;
        skewY = _imp->skewYDrag;
        skewOrder = _imp->skewOrderDrag;
        inverted = _imp->invertedDrag;
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

    _imp->orientation = TransformOverlayInteractPrivate::eOrientationAllDirections;

    double pressToleranceX = 5 * pscale.x;
    double pressToleranceY = 5 * pscale.y;
    bool didSomething = false;
    if ( squareContains(transformedPos, centerPoint, pressToleranceX, pressToleranceY) ) {
        _imp->mouseState = _imp->modifierStateCtrl ? TransformOverlayInteractPrivate::eDraggingCenter : TransformOverlayInteractPrivate::eDraggingTranslation;
        if (_imp->modifierStateShift > 0) {
            _imp->orientation = TransformOverlayInteractPrivate::eOrientationNotSet;
        }
        didSomething = true;
    } else if ( squareContains(transformedPos, leftPoint, pressToleranceX, pressToleranceY) ) {
        _imp->mouseState = TransformOverlayInteractPrivate::eDraggingLeftPoint;
        didSomething = true;
    } else if ( squareContains(transformedPos, rightPoint, pressToleranceX, pressToleranceY) ) {
        _imp->mouseState = TransformOverlayInteractPrivate::eDraggingRightPoint;
        didSomething = true;
    } else if ( squareContains(transformedPos, topPoint, pressToleranceX, pressToleranceY) ) {
        _imp->mouseState = TransformOverlayInteractPrivate::eDraggingTopPoint;
        didSomething = true;
    } else if ( squareContains(transformedPos, bottomPoint, pressToleranceX, pressToleranceY) ) {
        _imp->mouseState = TransformOverlayInteractPrivate::eDraggingBottomPoint;
        didSomething = true;
    } else if ( isOnEllipseBorder(transformedPos, targetRadius, targetCenter) ) {
        _imp->mouseState = TransformOverlayInteractPrivate::eDraggingCircle;
        didSomething = true;
    } else if ( isOnRotationBar(rotationPos, targetRadius.x, targetCenter, pscale, pressToleranceY) ) {
        _imp->mouseState = TransformOverlayInteractPrivate::eDraggingRotationBar;
        didSomething = true;
    } else if ( isOnSkewXBar(transformedPos, targetRadius.y, targetCenter, pscale, pressToleranceY) ) {
        _imp->mouseState = TransformOverlayInteractPrivate::eDraggingSkewXBar;
        didSomething = true;
    } else if ( isOnSkewYBar(transformedPos, targetRadius.x, targetCenter, pscale, pressToleranceX) ) {
        _imp->mouseState = TransformOverlayInteractPrivate::eDraggingSkewYBar;
        didSomething = true;
    } else {
        _imp->mouseState = TransformOverlayInteractPrivate::eReleased;
    }

    _imp->centerDrag = center;
    _imp->translateDrag = translate;
    _imp->scaleParamDrag = scaleParam;
    _imp->scaleUniformDrag = scaleUniform;
    _imp->rotateDrag = rotate;
    _imp->skewXDrag = skewX;
    _imp->skewYDrag = skewY;
    _imp->skewOrderDrag = skewOrder;
    _imp->invertedDrag = inverted;
    _imp->interactiveDrag = _imp->getInteractive();

    _imp->lastPenPos = penPosParam;

    if (didSomething) {
        redraw();
    }
    
    return didSomething;
} // onOverlayPenDown



bool
TransformOverlayInteract::onOverlayPenMotion(TimeValue time,
                                             const RenderScale & /*renderScale*/,
                                             ViewIdx view,
                                             const QPointF & /*viewportPos*/,
                                             const QPointF & penPosParam,
                                             double /*pressure*/,
                                             TimeValue /*timestamp*/)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    KnobDoublePtr translateKnob = _imp->translate.lock();

    if ( !translateKnob || !translateKnob->shouldDrawOverlayInteract() ) {
        return false;
    }

    RenderScale pscale;
    getPixelScale(pscale.x, pscale.y);

    OfxPointD center;
    OfxPointD translate;
    OfxPointD scaleParam;
    bool scaleUniform;
    double rotate;
    double skewX, skewY;
    int skewOrder;
    bool inverted = false;

    if (_imp->mouseState == TransformOverlayInteractPrivate::eReleased) {
        _imp->getCenter(time, &center.x, &center.y);
        _imp->getTranslate(time, &translate.x, &translate.y);
        _imp->getScale(time, &scaleParam.x, &scaleParam.y);
        _imp->getScaleUniform(time, &scaleUniform);
        _imp->getRotate(time, &rotate);
        _imp->getSkewX(time, &skewX);
        _imp->getSkewY(time, &skewY);
        _imp->getSkewOrder(time, &skewOrder);
        inverted = _imp->getInvert(time);
    } else {
        center = _imp->centerDrag;
        translate = _imp->translateDrag;
        scaleParam = _imp->scaleParamDrag;
        scaleUniform = _imp->scaleUniformDrag;
        rotate = _imp->rotateDrag;
        skewX = _imp->skewXDrag;
        skewY = _imp->skewYDrag;
        skewOrder = _imp->skewOrderDrag;
        inverted = _imp->invertedDrag;
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
    prevPenPos.x = _imp->lastPenPos.x();
    prevPenPos.y = _imp->lastPenPos.y();
    prevPenPos.z = 1.;

    Transform::Matrix3x3 rotation, transform, transformscale;
    ////for the rotation bar/translation/center dragging we dont use the same transform, we don't want to undo the rotation transform
    if ( (_imp->mouseState != TransformOverlayInteractPrivate::eDraggingTranslation) && (_imp->mouseState != TransformOverlayInteractPrivate::eDraggingCenter) ) {
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

    previousPos = Transform::matApply(transformscale, prevPenPos);
    if (previousPos.z != 0) {
        previousPos.x /= previousPos.z;
        previousPos.y /= previousPos.z;
    }

    currentPos = Transform::matApply(transformscale, penPos);
    if (currentPos.z != 0) {
        currentPos.x /= currentPos.z;
        currentPos.y /= currentPos.z;
    }

    double minX, minY, maxX, maxY;
    KnobDoublePtr scaleKnob = _imp->scale.lock();
    assert(scaleKnob);

    minX = scaleKnob->getMinimum(DimIdx(0));
    minY = scaleKnob->getMinimum(DimIdx(1));
    maxX = scaleKnob->getMaximum(DimIdx(0));
    maxY = scaleKnob->getMaximum(DimIdx(1));

    if (_imp->mouseState == TransformOverlayInteractPrivate::eReleased) {
        // we are not axis-aligned
        double meanPixelScale = (pscale.x + pscale.y) / 2.;
        double hoverTolerance = (TransformOverlayInteractPrivate::pointSize() / 2.) * meanPixelScale;
        if ( squareContains(transformedPos, centerPoint) ) {
            _imp->drawState = TransformOverlayInteractPrivate::eCenterPointHovered;
            didSomething = true;
        } else if ( squareContains(transformedPos, leftPoint) ) {
            _imp->drawState = TransformOverlayInteractPrivate::eLeftPointHovered;
            didSomething = true;
        } else if ( squareContains(transformedPos, rightPoint) ) {
            _imp->drawState = TransformOverlayInteractPrivate::eRightPointHovered;
            didSomething = true;
        } else if ( squareContains(transformedPos, topPoint) ) {
            _imp->drawState = TransformOverlayInteractPrivate::eTopPointHovered;
            didSomething = true;
        } else if ( squareContains(transformedPos, bottomPoint) ) {
            _imp->drawState = TransformOverlayInteractPrivate::eBottomPointHovered;
            didSomething = true;
        } else if ( isOnEllipseBorder(transformedPos, targetRadius, targetCenter) ) {
            _imp->drawState = TransformOverlayInteractPrivate::eCircleHovered;
            didSomething = true;
        } else if ( isOnRotationBar(rotationPos, targetRadius.x, targetCenter, pscale, hoverTolerance) ) {
            _imp->drawState = TransformOverlayInteractPrivate::eRotationBarHovered;
            didSomething = true;
        } else if ( isOnSkewXBar(transformedPos, targetRadius.y, targetCenter, pscale, hoverTolerance) ) {
            _imp->drawState = TransformOverlayInteractPrivate::eSkewXBarHoverered;
            didSomething = true;
        } else if ( isOnSkewYBar(transformedPos, targetRadius.x, targetCenter, pscale, hoverTolerance) ) {
            _imp->drawState = TransformOverlayInteractPrivate::eSkewYBarHoverered;
            didSomething = true;
        } else {
            _imp->drawState = TransformOverlayInteractPrivate::eInActive;
        }
    } else if (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingCircle) {
        // we need to compute the backtransformed points with the scale

        // the scale ratio is the ratio of distances to the center
        double prevDistSq = (targetCenter.x - previousPos.x) * (targetCenter.x - previousPos.x) + (targetCenter.y - previousPos.y) * (targetCenter.y - previousPos.y);
        if (prevDistSq != 0.) {
            const double distSq = (targetCenter.x - currentPos.x) * (targetCenter.x - currentPos.x) + (targetCenter.y - currentPos.y) * (targetCenter.y - currentPos.y);
            const double distRatio = std::sqrt(distSq / prevDistSq);
            scale.x *= distRatio;
            scale.y *= distRatio;
            //_scale->setValue(scale.x, scale.y);
            scaleChanged = true;
        }
    } else if ( (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingLeftPoint) || (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingRightPoint) ) {
        // avoid division by zero
        if (targetCenter.x != previousPos.x) {
            const double scaleRatio = (targetCenter.x - currentPos.x) / (targetCenter.x - previousPos.x);
            OfxPointD newScale;
            newScale.x = scale.x * scaleRatio;
            newScale.x = std::max( minX, std::min(newScale.x, maxX) );
            newScale.y = scaleUniform ? newScale.x : scale.y;
            scale = newScale;
            //_scale->setValue(scale.x, scale.y);
            scaleChanged = true;
        }
    } else if ( (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingTopPoint) || (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingBottomPoint) ) {
        // avoid division by zero
        if (targetCenter.y != previousPos.y) {
            const double scaleRatio = (targetCenter.y - currentPos.y) / (targetCenter.y - previousPos.y);
            OfxPointD newScale;
            newScale.y = scale.y * scaleRatio;
            newScale.y = std::max( minY, std::min(newScale.y, maxY) );
            newScale.x = scaleUniform ? newScale.y : scale.x;
            scale = newScale;
            //_scale->setValue(scale.x, scale.y);
            scaleChanged = true;
        }
    } else if (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingTranslation) {
        double dx = penPosParam.x() - _imp->lastPenPos.x();
        double dy = penPosParam.y() - _imp->lastPenPos.y();

        if ( (_imp->orientation == TransformOverlayInteractPrivate::eOrientationNotSet) && (_imp->modifierStateShift > 0) ) {
            _imp->orientation = std::abs(dx) > std::abs(dy) ? TransformOverlayInteractPrivate::eOrientationHorizontal : TransformOverlayInteractPrivate::eOrientationVertical;
        }

        dx = _imp->orientation == TransformOverlayInteractPrivate::eOrientationVertical ? 0 : dx;
        dy = _imp->orientation == TransformOverlayInteractPrivate::eOrientationHorizontal ? 0 : dy;
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
    } else if (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingCenter) {
        OfxPointD currentCenter = center;
        Transform::Matrix3x3 R = Transform::matMul( Transform::matMul( Transform::matScale(1. / scale.x, 1. / scale.y), Transform::matSkewXY(-skewX, -skewY, !skewOrder) ), Transform::matRotation(rot) );
        double dx = penPosParam.x() - _imp->lastPenPos.x();
        double dy = penPosParam.y() - _imp->lastPenPos.y();

        if ( (_imp->orientation == TransformOverlayInteractPrivate::eOrientationNotSet) && (_imp->modifierStateShift > 0) ) {
            _imp->orientation = std::abs(dx) > std::abs(dy) ? TransformOverlayInteractPrivate::eOrientationHorizontal : TransformOverlayInteractPrivate::eOrientationVertical;
        }

        dx = _imp->orientation == TransformOverlayInteractPrivate::eOrientationVertical ? 0 : dx;
        dy = _imp->orientation == TransformOverlayInteractPrivate::eOrientationHorizontal ? 0 : dy;

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
        double det = R.determinant();
        if (det != 0.) {
            Transform::Matrix3x3 Rinv;
            if (!R.inverse(&Rinv)) {
                Rinv.setIdentity();
            }

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
    } else if (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingRotationBar) {
        OfxPointD diffToCenter;
        ///the current mouse position (untransformed) is doing has a certain angle relative to the X axis
        ///which can be computed by : angle = arctan(opposite / adjacent)
        diffToCenter.y = rotationPos.y - targetCenter.y;
        diffToCenter.x = rotationPos.x - targetCenter.x;
        double angle = std::atan2(diffToCenter.y, diffToCenter.x);
        double angledegrees = rotate + Transform::toDegrees(angle);
        double closest90 = 90. * std::floor( (angledegrees + 45.) / 90. );
        if (std::fabs(angledegrees - closest90) < 5.) {
            // snap to closest multiple of 90.
            angledegrees = closest90;
        }
        rotate = angledegrees;
        //_rotate->setValue(rotate);
        rotateChanged = true;
    } else if (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingSkewXBar) {
        // avoid division by zero
        if ( (scale.y != 0.) && (targetCenter.y != previousPos.y) ) {
            const double addSkew = (scale.x / scale.y) * (currentPos.x - previousPos.x) / (currentPos.y - targetCenter.y);
            skewX = skewX + addSkew;
            //_skewX->setValue(skewX);
            skewXChanged = true;
        }
    } else if (_imp->mouseState == TransformOverlayInteractPrivate::eDraggingSkewYBar) {
        // avoid division by zero
        if ( (scale.x != 0.) && (targetCenter.x != previousPos.x) ) {
            const double addSkew = (scale.y / scale.x) * (currentPos.y - previousPos.y) / (currentPos.x - targetCenter.x);
            skewY = skewY + addSkew;
            //_skewY->setValue(skewY + addSkew);
            skewYChanged = true;
        }
    } else {
        assert(false);
    }

    _imp->centerDrag = center;
    _imp->translateDrag = translate;
    _imp->scaleParamDrag = scale;
    _imp->scaleUniformDrag = scaleUniform;
    _imp->rotateDrag = rotate;
    _imp->skewXDrag = skewX;
    _imp->skewYDrag = skewY;
    _imp->skewOrderDrag = skewOrder;
    _imp->invertedDrag = inverted;

    _imp->lastPenPos = penPosParam;

    bool valuesChanged = (centerChanged || translateChanged || scaleChanged || rotateChanged || skewXChanged || skewYChanged);

    if ( (_imp->mouseState != TransformOverlayInteractPrivate::eReleased) && _imp->interactiveDrag && valuesChanged ) {
        // no need to redraw overlay since it is slave to the paramaters
        EffectInstancePtr holder = getEffect();
        holder->beginMultipleEdits(tr("Modify transform handle").toStdString());
        if (centerChanged) {
            KnobDoublePtr knob = _imp->center.lock();
            std::vector<double> val(2);
            val[0] = center.x;
            val[1] = center.y;
            knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
        }
        if (translateChanged) {
            KnobDoublePtr knob = _imp->translate.lock();
            std::vector<double> val(2);
            val[0] = translate.x;
            val[1] = translate.y;
            knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
        }
        if (scaleChanged) {
            KnobDoublePtr knob = _imp->scale.lock();
            std::vector<double> val(2);
            val[0] = scale.x;
            val[1] = scale.y;
            knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
        }
        if (rotateChanged) {
            KnobDoublePtr knob = _imp->rotate.lock();
            knob->setValue(rotate, view, DimIdx(0), eValueChangedReasonUserEdited);
        }
        if (skewXChanged) {
            KnobDoublePtr knob = _imp->skewX.lock();
            knob->setValue(skewX, view, DimIdx(0), eValueChangedReasonUserEdited);
        }
        if (skewYChanged) {
            KnobDoublePtr knob = _imp->skewY.lock();
            knob->setValue(skewY, view, DimIdx(0), eValueChangedReasonUserEdited);
        }
        holder->endMultipleEdits();
    } else if (didSomething || valuesChanged) {
        redraw();
    }
    
    return didSomething || valuesChanged;
} // onOverlayPenMotion



bool
TransformOverlayInteract::onOverlayPenUp(TimeValue /*time*/,
                                         const RenderScale & /*renderScale*/,
                                         ViewIdx view,
                                         const QPointF & /*viewportPos*/,
                                         const QPointF & /*pos*/,
                                         double /*pressure*/,
                                         TimeValue /*timestamp*/)
{
    bool ret = _imp->mouseState != TransformOverlayInteractPrivate::eReleased;
    
    if ( !_imp->interactiveDrag && (_imp->mouseState != TransformOverlayInteractPrivate::eReleased) ) {
        // no need to redraw overlay since it is slave to the paramaters

        /*
         Give eValueChangedReasonPluginEdited reason so that the command uses the undo/redo stack
         see Knob::setValue
         */
        EffectInstancePtr holder = getEffect();
        holder->beginMultipleEdits(tr("Modify transform handle").toStdString());
        {
            KnobDoublePtr knob = _imp->center.lock();
            std::vector<double> val(2);
            val[0] = _imp->centerDrag.x;
            val[1] = _imp->centerDrag.y;
            knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
        }
        {
            KnobDoublePtr knob = _imp->translate.lock();
            std::vector<double> val(2);
            val[0] = _imp->translateDrag.x;
            val[1] = _imp->translateDrag.y;
            knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
        }
        {
            KnobDoublePtr knob = _imp->scale.lock();
            std::vector<double> val(2);
            val[0] = _imp->scaleParamDrag.x;
            val[1] = _imp->scaleParamDrag.y;
            knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
        }
        {
            KnobDoublePtr knob = _imp->rotate.lock();
            knob->setValue(_imp->rotateDrag, view, DimIdx(0), eValueChangedReasonUserEdited);
        }
        {
            KnobDoublePtr knob = _imp->skewX.lock();
            knob->setValue(_imp->skewXDrag, view, DimIdx(0), eValueChangedReasonUserEdited);
        }
        {
            KnobDoublePtr knob = _imp->skewY.lock();
            knob->setValue(_imp->skewYDrag, view, DimIdx(0), eValueChangedReasonUserEdited);
        }

        holder->endMultipleEdits();
    } else if (_imp->mouseState != TransformOverlayInteractPrivate::eReleased) {
        redraw();
    }

    _imp->mouseState = TransformOverlayInteractPrivate::eReleased;

    return ret;
} // onOverlayPenUp


bool
TransformOverlayInteract::onOverlayKeyDown(TimeValue /*time*/,
                                         const RenderScale & /*renderScale*/,
                                         ViewIdx /*view*/,
                                         Key key,
                                         KeyboardModifiers /*modifiers*/)
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
    if ( (key == kOfxKey_Control_L) || (key == kOfxKey_Control_R) ) {
        mustRedraw = _imp->modifierStateCtrl == 0;
        ++_imp->modifierStateCtrl;
    }
    if ( (key == kOfxKey_Shift_L) || (key == kOfxKey_Shift_R) ) {
        mustRedraw = _imp->modifierStateShift == 0;
        ++_imp->modifierStateShift;
        if (_imp->modifierStateShift > 0) {
            _imp->orientation = TransformOverlayInteractPrivate::eOrientationNotSet;
        }
    }
    if (mustRedraw) {
        redraw();
    }
    //std::cout << std::hex << args.keySymbol << std::endl;

    // modifiers are not "caught"
    return false;
} // onOverlayKeyDown

bool
TransformOverlayInteract::onOverlayKeyUp(TimeValue /*time*/,
                                         const RenderScale & /*renderScale*/,
                                         ViewIdx /*view*/,
                                         Key key,
                                         KeyboardModifiers /*modifiers*/)
{
    bool mustRedraw = false;

    if ( (key == kOfxKey_Control_L) || (key == kOfxKey_Control_R) ) {
        // we may have missed a keypress
        if (_imp->modifierStateCtrl > 0) {
            --_imp->modifierStateCtrl;
            mustRedraw = _imp->modifierStateCtrl == 0;
        }
    }
    if ( (key == kOfxKey_Shift_L) || (key == kOfxKey_Shift_R) ) {
        if (_imp->modifierStateShift > 0) {
            --_imp->modifierStateShift;
            mustRedraw = _imp->modifierStateShift == 0;
        }
        if (_imp->modifierStateShift == 0) {
            _imp->orientation = TransformOverlayInteractPrivate::eOrientationAllDirections;
        }
    }
    if (mustRedraw) {
        redraw();
    }

    // modifiers are not "caught"
    return false;
} // onOverlayKeyUp



bool
TransformOverlayInteract::onOverlayFocusLost(TimeValue /*time*/,
                                              const RenderScale & /*renderScale*/,
                                              ViewIdx /*view*/)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    KnobDoublePtr translateKnob = _imp->translate.lock();

    if ( !translateKnob || !translateKnob->shouldDrawOverlayInteract() ) {
        return false;
    }
    // reset the modifiers state
    _imp->modifierStateCtrl = 0;
    _imp->modifierStateShift = 0;
    _imp->interactiveDrag = false;
    _imp->mouseState =TransformOverlayInteractPrivate::eReleased;
    _imp->drawState = TransformOverlayInteractPrivate::eInActive;

    return false;
}



NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_TransformOverlayInteract.cpp"
