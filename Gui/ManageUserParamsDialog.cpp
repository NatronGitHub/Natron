/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include <stdexcept>

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
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/AddKnobDialog.h"
#include "Gui/Button.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PickKnobDialog.h"

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

struct TreeItem
{
    QTreeWidgetItem* item;
    KnobIPtr knob;
    QString scriptName;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


class UserParamsDialogTreeWidget
    : public QTreeWidget
{
public:

    UserParamsDialogTreeWidget(QWidget* parent)
        : QTreeWidget(parent)
    {
    }

    QModelIndex indexFromItemPublic(QTreeWidgetItem *item,
                                    int column = 0) const
    {
        return indexFromItem(item, column);
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

    void initializeKnobs(const KnobsVec& knobs, QTreeWidgetItem* parent, std::list<KnobI*>& markedKnobs);

    void rebuildUserPages();

    QTreeWidgetItem* createItemForKnob(const KnobIPtr& knob, int insertIndex = -1);

    void saveAndRebuildPages();

    std::list<TreeItem>::const_iterator findItemForTreeItem(const QTreeWidgetItem* item) const
    {
        for (std::list<TreeItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
            if (it->item == item) {
                return it;
            }
        }

        return items.end();
    }
};

ManageUserParamsDialog::ManageUserParamsDialog(DockablePanel* panel,
                                               QWidget* parent)
    : QDialog(parent)
    , _imp( new ManageUserParamsDialogPrivate(panel) )
{
    EffectInstance* effect = dynamic_cast<EffectInstance*>( panel->getHolder() );
    QString title = QString::fromUtf8("User Parameters");

    if (effect) {
        title += QString::fromUtf8(" for ");
        title += QString::fromUtf8( effect->getScriptName().c_str() );
    }
    setWindowTitle(title);
    _imp->mainLayout = new QHBoxLayout(this);

    _imp->tree = new UserParamsDialogTreeWidget(this);
    _imp->tree->setAlternatingRowColors(true);
    _imp->tree->setFocusPolicy(Qt::NoFocus);
    _imp->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    _imp->tree->setItemsExpandable(true);
    _imp->tree->header()->setStretchLastSection(false);
    _imp->tree->setTextElideMode(Qt::ElideMiddle);
    _imp->tree->setContextMenuPolicy(Qt::CustomContextMenu);
    _imp->tree->header()->setStretchLastSection(true);
    _imp->tree->header()->hide();


    QObject::connect( _imp->tree, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()) );
    QObject::connect( _imp->tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)) );
    std::list<KnobI*> markedKnobs;
    const KnobsVec& knobs = panel->getHolder()->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPage* page = dynamic_cast<KnobPage*>( it->get() );
        if (page) {
            TreeItem pageItem;

            pageItem.item = new QTreeWidgetItem(_imp->tree);
            pageItem.scriptName = QString::fromUtf8( page->getName().c_str() );
            pageItem.item->setText(0, pageItem.scriptName);
            pageItem.knob = *it;
            pageItem.item->setExpanded(true);
            _imp->items.push_back(pageItem);

            if ( (*it)->isUserKnob() ) {
                KnobsVec children = page->getChildren();
                _imp->initializeKnobs(children, pageItem.item, markedKnobs);
            }
        }
    }

    _imp->mainLayout->addWidget(_imp->tree);

    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QVBoxLayout(_imp->buttonsContainer);

    _imp->addButton = new Button(tr("Add..."), _imp->buttonsContainer);
    _imp->addButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add a new parameter, group or page"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddClicked()) );
    _imp->buttonsLayout->addWidget(_imp->addButton);

    _imp->pickButton = new Button(tr("Pick..."), _imp->buttonsContainer);
    _imp->pickButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add a new parameter that is directly copied from/linked to another parameter"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->pickButton, SIGNAL(clicked(bool)), this, SLOT(onPickClicked()) );
    _imp->buttonsLayout->addWidget(_imp->pickButton);

    _imp->editButton = new Button(tr("Edit..."), _imp->buttonsContainer);
    _imp->editButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Edit the selected parameter"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->editButton, SIGNAL(clicked(bool)), this, SLOT(onEditClicked()) );
    _imp->buttonsLayout->addWidget(_imp->editButton);

    _imp->removeButton = new Button(tr("Delete"), _imp->buttonsContainer);
    _imp->removeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Delete the selected parameter"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->removeButton, SIGNAL(clicked(bool)), this, SLOT(onDeleteClicked()) );
    _imp->buttonsLayout->addWidget(_imp->removeButton);

    _imp->upButton = new Button(tr("Up"), _imp->buttonsContainer);
    _imp->upButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Move the selected parameter one level up in the layout"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->upButton, SIGNAL(clicked(bool)), this, SLOT(onUpClicked()) );
    _imp->buttonsLayout->addWidget(_imp->upButton);

    _imp->downButton = new Button(tr("Down"), _imp->buttonsContainer);
    _imp->downButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Move the selected parameter one level down in the layout"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->downButton, SIGNAL(clicked(bool)), this, SLOT(onDownClicked()) );
    _imp->buttonsLayout->addWidget(_imp->downButton);

    _imp->closeButton = new Button(tr("Close"), _imp->buttonsContainer);
    _imp->closeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Close this dialog"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->closeButton, SIGNAL(clicked(bool)), this, SLOT(onCloseClicked()) );
    _imp->buttonsLayout->addWidget(_imp->closeButton);

    _imp->mainLayout->addWidget(_imp->buttonsContainer);
    onSelectionChanged();
}

ManageUserParamsDialog::~ManageUserParamsDialog()
{

}

static QString
createTextForKnob(const KnobIPtr& knob)
{
    QString text = QString::fromUtf8( knob->getName().c_str() );
    KnobI::ListenerDimsMap listeners;

    knob->getListeners(listeners);
    if ( !listeners.empty() ) {
        KnobIPtr listener = listeners.begin()->first.lock();
        if ( listener && (listener->getAliasMaster() == knob) ) {
            text += QString::fromUtf8(" (alias of ");
            EffectInstance* effect = dynamic_cast<EffectInstance*>( listener->getHolder() );
            if (effect) {
                text += QString::fromUtf8( effect->getScriptName_mt_safe().c_str() );
                text += QLatin1Char('.');
            }
            text += QString::fromUtf8( listener->getName().c_str() );
            text += QLatin1Char(')');
        }
    }

    return text;
}

void
ManageUserParamsDialogPrivate::initializeKnobs(const KnobsVec& knobs,
                                               QTreeWidgetItem* parent,
                                               std::list<KnobI*>& markedKnobs)
{
    for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        if ( std::find( markedKnobs.begin(), markedKnobs.end(), it2->get() ) != markedKnobs.end() ) {
            continue;
        }

        markedKnobs.push_back( it2->get() );

        TreeItem i;
        i.knob = *it2;
        i.item = new QTreeWidgetItem(parent);
        i.scriptName = QString::fromUtf8( (*it2)->getName().c_str() );
        i.item->setText( 0, createTextForKnob(i.knob) );
        i.item->setExpanded(true);
        items.push_back(i);

        KnobGroup* isGrp = dynamic_cast<KnobGroup*>( it2->get() );
        if (isGrp) {
            KnobsVec children = isGrp->getChildren();
            initializeKnobs(children, i.item, markedKnobs);
        }
    }
}

void
ManageUserParamsDialogPrivate::rebuildUserPages()
{
    panel->recreateUserKnobs(true);
}

bool
ManageUserParamsDialog::ensureHasUserPage()
{
    std::list<KnobPage*> pages;
    _imp->panel->getUserPages(pages);
    if (!pages.empty()) {
        return true;
    }

    Dialogs::warningDialog(tr("User Page").toStdString(), tr("This node does not have a user page yet. You need to create one "
                                                                                       "to add custom parameters to it.").toStdString());
    return false;
}

void
ManageUserParamsDialog::onPickClicked()
{
    PickKnobDialog dialog(_imp->panel, this);

    if ( dialog.exec() ) {
        bool useAlias;
        KnobPagePtr page;
        KnobGroupPtr group;
        KnobGuiPtr selectedKnob = dialog.getSelectedKnob(&useAlias, &page, &group);
        if (!selectedKnob) {
            return;
        }
        if (!ensureHasUserPage()) {
            return;
        }
        NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(_imp->panel);
        assert(nodePanel);
        if (!nodePanel) {
            throw std::logic_error("ManageUserParamsDialog::onPickClicked");
        }
        NodeGuiPtr nodeGui = nodePanel->getNode();
        assert(nodeGui);
        if (!nodeGui) {
            throw std::logic_error("ManageUserParamsDialog::onPickClicked");
        }
        NodePtr node = nodeGui->getNode();
        assert(node);
        if (!node) {
            throw std::logic_error("ManageUserParamsDialog::onPickClicked");
        }
        KnobIPtr duplicate = selectedKnob->createDuplicateOnNode(node->getEffectInstance().get(), useAlias, page, group, -1);
        if (duplicate) {

            _imp->createItemForKnob(duplicate);

            _imp->saveAndRebuildPages();

        }
    }
}

void
ManageUserParamsDialog::onAddClicked()
{
    if (!ensureHasUserPage()) {
        AddKnobDialog dialog(_imp->panel, KnobIPtr(), std::string(), std::string(), this);
        dialog.setVisibleType(false);
        dialog.setType(AddKnobDialog::eParamDataTypePage);
        dialog.setWindowTitle(tr("Add Page"));
        if (!dialog.exec() ) {
            return;
        }
        KnobIPtr knob = dialog.getKnob();
        _imp->createItemForKnob(knob);

    }
    std::string selectedPageName, selectedGroupName;

    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if ( !selection.isEmpty() ) {
        std::list<TreeItem>::const_iterator item = _imp->findItemForTreeItem( selection.front() );
        if ( item != _imp->items.end() ) {
            KnobPage* isPage = dynamic_cast<KnobPage*>( item->knob.get() );
            if (isPage) {
                selectedPageName = isPage->getName();
            } else {
                KnobGroup* isGrp = dynamic_cast<KnobGroup*>( item->knob.get() );
                if (isGrp) {
                    selectedGroupName = isGrp->getName();
                    KnobPagePtr topLevelPage = isGrp->getTopLevelPage();
                    if (topLevelPage) {
                        selectedPageName = topLevelPage->getName();
                    }
                }
            }
        }
    }

    AddKnobDialog dialog(_imp->panel, KnobIPtr(), selectedPageName, selectedGroupName, this);
    if ( dialog.exec() ) {
        KnobIPtr knob = dialog.getKnob();
        _imp->createItemForKnob(knob);
        _imp->saveAndRebuildPages();
    }
}

QTreeWidgetItem*
ManageUserParamsDialogPrivate::createItemForKnob(const KnobIPtr& knob,
                                                 int insertIndex)
{
    QTreeWidgetItem* parent = 0;
    KnobIPtr parentKnob;

    if (knob) {
        parentKnob = knob->getParentKnob();
    }
    if (parentKnob) {
        for (std::list<TreeItem>::iterator it = items.begin(); it != items.end(); ++it) {
            if ( it->knob.get() == parentKnob.get() ) {
                parent = it->item;
                break;
            }
        }
    }

    KnobPage* isPage = dynamic_cast<KnobPage*>( knob.get() );
    if (!parent && !isPage) {
        //Default to first user page
        for (std::list<TreeItem>::iterator it = items.begin(); it != items.end(); ++it) {
            KnobPage* isPage = dynamic_cast<KnobPage*>(it->knob.get());
            if (!isPage) {
                continue;
            }
            if (!isPage->isUserKnob()) {
                continue;
            }

            parent = it->item;
            break;
        }
    }
    TreeItem i;
    i.knob = knob;
    i.item = new QTreeWidgetItem;
    if (parent) {
        if ( (insertIndex == -1) || ( insertIndex >= parent->childCount() ) ) {
            parent->addChild(i.item);
        } else {
            parent->insertChild(insertIndex, i.item);
        }
        parent->setExpanded(true);
    } else {
        if ( (insertIndex == -1) || ( insertIndex >= tree->topLevelItemCount() ) ) {
            tree->addTopLevelItem(i.item);
        } else {
            tree->insertTopLevelItem(insertIndex, i.item);
        }
    }
    i.item->setExpanded(true);
    i.scriptName = QString::fromUtf8( knob->getName().c_str() );

    i.item->setText( 0, createTextForKnob(i.knob) );
    items.push_back(i);

    return i.item;
} // ManageUserParamsDialogPrivate::createItemForKnob

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
    if ( !selection.isEmpty() ) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    QString question;
                    question.append( tr("Removing") );
                    question += QLatin1Char(' ');
                    question += QString::fromUtf8( it->knob->getName().c_str() );
                    question += QLatin1Char(' ');
                    question += tr("cannot be undone. Are you sure you want to continue?");
                    StandardButtonEnum rep = Dialogs::questionDialog(tr("Remove parameter").toStdString(), question.toStdString(), false,
                                                                     StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonYes);
                    if (rep != eStandardButtonYes) {
                        return;
                    }
                    it->knob->getHolder()->deleteKnob(it->knob.get(), true);
                    delete it->item;
                    _imp->items.erase(it);

                    _imp->saveAndRebuildPages();
                    break;
                }
            }
        }
    }
}

void
ManageUserParamsDialog::onEditClickedInternal(const QList<QTreeWidgetItem*> &selection)
{
    if ( !selection.isEmpty() ) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    KnobIPtr oldParentKnob = it->knob->getParentKnob();
                    AddKnobDialog dialog(_imp->panel, it->knob, std::string(), std::string(), this);
                    if ( dialog.exec() ) {
                        int indexIndParent = -1;
                        QTreeWidgetItem* parent = it->item->parent();
                        KnobIPtr knob = dialog.getKnob();
                        KnobIPtr newParentKnob = knob->getParentKnob();
                        KnobPage* isPage = dynamic_cast<KnobPage*>( it->knob.get() );

                        if (parent) {
                            if (oldParentKnob != newParentKnob) {
                                indexIndParent = -1;
                            } else {
                                indexIndParent = parent->indexOfChild(it->item);
                            }
                        } else {
                            if (isPage) {
                                indexIndParent = isPage->getHolder()->getPageIndex(isPage);
                            }
                        }

                        QList<QTreeWidgetItem*> children = it->item->takeChildren();

                        delete it->item;
                        _imp->items.erase(it);
                        QTreeWidgetItem* item = _imp->createItemForKnob(knob, indexIndParent);
                        if ( item && !children.isEmpty() ) {
                            item->insertChildren(0, children);
                        }
                        if (!isPage) {
                            //Nothing is to be rebuilt when editing a page
                            _imp->saveAndRebuildPages();
                        }
                        QItemSelectionModel* model = _imp->tree->selectionModel();
                        model->select(_imp->tree->indexFromItemPublic(item), QItemSelectionModel::ClearAndSelect);

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
    if ( (e->key() == Qt::Key_Delete) || (e->key() == Qt::Key_Backspace) ) {
        if ( _imp->removeButton->isEnabled() ) {
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
    if ( selection.empty() ) {
        return;
    }
    QTreeWidgetItem* selectedItem = selection.front();
    for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->item == selectedItem) {
            QTreeWidgetItem* parent = 0;
            KnobIPtr parentKnob;
            if (it->knob) {
                parentKnob = it->knob->getParentKnob();
            }
            if (parentKnob) {
                for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                    if (it->knob == parentKnob) {
                        parent = it->item;
                        break;
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

            if ( !it->knob || !_imp->panel->getHolder()->moveKnobOneStepUp( it->knob.get() ) ) {
                return;
            }
            QList<QTreeWidgetItem*> children = it->item->takeChildren();
            delete it->item;

            KnobIPtr knob = it->knob;
            //invalidates iterator
            _imp->items.erase(it);

            QTreeWidgetItem* item = _imp->createItemForKnob(knob, index - 1);
            item->insertChildren(0, children);
            _imp->saveAndRebuildPages();
            QItemSelectionModel* model = _imp->tree->selectionModel();
            model->select(_imp->tree->indexFromItemPublic(item), QItemSelectionModel::ClearAndSelect);

            KnobPagePtr isPage = boost::dynamic_pointer_cast<KnobPage>(knob);
            if (isPage) {
                _imp->panel->setPageActiveIndex(isPage);
            }

            break;
        }
    }
} // ManageUserParamsDialog::onUpClicked

void
ManageUserParamsDialog::onDownClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if ( selection.empty() ) {
        return;
    }
    QTreeWidgetItem* selectedItem = selection.front();

    for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->item == selectedItem) {
            //Find the parent item
            QTreeWidgetItem* parent = 0;
            KnobIPtr parentKnob;
            if (it->knob) {
                parentKnob = it->knob->getParentKnob();
            }
            if (parentKnob) {
                for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                    if (it->knob == parentKnob) {
                        parent = it->item;
                        break;
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
                moveOk = _imp->panel->getHolder()->moveKnobOneStepDown( it->knob.get() );
            }
            if (!moveOk) {
                break;
            }

            QList<QTreeWidgetItem*> children = it->item->takeChildren();
            delete it->item;
            KnobIPtr knob = it->knob;
            //invalidates iterator
            _imp->items.erase(it);

            QTreeWidgetItem* item = _imp->createItemForKnob(knob, index + 1);
            item->insertChildren(0, children);
            _imp->saveAndRebuildPages();
            QItemSelectionModel* model = _imp->tree->selectionModel();
            model->select(_imp->tree->indexFromItemPublic(item), QItemSelectionModel::ClearAndSelect);

            KnobPagePtr isPage = boost::dynamic_pointer_cast<KnobPage>(knob);
            if (isPage) {
                _imp->panel->setPageActiveIndex(isPage);
            }

            break;
        }
    }
} // ManageUserParamsDialog::onDownClicked

void
ManageUserParamsDialog::onCloseClicked()
{
    accept();
}

void
ManageUserParamsDialog::onItemDoubleClicked(QTreeWidgetItem *item,
                                            int /*column*/)
{
    QList<QTreeWidgetItem*> selection;
    if (item) {
        selection.push_back(item);
    }
    if ( !_imp->editButton->isEnabled() ) {
        return;
    }
    onEditClickedInternal(selection);
}

void
ManageUserParamsDialog::onSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    bool canEdit = true;
    bool canDelete = true;
    if ( !selection.isEmpty() ) {
        QTreeWidgetItem* item = selection[0];
        for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            if (it->item == item) {
                KnobPage* isPage = dynamic_cast<KnobPage*>( it->knob.get() );
                KnobGroup* isGroup = dynamic_cast<KnobGroup*>( it->knob.get() );
                if (isPage) {
                    if ( !isPage->isUserKnob() ) {
                        canDelete = false;
                        canEdit = false;
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

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ManageUserParamsDialog.cpp"
