/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "RotoGui.h"

#include <algorithm> // min, max

#include "Global/GLIncludes.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QString>
#include <QToolBar>
#include <QWidget>
#include <QAction>
#include <QRectF>
#include <QLineF>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QStyle>
#include <QDialogButtonBox>
#include <QColorDialog>
#include <QCheckBox>
#include <QTimer>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <ofxNatron.h>

#include "Engine/Bezier.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoContextPrivate.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobGuiColor.h" // ColorPickerLabel
#include "Gui/Menu.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/RotoUndoCommand.h"
#include "Gui/SpinBox.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"


#define kControlPointMidSize 3
#define kBezierSelectionTolerance 8
#define kControlPointSelectionTolerance 8
#define kXHairSelectedCpsTolerance 8
#define kXHairSelectedCpsBox 8
#define kTangentHandleSelectionTolerance 8
#define kTransformArrowLenght 10
#define kTransformArrowWidth 3
#define kTransformArrowOffsetFromPoint 15


using namespace Natron;

namespace {
///A list of points and their counter-part, that is: either a control point and its feather point, or
///the feather point and its associated control point
typedef std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > SelectedCP;
typedef std::list< SelectedCP > SelectedCPs;
typedef std::list< boost::shared_ptr<RotoDrawableItem> > SelectedItems;

enum EventStateEnum
{
    eEventStateNone = 0,
    eEventStateDraggingControlPoint,
    eEventStateDraggingSelectedControlPoints,
    eEventStateBuildingBezierControlPointTangent,
    eEventStateBuildingEllipse,
    eEventStateBuildingRectangle,
    eEventStateDraggingLeftTangent,
    eEventStateDraggingRightTangent,
    eEventStateDraggingFeatherBar,
    eEventStateDraggingBBoxTopLeft,
    eEventStateDraggingBBoxTopRight,
    eEventStateDraggingBBoxBtmRight,
    eEventStateDraggingBBoxBtmLeft,
    eEventStateDraggingBBoxMidTop,
    eEventStateDraggingBBoxMidRight,
    eEventStateDraggingBBoxMidBtm,
    eEventStateDraggingBBoxMidLeft,
    eEventStateBuildingStroke,
    eEventStateDraggingCloneOffset,
    eEventStateDraggingBrushSize,
    eEventStateDraggingBrushOpacity,
};

enum HoverStateEnum
{
    eHoverStateNothing = 0,
    eHoverStateBboxTopLeft,
    eHoverStateBboxTopRight,
    eHoverStateBboxBtmRight,
    eHoverStateBboxBtmLeft,
    eHoverStateBboxMidTop,
    eHoverStateBboxMidRight,
    eHoverStateBboxMidBtm,
    eHoverStateBboxMidLeft,
    eHoverStateBbox
};

enum SelectedCpsTransformModeEnum
{
    eSelectedCpsTransformModeTranslateAndScale = 0,
    eSelectedCpsTransformModeRotateAndSkew = 1
};
}


///A small structure of all the data shared by all the viewers watching the same Roto
class RotoGuiSharedData
{
public:
    SelectedItems selectedItems;
    SelectedCPs selectedCps;
    QRectF selectedCpsBbox;
    bool showCpsBbox;

    ////This is by default eSelectedCpsTransformModeTranslateAndScale. When clicking the cross-hair in the center this will toggle the transform mode
    ////like it does in inkscape.
    SelectedCpsTransformModeEnum transformMode;
    boost::shared_ptr<Bezier> builtBezier; //< the bezier currently being built
    boost::shared_ptr<Bezier> bezierBeingDragged;
    SelectedCP cpBeingDragged; //< the cp being dragged
    boost::shared_ptr<BezierCP> tangentBeingDragged; //< the control point whose tangent is being dragged.
                                                     //only relevant when the state is DRAGGING_X_TANGENT
    SelectedCP featherBarBeingDragged,featherBarBeingHovered;
    bool displayFeather;
    boost::shared_ptr<RotoStrokeItem> strokeBeingPaint;
    std::pair<double,double> cloneOffset;
    QPointF click; // used for drawing ellipses and rectangles, to handle center/constrain. May also be used for the selection bbox.

    RotoGuiSharedData()
    : selectedItems()
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
    , displayFeather(true)
    , strokeBeingPaint()
    , cloneOffset()
    , click()
    {
        cloneOffset.first = cloneOffset.second = 0.;
    }
};

struct RotoGui::RotoGuiPrivate
{
    RotoGui* publicInterface;
    NodeGui* node;
    ViewerGL* viewer;
    ViewerTab* viewerTab;
    boost::shared_ptr<RotoContext> context;
    RotoTypeEnum type;
    QToolBar* toolbar;
    QWidget* selectionButtonsBar;
    QHBoxLayout* selectionButtonsBarLayout;
    Button* autoKeyingEnabled;
    Button* featherLinkEnabled;
    Button* displayFeatherEnabled;
    Button* stickySelectionEnabled;
    Button* bboxClickAnywhere;
    Button* rippleEditEnabled;
    Button* addKeyframeButton;
    Button* removeKeyframeButton;
    
    QWidget* brushButtonsBar;
    QHBoxLayout* brushButtonsBarLayout;
    ColorPickerLabel* colorPickerLabel;
    Button* colorWheelButton;
    ComboBox* compositingOperatorButton;
    QLabel* opacityLabel;
    SpinBox* opacitySpinbox;
    Button*  pressureOpacityButton;
    QLabel* sizeLabel;
    SpinBox* sizeSpinbox;
    Button* pressureSizeButton;
    QLabel* hardnessLabel;
    SpinBox* hardnessSpinBox;
    Button* pressureHardnessButton;
    QLabel* buildUpLabel;
    Button* buildUpButton;
    QLabel* timeOffsetLabel;
    SpinBox* timeOffsetSpinbox;
    ComboBox* timeOffsetMode;
    ComboBox* sourceTypeCombobox;
    Button* resetCloneOffset;
    Natron::Label* multiStrokeEnabledLabel;
    QCheckBox* multiStrokeEnabled;
    
    
    RotoToolButton* selectTool;
    RotoToolButton* pointsEditionTool;
    RotoToolButton* bezierEditionTool;
    RotoToolButton* paintBrushTool;
    RotoToolButton* cloneBrushTool;
    RotoToolButton* effectBrushTool;
    RotoToolButton* mergeBrushTool;
    std::list<RotoToolButton*> allTools;
    QAction* selectAllAction;
    QAction* lastPaintToolAction;
    QAction* eraserAction;
    RotoToolEnum selectedTool;
    QToolButton* selectedRole;
    EventStateEnum state;
    HoverStateEnum hoverState;
    QPointF lastClickPos;
    QPointF lastMousePos;
    boost::shared_ptr< RotoGuiSharedData > rotoData;
    bool evaluateOnPenUp; //< if true the next pen up will call context->evaluateChange()
    bool evaluateOnKeyUp;  //< if true the next key up will call context->evaluateChange()
    bool iSelectingwithCtrlA;
    int shiftDown;
    int ctrlDown;
    bool lastTabletDownTriggeredEraser;
    QPointF mouseCenterOnSizeChange;
    
    RotoGuiPrivate(RotoGui* pub,
                   NodeGui* n,
                   ViewerTab* tab,
                   const boost::shared_ptr<RotoGuiSharedData> & sharedData)
    : publicInterface(pub)
    , node(n)
    , viewer( tab->getViewer() )
    , viewerTab(tab)
    , context()
    , type(eRotoTypeRotoscoping)
    , toolbar(0)
    , selectionButtonsBar(0)
    , selectionButtonsBarLayout(0)
    , autoKeyingEnabled(0)
    , featherLinkEnabled(0)
    , displayFeatherEnabled(0)
    , stickySelectionEnabled(0)
    , bboxClickAnywhere(0)
    , rippleEditEnabled(0)
    , addKeyframeButton(0)
    , removeKeyframeButton(0)
    , brushButtonsBar(0)
    , brushButtonsBarLayout(0)
    , colorPickerLabel(0)
    , colorWheelButton(0)
    , compositingOperatorButton(0)
    , opacityLabel(0)
    , opacitySpinbox(0)
    , pressureOpacityButton(0)
    , sizeLabel(0)
    , sizeSpinbox(0)
    , pressureSizeButton(0)
    , hardnessLabel(0)
    , hardnessSpinBox(0)
    , pressureHardnessButton(0)
    , buildUpLabel(0)
    , buildUpButton(0)
    , timeOffsetLabel(0)
    , timeOffsetSpinbox(0)
    , timeOffsetMode(0)
    , sourceTypeCombobox(0)
    , resetCloneOffset(0)
    , multiStrokeEnabledLabel(0)
    , multiStrokeEnabled(0)
    , selectTool(0)
    , pointsEditionTool(0)
    , bezierEditionTool(0)
    , paintBrushTool(0)
    , cloneBrushTool(0)
    , effectBrushTool(0)
    , mergeBrushTool(0)
    , allTools()
    , selectAllAction(0)
    , lastPaintToolAction(0)
    , eraserAction(0)
    , selectedTool(eRotoToolSelectAll)
    , selectedRole(0)
    , state(eEventStateNone)
    , hoverState(eHoverStateNothing)
    , lastClickPos()
    , lastMousePos()
    , rotoData(sharedData)
    , evaluateOnPenUp(false)
    , evaluateOnKeyUp(false)
    , iSelectingwithCtrlA(false)
    , shiftDown(0)
    , ctrlDown(0)
    , lastTabletDownTriggeredEraser(false)
    , mouseCenterOnSizeChange()
    {
        if ( n->getNode()->isRotoPaintingNode() ) {
            type = eRotoTypeRotopainting;
        }
        context = node->getNode()->getRotoContext();
        assert(context);
        if (!rotoData) {
            rotoData.reset(new RotoGuiSharedData);
        }
    }

    void clearSelection();

    void clearCPSSelection();

    void clearBeziersSelection();

    bool hasSelection() const;

    void onCurveLockedChangedRecursive(const boost::shared_ptr<RotoItem> & item,bool* ret);

    bool removeItemFromSelection(const boost::shared_ptr<RotoDrawableItem>& b);

    void computeSelectedCpsBBOX();

    QPointF getSelectedCpsBBOXCenter();

    void drawSelectedCpsBBOX();

    ///by default draws a vertical arrow, which can be rotated by rotate amount.
    void drawArrow(double centerX,double centerY,double rotate,bool hovered,const std::pair<double,double> & pixelScale);

    ///same as drawArrow but the two ends will make an angle of 90 degrees
    void drawBendedArrow(double centerX,double centerY,double rotate,bool hovered,const std::pair<double,double> & pixelScale);

    void handleBezierSelection(const boost::shared_ptr<Bezier> & curve, QMouseEvent* e);

    void handleControlPointSelection(const std::pair<boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > & p, QMouseEvent* e);

    void drawSelectedCp(double time,
                        const boost::shared_ptr<BezierCP> & cp,
                        double x,double y,
                        const Transform::Matrix3x3& transform);

    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >
    isNearbyFeatherBar(double time,const std::pair<double,double> & pixelScale,const QPointF & pos) const;

    bool isNearbySelectedCpsCrossHair(const QPointF & pos) const;
    
    bool isWithinSelectedCpsBBox(const QPointF& pos) const;

    bool isNearbyBBoxTopLeft(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxTopRight(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxBtmLeft(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxBtmRight(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;

    bool isNearbyBBoxMidTop(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxMidRight(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxMidBtm(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxMidLeft(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;

    bool isNearbySelectedCpsBoundingBox(const QPointF & pos,double tolerance) const;
    
    EventStateEnum isMouseInteractingWithCPSBbox(const QPointF& pos,double tolerance,const std::pair<double, double>& pixelScale) const;
    
    bool isBboxClickAnywhereEnabled() const
    {
        return bboxClickAnywhere ? bboxClickAnywhere->isDown() : false;
    }
    
    void toggleToolsSelection(QToolButton* selected);
    
    void makeStroke(bool prepareForLater,const RotoPoint& p);
    
    void checkViewersAreDirectlyConnected();
};


RotoToolButton::RotoToolButton(QWidget* parent)
: QToolButton(parent)
, isSelected(false)
, wasMouseReleased(false)
{
    setFocusPolicy(Qt::ClickFocus);
}


RotoToolButton::~RotoToolButton()
{
    
}

void
RotoToolButton::mousePressEvent(QMouseEvent* /*e*/)
{
    setFocus();
    wasMouseReleased = false;
    QTimer::singleShot(300, this, SLOT(handleLongPress()));
}

void
RotoToolButton::handleLongPress()
{
    if (!wasMouseReleased) {
        showMenu();
    }
}

void
RotoToolButton::mouseReleaseEvent(QMouseEvent* e)
{
    wasMouseReleased = true;
    if (triggerButtonIsRight(e)) {
        showMenu();
    } else if ( triggerButtonIsLeft(e) ) {
        handleSelection();
    } else {
        QToolButton::mousePressEvent(e);
    }
}

bool
RotoToolButton::getIsSelected() const
{
    return isSelected;
}

void
RotoToolButton::setIsSelected(bool s)
{
    isSelected = s;
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void
RotoToolButton::handleSelection()
{
    QAction* curAction = defaultAction();

    if ( !isDown() ) {
        setDown(true);
        Q_EMIT triggered(curAction);
    } else {
        QList<QAction*> allAction = actions();
        for (int i = 0; i < allAction.size(); ++i) {
            if (allAction[i] == curAction) {
                int next = ( i == (allAction.size() - 1) ) ? 0 : i + 1;
                setDefaultAction(allAction[next]);
                Q_EMIT triggered(allAction[next]);
                break;
            }
        }
    }
}

QAction*
RotoGui::createToolAction(QToolButton* toolGroup,
                          const QIcon & icon,
                          const QString & text,
                          const QString & tooltip,
                          const QKeySequence & shortcut,
                          RotoGui::RotoToolEnum tool)
{
    QAction *action = new QAction(icon,text,toolGroup);

    
    action->setToolTip("<p>" + text + ": " + tooltip + "</p><p><b>" + tr("Keyboard shortcut:") + " " + shortcut.toString(QKeySequence::NativeText) + "</b></p>");

    QPoint data;
    data.setX( (int)tool );
    if (toolGroup == _imp->selectTool) {
        data.setY( (int)eRotoRoleSelection );
    } else if (toolGroup == _imp->pointsEditionTool) {
        data.setY( (int)eRotoRolePointsEdition );
    } else if (toolGroup == _imp->bezierEditionTool) {
        data.setY(eRotoRoleBezierEdition);
    } else if (toolGroup == _imp->paintBrushTool) {
        data.setY(eRotoRolePaintBrush);
    } else if (toolGroup == _imp->cloneBrushTool) {
        data.setY(eRotoRoleCloneBrush);
    } else if (toolGroup == _imp->effectBrushTool) {
        data.setY(eRotoRoleEffectBrush);
    } else if (toolGroup == _imp->mergeBrushTool) {
        data.setY(eRotoRoleMergeBrush);
    }
    action->setData( QVariant(data) );
    QObject::connect( action, SIGNAL( triggered() ), this, SLOT( onToolActionTriggered() ) );
    toolGroup->addAction(action);

    return action;
}

RotoGui::RotoGui(NodeGui* node,
                 ViewerTab* parent,
                 const boost::shared_ptr<RotoGuiSharedData> & sharedData)
    : _imp( new RotoGuiPrivate(this,node,parent,sharedData) )
{
    assert(parent);
    assert(_imp->context);
    
    bool hasShapes = _imp->context->getNCurves();
    
    bool effectIsPaint = _imp->context->isRotoPaint();

    QObject::connect( parent->getViewer(),SIGNAL( selectionRectangleChanged(bool) ),this,SLOT( updateSelectionFromSelectionRectangle(bool) ) );
    QObject::connect( parent->getViewer(), SIGNAL( selectionCleared() ), this, SLOT( onSelectionCleared() ) );
    QPixmap pixBezier,pixEllipse,pixRectangle,pixAddPts,pixRemovePts,pixCuspPts,pixSmoothPts,pixOpenCloseCurve,pixRemoveFeather;
    QPixmap pixSelectAll,pixSelectPoints,pixSelectFeather,pixSelectCurves,pixAutoKeyingEnabled,pixAutoKeyingDisabled;
    QPixmap pixStickySelEnabled,pixStickySelDisabled,pixFeatherLinkEnabled,pixFeatherLinkDisabled,pixAddKey,pixRemoveKey;
    QPixmap pixRippleEnabled,pixRippleDisabled;
    QPixmap pixFeatherEnabled,pixFeatherDisabled;
    QPixmap pixBboxClickEnabled,pixBboxClickDisabled;
    QPixmap pixPaintBrush,pixEraser,pixBlur,pixSmear,pixSharpen,pixDodge,pixBurn,pixClone,pixReveal,pixPencil;

    appPTR->getIcon(Natron::NATRON_PIXMAP_BEZIER_32, NATRON_LARGE_BUTTON_ICON_SIZE, &pixBezier);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ELLIPSE, NATRON_LARGE_BUTTON_ICON_SIZE, &pixEllipse);
    appPTR->getIcon(Natron::NATRON_PIXMAP_RECTANGLE, NATRON_LARGE_BUTTON_ICON_SIZE, &pixRectangle);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_POINTS, NATRON_LARGE_BUTTON_ICON_SIZE, &pixAddPts);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_POINTS, NATRON_LARGE_BUTTON_ICON_SIZE, &pixRemovePts);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CUSP_POINTS, NATRON_LARGE_BUTTON_ICON_SIZE, &pixCuspPts);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SMOOTH_POINTS, NATRON_LARGE_BUTTON_ICON_SIZE, &pixSmoothPts);
    appPTR->getIcon(Natron::NATRON_PIXMAP_OPEN_CLOSE_CURVE, NATRON_LARGE_BUTTON_ICON_SIZE, &pixOpenCloseCurve);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_FEATHER, NATRON_LARGE_BUTTON_ICON_SIZE, &pixRemoveFeather);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SELECT_ALL, NATRON_LARGE_BUTTON_ICON_SIZE, &pixSelectAll);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SELECT_POINTS, NATRON_LARGE_BUTTON_ICON_SIZE, &pixSelectPoints);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SELECT_FEATHER, NATRON_LARGE_BUTTON_ICON_SIZE, &pixSelectFeather);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SELECT_CURVES, NATRON_LARGE_BUTTON_ICON_SIZE, &pixSelectCurves);
    appPTR->getIcon(Natron::NATRON_PIXMAP_AUTO_KEYING_ENABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixAutoKeyingEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_AUTO_KEYING_DISABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixAutoKeyingDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_STICKY_SELECTION_ENABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixStickySelEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_STICKY_SELECTION_DISABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixStickySelDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FEATHER_LINK_ENABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixFeatherLinkEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FEATHER_LINK_DISABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixFeatherLinkDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_KEYFRAME, NATRON_LARGE_BUTTON_ICON_SIZE, &pixAddKey);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_KEYFRAME, NATRON_LARGE_BUTTON_ICON_SIZE, &pixRemoveKey);
    appPTR->getIcon(Natron::NATRON_PIXMAP_RIPPLE_EDIT_ENABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixRippleEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_RIPPLE_EDIT_DISABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixRippleDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FEATHER_VISIBLE, NATRON_LARGE_BUTTON_ICON_SIZE, &pixFeatherEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FEATHER_UNVISIBLE, NATRON_LARGE_BUTTON_ICON_SIZE, &pixFeatherDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_ROI_ENABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixBboxClickEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_ROI_DISABLED, NATRON_LARGE_BUTTON_ICON_SIZE, &pixBboxClickDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ROTOPAINT_SOLID, NATRON_LARGE_BUTTON_ICON_SIZE, &pixPaintBrush);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ROTOPAINT_ERASER, NATRON_LARGE_BUTTON_ICON_SIZE, &pixEraser);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ROTOPAINT_BLUR, NATRON_LARGE_BUTTON_ICON_SIZE, &pixBlur);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ROTOPAINT_SMEAR, NATRON_LARGE_BUTTON_ICON_SIZE, &pixSmear);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ROTOPAINT_SHARPEN, NATRON_LARGE_BUTTON_ICON_SIZE, &pixSharpen);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ROTOPAINT_DODGE, NATRON_LARGE_BUTTON_ICON_SIZE, &pixDodge);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ROTOPAINT_BURN, NATRON_LARGE_BUTTON_ICON_SIZE, &pixBurn);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ROTOPAINT_CLONE, NATRON_LARGE_BUTTON_ICON_SIZE, &pixClone);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ROTOPAINT_REVEAL, NATRON_LARGE_BUTTON_ICON_SIZE, &pixReveal);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PENCIL, NATRON_LARGE_BUTTON_ICON_SIZE, &pixPencil);
    
    _imp->toolbar = new QToolBar(parent);
    _imp->toolbar->setOrientation(Qt::Vertical);
    _imp->selectionButtonsBar = new QWidget(parent);
    _imp->selectionButtonsBarLayout = new QHBoxLayout(_imp->selectionButtonsBar);
    _imp->selectionButtonsBarLayout->setContentsMargins(3, 2, 0, 0);
    
    QIcon autoKeyIc;
    autoKeyIc.addPixmap(pixAutoKeyingEnabled,QIcon::Normal,QIcon::On);
    autoKeyIc.addPixmap(pixAutoKeyingDisabled,QIcon::Normal,QIcon::Off);
    
    
    _imp->autoKeyingEnabled = new Button(autoKeyIc,"",_imp->selectionButtonsBar);
    _imp->autoKeyingEnabled->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->autoKeyingEnabled->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->autoKeyingEnabled->setCheckable(true);
    _imp->autoKeyingEnabled->setChecked( _imp->context->isAutoKeyingEnabled() );
    _imp->autoKeyingEnabled->setDown( _imp->context->isAutoKeyingEnabled() );
    _imp->autoKeyingEnabled->setToolTip(Natron::convertFromPlainText(tr("Auto-keying: When activated any movement to a control point will set a keyframe at the current time."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->autoKeyingEnabled, SIGNAL( clicked(bool) ), this, SLOT( onAutoKeyingButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->autoKeyingEnabled);
    
    QIcon featherLinkIc;
    featherLinkIc.addPixmap(pixFeatherLinkEnabled,QIcon::Normal,QIcon::On);
    featherLinkIc.addPixmap(pixFeatherLinkDisabled,QIcon::Normal,QIcon::Off);
    _imp->featherLinkEnabled = new Button(featherLinkIc,"",_imp->selectionButtonsBar);
    _imp->featherLinkEnabled->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->featherLinkEnabled->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->featherLinkEnabled->setCheckable(true);
    _imp->featherLinkEnabled->setChecked( _imp->context->isFeatherLinkEnabled() );
    _imp->featherLinkEnabled->setDown( _imp->context->isFeatherLinkEnabled() );
    _imp->featherLinkEnabled->setToolTip(Natron::convertFromPlainText(tr("Feather-link: When activated the feather points will follow the same"
                                                                         " movement as their counter-part does."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->featherLinkEnabled, SIGNAL( clicked(bool) ), this, SLOT( onFeatherLinkButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->featherLinkEnabled);
    
    QIcon enableFeatherIC;
    enableFeatherIC.addPixmap(pixFeatherEnabled,QIcon::Normal,QIcon::On);
    enableFeatherIC.addPixmap(pixFeatherDisabled,QIcon::Normal,QIcon::Off);
    _imp->displayFeatherEnabled = new Button(enableFeatherIC,"",_imp->selectionButtonsBar);
    _imp->displayFeatherEnabled->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->displayFeatherEnabled->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->displayFeatherEnabled->setCheckable(true);
    _imp->displayFeatherEnabled->setChecked(true);
    _imp->displayFeatherEnabled->setDown(true);
    _imp->displayFeatherEnabled->setToolTip(Natron::convertFromPlainText(tr("When checked, the feather curve applied to the shape(s) will be visible and editable."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->displayFeatherEnabled, SIGNAL( clicked(bool) ), this, SLOT( onDisplayFeatherButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->displayFeatherEnabled);
    
    QIcon stickSelIc;
    stickSelIc.addPixmap(pixStickySelEnabled,QIcon::Normal,QIcon::On);
    stickSelIc.addPixmap(pixStickySelDisabled,QIcon::Normal,QIcon::Off);
    _imp->stickySelectionEnabled = new Button(stickSelIc,"",_imp->selectionButtonsBar);
    _imp->stickySelectionEnabled->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->stickySelectionEnabled->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->stickySelectionEnabled->setCheckable(true);
    _imp->stickySelectionEnabled->setChecked(false);
    _imp->stickySelectionEnabled->setDown(false);
    _imp->stickySelectionEnabled->setToolTip(Natron::convertFromPlainText(tr("Sticky-selection: When activated, "
                                                                             " clicking outside of any shape will not clear the current selection."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->stickySelectionEnabled, SIGNAL( clicked(bool) ), this, SLOT( onStickySelectionButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->stickySelectionEnabled);
    
    QIcon bboxClickIc;
    bboxClickIc.addPixmap(pixBboxClickEnabled,QIcon::Normal,QIcon::On);
    bboxClickIc.addPixmap(pixBboxClickDisabled,QIcon::Normal,QIcon::Off);
    _imp->bboxClickAnywhere = new Button(bboxClickIc,"",_imp->selectionButtonsBar);
    _imp->bboxClickAnywhere->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->bboxClickAnywhere->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->bboxClickAnywhere->setCheckable(true);
    _imp->bboxClickAnywhere->setChecked(true);
    _imp->bboxClickAnywhere->setDown(true);
    _imp->bboxClickAnywhere->setToolTip(Natron::convertFromPlainText(tr("Easy bounding box manipulation: When activated, "
                                                                        " clicking inside of the bounding box of selected points will move the points."
                                                                        "When deactivated, only clicking on the cross will move the points."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->bboxClickAnywhere, SIGNAL( clicked(bool) ), this, SLOT( onBboxClickButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->bboxClickAnywhere);
    
    
    QIcon rippleEditIc;
    rippleEditIc.addPixmap(pixRippleEnabled,QIcon::Normal,QIcon::On);
    rippleEditIc.addPixmap(pixRippleDisabled,QIcon::Normal,QIcon::Off);
    _imp->rippleEditEnabled = new Button(rippleEditIc,"",_imp->selectionButtonsBar);
    _imp->rippleEditEnabled->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->rippleEditEnabled->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->rippleEditEnabled->setCheckable(true);
    _imp->rippleEditEnabled->setChecked( _imp->context->isRippleEditEnabled() );
    _imp->rippleEditEnabled->setDown( _imp->context->isRippleEditEnabled() );
    _imp->rippleEditEnabled->setToolTip(Natron::convertFromPlainText(tr("Ripple-edit: When activated, moving a control point"
                                                                        " will move it by the same amount for all the keyframes "
                                                                        "it has."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->rippleEditEnabled, SIGNAL( clicked(bool) ), this, SLOT( onRippleEditButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->rippleEditEnabled);
    
    _imp->addKeyframeButton = new Button(QIcon(pixAddKey),"",_imp->selectionButtonsBar);
    _imp->addKeyframeButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->addKeyframeButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    QObject::connect( _imp->addKeyframeButton, SIGNAL( clicked(bool) ), this, SLOT( onAddKeyFrameClicked() ) );
    _imp->addKeyframeButton->setToolTip(Natron::convertFromPlainText(tr("Set a keyframe at the current time for the selected shape(s), if any."), Qt::WhiteSpaceNormal));
    _imp->selectionButtonsBarLayout->addWidget(_imp->addKeyframeButton);
    
    _imp->removeKeyframeButton = new Button(QIcon(pixRemoveKey),"",_imp->selectionButtonsBar);
    _imp->removeKeyframeButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->removeKeyframeButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    QObject::connect( _imp->removeKeyframeButton, SIGNAL( clicked(bool) ), this, SLOT( onRemoveKeyFrameClicked() ) );
    _imp->removeKeyframeButton->setToolTip(Natron::convertFromPlainText(tr("Remove a keyframe at the current time for the selected shape(s), if any."), Qt::WhiteSpaceNormal));
    _imp->selectionButtonsBarLayout->addWidget(_imp->removeKeyframeButton);
    _imp->selectionButtonsBarLayout->addStretch();
    _imp->selectionButtonsBar->setVisible(false);
    //////
    
    _imp->brushButtonsBar = new QWidget(parent);
    _imp->brushButtonsBarLayout = new QHBoxLayout(_imp->brushButtonsBar);
    _imp->brushButtonsBarLayout->setContentsMargins(3, 2, 0, 0);
    _imp->brushButtonsBarLayout->setSpacing(1);
    
    
    
    QString multiTt = Natron::convertFromPlainText(tr("When checked, strokes will be appended to the same item "
                                                      "in the hierarchy as long as the same tool is selected.\n"
                                                      "Select another tool to make a new item."),Qt::WhiteSpaceNormal);
    _imp->multiStrokeEnabledLabel = new Natron::Label(QObject::tr("Multi-stroke:"),_imp->brushButtonsBar);
    _imp->multiStrokeEnabledLabel->setToolTip(multiTt);
    _imp->brushButtonsBarLayout->addWidget(_imp->multiStrokeEnabledLabel);
    
    _imp->brushButtonsBarLayout->addSpacing(3);
    
    _imp->multiStrokeEnabled = new QCheckBox(_imp->brushButtonsBar);
    _imp->multiStrokeEnabled->setToolTip(multiTt);
    _imp->multiStrokeEnabled->setChecked(true);
    _imp->brushButtonsBarLayout->addWidget(_imp->multiStrokeEnabled);
    
    _imp->brushButtonsBarLayout->addSpacing(10);
    
    _imp->colorPickerLabel = new ColorPickerLabel(0,_imp->brushButtonsBar);
    _imp->colorPickerLabel->setColor(Qt::white);
    _imp->colorPickerLabel->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->colorPickerLabel->setToolTip(Natron::convertFromPlainText(tr("The color of the next paint brush stroke to be painted."), Qt::WhiteSpaceNormal));
    _imp->brushButtonsBarLayout->addWidget(_imp->colorPickerLabel);
    QPixmap colorWheelPix;
    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, NATRON_MEDIUM_BUTTON_ICON_SIZE, &colorWheelPix);
    _imp->colorWheelButton = new Button(QIcon(colorWheelPix),"",_imp->brushButtonsBar);
    _imp->colorWheelButton->setToolTip(Natron::convertFromPlainText(tr("Open the color dialog."), Qt::WhiteSpaceNormal));
    _imp->colorWheelButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->colorWheelButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    QObject::connect(_imp->colorWheelButton, SIGNAL(clicked(bool)), this, SLOT(onColorWheelButtonClicked()));
    _imp->brushButtonsBarLayout->addWidget(_imp->colorWheelButton);
    
    _imp->brushButtonsBarLayout->addSpacing(5);
    
    _imp->compositingOperatorButton = new ComboBox(_imp->brushButtonsBar);
    {
        std::vector<std::string> operators,tooltips;
        getNatronCompositingOperators(&operators, &tooltips);
        assert(operators.size() == tooltips.size());
        for (std::size_t i = 0; i < operators.size(); ++i) {
            _imp->compositingOperatorButton->addItem(operators[i].c_str(),QIcon(),QKeySequence(),tooltips[i].c_str());
        }
    }
    _imp->compositingOperatorButton->setCurrentIndex_no_emit((int)Natron::eMergeCopy);
    _imp->compositingOperatorButton->setToolTip(Natron::convertFromPlainText(tr("The blending mode of the next brush stroke."), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->compositingOperatorButton, SIGNAL(currentIndexChanged(int)), this, SLOT(onBreakMultiStrokeTriggered()));
    _imp->brushButtonsBarLayout->addWidget(_imp->compositingOperatorButton);
    
    
    _imp->brushButtonsBarLayout->addSpacing(5);
    
    QString opacitytt = Natron::convertFromPlainText(tr("The opacity of the next brush stroke to be painted. Use CTRL + SHIFT + drag "
                                                        "with the mouse to change the opacity."), Qt::WhiteSpaceNormal);
    _imp->opacityLabel = new Natron::Label(tr("Opacity:"),_imp->brushButtonsBar);
    _imp->opacityLabel->setToolTip(opacitytt);
    _imp->brushButtonsBarLayout->addWidget(_imp->opacityLabel);
    
    _imp->opacitySpinbox = new SpinBox(_imp->brushButtonsBar,SpinBox::eSpinBoxTypeDouble);
    QObject::connect(_imp->opacitySpinbox, SIGNAL(valueChanged(double)), this, SLOT(onBreakMultiStrokeTriggered()));
    _imp->opacitySpinbox->setToolTip(opacitytt);
    _imp->opacitySpinbox->setMinimum(0);
    _imp->opacitySpinbox->setMaximum(1);
    _imp->opacitySpinbox->setValue(1.);
    _imp->brushButtonsBarLayout->addWidget(_imp->opacitySpinbox);
    
    QPixmap pressureOnPix,pressureOffPix,buildupOnPix,buildupOffPix;
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pressureOnPix);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pressureOffPix);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &buildupOnPix);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &buildupOffPix);
    
    QIcon pressureIc;
    pressureIc.addPixmap(pressureOnPix,QIcon::Normal,QIcon::On);
    pressureIc.addPixmap(pressureOffPix,QIcon::Normal,QIcon::Off);
    QString pressOpatt = Natron::convertFromPlainText(tr("If checked, the pressure of the pen will dynamically alter the opacity of the next "
                                                         "brush stroke."), Qt::WhiteSpaceNormal);
    
    _imp->pressureOpacityButton = new Button(pressureIc,"",_imp->brushButtonsBar);
    QObject::connect(_imp->pressureOpacityButton, SIGNAL(clicked(bool)), this, SLOT(onPressureOpacityClicked(bool)));
    _imp->pressureOpacityButton->setToolTip(pressOpatt);
    _imp->pressureOpacityButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->pressureOpacityButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->pressureOpacityButton->setCheckable(true);
    _imp->pressureOpacityButton->setChecked(true);
    _imp->pressureOpacityButton->setDown(true);
    _imp->brushButtonsBarLayout->addWidget(_imp->pressureOpacityButton);
    
    _imp->brushButtonsBarLayout->addSpacing(5);
    
    QString sizett = Natron::convertFromPlainText(tr("The size of the next brush stroke to be painted. Use SHIFT + drag with the mouse "
                                                     "to change the size."), Qt::WhiteSpaceNormal);
    _imp->sizeLabel = new Natron::Label(tr("Size:"),_imp->brushButtonsBar);
    _imp->sizeLabel->setToolTip(sizett);
    _imp->brushButtonsBarLayout->addWidget(_imp->sizeLabel);
    
    _imp->sizeSpinbox = new SpinBox(_imp->brushButtonsBar, SpinBox::eSpinBoxTypeDouble);
    QObject::connect(_imp->sizeSpinbox, SIGNAL(valueChanged(double)), this, SLOT(onBreakMultiStrokeTriggered()));
    _imp->sizeSpinbox->setMinimum(0);
    _imp->sizeSpinbox->setMaximum(1000);
    _imp->sizeSpinbox->setValue(25.);
    _imp->sizeSpinbox->setToolTip(sizett);
    _imp->brushButtonsBarLayout->addWidget(_imp->sizeSpinbox);
    
    QString pressSizett = Natron::convertFromPlainText(tr("If checked, the pressure of the pen will dynamically alter the size of the next "
                                                          "brush stroke."), Qt::WhiteSpaceNormal);
    _imp->pressureSizeButton = new Button(pressureIc,"",_imp->brushButtonsBar);
    QObject::connect(_imp->pressureSizeButton, SIGNAL(clicked(bool)), this, SLOT(onPressureSizeClicked(bool)));
    _imp->pressureSizeButton->setToolTip(pressSizett);
    _imp->pressureSizeButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->pressureSizeButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->pressureSizeButton->setCheckable(true);
    _imp->pressureSizeButton->setChecked(false);
    _imp->pressureSizeButton->setDown(false);
    _imp->brushButtonsBarLayout->addWidget(_imp->pressureSizeButton);
    
    _imp->brushButtonsBarLayout->addSpacing(5);
    
    QString hardnesstt = Natron::convertFromPlainText(tr("The hardness of the next brush stroke to be painted."), Qt::WhiteSpaceNormal);
    _imp->hardnessLabel = new Natron::Label(tr("Hardness:"),_imp->brushButtonsBar);
    _imp->hardnessLabel->setToolTip(hardnesstt);
    _imp->brushButtonsBarLayout->addWidget(_imp->hardnessLabel);
    
    _imp->hardnessSpinBox = new SpinBox(_imp->brushButtonsBar,SpinBox::eSpinBoxTypeDouble);
    QObject::connect(_imp->hardnessSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onBreakMultiStrokeTriggered()));
    _imp->hardnessSpinBox->setMinimum(0);
    _imp->hardnessSpinBox->setMaximum(1);
    _imp->hardnessSpinBox->setValue(0.2);
    _imp->hardnessSpinBox->setToolTip(hardnesstt);
    _imp->brushButtonsBarLayout->addWidget(_imp->hardnessSpinBox);
    
    QString pressHardnesstt = Natron::convertFromPlainText(tr("If checked, the pressure of the pen will dynamically alter the hardness of the next "
                                                              "brush stroke."), Qt::WhiteSpaceNormal);
    _imp->pressureHardnessButton = new Button(pressureIc,"",_imp->brushButtonsBar);
    QObject::connect(_imp->pressureHardnessButton, SIGNAL(clicked(bool)), this, SLOT(onPressureHardnessClicked(bool)));
    _imp->pressureHardnessButton->setToolTip(pressHardnesstt);
    _imp->pressureHardnessButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->pressureHardnessButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->pressureHardnessButton->setCheckable(true);
    _imp->pressureHardnessButton->setChecked(false);
    _imp->pressureHardnessButton->setDown(false);
    _imp->brushButtonsBarLayout->addWidget(_imp->pressureHardnessButton);
    
    _imp->brushButtonsBarLayout->addSpacing(5);
    
    QString builduptt = Natron::convertFromPlainText(tr("When build-up is enabled, the next brush stroke will build up "
                                                        "when painted over itself."), Qt::WhiteSpaceNormal);
    
    _imp->buildUpLabel = new Natron::Label(tr("Build-up:"),_imp->brushButtonsBar);
    _imp->buildUpLabel->setToolTip(builduptt);
    _imp->brushButtonsBarLayout->addWidget(_imp->buildUpLabel);
    
    QIcon buildupIc;
    buildupIc.addPixmap(buildupOnPix,QIcon::Normal,QIcon::On);
    buildupIc.addPixmap(buildupOffPix,QIcon::Normal,QIcon::Off);
    _imp->buildUpButton = new Button(buildupIc,"",_imp->brushButtonsBar);
    QObject::connect(_imp->buildUpButton, SIGNAL(clicked(bool)), this, SLOT(onBuildupClicked(bool)));
    _imp->buildUpButton->setToolTip(builduptt);
    _imp->buildUpButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->buildUpButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->buildUpButton->setCheckable(true);
    _imp->buildUpButton->setChecked(true);
    _imp->buildUpButton->setDown(true);
    _imp->brushButtonsBarLayout->addWidget(_imp->buildUpButton);
    
    _imp->brushButtonsBarLayout->addSpacing(5);
    
    QString timeOfftt = Natron::convertFromPlainText(tr("When the Clone tool is used, this determines depending on the time offset "
                                                        "mode the source frame to clone. When in absolute mode, this is the frame "
                                                        "number of the source, when in relative mode, this is an offset relative "
                                                        "to the current frame."), Qt::WhiteSpaceNormal);
    
    _imp->timeOffsetLabel = new Natron::Label(tr("Time Offset:"),_imp->brushButtonsBar);
    _imp->timeOffsetLabel->setVisible(false);
    _imp->timeOffsetLabel->setToolTip(timeOfftt);
    _imp->brushButtonsBarLayout->addWidget(_imp->timeOffsetLabel);
    
    _imp->timeOffsetSpinbox = new SpinBox(_imp->brushButtonsBar, SpinBox::eSpinBoxTypeInt);
    QObject::connect(_imp->timeOffsetSpinbox, SIGNAL(valueChanged(double)), this, SLOT(onBreakMultiStrokeTriggered()));
    _imp->timeOffsetSpinbox->setValue(0);
    _imp->timeOffsetSpinbox->setVisible(false);
    _imp->timeOffsetSpinbox->setToolTip(timeOfftt);
    _imp->brushButtonsBarLayout->addWidget(_imp->timeOffsetSpinbox);
    
    _imp->timeOffsetMode = new ComboBox(_imp->brushButtonsBar);
    _imp->timeOffsetMode->setToolTip(Natron::convertFromPlainText(tr("When in absolute mode, this is the frame number of the source, "
                                                                     "when in relative mode, this is an offset relative to "
                                                                     "the current frame."), Qt::WhiteSpaceNormal));
    _imp->timeOffsetMode->addItem(tr("Relative"));
    _imp->timeOffsetMode->addItem(tr("Absolute"));
    _imp->timeOffsetMode->setCurrentIndex_no_emit(0);
    _imp->timeOffsetMode->setVisible(false);
    _imp->brushButtonsBarLayout->addWidget(_imp->timeOffsetMode);
    
    _imp->sourceTypeCombobox = new ComboBox(_imp->brushButtonsBar);
    _imp->sourceTypeCombobox->setToolTip(Natron::convertFromPlainText(tr(
                                                                         "Source color used for painting the stroke when the Reveal/Clone tools are used:\n"
                                                                         "- foreground: the painted result at this point in the hierarchy,\n"
                                                                         "- background: the original image unpainted connected to bg,\n"
                                                                         "- backgroundN: the original image unpainted connected to bgN."), Qt::WhiteSpaceNormal));
    {
        _imp->sourceTypeCombobox->addItem(tr("foreground"));
        _imp->sourceTypeCombobox->addItem(tr("background"));
        for (int i = 1; i < 10; ++i) {
            
            QString str = tr("background") + QString::number(i+1);
            _imp->sourceTypeCombobox->addItem(str);
        }
    }
    _imp->sourceTypeCombobox->setCurrentIndex_no_emit(1);
    _imp->sourceTypeCombobox->setVisible(false);
    _imp->brushButtonsBarLayout->addWidget(_imp->sourceTypeCombobox);
    
    _imp->resetCloneOffset = new Button(QIcon(),tr("Reset Transform"),_imp->brushButtonsBar);
    _imp->resetCloneOffset->setToolTip(Natron::convertFromPlainText(tr("Reset the transform applied before cloning to identity."),Qt::WhiteSpaceNormal));
    QObject::connect(_imp->resetCloneOffset, SIGNAL(clicked(bool)), this, SLOT(onResetCloneTransformClicked()));
    _imp->brushButtonsBarLayout->addWidget(_imp->resetCloneOffset);
    _imp->resetCloneOffset->setVisible(false);
    
    _imp->brushButtonsBarLayout->addStretch();
    _imp->brushButtonsBar->setVisible(false);
    
    ////////////////////////////////////// CREATING VIEWER LEFT TOOLBAR //////////////////////////////////////
    
    QSize rotoToolSize(NATRON_LARGE_BUTTON_SIZE, NATRON_LARGE_BUTTON_SIZE);

    _imp->selectTool = new RotoToolButton(_imp->toolbar);
    _imp->selectTool->setFixedSize(rotoToolSize);
    _imp->selectTool->setIconSize(rotoToolSize);
    _imp->selectTool->setPopupMode(QToolButton::InstantPopup);
    QObject::connect( _imp->selectTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
    QKeySequence selectShortCut(Qt::Key_Q);
    _imp->selectAllAction = createToolAction(_imp->selectTool, QIcon(pixSelectAll), tr("Select all"),
                                             tr("everything can be selected and moved."),
                                             selectShortCut, eRotoToolSelectAll);
    createToolAction(_imp->selectTool, QIcon(pixSelectPoints), tr("Select points"),
                     tr("works only for the points of the inner shape,"
                        " feather points will not be taken into account."),
                     selectShortCut, eRotoToolSelectPoints);
    createToolAction(_imp->selectTool, QIcon(pixSelectCurves), tr("Select curves"),
                     tr("only the curves can be selected.")
                     ,selectShortCut,eRotoToolSelectCurves);
    createToolAction(_imp->selectTool, QIcon(pixSelectFeather), tr("Select feather points"), tr("only the feather points can be selected."),selectShortCut,eRotoToolSelectFeatherPoints);
    
    _imp->selectTool->setDown(hasShapes);
    _imp->selectTool->setDefaultAction(_imp->selectAllAction);
    _imp->allTools.push_back(_imp->selectTool);
    _imp->toolbar->addWidget(_imp->selectTool);
    
    QAction* defaultAction = _imp->selectAllAction;
    
    _imp->pointsEditionTool = new RotoToolButton(_imp->toolbar);
    _imp->pointsEditionTool->setFixedSize(rotoToolSize);
    _imp->pointsEditionTool->setIconSize(rotoToolSize);
    _imp->pointsEditionTool->setPopupMode(QToolButton::InstantPopup);
    QObject::connect( _imp->pointsEditionTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
    _imp->pointsEditionTool->setText( tr("Add points") );
    QKeySequence pointsEditionShortcut(Qt::Key_D);
    QAction* addPtsAct = createToolAction(_imp->pointsEditionTool, QIcon(pixAddPts), tr("Add points"),tr("add a new control point to the shape")
                                          ,pointsEditionShortcut, eRotoToolAddPoints);
    createToolAction(_imp->pointsEditionTool, QIcon(pixRemovePts), tr("Remove points"),"",pointsEditionShortcut,eRotoToolRemovePoints);
    createToolAction(_imp->pointsEditionTool, QIcon(pixCuspPts), tr("Cusp points"),"", pointsEditionShortcut,eRotoToolCuspPoints);
    createToolAction(_imp->pointsEditionTool, QIcon(pixSmoothPts), tr("Smooth points"),"", pointsEditionShortcut,eRotoToolSmoothPoints);
    createToolAction(_imp->pointsEditionTool, QIcon(pixOpenCloseCurve), tr("Open/Close curve"),"", pointsEditionShortcut,eRotoToolOpenCloseCurve);
    createToolAction(_imp->pointsEditionTool, QIcon(pixRemoveFeather), tr("Remove feather"),tr("set the feather point to be equal to the control point"), pointsEditionShortcut,eRotoToolRemoveFeatherPoints);
    _imp->pointsEditionTool->setDown(false);
    _imp->pointsEditionTool->setDefaultAction(addPtsAct);
    _imp->allTools.push_back(_imp->pointsEditionTool);
    _imp->toolbar->addWidget(_imp->pointsEditionTool);
    
    _imp->bezierEditionTool = new RotoToolButton(_imp->toolbar);
    _imp->bezierEditionTool->setFixedSize(rotoToolSize);
    _imp->bezierEditionTool->setIconSize(rotoToolSize);
    _imp->bezierEditionTool->setPopupMode(QToolButton::InstantPopup);
    QObject::connect( _imp->bezierEditionTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
    _imp->bezierEditionTool->setText("Bezier");
    _imp->bezierEditionTool->setDown(!hasShapes && !effectIsPaint);
    
    
    QKeySequence editBezierShortcut(Qt::Key_V);
    QAction* drawBezierAct = createToolAction(_imp->bezierEditionTool, QIcon(pixBezier), tr("Bezier"),
                                              tr("Edit bezier paths. Click and drag the mouse to adjust tangents. Press enter to close the shape. ")
                                              ,editBezierShortcut, eRotoToolDrawBezier);
    
    ////B-splines are not implemented yet
    //createToolAction(_imp->bezierEditionTool, QIcon(), "B-Spline", eRotoToolDrawBSpline);
    
    createToolAction(_imp->bezierEditionTool, QIcon(pixEllipse), tr("Ellipse"),tr("Hold control to draw the ellipse from its center"),editBezierShortcut, eRotoToolDrawEllipse);
    createToolAction(_imp->bezierEditionTool, QIcon(pixRectangle), tr("Rectangle"),"", editBezierShortcut,eRotoToolDrawRectangle);
    _imp->bezierEditionTool->setDefaultAction(drawBezierAct);
    _imp->allTools.push_back(_imp->bezierEditionTool);
    _imp->toolbar->addWidget(_imp->bezierEditionTool);
    
    if (!hasShapes && !effectIsPaint) {
        defaultAction = drawBezierAct;
    }
    
    _imp->paintBrushTool = new RotoToolButton(_imp->toolbar);
    _imp->paintBrushTool->setFixedSize(rotoToolSize);
    _imp->paintBrushTool->setIconSize(rotoToolSize);
    _imp->paintBrushTool->setPopupMode(QToolButton::InstantPopup);
    QObject::connect( _imp->paintBrushTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
    _imp->paintBrushTool->setText("Brush");
    _imp->paintBrushTool->setDown(!hasShapes && effectIsPaint);
    QKeySequence brushPaintShortcut(Qt::Key_N);
    QAction* brushPaintAct = createToolAction(_imp->paintBrushTool, QIcon(pixPaintBrush), tr("Brush"), tr("Freehand painting"), brushPaintShortcut, eRotoToolSolidBrush);
    createToolAction(_imp->paintBrushTool, QIcon(pixPencil), tr("Pencil"), tr("Freehand painting based on bezier curves"), brushPaintShortcut, eRotoToolOpenBezier);
    if (effectIsPaint) {
        _imp->eraserAction = createToolAction(_imp->paintBrushTool, QIcon(pixEraser), tr("Eraser"), tr("Erase previous paintings"), brushPaintShortcut, eRotoToolEraserBrush);
    }
    _imp->paintBrushTool->setDefaultAction(brushPaintAct);
    _imp->allTools.push_back(_imp->paintBrushTool);
    _imp->toolbar->addWidget(_imp->paintBrushTool);
    
    if (effectIsPaint) {
        _imp->cloneBrushTool = new RotoToolButton(_imp->toolbar);
        _imp->cloneBrushTool->setFixedSize(rotoToolSize);
        _imp->cloneBrushTool->setIconSize(rotoToolSize);
        _imp->cloneBrushTool->setPopupMode(QToolButton::InstantPopup);
        QObject::connect( _imp->cloneBrushTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
        _imp->cloneBrushTool->setText("Clone");
        _imp->cloneBrushTool->setDown(false);
        QKeySequence cloneBrushShortcut(Qt::Key_C);
        QAction* cloneBrushAct = createToolAction(_imp->cloneBrushTool, QIcon(pixClone), tr("Clone"), tr("Clone a portion of the source image"), cloneBrushShortcut, eRotoToolClone);
        createToolAction(_imp->cloneBrushTool, QIcon(pixReveal), tr("Reveal"), tr("Reveal a portion of the source image"), cloneBrushShortcut, eRotoToolReveal);
        _imp->cloneBrushTool->setDefaultAction(cloneBrushAct);
        _imp->allTools.push_back(_imp->cloneBrushTool);
        _imp->toolbar->addWidget(_imp->cloneBrushTool);
        
        _imp->effectBrushTool = new RotoToolButton(_imp->toolbar);
        _imp->effectBrushTool->setFixedSize(rotoToolSize);
        _imp->effectBrushTool->setIconSize(rotoToolSize);
        _imp->effectBrushTool->setPopupMode(QToolButton::InstantPopup);
        QObject::connect( _imp->effectBrushTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
        _imp->effectBrushTool->setText("Blur");
        _imp->effectBrushTool->setDown(false);
        QKeySequence blurShortcut(Qt::Key_X);
        QAction* blurBrushAct = createToolAction(_imp->effectBrushTool, QIcon(pixBlur), tr("Blur"), tr("Blur a portion of the source image"), blurShortcut, eRotoToolBlur);
        //createToolAction(_imp->effectBrushTool, QIcon(pixSharpen), tr("Sharpen"), tr("Sharpen a portion of the source image"), blurShortcut, eRotoToolSharpen);
        createToolAction(_imp->effectBrushTool, QIcon(pixSmear), tr("Smear"), tr("Blur and displace a portion of the source image along the direction of the pen"), blurShortcut, eRotoToolSmear);
        _imp->effectBrushTool->setDefaultAction(blurBrushAct);
        _imp->allTools.push_back(_imp->effectBrushTool);
        _imp->toolbar->addWidget(_imp->effectBrushTool);
        
        _imp->mergeBrushTool = new RotoToolButton(_imp->toolbar);
        _imp->mergeBrushTool->setFixedSize(rotoToolSize);
        _imp->mergeBrushTool->setIconSize(rotoToolSize);
        _imp->mergeBrushTool->setPopupMode(QToolButton::InstantPopup);
        QObject::connect( _imp->mergeBrushTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
        _imp->mergeBrushTool->setText("Dodge");
        _imp->mergeBrushTool->setDown(false);
        QKeySequence dodgeBrushShortcut(Qt::Key_E);
        QAction* dodgeBrushAct = createToolAction(_imp->mergeBrushTool, QIcon(pixDodge), tr("Dodge"), tr("Make the source image brighter"), dodgeBrushShortcut, eRotoToolDodge);
        createToolAction(_imp->mergeBrushTool, QIcon(pixBurn), tr("Burn"), tr("Make the source image darker"), dodgeBrushShortcut, eRotoToolBurn);
        _imp->mergeBrushTool->setDefaultAction(dodgeBrushAct);
        _imp->allTools.push_back(_imp->mergeBrushTool);
        _imp->toolbar->addWidget(_imp->mergeBrushTool);
    }
    
    if (!hasShapes && effectIsPaint) {
        defaultAction = brushPaintAct;
    }
    
    ////////////Default action is to make a new bezier
    _imp->selectedRole = _imp->selectTool;
    onToolActionTriggered(defaultAction);
    
    QObject::connect( _imp->node->getNode()->getApp()->getTimeLine().get(), SIGNAL( frameChanged(SequenceTime,int) ),
                      this, SLOT( onCurrentFrameChanged(SequenceTime,int) ) );
    QObject::connect( _imp->context.get(), SIGNAL( refreshViewerOverlays() ), this, SLOT( onRefreshAsked() ) );
    QObject::connect( _imp->context.get(), SIGNAL( selectionChanged(int) ), this, SLOT( onSelectionChanged(int) ) );
    QObject::connect( _imp->context.get(), SIGNAL( itemLockedChanged(int) ), this, SLOT( onCurveLockedChanged(int) ) );
    QObject::connect( _imp->context.get(), SIGNAL( breakMultiStroke() ), this, SLOT( onBreakMultiStrokeTriggered() ) );
    QObject::connect (_imp->viewerTab->getGui()->getApp()->getTimeLine().get(), SIGNAL(frameChanged(SequenceTime,int)), this,
                      SLOT(onTimelineTimeChanged()));;
    restoreSelectionFromContext();
}

RotoGui::~RotoGui()
{
}

boost::shared_ptr<RotoGuiSharedData>
RotoGui::getRotoGuiSharedData() const
{
    return _imp->rotoData;
}

QWidget*
RotoGui::getButtonsBar(RotoGui::RotoRoleEnum role) const
{
    switch (role) {
        case eRotoRoleSelection:
            return _imp->selectionButtonsBar;
        case eRotoRolePointsEdition:
            return _imp->selectionButtonsBar;
        case eRotoRoleBezierEdition:
            return _imp->selectionButtonsBar;
        case eRotoRolePaintBrush:
            return _imp->brushButtonsBar;
        case eRotoRoleEffectBrush:
            return _imp->brushButtonsBar;
        case eRotoRoleCloneBrush:
            return _imp->brushButtonsBar;
        case eRotoRoleMergeBrush:
            return _imp->brushButtonsBar;
    }
    assert(false);
    return NULL;
}

GuiAppInstance*
RotoGui::getApp() const
{
    return _imp->node->getDagGui()->getGui()->getApp();
}

QWidget*
RotoGui::getCurrentButtonsBar() const
{
    return getButtonsBar( getCurrentRole() );
}

RotoGui::RotoToolEnum
RotoGui::getSelectedTool() const
{
    return _imp->selectedTool;
}

void
RotoGui::setCurrentTool(RotoGui::RotoToolEnum tool,
                        bool emitSignal)
{
    QList<QAction*> actions = _imp->selectTool->actions();
    if (_imp->pointsEditionTool) {
        actions.append( _imp->pointsEditionTool->actions() );
    }
    if (_imp->bezierEditionTool) {
        actions.append( _imp->bezierEditionTool->actions() );
    }
    if (_imp->paintBrushTool) {
        actions.append(_imp->paintBrushTool->actions());
    }
    if (_imp->cloneBrushTool) {
        actions.append(_imp->cloneBrushTool->actions());
    }
    if (_imp->effectBrushTool) {
        actions.append(_imp->effectBrushTool->actions());
    }
    if (_imp->mergeBrushTool) {
        actions.append(_imp->mergeBrushTool->actions());
    }
    for (int i = 0; i < actions.size(); ++i) {
        QPoint data = actions[i]->data().toPoint();
        if ( (RotoGui::RotoToolEnum)data.x() == tool ) {
            onToolActionTriggeredInternal(actions[i],emitSignal);

            return;
        }
    }
    assert(false);
}

QToolBar*
RotoGui::getToolBar() const
{
    return _imp->toolbar;
}

void
RotoGui::onToolActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );

    if (act) {
        onToolActionTriggered(act);
    }
}

void
RotoGui::RotoGuiPrivate::toggleToolsSelection(QToolButton* selected)
{
    for (std::list<RotoToolButton*>::iterator it = allTools.begin(); it!=allTools.end(); ++it) {
        if (*it == selected) {
            (*it)->setIsSelected(true);
        } else {
            (*it)->setIsSelected(false);
        }
    }
}

void
RotoGui::onToolActionTriggeredInternal(QAction* action,
                                       bool emitSignal)
{
    QPoint data = action->data().toPoint();

    if (_imp->selectedTool == (RotoToolEnum)data.x()) {
        return;
    }

    RotoRoleEnum actionRole = (RotoRoleEnum)data.y();
    QToolButton* toolButton = 0;
    RotoRoleEnum previousRole = getCurrentRole();
    
    switch (actionRole) {
        case eRotoRoleSelection:
            toolButton = _imp->selectTool;
            break;
        case eRotoRolePointsEdition:
            toolButton = _imp->pointsEditionTool;
            break;
        case eRotoRoleBezierEdition:
            toolButton = _imp->bezierEditionTool;
            break;
        case eRotoRoleEffectBrush:
            toolButton = _imp->effectBrushTool;
            break;
        case eRotoRoleMergeBrush:
            toolButton = _imp->mergeBrushTool;
            break;
        case eRotoRoleCloneBrush:
            toolButton = _imp->cloneBrushTool;
            break;
        case eRotoRolePaintBrush:
            toolButton = _imp->paintBrushTool;
            break;
        default:
            assert(false);
            break;
    }
    
    if (actionRole == eRotoRoleCloneBrush) {
        _imp->timeOffsetLabel->setVisible(true);
        _imp->timeOffsetMode->setVisible(true);
        _imp->timeOffsetSpinbox->setVisible(true);
        _imp->sourceTypeCombobox->setVisible(true);
        _imp->resetCloneOffset->setVisible(true);
        if ((RotoToolEnum)data.x() == eRotoToolClone) {
            _imp->sourceTypeCombobox->setCurrentIndex_no_emit(1);
        } else if ((RotoToolEnum)data.x() == eRotoToolReveal) {
            _imp->sourceTypeCombobox->setCurrentIndex_no_emit(2);
        }
    } else {
        if (_imp->timeOffsetLabel) {
            _imp->timeOffsetLabel->setVisible(false);
        }
        if (_imp->timeOffsetMode) {
            _imp->timeOffsetMode->setVisible(false);
        }
        if (_imp->timeOffsetSpinbox) {
            _imp->timeOffsetSpinbox->setVisible(false);
        }
        if (_imp->sourceTypeCombobox) {
            _imp->sourceTypeCombobox->setVisible(false);
        }
        if (_imp->resetCloneOffset) {
            _imp->resetCloneOffset->setVisible(false);
        }
    }
    if (actionRole == eRotoRolePaintBrush || actionRole == eRotoRoleCloneBrush || actionRole == eRotoRoleMergeBrush ||
        actionRole == eRotoRoleEffectBrush) {
        if ((RotoToolEnum)data.x() == eRotoToolSolidBrush || (RotoToolEnum)data.x() == eRotoToolOpenBezier) {
            _imp->compositingOperatorButton->setCurrentIndex_no_emit((int)Natron::eMergeOver);
        } else if ((RotoToolEnum)data.x() == eRotoToolBurn) {
            _imp->compositingOperatorButton->setCurrentIndex_no_emit((int)Natron::eMergeColorBurn);
        } else if ((RotoToolEnum)data.x() == eRotoToolDodge) {
            _imp->compositingOperatorButton->setCurrentIndex_no_emit((int)Natron::eMergeColorDodge);
        } else {
            _imp->compositingOperatorButton->setCurrentIndex_no_emit((int)Natron::eMergeCopy);
        }
    }
    
    if ((RotoToolEnum)data.x() != eRotoToolEraserBrush) {
        _imp->lastPaintToolAction = action;
    }
    
    _imp->toggleToolsSelection(toolButton);
    Q_EMIT roleChanged( (int)previousRole,(int)actionRole);

    assert(_imp->selectedRole);
    if (_imp->selectedRole != toolButton) {
        _imp->selectedRole->setDown(false);
    }

    ///reset the selected control points
    _imp->rotoData->selectedCps.clear();
    _imp->rotoData->showCpsBbox = false;
    _imp->rotoData->transformMode = eSelectedCpsTransformModeTranslateAndScale;
    _imp->rotoData->selectedCpsBbox.setTopLeft( QPointF(0,0) );
    _imp->rotoData->selectedCpsBbox.setTopRight( QPointF(0,0) );

    ///clear all selection if we were building a new bezier
    if ( (previousRole == eRotoRoleBezierEdition) &&
        (_imp->selectedTool == eRotoToolDrawBezier || _imp->selectedTool == eRotoToolOpenBezier) &&
        _imp->rotoData->builtBezier &&
         ( (RotoToolEnum)data.x() != _imp->selectedTool ) ) {
        _imp->rotoData->builtBezier->setCurveFinished(true);
        
        _imp->clearSelection();
    }


    assert(toolButton);
    toolButton->setDown(true);
    toolButton->setDefaultAction(action);
    _imp->selectedRole = toolButton;
    _imp->selectedTool = (RotoToolEnum)data.x();
    if (emitSignal) {
        Q_EMIT selectedToolChanged( (int)_imp->selectedTool );
    }
    
    if (_imp->selectedTool == eRotoToolBlur ||
        _imp->selectedTool == eRotoToolBurn ||
        _imp->selectedTool == eRotoToolDodge ||
        _imp->selectedTool == eRotoToolClone ||
        _imp->selectedTool == eRotoToolEraserBrush ||
        _imp->selectedTool == eRotoToolSolidBrush ||
        _imp->selectedTool == eRotoToolReveal ||
        _imp->selectedTool == eRotoToolSmear ||
        _imp->selectedTool == eRotoToolSharpen) {
        _imp->makeStroke(true, RotoPoint());
    }
} // onToolActionTriggeredInternal

void
RotoGui::onToolActionTriggered(QAction* act)
{
    onToolActionTriggeredInternal(act, true);
}

RotoGui::RotoRoleEnum
RotoGui::getCurrentRole() const
{
    if (_imp->selectedRole == _imp->selectTool) {
        return eRotoRoleSelection;
    } else if (_imp->selectedRole == _imp->pointsEditionTool) {
        return eRotoRolePointsEdition;
    } else if (_imp->selectedRole == _imp->bezierEditionTool) {
        return eRotoRoleBezierEdition;
    } else if (_imp->selectedRole == _imp->paintBrushTool) {
        return eRotoRolePaintBrush;
    } else if (_imp->selectedRole == _imp->effectBrushTool) {
        return eRotoRoleEffectBrush;
    } else if (_imp->selectedRole == _imp->cloneBrushTool) {
        return eRotoRoleCloneBrush;
    } else if (_imp->selectedRole == _imp->mergeBrushTool) {
        return eRotoRoleMergeBrush;
    }
    assert(false);
    return eRotoRoleSelection;
}

void
RotoGui::RotoGuiPrivate::drawSelectedCp(double time,
                                        const boost::shared_ptr<BezierCP> & cp,
                                        double x,
                                        double y,
                                        const Transform::Matrix3x3& transform)
{
    ///if the tangent is being dragged, color it
    bool colorLeftTangent = false;
    bool colorRightTangent = false;

    if ( (cp == rotoData->tangentBeingDragged) &&
         ( ( state == eEventStateDraggingLeftTangent) || ( state == eEventStateDraggingRightTangent) ) ) {
        colorLeftTangent = state == eEventStateDraggingLeftTangent ? true : false;
        colorRightTangent = !colorLeftTangent;
    }


    Transform::Point3D leftDeriv,rightDeriv;
    leftDeriv.z = rightDeriv.z = 1.;
    cp->getLeftBezierPointAtTime(true ,time, &leftDeriv.x, &leftDeriv.y);
    cp->getRightBezierPointAtTime(true ,time, &rightDeriv.x, &rightDeriv.y);
    leftDeriv = Transform::matApply(transform, leftDeriv);
    rightDeriv = Transform::matApply(transform, rightDeriv);

    bool drawLeftHandle = leftDeriv.x != x || leftDeriv.y != y;
    bool drawRightHandle = rightDeriv.y != x || rightDeriv.y != y;
    glBegin(GL_POINTS);
    if (drawLeftHandle) {
        if (colorLeftTangent) {
            glColor3f(0.2, 1., 0.);
        }
        glVertex2d(leftDeriv.x,leftDeriv.y);
        if (colorLeftTangent) {
            glColor3d(0.85, 0.67, 0.);
        }
    }
    if (drawRightHandle) {
        if (colorRightTangent) {
            glColor3f(0.2, 1., 0.);
        }
        glVertex2d(rightDeriv.x,rightDeriv.y);
        if (colorRightTangent) {
            glColor3d(0.85, 0.67, 0.);
        }
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    if (drawLeftHandle) {
        glVertex2d(leftDeriv.x,leftDeriv.y);
    }
    glVertex2d(x, y);
    if (drawRightHandle) {
        glVertex2d(rightDeriv.x,rightDeriv.y);
    }
    glEnd();
} // drawSelectedCp

static void
drawEllipse(double x, double y, double radiusX, double radiusY, int l, double r, double g, double b, double a)
{
    glColor3f(r*l*a, g*l*a, b*l*a);
    
    glPushMatrix();
    //  center the oval at x_center, y_center
    glTranslatef((float)x, (float)y, 0.f);
    //  draw the oval using line segments
    glBegin(GL_LINE_LOOP);
    // we don't need to be pixel-perfect here, it's just an interact!
    // 40 segments is enough.
    double m = 2 * 3.14159265358979323846264338327950288419717 / 40.;
    for (int i = 0; i < 40; ++i) {
        double theta = i * m;
        glVertex2d(radiusX * std::cos(theta), radiusY * std::sin(theta));
    }
    glEnd();
    
    glPopMatrix();
}

void
RotoGui::drawOverlays(double time,
                      double scaleX,
                      double scaleY) const
{
    
    
    std::list< boost::shared_ptr<RotoDrawableItem> > drawables = _imp->context->getCurvesByRenderOrder();
    std::pair<double,double> pixelScale;
    std::pair<double,double> viewportSize;
    
    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);
    _imp->viewer->getViewportSize(viewportSize.first, viewportSize.second);

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
        glLineWidth(1.5);
        glEnable(GL_POINT_SMOOTH);
        glPointSize(7.);
        for (std::list< boost::shared_ptr<RotoDrawableItem> >::const_iterator it = drawables.begin(); it != drawables.end(); ++it) {
            if ( !(*it)->isGloballyActivated() ) {
                continue;
            }
            
            
            
            Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(it->get());
            if (isStroke) {
                
                if (_imp->selectedTool != eRotoToolSelectAll) {
                    continue;
                }
                
                bool selected = false;
                for (SelectedItems::const_iterator it2 = _imp->rotoData->selectedItems.begin(); it2!=_imp->rotoData->selectedItems.end(); ++it2) {
                    if (it2->get() == isStroke) {
                        selected = true;
                        break;
                    }
                }
                if (!selected) {
                    continue;
                }

                std::list<std::list<std::pair<Point,double> > > strokes;
                isStroke->evaluateStroke(0, time, &strokes);
                bool locked = (*it)->isLockedRecursive();
                double curveColor[4];
                if (!locked) {
                    (*it)->getOverlayColor(curveColor);
                } else {
                    curveColor[0] = 0.8; curveColor[1] = 0.8; curveColor[2] = 0.8; curveColor[3] = 1.;
                }
                glColor4dv(curveColor);
                
                for (std::list<std::list<std::pair<Point,double> > >::iterator itStroke = strokes.begin(); itStroke != strokes.end(); ++itStroke) {
                    glBegin(GL_LINE_STRIP);
                    for (std::list<std::pair<Point,double> >::const_iterator it2 = itStroke->begin(); it2 != itStroke->end(); ++it2) {
                        glVertex2f(it2->first.x, it2->first.y);
                    }
                    glEnd();
                }
                
            } else if (isBezier) {
                ///draw the bezier
                // check if the bbox is visible
                // if the bbox is visible, compute the polygon and draw it.
                
                
                RectD bbox = isBezier->getBoundingBox(time);
                if (!_imp->viewer->isVisibleInViewport(bbox)) {
                    continue;
                }
                
                std::list< Point > points;
                isBezier->evaluateAtTime_DeCasteljau(true,time,0, 100, &points, NULL);
                
                bool locked = (*it)->isLockedRecursive();
                double curveColor[4];
                if (!locked) {
                    (*it)->getOverlayColor(curveColor);
                } else {
                    curveColor[0] = 0.8; curveColor[1] = 0.8; curveColor[2] = 0.8; curveColor[3] = 1.;
                }
                glColor4dv(curveColor);
                
                glBegin(GL_LINE_STRIP);
                for (std::list<Point >::const_iterator it2 = points.begin(); it2 != points.end(); ++it2) {
                    glVertex2f(it2->x, it2->y);
                }
                glEnd();
                
                ///draw the feather points
                std::list< Point > featherPoints;
                RectD featherBBox( std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::infinity(),
                                  -std::numeric_limits<double>::infinity(),
                                  -std::numeric_limits<double>::infinity() );
                
                bool clockWise = isBezier->isFeatherPolygonClockwiseOriented(true,time);
                
                
                if ( isFeatherVisible() ) {
                    ///Draw feather only if visible (button is toggled in the user interface)
                    isBezier->evaluateFeatherPointsAtTime_DeCasteljau(true,time,0, 100, true, &featherPoints, &featherBBox);
                    
                    if ( !featherPoints.empty() ) {
                        glLineStipple(2, 0xAAAA);
                        glEnable(GL_LINE_STIPPLE);
                        glBegin(GL_LINE_STRIP);
                        for (std::list<Point >::const_iterator it2 = featherPoints.begin(); it2 != featherPoints.end(); ++it2) {
                            glVertex2f(it2->x, it2->y);
                        }
                        glEnd();
                        glDisable(GL_LINE_STIPPLE);
                    }
                }
                
                ///draw the control points if the bezier is selected
                bool selected = false;
                for (SelectedItems::const_iterator it2 = _imp->rotoData->selectedItems.begin(); it2!=_imp->rotoData->selectedItems.end(); ++it2) {
                    if (it2->get() == isBezier) {
                        selected = true;
                        break;
                    }
                }
    
                
                if (selected && !locked) {
                    
                    Transform::Matrix3x3 transform;
                    isBezier->getTransformAtTime(time, &transform);
                    
                    const std::list< boost::shared_ptr<BezierCP> > & cps = isBezier->getControlPoints();
                    const std::list< boost::shared_ptr<BezierCP> > & featherPts = isBezier->getFeatherPoints();
                    assert( cps.size() == featherPts.size() );
                    
                    if ( cps.empty() ) {
                        continue;
                    }
                    
                    double cpHalfWidth = kControlPointMidSize * pixelScale.first;
                    double cpHalfHeight = kControlPointMidSize * pixelScale.second;
                    
                    glColor3d(0.85, 0.67, 0.);
                    
                    std::list< boost::shared_ptr<BezierCP> >::const_iterator itF = featherPts.begin();
                    int index = 0;
                    std::list< boost::shared_ptr<BezierCP> >::const_iterator prevCp = cps.end();
                    if (prevCp != cps.begin()) {
                        --prevCp;
                    }
                    std::list< boost::shared_ptr<BezierCP> >::const_iterator nextCp = cps.begin();
                    if (nextCp != cps.end()) {
                        ++nextCp;
                    }
                    for (std::list< boost::shared_ptr<BezierCP> >::const_iterator it2 = cps.begin(); it2 != cps.end();
                         ++it2) {
                        if ( nextCp == cps.end() ) {
                            nextCp = cps.begin();
                        }
                        if ( prevCp == cps.end() ) {
                            prevCp = cps.begin();
                        }
                        assert(itF != featherPts.end()); // because cps.size() == featherPts.size()
                        if (itF == featherPts.end()) {
                            break;
                        }
                        double x,y;
                        Transform::Point3D p,pF;
                        (*it2)->getPositionAtTime(true, time, &p.x, &p.y);
                        p.z = 1.;

                        double xF,yF;
                        (*itF)->getPositionAtTime(true, time, &pF.x, &pF.y);
                        pF.z = 1.;
                        
                        p = Transform::matApply(transform, p);
                        pF = Transform::matApply(transform, pF);
                        
                        x = p.x;
                        y = p.y;
                        xF = pF.x;
                        yF = pF.y;
                        
                        ///draw the feather point only if it is distinct from the associated point
                        bool drawFeather = isFeatherVisible();
                        if (drawFeather) {
                            drawFeather = !(*it2)->equalsAtTime(true, time, **itF);
                        }
                        
                        
                        ///if the control point is the only control point being dragged, color it to identify it to the user
                        bool colorChanged = false;
                        SelectedCPs::const_iterator firstSelectedCP = _imp->rotoData->selectedCps.begin();
                        if ( firstSelectedCP != _imp->rotoData->selectedCps.end() &&
                            (firstSelectedCP->first == *it2 || firstSelectedCP->second == *it2) &&
                            _imp->rotoData->selectedCps.size() == 1 &&
                            (_imp->state == eEventStateDraggingSelectedControlPoints || _imp->state == eEventStateDraggingControlPoint) ) {
                            glColor3f(0., 1., 1.);
                            colorChanged = true;
                        }
                        
                        for (SelectedCPs::const_iterator cpIt = _imp->rotoData->selectedCps.begin();
                             cpIt != _imp->rotoData->selectedCps.end();
                             ++cpIt) {
                            ///if the control point is selected, draw its tangent handles
                            if (cpIt->first == *it2) {
                                _imp->drawSelectedCp(time, cpIt->first, x, y, transform);
                                if (drawFeather) {
                                    _imp->drawSelectedCp(time, cpIt->second, xF, yF, transform);
                                }
                                glColor3f(0.2, 1., 0.);
                                colorChanged = true;
                                break;
                            } else if (cpIt->second == *it2) {
                                _imp->drawSelectedCp(time, cpIt->second, x, y, transform);
                                if (drawFeather) {
                                    _imp->drawSelectedCp(time, cpIt->first, xF, yF, transform);
                                }
                                glColor3f(0.2, 1., 0.);
                                colorChanged = true;
                                break;
                            }
                        } // for(cpIt)
                        
                        glBegin(GL_POLYGON);
                        glVertex2f(x - cpHalfWidth, y - cpHalfHeight);
                        glVertex2f(x + cpHalfWidth, y - cpHalfHeight);
                        glVertex2f(x + cpHalfWidth, y + cpHalfHeight);
                        glVertex2f(x - cpHalfWidth, y + cpHalfHeight);
                        glEnd();
                        
                        if (colorChanged) {
                            glColor3d(0.85, 0.67, 0.);
                        }
                        
                        if ( (firstSelectedCP->first == *itF)
                            && ( _imp->rotoData->selectedCps.size() == 1) &&
                            ( ( _imp->state == eEventStateDraggingSelectedControlPoints) || ( _imp->state == eEventStateDraggingControlPoint) )
                            && !colorChanged ) {
                            glColor3f(0.2, 1., 0.);
                            colorChanged = true;
                        }
                        
                        
                        double distFeatherX = 20. * pixelScale.first;
                        double distFeatherY = 20. * pixelScale.second;
                        bool isHovered = false;
                        if (_imp->rotoData->featherBarBeingHovered.first) {
                            assert(_imp->rotoData->featherBarBeingHovered.second);
                            if ( _imp->rotoData->featherBarBeingHovered.first->isFeatherPoint() ) {
                                isHovered = _imp->rotoData->featherBarBeingHovered.first == *itF;
                            } else if ( _imp->rotoData->featherBarBeingHovered.second->isFeatherPoint() ) {
                                isHovered = _imp->rotoData->featherBarBeingHovered.second == *itF;
                            }
                        }
                        
                        if (drawFeather) {
                            glBegin(GL_POLYGON);
                            glVertex2f(xF - cpHalfWidth, yF - cpHalfHeight);
                            glVertex2f(xF + cpHalfWidth, yF - cpHalfHeight);
                            glVertex2f(xF + cpHalfWidth, yF + cpHalfHeight);
                            glVertex2f(xF - cpHalfWidth, yF + cpHalfHeight);
                            glEnd();
                            
                            
                            if ( ( (_imp->state == eEventStateDraggingFeatherBar) &&
                                  ( ( *itF == _imp->rotoData->featherBarBeingDragged.first) || ( *itF == _imp->rotoData->featherBarBeingDragged.second) ) ) ||
                                isHovered ) {
                                glColor3f(0.2, 1., 0.);
                                colorChanged = true;
                            } else {
                                glColor4dv(curveColor);
                            }
                            
                            double beyondX,beyondY;
                            double dx = (xF - x);
                            double dy = (yF - y);
                            double dist = sqrt(dx * dx + dy * dy);
                            beyondX = ( dx * (dist + distFeatherX) ) / dist + x;
                            beyondY = ( dy * (dist + distFeatherY) ) / dist + y;
                            
                            ///draw a link between the feather point and the control point.
                            ///Also extend that link of 20 pixels beyond the feather point.
                            
                            glBegin(GL_LINE_STRIP);
                            glVertex2f(x, y);
                            glVertex2f(xF, yF);
                            glVertex2f(beyondX, beyondY);
                            glEnd();
                            
                            glColor3d(0.85, 0.67, 0.);
                        } else if ( isFeatherVisible() ) {
                            ///if the feather point is identical to the control point
                            ///draw a small hint line that the user can drag to move the feather point
                            if ( !isBezier->isOpenBezier() && (_imp->selectedTool == eRotoToolSelectAll || _imp->selectedTool == eRotoToolSelectFeatherPoints) ) {
                                int cpCount = (*it2)->getBezier()->getControlPointsCount();
                                if (cpCount > 1) {
                                    Natron::Point controlPoint;
                                    controlPoint.x = x;
                                    controlPoint.y = y;
                                    Natron::Point featherPoint;
                                    featherPoint.x = xF;
                                    featherPoint.y = yF;
                                    
                                    
                                    Bezier::expandToFeatherDistance(true,controlPoint, &featherPoint, distFeatherX, time, clockWise, transform, prevCp, it2, nextCp);
                                    
                                    if ( ( (_imp->state == eEventStateDraggingFeatherBar) &&
                                          ( ( *itF == _imp->rotoData->featherBarBeingDragged.first) ||
                                           ( *itF == _imp->rotoData->featherBarBeingDragged.second) ) ) || isHovered ) {
                                              glColor3f(0.2, 1., 0.);
                                              colorChanged = true;
                                          } else {
                                              glColor4dv(curveColor);
                                          }
                                    
                                    glBegin(GL_LINES);
                                    glVertex2f(x, y);
                                    glVertex2f(featherPoint.x, featherPoint.y);
                                    glEnd();
                                    
                                    glColor3d(0.85, 0.67, 0.);
                                }
                            }
                        } // isFeatherVisible()
                        
                        
                        if (colorChanged) {
                            glColor3d(0.85, 0.67, 0.);
                        }

                        // increment for next iteration
                        if (itF != featherPts.end()) {
                            ++itF;
                        }
                        if (nextCp != cps.end()) {
                            ++nextCp;
                        }
                        if (prevCp != cps.end()) {
                            ++prevCp;
                        }
                        ++index;
                    } // for(it2)
                } // if ( ( selected != _imp->rotoData->selectedBeziers.end() ) && !locked ) {
            } // if (isBezier)
            glCheckError();
        } // for (std::list< boost::shared_ptr<RotoDrawableItem> >::const_iterator it = drawables.begin(); it != drawables.end(); ++it) {
        
        if (_imp->context->isRotoPaint() &&
            (_imp->selectedRole == _imp->mergeBrushTool ||
             _imp->selectedRole == _imp->effectBrushTool ||
             _imp->selectedRole == _imp->paintBrushTool ||
             _imp->selectedRole == _imp->cloneBrushTool) &&
            _imp->selectedTool != eRotoToolOpenBezier) {
            
            QPoint widgetPos = _imp->viewer->mapToGlobal(_imp->viewer->mapFromParent(_imp->viewer->pos()));
            QRect r(widgetPos.x(),widgetPos.y(),_imp->viewer->width(),_imp->viewer->height());
            
            if (r.contains(QCursor::pos())) {
                //Draw a circle  around the cursor
                double brushSize = _imp->sizeSpinbox->value();
                
                GLdouble projection[16];
                glGetDoublev( GL_PROJECTION_MATRIX, projection);
                OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
                shadow.x = 2. / (projection[0] * viewportSize.first);
                shadow.y = 2. / (projection[5] * viewportSize.second);
                
                double halfBrush = brushSize / 2.;
                
                QPointF ellipsePos;
                if (_imp->state == eEventStateDraggingBrushSize || _imp->state == eEventStateDraggingBrushOpacity) {
                    ellipsePos = _imp->mouseCenterOnSizeChange;
                } else {
                    ellipsePos = _imp->lastMousePos;
                }
                double opacity = _imp->opacitySpinbox->value();

                for (int l = 0; l < 2; ++l) {
                    
                    glMatrixMode(GL_PROJECTION);
                    int direction = (l == 0) ? 1 : -1;
                    // translate (1,-1) pixels
                    glTranslated(direction * shadow.x, -direction * shadow.y, 0);
                    glMatrixMode(GL_MODELVIEW);
                    drawEllipse(ellipsePos.x(),ellipsePos.y(),halfBrush,halfBrush,l, 1.f, 1.f, 1.f, opacity);
                    
                    glColor3f(.5f*l*opacity, .5f*l*opacity, .5f*l*opacity);

                    
                    if ((_imp->selectedTool == eRotoToolClone || _imp->selectedTool == eRotoToolReveal) &&
                        (_imp->rotoData->cloneOffset.first != 0 || _imp->rotoData->cloneOffset.second != 0)) {
                        glBegin(GL_LINES);
                        
                        if (_imp->state == eEventStateDraggingCloneOffset) {
                            //draw a line between the center of the 2 ellipses
                            glVertex2d(ellipsePos.x(),ellipsePos.y());
                            glVertex2d(ellipsePos.x() + _imp->rotoData->cloneOffset.first,ellipsePos.y() + _imp->rotoData->cloneOffset.second);
                        }
                        //draw a cross in the center of the source ellipse
                        glVertex2d(ellipsePos.x() + _imp->rotoData->cloneOffset.first,ellipsePos.y()  + _imp->rotoData->cloneOffset.second - halfBrush);
                        glVertex2d(ellipsePos.x() + _imp->rotoData->cloneOffset.first,ellipsePos.y() +  _imp->rotoData->cloneOffset.second + halfBrush);
                        glVertex2d(ellipsePos.x() + _imp->rotoData->cloneOffset.first - halfBrush,ellipsePos.y()  + _imp->rotoData->cloneOffset.second);
                        glVertex2d(ellipsePos.x() + _imp->rotoData->cloneOffset.first + halfBrush,ellipsePos.y()  + _imp->rotoData->cloneOffset.second);
                        glEnd();
                        
                        
                        //draw the source ellipse
                        drawEllipse(ellipsePos.x() + _imp->rotoData->cloneOffset.first,ellipsePos.y() + _imp->rotoData->cloneOffset.second,halfBrush,halfBrush,l, 1.f, 1.f, 1.f, opacity / 2.);
                    }
                }
            }
        }
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

    if ( _imp->rotoData->showCpsBbox && _imp->node->isSettingsPanelVisible() ) {
        _imp->drawSelectedCpsBBOX();
    }
    glCheckError();
    
    NodePtr node = _imp->node->getNode();
    node->getLiveInstance()->setCurrentViewportForOverlays_public(_imp->viewer);
    node->drawHostOverlay(time, scaleX, scaleY);
} // drawOverlays

void
RotoGui::RotoGuiPrivate::drawArrow(double centerX,
                                   double centerY,
                                   double rotate,
                                   bool hovered,
                                   const std::pair<double,double> & pixelScale)
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
    QPointF bottom(0.,-arrowLenght);
    QPointF top(0, arrowLenght);
    ///the arrow head is 4 pixels long and kTransformArrowWidth * 2 large
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
RotoGui::RotoGuiPrivate::drawBendedArrow(double centerX,
                                         double centerY,
                                         double rotate,
                                         bool hovered,
                                         const std::pair<double,double> & pixelScale)
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
    QPointF bottom(0.,-arrowLenght / 2.);
    QPointF right(arrowLenght / 2., 0.);
    glBegin (GL_LINE_STRIP);
    glVertex2f ( bottom.x(),bottom.y() );
    glVertex2f (0., 0.);
    glVertex2f ( right.x(),right.y() );
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
RotoGui::RotoGuiPrivate::drawSelectedCpsBBOX()
{
    std::pair<double,double> pixelScale;

    viewer->getPixelScale(pixelScale.first, pixelScale.second);

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);


        QPointF topLeft = rotoData->selectedCpsBbox.topLeft();
        QPointF btmRight = rotoData->selectedCpsBbox.bottomRight();

        glLineWidth(1.5);

        if (hoverState == eHoverStateBbox) {
            glColor4f(0.9,0.5,0,1.);
        } else {
            glColor4f(0.8,0.8,0.8,1.);
        }
        glBegin(GL_LINE_LOOP);
        glVertex2f( topLeft.x(),btmRight.y() );
        glVertex2f( topLeft.x(),topLeft.y() );
        glVertex2f( btmRight.x(),topLeft.y() );
        glVertex2f( btmRight.x(),btmRight.y() );
        glEnd();

        double midX = ( topLeft.x() + btmRight.x() ) / 2.;
        double midY = ( btmRight.y() + topLeft.y() ) / 2.;
        double xHairMidSizeX = kXHairSelectedCpsBox * pixelScale.first;
        double xHairMidSizeY = kXHairSelectedCpsBox * pixelScale.second;
        QLineF selectedCpsCrossHorizLine;
        selectedCpsCrossHorizLine.setLine(midX - xHairMidSizeX, midY, midX + xHairMidSizeX, midY);
        QLineF selectedCpsCrossVertLine;
        selectedCpsCrossVertLine.setLine(midX, midY - xHairMidSizeY, midX, midY + xHairMidSizeY);

        glBegin(GL_LINES);
        glVertex2f( std::max( selectedCpsCrossHorizLine.p1().x(),topLeft.x() ),selectedCpsCrossHorizLine.p1().y() );
        glVertex2f( std::min( selectedCpsCrossHorizLine.p2().x(),btmRight.x() ),selectedCpsCrossHorizLine.p2().y() );
        glVertex2f( selectedCpsCrossVertLine.p1().x(),std::max( selectedCpsCrossVertLine.p1().y(),btmRight.y() ) );
        glVertex2f( selectedCpsCrossVertLine.p2().x(),std::min( selectedCpsCrossVertLine.p2().y(),topLeft.y() ) );
        glEnd();

        glCheckError();


        QPointF midTop( ( topLeft.x() + btmRight.x() ) / 2.,topLeft.y() );
        QPointF midRight(btmRight.x(),( topLeft.y() + btmRight.y() ) / 2.);
        QPointF midBtm( ( topLeft.x() + btmRight.x() ) / 2.,btmRight.y() );
        QPointF midLeft(topLeft.x(),( topLeft.y() + btmRight.y() ) / 2.);

        ///draw the 4 corners points and the 4 mid points
        glPointSize(5.f);
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
            if (rotoData->transformMode == eSelectedCpsTransformModeTranslateAndScale) {
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
                drawArrow(midLeft.x() - offset, midLeft.y(),0., hoverState == eHoverStateBboxMidLeft, pixelScale);
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
RotoGui::onSelectionCleared()
{
    if ( !isStickySelectionEnabled()  && !_imp->shiftDown ) {
        _imp->clearSelection();
    }
}

void
RotoGui::updateSelectionFromSelectionRectangle(bool onRelease)
{
    if ( !onRelease || !_imp->node->isSettingsPanelVisible() ) {
        return;
    }

    bool stickySel = isStickySelectionEnabled();
    if ( !stickySel && !_imp->shiftDown) {
        _imp->clearCPSSelection();
        _imp->rotoData->selectedItems.clear();
    }

    int selectionMode = -1;
    if (_imp->selectedTool == eRotoToolSelectAll) {
        selectionMode = 0;
    } else if (_imp->selectedTool == eRotoToolSelectPoints) {
        selectionMode = 1;
    } else if ( (_imp->selectedTool == eRotoToolSelectFeatherPoints) || (_imp->selectedTool == eRotoToolSelectCurves) ) {
        selectionMode = 2;
    }


    double l,r,b,t;
    _imp->viewer->getSelectionRectangle(l, r, b, t);
    std::list<boost::shared_ptr<RotoDrawableItem> > curves = _imp->context->getCurvesByRenderOrder();
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if ((*it)->isLockedRecursive()) {
            continue;
        }
        
        if (isBezier) {
            SelectedCPs points  = isBezier->controlPointsWithinRect(l, r, b, t, 0,selectionMode);
            if (_imp->selectedTool != eRotoToolSelectCurves) {
                for (SelectedCPs::iterator ptIt = points.begin(); ptIt != points.end(); ++ptIt) {
                    if ( !isFeatherVisible() && ptIt->first->isFeatherPoint() ) {
                        continue;
                    }
                    SelectedCPs::iterator foundCP = std::find(_imp->rotoData->selectedCps.begin(),_imp->rotoData->selectedCps.end(),*ptIt);
                    if (foundCP == _imp->rotoData->selectedCps.end()) {
                        if (!_imp->shiftDown || !_imp->ctrlDown) {
                            _imp->rotoData->selectedCps.push_back(*ptIt);
                        }
                    } else {
                        if (_imp->shiftDown && _imp->ctrlDown) {
                            _imp->rotoData->selectedCps.erase(foundCP);
                        }
                    }
                }
            }
            if ( !points.empty()) {
                _imp->rotoData->selectedItems.push_back(isBezier);
            }
        }
    }
    
    if (!_imp->rotoData->selectedItems.empty()) {
        _imp->context->select(_imp->rotoData->selectedItems, RotoItem::eSelectionReasonOverlayInteract);
    } else if (!stickySel && !_imp->shiftDown) {
        _imp->context->clearSelection(RotoItem::eSelectionReasonOverlayInteract);
    }
    
    
    _imp->computeSelectedCpsBBOX();
}

void
RotoGui::RotoGuiPrivate::clearSelection()
{
    clearBeziersSelection();
    clearCPSSelection();
}

bool
RotoGui::RotoGuiPrivate::hasSelection() const
{
    return !rotoData->selectedItems.empty() || !rotoData->selectedCps.empty();
}

void
RotoGui::RotoGuiPrivate::clearCPSSelection()
{
    rotoData->selectedCps.clear();
    rotoData->showCpsBbox = false;
    rotoData->transformMode = eSelectedCpsTransformModeTranslateAndScale;
    rotoData->selectedCpsBbox.setTopLeft( QPointF(0,0) );
    rotoData->selectedCpsBbox.setTopRight( QPointF(0,0) );
}

void
RotoGui::RotoGuiPrivate::clearBeziersSelection()
{
    context->clearSelection(RotoItem::eSelectionReasonOverlayInteract);
    rotoData->selectedItems.clear();
}

bool
RotoGui::RotoGuiPrivate::removeItemFromSelection(const boost::shared_ptr<RotoDrawableItem>& b)
{
    for (SelectedItems::iterator fb = rotoData->selectedItems.begin(); fb != rotoData->selectedItems.end(); ++fb) {
        if (fb->get() == b.get()) {
            context->deselect(*fb, RotoItem::eSelectionReasonOverlayInteract);
            rotoData->selectedItems.erase(fb);

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
    double x,y,xLeft,yLeft,xRight,yRight;

    p.getPositionAtTime(true, time, &x, &y);
    p.getLeftBezierPointAtTime(true, time, &xLeft, &yLeft);
    p.getRightBezierPointAtTime(true, time, &xRight, &yRight);

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

void
RotoGui::RotoGuiPrivate::computeSelectedCpsBBOX()
{
    boost::shared_ptr<Natron::Node> n = node->getNode();
    if (n && !n->isActivated()) {
        return;
    }
    double time = context->getTimelineCurrentTime();
    std::pair<double, double> pixelScale;

    viewer->getPixelScale(pixelScale.first,pixelScale.second);


    double l = INT_MAX,r = INT_MIN,b = INT_MAX,t = INT_MIN;
    for (SelectedCPs::iterator it = rotoData->selectedCps.begin(); it != rotoData->selectedCps.end(); ++it) {
        handleControlPointMaximum(time,*(it->first),&l,&b,&r,&t);
        if (it->second) {
            handleControlPointMaximum(time,*(it->second),&l,&b,&r,&t);
        }
    }
    rotoData->selectedCpsBbox.setCoords(l, t, r, b);
    if (rotoData->selectedCps.size() > 1) {
        rotoData->showCpsBbox = true;
    } else {
        rotoData->showCpsBbox = false;
    }
}

QPointF
RotoGui::RotoGuiPrivate::getSelectedCpsBBOXCenter()
{
    return rotoData->selectedCpsBbox.center();
}

void
RotoGui::RotoGuiPrivate::handleBezierSelection(const boost::shared_ptr<Bezier> & curve,
                                               QMouseEvent* e)
{
    ///find out if the bezier is already selected.
    bool found = false;
    for (SelectedItems::iterator it = rotoData->selectedItems.begin(); it != rotoData->selectedItems.end(); ++it) {
        if (it->get() == curve.get()) {
            found = true;
            break;
        }
    }
   
    if (!found) {
        ///clear previous selection if the SHIFT modifier isn't held
        if ( !modCASIsShift(e) ) {
            clearBeziersSelection();
        }
        rotoData->selectedItems.push_back(curve);
        context->select(curve, RotoItem::eSelectionReasonOverlayInteract);
    }
}

void
RotoGui::RotoGuiPrivate::handleControlPointSelection(const std::pair<boost::shared_ptr<BezierCP>,
                                                                     boost::shared_ptr<BezierCP> > & p,
                                                     QMouseEvent* e)
{
    ///find out if the cp is already selected.
    SelectedCPs::iterator foundCP = rotoData->selectedCps.end();

    for (SelectedCPs::iterator it = rotoData->selectedCps.begin(); it != rotoData->selectedCps.end(); ++it) {
        if (p.first == it->first) {
            foundCP = it;
            break;
        }
    }

    if ( foundCP == rotoData->selectedCps.end() ) {
        ///clear previous selection if the SHIFT modifier isn't held
        if ( !modCASIsShift(e) ) {
            rotoData->selectedCps.clear();
        }
        rotoData->selectedCps.push_back(p);
        computeSelectedCpsBBOX();
    } else {
        
        ///Erase the point from the selection to allow the user to toggle the selection
        if (modCASIsShift(e)) {
            rotoData->selectedCps.erase(foundCP);
            computeSelectedCpsBBOX();
        }
    }

    rotoData->cpBeingDragged = p;
    state = eEventStateDraggingControlPoint;
}

bool
RotoGui::penDown(double time,
                 double scaleX,
                 double scaleY,
                 Natron::PenType pen,
                 bool isTabletEvent,
                 const QPointF & viewportPos,
                 const QPointF & pos,
                 double pressure,
                 double timestamp,
                 QMouseEvent* e)
{
    NodePtr node = _imp->node->getNode();
    node->getLiveInstance()->setCurrentViewportForOverlays_public(_imp->viewer);
    if (node->onOverlayPenDownDefault(scaleX, scaleY, viewportPos, pos, pressure)) {
        return true;
    }
    
    std::pair<double, double> pixelScale;
    
    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);
    
    bool didSomething = false;
    double tangentSelectionTol = kTangentHandleSelectionTolerance * pixelScale.first;
    double cpSelectionTolerance = kControlPointSelectionTolerance * pixelScale.first;
    
    _imp->lastTabletDownTriggeredEraser = false;
    if (_imp->context->isRotoPaint() && isTabletEvent) {
        if (pen == ePenTypeEraser && _imp->selectedTool != eRotoToolEraserBrush) {
            onToolActionTriggered(_imp->eraserAction);
            _imp->lastTabletDownTriggeredEraser = true;
        }
    }
    
    
    
    //////////////////BEZIER SELECTION
    /////Check if the point is nearby a bezier
    ///tolerance for bezier selection
    double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
    double nearbyBezierT;
    int nearbyBezierCPIndex;
    bool isFeather;
    boost::shared_ptr<Bezier> nearbyBezier =
    _imp->context->isNearbyBezier(pos.x(), pos.y(), bezierSelectionTolerance,&nearbyBezierCPIndex,&nearbyBezierT,&isFeather);
    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > nearbyCP;
    int nearbyCpIndex = -1;
    if (nearbyBezier) {
        /////////////////CONTROL POINT SELECTION
        //////Check if the point is nearby a control point of a selected bezier
        ///Find out if the user selected a control point
        
        Bezier::ControlPointSelectionPrefEnum pref = Bezier::eControlPointSelectionPrefWhateverFirst;
        if ( (_imp->selectedTool == eRotoToolSelectFeatherPoints) && isFeatherVisible() ) {
            pref = Bezier::eControlPointSelectionPrefFeatherFirst;
        }
        
        nearbyCP = nearbyBezier->isNearbyControlPoint(pos.x(), pos.y(), cpSelectionTolerance,pref,&nearbyCpIndex);
        
    }
    
    ////////////////// TANGENT SELECTION
    ///in all cases except cusp/smooth if a control point is selected, check if the user clicked on a tangent handle
    ///in which case we go into eEventStateDraggingTangent mode
    if (!nearbyCP.first &&
        _imp->selectedTool != eRotoToolCuspPoints &&
        _imp->selectedTool != eRotoToolSmoothPoints &&
        _imp->selectedTool != eRotoToolSelectCurves) {
        
        for (SelectedCPs::iterator it = _imp->rotoData->selectedCps.begin(); it != _imp->rotoData->selectedCps.end(); ++it) {
            if ( (_imp->selectedTool == eRotoToolSelectAll) ||
                ( _imp->selectedTool == eRotoToolDrawBezier) ) {
                int ret = it->first->isNearbyTangent(true, time, pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->rotoData->tangentBeingDragged = it->first;
                    _imp->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                } else {
                    ///try with the counter part point
                    if (it->second) {
                        ret = it->second->isNearbyTangent(true, time, pos.x(), pos.y(), tangentSelectionTol);
                    }
                    if (ret >= 0) {
                        _imp->rotoData->tangentBeingDragged = it->second;
                        _imp->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                        didSomething = true;
                    }
                }
            } else if (_imp->selectedTool == eRotoToolSelectFeatherPoints) {
                const boost::shared_ptr<BezierCP> & fp = it->first->isFeatherPoint() ? it->first : it->second;
                int ret = fp->isNearbyTangent(true, time, pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->rotoData->tangentBeingDragged = fp;
                    _imp->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                }
            } else if (_imp->selectedTool == eRotoToolSelectPoints) {
                const boost::shared_ptr<BezierCP> & cp = it->first->isFeatherPoint() ? it->second : it->first;
                int ret = cp->isNearbyTangent(true, time, pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->rotoData->tangentBeingDragged = cp;
                    _imp->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                }
            }
            
            ///check in case this is a feather tangent
            if ( _imp->rotoData->tangentBeingDragged && _imp->rotoData->tangentBeingDragged->isFeatherPoint() && !isFeatherVisible() ) {
                _imp->rotoData->tangentBeingDragged.reset();
                _imp->state = eEventStateNone;
                didSomething = false;
            }
            
            if (didSomething) {
                return didSomething;
            }
        }
    }

    
    switch (_imp->selectedTool) {
        case eRotoToolSelectAll:
        case eRotoToolSelectPoints:
        case eRotoToolSelectFeatherPoints: {
            if ( ( _imp->selectedTool == eRotoToolSelectFeatherPoints) && !isFeatherVisible() ) {
                ///nothing to do
                break;
            }
            std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > featherBarSel;
            if ( ( ( _imp->selectedTool == eRotoToolSelectAll) || ( _imp->selectedTool == eRotoToolSelectFeatherPoints) ) ) {
                featherBarSel = _imp->isNearbyFeatherBar(time, pixelScale, pos);
                if ( featherBarSel.first && !isFeatherVisible() ) {
                    featherBarSel.first.reset();
                    featherBarSel.second.reset();
                }
            }
            
            
            if (nearbyBezier) {
                ///check if the user clicked nearby the cross hair of the selection rectangle in which case
                ///we drag all the control points selected
                if (nearbyCP.first) {
                    _imp->handleControlPointSelection(nearbyCP, e);
                    _imp->handleBezierSelection(nearbyBezier, e);
                    if ( buttonDownIsRight(e) ) {
                        _imp->state = eEventStateNone;
                        showMenuForControlPoint(nearbyBezier,nearbyCP);
                    }
                    didSomething = true;
                } else if (featherBarSel.first) {
                    _imp->clearCPSSelection();
                    _imp->rotoData->featherBarBeingDragged = featherBarSel;
                    
                    ///Also select the point only if the curve is the same!
                    if ( featherBarSel.first->getBezier() == nearbyBezier ) {
                        _imp->handleControlPointSelection(_imp->rotoData->featherBarBeingDragged, e);
                        _imp->handleBezierSelection(nearbyBezier, e);
                    }
                    _imp->state = eEventStateDraggingFeatherBar;
                    didSomething = true;
                } else {
                    
                    bool found = false;
                    for (SelectedItems::iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
                        if (it->get() == nearbyBezier.get()) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        _imp->handleBezierSelection(nearbyBezier, e);
                        
                    }
                    if (buttonDownIsLeft(e)) {
                        _imp->state = eEventStateDraggingSelectedControlPoints;
                        _imp->rotoData->bezierBeingDragged = nearbyBezier;
                    } else if ( buttonDownIsRight(e) ) {
                        showMenuForCurve(nearbyBezier);
                        
                    }
                    didSomething = true;
                }
            } else {
                
                if (featherBarSel.first) {
                    _imp->clearCPSSelection();
                    _imp->rotoData->featherBarBeingDragged = featherBarSel;
                    _imp->handleControlPointSelection(_imp->rotoData->featherBarBeingDragged, e);
                    _imp->state = eEventStateDraggingFeatherBar;
                    didSomething = true;
                }
                if (_imp->state == eEventStateNone) {
                    
                    _imp->state = _imp->isMouseInteractingWithCPSBbox(pos,cpSelectionTolerance,pixelScale);
                    if (_imp->state != eEventStateNone) {
                        didSomething = true;
                    }
                }
            }
            break;
        }
        case eRotoToolSelectCurves:
            
            if (nearbyBezier) {
                ///If the bezier is already selected and we re-click on it, change the transform mode
                bool found = false;
                for (SelectedItems::iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
                    if (it->get() == nearbyBezier.get()) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    _imp->handleBezierSelection(nearbyBezier, e);
                }
                if ( buttonDownIsRight(e) ) {
                    showMenuForCurve(nearbyBezier);
                }
                didSomething = true;
            } else {
                if (_imp->state == eEventStateNone) {
                    _imp->state = _imp->isMouseInteractingWithCPSBbox(pos,cpSelectionTolerance,pixelScale);
                    if (_imp->state != eEventStateNone) {
                        didSomething = true;
                    }
                }
            }
            break;
        case eRotoToolAddPoints:
            ///If the user clicked on a bezier and this bezier is selected add a control point by
            ///splitting up the targeted segment
            if (nearbyBezier) {
                bool found = false;
                for (SelectedItems::iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
                    if (it->get() == nearbyBezier.get()) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    ///check that the point is not too close to an existing point
                    if (nearbyCP.first) {
                        _imp->handleControlPointSelection(nearbyCP, e);
                    } else {
                        pushUndoCommand( new AddPointUndoCommand(this,nearbyBezier,nearbyBezierCPIndex,nearbyBezierT) );
                        _imp->evaluateOnPenUp = true;
                    }
                    didSomething = true;
                }
            }
            break;
        case eRotoToolRemovePoints:
            if (nearbyCP.first) {
                assert( nearbyBezier && nearbyBezier == nearbyCP.first->getBezier() );
                if ( nearbyCP.first->isFeatherPoint() ) {
                    pushUndoCommand( new RemovePointUndoCommand(this,nearbyBezier,nearbyCP.second) );
                } else {
                    pushUndoCommand( new RemovePointUndoCommand(this,nearbyBezier,nearbyCP.first) );
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
                pushUndoCommand( new RemoveFeatherUndoCommand(this,datas) );
                didSomething = true;
            }
            break;
        case eRotoToolOpenCloseCurve:
            if (nearbyBezier) {
                pushUndoCommand( new OpenCloseUndoCommand(this,nearbyBezier) );
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
                pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,false,pixelScale) );
                didSomething = true;
            }
            break;
        case eRotoToolCuspPoints:
            if ( nearbyCP.first && _imp->context->isAutoKeyingEnabled() ) {
                std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
                SmoothCuspUndoCommand::SmoothCuspCurveData data;
                data.curve = nearbyBezier;
                data.newPoints.push_back(nearbyCP);
                datas.push_back(data);
                pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,true,pixelScale) );
                didSomething = true;
            }
            break;
        case eRotoToolDrawBezier:
        case eRotoToolOpenBezier: {
            if ( _imp->rotoData->builtBezier && _imp->rotoData->builtBezier->isCurveFinished() ) {
                _imp->rotoData->builtBezier.reset();
                _imp->clearSelection();
                onToolActionTriggered(_imp->selectAllAction);
                
                return true;
            }
            if (_imp->rotoData->builtBezier) {
                ///if the user clicked on a control point of the bezier, select the point instead.
                ///if that point is the starting point of the curve, close the curve
                const std::list<boost::shared_ptr<BezierCP> > & cps = _imp->rotoData->builtBezier->getControlPoints();
                int i = 0;
                for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin(); it != cps.end(); ++it, ++i) {
                    double x,y;
                    (*it)->getPositionAtTime(true, time, &x, &y);
                    if ( ( x >= (pos.x() - cpSelectionTolerance) ) && ( x <= (pos.x() + cpSelectionTolerance) ) &&
                        ( y >= (pos.y() - cpSelectionTolerance) ) && ( y <= (pos.y() + cpSelectionTolerance) ) ) {
                        if ( it == cps.begin() ) {
                            pushUndoCommand( new OpenCloseUndoCommand(this,_imp->rotoData->builtBezier) );
                            
                            _imp->rotoData->builtBezier.reset();
                            
                            _imp->rotoData->selectedCps.clear();
                            onToolActionTriggered(_imp->selectAllAction);
                        } else {
                            boost::shared_ptr<BezierCP> fp = _imp->rotoData->builtBezier->getFeatherPointAtIndex(i);
                            assert(fp);
                            _imp->handleControlPointSelection(std::make_pair(*it, fp), e);
                        }
                        
                        return true;
                    }
                }
            }
            
            bool isOpenBezier = _imp->selectedTool == eRotoToolOpenBezier;
            MakeBezierUndoCommand* cmd = new MakeBezierUndoCommand(this,_imp->rotoData->builtBezier,isOpenBezier,true,pos.x(),pos.y(),time);
            pushUndoCommand(cmd);
            _imp->rotoData->builtBezier = cmd->getCurve();
            assert(_imp->rotoData->builtBezier);
            _imp->state = eEventStateBuildingBezierControlPointTangent;
            didSomething = true;
            break;
        }
        case eRotoToolDrawBSpline:
            
            break;
        case eRotoToolDrawEllipse: {
            _imp->rotoData->click = pos;
            pushUndoCommand( new MakeEllipseUndoCommand(this, true, false, false, pos.x(), pos.y(), pos.x(), pos.y(), time) );
            _imp->state = eEventStateBuildingEllipse;
            didSomething = true;
            break;
        }
        case eRotoToolDrawRectangle: {
            _imp->rotoData->click = pos;
            pushUndoCommand( new MakeRectangleUndoCommand(this, true, false, false, pos.x(), pos.y(), pos.x(), pos.y(), time) );
            _imp->evaluateOnPenUp = true;
            _imp->state = eEventStateBuildingRectangle;
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
            
            if ((_imp->selectedTool == eRotoToolClone || _imp->selectedTool == eRotoToolReveal) && modCASIsControl(e)) {
                _imp->state = eEventStateDraggingCloneOffset;
            } else if (modCASIsShift(e)) {
                _imp->state = eEventStateDraggingBrushSize;
                _imp->mouseCenterOnSizeChange = pos;
            } else if (modCASIsControlShift(e)) {
                _imp->state = eEventStateDraggingBrushOpacity;
                _imp->mouseCenterOnSizeChange = pos;
            } else {
                
                /*
                 Check that all viewers downstream are connected directly to the RotoPaint to avoid glitches and bugs
                 */
                _imp->checkViewersAreDirectlyConnected();
                
                if (_imp->rotoData->strokeBeingPaint &&
                    _imp->rotoData->strokeBeingPaint->getParentLayer() && 
                    _imp->multiStrokeEnabled->isChecked()) {
                    
                    
                    boost::shared_ptr<RotoLayer> layer = _imp->rotoData->strokeBeingPaint->getParentLayer();
                    if (!layer) {
                        layer = _imp->context->findDeepestSelectedLayer();
                        if (!layer) {
                            layer = _imp->context->getOrCreateBaseLayer();
                        }
                        assert(layer);
                        _imp->context->addItem(layer, 0, _imp->rotoData->strokeBeingPaint, RotoItem::eSelectionReasonOther);
                    }

                    _imp->context->getNode()->getApp()->setUserIsPainting(_imp->context->getNode(),_imp->rotoData->strokeBeingPaint, true);

                    boost::shared_ptr<KnobInt> lifeTimeFrameKnob = _imp->rotoData->strokeBeingPaint->getLifeTimeFrameKnob();
                    lifeTimeFrameKnob->setValue(_imp->context->getTimelineCurrentTime(), 0);
                    
                    _imp->rotoData->strokeBeingPaint->appendPoint(true, RotoPoint(pos.x(), pos.y(), pressure, timestamp));
                } else {
                    if (_imp->rotoData->strokeBeingPaint &&
                        !_imp->rotoData->strokeBeingPaint->getParentLayer() &&
                        _imp->multiStrokeEnabled->isChecked()) {
                        _imp->rotoData->strokeBeingPaint.reset();
                    }
                    _imp->makeStroke(false, RotoPoint(pos.x(), pos.y(), pressure, timestamp));
                }
                _imp->context->evaluateChange();
                _imp->state = eEventStateBuildingStroke;
                _imp->viewer->setCursor(Qt::BlankCursor);
            }
            didSomething = true;
            break;
        }
        default:
            assert(false);
            break;
    } // switch

    _imp->lastClickPos = pos;
    _imp->lastMousePos = pos;

    return didSomething;
} // penDown

bool
RotoGui::penDoubleClicked(double /*time*/,
                          double /*scaleX*/,
                          double /*scaleY*/,
                          const QPointF & /*viewportPos*/,
                          const QPointF & pos,
                          QMouseEvent* e)
{
    bool didSomething = false;
    std::pair<double, double> pixelScale;

    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);

    if (_imp->selectedTool == eRotoToolSelectAll) {
        double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
        double nearbyBezierT;
        int nearbyBezierCPIndex;
        bool isFeather;
        boost::shared_ptr<Bezier> nearbyBezier =
            _imp->context->isNearbyBezier(pos.x(), pos.y(), bezierSelectionTolerance,&nearbyBezierCPIndex,&nearbyBezierT,&isFeather);
    
        if (nearbyBezier) {
            ///If the bezier is already selected and we re-click on it, change the transform mode
            _imp->handleBezierSelection(nearbyBezier, e);
            _imp->clearCPSSelection();
            const std::list<boost::shared_ptr<BezierCP> > & cps = nearbyBezier->getControlPoints();
            const std::list<boost::shared_ptr<BezierCP> > & fps = nearbyBezier->getFeatherPoints();
            assert( cps.size() == fps.size() );
            std::list<boost::shared_ptr<BezierCP> >::const_iterator itCp = cps.begin();
            std::list<boost::shared_ptr<BezierCP> >::const_iterator itFp = fps.begin();
            for (; itCp != cps.end(); ++itCp, ++itFp) {
                _imp->rotoData->selectedCps.push_back( std::make_pair(*itCp, *itFp) );
            }
            if (_imp->rotoData->selectedCps.size() > 1) {
                _imp->computeSelectedCpsBBOX();
            }
            didSomething = true;
        }
    }

    return didSomething;
}

bool
RotoGui::penMotion(double time,
                   double scaleX,
                   double scaleY,
                   const QPointF & viewportPos,
                   const QPointF & pos,
                   double pressure,
                   double timestamp,
                   QInputEvent* e)
{
    NodePtr node = _imp->node->getNode();
    node->getLiveInstance()->setCurrentViewportForOverlays_public(_imp->viewer);
    if (node->onOverlayPenMotionDefault(scaleX, scaleY, viewportPos, pos, pressure)) {
        return true;
    }
    
    std::pair<double, double> pixelScale;

    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);

    bool didSomething = false;
    HoverStateEnum lastHoverState = _imp->hoverState;
    ///Set the cursor to the appropriate case
    bool cursorSet = false;
    
    double cpTol = kControlPointSelectionTolerance * pixelScale.first;
    
    if (_imp->context->isRotoPaint() &&
        (_imp->selectedRole == _imp->mergeBrushTool ||
         _imp->selectedRole == _imp->cloneBrushTool ||
         _imp->selectedRole == _imp->effectBrushTool ||
         _imp->selectedRole == _imp->paintBrushTool)) {
        if (_imp->state != eEventStateBuildingStroke) {
            _imp->viewer->setCursor(Qt::CrossCursor);
        } else {
            _imp->viewer->setCursor(Qt::BlankCursor);
        }
        didSomething = true;
        cursorSet = true;
    }
    
    if ( !cursorSet && _imp->rotoData->showCpsBbox && (_imp->state != eEventStateDraggingControlPoint) && (_imp->state != eEventStateDraggingSelectedControlPoints)
        && ( _imp->state != eEventStateDraggingLeftTangent) &&
        ( _imp->state != eEventStateDraggingRightTangent) ) {
        double bboxTol = cpTol;
        if ( _imp->isNearbyBBoxBtmLeft(pos, bboxTol,pixelScale) ) {
            _imp->hoverState = eHoverStateBboxBtmLeft;
            didSomething = true;
        } else if ( _imp->isNearbyBBoxBtmRight(pos,bboxTol,pixelScale) ) {
            _imp->hoverState = eHoverStateBboxBtmRight;
            didSomething = true;
        } else if ( _imp->isNearbyBBoxTopRight(pos, bboxTol,pixelScale) ) {
            _imp->hoverState = eHoverStateBboxTopRight;
            didSomething = true;
        } else if ( _imp->isNearbyBBoxTopLeft(pos, bboxTol,pixelScale) ) {
            _imp->hoverState = eHoverStateBboxTopLeft;
            didSomething = true;
        } else if ( _imp->isNearbyBBoxMidTop(pos, bboxTol,pixelScale) ) {
            _imp->hoverState = eHoverStateBboxMidTop;
            didSomething = true;
        } else if ( _imp->isNearbyBBoxMidRight(pos, bboxTol,pixelScale) ) {
            _imp->hoverState = eHoverStateBboxMidRight;
            didSomething = true;
        } else if ( _imp->isNearbyBBoxMidBtm(pos, bboxTol,pixelScale) ) {
            _imp->hoverState = eHoverStateBboxMidBtm;
            didSomething = true;
        } else if ( _imp->isNearbyBBoxMidLeft(pos, bboxTol,pixelScale) ) {
            _imp->hoverState = eHoverStateBboxMidLeft;
            didSomething = true;
        } else {
            _imp->hoverState = eHoverStateNothing;
            didSomething = true;
        }
    }
    if (_imp->state == eEventStateNone && _imp->hoverState == eHoverStateNothing) {
        if ( (_imp->state != eEventStateDraggingControlPoint) && (_imp->state != eEventStateDraggingSelectedControlPoints) ) {
            for (SelectedItems::const_iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
                int index = -1;
                Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
                if (isBezier) {
                    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > nb =
                    isBezier->isNearbyControlPoint(pos.x(), pos.y(), cpTol,Bezier::eControlPointSelectionPrefWhateverFirst,&index);
                    if ( (index != -1) && ( ( !nb.first->isFeatherPoint() && !isFeatherVisible() ) || isFeatherVisible() ) ) {
                        _imp->viewer->setCursor( QCursor(Qt::CrossCursor) );
                        cursorSet = true;
                        break;
                    }
                }
            }
        }
        if ( !cursorSet && (_imp->state != eEventStateDraggingLeftTangent) && (_imp->state != eEventStateDraggingRightTangent) ) {
            ///find a nearby tangent
            for (SelectedCPs::const_iterator it = _imp->rotoData->selectedCps.begin(); it != _imp->rotoData->selectedCps.end(); ++it) {
                if (it->first->isNearbyTangent(true, time, pos.x(), pos.y(), cpTol) != -1) {
                    _imp->viewer->setCursor( QCursor(Qt::CrossCursor) );
                    cursorSet = true;
                    break;
                }
            }
        }
        if ( !cursorSet && (_imp->state != eEventStateDraggingControlPoint) && (_imp->state != eEventStateDraggingSelectedControlPoints) && (_imp->state != eEventStateDraggingLeftTangent) &&
            ( _imp->state != eEventStateDraggingRightTangent) ) {
            double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
            double nearbyBezierT;
            int nearbyBezierCPIndex;
            bool isFeather;
            boost::shared_ptr<Bezier> nearbyBezier =
            _imp->context->isNearbyBezier(pos.x(), pos.y(), bezierSelectionTolerance,&nearbyBezierCPIndex,&nearbyBezierT,&isFeather);
            if ( isFeather && !isFeatherVisible() ) {
                nearbyBezier.reset();
            }
            if ( nearbyBezier) {
                _imp->viewer->setCursor( QCursor(Qt::PointingHandCursor) );
                cursorSet = true;
            }
        }
        
        bool clickAnywhere = _imp->isBboxClickAnywhereEnabled();
        
        if ( !cursorSet && (_imp->rotoData->selectedCps.size() > 1)) {
            if ((clickAnywhere && _imp->isWithinSelectedCpsBBox(pos)) ||
                (!clickAnywhere && _imp->isNearbySelectedCpsCrossHair(pos))) {
                _imp->viewer->setCursor( QCursor(Qt::SizeAllCursor) );
                cursorSet = true;
            }
        }
        
        SelectedCP nearbyFeatherBar;
        if ( !cursorSet && isFeatherVisible() ) {
            nearbyFeatherBar = _imp->isNearbyFeatherBar(time, pixelScale, pos);
            if (nearbyFeatherBar.first && nearbyFeatherBar.second) {
                _imp->rotoData->featherBarBeingHovered = nearbyFeatherBar;
            }
        }
        if (!nearbyFeatherBar.first || !nearbyFeatherBar.second) {
            _imp->rotoData->featherBarBeingHovered.first.reset();
            _imp->rotoData->featherBarBeingHovered.second.reset();
        }
        
        if ( (_imp->state != eEventStateNone) || _imp->rotoData->featherBarBeingHovered.first || cursorSet || (lastHoverState != eHoverStateNothing) ) {
            didSomething = true;
        }
    }
    
    
    if (!cursorSet) {
        _imp->viewer->unsetCursor();
    }


    double dx = pos.x() - _imp->lastMousePos.x();
    double dy = pos.y() - _imp->lastMousePos.y();
    switch (_imp->state) {
    case eEventStateDraggingSelectedControlPoints: {
        
        if (_imp->rotoData->bezierBeingDragged) {
            SelectedCPs cps;
            const std::list<boost::shared_ptr<BezierCP> >& c = _imp->rotoData->bezierBeingDragged->getControlPoints();
            const std::list<boost::shared_ptr<BezierCP> >& f = _imp->rotoData->bezierBeingDragged->getFeatherPoints();
            assert(c.size() == f.size() || !_imp->rotoData->bezierBeingDragged->useFeatherPoints());
            bool useFeather = _imp->rotoData->bezierBeingDragged->useFeatherPoints();
            std::list<boost::shared_ptr<BezierCP> >::const_iterator itFp = f.begin();
            for (std::list<boost::shared_ptr<BezierCP> >::const_iterator itCp = c.begin(); itCp != c.end(); ++itCp) {
                if (useFeather) {
                    cps.push_back(std::make_pair(*itCp,*itFp));
                    ++itFp;
                } else {
                    cps.push_back(std::make_pair(*itCp,boost::shared_ptr<BezierCP>()));
                }
            }
            pushUndoCommand( new MoveControlPointsUndoCommand(this,cps,dx,dy,time) );
        } else {
            pushUndoCommand( new MoveControlPointsUndoCommand(this,_imp->rotoData->selectedCps,dx,dy,time) );
        }
        _imp->evaluateOnPenUp = true;
        _imp->computeSelectedCpsBBOX();
        didSomething = true;
        break;
    }
    case eEventStateDraggingControlPoint: {
        assert(_imp->rotoData->cpBeingDragged.first);
        std::list<SelectedCP> toDrag;
        toDrag.push_back(_imp->rotoData->cpBeingDragged);
        pushUndoCommand( new MoveControlPointsUndoCommand(this,toDrag,dx,dy,time) );
        _imp->evaluateOnPenUp = true;
        _imp->computeSelectedCpsBBOX();
        didSomething = true;
    };  break;
    case eEventStateBuildingBezierControlPointTangent: {
        assert(_imp->rotoData->builtBezier);
        bool isOpenBezier = _imp->selectedTool == eRotoToolOpenBezier;
        pushUndoCommand( new MakeBezierUndoCommand(this,_imp->rotoData->builtBezier,isOpenBezier, false,dx,dy,time) );
        break;
    }
    case eEventStateBuildingEllipse: {
        bool fromCenter = modifierHasControl(e);
        bool constrained = modifierHasShift(e);
        pushUndoCommand( new MakeEllipseUndoCommand(this, false, fromCenter, constrained, _imp->rotoData->click.x(), _imp->rotoData->click.y(), pos.x(), pos.y(), time) );

        didSomething = true;
        _imp->evaluateOnPenUp = true;
        break;
    }
    case eEventStateBuildingRectangle: {
        bool fromCenter = modifierHasControl(e);
        bool constrained = modifierHasShift(e);
        pushUndoCommand( new MakeRectangleUndoCommand(this, false, fromCenter, constrained, _imp->rotoData->click.x(), _imp->rotoData->click.y(), pos.x(), pos.y(), time) );
        didSomething = true;
        _imp->evaluateOnPenUp = true;
        break;
    }
    case eEventStateDraggingLeftTangent: {
        assert(_imp->rotoData->tangentBeingDragged);
        pushUndoCommand( new MoveTangentUndoCommand( this,dx,dy,time,_imp->rotoData->tangentBeingDragged,true,
                                                     modCASIsControl(e) ) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingRightTangent: {
        assert(_imp->rotoData->tangentBeingDragged);
        pushUndoCommand( new MoveTangentUndoCommand( this,dx,dy,time,_imp->rotoData->tangentBeingDragged,false,
                                                     modCASIsControl(e) ) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingFeatherBar: {
        pushUndoCommand( new MoveFeatherBarUndoCommand(this,dx,dy,_imp->rotoData->featherBarBeingDragged,time) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingBBoxTopLeft:
    case eEventStateDraggingBBoxTopRight:
    case eEventStateDraggingBBoxBtmRight:
    case eEventStateDraggingBBoxBtmLeft: {
        QPointF center = _imp->getSelectedCpsBBOXCenter();
        double rot = 0;
        double sx = 1.,sy = 1.;

        if (_imp->rotoData->transformMode == eSelectedCpsTransformModeRotateAndSkew) {
            double angle = std::atan2( pos.y() - center.y(), pos.x() - center.x() );
            double prevAngle = std::atan2( _imp->lastMousePos.y() - center.y(), _imp->lastMousePos.x() - center.x() );
            rot = angle - prevAngle;
        } else {
            // the scale ratio is the ratio of distances to the center
            double prevDist = ( _imp->lastMousePos.x() - center.x() ) * ( _imp->lastMousePos.x() - center.x() ) +
                              ( _imp->lastMousePos.y() - center.y() ) * ( _imp->lastMousePos.y() - center.y() );
            if (prevDist != 0) {
                double dist = ( pos.x() - center.x() ) * ( pos.x() - center.x() ) + ( pos.y() - center.y() ) * ( pos.y() - center.y() );
                double ratio = std::sqrt(dist / prevDist);
                sx *= ratio;
                sy *= ratio;
            }
        }

        double tx = 0., ty = 0.;
        double skewX = 0.,skewY = 0.;
        pushUndoCommand( new TransformUndoCommand(this,center.x(),center.y(),rot,skewX,skewY,tx,ty,sx,sy,time) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingBBoxMidTop:
    case eEventStateDraggingBBoxMidBtm:
    case eEventStateDraggingBBoxMidLeft:
    case eEventStateDraggingBBoxMidRight: {
        QPointF center = _imp->getSelectedCpsBBOXCenter();
        double rot = 0;
        double sx = 1.,sy = 1.;
        double skewX = 0.,skewY = 0.;
        double tx = 0., ty = 0.;
        
        TransformUndoCommand::TransformPointsSelectionEnum type = TransformUndoCommand::eTransformAllPoints;
        if (!modCASIsShift(e)) {
            type = TransformUndoCommand::eTransformAllPoints;
        } else {
            if (_imp->state == eEventStateDraggingBBoxMidTop) {
                type = TransformUndoCommand::eTransformMidTop;
            } else if (_imp->state == eEventStateDraggingBBoxMidBtm) {
                type = TransformUndoCommand::eTransformMidBottom;
            } else if (_imp->state == eEventStateDraggingBBoxMidLeft) {
                type = TransformUndoCommand::eTransformMidLeft;
            } else if (_imp->state == eEventStateDraggingBBoxMidRight) {
                type = TransformUndoCommand::eTransformMidRight;
            }
        }
        
        const QRectF& bbox = _imp->rotoData->selectedCpsBbox;
        
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
        
        bool processX = _imp->state == eEventStateDraggingBBoxMidRight || _imp->state == eEventStateDraggingBBoxMidLeft;
        
        if (_imp->rotoData->transformMode == eSelectedCpsTransformModeRotateAndSkew) {
            if (!processX) {
                const double addSkew = ( pos.x() - _imp->lastMousePos.x() ) / ( pos.y() - center.y() );
                skewX += addSkew;
            } else {
                const double addSkew = ( pos.y() - _imp->lastMousePos.y() ) / ( pos.x() - center.x() );
                skewY += addSkew;
            }
        } else {
            // the scale ratio is the ratio of distances to the center
            double prevDist = ( _imp->lastMousePos.x() - center.x() ) * ( _imp->lastMousePos.x() - center.x() ) +
                              ( _imp->lastMousePos.y() - center.y() ) * ( _imp->lastMousePos.y() - center.y() );
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
    

        
        pushUndoCommand( new TransformUndoCommand(this,center.x(),center.y(),rot,skewX,skewY,tx,ty,sx,sy,time) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateBuildingStroke: {
        if (_imp->rotoData->strokeBeingPaint) {
            RotoPoint p(pos.x(), pos.y(), pressure, timestamp);
            if (_imp->rotoData->strokeBeingPaint->appendPoint(false,p)) {
                _imp->lastMousePos = pos;
                _imp->context->evaluateChange_noIncrement();
                return true;
            }
        }
        break;
    }
    case eEventStateDraggingCloneOffset: {
        _imp->rotoData->cloneOffset.first -= dx;
        _imp->rotoData->cloneOffset.second -= dy;
        onBreakMultiStrokeTriggered();
    }   break;
    case eEventStateDraggingBrushSize: {
        double size = _imp->sizeSpinbox->value();
        size += ((dx + dy) / 2.);
        _imp->sizeSpinbox->setValue(std::max(1.,size));
        onBreakMultiStrokeTriggered();
        didSomething = true;
    }   break;
    case eEventStateDraggingBrushOpacity: {
        double opa = _imp->opacitySpinbox->value();
        double newOpa = opa + ((dx + dy) / 2.);
        if (opa != 0) {
            newOpa = std::max(0.,std::min(1.,newOpa / opa));
            newOpa = newOpa > 0 ? std::min(1.,opa + 0.05) : std::max(0.,opa - 0.05);
        } else {
            newOpa = newOpa < 0 ? .0 : 0.05;
        }
        _imp->opacitySpinbox->setValue(newOpa);
        onBreakMultiStrokeTriggered();
        didSomething = true;
    }   break;
    case eEventStateNone:
    default:
        break;
    } // switch
    _imp->lastMousePos = pos;

    return didSomething;
} // penMotion

void
RotoGui::moveSelectedCpsWithKeyArrows(int x,
                                      int y)
{
    if ( !_imp->rotoData->selectedCps.empty() ) {
        std::pair<double,double> pixelScale;
        _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);
        double time = _imp->context->getTimelineCurrentTime();
        pushUndoCommand( new MoveControlPointsUndoCommand(this,_imp->rotoData->selectedCps,(double)x * pixelScale.first,
                                                          (double)y * pixelScale.second,time) );
        _imp->computeSelectedCpsBBOX();
        _imp->context->evaluateChange();
        _imp->node->getNode()->getApp()->triggerAutoSave();
        _imp->viewerTab->onRotoEvaluatedForThisViewer();
    }
}

void
RotoGui::evaluate(bool redraw)
{
    if (redraw) {
        _imp->viewer->redraw();
    }
    _imp->context->evaluateChange();
    _imp->node->getNode()->getApp()->triggerAutoSave();
    _imp->viewerTab->onRotoEvaluatedForThisViewer();
}

void
RotoGui::autoSaveAndRedraw()
{
    _imp->viewer->redraw();
    _imp->node->getNode()->getApp()->triggerAutoSave();
}

bool
RotoGui::penUp(double /*time*/,
               double scaleX,
               double scaleY,
               const QPointF & viewportPos,
               const QPointF & pos,
               double pressure ,
               double /*timestamp*/,
               QMouseEvent* /*e*/)
{
    NodePtr node = _imp->node->getNode();
    node->getLiveInstance()->setCurrentViewportForOverlays_public(_imp->viewer);
    if (node->onOverlayPenUpDefault(scaleX, scaleY, viewportPos, pos, pressure)) {
        return true;
    }
    
    if (_imp->evaluateOnPenUp) {
        _imp->context->evaluateChange();
        node->getApp()->triggerAutoSave();

        //sync other viewers linked to this roto
        _imp->viewerTab->onRotoEvaluatedForThisViewer();
        _imp->evaluateOnPenUp = false;
    }
    _imp->rotoData->tangentBeingDragged.reset();
    _imp->rotoData->bezierBeingDragged.reset();
    _imp->rotoData->cpBeingDragged.first.reset();
    _imp->rotoData->cpBeingDragged.second.reset();
    _imp->rotoData->featherBarBeingDragged.first.reset();
    _imp->rotoData->featherBarBeingDragged.second.reset();
    
    if (_imp->state == eEventStateDraggingBBoxMidLeft ||
        _imp->state == eEventStateDraggingBBoxMidLeft ||
        _imp->state == eEventStateDraggingBBoxMidTop ||
        _imp->state == eEventStateDraggingBBoxMidBtm ||
        _imp->state == eEventStateDraggingBBoxTopLeft ||
        _imp->state == eEventStateDraggingBBoxTopRight ||
        _imp->state == eEventStateDraggingBBoxBtmRight ||
        _imp->state == eEventStateDraggingBBoxBtmLeft) {
        refreshSelectionBBox();
    }
    
    if (_imp->state == eEventStateBuildingStroke) {
        assert(_imp->rotoData->strokeBeingPaint);
        _imp->context->getNode()->getApp()->setUserIsPainting(_imp->context->getNode(),_imp->rotoData->strokeBeingPaint, false);
        assert(_imp->rotoData->strokeBeingPaint->getParentLayer());
        _imp->rotoData->strokeBeingPaint->setStrokeFinished();
        if (!_imp->multiStrokeEnabled->isChecked()) {
            pushUndoCommand(new AddStrokeUndoCommand(this,_imp->rotoData->strokeBeingPaint));
            _imp->makeStroke(true, RotoPoint());
        } else {
            pushUndoCommand(new AddMultiStrokeUndoCommand(this,_imp->rotoData->strokeBeingPaint));
        }
        
        /**
         * Do a neat render for the stroke (better interpolation). This call is blocking otherwise the user might
         * attempt to make a new stroke while the previous stroke is not finished... this would yield artifacts.
         **/
        _imp->viewer->setCursor(QCursor(Qt::BusyCursor));
        _imp->context->evaluateNeatStrokeRender();
        _imp->viewer->unsetCursor();
    }
    
    _imp->state = eEventStateNone;

    if ( (_imp->selectedTool == eRotoToolDrawEllipse) || (_imp->selectedTool == eRotoToolDrawRectangle) ) {
        _imp->rotoData->selectedCps.clear();
        onToolActionTriggered(_imp->selectAllAction);
    }

    if (_imp->lastTabletDownTriggeredEraser) {
        onToolActionTriggered(_imp->lastPaintToolAction);
    }
    
    return true;
}

void
RotoGui::onTimelineTimeChanged()
{
    if (_imp->selectedTool == eRotoToolBlur ||
        _imp->selectedTool == eRotoToolBurn ||
        _imp->selectedTool == eRotoToolDodge ||
        _imp->selectedTool == eRotoToolClone ||
        _imp->selectedTool == eRotoToolEraserBrush ||
        _imp->selectedTool == eRotoToolSolidBrush ||
        _imp->selectedTool == eRotoToolReveal ||
        _imp->selectedTool == eRotoToolSmear ||
        _imp->selectedTool == eRotoToolSharpen) {
        _imp->makeStroke(true, RotoPoint());
    }
}

void
RotoGui::onBreakMultiStrokeTriggered()
{
    _imp->makeStroke(true, RotoPoint());
}

static bool isBranchConnectedToRotoNodeRecursive(Node* node,
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
    int maxInputs = node->getMaxInputCount();
    *recursion = *recursion + 1;
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr inp = node->getInput(i);
        if (inp) {
            if (isBranchConnectedToRotoNodeRecursive(inp.get(),rotoNode,recursion,markedNodes)) {
                return true;
            }
        }
    }
    return false;
}

void
RotoGui::RotoGuiPrivate::checkViewersAreDirectlyConnected()
{
    boost::shared_ptr<Node> rotoNode = context->getNode();
    std::list<ViewerInstance*> viewers;
    rotoNode->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance*>::iterator it = viewers.begin(); it!=viewers.end(); ++it) {
        NodePtr viewerNode = (*it)->getNode();
        int maxInputs = viewerNode->getMaxInputCount();
        int hasBranchConnectedToRoto = -1;
        for (int i = 0; i < maxInputs; ++i) {
            NodePtr input = viewerNode->getInput(i);
            if (input) {
                std::list<Node*> markedNodes;
                int recursion = 0;
                if (isBranchConnectedToRotoNodeRecursive(input.get(), rotoNode.get(), &recursion, markedNodes)) {
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
RotoGui::RotoGuiPrivate::makeStroke(bool prepareForLater, const RotoPoint& p)
{
    Natron::RotoStrokeType strokeType;
    std::string itemName;
    switch (selectedTool) {
        case eRotoToolSolidBrush:
            strokeType = Natron::eRotoStrokeTypeSolid;
            itemName = kRotoPaintBrushBaseName;
            break;
        case eRotoToolEraserBrush:
            strokeType = Natron::eRotoStrokeTypeEraser;
            itemName = kRotoPaintEraserBaseName;
            break;
        case eRotoToolClone:
            strokeType = Natron::eRotoStrokeTypeClone;
            itemName = kRotoPaintCloneBaseName;
            break;
        case eRotoToolReveal:
            strokeType = Natron::eRotoStrokeTypeReveal;
            itemName = kRotoPaintRevealBaseName;
            break;
        case eRotoToolBlur:
            strokeType = Natron::eRotoStrokeTypeBlur;
            itemName = kRotoPaintBlurBaseName;
            break;
        case eRotoToolSharpen:
            strokeType = Natron::eRotoStrokeTypeSharpen;
            itemName = kRotoPaintSharpenBaseName;
            break;
        case eRotoToolSmear:
            strokeType = Natron::eRotoStrokeTypeSmear;
            itemName = kRotoPaintSmearBaseName;
            break;
        case eRotoToolDodge:
            strokeType = Natron::eRotoStrokeTypeDodge;
            itemName = kRotoPaintDodgeBaseName;
            break;
        case eRotoToolBurn:
            strokeType = Natron::eRotoStrokeTypeBurn;
            itemName = kRotoPaintBurnBaseName;
            break;
        default:
            return;
    }
    
    if (prepareForLater || !rotoData->strokeBeingPaint) {
        
        if (rotoData->strokeBeingPaint &&
            rotoData->strokeBeingPaint->getBrushType() == strokeType &&
            rotoData->strokeBeingPaint->isEmpty()) {
            ///We already have a fresh stroke prepared for that type
            return;
        }
            
        std::string name = context->generateUniqueName(itemName);
        rotoData->strokeBeingPaint.reset(new RotoStrokeItem(strokeType, context, name, boost::shared_ptr<RotoLayer>()));
        rotoData->strokeBeingPaint->createNodes(false);
    }
   
    
    assert(rotoData->strokeBeingPaint);
    boost::shared_ptr<KnobColor> colorKnob = rotoData->strokeBeingPaint->getColorKnob();
    boost::shared_ptr<KnobChoice> operatorKnob = rotoData->strokeBeingPaint->getOperatorKnob();
    boost::shared_ptr<KnobDouble> opacityKnob = rotoData->strokeBeingPaint->getOpacityKnob();
    boost::shared_ptr<KnobDouble> sizeKnob = rotoData->strokeBeingPaint->getBrushSizeKnob();
    boost::shared_ptr<KnobDouble> hardnessKnob = rotoData->strokeBeingPaint->getBrushHardnessKnob();
    boost::shared_ptr<KnobBool> pressureOpaKnob = rotoData->strokeBeingPaint->getPressureOpacityKnob();
    boost::shared_ptr<KnobBool> pressureSizeKnob = rotoData->strokeBeingPaint->getPressureSizeKnob();
    boost::shared_ptr<KnobBool> pressureHardnessKnob = rotoData->strokeBeingPaint->getPressureHardnessKnob();
    boost::shared_ptr<KnobBool> buildUpKnob = rotoData->strokeBeingPaint->getBuildupKnob();
    boost::shared_ptr<KnobChoice> timeOffsetModeKnob = rotoData->strokeBeingPaint->getTimeOffsetModeKnob();
    boost::shared_ptr<KnobChoice> sourceTypeKnob = rotoData->strokeBeingPaint->getBrushSourceTypeKnob();
    boost::shared_ptr<KnobInt> timeOffsetKnob = rotoData->strokeBeingPaint->getTimeOffsetKnob();
    boost::shared_ptr<KnobDouble> translateKnob = rotoData->strokeBeingPaint->getBrushCloneTranslateKnob();
    
    const QColor& color = colorPickerLabel->getCurrentColor();
    MergingFunctionEnum compOp = (MergingFunctionEnum)compositingOperatorButton->activeIndex();
    double opacity = opacitySpinbox->value();
    double size = sizeSpinbox->value();
    double hardness = hardnessSpinBox->value();
    bool pressOpa = pressureOpacityButton->isDown();
    bool pressSize = pressureSizeButton->isDown();
    bool pressHarness = pressureHardnessButton->isDown();
    bool buildUp = buildUpButton->isDown();
    double timeOffset = timeOffsetSpinbox->value();
    double timeOffsetMode_i = timeOffsetMode->activeIndex();
    int sourceType_i = sourceTypeCombobox->activeIndex();
    

    double r = Natron::Color::from_func_srgb(color.redF());
    double g = Natron::Color::from_func_srgb(color.greenF());
    double b = Natron::Color::from_func_srgb(color.blueF());

    colorKnob->setValues(r,g,b, Natron::eValueChangedReasonNatronGuiEdited);
    operatorKnob->setValueFromLabel(getNatronOperationString(compOp),0);
    opacityKnob->setValue(opacity, 0);
    sizeKnob->setValue(size, 0);
    hardnessKnob->setValue(hardness, 0);
    pressureOpaKnob->setValue(pressOpa, 0);
    pressureSizeKnob->setValue(pressSize, 0);
    pressureHardnessKnob->setValue(pressHarness, 0);
    buildUpKnob->setValue(buildUp, 0);
    if (!prepareForLater) {
        boost::shared_ptr<KnobInt> lifeTimeFrameKnob = rotoData->strokeBeingPaint->getLifeTimeFrameKnob();
        lifeTimeFrameKnob->setValue(context->getTimelineCurrentTime(), 0);
    }
    if (strokeType == Natron::eRotoStrokeTypeClone || strokeType == Natron::eRotoStrokeTypeReveal) {
        timeOffsetKnob->setValue(timeOffset, 0);
        timeOffsetModeKnob->setValue(timeOffsetMode_i, 0);
        sourceTypeKnob->setValue(sourceType_i, 0);
        translateKnob->setValues(-rotoData->cloneOffset.first, -rotoData->cloneOffset.second, Natron::eValueChangedReasonNatronGuiEdited);
    }
    if (!prepareForLater) {
        boost::shared_ptr<RotoLayer> layer = context->findDeepestSelectedLayer();
        if (!layer) {
            layer = context->getOrCreateBaseLayer();
        }
        assert(layer);
        context->addItem(layer, 0, rotoData->strokeBeingPaint, RotoItem::eSelectionReasonOther);
        context->getNode()->getApp()->setUserIsPainting(context->getNode(),rotoData->strokeBeingPaint, true);
        rotoData->strokeBeingPaint->appendPoint(true,p);
    }
    
    //context->clearSelection(RotoItem::eSelectionReasonOther);
    //context->select(rotoData->strokeBeingPaint, RotoItem::eSelectionReasonOther);
}

void
RotoGui::removeCurve(const boost::shared_ptr<RotoDrawableItem>& curve)
{
    if (curve == _imp->rotoData->builtBezier) {
        _imp->rotoData->builtBezier.reset();
    } else if (curve == _imp->rotoData->strokeBeingPaint) {
        _imp->rotoData->strokeBeingPaint.reset();
    }
    _imp->context->removeItem(curve);
}

bool
RotoGui::keyDown(double /*time*/,
                 double scaleX,
                 double scaleY,
                 QKeyEvent* e)
{
    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    NodePtr node = _imp->node->getNode();
    node->getLiveInstance()->setCurrentViewportForOverlays_public(_imp->viewer);
    if (node->onOverlayKeyDownDefault(scaleX, scaleY, natronKey, natronMod)) {
        return true;
    }
    
    bool didSomething = false;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();

    if (key == Qt::Key_Shift) {
        ++_imp->shiftDown;
    } else if (key == Qt::Key_Control) {
        ++_imp->ctrlDown;
    }
    
    if ( modCASIsControl(e) ) {
        if ( !_imp->iSelectingwithCtrlA && _imp->rotoData->showCpsBbox && (e->key() == Qt::Key_Control) ) {
            _imp->rotoData->transformMode = _imp->rotoData->transformMode == eSelectedCpsTransformModeTranslateAndScale ?
                                            eSelectedCpsTransformModeRotateAndSkew : eSelectedCpsTransformModeTranslateAndScale;
            didSomething = true;
        }
    }

    if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoDelete, modifiers, key) ) {
        ///if control points are selected, delete them, otherwise delete the selected beziers
        if ( !_imp->rotoData->selectedCps.empty() ) {
            pushUndoCommand( new RemovePointUndoCommand(this,_imp->rotoData->selectedCps) );
            didSomething = true;
        } else if (!_imp->rotoData->selectedItems.empty()) {
            pushUndoCommand( new RemoveCurveUndoCommand(this,_imp->rotoData->selectedItems) );
            didSomething = true;
        }
    } else if ( (key == Qt::Key_Escape && (_imp->selectedTool == eRotoToolDrawBezier || _imp->selectedTool == eRotoToolOpenBezier)) || isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCloseBezier, modifiers, key) ) {
        if ( (_imp->selectedTool == eRotoToolDrawBezier || _imp->selectedTool == eRotoToolOpenBezier) && _imp->rotoData->builtBezier && !_imp->rotoData->builtBezier->isCurveFinished() ) {
            
            pushUndoCommand( new OpenCloseUndoCommand(this,_imp->rotoData->builtBezier) );
            
            _imp->rotoData->builtBezier.reset();
            _imp->rotoData->selectedCps.clear();
            onToolActionTriggered(_imp->selectAllAction);
            _imp->context->evaluateChange();
            didSomething = true;
        }
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSelectAll, modifiers, key) ) {
        _imp->iSelectingwithCtrlA = true;
        ///if no bezier are selected, select all beziers
        if ( _imp->rotoData->selectedItems.empty() ) {
            std::list<boost::shared_ptr<RotoDrawableItem> > bez = _imp->context->getCurvesByRenderOrder();
            for (std::list<boost::shared_ptr<RotoDrawableItem> >::const_iterator it = bez.begin(); it != bez.end(); ++it) {
                _imp->context->select(*it, RotoItem::eSelectionReasonOverlayInteract);
                _imp->rotoData->selectedItems.push_back(*it);
            }
        } else {
            ///select all the control points of all selected beziers
            _imp->rotoData->selectedCps.clear();
            for (SelectedItems::iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
                Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
                if (!isBezier) {
                    continue;
                }
                const std::list<boost::shared_ptr<BezierCP> > & cps = isBezier->getControlPoints();
                const std::list<boost::shared_ptr<BezierCP> > & fps = isBezier->getFeatherPoints();
                assert( cps.size() == fps.size() );

                std::list<boost::shared_ptr<BezierCP> >::const_iterator cpIT = cps.begin();
                for (std::list<boost::shared_ptr<BezierCP> >::const_iterator fpIT = fps.begin(); fpIT != fps.end(); ++fpIT, ++cpIT) {
                    _imp->rotoData->selectedCps.push_back( std::make_pair(*cpIT, *fpIT) );
                }
            }
            _imp->computeSelectedCpsBBOX();
        }
        didSomething = true;
    } else if ( _imp->state != eEventStateBuildingStroke && isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSelectionTool, modifiers, key) ) {
        _imp->selectTool->handleSelection();
        didSomething = true;
    } else if ( _imp->state != eEventStateBuildingStroke && isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoEditTool, modifiers, key) ) {
        if (_imp->bezierEditionTool) {
            _imp->bezierEditionTool->handleSelection();
            didSomething = true;
        }
    } else if ( _imp->state != eEventStateBuildingStroke && isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoAddTool, modifiers, key) ) {
        if (_imp->pointsEditionTool) {
            _imp->pointsEditionTool->handleSelection();
            didSomething = true;
        }
    } else if ( _imp->state != eEventStateBuildingStroke && isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoBrushTool, modifiers, key) ) {
        if (_imp->paintBrushTool) {
            _imp->paintBrushTool->handleSelection();
            didSomething = true;
        }
    } else if ( _imp->state != eEventStateBuildingStroke && isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCloneTool, modifiers, key) ) {
        if (_imp->cloneBrushTool) {
            _imp->cloneBrushTool->handleSelection();
            didSomething = true;
        }
    } else if ( _imp->state != eEventStateBuildingStroke && isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoEffectTool, modifiers, key) ) {
        if (_imp->effectBrushTool) {
            _imp->effectBrushTool->handleSelection();
            didSomething = true;
        }
    } else if ( _imp->state != eEventStateBuildingStroke && isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoColorTool, modifiers, key) ) {
        if (_imp->mergeBrushTool) {
            _imp->mergeBrushTool->handleSelection();
            didSomething = true;
        }
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeRight, modifiers, key) ) {
        moveSelectedCpsWithKeyArrows(1,0);
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeLeft, modifiers, key) ) {
        moveSelectedCpsWithKeyArrows(-1,0);
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeTop, modifiers, key) ) {
        moveSelectedCpsWithKeyArrows(0,1);
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeBottom, modifiers, key) ) {
        moveSelectedCpsWithKeyArrows(0,-1);
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSmooth, modifiers, key) ) {
        smoothSelectedCurve();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCuspBezier, modifiers, key) ) {
        cuspSelectedCurve();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoRemoveFeather, modifiers, key) ) {
        removeFeatherForSelectedCurve();
        didSomething = true;
    } else if (key == Qt::Key_Escape) {
        _imp->clearSelection();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoLockCurve, modifiers, key) ) {
        lockSelectedCurves();
        didSomething = true;
    }

    return didSomething;
} // keyDown

bool
RotoGui::keyRepeat(double /*time*/,
                   double scaleX,
                   double scaleY,
                   QKeyEvent* e)
{
    NodePtr node = _imp->node->getNode();
    node->getLiveInstance()->setCurrentViewportForOverlays_public(_imp->viewer);
    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    if (node->onOverlayKeyRepeatDefault(scaleX, scaleY, natronKey, natronMod)) {
        return true;
    }
    
    bool didSomething = false;

    if ( (e->key() == Qt::Key_Right) && modCASIsAlt(e) ) {
        moveSelectedCpsWithKeyArrows(1,0);
        didSomething = true;
    } else if ( (e->key() == Qt::Key_Left) && modCASIsAlt(e) ) {
        moveSelectedCpsWithKeyArrows(-1,0);
        didSomething = true;
    } else if ( (e->key() == Qt::Key_Up) && modCASIsAlt(e) ) {
        moveSelectedCpsWithKeyArrows(0,1);
        didSomething = true;
    } else if ( (e->key() == Qt::Key_Down) && modCASIsAlt(e) ) {
        moveSelectedCpsWithKeyArrows(0,-1);
        didSomething = true;
    }

    return didSomething;
}

void
RotoGui::focusOut(double /*time*/)
{
    _imp->shiftDown = 0;
    _imp->ctrlDown = 0;
}

bool
RotoGui::keyUp(double /*time*/,
               double scaleX,
               double scaleY,
               QKeyEvent* e)
{
    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    NodePtr node = _imp->node->getNode();
    node->getLiveInstance()->setCurrentViewportForOverlays_public(_imp->viewer);
    if (node->onOverlayKeyUpDefault(scaleX, scaleY, natronKey, natronMod)) {
        return true;
    }
    
    bool didSomething = false;

    if (e->key() == Qt::Key_Shift) {
        --_imp->shiftDown;
    } else if (e->key() == Qt::Key_Control) {
        --_imp->ctrlDown;
    }

    
    if ( !modCASIsControl(e) ) {
        if ( !_imp->iSelectingwithCtrlA && _imp->rotoData->showCpsBbox && (e->key() == Qt::Key_Control) ) {
            _imp->rotoData->transformMode = (_imp->rotoData->transformMode == eSelectedCpsTransformModeTranslateAndScale ?
                                             eSelectedCpsTransformModeRotateAndSkew : eSelectedCpsTransformModeTranslateAndScale);
            didSomething = true;
        }
    }

    if ( (e->key() == Qt::Key_Control) && _imp->iSelectingwithCtrlA ) {
        _imp->iSelectingwithCtrlA = false;
    }

    if (_imp->evaluateOnKeyUp) {
        _imp->context->evaluateChange();
        _imp->node->getNode()->getApp()->triggerAutoSave();
        _imp->viewerTab->onRotoEvaluatedForThisViewer();
        _imp->evaluateOnKeyUp = false;
    }

    return didSomething;
}

bool
RotoGui::RotoGuiPrivate::isNearbySelectedCpsCrossHair(const QPointF & pos) const
{
    std::pair<double, double> pixelScale;

    viewer->getPixelScale(pixelScale.first,pixelScale.second);

    double xHairMidSizeX = kXHairSelectedCpsBox * pixelScale.first;
    double xHairMidSizeY = kXHairSelectedCpsBox * pixelScale.second;
    double l = rotoData->selectedCpsBbox.topLeft().x();
    double r = rotoData->selectedCpsBbox.bottomRight().x();
    double b = rotoData->selectedCpsBbox.bottomRight().y();
    double t = rotoData->selectedCpsBbox.topLeft().y();
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
RotoGui::RotoGuiPrivate::isWithinSelectedCpsBBox(const QPointF& pos) const
{
 //   std::pair<double, double> pixelScale;
//    viewer->getPixelScale(pixelScale.first,pixelScale.second);
    
    double l = rotoData->selectedCpsBbox.topLeft().x();
    double r = rotoData->selectedCpsBbox.bottomRight().x();
    double b = rotoData->selectedCpsBbox.bottomRight().y();
    double t = rotoData->selectedCpsBbox.topLeft().y();
    
    double toleranceX = 0;//kXHairSelectedCpsTolerance * pixelScale.first;
    double toleranceY = 0;//kXHairSelectedCpsTolerance * pixelScale.second;
    
    return pos.x() > (l - toleranceX) && pos.x() < (r + toleranceX) &&
    pos.y() > (b - toleranceY) && pos.y() < (t + toleranceY);
}

bool
RotoGui::RotoGuiPrivate::isNearbyBBoxTopLeft(const QPointF & p,
                                             double tolerance,
                                             const std::pair<double, double> & pixelScale) const
{
    QPointF corner = rotoData->selectedCpsBbox.topLeft();

    if ( ( p.x() >= (corner.x() - tolerance) ) && ( p.x() <= (corner.x() + tolerance) ) &&
         ( p.y() >= (corner.y() - tolerance) ) && ( p.y() <= (corner.y() + tolerance) ) ) {
        return true;
    } else {
        double halfOffset = kTransformArrowOffsetFromPoint * pixelScale.first / 2.;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(corner.x() - halfOffset,corner.y() + halfOffset);
        RectD arrowBbox(center.x() - halfLength,center.y() - halfLength,center.x() + halfLength,center.y() + halfLength);

        return arrowBbox.contains( p.x(),p.y() );
    }
}

bool
RotoGui::RotoGuiPrivate::isNearbyBBoxTopRight(const QPointF & p,
                                              double tolerance,
                                              const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = rotoData->selectedCpsBbox.topLeft();
    QPointF btmRight = rotoData->selectedCpsBbox.bottomRight();
    QPointF corner( btmRight.x(),topLeft.y() );

    if ( ( p.x() >= (corner.x() - tolerance) ) && ( p.x() <= (corner.x() + tolerance) ) &&
         ( p.y() >= (corner.y() - tolerance) ) && ( p.y() <= (corner.y() + tolerance) ) ) {
        return true;
    } else {
        double halfOffset = kTransformArrowOffsetFromPoint * pixelScale.first / 2.;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(corner.x() + halfOffset,corner.y() + halfOffset);
        RectD arrowBbox(center.x() - halfLength,center.y() - halfLength,center.x() + halfLength,center.y() + halfLength);

        return arrowBbox.contains( p.x(),p.y() );
    }
}

bool
RotoGui::RotoGuiPrivate::isNearbyBBoxBtmLeft(const QPointF & p,
                                             double tolerance,
                                             const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = rotoData->selectedCpsBbox.topLeft();
    QPointF btmRight = rotoData->selectedCpsBbox.bottomRight();
    QPointF corner( topLeft.x(),btmRight.y() );

    if ( ( p.x() >= (corner.x() - tolerance) ) && ( p.x() <= (corner.x() + tolerance) ) &&
         ( p.y() >= (corner.y() - tolerance) ) && ( p.y() <= (corner.y() + tolerance) ) ) {
        return true;
    } else {
        double halfOffset = kTransformArrowOffsetFromPoint * pixelScale.first / 2.;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(corner.x() - halfOffset,corner.y() - halfOffset);
        RectD arrowBbox(center.x() - halfLength,center.y() - halfLength,center.x() + halfLength,center.y() + halfLength);

        return arrowBbox.contains( p.x(),p.y() );
    }
}

bool
RotoGui::RotoGuiPrivate::isNearbyBBoxBtmRight(const QPointF & p,
                                              double tolerance,
                                              const std::pair<double, double> & pixelScale) const
{
    QPointF corner = rotoData->selectedCpsBbox.bottomRight();

    if ( ( p.x() >= (corner.x() - tolerance) ) && ( p.x() <= (corner.x() + tolerance) ) &&
         ( p.y() >= (corner.y() - tolerance) ) && ( p.y() <= (corner.y() + tolerance) ) ) {
        return true;
    } else {
        double halfOffset = kTransformArrowOffsetFromPoint * pixelScale.first / 2.;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(corner.x() + halfOffset,corner.y() - halfOffset);
        RectD arrowBbox(center.x() - halfLength,center.y() - halfLength,center.x() + halfLength,center.y() + halfLength);

        return arrowBbox.contains( p.x(),p.y() );
    }
}

bool
RotoGui::RotoGuiPrivate::isNearbyBBoxMidTop(const QPointF & p,
                                            double tolerance,
                                            const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = rotoData->selectedCpsBbox.topLeft();
    QPointF btmRight = rotoData->selectedCpsBbox.bottomRight();
    QPointF topRight( btmRight.x(),topLeft.y() );
    QPointF mid = (topLeft + topRight) / 2.;

    if ( ( p.x() >= (mid.x() - tolerance) ) && ( p.x() <= (mid.x() + tolerance) ) &&
         ( p.y() >= (mid.y() - tolerance) ) && ( p.y() <= (mid.y() + tolerance) ) ) {
        return true;
    } else {
        double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;
        ///test if pos is within the arrow bounding box
        QPointF center(mid.x(),mid.y() + offset);
        RectD arrowBbox(center.x() - halfLength,center.y() - halfLength,center.x() + halfLength,center.y() + halfLength);

        return arrowBbox.contains( p.x(),p.y() );
    }
}

bool
RotoGui::RotoGuiPrivate::isNearbyBBoxMidRight(const QPointF & p,
                                              double tolerance,
                                              const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = rotoData->selectedCpsBbox.topLeft();
    QPointF btmRight = rotoData->selectedCpsBbox.bottomRight();
    QPointF topRight( btmRight.x(),topLeft.y() );
    QPointF mid = (btmRight + topRight) / 2.;

    if ( ( p.x() >= (mid.x() - tolerance) ) && ( p.x() <= (mid.x() + tolerance) ) &&
         ( p.y() >= (mid.y() - tolerance) ) && ( p.y() <= (mid.y() + tolerance) ) ) {
        return true;
    } else {
        double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center( mid.x() + offset,mid.y() );
        RectD arrowBbox(center.x() - halfLength,center.y() - halfLength,center.x() + halfLength,center.y() + halfLength);

        return arrowBbox.contains( p.x(),p.y() );
    }
}

bool
RotoGui::RotoGuiPrivate::isNearbyBBoxMidBtm(const QPointF & p,
                                            double tolerance,
                                            const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = rotoData->selectedCpsBbox.topLeft();
    QPointF btmRight = rotoData->selectedCpsBbox.bottomRight();
    QPointF btmLeft( topLeft.x(),btmRight.y() );
    QPointF mid = (btmRight + btmLeft) / 2.;

    if ( ( p.x() >= (mid.x() - tolerance) ) && ( p.x() <= (mid.x() + tolerance) ) &&
         ( p.y() >= (mid.y() - tolerance) ) && ( p.y() <= (mid.y() + tolerance) ) ) {
        return true;
    } else {
        double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center(mid.x(),mid.y() - offset);
        RectD arrowBbox(center.x() - halfLength,center.y() - halfLength,center.x() + halfLength,center.y() + halfLength);

        return arrowBbox.contains( p.x(),p.y() );
    }
}

EventStateEnum
RotoGui::RotoGuiPrivate::isMouseInteractingWithCPSBbox(const QPointF& pos,double cpSelectionTolerance,const std::pair<double, double>& pixelScale) const
{
    bool clickAnywhere = isBboxClickAnywhereEnabled();

    EventStateEnum state = eEventStateNone;
    if ( rotoData->showCpsBbox && isNearbyBBoxTopLeft(pos, cpSelectionTolerance,pixelScale) ) {
        state = eEventStateDraggingBBoxTopLeft;
    } else if ( rotoData->showCpsBbox && isNearbyBBoxTopRight(pos, cpSelectionTolerance,pixelScale) ) {
        state = eEventStateDraggingBBoxTopRight;
    } else if ( rotoData->showCpsBbox && isNearbyBBoxBtmLeft(pos, cpSelectionTolerance,pixelScale) ) {
        state = eEventStateDraggingBBoxBtmLeft;
    } else if ( rotoData->showCpsBbox && isNearbyBBoxBtmRight(pos, cpSelectionTolerance,pixelScale) ) {
        state = eEventStateDraggingBBoxBtmRight;
    } else if ( rotoData->showCpsBbox && isNearbyBBoxMidTop(pos, cpSelectionTolerance,pixelScale) ) {
        state = eEventStateDraggingBBoxMidTop;
    } else if ( rotoData->showCpsBbox && isNearbyBBoxMidRight(pos, cpSelectionTolerance,pixelScale) ) {
        state = eEventStateDraggingBBoxMidRight;
    } else if ( rotoData->showCpsBbox && isNearbyBBoxMidBtm(pos, cpSelectionTolerance,pixelScale) ) {
        state = eEventStateDraggingBBoxMidBtm;
    } else if ( rotoData->showCpsBbox && isNearbyBBoxMidLeft(pos, cpSelectionTolerance,pixelScale) ) {
        state = eEventStateDraggingBBoxMidLeft;
    } else if ( clickAnywhere && rotoData->showCpsBbox && isWithinSelectedCpsBBox(pos) ) {
        state = eEventStateDraggingSelectedControlPoints;
    } else if ( !clickAnywhere && rotoData->showCpsBbox && isNearbySelectedCpsCrossHair(pos) ) {
        state = eEventStateDraggingSelectedControlPoints;
    }
    return state;
}

bool
RotoGui::RotoGuiPrivate::isNearbyBBoxMidLeft(const QPointF & p,
                                             double tolerance,
                                             const std::pair<double, double> & pixelScale) const
{
    QPointF topLeft = rotoData->selectedCpsBbox.topLeft();
    QPointF btmRight = rotoData->selectedCpsBbox.bottomRight();
    QPointF btmLeft( topLeft.x(),btmRight.y() );
    QPointF mid = (topLeft + btmLeft) / 2.;

    if ( ( p.x() >= (mid.x() - tolerance) ) && ( p.x() <= (mid.x() + tolerance) ) &&
         ( p.y() >= (mid.y() - tolerance) ) && ( p.y() <= (mid.y() + tolerance) ) ) {
        return true;
    } else {
        double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
        double length = kTransformArrowLenght * pixelScale.first;
        double halfLength = length / 2.;;
        ///test if pos is within the arrow bounding box
        QPointF center( mid.x() - offset,mid.y() );
        RectD arrowBbox(center.x() - halfLength,center.y() - halfLength,center.x() + halfLength,center.y() + halfLength);

        return arrowBbox.contains( p.x(),p.y() );
    }
}

bool
RotoGui::RotoGuiPrivate::isNearbySelectedCpsBoundingBox(const QPointF & pos,
                                                        double tolerance) const
{
    QPointF topLeft = rotoData->selectedCpsBbox.topLeft();
    QPointF btmRight = rotoData->selectedCpsBbox.bottomRight();
    QPointF btmLeft( topLeft.x(),btmRight.y() );
    QPointF topRight( btmRight.x(),topLeft.y() );

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

std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >
RotoGui::RotoGuiPrivate::isNearbyFeatherBar(double time,
                                            const std::pair<double,double> & pixelScale,
                                            const QPointF & pos) const
{
    double distFeatherX = 20. * pixelScale.first;
    double acceptance = 10 * pixelScale.second;

    for (SelectedItems::const_iterator it = rotoData->selectedItems.begin(); it != rotoData->selectedItems.end(); ++it) {
        
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(it->get());
        assert(isStroke || isBezier);
        if (isStroke || !isBezier || (isBezier && isBezier->isOpenBezier())) {
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
        isBezier->getTransformAtTime(time, &transform);
        
        const std::list<boost::shared_ptr<BezierCP> > & fps = isBezier->getFeatherPoints();
        const std::list<boost::shared_ptr<BezierCP> > & cps = isBezier->getControlPoints();
        assert( cps.size() == fps.size() );

        int cpCount = (int)cps.size();
        if (cpCount <= 1) {
            continue;
        }
//        std::list<Point> polygon;
//        RectD polygonBBox( std::numeric_limits<double>::infinity(),
//                           std::numeric_limits<double>::infinity(),
//                           -std::numeric_limits<double>::infinity(),
//                           -std::numeric_limits<double>::infinity() );
//        (*it)->evaluateFeatherPointsAtTime_DeCasteljau(time, 0, 50, true, &polygon, &polygonBBox);

        std::list<boost::shared_ptr<BezierCP> >::const_iterator itF = fps.begin();
        std::list<boost::shared_ptr<BezierCP> >::const_iterator nextF = itF;
        if (nextF != fps.end()) {
            ++nextF;
        }
        std::list<boost::shared_ptr<BezierCP> >::const_iterator prevF = fps.end();
        if (prevF != fps.begin()) {
            --prevF;
        }
        bool isClockWiseOriented = isBezier->isFeatherPolygonClockwiseOriented(true,time);
        
        for (std::list<boost::shared_ptr<BezierCP> >::const_iterator itCp = cps.begin();
             itCp != cps.end();
             ++itCp) {
            if ( prevF == fps.end() ) {
                prevF = fps.begin();
            }
            if ( nextF == fps.end() ) {
                nextF = fps.begin();
            }
            assert(itF != fps.end()); // because cps.size() == fps.size()
            if (itF == fps.end()) {
                itF = fps.begin();
            }

            Transform::Point3D controlPoint,featherPoint;
            controlPoint.z = featherPoint.z = 1;
            (*itCp)->getPositionAtTime(true, time, &controlPoint.x, &controlPoint.y);
            (*itF)->getPositionAtTime(true, time, &featherPoint.x, &featherPoint.y);

            controlPoint = Transform::matApply(transform, controlPoint);
            featherPoint = Transform::matApply(transform, featherPoint);
            {
                Natron::Point cp,fp;
                cp.x = controlPoint.x;
                cp.y = controlPoint.y;
                fp.x = featherPoint.x;
                fp.y = featherPoint.y;
                Bezier::expandToFeatherDistance(true,cp, &fp, distFeatherX, time, isClockWiseOriented, transform, prevF, itF, nextF);
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
            if (itF != fps.end()) {
                ++itF;
            }
            if (nextF != fps.end()) {
                ++nextF;
            }
            if (prevF != fps.end()) {
                ++prevF;
            }
        } // for(itCp)
    }

    return std::make_pair( boost::shared_ptr<BezierCP>(), boost::shared_ptr<BezierCP>() );
} // isNearbyFeatherBar

void
RotoGui::onAutoKeyingButtonClicked(bool e)
{
    _imp->autoKeyingEnabled->setDown(e);
    _imp->context->onAutoKeyingChanged(e);
}

void
RotoGui::onFeatherLinkButtonClicked(bool e)
{
    _imp->featherLinkEnabled->setDown(e);
    _imp->context->onFeatherLinkChanged(e);
}

void
RotoGui::onRippleEditButtonClicked(bool e)
{
    _imp->rippleEditEnabled->setDown(e);
    _imp->context->onRippleEditChanged(e);
}

void
RotoGui::onStickySelectionButtonClicked(bool e)
{
    _imp->stickySelectionEnabled->setDown(e);
}

void
RotoGui::onBboxClickButtonClicked(bool e)
{
    _imp->bboxClickAnywhere->setDown(e);
}

bool
RotoGui::isStickySelectionEnabled() const
{
    return _imp->stickySelectionEnabled ? _imp->stickySelectionEnabled->isChecked() : false;
}

void
RotoGui::onAddKeyFrameClicked()
{
    double time = _imp->context->getTimelineCurrentTime();

    for (SelectedItems::iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (!isBezier) {
            continue;
        }
        isBezier->setKeyframe(time);
    }
    _imp->context->evaluateChange();
}

void
RotoGui::onRemoveKeyFrameClicked()
{
    double time = _imp->context->getTimelineCurrentTime();

    for (SelectedItems::iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (!isBezier) {
            continue;
        }
        isBezier->removeKeyframe(time);
    }
    _imp->context->evaluateChange();
}

void
RotoGui::onCurrentFrameChanged(SequenceTime /*time*/,
                               int)
{
    _imp->computeSelectedCpsBBOX();
}

void
RotoGui::restoreSelectionFromContext()
{
    _imp->rotoData->selectedItems = _imp->context->getSelectedCurves();
}

void
RotoGui::onRefreshAsked()
{
    _imp->viewer->redraw();
}

void
RotoGui::RotoGuiPrivate::onCurveLockedChangedRecursive(const boost::shared_ptr<RotoItem> & item,
                                                       bool* ret)
{
    boost::shared_ptr<Bezier> b = boost::dynamic_pointer_cast<Bezier>(item);
    boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(item);

    if (b) {
        if ( item->isLockedRecursive() ) {
            for (SelectedItems::iterator fb = rotoData->selectedItems.begin(); fb != rotoData->selectedItems.end(); ++fb) {
                if ( fb->get() == b.get() ) {
                    ///if the curve was selected, wipe the selection CP bbox
                    clearCPSSelection();
                    rotoData->selectedItems.erase(fb);
                    *ret = true;
                    break;
                }
            }
        } else {
            ///Explanation: This change has been made in result to a user click on the settings panel.
            ///We have to reselect the bezier overlay hence put a reason different of eSelectionReasonOverlayInteract
            SelectedItems::iterator found = std::find(rotoData->selectedItems.begin(),rotoData->selectedItems.end(),b);
            if ( found == rotoData->selectedItems.end() ) {
                rotoData->selectedItems.push_back(b);
                context->select(b, RotoItem::eSelectionReasonSettingsPanel);
                *ret  = true;
            }
        }
    } else if (layer) {
        const std::list<boost::shared_ptr<RotoItem> > & items = layer->getItems();
        for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
            onCurveLockedChangedRecursive(*it, ret);
        }
    }
}

void
RotoGui::onCurveLockedChanged(int reason)
{
    boost::shared_ptr<RotoItem> item = _imp->context->getLastItemLocked();
    if (item && (RotoItem::SelectionReasonEnum)reason != RotoItem::eSelectionReasonOverlayInteract) {
        assert(item);
        bool changed = false;
        if (item) {
            _imp->onCurveLockedChangedRecursive(item, &changed);
        }
        
        if (changed) {
            _imp->viewer->redraw();
        }
    }
}

void
RotoGui::onSelectionChanged(int reason)
{
    if ( (RotoItem::SelectionReasonEnum)reason != RotoItem::eSelectionReasonOverlayInteract ) {
        _imp->rotoData->selectedItems = _imp->context->getSelectedCurves();
        _imp->viewer->redraw();
    }
}

void
RotoGui::setSelection(const std::list<boost::shared_ptr<RotoDrawableItem> > & selectedItems,
                      const std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & selectedCps)
{
    _imp->rotoData->selectedItems.clear();
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        if (*it) {
            _imp->rotoData->selectedItems.push_back(*it);
        }
    }
    _imp->rotoData->selectedCps.clear();
    for (SelectedCPs::const_iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
        if (it->first && it->second) {
            _imp->rotoData->selectedCps.push_back(*it);
        }
    }
    _imp->context->select(_imp->rotoData->selectedItems, RotoItem::eSelectionReasonOverlayInteract);
    _imp->computeSelectedCpsBBOX();
}

void
RotoGui::setSelection(const boost::shared_ptr<Bezier> & curve,
                      const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > & point)
{
    _imp->rotoData->selectedItems.clear();
    if (curve) {
        _imp->rotoData->selectedItems.push_back(curve);
    }
    _imp->rotoData->selectedCps.clear();
    if (point.first && point.second) {
        _imp->rotoData->selectedCps.push_back(point);
    }
    if (curve) {
        _imp->context->select(curve, RotoItem::eSelectionReasonOverlayInteract);
    }
    _imp->computeSelectedCpsBBOX();
}

void
RotoGui::getSelection(std::list<boost::shared_ptr<RotoDrawableItem> >* selectedBeziers,
                      std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >* selectedCps)
{
    *selectedBeziers = _imp->rotoData->selectedItems;
    *selectedCps = _imp->rotoData->selectedCps;
}

void
RotoGui::refreshSelectionBBox()
{
    _imp->computeSelectedCpsBBOX();
}

void
RotoGui::setBuiltBezier(const boost::shared_ptr<Bezier> & curve)
{
    assert(curve);
    _imp->rotoData->builtBezier = curve;
}

boost::shared_ptr<Bezier> RotoGui::getBezierBeingBuild() const
{
    return _imp->rotoData->builtBezier;
}

void
RotoGui::pushUndoCommand(QUndoCommand* cmd)
{
    NodeSettingsPanel* panel = _imp->node->getSettingPanel();

    assert(panel);
    panel->pushUndoCommand(cmd);
}

QString
RotoGui::getNodeName() const
{
    return _imp->node->getNode()->getScriptName().c_str();
}

RotoContext*
RotoGui::getContext()
{
    return _imp->context.get();
}

void
RotoGui::onDisplayFeatherButtonClicked(bool toggled)
{
    _imp->displayFeatherEnabled->setDown(toggled);
    _imp->rotoData->displayFeather = toggled;
    _imp->viewer->redraw();
}

bool
RotoGui::isFeatherVisible() const
{
    return _imp->rotoData->displayFeather;
}

void
RotoGui::showMenuForCurve(const boost::shared_ptr<Bezier> & curve)
{
    QPoint pos = QCursor::pos();
    Natron::Menu menu(_imp->viewer);
    


    //menu.setFont( QFont(appFont,appFontSize) );

    ActionWithShortcut* selectAllAction = new ActionWithShortcut(kShortcutGroupRoto,
                                                                 kShortcutIDActionRotoSelectAll,
                                                                 kShortcutDescActionRotoSelectAll,&menu);
    menu.addAction(selectAllAction);

    ActionWithShortcut* deleteCurve = new ActionWithShortcut(kShortcutGroupRoto,
                                                             kShortcutIDActionRotoDelete,
                                                             kShortcutDescActionRotoDelete,&menu);
    menu.addAction(deleteCurve);
    
    ActionWithShortcut* openCloseCurve = 0;
    if (!curve->isOpenBezier()) {
        openCloseCurve = new ActionWithShortcut(kShortcutGroupRoto,
                                                                    kShortcutIDActionRotoCloseBezier,
                                                                    kShortcutDescActionRotoCloseBezier
                                                                    ,&menu);
        menu.addAction(openCloseCurve);
    }
    
    ActionWithShortcut* smoothAction = new ActionWithShortcut(kShortcutGroupRoto,
                                                              kShortcutIDActionRotoSmooth,
                                                              kShortcutDescActionRotoSmooth
                                                              ,&menu);
    menu.addAction(smoothAction);

    ActionWithShortcut* cuspAction = new ActionWithShortcut(kShortcutGroupRoto,
                                                            kShortcutIDActionRotoCuspBezier,
                                                            kShortcutDescActionRotoCuspBezier
                                                            ,&menu);
    menu.addAction(cuspAction);
    
    ActionWithShortcut* removeFeather = 0;
    if (!curve->isOpenBezier()) {
        removeFeather = new ActionWithShortcut(kShortcutGroupRoto,
                                                                   kShortcutIDActionRotoRemoveFeather,
                                                                   kShortcutDescActionRotoRemoveFeather
                                                                   ,&menu);
        menu.addAction(removeFeather);
    }
    
    ActionWithShortcut* lockShape = new ActionWithShortcut(kShortcutGroupRoto,
                                                               kShortcutIDActionRotoLockCurve,
                                                               kShortcutDescActionRotoLockCurve
                                                               ,&menu);
    menu.addAction(lockShape);


    ActionWithShortcut* linkTo = new ActionWithShortcut(kShortcutGroupRoto,
                                                        kShortcutIDActionRotoLinkToTrack,
                                                        kShortcutDescActionRotoLinkToTrack
                                                        ,&menu);
    menu.addAction(linkTo);
    ActionWithShortcut* unLinkFrom = new ActionWithShortcut(kShortcutGroupRoto,
                                                            kShortcutIDActionRotoUnlinkToTrack,
                                                            kShortcutDescActionRotoUnlinkToTrack
                                                 ,&menu);
    menu.addAction(unLinkFrom);


    QAction* ret = menu.exec(pos);


    if (ret == selectAllAction) {
        const std::list<boost::shared_ptr<BezierCP> > & cps = curve->getControlPoints();
        const std::list<boost::shared_ptr<BezierCP> > & fps = curve->getFeatherPoints();
        assert( cps.size() == fps.size() );

        std::list<boost::shared_ptr<BezierCP> >::const_iterator cpIT = cps.begin();
        for (std::list<boost::shared_ptr<BezierCP> >::const_iterator fpIT = fps.begin(); fpIT != fps.end(); ++fpIT, ++cpIT) {
            _imp->rotoData->selectedCps.push_back( std::make_pair(*cpIT, *fpIT) );
        }
        _imp->computeSelectedCpsBBOX();
        _imp->viewer->redraw();
    } else if (ret == deleteCurve) {
        std::list<boost::shared_ptr<RotoDrawableItem> > beziers;
        beziers.push_back(curve);
        pushUndoCommand( new RemoveCurveUndoCommand(this,beziers) );
        _imp->viewer->redraw();
    } else if (openCloseCurve && ret == openCloseCurve) {
        pushUndoCommand( new OpenCloseUndoCommand(this,curve) );
        _imp->viewer->redraw();
    } else if (ret == smoothAction) {
        smoothSelectedCurve();
    } else if (ret == cuspAction) {
        cuspSelectedCurve();
    } else if (removeFeather && ret == removeFeather) {
        removeFeatherForSelectedCurve();
    } else if (ret == linkTo) {
        SelectedCPs points;
        const std::list<boost::shared_ptr<BezierCP> > & cps = curve->getControlPoints();
        const std::list<boost::shared_ptr<BezierCP> > & fps = curve->getFeatherPoints();
        assert( cps.size() == fps.size() );

        std::list<boost::shared_ptr<BezierCP> >::const_iterator cpIT = cps.begin();
        for (std::list<boost::shared_ptr<BezierCP> >::const_iterator fpIT = fps.begin(); fpIT != fps.end(); ++fpIT, ++cpIT) {
            if ( !(*cpIT)->isSlaved() && !(*fpIT)->isSlaved() ) {
                points.push_back( std::make_pair(*cpIT, *fpIT) );
            }
        }

        linkPointTo(points);
    } else if (ret == unLinkFrom) {
        SelectedCPs points;
        const std::list<boost::shared_ptr<BezierCP> > & cps = curve->getControlPoints();
        const std::list<boost::shared_ptr<BezierCP> > & fps = curve->getFeatherPoints();
        assert( cps.size() == fps.size() );

        std::list<boost::shared_ptr<BezierCP> >::const_iterator cpIT = cps.begin();
        for (std::list<boost::shared_ptr<BezierCP> >::const_iterator fpIT = fps.begin(); fpIT != fps.end(); ++fpIT, ++cpIT) {
            if ( (*cpIT)->isSlaved() && (*fpIT)->isSlaved() ) {
                points.push_back( std::make_pair(*cpIT, *fpIT) );
            }
        }

        pushUndoCommand( new UnLinkFromTrackUndoCommand(this,points) );
    } else if (ret == lockShape) {
        lockSelectedCurves();
    }
} // showMenuForCurve

void
RotoGui::smoothSelectedCurve()
{
    
    std::pair<double,double> pixelScale;
    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);
    double time = _imp->context->getTimelineCurrentTime();
    std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
    
    if (!_imp->rotoData->selectedCps.empty()) {
        for (SelectedCPs::const_iterator it = _imp->rotoData->selectedCps.begin(); it != _imp->rotoData->selectedCps.end(); ++it) {
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = it->first->getBezier();
            data.newPoints.push_back(*it);
            datas.push_back(data);
        }
    } else {
        for (SelectedItems::const_iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
            
            boost::shared_ptr<Bezier> bezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (bezier) {
                SmoothCuspUndoCommand::SmoothCuspCurveData data;
                data.curve = bezier;
                const std::list<boost::shared_ptr<BezierCP> > & cps = bezier->getControlPoints();
                const std::list<boost::shared_ptr<BezierCP> > & fps = bezier->getFeatherPoints();
                std::list<boost::shared_ptr<BezierCP> >::const_iterator itFp = fps.begin();
                for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin(); it != cps.end(); ++it, ++itFp) {
                    data.newPoints.push_back( std::make_pair(*it, *itFp) );
                }
                datas.push_back(data);
            }
        }
    }
    if (!datas.empty()) {
        pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,false,pixelScale) );
        _imp->viewer->redraw();
    }
}

void
RotoGui::cuspSelectedCurve()
{
    std::pair<double,double> pixelScale;
    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);
    double time = _imp->context->getTimelineCurrentTime();
    std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
    
    if (!_imp->rotoData->selectedCps.empty()) {
        for (SelectedCPs::const_iterator it = _imp->rotoData->selectedCps.begin(); it != _imp->rotoData->selectedCps.end(); ++it) {
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = it->first->getBezier();
            data.newPoints.push_back(*it);
            datas.push_back(data);
        }
    } else {
        for (SelectedItems::const_iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
            boost::shared_ptr<Bezier> bezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (bezier) {
                SmoothCuspUndoCommand::SmoothCuspCurveData data;
                data.curve = bezier;
                const std::list<boost::shared_ptr<BezierCP> > & cps = bezier->getControlPoints();
                const std::list<boost::shared_ptr<BezierCP> > & fps = bezier->getFeatherPoints();
                std::list<boost::shared_ptr<BezierCP> >::const_iterator itFp = fps.begin();
                for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin(); it != cps.end(); ++it, ++itFp) {
                    data.newPoints.push_back( std::make_pair(*it, *itFp) );
                }
                datas.push_back(data);
            }
        }
    }
    if (!datas.empty()) {
        pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,true,pixelScale) );
    }
    _imp->viewer->redraw();
}

void
RotoGui::removeFeatherForSelectedCurve()
{
    std::list<RemoveFeatherUndoCommand::RemoveFeatherData> datas;

    for (SelectedItems::const_iterator it = _imp->rotoData->selectedItems.begin(); it != _imp->rotoData->selectedItems.end(); ++it) {
        boost::shared_ptr<Bezier> bezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (bezier) {
            RemoveFeatherUndoCommand::RemoveFeatherData data;
            data.curve = bezier;
            data.newPoints = bezier->getFeatherPoints();
            datas.push_back(data);
        }
    }
        if (!datas.empty()) {
            pushUndoCommand( new RemoveFeatherUndoCommand(this,datas) );
            _imp->viewer->redraw();
        }
    }

void
RotoGui::lockSelectedCurves()
{
    ///Make a copy because setLocked will change the selection internally and invalidate the iterator
    SelectedItems selection = _imp->rotoData->selectedItems;
    
    for (SelectedItems::const_iterator it = selection.begin(); it != selection.end(); ++it) {

        (*it)->setLocked(true, false,RotoItem::eSelectionReasonOverlayInteract);
    }
    _imp->clearSelection();
    _imp->viewer->redraw();
}

void
RotoGui::showMenuForControlPoint(const boost::shared_ptr<Bezier> & curve,
                                 const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > & cp)
{
    
    std::pair<double,double> pixelScale;
    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);
    
    QPoint pos = QCursor::pos();
    Natron::Menu menu(_imp->viewer);

    //menu.setFont( QFont(appFont,appFontSize) );

    ActionWithShortcut* deleteCp =new ActionWithShortcut(kShortcutGroupRoto,
                                                         kShortcutIDActionRotoDelete,
                                                         kShortcutDescActionRotoDelete,&menu);
    menu.addAction(deleteCp);

    ActionWithShortcut* smoothAction = new ActionWithShortcut(kShortcutGroupRoto,
                                                              kShortcutIDActionRotoSmooth,
                                                              kShortcutDescActionRotoSmooth
                                                              ,&menu);

    menu.addAction(smoothAction);

    ActionWithShortcut* cuspAction = new ActionWithShortcut(kShortcutGroupRoto,
                                                            kShortcutIDActionRotoCuspBezier,
                                                            kShortcutDescActionRotoCuspBezier
                                                            ,&menu);
    menu.addAction(cuspAction);

    ActionWithShortcut* removeFeather = new ActionWithShortcut(kShortcutGroupRoto,
                                                               kShortcutIDActionRotoRemoveFeather,
                                                               kShortcutDescActionRotoRemoveFeather
                                                               ,&menu);

    menu.addAction(removeFeather);

    menu.addSeparator();

    boost::shared_ptr<KnobDouble> isSlaved = cp.first->isSlaved();
    ActionWithShortcut* linkTo = 0,*unLinkFrom = 0;
    if (!isSlaved) {
        linkTo = new ActionWithShortcut(kShortcutGroupRoto,
                                        kShortcutIDActionRotoLinkToTrack,
                                        kShortcutDescActionRotoLinkToTrack
                                        ,&menu);

        menu.addAction(linkTo);
    } else {
        unLinkFrom = new ActionWithShortcut(kShortcutGroupRoto,
                                            kShortcutIDActionRotoUnlinkToTrack,
                                            kShortcutDescActionRotoUnlinkToTrack
                                            ,&menu);
        menu.addAction(unLinkFrom);
    }

    QAction* ret = menu.exec(pos);
    double time = _imp->context->getTimelineCurrentTime();
    if (ret == deleteCp) {
        pushUndoCommand( new RemovePointUndoCommand(this,curve,!cp.first->isFeatherPoint() ? cp.first : cp.second) );
        _imp->viewer->redraw();
    } else if (ret == smoothAction) {
        std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
        SmoothCuspUndoCommand::SmoothCuspCurveData data;
        data.curve = curve;
        data.newPoints.push_back(cp);
        datas.push_back(data);
        pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,false,pixelScale) );
        _imp->viewer->redraw();
    } else if (ret == cuspAction) {
        std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
        SmoothCuspUndoCommand::SmoothCuspCurveData data;
        data.curve = curve;
        data.newPoints.push_back(cp);
        datas.push_back(data);
        pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,true,pixelScale) );
        _imp->viewer->redraw();
    } else if (ret == removeFeather) {
        std::list<RemoveFeatherUndoCommand::RemoveFeatherData> datas;
        RemoveFeatherUndoCommand::RemoveFeatherData data;
        data.curve = curve;
        data.newPoints.push_back(cp.first->isFeatherPoint() ? cp.first : cp.second);
        datas.push_back(data);
        pushUndoCommand( new RemoveFeatherUndoCommand(this,datas) );
        _imp->viewer->redraw();
    } else if ( (ret == linkTo) && (ret != NULL) ) {
        SelectedCPs points;
        points.push_back(cp);
        linkPointTo(points);
    } else if ( (ret == unLinkFrom) && (ret != NULL) ) {
        SelectedCPs points;
        points.push_back(cp);
        pushUndoCommand( new UnLinkFromTrackUndoCommand(this,points) );
    }
} // showMenuForControlPoint

class LinkToTrackDialog
    : public QDialog
{
    ComboBox* _choice;

public:

    LinkToTrackDialog(const std::vector< std::pair<std::string,boost::shared_ptr<KnobDouble> > > & knobs,
                      QWidget* parent)
        : QDialog(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        _choice = new ComboBox(this);

        for (std::vector< std::pair<std::string,boost::shared_ptr<KnobDouble> > >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            _choice->addItem( it->first.c_str() );
        }

        mainLayout->addWidget(_choice);

        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal,this);
        connect( buttons, SIGNAL( accepted() ), this, SLOT( accept() ) );
        connect( buttons, SIGNAL( rejected() ), this, SLOT( reject() ) );
        mainLayout->addWidget(buttons);
        setWindowTitle( QObject::tr("Link to track") );
    }

    int getSelectedKnob() const
    {
        return _choice->activeIndex();
    }

    virtual ~LinkToTrackDialog()
    {
    }
};

void
RotoGui::linkPointTo(const std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & points)
{
    std::vector< std::pair<std::string,boost::shared_ptr<KnobDouble> > > knobs;
    NodeList activeNodes;
    _imp->node->getNode()->getGroup()->getActiveNodes(&activeNodes);
    for (NodeList::iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
        if ( (*it)->isPointTrackerNode() && (*it)->getParentMultiInstance()) {
            boost::shared_ptr<KnobI> k = (*it)->getKnobByName("center");
            boost::shared_ptr<KnobI> name = (*it)->getKnobByName(kNatronOfxParamStringSublabelName);
            if (k && name) {
                boost::shared_ptr<KnobDouble> dk = boost::dynamic_pointer_cast<KnobDouble>(k);
                KnobString* nameKnob = dynamic_cast<KnobString*>( name.get() );
                if (dk && nameKnob) {
                    std::string trackName = nameKnob->getValue();
                    trackName += "/";
                    trackName += dk->getLabel();
                    knobs.push_back( std::make_pair(trackName, dk) );
                }
            }
        }
    }
    if ( knobs.empty() ) {
        Natron::warningDialog( "", tr("No tracker found in the project.").toStdString() );

        return;
    }
    LinkToTrackDialog dialog(knobs,_imp->viewerTab);
    if ( dialog.exec() ) {
        int index = dialog.getSelectedKnob();
        if ( (index >= 0) && ( index < (int)knobs.size() ) ) {
            const boost::shared_ptr<KnobDouble> & knob = knobs[index].second;
            if ( knob && (knob->getDimension() == 2) ) {
                pushUndoCommand( new LinkToTrackUndoCommand(this,points,knob) );
            }
        }
    }
}

void
RotoGui::onColorWheelButtonClicked()
{
    QColorDialog dialog(_imp->viewerTab);
    QColor previousColor = _imp->colorPickerLabel->getCurrentColor();
    dialog.setCurrentColor(previousColor);
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( onDialogCurrentColorChanged(QColor) ) );
    if (!dialog.exec()) {
        _imp->colorPickerLabel->setColor(previousColor);
    } else {
        _imp->colorPickerLabel->setColor(dialog.currentColor());
        onBreakMultiStrokeTriggered();
    }
}

void
RotoGui::onDialogCurrentColorChanged(const QColor& color)
{
    assert(_imp->colorPickerLabel);
    _imp->colorPickerLabel->setColor(color);
}

void
RotoGui::onPressureOpacityClicked(bool isDown)
{
    _imp->pressureOpacityButton->setDown(isDown);
    _imp->pressureOpacityButton->setChecked(isDown);
    onBreakMultiStrokeTriggered();
}

void
RotoGui::onPressureSizeClicked(bool isDown)
{
    _imp->pressureSizeButton->setDown(isDown);
    _imp->pressureSizeButton->setChecked(isDown);
    onBreakMultiStrokeTriggered();
}

void
RotoGui::onPressureHardnessClicked(bool isDown)
{
    _imp->pressureHardnessButton->setDown(isDown);
    _imp->pressureHardnessButton->setChecked(isDown);
    onBreakMultiStrokeTriggered();
}

void
RotoGui::onBuildupClicked(bool isDown)
{
    _imp->buildUpButton->setDown(isDown);
    onBreakMultiStrokeTriggered();
}


void
RotoGui::notifyGuiClosing()
{
    _imp->viewerTab = 0;
    _imp->viewer = 0;
    _imp->rotoData->strokeBeingPaint.reset();
}

void
RotoGui::onResetCloneTransformClicked()
{
    _imp->rotoData->cloneOffset.first = _imp->rotoData->cloneOffset.second = 0;
    onBreakMultiStrokeTriggered();
}
