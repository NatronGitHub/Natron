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


#include "RotoPaintPrivate.h"

#include <QtCore/QLineF>

#include "Global/GLIncludes.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoPaint.h"
#include "Engine/MergingEnum.h"
#include "Engine/Node.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoUndoCommand.h"
#include "Engine/RotoLayer.h"
#include "Engine/Bezier.h"
#include "Engine/Transform.h"
#include "Engine/TimeLine.h"
#include "Engine/AppInstance.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER;

RotoPaintKnobItemsTable::RotoPaintKnobItemsTable(RotoPaintPrivate* imp,
                        KnobItemsTableTypeEnum type)
: KnobItemsTable(imp->publicInterface->shared_from_this(), type)
, _imp(imp)
{
    setSupportsDragAndDrop(true);
    setDropSupportsExternalSources(true);
}

RotoPaintPrivate::RotoPaintPrivate(RotoPaint* publicInterface,
                                   RotoPaint::RotoPaintTypeEnum type)
    : publicInterface(publicInterface)
    , nodeType(type)
    , premultKnob()
    , enabledKnobs()
    , doingNeatRender(false)
    , mustDoNeatRender(false)
    , ui( RotoPaintInteract::create(this) )
    , inputNodes()
    , premultNode()
    , premultFixerNode()
    , knobsTable()
    , globalMergeNodes()
    , treeRefreshBlocked(0)
{
}

RotoPaintInteract::RotoPaintInteract(RotoPaintPrivate* p)
    : p(p)
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
    cp->getLeftBezierPointAtTime(time, &leftDeriv.x, &leftDeriv.y);
    cp->getRightBezierPointAtTime(time,  &rightDeriv.x, &rightDeriv.y);
    leftDeriv = Transform::matApply(transform, leftDeriv);
    rightDeriv = Transform::matApply(transform, rightDeriv);

    bool drawLeftHandle = leftDeriv.x != x || leftDeriv.y != y;
    bool drawRightHandle = rightDeriv.y != x || rightDeriv.y != y;
    GL_GPU::Enable(GL_POINT_SMOOTH);
    GL_GPU::Begin(GL_POINTS);
    if (drawLeftHandle) {
        if (colorLeftTangent) {
            GL_GPU::Color3f(0.2, 1., 0.);
        }
        GL_GPU::Vertex2d(leftDeriv.x, leftDeriv.y);
        if (colorLeftTangent) {
            GL_GPU::Color3d(0.85, 0.67, 0.);
        }
    }
    if (drawRightHandle) {
        if (colorRightTangent) {
            GL_GPU::Color3f(0.2, 1., 0.);
        }
        GL_GPU::Vertex2d(rightDeriv.x, rightDeriv.y);
        if (colorRightTangent) {
            GL_GPU::Color3d(0.85, 0.67, 0.);
        }
    }
    GL_GPU::End();

    GL_GPU::Begin(GL_LINE_STRIP);
    if (drawLeftHandle) {
        GL_GPU::Vertex2d(leftDeriv.x, leftDeriv.y);
    }
    GL_GPU::Vertex2d(x, y);
    if (drawRightHandle) {
        GL_GPU::Vertex2d(rightDeriv.x, rightDeriv.y);
    }
    GL_GPU::End();
    GL_GPU::Disable(GL_POINT_SMOOTH);
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
    GL_GPU::Color3f(r * l * a, g * l * a, b * l * a);

    GL_GPU::PushMatrix();
    //  center the oval at x_center, y_center
    GL_GPU::Translatef( (float)x, (float)y, 0.f );
    //  draw the oval using line segments
    GL_GPU::Begin(GL_LINE_LOOP);
    // we don't need to be pixel-perfect here, it's just an interact!
    // 40 segments is enough.
    double m = 2 * 3.14159265358979323846264338327950288419717 / 40.;
    for (int i = 0; i < 40; ++i) {
        double theta = i * m;
        GL_GPU::Vertex2d( radiusX * std::cos(theta), radiusY * std::sin(theta) );
    }
    GL_GPU::End();

    GL_GPU::PopMatrix();
}

void
RotoPaintInteract::drawArrow(double centerX,
                             double centerY,
                             double rotate,
                             bool hovered,
                             const std::pair<double, double> & pixelScale)
{
    GLProtectMatrix<GL_GPU> p(GL_MODELVIEW);

    if (hovered) {
        GL_GPU::Color3f(0., 1., 0.);
    } else {
        GL_GPU::Color3f(1., 1., 1.);
    }

    double arrowLenght =  kTransformArrowLenght * pixelScale.second;
    double arrowWidth = kTransformArrowWidth * pixelScale.second;
    double arrowHeadHeight = 4 * pixelScale.second;

    GL_GPU::Translatef(centerX, centerY, 0.);
    GL_GPU::Rotatef(rotate, 0., 0., 1.);
    QPointF bottom(0., -arrowLenght);
    QPointF top(0, arrowLenght);
    ///the arrow head is 4 pixels long and kTransformArrowWidth * 2 large
    GL_GPU::Begin(GL_LINES);
    GL_GPU::Vertex2f( top.x(), top.y() );
    GL_GPU::Vertex2f( bottom.x(), bottom.y() );
    GL_GPU::End();

    GL_GPU::Begin(GL_POLYGON);
    GL_GPU::Vertex2f( bottom.x(), bottom.y() );
    GL_GPU::Vertex2f(bottom.x() + arrowWidth, bottom.y() + arrowHeadHeight);
    GL_GPU::Vertex2f(bottom.x() - arrowWidth, bottom.y() + arrowHeadHeight);
    GL_GPU::End();

    GL_GPU::Begin(GL_POLYGON);
    GL_GPU::Vertex2f( top.x(), top.y() );
    GL_GPU::Vertex2f(top.x() - arrowWidth, top.y() - arrowHeadHeight);
    GL_GPU::Vertex2f(top.x() + arrowWidth, top.y() - arrowHeadHeight);
    GL_GPU::End();
}

void
RotoPaintInteract::drawBendedArrow(double centerX,
                                   double centerY,
                                   double rotate,
                                   bool hovered,
                                   const std::pair<double, double> & pixelScale)
{
    GLProtectMatrix<GL_GPU> p(GL_MODELVIEW);

    if (hovered) {
        GL_GPU::Color3f(0., 1., 0.);
    } else {
        GL_GPU::Color3f(1., 1., 1.);
    }

    double arrowLenght =  kTransformArrowLenght * pixelScale.second;
    double arrowWidth = kTransformArrowWidth * pixelScale.second;
    double arrowHeadHeight = 4 * pixelScale.second;

    GL_GPU::Translatef(centerX, centerY, 0.);
    GL_GPU::Rotatef(rotate, 0., 0., 1.);

    /// by default we draw the top left
    QPointF bottom(0., -arrowLenght / 2.);
    QPointF right(arrowLenght / 2., 0.);
    GL_GPU::Begin (GL_LINE_STRIP);
    GL_GPU::Vertex2f ( bottom.x(), bottom.y() );
    GL_GPU::Vertex2f (0., 0.);
    GL_GPU::Vertex2f ( right.x(), right.y() );
    GL_GPU::End ();

    GL_GPU::Begin(GL_POLYGON);
    GL_GPU::Vertex2f(bottom.x(), bottom.y() - arrowHeadHeight);
    GL_GPU::Vertex2f( bottom.x() - arrowWidth, bottom.y() );
    GL_GPU::Vertex2f( bottom.x() + arrowWidth, bottom.y() );
    GL_GPU::End();

    GL_GPU::Begin(GL_POLYGON);
    GL_GPU::Vertex2f( right.x() + arrowHeadHeight, right.y() );
    GL_GPU::Vertex2f(right.x(), right.y() - arrowWidth);
    GL_GPU::Vertex2f(right.x(), right.y() + arrowWidth);
    GL_GPU::End();
}

void
RotoPaintInteract::drawSelectedCpsBBOX()
{
    std::pair<double, double> pixelScale;

    p->publicInterface->getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::Enable(GL_LINE_SMOOTH);
        GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);


        QPointF topLeft = selectedCpsBbox.topLeft();
        QPointF btmRight = selectedCpsBbox.bottomRight();

        GL_GPU::LineWidth(1.5);

        if (hoverState == eHoverStateBbox) {
            GL_GPU::Color4f(0.9, 0.5, 0, 1.);
        } else {
            GL_GPU::Color4f(0.8, 0.8, 0.8, 1.);
        }
        GL_GPU::Begin(GL_LINE_LOOP);
        GL_GPU::Vertex2f( topLeft.x(), btmRight.y() );
        GL_GPU::Vertex2f( topLeft.x(), topLeft.y() );
        GL_GPU::Vertex2f( btmRight.x(), topLeft.y() );
        GL_GPU::Vertex2f( btmRight.x(), btmRight.y() );
        GL_GPU::End();

        double midX = ( topLeft.x() + btmRight.x() ) / 2.;
        double midY = ( btmRight.y() + topLeft.y() ) / 2.;
        double xHairMidSizeX = kXHairSelectedCpsBox * pixelScale.first;
        double xHairMidSizeY = kXHairSelectedCpsBox * pixelScale.second;
        QLineF selectedCpsCrossHorizLine;
        selectedCpsCrossHorizLine.setLine(midX - xHairMidSizeX, midY, midX + xHairMidSizeX, midY);
        QLineF selectedCpsCrossVertLine;
        selectedCpsCrossVertLine.setLine(midX, midY - xHairMidSizeY, midX, midY + xHairMidSizeY);

        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2f( std::max( selectedCpsCrossHorizLine.p1().x(), topLeft.x() ), selectedCpsCrossHorizLine.p1().y() );
        GL_GPU::Vertex2f( std::min( selectedCpsCrossHorizLine.p2().x(), btmRight.x() ), selectedCpsCrossHorizLine.p2().y() );
        GL_GPU::Vertex2f( selectedCpsCrossVertLine.p1().x(), std::max( selectedCpsCrossVertLine.p1().y(), btmRight.y() ) );
        GL_GPU::Vertex2f( selectedCpsCrossVertLine.p2().x(), std::min( selectedCpsCrossVertLine.p2().y(), topLeft.y() ) );
        GL_GPU::End();

        glCheckError(GL_GPU);


        QPointF midTop( ( topLeft.x() + btmRight.x() ) / 2., topLeft.y() );
        QPointF midRight(btmRight.x(), ( topLeft.y() + btmRight.y() ) / 2.);
        QPointF midBtm( ( topLeft.x() + btmRight.x() ) / 2., btmRight.y() );
        QPointF midLeft(topLeft.x(), ( topLeft.y() + btmRight.y() ) / 2.);

        ///draw the 4 corners points and the 4 mid points
        GL_GPU::PointSize(5.f);
        GL_GPU::Begin(GL_POINTS);
        GL_GPU::Vertex2f( topLeft.x(), topLeft.y() );
        GL_GPU::Vertex2f( btmRight.x(), topLeft.y() );
        GL_GPU::Vertex2f( btmRight.x(), btmRight.y() );
        GL_GPU::Vertex2f( topLeft.x(), btmRight.y() );

        GL_GPU::Vertex2f( midTop.x(), midTop.y() );
        GL_GPU::Vertex2f( midRight.x(), midRight.y() );
        GL_GPU::Vertex2f( midBtm.x(), midBtm.y() );
        GL_GPU::Vertex2f( midLeft.x(), midLeft.y() );
        GL_GPU::End();

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
    return !p->knobsTable->getSelectedItems().empty() || !selectedCps.empty();
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
    p->knobsTable->clearSelection(eTableChangeReasonViewer);
}

void
RotoPaintInteract::removeItemFromSelection(const RotoDrawableItemPtr& b)
{
    p->knobsTable->removeFromSelection(b, eTableChangeReasonViewer);
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

    p.getPositionAtTime(time, &x, &y);
    p.getLeftBezierPointAtTime(time, &xLeft, &yLeft);
    p.getRightBezierPointAtTime(time, &xRight, &yRight);

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
        builtBezier->setCurveFinished(true, ViewSetSpec::all());
        clearSelection();
    }
    selectedToolAction = actionButton;
    {
        KnobIPtr parentKnob = actionButton->getParentKnob();
        KnobGroupPtr parentGroup = toKnobGroup(parentKnob);

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
    KnobGroupPtr parentGroup = toKnobGroup(parentKnob);
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
        if (!parentGroup->getValue()) {
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
    // find out if the bezier is already selected.
    bool found = p->knobsTable->isItemSelected(curve);
    if (!found) {
        ///clear previous selection if the SHIFT modifier isn't held
        if (!shiftDown) {
            clearBeziersSelection();
        }
        p->knobsTable->addToSelection(curve, eTableChangeReasonViewer);
    }
}

void
RotoPaintInteract::handleControlPointSelection(const std::pair<BezierCPPtr,
                                                               BezierCPPtr > & p)
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
}

void
RotoPaintInteract::showMenuForControlPoint(const BezierCPPtr& /*cp*/)
{
    KnobChoicePtr menu = rightClickMenuKnob.lock();

    if (!menu) {
        return;
    }
    std::vector<std::string> choices;

    choices.push_back( removeItemsMenuAction.lock()->getName() );
    choices.push_back( smoothItemMenuAction.lock()->getName() );
    choices.push_back( cuspItemMenuAction.lock()->getName() );
    choices.push_back( removeItemFeatherMenuAction.lock()->getName() );
    choices.push_back( nudgeLeftMenuAction.lock()->getName() );
    choices.push_back( nudgeBottomMenuAction.lock()->getName() );
    choices.push_back( nudgeRightMenuAction.lock()->getName() );
    choices.push_back( nudgeTopMenuAction.lock()->getName() );
    menu->populateChoices(choices);
} // showMenuForControlPoint

void
RotoPaintInteract::showMenuForCurve(const BezierPtr & curve)
{
    KnobChoicePtr menu = rightClickMenuKnob.lock();

    if (!menu) {
        return;
    }
    std::vector<std::string> choices;

    choices.push_back( selectAllMenuAction.lock()->getName() );
    choices.push_back( removeItemsMenuAction.lock()->getName() );
    if ( !curve->isOpenBezier() ) {
        choices.push_back( openCloseCurveAction.lock()->getName() );
    }

    choices.push_back( smoothItemMenuAction.lock()->getName() );
    choices.push_back( cuspItemMenuAction.lock()->getName() );
    if ( !curve->isOpenBezier() ) {
        choices.push_back( removeItemFeatherMenuAction.lock()->getName() );
    }
    choices.push_back( lockShapeMenuAction.lock()->getName() );
    menu->populateChoices(choices);
} // showMenuForCurve

void
RotoPaintInteract::onBreakMultiStrokeTriggered()
{
    makeStroke( true, RotoPoint() );
}

static bool
isBranchConnectedToRotoNodeRecursive(const NodePtr& node,
                                     const NodeConstPtr& rotoNode,
                                     int* recursion,
                                     std::list<NodePtr>& markedNodes)
{
    assert(recursion);
    if (!node) {
        return false;
    }
    if (rotoNode == node) {
        return true;
    }
    markedNodes.push_back(node);
    int maxInputs = node->getMaxInputCount();
    *recursion = *recursion + 1;
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr inp = node->getInput(i);
        if (inp) {
            if ( isBranchConnectedToRotoNodeRecursive(inp, rotoNode, recursion, markedNodes) ) {
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
    std::list<ViewerInstancePtr> viewers;

    rotoNode->hasViewersConnected(&viewers);
    for (std::list<ViewerInstancePtr>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        NodePtr viewerNode = (*it)->getNode();
        int maxInputs = viewerNode->getMaxInputCount();
        int hasBranchConnectedToRoto = -1;
        for (int i = 0; i < maxInputs; ++i) {
            NodePtr input = viewerNode->getInput(i);
            if (input) {
                std::list<NodePtr> markedNodes;
                int recursion = 0;
                if ( isBranchConnectedToRotoNodeRecursive(input, rotoNode, &recursion, markedNodes) ) {
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

    if (prepareForLater || !strokeBeingPaint) {
        if ( strokeBeingPaint &&
             ( strokeBeingPaint->getBrushType() == strokeType) &&
             strokeBeingPaint->isEmpty() ) {
            ///We already have a fresh stroke prepared for that type
            return;
        }
        strokeBeingPaint.reset( new RotoStrokeItem( strokeType, p->knobsTable ) );
        strokeBeingPaint->initializeKnobsPublic();
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
    KnobChoicePtr sourceTypeKnob = strokeBeingPaint->getMergeInputAChoiceKnob();
    KnobIntPtr timeOffsetKnob = strokeBeingPaint->getTimeOffsetKnob();
    KnobDoublePtr translateKnob = strokeBeingPaint->getBrushCloneTranslateKnob();
    KnobDoublePtr effectKnob = strokeBeingPaint->getBrushEffectKnob();
    KnobColorPtr colorWheel = colorWheelButton.lock();
    std::vector<double> color(3);
    for (int i = 0; i < 3; ++i) {
        color[i] = colorWheel->getValue(DimIdx(i));
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

    double time = strokeBeingPaint->getApp()->getTimeLine()->currentFrame();
    strokeBeingPaintedTimelineFrame = time;
    if (colorKnob) {
        colorKnob->setValueAcrossDimensions(color, DimIdx(0));
    }
    operatorKnob->setValueFromLabel(Merge::getOperatorString(compOp));
    if (opacityKnob) {
        opacityKnob->setValue(opacity);
    }
    
    sizeKnob->setValue(size);
    hardnessKnob->setValue(hardness);
    pressureOpaKnob->setValue(pressOpa);
    pressureSizeKnob->setValue(pressSize);
    pressureHardnessKnob->setValue(pressHarness);
    buildUpKnob->setValue(buildUp);
    if (effectKnob) {
        effectKnob->setValue(effectValue);
    }
    if (!prepareForLater) {
        KnobIntPtr lifeTimeFrameKnob = strokeBeingPaint->getLifeTimeFrameKnob();
        lifeTimeFrameKnob->setValue( time );
    }
    if ( (strokeType == eRotoStrokeTypeClone) || (strokeType == eRotoStrokeTypeReveal) ) {
        timeOffsetKnob->setValue(timeOffset);
        timeOffsetModeKnob->setValue(timeOffsetMode_i);
        sourceTypeKnob->setValue(sourceType_i);
        std::vector<double> translateValues(2);
        translateValues[0] = -cloneOffset.first;
        translateValues[1] = -cloneOffset.second;
        translateKnob->setValueAcrossDimensions(translateValues);
    }
    if (!prepareForLater) {
        RotoLayerPtr layer = p->publicInterface->getLayerForNewItem();
        p->knobsTable->addItem(strokeBeingPaint, layer, eTableChangeReasonInternal);
        strokeBeingPaint->getApp()->setUserIsPainting(strokeBeingPaint);
        strokeBeingPaint->beginSubStroke();
        strokeBeingPaint->appendPoint(point);
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

std::pair<BezierCPPtr, BezierCPPtr >
RotoPaintInteract::isNearbyFeatherBar(double time,
                                      ViewIdx view,
                                      const std::pair<double, double> & pixelScale,
                                      const QPointF & pos) const
{
    double distFeatherX = 20. * pixelScale.first;
    double acceptance = 10 * pixelScale.second;

    std::list<KnobTableItemPtr> selectedItems = p->knobsTable->getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        BezierPtr isBezier = toBezier(*it);
        RotoStrokeItemPtr isStroke = toRotoStrokeItem(*it);
        if ( !isBezier || isBezier->isOpenBezier() ) {
            continue;
        }

        /*
           For each selected bezier, we compute the extent of the feather bars and check if the mouse would be nearby one of these bars.
           The feather bar of a control point is only displayed is the feather point is equal to the bezier control point.
           In order to give it the  correc direction we use the derivative of the bezier curve at the control point and then use
           the pointInPolygon function to make sure the feather bar is always oriented on the outter part of the polygon.
           The pointInPolygon function needs the polygon of the bezier to test whether the point is inside or outside the polygon
           hence in this loop we compute the polygon for each bezier.
         */

        Transform::Matrix3x3 transform;
        isBezier->getTransformAtTime(time, view, &transform);

        const std::list<BezierCPPtr > & fps = isBezier->getFeatherPoints(view);
        const std::list<BezierCPPtr > & cps = isBezier->getControlPoints(view);
        assert( cps.size() == fps.size() );

        int cpCount = (int)cps.size();
        if (cpCount <= 1) {
            continue;
        }


        std::list<BezierCPPtr >::const_iterator itF = fps.begin();
        std::list<BezierCPPtr >::const_iterator nextF = itF;
        if ( nextF != fps.end() ) {
            ++nextF;
        }
        std::list<BezierCPPtr >::const_iterator prevF = fps.end();
        if ( prevF != fps.begin() ) {
            --prevF;
        }
        bool isClockWiseOriented = isBezier->isFeatherPolygonClockwiseOriented(time, view);

        for (std::list<BezierCPPtr >::const_iterator itCp = cps.begin();
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
            (*itCp)->getPositionAtTime(time,  &controlPoint.x, &controlPoint.y);
            (*itF)->getPositionAtTime(time,  &featherPoint.x, &featherPoint.y);

            controlPoint = Transform::matApply(transform, controlPoint);
            featherPoint = Transform::matApply(transform, featherPoint);
            {
                Point cp, fp;
                cp.x = controlPoint.x;
                cp.y = controlPoint.y;
                fp.x = featherPoint.x;
                fp.y = featherPoint.y;
                Bezier::expandToFeatherDistance(cp, &fp, distFeatherX, time, isClockWiseOriented, transform, prevF, itF, nextF);
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


BezierPtr
RotoPaintInteract::isNearbyBezier(double x,
                                  double y,
                                  double time, ViewIdx view,
                                  double acceptance,
                                  int* index,
                                  double* t,
                                  bool* feather) const
{


    std::list<std::pair<BezierPtr, std::pair<int, double> > > nearbyBeziers;
    std::vector<KnobTableItemPtr> allItems = p->knobsTable->getAllItems();
    for (std::vector<KnobTableItemPtr>::const_iterator it = allItems.begin(); it!=allItems.end(); ++it) {
        BezierPtr b = toBezier(*it);
        if ( b && !b->isLockedRecursive() ) {
            double param;
            int i = b->isPointOnCurve(x, y, acceptance, time, view, &param, feather);
            if (i != -1) {
                nearbyBeziers.push_back( std::make_pair( b, std::make_pair(i, param) ) );
            }
        }
    }

    std::list<KnobTableItemPtr> selectedItems = p->knobsTable->getSelectedItems();

    std::list<std::pair<BezierPtr, std::pair<int, double> > >::iterator firstNotSelected = nearbyBeziers.end();
    for (std::list<std::pair<BezierPtr, std::pair<int, double> > >::iterator it = nearbyBeziers.begin(); it != nearbyBeziers.end(); ++it) {
        bool foundSelected = p->knobsTable->isItemSelected(it->first);
        if (foundSelected) {
            *index = it->second.first;
            *t = it->second.second;

            return it->first;
        } else if ( firstNotSelected == nearbyBeziers.end() ) {
            firstNotSelected = it;
        }
    }
    if ( firstNotSelected != nearbyBeziers.end() ) {
        *index = firstNotSelected->second.first;
        *t = firstNotSelected->second.second;

        return firstNotSelected->first;
    }
    
    return BezierPtr();
} // isNearbyBezier


void
RotoPaintInteract::setSelection(const std::list<RotoDrawableItemPtr > & drawables,
                                const std::list<std::pair<BezierCPPtr, BezierCPPtr > > & points)
{
    p->knobsTable->beginEditSelection();
    p->knobsTable->clearSelection(eTableChangeReasonViewer);
    for (std::list<RotoDrawableItemPtr >::const_iterator it = drawables.begin(); it != drawables.end(); ++it) {
        p->knobsTable->addToSelection(*it, eTableChangeReasonViewer);
    }
    selectedCps.clear();
    for (SelectedCPs::const_iterator it = points.begin(); it != points.end(); ++it) {
        if (it->first && it->second) {
            selectedCps.push_back(*it);
        }
    }
    p->knobsTable->endEditSelection(eTableChangeReasonViewer);
    computeSelectedCpsBBOX();
}

void
RotoPaintInteract::setSelection(const BezierPtr & curve,
                                const std::pair<BezierCPPtr, BezierCPPtr > & point)
{
    p->knobsTable->beginEditSelection();
    p->knobsTable->clearSelection(eTableChangeReasonViewer);

    if (curve) {
        p->knobsTable->addToSelection(curve, eTableChangeReasonViewer);
    }
    selectedCps.clear();
    if (point.first && point.second) {
        selectedCps.push_back(point);
    }
    p->knobsTable->endEditSelection(eTableChangeReasonViewer);
    computeSelectedCpsBBOX();
}

void
RotoPaintInteract::getSelection(std::list<RotoDrawableItemPtr >* beziers,
                                std::list<std::pair<BezierCPPtr, BezierCPPtr > >* points)
{
    std::list<KnobTableItemPtr> selectedItems = p->knobsTable->getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it!=selectedItems.end(); ++it) {
        RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*it);
        if (isDrawable) {
            beziers->push_back(isDrawable);
        }
    }
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
RotoPaintInteract::smoothSelectedCurve(double time, ViewIdx view)
{
    std::pair<double, double> pixelScale;

    p->publicInterface->getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);
    std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;

    if ( !selectedCps.empty() ) {
        for (SelectedCPs::const_iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = it->first->getBezier();
            data.newPoints.push_back(*it);
            datas.push_back(data);
        }
    } else {
        std::list<KnobTableItemPtr> selectedItems = p->knobsTable->getSelectedItems();
        for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
            BezierPtr bezier = toBezier(*it);
            if (bezier) {
                SmoothCuspUndoCommand::SmoothCuspCurveData data;
                data.curve = bezier;
                const std::list<BezierCPPtr > & cps = bezier->getControlPoints(view);
                const std::list<BezierCPPtr > & fps = bezier->getFeatherPoints(view);
                std::list<BezierCPPtr >::const_iterator itFp = fps.begin();
                for (std::list<BezierCPPtr >::const_iterator it = cps.begin(); it != cps.end(); ++it, ++itFp) {
                    data.newPoints.push_back( std::make_pair(*it, *itFp) );
                }
                datas.push_back(data);
            }
        }
    }
    if ( !datas.empty() ) {
        p->publicInterface->pushUndoCommand( new SmoothCuspUndoCommand(shared_from_this(), datas, time, view, false, pixelScale) );

        return true;
    }

    return false;
}

bool
RotoPaintInteract::cuspSelectedCurve(double time, ViewIdx view)
{
    std::pair<double, double> pixelScale;

    p->publicInterface->getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;

    if ( !selectedCps.empty() ) {
        for (SelectedCPs::const_iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = it->first->getBezier();
            data.newPoints.push_back(*it);
            datas.push_back(data);
        }
    } else {
        std::list<KnobTableItemPtr> selectedItems = p->knobsTable->getSelectedItems();
        for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
            BezierPtr bezier = toBezier(*it);
            if (bezier) {
                SmoothCuspUndoCommand::SmoothCuspCurveData data;
                data.curve = bezier;
                const std::list<BezierCPPtr > & cps = bezier->getControlPoints(view);
                const std::list<BezierCPPtr > & fps = bezier->getFeatherPoints(view);
                std::list<BezierCPPtr >::const_iterator itFp = fps.begin();
                for (std::list<BezierCPPtr >::const_iterator it = cps.begin(); it != cps.end(); ++it, ++itFp) {
                    data.newPoints.push_back( std::make_pair(*it, *itFp) );
                }
                datas.push_back(data);
            }
        }
    }
    if ( !datas.empty() ) {
        p->publicInterface->pushUndoCommand( new SmoothCuspUndoCommand(shared_from_this(), datas, time, view, true, pixelScale) );

        return true;
    }

    return false;
}

bool
RotoPaintInteract::removeFeatherForSelectedCurve(ViewIdx view)
{
    std::list<RemoveFeatherUndoCommand::RemoveFeatherData> datas;

    if ( !selectedCps.empty() ) {
        for (SelectedCPs::const_iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
            RemoveFeatherUndoCommand::RemoveFeatherData data;
            data.curve = it->first->getBezier();
            data.newPoints = data.curve->getFeatherPoints(view);
            datas.push_back(data);
        }
    } else {
        std::list<KnobTableItemPtr> selectedItems = p->knobsTable->getSelectedItems();
        for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
            BezierPtr bezier = toBezier(*it);
            if (bezier) {
                RemoveFeatherUndoCommand::RemoveFeatherData data;
                data.curve = bezier;
                data.newPoints = bezier->getFeatherPoints(view);
                datas.push_back(data);
            }
        }
    }
    if ( !datas.empty() ) {
        p->publicInterface->pushUndoCommand( new RemoveFeatherUndoCommand(shared_from_this(), datas, view) );

        return true;
    }

    return false;
}

bool
RotoPaintInteract::lockSelectedCurves()
{
    ///Make a copy because setLocked will change the selection internally and invalidate the iterator
    std::list<KnobTableItemPtr> selectedItems = p->knobsTable->getSelectedItems();
    if ( selectedItems.empty() ) {
        return false;
    }
    for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        KnobHolderPtr t = *it;
        RotoItemPtr item = toRotoItem(t);
        if (!item) {
            continue;
        }
        item->getLockedKnob()->setValue(true);
    }
    clearSelection();

    return true;
}

bool
RotoPaintInteract::moveSelectedCpsWithKeyArrows(int x,
                                                int y,
                                                double time,
                                                ViewIdx view)
{
    std::list< std::pair<BezierCPPtr, BezierCPPtr > > points;

    if ( !selectedCps.empty() ) {
        points = selectedCps;
    } else {
        std::list<KnobTableItemPtr> selectedItems = p->knobsTable->getSelectedItems();
        for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
            BezierPtr bezier = toBezier(*it);
            if (bezier) {
                const std::list< BezierCPPtr > & cps = bezier->getControlPoints(view);
                const std::list< BezierCPPtr > & fps = bezier->getFeatherPoints(view);
                std::list< BezierCPPtr >::const_iterator fpIt = fps.begin();
                assert( fps.empty() || fps.size() == cps.size() );
                for (std::list< BezierCPPtr >::const_iterator it = cps.begin(); it != cps.end(); ++it) {
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
        p->publicInterface->pushUndoCommand( new MoveControlPointsUndoCommand(shared_from_this(), points, (double)x * pixelScale.first,
                                                                              (double)y * pixelScale.second, time, view) );
        computeSelectedCpsBBOX();

        return true;
    }

    return false;
}

void
RotoPaintInteract::removeCurve(const RotoDrawableItemPtr& curve)
{
    if (curve == builtBezier) {
        builtBezier.reset();
    } else if (curve == strokeBeingPaint) {
        strokeBeingPaint.reset();
    }
    p->knobsTable->removeItem(curve, eTableChangeReasonViewer);
}

bool
RotoPaint::hasOverlay() const
{
    return _imp->nodeType == eRotoPaintTypeRoto || _imp->nodeType == eRotoPaintTypeRotoPaint;
}

void
RotoPaint::drawOverlay(double time,
                       const RenderScale & /*renderScale*/,
                       ViewIdx view)
{
    
    std::list< RotoDrawableItemPtr > drawables = _imp->knobsTable->getRotoItemsByRenderOrder(time, view);
    std::pair<double, double> pixelScale;
    std::pair<double, double> viewportSize;

    getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);
    getCurrentViewportForOverlays()->getViewportSize(viewportSize.first, viewportSize.second);

    bool featherVisible = _imp->ui->isFeatherVisible();

    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::Enable(GL_LINE_SMOOTH);
        GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        GL_GPU::LineWidth(1.5);


        double cpWidth = kControlPointMidSize * 2;
        GL_GPU::PointSize(cpWidth);
        for (std::list< RotoDrawableItemPtr >::const_iterator it = drawables.begin(); it != drawables.end(); ++it) {

            if ( !(*it)->isGloballyActivated() ) {
                continue;
            }


            BezierPtr isBezier = toBezier(*it);
            RotoStrokeItemPtr isStroke = toRotoStrokeItem(*it);
            if (isStroke) {
                if (_imp->ui->selectedTool != eRotoToolSelectAll) {
                    continue;
                }

                bool selected = _imp->knobsTable->isItemSelected(isStroke);
                if (!selected) {
                    continue;
                }

                std::list<std::list<std::pair<Point, double> > > strokes;
                isStroke->evaluateStroke(0, time, view, &strokes);
                bool locked = (*it)->isLockedRecursive();
                double curveColor[4];
                if (!locked) {
                    KnobColorPtr overlayColorKnob = (*it)->getOverlayColorKnob();
                    for (int i = 0; i < 3; ++i) {
                        curveColor[i] = overlayColorKnob->getValue(DimIdx(i));
                    }
                } else {
                    curveColor[0] = 0.8; curveColor[1] = 0.8; curveColor[2] = 0.8; curveColor[3] = 1.;
                }
                GL_GPU::Color4dv(curveColor);

                for (std::list<std::list<std::pair<Point, double> > >::iterator itStroke = strokes.begin(); itStroke != strokes.end(); ++itStroke) {
                    GL_GPU::Begin(GL_LINE_STRIP);
                    for (std::list<std::pair<Point, double> >::const_iterator it2 = itStroke->begin(); it2 != itStroke->end(); ++it2) {
                        GL_GPU::Vertex2f(it2->first.x, it2->first.y);
                    }
                    GL_GPU::End();
                }
            } else if (isBezier) {
                ///draw the bezier
                // check if the bbox is visible
                // if the bbox is visible, compute the polygon and draw it.


                RectD bbox = isBezier->getBoundingBox(time, view);
                if ( !getCurrentViewportForOverlays()->isVisibleInViewport(bbox) ) {
                    continue;
                }

                std::vector< ParametricPoint > points;
                isBezier->evaluateAtTime_DeCasteljau(time, view, 0,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                     100,
#else
                                                     1,
#endif
                                                     &points, NULL);

                bool locked = (*it)->isLockedRecursive();
                double curveColor[4];
                if (!locked) {
                    KnobColorPtr overlayColorKnob = (*it)->getOverlayColorKnob();
                    for (int i = 0; i < 3; ++i) {
                        curveColor[i] = overlayColorKnob->getValue(DimIdx(i));
                    }
                    curveColor[3] = 1.;
                } else {
                    curveColor[0] = 0.8; curveColor[1] = 0.8; curveColor[2] = 0.8; curveColor[3] = 1.;
                }
                GL_GPU::Color4dv(curveColor);

                GL_GPU::Begin(GL_LINE_STRIP);
                for (std::vector<ParametricPoint >::const_iterator it2 = points.begin(); it2 != points.end(); ++it2) {
                    GL_GPU::Vertex2f(it2->x, it2->y);
                }
                GL_GPU::End();

                ///draw the feather points
                std::vector< ParametricPoint > featherPoints;
                RectD featherBBox( std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::infinity(),
                                  -std::numeric_limits<double>::infinity(),
                                  -std::numeric_limits<double>::infinity() );
                bool clockWise = isBezier->isFeatherPolygonClockwiseOriented(time, view);


                if (featherVisible) {
                    ///Draw feather only if visible (button is toggled in the user interface)
                    isBezier->evaluateFeatherPointsAtTime_DeCasteljau(time, view, 0,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                                      100,
#else
                                                                      1,
#endif
                                                                      true, &featherPoints, &featherBBox);

                    if ( !featherPoints.empty() ) {
                        GL_GPU::LineStipple(2, 0xAAAA);
                        GL_GPU::Enable(GL_LINE_STIPPLE);
                        GL_GPU::Begin(GL_LINE_STRIP);
                        for (std::vector<ParametricPoint >::const_iterator it2 = featherPoints.begin(); it2 != featherPoints.end(); ++it2) {
                            GL_GPU::Vertex2f(it2->x, it2->y);
                        }
                        GL_GPU::End();
                        GL_GPU::Disable(GL_LINE_STIPPLE);
                    }
                }

                ///draw the control points if the bezier is selected
                bool selected = _imp->knobsTable->isItemSelected(isBezier);
                if (selected && !locked) {
                    Transform::Matrix3x3 transform;
                    isBezier->getTransformAtTime(time, view, &transform);

                    const std::list< BezierCPPtr > & cps = isBezier->getControlPoints(view);
                    const std::list< BezierCPPtr > & featherPts = isBezier->getFeatherPoints(view);
                    assert( cps.size() == featherPts.size() );

                    if ( cps.empty() ) {
                        continue;
                    }


                    GL_GPU::Color3d(0.85, 0.67, 0.);

                    std::list< BezierCPPtr >::const_iterator itF = featherPts.begin();
                    int index = 0;
                    std::list< BezierCPPtr >::const_iterator prevCp = cps.end();
                    if ( prevCp != cps.begin() ) {
                        --prevCp;
                    }
                    std::list< BezierCPPtr >::const_iterator nextCp = cps.begin();
                    if ( nextCp != cps.end() ) {
                        ++nextCp;
                    }
                    for (std::list< BezierCPPtr >::const_iterator it2 = cps.begin(); it2 != cps.end();
                         ++it2) {
                        if ( nextCp == cps.end() ) {
                            nextCp = cps.begin();
                        }
                        if ( prevCp == cps.end() ) {
                            prevCp = cps.begin();
                        }
                        assert( itF != featherPts.end() ); // because cps.size() == featherPts.size()
                        if ( itF == featherPts.end() ) {
                            break;
                        }
                        double x, y;
                        Transform::Point3D p, pF;
                        (*it2)->getPositionAtTime(time, &p.x, &p.y);
                        p.z = 1.;

                        double xF, yF;
                        (*itF)->getPositionAtTime(time, &pF.x, &pF.y);
                        pF.z = 1.;

                        p = Transform::matApply(transform, p);
                        pF = Transform::matApply(transform, pF);

                        x = p.x;
                        y = p.y;
                        xF = pF.x;
                        yF = pF.y;

                        ///draw the feather point only if it is distinct from the associated point
                        bool drawFeather = featherVisible;
                        if (featherVisible) {
                            drawFeather = !(*it2)->equalsAtTime(time, **itF);
                        }


                        ///if the control point is the only control point being dragged, color it to identify it to the user
                        bool colorChanged = false;
                        SelectedCPs::const_iterator firstSelectedCP = _imp->ui->selectedCps.begin();
                        if ( ( firstSelectedCP != _imp->ui->selectedCps.end() ) &&
                            ( ( firstSelectedCP->first == *it2) || ( firstSelectedCP->second == *it2) ) &&
                            ( _imp->ui->selectedCps.size() == 1) &&
                            ( ( _imp->ui->state == eEventStateDraggingSelectedControlPoints) || ( _imp->ui->state == eEventStateDraggingControlPoint) ) ) {
                            GL_GPU::Color3f(0., 1., 1.);
                            colorChanged = true;
                        }

                        for (SelectedCPs::const_iterator cpIt = _imp->ui->selectedCps.begin();
                             cpIt != _imp->ui->selectedCps.end();
                             ++cpIt) {
                            ///if the control point is selected, draw its tangent handles
                            if (cpIt->first == *it2) {
                                _imp->ui->drawSelectedCp(time, cpIt->first, x, y, transform);
                                if (drawFeather) {
                                    _imp->ui->drawSelectedCp(time, cpIt->second, xF, yF, transform);
                                }
                                GL_GPU::Color3f(0.2, 1., 0.);
                                colorChanged = true;
                                break;
                            } else if (cpIt->second == *it2) {
                                _imp->ui->drawSelectedCp(time, cpIt->second, x, y, transform);
                                if (drawFeather) {
                                    _imp->ui->drawSelectedCp(time, cpIt->first, xF, yF, transform);
                                }
                                GL_GPU::Color3f(0.2, 1., 0.);
                                colorChanged = true;
                                break;
                            }
                        } // for(cpIt)

                        GL_GPU::Begin(GL_POINTS);
                        GL_GPU::Vertex2f(x,y);
                        GL_GPU::End();

                        if (colorChanged) {
                            GL_GPU::Color3d(0.85, 0.67, 0.);
                        }

                        if ( (firstSelectedCP->first == *itF)
                            && ( _imp->ui->selectedCps.size() == 1) &&
                            ( ( _imp->ui->state == eEventStateDraggingSelectedControlPoints) || ( _imp->ui->state == eEventStateDraggingControlPoint) )
                            && !colorChanged ) {
                            GL_GPU::Color3f(0.2, 1., 0.);
                            colorChanged = true;
                        }


                        double distFeatherX = 20. * pixelScale.first;
                        double distFeatherY = 20. * pixelScale.second;
                        bool isHovered = false;
                        if (_imp->ui->featherBarBeingHovered.first) {
                            assert(_imp->ui->featherBarBeingHovered.second);
                            if ( _imp->ui->featherBarBeingHovered.first->isFeatherPoint() ) {
                                isHovered = _imp->ui->featherBarBeingHovered.first == *itF;
                            } else if ( _imp->ui->featherBarBeingHovered.second->isFeatherPoint() ) {
                                isHovered = _imp->ui->featherBarBeingHovered.second == *itF;
                            }
                        }

                        if (drawFeather) {
                            GL_GPU::Begin(GL_POINTS);
                            GL_GPU::Vertex2f(xF, yF);
                            GL_GPU::End();


                            if ( ( (_imp->ui->state == eEventStateDraggingFeatherBar) &&
                                  ( ( *itF == _imp->ui->featherBarBeingDragged.first) || ( *itF == _imp->ui->featherBarBeingDragged.second) ) ) ||
                                isHovered ) {
                                GL_GPU::Color3f(0.2, 1., 0.);
                                colorChanged = true;
                            } else {
                                GL_GPU::Color4dv(curveColor);
                            }

                            double beyondX, beyondY;
                            double dx = (xF - x);
                            double dy = (yF - y);
                            double dist = sqrt(dx * dx + dy * dy);
                            beyondX = ( dx * (dist + distFeatherX) ) / dist + x;
                            beyondY = ( dy * (dist + distFeatherY) ) / dist + y;

                            ///draw a link between the feather point and the control point.
                            ///Also extend that link of 20 pixels beyond the feather point.

                            GL_GPU::Begin(GL_LINE_STRIP);
                            GL_GPU::Vertex2f(x, y);
                            GL_GPU::Vertex2f(xF, yF);
                            GL_GPU::Vertex2f(beyondX, beyondY);
                            GL_GPU::End();

                            GL_GPU::Color3d(0.85, 0.67, 0.);
                        } else if (featherVisible) {
                            ///if the feather point is identical to the control point
                            ///draw a small hint line that the user can drag to move the feather point
                            if ( !isBezier->isOpenBezier() && ( (_imp->ui->selectedTool == eRotoToolSelectAll) || (_imp->ui->selectedTool == eRotoToolSelectFeatherPoints) ) ) {
                                int cpCount = (*it2)->getBezier()->getControlPointsCount(view);
                                if (cpCount > 1) {
                                    Point controlPoint;
                                    controlPoint.x = x;
                                    controlPoint.y = y;
                                    Point featherPoint;
                                    featherPoint.x = xF;
                                    featherPoint.y = yF;


                                    Bezier::expandToFeatherDistance(controlPoint, &featherPoint, distFeatherX, time, clockWise, transform, prevCp, it2, nextCp);

                                    if ( ( (_imp->ui->state == eEventStateDraggingFeatherBar) &&
                                          ( ( *itF == _imp->ui->featherBarBeingDragged.first) ||
                                           ( *itF == _imp->ui->featherBarBeingDragged.second) ) ) || isHovered ) {
                                              GL_GPU::Color3f(0.2, 1., 0.);
                                              colorChanged = true;
                                          } else {
                                              GL_GPU::Color4dv(curveColor);
                                          }

                                    GL_GPU::Begin(GL_LINES);
                                    GL_GPU::Vertex2f(x, y);
                                    GL_GPU::Vertex2f(featherPoint.x, featherPoint.y);
                                    GL_GPU::End();

                                    GL_GPU::Color3d(0.85, 0.67, 0.);
                                }
                            }
                        } // isFeatherVisible()


                        if (colorChanged) {
                            GL_GPU::Color3d(0.85, 0.67, 0.);
                        }

                        // increment for next iteration
                        if ( itF != featherPts.end() ) {
                            ++itF;
                        }
                        if ( nextCp != cps.end() ) {
                            ++nextCp;
                        }
                        if ( prevCp != cps.end() ) {
                            ++prevCp;
                        }
                        ++index;
                    } // for(it2)
                } // if ( ( selected != _imp->ui->selectedBeziers.end() ) && !locked ) {
            } // if (isBezier)
            glCheckError(GL_GPU);
        } // for (std::list< RotoDrawableItemPtr >::const_iterator it = drawables.begin(); it != drawables.end(); ++it) {



        if ( _imp->nodeType == RotoPaint::eRotoPaintTypeRotoPaint &&
            ( ( _imp->ui->selectedRole == eRotoRoleMergeBrush) ||
             ( _imp->ui->selectedRole == eRotoRolePaintBrush) ||
             ( _imp->ui->selectedRole == eRotoRoleEffectBrush) ||
             ( _imp->ui->selectedRole == eRotoRoleCloneBrush) ) &&
            ( _imp->ui->selectedTool != eRotoToolOpenBezier) ) {
            Point cursorPos;
            getCurrentViewportForOverlays()->getCursorPosition(cursorPos.x, cursorPos.y);
            RectD viewportRect = getCurrentViewportForOverlays()->getViewportRect();

            if ( viewportRect.contains(cursorPos.x, cursorPos.y) ) {
                //Draw a circle  around the cursor
                double brushSize = _imp->ui->sizeSpinbox.lock()->getValue();
                GLdouble projection[16];
                GL_GPU::GetDoublev( GL_PROJECTION_MATRIX, projection);
                Point shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
                shadow.x = 2. / (projection[0] * viewportSize.first);
                shadow.y = 2. / (projection[5] * viewportSize.second);

                double halfBrush = brushSize / 2.;
                QPointF ellipsePos;
                if ( (_imp->ui->state == eEventStateDraggingBrushSize) || (_imp->ui->state == eEventStateDraggingBrushOpacity) ) {
                    ellipsePos = _imp->ui->mouseCenterOnSizeChange;
                } else {
                    ellipsePos = _imp->ui->lastMousePos;
                }
                double opacity = _imp->ui->opacitySpinbox.lock()->getValue();

                for (int l = 0; l < 2; ++l) {
                    GL_GPU::MatrixMode(GL_PROJECTION);
                    int direction = (l == 0) ? 1 : -1;
                    // translate (1,-1) pixels
                    GL_GPU::Translated(direction * shadow.x, -direction * shadow.y, 0);
                    GL_GPU::MatrixMode(GL_MODELVIEW);
                    _imp->ui->drawEllipse(ellipsePos.x(), ellipsePos.y(), halfBrush, halfBrush, l, 1.f, 1.f, 1.f, opacity);

                    GL_GPU::Color3f(.5f * l * opacity, .5f * l * opacity, .5f * l * opacity);


                    if ( ( (_imp->ui->selectedTool == eRotoToolClone) || (_imp->ui->selectedTool == eRotoToolReveal) ) &&
                        ( ( _imp->ui->cloneOffset.first != 0) || ( _imp->ui->cloneOffset.second != 0) ) ) {
                        GL_GPU::Begin(GL_LINES);

                        if (_imp->ui->state == eEventStateDraggingCloneOffset) {
                            //draw a line between the center of the 2 ellipses
                            GL_GPU::Vertex2d( ellipsePos.x(), ellipsePos.y() );
                            GL_GPU::Vertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first, ellipsePos.y() + _imp->ui->cloneOffset.second);
                        }
                        //draw a cross in the center of the source ellipse
                        GL_GPU::Vertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first, ellipsePos.y()  + _imp->ui->cloneOffset.second - halfBrush);
                        GL_GPU::Vertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first, ellipsePos.y() +  _imp->ui->cloneOffset.second + halfBrush);
                        GL_GPU::Vertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first - halfBrush, ellipsePos.y()  + _imp->ui->cloneOffset.second);
                        GL_GPU::Vertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first + halfBrush, ellipsePos.y()  + _imp->ui->cloneOffset.second);
                        GL_GPU::End();
                        
                        
                        //draw the source ellipse
                        _imp->ui->drawEllipse(ellipsePos.x() + _imp->ui->cloneOffset.first, ellipsePos.y() + _imp->ui->cloneOffset.second, halfBrush, halfBrush, l, 1.f, 1.f, 1.f, opacity / 2.);
                    }
                }
            }
        }
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);
    
    if (_imp->ui->showCpsBbox) {
        _imp->ui->drawSelectedCpsBBOX();
    }
    glCheckError(GL_GPU);
} // drawOverlay


void
RotoPaint::onInteractViewportSelectionCleared()
{
    if (!_imp->ui->isStickySelectionEnabled()  && !_imp->ui->shiftDown) {
        _imp->ui->clearSelection();
    }

    if ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) {
        if ( ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) && _imp->ui->builtBezier && !_imp->ui->builtBezier->isCurveFinished(ViewIdx(0)) ) {
            pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, _imp->ui->builtBezier, ViewIdx(0)) );

            _imp->ui->builtBezier.reset();
            _imp->ui->selectedCps.clear();
            _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
        }
    }
}

void
RotoPaint::onInteractViewportSelectionUpdated(const RectD& rectangle,
                                              bool onRelease)
{
    if ( !onRelease || !getNode()->isSettingsPanelVisible() ) {
        return;
    }

    std::list<KnobTableItemPtr> selectedItems = _imp->knobsTable->getSelectedItems();

    _imp->knobsTable->beginEditSelection();
    bool stickySel = _imp->ui->isStickySelectionEnabled();
    if (!stickySel && !_imp->ui->shiftDown) {
        _imp->ui->clearCPSSelection();
        _imp->knobsTable->clearSelection(eTableChangeReasonViewer);
    }

    int selectionMode = -1;
    if (_imp->ui->selectedTool == eRotoToolSelectAll) {
        selectionMode = 0;
    } else if (_imp->ui->selectedTool == eRotoToolSelectPoints) {
        selectionMode = 1;
    } else if ( (_imp->ui->selectedTool == eRotoToolSelectFeatherPoints) || (_imp->ui->selectedTool == eRotoToolSelectCurves) ) {
        selectionMode = 2;
    }

    double time = getCurrentTime();
    ViewIdx view = getCurrentView();

    bool featherVisible = _imp->ui->isFeatherVisible();
    std::list<RotoDrawableItemPtr > curves = _imp->knobsTable->getRotoItemsByRenderOrder(time, view);
    for (std::list<RotoDrawableItemPtr >::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        BezierPtr isBezier = toBezier(*it);
        if ( (*it)->isLockedRecursive() ) {
            continue;
        }

        if (isBezier) {
            SelectedCPs points  = isBezier->controlPointsWithinRect(time, view, rectangle.x1, rectangle.x2, rectangle.y1, rectangle.y2, 0, selectionMode);
            if (_imp->ui->selectedTool != eRotoToolSelectCurves) {
                for (SelectedCPs::iterator ptIt = points.begin(); ptIt != points.end(); ++ptIt) {
                    if ( !featherVisible && ptIt->first->isFeatherPoint() ) {
                        continue;
                    }
                    SelectedCPs::iterator foundCP = std::find(_imp->ui->selectedCps.begin(), _imp->ui->selectedCps.end(), *ptIt);
                    if ( foundCP == _imp->ui->selectedCps.end() ) {
                        if (!_imp->ui->shiftDown || !_imp->ui->ctrlDown) {
                            _imp->ui->selectedCps.push_back(*ptIt);
                        }
                    } else {
                        if (_imp->ui->shiftDown && _imp->ui->ctrlDown) {
                            _imp->ui->selectedCps.erase(foundCP);
                        }
                    }
                }
            }
            if ( !points.empty() ) {
                _imp->knobsTable->addToSelection(isBezier, eTableChangeReasonViewer);
            }
        }
    }

    if ( !selectedItems.empty() ) {
        _imp->knobsTable->addToSelection(selectedItems, eTableChangeReasonViewer);
    } else if (!stickySel && !_imp->ui->shiftDown) {
        _imp->knobsTable->clearSelection(eTableChangeReasonViewer);
    }
    _imp->knobsTable->endEditSelection(eTableChangeReasonViewer);
    _imp->ui->computeSelectedCpsBBOX();
} // RotoPaint::onInteractViewportSelectionUpdated

bool
RotoPaint::onOverlayPenDoubleClicked(double time,
                                     const RenderScale & /*renderScale*/,
                                     ViewIdx view,
                                     const QPointF & /*viewportPos*/,
                                     const QPointF & pos)
{
    bool didSomething = false;
    std::pair<double, double> pixelScale;

    getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    if (_imp->ui->selectedTool == eRotoToolSelectAll) {
        double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
        double nearbyBezierT;
        int nearbyBezierCPIndex;
        bool isFeather;
        BezierPtr nearbyBezier =
        _imp->ui->isNearbyBezier(pos.x(), pos.y(), time, view, bezierSelectionTolerance, &nearbyBezierCPIndex, &nearbyBezierT, &isFeather);

        if (nearbyBezier) {
            ///If the bezier is already selected and we re-click on it, change the transform mode
            _imp->ui->handleBezierSelection(nearbyBezier);
            _imp->ui->clearCPSSelection();
            const std::list<BezierCPPtr > & cps = nearbyBezier->getControlPoints(view);
            const std::list<BezierCPPtr > & fps = nearbyBezier->getFeatherPoints(view);
            assert( cps.size() == fps.size() );
            std::list<BezierCPPtr >::const_iterator itCp = cps.begin();
            std::list<BezierCPPtr >::const_iterator itFp = fps.begin();
            for (; itCp != cps.end(); ++itCp, ++itFp) {
                _imp->ui->selectedCps.push_back( std::make_pair(*itCp, *itFp) );
            }
            if (_imp->ui->selectedCps.size() > 1) {
                _imp->ui->computeSelectedCpsBBOX();
            }
            didSomething = true;
        }
    }

    return didSomething;
} // onOverlayPenDoubleClicked

bool
RotoPaint::onOverlayPenDown(double time,
                            const RenderScale & /*renderScale*/,
                            ViewIdx view,
                            const QPointF & /*viewportPos*/,
                            const QPointF & pos,
                            double pressure,
                            double timestamp,
                            PenType pen)
{
    if (isDoingNeatRender()) {
        return true;
    }
    NodePtr node = getNode();

    std::pair<double, double> pixelScale;
    getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    bool didSomething = false;
    double tangentSelectionTol = kTangentHandleSelectionTolerance * pixelScale.first;
    double cpSelectionTolerance = kControlPointSelectionTolerance * pixelScale.first;

    _imp->ui->lastTabletDownTriggeredEraser = false;
    if ( _imp->nodeType == RotoPaint::eRotoPaintTypeRotoPaint && ( (pen == ePenTypeEraser) || (pen == ePenTypePen) || (pen == ePenTypeCursor) ) ) {
        if ( (pen == ePenTypeEraser) && (_imp->ui->selectedTool != eRotoToolEraserBrush) ) {
            _imp->ui->setCurrentTool( _imp->ui->eraserAction.lock() );
            _imp->ui->lastTabletDownTriggeredEraser = true;
        }
    }

    const bool featherVisible = _imp->ui->isFeatherVisible();

    //////////////////BEZIER SELECTION
    /////Check if the point is nearby a bezier
    ///tolerance for bezier selection
    double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
    double nearbyBezierT;
    int nearbyBezierCPIndex;
    bool isFeather;
    BezierPtr nearbyBezier =
    _imp->ui->isNearbyBezier(pos.x(), pos.y(), time, view, bezierSelectionTolerance, &nearbyBezierCPIndex, &nearbyBezierT, &isFeather);
    std::pair<BezierCPPtr, BezierCPPtr > nearbyCP;
    int nearbyCpIndex = -1;
    if (nearbyBezier) {
        /////////////////CONTROL POINT SELECTION
        //////Check if the point is nearby a control point of a selected bezier
        ///Find out if the user selected a control point

        Bezier::ControlPointSelectionPrefEnum pref = Bezier::eControlPointSelectionPrefWhateverFirst;
        if ( (_imp->ui->selectedTool == eRotoToolSelectFeatherPoints) && featherVisible ) {
            pref = Bezier::eControlPointSelectionPrefFeatherFirst;
        }

        nearbyCP = nearbyBezier->isNearbyControlPoint(pos.x(), pos.y(), cpSelectionTolerance, time, view, pref, &nearbyCpIndex);
    }

    ////////////////// TANGENT SELECTION
    ///in all cases except cusp/smooth if a control point is selected, check if the user clicked on a tangent handle
    ///in which case we go into eEventStateDraggingTangent mode
    if ( !nearbyCP.first &&
        ( _imp->ui->selectedTool != eRotoToolCuspPoints) &&
        ( _imp->ui->selectedTool != eRotoToolSmoothPoints) &&
        ( _imp->ui->selectedTool != eRotoToolSelectCurves) ) {
        for (SelectedCPs::iterator it = _imp->ui->selectedCps.begin(); it != _imp->ui->selectedCps.end(); ++it) {
            if ( (_imp->ui->selectedTool == eRotoToolSelectAll) ||
                ( _imp->ui->selectedTool == eRotoToolDrawBezier) ) {
                int ret = it->first->isNearbyTangent(time, ViewIdx(0), pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->ui->tangentBeingDragged = it->first;
                    _imp->ui->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                } else {
                    ///try with the counter part point
                    if (it->second) {
                        ret = it->second->isNearbyTangent(time, ViewIdx(0), pos.x(), pos.y(), tangentSelectionTol);
                    }
                    if (ret >= 0) {
                        _imp->ui->tangentBeingDragged = it->second;
                        _imp->ui->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                        didSomething = true;
                    }
                }
            } else if (_imp->ui->selectedTool == eRotoToolSelectFeatherPoints) {
                const BezierCPPtr & fp = it->first->isFeatherPoint() ? it->first : it->second;
                int ret = fp->isNearbyTangent(time, ViewIdx(0), pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->ui->tangentBeingDragged = fp;
                    _imp->ui->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                }
            } else if (_imp->ui->selectedTool == eRotoToolSelectPoints) {
                const BezierCPPtr & cp = it->first->isFeatherPoint() ? it->second : it->first;
                int ret = cp->isNearbyTangent(time,  ViewIdx(0), pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->ui->tangentBeingDragged = cp;
                    _imp->ui->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                }
            }

            ///check in case this is a feather tangent
            if (_imp->ui->tangentBeingDragged && _imp->ui->tangentBeingDragged->isFeatherPoint() && !featherVisible) {
                _imp->ui->tangentBeingDragged.reset();
                _imp->ui->state = eEventStateNone;
                didSomething = false;
            }

            if (didSomething) {
                return didSomething;
            }
        }
    }


    switch (_imp->ui->selectedTool) {
        case eRotoToolSelectAll:
        case eRotoToolSelectPoints:
        case eRotoToolSelectFeatherPoints: {
            if ( ( _imp->ui->selectedTool == eRotoToolSelectFeatherPoints) && !featherVisible ) {
                ///nothing to do
                break;
            }
            std::pair<BezierCPPtr, BezierCPPtr > featherBarSel;
            if ( ( ( _imp->ui->selectedTool == eRotoToolSelectAll) || ( _imp->ui->selectedTool == eRotoToolSelectFeatherPoints) ) ) {
                featherBarSel = _imp->ui->isNearbyFeatherBar(time, view,  pixelScale, pos);
                if (featherBarSel.first && !featherVisible) {
                    featherBarSel.first.reset();
                    featherBarSel.second.reset();
                }
            }


            if (nearbyBezier) {
                ///check if the user clicked nearby the cross hair of the selection rectangle in which case
                ///we drag all the control points selected
                if (nearbyCP.first) {
                    _imp->ui->handleControlPointSelection(nearbyCP);
                    _imp->ui->handleBezierSelection(nearbyBezier);
                    if (pen == ePenTypeRMB) {
                        _imp->ui->state = eEventStateNone;
                        _imp->ui->showMenuForControlPoint(nearbyCP.first);
                    }
                    didSomething = true;
                } else if (featherBarSel.first) {
                    _imp->ui->clearCPSSelection();
                    _imp->ui->featherBarBeingDragged = featherBarSel;

                    ///Also select the point only if the curve is the same!
                    if (featherBarSel.first->getBezier() == nearbyBezier) {
                        _imp->ui->handleControlPointSelection(_imp->ui->featherBarBeingDragged);
                        _imp->ui->handleBezierSelection(nearbyBezier);
                    }
                    _imp->ui->state = eEventStateDraggingFeatherBar;
                    didSomething = true;
                } else {
                    bool found = _imp->knobsTable->isItemSelected(nearbyBezier);
                    if (!found) {
                        _imp->ui->handleBezierSelection(nearbyBezier);
                    }
                    if (pen == ePenTypeLMB || pen == ePenTypeMMB) {
                        if (_imp->ui->ctrlDown && _imp->ui->altDown && !_imp->ui->shiftDown) {
                            pushUndoCommand( new AddPointUndoCommand(_imp->ui, nearbyBezier, nearbyBezierCPIndex, nearbyBezierT, view) );
                            _imp->ui->evaluateOnPenUp = true;
                        } else {
                            _imp->ui->state = eEventStateDraggingSelectedControlPoints;
                            _imp->ui->bezierBeingDragged = nearbyBezier;
                        }
                    } else if (pen == ePenTypeRMB) {
                        _imp->ui->showMenuForCurve(nearbyBezier);
                    }
                    didSomething = true;
                }
            } else {
                if (featherBarSel.first) {
                    _imp->ui->clearCPSSelection();
                    _imp->ui->featherBarBeingDragged = featherBarSel;
                    _imp->ui->handleControlPointSelection(_imp->ui->featherBarBeingDragged);
                    _imp->ui->state = eEventStateDraggingFeatherBar;
                    didSomething = true;
                }
                if (_imp->ui->state == eEventStateNone) {
                    _imp->ui->state = _imp->ui->isMouseInteractingWithCPSBbox(pos, cpSelectionTolerance, pixelScale);
                    if (_imp->ui->state != eEventStateNone) {
                        didSomething = true;
                    }
                }
            }
            break;
        }
        case eRotoToolSelectCurves:

            if (nearbyBezier) {
                ///If the bezier is already selected and we re-click on it, change the transform mode
                bool found = _imp->knobsTable->isItemSelected(nearbyBezier);
                if (!found) {
                    _imp->ui->handleBezierSelection(nearbyBezier);
                }
                if (pen == ePenTypeRMB) {
                    _imp->ui->showMenuForCurve(nearbyBezier);
                } else {
                    if (_imp->ui->ctrlDown && _imp->ui->altDown && !_imp->ui->shiftDown) {
                        pushUndoCommand( new AddPointUndoCommand(_imp->ui, nearbyBezier, nearbyBezierCPIndex, nearbyBezierT, view) );
                        _imp->ui->evaluateOnPenUp = true;
                    }
                }
                didSomething = true;
            } else {
                if (_imp->ui->state == eEventStateNone) {
                    _imp->ui->state = _imp->ui->isMouseInteractingWithCPSBbox(pos, cpSelectionTolerance, pixelScale);
                    if (_imp->ui->state != eEventStateNone) {
                        didSomething = true;
                    }
                }
            }
            break;
        case eRotoToolAddPoints:
            ///If the user clicked on a bezier and this bezier is selected add a control point by
            ///splitting up the targeted segment
            if (nearbyBezier) {
                bool found = _imp->knobsTable->isItemSelected(nearbyBezier);
                if (found) {
                    ///check that the point is not too close to an existing point
                    if (nearbyCP.first) {
                        _imp->ui->handleControlPointSelection(nearbyCP);
                    } else {
                        pushUndoCommand( new AddPointUndoCommand(_imp->ui, nearbyBezier, nearbyBezierCPIndex, nearbyBezierT, view) );
                        _imp->ui->evaluateOnPenUp = true;
                    }
                    didSomething = true;
                }
            }
            break;
        case eRotoToolRemovePoints:
            if (nearbyCP.first) {
                assert( nearbyBezier && nearbyBezier == nearbyCP.first->getBezier() );
                if ( nearbyCP.first->isFeatherPoint() ) {
                    pushUndoCommand( new RemovePointUndoCommand(_imp->ui, nearbyBezier, nearbyCP.second, view) );
                } else {
                    pushUndoCommand( new RemovePointUndoCommand(_imp->ui, nearbyBezier, nearbyCP.first, view) );
                }
                didSomething = true;
            }
            break;
        case eRotoToolRemoveFeatherPoints:
            if (nearbyCP.first) {
                assert(nearbyBezier);
                std::list<RemoveFeatherUndoCommand::RemoveFeatherData> datas;
                RemoveFeatherUndoCommand::RemoveFeatherData data;
                data.curve = nearbyBezier;
                data.newPoints.push_back(nearbyCP.first->isFeatherPoint() ? nearbyCP.first : nearbyCP.second);
                datas.push_back(data);
                pushUndoCommand( new RemoveFeatherUndoCommand(_imp->ui, datas, view) );
                didSomething = true;
            }
            break;
        case eRotoToolOpenCloseCurve:
            if (nearbyBezier) {
                pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, nearbyBezier, view) );
                didSomething = true;
            }
            break;
        case eRotoToolSmoothPoints:

            if (nearbyCP.first) {
                std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
                SmoothCuspUndoCommand::SmoothCuspCurveData data;
                data.curve = nearbyBezier;
                data.newPoints.push_back(nearbyCP);
                datas.push_back(data);
                pushUndoCommand( new SmoothCuspUndoCommand(_imp->ui, datas, time, view, false, pixelScale) );
                didSomething = true;
            }
            break;
        case eRotoToolCuspPoints:
            if ( nearbyCP.first && _imp->ui->autoKeyingEnabledButton.lock()->getValue() ) {
                std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
                SmoothCuspUndoCommand::SmoothCuspCurveData data;
                data.curve = nearbyBezier;
                data.newPoints.push_back(nearbyCP);
                datas.push_back(data);
                pushUndoCommand( new SmoothCuspUndoCommand(_imp->ui, datas, time, view, true, pixelScale) );
                didSomething = true;
            }
            break;
        case eRotoToolDrawBezier:
        case eRotoToolOpenBezier: {
            if ( _imp->ui->builtBezier && _imp->ui->builtBezier->isCurveFinished(view) ) {
                _imp->ui->builtBezier.reset();
                _imp->ui->clearSelection();
                _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );

                return true;
            }
            if (_imp->ui->builtBezier) {
                ///if the user clicked on a control point of the bezier, select the point instead.
                ///if that point is the starting point of the curve, close the curve
                const std::list<BezierCPPtr > & cps = _imp->ui->builtBezier->getControlPoints(view);
                int i = 0;
                for (std::list<BezierCPPtr >::const_iterator it = cps.begin(); it != cps.end(); ++it, ++i) {
                    double x, y;
                    (*it)->getPositionAtTime(time, &x, &y);
                    if ( ( x >= (pos.x() - cpSelectionTolerance) ) && ( x <= (pos.x() + cpSelectionTolerance) ) &&
                        ( y >= (pos.y() - cpSelectionTolerance) ) && ( y <= (pos.y() + cpSelectionTolerance) ) ) {
                        if ( it == cps.begin() ) {
                            pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, _imp->ui->builtBezier, view) );

                            _imp->ui->builtBezier.reset();

                            _imp->ui->selectedCps.clear();
                            _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
                        } else {
                            BezierCPPtr fp = _imp->ui->builtBezier->getFeatherPointAtIndex(i, view);
                            assert(fp);
                            _imp->ui->handleControlPointSelection( std::make_pair(*it, fp) );
                        }

                        return true;
                    }
                }
            }

            bool isOpenBezier = _imp->ui->selectedTool == eRotoToolOpenBezier;
            MakeBezierUndoCommand* cmd = new MakeBezierUndoCommand(_imp->ui, _imp->ui->builtBezier, isOpenBezier, true, pos.x(), pos.y(), time);
            pushUndoCommand(cmd);
            _imp->ui->builtBezier = cmd->getCurve();
            assert(_imp->ui->builtBezier);
            _imp->ui->state = eEventStateBuildingBezierControlPointTangent;
            didSomething = true;
            break;
        }
        case eRotoToolDrawBSpline:

            break;
        case eRotoToolDrawEllipse: {
            _imp->ui->click = pos;
            pushUndoCommand( new MakeEllipseUndoCommand(_imp->ui, true, false, false, pos.x(), pos.y(), pos.x(), pos.y(), time) );
            _imp->ui->state = eEventStateBuildingEllipse;
            didSomething = true;
            break;
        }
        case eRotoToolDrawRectangle: {
            _imp->ui->click = pos;
            pushUndoCommand( new MakeRectangleUndoCommand(_imp->ui, true, false, false, pos.x(), pos.y(), pos.x(), pos.y(), time) );
            _imp->ui->evaluateOnPenUp = true;
            _imp->ui->state = eEventStateBuildingRectangle;
            didSomething = true;
            break;
        }
        case eRotoToolSolidBrush:
        case eRotoToolEraserBrush:
        case eRotoToolClone:
        case eRotoToolReveal:
        case eRotoToolBlur:
        case eRotoToolSharpen:
        case eRotoToolSmear:
        case eRotoToolDodge:
        case eRotoToolBurn: {
            if ( ( ( _imp->ui->selectedTool == eRotoToolClone) || ( _imp->ui->selectedTool == eRotoToolReveal) ) && _imp->ui->ctrlDown &&
                !_imp->ui->shiftDown && !_imp->ui->altDown ) {
                _imp->ui->state = eEventStateDraggingCloneOffset;
            } else if (_imp->ui->shiftDown && !_imp->ui->ctrlDown && !_imp->ui->altDown) {
                _imp->ui->state = eEventStateDraggingBrushSize;
                _imp->ui->mouseCenterOnSizeChange = pos;
            } else if (_imp->ui->ctrlDown && _imp->ui->shiftDown) {
                _imp->ui->state = eEventStateDraggingBrushOpacity;
                _imp->ui->mouseCenterOnSizeChange = pos;
            } else {
                /*
                 Check that all viewers downstream are connected directly to the RotoPaint to avoid glitches and bugs
                 */
                _imp->ui->checkViewersAreDirectlyConnected();
                bool multiStrokeEnabled = _imp->ui->isMultiStrokeEnabled();
                if (_imp->ui->strokeBeingPaint &&
                    _imp->ui->strokeBeingPaint->getParent() &&
                    multiStrokeEnabled) {
                    RotoLayerPtr layer = toRotoLayer(_imp->ui->strokeBeingPaint->getParent());
                    if (!layer) {
                        layer = getLayerForNewItem();
                        _imp->knobsTable->insertItem(0, _imp->ui->strokeBeingPaint, layer, eTableChangeReasonInternal);
                    }
                    getApp()->setUserIsPainting(_imp->ui->strokeBeingPaint);

                    KnobIntPtr lifeTimeFrameKnob = _imp->ui->strokeBeingPaint->getLifeTimeFrameKnob();
                    lifeTimeFrameKnob->setValue(time);

                    _imp->ui->strokeBeingPaint->beginSubStroke();
                    _imp->ui->strokeBeingPaint->appendPoint( RotoPoint(pos.x(), pos.y(), pressure, timestamp) );
                } else {
                    if (_imp->ui->strokeBeingPaint &&
                        !_imp->ui->strokeBeingPaint->getParent() &&
                        multiStrokeEnabled) {
                        _imp->ui->strokeBeingPaint.reset();
                    }
                    _imp->ui->makeStroke( false, RotoPoint(pos.x(), pos.y(), pressure, timestamp) );
                }
                _imp->ui->state = eEventStateBuildingStroke;
                setCurrentCursor(eCursorBlank);
            }
            didSomething = true;
            break;
        }
        default:
            assert(false);
            break;
    } // switch

    _imp->ui->lastClickPos = pos;
    _imp->ui->lastMousePos = pos;

    return didSomething;
} // penDown

bool
RotoPaint::onOverlayPenMotion(double time,
                              const RenderScale & /*renderScale*/,
                              ViewIdx view,
                              const QPointF & /*viewportPos*/,
                              const QPointF & pos,
                              double pressure,
                              double timestamp)
{
    if (isDoingNeatRender()) {
        return true;
    }
    std::pair<double, double> pixelScale;

    getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    bool didSomething = false;
    HoverStateEnum lastHoverState = _imp->ui->hoverState;
    ///Set the cursor to the appropriate case
    bool cursorSet = false;
    double cpTol = kControlPointSelectionTolerance * pixelScale.first;

    if ( _imp->nodeType == RotoPaint::eRotoPaintTypeRotoPaint &&
        ( ( _imp->ui->selectedRole == eRotoRoleMergeBrush) ||
         ( _imp->ui->selectedRole == eRotoRoleCloneBrush) ||
         ( _imp->ui->selectedRole == eRotoRolePaintBrush) ||
         ( _imp->ui->selectedRole == eRotoRoleEffectBrush) ) ) {
            if (_imp->ui->state != eEventStateBuildingStroke) {
                setCurrentCursor(eCursorCross);
            } else {
                setCurrentCursor(eCursorBlank);
            }
            didSomething = true;
            cursorSet = true;
        }

    if ( !cursorSet && _imp->ui->showCpsBbox && (_imp->ui->state != eEventStateDraggingControlPoint) && (_imp->ui->state != eEventStateDraggingSelectedControlPoints)
        && ( _imp->ui->state != eEventStateDraggingLeftTangent) &&
        ( _imp->ui->state != eEventStateDraggingRightTangent) ) {
        double bboxTol = cpTol;
        if ( _imp->ui->isNearbyBBoxBtmLeft(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxBtmLeft;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxBtmRight(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxBtmRight;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxTopRight(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxTopRight;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxTopLeft(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxTopLeft;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxMidTop(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxMidTop;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxMidRight(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxMidRight;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxMidBtm(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxMidBtm;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxMidLeft(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxMidLeft;
            didSomething = true;
        } else {
            _imp->ui->hoverState = eHoverStateNothing;
            didSomething = true;
        }
    }
    const bool featherVisible = _imp->ui->isFeatherVisible();

    if ( (_imp->ui->state == eEventStateNone) && (_imp->ui->hoverState == eHoverStateNothing) ) {
        if ( (_imp->ui->state != eEventStateDraggingControlPoint) && (_imp->ui->state != eEventStateDraggingSelectedControlPoints) ) {
            std::list<KnobTableItemPtr> selectedItems = _imp->knobsTable->getSelectedItems();
            for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
                int index = -1;
                BezierPtr isBezier = toBezier(*it);
                if (isBezier) {
                    std::pair<BezierCPPtr, BezierCPPtr > nb =
                    isBezier->isNearbyControlPoint(pos.x(), pos.y(), cpTol, time, view, Bezier::eControlPointSelectionPrefWhateverFirst, &index);
                    if ( (index != -1) && ( ( !nb.first->isFeatherPoint() && !featherVisible ) || featherVisible ) ) {
                        setCurrentCursor(eCursorCross);
                        cursorSet = true;
                        break;
                    }
                }
            }
        }
        if ( !cursorSet && (_imp->ui->state != eEventStateDraggingLeftTangent) && (_imp->ui->state != eEventStateDraggingRightTangent) ) {
            ///find a nearby tangent
            for (SelectedCPs::const_iterator it = _imp->ui->selectedCps.begin(); it != _imp->ui->selectedCps.end(); ++it) {
                if (it->first->isNearbyTangent(time,  view, pos.x(), pos.y(), cpTol) != -1) {
                    setCurrentCursor(eCursorCross);
                    cursorSet = true;
                    break;
                }
            }
        }
        if ( !cursorSet && (_imp->ui->state != eEventStateDraggingControlPoint) && (_imp->ui->state != eEventStateDraggingSelectedControlPoints) && (_imp->ui->state != eEventStateDraggingLeftTangent) &&
            ( _imp->ui->state != eEventStateDraggingRightTangent) ) {
            double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
            double nearbyBezierT;
            int nearbyBezierCPIndex;
            bool isFeather;
            BezierPtr nearbyBezier =
            _imp->ui->isNearbyBezier(pos.x(), pos.y(), time, view, bezierSelectionTolerance, &nearbyBezierCPIndex, &nearbyBezierT, &isFeather);
            if (isFeather && !featherVisible) {
                nearbyBezier.reset();
            }
            if (nearbyBezier) {
                setCurrentCursor(eCursorPointingHand);
                cursorSet = true;
            }
        }

        bool clickAnywhere = _imp->ui->isBboxClickAnywhereEnabled();

        if ( !cursorSet && (_imp->ui->selectedCps.size() > 1) ) {
            if ( ( clickAnywhere && _imp->ui->isWithinSelectedCpsBBox(pos) ) ||
                ( !clickAnywhere && _imp->ui->isNearbySelectedCpsCrossHair(pos) ) ) {
                setCurrentCursor(eCursorSizeAll);
                cursorSet = true;
            }
        }

        SelectedCP nearbyFeatherBar;
        if (!cursorSet && featherVisible) {
            nearbyFeatherBar = _imp->ui->isNearbyFeatherBar(time, view, pixelScale, pos);
            if (nearbyFeatherBar.first && nearbyFeatherBar.second) {
                _imp->ui->featherBarBeingHovered = nearbyFeatherBar;
            }
        }
        if (!nearbyFeatherBar.first || !nearbyFeatherBar.second) {
            _imp->ui->featherBarBeingHovered.first.reset();
            _imp->ui->featherBarBeingHovered.second.reset();
        }

        if ( (_imp->ui->state != eEventStateNone) || _imp->ui->featherBarBeingHovered.first || cursorSet || (lastHoverState != eHoverStateNothing) ) {
            didSomething = true;
        }
    }


    if (!cursorSet) {
        setCurrentCursor(eCursorDefault);
    }


    double dx = pos.x() - _imp->ui->lastMousePos.x();
    double dy = pos.y() - _imp->ui->lastMousePos.y();
    switch (_imp->ui->state) {
        case eEventStateDraggingSelectedControlPoints: {
            if (_imp->ui->bezierBeingDragged) {
                SelectedCPs cps;
                const std::list<BezierCPPtr >& c = _imp->ui->bezierBeingDragged->getControlPoints(view);
                const std::list<BezierCPPtr >& f = _imp->ui->bezierBeingDragged->getFeatherPoints(view);
                assert( c.size() == f.size() || !_imp->ui->bezierBeingDragged->useFeatherPoints() );
                bool useFeather = _imp->ui->bezierBeingDragged->useFeatherPoints();
                std::list<BezierCPPtr >::const_iterator itFp = f.begin();
                for (std::list<BezierCPPtr >::const_iterator itCp = c.begin(); itCp != c.end(); ++itCp) {
                    if (useFeather) {
                        cps.push_back( std::make_pair(*itCp, *itFp) );
                        ++itFp;
                    } else {
                        cps.push_back( std::make_pair( *itCp, BezierCPPtr() ) );
                    }
                }
                pushUndoCommand( new MoveControlPointsUndoCommand(_imp->ui, cps, dx, dy, time, view) );
            } else {
                pushUndoCommand( new MoveControlPointsUndoCommand(_imp->ui, _imp->ui->selectedCps, dx, dy, time, view) );
            }
            _imp->ui->evaluateOnPenUp = true;
            _imp->ui->computeSelectedCpsBBOX();
            didSomething = true;
            break;
        }
        case eEventStateDraggingControlPoint: {
            assert(_imp->ui->cpBeingDragged.first);
            std::list<SelectedCP> toDrag;
            toDrag.push_back(_imp->ui->cpBeingDragged);
            pushUndoCommand( new MoveControlPointsUndoCommand(_imp->ui, toDrag, dx, dy, time, view) );
            _imp->ui->evaluateOnPenUp = true;
            _imp->ui->computeSelectedCpsBBOX();
            didSomething = true;
        };  break;
        case eEventStateBuildingBezierControlPointTangent: {
            assert(_imp->ui->builtBezier);
            bool isOpenBezier = _imp->ui->selectedTool == eRotoToolOpenBezier;
            pushUndoCommand( new MakeBezierUndoCommand(_imp->ui, _imp->ui->builtBezier, isOpenBezier, false, dx, dy, time) );
            break;
        }
        case eEventStateBuildingEllipse: {
            bool fromCenter = _imp->ui->ctrlDown > 0;
            bool constrained = _imp->ui->shiftDown > 0;
            pushUndoCommand( new MakeEllipseUndoCommand(_imp->ui, false, fromCenter, constrained, _imp->ui->click.x(), _imp->ui->click.y(), pos.x(), pos.y(), time) );

            didSomething = true;
            _imp->ui->evaluateOnPenUp = true;
            break;
        }
        case eEventStateBuildingRectangle: {
            bool fromCenter = _imp->ui->ctrlDown > 0;
            bool constrained = _imp->ui->shiftDown > 0;
            pushUndoCommand( new MakeRectangleUndoCommand(_imp->ui, false, fromCenter, constrained, _imp->ui->click.x(), _imp->ui->click.y(), pos.x(), pos.y(), time) );
            didSomething = true;
            _imp->ui->evaluateOnPenUp = true;
            break;
        }
        case eEventStateDraggingLeftTangent: {
            assert(_imp->ui->tangentBeingDragged);
            pushUndoCommand( new MoveTangentUndoCommand( _imp->ui, dx, dy, time, view, _imp->ui->tangentBeingDragged, true,
                                                        _imp->ui->ctrlDown && !_imp->ui->shiftDown && !_imp->ui->altDown ) );
            _imp->ui->evaluateOnPenUp = true;
            didSomething = true;
            break;
        }
        case eEventStateDraggingRightTangent: {
            assert(_imp->ui->tangentBeingDragged);
            pushUndoCommand( new MoveTangentUndoCommand( _imp->ui, dx, dy, time, view, _imp->ui->tangentBeingDragged, false,
                                                        _imp->ui->ctrlDown && !_imp->ui->shiftDown && !_imp->ui->altDown ) );
            _imp->ui->evaluateOnPenUp = true;
            didSomething = true;
            break;
        }
        case eEventStateDraggingFeatherBar: {
            pushUndoCommand( new MoveFeatherBarUndoCommand(_imp->ui, dx, dy, _imp->ui->featherBarBeingDragged, time, view) );
            _imp->ui->evaluateOnPenUp = true;
            didSomething = true;
            break;
        }
        case eEventStateDraggingBBoxTopLeft:
        case eEventStateDraggingBBoxTopRight:
        case eEventStateDraggingBBoxBtmRight:
        case eEventStateDraggingBBoxBtmLeft: {
            QPointF center = _imp->ui->getSelectedCpsBBOXCenter();
            double rot = 0;
            double sx = 1., sy = 1.;

            if (_imp->ui->transformMode == eSelectedCpsTransformModeRotateAndSkew) {
                double angle = std::atan2( pos.y() - center.y(), pos.x() - center.x() );
                double prevAngle = std::atan2( _imp->ui->lastMousePos.y() - center.y(), _imp->ui->lastMousePos.x() - center.x() );
                rot = angle - prevAngle;
            } else {
                // the scale ratio is the ratio of distances to the center
                double prevDist = ( _imp->ui->lastMousePos.x() - center.x() ) * ( _imp->ui->lastMousePos.x() - center.x() ) +
                ( _imp->ui->lastMousePos.y() - center.y() ) * ( _imp->ui->lastMousePos.y() - center.y() );
                if (prevDist != 0) {
                    double dist = ( pos.x() - center.x() ) * ( pos.x() - center.x() ) + ( pos.y() - center.y() ) * ( pos.y() - center.y() );
                    double ratio = std::sqrt(dist / prevDist);
                    sx *= ratio;
                    sy *= ratio;
                }
            }

            double tx = 0., ty = 0.;
            double skewX = 0., skewY = 0.;
            pushUndoCommand( new TransformUndoCommand(_imp->ui, center.x(), center.y(), rot, skewX, skewY, tx, ty, sx, sy, time, view) );
            _imp->ui->evaluateOnPenUp = true;
            didSomething = true;
            break;
        }
        case eEventStateDraggingBBoxMidTop:
        case eEventStateDraggingBBoxMidBtm:
        case eEventStateDraggingBBoxMidLeft:
        case eEventStateDraggingBBoxMidRight: {
            QPointF center = _imp->ui->getSelectedCpsBBOXCenter();
            double rot = 0;
            double sx = 1., sy = 1.;
            double skewX = 0., skewY = 0.;
            double tx = 0., ty = 0.;
            TransformUndoCommand::TransformPointsSelectionEnum type = TransformUndoCommand::eTransformAllPoints;
            if (!_imp->ui->shiftDown) {
                type = TransformUndoCommand::eTransformAllPoints;
            } else {
                if (_imp->ui->state == eEventStateDraggingBBoxMidTop) {
                    type = TransformUndoCommand::eTransformMidTop;
                } else if (_imp->ui->state == eEventStateDraggingBBoxMidBtm) {
                    type = TransformUndoCommand::eTransformMidBottom;
                } else if (_imp->ui->state == eEventStateDraggingBBoxMidLeft) {
                    type = TransformUndoCommand::eTransformMidLeft;
                } else if (_imp->ui->state == eEventStateDraggingBBoxMidRight) {
                    type = TransformUndoCommand::eTransformMidRight;
                }
            }

            const QRectF& bbox = _imp->ui->selectedCpsBbox;

            switch (type) {
                case TransformUndoCommand::eTransformMidBottom:
                    center.rx() = bbox.center().x();
                    center.ry() = bbox.top();
                    break;
                case TransformUndoCommand::eTransformMidTop:
                    center.rx() = bbox.center().x();
                    center.ry() = bbox.bottom();
                    break;
                case TransformUndoCommand::eTransformMidRight:
                    center.rx() = bbox.left();
                    center.ry() = bbox.center().y();
                    break;
                case TransformUndoCommand::eTransformMidLeft:
                    center.rx() = bbox.right();
                    center.ry() = bbox.center().y();
                    break;
                default:
                    break;
            }

            bool processX = _imp->ui->state == eEventStateDraggingBBoxMidRight || _imp->ui->state == eEventStateDraggingBBoxMidLeft;

            if (_imp->ui->transformMode == eSelectedCpsTransformModeRotateAndSkew) {
                if (!processX) {
                    const double addSkew = ( pos.x() - _imp->ui->lastMousePos.x() ) / ( pos.y() - center.y() );
                    skewX += addSkew;
                } else {
                    const double addSkew = ( pos.y() - _imp->ui->lastMousePos.y() ) / ( pos.x() - center.x() );
                    skewY += addSkew;
                }
            } else {
                // the scale ratio is the ratio of distances to the center
                double prevDist = ( _imp->ui->lastMousePos.x() - center.x() ) * ( _imp->ui->lastMousePos.x() - center.x() ) +
                ( _imp->ui->lastMousePos.y() - center.y() ) * ( _imp->ui->lastMousePos.y() - center.y() );
                if (prevDist != 0) {
                    double dist = ( pos.x() - center.x() ) * ( pos.x() - center.x() ) + ( pos.y() - center.y() ) * ( pos.y() - center.y() );
                    double ratio = std::sqrt(dist / prevDist);
                    if (processX) {
                        sx *= ratio;
                    } else {
                        sy *= ratio;
                    }
                }
            }


            pushUndoCommand( new TransformUndoCommand(_imp->ui, center.x(), center.y(), rot, skewX, skewY, tx, ty, sx, sy, time, view) );
            _imp->ui->evaluateOnPenUp = true;
            didSomething = true;
            break;
        }
        case eEventStateBuildingStroke: {
            if (_imp->ui->strokeBeingPaint) {
                // disable draft during painting
                if ( getApp()->isDraftRenderEnabled() ) {
                    getApp()->setDraftRenderEnabled(false);
                }

                RotoPoint p(pos.x(), pos.y(), pressure, timestamp);
                _imp->ui->strokeBeingPaint->appendPoint(p);
                _imp->ui->lastMousePos = pos;
                _imp->ui->strokeBeingPaint->invalidateCacheHashAndEvaluate(true, false);

                return true;

            }
            break;
        }
        case eEventStateDraggingCloneOffset: {
            _imp->ui->cloneOffset.first -= dx;
            _imp->ui->cloneOffset.second -= dy;
            _imp->ui->onBreakMultiStrokeTriggered();
            break;
        }
        case eEventStateDraggingBrushSize: {
            KnobDoublePtr sizeSb = _imp->ui->sizeSpinbox.lock();
            double size = 0;
            if (sizeSb) {
                size = sizeSb->getValue();
            }
            size += ( (dx + dy) / 2. );
            const double scale = 0.01;      // i.e. round to nearest one-hundreth
            size = std::floor(size / scale + 0.5) * scale;
            if (sizeSb) {
                sizeSb->setValue( std::max(1., size) );
            }
            _imp->ui->onBreakMultiStrokeTriggered();
            didSomething = true;
            break;
        }
        case eEventStateDraggingBrushOpacity: {
            KnobDoublePtr opaSb = _imp->ui->sizeSpinbox.lock();
            double opa = 0;
            if (opaSb) {
                opa = opaSb->getValue();
            }
            double newOpa = opa + ( (dx + dy) / 2. );
            if (opa != 0) {
                newOpa = std::max( 0., std::min(1., newOpa / opa) );
                newOpa = newOpa > 0 ? std::min(1., opa + 0.05) : std::max(0., opa - 0.05);
            } else {
                newOpa = newOpa < 0 ? .0 : 0.05;
            }
            const double scale = 0.01;      // i.e. round to nearest one-hundreth
            newOpa = std::floor(newOpa / scale + 0.5) * scale;
            if (opaSb) {
                opaSb->setValue(newOpa);
            }
            _imp->ui->onBreakMultiStrokeTriggered();
            didSomething = true;
            break;
        }
        case eEventStateNone:
        default:
            break;
    } // switch
    _imp->ui->lastMousePos = pos;

    return didSomething;
} // onOverlayPenMotion

bool
RotoPaint::onOverlayPenUp(double /*time*/,
                          const RenderScale & /*renderScale*/,
                          ViewIdx /*view*/,
                          const QPointF & /*viewportPos*/,
                          const QPointF & /*pos*/,
                          double /*pressure*/,
                          double /*timestamp*/)
{
    if (isDoingNeatRender()) {
        return true;
    }

    if (_imp->ui->evaluateOnPenUp) {
        invalidateCacheHashAndEvaluate(true, false);

        //sync other viewers linked to this roto
        redrawOverlayInteract();
        _imp->ui->evaluateOnPenUp = false;
    }

    bool ret = false;
    _imp->ui->tangentBeingDragged.reset();
    _imp->ui->bezierBeingDragged.reset();
    _imp->ui->cpBeingDragged.first.reset();
    _imp->ui->cpBeingDragged.second.reset();
    _imp->ui->featherBarBeingDragged.first.reset();
    _imp->ui->featherBarBeingDragged.second.reset();

    if ( (_imp->ui->state == eEventStateDraggingBBoxMidLeft) ||
        ( _imp->ui->state == eEventStateDraggingBBoxMidLeft) ||
        ( _imp->ui->state == eEventStateDraggingBBoxMidTop) ||
        ( _imp->ui->state == eEventStateDraggingBBoxMidBtm) ||
        ( _imp->ui->state == eEventStateDraggingBBoxTopLeft) ||
        ( _imp->ui->state == eEventStateDraggingBBoxTopRight) ||
        ( _imp->ui->state == eEventStateDraggingBBoxBtmRight) ||
        ( _imp->ui->state == eEventStateDraggingBBoxBtmLeft) ) {
        _imp->ui->computeSelectedCpsBBOX();
    }

    if (_imp->ui->state == eEventStateBuildingStroke) {
        assert(_imp->ui->strokeBeingPaint);
        assert( _imp->ui->strokeBeingPaint->getParent() );

        bool multiStrokeEnabled = _imp->ui->isMultiStrokeEnabled();
        if (!multiStrokeEnabled) {
            pushUndoCommand( new AddStrokeUndoCommand(_imp->ui, _imp->ui->strokeBeingPaint) );
            _imp->ui->makeStroke( true, RotoPoint() );
        } else {
            pushUndoCommand( new AddMultiStrokeUndoCommand(_imp->ui, _imp->ui->strokeBeingPaint) );
        }

        // Do a neat render for the stroke (better interpolation).
        evaluateNeatStrokeRender();
        ret = true;
    }

    _imp->ui->state = eEventStateNone;

    if ( (_imp->ui->selectedTool == eRotoToolDrawEllipse) || (_imp->ui->selectedTool == eRotoToolDrawRectangle) ) {
        _imp->ui->selectedCps.clear();
        _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
        ret = true;
    }

    if (_imp->ui->lastTabletDownTriggeredEraser) {
        _imp->ui->setCurrentTool( _imp->ui->lastPaintToolAction.lock() );
        ret = true;
    }

    return ret;
} // onOverlayPenUp

bool
RotoPaint::onOverlayKeyDown(double /*time*/,
                            const RenderScale & /*renderScale*/,
                            ViewIdx view,
                            Key key,
                            KeyboardModifiers /*modifiers*/)
{
    if (isDoingNeatRender()) {
        return true;
    }
    bool didSomething = false;

    if ( (key == Key_Shift_L) || (key == Key_Shift_R) ) {
        ++_imp->ui->shiftDown;
    } else if ( (key == Key_Control_L) || (key == Key_Control_R) ) {
        ++_imp->ui->ctrlDown;
    } else if ( (key == Key_Alt_L) || (key == Key_Alt_R) ) {
        ++_imp->ui->altDown;
    }

    if (_imp->ui->ctrlDown && !_imp->ui->shiftDown && !_imp->ui->altDown) {
        if ( !_imp->ui->iSelectingwithCtrlA && _imp->ui->showCpsBbox && ( (key == Key_Control_L) || (key == Key_Control_R) ) ) {
            _imp->ui->transformMode = _imp->ui->transformMode == eSelectedCpsTransformModeTranslateAndScale ?
            eSelectedCpsTransformModeRotateAndSkew : eSelectedCpsTransformModeTranslateAndScale;
            didSomething = true;
        }
    }

    if ( ( (key == Key_Escape) && ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) ) ) {
        if ( ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) && _imp->ui->builtBezier && !_imp->ui->builtBezier->isCurveFinished(view) ) {
            pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, _imp->ui->builtBezier, view) );
            
            _imp->ui->builtBezier.reset();
            _imp->ui->selectedCps.clear();
            _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
            didSomething = true;
        }
    }
    
    return didSomething;
} //onOverlayKeyDown

bool
RotoPaint::onOverlayKeyUp(double /*time*/,
                          const RenderScale & /*renderScale*/,
                          ViewIdx /*view*/,
                          Key key,
                          KeyboardModifiers /*modifiers*/)
{
    if (isDoingNeatRender()) {
        return true;
    }
    bool didSomething = false;
    
    if ( (key == Key_Shift_L) || (key == Key_Shift_R) ) {
        if (_imp->ui->shiftDown) {
            --_imp->ui->shiftDown;
        }
    } else if ( (key == Key_Control_L) || (key == Key_Control_R) ) {
        if (_imp->ui->ctrlDown) {
            --_imp->ui->ctrlDown;
        }
    } else if ( (key == Key_Alt_L) || (key == Key_Alt_R) ) {
        if (_imp->ui->altDown) {
            --_imp->ui->altDown;
        }
    }
    
    
    if (!_imp->ui->ctrlDown) {
        if ( !_imp->ui->iSelectingwithCtrlA && _imp->ui->showCpsBbox && ( (key == Key_Control_L) || (key == Key_Control_R) ) ) {
            _imp->ui->transformMode = (_imp->ui->transformMode == eSelectedCpsTransformModeTranslateAndScale ?
                                       eSelectedCpsTransformModeRotateAndSkew : eSelectedCpsTransformModeTranslateAndScale);
            didSomething = true;
        }
    }
    
    if ( ( (key == Key_Control_L) || (key == Key_Control_R) ) && _imp->ui->iSelectingwithCtrlA ) {
        _imp->ui->iSelectingwithCtrlA = false;
    }
    
    if (_imp->ui->evaluateOnKeyUp) {
        invalidateCacheHashAndEvaluate(true, false);
        redrawOverlayInteract();
        _imp->ui->evaluateOnKeyUp = false;
    }
    
    return didSomething;
} // onOverlayKeyUp

bool
RotoPaint::onOverlayKeyRepeat(double /*time*/,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/,
                              Key /*key*/,
                              KeyboardModifiers /*modifiers*/)
{
    return false;
} // onOverlayKeyRepeat

bool
RotoPaint::onOverlayFocusGained(double /*time*/,
                                const RenderScale & /*renderScale*/,
                                ViewIdx /*view*/)
{
    return false;
} // onOverlayFocusGained

bool
RotoPaint::onOverlayFocusLost(double /*time*/,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/)
{
    _imp->ui->shiftDown = 0;
    _imp->ui->ctrlDown = 0;
    _imp->ui->altDown = 0;
    
    return true;
} // onOverlayFocusLost


NATRON_NAMESPACE_EXIT;
