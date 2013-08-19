//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H


#include <QFrame>

class LineEdit;
class Node;
class Knob;
class NodeGui;
class KnobCallback;
class QVBoxLayout;
class QTabWidget;
class Button;
class QHBoxLayout;

/*Represents 1 node settings panel in the right dock.
 A panel is structured as following :
 QFrame
    | QFrame with horizontal layout -> header widget (for buttons)
    | QTabWidget : the different tabs (node, label, etc...)
            - Node tab:
                | QWidget with vertical layout 
                    | Knobs... 
            - Label tab ...
*/
class SettingsPanel:public QFrame
{
    Q_OBJECT
    
    Q_PROPERTY( bool _selected READ isSelected WRITE setSelected)
    
    /*Pointer to the node GUI*/
    NodeGui* _nodeGUI;
    
    /*Pointer to the knob_callback*/
    KnobCallback *_cb;

    /*Header related*/
    QFrame* _headerWidget;
    QHBoxLayout *_headerLayout;
    LineEdit* _nodeName;
    Button* _minimize;
    Button* _cross;
    
    /*Tab related*/
    QTabWidget* _tabWidget;
    
    /*settings related*/
    QWidget* _settingsTab;
    QVBoxLayout* _layoutSettings;
    
    /*labels related*/
    QVBoxLayout* _layoutLabel;
    QWidget* _labelWidget;
    
    /*global layout*/
    QVBoxLayout* _mainLayout;
    
    /*true if the panel is minimized*/
    bool _minimized;
    
    bool _selected;

public:

    SettingsPanel(NodeGui* NodeUi, QWidget *parent=0);
    
    virtual ~SettingsPanel();
    
    void setSelected(bool s);
    
    bool isSelected() const {return _selected;}
    
	void addKnobDynamically(Knob* knob);
    
    void removeAndDeleteKnob(Knob* knob);
    
    bool isMinimized() const {return _minimized;}
    
    virtual void mousePressEvent(QMouseEvent* e);
    
    virtual void paintEvent(QPaintEvent * event);
public slots:
    void close();
    void minimizeOrMaximize(bool toggled);

private:
    

    void initialize_knobs();
	
};

#endif // SETTINGSPANEL_H
