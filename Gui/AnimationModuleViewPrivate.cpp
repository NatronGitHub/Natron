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

#include "AnimationModuleViewPrivate.h"

#include "Global/GLIncludes.h" //!<must be included before QGLWidget
#include <QtOpenGL/QGLWidget>
#include "Global/GLObfuscate.h" //!<must be included after QGLWidget
#include <QApplication>

#include <QtCore/QThread>
#include <QDebug>
#include <QImage>

#include "Engine/KnobTypes.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobAnim.h"
#include "Gui/Menu.h"
#include "Gui/NodeAnim.h"
#include "Gui/Gui.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/TableItemAnim.h"




NATRON_NAMESPACE_ENTER

AnimationModuleViewPrivate::AnimationModuleViewPrivate(Gui* gui, AnimationModuleView* publicInterface, const AnimationModuleBasePtr& model)
: _publicInterface(publicInterface)
, _gui(gui)
, _model(model)
, zoomCtxMutex()
, curveEditorZoomContext(0.000001, 1000000.)
, dopeSheetZoomContext(0.01, 100.)
, zoomOrPannedSinceLastFit(false)
, textRenderer()
, selectionRect()
, curveEditorSelectedKeysBRect()
, dopeSheetSelectedKeysBRect()
, kfTexturesIDs()
, savedTexture(0)
, drawnOnce(false)
, dragStartPoint()
, lastMousePos()
, mustSetDragOrientation(false)
, mouseDragOrientation()
, keyDragLastMovement()
, state(eEventStateNone)
, eventTriggeredFromCurveEditor(false)
, treeView(0)
, currentEditedReader()
, customInteract()
{
    for (int i = 0; i < KF_TEXTURES_COUNT; ++i) {
        kfTexturesIDs[i] = 0;
    }
}

AnimationModuleViewPrivate::~AnimationModuleViewPrivate()
{
    if (kfTexturesIDs[0]) {
        GL_GPU::DeleteTextures(KF_TEXTURES_COUNT, kfTexturesIDs);
    }

}


RectD
AnimationModuleViewPrivate::getKeyFrameBoundingRectCanonical(const ZoomContext& ctx, double xCanonical, double yCanonical) const
{
    QPointF posWidget = ctx.toWidgetCoordinates(xCanonical, yCanonical);
    RectD ret;
    double keyframeTexSize = TO_DPIX(getKeyframeTextureSize());
    QPointF x1y1 = ctx.toZoomCoordinates(posWidget.x() - keyframeTexSize / 2., posWidget.y() + keyframeTexSize / 2);
    QPointF x2y2 = ctx.toZoomCoordinates(posWidget.x() + keyframeTexSize / 2., posWidget.y() - keyframeTexSize / 2);

    ret.x1 = x1y1.x();
    ret.y1 = x1y1.y();
    ret.x2 = x2y2.x();
    ret.y2 = x2y2.y();

    return ret;
}



void
AnimationModuleViewPrivate::drawTimelineMarkers(const ZoomContext& ctx)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _publicInterface->context() );
    glCheckError(GL_GPU);

    TimeLinePtr timeline = _model.lock()->getTimeline();
    if (!timeline) {
        return;
    }

    double cursorR, cursorG, cursorB;
    double boundsR, boundsG, boundsB;
    SettingsPtr settings = appPTR->getCurrentSettings();
    settings->getTimelinePlayheadColor(&cursorR, &cursorG, &cursorB);
    settings->getTimelineBoundsColor(&boundsR, &boundsG, &boundsB);

    QPointF topLeft = ctx.toZoomCoordinates(0, 0);
    QPointF btmRight = ctx.toZoomCoordinates(_publicInterface->width() - 1, _publicInterface->height() - 1);

    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::Enable(GL_LINE_SMOOTH);
        GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        GL_GPU::Color4f(boundsR, boundsG, boundsB, 1.);

        TimeValue leftBound, rightBound;
        _gui->getApp()->getProject()->getFrameRange(&leftBound, &rightBound);
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2f( leftBound, btmRight.y() );
        GL_GPU::Vertex2f( leftBound, topLeft.y() );
        GL_GPU::Vertex2f( rightBound, btmRight.y() );
        GL_GPU::Vertex2f( rightBound, topLeft.y() );
        GL_GPU::Color4f(cursorR, cursorG, cursorB, 1.);
        GL_GPU::Vertex2f( timeline->currentFrame(), btmRight.y() );
        GL_GPU::Vertex2f( timeline->currentFrame(), topLeft.y() );
        GL_GPU::End();
        glCheckErrorIgnoreOSXBug(GL_GPU);

        GL_GPU::Enable(GL_POLYGON_SMOOTH);
        GL_GPU::Hint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

        QPointF topLeft = ctx.toZoomCoordinates(0, 0);
        QPointF btmRight = ctx.toZoomCoordinates(_publicInterface->width() - 1, _publicInterface->height() - 1);
        QPointF btmCursorBtm( timeline->currentFrame(), btmRight.y() );
        QPointF btmcursorBtmWidgetCoord = ctx.toWidgetCoordinates( btmCursorBtm.x(), btmCursorBtm.y() );
        QPointF btmCursorTop = ctx.toZoomCoordinates(btmcursorBtmWidgetCoord.x(), btmcursorBtmWidgetCoord.y() - TO_DPIX(CURSOR_HEIGHT));
        QPointF btmCursorLeft = ctx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() - TO_DPIX(CURSOR_WIDTH) / 2., btmcursorBtmWidgetCoord.y() );
        QPointF btmCursorRight = ctx.toZoomCoordinates( btmcursorBtmWidgetCoord.x() + TO_DPIX(CURSOR_WIDTH) / 2., btmcursorBtmWidgetCoord.y() );
        QPointF topCursortop( timeline->currentFrame(), topLeft.y() );
        QPointF topcursorTopWidgetCoord = ctx.toWidgetCoordinates( topCursortop.x(), topCursortop.y() );
        QPointF topCursorBtm = ctx.toZoomCoordinates(topcursorTopWidgetCoord.x(), topcursorTopWidgetCoord.y() + TO_DPIX(CURSOR_HEIGHT));
        QPointF topCursorLeft = ctx.toZoomCoordinates( topcursorTopWidgetCoord.x() - TO_DPIX(CURSOR_WIDTH) / 2., topcursorTopWidgetCoord.y() );
        QPointF topCursorRight = ctx.toZoomCoordinates( topcursorTopWidgetCoord.x() + TO_DPIX(CURSOR_WIDTH) / 2., topcursorTopWidgetCoord.y() );


        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f( btmCursorTop.x(), btmCursorTop.y() );
        GL_GPU::Vertex2f( btmCursorLeft.x(), btmCursorLeft.y() );
        GL_GPU::Vertex2f( btmCursorRight.x(), btmCursorRight.y() );
        GL_GPU::End();
        glCheckErrorIgnoreOSXBug(GL_GPU);

        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f( topCursorBtm.x(), topCursorBtm.y() );
        GL_GPU::Vertex2f( topCursorLeft.x(), topCursorLeft.y() );
        GL_GPU::Vertex2f( topCursorRight.x(), topCursorRight.y() );
        GL_GPU::End();
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
    glCheckErrorIgnoreOSXBug(GL_GPU);
} // AnimationModuleViewPrivate::drawTimelineMarkers

void
AnimationModuleViewPrivate::drawSelectionRectangle()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == _publicInterface->context() );

    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::Enable(GL_LINE_SMOOTH);
        GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        GL_GPU::Color4f(0.3, 0.3, 0.3, 0.2);

        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f( selectionRect.x1, selectionRect.y1 );
        GL_GPU::Vertex2f( selectionRect.x1, selectionRect.y2 );
        GL_GPU::Vertex2f( selectionRect.x2, selectionRect.y2 );
        GL_GPU::Vertex2f( selectionRect.x2, selectionRect.y1 );
        GL_GPU::End();


        GL_GPU::LineWidth(1.5);

        GL_GPU::Color4f(0.5, 0.5, 0.5, 1.);
        GL_GPU::Begin(GL_LINE_LOOP);
        GL_GPU::Vertex2f( selectionRect.x1, selectionRect.y1 );
        GL_GPU::Vertex2f( selectionRect.x1, selectionRect.y2 );
        GL_GPU::Vertex2f( selectionRect.x2, selectionRect.y2 );
        GL_GPU::Vertex2f( selectionRect.x2, selectionRect.y1 );
        GL_GPU::End();

        glCheckError(GL_GPU);
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
}


void
AnimationModuleViewPrivate::drawSelectedKeyFramesBbox(bool isCurveEditor)
{
    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::Enable(GL_LINE_SMOOTH);
        GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        const ZoomContext* ctx = isCurveEditor ? &curveEditorZoomContext : &dopeSheetZoomContext;
        RectD selectedKeysBRect = isCurveEditor ? curveEditorSelectedKeysBRect : dopeSheetSelectedKeysBRect;

        QPointF topLeftWidget = ctx->toWidgetCoordinates( selectedKeysBRect.x1, selectedKeysBRect.y2 );
        QPointF btmRightWidget = ctx->toWidgetCoordinates( selectedKeysBRect.x2, selectedKeysBRect.y1 );
        double xMid = ( selectedKeysBRect.x1 + selectedKeysBRect.x2 ) / 2.;
        double yMid = ( selectedKeysBRect.y1 + selectedKeysBRect.y2 ) / 2.;

        GL_GPU::LineWidth(1.5);

        GL_GPU::Color4f(0.5, 0.5, 0.5, 1.);
        GL_GPU::Begin(GL_LINE_LOOP);
        GL_GPU::Vertex2f( selectedKeysBRect.x1, selectedKeysBRect.y1 );
        GL_GPU::Vertex2f( selectedKeysBRect.x1, selectedKeysBRect.y2 );
        GL_GPU::Vertex2f( selectedKeysBRect.x2, selectedKeysBRect.y2 );
        GL_GPU::Vertex2f( selectedKeysBRect.x2, selectedKeysBRect.y1 );
        GL_GPU::End();

        QPointF middleWidgetCoord = ctx->toWidgetCoordinates( xMid, yMid );
        QPointF middleLeft = ctx->toZoomCoordinates( middleWidgetCoord.x() - TO_DPIX(XHAIR_SIZE), middleWidgetCoord.y() );
        QPointF middleRight = ctx->toZoomCoordinates( middleWidgetCoord.x() + TO_DPIX(XHAIR_SIZE), middleWidgetCoord.y() );
        QPointF middleTop = ctx->toZoomCoordinates(middleWidgetCoord.x(), middleWidgetCoord.y() - TO_DPIX(XHAIR_SIZE));
        QPointF middleBottom = ctx->toZoomCoordinates(middleWidgetCoord.x(), middleWidgetCoord.y() + TO_DPIX(XHAIR_SIZE));



        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2f( std::max( middleLeft.x(), selectedKeysBRect.x1 ), middleLeft.y() );
        GL_GPU::Vertex2f( std::min( middleRight.x(), selectedKeysBRect.x2 ), middleRight.y() );
        GL_GPU::Vertex2f( middleBottom.x(), std::max( middleBottom.y(), selectedKeysBRect.y1 ) );
        GL_GPU::Vertex2f( middleTop.x(), std::min( middleTop.y(), selectedKeysBRect.y2 ) );

        //top tick
        if (isCurveEditor) {
            double yBottom = ctx->toZoomCoordinates(0, topLeftWidget.y() + TO_DPIX(BOUNDING_BOX_HANDLE_SIZE)).y();
            double yTop = ctx->toZoomCoordinates(0, topLeftWidget.y() - TO_DPIX(BOUNDING_BOX_HANDLE_SIZE)).y();
            GL_GPU::Vertex2f(xMid, yBottom);
            GL_GPU::Vertex2f(xMid, yTop);
        }
        //left tick
        {
            double xLeft = ctx->toZoomCoordinates(topLeftWidget.x() - TO_DPIX(BOUNDING_BOX_HANDLE_SIZE), 0).x();
            double xRight = ctx->toZoomCoordinates(topLeftWidget.x() + TO_DPIX(BOUNDING_BOX_HANDLE_SIZE), 0).x();
            GL_GPU::Vertex2f(xLeft, yMid);
            GL_GPU::Vertex2f(xRight, yMid);
        }
        //bottom tick
        if (isCurveEditor) {
            double yBottom = ctx->toZoomCoordinates(0, btmRightWidget.y() + TO_DPIX(BOUNDING_BOX_HANDLE_SIZE)).y();
            double yTop = ctx->toZoomCoordinates(0, btmRightWidget.y() - TO_DPIX(BOUNDING_BOX_HANDLE_SIZE)).y();
            GL_GPU::Vertex2f(xMid, yBottom);
            GL_GPU::Vertex2f(xMid, yTop);
        }
        //right tick
        {
            double xLeft = ctx->toZoomCoordinates(btmRightWidget.x() - TO_DPIX(BOUNDING_BOX_HANDLE_SIZE), 0).x();
            double xRight = ctx->toZoomCoordinates(btmRightWidget.x() + TO_DPIX(BOUNDING_BOX_HANDLE_SIZE), 0).x();
            GL_GPU::Vertex2f(xLeft, yMid);
            GL_GPU::Vertex2f(xRight, yMid);
        }
        GL_GPU::End();

        GL_GPU::PointSize(TO_DPIX(BOUNDING_BOX_HANDLE_SIZE));
        std::vector<Point> ptsToDraw;
        if (isCurveEditor) {
            Point p = {selectedKeysBRect.x1, selectedKeysBRect.y2};
            ptsToDraw.push_back(p);
        }
        if (isCurveEditor) {
            Point p = {selectedKeysBRect.x2, selectedKeysBRect.y2};
            ptsToDraw.push_back(p);
        }
        if (isCurveEditor) {
            Point p = {selectedKeysBRect.x1, selectedKeysBRect.y1};
            ptsToDraw.push_back(p);
        }
        if (isCurveEditor) {
            Point p = {selectedKeysBRect.x2, selectedKeysBRect.y1};
            ptsToDraw.push_back(p);
        }
        if (!ptsToDraw.empty()) {
            GL_GPU::Begin(GL_POINTS);
            for (std::size_t i = 0; i < ptsToDraw.size(); ++i) {
                GL_GPU::Vertex2d(ptsToDraw[i].x, ptsToDraw[i].y);
            }
            GL_GPU::End();
        }

        glCheckError(GL_GPU);
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
} // AnimationModuleViewPrivate::drawSelectedKeyFramesBbox


void
AnimationModuleViewPrivate::drawSelectionRect() const
{

    // Perform drawing
    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_LINE_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::Enable(GL_LINE_SMOOTH);
        GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        GL_GPU::Color4f(0.3, 0.3, 0.3, 0.2);

        // Draw rect
        GL_GPU::Begin(GL_POLYGON);
        GL_GPU::Vertex2f(selectionRect.x1, selectionRect.y1);
        GL_GPU::Vertex2f(selectionRect.x1, selectionRect.y2);
        GL_GPU::Vertex2f(selectionRect.x2, selectionRect.y2);
        GL_GPU::Vertex2f(selectionRect.x2, selectionRect.y1);
        GL_GPU::End();

        GL_GPU::LineWidth(1.5);

        // Draw outline
        GL_GPU::Color4f(0.5, 0.5, 0.5, 1.);
        GL_GPU::Begin(GL_LINE_LOOP);
        GL_GPU::Vertex2f(selectionRect.x1, selectionRect.y1);
        GL_GPU::Vertex2f(selectionRect.x1, selectionRect.y2);
        GL_GPU::Vertex2f(selectionRect.x2, selectionRect.y2);
        GL_GPU::Vertex2f(selectionRect.x2, selectionRect.y1);
        GL_GPU::End();
        
        glCheckError(GL_GPU);
    }
}

void
AnimationModuleViewPrivate::drawTexturedKeyframe(AnimationModuleViewPrivate::KeyframeTexture textureType, const RectD &rect, bool drawDimed) const
{

    double alpha = drawDimed ? 0.5 : 1.;
    GL_GPU::Color4f(1, 1, 1, alpha);

    GL_GPU::Enable(GL_TEXTURE_2D);
    GL_GPU::BindTexture(GL_TEXTURE_2D, kfTexturesIDs[textureType]);
    
    GL_GPU::Begin(GL_POLYGON);
    GL_GPU::TexCoord2f(0.0f, 1.0f);
    GL_GPU::Vertex2f( rect.left(), rect.top() );
    GL_GPU::TexCoord2f(0.0f, 0.0f);
    GL_GPU::Vertex2f( rect.left(), rect.bottom() );
    GL_GPU::TexCoord2f(1.0f, 0.0f);
    GL_GPU::Vertex2f( rect.right(), rect.bottom() );
    GL_GPU::TexCoord2f(1.0f, 1.0f);
    GL_GPU::Vertex2f( rect.right(), rect.top() );
    GL_GPU::End();
    glCheckError(GL_GPU);

    GL_GPU::BindTexture(GL_TEXTURE_2D, 0);

    GL_GPU::Disable(GL_TEXTURE_2D);
    glCheckErrorIgnoreOSXBug(GL_GPU);
    GL_GPU::Color4f(1, 1, 1, 1);

}

void
AnimationModuleViewPrivate::drawKeyFrameTime(const ZoomContext& ctx,  TimeValue time, const QColor& textColor, const RectD& rect) const
{
    QString text = QString::number(time);
    QPointF p = ctx.toWidgetCoordinates( rect.right(), rect.bottom() );
    p.rx() += 3;
    p = ctx.toZoomCoordinates( p.x(), p.y() );
    renderText(ctx, p.x(), p.y(), text, textColor, _publicInterface->font());
}

void
AnimationModuleViewPrivate::renderText(const ZoomContext& ctx,
                                       double x,
                                      double y,
                                      const QString & text,
                                      const QColor & color,
                                      const QFont & font,
                                      int flags) const
{
    if ( text.isEmpty() ) {
        return;
    }

    double w = (double)_publicInterface->width();
    double h = (double)_publicInterface->height();
    double bottom = ctx.bottom();
    double left = ctx.left();
    double top =  ctx.top();
    double right = ctx.right();
    if ( (w <= 0) || (h <= 0) || (right <= left) || (top <= bottom) ) {
        return;
    }
    double scalex = (right - left) / w;
    double scaley = (top - bottom) / h;
    textRenderer.renderText(x, y, scalex, scaley, text, color, font, flags);
    glCheckError(GL_GPU);
}


bool
AnimationModuleViewPrivate::isNearbyTimelineTopPoly(const QPoint & pt) const
{
    TimeLinePtr timeline = _model.lock()->getTimeline();
    if (!timeline) {
        return false;
    }

    QPointF pt_opengl = curveEditorZoomContext.toZoomCoordinates( pt.x(), pt.y() );

    QPointF topLeft = curveEditorZoomContext.toZoomCoordinates(0, 0);
    QPointF topCursortop( timeline->currentFrame(), topLeft.y() );
    QPointF topcursorTopWidgetCoord = curveEditorZoomContext.toWidgetCoordinates( topCursortop.x(), topCursortop.y() );
    QPointF topCursorBtm = curveEditorZoomContext.toZoomCoordinates(topcursorTopWidgetCoord.x(), topcursorTopWidgetCoord.y() + TO_DPIY(CURSOR_HEIGHT));
    QPointF topCursorLeft = curveEditorZoomContext.toZoomCoordinates( topcursorTopWidgetCoord.x() - TO_DPIX(CURSOR_WIDTH) / 2., topcursorTopWidgetCoord.y() );
    QPointF topCursorRight = curveEditorZoomContext.toZoomCoordinates( topcursorTopWidgetCoord.x() + TO_DPIX(CURSOR_WIDTH) / 2., topcursorTopWidgetCoord.y() );

    QPolygonF poly;
    poly.push_back(topCursorBtm);
    poly.push_back(topCursorLeft);
    poly.push_back(topCursorRight);

    return poly.containsPoint(pt_opengl, Qt::OddEvenFill);
}

bool
AnimationModuleViewPrivate::isNearbyTimelineBtmPoly(const QPoint & pt) const
{
    TimeLinePtr timeline = _model.lock()->getTimeline();
    if (!timeline) {
        return false;
    }
    QPointF pt_opengl = curveEditorZoomContext.toZoomCoordinates( pt.x(), pt.y() );

    QPointF btmRight = curveEditorZoomContext.toZoomCoordinates(_publicInterface->width() - 1, _publicInterface->height() - 1);
    QPointF btmCursorBtm( timeline->currentFrame(), btmRight.y() );
    QPointF btmcursorBtmWidgetCoord = curveEditorZoomContext.toWidgetCoordinates( btmCursorBtm.x(), btmCursorBtm.y() );
    QPointF btmCursorTop = curveEditorZoomContext.toZoomCoordinates(btmcursorBtmWidgetCoord.x(), btmcursorBtmWidgetCoord.y() - TO_DPIY(CURSOR_HEIGHT));
    QPointF btmCursorLeft = curveEditorZoomContext.toZoomCoordinates( btmcursorBtmWidgetCoord.x() - TO_DPIX(CURSOR_WIDTH) / 2., btmcursorBtmWidgetCoord.y() );
    QPointF btmCursorRight = curveEditorZoomContext.toZoomCoordinates( btmcursorBtmWidgetCoord.x() + TO_DPIX(CURSOR_WIDTH) / 2., btmcursorBtmWidgetCoord.y() );

    QPolygonF poly;
    poly.push_back(btmCursorTop);
    poly.push_back(btmCursorLeft);
    poly.push_back(btmCursorRight);

    return poly.containsPoint(pt_opengl, Qt::OddEvenFill);
}

bool
AnimationModuleViewPrivate::isNearbySelectedKeyFramesCrossWidget(const ZoomContext& ctx, const RectD& rect, const QPoint & pt) const
{
    double xMid = ( rect.x1 + rect.x2 ) / 2.;
    double yMid = ( rect.y1 + rect.y2 ) / 2.;

    QPointF middleWidgetCoord = ctx.toWidgetCoordinates( xMid, yMid );
    QPointF middleLeft = QPointF(middleWidgetCoord.x() - TO_DPIX(XHAIR_SIZE), middleWidgetCoord.y() );
    QPointF middleRight = QPointF( middleWidgetCoord.x() + TO_DPIX(XHAIR_SIZE), middleWidgetCoord.y() );
    QPointF middleTop = QPointF(middleWidgetCoord.x(), middleWidgetCoord.y() - TO_DPIX(XHAIR_SIZE));
    QPointF middleBottom = QPointF(middleWidgetCoord.x(), middleWidgetCoord.y() + TO_DPIX(XHAIR_SIZE));


    if ( ( pt.x() >= (middleLeft.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.x() <= (middleRight.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() <= (middleLeft.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() >= (middleLeft.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        //is nearby horizontal line
        return true;
    } else if ( ( pt.y() >= (middleBottom.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
               ( pt.y() <= (middleTop.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
               ( pt.x() <= (middleBottom.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
               ( pt.x() >= (middleBottom.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        //is nearby vertical line
        return true;
    } else {
        return false;
    }
}

bool
AnimationModuleViewPrivate::isNearbyBboxTopLeft(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const
{
    QPointF other = ctx.toWidgetCoordinates( rect.x1, rect.y2 );

    if ( ( pt.x() >= (other.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.x() <= (other.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() <= (other.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() >= (other.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        return true;
    }

    return false;
}

bool
AnimationModuleViewPrivate::isNearbyBboxMidLeft(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const
{
    QPointF other = ctx.toWidgetCoordinates(rect.x1,
                                                rect.y2 - (rect.height()) / 2.);

    if ( ( pt.x() >= (other.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.x() <= (other.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() <= (other.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() >= (other.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        return true;
    }

    return false;
}

bool
AnimationModuleViewPrivate::isNearbyBboxBtmLeft(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const
{
    QPointF other = ctx.toWidgetCoordinates( rect.x1, rect.y1);

    if ( ( pt.x() >= (other.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.x() <= (other.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() <= (other.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() >= (other.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        return true;
    }

    return false;
}

bool
AnimationModuleViewPrivate::isNearbyBboxMidBtm(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const
{
    QPointF other = ctx.toWidgetCoordinates( rect.x1 + rect.width() / 2., rect.y1 );

    if ( ( pt.x() >= (other.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.x() <= (other.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() <= (other.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() >= (other.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        return true;
    }

    return false;
}

bool
AnimationModuleViewPrivate::isNearbyBboxBtmRight(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const
{
    QPointF other = ctx.toWidgetCoordinates( rect.x2, rect.y1);

    if ( ( pt.x() >= (other.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.x() <= (other.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() <= (other.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() >= (other.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        return true;
    }

    return false;
}

bool
AnimationModuleViewPrivate::isNearbyBboxMidRight(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const
{
    QPointF other = ctx.toWidgetCoordinates(rect.x2, rect.y1 + rect.height() / 2.);

    if ( ( pt.x() >= (other.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.x() <= (other.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() <= (other.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() >= (other.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        return true;
    }

    return false;
}

bool
AnimationModuleViewPrivate::isNearbyBboxTopRight(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const
{
    QPointF other = ctx.toWidgetCoordinates( rect.x2, rect.y2 );

    if ( ( pt.x() >= (other.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.x() <= (other.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() <= (other.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() >= (other.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        return true;
    }

    return false;
}

bool
AnimationModuleViewPrivate::isNearbyBboxMidTop(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const
{
    QPointF other = ctx.toWidgetCoordinates( rect.x1 + rect.width() / 2., rect.y2 );

    if ( ( pt.x() >= (other.x() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.x() <= (other.x() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() <= (other.y() + TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) &&
        ( pt.y() >= (other.y() - TO_DPIX(CLICK_DISTANCE_TOLERANCE)) ) ) {
        return true;
    }
    
    return false;
}

void
AnimationModuleViewPrivate::generateKeyframeTextures()
{
    QImage kfTexturesImages[KF_TEXTURES_COUNT];

    kfTexturesImages[0].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_constant.png") );
    kfTexturesImages[1].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_constant_selected.png") );
    kfTexturesImages[2].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_linear.png") );
    kfTexturesImages[3].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_linear_selected.png") );
    kfTexturesImages[4].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve.png") );
    kfTexturesImages[5].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_selected.png") );
    kfTexturesImages[6].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_break.png") );
    kfTexturesImages[7].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_break_selected.png") );
    kfTexturesImages[8].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_c.png") );
    kfTexturesImages[9].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_c_selected.png") );
    kfTexturesImages[10].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_h.png") );
    kfTexturesImages[11].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_h_selected.png") );
    kfTexturesImages[12].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_r.png") );
    kfTexturesImages[13].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_r_selected.png") );
    kfTexturesImages[14].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_z.png") );
    kfTexturesImages[15].load( QString::fromUtf8(NATRON_IMAGES_PATH "interp_curve_z_selected.png") );
    kfTexturesImages[16].load( QString::fromUtf8(NATRON_IMAGES_PATH "keyframe_node_root.png") );
    kfTexturesImages[17].load( QString::fromUtf8(NATRON_IMAGES_PATH "keyframe_node_root_selected.png") );

    int keyFrameTextureSize = TO_DPIX(getKeyframeTextureSize());

    GL_GPU::GenTextures(KF_TEXTURES_COUNT, kfTexturesIDs);

    GL_GPU::Enable(GL_TEXTURE_2D);

    for (int i = 0; i < KF_TEXTURES_COUNT; ++i) {
        if (std::max( kfTexturesImages[i].width(), kfTexturesImages[i].height() ) != keyFrameTextureSize) {
            kfTexturesImages[i] = kfTexturesImages[i].scaled(keyFrameTextureSize, keyFrameTextureSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        kfTexturesImages[i] = QGLWidget::convertToGLFormat(kfTexturesImages[i]);
        GL_GPU::BindTexture(GL_TEXTURE_2D, kfTexturesIDs[i]);

        GL_GPU::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL_GPU::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        GL_GPU::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        GL_GPU::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GL_GPU::TexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, keyFrameTextureSize, keyFrameTextureSize, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, kfTexturesImages[i].bits() );
    }

    GL_GPU::BindTexture(GL_TEXTURE_2D, 0);
    GL_GPU::Disable(GL_TEXTURE_2D);
}

AnimationModuleViewPrivate::KeyframeTexture
AnimationModuleViewPrivate::kfTextureFromKeyframeType(KeyframeTypeEnum kfType,
                                                bool selected) 
{
    AnimationModuleViewPrivate::KeyframeTexture ret = AnimationModuleViewPrivate::kfTextureNone;

    switch (kfType) {
        case eKeyframeTypeConstant:
            ret = (selected) ? AnimationModuleViewPrivate::kfTextureInterpConstantSelected : AnimationModuleViewPrivate::kfTextureInterpConstant;
            break;
        case eKeyframeTypeLinear:
            ret = (selected) ? AnimationModuleViewPrivate::kfTextureInterpLinearSelected : AnimationModuleViewPrivate::kfTextureInterpLinear;
            break;
        case eKeyframeTypeBroken:
            ret = (selected) ? AnimationModuleViewPrivate::kfTextureInterpBreakSelected : AnimationModuleViewPrivate::kfTextureInterpBreak;
            break;
        case eKeyframeTypeFree:
            ret = (selected) ? AnimationModuleViewPrivate::kfTextureInterpCurveSelected : AnimationModuleViewPrivate::kfTextureInterpCurve;
            break;
        case eKeyframeTypeSmooth:
            ret = (selected) ? AnimationModuleViewPrivate::kfTextureInterpCurveZSelected : AnimationModuleViewPrivate::kfTextureInterpCurveZ;
            break;
        case eKeyframeTypeCatmullRom:
            ret = (selected) ? AnimationModuleViewPrivate::kfTextureInterpCurveRSelected : AnimationModuleViewPrivate::kfTextureInterpCurveR;
            break;
        case eKeyframeTypeCubic:
            ret = (selected) ? AnimationModuleViewPrivate::kfTextureInterpCurveCSelected : AnimationModuleViewPrivate::kfTextureInterpCurveC;
            break;
        case eKeyframeTypeHorizontal:
            ret = (selected) ? AnimationModuleViewPrivate::kfTextureInterpCurveHSelected : AnimationModuleViewPrivate::kfTextureInterpCurveH;
            break;
        default:
            ret = AnimationModuleViewPrivate::kfTextureNone;
            break;
    }
    
    return ret;
}

std::vector<CurveGuiPtr>
AnimationModuleViewPrivate::getSelectedCurves() const
{
    std::vector<CurveGuiPtr> curves;

    const AnimItemDimViewKeyFramesMap& keys = _model.lock()->getSelectionModel()->getCurrentKeyFramesSelection();
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (guiCurve) {
            curves.push_back(guiCurve);
        }
    }
    return curves;
}

std::vector<CurveGuiPtr>
AnimationModuleViewPrivate::getVisibleCurves() const
{
    AnimItemDimViewKeyFramesMap keys;
    AnimationModuleBasePtr model = _model.lock();
    model->getSelectionModel()->getAllItems(false, &keys, 0, 0);
    std::vector<CurveGuiPtr> curves;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        if (!model->isCurveVisible(it->first.item, it->first.dim, it->first.view)) {
            continue;
        }
        CurveGuiPtr guiCurve = it->first.item->getCurveGui(it->first.dim, it->first.view);
        if (guiCurve) {
            curves.push_back(guiCurve);
        }
    }
    return curves;
}


void
AnimationModuleViewPrivate::createMenu(bool isCurveWidget, const QPoint& globalPos)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    Menu rightClickMenu(_publicInterface);

    Menu* editMenu = new Menu(&rightClickMenu);
    //editMenu->setFont( QFont(appFont,appFontSize) );
    editMenu->setTitle( tr("Edit") );
    rightClickMenu.addAction( editMenu->menuAction() );

    Menu* interpMenu = new Menu(&rightClickMenu);
    //interpMenu->setFont( QFont(appFont,appFontSize) );
    interpMenu->setTitle( tr("Interpolation") );
    rightClickMenu.addAction( interpMenu->menuAction() );

    Menu* viewMenu = new Menu(&rightClickMenu);
    //viewMenu->setFont( QFont(appFont,appFontSize) );
    viewMenu->setTitle( tr("View") );
    rightClickMenu.addAction( viewMenu->menuAction() );


    Menu* optionsMenu = new Menu(&rightClickMenu);
    optionsMenu->setTitle( tr("Options") );
    rightClickMenu.addAction( optionsMenu->menuAction() );


    QAction* deleteKeyFramesAction = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleRemoveKeys,
                                                            kShortcutActionAnimationModuleRemoveKeysLabel, editMenu);
    deleteKeyFramesAction->setShortcut( QKeySequence(Qt::Key_Backspace) );
    QObject::connect( deleteKeyFramesAction, SIGNAL(triggered()), _publicInterface, SLOT(onRemoveSelectedKeyFramesActionTriggered()) );
    editMenu->addAction(deleteKeyFramesAction);

    QAction* copyKeyFramesAction = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCopy,
                                                          kShortcutActionAnimationModuleCopyLabel, editMenu);
    copyKeyFramesAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_C) );

    QObject::connect( copyKeyFramesAction, SIGNAL(triggered()), _publicInterface, SLOT(onCopySelectedKeyFramesToClipBoardActionTriggered()) );
    editMenu->addAction(copyKeyFramesAction);

    QAction* pasteKeyFramesAction = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModulePasteKeyframes,
                                                           kShortcutActionAnimationModulePasteKeyframesLabel, editMenu);
    pasteKeyFramesAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_V) );
    QObject::connect( pasteKeyFramesAction, SIGNAL(triggered()), _publicInterface, SLOT(onPasteClipBoardKeyFramesActionTriggered()) );
    editMenu->addAction(pasteKeyFramesAction);

    QAction* selectAllAction = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleSelectAll,
                                                      kShortcutActionAnimationModuleSelectAllLabel, editMenu);
    selectAllAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_A) );
    QObject::connect( selectAllAction, SIGNAL(triggered()), _publicInterface, SLOT(onSelectAllKeyFramesActionTriggered()) );
    editMenu->addAction(selectAllAction);


    QAction* constantInterp = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleConstant,
                                                     kShortcutActionAnimationModuleConstantLabel, interpMenu);
    constantInterp->setShortcut( QKeySequence(Qt::Key_K) );
    constantInterp->setData((int)eKeyframeTypeConstant);
    QObject::connect( constantInterp, SIGNAL(triggered()), _publicInterface, SLOT(onSetInterpolationActionTriggered()) );
    interpMenu->addAction(constantInterp);

    QAction* linearInterp = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleLinear,
                                                   kShortcutActionAnimationModuleLinearLabel, interpMenu);
    linearInterp->setShortcut( QKeySequence(Qt::Key_L) );
    linearInterp->setData((int)eKeyframeTypeLinear);
    QObject::connect( linearInterp, SIGNAL(triggered()), _publicInterface, SLOT(onSetInterpolationActionTriggered()) );
    interpMenu->addAction(linearInterp);


    QAction* smoothInterp = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleSmooth,
                                                   kShortcutActionAnimationModuleSmoothLabel, interpMenu);
    smoothInterp->setShortcut( QKeySequence(Qt::Key_Z) );
    smoothInterp->setData((int)eKeyframeTypeSmooth);
    QObject::connect( smoothInterp, SIGNAL(triggered()), _publicInterface, SLOT(onSetInterpolationActionTriggered()) );
    interpMenu->addAction(smoothInterp);


    QAction* catmullRomInterp = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCatmullrom,
                                                       kShortcutActionAnimationModuleCatmullromLabel, interpMenu);
    catmullRomInterp->setShortcut( QKeySequence(Qt::Key_R) );
    catmullRomInterp->setData((int)eKeyframeTypeCatmullRom);
    QObject::connect( catmullRomInterp, SIGNAL(triggered()), _publicInterface, SLOT(onSetInterpolationActionTriggered()) );
    interpMenu->addAction(catmullRomInterp);


    QAction* cubicInterp = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCubic,
                                                  kShortcutActionAnimationModuleCubicLabel, interpMenu);
    cubicInterp->setShortcut( QKeySequence(Qt::Key_C) );
    cubicInterp->setData((int)eKeyframeTypeCubic);
    QObject::connect( cubicInterp, SIGNAL(triggered()), _publicInterface, SLOT(onSetInterpolationActionTriggered()) );
    interpMenu->addAction(cubicInterp);

    QAction* horizontalInterp = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleHorizontal,
                                                       kShortcutActionAnimationModuleHorizontalLabel, interpMenu);
    horizontalInterp->setShortcut( QKeySequence(Qt::Key_H) );
    horizontalInterp->setData((int)eKeyframeTypeHorizontal);
    QObject::connect( horizontalInterp, SIGNAL(triggered()), _publicInterface, SLOT(onSetInterpolationActionTriggered()) );
    interpMenu->addAction(horizontalInterp);


    QAction* breakDerivatives = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleBreak,
                                                       kShortcutActionAnimationModuleBreakLabel, interpMenu);
    breakDerivatives->setShortcut( QKeySequence(Qt::Key_X) );
    breakDerivatives->setData((int)eKeyframeTypeBroken);
    QObject::connect( breakDerivatives, SIGNAL(triggered()), _publicInterface, SLOT(onSetInterpolationActionTriggered()) );
    interpMenu->addAction(breakDerivatives);

    QAction* frameAll = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCenterAll,
                                               kShortcutActionAnimationModuleCenterAllLabel, interpMenu);
    frameAll->setShortcut( QKeySequence(Qt::Key_A) );
    QObject::connect( frameAll, SIGNAL(triggered()), _publicInterface, SLOT(onCenterAllCurvesActionTriggered()) );
    viewMenu->addAction(frameAll);

    QAction* frameCurve = new ActionWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCenter,
                                                 kShortcutActionAnimationModuleCenterLabel, interpMenu);
    frameCurve->setShortcut( QKeySequence(Qt::Key_F) );
    QObject::connect( frameCurve, SIGNAL(triggered()), _publicInterface, SLOT(onCenterOnSelectedCurvesActionTriggered()) );
    viewMenu->addAction(frameCurve);


    QAction* updateOnPenUp = new QAction(tr("Update on mouse release only"), &rightClickMenu);
    updateOnPenUp->setCheckable(true);
    updateOnPenUp->setChecked( appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly() );
    optionsMenu->addAction(updateOnPenUp);
    QObject::connect( updateOnPenUp, SIGNAL(triggered()), _publicInterface, SLOT(onUpdateOnPenUpActionTriggered()) );

    if (isCurveWidget) {
        addMenuCurveEditorMenuOptions(&rightClickMenu);
    }

    rightClickMenu.exec(globalPos);
    
} // createMenu



void
AnimationModuleViewPrivate::moveSelectedKeyFrames(const QPointF & oldCanonicalPos,  const QPointF & newCanonicalPos)
{

    AnimationModuleBasePtr model = _model.lock();
    const AnimItemDimViewKeyFramesMap& keys = model->getSelectionModel()->getCurrentKeyFramesSelection();
    const std::list<NodeAnimPtr>& selectedNodes = model->getSelectionModel()->getCurrentNodesSelection();

    // Animation curves can only be moved on integer X values, hence clamp the motion
    bool clampXToIntegers = false;
    bool clampYToIntegers = false;
    bool canMoveY = true;

    QPointF dragStartPointOpenGL;
    if (eventTriggeredFromCurveEditor) {
        dragStartPointOpenGL = curveEditorZoomContext.toZoomCoordinates( dragStartPoint.x(), dragStartPoint.y() );
    } else {
        dragStartPointOpenGL = dopeSheetZoomContext.toZoomCoordinates( dragStartPoint.x(), dragStartPoint.y() );
    }
    QPointF deltaSinceDragStart;
    deltaSinceDragStart.rx() = newCanonicalPos.x() - dragStartPointOpenGL.x();
    deltaSinceDragStart.ry() = newCanonicalPos.y() - dragStartPointOpenGL.y();

    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        CurvePtr curve = it->first.item->getCurve(it->first.dim, it->first.view);
        if (!curve) {
            continue;
        }
        if (curve->areKeyFramesTimeClampedToIntegers()) {
            clampXToIntegers = true;
        }
        if (!curve->isYComponentMovable()) {
            canMoveY = false;
        }

        // Also round the motion in Y if the curve represent a boolean or integer curve
        if ( curve->areKeyFramesValuesClampedToBooleans() ) {
            deltaSinceDragStart.ry() = std::max( 0., std::min(std::floor(deltaSinceDragStart.y() + 0.5), 1.) );
        } else if ( curve->areKeyFramesValuesClampedToIntegers() ) {
            clampYToIntegers = true;
        }
    }

    if (!clampXToIntegers) {
        // Check if there's a node that needs clamping to integers aswell
        for (std::list<NodeAnimPtr>::const_iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
            if ((*it)->isRangeDrawingEnabled()) {
                clampXToIntegers = true;
                break;
            }
        }
    }

    if (clampXToIntegers) {
        // Round to the nearest integer the keyframes total motion (in X only)
        deltaSinceDragStart.rx() = std::floor(deltaSinceDragStart.x() + 0.5);
    }
    if (clampYToIntegers) {
        deltaSinceDragStart.ry() = std::floor(deltaSinceDragStart.y() + 0.5);
    }

    // Dopesheet can only move X
    if (!eventTriggeredFromCurveEditor) {
        canMoveY = false;
    }


    // Compute delta in time
    double dt = 0;
    if (mouseDragOrientation.x() != 0) {
        if (clampXToIntegers) {
            // We clamp the whole motion and not just the delta between the last pos and the new position otherwise dt would always be clamped
            dt = deltaSinceDragStart.x() - keyDragLastMovement.x();
            // And update _keyDragLastMovement
            keyDragLastMovement.rx() = deltaSinceDragStart.x();
        } else {
            dt = newCanonicalPos.x() - oldCanonicalPos.x();
        }
    }


    // Compute delta in value
    double dv = 0;
    if (canMoveY) {
        if (mouseDragOrientation.y() != 0) {
            if (clampYToIntegers) {
                // We clamp the whole motion and not just the delta between the last pos and the new position otherwise dv would always be clamped
                dv = deltaSinceDragStart.y() - keyDragLastMovement.y();
                keyDragLastMovement.ry() = deltaSinceDragStart.y();
            } else {
                dv = newCanonicalPos.y() - oldCanonicalPos.y();
            }
        }

    }

    _model.lock()->moveSelectedKeysAndNodes(dt, dv);
} // moveSelectedKeyFrames



void
AnimationModuleViewPrivate::transformSelectedKeyFrames(const QPointF & oldPosCanonical,  const QPointF & newPosCanonical, bool shiftHeld, TransformSelectionCenterEnum centerType, bool scaleX, bool scaleY)
{
    if (newPosCanonical == oldPosCanonical) {
        return;
    }

    double dt = newPosCanonical.x() - oldPosCanonical.x();
    double dv = newPosCanonical.y() - oldPosCanonical.y();

    switch (centerType) {
        case eTransformSelectionCenterBottom:
        case eTransformSelectionCenterTop:
            dt = 0;
            break;
        case eTransformSelectionCenterLeft:
        case eTransformSelectionCenterRight:
            dv = 0;
        default:
            break;
    }

    if (dt == 0 && dv == 0) {
        return;
    }

    QPointF center((curveEditorSelectedKeysBRect.x1 + curveEditorSelectedKeysBRect.x2) / 2., (curveEditorSelectedKeysBRect.y1 + curveEditorSelectedKeysBRect.y2) / 2.);

    if (shiftHeld) {
        switch (centerType) {
            case eTransformSelectionCenterBottom:
                center.ry() = curveEditorSelectedKeysBRect.y1;
                break;
            case eTransformSelectionCenterLeft:
                center.rx() = curveEditorSelectedKeysBRect.x2;
                break;
            case eTransformSelectionCenterRight:
                center.rx() = curveEditorSelectedKeysBRect.x1;
                break;
            case eTransformSelectionCenterTop:
                center.ry() = curveEditorSelectedKeysBRect.y2;
                break;
            case eTransformSelectionCenterMiddle:
                break;
        }
    }
    double sx = 1., sy = 1.;
    double tx = 0., ty = 0.;
    double oldX = newPosCanonical.x() - dt;
    double oldY = newPosCanonical.y() - dv;
    // the scale ratio is the ratio of distances to the center
    double prevDist = ( oldX - center.x() ) * ( oldX - center.x() ) + ( oldY - center.y() ) * ( oldY - center.y() );

    if (prevDist != 0) {
        double dist = ( newPosCanonical.x() - center.x() ) * ( newPosCanonical.x() - center.x() ) + ( newPosCanonical.y() - center.y() ) * ( newPosCanonical.y() - center.y() );
        double ratio = std::sqrt(dist / prevDist);

        if (scaleX) {
            sx *= ratio;
        }
        if (scaleY) {
            sy *= ratio;
        }
    }
    Transform::Matrix3x3 transform =  Transform::matTransformCanonical(tx, ty, sx, sy, 0, 0, true, 0, center.x(), center.y());
    _model.lock()->transformSelectedKeys(transform);


} // transformSelectedKeyFrames


void
AnimationModuleViewPrivate::moveCurrentFrameIndicator(int frame)
{
    AnimationModuleBasePtr animModel = _model.lock();
    if (!animModel) {
        return;
    }
    TimeLinePtr timeline = animModel->getTimeline();
    if (!timeline) {
        return;
    }
    if (!_gui) {
        return;
    }
    _gui->getApp()->setLastViewerUsingTimeline( NodePtr() );

    _gui->setDraftRenderEnabled(true);
    timeline->seekFrame(frame, false, EffectInstancePtr(), eTimelineChangeReasonAnimationModuleSeek);
}

void
AnimationModuleViewPrivate::zoomView(const QPointF& evPos, int deltaX, int deltaY, ZoomContext& zoomCtx, bool canZoomX,  bool canZoomY)
{
    const double zoomFactor_min = 0.0001;
    const double zoomFactor_max = 10000.;
    const double par_min = 0.0001;
    const double par_max = 10000.;
    double zoomFactor;
    double scaleFactorX = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaX); // no need to use ipow() here, because the result is not cast to int
    double scaleFactorY = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaY); // no need to use ipow() here, because the result is not cast to int


    if ( (zoomCtx.screenWidth() > 0) && (zoomCtx.screenHeight() > 0) ) {
        QPointF zoomCenter = zoomCtx.toZoomCoordinates( evPos.x(), evPos.y() );

        if (canZoomY) {
            // Alt + Shift + Wheel: zoom values only, keep point under mouse
            zoomFactor = zoomCtx.factor() * scaleFactorY;

            if (zoomFactor <= zoomFactor_min) {
                zoomFactor = zoomFactor_min;
                scaleFactorY = zoomFactor / zoomCtx.factor();
            } else if (zoomFactor > zoomFactor_max) {
                zoomFactor = zoomFactor_max;
                scaleFactorY = zoomFactor / zoomCtx.factor();
            }

            double par = zoomCtx.aspectRatio() / scaleFactorY;
            if (par <= par_min) {
                par = par_min;
                scaleFactorY = par / zoomCtx.aspectRatio();
            } else if (par > par_max) {
                par = par_max;
                scaleFactorY = par / zoomCtx.factor();
            }

            {
                QMutexLocker k(&zoomCtxMutex);
                zoomCtx.zoomy(zoomCenter.x(), zoomCenter.y(), scaleFactorY);
            }
        }

        if (canZoomX) {
            // Alt + Wheel: zoom time only, keep point under mouse
            double par = zoomCtx.aspectRatio() * scaleFactorX;
            if (par <= par_min) {
                par = par_min;
                scaleFactorX = par / zoomCtx.aspectRatio();
            } else if (par > par_max) {
                par = par_max;
                scaleFactorX = par / zoomCtx.factor();
            }

            {
                QMutexLocker k(&zoomCtxMutex);
                zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactorX);
            }
        }
    }
} // zoomView

NATRON_NAMESPACE_EXIT
