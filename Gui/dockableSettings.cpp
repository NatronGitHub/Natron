//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "Gui/dockableSettings.h"
#include "Gui/node_ui.h"
#include "Gui/knob_callback.h"
#include "Core/node.h"
#include "Gui/knob.h"
#include "Superviser/powiterFn.h"
#include <QtWidgets/QtWidgets>


SettingsPanel::SettingsPanel(NodeGui* NodeUi ,QWidget *parent):QFrame(parent),_nodeGUI(NodeUi),_minimized(false)
{
    
    _mainLayout=new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(_mainLayout);
    setObjectName((_nodeGUI->getNode()->getName()));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    
    setMinimumWidth(MINIMUM_WIDTH);
    setFrameShape(QFrame::Box);
    
    _headerWidget = new QFrame(this);
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
    _minimize=new QPushButton(QIcon(pixM),"",_headerWidget);
    _minimize->setFixedSize(15,15);
    _minimize->setCheckable(true);
    QObject::connect(_minimize,SIGNAL(toggled(bool)),this,SLOT(minimizeOrMaximize(bool)));
    _cross=new QPushButton(QIcon(pixC),"",_headerWidget);
    _cross->setFixedSize(15,15);
    QObject::connect(_cross,SIGNAL(clicked()),this,SLOT(close()));
   
    
    _nodeName = new QLineEdit(_headerWidget);
    _nodeName->setText(_nodeGUI->getNode()->getName());
    QObject::connect(_nodeName,SIGNAL(textChanged(QString)),_nodeGUI,SLOT(setName(QString)));
    _headerLayout->addWidget(_nodeName);
    _headerLayout->insertSpacing(1,40);
    _headerLayout->addWidget(_minimize);
    _headerLayout->addWidget(_cross);
   
    _mainLayout->addWidget(_headerWidget);

    _tabWidget = new QTabWidget(this);
    _mainLayout->addWidget(_tabWidget);
    
    
    
    _settingsTab = new QWidget(_tabWidget);
    _layoutSettings=new QVBoxLayout(_settingsTab);
    _settingsTab->setLayout(_layoutSettings);
    _tabWidget->addTab(_settingsTab,"Settings");

    
    _labelWidget=new QWidget(_tabWidget);
    _layoutLabel=new QVBoxLayout(_labelWidget);
    _labelWidget->setLayout(_layoutLabel);
    _tabWidget->addTab(_labelWidget,"Label");
    
    
    initialize_knobs();
    setVisible(true);
    
}
SettingsPanel::~SettingsPanel(){delete _cb;}

void SettingsPanel::initialize_knobs(){
    
    _cb=new Knob_Callback(this,_nodeGUI->getNode());
    _nodeGUI->getNode()->initKnobs(_cb);
    std::vector<Knob*> knobs=_nodeGUI->getNode()->getKnobs();
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
    QWidget* pr=_nodeGUI->getSettingsLayout()->parentWidget();
    pr->setMinimumSize(_nodeGUI->getSettingsLayout()->sizeHint());
    setVisible(false);
    
}
void SettingsPanel::minimizeOrMaximize(bool toggled){
    _minimized=toggled;
    if(_minimized){
        
        QImage imgM(IMAGES_PATH"maximize.png");
        QPixmap pixM=QPixmap::fromImage(imgM);
        pixM.scaled(15,15);
        _minimize->setIcon(QIcon(pixM));
        _tabWidget->setVisible(false);
        
        
        QWidget* pr=_nodeGUI->getSettingsLayout()->parentWidget();
        pr->setMinimumSize(_nodeGUI->getSettingsLayout()->sizeHint());
        
        
    }else{
        QImage imgM(IMAGES_PATH"minimize.png");
        QPixmap pixM=QPixmap::fromImage(imgM);
        pixM.scaled(15,15);
        _minimize->setIcon(QIcon(pixM));
        _tabWidget->setVisible(true);
        
        QWidget* pr=_nodeGUI->getSettingsLayout()->parentWidget();
        pr->setMinimumSize(_nodeGUI->getSettingsLayout()->sizeHint());
        
        
    }
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