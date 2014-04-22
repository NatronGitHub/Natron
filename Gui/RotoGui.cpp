//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#include "RotoGui.h"

#include <QString>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>
#include <QAction>

#include "Engine/Node.h"
#include "Gui/NodeGui.h"
#include "Gui/Button.h"

struct RotoGui::RotoGuiPrivate
{
    NodeGui* node;
    Roto_Type type;
    
    QToolBar* toolbar;
    
    QWidget* selectionButtonsBar;
    QWidget* pointsEditionButtonsBar;
    QWidget* bezierEditionButtonsBar;
    
    QToolButton* selectTool;
    QToolButton* pointsEditionTool;
    QToolButton* bezierEditionTool;
    
    Roto_Tool selectedTool;
    QToolButton* selectedRole;
    
    RotoGuiPrivate(NodeGui* n)
    : node(n)
    , type(ROTOSCOPING)
    , toolbar(0)
    , selectionButtonsBar(0)
    , pointsEditionButtonsBar(0)
    , bezierEditionButtonsBar(0)
    , selectTool(0)
    , pointsEditionTool(0)
    , bezierEditionTool(0)
    {
        if (n->getNode()->isRotoPaintingNode()) {
            type = ROTOPAINTING;
        }
    }
};

void RotoGui::createToolAction(QToolButton* toolGroup,const QIcon& icon,const QString& text,RotoGui::Roto_Tool tool)
{
    
#pragma message WARN("Change constructor when icons will be added")
    QAction *action = new QAction(icon,text,toolGroup);
    action->setToolTip(text);
    
    QPoint data;
    data.setX((int)tool);
    if (toolGroup == _imp->selectTool) {
        data.setY((int)SELECTION_ROLE);
    } else if (toolGroup == _imp->pointsEditionTool) {
        data.setY((int)POINTS_EDITION_ROLE);
    } else if (toolGroup == _imp->bezierEditionTool) {
        data.setY(BEZIER_EDITION_ROLE);
    }
    action->setData(QVariant(data));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(onToolActionTriggered()));
    toolGroup->addAction(action);
}

RotoGui::RotoGui(NodeGui* node,QWidget* parent)
: _imp(new RotoGuiPrivate(node))
{
    assert(parent);
    
    _imp->toolbar = new QToolBar(parent);
    _imp->selectionButtonsBar = new QWidget(parent);
    _imp->pointsEditionButtonsBar = new QWidget(parent);
    _imp->bezierEditionButtonsBar = new QWidget(parent);
    
    _imp->selectTool = new QToolButton(_imp->toolbar);
    _imp->selectTool->setPopupMode(QToolButton::InstantPopup);
    _imp->selectTool->setText("Select");
    createToolAction(_imp->selectTool, QIcon(), "Select all", SELECT_ALL);
    createToolAction(_imp->selectTool, QIcon(), "Select points", SELECT_POINTS);
    createToolAction(_imp->selectTool, QIcon(), "Select curves", SELECT_CURVES);
    createToolAction(_imp->selectTool, QIcon(), "Select feather points", SELECT_FEATHER_POINTS);
    _imp->toolbar->addWidget(_imp->selectTool);
    
    _imp->pointsEditionTool = new QToolButton(_imp->toolbar);
    _imp->pointsEditionTool->setPopupMode(QToolButton::InstantPopup);
    _imp->pointsEditionTool->setText("Add points");
    createToolAction(_imp->pointsEditionTool, QIcon(), "Add points", ADD_POINTS);
    createToolAction(_imp->pointsEditionTool, QIcon(), "Remove points", REMOVE_POINTS);
    createToolAction(_imp->pointsEditionTool, QIcon(), "Cusp points", CUSP_POINTS);
    createToolAction(_imp->pointsEditionTool, QIcon(), "Smooth points", SMOOTH_POINTS);
    createToolAction(_imp->pointsEditionTool, QIcon(), "Open/Close curve", OPEN_CLOSE_CURVE);
    createToolAction(_imp->pointsEditionTool, QIcon(), "Remove feather", REMOVE_FEATHER_POINTS);
    _imp->toolbar->addWidget(_imp->pointsEditionTool);
    
    _imp->bezierEditionTool = new QToolButton(_imp->toolbar);
    _imp->bezierEditionTool->setPopupMode(QToolButton::InstantPopup);
    _imp->bezierEditionTool->setText("Bezier");
    createToolAction(_imp->bezierEditionTool, QIcon(), "Bezier", DRAW_BEZIER);
    createToolAction(_imp->bezierEditionTool, QIcon(), "B-Spline", DRAW_B_SPLINE);
    createToolAction(_imp->bezierEditionTool, QIcon(), "Ellipse", DRAW_ELLIPSE);
    createToolAction(_imp->bezierEditionTool, QIcon(), "Rectangle", DRAW_RECTANGLE);
    _imp->toolbar->addWidget(_imp->bezierEditionTool);
    
    _imp->selectedTool = SELECT_ALL;
    _imp->selectedRole = _imp->selectTool;
}

RotoGui::~RotoGui()
{
    
}

QWidget* RotoGui::getButtonsBar(RotoGui::Roto_Role role) const
{
    switch (role) {
        case SELECTION_ROLE:
            return _imp->selectionButtonsBar;
            break;
        case POINTS_EDITION_ROLE:
            return _imp->pointsEditionButtonsBar;
            break;
        case BEZIER_EDITION_ROLE:
            return _imp->bezierEditionButtonsBar;
            break;
        default:
            assert(false);
            break;
    }
}

QWidget* RotoGui::getCurrentButtonsBar() const
{
    return getButtonsBar(getCurrentRole());
}

RotoGui::Roto_Tool RotoGui::getSelectedTool() const
{
    return _imp->selectedTool;
}

QToolBar* RotoGui::getToolBar() const
{
    return _imp->toolbar;
}

void RotoGui::onToolActionTriggered()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act) {
        QPoint data = act->data().toPoint();
        _imp->selectedTool = (Roto_Tool)data.x();
        Roto_Role actionRole = (Roto_Role)data.y();
        QToolButton* toolButton = 0;
        
        Roto_Role previousRole = getCurrentRole();
        
        switch (actionRole) {
            case SELECTION_ROLE:
                toolButton = _imp->selectTool;
                emit roleChanged((int)previousRole,(int)SELECTION_ROLE);
                break;
            case POINTS_EDITION_ROLE:
                toolButton = _imp->pointsEditionTool;
                emit roleChanged((int)previousRole,(int)POINTS_EDITION_ROLE);
                break;
            case BEZIER_EDITION_ROLE:
                toolButton = _imp->bezierEditionTool;
                emit roleChanged((int)previousRole,(int)BEZIER_EDITION_ROLE);
                break;
            default:
                assert(false);
                break;
        }
        
        
        assert(_imp->selectedRole);
        if (_imp->selectedRole != toolButton) {
            _imp->selectedRole->setDown(false);
        }
        
        
        assert(toolButton);
        toolButton->setDown(true);
        toolButton->setIcon(act->icon());
        toolButton->setToolTip(act->text());
        
#pragma message WARN("Remove when icons will be added")
        toolButton->setText(act->text());
        _imp->selectedRole = toolButton;
    }
}

RotoGui::Roto_Role RotoGui::getCurrentRole() const
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

void RotoGui::drawOverlays(double scaleX,double scaleY) const
{
    
}

bool RotoGui::penDown(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos)
{
    
}

bool RotoGui::penMotion(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos)
{
    
}

bool RotoGui::penUp(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos)
{
    
}

bool RotoGui::keyDown(double scaleX,double scaleY,QKeyEvent* e)
{
    
}

bool RotoGui::keyUp(double scaleX,double scaleY,QKeyEvent* e)
{
    
}