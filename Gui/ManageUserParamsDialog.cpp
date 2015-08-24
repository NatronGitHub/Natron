//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "ManageUserParamsDialog.h"

#include <cfloat>
#include <iostream>
#include <fstream>
#include <QLayout>
#include <QAction>
#include <QApplication>
#include <QTabWidget>
#include <QStyle>
#include <QUndoStack>
#include <QGridLayout>
#include <QUndoCommand>
#include <QFormLayout>
#include <QDebug>
#include <QToolTip>
#include <QHeaderView>
#include <QMutex>
#include <QTreeWidget>
#include <QCheckBox>
#include <QHeaderView>
#include <QColorDialog>
#include <QTimer>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QPaintEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QPainter>
#include <QImage>
#include <QToolButton>
#include <QDialogButtonBox>

#include <ofxNatron.h>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#include <boost/serialization/utility.hpp>

#include "Engine/BackDrop.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AddKnobDialog.h"
#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveEditorUndoRedo.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanelPrivate.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/KnobGuiTypes.h"
#include "Gui/KnobGuiTypes.h" // for Group_KnobGui
#include "Gui/KnobUndoCommand.h"
#include "Gui/LineEdit.h"
#include "Gui/Menu.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PickKnobDialog.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RotoPanel.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/TabWidget.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

namespace {
struct TreeItem
{
    QTreeWidgetItem* item;
    boost::shared_ptr<KnobI> knob;
};
}

struct ManageUserParamsDialogPrivate
{
    DockablePanel* panel;
    
    QHBoxLayout* mainLayout;
    QTreeWidget* tree;
    
    std::list<TreeItem> items;
    
    
    QWidget* buttonsContainer;
    QVBoxLayout* buttonsLayout;
    
    Button* addButton;
    Button* pickButton;
    Button* editButton;
    Button* removeButton;
    Button* upButton;
    Button* downButton;
    Button* closeButton;

    
    ManageUserParamsDialogPrivate(DockablePanel* panel)
    : panel(panel)
    , mainLayout(0)
    , tree(0)
    , items()
    , buttonsContainer(0)
    , buttonsLayout(0)
    , addButton(0)
    , pickButton(0)
    , editButton(0)
    , removeButton(0)
    , upButton(0)
    , downButton(0)
    , closeButton(0)
    {
        
    }
    
    boost::shared_ptr<Page_Knob> getUserPageKnob() const;
    
    void initializeKnobs(const std::vector<boost::shared_ptr<KnobI> >& knobs,QTreeWidgetItem* parent,std::list<KnobI*>& markedKnobs);
    
    void rebuildUserPages();
};

ManageUserParamsDialog::ManageUserParamsDialog(DockablePanel* panel,QWidget* parent)
: QDialog(parent)
, _imp(new ManageUserParamsDialogPrivate(panel))
{
    _imp->mainLayout = new QHBoxLayout(this);
    
    _imp->tree = new QTreeWidget(this);
    _imp->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    //_imp->tree->setRootIsDecorated(false);
    _imp->tree->setItemsExpandable(true);
    _imp->tree->header()->setStretchLastSection(false);
    _imp->tree->setTextElideMode(Qt::ElideMiddle);
    _imp->tree->setContextMenuPolicy(Qt::CustomContextMenu);
    _imp->tree->header()->setStretchLastSection(true);
    _imp->tree->header()->hide();
    
    TreeItem userPageItem;
    userPageItem.item = new QTreeWidgetItem(_imp->tree);
    userPageItem.item->setText(0, NATRON_USER_MANAGED_KNOBS_PAGE);
    userPageItem.item->setExpanded(true);
    _imp->items.push_back(userPageItem);
    
    QObject::connect(_imp->tree, SIGNAL(itemSelectionChanged()),this,SLOT(onSelectionChanged()));
    
    std::list<KnobI*> markedKnobs;
    const std::vector<boost::shared_ptr<KnobI> >& knobs = panel->getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ((*it)->isUserKnob()) {
            
            Page_Knob* page = dynamic_cast<Page_Knob*>(it->get());
            if (page) {
                
                TreeItem pageItem;
                if (page->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                    pageItem.item = userPageItem.item;
                } else {
                    pageItem.item = new QTreeWidgetItem(_imp->tree);
                    pageItem.item->setText(0, page->getName().c_str());
                    pageItem.knob = *it;
                    pageItem.item->setExpanded(true);
                }
                _imp->items.push_back(pageItem);
                
                std::vector<boost::shared_ptr<KnobI> > children = page->getChildren();
                _imp->initializeKnobs(children, pageItem.item, markedKnobs);
                
            }
        }
    }
    
    _imp->mainLayout->addWidget(_imp->tree);
    
    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QVBoxLayout(_imp->buttonsContainer);
    
    _imp->addButton = new Button(tr("Add"),_imp->buttonsContainer);
    _imp->addButton->setToolTip(Natron::convertFromPlainText(tr("Add a new parameter, group or page"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->addButton,SIGNAL(clicked(bool)),this,SLOT(onAddClicked()));
    _imp->buttonsLayout->addWidget(_imp->addButton);
    
    _imp->pickButton = new Button(tr("Pick"),_imp->buttonsContainer);
    _imp->pickButton->setToolTip(Natron::convertFromPlainText(tr("Add a new parameter that is directly copied from/linked to another parameter"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->pickButton,SIGNAL(clicked(bool)),this,SLOT(onPickClicked()));
    _imp->buttonsLayout->addWidget(_imp->pickButton);
    
    _imp->editButton = new Button(tr("Edit"),_imp->buttonsContainer);
    _imp->editButton->setToolTip(Natron::convertFromPlainText(tr("Edit the selected parameter"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->editButton,SIGNAL(clicked(bool)),this,SLOT(onEditClicked()));
    _imp->buttonsLayout->addWidget(_imp->editButton);
    
    _imp->removeButton = new Button(tr("Delete"),_imp->buttonsContainer);
    _imp->removeButton->setToolTip(Natron::convertFromPlainText(tr("Delete the selected parameter"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->removeButton,SIGNAL(clicked(bool)),this,SLOT(onDeleteClicked()));
    _imp->buttonsLayout->addWidget(_imp->removeButton);
    
    _imp->upButton = new Button(tr("Up"),_imp->buttonsContainer);
    _imp->upButton->setToolTip(Natron::convertFromPlainText(tr("Move the selected parameter one level up in the layout"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->upButton,SIGNAL(clicked(bool)),this,SLOT(onUpClicked()));
    _imp->buttonsLayout->addWidget(_imp->upButton);
    
    _imp->downButton = new Button(tr("Down"),_imp->buttonsContainer);
    _imp->downButton->setToolTip(Natron::convertFromPlainText(tr("Move the selected parameter one level down in the layout"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->downButton,SIGNAL(clicked(bool)),this,SLOT(onDownClicked()));
    _imp->buttonsLayout->addWidget(_imp->downButton);
    
    _imp->closeButton = new Button(tr("Close"),_imp->buttonsContainer);
    _imp->closeButton->setToolTip(Natron::convertFromPlainText(tr("Close this dialog"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->closeButton,SIGNAL(clicked(bool)),this,SLOT(onCloseClicked()));
    _imp->buttonsLayout->addWidget(_imp->closeButton);
    
    _imp->mainLayout->addWidget(_imp->buttonsContainer);
    onSelectionChanged();
    _imp->panel->setUserPageActiveIndex();

}

ManageUserParamsDialog::~ManageUserParamsDialog()
{
//    for (std::list<TreeItem>::iterator it = _imp->items.begin() ; it != _imp->items.end(); ++it) {
//        delete it->item;
//    }
}

void
ManageUserParamsDialogPrivate::initializeKnobs(const std::vector<boost::shared_ptr<KnobI> >& knobs,QTreeWidgetItem* parent,std::list<KnobI*>& markedKnobs)
{
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it2 = knobs.begin(); it2!=knobs.end(); ++it2) {
        
        if (std::find(markedKnobs.begin(),markedKnobs.end(),it2->get()) != markedKnobs.end()) {
            continue;
        }
        
        markedKnobs.push_back(it2->get());
        
        TreeItem i;
        i.knob = *it2;
        i.item = new QTreeWidgetItem(parent);
        i.item->setText(0, (*it2)->getName().c_str());
        i.item->setExpanded(true);
        items.push_back(i);
        
        Group_Knob* isGrp = dynamic_cast<Group_Knob*>(it2->get());
        if (isGrp) {
            std::vector<boost::shared_ptr<KnobI> > children = isGrp->getChildren();
            initializeKnobs(children, i.item, markedKnobs);
        }
    }
}


boost::shared_ptr<Page_Knob>
ManageUserParamsDialogPrivate::getUserPageKnob() const
{
    return panel->getUserPageKnob();
}


void
ManageUserParamsDialogPrivate::rebuildUserPages()
{
    panel->scanForNewKnobs();
}

void
ManageUserParamsDialog::onPickClicked()
{
    PickKnobDialog dialog(_imp->panel,this);
    if (dialog.exec()) {
        bool useExpr;
        KnobGui* selectedKnob = dialog.getSelectedKnob(&useExpr);
        if (!selectedKnob) {
            return;
        }
        
        NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(_imp->panel);
        assert(nodePanel);
        boost::shared_ptr<NodeGui> nodeGui = nodePanel->getNode();
        assert(nodeGui);
        NodePtr node = nodeGui->getNode();
        assert(node);
        selectedKnob->createDuplicateOnNode(node->getLiveInstance(), useExpr);
    }
}

void
ManageUserParamsDialog::onAddClicked()
{
    AddKnobDialog dialog(_imp->panel,boost::shared_ptr<KnobI>(),this);
    if (dialog.exec()) {
        //Ensure the user page knob exists
        boost::shared_ptr<Page_Knob> userPageKnob = _imp->panel->getUserPageKnob();
        
        boost::shared_ptr<KnobI> knob = dialog.getKnob();
        QTreeWidgetItem* parent = 0;
        
        boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
        if (parentKnob) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->knob.get() == parentKnob.get()) {
                    parent = it->item;
                    break;
                }
            }
        }
        if (!parent) {
            Page_Knob* isPage = dynamic_cast<Page_Knob*>(knob.get());
            if (!isPage) {
                //Default to user page
                for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                    if (it->item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                        parent = it->item;
                        break;
                    }
                }
            }

        }
        TreeItem i;
        i.knob = knob;
        i.item = new QTreeWidgetItem;
        if (parent) {
            parent->addChild(i.item);
            parent->setExpanded(true);
        } else {
            _imp->tree->addTopLevelItem(i.item);
        }
        i.item->setText(0, knob->getName().c_str());
        _imp->items.push_back(i);
    }
    _imp->rebuildUserPages();
    _imp->panel->getGui()->getApp()->triggerAutoSave();
}

void
ManageUserParamsDialog::onDeleteClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    it->knob->getHolder()->removeDynamicKnob(it->knob.get());
                    delete it->item;
                    _imp->items.erase(it);
                    
                    boost::shared_ptr<Page_Knob> userPage = _imp->getUserPageKnob();
                    if (userPage->getChildren().empty()) {
                        userPage->getHolder()->removeDynamicKnob(userPage.get());
                    }
                    _imp->panel->getGui()->getApp()->triggerAutoSave();
                    break;
                }
            }
        }
    }
}

void
ManageUserParamsDialog::onEditClicked()
{
    
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    
                    std::list<boost::shared_ptr<KnobI> > listeners;
                    it->knob->getListeners(listeners);
                    if (!listeners.empty()) {
                        Natron::StandardButtonEnum rep = Natron::questionDialog(tr("Edit parameter").toStdString(),
                                                                                tr("This parameter has one or several "
                                                                                   "parameters from which other parameters "
                                                                                   "of the project rely on through expressions "
                                                                                   "or links. Editing this parameter may "
                                                                                   "remove these expressions if the script-name is changed.\n"
                                                                                   "Do you want to continue?").toStdString(), false);
                        if (rep == Natron::eStandardButtonNo) {
                            return;
                        }
                    }
                    
                    AddKnobDialog dialog(_imp->panel,it->knob,this);
                    if (dialog.exec()) {
                        it->knob = dialog.getKnob();
                        it->item->setText(0, it->knob->getName().c_str());
                        _imp->rebuildUserPages();
                        _imp->panel->getGui()->getApp()->triggerAutoSave();
                        break;
                    }
                }
                
            }
        }
    }
    
}

void
ManageUserParamsDialog::onUpClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    
                    if (dynamic_cast<Page_Knob*>(it->knob.get())) {
                        break;
                    }
                    QTreeWidgetItem* parent = 0;
                    boost::shared_ptr<KnobI> parentKnob = it->knob->getParentKnob();
                    if (parentKnob) {
                        if (parentKnob->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        } else {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->knob == parentKnob) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        }
                    }
                    
                    int index;
                    if (!parent) {
                        index = _imp->tree->indexOfTopLevelItem(it->item);
                    } else {
                        index = parent->indexOfChild(it->item);
                    }
                    if (index == 0) {
                        break;
                    }
                    _imp->panel->getHolder()->moveKnobOneStepUp(it->knob.get());
                    QList<QTreeWidgetItem*> children = it->item->takeChildren();
                    delete it->item;
                    
                    it->item = new QTreeWidgetItem;
                    it->item->setText(0, it->knob->getName().c_str());
                    if (!parent) {
                        _imp->tree->insertTopLevelItem(index - 1, it->item);
                    } else {
                        parent->insertChild(index - 1, it->item);
                    }
                    it->item->insertChildren(0, children);
                    it->item->setExpanded(true);
                    _imp->tree->clearSelection();
                    it->item->setSelected(true);
                    break;
                }
            }
            
        }
        _imp->rebuildUserPages();
    }
}

void
ManageUserParamsDialog::onDownClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    if (dynamic_cast<Page_Knob*>(it->knob.get())) {
                        break;
                    }

                    QTreeWidgetItem* parent = 0;
                    boost::shared_ptr<KnobI> parentKnob = it->knob->getParentKnob();
                    if (parentKnob) {
                        if (parentKnob->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        } else {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->knob == parentKnob) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        }
                    }
                    int index;
                    
                    if (!parent) {
                        index = _imp->tree->indexOfTopLevelItem(it->item);
                        if (index == _imp->tree->topLevelItemCount() - 1) {
                            break;
                        }
                    } else {
                        index = parent->indexOfChild(it->item);
                        if (index == parent->childCount() - 1) {
                            break;
                        }
                    }
                    
                    _imp->panel->getHolder()->moveKnobOneStepDown(it->knob.get());
                    QList<QTreeWidgetItem*> children = it->item->takeChildren();
                    delete it->item;
                    
                    
                    it->item = new QTreeWidgetItem;
                    if (parent) {
                        parent->insertChild(index + 1, it->item);
                    } else {
                        _imp->tree->insertTopLevelItem(index + 1, it->item);
                    }
                    it->item->setText(0, it->knob->getName().c_str());
                    it->item->insertChildren(0, children);
                    it->item->setExpanded(true);
                    _imp->tree->clearSelection();
                    it->item->setSelected(true);
                    break;
                }
            }
            
        }
        _imp->rebuildUserPages();
    }
}

void
ManageUserParamsDialog::onCloseClicked()
{
    accept();
}

void
ManageUserParamsDialog::onSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    bool canEdit = true;
    bool canMove = true;
    bool canDelete = true;
    if (!selection.isEmpty()) {
        QTreeWidgetItem* item = selection[0];
        if (item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
            canEdit = false;
            canDelete = false;
            canMove = false;
        }
        for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            if (it->item == item) {
                Page_Knob* isPage = dynamic_cast<Page_Knob*>(it->knob.get());
                Group_Knob* isGroup = dynamic_cast<Group_Knob*>(it->knob.get());
                if (isPage) {
                    canMove = false;
                    canEdit = false;
                } else if (isGroup) {
                    canEdit = false;
                }
                break;
            }
        }
    }
    
    _imp->removeButton->setEnabled(selection.size() == 1 && canDelete);
    _imp->editButton->setEnabled(selection.size() == 1 && canEdit);
    _imp->upButton->setEnabled(selection.size() == 1 && canMove);
    _imp->downButton->setEnabled(selection.size() == 1 && canMove);
}
