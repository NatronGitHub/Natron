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
#include <QFormLayout>
#include <QUndoCommand>
#include <QDebug>
#include <QToolTip>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QPaintEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QTextDocument> // for Qt::convertFromPlainText

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiTypes.h" // for Group_KnobGui
#include "Gui/KnobGuiFactory.h"
#include "Gui/LineEdit.h"
#include "Gui/Button.h"
#include "Gui/NodeGraph.h"
#include "Gui/ClickableLabel.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"

using std::make_pair;
using namespace Natron;

DockablePanel::DockablePanel(Gui* gui
                             ,KnobHolder* holder
                             , QVBoxLayout* container
                             , HeaderMode headerMode
                             ,bool useScrollAreasForTabs
                             , const QString& initialName
                             , const QString& helpToolTip
                             , bool createDefaultTab, const QString& defaultTab
                             , QWidget *parent)
:QFrame(parent)
,_gui(gui)
,_container(container)
,_mainLayout(NULL)
,_headerWidget(NULL)
,_headerLayout(NULL)
,_nameLineEdit(NULL)
,_nameLabel(NULL)
,_tabWidget(NULL)
,_helpButton(NULL)
,_minimize(NULL)
,_floatButton(NULL)
,_cross(NULL)
,_undoButton(NULL)
,_redoButton(NULL)
,_restoreDefaultsButton(NULL)
,_minimized(false)
,_undoStack(new QUndoStack)
,_floating(false)
,_floatingWidget(NULL)
,_knobs()
,_holder(holder)
,_tabs()
,_defaultTabName(defaultTab)
,_useScrollAreasForTabs(useScrollAreasForTabs)
,_mode(headerMode)
{
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(_mainLayout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFrameShape(QFrame::Box);
    
    if(headerMode != NO_HEADER){
        
        _headerWidget = new QFrame(this);
        _headerWidget->setFrameShape(QFrame::Box);
        _headerLayout = new QHBoxLayout(_headerWidget);
        _headerLayout->setContentsMargins(0, 0, 0, 0);
        _headerLayout->setSpacing(2);
        _headerWidget->setLayout(_headerLayout);
        
        
        QPixmap pixHelp ;
        appPTR->getIcon(NATRON_PIXMAP_HELP_WIDGET,&pixHelp);
        _helpButton = new Button(QIcon(pixHelp),"",_headerWidget);
        if (!helpToolTip.isEmpty()) {
            _helpButton->setToolTip(Qt::convertFromPlainText(helpToolTip, Qt::WhiteSpaceNormal));
        }
        _helpButton->setFixedSize(15, 15);
        QObject::connect(_helpButton, SIGNAL(clicked()), this, SLOT(showHelp()));
        
        
        QPixmap pixM;
        appPTR->getIcon(NATRON_PIXMAP_MINIMIZE_WIDGET,&pixM);
        
        QPixmap pixC;
        appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET,&pixC);
        
        QPixmap pixF;
        appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, &pixF);
        
        _minimize=new Button(QIcon(pixM),"",_headerWidget);
        _minimize->setFixedSize(15,15);
        _minimize->setCheckable(true);
        QObject::connect(_minimize,SIGNAL(toggled(bool)),this,SLOT(minimizeOrMaximize(bool)));
        
        _floatButton = new Button(QIcon(pixF),"",_headerWidget);
        _floatButton->setFixedSize(15, 15);
        QObject::connect(_floatButton,SIGNAL(clicked()),this,SLOT(floatPanel()));
        
        
        _cross=new Button(QIcon(pixC),"",_headerWidget);
        _cross->setFixedSize(15,15);
        QObject::connect(_cross,SIGNAL(clicked()),this,SLOT(closePanel()));
        
        
        QPixmap pixUndo ;
        appPTR->getIcon(NATRON_PIXMAP_UNDO,&pixUndo);
        QPixmap pixUndo_gray ;
        appPTR->getIcon(NATRON_PIXMAP_UNDO_GRAYSCALE,&pixUndo_gray);
        QIcon icUndo;
        icUndo.addPixmap(pixUndo,QIcon::Normal);
        icUndo.addPixmap(pixUndo_gray,QIcon::Disabled);
        _undoButton = new Button(icUndo,"",_headerWidget);
        _undoButton->setToolTip(Qt::convertFromPlainText("Undo the last change made to this operator", Qt::WhiteSpaceNormal));
        _undoButton->setEnabled(false);
        
        QPixmap pixRedo ;
        appPTR->getIcon(NATRON_PIXMAP_REDO,&pixRedo);
        QPixmap pixRedo_gray;
        appPTR->getIcon(NATRON_PIXMAP_REDO_GRAYSCALE,&pixRedo_gray);
        QIcon icRedo;
        icRedo.addPixmap(pixRedo,QIcon::Normal);
        icRedo.addPixmap(pixRedo_gray,QIcon::Disabled);
        _redoButton = new Button(icRedo,"",_headerWidget);
        _redoButton->setToolTip(Qt::convertFromPlainText("Redo the last change undone to this operator", Qt::WhiteSpaceNormal));
        _redoButton->setEnabled(false);
        
        QPixmap pixRestore;
        appPTR->getIcon(NATRON_PIXMAP_RESTORE_DEFAULTS, &pixRestore);
        QIcon icRestore;
        icRestore.addPixmap(pixRestore);
        _restoreDefaultsButton = new Button(icRestore,"",_headerWidget);
        _restoreDefaultsButton->setToolTip(Qt::convertFromPlainText("Restore default values for this operator."
                                                                    " This cannot be undone!",Qt::WhiteSpaceNormal));
        QObject::connect(_restoreDefaultsButton,SIGNAL(clicked()),this,SLOT(onRestoreDefaultsButtonClicked()));
        
        
        QObject::connect(_undoButton, SIGNAL(clicked()),this, SLOT(onUndoClicked()));
        QObject::connect(_redoButton, SIGNAL(clicked()),this, SLOT(onRedoPressed()));
        
        if(headerMode != READ_ONLY_NAME){
            _nameLineEdit = new LineEdit(_headerWidget);
            _nameLineEdit->setText(initialName);
            QObject::connect(_nameLineEdit,SIGNAL(editingFinished()),this,SLOT(onLineEditNameEditingFinished()));
            _headerLayout->addWidget(_nameLineEdit);
        }else{
            _nameLabel = new QLabel(initialName,_headerWidget);
            _headerLayout->addWidget(_nameLabel);
        }
        
        _headerLayout->addStretch();
        
        _headerLayout->addWidget(_undoButton);
        _headerLayout->addWidget(_redoButton);
        _headerLayout->addWidget(_restoreDefaultsButton);
        
        _headerLayout->addStretch();
        _headerLayout->addWidget(_helpButton);
        _headerLayout->addWidget(_minimize);
        _headerLayout->addWidget(_floatButton);
        _headerLayout->addWidget(_cross);
        
        _mainLayout->addWidget(_headerWidget);
        
    }
    
    _tabWidget = new QTabWidget(this);
    _tabWidget->setObjectName("QTabWidget");
    _mainLayout->addWidget(_tabWidget);
    
    if(createDefaultTab){
        addTab(_defaultTabName);
    }
}

DockablePanel::~DockablePanel(){
    delete _undoStack;
    
    ///Delete the knob gui if they weren't before
    ///normally the onKnobDeletion() function should have cleared them
    for(std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _knobs.begin();it!=_knobs.end();++it){
        if(it->second){
            KnobHelper* helper = dynamic_cast<KnobHelper*>(it->first.get());
            QObject::disconnect(helper->getSignalSlotHandler().get(),SIGNAL(deleted()),this,SLOT(onKnobDeletion()));
            delete it->second;
        }
    }
}

void DockablePanel::onRestoreDefaultsButtonClicked() {
    for(std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _knobs.begin();it!=_knobs.end();++it) {
        for (int i = 0; i < it->first->getDimension(); ++i) {
            if (it->first->typeName() != Button_Knob::typeNameStatic()) {
                it->first->resetToDefaultValue(i);
            }
        }
    }
}

void DockablePanel::onLineEditNameEditingFinished() {
    
    if (_gui->isClosing()) {
        return;
    }
    
    Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(_holder);
    if (effect) {
        
        std::string newName = _nameLineEdit->text().toStdString();
        if (newName.empty()) {
            _nameLineEdit->blockSignals(true);
            Natron::errorDialog("Node name", "A node must have a unique name.");
            _nameLineEdit->setText(effect->getName().c_str());
            _nameLineEdit->blockSignals(false);
            return;
        }
        
        ///if the node name hasn't changed return
        if (effect->getName() == newName) {
            return;
        }
        
        std::vector<boost::shared_ptr<Natron::Node> > allNodes = _holder->getApp()->getProject()->getCurrentNodes();
        for (U32 i = 0;  i < allNodes.size(); ++i) {
            if (allNodes[i]->getName() == newName) {
                _nameLineEdit->blockSignals(true);
                Natron::errorDialog("Node name", "A node with the same name already exists in the project.");
                _nameLineEdit->setText(effect->getName().c_str());
                _nameLineEdit->blockSignals(false);
                return;
            }
        }
    }
    emit nameChanged(_nameLineEdit->text());
}

void DockablePanel::initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> >& knobs,bool onlyTopLevelKnobs) {
    QWidget* lastRowWidget = 0;
    for(U32 i = 0 ; i < knobs.size(); ++i) {
        
        ///we create only top level knobs, they will in-turn create their children if they have any
        if ((!onlyTopLevelKnobs) || (onlyTopLevelKnobs && !knobs[i]->getParentKnob())) {
            bool makeNewLine = true;
            
            boost::shared_ptr<Group_Knob> isGroup = boost::dynamic_pointer_cast<Group_Knob>(knobs[i]);
            boost::shared_ptr<Tab_Knob> isTab = boost::dynamic_pointer_cast<Tab_Knob>(knobs[i]);
            
            ////The knob  will have a vector of all other knobs on the same line.
            std::vector< boost::shared_ptr< KnobI > > knobsOnSameLine;
            
            if (!isGroup && !isTab) { //< a knob with children (i.e a group) cannot have children on the same line
                if (i > 0 && knobs[i-1]->isNewLineTurnedOff()) {
                    makeNewLine = false;
                }
                ///find all knobs backward that are on the same line.
                int k = i -1;
                while (k >= 0 && knobs[k]->isNewLineTurnedOff()) {
                    knobsOnSameLine.push_back(knobs[k]);
                    --k;
                }
                
                ///find all knobs forward that are on the same line.
                k = i;
                while (k < (int)(knobs.size() - 1) && knobs[k]->isNewLineTurnedOff()) {
                    knobsOnSameLine.push_back(knobs[k + 1]);
                    ++k;
                }
            }
            
            KnobGui* newGui = findKnobGuiOrCreate(knobs[i],makeNewLine,lastRowWidget,knobsOnSameLine);
            ///childrens cannot be on the same row than their parent
            if (!isGroup && !isTab && newGui) {
                lastRowWidget = newGui->getFieldContainer();
            }

        }
    }
}

void DockablePanel::initializeKnobs() {
    
    /// function called to create the gui for each knob. It can be called several times in a row
    /// without any damage
    initializeKnobVector(_holder->getKnobs(),true);
}


KnobGui* DockablePanel::findKnobGuiOrCreate(boost::shared_ptr<KnobI> knob,bool makeNewLine,QWidget* lastRowWidget,
                                            const std::vector< boost::shared_ptr< KnobI > >& knobsOnSameLine) {
    
    assert(knob);
    KnobGui* ret = 0;
    boost::shared_ptr<Group_Knob> isGroup = boost::dynamic_pointer_cast<Group_Knob>(knob);
    boost::shared_ptr<Tab_Knob> isTab = boost::dynamic_pointer_cast<Tab_Knob>(knob);
    if (isTab) {
        QString tabName(isTab->getDescription().c_str());
        for (int j = 0 ; j < _tabWidget->count(); ++j) {
            if(_tabWidget->tabText(j) == tabName){
                return ret;
            }
        }
        ///if it doesn't exist, create it
        addTab(tabName);

    } else {
        for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
            if(it->first == knob){
                return it->second;
            }
        }
        
        KnobHelper* helper = dynamic_cast<KnobHelper*>(knob.get());
        QObject::connect(helper->getSignalSlotHandler().get(),SIGNAL(deleted()),this,SLOT(onKnobDeletion()));
        
        ret =  appPTR->createGuiForKnob(knob,this);
        if (!ret) {
            qDebug() << "Failed to create Knob GUI";
            return NULL;
        }
        _knobs.insert(make_pair(knob, ret));
        
        ///if widgets for the KnobGui have already been created, don't the following
        if (!ret->hasWidgetBeenCreated()) {
            
            
            ///find to what to belongs the knob. by default it belongs to the default tab.
            std::map<QString,std::pair<QWidget*,int> >::iterator parentTab = _tabs.end() ;
            
            
            boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
            boost::shared_ptr<KnobI> parentKnobTmp = parentKnob;
            boost::shared_ptr<Tab_Knob> parentIsTab = boost::dynamic_pointer_cast<Tab_Knob>(parentKnob);
            while (parentKnobTmp) {
                parentKnobTmp = parentKnobTmp->getParentKnob();
                boost::shared_ptr<Tab_Knob> parentIsTabTmp = boost::dynamic_pointer_cast<Tab_Knob>(parentKnobTmp);
                if (parentIsTabTmp) {
                    parentIsTab = parentIsTabTmp;
                } else if (parentIsTab) { //< we found a lower knob that is a tab
                    break;
                }
            }
            boost::shared_ptr<Group_Knob> parentIsGroup = boost::dynamic_pointer_cast<Group_Knob>(parentKnob);
            
            ///if the parent is a tab find it
            if (parentIsTab) {
                //make sure the tab has been created;
                std::map<QString,std::pair<QWidget*,int> >::iterator it = _tabs.find(parentIsTab->getDescription().c_str());
                
                ///if it crashes there's serious problem because findKnobGuiOrCreate(parentKnob) should have registered
                ///the tab. Maybe you passed incorrect pointers ?
                assert(it != _tabs.end());
                parentTab = it;
                
            } else {
                ///defaults to the default tab
                parentTab = _tabs.find(_defaultTabName);
                ///The dockpanel must have a default tab if you didn't declare your own tab to
                ///put your knobs into!
                assert(parentTab != _tabs.end());
            }
            assert(parentTab != _tabs.end());
            
            /// if this knob is within a group, make sure the group is created so far
            Group_KnobGui* parentGui = 0;
            if (parentIsGroup) {
                parentGui = dynamic_cast<Group_KnobGui*>(findKnobGuiOrCreate(parentKnob,true,NULL));
                assert(parentGui);
            }
            
            
            ///if the knob has specified that it didn't want to trigger a new line, decrement the current row
            /// index of the tab
            
            if (!makeNewLine) {
                --parentTab->second.second;
            }
            
            ///retrieve the form layout
            QFormLayout* layout;
            if (_useScrollAreasForTabs) {
                layout = dynamic_cast<QFormLayout*>(
                                                    dynamic_cast<QScrollArea*>(parentTab->second.first)->widget()->layout());
            } else {
                layout = dynamic_cast<QFormLayout*>(parentTab->second.first->layout());
            }
            assert(layout);
            
            QWidget* fieldContainer = 0;
            QHBoxLayout* fieldLayout = 0;
            
            if (makeNewLine) {
                ///if new line is not turned off, create a new line
                fieldContainer = new QWidget(parentTab->second.first);
                fieldLayout = new QHBoxLayout(fieldContainer);
                fieldLayout->setContentsMargins(3,0,0,0);
                fieldContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            } else {
                ///otherwise re-use the last row's widget and layout
                assert(lastRowWidget);
                fieldContainer = lastRowWidget;
                fieldLayout = dynamic_cast<QHBoxLayout*>(fieldContainer->layout());
                
                ///the knobs use this value to know whether we should remove the row or not
                //fieldContainer->setObjectName("multi-line");
                
            }
            assert(fieldContainer);
            assert(fieldLayout);
            
            ClickableLabel* label = new ClickableLabel("",parentTab->second.first);

            
            if (ret->showDescriptionLabel() && label) {
                label->setText_overload(QString(QString(ret->getKnob()->getDescription().c_str()) + ":"));
                QObject::connect(label, SIGNAL(clicked(bool)), ret, SIGNAL(labelClicked(bool)));
            }
            
            ///fill the fieldLayout with the widgets
            ret->createGUI(layout,fieldContainer,label,fieldLayout,parentTab->second.second,makeNewLine,knobsOnSameLine);
            
            ///increment the row count
            ++parentTab->second.second;
            
            /// if this knob is within a group, check that the group is visible, i.e. the toplevel group is unfolded
            if (parentIsGroup) {
                assert(parentGui);
                ///FIXME: this offsetColumn is never really used. Shall we use this anyway? It seems
                ///to work fine without it.
                int offsetColumn = knob->determineHierarchySize();
                parentGui->addKnob(ret,parentTab->second.second,offsetColumn);
                
                bool showit = !ret->getKnob()->getIsSecret();
                // see KnobGui::setSecret() for a very similar code
                while (showit && parentIsGroup) {
                    assert(parentGui);
                    // check for secretness and visibility of the group
                    if (parentKnob->getIsSecret() || !parentGui->isChecked()) {
                        showit = false; // one of the including groups is folder, so this item is hidden
                    }
                    // prepare for next loop iteration
                    parentKnob = parentKnob->getParentKnob();
                    parentIsGroup =  boost::dynamic_pointer_cast<Group_Knob>(parentKnob);
                    if (parentKnob) {
                        parentGui = dynamic_cast<Group_KnobGui*>(findKnobGuiOrCreate(parentKnob,true,NULL));
                    }
                }
                if (showit) {
                    ret->show();
                } else {
                    //gui->hide(); // already hidden? please comment if it's not.
                }
            }
        }
    } //!isTab
    
    ///if the knob is a group or a tab, create all the children
    if (isTab || isGroup) {
        if (isTab) {
            initializeKnobVector(isTab->getChildren(), false);
        } else {
            initializeKnobVector(isGroup->getChildren(), false);
        }
    }
    
    
    return ret;
    
}



void DockablePanel::addTab(const QString& name){
    QWidget* newTab;
    QWidget* layoutContainer;
    if (_useScrollAreasForTabs) {
        QScrollArea* sa = new QScrollArea(_tabWidget);
        layoutContainer = new QWidget(sa);
        sa->setWidgetResizable(true);
        sa->setWidget(layoutContainer);
        newTab = sa;
    } else {
        newTab = new QWidget(_tabWidget);
        layoutContainer = newTab;
    }
    newTab->setObjectName(name);
    QFormLayout *tabLayout = new QFormLayout(layoutContainer);
    tabLayout->setObjectName("formLayout");
    layoutContainer->setLayout(tabLayout);
    //tabLayout->setVerticalSpacing(2); // unfortunately, this leaves extra space when parameters are hidden
    tabLayout->setVerticalSpacing(3);
    tabLayout->setContentsMargins(3, 0, 0, 0);
    tabLayout->setHorizontalSpacing(3);
    tabLayout->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);
    tabLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    tabLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    _tabWidget->addTab(newTab,name);
    _tabs.insert(make_pair(name,make_pair(newTab,0)));
}

const QUndoCommand* DockablePanel::getLastUndoCommand() const{
    return _undoStack->command(_undoStack->index()-1);
}

void DockablePanel::pushUndoCommand(QUndoCommand* cmd){
    _undoStack->setActive();
    _undoStack->push(cmd);
    if(_undoButton && _redoButton){
        _undoButton->setEnabled(_undoStack->canUndo());
        _redoButton->setEnabled(_undoStack->canRedo());
    }
}

void DockablePanel::onUndoClicked(){
    _undoStack->undo();
    if(_undoButton && _redoButton){
        _undoButton->setEnabled(_undoStack->canUndo());
        _redoButton->setEnabled(_undoStack->canRedo());
    }
    emit undoneChange();
}

void DockablePanel::onRedoPressed(){
    _undoStack->redo();
    if(_undoButton && _redoButton){
        _undoButton->setEnabled(_undoStack->canUndo());
        _redoButton->setEnabled(_undoStack->canRedo());
    }
    emit redoneChange();
}

void DockablePanel::showHelp(){
    QToolTip::showText(QCursor::pos(), _helpButton->toolTip());
}

void DockablePanel::closePanel() {
    if (_floating) {
        floatPanel();
    }
    close();
    getGui()->getApp()->redrawAllViewers();
    
}
void DockablePanel::minimizeOrMaximize(bool toggled){
    _minimized=toggled;
    if(_minimized){
        emit minimized();
    }else{
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

void DockablePanel::floatPanel() {
    _floating = !_floating;
    if (_floating) {
        assert(!_floatingWidget);
        _floatingWidget = new FloatingWidget(_gui);
        QObject::connect(_floatingWidget,SIGNAL(closed()),this,SLOT(closePanel()));
        _container->removeWidget(this);
        _floatingWidget->setWidget(size(),this);
    } else {
        assert(_floatingWidget);
        _floatingWidget->removeWidget();
        setParent(_container->parentWidget());
        _container->insertWidget(0, this);
        delete _floatingWidget;
        _floatingWidget = 0;
    }
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

void DockablePanel::onKnobDeletion(){
    
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (handler) {
        for(std::map<boost::shared_ptr<KnobI>,KnobGui*>::iterator it = _knobs.begin();it!=_knobs.end();++it){
            KnobHelper* helper = dynamic_cast<KnobHelper*>(it->first.get());
            if (helper->getSignalSlotHandler().get() == handler) {
                if(it->second){
                    delete it->second;
                }
                _knobs.erase(it);
                return;
            }
        }
    }
    
}


Gui* DockablePanel::getGui() const {
    return _gui;
}

void DockablePanel::insertHeaderWidget(int index,QWidget* widget) {
    if (_mode != NO_HEADER) {
        _headerLayout->insertWidget(index, widget);
    }
}

void DockablePanel::appendHeaderWidget(QWidget* widget) {
    if (_mode != NO_HEADER) {
        _headerLayout->addWidget(widget);
    }
}

QWidget* DockablePanel::getHeaderWidget() const {
    return _headerWidget;
}

NodeSettingsPanel::NodeSettingsPanel(Gui* gui,boost::shared_ptr<NodeGui> NodeUi ,QVBoxLayout* container,QWidget *parent)
:DockablePanel(gui,NodeUi->getNode()->getLiveInstance(),
               container,
               DockablePanel::FULLY_FEATURED,
               false,
               NodeUi->getNode()->getName().c_str(),
               NodeUi->getNode()->description().c_str(),
               true,
               "Settings",
               parent)
,_nodeGUI(NodeUi)
{
    QPixmap pixC;
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER,&pixC);
    _centerNodeButton = new Button(QIcon(pixC),"",getHeaderWidget());
    _centerNodeButton->setToolTip("Centers the node graph on this node.");
    _centerNodeButton->setFixedSize(15, 15);
    QObject::connect(_centerNodeButton,SIGNAL(clicked()),this,SLOT(centerNode()));
    insertHeaderWidget(0, _centerNodeButton);
}

NodeSettingsPanel::~NodeSettingsPanel(){
    _nodeGUI->removeSettingsPanel();
}


void NodeSettingsPanel::setSelected(bool s){
    _selected = s;
    style()->unpolish(this);
    style()->polish(this);
}

void NodeSettingsPanel::centerNode() {
    _nodeGUI->centerGraphOnIt();
}


