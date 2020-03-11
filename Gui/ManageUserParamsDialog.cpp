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
#include "Engine/ViewerNode.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/AddKnobDialog.h"
#include "Gui/Button.h"
#include "Gui/EditNodeViewerContextDialog.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/Label.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PickKnobDialog.h"
#include "Gui/Splitter.h"

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

struct TreeItem
{
    QTreeWidgetItem* item;
    KnobIPtr knob;
    QString scriptName;
};

static QString
createTextForKnob(const KnobIPtr& knob)
{
    QString text = QString::fromUtf8( knob->getName().c_str() );
    return text;
}

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
    UserParamsDialogTreeWidget* knobsTree, *viewerUITree;
    QWidget* treesContainer;
    QVBoxLayout* treesLayout;
    QFrame* treesHeader;
    Label* treesHeaderLabel;
    QFrame* treesSeparator;
    Label* separatorLabel;
    std::list<TreeItem> knobsItems, viewerUIItems;
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
        , knobsTree(0)
        , viewerUITree(0)
        , treesContainer(0)
        , treesLayout(0)
        , treesHeader(0)
        , treesHeaderLabel(0)
        , treesSeparator(0)
        , separatorLabel(0)
        , knobsItems()
        , viewerUIItems()
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

    void initializeKnobs(const KnobsVec& knobs, QTreeWidgetItem* parent, std::list<KnobIPtr>& markedKnobs);

    void rebuildUserPages();

    void rebuildViewerUI();

    QTreeWidgetItem* createItemForKnob(const KnobIPtr& knob, int insertIndex = -1);

    void saveAndRebuildPages();

    void saveAndRebuildViewerUI();

    void moveKnobItemUp(std::list<TreeItem>::iterator selectedItem);
    void moveKnobItemDown(std::list<TreeItem>::iterator selectedItem);

    void moveViewerUIItemUp(std::list<TreeItem>::iterator selectedItem);
    void moveViewerUIItemDown(std::list<TreeItem>::iterator selectedItem);

    void deleteKnobItem(std::list<TreeItem>::iterator selectedItem);
    void deleteViewerUIItem(std::list<TreeItem>::iterator selectedItem);


    std::list<TreeItem>::const_iterator findItemForTreeItem(const QTreeWidgetItem* item) const
    {
        for (std::list<TreeItem>::const_iterator it = knobsItems.begin(); it != knobsItems.end(); ++it) {
            if (it->item == item) {
                return it;
            }
        }

        return knobsItems.end();
    }
};

ManageUserParamsDialog::ManageUserParamsDialog(DockablePanel* panel,
                                               QWidget* parent)
    : QDialog(parent)
    , _imp( new ManageUserParamsDialogPrivate(panel) )
{
    EffectInstancePtr effect = toEffectInstance( panel->getHolder() );
    QString title = QString::fromUtf8("User Parameters");

    if (effect) {
        title += QString::fromUtf8(" for ");
        title += QString::fromUtf8( effect->getScriptName().c_str() );
    }
    setWindowTitle(title);
    _imp->mainLayout = new QHBoxLayout(this);

    _imp->treesContainer = new QWidget(this);
    _imp->treesLayout = new QVBoxLayout(_imp->treesContainer);
    _imp->mainLayout->addWidget(_imp->treesContainer);

    {
        QWidget* separatorContainer = new QWidget(_imp->treesContainer);
        QHBoxLayout* separatorContainerLayout = new QHBoxLayout(separatorContainer);
        _imp->treesHeaderLabel = new Label(tr("Settings Panel"), separatorContainer);
        separatorContainerLayout->addWidget(_imp->treesHeaderLabel);

        _imp->treesHeader = new QFrame(separatorContainer);
        _imp->treesHeader->setFixedHeight( TO_DPIY(2) );
        _imp->treesHeader->setGeometry( 0, 0, TO_DPIX(300), TO_DPIY(2) );
        _imp->treesHeader->setFrameShape(QFrame::HLine);
        _imp->treesHeader->setFrameShadow(QFrame::Sunken);
        _imp->treesHeader->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        separatorContainerLayout->addWidget(_imp->treesHeader);

        _imp->treesLayout->addWidget(separatorContainer);
    }

    _imp->knobsTree = new UserParamsDialogTreeWidget(this);
    _imp->knobsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->knobsTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    _imp->knobsTree->setItemsExpandable(true);
    _imp->knobsTree->header()->setStretchLastSection(false);
    _imp->knobsTree->setTextElideMode(Qt::ElideMiddle);
    _imp->knobsTree->setContextMenuPolicy(Qt::CustomContextMenu);
    _imp->knobsTree->header()->setStretchLastSection(true);
    _imp->knobsTree->header()->hide();


    QObject::connect( _imp->knobsTree, SIGNAL(itemSelectionChanged()), this, SLOT(onKnobsTreeSelectionChanged()) );
    QObject::connect( _imp->knobsTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onKnobsTreeItemDoubleClicked(QTreeWidgetItem*,int)) );
    std::list<KnobIPtr> markedKnobs;

    const KnobsVec& knobs = panel->getHolder()->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPagePtr page = toKnobPage(*it);
        if (page) {
            TreeItem pageItem;
            pageItem.item = new QTreeWidgetItem(_imp->knobsTree);
            pageItem.scriptName = QString::fromUtf8( page->getName().c_str() );
            pageItem.item->setText(0, pageItem.scriptName);
            pageItem.knob = *it;
            pageItem.item->setExpanded(true);
            _imp->knobsItems.push_back(pageItem);


            if ( (*it)->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser ) {
                KnobsVec children = page->getChildren();
                _imp->initializeKnobs(children, pageItem.item, markedKnobs);
            }
        }
    }

    _imp->treesLayout->addWidget(_imp->knobsTree);

    {
        QWidget* separatorContainer = new QWidget(_imp->treesContainer);
        QHBoxLayout* separatorContainerLayout = new QHBoxLayout(separatorContainer);
        _imp->separatorLabel = new Label(tr("Viewer Interface"), separatorContainer);
        separatorContainerLayout->addWidget(_imp->separatorLabel);

        _imp->treesSeparator = new QFrame(separatorContainer);
        _imp->treesSeparator->setFixedHeight( TO_DPIY(2) );
        _imp->treesSeparator->setGeometry( 0, 0, TO_DPIX(300), TO_DPIY(2) );
        _imp->treesSeparator->setFrameShape(QFrame::HLine);
        _imp->treesSeparator->setFrameShadow(QFrame::Sunken);
        _imp->treesSeparator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        separatorContainerLayout->addWidget(_imp->treesSeparator);

        _imp->treesLayout->addWidget(separatorContainer);
    }

    _imp->viewerUITree = new UserParamsDialogTreeWidget(this);
    _imp->viewerUITree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->viewerUITree->setSelectionBehavior(QAbstractItemView::SelectRows);
    _imp->viewerUITree->setItemsExpandable(true);
    _imp->viewerUITree->header()->setStretchLastSection(false);
    _imp->viewerUITree->setTextElideMode(Qt::ElideMiddle);
    _imp->viewerUITree->setContextMenuPolicy(Qt::CustomContextMenu);
    _imp->viewerUITree->header()->setStretchLastSection(true);
    _imp->viewerUITree->header()->hide();

    QObject::connect( _imp->viewerUITree, SIGNAL(itemSelectionChanged()), this, SLOT(onViewerTreeSelectionChanged()) );
    QObject::connect( _imp->viewerUITree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onViewerTreeItemDoubleClicked(QTreeWidgetItem*,int)) );

    KnobsVec viewerUIKnobs = panel->getHolder()->getViewerUIKnobs();
    for (KnobsVec::const_iterator it = viewerUIKnobs.begin(); it != viewerUIKnobs.end(); ++it) {
        TreeItem treeItem;
        treeItem.item = new QTreeWidgetItem(_imp->viewerUITree);
        treeItem.scriptName = createTextForKnob(*it);
        treeItem.item->setText(0, treeItem.scriptName);
        treeItem.knob = *it;
        treeItem.item->setExpanded(true);
        _imp->viewerUIItems.push_back(treeItem);
    }

    _imp->treesLayout->addWidget(_imp->viewerUITree);

    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QVBoxLayout(_imp->buttonsContainer);
    _imp->buttonsLayout->addStretch();
    _imp->addButton = new Button(tr("Add..."), _imp->buttonsContainer);
    _imp->addButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add a new parameter, group or page"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddClicked()) );
    _imp->buttonsLayout->addWidget(_imp->addButton);

    _imp->pickButton = new Button(tr("Pick..."), _imp->buttonsContainer);
    _imp->pickButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add a new parameter that is directly copied from/linked to another parameter"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->pickButton, SIGNAL(clicked(bool)), this, SLOT(onPickClicked()) );
    _imp->buttonsLayout->addWidget(_imp->pickButton);

    _imp->buttonsLayout->addStretch();

    _imp->editButton = new Button(tr("Edit..."), _imp->buttonsContainer);
    _imp->editButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Edit the selected parameter"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _imp->editButton, SIGNAL(clicked(bool)), this, SLOT(onEditClicked()) );
    _imp->buttonsLayout->addWidget(_imp->editButton);

    _imp->removeButton = new Button(tr("Remove"), _imp->buttonsContainer);
    _imp->removeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove the selected parameter"), NATRON_NAMESPACE::WhiteSpaceNormal) );
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
    _imp->buttonsLayout->addStretch();
    _imp->mainLayout->addWidget(_imp->buttonsContainer);
    onKnobsTreeSelectionChanged();

}

ManageUserParamsDialog::~ManageUserParamsDialog()
{

}



void
ManageUserParamsDialogPrivate::initializeKnobs(const KnobsVec& knobs,
                                               QTreeWidgetItem* parent,
                                               std::list<KnobIPtr>& markedKnobs)
{
    for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        if ( std::find( markedKnobs.begin(), markedKnobs.end(), *it2 ) != markedKnobs.end() ) {
            continue;
        }

        markedKnobs.push_back(*it2);

        TreeItem i;
        i.knob = *it2;
        i.item = new QTreeWidgetItem(parent);
        i.scriptName = QString::fromUtf8( (*it2)->getName().c_str() );
        i.item->setText( 0, createTextForKnob(i.knob) );
        i.item->setExpanded(true);
        knobsItems.push_back(i);

        KnobGroupPtr isGrp = toKnobGroup(*it2);
        if (isGrp) {
            KnobsVec children = isGrp->getChildren();
            initializeKnobs(children, i.item, markedKnobs);
        }
    }
}

void
ManageUserParamsDialogPrivate::rebuildViewerUI()
{
    panel->recreateViewerUIKnobs();
}

void
ManageUserParamsDialogPrivate::rebuildUserPages()
{
    panel->recreateUserKnobs(true);
}

bool
ManageUserParamsDialog::ensureHasUserPage()
{
    std::list<KnobPagePtr> pages;
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
        NodeGuiPtr nodeGui = nodePanel->getNodeGui();
        assert(nodeGui);
        if (!nodeGui) {
            throw std::logic_error("ManageUserParamsDialog::onPickClicked");
        }
        NodePtr node = nodeGui->getNode();
        assert(node);
        if (!node) {
            throw std::logic_error("ManageUserParamsDialog::onPickClicked");
        }
        KnobIPtr duplicate = selectedKnob->createDuplicateOnNode(node->getEffectInstance(), useAlias, page, group, -1);
        if (duplicate) {

            _imp->createItemForKnob(duplicate);

            _imp->saveAndRebuildPages();

            // Adjust viewer UI
            int viewerContextIndex = duplicate->getHolder()->getInViewerContextKnobIndex(duplicate);
            if (viewerContextIndex != -1) {
                TreeItem treeIt;

                treeIt.knob = duplicate;
                treeIt.item  = new QTreeWidgetItem;
                treeIt.scriptName = createTextForKnob(duplicate);
                treeIt.item->setText(0, treeIt.scriptName);
                treeIt.item->setExpanded(true);
                if ( (viewerContextIndex == -1) || ( viewerContextIndex >= _imp->viewerUITree->topLevelItemCount() ) ) {
                    _imp->viewerUITree->addTopLevelItem(treeIt.item);
                } else {
                    _imp->viewerUITree->insertTopLevelItem(viewerContextIndex, treeIt.item);
                }
                
                _imp->rebuildViewerUI();
            }

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

    QList<QTreeWidgetItem*> selection = _imp->knobsTree->selectedItems();
    if ( !selection.isEmpty() ) {
        std::list<TreeItem>::const_iterator item = _imp->findItemForTreeItem( selection.front() );
        if ( item != _imp->knobsItems.end() ) {
            KnobPagePtr isPage = toKnobPage( item->knob );
            if (isPage) {
                selectedPageName = isPage->getName();
            } else {
                KnobGroupPtr isGrp = toKnobGroup( item->knob );
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


        // Adjust viewer UI
        int viewerContextIndex = knob->getHolder()->getInViewerContextKnobIndex(knob);
        if (viewerContextIndex != -1) {
            TreeItem treeIt;

            treeIt.knob = knob;
            treeIt.item  = new QTreeWidgetItem;
            treeIt.scriptName = createTextForKnob(knob);
            treeIt.item->setText(0, treeIt.scriptName);
            treeIt.item->setExpanded(true);
            if ( (viewerContextIndex == -1) || ( viewerContextIndex >= _imp->viewerUITree->topLevelItemCount() ) ) {
                _imp->viewerUITree->addTopLevelItem(treeIt.item);
            } else {
                _imp->viewerUITree->insertTopLevelItem(viewerContextIndex, treeIt.item);
            }
            _imp->viewerUIItems.push_back(treeIt);
            _imp->rebuildViewerUI();
        }
        
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
        for (std::list<TreeItem>::iterator it = knobsItems.begin(); it != knobsItems.end(); ++it) {
            if ( it->knob == parentKnob ) {
                parent = it->item;
                break;
            }
        }
    }

    KnobPagePtr isPage = toKnobPage(knob);
    if (!parent && !isPage) {
        //Default to first user page
        for (std::list<TreeItem>::iterator it = knobsItems.begin(); it != knobsItems.end(); ++it) {
            KnobPagePtr isPage = toKnobPage(it->knob);
            if (!isPage) {
                continue;
            }
            if (isPage->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser) {
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
        if ( (insertIndex == -1) || ( insertIndex >= knobsTree->topLevelItemCount() ) ) {
            knobsTree->addTopLevelItem(i.item);
        } else {
            knobsTree->insertTopLevelItem(insertIndex, i.item);
        }
    }
    i.item->setExpanded(true);
    i.scriptName = QString::fromUtf8( knob->getName().c_str() );

    i.item->setText( 0, createTextForKnob(i.knob) );
    knobsItems.push_back(i);

    return i.item;
} // ManageUserParamsDialogPrivate::createItemForKnob

void
ManageUserParamsDialogPrivate::saveAndRebuildPages()
{
    rebuildUserPages();
    panel->getGui()->getApp()->triggerAutoSave();
}

void
ManageUserParamsDialogPrivate::saveAndRebuildViewerUI()
{
    rebuildViewerUI();
    panel->getGui()->getApp()->triggerAutoSave();
}

void
ManageUserParamsDialogPrivate::deleteKnobItem(std::list<TreeItem>::iterator selectedItem)
{

    selectedItem->knob->getHolder()->deleteKnob(selectedItem->knob, true);
    delete selectedItem->item;
    knobsItems.erase(selectedItem);

    saveAndRebuildPages();

}

void
ManageUserParamsDialogPrivate::deleteViewerUIItem(std::list<TreeItem>::iterator selectedItem)
{
    panel->getHolder()->removeKnobViewerUI(selectedItem->knob);
    delete selectedItem->item;
    knobsItems.erase(selectedItem);
    saveAndRebuildViewerUI();
}

void
ManageUserParamsDialog::onDeleteClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->knobsTree->selectedItems();
    if ( !selection.isEmpty() ) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->knobsItems.begin(); it != _imp->knobsItems.end(); ++it) {
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
                    _imp->deleteKnobItem(it);

                    break;
                }
            }
        }
    } else {
        selection = _imp->viewerUITree->selectedItems();
        if (!selection.empty()) {
            QTreeWidgetItem* selectedItem = selection.front();
            for (std::list<TreeItem>::iterator it = _imp->viewerUIItems.begin(); it != _imp->viewerUIItems.end(); ++it) {
                if (it->item == selectedItem) {
                    _imp->deleteViewerUIItem(it);
                    break;
                }
            }

        }
    }
}

void
ManageUserParamsDialog::onViewerUIEditClickedInternal(const QList<QTreeWidgetItem*> &selection)
{
    if (selection.isEmpty()) {
        return;
    }
    QTreeWidgetItem* selectedItem = selection.front();
    for (std::list<TreeItem>::iterator it = _imp->viewerUIItems.begin(); it != _imp->viewerUIItems.end(); ++it) {
        if (it->item == selectedItem) {
            EditNodeViewerContextDialog dialog(it->knob, this);
            if (dialog.exec()) {
                _imp->saveAndRebuildViewerUI();
            }
            break;
        }
    }

}

void
ManageUserParamsDialog::onKnobsEditClickedInternal(const QList<QTreeWidgetItem*> &selection)
{
    if ( !selection.isEmpty() ) {
        QTreeWidgetItem* selectedItem = selection.front();
        for (std::list<TreeItem>::iterator it = _imp->knobsItems.begin(); it != _imp->knobsItems.end(); ++it) {
            if (it->item == selectedItem) {
                KnobIPtr oldParentKnob = it->knob->getParentKnob();
                KnobIPtr oldKnob = it->knob;
                AddKnobDialog dialog(_imp->panel, it->knob, std::string(), std::string(), this);
                if ( dialog.exec() ) {
                    int indexIndParent = -1;
                    QTreeWidgetItem* parent = it->item->parent();
                    KnobIPtr knob = dialog.getKnob();
                    KnobIPtr newParentKnob = knob->getParentKnob();
                    KnobPagePtr isPage = toKnobPage( it->knob );

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
                    _imp->knobsItems.erase(it);
                    QTreeWidgetItem* item = _imp->createItemForKnob(knob, indexIndParent);
                    if ( item && !children.isEmpty() ) {
                        item->insertChildren(0, children);
                    }
                    if (!isPage) {
                        //Nothing is to be rebuilt when editing a page
                        _imp->saveAndRebuildPages();
                    }
                    QItemSelectionModel* model = _imp->knobsTree->selectionModel();
                    model->select(_imp->knobsTree->indexFromItemPublic(item), QItemSelectionModel::ClearAndSelect);

                    // Adjust the viewer ui tree
                    int newViewerContextIndex = _imp->panel->getHolder()->getInViewerContextKnobIndex(knob);
                    for (std::list<TreeItem>::iterator it2 = _imp->viewerUIItems.begin(); it2 != _imp->viewerUIItems.end(); ++it2) {
                        if (it2->knob == oldKnob) {
                            it2->knob = knob;
                            // Remove the old item from the tree
                            delete it2->item;
                            _imp->viewerUIItems.erase(it2);
                            break;
                        }
                    }

                    if (newViewerContextIndex != -1) {
                        TreeItem treeIt;

                        treeIt.knob = knob;
                        treeIt.item  = new QTreeWidgetItem;
                        treeIt.scriptName = createTextForKnob(knob);
                        treeIt.item->setText(0, treeIt.scriptName);
                        treeIt.item->setExpanded(true);
                        if ( (newViewerContextIndex == -1) || ( newViewerContextIndex >= _imp->viewerUITree->topLevelItemCount() ) ) {
                            _imp->viewerUITree->addTopLevelItem(treeIt.item);
                        } else {
                            _imp->viewerUITree->insertTopLevelItem(newViewerContextIndex, treeIt.item);
                        }
                        _imp->viewerUIItems.push_back(treeIt);
                        _imp->rebuildViewerUI();
                    }
                    break;
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
    QList<QTreeWidgetItem*> selection = _imp->knobsTree->selectedItems();
    if (!selection.empty()) {
        onKnobsEditClickedInternal(selection);
    } else {
        selection = _imp->viewerUITree->selectedItems();
        if (!selection.empty()) {
            onViewerUIEditClickedInternal(selection);
        }
    }

}

void
ManageUserParamsDialogPrivate::moveViewerUIItemUp(std::list<TreeItem>::iterator selectedItem)
{
    int index = viewerUITree->indexOfTopLevelItem(selectedItem->item);

    if (index == 0) {
        return;
    }


    if ( !selectedItem->knob) {
        return;
    }
    if (!panel->getHolder()->moveViewerUIKnobOneStepUp( selectedItem->knob ) ) {
        return;
    }

    QList<QTreeWidgetItem*> children = selectedItem->item->takeChildren();
    delete selectedItem->item;

    KnobIPtr knob = selectedItem->knob;
    //invalidates iterator
    viewerUIItems.erase(selectedItem);

    TreeItem treeIt;

    treeIt.knob = knob;
    treeIt.scriptName = createTextForKnob(knob);
    treeIt.item  = new QTreeWidgetItem;
    treeIt.item->setText(0, treeIt.scriptName);
    treeIt.item->setExpanded(true);
    treeIt.item->insertChildren(0, children);
    if ( (index == -1) || ( (index - 1) >= viewerUITree->topLevelItemCount() ) ) {
        viewerUITree->addTopLevelItem(treeIt.item);
    } else {
        viewerUITree->insertTopLevelItem(index - 1, treeIt.item);
    }

    viewerUIItems.push_back(treeIt);
    saveAndRebuildViewerUI();
    QItemSelectionModel* model = viewerUITree->selectionModel();
    model->select(viewerUITree->indexFromItemPublic(treeIt.item), QItemSelectionModel::ClearAndSelect);

} // ManageUserParamsDialogPrivate::moveViewerUIItemUp

void
ManageUserParamsDialogPrivate::moveViewerUIItemDown(std::list<TreeItem>::iterator selectedItem)
{
    int index = viewerUITree->indexOfTopLevelItem(selectedItem->item);

    if (index == viewerUITree->topLevelItemCount() - 1) {
        return;
    }

    if ( !selectedItem->knob) {
        return;
    }
    if (!panel->getHolder()->moveViewerUIOneStepDown( selectedItem->knob ) ) {
        return;
    }

    QList<QTreeWidgetItem*> children = selectedItem->item->takeChildren();
    delete selectedItem->item;

    KnobIPtr knob = selectedItem->knob;
    //invalidates iterator
    viewerUIItems.erase(selectedItem);

    TreeItem treeIt;

    treeIt.knob = knob;
    treeIt.item  = new QTreeWidgetItem;
    treeIt.scriptName = createTextForKnob(knob);
    treeIt.item->setText(0, treeIt.scriptName);
    treeIt.item->setExpanded(true);
    treeIt.item->insertChildren(0, children);
    if ( (index == -1) || ( (index + 1) >= viewerUITree->topLevelItemCount() ) ) {
        viewerUITree->addTopLevelItem(treeIt.item);
    } else {
        viewerUITree->insertTopLevelItem(index + 1, treeIt.item);
    }

    viewerUIItems.push_back(treeIt);
    saveAndRebuildViewerUI();
    QItemSelectionModel* model = viewerUITree->selectionModel();
    model->select(viewerUITree->indexFromItemPublic(treeIt.item), QItemSelectionModel::ClearAndSelect);

} // ManageUserParamsDialogPrivate::moveViewerUIItemDown

void
ManageUserParamsDialogPrivate::moveKnobItemUp(std::list<TreeItem>::iterator selectedItem)
{
    QTreeWidgetItem* parent = 0;
    KnobIPtr parentKnob;
    if (selectedItem->knob) {
        parentKnob = selectedItem->knob->getParentKnob();
    }
    if (parentKnob) {

        for (std::list<TreeItem>::iterator it = knobsItems.begin(); it != knobsItems.end(); ++it) {
            if (selectedItem->knob == parentKnob) {
                parent = selectedItem->item;
                break;
            }

        }
    }

    int index;
    if (!parent) {
        index = knobsTree->indexOfTopLevelItem(selectedItem->item);
    } else {
        index = parent->indexOfChild(selectedItem->item);
    }
    if (index == 0) {
        return;
    }

    if ( !selectedItem->knob || !panel->getHolder()->moveKnobOneStepUp( selectedItem->knob ) ) {
        return;
    }
    QList<QTreeWidgetItem*> children = selectedItem->item->takeChildren();
    delete selectedItem->item;

    KnobIPtr knob = selectedItem->knob;
    //invalidates iterator
    knobsItems.erase(selectedItem);

    QTreeWidgetItem* item = createItemForKnob(knob, index - 1);
    item->insertChildren(0, children);
    saveAndRebuildPages();
    QItemSelectionModel* model = knobsTree->selectionModel();
    model->select(knobsTree->indexFromItemPublic(item), QItemSelectionModel::ClearAndSelect);

    KnobPagePtr isPage = toKnobPage(knob);
    if (isPage) {
        panel->setPageActiveIndex(isPage);
    }

} // ManageUserParamsDialogPrivate::moveKnobItemUp

void
ManageUserParamsDialogPrivate::moveKnobItemDown(std::list<TreeItem>::iterator selectedItem)
{
    //Find the parent item
    QTreeWidgetItem* parent = 0;
    KnobIPtr parentKnob;
    if (selectedItem->knob) {
        parentKnob = selectedItem->knob->getParentKnob();
    }
    if (parentKnob) {
        if (parentKnob->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
            for (std::list<TreeItem>::iterator it = knobsItems.begin(); it != knobsItems.end(); ++it) {
                if ( selectedItem->scriptName == QString::fromUtf8(NATRON_USER_MANAGED_KNOBS_PAGE) ) {
                    parent = selectedItem->item;
                    break;
                }
            }
        } else {
            for (std::list<TreeItem>::iterator it = knobsItems.begin(); it != knobsItems.end(); ++it) {
                if (selectedItem->knob == parentKnob) {
                    parent = selectedItem->item;
                    break;
                }
            }
        }
    }
    int index;

    if (!parent) {
        index = knobsTree->indexOfTopLevelItem(selectedItem->item);
        if (index == knobsTree->topLevelItemCount() - 1) {
            return;
        }
    } else {
        index = parent->indexOfChild(selectedItem->item);
        if (index == parent->childCount() - 1) {
            return;
        }
    }

    bool moveOk = false;
    if (selectedItem->knob) {
        moveOk = panel->getHolder()->moveKnobOneStepDown( selectedItem->knob );
    }
    if (!moveOk) {
        return;
    }

    QList<QTreeWidgetItem*> children = selectedItem->item->takeChildren();
    delete selectedItem->item;
    KnobIPtr knob = selectedItem->knob;
    //invalidates iterator
    knobsItems.erase(selectedItem);

    QTreeWidgetItem* item = createItemForKnob(knob, index + 1);
    item->insertChildren(0, children);
    saveAndRebuildPages();
    QItemSelectionModel* model = knobsTree->selectionModel();
    model->select(knobsTree->indexFromItemPublic(item), QItemSelectionModel::ClearAndSelect);

    KnobPagePtr isPage = toKnobPage(knob);
    if (isPage) {
        panel->setPageActiveIndex(isPage);
    }

} // ManageUserParamsDialogPrivate::moveKnobItemDown

void
ManageUserParamsDialog::onUpClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->knobsTree->selectedItems();
    if ( !selection.empty() ) {
        QTreeWidgetItem* selectedItem = selection.front();
        for (std::list<TreeItem>::iterator it = _imp->knobsItems.begin(); it != _imp->knobsItems.end(); ++it) {
            if (it->item == selectedItem) {
                _imp->moveKnobItemUp(it);
                break;
            }
        }
    } else {
        selection = _imp->viewerUITree->selectedItems();
        if (!selection.empty()) {
            QTreeWidgetItem* selectedItem = selection.front();
            for (std::list<TreeItem>::iterator it = _imp->viewerUIItems.begin(); it != _imp->viewerUIItems.end(); ++it) {
                if (it->item == selectedItem) {
                    _imp->moveViewerUIItemUp(it);
                    break;
                }
            }
        }
    }
} // ManageUserParamsDialog::onUpClicked

void
ManageUserParamsDialog::onDownClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->knobsTree->selectedItems();
    if ( !selection.empty() ) {
        QTreeWidgetItem* selectedItem = selection.front();
        for (std::list<TreeItem>::iterator it = _imp->knobsItems.begin(); it != _imp->knobsItems.end(); ++it) {
            if (it->item == selectedItem) {
                _imp->moveKnobItemDown(it);
                break;
            }
        }

    } else {
        selection = _imp->viewerUITree->selectedItems();
        if (!selection.empty()) {
            QTreeWidgetItem* selectedItem = selection.front();
            for (std::list<TreeItem>::iterator it = _imp->viewerUIItems.begin(); it != _imp->viewerUIItems.end(); ++it) {
                if (it->item == selectedItem) {
                    _imp->moveViewerUIItemDown(it);
                    break;
                }
            }
        }
    }
} // ManageUserParamsDialog::onDownClicked

void
ManageUserParamsDialog::onCloseClicked()
{
    accept();
}

void
ManageUserParamsDialog::onKnobsTreeItemDoubleClicked(QTreeWidgetItem *item,
                                            int /*column*/)
{
    QList<QTreeWidgetItem*> selection;
    if (item) {
        selection.push_back(item);
    }
    if ( !_imp->editButton->isEnabled() ) {
        return;
    }
    onKnobsEditClickedInternal(selection);
}

void
ManageUserParamsDialog::onViewerTreeItemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    QList<QTreeWidgetItem*> selection;
    if (item) {
        selection.push_back(item);
    }
    if ( !_imp->editButton->isEnabled() ) {
        return;
    }
    onViewerUIEditClickedInternal(selection);
}

void
ManageUserParamsDialog::onKnobsTreeSelectionChanged()
{

    _imp->viewerUITree->blockSignals(true);
    _imp->viewerUITree->clearSelection();
    _imp->viewerUITree->blockSignals(false);

    QList<QTreeWidgetItem*> selection = _imp->knobsTree->selectedItems();
    bool canEdit = true;
    bool canDelete = true;
    if ( !selection.isEmpty() ) {
        QTreeWidgetItem* item = selection[0];

        for (std::list<TreeItem>::iterator it = _imp->knobsItems.begin(); it != _imp->knobsItems.end(); ++it) {

            if (it->item == item) {
                KnobPagePtr isPage = toKnobPage( it->knob );
                KnobGroupPtr isGroup = toKnobGroup( it->knob );
                if (isPage) {
                    if ( isPage->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser ) {
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


void
ManageUserParamsDialog::onViewerTreeSelectionChanged()
{
    _imp->knobsTree->blockSignals(true);
    _imp->knobsTree->clearSelection();
    _imp->knobsTree->blockSignals(false);

    QList<QTreeWidgetItem*> selection = _imp->viewerUITree->selectedItems();
    
    _imp->removeButton->setEnabled(selection.size() == 1);
    _imp->editButton->setEnabled(selection.size() == 1);
    _imp->upButton->setEnabled(selection.size() == 1);
    _imp->downButton->setEnabled(selection.size() == 1);
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_ManageUserParamsDialog.cpp"
