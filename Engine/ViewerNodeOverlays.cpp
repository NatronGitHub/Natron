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

#include "ViewerNodePrivate.h"



#define WIPE_MIX_HANDLE_LENGTH 50.
#define WIPE_ROTATE_HANDLE_LENGTH 100.
#define WIPE_ROTATE_OFFSET 30


#define USER_ROI_BORDER_TICK_SIZE 15.f
#define USER_ROI_CROSS_RADIUS 15.f
#define USER_ROI_SELECTION_POINT_SIZE 8.f
#define USER_ROI_CLICK_TOLERANCE 8.f


NATRON_NAMESPACE_ENTER;

void
ViewerNodePrivate::showRightClickMenu()
{
    KnobChoicePtr menu = rightClickMenu.lock();
    std::vector<ChoiceOption> entries, showHideEntries;

    std::vector<KnobButtonPtr> entriesButtons;
    entriesButtons.push_back(rightClickToggleWipe.lock());
    entriesButtons.push_back(rightClickCenterWipe.lock());
    entriesButtons.push_back(centerViewerButtonKnob.lock());
    entriesButtons.push_back(zoomScaleOneAction.lock());
    entriesButtons.push_back(zoomInAction.lock());
    entriesButtons.push_back(zoomOutAction.lock());
    entriesButtons.push_back(rightClickPreviousLayer.lock());
    entriesButtons.push_back(rightClickNextLayer.lock());
    entriesButtons.push_back(rightClickPreviousView.lock());
    entriesButtons.push_back(rightClickNextView.lock());
    entriesButtons.push_back(rightClickSwitchAB.lock());
    entriesButtons.push_back(rightClickShowHideOverlays.lock());
    entriesButtons.push_back(enableStatsAction.lock());

    for (std::size_t i = 0; i < entriesButtons.size(); ++i) {
        entries.push_back(ChoiceOption(entriesButtons[i]->getName(), "", ""));
    }

    entries.push_back(ChoiceOption(kViewerNodeParamRightClickMenuShowHideSubMenu, "", ""));


    KnobChoicePtr showHideMenu = rightClickShowHideSubMenu.lock();
    showHideEntries.push_back(ChoiceOption(kViewerNodeParamRightClickMenuHideAll, "", ""));
    showHideEntries.push_back(ChoiceOption(kViewerNodeParamRightClickMenuHideAllTop, "", ""));
    showHideEntries.push_back(ChoiceOption(kViewerNodeParamRightClickMenuHideAllBottom, "", ""));
    showHideEntries.push_back(ChoiceOption(kViewerNodeParamRightClickMenuShowHideTopToolbar, "", ""));
    showHideEntries.push_back(ChoiceOption(kViewerNodeParamRightClickMenuShowHideLeftToolbar, "", ""));
    showHideEntries.push_back(ChoiceOption(kViewerNodeParamRightClickMenuShowHidePlayer, "", ""));
    showHideEntries.push_back(ChoiceOption(kViewerNodeParamRightClickMenuShowHideTimeline, "", ""));
    showHideEntries.push_back(ChoiceOption(kViewerNodeParamRightClickMenuShowHideTabHeader, "", ""));
    showHideEntries.push_back(ChoiceOption(kViewerNodeParamEnableColorPicker, "", ""));

    {
        std::vector<int> separators;
        separators.push_back(2);
        showHideMenu->setSeparators(separators);
    }

    showHideMenu->populateChoices(showHideEntries);
    menu->populateChoices(entries);
    
}



void
ViewerNodePrivate::drawUserRoI()
{
    OverlaySupport* viewport = _publicInterface->getNode()->getCurrentViewportForOverlays();
    Point pixelScale;
    viewport->getPixelScale(pixelScale.x, pixelScale.y);

    {
        GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        GL_GPU::Disable(GL_BLEND);

        GL_GPU::Color4f(0.9, 0.9, 0.9, 1.);

        RectD userRoI;
        if ( uiState == eViewerNodeInteractMouseStateBuildingUserRoI ||
            uiState == eViewerNodeInteractMouseStateDraggingRoiBottomEdge ||
            uiState == eViewerNodeInteractMouseStateDraggingRoiBottomLeft ||
            uiState == eViewerNodeInteractMouseStateDraggingRoiBottomRight ||
            uiState == eViewerNodeInteractMouseStateDraggingRoiRightEdge ||
            uiState == eViewerNodeInteractMouseStateDraggingRoiTopRight ||
            uiState == eViewerNodeInteractMouseStateDraggingRoiTopEdge ||
            uiState == eViewerNodeInteractMouseStateDraggingRoiTopLeft ||
            uiState == eViewerNodeInteractMouseStateDraggingRoiLeftEdge ||
            uiState == eViewerNodeInteractMouseStateDraggingRoiCross ||
            buildUserRoIOnNextPress ) {
            userRoI = draggedUserRoI;
        } else {
            userRoI = _publicInterface->getUserRoI();
        }


        if (buildUserRoIOnNextPress) {
            GL_GPU::LineStipple(2, 0xAAAA);
            GL_GPU::Enable(GL_LINE_STIPPLE);
        }

        ///base rect
        GL_GPU::Begin(GL_LINE_LOOP);
        GL_GPU::Vertex2f(userRoI.x1, userRoI.y1); //bottom left
        GL_GPU::Vertex2f(userRoI.x1, userRoI.y2); //top left
        GL_GPU::Vertex2f(userRoI.x2, userRoI.y2); //top right
        GL_GPU::Vertex2f(userRoI.x2, userRoI.y1); //bottom right
        GL_GPU::End();


        GL_GPU::Begin(GL_LINES);
        ///border ticks
        double borderTickWidth = USER_ROI_BORDER_TICK_SIZE * pixelScale.x;
        double borderTickHeight = USER_ROI_BORDER_TICK_SIZE * pixelScale.y;
        GL_GPU::Vertex2f(userRoI.x1, (userRoI.y1 + userRoI.y2) / 2);
        GL_GPU::Vertex2f(userRoI.x1 - borderTickWidth, (userRoI.y1 + userRoI.y2) / 2);

        GL_GPU::Vertex2f(userRoI.x2, (userRoI.y1 + userRoI.y2) / 2);
        GL_GPU::Vertex2f(userRoI.x2 + borderTickWidth, (userRoI.y1 + userRoI.y2) / 2);

        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2, userRoI.y2 );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2, userRoI.y2 + borderTickHeight );

        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2, userRoI.y1 );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2, userRoI.y1 - borderTickHeight );

        ///middle cross
        double crossWidth = USER_ROI_CROSS_RADIUS * pixelScale.x;
        double crossHeight = USER_ROI_CROSS_RADIUS * pixelScale.y;
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2, (userRoI.y1 + userRoI.y2) / 2 - crossHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2, (userRoI.y1 + userRoI.y2) / 2 + crossHeight );

        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2  - crossWidth, (userRoI.y1 + userRoI.y2) / 2 );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2  + crossWidth, (userRoI.y1 + userRoI.y2) / 2 );
        GL_GPU::End();


        ///draw handles hint for the user
        GL_GPU::Begin(GL_QUADS);

        double rectHalfWidth = (USER_ROI_SELECTION_POINT_SIZE * pixelScale.x) / 2.;
        double rectHalfHeight = (USER_ROI_SELECTION_POINT_SIZE * pixelScale.y) / 2.;
        //left
        GL_GPU::Vertex2f(userRoI.x1 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x1 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x1 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x1 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);

        //top
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y2 - rectHalfHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y2 + rectHalfHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y2 + rectHalfHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y2 - rectHalfHeight );

        //right
        GL_GPU::Vertex2f(userRoI.x2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight);

        //bottom
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y1 - rectHalfHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, userRoI.y1 + rectHalfHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y1 + rectHalfHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, userRoI.y1 - rectHalfHeight );

        //middle
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 - rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 + rectHalfHeight );
        GL_GPU::Vertex2f( (userRoI.x1 +  userRoI.x2) / 2 + rectHalfWidth, (userRoI.y1 + userRoI.y2) / 2 - rectHalfHeight );


        //top left
        GL_GPU::Vertex2f(userRoI.x1 - rectHalfWidth, userRoI.y2 - rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x1 - rectHalfWidth, userRoI.y2 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x1 + rectHalfWidth, userRoI.y2 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x1 + rectHalfWidth, userRoI.y2 - rectHalfHeight);

        //top right
        GL_GPU::Vertex2f(userRoI.x2 - rectHalfWidth, userRoI.y2 - rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x2 - rectHalfWidth, userRoI.y2 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x2 + rectHalfWidth, userRoI.y2 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x2 + rectHalfWidth, userRoI.y2 - rectHalfHeight);

        //bottom right
        GL_GPU::Vertex2f(userRoI.x2 - rectHalfWidth, userRoI.y1 - rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x2 - rectHalfWidth, userRoI.y1 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x2 + rectHalfWidth, userRoI.y1 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x2 + rectHalfWidth, userRoI.y1 - rectHalfHeight);


        //bottom left
        GL_GPU::Vertex2f(userRoI.x1 - rectHalfWidth, userRoI.y1 - rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x1 - rectHalfWidth, userRoI.y1 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x1 + rectHalfWidth, userRoI.y1 + rectHalfHeight);
        GL_GPU::Vertex2f(userRoI.x1 + rectHalfWidth, userRoI.y1 - rectHalfHeight);

        GL_GPU::End();

        if (buildUserRoIOnNextPress) {
            GL_GPU::Disable(GL_LINE_STIPPLE);
        }
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
} // drawUserRoI

void
ViewerNodePrivate::drawArcOfCircle(const QPointF & center,
                                   double radiusX,
                                   double radiusY,
                                   double startAngle,
                                   double endAngle)
{
    double alpha = startAngle;
    double x, y;

    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT);

        if ( (hoverState == eHoverStateWipeMix) || (uiState == eViewerNodeInteractMouseStateDraggingWipeMixHandle) ) {
            GL_GPU::Color3f(0, 1, 0);
        }
        GL_GPU::Begin(GL_POINTS);
        while (alpha <= endAngle) {
            x = center.x()  + radiusX * std::cos(alpha);
            y = center.y()  + radiusY * std::sin(alpha);
            GL_GPU::Vertex2d(x, y);
            alpha += 0.01;
        }
        GL_GPU::End();
    } // GLProtectAttrib a(GL_CURRENT_BIT);
}

void
ViewerNodePrivate::drawWipeControl()
{
    OverlaySupport* viewport = _publicInterface->getNode()->getCurrentViewportForOverlays();
    Point pixelScale;
    viewport->getPixelScale(pixelScale.x, pixelScale.y);

    double angle;
    QPointF center;
    double mixAmount;
    {
        angle = wipeAngle.lock()->getValue();
        KnobDoublePtr centerKnob = wipeCenter.lock();
        center.rx() = centerKnob->getValue();
        center.ry() = centerKnob->getValue(DimIdx(1));
        mixAmount = wipeAmount.lock()->getValue();
    }
    double alphaMix1, alphaMix0, alphaCurMix;

    alphaMix1 = angle + M_PI_4 / 2;
    alphaMix0 = angle + 3. * M_PI_4 / 2;
    alphaCurMix = mixAmount * (alphaMix1 - alphaMix0) + alphaMix0;
    QPointF mix0Pos, mixPos, mix1Pos;
    double mixX, mixY, rotateW, rotateH, rotateOffsetX, rotateOffsetY;

    mixX = WIPE_MIX_HANDLE_LENGTH * pixelScale.x;
    mixY = WIPE_MIX_HANDLE_LENGTH * pixelScale.y;
    rotateW = WIPE_ROTATE_HANDLE_LENGTH * pixelScale.x;
    rotateH = WIPE_ROTATE_HANDLE_LENGTH * pixelScale.y;
    rotateOffsetX = WIPE_ROTATE_OFFSET * pixelScale.x;
    rotateOffsetY = WIPE_ROTATE_OFFSET * pixelScale.y;


    mixPos.setX(center.x() + std::cos(alphaCurMix) * mixX);
    mixPos.setY(center.y() + std::sin(alphaCurMix) * mixY);
    mix0Pos.setX(center.x() + std::cos(alphaMix0) * mixX);
    mix0Pos.setY(center.y() + std::sin(alphaMix0) * mixY);
    mix1Pos.setX(center.x() + std::cos(alphaMix1) * mixX);
    mix1Pos.setY(center.y() + std::sin(alphaMix1) * mixY);

    QPointF oppositeAxisBottom, oppositeAxisTop, rotateAxisLeft, rotateAxisRight;
    rotateAxisRight.setX( center.x() + std::cos(angle) * (rotateW - rotateOffsetX) );
    rotateAxisRight.setY( center.y() + std::sin(angle) * (rotateH - rotateOffsetY) );
    rotateAxisLeft.setX(center.x() - std::cos(angle) * rotateOffsetX);
    rotateAxisLeft.setY( center.y() - (std::sin(angle) * rotateOffsetY) );

    oppositeAxisTop.setX( center.x() + std::cos(angle + M_PI_2) * (rotateW / 2.) );
    oppositeAxisTop.setY( center.y() + std::sin(angle + M_PI_2) * (rotateH / 2.) );
    oppositeAxisBottom.setX( center.x() - std::cos(angle + M_PI_2) * (rotateW / 2.) );
    oppositeAxisBottom.setY( center.y() - std::sin(angle + M_PI_2) * (rotateH / 2.) );

    {
        GLProtectAttrib<GL_GPU> a(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)

        // Draw everything twice
        // l = 0: shadow
        // l = 1: drawing
        double baseColor[3];
        for (int l = 0; l < 2; ++l) {
            // shadow (uses GL_PROJECTION)
            GL_GPU::MatrixMode(GL_PROJECTION);
            int direction = (l == 0) ? 1 : -1;
            // translate (1,-1) pixels
            GL_GPU::Translated(direction * pixelScale.x, -direction * pixelScale.y, 0);
            GL_GPU::MatrixMode(GL_MODELVIEW); // Modelview should be used on Nuke

            if (l == 0) {
                // Draw a shadow for the cross hair
                baseColor[0] = baseColor[1] = baseColor[2] = 0.;
            } else {
                baseColor[0] = baseColor[1] = baseColor[2] = 0.8;
            }

            GL_GPU::Enable(GL_BLEND);
            GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            GL_GPU::Enable(GL_LINE_SMOOTH);
            GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
            GL_GPU::LineWidth(1.5);
            GL_GPU::Begin(GL_LINES);
            if ( (hoverState == eHoverStateWipeRotateHandle) || (uiState == eViewerNodeInteractMouseStateRotatingWipeHandle) ) {
                GL_GPU::Color4f(0., 1. * l, 0., 1.);
            }
            GL_GPU::Color4f(baseColor[0], baseColor[1], baseColor[2], 1.);
            GL_GPU::Vertex2d( rotateAxisLeft.x(), rotateAxisLeft.y() );
            GL_GPU::Vertex2d( rotateAxisRight.x(), rotateAxisRight.y() );
            GL_GPU::Vertex2d( oppositeAxisBottom.x(), oppositeAxisBottom.y() );
            GL_GPU::Vertex2d( oppositeAxisTop.x(), oppositeAxisTop.y() );
            GL_GPU::Vertex2d( center.x(), center.y() );
            GL_GPU::Vertex2d( mixPos.x(), mixPos.y() );
            GL_GPU::End();
            GL_GPU::LineWidth(1.);

            ///if hovering the rotate handle or dragging it show a small bended arrow
            if ( (hoverState == eHoverStateWipeRotateHandle) || (uiState == eViewerNodeInteractMouseStateRotatingWipeHandle) ) {
                GLProtectMatrix<GL_GPU> p(GL_MODELVIEW);

                GL_GPU::Color4f(0., 1. * l, 0., 1.);
                double arrowCenterX = WIPE_ROTATE_HANDLE_LENGTH * pixelScale.x / 2;
                ///draw an arrow slightly bended. This is an arc of circle of radius 5 in X, and 10 in Y.
                OfxPointD arrowRadius;
                arrowRadius.x = 5. * pixelScale.x;
                arrowRadius.y = 10. * pixelScale.y;

                GL_GPU::Translatef(center.x(), center.y(), 0.);
                GL_GPU::Rotatef(angle * 180.0 / M_PI, 0, 0, 1);
                //  center the oval at x_center, y_center
                GL_GPU::Translatef (arrowCenterX, 0., 0);
                //  draw the oval using line segments
                GL_GPU::Begin (GL_LINE_STRIP);
                GL_GPU::Vertex2f (0, arrowRadius.y);
                GL_GPU::Vertex2f (arrowRadius.x, 0.);
                GL_GPU::Vertex2f (0, -arrowRadius.y);
                GL_GPU::End ();


                GL_GPU::Begin(GL_LINES);
                ///draw the top head
                GL_GPU::Vertex2f(0., arrowRadius.y);
                GL_GPU::Vertex2f(0., arrowRadius.y -  arrowRadius.x );

                GL_GPU::Vertex2f(0., arrowRadius.y);
                GL_GPU::Vertex2f(4. * pixelScale.x, arrowRadius.y - 3. * pixelScale.y); // 5^2 = 3^2+4^2

                ///draw the bottom head
                GL_GPU::Vertex2f(0., -arrowRadius.y);
                GL_GPU::Vertex2f(0., -arrowRadius.y + 5. * pixelScale.y);

                GL_GPU::Vertex2f(0., -arrowRadius.y);
                GL_GPU::Vertex2f(4. * pixelScale.x, -arrowRadius.y + 3. * pixelScale.y); // 5^2 = 3^2+4^2

                GL_GPU::End();

                GL_GPU::Color4f(baseColor[0], baseColor[1], baseColor[2], 1.);
            }

            GL_GPU::PointSize(5.);
            GL_GPU::Enable(GL_POINT_SMOOTH);
            GL_GPU::Begin(GL_POINTS);
            GL_GPU::Vertex2d( center.x(), center.y() );
            if ( ( (hoverState == eHoverStateWipeMix) &&
                  (uiState != eViewerNodeInteractMouseStateRotatingWipeHandle) )
                || (uiState == eViewerNodeInteractMouseStateDraggingWipeMixHandle) ) {
                GL_GPU::Color4f(0., 1. * l, 0., 1.);
            }
            GL_GPU::Vertex2d( mixPos.x(), mixPos.y() );
            GL_GPU::End();
            GL_GPU::PointSize(1.);

            drawArcOfCircle(center, mixX, mixY, angle + M_PI_4 / 2, angle + 3. * M_PI_4 / 2);
        }
    } // GLProtectAttrib a(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
} // drawWipeControl

void
ViewerNode::drawOverlay(TimeValue /*time*/, const RenderScale & /*renderScale*/, ViewIdx /*view*/)
{
    bool userRoIEnabled = _imp->toggleUserRoIButtonKnob.lock()->getValue();
    if (userRoIEnabled) {
        _imp->drawUserRoI();
    }

    ViewerCompositingOperatorEnum compOperator = (ViewerCompositingOperatorEnum)_imp->blendingModeChoiceKnob.lock()->getValue();
    if (compOperator != eViewerCompositingOperatorNone &&
        compOperator != eViewerCompositingOperatorStackUnder &&
        compOperator != eViewerCompositingOperatorStackOver &&
        compOperator != eViewerCompositingOperatorStackMinus &&
        compOperator != eViewerCompositingOperatorStackOnionSkin) {
        _imp->drawWipeControl();
    }


} // drawOverlay


bool
ViewerNodePrivate::isNearbyWipeCenter(const QPointF& wipeCenter,
                                      const QPointF & pos,
                                      double zoomScreenPixelWidth,
                                      double zoomScreenPixelHeight)
{
    double toleranceX = zoomScreenPixelWidth * 8.;
    double toleranceY = zoomScreenPixelHeight * 8.;

    if ( ( pos.x() >= (wipeCenter.x() - toleranceX) ) && ( pos.x() <= (wipeCenter.x() + toleranceX) ) &&
        ( pos.y() >= (wipeCenter.y() - toleranceY) ) && ( pos.y() <= (wipeCenter.y() + toleranceY) ) ) {
        return true;
    }

    return false;
}

bool
ViewerNodePrivate::isNearbyWipeRotateBar(const QPointF& wipeCenter,
                                         double wipeAngle,
                                         const QPointF & pos,
                                         double zoomScreenPixelWidth,
                                         double zoomScreenPixelHeight)
{
    double toleranceX = zoomScreenPixelWidth * 8.;
    double toleranceY = zoomScreenPixelHeight * 8.;
    double rotateX, rotateY, rotateOffsetX, rotateOffsetY;

    rotateX = WIPE_ROTATE_HANDLE_LENGTH * zoomScreenPixelWidth;
    rotateY = WIPE_ROTATE_HANDLE_LENGTH * zoomScreenPixelHeight;
    rotateOffsetX = WIPE_ROTATE_OFFSET * zoomScreenPixelWidth;
    rotateOffsetY = WIPE_ROTATE_OFFSET * zoomScreenPixelHeight;

    QPointF outterPoint;

    outterPoint.setX( wipeCenter.x() + std::cos(wipeAngle) * (rotateX - rotateOffsetX) );
    outterPoint.setY( wipeCenter.y() + std::sin(wipeAngle) * (rotateY - rotateOffsetY) );
    if ( ( ( ( pos.y() >= (wipeCenter.y() - toleranceY) ) && ( pos.y() <= (outterPoint.y() + toleranceY) ) ) ||
          ( ( pos.y() >= (outterPoint.y() - toleranceY) ) && ( pos.y() <= (wipeCenter.y() + toleranceY) ) ) ) &&
        ( ( ( pos.x() >= (wipeCenter.x() - toleranceX) ) && ( pos.x() <= (outterPoint.x() + toleranceX) ) ) ||
         ( ( pos.x() >= (outterPoint.x() - toleranceX) ) && ( pos.x() <= (wipeCenter.x() + toleranceX) ) ) ) ) {
            Point a;
            a.x = ( outterPoint.x() - wipeCenter.x() );
            a.y = ( outterPoint.y() - wipeCenter.y() );
            double norm = sqrt(a.x * a.x + a.y * a.y);

            ///The point is in the bounding box of the segment, if it is vertical it must be on the segment anyway
            if (norm == 0) {
                return false;
            }

            a.x /= norm;
            a.y /= norm;
            Point b;
            b.x = ( pos.x() - wipeCenter.x() );
            b.y = ( pos.y() - wipeCenter.y() );
            norm = sqrt(b.x * b.x + b.y * b.y);

            ///This vector is not vertical
            if (norm != 0) {
                b.x /= norm;
                b.y /= norm;

                double crossProduct = b.y * a.x - b.x * a.y;
                if (std::abs(crossProduct) <  0.1) {
                    return true;
                }
            }
        }

    return false;
} // ViewerGL::Implementation::isNearbyWipeRotateBar

bool
ViewerNodePrivate::isNearbyWipeMixHandle(const QPointF& wipeCenter,
                                         double wipeAngle,
                                         double mixAmount,
                                         const QPointF & pos,
                                         double zoomScreenPixelWidth,
                                         double zoomScreenPixelHeight)
{
    double toleranceX = zoomScreenPixelWidth * 8.;
    double toleranceY = zoomScreenPixelHeight * 8.;
    ///mix 1 is at rotation bar + pi / 8
    ///mix 0 is at rotation bar + 3pi / 8
    double alphaMix1, alphaMix0, alphaCurMix;

    alphaMix1 = wipeAngle + M_PI_4 / 2;
    alphaMix0 = wipeAngle + 3 * M_PI_4 / 2;
    alphaCurMix = mixAmount * (alphaMix1 - alphaMix0) + alphaMix0;
    QPointF mixPos;
    double mixX = WIPE_MIX_HANDLE_LENGTH * zoomScreenPixelWidth;
    double mixY = WIPE_MIX_HANDLE_LENGTH * zoomScreenPixelHeight;

    mixPos.setX(wipeCenter.x() + std::cos(alphaCurMix) * mixX);
    mixPos.setY(wipeCenter.y() + std::sin(alphaCurMix) * mixY);
    if ( ( pos.x() >= (mixPos.x() - toleranceX) ) && ( pos.x() <= (mixPos.x() + toleranceX) ) &&
        ( pos.y() >= (mixPos.y() - toleranceY) ) && ( pos.y() <= (mixPos.y() + toleranceY) ) ) {
        return true;
    }

    return false;
}

bool
ViewerNodePrivate::isNearByUserRoITopEdge(const RectD & roi,
                                          const QPointF & zoomPos,
                                          double zoomScreenPixelWidth,
                                          double zoomScreenPixelHeight)
{
    double length = std::min(roi.x2 - roi.x1 - 10, (USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth) * 2);
    RectD r(roi.x1 + length / 2,
            roi.y2 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight,
            roi.x2 - length / 2,
            roi.y2 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight);

    return r.contains( zoomPos.x(), zoomPos.y() );
}

bool
ViewerNodePrivate::isNearByUserRoIRightEdge(const RectD & roi,
                                            const QPointF & zoomPos,
                                            double zoomScreenPixelWidth,
                                            double zoomScreenPixelHeight)
{
    double length = std::min(roi.y2 - roi.y1 - 10, (USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight) * 2);
    RectD r(roi.x2 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
            roi.y1 + length / 2,
            roi.x2 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
            roi.y2 - length / 2);

    return r.contains( zoomPos.x(), zoomPos.y() );
}

bool
ViewerNodePrivate::isNearByUserRoILeftEdge(const RectD & roi,
                                           const QPointF & zoomPos,
                                           double zoomScreenPixelWidth,
                                           double zoomScreenPixelHeight)
{
    double length = std::min(roi.y2 - roi.y1 - 10, (USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight) * 2);
    RectD r(roi.x1 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
            roi.y1 + length / 2,
            roi.x1 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth,
            roi.y2 - length / 2);

    return r.contains( zoomPos.x(), zoomPos.y() );
}

bool
ViewerNodePrivate::isNearByUserRoIBottomEdge(const RectD & roi,
                                             const QPointF & zoomPos,
                                             double zoomScreenPixelWidth,
                                             double zoomScreenPixelHeight)
{
    double length = std::min(roi.x2 - roi.x1 - 10, (USER_ROI_CLICK_TOLERANCE * zoomScreenPixelWidth) * 2);
    RectD r(roi.x1 + length / 2,
            roi.y1 - USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight,
            roi.x2 - length / 2,
            roi.y1 + USER_ROI_CLICK_TOLERANCE * zoomScreenPixelHeight);

    return r.contains( zoomPos.x(), zoomPos.y() );
}

bool
ViewerNodePrivate::isNearByUserRoI(double x,
                                   double y,
                                   const QPointF & zoomPos,
                                   double zoomScreenPixelWidth,
                                   double zoomScreenPixelHeight)
{
    RectD r(x - USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
            y - USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight,
            x + USER_ROI_CROSS_RADIUS * zoomScreenPixelWidth,
            y + USER_ROI_CROSS_RADIUS * zoomScreenPixelHeight);

    return r.contains( zoomPos.x(), zoomPos.y() );
}


bool
ViewerNode::onOverlayPenDown(TimeValue /*time*/,
                             const RenderScale & /*renderScale*/,
                             ViewIdx /*view*/,
                             const QPointF & /*viewportPos*/,
                             const QPointF & pos,
                             double /*pressure*/,
                             TimeValue /*timestamp*/,
                             PenType pen)
{
    OverlaySupport* viewport = getNode()->getCurrentViewportForOverlays();
    Point pixelScale;
    viewport->getPixelScale(pixelScale.x, pixelScale.y);

    bool overlaysCaught = false;
    if (!overlaysCaught &&
        pen == ePenTypeLMB &&
        _imp->buildUserRoIOnNextPress) {
        _imp->draggedUserRoI.x1 = pos.x();
        _imp->draggedUserRoI.y1 = pos.y();
        _imp->draggedUserRoI.x2 = pos.x();
        _imp->draggedUserRoI.y2 = pos.y();
        _imp->buildUserRoIOnNextPress = false;
        _imp->uiState = eViewerNodeInteractMouseStateBuildingUserRoI;
        overlaysCaught = true;
    }

    bool userRoIEnabled = _imp->toggleUserRoIButtonKnob.lock()->getValue();
    RectD userRoI;
    if (userRoIEnabled) {
        userRoI = getUserRoI();
    }
    // Catch wipe
    bool wipeEnabled = (ViewerCompositingOperatorEnum)_imp->blendingModeChoiceKnob.lock()->getValue() != eViewerCompositingOperatorNone;
    double wipeAmount = getWipeAmount();
    double wipeAngle = getWipeAngle();
    QPointF wipeCenter = getWipeCenter();

    if ( !overlaysCaught &&
        wipeEnabled &&
        pen == ePenTypeLMB &&
        _imp->isNearbyWipeCenter(wipeCenter, pos, pixelScale.x, pixelScale.y) ) {
        _imp->uiState = eViewerNodeInteractMouseStateDraggingWipeCenter;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
        wipeEnabled &&
        pen == ePenTypeLMB &&
        _imp->isNearbyWipeMixHandle(wipeCenter, wipeAngle, wipeAmount, pos, pixelScale.x, pixelScale.y) ) {
        _imp->uiState = eViewerNodeInteractMouseStateDraggingWipeMixHandle;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
        wipeEnabled &&
        pen == ePenTypeLMB &&
        _imp->isNearbyWipeRotateBar(wipeCenter, wipeAngle, pos, pixelScale.x, pixelScale.y) ) {
        _imp->uiState = eViewerNodeInteractMouseStateRotatingWipeHandle;
        overlaysCaught = true;
    }


    // Catch User RoI

    if ( !overlaysCaught &&
        pen == ePenTypeLMB &&
        userRoIEnabled &&
        _imp->isNearByUserRoIBottomEdge(userRoI, pos, pixelScale.x, pixelScale.y) ) {
        // start dragging the bottom edge of the user ROI
        _imp->uiState = eViewerNodeInteractMouseStateDraggingRoiBottomEdge;
        _imp->draggedUserRoI = userRoI;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
        pen == ePenTypeLMB &&
        userRoIEnabled &&
        _imp->isNearByUserRoILeftEdge(userRoI, pos, pixelScale.x, pixelScale.y) ) {
        // start dragging the left edge of the user ROI
        _imp->uiState = eViewerNodeInteractMouseStateDraggingRoiLeftEdge;
        _imp->draggedUserRoI = userRoI;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
        pen == ePenTypeLMB &&
        userRoIEnabled &&
        _imp->isNearByUserRoIRightEdge(userRoI, pos, pixelScale.x, pixelScale.y) ) {
        // start dragging the right edge of the user ROI
        _imp->uiState = eViewerNodeInteractMouseStateDraggingRoiRightEdge;
        _imp->draggedUserRoI = userRoI;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
        pen == ePenTypeLMB &&
        userRoIEnabled &&
        _imp->isNearByUserRoITopEdge(userRoI, pos, pixelScale.x, pixelScale.y) ) {
        // start dragging the top edge of the user ROI
        _imp->uiState = eViewerNodeInteractMouseStateDraggingRoiTopEdge;
        _imp->draggedUserRoI = userRoI;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
        pen == ePenTypeLMB &&
        userRoIEnabled &&
        _imp->isNearByUserRoI( (userRoI.x1 + userRoI.x2) / 2., (userRoI.y1 + userRoI.y2) / 2.,
                              pos, pixelScale.x, pixelScale.y ) ) {
            // start dragging the midpoint of the user ROI
            _imp->uiState = eViewerNodeInteractMouseStateDraggingRoiCross;
            _imp->draggedUserRoI = userRoI;
            overlaysCaught = true;
        }
    if ( !overlaysCaught &&
        pen == ePenTypeLMB &&
        userRoIEnabled &&
        _imp->isNearByUserRoI(userRoI.x1, userRoI.y2, pos, pixelScale.x, pixelScale.y) ) {
        // start dragging the topleft corner of the user ROI
        _imp->uiState = eViewerNodeInteractMouseStateDraggingRoiTopLeft;
        _imp->draggedUserRoI = userRoI;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
        pen == ePenTypeLMB &&
        userRoIEnabled &&
        _imp->isNearByUserRoI(userRoI.x2, userRoI.y2, pos, pixelScale.x, pixelScale.y) ) {
        // start dragging the topright corner of the user ROI
        _imp->uiState = eViewerNodeInteractMouseStateDraggingRoiTopRight;
        _imp->draggedUserRoI = userRoI;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
        pen == ePenTypeLMB &&
        userRoIEnabled &&
        _imp->isNearByUserRoI(userRoI.x1, userRoI.y1, pos, pixelScale.x, pixelScale.y) ) {
        // start dragging the bottomleft corner of the user ROI
        _imp->uiState = eViewerNodeInteractMouseStateDraggingRoiBottomLeft;
        _imp->draggedUserRoI = userRoI;
        overlaysCaught = true;
    }
    if ( !overlaysCaught &&
        pen == ePenTypeLMB &&
        userRoIEnabled &&
        _imp->isNearByUserRoI(userRoI.x2, userRoI.y1, pos, pixelScale.x, pixelScale.y) ) {
        // start dragging the bottomright corner of the user ROI
        _imp->uiState = eViewerNodeInteractMouseStateDraggingRoiBottomRight;
        _imp->draggedUserRoI = userRoI;
        overlaysCaught = true;
    }

    if ( !overlaysCaught &&
        pen == ePenTypeRMB ) {
        _imp->showRightClickMenu();
        overlaysCaught = true;
    }

    _imp->lastMousePos = pos;

    return overlaysCaught;

} // onOverlayPenDown

bool
ViewerNode::onOverlayPenMotion(TimeValue /*time*/, const RenderScale & /*renderScale*/, ViewIdx /*view*/,
                               const QPointF & /*viewportPos*/, const QPointF & pos, double /*pressure*/, TimeValue /*timestamp*/)
{
    OverlaySupport* viewport = getNode()->getCurrentViewportForOverlays();
    Point pixelScale;
    viewport->getPixelScale(pixelScale.x, pixelScale.y);

    bool userRoIEnabled = _imp->toggleUserRoIButtonKnob.lock()->getValue();
    RectD userRoI;
    if (userRoIEnabled) {
        if (_imp->uiState == eViewerNodeInteractMouseStateDraggingRoiBottomEdge ||
            _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiTopEdge ||
            _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiLeftEdge ||
            _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiRightEdge ||
            _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiCross ||
            _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiBottomLeft ||
            _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiBottomRight ||
            _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiTopLeft ||
            _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiTopRight) {
            userRoI = _imp->draggedUserRoI;
        } else {
            userRoI = getUserRoI();
        }
    }
    bool wipeEnabled = (ViewerCompositingOperatorEnum)_imp->blendingModeChoiceKnob.lock()->getValue() != eViewerCompositingOperatorNone;
    double wipeAmount = getWipeAmount();
    double wipeAngle = getWipeAngle();
    QPointF wipeCenter = getWipeCenter();

    bool wasHovering = _imp->hoverState != eHoverStateNothing;
    bool cursorSet = false;
    bool overlayCaught = false;
    _imp->hoverState = eHoverStateNothing;
    if ( wipeEnabled && _imp->isNearbyWipeCenter(wipeCenter, pos, pixelScale.x, pixelScale.y) ) {
        setCurrentCursor(eCursorSizeAll);
        cursorSet = true;
    } else if ( wipeEnabled && _imp->isNearbyWipeMixHandle(wipeCenter, wipeAngle, wipeAmount, pos, pixelScale.x, pixelScale.y) ) {
        _imp->hoverState = eHoverStateWipeMix;
        overlayCaught = true;
    } else if ( wipeEnabled && _imp->isNearbyWipeRotateBar(wipeCenter, wipeAngle, pos, pixelScale.x, pixelScale.y) ) {
        _imp->hoverState = eHoverStateWipeRotateHandle;
        overlayCaught = true;
    } else if (userRoIEnabled) {
        if ( _imp->isNearByUserRoIBottomEdge(userRoI, pos, pixelScale.x, pixelScale.y)
            || _imp->isNearByUserRoITopEdge(userRoI, pos, pixelScale.x, pixelScale.y)
            || ( _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiBottomEdge)
            || ( _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiTopEdge) ) {
            setCurrentCursor(eCursorSizeVer);
            cursorSet = true;
        } else if ( _imp->isNearByUserRoILeftEdge(userRoI, pos, pixelScale.x, pixelScale.y)
                   || _imp->isNearByUserRoIRightEdge(userRoI, pos, pixelScale.x, pixelScale.y)
                   || ( _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiLeftEdge)
                   || ( _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiRightEdge) ) {
            setCurrentCursor(eCursorSizeHor);
            cursorSet = true;
        } else if ( _imp->isNearByUserRoI( (userRoI.x1 + userRoI.x2) / 2, (userRoI.y1 + userRoI.y2) / 2, pos, pixelScale.x, pixelScale.y )
                   || ( _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiCross) ) {
            setCurrentCursor(eCursorSizeAll);
            cursorSet = true;
        } else if ( _imp->isNearByUserRoI(userRoI.x2, userRoI.y1, pos, pixelScale.x, pixelScale.y) ||
                   _imp->isNearByUserRoI(userRoI.x1, userRoI.y2, pos, pixelScale.x, pixelScale.y) ||
                   ( _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiBottomRight) ||
                   ( _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiTopLeft) ) {
            setCurrentCursor(eCursorFDiag);
            cursorSet = true;
        } else if ( _imp->isNearByUserRoI(userRoI.x1, userRoI.y1, pos, pixelScale.x, pixelScale.y) ||
                   _imp->isNearByUserRoI(userRoI.x2, userRoI.y2, pos, pixelScale.x, pixelScale.y) ||
                   ( _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiBottomLeft) ||
                   ( _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiTopRight) ) {
            setCurrentCursor(eCursorBDiag);
            cursorSet = true;
        }
    }

    if (!cursorSet) {
        setCurrentCursor(eCursorDefault);
    }


    if ( (_imp->hoverState == eHoverStateNothing) && wasHovering ) {
        overlayCaught = true;
    }

    double dx = pos.x() - _imp->lastMousePos.x();
    double dy = pos.y() - _imp->lastMousePos.y();

    switch (_imp->uiState) {
        case eViewerNodeInteractMouseStateDraggingRoiBottomEdge: {
            if ( (_imp->draggedUserRoI.y1 + dy) < _imp->draggedUserRoI.y2 ) {
                _imp->draggedUserRoI.y1 += dy;
                overlayCaught = true;
            }
            break;
        }
        case eViewerNodeInteractMouseStateDraggingRoiLeftEdge: {
            if ( (_imp->draggedUserRoI.x1 + dx) < _imp->draggedUserRoI.x2 ) {
                _imp->draggedUserRoI.x1 += dx;
                overlayCaught = true;
            }
            break;
        }
        case eViewerNodeInteractMouseStateDraggingRoiRightEdge: {
            if ( (_imp->draggedUserRoI.x2 + dx) > _imp->draggedUserRoI.x1 ) {
                _imp->draggedUserRoI.x2 += dx;
                overlayCaught = true;
            }
            break;
        }
        case eViewerNodeInteractMouseStateDraggingRoiTopEdge: {
            if ( (_imp->draggedUserRoI.y2 + dy) > _imp->draggedUserRoI.y1 ) {
                _imp->draggedUserRoI.y2 += dy;
                overlayCaught = true;
            }
            break;
        }
        case eViewerNodeInteractMouseStateDraggingRoiCross: {
            _imp->draggedUserRoI.translate(dx, dy);
            overlayCaught = true;
            break;
        }
        case eViewerNodeInteractMouseStateDraggingRoiTopLeft: {
            if ( (_imp->draggedUserRoI.y2 + dy) > _imp->draggedUserRoI.y1 ) {
                _imp->draggedUserRoI.y2 += dy;
            }
            if ( (_imp->draggedUserRoI.x1 + dx) < _imp->draggedUserRoI.x2 ) {
                _imp->draggedUserRoI.x1 += dx;
            }
            overlayCaught = true;
            break;
        }
        case eViewerNodeInteractMouseStateDraggingRoiTopRight: {
            if ( (_imp->draggedUserRoI.y2 + dy) > _imp->draggedUserRoI.y1 ) {
                _imp->draggedUserRoI.y2 += dy;
            }
            if ( (_imp->draggedUserRoI.x2 + dx) > _imp->draggedUserRoI.x1 ) {
                _imp->draggedUserRoI.x2 += dx;
            }
            overlayCaught = true;
            break;
        }
        case eViewerNodeInteractMouseStateDraggingRoiBottomRight:
        case eViewerNodeInteractMouseStateBuildingUserRoI:{
            if ( (_imp->draggedUserRoI.x2 + dx) > _imp->draggedUserRoI.x1 ) {
                _imp->draggedUserRoI.x2 += dx;
            }
            if ( (_imp->draggedUserRoI.y1 + dy) < _imp->draggedUserRoI.y2 ) {
                _imp->draggedUserRoI.y1 += dy;
            }
            overlayCaught = true;
            break;
        }
        case eViewerNodeInteractMouseStateDraggingRoiBottomLeft: {
            if ( (_imp->draggedUserRoI.y1 + dy) < _imp->draggedUserRoI.y2 ) {
                _imp->draggedUserRoI.y1 += dy;
            }
            if ( (_imp->draggedUserRoI.x1 + dx) < _imp->draggedUserRoI.x2 ) {
                _imp->draggedUserRoI.x1 += dx;
            }
            overlayCaught = true;

            break;
        }
        case eViewerNodeInteractMouseStateDraggingWipeCenter: {
            KnobDoublePtr centerKnob = _imp->wipeCenter.lock();
            centerKnob->setValue(centerKnob->getValue() + dx);
            centerKnob->setValue(centerKnob->getValue(DimIdx(1)) + dy, ViewSetSpec::all(), DimIdx(1));
            overlayCaught = true;
            break;
        }
        case eViewerNodeInteractMouseStateDraggingWipeMixHandle: {
            KnobDoublePtr centerKnob = _imp->wipeCenter.lock();
            Point center;
            center.x = centerKnob->getValue();
            center.y = centerKnob->getValue(DimIdx(1));
            double angle = std::atan2( pos.y() - center.y, pos.x() - center.x );
            double prevAngle = std::atan2( _imp->lastMousePos.y() - center.y,
                                          _imp->lastMousePos.x() - center.x );
            KnobDoublePtr mixKnob = _imp->wipeAmount.lock();
            double mixAmount = mixKnob->getValue();
            mixAmount -= (angle - prevAngle);
            mixAmount = std::max( 0., std::min(mixAmount, 1.) );
            mixKnob->setValue(mixAmount);
            overlayCaught = true;
            break;
        }
        case eViewerNodeInteractMouseStateRotatingWipeHandle: {
            KnobDoublePtr centerKnob = _imp->wipeCenter.lock();
            Point center;
            center.x = centerKnob->getValue();
            center.y = centerKnob->getValue(DimIdx(1));
            double angle = std::atan2( pos.y() - center.y, pos.x() - center.x );

            KnobDoublePtr angleKnob = _imp->wipeAngle.lock();
            double closestPI2 = M_PI_2 * std::floor(angle / M_PI_2 + 0.5);
            if (std::fabs(angle - closestPI2) < 0.1) {
                // snap to closest multiple of PI / 2.
                angle = closestPI2;
            }

            angleKnob->setValue(angle);

            overlayCaught = true;
            break;
        }
        default:
            break;
    }
    _imp->lastMousePos = pos;
    return overlayCaught;

} // onOverlayPenMotion

bool
ViewerNode::onOverlayPenUp(TimeValue /*time*/, const RenderScale & /*renderScale*/, ViewIdx /*view*/, const QPointF & /*viewportPos*/, const QPointF & /*pos*/, double /*pressure*/, TimeValue /*timestamp*/)
{

    bool caught = false;
    if (_imp->uiState == eViewerNodeInteractMouseStateDraggingRoiBottomEdge ||
        _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiTopEdge ||
        _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiLeftEdge ||
        _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiRightEdge ||
        _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiCross ||
        _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiBottomLeft ||
        _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiBottomRight ||
        _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiTopLeft ||
        _imp->uiState == eViewerNodeInteractMouseStateDraggingRoiTopRight ||
        _imp->uiState == eViewerNodeInteractMouseStateBuildingUserRoI) {
        setUserRoI(_imp->draggedUserRoI);
        caught = true;
    }

    _imp->uiState = eViewerNodeInteractMouseStateIdle;


    return caught;
} // onOverlayPenUp

bool
ViewerNode::onOverlayPenDoubleClicked(TimeValue /*time*/,
                                      const RenderScale & /*renderScale*/,
                                      ViewIdx /*view*/,
                                      const QPointF & /*viewportPos*/,
                                      const QPointF & /*pos*/)
{
    return false;
} // onOverlayPenDoubleClicked

bool
ViewerNode::onOverlayKeyDown(TimeValue /*time*/, const RenderScale & /*renderScale*/, ViewIdx /*view*/, Key /*key*/, KeyboardModifiers /*modifiers*/)
{
    return false;
} // onOverlayKeyDown

bool
ViewerNode::onOverlayKeyUp(TimeValue /*time*/, const RenderScale & /*renderScale*/, ViewIdx /*view*/, Key /*key*/, KeyboardModifiers /*modifiers*/)
{
    return false;
} // onOverlayKeyUp

bool
ViewerNode::onOverlayKeyRepeat(TimeValue /*time*/, const RenderScale & /*renderScale*/, ViewIdx /*view*/, Key /*key*/, KeyboardModifiers /*modifiers*/)
{
    return false;
} // onOverlayKeyRepeat

bool
ViewerNode::onOverlayFocusGained(TimeValue /*time*/, const RenderScale & /*renderScale*/, ViewIdx /*view*/)
{
    return false;
} // onOverlayFocusGained

bool
ViewerNode::onOverlayFocusLost(TimeValue /*time*/, const RenderScale & /*renderScale*/, ViewIdx /*view*/)
{
    return false;
} // onOverlayFocusLost


NATRON_NAMESPACE_EXIT;
