//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "SettingsPanel.h"

#include <iostream>
#include <QLayout>
#include <QAction>
#include <QTabWidget>
#include <QStyle>

#include "Global/Macros.h"

#include "Engine/Node.h"


#include "Gui/NodeGui.h"
#include "Gui/KnobGui.h"
#include "Gui/LineEdit.h"
#include "Gui/Button.h"
#include "Gui/NodeGraph.h"

using namespace std;
SettingsPanel::SettingsPanel(NodeGui* NodeUi ,QWidget *parent):QFrame(parent),_nodeGUI(NodeUi),_minimized(false){
    
    _mainLayout=new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(_mainLayout);
    setObjectName(_nodeGUI->getNode()->getName().c_str());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    setFrameShape(QFrame::Box);
    
    _headerWidget = new QFrame(this);
    _headerWidget->setFrameShape(QFrame::Box);
    
    _headerLayout = new QHBoxLayout(_headerWidget);
    _headerLayout->setContentsMargins(0, 0, 0, 0);
    _headerLayout->setSpacing(0);
    _headerWidget->setLayout(_headerLayout);
    
    
    QImage imgM(POWITER_IMAGES_PATH"minimize.png");
    QPixmap pixM=QPixmap::fromImage(imgM);
    pixM = pixM.scaled(15,15);
    QImage imgC(POWITER_IMAGES_PATH"close.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC = pixC.scaled(15,15);
    _minimize=new Button(QIcon(pixM),"",_headerWidget);
    _minimize->setFixedSize(15,15);
    _minimize->setCheckable(true);
    QObject::connect(_minimize,SIGNAL(toggled(bool)),this,SLOT(minimizeOrMaximize(bool)));
    _cross=new Button(QIcon(pixC),"",_headerWidget);
    _cross->setFixedSize(15,15);
    QObject::connect(_cross,SIGNAL(clicked()),this,SLOT(close()));
    
    QImage imgUndo(POWITER_IMAGES_PATH"undo.png");
    QPixmap pixUndo = QPixmap::fromImage(imgUndo);
    pixUndo = pixUndo.scaled(15, 15);
    QImage imgUndo_gray(POWITER_IMAGES_PATH"undo_grayscale.png");
    QPixmap pixUndo_gray = QPixmap::fromImage(imgUndo_gray);
    pixUndo_gray = pixUndo_gray.scaled(15, 15);
    QIcon icUndo;
    icUndo.addPixmap(pixUndo,QIcon::Normal);
    icUndo.addPixmap(pixUndo_gray,QIcon::Disabled);
    _undoButton = new Button(icUndo,"",_headerWidget);
    _undoButton->setToolTip("Undo the last change made to this operator");
    _undoButton->setEnabled(false);
    
    QImage imgRedo(POWITER_IMAGES_PATH"redo.png");
    QPixmap pixRedo = QPixmap::fromImage(imgRedo);
    pixRedo = pixRedo.scaled(15, 15);
    QImage imgRedo_gray(POWITER_IMAGES_PATH"redo_grayscale.png");
    QPixmap pixRedo_gray = QPixmap::fromImage(imgRedo_gray);
    pixRedo_gray = pixRedo_gray.scaled(15, 15);
    QIcon icRedo;
    icRedo.addPixmap(pixRedo,QIcon::Normal);
    icRedo.addPixmap(pixRedo_gray,QIcon::Disabled);
    _redoButton = new Button(icRedo,"",_headerWidget);
    _redoButton->setToolTip("Redo the last change undone to this operator");
    _redoButton->setEnabled(false);
    
    QObject::connect(_undoButton, SIGNAL(pressed()),_nodeGUI, SLOT(undoCommand()));
    QObject::connect(_redoButton, SIGNAL(pressed()),_nodeGUI, SLOT(redoCommand()));
    
    _nodeName = new LineEdit(_headerWidget);
    _nodeName->setText(_nodeGUI->getNode()->getName().c_str());
    QObject::connect(_nodeName,SIGNAL(textEdited(QString)),_nodeGUI,SIGNAL(nameChanged(QString)));
    _headerLayout->addWidget(_nodeName);
    _headerLayout->addStretch();
    _headerLayout->addWidget(_undoButton);
    _headerLayout->addWidget(_redoButton);
    _headerLayout->addStretch();
    _headerLayout->addWidget(_minimize);
    _headerLayout->addWidget(_cross);
   
    _mainLayout->addWidget(_headerWidget);

    _tabWidget = new QTabWidget(this);
    _tabWidget->setObjectName("_tabWidget");
    _mainLayout->addWidget(_tabWidget);
    
    _settingsTab = new QWidget(_tabWidget);
    _settingsTab->setObjectName("_settingsTab");
    _layoutSettings=new QGridLayout(_settingsTab);
    _layoutSettings->setSpacing(0);
    _settingsTab->setLayout(_layoutSettings);
    _tabWidget->addTab(_settingsTab,"Settings");
    
    _labelWidget=new QWidget(_tabWidget);
    _layoutLabel=new QVBoxLayout(_labelWidget);
    _labelWidget->setLayout(_layoutLabel);
    _tabWidget->addTab(_labelWidget,"Label");
    
    
    //initialize_knobs();
    
}
SettingsPanel::~SettingsPanel(){}

void SettingsPanel::setEnabledUndoButton(bool b){
    _undoButton->setEnabled(b);
}
void SettingsPanel::setEnabledRedoButton(bool b){
    _redoButton->setEnabled(b);
}

void SettingsPanel::initialize_knobs(){
    int maxSpacing = 0;
    /*We now have all knobs in a vector, just add them to the layout.*/
    const std::vector<Knob*>& knobs = _nodeGUI->getNode()->getKnobs();
    for(U32 i = 0 ; i < knobs.size(); ++i){
        KnobGui* gui = _nodeGUI->findKnobGuiOrCreate(knobs[i]);
        if(!gui){
            return;
        }
        /*If the GUI doesn't already exist, we create it.*/
        if(!gui->hasWidgetBeenCreated()){
            Knob* parentKnob = knobs[i]->getParentKnob();
            if(knobs[i]->name() == "Tab"){
                /*create all children*/
                Tab_Knob* tabKnob = dynamic_cast<Tab_Knob*>(knobs[i]);
                std::map<std::string,std::vector<KnobGui*> > childrenGui;
                const std::map<std::string,std::vector<Knob*> >& children = tabKnob->getKnobs();
                for (std::map<std::string,std::vector<Knob*> >::const_iterator it = children.begin();
                     it!=children.end();++it) {
                    std::vector<KnobGui*> tabKnobs;
                    for (U32 j = 0; j < it->second.size(); ++j) {
                        KnobGui* child = _nodeGUI->findKnobGuiOrCreate(it->second[j]);
                        if(child)
                            tabKnobs.push_back(child);
                    }
                    childrenGui.insert(make_pair(it->first, tabKnobs));
                }
                dynamic_cast<Tab_KnobGui*>(gui)->addKnobs(childrenGui);
            }else if(parentKnob && parentKnob->name() == "Tab"){
                /*the tab widget already created all children*/
                continue;
            }else{
                gui->setParent(_settingsTab);
                int row = i;
                int offsetColumn = knobs[i]->determineHierarchySize();
                if(!gui->triggerNewLine() && i!=0) --i;
                gui->createGUI(_layoutSettings,i,knobs[i]->determineHierarchySize());
                if(parentKnob){
                    Group_KnobGui* parentGui = dynamic_cast<Group_KnobGui*>(_nodeGUI->findKnobGuiOrCreate(parentKnob));
                    parentGui->addKnob(gui,row,offsetColumn);
                    gui->setVisible(parentGui->isChecked());
                }
            }
        }
        int knobSpacing  =  gui->getSpacingBetweenItems();
        if(knobSpacing > maxSpacing)
            maxSpacing = knobSpacing;
    }
    _layoutSettings->setHorizontalSpacing(maxSpacing);
}
void SettingsPanel::addKnobDynamically(KnobGui* knob){
	_layoutSettings->addWidget(knob);
}
void SettingsPanel::removeAndDeleteKnob(KnobGui* knob){
    _layoutSettings->removeWidget(knob);
    delete knob;
}
void SettingsPanel::close(){
    
    _nodeGUI->setSettingsPanelEnabled(false);
    setVisible(false);

    QVBoxLayout* container = _nodeGUI->getDockContainer();
    vector<QWidget*> _panels;
    for(int i =0 ; i < container->count(); ++i) {
        if (QWidget *myItem = dynamic_cast <QWidget*>(container->itemAt(i))){
            _panels.push_back(myItem);
            container->removeWidget(myItem);
        }
    }
    for (U32 i =0 ; i < _panels.size(); ++i) {
        container->addWidget(_panels[i]);
    }
 
    update();
}
void SettingsPanel::minimizeOrMaximize(bool toggled){
    _minimized=toggled;
    if(_minimized){
        
        QImage imgM(POWITER_IMAGES_PATH"maximize.png");
        QPixmap pixM=QPixmap::fromImage(imgM);
        pixM.scaled(15,15);
        _minimize->setIcon(QIcon(pixM));
        _tabWidget->setVisible(false);
        
        QVBoxLayout* container = _nodeGUI->getDockContainer();
        vector<QWidget*> _panels;
        for(int i =0 ; i < container->count(); ++i) {
            if (QWidget *myItem = dynamic_cast <QWidget*>(container->itemAt(i))){
                _panels.push_back(myItem);
                container->removeWidget(myItem);
            }
        }
        for (U32 i =0 ; i < _panels.size(); ++i) {
            container->addWidget(_panels[i]);
        }
    
    }else{
        QImage imgM(POWITER_IMAGES_PATH"minimize.png");
        QPixmap pixM=QPixmap::fromImage(imgM);
        pixM.scaled(15,15);
        _minimize->setIcon(QIcon(pixM));
        _tabWidget->setVisible(true);
        
        QVBoxLayout* container = _nodeGUI->getDockContainer();
        vector<QWidget*> _panels;
        for(int i =0 ; i < container->count(); ++i) {
            if (QWidget *myItem = dynamic_cast <QWidget*>(container->itemAt(i))){
                _panels.push_back(myItem);
                container->removeWidget(myItem);
            }
        }
        for (U32 i =0 ; i < _panels.size(); ++i) {
            container->addWidget(_panels[i]);
        }        
    }
    update();
}

void SettingsPanel::paintEvent(QPaintEvent * event){

    QFrame::paintEvent(event);
}

void SettingsPanel::mousePressEvent(QMouseEvent* e){
    _nodeGUI->getDagGui()->selectNode(_nodeGUI);
    QFrame::mousePressEvent(e);
}
void SettingsPanel::setSelected(bool s){
    _selected = s;
    style()->unpolish(this);
    style()->polish(this);
}
void SettingsPanel::setNodeName(const QString& name){
    _nodeName->setText(name);
}
