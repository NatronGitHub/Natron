/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ManageUserParamsDialog.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>

#include "Engine/Knob.h" // KnobI
#include "Engine/KnobTypes.h" // KnobPage
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodePtr

#include "Gui/AddKnobDialog.h"
#include "Gui/Button.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PickKnobDialog.h"
#include "Gui/Utils.h" // convertFromPlainText

namespace {
struct TreeItem
{
    QTreeWidgetItem* item;
    boost::shared_ptr<KnobI> knob;
    QString scriptName;
};
}

class UserParamsDialogTreeWidget : public QTreeWidget
{
public:
    
    UserParamsDialogTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
    {
        
    }
    
    QModelIndex indexFromItemPublic(QTreeWidgetItem *item, int column = 0) const
    {
        return indexFromItem(item,column);
    }
};

struct ManageUserParamsDialogPrivate
{
    DockablePanel* panel;
    
    QHBoxLayout* mainLayout;
    UserParamsDialogTreeWidget* tree;
    
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
    
    boost::shared_ptr<KnobPage> getUserPageKnob() const;
    
    void initializeKnobs(const std::vector<boost::shared_ptr<KnobI> >& knobs,QTreeWidgetItem* parent,std::list<KnobI*>& markedKnobs);
    
    void rebuildUserPages();
    
    QTreeWidgetItem* createItemForKnob(const boost::shared_ptr<KnobI>& knob,int insertIndex = -1);
    
    void saveAndRebuildPages();
    
    std::list<TreeItem>::const_iterator findItemForTreeItem(const QTreeWidgetItem* item) const
    {
        for (std::list<TreeItem>::const_iterator it = items.begin(); it!=items.end(); ++it) {
            if (it->item == item) {
                return it;
            }
        }
        return items.end();
    }
    
};

ManageUserParamsDialog::ManageUserParamsDialog(DockablePanel* panel,QWidget* parent)
: QDialog(parent)
, _imp(new ManageUserParamsDialogPrivate(panel))
{
    _imp->mainLayout = new QHBoxLayout(this);
    
    _imp->tree = new UserParamsDialogTreeWidget(this);
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
    userPageItem.scriptName = NATRON_USER_MANAGED_KNOBS_PAGE;
    
    _imp->items.push_back(userPageItem);
    
    QObject::connect(_imp->tree, SIGNAL(itemSelectionChanged()),this,SLOT(onSelectionChanged()));
    QObject::connect(_imp->tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));

    std::list<KnobI*> markedKnobs;
    const std::vector<boost::shared_ptr<KnobI> >& knobs = panel->getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        
        
        KnobPage* page = dynamic_cast<KnobPage*>(it->get());
        if (page) {
            
            TreeItem pageItem;
            if (page->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                TreeItem & item = _imp->items.front();
                item.knob = *it;
                
                //Reposition the tree item correctly
                delete item.item;
                item.item = new QTreeWidgetItem(_imp->tree);
                item.item->setText(0, NATRON_USER_MANAGED_KNOBS_PAGE);
                item.item->setExpanded(true);
                pageItem = userPageItem;
            } else {
                pageItem.item = new QTreeWidgetItem(_imp->tree);
                pageItem.scriptName = page->getName().c_str();
                pageItem.item->setText(0, pageItem.scriptName);
                pageItem.knob = *it;
                pageItem.item->setExpanded(true);
                _imp->items.push_back(pageItem);
            }
            
            if ((*it)->isUserKnob()) {
                
                std::vector<boost::shared_ptr<KnobI> > children = page->getChildren();
                _imp->initializeKnobs(children, pageItem.item, markedKnobs);
            }
            
        }
        
    }
    
    _imp->mainLayout->addWidget(_imp->tree);
    
    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QVBoxLayout(_imp->buttonsContainer);
    
    _imp->addButton = new Button(tr("Add..."),_imp->buttonsContainer);
    _imp->addButton->setToolTip(Natron::convertFromPlainText(tr("Add a new parameter, group or page"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->addButton,SIGNAL(clicked(bool)),this,SLOT(onAddClicked()));
    _imp->buttonsLayout->addWidget(_imp->addButton);
    
    _imp->pickButton = new Button(tr("Pick..."),_imp->buttonsContainer);
    _imp->pickButton->setToolTip(Natron::convertFromPlainText(tr("Add a new parameter that is directly copied from/linked to another parameter"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->pickButton,SIGNAL(clicked(bool)),this,SLOT(onPickClicked()));
    _imp->buttonsLayout->addWidget(_imp->pickButton);
    
    _imp->editButton = new Button(tr("Edit..."),_imp->buttonsContainer);
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

static QString createTextForKnob(const boost::shared_ptr<KnobI>& knob)
{
    QString text = knob->getName().c_str();
    std::list<boost::shared_ptr<KnobI> > listeners;
    knob->getListeners(listeners);
    if (!listeners.empty()) {
        boost::shared_ptr<KnobI> listener = listeners.front();
        if (listener->getAliasMaster() == knob) {
            text += " (alias of ";
            Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(listener->getHolder());
            if (effect) {
                text += effect->getScriptName_mt_safe().c_str();
                text += '.';
            }
            text += listener->getName().c_str();
            text += ')';
        }
    }
    return text;
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
        i.scriptName = (*it2)->getName().c_str();
        i.item->setText(0, createTextForKnob(i.knob));
        i.item->setExpanded(true);
        items.push_back(i);
        
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(it2->get());
        if (isGrp) {
            std::vector<boost::shared_ptr<KnobI> > children = isGrp->getChildren();
            initializeKnobs(children, i.item, markedKnobs);
        }
    }
}


boost::shared_ptr<KnobPage>
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
        bool useAlias;
        boost::shared_ptr<KnobPage> page;
        boost::shared_ptr<KnobGroup> group;
        KnobGui* selectedKnob = dialog.getSelectedKnob(&useAlias,&page,&group);
        if (!selectedKnob) {
            return;
        }
        
        NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(_imp->panel);
        assert(nodePanel);
        boost::shared_ptr<NodeGui> nodeGui = nodePanel->getNode();
        assert(nodeGui);
        NodePtr node = nodeGui->getNode();
        assert(node);
        boost::shared_ptr<KnobI> duplicate = selectedKnob->createDuplicateOnNode(node->getLiveInstance(), useAlias, page, group, -1);
        _imp->createItemForKnob(duplicate);
        _imp->saveAndRebuildPages();
    }
}

void
ManageUserParamsDialog::onAddClicked()
{
    AddKnobDialog dialog(_imp->panel,boost::shared_ptr<KnobI>(),this);
    if (dialog.exec()) {
        //Ensure the user page knob exists
        boost::shared_ptr<KnobPage> userPageKnob = _imp->panel->getUserPageKnob();
        (void)userPageKnob;
        boost::shared_ptr<KnobI> knob = dialog.getKnob();
        _imp->createItemForKnob(knob);
        _imp->saveAndRebuildPages();
    }
}



QTreeWidgetItem*
ManageUserParamsDialogPrivate::createItemForKnob(const boost::shared_ptr<KnobI>& knob,int insertIndex)
{
    QTreeWidgetItem* parent = 0;
    
    boost::shared_ptr<KnobI> parentKnob;
    if (knob) {
        parentKnob = knob->getParentKnob();
    }
    if (parentKnob) {
        for (std::list<TreeItem>::iterator it = items.begin(); it != items.end(); ++it) {
            if (it->knob.get() == parentKnob.get()) {
                parent = it->item;
                break;
            }
        }
    }
    if (!parent) {
        KnobPage* isPage = dynamic_cast<KnobPage*>(knob.get());
        if (!isPage) {
            //Default to user page
            for (std::list<TreeItem>::iterator it = items.begin(); it != items.end(); ++it) {
                if (it->scriptName == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
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
        if (insertIndex == -1) {
            parent->addChild(i.item);
        } else {
            parent->insertChild(insertIndex, i.item);
        }
        parent->setExpanded(true);
    } else {
        if (insertIndex == -1) {
            tree->addTopLevelItem(i.item);
        } else {
            tree->insertTopLevelItem(insertIndex, i.item);
        }
    }
    i.item->setExpanded(true);
    i.scriptName = knob->getName().c_str();
    
    i.item->setText(0, createTextForKnob(i.knob));
    items.push_back(i);
    return i.item;
    

}

void
ManageUserParamsDialogPrivate::saveAndRebuildPages()
{
    rebuildUserPages();
    panel->getGui()->getApp()->triggerAutoSave();
}

void
ManageUserParamsDialog::onDeleteClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    
                    QString question;
                    question.append(tr("Removing"));
                    question += ' ';
                    question += it->knob->getName().c_str();
                    question += ' ';
                    question += tr("cannot be undone. Are you sure you want to continue?");
                    Natron::StandardButtonEnum rep = Natron::questionDialog(tr("Remove parameter").toStdString(), question.toStdString(), false,
                                                                             Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo), Natron::eStandardButtonYes);
                    if (rep != Natron::eStandardButtonYes) {
                        return;
                    }
                    it->knob->getHolder()->removeDynamicKnob(it->knob.get());
                    delete it->item;
                    _imp->items.erase(it);
                    
                    boost::shared_ptr<KnobPage> userPage = _imp->getUserPageKnob();
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
ManageUserParamsDialog::onEditClickedInternal(const QList<QTreeWidgetItem*> &selection)
{
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {

                    AddKnobDialog dialog(_imp->panel,it->knob,this);
                    if (dialog.exec()) {
                        
                        int indexIndParent = -1;
                        QTreeWidgetItem* parent = it->item->parent();
                        if (parent) {
                            indexIndParent = parent->indexOfChild(it->item);
                        }
                        delete it->item;
                        _imp->items.erase(it);
                        boost::shared_ptr<KnobI> knob = dialog.getKnob();
                        _imp->createItemForKnob(knob,indexIndParent);
                        _imp->saveAndRebuildPages();
                        break;
                    }
                }
                
            }
        }
    }
}

void
ManageUserParamsDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
        if (_imp->removeButton->isEnabled()) {
            onDeleteClicked();
            e->accept();
            return;
        }
    }
    QDialog::keyPressEvent(e);
    
}

void
ManageUserParamsDialog::onEditClicked()
{
    
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    onEditClickedInternal(selection);
    
}

void
ManageUserParamsDialog::onUpClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (selection.empty()) {
        return;
    }
    QTreeWidgetItem* selectedItem = selection.front();
    for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->item == selectedItem) {
            
            QTreeWidgetItem* parent = 0;
            boost::shared_ptr<KnobI> parentKnob;
            if (it->knob) {
                parentKnob = it->knob->getParentKnob();
            }
            if (parentKnob) {
                if (parentKnob->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                    for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                        if (it->scriptName == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
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
            
            if (!it->knob || !_imp->panel->getHolder()->moveKnobOneStepUp(it->knob.get())) {
                return;
            }
            QList<QTreeWidgetItem*> children = it->item->takeChildren();
            delete it->item;
            
            QTreeWidgetItem* item = _imp->createItemForKnob(it->knob, index -1);
            item->insertChildren(0, children);
            _imp->saveAndRebuildPages();
            QItemSelectionModel* model = _imp->tree->selectionModel();
            model->select(_imp->tree->indexFromItemPublic(item), QItemSelectionModel::ClearAndSelect);
            
            break;
        }
    }
    
    
}

void
ManageUserParamsDialog::onDownClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (selection.empty()) {
        return;
    }
    QTreeWidgetItem* selectedItem = selection.front();
    
    for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->item == selectedItem) {
            
            
            //Find the parent item
            QTreeWidgetItem* parent = 0;
            boost::shared_ptr<KnobI> parentKnob;
            if (it->knob) {
                parentKnob = it->knob->getParentKnob();
            }
            if (parentKnob) {
                if (parentKnob->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                    for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                        if (it->scriptName == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
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
            
            bool moveOk = false;
            if (it->knob) {
                moveOk = _imp->panel->getHolder()->moveKnobOneStepDown(it->knob.get());
            }
            if (!moveOk) {
                break;
            }
            
            QList<QTreeWidgetItem*> children = it->item->takeChildren();
            delete it->item;
            
            QTreeWidgetItem* item = _imp->createItemForKnob(it->knob, index + 1);
            item->insertChildren(0, children);
            _imp->saveAndRebuildPages();
            QItemSelectionModel* model = _imp->tree->selectionModel();
            model->select(_imp->tree->indexFromItemPublic(item), QItemSelectionModel::ClearAndSelect);
            break;
        }
    }
}

void
ManageUserParamsDialog::onCloseClicked()
{
    accept();
}

void
ManageUserParamsDialog::onItemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    QList<QTreeWidgetItem*> selection;
    if (item) {
        selection.push_back(item);
    }
    onEditClickedInternal(selection);
}

void
ManageUserParamsDialog::onSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    bool canEdit = true;
    bool canDelete = true;
    if (!selection.isEmpty()) {
        QTreeWidgetItem* item = selection[0];
        if (item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
            canEdit = false;
            canDelete = false;
        }
        for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            if (it->item == item) {
                KnobPage* isPage = dynamic_cast<KnobPage*>(it->knob.get());
                KnobGroup* isGroup = dynamic_cast<KnobGroup*>(it->knob.get());
                if (isPage) {
                    canEdit = false;
                    if (!isPage->isUserKnob()) {
                        canDelete = false;
                    }
                } else if (isGroup) {
                    canEdit = false;
                }
                break;
            }
        }
    }
    
    _imp->removeButton->setEnabled(selection.size() == 1 && canDelete);
    _imp->editButton->setEnabled(selection.size() == 1 && canEdit);
    _imp->upButton->setEnabled(selection.size() == 1);
    _imp->downButton->setEnabled(selection.size() == 1);
}
