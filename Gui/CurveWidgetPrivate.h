/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#ifndef _Gui_CurveWidgetPrivate_h
#define _Gui_CurveWidgetPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <boost/shared_ptr.hpp>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/Macros.h"

#include "Gui/CurveEditorUndoRedo.h" // MoveTangentCommand
#include "Gui/CurveGui.h"
#include "Gui/Menu.h"
#include "Gui/TextRenderer.h"
#include "Gui/ZoomContext.h"

class Gui;
class CurveSelection;
class TimeLine;
class CurveWidget;

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

using namespace Natron;

#define CURVEWIDGET_DERIVATIVE_ROUND_PRECISION 3.

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

public:
    CurveWidgetPrivate(Gui* gui,
                       CurveSelection* selection,
                       boost::shared_ptr<TimeLine> timeline,
                       CurveWidget* widget);

    ~CurveWidgetPrivate();

    void drawSelectionRectangle();

    void refreshTimelinePositions();

    void drawTimelineMarkers();

    void drawCurves();

    void drawScale();
    void drawSelectedKeyFramesBbox();

    /**
     * @brief Returns whether the click at position pt is nearby the curve.
     * If so then the value x and y will be set to the position on the curve
     * if they are not NULL.
     **/
    Curves::const_iterator isNearbyCurve(const QPoint &pt,double* x = NULL,double *y = NULL) const;
    std::pair<boost::shared_ptr<CurveGui>,KeyFrame> isNearbyKeyFrame(const QPoint & pt) const;
    std::pair<MoveTangentCommand::SelectedTangentEnum, KeyPtr> isNearbyTangent(const QPoint & pt) const;
    std::pair<MoveTangentCommand::SelectedTangentEnum, KeyPtr> isNearbySelectedTangentText(const QPoint & pt) const;

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

    KeyPtr isNearbyKeyFrameText(const QPoint& pt) const;

    /**
     * @brief Selects the curve given in parameter and deselects any other curve in the widget.
     **/
    void selectCurve(const boost::shared_ptr<CurveGui>& curve);

    void moveSelectedKeyFrames(const QPointF & oldClick_opengl,const QPointF & newClick_opengl);
    
    void transformSelectedKeyFrames(const QPointF & oldClick_opengl,const QPointF & newClick_opengl, bool shiftHeld);

    void moveSelectedTangent(const QPointF & pos);

    void refreshKeyTangents(KeyPtr & key);

    void refreshSelectionRectangle(double x,double y);

    void setSelectedKeysInterpolation(Natron::KeyframeTypeEnum type);

    void createMenu();

    void insertSelectedKeyFrameConditionnaly(const KeyPtr & key);

    void updateDopeSheetViewFrameRange();

private:


    void keyFramesWithinRect(const QRectF & rect,std::vector< std::pair<boost::shared_ptr<CurveGui>,KeyFrame > >* keys) const;

public:

    QPoint _lastMousePos; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
    ZoomContext zoomCtx;
    EventStateEnum _state;
    Menu* _rightClickMenu;
    QColor _selectedCurveColor;
    QColor _nextCurveAddedColor;
    TextRenderer textRenderer;
    QFont* _font;
    Curves _curves;
    SelectedKeys _selectedKeyFrames;
    bool _hasOpenGLVAOSupport;
    bool _mustSetDragOrientation;
    QPoint _mouseDragOrientation; ///used to drag a key frame in only 1 direction (horizontal or vertical)
    ///the value is either (1,0) or (0,1)
    std::vector< KeyFrame > _keyFramesClipBoard;
    QRectF _selectionRectangle;
    QPointF _dragStartPoint;
    bool _drawSelectedKeyFramesBbox;
    QRectF _selectedKeyFramesBbox;
    QLineF _selectedKeyFramesCrossVertLine;
    QLineF _selectedKeyFramesCrossHorizLine;
    boost::shared_ptr<TimeLine> _timeline;
    bool _timelineEnabled;
    std::pair<MoveTangentCommand::SelectedTangentEnum,KeyPtr> _selectedDerivative;
    bool _evaluateOnPenUp; //< true if we must re-evaluate the nodes associated to the selected keyframes on penup
    QPointF _keyDragLastMovement;
    boost::scoped_ptr<QUndoStack> _undoStack;
    Gui* _gui;

    GLuint savedTexture;
    QSize sizeH;
    bool zoomOrPannedSinceLastFit;
    
private:

    QPolygonF _timelineTopPoly;
    QPolygonF _timelineBtmPoly;
    CurveWidget* _widget;
    
public:
    
    CurveSelection* _selectionModel;
};

#endif // _Gui_CurveWidgetPrivate_h
