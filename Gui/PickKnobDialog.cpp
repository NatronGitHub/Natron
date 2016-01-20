/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "PickKnobDialog.h"

#include "Global/Macros.h"

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include <QGridLayout>
#include <QCheckBox>
#include <QTimer>
#include <QDialogButtonBox>

#include "Engine/Knob.h" // KnobI
#include "Engine/KnobTypes.h" // KnobButton...
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodeCollection

#include "Gui/ComboBox.h"
#include "Gui/Label.h"
#include "Gui/NodeCreationDialog.h" // CompleterLineEdit
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/Utils.h" // convertFromPlainText

NATRON_NAMESPACE_ENTER;

struct PickKnobDialogPrivate
{
    
    DockablePanel* panel;
    QGridLayout* mainLayout;
    Label* selectNodeLabel;
    CompleterLineEdit* nodeSelectionCombo;
    ComboBox* knobSelectionCombo;
    Label* useAliasLabel;
    QCheckBox* useAliasCheckBox;
    Label* destPageLabel;
    ComboBox* destPageCombo;
    Label* groupLabel;
    ComboBox* groupCombo;
    std::vector<boost::shared_ptr<KnobPage> > pages;
    std::vector<boost::shared_ptr<KnobGroup> > groups;
    QDialogButtonBox* buttons;
    NodeList allNodes;
    std::map<QString,boost::shared_ptr<KnobI > > allKnobs;
    
    PickKnobDialogPrivate(DockablePanel* panel)
    : panel(panel)
    , mainLayout(0)
    , selectNodeLabel(0)
    , nodeSelectionCombo(0)
    , knobSelectionCombo(0)
    , useAliasLabel(0)
    , useAliasCheckBox(0)
    , destPageLabel(0)
    , destPageCombo(0)
    , groupLabel(0)
    , groupCombo(0)
    , pages()
    , groups()
    , buttons(0)
    , allNodes()
    , allKnobs()
    {
        
    }
    
    boost::shared_ptr<KnobGroup> getSelectedGroup() const;
};

boost::shared_ptr<KnobGroup>
PickKnobDialogPrivate::getSelectedGroup() const
{
    if (!groupCombo->isVisible()) {
        return boost::shared_ptr<KnobGroup>();
    }
    std::string selectedItem = groupCombo->getCurrentIndexText().toStdString();
    if (selectedItem != "-") {
        for (std::vector<boost::shared_ptr<KnobGroup> >::const_iterator it = groups.begin(); it != groups.end(); ++it) {
            if ((*it)->getName() == selectedItem) {
                return *it;
            }
        }
    }
    
    return boost::shared_ptr<KnobGroup>();
}


PickKnobDialog::PickKnobDialog(DockablePanel* panel, QWidget* parent)
: QDialog(parent)
, _imp(new PickKnobDialogPrivate(panel))
{
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(panel);
    assert(nodePanel);
    boost::shared_ptr<NodeGui> nodeGui = nodePanel->getNode();
    NodePtr node = nodeGui->getNode();
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(node->getLiveInstance());
    boost::shared_ptr<NodeCollection> collec = node->getGroup();
    NodeGroup* isCollecGroup = dynamic_cast<NodeGroup*>(collec.get());
    NodeList collectNodes = collec->getNodes();
    for (NodeList::iterator it = collectNodes.begin(); it != collectNodes.end(); ++it) {
        if (!(*it)->getParentMultiInstance() && (*it)->isActivated() && (*it)->getKnobs().size() > 0) {
            _imp->allNodes.push_back(*it);
        }
    }
    if (isCollecGroup) {
        _imp->allNodes.push_back(isCollecGroup->getNode());
    }
    if (isGroup) {
        NodeList groupnodes = isGroup->getNodes();
        for (NodeList::iterator it = groupnodes.begin(); it != groupnodes.end(); ++it) {
            if (!(*it)->getParentMultiInstance() && (*it)->isActivated() && (*it)->getKnobs().size() > 0) {
                _imp->allNodes.push_back(*it);
            }
        }
    }
    QStringList nodeNames;
    for (NodeList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        QString name( (*it)->getLabel().c_str() );
        nodeNames.push_back(name);
    }
    nodeNames.sort();

    _imp->mainLayout = new QGridLayout(this);
    _imp->selectNodeLabel = new Label(tr("Node:"));
    _imp->nodeSelectionCombo = new CompleterLineEdit(nodeNames,nodeNames,false,this);
    _imp->nodeSelectionCombo->setToolTip(GuiUtils::convertFromPlainText(tr("Input the name of a node in the current project."), Qt::WhiteSpaceNormal));
    _imp->nodeSelectionCombo->setFocus(Qt::PopupFocusReason);
    
    _imp->knobSelectionCombo = new ComboBox(this);
    
    QString useAliasTt = GuiUtils::convertFromPlainText(QObject::tr("If checked, an alias of the selected parameter will be created, coyping entirely its state. "
                                     "Only the script-name, label and tooltip will be editable. For choice parameters this will also "
                                     "dynamically refresh the menu entries when the original parameter's menu is changed.\n"
                                     "When unchecked a simple expression will be set linking the 2 parameters but things such as  dynamic menus "
                                                                  "will not be enabled."),Qt::WhiteSpaceNormal);
    _imp->useAliasLabel = new Label(tr("Make Alias:"),this);
    _imp->useAliasLabel->setToolTip(useAliasTt);
    _imp->useAliasCheckBox = new QCheckBox(this);
    _imp->useAliasCheckBox->setToolTip(useAliasTt);
    _imp->useAliasCheckBox->setChecked(true);
    
    QObject::connect( _imp->nodeSelectionCombo,SIGNAL( itemCompletionChosen() ),this,SLOT( onNodeComboEditingFinished() ) );
    
    _imp->destPageLabel = new Label(tr("Page:"),this);
    QString pagett = GuiUtils::convertFromPlainText(QObject::tr("Select the page into which the parameter will be created"), Qt::WhiteSpaceNormal);
    _imp->destPageLabel->setToolTip(pagett);
    _imp->destPageCombo = new ComboBox(this);
    _imp->destPageCombo->setToolTip(pagett);
    QObject::connect(_imp->destPageCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onPageComboIndexChanged(int)));
    
    const std::vector<boost::shared_ptr<KnobI> >& knobs = node->getKnobs();
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->isUserKnob()) {
            boost::shared_ptr<KnobPage> isPage = boost::dynamic_pointer_cast<KnobPage>(knobs[i]);
            if (isPage) {
                _imp->pages.push_back(isPage);
                _imp->destPageCombo->addItem(isPage->getName().c_str());
            } else {
                boost::shared_ptr<KnobGroup> isGrp = boost::dynamic_pointer_cast<KnobGroup>(knobs[i]);
                if (isGrp) {
                    _imp->groups.push_back(isGrp);
                }
            }
        }
    }
    if (_imp->destPageCombo->count() == 0) {
        _imp->destPageCombo->hide();
    }
    
    
    _imp->groupLabel = new Label(tr("Group:"), this);
    QString grouptt = GuiUtils::convertFromPlainText(QObject::tr("Select the group into which the parameter will be created"), Qt::WhiteSpaceNormal);
    _imp->groupCombo = new ComboBox(this);
    _imp->groupLabel->setToolTip(grouptt);
    _imp->groupCombo->setToolTip(grouptt);
    onPageComboIndexChanged(0);
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal,this);
    QObject::connect( _imp->buttons, SIGNAL( accepted() ), this, SLOT( accept() ) );
    QObject::connect( _imp->buttons, SIGNAL( rejected() ), this, SLOT( reject() ) );
    
    _imp->mainLayout->addWidget(_imp->selectNodeLabel, 0, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->nodeSelectionCombo, 0, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->knobSelectionCombo, 0, 2, 1, 1);
    _imp->mainLayout->addWidget(_imp->useAliasLabel, 1, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->useAliasCheckBox, 1, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->destPageLabel, 2, 0 , 1, 1);
    _imp->mainLayout->addWidget(_imp->destPageCombo, 2, 1 , 1, 1);
    _imp->mainLayout->addWidget(_imp->groupLabel, 2, 2 , 1, 1);
    _imp->mainLayout->addWidget(_imp->groupCombo, 2, 3 , 1, 1);
    _imp->mainLayout->addWidget(_imp->buttons, 3, 0, 1, 3);
    
    QTimer::singleShot( 25, _imp->nodeSelectionCombo, SLOT( showCompleter() ) );

}

PickKnobDialog::~PickKnobDialog()
{
    
}

void
PickKnobDialog::onNodeComboEditingFinished()
{
    QString index = _imp->nodeSelectionCombo->text();
    _imp->knobSelectionCombo->clear();
    _imp->allKnobs.clear();
    boost::shared_ptr<Node> selectedNode;
    std::string currentNodeName = index.toStdString();
    for (NodeList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        if ((*it)->getLabel() == currentNodeName) {
            selectedNode = *it;
            break;
        }
    }
    if (!selectedNode) {
        return;
    }
    const std::vector< boost::shared_ptr<KnobI> > & knobs = selectedNode->getKnobs();
    for (U32 j = 0; j < knobs.size(); ++j) {
        if (!knobs[j]->getIsSecret()) {
            KnobPage* isPage = dynamic_cast<KnobPage*>( knobs[j].get() );
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knobs[j].get() );
            if (!isPage && !isGroup) {
                QString name( knobs[j]->getName().c_str() );
                
                bool canInsertKnob = true;
                for (int k = 0; k < knobs[j]->getDimension(); ++k) {
                    if ( knobs[j]->isSlave(k) || !knobs[j]->isEnabled(k) || name.isEmpty() ) {
                        canInsertKnob = false;
                    }
                }
                if (canInsertKnob) {
                    _imp->allKnobs.insert(std::make_pair( name, knobs[j]));
                    _imp->knobSelectionCombo->addItem(name);
                }
            }
            
        }
    }
}

void
PickKnobDialog::onPageComboIndexChanged(int index)
{
    if (_imp->pages.empty()) {
        _imp->groupCombo->hide();
    }
    _imp->groupCombo->clear();
    _imp->groupCombo->addItem("-");
    
    std::string selectedPage = _imp->destPageCombo->itemText(index).toStdString();
    boost::shared_ptr<KnobPage> parentPage ;
    
    if (selectedPage == NATRON_USER_MANAGED_KNOBS_PAGE) {
        parentPage = _imp->panel->getUserPageKnob();
    } else {
        for (std::vector<boost::shared_ptr<KnobPage> >::iterator it = _imp->pages.begin(); it != _imp->pages.end(); ++it) {
            if ((*it)->getName() == selectedPage) {
                parentPage = *it;
                break;
            }
        }
    }
    
    for (std::vector<boost::shared_ptr<KnobGroup> >::iterator it = _imp->groups.begin(); it != _imp->groups.end(); ++it) {
        boost::shared_ptr<KnobPage> page = (*it)->getTopLevelPage();
        assert(page);
        
        ///add only grps whose parent page is the selected page
        if (page == parentPage) {
            _imp->groupCombo->addItem((*it)->getName().c_str());
        }
        
    }
}

KnobGui*
PickKnobDialog::getSelectedKnob(bool* makeAlias,boost::shared_ptr<KnobPage>* page,boost::shared_ptr<KnobGroup>* group) const
{
    QString index = _imp->nodeSelectionCombo->text();
    boost::shared_ptr<Node> selectedNode;
    std::string currentNodeName = index.toStdString();
    for (NodeList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        if ((*it)->getLabel() == currentNodeName) {
            selectedNode = *it;
            break;
        }
    }
    if (!selectedNode) {
        return 0;
    }
    
    QString str = _imp->knobSelectionCombo->itemText( _imp->knobSelectionCombo->activeIndex() );
    std::map<QString,boost::shared_ptr<KnobI> >::const_iterator it = _imp->allKnobs.find(str);
    
    boost::shared_ptr<KnobI> selectedKnob;
    if ( it != _imp->allKnobs.end() ) {
        selectedKnob = it->second;
    } else {
        return 0;
    }
    
    boost::shared_ptr<NodeGuiI> selectedNodeGuiI = selectedNode->getNodeGui();
    assert(selectedNodeGuiI);
    NodeGui* selectedNodeGui = dynamic_cast<NodeGui*>(selectedNodeGuiI.get());
    assert(selectedNodeGui);
    NodeSettingsPanel* selectedPanel = selectedNodeGui->getSettingPanel();
    bool hadPanelVisible = selectedPanel && !selectedPanel->isClosed();
    if (!selectedPanel) {
        selectedNodeGui->ensurePanelCreated();
        selectedPanel = selectedNodeGui->getSettingPanel();
        selectedNodeGui->setVisibleSettingsPanel(false);
    }
    if (!selectedPanel) {
        return 0;
    }
    if (!hadPanelVisible && selectedPanel) {
        selectedPanel->setClosed(true);
    }
    
    
    int page_i = _imp->destPageCombo->activeIndex();
    if (page_i >= 0 && page_i < (int)_imp->pages.size()) {
        *page = _imp->pages[page_i];
    }
    
    *group = _imp->getSelectedGroup();
    
    const std::map<boost::weak_ptr<KnobI>,KnobGui*>& knobsMap = selectedPanel->getKnobs();
    std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator found = knobsMap.find(selectedKnob);
    if (found != knobsMap.end()) {
        *makeAlias = _imp->useAliasCheckBox->isChecked();
        return found->second;
    }
    
    
    return 0;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_PickKnobDialog.cpp"
