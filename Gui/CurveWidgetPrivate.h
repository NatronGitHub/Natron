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

#ifndef Gui_CurveWidgetPrivate_h
#define Gui_CurveWidgetPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QMutex>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h

#include "Engine/OfxOverlayInteract.h"


#include "Gui/AnimationModuleBase.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleUndoRedo.h"
#include "Gui/CurveGui.h"
#include "Gui/Menu.h"
#include "Gui/TextRenderer.h"
#include "Gui/ZoomContext.h"


#define CURVEWIDGET_DERIVATIVE_ROUND_PRECISION 3.

NATRON_NAMESPACE_ENTER;


/*****************************CURVE WIDGET***********************************************/

enum EventStateEnum
{
    eEventStateDraggingView = 0,
    eEventStateDraggingKeys,
    eEventStateSelecting,
    eEventStateDraggingTangent,
    eEventStateDraggingTimeline,
    eEventStateZooming,
    eEventStateDraggingTopLeftBbox,
    eEventStateDraggingMidLeftBbox,
    eEventStateDraggingBtmLeftBbox,
    eEventStateDraggingMidBtmBbox,
    eEventStateDraggingBtmRightBbox,
    eEventStateDraggingMidRightBbox,
    eEventStateDraggingTopRightBbox,
    eEventStateDraggingMidTopBbox,
    eEventStateNone
};

// although all members are public, CurveWidgetPrivate is really a class because it has lots of member functions
// (in fact, many members could probably be made private)
class CurveWidgetPrivate
{
    Q_DECLARE_TR_FUNCTIONS(CurveWidget)

public:
    CurveWidgetPrivate(Gui* gui,
                       const AnimationModuleBasePtr& model,
                       CurveWidget* widget);

    ~CurveWidgetPrivate();

    AnimationModuleBasePtr getModel() const
    {
        return _model.lock();
    }

    void drawSelectionRectangle();

    void drawTimelineMarkers();

    void drawCurves();

    void drawScale();

    void drawSelectedKeyFramesBbox();

    /**
     * @brief Returns whether the click at position pt is nearby the curve.
     * If so then the value x and y will be set to the position on the curve
     * if they are not NULL.
     **/
    CurveGuiPtr isNearbyCurve(const QPoint &pt, double* x = NULL, double *y = NULL) const;
    AnimItemDimViewAndTime isNearbyKeyFrame(const QPoint & pt) const;
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewAndTime > isNearbyTangent(const QPoint & pt) const;
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewAndTime > isNearbySelectedTangentText(const QPoint & pt) const;

    bool isNearbySelectedKeyFramesCrossWidget(const QPoint & pt) const;

    bool isNearbyBboxTopLeft(const QPoint& pt) const;
    bool isNearbyBboxMidLeft(const QPoint& pt) const;
    bool isNearbyBboxBtmLeft(const QPoint& pt) const;
    bool isNearbyBboxMidBtm(const QPoint& pt) const;
    bool isNearbyBboxBtmRight(const QPoint& pt) const;
    bool isNearbyBboxMidRight(const QPoint& pt) const;
    bool isNearbyBboxTopRight(const QPoint& pt) const;
    bool isNearbyBboxMidTop(const QPoint& pt) const;

    bool isNearbyTimelineTopPoly(const QPoint & pt) const;

    bool isNearbyTimelineBtmPoly(const QPoint & pt) const;

    AnimItemDimViewAndTime isNearbyKeyFrameText(const QPoint& pt) const;

    void moveSelectedKeyFrames(const QPointF & oldClick_opengl, const QPointF & newClick_opengl);

    void transformSelectedKeyFrames(const QPointF & oldClick_opengl, const QPointF & newClick_opengl, bool shiftHeld);

    void moveSelectedTangent(const QPointF & pos);

    void refreshSelectionRectangle(double x, double y);

    void setSelectedKeysInterpolation(KeyframeTypeEnum type);

    void createMenu();

    void updateDopeSheetViewFrameRange();

private:


    void keyFramesWithinRect(const QRectF & rect, AnimItemDimViewKeyFramesMap* keys) const;

public:

    // Model providing selection and curves
    AnimationModuleBaseWPtr _model;

    // If there's a custom interact to draw (for parametric parameters)
    boost::weak_ptr<OfxParamOverlayInteract> _customInteract;

    // the last mouse press or move, in widget coordinates
    QPoint _lastMousePos;

    // protects zoomCtx for serialization thread
    mutable QMutex zoomCtxMutex;
    ZoomContext zoomCtx;

    // Interaction State
    EventStateEnum _state;

    // Right click menu
    Menu* _rightClickMenu;


    TextRenderer textRenderer;
    bool _mustSetDragOrientation;
    QPoint _mouseDragOrientation; ///used to drag a key frame in only 1 direction (horizontal or vertical)
    ///the value is either (1,0) or (0,1)
    std::vector< KeyFrame > _keyFramesClipBoard;
    QRectF _selectionRectangle;
    QPointF _dragStartPoint;
    QRectF _selectedKeyFramesBbox;
    QLineF _selectedKeyFramesCrossVertLine;
    QLineF _selectedKeyFramesCrossHorizLine;
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewIndexID> _selectedDerivative;
    bool _evaluateOnPenUp; //< true if we must re-evaluate the nodes associated to the selected keyframes on penup
    QPointF _keyDragLastMovement;
    Gui* _gui;
    GLuint savedTexture;
    QSize sizeH;
    bool zoomOrPannedSinceLastFit;

    // True if paintGL() was run at least once
    bool drawnOnce;

    CurveWidget* _widget;


};

NATRON_NAMESPACE_EXIT;

#endif // Gui_CurveWidgetPrivate_h
