//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "DockablePanel.h"

#include <iostream>
#include <QLayout>
#include <QAction>
#include <QTabWidget>
#include <QStyle>
#include <QUndoStack>
#include <QUndoCommand>
#include <QToolTip>

#include "Global/AppManager.h"

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Knob.h"
#include "Engine/EffectInstance.h"

#include "Gui/NodeGui.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiTypes.h" // for Group_KnobGui
#include "Gui/KnobGuiFactory.h"
#include "Gui/LineEdit.h"
#include "Gui/Button.h"
#include "Gui/NodeGraph.h"

using std::make_pair;

DockablePanel::DockablePanel(KnobHolder* holder
                             ,QVBoxLayout* container
                             ,bool readOnlyName
                             ,const QString& initialName
                             ,const QString& helpToolTip
                             ,const QString& defaultTab
                             ,QWidget *parent)
:QFrame(parent)
,_container(container)
,_mainLayout(NULL)
,_headerWidget(NULL)
,_headerLayout(NULL)
,_nameLineEdit(NULL)
,_nameLabel(NULL)
,_tabWidget(NULL)
,_helpButton(NULL)
,_minimize(NULL)
,_cross(NULL)
,_undoButton(NULL)
,_redoButton(NULL)
,_minimized(false)
,_undoStack(new QUndoStack)
,_knobs()
,_holder(holder)
,_tabs()
{
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(_mainLayout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFrameShape(QFrame::Box);

    _headerWidget = new QFrame(this);
    _headerWidget->setFrameShape(QFrame::Box);
    _headerLayout = new QHBoxLayout(_headerWidget);
    _headerLayout->setContentsMargins(0, 0, 0, 0);
    _headerLayout->setSpacing(0);
    _headerWidget->setLayout(_headerLayout);
    

    QPixmap pixHelp ;
    appPTR->getIcon(Natron::HELP_WIDGET_PIXMAP,&pixHelp);
    _helpButton = new Button(QIcon(pixHelp),"",_headerWidget);
    
    QString tooltip = helpToolTip;
    for (int i = 0; i < tooltip.size();++i) {
        if (i%100 == 0 && i!=0) {
            /*Find closest word end and insert a new line*/
            while(i < tooltip.size() && tooltip.at(i)!=QChar(' ')) ++i;
            tooltip.insert(i, QChar('\n'));
        }
    }
    _helpButton->setToolTip(tooltip);
    _helpButton->setFixedSize(15, 15);
    QObject::connect(_helpButton, SIGNAL(clicked()), this, SLOT(showHelp()));
    
    
    QPixmap pixM;
    appPTR->getIcon(Natron::MINIMIZE_WIDGET_PIXMAP,&pixM);

    QPixmap pixC;
    appPTR->getIcon(Natron::CLOSE_WIDGET_PIXMAP,&pixC);
    _minimize=new Button(QIcon(pixM),"",_headerWidget);
    _minimize->setFixedSize(15,15);
    _minimize->setCheckable(true);
    QObject::connect(_minimize,SIGNAL(toggled(bool)),this,SLOT(minimizeOrMaximize(bool)));
    _cross=new Button(QIcon(pixC),"",_headerWidget);
    _cross->setFixedSize(15,15);
    QObject::connect(_cross,SIGNAL(clicked()),this,SLOT(close()));


    QPixmap pixUndo ;
    appPTR->getIcon(Natron::UNDO_PIXMAP,&pixUndo);
    QPixmap pixUndo_gray ;
    appPTR->getIcon(Natron::UNDO_GRAYSCALE_PIXMAP,&pixUndo_gray);
    QIcon icUndo;
    icUndo.addPixmap(pixUndo,QIcon::Normal);
    icUndo.addPixmap(pixUndo_gray,QIcon::Disabled);
    _undoButton = new Button(icUndo,"",_headerWidget);
    _undoButton->setToolTip("Undo the last change made to this operator");
    _undoButton->setEnabled(false);
    
    QPixmap pixRedo ;
    appPTR->getIcon(Natron::REDO_PIXMAP,&pixRedo);
    QPixmap pixRedo_gray;
    appPTR->getIcon(Natron::REDO_GRAYSCALE_PIXMAP,&pixRedo_gray);
    QIcon icRedo;
    icRedo.addPixmap(pixRedo,QIcon::Normal);
    icRedo.addPixmap(pixRedo_gray,QIcon::Disabled);
    _redoButton = new Button(icRedo,"",_headerWidget);
    _redoButton->setToolTip("Redo the last change undone to this operator");
    _redoButton->setEnabled(false);
    
    
    QObject::connect(_undoButton, SIGNAL(pressed()),this, SLOT(onUndoPressed()));
    QObject::connect(_redoButton, SIGNAL(pressed()),this, SLOT(onRedoPressed()));
    
    if(!readOnlyName){
        _nameLineEdit = new LineEdit(_headerWidget);
        _nameLineEdit->setText(initialName);
        QObject::connect(_nameLineEdit,SIGNAL(textEdited(QString)),this,SIGNAL(nameChanged(QString)));
        _headerLayout->addWidget(_nameLineEdit);
    }else{
        _nameLabel = new QLabel(initialName,_headerWidget);
        _headerLayout->addWidget(_nameLabel);
    }
    
    _headerLayout->addStretch();
    
    _headerLayout->addWidget(_undoButton);
    _headerLayout->addWidget(_redoButton);
    
    _headerLayout->addStretch();
    _headerLayout->addWidget(_helpButton);
    _headerLayout->addWidget(_minimize);
    _headerLayout->addWidget(_cross);

    _mainLayout->addWidget(_headerWidget);

    _tabWidget = new QTabWidget(this);
    _mainLayout->addWidget(_tabWidget);
    
    
    addTab(defaultTab);
}

DockablePanel::~DockablePanel(){
    delete _undoStack;
    //removing parentship otherwise Qt will attempt to delete knobs a second time
    for(std::map<Knob*,KnobGui*>::const_iterator it = _knobs.begin();it!=_knobs.end();++it){
        it->second->setParent(NULL);
        QObject::disconnect(it->second,SIGNAL(deleted(KnobGui*)),this,SLOT(onKnobDeletion(KnobGui*)));

    }
}

void DockablePanel::initializeKnobs(){
    /*We now have all knobs in a vector, just add them to the layout.*/
    const std::vector<boost::shared_ptr<Knob> >& knobs = _holder->getKnobs();
    for(U32 i = 0 ; i < knobs.size(); ++i){
         if(knobs[i]->typeName() == "Tab"){
             bool found = false;
             QString tabName(knobs[i]->getDescription().c_str());
             for(int j = 0 ; j < _tabWidget->count(); ++j){
                 if(_tabWidget->tabText(j) == tabName){
                     found = true;
                     break;
                 }
             }
             if(!found)
                 addTab(tabName);
         }else{
             KnobGui* gui = findKnobGuiOrCreate(knobs[i].get());
             if(!gui){
                 return;
             }
             if(!gui->hasWidgetBeenCreated()){
                 /*Defaults to the first tab*/
                 int offsetColumn = knobs[i]->determineHierarchySize();
                 std::map<QString,std::pair<QWidget*,int> >::iterator parentTab = _tabs.begin();
                 Knob* parentKnob = knobs[i]->getParentKnob();
                 if(parentKnob && parentKnob->typeName() == "Tab"){
                     std::map<QString,std::pair<QWidget*,int> >::iterator it = _tabs.find(parentKnob->getDescription().c_str());
                     if(it!=_tabs.end()){
                         parentTab = it;
                     }
                 }
                 ++parentTab->second.second;
                 
                 gui->setParent(parentTab->second.first);
                 if(!gui->triggerNewLine() && i!=0) --parentTab->second.second;
                 gui->createGUI(dynamic_cast<QGridLayout*>(parentTab->second.first->layout()),parentTab->second.second);
                 if(parentKnob && parentKnob->typeName() == "Group"){
#warning "FIXME: if the Knob is within a group, only show it if the group is unfolded!"
                     // FIXME: this should be done recursively
                     Group_KnobGui* parentGui = dynamic_cast<Group_KnobGui*>(findKnobGuiOrCreate(parentKnob));
                     parentGui->addKnob(gui,parentTab->second.second,offsetColumn);
                     if (parentGui->isChecked()) {
                         gui->show();
                     }else{
                         gui->hide();
                     }
                     
                 }
                 

             }
            
         }
    
    }
}


void DockablePanel::addTab(const QString& name){
    QWidget* newTab = new QWidget(_tabWidget);
    QGridLayout *tabLayout = new QGridLayout(newTab);
    newTab->setLayout(tabLayout);
    tabLayout->setVerticalSpacing(2);
    tabLayout->setContentsMargins(3, 0, 0, 0);
    tabLayout->setHorizontalSpacing(5);
    
    _tabWidget->addTab(newTab,name);
    _tabs.insert(make_pair(name,make_pair(newTab,0)));
}

void DockablePanel::pushUndoCommand(QUndoCommand* cmd){
    _undoStack->push(cmd);
    _undoButton->setEnabled(_undoStack->canUndo());
    _redoButton->setEnabled(_undoStack->canRedo());
}

void DockablePanel::onUndoPressed(){
    _undoStack->undo();
    _undoButton->setEnabled(_undoStack->canUndo());
    _redoButton->setEnabled(_undoStack->canRedo());
    emit undoneChange();
}

void DockablePanel::onRedoPressed(){
    _undoStack->redo();
    _undoButton->setEnabled(_undoStack->canUndo());
    _redoButton->setEnabled(_undoStack->canRedo());
    emit redoneChange();
}

void DockablePanel::showHelp(){
    QToolTip::showText(QCursor::pos(), _helpButton->toolTip());
}

void DockablePanel::closePanel(){
    
    setVisible(false);
    
    std::vector<QWidget*> _panels;
    for(int i =0 ; i < _container->count(); ++i) {
        if (QWidget *myItem = dynamic_cast <QWidget*>(_container->itemAt(i))){
            _panels.push_back(myItem);
            _container->removeWidget(myItem);
        }
    }
    for (U32 i =0 ; i < _panels.size(); ++i) {
        _container->addWidget(_panels[i]);
    }
    
    update();
    emit closed();
}
void DockablePanel::minimizeOrMaximize(bool toggled){
    _minimized=toggled;
    if(_minimized){
        QPixmap pixM;
        appPTR->getIcon(Natron::MAXIMIZE_WIDGET_PIXMAP,&pixM);
        _minimize->setIcon(QIcon(pixM));
        emit minimized();
    }else{
        QPixmap pixM;
        appPTR->getIcon(Natron::MINIMIZE_WIDGET_PIXMAP,&pixM);
        _minimize->setIcon(QIcon(pixM));
        emit maximized();
    }
    _tabWidget->setVisible(!_minimized);
    std::vector<QWidget*> _panels;
    for(int i =0 ; i < _container->count(); ++i) {
        if (QWidget *myItem = dynamic_cast <QWidget*>(_container->itemAt(i))){
            _panels.push_back(myItem);
            _container->removeWidget(myItem);
        }
    }
    for (U32 i =0 ; i < _panels.size(); ++i) {
        _container->addWidget(_panels[i]);
    }
    update();
}


void DockablePanel::onNameChanged(const QString& str){
    if(_nameLabel){
        _nameLabel->setText(str);
    }else if(_nameLineEdit){
        _nameLineEdit->setText(str);
    }
}


Button* DockablePanel::insertHeaderButton(int headerPosition){
    Button* ret = new Button(_headerWidget);
    _headerLayout->insertWidget(headerPosition, ret);
    return ret;
}

KnobGui* DockablePanel::findKnobGuiOrCreate(Knob* knob){
    for (std::map<Knob*,KnobGui*>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
        if(it->first == knob){
            return it->second;
        }
    }
    
    
    KnobGui* ret =  appPTR->getKnobGuiFactory().createGuiForKnob(knob,this);
    QObject::connect(ret,SIGNAL(deleted(KnobGui*)),this,SLOT(onKnobDeletion(KnobGui*)));
    if(!ret){
        std::cout << "Failed to create gui for Knob" << std::endl;
        return NULL;
    }
    _knobs.insert(make_pair(knob, ret));
    return ret;
    
}

void DockablePanel::onKnobDeletion(KnobGui* k){
    for(std::map<Knob*,KnobGui*>::iterator it = _knobs.begin();it!=_knobs.end();++it){
        if (it->second == k) {
            _knobs.erase(it);
            return;
        }
    }
}





NodeSettingsPanel::NodeSettingsPanel(NodeGui* NodeUi ,QVBoxLayout* container,QWidget *parent)
:DockablePanel(NodeUi->getNode()->getLiveInstance(),
               container,
               false,
               NodeUi->getNode()->getName().c_str(),
               NodeUi->getNode()->description().c_str(),
               "Settings",
               parent)
,_nodeGUI(NodeUi)
{}



void NodeSettingsPanel::setSelected(bool s){
    _selected = s;
    style()->unpolish(this);
    style()->polish(this);
}


