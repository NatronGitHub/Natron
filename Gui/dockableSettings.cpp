//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 




#include "Gui/dockableSettings.h"

#include <QLayout>
#include <QTabWidget>

#include "Gui/node_ui.h"
#include "Gui/knob_callback.h"
#include "Core/node.h"
#include "Gui/knob.h"
#include "Gui/lineEdit.h"
#include "Superviser/powiterFn.h"
#include "Gui/button.h"
#include "Gui/DAG.h"

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
//    setStyleSheet();
    _headerWidget->setFrameShape(QFrame::Box);
    
    _headerLayout = new QHBoxLayout(_headerWidget);
    _headerLayout->setContentsMargins(0, 0, 0, 0);
    _headerLayout->setSpacing(0);
    _headerWidget->setLayout(_headerLayout);
    
    
    QImage imgM(IMAGES_PATH"minimize.png");
    QPixmap pixM=QPixmap::fromImage(imgM);
    pixM.scaled(15,15);
    QImage imgC(IMAGES_PATH"close.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC.scaled(15,15);
    _minimize=new Button(QIcon(pixM),"",_headerWidget);
    _minimize->setFixedSize(15,15);
    _minimize->setCheckable(true);
    QObject::connect(_minimize,SIGNAL(toggled(bool)),this,SLOT(minimizeOrMaximize(bool)));
    _cross=new Button(QIcon(pixC),"",_headerWidget);
    _cross->setFixedSize(15,15);
    QObject::connect(_cross,SIGNAL(clicked()),this,SLOT(close()));
   
    
    _nodeName = new LineEdit(_headerWidget);
    _nodeName->setText(_nodeGUI->getNode()->getName().c_str());
    QObject::connect(_nodeName,SIGNAL(textChanged(QString)),_nodeGUI,SLOT(setName(QString)));
    _headerLayout->addWidget(_nodeName);
    _headerLayout->addStretch();
    _headerLayout->addWidget(_minimize);
    _headerLayout->addWidget(_cross);
   
    _mainLayout->addWidget(_headerWidget);

    _tabWidget = new QTabWidget(this);
    _mainLayout->addWidget(_tabWidget);
    
    
    
    _settingsTab = new QWidget(_tabWidget);
   // _settingsTab->setStyleSheet("background-color : rgb(50,50,50); color:(200,200,200);");
    _layoutSettings=new QVBoxLayout(_settingsTab);
    _layoutSettings->setSpacing(0);
    _settingsTab->setLayout(_layoutSettings);
    _tabWidget->addTab(_settingsTab,"Settings");

    
    _labelWidget=new QWidget(_tabWidget);
    _layoutLabel=new QVBoxLayout(_labelWidget);
    _labelWidget->setLayout(_layoutLabel);
    _tabWidget->addTab(_labelWidget,"Label");
    
    
    initialize_knobs();
    
}
SettingsPanel::~SettingsPanel(){}

void SettingsPanel::initialize_knobs(){
    
    _cb=_nodeGUI->getNode()->getKnobCallBack();
    _cb->setSettingsPanel(this);
    _nodeGUI->getNode()->initKnobs(_cb);
    const std::vector<Knob*>& knobs=_nodeGUI->getNode()->getKnobs();
    foreach(Knob* k,knobs){
        _layoutSettings->addWidget(k);
    }
    
}
void SettingsPanel::addKnobDynamically(Knob* knob){
	_layoutSettings->addWidget(knob);
}
void SettingsPanel::removeAndDeleteKnob(Knob* knob){
    _layoutSettings->removeWidget(knob);
    delete knob;
}
void SettingsPanel::close(){
    
    _nodeGUI->setSettingsPanelEnabled(false);
    setVisible(false);

    QVBoxLayout* container = _nodeGUI->getDockContainer();
    vector<QWidget*> _panels;
    for(int i =0 ; i < container->count(); i++){
        if (QWidget *myItem = dynamic_cast <QWidget*>(container->itemAt(i))){
            _panels.push_back(myItem);
            container->removeWidget(myItem);
        }
    }
    for (U32 i =0 ; i < _panels.size(); i++) {
        container->addWidget(_panels[i]);
    }
 
    update();
}
void SettingsPanel::minimizeOrMaximize(bool toggled){
    _minimized=toggled;
    if(_minimized){
        
        QImage imgM(IMAGES_PATH"maximize.png");
        QPixmap pixM=QPixmap::fromImage(imgM);
        pixM.scaled(15,15);
        _minimize->setIcon(QIcon(pixM));
        _tabWidget->setVisible(false);
        
        QVBoxLayout* container = _nodeGUI->getDockContainer();
        vector<QWidget*> _panels;
        for(int i =0 ; i < container->count(); i++){
            if (QWidget *myItem = dynamic_cast <QWidget*>(container->itemAt(i))){
                _panels.push_back(myItem);
                container->removeWidget(myItem);
            }
        }
        for (U32 i =0 ; i < _panels.size(); i++) {
            container->addWidget(_panels[i]);
        }
    
    }else{
        QImage imgM(IMAGES_PATH"minimize.png");
        QPixmap pixM=QPixmap::fromImage(imgM);
        pixM.scaled(15,15);
        _minimize->setIcon(QIcon(pixM));
        _tabWidget->setVisible(true);
        
        QVBoxLayout* container = _nodeGUI->getDockContainer();
        vector<QWidget*> _panels;
        for(int i =0 ; i < container->count(); i++){
            if (QWidget *myItem = dynamic_cast <QWidget*>(container->itemAt(i))){
                _panels.push_back(myItem);
                container->removeWidget(myItem);
            }
        }
        for (U32 i =0 ; i < _panels.size(); i++) {
            container->addWidget(_panels[i]);
        }        
    }
    update();
}

void SettingsPanel::paintEvent(QPaintEvent * event){
    if(_nodeGUI->isSelected()){
        QPalette* palette = new QPalette();
        palette->setColor(QPalette::Foreground,Qt::yellow);
        setPalette(*palette);
    }else{
        QPalette* palette = new QPalette();
        palette->setColor(QPalette::Foreground,Qt::black);
        setPalette(*palette);
    }
    QFrame::paintEvent(event);
}

void SettingsPanel::mousePressEvent(QMouseEvent* e){
    _nodeGUI->getDagGui()->selectNode(_nodeGUI);
    QFrame::mousePressEvent(e);
}
