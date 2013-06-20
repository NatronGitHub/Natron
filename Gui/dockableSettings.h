//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H


#include <QtWidgets/QFrame>

class LineEdit;
class Node;
class Knob;
class NodeGui;
class Knob_Callback;
class QVBoxLayout;
class QTabWidget;
class QPushButton;
class QHBoxLayout;
#define MINIMUM_WIDTH 340

/*Represent 1 node settings panel in the right dock.
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
    
    /*Pointer to the node GUI*/
    NodeGui* _nodeGUI;
    
    /*Pointer to the knob_callback*/
    Knob_Callback *_cb;

    /*Header related*/
    QFrame* _headerWidget;
    QHBoxLayout *_headerLayout;
    LineEdit* _nodeName;
    QPushButton* _minimize;
    QPushButton* _cross;
    
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

public:

    SettingsPanel(NodeGui* NodeUi, QWidget *parent=0);
    virtual ~SettingsPanel();
	void addKnobDynamically(Knob* knob);
    void removeAndDeleteKnob(Knob* knob);
    
    virtual void paintEvent(QPaintEvent * event);
public slots:
    void close();
    void minimizeOrMaximize(bool toggled);

private:
    

    void initialize_knobs();
	
};

#endif // SETTINGSPANEL_H
