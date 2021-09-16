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


#include "RotoPaintInteract.h"

#include <QtCore/QLineF>

#include "Global/GLIncludes.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoPaint.h"
#include "Engine/MergingEnum.h"
#include "Engine/Node.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoUndoCommand.h"
#include "Engine/RotoLayer.h"
#include "Engine/Bezier.h"
#include "Engine/Transform.h"
#include "Engine/AppInstance.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER

RotoPaintPrivate::RotoPaintPrivate(RotoPaint* publicInterface,
                                   bool isPaintByDefault)
    : publicInterface(publicInterface)
    , isPaintByDefault(isPaintByDefault)
    , premultKnob()
    , enabledKnobs()
, ui( RotoPaintInteract::create(this) )
{
}

RotoPaintInteract::RotoPaintInteract(RotoPaintPrivate* p)
    : p(p)
    , selectedItems()
    , selectedCps()
    , selectedCpsBbox()
    , showCpsBbox(false)
    , transformMode()
    , builtBezier()
    , bezierBeingDragged()
    , cpBeingDragged()
    , tangentBeingDragged()
    , featherBarBeingDragged()
    , featherBarBeingHovered()
    , strokeBeingPaint()
    , strokeBeingPaintedTimelineFrame(0)
    , cloneOffset()
    , click()
    , selectedTool(eRotoToolSelectAll)
    , selectedRole(eRotoRoleSelection)
    , state(eEventStateNone)
    , hoverState(eHoverStateNothing)
    , lastClickPos()
    , lastMousePos()
    , evaluateOnPenUp(false)
    , evaluateOnKeyUp(false)
    , iSelectingwithCtrlA(false)
    , shiftDown(0)
    , ctrlDown(0)
    , altDown(0)
    , lastTabletDownTriggeredEraser(false)
    , mouseCenterOnSizeChange()
{
    cloneOffset.first = cloneOffset.second = 0.;
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct RotoPaintInteract::MakeSharedEnabler: public RotoPaintInteract
{
    MakeSharedEnabler(RotoPaintPrivate* p) : RotoPaintInteract(p) {
    }
};


RotoPaintInteractPtr
RotoPaintInteract::create(RotoPaintPrivate* p)
{
    return boost::make_shared<RotoPaintInteract::MakeSharedEnabler>(p);
}


void
RotoPaintInteract::evaluate(bool redraw)
{
    if (redraw) {
        p->publicInterface->redrawOverlayInteract();
    }
    p->publicInterface->getNode()->getRotoContext()->evaluateChange();
    p->publicInterface->getApp()->triggerAutoSave();
}

void
RotoPaintInteract::autoSaveAndRedraw()
{
    p->publicInterface->redrawOverlayInteract();
    p->publicInterface->getApp()->triggerAutoSave();
}

void
RotoPaintInteract::redrawOverlays()
{
    p->publicInterface->redrawOverlayInteract();
}

RotoContextPtr
RotoPaintInteract::getContext()
{
    return p->publicInterface->getNode()->getRotoContext();
}

bool
RotoPaintInteract::isFeatherVisible() const
{
    KnobButtonPtr b =  displayFeatherEnabledButton.lock();

    if (b) {
        return b->getValue();
    } else {
        return true;
    }
}

bool
RotoPaintInteract::isStickySelectionEnabled() const
{
    KnobButtonPtr b =  stickySelectionEnabledButton.lock();

    if (b) {
        return b->getValue();
    } else {
        return false;
    }
}

bool
RotoPaintInteract::isMultiStrokeEnabled() const
{
    KnobBoolPtr b =  multiStrokeEnabled.lock();

    if (b) {
        return b->getValue();
    } else {
        return false;
    }
}

bool
RotoPaintInteract::isBboxClickAnywhereEnabled() const
{
    KnobButtonPtr b = bboxClickAnywhereButton.lock();

    return b ? b->getValue() : false;
}

void
RotoPaintInteract::drawSelectedCp(double time,
                                  const BezierCPPtr & cp,
                                  double x,
                                  double y,
                                  const Transform::Matrix3x3& transform)
{
    ///if the tangent is being dragged, color it
    bool colorLeftTangent = false;
    bool colorRightTangent = false;

    if ( (cp == tangentBeingDragged) &&
         ( ( state == eEventStateDraggingLeftTangent) || ( state == eEventStateDraggingRightTangent) ) ) {
        colorLeftTangent = state == eEventStateDraggingLeftTangent ? true : false;
        colorRightTangent = !colorLeftTangent;
    }


    Transform::Point3D leftDeriv, rightDeriv;
    leftDeriv.z = rightDeriv.z = 1.;
    cp->getLeftBezierPointAtTime(true, time, ViewIdx(0), &leftDeriv.x, &leftDeriv.y);
    cp->getRightBezierPointAtTime(true, time, ViewIdx(0), &rightDeriv.x, &rightDeriv.y);
    leftDeriv = Transform::matApply(transform, leftDeriv);
    rightDeriv = Transform::matApply(transform, rightDeriv);

    bool drawLeftHandle = leftDeriv.x != x || leftDeriv.y != y;
    bool drawRightHandle = rightDeriv.y != x || rightDeriv.y != y;
    glBegin(GL_POINTS);
    if (drawLeftHandle) {
        if (colorLeftTangent) {
            glColor3f(0.2, 1., 0.);
        }
        glVertex2d(leftDeriv.x, leftDeriv.y);
        if (colorLeftTangent) {
            glColor3d(0.85, 0.67, 0.);
        }
    }
    if (drawRightHandle) {
        if (colorRightTangent) {
            glColor3f(0.2, 1., 0.);
        }
        glVertex2d(rightDeriv.x, rightDeriv.y);
        if (colorRightTangent) {
            glColor3d(0.85, 0.67, 0.);
        }
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    if (drawLeftHandle) {
        glVertex2d(leftDeriv.x, leftDeriv.y);
    }
    glVertex2d(x, y);
    if (drawRightHandle) {
        glVertex2d(rightDeriv.x, rightDeriv.y);
    }
    glEnd();
} // drawSelectedCp

void
RotoPaintInteract::drawEllipse(double x,
                               double y,
                               double radiusX,
                               double radiusY,
                               int l,
                               double r,
                               double g,
                               double b,
                               double a)
{
    glColor3f(r * l * a, g * l * a, b * l * a);

    glPushMatrix();
    //  center the oval at x_center, y_center
    glTranslatef( (float)x, (float)y, 0.f );
    //  draw the oval using line segments
    glBegin(GL_LINE_LOOP);
    // we don't need to be pixel-perfect here, it's just an interact!
    // 40 segments is enough.
    double m = 2 * 3.14159265358979323846264338327950288419717 / 40.;
    for (int i = 0; i < 40; ++i) {
        double theta = i * m;
        glVertex2d( radiusX * std::cos(theta), radiusY * std::sin(theta) );
    }
    glEnd();

    glPopMatrix();
}

void
RotoPaintInteract::drawArrow(double centerX,
                             double centerY,
                             double rotate,
                             bool hovered,
                             const std::pair<double, double> & pixelScale)
{
    GLProtectMatrix pm(GL_MODELVIEW);

    if (hovered) {
        glColor3f(0., 1., 0.);
    } else {
        glColor3f(1., 1., 1.);
    }

    double arrowLenght =  kTransformArrowLenght * pixelScale.second;
    double arrowWidth = kTransformArrowWidth * pixelScale.second;
    double arrowHeadHeight = 4 * pixelScale.second;

    glTranslatef(centerX, centerY, 0.);
    glRotatef(rotate, 0., 0., 1.);
    QPointF bottom(0., -arrowLenght);
    QPointF top(0, arrowLenght);
    ///the arrow head is 4 pixels long and kTransformArrowWidth * 2 large
    double screenPixelRatio = p->publicInterface->getCurrentViewportForOverlays()->getScreenPixelRatio();
    glLineWidth(1 * screenPixelRatio);
    glBegin(GL_LINES);
    glVertex2f( top.x(), top.y() );
    glVertex2f( bottom.x(), bottom.y() );
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f( bottom.x(), bottom.y() );
    glVertex2f(bottom.x() + arrowWidth, bottom.y() + arrowHeadHeight);
    glVertex2f(bottom.x() - arrowWidth, bottom.y() + arrowHeadHeight);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f( top.x(), top.y() );
    glVertex2f(top.x() - arrowWidth, top.y() - arrowHeadHeight);
    glVertex2f(top.x() + arrowWidth, top.y() - arrowHeadHeight);
    glEnd();
}

void
RotoPaintInteract::drawBendedArrow(double centerX,
                                   double centerY,
                                   double rotate,
                                   bool hovered,
                                   const std::pair<double, double> & pixelScale)
{
    GLProtectMatrix p(GL_MODELVIEW);

    if (hovered) {
        glColor3f(0., 1., 0.);
    } else {
        glColor3f(1., 1., 1.);
    }

    double arrowLenght =  kTransformArrowLenght * pixelScale.second;
    double arrowWidth = kTransformArrowWidth * pixelScale.second;
    double arrowHeadHeight = 4 * pixelScale.second;

    glTranslatef(centerX, centerY, 0.);
    glRotatef(rotate, 0., 0., 1.);

    /// by default we draw the top left
    QPointF bottom(0., -arrowLenght / 2.);
    QPointF right(arrowLenght / 2., 0.);
    glBegin (GL_LINE_STRIP);
    glVertex2f ( bottom.x(), bottom.y() );
    glVertex2f (0., 0.);
    glVertex2f ( right.x(), right.y() );
    glEnd ();

    glBegin(GL_POLYGON);
    glVertex2f(bottom.x(), bottom.y() - arrowHeadHeight);
    glVertex2f( bottom.x() - arrowWidth, bottom.y() );
    glVertex2f( bottom.x() + arrowWidth, bottom.y() );
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f( right.x() + arrowHeadHeight, right.y() );
    glVertex2f(right.x(), right.y() - arrowWidth);
    glVertex2f(right.x(), right.y() + arrowWidth);
    glEnd();
}

void
RotoPaintInteract::drawSelectedCpsBBOX()
{
    std::pair<double, double> pixelScale;

    p->publicInterface->getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);


        QPointF topLeft = selectedCpsBbox.topLeft();
        QPointF btmRight = selectedCpsBbox.bottomRight();

        double screenPixelRatio = p->publicInterface->getCurrentViewportForOverlays()->getScreenPixelRatio();

        if (hoverState == eHoverStateBbox) {
            glColor4f(0.9, 0.5, 0, 1.);
        } else {
            glColor4f(0.8, 0.8, 0.8, 1.);
        }
        glLineWidth(1.5 * screenPixelRatio);
        glBegin(GL_LINE_LOOP);
        glVertex2f( topLeft.x(), btmRight.y() );
        glVertex2f( topLeft.x(), topLeft.y() );
        glVertex2f( btmRight.x(), topLeft.y() );
        glVertex2f( btmRight.x(), btmRight.y() );
        glEnd();

        double midX = ( topLeft.x() + btmRight.x() ) / 2.;
        double midY = ( btmRight.y() + topLeft.y() ) / 2.;
        double xHairMidSizeX = kXHairSelectedCpsBox * pixelScale.first;
        double xHairMidSizeY = kXHairSelectedCpsBox * pixelScale.second;
        QLineF selectedCpsCrossHorizLine;
        selectedCpsCrossHorizLine.setLine(midX - xHairMidSizeX, midY, midX + xHairMidSizeX, midY);
        QLineF selectedCpsCrossVertLine;
        selectedCpsCrossVertLine.setLine(midX, midY - xHairMidSizeY, midX, midY + xHairMidSizeY);

        glLineWidth(1.5 * screenPixelRatio);
        glBegin(GL_LINES);
        glVertex2f( std::max( selectedCpsCrossHorizLine.p1().x(), topLeft.x() ), selectedCpsCrossHorizLine.p1().y() );
        glVertex2f( std::min( selectedCpsCrossHorizLine.p2().x(), btmRight.x() ), selectedCpsCrossHorizLine.p2().y() );
        glVertex2f( selectedCpsCrossVertLine.p1().x(), std::max( selectedCpsCrossVertLine.p1().y(), btmRight.y() ) );
        glVertex2f( selectedCpsCrossVertLine.p2().x(), std::min( selectedCpsCrossVertLine.p2().y(), topLeft.y() ) );
        glEnd();

        glCheckError();


        QPointF midTop( ( topLeft.x() + btmRight.x() ) / 2., topLeft.y() );
        QPointF midRight(btmRight.x(), ( topLeft.y() + btmRight.y() ) / 2.);
        QPointF midBtm( ( topLeft.x() + btmRight.x() ) / 2., btmRight.y() );
        QPointF midLeft(topLeft.x(), ( topLeft.y() + btmRight.y() ) / 2.);

        ///draw the 4 corners points and the 4 mid points
        glPointSize(5.f * screenPixelRatio);
        glBegin(GL_POINTS);
        glVertex2f( topLeft.x(), topLeft.y() );
        glVertex2f( btmRight.x(), topLeft.y() );
        glVertex2f( btmRight.x(), btmRight.y() );
        glVertex2f( topLeft.x(), btmRight.y() );

        glVertex2f( midTop.x(), midTop.y() );
        glVertex2f( midRight.x(), midRight.y() );
        glVertex2f( midBtm.x(), midBtm.y() );
        glVertex2f( midLeft.x(), midLeft.y() );
        glEnd();

        ///now draw the handles to indicate the user he/she can transform the selection rectangle
        ///draw it only if it is not dragged
        bool drawHandles = state != eEventStateDraggingBBoxBtmLeft && state != eEventStateDraggingBBoxBtmRight &&
                           state != eEventStateDraggingBBoxTopLeft && state != eEventStateDraggingBBoxTopRight && state != eEventStateDraggingBBoxMidTop
                           && state != eEventStateDraggingBBoxMidRight && state != eEventStateDraggingBBoxMidLeft && state != eEventStateDraggingBBoxMidBtm;


        if (drawHandles) {
            double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
            double halfOffset = offset / 2.;
            if (transformMode == eSelectedCpsTransformModeTranslateAndScale) {
                ///draw mid top arrow vertical
                drawArrow(midTop.x(), midTop.y() + offset, 0., hoverState == eHoverStateBboxMidTop, pixelScale);
                ///draw mid right arrow horizontal
                drawArrow(midRight.x() + offset, midRight.y(), 90., hoverState == eHoverStateBboxMidRight, pixelScale);
                ///draw mid btm arrow vertical
                drawArrow(midBtm.x(), midBtm.y() - offset, 0., hoverState == eHoverStateBboxMidBtm, pixelScale);
                ///draw mid left arrow horizontal
                drawArrow(midLeft.x() - offset, midLeft.y(), 90., hoverState == eHoverStateBboxMidLeft, pixelScale);
                ///draw top left arrow rotated
                drawArrow(topLeft.x() - offset, topLeft.y() + offset, 45., hoverState == eHoverStateBboxTopLeft, pixelScale);
                ///draw top right arrow rotated
                drawArrow(btmRight.x() + offset, topLeft.y() + offset, -45., hoverState == eHoverStateBboxTopRight, pixelScale);
                ///draw btm right arrow rotated
                drawArrow(btmRight.x() + offset, btmRight.y() - offset, 45., hoverState == eHoverStateBboxBtmRight, pixelScale);
                ///draw btm left arrow rotated
                drawArrow(topLeft.x() - offset, btmRight.y() - offset, -45., hoverState == eHoverStateBboxBtmLeft, pixelScale);
            } else {
                ///draw mid top arrow horizontal
                drawArrow(midTop.x(), midTop.y() + offset, 90., hoverState == eHoverStateBboxMidTop, pixelScale);
                ///draw mid right arrow vertical
                drawArrow(midRight.x() + offset, midRight.y(), 0., hoverState == eHoverStateBboxMidRight, pixelScale);
                ///draw mid btm arrow horizontal
                drawArrow(midBtm.x(), midBtm.y() - offset, 90., hoverState == eHoverStateBboxMidBtm, pixelScale);
                ///draw mid left arrow vertical
                drawArrow(midLeft.x() - offset, midLeft.y(), 0., hoverState == eHoverStateBboxMidLeft, pixelScale);
                ///draw the top left bended
                drawBendedArrow(topLeft.x() - halfOffset, topLeft.y() + halfOffset, 0., hoverState == eHoverStateBboxTopLeft, pixelScale);
                ///draw the top right bended
                drawBendedArrow(btmRight.x() + halfOffset, topLeft.y() + halfOffset, -90, hoverState == eHoverStateBboxTopRight, pixelScale);
                ///draw the btm right bended
                drawBendedArrow(btmRight.x() + halfOffset, btmRight.y() - halfOffset, -180, hoverState == eHoverStateBboxBtmRight, pixelScale);
                ///draw the btm left bended
                drawBendedArrow(topLeft.x() - halfOffset, btmRight.y() - halfOffset, 90, hoverState == eHoverStateBboxBtmLeft, pixelScale);
            }
        }
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
} // drawSelectedCpsBBOX

void
RotoPaintInteract::clearSelection()
{
    clearBeziersSelection();
    clearCPSSelection();
}

bool
RotoPaintInteract::hasSelection() const
{
    return !selectedItems.empty() || !selectedCps.empty();
}

void
RotoPaintInteract::clearCPSSelection()
{
    selectedCps.clear();
    showCpsBbox = false;
    transformMode = eSelectedCpsTransformModeTranslateAndScale;
    selectedCpsBbox.setTopLeft( QPointF(0, 0) );
    selectedCpsBbox.setTopRight( QPointF(0, 0) );
}

void
RotoPaintInteract::clearBeziersSelection()
{
    RotoContextPtr ctx = p->publicInterface->getNode()->getRotoContext();

    assert(ctx);
    ctx->clearSelection(RotoItem::eSelectionReasonOverlayInteract);
    selectedItems.clear();
}

bool
RotoPaintInteract::removeItemFromSelection(const RotoDrawableItemPtr& b)
{
    RotoContextPtr ctx = p->publicInterface->getNode()->getRotoContext();

    assert(ctx);
    for (SelectedItems::iterator fb = selectedItems.begin(); fb != selectedItems.end(); ++fb) {
        if ( fb->get() == b.get() ) {
            ctx->deselect(*fb, RotoItem::eSelectionReasonOverlayInteract);
            selectedItems.erase(fb);

            return true;
        }
    }

    return false;
}

static void
handleControlPointMaximum(double time,
                          const BezierCP & p,
                          double* l,
                          double *b,
                          double *r,
                          double *t)
{
    double x, y, xLeft, yLeft, xRight, yRight;

    p.getPositionAtTime(true, time, ViewIdx(0), &x, &y);
    p.getLeftBezierPointAtTime(true, time, ViewIdx(0), &xLeft, &yLeft);
    p.getRightBezierPointAtTime(true, time,  ViewIdx(0), &xRight, &yRight);

    *r = std::max(x, *r);
    *l = std::min(x, *l);

    *r = std::max(xLeft, *r);
    *l = std::min(xLeft, *l);

    *r = std::max(xRight, *r);
    *l = std::min(xRight, *l);

    *t = std::max(y, *t);
    *b = std::min(y, *b);

    *t = std::max(yLeft, *t);
    *b = std::min(yLeft, *b);


    *t = std::max(yRight, *t);
    *b = std::min(yRight, *b);
}

bool
RotoPaintInteract::getRoleForGroup(const KnobGroupPtr& k,
                                   RotoRoleEnum* role) const
{
    bool ret = true;

    if ( k == selectToolGroup.lock() ) {
        *role = eRotoRoleSelection;
    } else if ( k == pointsEditionToolGroup.lock() ) {
        *role = eRotoRolePointsEdition;
    } else if ( k == bezierEditionToolGroup.lock() ) {
        *role = eRotoRoleBezierEdition;
    } else if ( k == paintBrushToolGroup.lock() ) {
        *role = eRotoRolePaintBrush;
    } else if ( k == cloneBrushToolGroup.lock() ) {
        *role = eRotoRoleCloneBrush;
    } else if ( k == effectBrushToolGroup.lock() ) {
        *role = eRotoRoleEffectBrush;
    } else if ( k == mergeBrushToolGroup.lock() ) {
        *role = eRotoRoleMergeBrush;
    } else {
        ret = false;
    }

    return ret;
}

bool
RotoPaintInteract::getToolForAction(const KnobButtonPtr& k,
                                    RotoToolEnum* tool) const
{
    bool ret = true;

    if ( k == selectAllAction.lock() ) {
        *tool = eRotoToolSelectAll;
    } else if ( k == selectPointsAction.lock() ) {
        *tool = eRotoToolSelectPoints;
    } else if ( k == selectCurvesAction.lock() ) {
        *tool = eRotoToolSelectCurves;
    } else if ( k == selectFeatherPointsAction.lock() ) {
        *tool = eRotoToolSelectFeatherPoints;
    } else if ( k == addPointsAction.lock() ) {
        *tool = eRotoToolAddPoints;
    } else if ( k == removePointsAction.lock() ) {
        *tool = eRotoToolRemovePoints;
    } else if ( k == cuspPointsAction.lock() ) {
        *tool = eRotoToolCuspPoints;
    } else if ( k == smoothPointsAction.lock() ) {
        *tool = eRotoToolSmoothPoints;
    } else if ( k == openCloseCurveAction.lock() ) {
        *tool = eRotoToolOpenCloseCurve;
    } else if ( k == removeFeatherAction.lock() ) {
        *tool = eRotoToolRemoveFeatherPoints;
    } else if ( k == drawBezierAction.lock() ) {
        *tool = eRotoToolDrawBezier;
    } else if ( k == drawEllipseAction.lock() ) {
        *tool = eRotoToolDrawEllipse;
    } else if ( k == drawRectangleAction.lock() ) {
        *tool = eRotoToolDrawRectangle;
    } else if ( k == brushAction.lock() ) {
        *tool = eRotoToolSolidBrush;
    } else if ( k == pencilAction.lock() ) {
        *tool = eRotoToolOpenBezier;
    } else if ( k == eraserAction.lock() ) {
        *tool = eRotoToolEraserBrush;
    } else if ( k == cloneAction.lock() ) {
        *tool = eRotoToolClone;
    } else if ( k == revealAction.lock() ) {
        *tool = eRotoToolReveal;
    } else if ( k == blurAction.lock() ) {
        *tool = eRotoToolBlur;
    } else if ( k == smearAction.lock() ) {
        *tool = eRotoToolSmear;
    } else if ( k == dodgeAction.lock() ) {
        *tool = eRotoToolDodge;
    } else if ( k == burnAction.lock() ) {
        *tool = eRotoToolBurn;
    } else {
        ret = false;
    }

    return ret;
} // RotoPaintInteract::getToolForAction

bool
RotoPaintInteract::onRoleChangedInternal(const KnobGroupPtr& roleGroup)
{
    RotoRoleEnum role;

    if ( !getRoleForGroup(roleGroup, &role) ) {
        return false;
    }

    // The gui just deactivated this action
    if ( !roleGroup->getValue() ) {
        return true;
    }

    bool isPaintRole = (role == eRotoRolePaintBrush) || (role == eRotoRoleCloneBrush) || (role == eRotoRoleMergeBrush) ||
                       (role == eRotoRoleEffectBrush);


    // Reset the selected control points
    selectedCps.clear();
    showCpsBbox = false;
    transformMode = eSelectedCpsTransformModeTranslateAndScale;
    selectedCpsBbox.setTopLeft( QPointF(0, 0) );
    selectedCpsBbox.setTopRight( QPointF(0, 0) );


    // Roto action bar
    autoKeyingEnabledButton.lock()->setInViewerContextSecret(isPaintRole);
    featherLinkEnabledButton.lock()->setInViewerContextSecret(isPaintRole);
    displayFeatherEnabledButton.lock()->setInViewerContextSecret(isPaintRole);
    stickySelectionEnabledButton.lock()->setInViewerContextSecret(isPaintRole);
    bboxClickAnywhereButton.lock()->setInViewerContextSecret(isPaintRole);
    rippleEditEnabledButton.lock()->setInViewerContextSecret(isPaintRole);
    addKeyframeButton.lock()->setInViewerContextSecret(isPaintRole);
    removeKeyframeButton.lock()->setInViewerContextSecret(isPaintRole);
    showTransformHandle.lock()->setInViewerContextSecret(isPaintRole);

    // RotoPaint action bar
    colorWheelButton.lock()->setInViewerContextSecret(!isPaintRole);
    compositingOperatorChoice.lock()->setInViewerContextSecret(!isPaintRole);
    opacitySpinbox.lock()->setInViewerContextSecret(!isPaintRole);
    pressureOpacityButton.lock()->setInViewerContextSecret(!isPaintRole);
    sizeSpinbox.lock()->setInViewerContextSecret(!isPaintRole);
    pressureSizeButton.lock()->setInViewerContextSecret(!isPaintRole);
    hardnessSpinbox.lock()->setInViewerContextSecret(!isPaintRole);
    pressureHardnessButton.lock()->setInViewerContextSecret(!isPaintRole);
    buildUpButton.lock()->setInViewerContextSecret(!isPaintRole);
    effectSpinBox.lock()->setInViewerContextSecret(!isPaintRole);
    timeOffsetSpinBox.lock()->setInViewerContextSecret(!isPaintRole);
    timeOffsetModeChoice.lock()->setInViewerContextSecret(!isPaintRole);
    sourceTypeChoice.lock()->setInViewerContextSecret(!isPaintRole);
    resetCloneOffsetButton.lock()->setInViewerContextSecret(!isPaintRole);
    multiStrokeEnabled.lock()->setInViewerContextSecret(!isPaintRole);

    selectedRole = role;

    return true;
} // RotoPaintInteract::onRoleChangedInternal

bool
RotoPaintInteract::onToolChangedInternal(const KnobButtonPtr& actionButton)
{
    RotoToolEnum tool;

    if ( !getToolForAction(actionButton, &tool) ) {
        return false;
    }

    // The gui just deactivated this action
    if ( !actionButton->getValue() ) {
        return true;
    }

    bool isPaintRole = (selectedRole == eRotoRolePaintBrush) || (selectedRole == eRotoRoleCloneBrush) || (selectedRole == eRotoRoleMergeBrush) ||
                       (selectedRole == eRotoRoleEffectBrush);
    if (isPaintRole) {
        effectSpinBox.lock()->setInViewerContextSecret(tool != eRotoToolBlur);
        timeOffsetModeChoice.lock()->setInViewerContextSecret(selectedRole != eRotoRoleCloneBrush);
        timeOffsetSpinBox.lock()->setInViewerContextSecret(selectedRole != eRotoRoleCloneBrush);
        sourceTypeChoice.lock()->setInViewerContextSecret(selectedRole != eRotoRoleCloneBrush);
        resetCloneOffsetButton.lock()->setInViewerContextSecret(selectedRole != eRotoRoleCloneBrush);
        if (tool == eRotoToolClone) {
            sourceTypeChoice.lock()->setValue(1);
        } else if (tool == eRotoToolReveal) {
            sourceTypeChoice.lock()->setValue(2);
        }

        if ( (tool == eRotoToolSolidBrush ) || ( tool == eRotoToolOpenBezier ) ) {
            compositingOperatorChoice.lock()->setValue( (int)eMergeOver );
        } else if (tool == eRotoToolBurn) {
            compositingOperatorChoice.lock()->setValue( (int)eMergeColorBurn );
        } else if (tool == eRotoToolDodge) {
            compositingOperatorChoice.lock()->setValue( (int)eMergeColorDodge );
        } else {
            compositingOperatorChoice.lock()->setValue( (int)eMergeCopy );
        }
    }

    // Clear all selection if we were building a new bezier
    if ( (selectedRole == eRotoRoleBezierEdition) &&
         ( ( selectedTool == eRotoToolDrawBezier) || ( selectedTool == eRotoToolOpenBezier) ) &&
         builtBezier &&
         ( tool != selectedTool ) ) {
        builtBezier->setCurveFinished(true);
        clearSelection();
    }
    selectedToolAction = actionButton;
    {
        KnobIPtr parentKnob = actionButton->getParentKnob();
        KnobGroupPtr parentGroup = boost::dynamic_pointer_cast<KnobGroup>(parentKnob);
        assert(parentGroup);
        selectedToolRole = parentGroup;
    }
    selectedTool = tool;
    if ( (tool != eRotoToolEraserBrush) && isPaintRole ) {
        lastPaintToolAction = actionButton;
    }


    if ( (selectedTool == eRotoToolBlur) ||
         ( selectedTool == eRotoToolBurn) ||
         ( selectedTool == eRotoToolDodge) ||
         ( selectedTool == eRotoToolClone) ||
         ( selectedTool == eRotoToolEraserBrush) ||
         ( selectedTool == eRotoToolSolidBrush) ||
         ( selectedTool == eRotoToolReveal) ||
         ( selectedTool == eRotoToolSmear) ||
         ( selectedTool == eRotoToolSharpen) ) {
        makeStroke( true, RotoPoint() );
    }

    return true;
} // RotoPaintInteract::onToolChangedInternal

void
RotoPaintInteract::setCurrentTool(const KnobButtonPtr& tool)
{
    if (!tool) {
        return;
    }
    KnobIPtr parentKnob = tool->getParentKnob();
    KnobGroupPtr parentGroup = boost::dynamic_pointer_cast<KnobGroup>(parentKnob);
    assert(parentGroup);
    if (!parentGroup) {
        return;
    }

    RotoToolEnum toolEnum;
    getToolForAction(tool, &toolEnum);

    RotoRoleEnum roleEnum;
    getRoleForGroup(parentGroup, &roleEnum);

    KnobGroupPtr curGroup = selectedToolRole.lock();
    KnobButtonPtr curToolButton = selectedToolAction.lock();
    if ( curGroup && (curGroup != parentGroup) ) {
        curGroup->setValue(false);
    }

    // If we changed group, just keep this action on
    if ( curToolButton && (curGroup == parentGroup) ) {
        curToolButton->setValue(false);
    }
    selectedToolAction = tool;
    selectedToolRole = parentGroup;
    if (parentGroup != curGroup) {
        if (parentGroup->getValue() != true) {
            parentGroup->setValue(true);
        } else {
            onRoleChangedInternal(parentGroup);
        }
    }
    if (tool->getValue() != true) {
        tool->setValue(true);
    } else {
        // We must notify of the change
        onToolChangedInternal(tool);
    }
}

void
RotoPaintInteract::computeSelectedCpsBBOX()
{
    NodePtr n = p->publicInterface->getNode();

    if ( !n || !n->isActivated() ) {
        return;
    }

    double time = p->publicInterface->getCurrentTime();

    double l = INT_MAX, r = INT_MIN, b = INT_MAX, t = INT_MIN;
    for (SelectedCPs::iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
        handleControlPointMaximum(time, *(it->first), &l, &b, &r, &t);
        if (it->second) {
            handleControlPointMaximum(time, *(it->second), &l, &b, &r, &t);
        }
    }
    selectedCpsBbox.setCoords(l, t, r, b);
    if (selectedCps.size() > 1) {
        showCpsBbox = true;
    } else {
        showCpsBbox = false;
    }
}

QPointF
RotoPaintInteract::getSelectedCpsBBOXCenter()
{
    return selectedCpsBbox.center();
}

void
RotoPaintInteract::handleBezierSelection(const BezierPtr & curve)
{
    ///find out if the bezier is already selected.
    bool found = false;

    for (SelectedItems::iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        if ( it->get() == curve.get() ) {
            found = true;
            break;
        }
    }

    if (!found) {
        ///clear previous selection if the SHIFT modifier isn't held
        if (!shiftDown) {
            clearBeziersSelection();
        }
        selectedItems.push_back(curve);

        RotoContextPtr ctx = p->publicInterface->getNode()->getRotoContext();
        assert(ctx);
        ctx->select(curve, RotoItem::eSelectionReasonOverlayInteract);
    }
}

void
RotoPaintInteract::handleControlPointSelection(const std::pair<BezierCPPtr,
                                                               BezierCPPtr> & p)
{
    ///find out if the cp is already selected.
    SelectedCPs::iterator foundCP = selectedCps.end();

    for (SelectedCPs::iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
        if (p.first == it->first) {
            foundCP = it;
            break;
        }
    }

    if ( foundCP == selectedCps.end() ) {
        ///clear previous selection if the SHIFT modifier isn't held
        if (!shiftDown) {
            selectedCps.clear();
        }
        selectedCps.push_back(p);
        computeSelectedCpsBBOX();
    } else {
        ///Erase the point from the selection to allow the user to toggle the selection
        if (shiftDown) {
            selectedCps.erase(foundCP);
            computeSelectedCpsBBOX();
        }
    }

    cpBeingDragged = p;
    state = eEventStateDraggingControlPoint;
    evaluateOnPenUp = true; // so that color is restored to normal on pen up, even if there's no motion
}

void
RotoPaintInteract::showMenuForControlPoint(const BezierCPPtr& /*cp*/)
{
    KnobChoicePtr menu = rightClickMenuKnob.lock();

    if (!menu) {
        return;
    }
    std::vector<KnobButtonPtr> menuKnobs;

    menuKnobs.push_back( removeItemsMenuAction.lock());
    menuKnobs.push_back( smoothItemMenuAction.lock());
    menuKnobs.push_back( cuspItemMenuAction.lock());
    menuKnobs.push_back( removeItemFeatherMenuAction.lock());
    menuKnobs.push_back( nudgeLeftMenuAction.lock());
    menuKnobs.push_back( nudgeBottomMenuAction.lock() );
    menuKnobs.push_back( nudgeRightMenuAction.lock());
    menuKnobs.push_back( nudgeTopMenuAction.lock());

    std::vector<ChoiceOption> choices(menuKnobs.size());
    for (std::size_t i = 0; i < menuKnobs.size(); ++i) {
        choices[i].id = menuKnobs[i]->getName();
        choices[i].tooltip = menuKnobs[i]->getHintToolTip();
    }
    menu->populateChoices(choices);

} // showMenuForControlPoint

void
RotoPaintInteract::showMenuForCurve(const BezierPtr & curve)
{
    KnobChoicePtr menu = rightClickMenuKnob.lock();

    if (!menu) {
        return;
    }
    std::vector<KnobButtonPtr> menuKnobs;

    menuKnobs.push_back( selectAllMenuAction.lock());
    menuKnobs.push_back( removeItemsMenuAction.lock());
    if ( !curve->isOpenBezier() ) {
        menuKnobs.push_back( openCloseCurveAction.lock());
    }

    menuKnobs.push_back( smoothItemMenuAction.lock());
    menuKnobs.push_back( cuspItemMenuAction.lock());
    if ( !curve->isOpenBezier() ) {
        menuKnobs.push_back( removeItemFeatherMenuAction.lock());
    }
    menuKnobs.push_back( lockShapeMenuAction.lock() );

    std::vector<ChoiceOption> choices(menuKnobs.size());
    for (std::size_t i = 0; i < menuKnobs.size(); ++i) {
        choices[i].id = menuKnobs[i]->getName();
        choices[i].tooltip = menuKnobs[i]->getHintToolTip();
    }

    menu->populateChoices(choices);
} // showMenuForCurve

void
RotoPaintInteract::onBreakMultiStrokeTriggered()
{
    makeStroke( true, RotoPoint() );
}

static bool
isBranchConnectedToRotoNodeRecursive(Node* node,
                                     const Node* rotoNode,
                                     int* recursion,
                                     std::list<Node*>& markedNodes)
{
    assert(recursion);
    if (!node) {
        return false;
    }
    if (rotoNode == node) {
        return true;
    }
    markedNodes.push_back(node);
    int maxInputs = node->getNInputs();
    *recursion = *recursion + 1;
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr inp = node->getInput(i);
        if (inp) {
            if ( isBranchConnectedToRotoNodeRecursive(inp.get(), rotoNode, recursion, markedNodes) ) {
                return true;
            }
        }
    }

    return false;
}

void
RotoPaintInteract::checkViewersAreDirectlyConnected()
{
    NodePtr rotoNode = p->publicInterface->getNode();
    std::list<ViewerInstance*> viewers;

    rotoNode->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance*>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        NodePtr viewerNode = (*it)->getNode();
        int maxInputs = viewerNode->getNInputs();
        int hasBranchConnectedToRoto = -1;
        for (int i = 0; i < maxInputs; ++i) {
            NodePtr input = viewerNode->getInput(i);
            if (input) {
                std::list<Node*> markedNodes;
                int recursion = 0;
                if ( isBranchConnectedToRotoNodeRecursive(input.get(), rotoNode.get(), &recursion, markedNodes) ) {
                    if (recursion == 0) {
                        //This viewer is already connected to the Roto node directly.
                        break;
                    }
                    viewerNode->disconnectInput(i);
                    if (hasBranchConnectedToRoto == -1) {
                        viewerNode->connectInput(rotoNode, i);
                        hasBranchConnectedToRoto = i;
                    }
                }
            }
        }
    }
}

void
RotoPaintInteract::makeStroke(bool prepareForLater,
                              const RotoPoint& point)
{
    RotoStrokeType strokeType;
    std::string itemName;

    switch (selectedTool) {
    case eRotoToolSolidBrush:
        strokeType = eRotoStrokeTypeSolid;
        itemName = kRotoPaintBrushBaseName;
        break;
    case eRotoToolEraserBrush:
        strokeType = eRotoStrokeTypeEraser;
        itemName = kRotoPaintEraserBaseName;
        break;
    case eRotoToolClone:
        strokeType = eRotoStrokeTypeClone;
        itemName = kRotoPaintCloneBaseName;
        break;
    case eRotoToolReveal:
        strokeType = eRotoStrokeTypeReveal;
        itemName = kRotoPaintRevealBaseName;
        break;
    case eRotoToolBlur:
        strokeType = eRotoStrokeTypeBlur;
        itemName = kRotoPaintBlurBaseName;
        break;
    case eRotoToolSharpen:
        strokeType = eRotoStrokeTypeSharpen;
        itemName = kRotoPaintSharpenBaseName;
        break;
    case eRotoToolSmear:
        strokeType = eRotoStrokeTypeSmear;
        itemName = kRotoPaintSmearBaseName;
        break;
    case eRotoToolDodge:
        strokeType = eRotoStrokeTypeDodge;
        itemName = kRotoPaintDodgeBaseName;
        break;
    case eRotoToolBurn:
        strokeType = eRotoStrokeTypeBurn;
        itemName = kRotoPaintBurnBaseName;
        break;
    default:

        return;
    }

    RotoContextPtr context = p->publicInterface->getNode()->getRotoContext();


    if (prepareForLater || !strokeBeingPaint) {
        if ( strokeBeingPaint &&
             ( strokeBeingPaint->getBrushType() == strokeType) &&
             strokeBeingPaint->isEmpty() ) {
            ///We already have a fresh stroke prepared for that type
            return;
        }
        std::string name = context->generateUniqueName(itemName);
        strokeBeingPaint.reset( new RotoStrokeItem( strokeType, context, name, RotoLayerPtr() ) );
        strokeBeingPaint->createNodes(false);
    }


    assert(strokeBeingPaint);
    KnobColorPtr colorKnob = strokeBeingPaint->getColorKnob();
    KnobChoicePtr operatorKnob = strokeBeingPaint->getOperatorKnob();
    KnobDoublePtr opacityKnob = strokeBeingPaint->getOpacityKnob();
    KnobDoublePtr sizeKnob = strokeBeingPaint->getBrushSizeKnob();
    KnobDoublePtr hardnessKnob = strokeBeingPaint->getBrushHardnessKnob();
    KnobBoolPtr pressureOpaKnob = strokeBeingPaint->getPressureOpacityKnob();
    KnobBoolPtr pressureSizeKnob = strokeBeingPaint->getPressureSizeKnob();
    KnobBoolPtr pressureHardnessKnob = strokeBeingPaint->getPressureHardnessKnob();
    KnobBoolPtr buildUpKnob = strokeBeingPaint->getBuildupKnob();
    KnobChoicePtr timeOffsetModeKnob = strokeBeingPaint->getTimeOffsetModeKnob();
    KnobChoicePtr sourceTypeKnob = strokeBeingPaint->getBrushSourceTypeKnob();
    KnobIntPtr timeOffsetKnob = strokeBeingPaint->getTimeOffsetKnob();
    KnobDoublePtr translateKnob = strokeBeingPaint->getBrushCloneTranslateKnob();
    KnobDoublePtr effectKnob = strokeBeingPaint->getBrushEffectKnob();
    KnobColorPtr colorWheel = colorWheelButton.lock();
    double color[4];
    for (int i = 0; i < 3; ++i) {
        color[i] = colorWheel->getValue(i);
    }

    MergingFunctionEnum compOp = (MergingFunctionEnum)compositingOperatorChoice.lock()->getValue();
    double opacity = opacitySpinbox.lock()->getValue();
    double size = sizeSpinbox.lock()->getValue();
    double hardness = hardnessSpinbox.lock()->getValue();
    bool pressOpa = pressureOpacityButton.lock()->getValue();
    bool pressSize = pressureSizeButton.lock()->getValue();
    bool pressHarness = pressureHardnessButton.lock()->getValue();
    bool buildUp = buildUpButton.lock()->getValue();
    double timeOffset = timeOffsetSpinBox.lock()->getValue();
    double timeOffsetMode_i = timeOffsetModeChoice.lock()->getValue();
    int sourceType_i = sourceTypeChoice.lock()->getValue();
    double effectValue = effectSpinBox.lock()->getValue();

    int time = context->getTimelineCurrentTime();
    strokeBeingPaintedTimelineFrame = time;

    colorKnob->setValues(color[0], color[1], color[2], ViewSpec::all(), eValueChangedReasonNatronGuiEdited);
    operatorKnob->setValueFromID(Merge::getOperatorString(compOp), 0);
    opacityKnob->setValue(opacity);
    sizeKnob->setValue(size);
    hardnessKnob->setValue(hardness);
    pressureOpaKnob->setValue(pressOpa);
    pressureSizeKnob->setValue(pressSize);
    pressureHardnessKnob->setValue(pressHarness);
    buildUpKnob->setValue(buildUp);
    effectKnob->setValue(effectValue);
    if (!prepareForLater) {
        KnobIntPtr lifeTimeFrameKnob = strokeBeingPaint->getLifeTimeFrameKnob();
        lifeTimeFrameKnob->setValue( time );
    }
    if ( (strokeType == eRotoStrokeTypeClone) || (strokeType == eRotoStrokeTypeReveal) ) {
        timeOffsetKnob->setValue(timeOffset);
        timeOffsetModeKnob->setValue(timeOffsetMode_i);
        sourceTypeKnob->setValue(sourceType_i);
        translateKnob->setValues(-cloneOffset.first, -cloneOffset.second, ViewSpec::all(), eValueChangedReasonNatronGuiEdited);
    }
    if (!prepareForLater) {
        RotoLayerPtr layer = context->findDeepestSelectedLayer();
        if (!layer) {
            layer = context->getOrCreateBaseLayer();
        }
        assert(layer);
        context->addItem(layer, 0, strokeBeingPaint, RotoItem::eSelectionReasonOther);
        context->getNode()->getApp()->setUserIsPainting(context->getNode(), strokeBeingPaint, true);
        strokeBeingPaint->appendPoint(true, point);
    }
} // RotoGui::RotoGuiPrivate::makeStroke

bool
RotoPaintInteract::isNearbySelectedCpsCrossHair(const QPointF & pos) const
{
    std::pair<double, double> pixelScale;

    p->publicInterface->getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    double xHairMidSizeX = kXHairSelectedCpsBox * pixelScale.first;
    double xHairMidSizeY = kXHairSelectedCpsBox * pixelScale.second;
    double l = selectedCpsBbox.topLeft().x();
    double r = selectedCpsBbox.bottomRight().x();
    double b = selectedCpsBbox.bottomRight().y();
    double t = selectedCpsBbox.topLeft().y();
    double toleranceX = kXHairSelectedCpsTolerance * pixelScale.first;
    double toleranceY = kXHairSelectedCpsTolerance * pixelScale.second;
    double midX = (l + r) / 2.;
    double midY = (b + t) / 2.;
    double lCross = midX - xHairMidSizeX;
    double rCross = midX + xHairMidSizeX;
    double bCross = midY - xHairMidSizeY;
    double tCross = midY + xHairMidSizeY;

    if ( ( pos.x() >= (lCross - toleranceX) ) &&
         ( pos.x() <= (rCross + toleranceX) ) &&
         ( pos.y() <= (tCross + toleranceY) ) &&
         ( pos.y() >= (bCross - toleranceY) ) ) {
        return true;
    } else {
        return false;
    }
}

bool
RotoPaintInteract::isWithinSelectedCpsBBox(const QPointF& pos) const
{
    //   std::pair<double, double> pixelScale;
    //    viewer->getPixelScale(pixelScale.first,pixelScale.second);

    double l = selectedCpsBbox.topLeft().x();
    double r = selectedCpsBbox.bottomRight().x();
    double b = selectedCpsBbox.bottomRight().y();
    double t = selectedCpsBbox.topLeft().y();
    double toleranceX = 0; //kXHairSelectedCpsTolerance * pixelScale.first;
    double toleranceY = 0; //kXHairSelectedCpsTolerance * pixelScale.second;

    return pos.x() > (l - toleranceX) && pos.x() < (r + toleranceX) &&
           pos.y() > (b - toleranceY) && pos.y() < (t + toleranceY);
}

bool
RotoPaintInteract::isNearbyBBoxTopLeft(const QPointF & p,
                                       double tolerance,
                                       const std::pair<double, double> & pixelScale) const
{
    QPointF corner = selectedCpsBbox.topLeft();

    if ( ( p.x() >= (corner.x() - tolerance) ) && ( p.x() <= (corner.x() + tolerance) ) &&
         ( p.y() >= (corner.y() - tolerance) ) && ( p.y() <= (corner.y() + tolerance) ) ) {
        return true;
    } else {
        double halfOffset = kTransformArrowOffsetFromPoint * pixelScale.first / 2.;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(corner.x() - halfOffset, corner.y() + halfOffset);
        RectD arrowBbox(center.x() - halfLength, center.y() - halfLength, center.x() + halfLength, center.y() + halfLength);

        return arrowBbox.contains( p.x(), p.y() );
    }
}

bool
RotoPaintInteract::isNearbyBBoxTopRight(const QPointF & p,
                                        double tolerance,
                                        const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = selectedCpsBbox.topLeft();
    QPointF btmRight = selectedCpsBbox.bottomRight();
    QPointF corner( btmRight.x(), topLeft.y() );

    if ( ( p.x() >= (corner.x() - tolerance) ) && ( p.x() <= (corner.x() + tolerance) ) &&
         ( p.y() >= (corner.y() - tolerance) ) && ( p.y() <= (corner.y() + tolerance) ) ) {
        return true;
    } else {
        double halfOffset = kTransformArrowOffsetFromPoint * pixelScale.first / 2.;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(corner.x() + halfOffset, corner.y() + halfOffset);
        RectD arrowBbox(center.x() - halfLength, center.y() - halfLength, center.x() + halfLength, center.y() + halfLength);

        return arrowBbox.contains( p.x(), p.y() );
    }
}

bool
RotoPaintInteract::isNearbyBBoxBtmLeft(const QPointF & p,
                                       double tolerance,
                                       const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = selectedCpsBbox.topLeft();
    QPointF btmRight = selectedCpsBbox.bottomRight();
    QPointF corner( topLeft.x(), btmRight.y() );

    if ( ( p.x() >= (corner.x() - tolerance) ) && ( p.x() <= (corner.x() + tolerance) ) &&
         ( p.y() >= (corner.y() - tolerance) ) && ( p.y() <= (corner.y() + tolerance) ) ) {
        return true;
    } else {
        double halfOffset = kTransformArrowOffsetFromPoint * pixelScale.first / 2.;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(corner.x() - halfOffset, corner.y() - halfOffset);
        RectD arrowBbox(center.x() - halfLength, center.y() - halfLength, center.x() + halfLength, center.y() + halfLength);

        return arrowBbox.contains( p.x(), p.y() );
    }
}

bool
RotoPaintInteract::isNearbyBBoxBtmRight(const QPointF & p,
                                        double tolerance,
                                        const std::pair<double, double> & pixelScale) const
{
    QPointF corner = selectedCpsBbox.bottomRight();

    if ( ( p.x() >= (corner.x() - tolerance) ) && ( p.x() <= (corner.x() + tolerance) ) &&
         ( p.y() >= (corner.y() - tolerance) ) && ( p.y() <= (corner.y() + tolerance) ) ) {
        return true;
    } else {
        double halfOffset = kTransformArrowOffsetFromPoint * pixelScale.first / 2.;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(corner.x() + halfOffset, corner.y() - halfOffset);
        RectD arrowBbox(center.x() - halfLength, center.y() - halfLength, center.x() + halfLength, center.y() + halfLength);

        return arrowBbox.contains( p.x(), p.y() );
    }
}

bool
RotoPaintInteract::isNearbyBBoxMidTop(const QPointF & p,
                                      double tolerance,
                                      const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = selectedCpsBbox.topLeft();
    QPointF btmRight = selectedCpsBbox.bottomRight();
    QPointF topRight( btmRight.x(), topLeft.y() );
    QPointF mid = (topLeft + topRight) / 2.;

    if ( ( p.x() >= (mid.x() - tolerance) ) && ( p.x() <= (mid.x() + tolerance) ) &&
         ( p.y() >= (mid.y() - tolerance) ) && ( p.y() <= (mid.y() + tolerance) ) ) {
        return true;
    } else {
        double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;
        ///test if pos is within the arrow bounding box
        QPointF center(mid.x(), mid.y() + offset);
        RectD arrowBbox(center.x() - halfLength, center.y() - halfLength, center.x() + halfLength, center.y() + halfLength);

        return arrowBbox.contains( p.x(), p.y() );
    }
}

bool
RotoPaintInteract::isNearbyBBoxMidRight(const QPointF & p,
                                        double tolerance,
                                        const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = selectedCpsBbox.topLeft();
    QPointF btmRight = selectedCpsBbox.bottomRight();
    QPointF topRight( btmRight.x(), topLeft.y() );
    QPointF mid = (btmRight + topRight) / 2.;

    if ( ( p.x() >= (mid.x() - tolerance) ) && ( p.x() <= (mid.x() + tolerance) ) &&
         ( p.y() >= (mid.y() - tolerance) ) && ( p.y() <= (mid.y() + tolerance) ) ) {
        return true;
    } else {
        double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center( mid.x() + offset, mid.y() );
        RectD arrowBbox(center.x() - halfLength, center.y() - halfLength, center.x() + halfLength, center.y() + halfLength);

        return arrowBbox.contains( p.x(), p.y() );
    }
}

bool
RotoPaintInteract::isNearbyBBoxMidBtm(const QPointF & p,
                                      double tolerance,
                                      const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = selectedCpsBbox.topLeft();
    QPointF btmRight = selectedCpsBbox.bottomRight();
    QPointF btmLeft( topLeft.x(), btmRight.y() );
    QPointF mid = (btmRight + btmLeft) / 2.;

    if ( ( p.x() >= (mid.x() - tolerance) ) && ( p.x() <= (mid.x() + tolerance) ) &&
         ( p.y() >= (mid.y() - tolerance) ) && ( p.y() <= (mid.y() + tolerance) ) ) {
        return true;
    } else {
        double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(mid.x(), mid.y() - offset);
        RectD arrowBbox(center.x() - halfLength, center.y() - halfLength, center.x() + halfLength, center.y() + halfLength);

        return arrowBbox.contains( p.x(), p.y() );
    }
}

EventStateEnum
RotoPaintInteract::isMouseInteractingWithCPSBbox(const QPointF& pos,
                                                 double cpSelectionTolerance,
                                                 const std::pair<double, double>& pixelScale) const
{
    bool clickAnywhere = isBboxClickAnywhereEnabled();
    EventStateEnum state = eEventStateNone;

    if ( showCpsBbox && isNearbyBBoxTopLeft(pos, cpSelectionTolerance, pixelScale) ) {
        state = eEventStateDraggingBBoxTopLeft;
    } else if ( showCpsBbox && isNearbyBBoxTopRight(pos, cpSelectionTolerance, pixelScale) ) {
        state = eEventStateDraggingBBoxTopRight;
    } else if ( showCpsBbox && isNearbyBBoxBtmLeft(pos, cpSelectionTolerance, pixelScale) ) {
        state = eEventStateDraggingBBoxBtmLeft;
    } else if ( showCpsBbox && isNearbyBBoxBtmRight(pos, cpSelectionTolerance, pixelScale) ) {
        state = eEventStateDraggingBBoxBtmRight;
    } else if ( showCpsBbox && isNearbyBBoxMidTop(pos, cpSelectionTolerance, pixelScale) ) {
        state = eEventStateDraggingBBoxMidTop;
    } else if ( showCpsBbox && isNearbyBBoxMidRight(pos, cpSelectionTolerance, pixelScale) ) {
        state = eEventStateDraggingBBoxMidRight;
    } else if ( showCpsBbox && isNearbyBBoxMidBtm(pos, cpSelectionTolerance, pixelScale) ) {
        state = eEventStateDraggingBBoxMidBtm;
    } else if ( showCpsBbox && isNearbyBBoxMidLeft(pos, cpSelectionTolerance, pixelScale) ) {
        state = eEventStateDraggingBBoxMidLeft;
    } else if ( clickAnywhere && showCpsBbox && isWithinSelectedCpsBBox(pos) ) {
        state = eEventStateDraggingSelectedControlPoints;
    } else if ( !clickAnywhere && showCpsBbox && isNearbySelectedCpsCrossHair(pos) ) {
        state = eEventStateDraggingSelectedControlPoints;
    }

    return state;
}

bool
RotoPaintInteract::isNearbyBBoxMidLeft(const QPointF & p,
                                       double tolerance,
                                       const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = selectedCpsBbox.topLeft();
    QPointF btmRight = selectedCpsBbox.bottomRight();
    QPointF btmLeft( topLeft.x(), btmRight.y() );
    QPointF mid = (topLeft + btmLeft) / 2.;

    if ( ( p.x() >= (mid.x() - tolerance) ) && ( p.x() <= (mid.x() + tolerance) ) &&
         ( p.y() >= (mid.y() - tolerance) ) && ( p.y() <= (mid.y() + tolerance) ) ) {
        return true;
    } else {
        double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center( mid.x() - offset, mid.y() );
        RectD arrowBbox(center.x() - halfLength, center.y() - halfLength, center.x() + halfLength, center.y() + halfLength);

        return arrowBbox.contains( p.x(), p.y() );
    }
}

bool
RotoPaintInteract::isNearbySelectedCpsBoundingBox(const QPointF & pos,
                                                  double tolerance) const
{
    QPointF topLeft = selectedCpsBbox.topLeft();
    QPointF btmRight = selectedCpsBbox.bottomRight();
    QPointF btmLeft( topLeft.x(), btmRight.y() );
    QPointF topRight( btmRight.x(), topLeft.y() );

    ///check if it is nearby top edge
    if ( ( pos.x() >= (topLeft.x() - tolerance) ) && ( pos.x() <= (topRight.x() + tolerance) ) &&
         ( pos.y() >= (topLeft.y() - tolerance) ) && ( pos.y() <= (topLeft.y() + tolerance) ) ) {
        return true;
    }

    ///right edge
    if ( ( pos.x() >= (topRight.x() - tolerance) ) && ( pos.x() <= (topRight.x() + tolerance) ) &&
         ( pos.y() >= (btmRight.y() - tolerance) ) && ( pos.y() <= (topRight.y() + tolerance) ) ) {
        return true;
    }

    ///btm edge
    if ( ( pos.x() >= (btmLeft.x() - tolerance) ) && ( pos.x() <= (btmRight.x() + tolerance) ) &&
         ( pos.y() >= (btmLeft.y() - tolerance) ) && ( pos.y() <= (btmLeft.y() + tolerance) ) ) {
        return true;
    }

    ///left edge
    if ( ( pos.x() >= (btmLeft.x() - tolerance) ) && ( pos.x() <= (btmLeft.x() + tolerance) ) &&
         ( pos.y() >= (btmLeft.y() - tolerance) ) && ( pos.y() <= (topLeft.y() + tolerance) ) ) {
        return true;
    }

    return false;
}

std::pair<BezierCPPtr, BezierCPPtr>
RotoPaintInteract::isNearbyFeatherBar(double time,
                                      const std::pair<double, double> & pixelScale,
                                      const QPointF & pos) const
{
    double distFeatherX = 20. * pixelScale.first;
    double acceptance = 10 * pixelScale.second;

    for (SelectedItems::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( it->get() );
        assert(isStroke || isBezier);
        if ( isStroke || !isBezier || ( isBezier && isBezier->isOpenBezier() ) ) {
            continue;
        }

        /*
           For each selected bezier, we compute the extent of the feather bars and check if the mouse would be nearby one of these bars.
           The feather bar of a control point is only displayed is the feather point is equal to the bezier control point.
           In order to give it the  correc direction we use the derivative of the bezier curve at the control point and then use
           the pointInPolygon function to make sure the feather bar is always oriented on the outer part of the polygon.
           The pointInPolygon function needs the polygon of the bezier to test whether the point is inside or outside the polygon
           hence in this loop we compute the polygon for each bezier.
         */

        Transform::Matrix3x3 transform;
        isBezier->getTransformAtTime(time, &transform);

        const std::list<BezierCPPtr> & fps = isBezier->getFeatherPoints();
        const std::list<BezierCPPtr> & cps = isBezier->getControlPoints();
        assert( cps.size() == fps.size() );

        int cpCount = (int)cps.size();
        if (cpCount <= 1) {
            continue;
        }


        std::list<BezierCPPtr>::const_iterator itF = fps.begin();
        std::list<BezierCPPtr>::const_iterator nextF = itF;
        if ( nextF != fps.end() ) {
            ++nextF;
        }
        std::list<BezierCPPtr>::const_iterator prevF = fps.end();
        if ( prevF != fps.begin() ) {
            --prevF;
        }
        bool isClockWiseOriented = isBezier->isFeatherPolygonClockwiseOriented(true, time);

        for (std::list<BezierCPPtr>::const_iterator itCp = cps.begin();
             itCp != cps.end();
             ++itCp) {
            if ( prevF == fps.end() ) {
                prevF = fps.begin();
            }
            if ( nextF == fps.end() ) {
                nextF = fps.begin();
            }
            assert( itF != fps.end() ); // because cps.size() == fps.size()
            if ( itF == fps.end() ) {
                itF = fps.begin();
            }

            Transform::Point3D controlPoint, featherPoint;
            controlPoint.z = featherPoint.z = 1;
            (*itCp)->getPositionAtTime(true, time, ViewIdx(0), &controlPoint.x, &controlPoint.y);
            (*itF)->getPositionAtTime(true, time, ViewIdx(0), &featherPoint.x, &featherPoint.y);

            controlPoint = Transform::matApply(transform, controlPoint);
            featherPoint = Transform::matApply(transform, featherPoint);
            {
                Point cp, fp;
                cp.x = controlPoint.x;
                cp.y = controlPoint.y;
                fp.x = featherPoint.x;
                fp.y = featherPoint.y;
                Bezier::expandToFeatherDistance(true, cp, &fp, distFeatherX, time, isClockWiseOriented, transform, prevF, itF, nextF);
                featherPoint.x = fp.x;
                featherPoint.y = fp.y;
            }
            assert(featherPoint.x != controlPoint.x || featherPoint.y != controlPoint.y);

            ///Now test if the user mouse click is on the line using bounding box and cross product.
            if ( ( ( ( pos.y() >= (controlPoint.y - acceptance) ) && ( pos.y() <= (featherPoint.y + acceptance) ) ) ||
                   ( ( pos.y() >= (featherPoint.y - acceptance) ) && ( pos.y() <= (controlPoint.y + acceptance) ) ) ) &&
                 ( ( ( pos.x() >= (controlPoint.x - acceptance) ) && ( pos.x() <= (featherPoint.x + acceptance) ) ) ||
                   ( ( pos.x() >= (featherPoint.x - acceptance) ) && ( pos.x() <= (controlPoint.x + acceptance) ) ) ) ) {
                Point a;
                a.x = (featherPoint.x - controlPoint.x);
                a.y = (featherPoint.y - controlPoint.y);
                double norm = sqrt(a.x * a.x + a.y * a.y);

                ///The point is in the bounding box of the segment, if it is vertical it must be on the segment anyway
                if (norm == 0) {
                    return std::make_pair(*itCp, *itF);
                }

                a.x /= norm;
                a.y /= norm;
                Point b;
                b.x = (pos.x() - controlPoint.x);
                b.y = (pos.y() - controlPoint.y);
                norm = sqrt(b.x * b.x + b.y * b.y);

                ///This vector is not vertical
                if (norm != 0) {
                    b.x /= norm;
                    b.y /= norm;

                    double crossProduct = b.y * a.x - b.x * a.y;
                    if (std::abs(crossProduct) <  0.3) {
                        return std::make_pair(*itCp, *itF);
                    }
                }
            }

            // increment for next iteration
            // ++itF, ++nextF, ++prevF
            if ( itF != fps.end() ) {
                ++itF;
            }
            if ( nextF != fps.end() ) {
                ++nextF;
            }
            if ( prevF != fps.end() ) {
                ++prevF;
            }
        } // for(itCp)
    }

    return std::make_pair( BezierCPPtr(), BezierCPPtr() );
} // isNearbyFeatherBar

void
RotoPaintInteract::setSelection(const std::list<RotoDrawableItemPtr> & drawables,
                                const std::list<std::pair<BezierCPPtr, BezierCPPtr> > & points)
{
    selectedItems.clear();
    for (std::list<RotoDrawableItemPtr>::const_iterator it = drawables.begin(); it != drawables.end(); ++it) {
        if (*it) {
            selectedItems.push_back(*it);
        }
    }
    selectedCps.clear();
    for (SelectedCPs::const_iterator it = points.begin(); it != points.end(); ++it) {
        if (it->first && it->second) {
            selectedCps.push_back(*it);
        }
    }
    p->publicInterface->getNode()->getRotoContext()->select(selectedItems, RotoItem::eSelectionReasonOverlayInteract);
    computeSelectedCpsBBOX();
}

void
RotoPaintInteract::setSelection(const BezierPtr & curve,
                                const std::pair<BezierCPPtr, BezierCPPtr> & point)
{
    selectedItems.clear();
    if (curve) {
        selectedItems.push_back(curve);
    }
    selectedCps.clear();
    if (point.first && point.second) {
        selectedCps.push_back(point);
    }
    if (curve) {
        p->publicInterface->getNode()->getRotoContext()->select(curve, RotoItem::eSelectionReasonOverlayInteract);
    }
    computeSelectedCpsBBOX();
}

void
RotoPaintInteract::getSelection(std::list<RotoDrawableItemPtr>* beziers,
                                std::list<std::pair<BezierCPPtr, BezierCPPtr> >* points)
{
    *beziers = selectedItems;
    *points = selectedCps;
}

void
RotoPaintInteract::setBuiltBezier(const BezierPtr & curve)
{
    assert(curve);
    builtBezier = curve;
}

BezierPtr RotoPaintInteract::getBezierBeingBuild() const
{
    return builtBezier;
}

bool
RotoPaintInteract::smoothSelectedCurve()
{
    std::pair<double, double> pixelScale;

    p->publicInterface->getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);
    RotoContextPtr context = p->publicInterface->getNode()->getRotoContext();
    double time = context->getTimelineCurrentTime();
    std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;

    if ( !selectedCps.empty() ) {
        for (SelectedCPs::const_iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = it->first->getBezier();
            data.newPoints.push_back(*it);
            datas.push_back(data);
        }
    } else {
        for (SelectedItems::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
            BezierPtr bezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (bezier) {
                SmoothCuspUndoCommand::SmoothCuspCurveData data;
                data.curve = bezier;
                const std::list<BezierCPPtr> & cps = bezier->getControlPoints();
                const std::list<BezierCPPtr> & fps = bezier->getFeatherPoints();
                std::list<BezierCPPtr>::const_iterator itFp = fps.begin();
                for (std::list<BezierCPPtr>::const_iterator it = cps.begin(); it != cps.end(); ++it, ++itFp) {
                    data.newPoints.push_back( std::make_pair(*it, *itFp) );
                }
                datas.push_back(data);
            }
        }
    }
    if ( !datas.empty() ) {
        p->publicInterface->pushUndoCommand( new SmoothCuspUndoCommand(shared_from_this(), datas, time, false, pixelScale) );

        return true;
    }

    return false;
}

bool
RotoPaintInteract::cuspSelectedCurve()
{
    std::pair<double, double> pixelScale;

    p->publicInterface->getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);
    RotoContextPtr context = p->publicInterface->getNode()->getRotoContext();
    double time = context->getTimelineCurrentTime();
    std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;

    if ( !selectedCps.empty() ) {
        for (SelectedCPs::const_iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = it->first->getBezier();
            data.newPoints.push_back(*it);
            datas.push_back(data);
        }
    } else {
        for (SelectedItems::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
            BezierPtr bezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (bezier) {
                SmoothCuspUndoCommand::SmoothCuspCurveData data;
                data.curve = bezier;
                const std::list<BezierCPPtr> & cps = bezier->getControlPoints();
                const std::list<BezierCPPtr> & fps = bezier->getFeatherPoints();
                std::list<BezierCPPtr>::const_iterator itFp = fps.begin();
                for (std::list<BezierCPPtr>::const_iterator it = cps.begin(); it != cps.end(); ++it, ++itFp) {
                    data.newPoints.push_back( std::make_pair(*it, *itFp) );
                }
                datas.push_back(data);
            }
        }
    }
    if ( !datas.empty() ) {
        p->publicInterface->pushUndoCommand( new SmoothCuspUndoCommand(shared_from_this(), datas, time, true, pixelScale) );

        return true;
    }

    return false;
}

bool
RotoPaintInteract::removeFeatherForSelectedCurve()
{
    std::list<RemoveFeatherUndoCommand::RemoveFeatherData> datas;

    if ( !selectedCps.empty() ) {
        for (SelectedCPs::const_iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
            RemoveFeatherUndoCommand::RemoveFeatherData data;
            data.curve = it->first->getBezier();
            data.newPoints = data.curve->getFeatherPoints();
            datas.push_back(data);
        }
    } else {
        for (SelectedItems::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
            BezierPtr bezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (bezier) {
                RemoveFeatherUndoCommand::RemoveFeatherData data;
                data.curve = bezier;
                data.newPoints = bezier->getFeatherPoints();
                datas.push_back(data);
            }
        }
    }
    if ( !datas.empty() ) {
        p->publicInterface->pushUndoCommand( new RemoveFeatherUndoCommand(shared_from_this(), datas) );

        return true;
    }

    return false;
}

bool
RotoPaintInteract::lockSelectedCurves()
{
    ///Make a copy because setLocked will change the selection internally and invalidate the iterator
    SelectedItems selection = selectedItems;

    if ( selection.empty() ) {
        return false;
    }
    for (SelectedItems::const_iterator it = selection.begin(); it != selection.end(); ++it) {
        (*it)->setLocked(true, false, RotoItem::eSelectionReasonOverlayInteract);
    }
    clearSelection();

    return true;
}

bool
RotoPaintInteract::moveSelectedCpsWithKeyArrows(int x,
                                                int y)
{
    std::list<std::pair<BezierCPPtr, BezierCPPtr> > points;

    if ( !selectedCps.empty() ) {
        points = selectedCps;
    } else {
        for (SelectedItems::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
            BezierPtr bezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (bezier) {
                const std::list<BezierCPPtr> & cps = bezier->getControlPoints();
                const std::list<BezierCPPtr> & fps = bezier->getFeatherPoints();
                std::list<BezierCPPtr>::const_iterator fpIt = fps.begin();
                assert( fps.empty() || fps.size() == cps.size() );
                for (std::list<BezierCPPtr>::const_iterator it = cps.begin(); it != cps.end(); ++it) {
                    points.push_back( std::make_pair(*it, *fpIt) );
                    if ( !fps.empty() ) {
                        ++fpIt;
                    }
                }
            }
        }
    }

    if ( !points.empty() ) {
        std::pair<double, double> pixelScale;
        p->publicInterface->getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);
        double time = p->publicInterface->getCurrentTime();

        p->publicInterface->pushUndoCommand( new MoveControlPointsUndoCommand(shared_from_this(), points, (double)x * pixelScale.first,
                                                                              (double)y * pixelScale.second, time) );
        computeSelectedCpsBBOX();
        p->publicInterface->getNode()->getRotoContext()->evaluateChange();

        return true;
    }

    return false;
}

void
RotoPaintInteract::onCurveLockedChangedRecursive(const RotoItemPtr & item,
                                                 bool* ret)
{
    BezierPtr b = boost::dynamic_pointer_cast<Bezier>(item);
    RotoLayerPtr layer = boost::dynamic_pointer_cast<RotoLayer>(item);

    if (b) {
        if ( item->isLockedRecursive() ) {
            for (SelectedItems::iterator fb = selectedItems.begin(); fb != selectedItems.end(); ++fb) {
                if ( fb->get() == b.get() ) {
                    ///if the curve was selected, wipe the selection CP bbox
                    clearCPSSelection();
                    selectedItems.erase(fb);
                    *ret = true;
                    break;
                }
            }
        } else {
            ///Explanation: This change has been made in result to a user click on the settings panel.
            ///We have to reselect the bezier overlay hence put a reason different of eSelectionReasonOverlayInteract
            SelectedItems::iterator found = std::find(selectedItems.begin(), selectedItems.end(), b);
            if ( found == selectedItems.end() ) {
                selectedItems.push_back(b);
                p->publicInterface->getNode()->getRotoContext()->select(b, RotoItem::eSelectionReasonSettingsPanel);
                *ret  = true;
            }
        }
    } else if (layer) {
        const std::list<RotoItemPtr> & items = layer->getItems();
        for (std::list<RotoItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
            onCurveLockedChangedRecursive(*it, ret);
        }
    }
}

void
RotoPaintInteract::removeCurve(const RotoDrawableItemPtr& curve)
{
    if (curve == builtBezier) {
        builtBezier.reset();
    } else if (curve == strokeBeingPaint) {
        strokeBeingPaint.reset();
    }
    getContext()->removeItem(curve);
}

NATRON_NAMESPACE_EXIT
