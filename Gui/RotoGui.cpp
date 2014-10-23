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
#include <QMenu>
#include <QDialogButtonBox>
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
    NONE = 0,
    DRAGGING_CP,
    DRAGGING_SELECTED_CPS,
    BUILDING_BEZIER_CP_TANGENT,
    BUILDING_ELLIPSE,
    BULDING_ELLIPSE_CENTER,
    BUILDING_RECTANGLE,
    DRAGGING_LEFT_TANGENT,
    DRAGGING_RIGHT_TANGENT,
    DRAGGING_FEATHER_BAR,
    DRAGGING_BBOX_TOP_LEFT,
    DRAGGING_BBOX_TOP_RIGHT,
    DRAGGING_BBOX_BTM_RIGHT,
    DRAGGING_BBOX_BTM_LEFT,
    DRAGGING_BBOX_MID_TOP,
    DRAGGING_BBOX_MID_RIGHT,
    DRAGGING_BBOX_MID_BTM,
    DRAGGING_BBOX_MID_LEFT
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

enum SelectedCpsTransformMode
{
    TRANSLATE_AND_SCALE = 0,
    ROTATE_AND_SKEW = 1
};
}

///A small structure of all the data shared by all the viewers watching the same Roto
struct RotoGuiSharedData
{
    SelectedBeziers selectedBeziers;
    SelectedCPs selectedCps;
    QRectF selectedCpsBbox;
    bool showCpsBbox;

    ////This is by default TRANSLATE_AND_SCALE. When clicking the cross-hair in the center this will toggle the transform mode
    ////like it does in inkscape.
    SelectedCpsTransformMode transformMode;
    boost::shared_ptr<Bezier> builtBezier; //< the bezier currently being built
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
    Roto_Type type;
    QToolBar* toolbar;
    QWidget* selectionButtonsBar;
    QHBoxLayout* selectionButtonsBarLayout;
    Button* autoKeyingEnabled;
    Button* featherLinkEnabled;
    Button* displayFeatherEnabled;
    Button* stickySelectionEnabled;
    Button* rippleEditEnabled;
    Button* addKeyframeButton;
    Button* removeKeyframeButton;
    RotoToolButton* selectTool;
    RotoToolButton* pointsEditionTool;
    RotoToolButton* bezierEditionTool;
    QAction* selectAllAction;
    Roto_Tool selectedTool;
    QToolButton* selectedRole;
    EventStateEnum state;
    HoverStateEnum hoverState;
    QPointF lastClickPos;
    QPointF lastMousePos;
    boost::shared_ptr< RotoGuiSharedData > rotoData;
    bool evaluateOnPenUp; //< if true the next pen up will call context->evaluateChange()
    bool evaluateOnKeyUp;  //< if true the next key up will call context->evaluateChange()
    bool iSelectingwithCtrlA;

    RotoGuiPrivate(RotoGui* pub,
                   NodeGui* n,
                   ViewerTab* tab,
                   const boost::shared_ptr<RotoGuiSharedData> & sharedData)
        : publicInterface(pub)
          , node(n)
          , viewer( tab->getViewer() )
          , viewerTab(tab)
          , context()
          , type(ROTOSCOPING)
          , toolbar(0)
          , selectionButtonsBar(0)
          , selectTool(0)
          , pointsEditionTool(0)
          , bezierEditionTool(0)
          , selectAllAction(0)
          , selectedTool(SELECT_ALL)
          , selectedRole(0)
          , state(NONE)
          , hoverState(eHoverStateNothing)
          , lastClickPos()
          , lastMousePos()
          , rotoData(sharedData)
          , evaluateOnPenUp(false)
          , evaluateOnKeyUp(false)
          , iSelectingwithCtrlA(false)
    {
        if ( n->getNode()->isRotoPaintingNode() ) {
            type = ROTOPAINTING;
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

    bool removeBezierFromSelection(const Bezier* b);

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

    bool isNearbyBBoxTopLeft(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxTopRight(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxBtmLeft(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxBtmRight(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;

    bool isNearbyBBoxMidTop(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxMidRight(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxMidBtm(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;
    bool isNearbyBBoxMidLeft(const QPointF & p,double tolerance,const std::pair<double,double> & pixelScale) const;

    bool isNearbySelectedCpsBoundingBox(const QPointF & pos,double tolerance) const;
};

RotoToolButton::RotoToolButton(QWidget* parent)
    : QToolButton(parent)
{
}

void
RotoToolButton::mousePressEvent(QMouseEvent* /*e*/)
{
}

void
RotoToolButton::mouseReleaseEvent(QMouseEvent* e)
{
    if ( buttonDownIsLeft(e) ) {
        handleSelection();
    } else if ( buttonDownIsRight(e) ) {
        showMenu();
    } else {
        QToolButton::mousePressEvent(e);
    }
}

void
RotoToolButton::handleSelection()
{
    QAction* curAction = defaultAction();

    if ( !isDown() ) {
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
                          RotoGui::Roto_Tool tool)
{
    QAction *action = new QAction(icon,text,toolGroup);

    action->setToolTip(text + ": " + tooltip + "<p><b>" + tr("Keyboard shortcut: ") + shortcut.toString(QKeySequence::NativeText) + "</b></p>");

    QPoint data;
    data.setX( (int)tool );
    if (toolGroup == _imp->selectTool) {
        data.setY( (int)SELECTION_ROLE );
    } else if (toolGroup == _imp->pointsEditionTool) {
        data.setY( (int)POINTS_EDITION_ROLE );
    } else if (toolGroup == _imp->bezierEditionTool) {
        data.setY(BEZIER_EDITION_ROLE);
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

    QObject::connect( parent->getViewer(),SIGNAL( selectionRectangleChanged(bool) ),this,SLOT( updateSelectionFromSelectionRectangle(bool) ) );
    QObject::connect( parent->getViewer(), SIGNAL( selectionCleared() ), this, SLOT( onSelectionCleared() ) );
    QPixmap pixBezier,pixEllipse,pixRectangle,pixAddPts,pixRemovePts,pixCuspPts,pixSmoothPts,pixOpenCloseCurve,pixRemoveFeather;
    QPixmap pixSelectAll,pixSelectPoints,pixSelectFeather,pixSelectCurves,pixAutoKeyingEnabled,pixAutoKeyingDisabled;
    QPixmap pixStickySelEnabled,pixStickySelDisabled,pixFeatherLinkEnabled,pixFeatherLinkDisabled,pixAddKey,pixRemoveKey;
    QPixmap pixRippleEnabled,pixRippleDisabled;
    QPixmap pixFeatherEnabled,pixFeatherDisabled;

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
                                             selectShortCut, SELECT_ALL);
    createToolAction(_imp->selectTool, QIcon(pixSelectPoints), tr("Select points"),
                     tr("works only for the points of the inner shape,"
                        " feather points will not be taken into account."),
                     selectShortCut, SELECT_POINTS);
    createToolAction(_imp->selectTool, QIcon(pixSelectCurves), tr("Select curves"),
                     tr("only the curves can be selected.")
                     ,selectShortCut,SELECT_CURVES);
    createToolAction(_imp->selectTool, QIcon(pixSelectFeather), tr("Select feather points"), tr("only the feather points can be selected."),selectShortCut,SELECT_FEATHER_POINTS);
    _imp->selectTool->setDown(false);
    _imp->selectTool->setDefaultAction(_imp->selectAllAction);
    _imp->toolbar->addWidget(_imp->selectTool);

    _imp->pointsEditionTool = new RotoToolButton(_imp->toolbar);
    _imp->pointsEditionTool->setFixedSize(rotoToolSize);
    _imp->pointsEditionTool->setPopupMode(QToolButton::InstantPopup);
    QObject::connect( _imp->pointsEditionTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
    _imp->pointsEditionTool->setText( tr("Add points") );
    QKeySequence pointsEditionShortcut(Qt::Key_D);
    QAction* addPtsAct = createToolAction(_imp->pointsEditionTool, QIcon(pixAddPts), tr("Add points"),tr("add a new control point to the shape")
                                          ,pointsEditionShortcut, ADD_POINTS);
    createToolAction(_imp->pointsEditionTool, QIcon(pixRemovePts), tr("Remove points"),"",pointsEditionShortcut,REMOVE_POINTS);
    createToolAction(_imp->pointsEditionTool, QIcon(pixCuspPts), tr("Cusp points"),"", pointsEditionShortcut,CUSP_POINTS);
    createToolAction(_imp->pointsEditionTool, QIcon(pixSmoothPts), tr("Smooth points"),"", pointsEditionShortcut,SMOOTH_POINTS);
    createToolAction(_imp->pointsEditionTool, QIcon(pixOpenCloseCurve), tr("Open/Close curve"),"", pointsEditionShortcut,OPEN_CLOSE_CURVE);
    createToolAction(_imp->pointsEditionTool, QIcon(pixRemoveFeather), tr("Remove feather"),tr("set the feather point to be equal to the control point"), pointsEditionShortcut,REMOVE_FEATHER_POINTS);
    _imp->pointsEditionTool->setDown(false);
    _imp->pointsEditionTool->setDefaultAction(addPtsAct);
    _imp->toolbar->addWidget(_imp->pointsEditionTool);

    _imp->bezierEditionTool = new RotoToolButton(_imp->toolbar);
    _imp->bezierEditionTool->setFixedSize(rotoToolSize);
    _imp->bezierEditionTool->setPopupMode(QToolButton::InstantPopup);
    QObject::connect( _imp->bezierEditionTool, SIGNAL( triggered(QAction*) ), this, SLOT( onToolActionTriggered(QAction*) ) );
    _imp->bezierEditionTool->setText("Bezier");
    QKeySequence editBezierShortcut(Qt::Key_V);
    QAction* drawBezierAct = createToolAction(_imp->bezierEditionTool, QIcon(pixBezier), tr("Bezier"),
                                              tr("Edit bezier paths. Click and drag the mouse to adjust tangents. Press enter to close the shape. ")
                                              ,editBezierShortcut, DRAW_BEZIER);

    ////B-splines are not implemented yet
    //createToolAction(_imp->bezierEditionTool, QIcon(), "B-Spline", DRAW_B_SPLINE);

    createToolAction(_imp->bezierEditionTool, QIcon(pixEllipse), tr("Ellipse"),tr("Hold control to draw the ellipse from its center"),editBezierShortcut, DRAW_ELLIPSE);
    createToolAction(_imp->bezierEditionTool, QIcon(pixRectangle), tr("Rectangle"),"", editBezierShortcut,DRAW_RECTANGLE);
    _imp->toolbar->addWidget(_imp->bezierEditionTool);

    ////////////Default action is to make a new bezier
    _imp->selectedRole = _imp->selectTool;
    onToolActionTriggered(drawBezierAct);

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
RotoGui::getButtonsBar(RotoGui::Roto_Role role) const
{
    switch (role) {
    case SELECTION_ROLE:

        return _imp->selectionButtonsBar;
        break;
    case POINTS_EDITION_ROLE:

        return _imp->selectionButtonsBar;
        break;
    case BEZIER_EDITION_ROLE:

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

RotoGui::Roto_Tool
RotoGui::getSelectedTool() const
{
    return _imp->selectedTool;
}

void
RotoGui::setCurrentTool(RotoGui::Roto_Tool tool,
                        bool emitSignal)
{
    QList<QAction*> actions = _imp->selectTool->actions();
    actions.append( _imp->pointsEditionTool->actions() );
    actions.append( _imp->bezierEditionTool->actions() );
    for (int i = 0; i < actions.size(); ++i) {
        QPoint data = actions[i]->data().toPoint();
        if ( (RotoGui::Roto_Tool)data.x() == tool ) {
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
    Roto_Role actionRole = (Roto_Role)data.y();
    QToolButton* toolButton = 0;
    Roto_Role previousRole = getCurrentRole();

    switch (actionRole) {
    case SELECTION_ROLE:
        toolButton = _imp->selectTool;
        emit roleChanged( (int)previousRole,(int)SELECTION_ROLE );
        break;
    case POINTS_EDITION_ROLE:
        toolButton = _imp->pointsEditionTool;
        emit roleChanged( (int)previousRole,(int)POINTS_EDITION_ROLE );
        break;
    case BEZIER_EDITION_ROLE:
        toolButton = _imp->bezierEditionTool;
        emit roleChanged( (int)previousRole,(int)BEZIER_EDITION_ROLE );
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
    _imp->rotoData->transformMode = TRANSLATE_AND_SCALE;
    _imp->rotoData->selectedCpsBbox.setTopLeft( QPointF(0,0) );
    _imp->rotoData->selectedCpsBbox.setTopRight( QPointF(0,0) );

    ///clear all selection if we were building a new bezier
    if ( (previousRole == BEZIER_EDITION_ROLE) && (_imp->selectedTool == DRAW_BEZIER) && _imp->rotoData->builtBezier &&
         ( (Roto_Tool)data.x() != _imp->selectedTool ) ) {
        _imp->rotoData->builtBezier->setCurveFinished(true);
        _imp->clearSelection();
    }


    assert(toolButton);
    toolButton->setDown(true);
    toolButton->setDefaultAction(action);
    _imp->selectedRole = toolButton;
    _imp->selectedTool = (Roto_Tool)data.x();
    if (emitSignal) {
        emit selectedToolChanged( (int)_imp->selectedTool );
    }
} // onToolActionTriggeredInternal

void
RotoGui::onToolActionTriggered(QAction* act)
{
    onToolActionTriggeredInternal(act, true);
}

RotoGui::Roto_Role
RotoGui::getCurrentRole() const
{
    if (_imp->selectedRole == _imp->selectTool) {
        return SELECTION_ROLE;
    } else if (_imp->selectedRole == _imp->pointsEditionTool) {
        return POINTS_EDITION_ROLE;
    } else if (_imp->selectedRole == _imp->bezierEditionTool) {
        return BEZIER_EDITION_ROLE;
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
         ( ( state == DRAGGING_LEFT_TANGENT) || ( state == DRAGGING_RIGHT_TANGENT) ) ) {
        colorLeftTangent = state == DRAGGING_LEFT_TANGENT ? true : false;
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
            if ( (*it)->isActivated(time) ) {
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

                        ///if the control point is the only control point being dragged, color it to identify it to the user
                        bool colorChanged = false;
                        SelectedCPs::const_iterator firstSelectedCP = _imp->rotoData->selectedCps.begin();
                        if ( (firstSelectedCP->first == *it2)
                            && ( _imp->rotoData->selectedCps.size() == 1) &&
                            ( ( _imp->state == DRAGGING_SELECTED_CPS) || ( _imp->state == DRAGGING_CP) ) ) {
                            glColor3f(0.2, 1., 0.);
                            colorChanged = true;
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
                            ( ( _imp->state == DRAGGING_SELECTED_CPS) || ( _imp->state == DRAGGING_CP) )
                            && !colorChanged ) {
                            glColor3f(0.2, 1., 0.);
                            colorChanged = true;
                        }

                        double xF,yF;
                        (*itF)->getPositionAtTime(time, &xF, &yF);
                        ///draw the feather point only if it is distinct from the associated point
                        bool drawFeather = isFeatherVisible();
                        if (drawFeather) {
                            drawFeather = !(*it2)->equalsAtTime(time, **itF);
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


                            if ( ( (_imp->state == DRAGGING_FEATHER_BAR) &&
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
                            if ( (_imp->selectedTool == SELECT_ALL) || (_imp->selectedTool == SELECT_FEATHER_POINTS) ) {
                                int cpCount = (*it2)->getBezier()->getControlPointsCount();
                                if (cpCount > 1) {
                                    Natron::Point controlPoint;
                                    controlPoint.x = x;
                                    controlPoint.y = y;
                                    Natron::Point featherPoint;
                                    featherPoint.x = xF;
                                    featherPoint.y = yF;

                                    Bezier::expandToFeatherDistance(controlPoint, &featherPoint, distFeatherX, featherPoints, featherBBox, time, prevCp, it2, nextCp);

                                    if ( ( (_imp->state == DRAGGING_FEATHER_BAR) &&
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
                        }
                        
                        
                        if (colorChanged) {
                            glColor3d(0.85, 0.67, 0.);
                        }
                        
                        
                        for (SelectedCPs::const_iterator cpIt = _imp->rotoData->selectedCps.begin();
                             cpIt != _imp->rotoData->selectedCps.end(); ++cpIt) {
                            ///if the control point is selected, draw its tangent handles
                            if (cpIt->first == *it2) {
                                _imp->drawSelectedCp(time, cpIt->first, x, y);
                                if (drawFeather) {
                                    _imp->drawSelectedCp(time, cpIt->second, xF, yF);
                                }
                            } else if (cpIt->second == *it2) {
                                _imp->drawSelectedCp(time, cpIt->second, x, y);
                                if (drawFeather) {
                                    _imp->drawSelectedCp(time, cpIt->first, xF, yF);
                                }
                            }
                        }
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
    GLProtectMatrix p(GL_PROJECTION); // or should it be GL_MODELVIEW?

    if (hovered) {
        glColor3f(0., 1., 0.);
    } else {
        glColor3f(1., 1., 1.);
    }

    double arrowLenght =  kTransformArrowLenght * pixelScale.second;
    double arrowWidth = kTransformArrowWidth * pixelScale.second;
    double arrowHeadHeight = 4 * pixelScale.second;


    glMatrixMode(GL_PROJECTION); // or should it be GL_MODELVIEW?
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
    GLProtectMatrix p(GL_PROJECTION); // or should it be GL_MODELVIEW?

    if (hovered) {
        glColor3f(0., 1., 0.);
    } else {
        glColor3f(1., 1., 1.);
    }

    double arrowLenght =  kTransformArrowLenght * pixelScale.second;
    double arrowWidth = kTransformArrowWidth * pixelScale.second;
    double arrowHeadHeight = 4 * pixelScale.second;

    glMatrixMode(GL_PROJECTION); // or should it be GL_MODELVIEW?
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
        bool drawHandles = state != DRAGGING_BBOX_BTM_LEFT && state != DRAGGING_BBOX_BTM_RIGHT &&
        state != DRAGGING_BBOX_TOP_LEFT && state != DRAGGING_BBOX_TOP_RIGHT && state != DRAGGING_BBOX_MID_TOP
        && state != DRAGGING_BBOX_MID_RIGHT && state != DRAGGING_BBOX_MID_LEFT && state != DRAGGING_BBOX_MID_BTM;


        if (drawHandles) {
            double offset = kTransformArrowOffsetFromPoint * pixelScale.first;
            double halfOffset = offset / 2.;
            if (rotoData->transformMode == TRANSLATE_AND_SCALE) {
                ///draw mid top arrow vertical
                drawArrow(midTop.x(), midTop.y() + offset, 0., hoverState == eHoverStateBboxMidTop, pixelScale);
                ///draw mid right arrow horizontal
                drawArrow(midRight.x() + offset, midRight.y(), 90., hoverState == eHoverStateBboxMidRight, pixelScale);
                ///draw mid btm arrow vertical
                drawArrow(midBtm.x(), midBtm.y() - offset, 0., hoverState == eHoverStateBboxMidBtm, pixelScale);
                ///draw mid left arrow horizontal
                drawArrow(midLeft.x() - offset, midLeft.y(), 90., hoverState == eHoverStateBboxMidLeft, pixelScale);
                ///draw top left arrow rotated
                drawArrow(topLeft.x() - halfOffset, topLeft.y() + halfOffset, -45., hoverState == eHoverStateBboxTopLeft, pixelScale);
                ///draw top right arrow rotated
                drawArrow(btmRight.x() + halfOffset, topLeft.y() + halfOffset, 45., hoverState == eHoverStateBboxTopRight, pixelScale);
                ///draw btm right arrow rotated
                drawArrow(btmRight.x() + halfOffset, btmRight.y() - halfOffset, -45., hoverState == eHoverStateBboxBtmRight, pixelScale);
                ///draw btm left arrow rotated
                drawArrow(topLeft.x() - halfOffset, btmRight.y() - halfOffset, 45., hoverState == eHoverStateBboxBtmLeft, pixelScale);
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
    if ( !isStickySelectionEnabled() ) {
        _imp->clearSelection();
    }
}

void
RotoGui::updateSelectionFromSelectionRectangle(bool onRelease)
{
    if ( !onRelease || !_imp->node->isSettingsPanelVisible() ) {
        return;
    }

    if ( !isStickySelectionEnabled() ) {
        _imp->clearSelection();
    }

    int selectionMode = -1;
    if (_imp->selectedTool == SELECT_ALL) {
        selectionMode = 0;
    } else if (_imp->selectedTool == SELECT_POINTS) {
        selectionMode = 1;
    } else if ( (_imp->selectedTool == SELECT_FEATHER_POINTS) || (_imp->selectedTool == SELECT_CURVES) ) {
        selectionMode = 2;
    }

    double l,r,b,t;
    _imp->viewer->getSelectionRectangle(l, r, b, t);
    std::list<boost::shared_ptr<Bezier> > curves = _imp->context->getCurvesByRenderOrder();
    for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        if ( !(*it)->isLockedRecursive() ) {
            SelectedCPs points  = (*it)->controlPointsWithinRect(l, r, b, t, 0,selectionMode);
            if (_imp->selectedTool != SELECT_CURVES) {
                for (SelectedCPs::iterator ptIt = points.begin(); ptIt != points.end(); ++ptIt) {
                    if ( !isFeatherVisible() && ptIt->first->isFeatherPoint() ) {
                        continue;
                    }
                    _imp->rotoData->selectedCps.push_back(*ptIt);
                }
            }
            if ( !points.empty() ) {
                _imp->rotoData->selectedBeziers.push_back(*it);
            }
        }
    }


    _imp->context->select(_imp->rotoData->selectedBeziers, RotoContext::OVERLAY_INTERACT);

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
    rotoData->transformMode = TRANSLATE_AND_SCALE;
    rotoData->selectedCpsBbox.setTopLeft( QPointF(0,0) );
    rotoData->selectedCpsBbox.setTopRight( QPointF(0,0) );
}

void
RotoGui::RotoGuiPrivate::clearBeziersSelection()
{
    context->clearSelection(RotoContext::OVERLAY_INTERACT);
    rotoData->selectedBeziers.clear();
}

bool
RotoGui::RotoGuiPrivate::removeBezierFromSelection(const Bezier* b)
{
    for (SelectedBeziers::iterator fb = rotoData->selectedBeziers.begin(); fb != rotoData->selectedBeziers.end(); ++fb) {
        if (fb->get() == b) {
            context->deselect(*fb,RotoContext::OVERLAY_INTERACT);
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
        context->select(curve,RotoContext::OVERLAY_INTERACT);
    }
}

void
RotoGui::RotoGuiPrivate::handleControlPointSelection(const std::pair<boost::shared_ptr<BezierCP>,
                                                                     boost::shared_ptr<BezierCP> > & p,
                                                     QMouseEvent* e)
{
    ///find out if the cp is already selected.
    SelectedCPs::const_iterator foundCP = rotoData->selectedCps.end();

    for (SelectedCPs::const_iterator it = rotoData->selectedCps.begin(); it != rotoData->selectedCps.end(); ++it) {
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
    }

    rotoData->cpBeingDragged = p;
    state = DRAGGING_CP;
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
    int tangentSelectionTol = kTangentHandleSelectionTolerance * pixelScale.first;
    double cpSelectionTolerance = kControlPointSelectionTolerance * pixelScale.first;

    if ( _imp->rotoData->showCpsBbox && _imp->isNearbySelectedCpsCrossHair(pos) ) {
        _imp->state = DRAGGING_SELECTED_CPS;
    } else if ( _imp->rotoData->showCpsBbox && _imp->isNearbyBBoxTopLeft(pos, cpSelectionTolerance,pixelScale) ) {
        _imp->state = DRAGGING_BBOX_TOP_LEFT;
    } else if ( _imp->rotoData->showCpsBbox && _imp->isNearbyBBoxTopRight(pos, cpSelectionTolerance,pixelScale) ) {
        _imp->state = DRAGGING_BBOX_TOP_RIGHT;
    } else if ( _imp->rotoData->showCpsBbox && _imp->isNearbyBBoxBtmLeft(pos, cpSelectionTolerance,pixelScale) ) {
        _imp->state = DRAGGING_BBOX_BTM_LEFT;
    } else if ( _imp->rotoData->showCpsBbox && _imp->isNearbyBBoxBtmRight(pos, cpSelectionTolerance,pixelScale) ) {
        _imp->state = DRAGGING_BBOX_BTM_RIGHT;
    } else if ( _imp->rotoData->showCpsBbox && _imp->isNearbyBBoxMidTop(pos, cpSelectionTolerance,pixelScale) ) {
        _imp->state = DRAGGING_BBOX_MID_TOP;
    } else if ( _imp->rotoData->showCpsBbox && _imp->isNearbyBBoxMidRight(pos, cpSelectionTolerance,pixelScale) ) {
        _imp->state = DRAGGING_BBOX_MID_RIGHT;
    } else if ( _imp->rotoData->showCpsBbox && _imp->isNearbyBBoxMidBtm(pos, cpSelectionTolerance,pixelScale) ) {
        _imp->state = DRAGGING_BBOX_MID_BTM;
    } else if ( _imp->rotoData->showCpsBbox && _imp->isNearbyBBoxMidLeft(pos, cpSelectionTolerance,pixelScale) ) {
        _imp->state = DRAGGING_BBOX_MID_LEFT;
    }

    if (_imp->state != NONE) {
        return true;
    }


    ////////////////// TANGENT SELECTION
    ///in all cases except cusp/smooth if a control point is selected, check if the user clicked on a tangent handle
    ///in which case we go into eEventStateDraggingTangent mode
    if ( (_imp->selectedTool != CUSP_POINTS) && (_imp->selectedTool != SMOOTH_POINTS) && (_imp->selectedTool != SELECT_CURVES) ) {
        for (SelectedCPs::iterator it = _imp->rotoData->selectedCps.begin(); it != _imp->rotoData->selectedCps.end(); ++it) {
            if ( (_imp->selectedTool == SELECT_ALL) ||
                 ( _imp->selectedTool == DRAW_BEZIER) ) {
                int ret = it->first->isNearbyTangent(time, pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->rotoData->tangentBeingDragged = it->first;
                    _imp->state = ret == 0 ? DRAGGING_LEFT_TANGENT : DRAGGING_RIGHT_TANGENT;
                    didSomething = true;
                } else {
                    ///try with the counter part point
                    ret = it->second->isNearbyTangent(time, pos.x(), pos.y(), tangentSelectionTol);
                    if (ret >= 0) {
                        _imp->rotoData->tangentBeingDragged = it->second;
                        _imp->state = ret == 0 ? DRAGGING_LEFT_TANGENT : DRAGGING_RIGHT_TANGENT;
                        didSomething = true;
                    }
                }
            } else if (_imp->selectedTool == SELECT_FEATHER_POINTS) {
                const boost::shared_ptr<BezierCP> & fp = it->first->isFeatherPoint() ? it->first : it->second;
                int ret = fp->isNearbyTangent(time, pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->rotoData->tangentBeingDragged = fp;
                    _imp->state = ret == 0 ? DRAGGING_LEFT_TANGENT : DRAGGING_RIGHT_TANGENT;
                    didSomething = true;
                }
            } else if (_imp->selectedTool == SELECT_POINTS) {
                const boost::shared_ptr<BezierCP> & cp = it->first->isFeatherPoint() ? it->second : it->first;
                int ret = cp->isNearbyTangent(time, pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->rotoData->tangentBeingDragged = cp;
                    _imp->state = ret == 0 ? DRAGGING_LEFT_TANGENT : DRAGGING_RIGHT_TANGENT;
                    didSomething = true;
                }
            }

            ///check in case this is a feather tangent
            if ( _imp->rotoData->tangentBeingDragged && _imp->rotoData->tangentBeingDragged->isFeatherPoint() && !isFeatherVisible() ) {
                _imp->rotoData->tangentBeingDragged.reset();
                _imp->state = NONE;
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
        if ( nearbyBezier->isLockedRecursive() ) {
            nearbyBezier.reset();
        } else {
            Bezier::ControlPointSelectionPref pref = Bezier::WHATEVER_FIRST;
            if ( (_imp->selectedTool == SELECT_FEATHER_POINTS) && isFeatherVisible() ) {
                pref = Bezier::FEATHER_FIRST;
            }

            nearbyCP = nearbyBezier->isNearbyControlPoint(pos.x(), pos.y(), cpSelectionTolerance,pref,&nearbyCpIndex);
        }
    }
    switch (_imp->selectedTool) {
    case SELECT_ALL:
    case SELECT_POINTS:
    case SELECT_FEATHER_POINTS: {
        if ( ( _imp->selectedTool == SELECT_FEATHER_POINTS) && !isFeatherVisible() ) {
            ///nothing to do
            break;
        }
        std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > featherBarSel;
        if ( ( ( _imp->selectedTool == SELECT_ALL) || ( _imp->selectedTool == SELECT_FEATHER_POINTS) ) ) {
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
                    _imp->state = NONE;
                    showMenuForControlPoint(nearbyBezier,nearbyCP);
                }
                didSomething = true;
            } else if (featherBarSel.first) {
                _imp->clearCPSSelection();
                _imp->rotoData->featherBarBeingDragged = featherBarSel;

                ///Also select the point only if the curve is the same!
                if ( featherBarSel.first->getBezier() == nearbyBezier.get() ) {
                    _imp->handleControlPointSelection(_imp->rotoData->featherBarBeingDragged, e);
                    _imp->handleBezierSelection(nearbyBezier, e);
                }
                _imp->state = DRAGGING_FEATHER_BAR;
                didSomething = true;
            } else {
                SelectedBeziers::const_iterator found =
                    std::find(_imp->rotoData->selectedBeziers.begin(),_imp->rotoData->selectedBeziers.end(),nearbyBezier);
                if ( found == _imp->rotoData->selectedBeziers.end() ) {
                    _imp->handleBezierSelection(nearbyBezier, e);
                }
                if ( buttonDownIsRight(e) ) {
                    showMenuForCurve(nearbyBezier);
                }
                didSomething = true;
            }
        } else {
            bool nearbySelectedBeziersBbox = _imp->isNearbySelectedCpsBoundingBox(pos, cpSelectionTolerance);

            if (featherBarSel.first) {
                _imp->clearCPSSelection();
                _imp->rotoData->featherBarBeingDragged = featherBarSel;
                _imp->handleControlPointSelection(_imp->rotoData->featherBarBeingDragged, e);
                _imp->state = DRAGGING_FEATHER_BAR;
                didSomething = true;
            } else if (nearbySelectedBeziersBbox) {
                _imp->rotoData->transformMode = _imp->rotoData->transformMode == TRANSLATE_AND_SCALE ?
                                                ROTATE_AND_SKEW : TRANSLATE_AND_SCALE;
                didSomething = true;
            }
        }
        break;
    }
    case SELECT_CURVES:

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
        }
        break;
    case ADD_POINTS:
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
    case REMOVE_POINTS:
        if (nearbyCP.first) {
            assert( nearbyBezier && nearbyBezier.get() == nearbyCP.first->getBezier() );
            if ( nearbyCP.first->isFeatherPoint() ) {
                pushUndoCommand( new RemovePointUndoCommand(this,nearbyBezier,nearbyCP.second) );
            } else {
                pushUndoCommand( new RemovePointUndoCommand(this,nearbyBezier,nearbyCP.first) );
            }
            didSomething = true;
        }
        break;
    case REMOVE_FEATHER_POINTS:
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
    case OPEN_CLOSE_CURVE:
        if (nearbyBezier) {
            pushUndoCommand( new OpenCloseUndoCommand(this,nearbyBezier) );
            didSomething = true;
        }
        break;
    case SMOOTH_POINTS:

        if (nearbyCP.first) {
            std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = nearbyBezier;
            data.newPoints.push_back(nearbyCP);
            datas.push_back(data);
            pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,false) );
            didSomething = true;
        }
        break;
    case CUSP_POINTS:
        if ( nearbyCP.first && _imp->context->isAutoKeyingEnabled() ) {
            std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = nearbyBezier;
            data.newPoints.push_back(nearbyCP);
            datas.push_back(data);
            pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,true) );
            didSomething = true;
        }
        break;
    case DRAW_BEZIER: {
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
        _imp->state = BUILDING_BEZIER_CP_TANGENT;
        didSomething = true;
        break;
    }
    case DRAW_B_SPLINE:

        break;
    case DRAW_ELLIPSE: {
        bool fromCenter = modCASIsControl(e);
        pushUndoCommand( new MakeEllipseUndoCommand(this,true,fromCenter,pos.x(),pos.y(),time) );
        if (fromCenter) {
            _imp->state = BULDING_ELLIPSE_CENTER;
        } else {
            _imp->state = BUILDING_ELLIPSE;
        }
        didSomething = true;
        break;
    }
    case DRAW_RECTANGLE: {
        pushUndoCommand( new MakeRectangleUndoCommand(this,true,pos.x(),pos.y(),time) );
        _imp->evaluateOnPenUp = true;
        _imp->state = BUILDING_RECTANGLE;
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

    if (_imp->selectedTool == SELECT_ALL) {
        double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
        double nearbyBezierT;
        int nearbyBezierCPIndex;
        bool isFeather;
        boost::shared_ptr<Bezier> nearbyBezier =
            _imp->context->isNearbyBezier(pos.x(), pos.y(), bezierSelectionTolerance,&nearbyBezierCPIndex,&nearbyBezierT,&isFeather);
        if ( nearbyBezier && nearbyBezier->isLockedRecursive() ) {
            nearbyBezier.reset();
        }
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
    if ( (_imp->rotoData->selectedCps.size() > 1) && _imp->isNearbySelectedCpsCrossHair(pos) ) {
        _imp->viewer->setCursor( QCursor(Qt::SizeAllCursor) );
        cursorSet = true;
    } else {
        double cpTol = kControlPointSelectionTolerance * pixelScale.first;


        if ( !cursorSet && _imp->rotoData->showCpsBbox && (_imp->state != DRAGGING_CP) && (_imp->state != DRAGGING_SELECTED_CPS)
             && ( _imp->state != DRAGGING_LEFT_TANGENT) &&
             ( _imp->state != DRAGGING_RIGHT_TANGENT) ) {
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
            if ( (_imp->state != DRAGGING_CP) && (_imp->state != DRAGGING_SELECTED_CPS) ) {
                for (SelectedBeziers::const_iterator it = _imp->rotoData->selectedBeziers.begin(); it != _imp->rotoData->selectedBeziers.end(); ++it) {
                    int index = -1;
                    std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > nb =
                        (*it)->isNearbyControlPoint(pos.x(), pos.y(), cpTol,Bezier::WHATEVER_FIRST,&index);
                    if ( (index != -1) && ( ( !nb.first->isFeatherPoint() && !isFeatherVisible() ) || isFeatherVisible() ) ) {
                        _imp->viewer->setCursor( QCursor(Qt::CrossCursor) );
                        cursorSet = true;
                        break;
                    }
                }
            }
            if ( !cursorSet && (_imp->state != DRAGGING_LEFT_TANGENT) && (_imp->state != DRAGGING_RIGHT_TANGENT) ) {
                ///find a nearby tangent
                for (SelectedCPs::const_iterator it = _imp->rotoData->selectedCps.begin(); it != _imp->rotoData->selectedCps.end(); ++it) {
                    if (it->first->isNearbyTangent(time, pos.x(), pos.y(), cpTol) != -1) {
                        _imp->viewer->setCursor( QCursor(Qt::CrossCursor) );
                        cursorSet = true;
                        break;
                    }
                }
            }
            if ( !cursorSet && (_imp->state != DRAGGING_CP) && (_imp->state != DRAGGING_SELECTED_CPS) && (_imp->state != DRAGGING_LEFT_TANGENT) &&
                 ( _imp->state != DRAGGING_RIGHT_TANGENT) ) {
                double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
                double nearbyBezierT;
                int nearbyBezierCPIndex;
                bool isFeather;
                boost::shared_ptr<Bezier> nearbyBezier =
                    _imp->context->isNearbyBezier(pos.x(), pos.y(), bezierSelectionTolerance,&nearbyBezierCPIndex,&nearbyBezierT,&isFeather);
                if ( isFeather && !isFeatherVisible() ) {
                    nearbyBezier.reset();
                }
                if ( nearbyBezier && !nearbyBezier->isLockedRecursive() ) {
                    _imp->viewer->setCursor( QCursor(Qt::PointingHandCursor) );
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
            if ( (_imp->state != NONE) || _imp->rotoData->featherBarBeingHovered.first || cursorSet || (lastHoverState != eHoverStateNothing) ) {
                didSomething = true;
            }
        }
    }

    if (!cursorSet) {
        _imp->viewer->setCursor( QCursor(Qt::ArrowCursor) );
    }


    double dx = pos.x() - _imp->lastMousePos.x();
    double dy = pos.y() - _imp->lastMousePos.y();
    switch (_imp->state) {
    case DRAGGING_SELECTED_CPS: {
        pushUndoCommand( new MoveControlPointsUndoCommand(this,_imp->rotoData->selectedCps,dx,dy,time) );
        _imp->evaluateOnPenUp = true;
        _imp->computeSelectedCpsBBOX();
        didSomething = true;
        break;
    }
    case DRAGGING_CP: {
        assert(_imp->rotoData->cpBeingDragged.first && _imp->rotoData->cpBeingDragged.second);
        std::list<SelectedCP> toDrag;
        toDrag.push_back(_imp->rotoData->cpBeingDragged);
        pushUndoCommand( new MoveControlPointsUndoCommand(this,toDrag,dx,dy,time) );
        _imp->evaluateOnPenUp = true;
        _imp->computeSelectedCpsBBOX();
        didSomething = true;
    };  break;
    case BUILDING_BEZIER_CP_TANGENT: {
        assert(_imp->rotoData->builtBezier);
        pushUndoCommand( new MakeBezierUndoCommand(this,_imp->rotoData->builtBezier,false,dx,dy,time) );
        break;
    }
    case BUILDING_ELLIPSE: {
        pushUndoCommand( new MakeEllipseUndoCommand(this,false,false,dx,dy,time) );

        didSomething = true;
        _imp->evaluateOnPenUp = true;
        break;
    }
    case BULDING_ELLIPSE_CENTER: {
        pushUndoCommand( new MakeEllipseUndoCommand(this,false,true,dx,dy,time) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case BUILDING_RECTANGLE: {
        pushUndoCommand( new MakeRectangleUndoCommand(this,false,dx,dy,time) );
        didSomething = true;
        _imp->evaluateOnPenUp = true;
        break;
    }
    case DRAGGING_LEFT_TANGENT: {
        assert(_imp->rotoData->tangentBeingDragged);
        pushUndoCommand( new MoveTangentUndoCommand( this,dx,dy,time,_imp->rotoData->tangentBeingDragged,true,
                                                     modCASIsControl(e) ) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case DRAGGING_RIGHT_TANGENT: {
        assert(_imp->rotoData->tangentBeingDragged);
        pushUndoCommand( new MoveTangentUndoCommand( this,dx,dy,time,_imp->rotoData->tangentBeingDragged,false,
                                                     modCASIsControl(e) ) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case DRAGGING_FEATHER_BAR: {
        pushUndoCommand( new MoveFeatherBarUndoCommand(this,dx,dy,_imp->rotoData->featherBarBeingDragged,time) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case DRAGGING_BBOX_TOP_LEFT:
    case DRAGGING_BBOX_TOP_RIGHT:
    case DRAGGING_BBOX_BTM_RIGHT:
    case DRAGGING_BBOX_BTM_LEFT: {
        QPointF center = _imp->getSelectedCpsBBOXCenter();
        double rot = 0;
        double sx = 1.,sy = 1.;

        if (_imp->rotoData->transformMode == ROTATE_AND_SKEW) {
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
    case DRAGGING_BBOX_MID_TOP:
    case DRAGGING_BBOX_MID_BTM: {
        QPointF center = _imp->getSelectedCpsBBOXCenter();
        double rot = 0;
        double sx = 1.,sy = 1.;
        double skewX = 0.,skewY = 0.;
        double tx = 0., ty = 0.;

        if (_imp->rotoData->transformMode == ROTATE_AND_SKEW) {
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
        pushUndoCommand( new TransformUndoCommand(this,center.x(),center.y(),rot,skewX,skewY,tx,ty,sx,sy,time) );
        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case DRAGGING_BBOX_MID_RIGHT:
    case DRAGGING_BBOX_MID_LEFT: {
        QPointF center = _imp->getSelectedCpsBBOXCenter();
        double rot = 0;
        double sx = 1.,sy = 1.;
        double skewX = 0.,skewY = 0.;
        double tx = 0., ty = 0.;

        if (_imp->rotoData->transformMode == ROTATE_AND_SKEW) {
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
        pushUndoCommand( new TransformUndoCommand(this,center.x(),center.y(),rot,skewX,skewY,tx,ty,sx,sy,time) );

        _imp->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case NONE:
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
    _imp->rotoData->cpBeingDragged.first.reset();
    _imp->rotoData->cpBeingDragged.second.reset();
    _imp->rotoData->featherBarBeingDragged.first.reset();
    _imp->rotoData->featherBarBeingDragged.second.reset();
    _imp->state = NONE;

    if ( (_imp->selectedTool == DRAW_ELLIPSE) || (_imp->selectedTool == DRAW_RECTANGLE) ) {
        _imp->rotoData->selectedCps.clear();
        onToolActionTriggered(_imp->selectAllAction);
    }

    return true;
}

void
RotoGui::removeCurve(Bezier* curve)
{
    if ( curve == _imp->rotoData->builtBezier.get() ) {
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

    if ( modCASIsControl(e) ) {
        if ( !_imp->iSelectingwithCtrlA && _imp->rotoData->showCpsBbox && (e->key() == Qt::Key_Control) ) {
            _imp->rotoData->transformMode = _imp->rotoData->transformMode == TRANSLATE_AND_SCALE ?
                                            ROTATE_AND_SKEW : TRANSLATE_AND_SCALE;
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
        if ( (_imp->selectedTool == DRAW_BEZIER) && _imp->rotoData->builtBezier && !_imp->rotoData->builtBezier->isCurveFinished() ) {
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
                _imp->context->select(*it,RotoContext::OVERLAY_INTERACT);
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

bool
RotoGui::keyUp(double /*scaleX*/,
               double /*scaleY*/,
               QKeyEvent* e)
{
    bool didSomething = false;

    if ( !modCASIsControl(e) ) {
        if ( !_imp->iSelectingwithCtrlA && _imp->rotoData->showCpsBbox && (e->key() == Qt::Key_Control) ) {
            _imp->rotoData->transformMode = (_imp->rotoData->transformMode == TRANSLATE_AND_SCALE ?
                                             ROTATE_AND_SKEW : TRANSLATE_AND_SCALE);
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
            ///We have to reselect the bezier overlay hence put a reason different of OVERLAY_INTERACT
            SelectedBeziers::iterator found = std::find(rotoData->selectedBeziers.begin(),rotoData->selectedBeziers.end(),b);
            if ( found == rotoData->selectedBeziers.end() ) {
                rotoData->selectedBeziers.push_back(b);
                context->select(b, RotoContext::SETTINGS_PANEL);
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

    assert(item);
    bool changed = false;
    if (item) {
        _imp->onCurveLockedChangedRecursive(item, &changed);
    }

    if (changed) {
        _imp->viewer->redraw();
    }
}

void
RotoGui::onSelectionChanged(int reason)
{
    if ( (RotoContext::SelectionReason)reason != RotoContext::OVERLAY_INTERACT ) {
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
    _imp->context->select(_imp->rotoData->selectedBeziers,RotoContext::OVERLAY_INTERACT);
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
        _imp->context->select(curve, RotoContext::OVERLAY_INTERACT);
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

    menu.setFont( QFont(NATRON_FONT,NATRON_FONT_SIZE_11) );

    QAction* selectAllAction = new QAction(tr("Select All"),&menu);
    selectAllAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_A) );
    menu.addAction(selectAllAction);

    QAction* deleteCurve = new QAction(tr("Delete"),&menu);
    deleteCurve->setShortcut( QKeySequence(Qt::Key_Backspace) );
    menu.addAction(deleteCurve);

    QAction* openCloseCurve = new QAction(tr("Open/Close curve"),&menu);
    menu.addAction(openCloseCurve);

    QAction* smoothAction = new QAction(tr("Smooth points"),&menu);
    smoothAction->setShortcut( QKeySequence(Qt::Key_Z) );
    menu.addAction(smoothAction);

    QAction* cuspAction = new QAction(tr("Cusp points"),&menu);
    cuspAction->setShortcut( QKeySequence(Qt::SHIFT + Qt::Key_Z) );
    menu.addAction(cuspAction);

    QAction* removeFeather = new QAction(tr("Remove feather"),&menu);
    removeFeather->setShortcut( QKeySequence(Qt::SHIFT + Qt::Key_E) );
    menu.addAction(removeFeather);

    QAction* linkTo = new QAction(tr("Link to track..."),&menu);
    menu.addAction(linkTo);
    QAction* unLinkFrom = new QAction(tr("Unlink from track"),&menu);
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
    }
} // showMenuForCurve

void
RotoGui::smoothSelectedCurve()
{
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
    pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,false) );
    _imp->viewer->redraw();
}

void
RotoGui::cuspSelectedCurve()
{
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
    pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,true) );
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
RotoGui::showMenuForControlPoint(const boost::shared_ptr<Bezier> & curve,
                                 const std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > & cp)
{
    QPoint pos = QCursor::pos();
    QMenu menu(_imp->viewer);

    menu.setFont( QFont(NATRON_FONT,NATRON_FONT_SIZE_11) );

    QAction* deleteCp = new QAction(tr("Delete"),&menu);
    deleteCp->setShortcut( QKeySequence(Qt::Key_Backspace) );
    menu.addAction(deleteCp);

    QAction* smoothAction = new QAction(tr("Smooth points"),&menu);
    smoothAction->setShortcut( QKeySequence(Qt::Key_Z) );
    menu.addAction(smoothAction);

    QAction* cuspAction = new QAction(tr("Cusp points"),&menu);
    cuspAction->setShortcut( QKeySequence(Qt::SHIFT + Qt::Key_Z) );
    menu.addAction(cuspAction);

    QAction* removeFeather = new QAction(tr("Remove feather"),&menu);
    removeFeather->setShortcut( QKeySequence(Qt::SHIFT + Qt::Key_E) );
    menu.addAction(removeFeather);

    menu.addSeparator();

    boost::shared_ptr<Double_Knob> isSlaved = cp.first->isSlaved();
    QAction* linkTo = 0,*unLinkFrom = 0;
    if (!isSlaved) {
        linkTo = new QAction(tr("Link to track..."),&menu);
        menu.addAction(linkTo);
    } else {
        unLinkFrom = new QAction(tr("Unlink from track"),&menu);
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
        pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,false) );
        _imp->viewer->redraw();
    } else if (ret == cuspAction) {
        std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
        SmoothCuspUndoCommand::SmoothCuspCurveData data;
        data.curve = curve;
        data.newPoints.push_back(cp);
        datas.push_back(data);
        pushUndoCommand( new SmoothCuspUndoCommand(this,datas,time,true) );
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

