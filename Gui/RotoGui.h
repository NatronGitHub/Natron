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

#ifndef ROTOGUI_H
#define ROTOGUI_H

#include <boost/scoped_ptr.hpp>
#include <QObject>

class QToolBar;
class QWidget;
class QIcon;
class QString;
class QToolButton;
class QKeyEvent;
class QPointF;

class NodeGui;

class RotoGui : public QObject
{
    Q_OBJECT
    
public:
    
    enum Roto_Type {
        ROTOSCOPING = 0,
        ROTOPAINTING
    };
    
    enum Roto_Role {
        SELECTION_ROLE = 0,
        POINTS_EDITION_ROLE,
        BEZIER_EDITION_ROLE
    };
    
    enum Roto_Tool {
        SELECT_ALL = 0,
        SELECT_POINTS,
        SELECT_CURVES,
        SELECT_FEATHER_POINTS,
        
        ADD_POINTS,
        REMOVE_POINTS,
        REMOVE_FEATHER_POINTS,
        OPEN_CLOSE_CURVE,
        SMOOTH_POINTS,
        CUSP_POINTS,
        
        DRAW_BEZIER,
        DRAW_B_SPLINE,
        DRAW_ELLIPSE,
        DRAW_RECTANGLE,
        
    };
    
    RotoGui(NodeGui* node,QWidget* parent);
    
    ~RotoGui();
    
    /**
     * @brief Return the horizontal buttons bar for the given role
     **/
    QWidget* getButtonsBar(RotoGui::Roto_Role role) const;
    
    /**
     * @brief Same as getButtonsBar(getCurrentRole())
     **/
    QWidget* getCurrentButtonsBar() const;
    
    /**
     * @brief The currently used tool
     **/
    RotoGui::Roto_Tool getSelectedTool() const;
    
    QToolBar* getToolBar() const;
    
    /**
     * @brief The selected role (selection,draw,add points, etc...)
     **/
    RotoGui::Roto_Role getCurrentRole() const;
    
    void drawOverlays(double scaleX,double scaleY) const;
    
    bool penDown(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos);
    
    bool penMotion(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos);
    
    bool penUp(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos);
    
    bool keyDown(double scaleX,double scaleY,QKeyEvent* e);
    
    bool keyUp(double scaleX,double scaleY,QKeyEvent* e);
    

    
signals:
    
    /**
      * @brief Emitted when the selected role changes
     **/
    void roleChanged(int previousRole,int newRole);
    
public slots:
    
    void onToolActionTriggered();
    
private:
    
    void createToolAction(QToolButton* toolGroup,const QIcon& icon,const QString& text,RotoGui::Roto_Tool tool);
    
    struct RotoGuiPrivate;
    boost::scoped_ptr<RotoGuiPrivate> _imp;
    
};

#endif // ROTOGUI_H
