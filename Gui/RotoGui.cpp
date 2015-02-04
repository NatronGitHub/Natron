//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "RotoGui.h"

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
#include <QMenu>
#include <QDialogButtonBox>
#include <QTimer>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/RotoContext.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobTypes.h"

#include <ofxNatron.h>

#include "Gui/FromQtEnums.h"
#include "Gui/NodeGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/Button.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/RotoUndoCommand.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ComboBox.h"
#include "Gui/Gui.h"
#include "Gui/NodeGraph.h"
#include "Gui/GuiMacros.h"
#include "Gui/ActionShortcuts.h"

#include "Global/GLIncludes.h"

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
typedef std::list< boost::shared_ptr<Bezier> > SelectedBeziers;

enum EventStateEnum
{
    eEventStateNone = 0,
    eEventStateDraggingControlPoint,
    eEventStateDraggingSelectedControlPoints,
    eEventStateBuildingBezierControlPointTangent,
    eEventStateBuildingEllipse,
    eEventStateBuildingEllipseCenter,
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
    eEventStateDraggingBBoxMidLeft
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
struct RotoGuiSharedData
{
    SelectedBeziers selectedBeziers;
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

    RotoGuiSharedData()
        : selectedBeziers()
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
    {
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
    RotoToolButton* selectTool;
    RotoToolButton* pointsEditionTool;
    RotoToolButton* bezierEditionTool;
    QAction* selectAllAction;
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
          , selectTool(0)
          , pointsEditionTool(0)
          , bezierEditionTool(0)
          , selectAllAction(0)
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

    bool removeBezierFromSelection(const boost::shared_ptr<Bezier>& b);

    void computeSelectedCpsBBOX();

    QPointF getSelectedCpsBBOXCenter();

    void drawSelectedCpsBBOX();

    ///by default draws a vertical arrow, which can be rotated by rotate amount.
    void drawArrow(double centerX,double centerY,double rotate,bool hovered,const std::pair<double,double> & pixelScale);

    ///same as drawArrow but the two ends will make an angle of 90 degrees
    void drawBendedArrow(double centerX,double centerY,double rotate,bool hovered,const std::pair<double,double> & pixelScale);

    void handleBezierSelection(const boost::shared_ptr<Bezier> & curve, QMouseEvent* e);

    void handleControlPointSelection(const std::pair<boost::shared_ptr<BezierCP>, boost::shared_ptr<BezierCP> > & p, QMouseEvent* e);

    void drawSelectedCp(int time,const boost::shared_ptr<BezierCP> & cp,double x,double y);

    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >
    isNearbyFeatherBar(int time,const std::pair<double,double> & pixelScale,const QPointF & pos) const;

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
        return bboxClickAnywhere->isDown();
    }
};


RotoToolButton::RotoToolButton(QWidget* parent)
: QToolButton(parent)
, isSelected(false)
{
    setFocusPolicy(Qt::ClickFocus);
}


RotoToolButton::~RotoToolButton()
{
    
}

void
RotoToolButton::mousePressEvent(QMouseEvent* /*e*/)
{
}


void
RotoToolButton::mouseReleaseEvent(QMouseEvent* e)
{
    if ( triggerButtonisLeft(e) ) {
        handleSelection();
    } else if ( triggerButtonisRight(e) ) {
        showMenu();
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
        emit triggered(curAction);
    } else {
        QList<QAction*> allAction = actions();
        for (int i = 0; i < allAction.size(); ++i) {
            if (allAction[i] == curAction) {
                int next = ( i == (allAction.size() - 1) ) ? 0 : i + 1;
                setDefaultAction(allAction[next]);
                emit triggered(allAction[next]);
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

    action->setToolTip(text + ": " + tooltip + "<p><b>" + tr("Keyboard shortcut: ") + shortcut.toString(QKeySequence::NativeText) + "</b></p>");

    QPoint data;
    data.setX( (int)tool );
    if (toolGroup == _imp->selectTool) {
        data.setY( (int)eRotoRoleSelection );
    } else if (toolGroup == _imp->pointsEditionTool) {
        data.setY( (int)eRotoRolePointsEdition );
    } else if (toolGroup == _imp->bezierEditionTool) {
        data.setY(eRotoRoleBezierEdition);
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

    QObject::connect( parent->getViewer(),SIGNAL( selectionRectangleChanged(bool) ),this,SLOT( updateSelectionFromSelectionRectangle(bool) ) );
    QObject::connect( parent->getViewer(), SIGNAL( selectionCleared() ), this, SLOT( onSelectionCleared() ) );
    QPixmap pixBezier,pixEllipse,pixRectangle,pixAddPts,pixRemovePts,pixCuspPts,pixSmoothPts,pixOpenCloseCurve,pixRemoveFeather;
    QPixmap pixSelectAll,pixSelectPoints,pixSelectFeather,pixSelectCurves,pixAutoKeyingEnabled,pixAutoKeyingDisabled;
    QPixmap pixStickySelEnabled,pixStickySelDisabled,pixFeatherLinkEnabled,pixFeatherLinkDisabled,pixAddKey,pixRemoveKey;
    QPixmap pixRippleEnabled,pixRippleDisabled;
    QPixmap pixFeatherEnabled,pixFeatherDisabled;
    QPixmap pixBboxClickEnabled,pixBboxClickDisabled;

    appPTR->getIcon(Natron::NATRON_PIXMAP_BEZIER_32, &pixBezier);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ELLIPSE,&pixEllipse);
    appPTR->getIcon(Natron::NATRON_PIXMAP_RECTANGLE,&pixRectangle);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_POINTS,&pixAddPts);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_POINTS,&pixRemovePts);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CUSP_POINTS,&pixCuspPts);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SMOOTH_POINTS,&pixSmoothPts);
    appPTR->getIcon(Natron::NATRON_PIXMAP_OPEN_CLOSE_CURVE,&pixOpenCloseCurve);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_FEATHER,&pixRemoveFeather);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SELECT_ALL,&pixSelectAll);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SELECT_POINTS,&pixSelectPoints);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SELECT_FEATHER,&pixSelectFeather);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SELECT_CURVES,&pixSelectCurves);
    appPTR->getIcon(Natron::NATRON_PIXMAP_AUTO_KEYING_ENABLED,&pixAutoKeyingEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_AUTO_KEYING_DISABLED,&pixAutoKeyingDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_STICKY_SELECTION_ENABLED,&pixStickySelEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_STICKY_SELECTION_DISABLED,&pixStickySelDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FEATHER_LINK_ENABLED,&pixFeatherLinkEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FEATHER_LINK_DISABLED,&pixFeatherLinkDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_KEYFRAME,&pixAddKey);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_KEYFRAME,&pixRemoveKey);
    appPTR->getIcon(Natron::NATRON_PIXMAP_RIPPLE_EDIT_ENABLED,&pixRippleEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_RIPPLE_EDIT_DISABLED,&pixRippleDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FEATHER_VISIBLE, &pixFeatherEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FEATHER_UNVISIBLE, &pixFeatherDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_ROI_ENABLED, &pixBboxClickEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_ROI_DISABLED, &pixBboxClickDisabled);

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
    _imp->autoKeyingEnabled->setCheckable(true);
    _imp->autoKeyingEnabled->setChecked( _imp->context->isAutoKeyingEnabled() );
    _imp->autoKeyingEnabled->setDown( _imp->context->isAutoKeyingEnabled() );
    _imp->autoKeyingEnabled->setToolTip( tr("Auto-keying: When activated any movement to a control point will set a keyframe at the current time.") );
    QObject::connect( _imp->autoKeyingEnabled, SIGNAL( clicked(bool) ), this, SLOT( onAutoKeyingButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->autoKeyingEnabled);

    QIcon featherLinkIc;
    featherLinkIc.addPixmap(pixFeatherLinkEnabled,QIcon::Normal,QIcon::On);
    featherLinkIc.addPixmap(pixFeatherLinkDisabled,QIcon::Normal,QIcon::Off);
    _imp->featherLinkEnabled = new Button(featherLinkIc,"",_imp->selectionButtonsBar);
    _imp->featherLinkEnabled->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->featherLinkEnabled->setCheckable(true);
    _imp->featherLinkEnabled->setChecked( _imp->context->isFeatherLinkEnabled() );
    _imp->featherLinkEnabled->setDown( _imp->context->isFeatherLinkEnabled() );
    _imp->featherLinkEnabled->setToolTip( tr("Feather-link: When activated the feather points will follow the same"
                                             " movement as their counter-part does.") );
    QObject::connect( _imp->featherLinkEnabled, SIGNAL( clicked(bool) ), this, SLOT( onFeatherLinkButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->featherLinkEnabled);

    QIcon enableFeatherIC;
    enableFeatherIC.addPixmap(pixFeatherEnabled,QIcon::Normal,QIcon::On);
    enableFeatherIC.addPixmap(pixFeatherDisabled,QIcon::Normal,QIcon::Off);
    _imp->displayFeatherEnabled = new Button(enableFeatherIC,"",_imp->selectionButtonsBar);
    _imp->displayFeatherEnabled->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->displayFeatherEnabled->setCheckable(true);
    _imp->displayFeatherEnabled->setChecked(true);
    _imp->displayFeatherEnabled->setDown(true);
    _imp->displayFeatherEnabled->setToolTip( tr("When checked, the feather curve applied to the shape(s) will be visible and editable.") );
    QObject::connect( _imp->displayFeatherEnabled, SIGNAL( clicked(bool) ), this, SLOT( onDisplayFeatherButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->displayFeatherEnabled);

    QIcon stickSelIc;
    stickSelIc.addPixmap(pixStickySelEnabled,QIcon::Normal,QIcon::On);
    stickSelIc.addPixmap(pixStickySelDisabled,QIcon::Normal,QIcon::Off);
    _imp->stickySelectionEnabled = new Button(stickSelIc,"",_imp->selectionButtonsBar);
    _imp->stickySelectionEnabled->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->stickySelectionEnabled->setCheckable(true);
    _imp->stickySelectionEnabled->setChecked(false);
    _imp->stickySelectionEnabled->setDown(false);
    _imp->stickySelectionEnabled->setToolTip( tr("Sticky-selection: When activated, "
                                                 " clicking outside of any shape will not clear the current selection.") );
    QObject::connect( _imp->stickySelectionEnabled, SIGNAL( clicked(bool) ), this, SLOT( onStickySelectionButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->stickySelectionEnabled);
    
    QIcon bboxClickIc;
    bboxClickIc.addPixmap(pixBboxClickEnabled,QIcon::Normal,QIcon::On);
    bboxClickIc.addPixmap(pixBboxClickDisabled,QIcon::Normal,QIcon::Off);
    _imp->bboxClickAnywhere = new Button(bboxClickIc,"",_imp->selectionButtonsBar);
    _imp->bboxClickAnywhere->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->bboxClickAnywhere->setCheckable(true);
    _imp->bboxClickAnywhere->setChecked(true);
    _imp->bboxClickAnywhere->setDown(true);
    _imp->bboxClickAnywhere->setToolTip( tr("Easy bounding box manipulation: When activated, "
                                                 " clicking inside of the bounding box of selected points will move the points."
                                            "When deactivated, only clicking on the cross will move the points.") );
    QObject::connect( _imp->bboxClickAnywhere, SIGNAL( clicked(bool) ), this, SLOT( onBboxClickButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->bboxClickAnywhere);
    

    QIcon rippleEditIc;
    rippleEditIc.addPixmap(pixRippleEnabled,QIcon::Normal,QIcon::On);
    rippleEditIc.addPixmap(pixRippleDisabled,QIcon::Normal,QIcon::Off);
    _imp->rippleEditEnabled = new Button(rippleEditIc,"",_imp->selectionButtonsBar);
    _imp->rippleEditEnabled->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->rippleEditEnabled->setCheckable(true);
    _imp->rippleEditEnabled->setChecked( _imp->context->isRippleEditEnabled() );
    _imp->rippleEditEnabled->setDown( _imp->context->isRippleEditEnabled() );
    _imp->rippleEditEnabled->setToolTip( tr("Ripple-edit: When activated, moving a control point"
                                            " will move it by the same amount for all the keyframes "
                                            "it has.") );
    QObject::connect( _imp->rippleEditEnabled, SIGNAL( clicked(bool) ), this, SLOT( onRippleEditButtonClicked(bool) ) );
    _imp->selectionButtonsBarLayout->addWidget(_imp->rippleEditEnabled);

    _imp->addKeyframeButton = new Button(QIcon(pixAddKey),"",_imp->selectionButtonsBar);
    _imp->addKeyframeButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QObject::connect( _imp->addKeyframeButton, SIGNAL( clicked(bool) ), this, SLOT( onAddKeyFrameClicked() ) );
    _imp->addKeyframeButton->setToolTip( tr("Set a keyframe at the current time for the selected shape(s), if any.") );
    _imp->selectionButtonsBarLayout->addWidget(_imp->addKeyframeButton);

    _imp->removeKeyframeButton = new Button(QIcon(pixRemoveKey),"",_imp->selectionButtonsBar);
    _imp->removeKeyframeButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QObject::connect( _imp->removeKeyframeButton, SIGNAL( clicked(bool) ), this, SLOT( onRemoveKeyFrameClicked() ) );
    _imp->removeKeyframeButton->setToolTip("Remove a keyframe at the current time for the selected shape(s), if any.");
    _imp->selectionButtonsBarLayout->addWidget(_imp->removeKeyframeButton);
    _imp->selectionButtonsBarLayout->addStretch();

    QSize rotoToolSize(NATRON_LARGE_BUTTON_SIZE,NATRON_LARGE_BUTTON_SIZE);

    _imp->selectTool = new RotoToolButton(_imp->toolbar);
    _imp->selectTool->setFixedSize(rotoToolSize);
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
    _imp->toolbar->addWidget(_imp->selectTool);

    _imp->pointsEditionTool = new RotoToolButton(_imp->toolbar);
    _imp->pointsEditionTool->setFixedSize(rotoToolSize);
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
    _imp->toolbar->addWidget(_imp->pointsEditionTool);

    _imp->bezierEditionTool = new RotoToolButton(_imp->toolbar);
    _imp->bezierEditionTool->setFixedSize(rotoToolSize);
    _imp->bezierEditionTool->setPopupMode(QToolButton::InstantPopup);
    QObject::connect( _imp->bezierEditionTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
    _imp->bezierEditionTool->setText("Bezier");
    _imp->bezierEditionTool->setDown(!hasShapes);
    
    QKeySequence editBezierShortcut(Qt::Key_V);
    QAction* drawBezierAct = createToolAction(_imp->bezierEditionTool, QIcon(pixBezier), tr("Bezier"),
                                              tr("Edit bezier paths. Click and drag the mouse to adjust tangents. Press enter to close the shape. ")
                                              ,editBezierShortcut, eRotoToolDrawBezier);

    ////B-splines are not implemented yet
    //createToolAction(_imp->bezierEditionTool, QIcon(), "B-Spline", eRotoToolDrawBSpline);

    createToolAction(_imp->bezierEditionTool, QIcon(pixEllipse), tr("Ellipse"),tr("Hold control to draw the ellipse from its center"),editBezierShortcut, eRotoToolDrawEllipse);
    createToolAction(_imp->bezierEditionTool, QIcon(pixRectangle), tr("Rectangle"),"", editBezierShortcut,eRotoToolDrawRectangle);
    _imp->bezierEditionTool->setDefaultAction(drawBezierAct);
    _imp->toolbar->addWidget(_imp->bezierEditionTool);
    
    ////////////Default action is to make a new bezier
    _imp->selectedRole = _imp->selectTool;
    onToolActionTriggered(hasShapes ? _imp->selectAllAction : drawBezierAct);

    QObject::connect( _imp->node->getNode()->getApp()->getTimeLine().get(), SIGNAL( frameChanged(SequenceTime,int) ),
                      this, SLOT( onCurrentFrameChanged(SequenceTime,int) ) );
    QObject::connect( _imp->context.get(), SIGNAL( refreshViewerOverlays() ), this, SLOT( onRefreshAsked() ) );
    QObject::connect( _imp->context.get(), SIGNAL( selectionChanged(int) ), this, SLOT( onSelectionChanged(int) ) );
    QObject::connect( _imp->context.get(), SIGNAL( itemLockedChanged() ), this, SLOT( onCurveLockedChanged() ) );
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
        break;
    case eRotoRolePointsEdition:

        return _imp->selectionButtonsBar;
        break;
    case eRotoRoleBezierEdition:

        return _imp->selectionButtonsBar;
        break;
    default:
        assert(false);
        break;
    }
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
    actions.append( _imp->pointsEditionTool->actions() );
    actions.append( _imp->bezierEditionTool->actions() );
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
RotoGui::onToolActionTriggeredInternal(QAction* action,
                                       bool emitSignal)
{
    QPoint data = action->data().toPoint();
    RotoRoleEnum actionRole = (RotoRoleEnum)data.y();
    QToolButton* toolButton = 0;
    RotoRoleEnum previousRole = getCurrentRole();
    
    switch (actionRole) {
        case eRotoRoleSelection:
            toolButton = _imp->selectTool;
            _imp->selectTool->setIsSelected(true);
            _imp->pointsEditionTool->setIsSelected(false);
            _imp->bezierEditionTool->setIsSelected(false);
            emit roleChanged( (int)previousRole,(int)eRotoRoleSelection );
            break;
        case eRotoRolePointsEdition:
            toolButton = _imp->pointsEditionTool;
            _imp->selectTool->setIsSelected(false);
            _imp->pointsEditionTool->setIsSelected(true);
            _imp->bezierEditionTool->setIsSelected(false);
            emit roleChanged( (int)previousRole,(int)eRotoRolePointsEdition );
            break;
        case eRotoRoleBezierEdition:
            toolButton = _imp->bezierEditionTool;
            _imp->selectTool->setIsSelected(false);
            _imp->pointsEditionTool->setIsSelected(false);
            _imp->bezierEditionTool->setIsSelected(true);
            emit roleChanged( (int)previousRole,(int)eRotoRoleBezierEdition );
            break;
        default:
            assert(false);
            break;
    }


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
    if ( (previousRole == eRotoRoleBezierEdition) && (_imp->selectedTool == eRotoToolDrawBezier) && _imp->rotoData->builtBezier &&
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
        emit selectedToolChanged( (int)_imp->selectedTool );
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
    }
    assert(false);
}

void
RotoGui::RotoGuiPrivate::drawSelectedCp(int time,
                                        const boost::shared_ptr<BezierCP> & cp,
                                        double x,
                                        double y)
{
    ///if the tangent is being dragged, color it
    bool colorLeftTangent = false;
    bool colorRightTangent = false;

    if ( (cp == rotoData->tangentBeingDragged) &&
         ( ( state == eEventStateDraggingLeftTangent) || ( state == eEventStateDraggingRightTangent) ) ) {
        colorLeftTangent = state == eEventStateDraggingLeftTangent ? true : false;
        colorRightTangent = !colorLeftTangent;
    }


    double leftDerivX,leftDerivY,rightDerivX,rightDerivY;
    cp->getLeftBezierPointAtTime(time, &leftDerivX, &leftDerivY);
    cp->getRightBezierPointAtTime(time, &rightDerivX, &rightDerivY);

    bool drawLeftHandle = leftDerivX != x || leftDerivY != y;
    bool drawRightHandle = rightDerivX != x || rightDerivY != y;
    glBegin(GL_POINTS);
    if (drawLeftHandle) {
        if (colorLeftTangent) {
            glColor3f(0.2, 1., 0.);
        }
        glVertex2d(leftDerivX,leftDerivY);
        if (colorLeftTangent) {
            glColor3d(0.85, 0.67, 0.);
        }
    }
    if (drawRightHandle) {
        if (colorRightTangent) {
            glColor3f(0.2, 1., 0.);
        }
        glVertex2d(rightDerivX,rightDerivY);
        if (colorRightTangent) {
            glColor3d(0.85, 0.67, 0.);
        }
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    if (drawLeftHandle) {
        glVertex2d(leftDerivX,leftDerivY);
    }
    glVertex2d(x, y);
    if (drawRightHandle) {
        glVertex2d(rightDerivX,rightDerivY);
    }
    glEnd();
} // drawSelectedCp

void
RotoGui::drawOverlays(double /*scaleX*/,
                      double /*scaleY*/) const
{
    std::list< boost::shared_ptr<Bezier> > beziers = _imp->context->getCurvesByRenderOrder();
    int time = _imp->context->getTimelineCurrentTime();
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
        for (std::list< boost::shared_ptr<Bezier> >::const_iterator it = beziers.begin(); it != beziers.end(); ++it) {
            if ( !(*it)->isGloballyActivated() ) {
                continue;
            }
            ///draw the bezier
#pragma message WARN("Roto drawing: please update this algorithm")
            // Please update this algorithm:
            // It should first compute the bbox (this is cheap)
            // then check if the bbox is visible
            // if the bbox is visible, compute the polygon and draw it.
            std::list< Point > points;
            (*it)->evaluateAtTime_DeCasteljau(time,0, 100, &points, NULL);
            
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
            
            if ( isFeatherVisible() ) {
                ///Draw feather only if visible (button is toggled in the user interface)
#pragma message WARN("Roto drawing: please update this algorithm")
                // Plese update this algorithm:
                // It should first compute the bbox (this is cheap)
                // then check if the bbox is visible
                // if the bbox is visible, compute the polygon and draw it.
                (*it)->evaluateFeatherPointsAtTime_DeCasteljau(time,0, 100, true, &featherPoints, &featherBBox);
                
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
            std::list< boost::shared_ptr<Bezier> >::const_iterator selected =
            std::find(_imp->rotoData->selectedBeziers.begin(),_imp->rotoData->selectedBeziers.end(),*it);
            
            if ( ( selected != _imp->rotoData->selectedBeziers.end() ) && !locked ) {
                const std::list< boost::shared_ptr<BezierCP> > & cps = (*selected)->getControlPoints();
                const std::list< boost::shared_ptr<BezierCP> > & featherPts = (*selected)->getFeatherPoints();
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
                --prevCp;
                std::list< boost::shared_ptr<BezierCP> >::const_iterator nextCp = cps.begin();
                ++nextCp;
                for (std::list< boost::shared_ptr<BezierCP> >::const_iterator it2 = cps.begin(); it2 != cps.end();
                     ++it2,++itF,++index,++nextCp,++prevCp) {
                    if ( nextCp == cps.end() ) {
                        nextCp = cps.begin();
                    }
                    if ( prevCp == cps.end() ) {
                        prevCp = cps.begin();
                    }
                    
                    double x,y;
                    (*it2)->getPositionAtTime(time, &x, &y);
                    
                    double xF,yF;
                    (*itF)->getPositionAtTime(time, &xF, &yF);
                    ///draw the feather point only if it is distinct from the associated point
                    bool drawFeather = isFeatherVisible();
                    if (drawFeather) {
                        drawFeather = !(*it2)->equalsAtTime(time, **itF);
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
                         cpIt != _imp->rotoData->selectedCps.end(); ++cpIt) {
                        ///if the control point is selected, draw its tangent handles
                        if (cpIt->first == *it2) {
                            _imp->drawSelectedCp(time, cpIt->first, x, y);
                            if (drawFeather) {
                                _imp->drawSelectedCp(time, cpIt->second, xF, yF);
                            }
                            glColor3f(0.2, 1., 0.);
                            colorChanged = true;
                            break;
                        } else if (cpIt->second == *it2) {
                            _imp->drawSelectedCp(time, cpIt->second, x, y);
                            if (drawFeather) {
                                _imp->drawSelectedCp(time, cpIt->first, xF, yF);
                            }
                            glColor3f(0.2, 1., 0.);
                            colorChanged = true;
                            break;
                        }
                    }
                    
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
                        if ( (_imp->selectedTool == eRotoToolSelectAll) || (_imp->selectedTool == eRotoToolSelectFeatherPoints) ) {
                            int cpCount = (*it2)->getBezier()->getControlPointsCount();
                            if (cpCount > 1) {
                                Natron::Point controlPoint;
                                controlPoint.x = x;
                                controlPoint.y = y;
                                Natron::Point featherPoint;
                                featherPoint.x = xF;
                                featherPoint.y = yF;
                                
                                Bezier::expandToFeatherDistance(controlPoint, &featherPoint, distFeatherX, featherPoints, featherBBox, time, prevCp, it2, nextCp);
                                
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
                    
                }
                
            }
            glCheckError();
        }
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

    if ( _imp->rotoData->showCpsBbox && _imp->node->isSettingsPanelVisible() ) {
        _imp->drawSelectedCpsBBOX();
    }
    glCheckError();
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
        _imp->rotoData->selectedBeziers.clear();
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
    std::list<boost::shared_ptr<Bezier> > curves = _imp->context->getCurvesByRenderOrder();
    for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        
        if ((*it)->isLockedRecursive()) {
            continue;
        }
//        SelectedBeziers::iterator isSelected = std::find(_imp->rotoData->selectedBeziers.begin(),
//                                                               _imp->rotoData->selectedBeziers.end(),
//                                                               *it);
        
     //   if (isSelected != _imp->rotoData->selectedBeziers.end() || mustAddToBezierSelection) {
            SelectedCPs points  = (*it)->controlPointsWithinRect(l, r, b, t, 0,selectionMode);
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
                _imp->rotoData->selectedBeziers.push_back(*it);
            }
     //   }
    }

    if (!_imp->rotoData->selectedBeziers.empty()) {
        _imp->context->select(_imp->rotoData->selectedBeziers, RotoContext::eSelectionReasonOverlayInteract);
    } else if (!stickySel && !_imp->shiftDown) {
        _imp->context->clearSelection(RotoContext::eSelectionReasonOverlayInteract);
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
    return !rotoData->selectedBeziers.empty() || !rotoData->selectedCps.empty();
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
    context->clearSelection(RotoContext::eSelectionReasonOverlayInteract);
    rotoData->selectedBeziers.clear();
}

bool
RotoGui::RotoGuiPrivate::removeBezierFromSelection(const boost::shared_ptr<Bezier>& b)
{
    for (SelectedBeziers::iterator fb = rotoData->selectedBeziers.begin(); fb != rotoData->selectedBeziers.end(); ++fb) {
        if (*fb == b) {
            context->deselect(*fb, RotoContext::eSelectionReasonOverlayInteract);
            rotoData->selectedBeziers.erase(fb);

            return true;
        }
    }

    return false;
}

static void
handleControlPointMaximum(int time,
                          const BezierCP & p,
                          double* l,
                          double *b,
                          double *r,
                          double *t)
{
    double x,y,xLeft,yLeft,xRight,yRight;

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

void
RotoGui::RotoGuiPrivate::computeSelectedCpsBBOX()
{
    if (!node->getNode()->isActivated()) {
        return;
    }
    int time = context->getTimelineCurrentTime();
    std::pair<double, double> pixelScale;

    viewer->getPixelScale(pixelScale.first,pixelScale.second);


    double l = INT_MAX,r = INT_MIN,b = INT_MAX,t = INT_MIN;
    for (SelectedCPs::iterator it = rotoData->selectedCps.begin(); it != rotoData->selectedCps.end(); ++it) {
        handleControlPointMaximum(time,*(it->first),&l,&b,&r,&t);
        handleControlPointMaximum(time,*(it->second),&l,&b,&r,&t);
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
    SelectedBeziers::const_iterator found =
        std::find(rotoData->selectedBeziers.begin(),rotoData->selectedBeziers.end(),curve);

    if ( found == rotoData->selectedBeziers.end() ) {
        ///clear previous selection if the SHIFT modifier isn't held
        if ( !modCASIsShift(e) ) {
            clearBeziersSelection();
        }
        rotoData->selectedBeziers.push_back(curve);
        context->select(curve, RotoContext::eSelectionReasonOverlayInteract);
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
RotoGui::penDown(double /*scaleX*/,
                 double /*scaleY*/,
                 const QPointF & /*viewportPos*/,
                 const QPointF & pos,
                 QMouseEvent* e)
{
    std::pair<double, double> pixelScale;

    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);

    bool didSomething = false;
    int time = _imp->context->getTimelineCurrentTime();
    double tangentSelectionTol = kTangentHandleSelectionTolerance * pixelScale.first;
    double cpSelectionTolerance = kControlPointSelectionTolerance * pixelScale.first;


    ////////////////// TANGENT SELECTION
    ///in all cases except cusp/smooth if a control point is selected, check if the user clicked on a tangent handle
    ///in which case we go into eEventStateDraggingTangent mode
    if ( (_imp->selectedTool != eRotoToolCuspPoints) && (_imp->selectedTool != eRotoToolSmoothPoints) && (_imp->selectedTool != eRotoToolSelectCurves) ) {
        for (SelectedCPs::iterator it = _imp->rotoData->selectedCps.begin(); it != _imp->rotoData->selectedCps.end(); ++it) {
            if ( (_imp->selectedTool == eRotoToolSelectAll) ||
                 ( _imp->selectedTool == eRotoToolDrawBezier) ) {
                int ret = it->first->isNearbyTangent(time, pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->rotoData->tangentBeingDragged = it->first;
                    _imp->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                } else {
                    ///try with the counter part point
                    ret = it->second->isNearbyTangent(time, pos.x(), pos.y(), tangentSelectionTol);
                    if (ret >= 0) {
                        _imp->rotoData->tangentBeingDragged = it->second;
                        _imp->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                        didSomething = true;
                    }
                }
            } else if (_imp->selectedTool == eRotoToolSelectFeatherPoints) {
                const boost::shared_ptr<BezierCP> & fp = it->first->isFeatherPoint() ? it->first : it->second;
                int ret = fp->isNearbyTangent(time, pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->rotoData->tangentBeingDragged = fp;
                    _imp->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                }
            } else if (_imp->selectedTool == eRotoToolSelectPoints) {
                const boost::shared_ptr<BezierCP> & cp = it->first->isFeatherPoint() ? it->second : it->first;
                int ret = cp->isNearbyTangent(time, pos.x(), pos.y(), tangentSelectionTol);
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
                SelectedBeziers::const_iterator found =
                    std::find(_imp->rotoData->selectedBeziers.begin(),_imp->rotoData->selectedBeziers.end(),nearbyBezier);
                if ( found == _imp->rotoData->selectedBeziers.end() ) {
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
            SelectedBeziers::const_iterator found =
                std::find(_imp->rotoData->selectedBeziers.begin(),_imp->rotoData->selectedBeziers.end(),nearbyBezier);
            if ( found == _imp->rotoData->selectedBeziers.end() ) {
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
            SelectedBeziers::const_iterator foundBezier =
                std::find(_imp->rotoData->selectedBeziers.begin(), _imp->rotoData->selectedBeziers.end(), nearbyBezier);
            if ( foundBezier != _imp->rotoData->selectedBeziers.end() ) {
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
    case eRotoToolDrawBezier: {
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
            for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin(); it != cps.end(); ++it,++i) {
                double x,y;
                (*it)->getPositionAtTime(time, &x, &y);
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
        MakeBezierUndoCommand* cmd = new MakeBezierUndoCommand(this,_imp->rotoData->builtBezier,true,pos.x(),pos.y(),time);
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
        bool fromCenter = modCASIsControl(e);
        pushUndoCommand( new MakeEllipseUndoCommand(this,true,fromCenter,pos.x(),pos.y(),time) );
        if (fromCenter) {
            _imp->state = eEventStateBuildingEllipseCenter;
        } else {
            _imp->state = eEventStateBuildingEllipse;
        }
        didSomething = true;
        break;
    }
    case eRotoToolDrawRectangle: {
        pushUndoCommand( new MakeRectangleUndoCommand(this,true,pos.x(),pos.y(),time) );
        _imp->evaluateOnPenUp = true;
        _imp->state = eEventStateBuildingRectangle;
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
RotoGui::penDoubleClicked(double /*scaleX*/,
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
            for (; itCp != cps.end(); ++itCp,++itFp) {
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
RotoGui::penMotion(double /*scaleX*/,
                   double /*scaleY*/,
                   const QPointF & /*viewportPos*/,
                   const QPointF & pos,
                   QMouseEvent* e)
{
    std::pair<double, double> pixelScale;

    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);

    bool didSomething = false;
    HoverStateEnum lastHoverState = _imp->hoverState;
    int time = _imp->context->getTimelineCurrentTime();
    ///Set the cursor to the appropriate case
    bool cursorSet = false;
    
    double cpTol = kControlPointSelectionTolerance * pixelScale.first;
    
    
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
    if (_imp->hoverState == eHoverStateNothing) {
        if ( (_imp->state != eEventStateDraggingControlPoint) && (_imp->state != eEventStateDraggingSelectedControlPoints) ) {
            for (SelectedBeziers::const_iterator it = _imp->rotoData->selectedBeziers.begin(); it != _imp->rotoData->selectedBeziers.end(); ++it) {
                int index = -1;
                std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > nb =
                (*it)->isNearbyControlPoint(pos.x(), pos.y(), cpTol,Bezier::eControlPointSelectionPrefWhateverFirst,&index);
                if ( (index != -1) && ( ( !nb.first->isFeatherPoint() && !isFeatherVisible() ) || isFeatherVisible() ) ) {
                    _imp->viewer->setCursor( QCursor(Qt::CrossCursor) );
                    cursorSet = true;
                    break;
                }
            }
        }
        if ( !cursorSet && (_imp->state != eEventStateDraggingLeftTangent) && (_imp->state != eEventStateDraggingRightTangent) ) {
            ///find a nearby tangent
            for (SelectedCPs::const_iterator it = _imp->rotoData->selectedCps.begin(); it != _imp->rotoData->selectedCps.end(); ++it) {
                if (it->first->isNearbyTangent(time, pos.x(), pos.y(), cpTol) != -1) {
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
            assert(c.size() == f.size());
            std::list<boost::shared_ptr<BezierCP> >::const_iterator itFp = f.begin();
            for (std::list<boost::shared_ptr<BezierCP> >::const_iterator itCp = c.begin(); itCp != c.end(); ++itCp,++itFp) {
                cps.push_back(std::make_pair(*itCp,*itFp));
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
        assert(_imp->rotoData->cpBeingDragged.first && _imp->rotoData->cpBeingDragged.second);
        std::list<SelectedCP> toDrag;
        toDrag.push_back(_imp->rotoData->cpBeingDragged);
        pushUndoCommand( new MoveControlPointsUndoCommand(this,toDrag,dx,dy,time) );
        _imp->evaluateOnPenUp = true;
        _imp->computeSelectedCpsBBOX();
        didSomething = true;
    };  break;
    case eEventStateBuildingBezierControlPointTangent: {
        assert(_imp->rotoData->builtBezier);
        pushUndoCommand( new MakeBezierUndoCommand(this,_imp->rotoData->builtBezier,false,dx,dy,time) );
        break;
    }
    case eEventStateBuildingEllipse: {
        pushUndoCommand( new MakeEllipseUndoCommand(this,false,false,dx,dy,time) );

        didSomething = true;
        _imp->evaluateOnPenUp = true;
        break;
    }
    case eEventStateBuildingEllipseCenter: {
        pushUndoCommand( new MakeEllipseUndoCommand(this,false,true,dx,dy,time) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateBuildingRectangle: {
        pushUndoCommand( new MakeRectangleUndoCommand(this,false,dx,dy,time) );
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
        TransformUndoCommand::TransformPointsSelectionEnum type = TransformUndoCommand::eTransformAllPoints;
        pushUndoCommand( new TransformUndoCommand(this,center.x(),center.y(),rot,skewX,skewY,tx,ty,sx,sy,time,type,_imp->rotoData->selectedCpsBbox) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingBBoxMidTop:
    case eEventStateDraggingBBoxMidBtm: {
        QPointF center = _imp->getSelectedCpsBBOXCenter();
        double rot = 0;
        double sx = 1.,sy = 1.;
        double skewX = 0.,skewY = 0.;
        double tx = 0., ty = 0.;
        
        TransformUndoCommand::TransformPointsSelectionEnum type;
        if (!modCASIsShift(e)) {
            type = TransformUndoCommand::eTransformAllPoints;
        } else {
            if (_imp->state == eEventStateDraggingBBoxMidTop) {
                type = TransformUndoCommand::eTransformMidTop;
            } else if (_imp->state == eEventStateDraggingBBoxMidBtm) {
                type = TransformUndoCommand::eTransformMidBottom;
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

        if (_imp->rotoData->transformMode == eSelectedCpsTransformModeRotateAndSkew) {
            const double addSkew = ( pos.x() - _imp->lastMousePos.x() ) / ( pos.y() - center.y() );
            skewX += addSkew;
        } else {
            // the scale ratio is the ratio of distances to the center
            double prevDist = ( _imp->lastMousePos.x() - center.x() ) * ( _imp->lastMousePos.x() - center.x() ) +
                              ( _imp->lastMousePos.y() - center.y() ) * ( _imp->lastMousePos.y() - center.y() );
            if (prevDist != 0) {
                double dist = ( pos.x() - center.x() ) * ( pos.x() - center.x() ) + ( pos.y() - center.y() ) * ( pos.y() - center.y() );
                double ratio = std::sqrt(dist / prevDist);
                sy *= ratio;
            }
        }
    

        
        pushUndoCommand( new TransformUndoCommand(this,center.x(),center.y(),rot,skewX,skewY,tx,ty,sx,sy,time,type,bbox) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingBBoxMidRight:
    case eEventStateDraggingBBoxMidLeft: {
        QPointF center = _imp->getSelectedCpsBBOXCenter();
        double rot = 0;
        double sx = 1.,sy = 1.;
        double skewX = 0.,skewY = 0.;
        double tx = 0., ty = 0.;
        
        
        TransformUndoCommand::TransformPointsSelectionEnum type;
        if (!modCASIsShift(e)) {
            type = TransformUndoCommand::eTransformAllPoints;
        } else {
            if (_imp->state == eEventStateDraggingBBoxMidRight) {
                type = TransformUndoCommand::eTransformMidRight;
            } else if (_imp->state == eEventStateDraggingBBoxMidLeft) {
                type = TransformUndoCommand::eTransformMidLeft;
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


        if (_imp->rotoData->transformMode == eSelectedCpsTransformModeRotateAndSkew) {
            const double addSkew = ( pos.y() - _imp->lastMousePos.y() ) / ( pos.x() - center.x() );
            skewY += addSkew;
        } else {
            // the scale ratio is the ratio of distances to the center
            double prevDist = ( _imp->lastMousePos.x() - center.x() ) * ( _imp->lastMousePos.x() - center.x() ) +
                              ( _imp->lastMousePos.y() - center.y() ) * ( _imp->lastMousePos.y() - center.y() );
            if (prevDist != 0) {
                double dist = ( pos.x() - center.x() ) * ( pos.x() - center.x() ) + ( pos.y() - center.y() ) * ( pos.y() - center.y() );
                double ratio = std::sqrt(dist / prevDist);
                sx *= ratio;
            }
        }
        
        pushUndoCommand( new TransformUndoCommand(this,center.x(),center.y(),rot,skewX,skewY,tx,ty,sx,sy,time,type,_imp->rotoData->selectedCpsBbox) );

        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
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
        int time = _imp->context->getTimelineCurrentTime();
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
RotoGui::penUp(double /*scaleX*/,
               double /*scaleY*/,
               const QPointF & /*viewportPos*/,
               const QPointF & /*pos*/,
               QMouseEvent* /*e*/)
{
    if (_imp->evaluateOnPenUp) {
        _imp->context->evaluateChange();
        _imp->node->getNode()->getApp()->triggerAutoSave();

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
    _imp->state = eEventStateNone;

    if ( (_imp->selectedTool == eRotoToolDrawEllipse) || (_imp->selectedTool == eRotoToolDrawRectangle) ) {
        _imp->rotoData->selectedCps.clear();
        onToolActionTriggered(_imp->selectAllAction);
    }

    return true;
}

void
RotoGui::removeCurve(const boost::shared_ptr<Bezier>& curve)
{
    if ( curve == _imp->rotoData->builtBezier ) {
        _imp->rotoData->builtBezier.reset();
    }
    _imp->context->removeItem(curve);
}

bool
RotoGui::keyDown(double /*scaleX*/,
                 double /*scaleY*/,
                 QKeyEvent* e)
{
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
        } else if ( !_imp->rotoData->selectedBeziers.empty() ) {
            pushUndoCommand( new RemoveCurveUndoCommand(this,_imp->rotoData->selectedBeziers) );
            didSomething = true;
        }
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCloseBezier, modifiers, key) ) {
        if ( (_imp->selectedTool == eRotoToolDrawBezier) && _imp->rotoData->builtBezier && !_imp->rotoData->builtBezier->isCurveFinished() ) {
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
        if ( _imp->rotoData->selectedBeziers.empty() ) {
            std::list<boost::shared_ptr<Bezier> > bez = _imp->context->getCurvesByRenderOrder();
            for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = bez.begin(); it != bez.end(); ++it) {
                _imp->context->select(*it, RotoContext::eSelectionReasonOverlayInteract);
                _imp->rotoData->selectedBeziers.push_back(*it);
            }
        } else {
            ///select all the control points of all selected beziers
            _imp->rotoData->selectedCps.clear();
            for (SelectedBeziers::iterator it = _imp->rotoData->selectedBeziers.begin(); it != _imp->rotoData->selectedBeziers.end(); ++it) {
                const std::list<boost::shared_ptr<BezierCP> > & cps = (*it)->getControlPoints();
                const std::list<boost::shared_ptr<BezierCP> > & fps = (*it)->getFeatherPoints();
                assert( cps.size() == fps.size() );

                std::list<boost::shared_ptr<BezierCP> >::const_iterator cpIT = cps.begin();
                for (std::list<boost::shared_ptr<BezierCP> >::const_iterator fpIT = fps.begin(); fpIT != fps.end(); ++fpIT, ++cpIT) {
                    _imp->rotoData->selectedCps.push_back( std::make_pair(*cpIT, *fpIT) );
                }
            }
            _imp->computeSelectedCpsBBOX();
        }
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSelectionTool, modifiers, key) ) {
        _imp->selectTool->handleSelection();
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoEditTool, modifiers, key) ) {
        _imp->bezierEditionTool->handleSelection();
    } else if ( isKeybind(kShortcutGroupRoto, kShortcutIDActionRotoAddTool, modifiers, key) ) {
        _imp->pointsEditionTool->handleSelection();
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
    }

    return didSomething;
} // keyDown

bool
RotoGui::keyRepeat(double /*scaleX*/,
                   double /*scaleY*/,
                   QKeyEvent* e)
{
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
RotoGui::focusOut()
{
    _imp->shiftDown = 0;
    _imp->ctrlDown = 0;
}

bool
RotoGui::keyUp(double /*scaleX*/,
               double /*scaleY*/,
               QKeyEvent* e)
{
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
RotoGui::RotoGuiPrivate::isNearbyFeatherBar(int time,
                                            const std::pair<double,double> & pixelScale,
                                            const QPointF & pos) const
{
    double distFeatherX = 20. * pixelScale.first;
    double acceptance = 10 * pixelScale.second;

    for (SelectedBeziers::const_iterator it = rotoData->selectedBeziers.begin(); it != rotoData->selectedBeziers.end(); ++it) {
        /*
            For each selected bezier, we compute the extent of the feather bars and check if the mouse would be nearby one of these bars.
            The feather bar of a control point is only displayed is the feather point is equal to the bezier control point.
            In order to give it the  correc direction we use the derivative of the bezier curve at the control point and then use
            the pointInPolygon function to make sure the feather bar is always oriented on the outter part of the polygon.
            The pointInPolygon function needs the polygon of the bezier to test whether the point is inside or outside the polygon
            hence in this loop we compute the polygon for each bezier.
         */
#pragma message WARN("pointInPolygon should not be used, see comment")
        /*
           The pointInPolygon function should not be used.
           The algorithm to know which side is the outside of a polygon consists in computing the global polygon orientation.
           To compute the orientation, compute its surface. If positive the polygon is clockwise, if negative it's counterclockwise.
           to compute the surface, take the starting point of the polygon, and imagine a fan made of all the triangles
           pointing at this point. The surface of a tringle is half the cross-product of two of its sides issued from
           the same point (the starting point of the polygon, in this case.
           The orientation of a polygon has to be computed only once for each modification of the polygon (whenever it's edited), and
           should be stored with the polygon.
           Of course an 8-shaped polygon doesn't have an outside, but it still has an orientation. The feather direction
           should follow this orientation.
         */

        const std::list<boost::shared_ptr<BezierCP> > & fps = (*it)->getFeatherPoints();
        const std::list<boost::shared_ptr<BezierCP> > & cps = (*it)->getControlPoints();
        int cpCount = (int)cps.size();
        if (cpCount <= 1) {
            continue;
        }
        std::list<Point> polygon;
        RectD polygonBBox( std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity() );
        (*it)->evaluateFeatherPointsAtTime_DeCasteljau(time, 0, 50, true, &polygon, &polygonBBox);

        std::list<boost::shared_ptr<BezierCP> >::const_iterator itF = fps.begin();
        std::list<boost::shared_ptr<BezierCP> >::const_iterator nextF = itF;
        ++nextF;
        std::list<boost::shared_ptr<BezierCP> >::const_iterator prevF = fps.end();
        --prevF;
        std::list<boost::shared_ptr<BezierCP> >::const_iterator itCp = cps.begin();

        for (; itCp != cps.end(); ++itF,++nextF,++prevF,++itCp) {
            if ( prevF == fps.end() ) {
                prevF = fps.begin();
            }
            if ( nextF == fps.end() ) {
                nextF = fps.begin();
            }

            Point controlPoint,featherPoint;
            (*itCp)->getPositionAtTime(time, &controlPoint.x, &controlPoint.y);
            (*itF)->getPositionAtTime(time, &featherPoint.x, &featherPoint.y);

            Bezier::expandToFeatherDistance(controlPoint, &featherPoint, distFeatherX, polygon,
                                            polygonBBox, time, prevF, itF, nextF);
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
        }
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
    return _imp->stickySelectionEnabled->isChecked();
}

void
RotoGui::onAddKeyFrameClicked()
{
    int time = _imp->context->getTimelineCurrentTime();

    for (SelectedBeziers::iterator it = _imp->rotoData->selectedBeziers.begin(); it != _imp->rotoData->selectedBeziers.end(); ++it) {
        (*it)->setKeyframe(time);
    }
    _imp->context->evaluateChange();
}

void
RotoGui::onRemoveKeyFrameClicked()
{
    int time = _imp->context->getTimelineCurrentTime();

    for (SelectedBeziers::iterator it = _imp->rotoData->selectedBeziers.begin(); it != _imp->rotoData->selectedBeziers.end(); ++it) {
        (*it)->removeKeyframe(time);
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
    _imp->rotoData->selectedBeziers = _imp->context->getSelectedCurves();
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
            for (SelectedBeziers::iterator fb = rotoData->selectedBeziers.begin(); fb != rotoData->selectedBeziers.end(); ++fb) {
                if ( fb->get() == b.get() ) {
                    ///if the curve was selected, wipe the selection CP bbox
                    clearCPSSelection();
                    rotoData->selectedBeziers.erase(fb);
                    *ret = true;
                    break;
                }
            }
        } else {
            ///Explanation: This change has been made in result to a user click on the settings panel.
            ///We have to reselect the bezier overlay hence put a reason different of eSelectionReasonOverlayInteract
            SelectedBeziers::iterator found = std::find(rotoData->selectedBeziers.begin(),rotoData->selectedBeziers.end(),b);
            if ( found == rotoData->selectedBeziers.end() ) {
                rotoData->selectedBeziers.push_back(b);
                context->select(b, RotoContext::eSelectionReasonSettingsPanel);
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
RotoGui::onCurveLockedChanged()
{
    boost::shared_ptr<RotoItem> item = _imp->context->getLastItemLocked();
    if (item) {
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
    if ( (RotoContext::SelectionReasonEnum)reason != RotoContext::eSelectionReasonOverlayInteract ) {
        _imp->rotoData->selectedBeziers = _imp->context->getSelectedCurves();
        _imp->viewer->redraw();
    }
}

void
RotoGui::setSelection(const std::list<boost::shared_ptr<Bezier> > & selectedBeziers,
                      const std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > & selectedCps)
{
    _imp->rotoData->selectedBeziers.clear();
    for (SelectedBeziers::const_iterator it = selectedBeziers.begin(); it != selectedBeziers.end(); ++it) {
        if (*it) {
            _imp->rotoData->selectedBeziers.push_back(*it);
        }
    }
    _imp->rotoData->selectedCps.clear();
    for (SelectedCPs::const_iterator it = selectedCps.begin(); it != selectedCps.end(); ++it) {
        if (it->first && it->second) {
            _imp->rotoData->selectedCps.push_back(*it);
        }
    }
    _imp->context->select(_imp->rotoData->selectedBeziers, RotoContext::eSelectionReasonOverlayInteract);
    _imp->computeSelectedCpsBBOX();
}

void
RotoGui::setSelection(const boost::shared_ptr<Bezier> & curve,
                      const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > & point)
{
    _imp->rotoData->selectedBeziers.clear();
    if (curve) {
        _imp->rotoData->selectedBeziers.push_back(curve);
    }
    _imp->rotoData->selectedCps.clear();
    if (point.first && point.second) {
        _imp->rotoData->selectedCps.push_back(point);
    }
    if (curve) {
        _imp->context->select(curve, RotoContext::eSelectionReasonOverlayInteract);
    }
    _imp->computeSelectedCpsBBOX();
}

void
RotoGui::getSelection(std::list<boost::shared_ptr<Bezier> >* selectedBeziers,
                      std::list<std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >* selectedCps)
{
    *selectedBeziers = _imp->rotoData->selectedBeziers;
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
    return _imp->node->getNode()->getName().c_str();
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
    QMenu menu(_imp->viewer);

    menu.setFont( QFont(appFont,appFontSize) );

    ActionWithShortcut* selectAllAction = new ActionWithShortcut(kShortcutGroupRoto,
                                                                 kShortcutIDActionRotoSelectAll,
                                                                 kShortcutDescActionRotoSelectAll,&menu);
    menu.addAction(selectAllAction);

    ActionWithShortcut* deleteCurve = new ActionWithShortcut(kShortcutGroupRoto,
                                                             kShortcutIDActionRotoDelete,
                                                             kShortcutDescActionRotoDelete,&menu);
    menu.addAction(deleteCurve);

    ActionWithShortcut* openCloseCurve = new ActionWithShortcut(kShortcutGroupRoto,
                                                                kShortcutIDActionRotoCloseBezier,
                                                                kShortcutDescActionRotoCloseBezier
                                                                ,&menu);
    menu.addAction(openCloseCurve);

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
        std::list<boost::shared_ptr<Bezier> > beziers;
        beziers.push_back(curve);
        pushUndoCommand( new RemoveCurveUndoCommand(this,beziers) );
        _imp->viewer->redraw();
    } else if (ret == openCloseCurve) {
        pushUndoCommand( new OpenCloseUndoCommand(this,curve) );
        _imp->viewer->redraw();
    } else if (ret == smoothAction) {
        smoothSelectedCurve();
    } else if (ret == cuspAction) {
        cuspSelectedCurve();
    } else if (ret == removeFeather) {
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
    int time = _imp->context->getTimelineCurrentTime();
    std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;

    for (SelectedBeziers::const_iterator it = _imp->rotoData->selectedBeziers.begin(); it != _imp->rotoData->selectedBeziers.end(); ++it) {
        SmoothCuspUndoCommand::SmoothCuspCurveData data;
        data.curve = *it;
        const std::list<boost::shared_ptr<BezierCP> > & cps = (*it)->getControlPoints();
        const std::list<boost::shared_ptr<BezierCP> > & fps = (*it)->getFeatherPoints();
        std::list<boost::shared_ptr<BezierCP> >::const_iterator itFp = fps.begin();
        for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin(); it != cps.end(); ++it,++itFp) {
            data.newPoints.push_back( std::make_pair(*it, *itFp) );
        }
        datas.push_back(data);
    }
    pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,false,pixelScale) );
    _imp->viewer->redraw();
}

void
RotoGui::cuspSelectedCurve()
{
    std::pair<double,double> pixelScale;
    _imp->viewer->getPixelScale(pixelScale.first, pixelScale.second);
    int time = _imp->context->getTimelineCurrentTime();
    std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;

    for (SelectedBeziers::const_iterator it = _imp->rotoData->selectedBeziers.begin(); it != _imp->rotoData->selectedBeziers.end(); ++it) {
        SmoothCuspUndoCommand::SmoothCuspCurveData data;
        data.curve = *it;
        const std::list<boost::shared_ptr<BezierCP> > & cps = (*it)->getControlPoints();
        const std::list<boost::shared_ptr<BezierCP> > & fps = (*it)->getFeatherPoints();
        std::list<boost::shared_ptr<BezierCP> >::const_iterator itFp = fps.begin();
        for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin(); it != cps.end(); ++it,++itFp) {
            data.newPoints.push_back( std::make_pair(*it, *itFp) );
        }
        datas.push_back(data);
    }
    pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,true,pixelScale) );
    _imp->viewer->redraw();
}

void
RotoGui::removeFeatherForSelectedCurve()
{
    std::list<RemoveFeatherUndoCommand::RemoveFeatherData> datas;

    for (SelectedBeziers::const_iterator it = _imp->rotoData->selectedBeziers.begin(); it != _imp->rotoData->selectedBeziers.end(); ++it) {
        RemoveFeatherUndoCommand::RemoveFeatherData data;
        data.curve = *it;
        data.newPoints = (*it)->getFeatherPoints();
        datas.push_back(data);
    }
    pushUndoCommand( new RemoveFeatherUndoCommand(this,datas) );
    _imp->viewer->redraw();
}

void
RotoGui::lockSelectedCurves()
{
    ///Make a copy because setLocked will change the selection internally and invalidate the iterator
    SelectedBeziers selection = _imp->rotoData->selectedBeziers;
    
    for (SelectedBeziers::const_iterator it = selection.begin(); it != selection.end(); ++it) {

        (*it)->setLocked(true, false);
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
    QMenu menu(_imp->viewer);

    menu.setFont( QFont(appFont,appFontSize) );

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

    boost::shared_ptr<Double_Knob> isSlaved = cp.first->isSlaved();
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
    int time = _imp->context->getTimelineCurrentTime();
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

    LinkToTrackDialog(const std::vector< std::pair<std::string,boost::shared_ptr<Double_Knob> > > & knobs,
                      QWidget* parent)
        : QDialog(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        _choice = new ComboBox(this);

        for (std::vector< std::pair<std::string,boost::shared_ptr<Double_Knob> > >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
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
    std::vector< std::pair<std::string,boost::shared_ptr<Double_Knob> > > knobs;
    std::vector<boost::shared_ptr<Natron::Node> > activeNodes;

    _imp->node->getNode()->getApp()->getActiveNodes(&activeNodes);
    for (U32 i = 0; i < activeNodes.size(); ++i) {
        if ( activeNodes[i]->isTrackerNode() ) {
            boost::shared_ptr<KnobI> k = activeNodes[i]->getKnobByName("center");
            boost::shared_ptr<KnobI> name = activeNodes[i]->getKnobByName(kOfxParamStringSublabelName);
            if (k && name) {
                boost::shared_ptr<Double_Knob> dk = boost::dynamic_pointer_cast<Double_Knob>(k);
                String_Knob* nameKnob = dynamic_cast<String_Knob*>( name.get() );
                if (dk && nameKnob) {
                    std::string trackName = nameKnob->getValue();
                    trackName += "/";
                    trackName += dk->getDescription();
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
            const boost::shared_ptr<Double_Knob> & knob = knobs[index].second;
            if ( knob && (knob->getDimension() == 2) ) {
                pushUndoCommand( new LinkToTrackUndoCommand(this,points,knob) );
            }
        }
    }
}

