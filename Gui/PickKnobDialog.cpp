/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <stdexcept>
#include <map>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include <QGridLayout>
#include <QCheckBox>
#include <QtCore/QTimer>

#include "Engine/Knob.h" // KnobI
#include "Engine/KnobTypes.h" // KnobButton...
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodeCollection
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/ComboBox.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/Label.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeCreationDialog.h" // CompleterLineEdit
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"

NATRON_NAMESPACE_ENTER

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
    std::vector<KnobPagePtr> pages;
    std::vector<KnobGroupPtr> groups;
    DialogButtonBox* buttons;
    NodesList allNodes;
    std::map<QString, KnobIPtr> allKnobs;
    KnobGuiPtr selectedKnob;

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
        , selectedKnob()
    {
    }

    KnobGroupPtr getSelectedGroup() const;

    void onSelectedKnobChanged()
    {
        if (!selectedKnob) {
            return;
        }
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>( selectedKnob->getKnob().get() );
        if (isParametric) {
            useAliasCheckBox->setChecked(true);
        }
        useAliasLabel->setEnabled(!isParametric);
        useAliasCheckBox->setEnabled(!isParametric);
    }
};

KnobGroupPtr
PickKnobDialogPrivate::getSelectedGroup() const
{
    if ( pages.empty() ) {
        return KnobGroupPtr();
    }
    std::string selectedItem = groupCombo->getCurrentIndexText().toStdString();
    if (selectedItem != "-") {
        for (std::vector<KnobGroupPtr>::const_iterator it = groups.begin(); it != groups.end(); ++it) {
            if ( (*it)->getName() == selectedItem ) {
                return *it;
            }
        }
    }

    return KnobGroupPtr();
}

PickKnobDialog::PickKnobDialog(DockablePanel* panel,
                               QWidget* parent)
    : QDialog(parent)
    , _imp( new PickKnobDialogPrivate(panel) )
{
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(panel);

    assert(nodePanel);
    if (!nodePanel) {
        throw std::logic_error("PickKnobDialog::PickKnobDialog()");
    }
    NodeGuiPtr nodeGui = nodePanel->getNode();
    NodePtr node = nodeGui->getNode();
    NodeGroup* isGroup = node->isEffectGroup();
    NodeCollectionPtr collec = node->getGroup();
    NodeGroup* isCollecGroup = dynamic_cast<NodeGroup*>( collec.get() );
    NodesList collectNodes = collec->getNodes();
    for (NodesList::iterator it = collectNodes.begin(); it != collectNodes.end(); ++it) {
        if ( !(*it)->getParentMultiInstance() && (*it)->isActivated() && ( (*it)->getKnobs().size() > 0 ) ) {
            _imp->allNodes.push_back(*it);
        }
    }
    if (isCollecGroup) {
        _imp->allNodes.push_back( isCollecGroup->getNode() );
    }
    if (isGroup) {
        NodesList groupnodes = isGroup->getNodes();
        for (NodesList::iterator it = groupnodes.begin(); it != groupnodes.end(); ++it) {
            if ( (*it)->getNodeGui() &&
                !(*it)->getParentMultiInstance() &&
                (*it)->isActivated() &&
                ( (*it)->getKnobs().size() > 0 ) ) {
                _imp->allNodes.push_back(*it);
            }
        }
    }
    QStringList nodeNames;
    for (NodesList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        QString name = QString::fromUtf8( (*it)->getLabel().c_str() );
        nodeNames.push_back(name);
    }
    nodeNames.sort();

    _imp->mainLayout = new QGridLayout(this);
    _imp->selectNodeLabel = new Label( tr("Node:") );
    _imp->nodeSelectionCombo = new CompleterLineEdit(nodeNames, nodeNames, false, this);
    _imp->nodeSelectionCombo->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Input the name of a node in the current project."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->nodeSelectionCombo->setFocus(Qt::PopupFocusReason);

    _imp->knobSelectionCombo = new ComboBox(this);
    QObject::connect( _imp->knobSelectionCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onKnobComboIndexChanged(int)) );
    QString useAliasTt = NATRON_NAMESPACE::convertFromPlainText(tr("If checked, an alias of the selected parameter will be created, coyping entirely its state. "
                                                           "Only the script-name, label and tooltip will be editable.\n"
                                                           "For choice parameters this will also "
                                                           "dynamically refresh the menu entries when the original parameter's menu is changed.\n"
                                                           "When unchecked, a simple expression will be set linking the two parameters, but things such as dynamic menus "
                                                           "will be disabled."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->useAliasLabel = new Label(tr("Make Alias:"), this);
    _imp->useAliasLabel->setToolTip(useAliasTt);
    _imp->useAliasCheckBox = new QCheckBox(this);
    _imp->useAliasCheckBox->setToolTip(useAliasTt);
    _imp->useAliasCheckBox->setChecked(true);

    QObject::connect( _imp->nodeSelectionCombo, SIGNAL(itemCompletionChosen()), this, SLOT(onNodeComboEditingFinished()) );

    _imp->destPageLabel = new Label(tr("Page:"), this);
    QString pagett = NATRON_NAMESPACE::convertFromPlainText(tr("Select the page into which the parameter will be created."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->destPageLabel->setToolTip(pagett);
    _imp->destPageCombo = new ComboBox(this);
    _imp->destPageCombo->setToolTip(pagett);
    QObject::connect( _imp->destPageCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onPageComboIndexChanged(int)) );
    const KnobsVec& knobs = node->getKnobs();
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->isUserKnob() ) {
            KnobPagePtr isPage = boost::dynamic_pointer_cast<KnobPage>(knobs[i]);
            if (isPage) {
                _imp->pages.push_back(isPage);
                _imp->destPageCombo->addItem( QString::fromUtf8( isPage->getName().c_str() ) );
            } else {
                KnobGroupPtr isGrp = boost::dynamic_pointer_cast<KnobGroup>(knobs[i]);
                if (isGrp) {
                    _imp->groups.push_back(isGrp);
                }
            }
        }
    }
    if (_imp->destPageCombo->count() == 0) {
        _imp->destPageLabel->hide();
        _imp->destPageCombo->hide();
    }


    _imp->groupLabel = new Label(tr("Group:"), this);
    QString grouptt = NATRON_NAMESPACE::convertFromPlainText(tr("Select the group into which the parameter will be created."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->groupCombo = new ComboBox(this);
    _imp->groupLabel->setToolTip(grouptt);
    _imp->groupCombo->setToolTip(grouptt);
    onPageComboIndexChanged(0);

    _imp->buttons = new DialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal, this);
    QObject::connect( _imp->buttons, SIGNAL(accepted()), this, SLOT(accept()) );
    QObject::connect( _imp->buttons, SIGNAL(rejected()), this, SLOT(reject()) );

    _imp->mainLayout->addWidget(_imp->selectNodeLabel, 0, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->nodeSelectionCombo, 0, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->knobSelectionCombo, 0, 2, 1, 1);
    _imp->mainLayout->addWidget(_imp->useAliasLabel, 1, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->useAliasCheckBox, 1, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->destPageLabel, 2, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->destPageCombo, 2, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->groupLabel, 2, 2, 1, 1);
    _imp->mainLayout->addWidget(_imp->groupCombo, 2, 3, 1, 1);
    _imp->mainLayout->addWidget(_imp->buttons, 3, 0, 1, 3);

    QTimer::singleShot( 25, _imp->nodeSelectionCombo, SLOT(showCompleter()) );
}

PickKnobDialog::~PickKnobDialog()
{
}

static KnobGuiPtr
getKnobGuiForKnob(const NodePtr& selectedNode,
                  const KnobIPtr& knob)
{
    NodeGuiIPtr selectedNodeGuiI = selectedNode->getNodeGui();

    assert(selectedNodeGuiI);
    if (!selectedNodeGuiI) {
        return KnobGuiPtr();
    }
    NodeGui* selectedNodeGui = dynamic_cast<NodeGui*>( selectedNodeGuiI.get() );
    assert(selectedNodeGui);
    if (!selectedNodeGui) {
        return KnobGuiPtr();
    }
    NodeSettingsPanel* selectedPanel = selectedNodeGui->getSettingPanel();
    bool hadPanelVisible = selectedPanel && !selectedPanel->isClosed();
    if (!selectedPanel) {
        selectedNodeGui->ensurePanelCreated();
        selectedPanel = selectedNodeGui->getSettingPanel();
        selectedNodeGui->setVisibleSettingsPanel(false);
    }
    if (!selectedPanel) {
        return KnobGuiPtr();
    }
    if (!hadPanelVisible && selectedPanel) {
        selectedPanel->setClosed(true);
    }


    const std::list<std::pair<KnobIWPtr, KnobGuiPtr> >& knobsMap = selectedPanel->getKnobsMapping();
    for (std::list<std::pair<KnobIWPtr, KnobGuiPtr> >::const_iterator it = knobsMap.begin(); it != knobsMap.end(); ++it) {
        if (it->first.lock() == knob) {
            return it->second;
        }
    }

    return KnobGuiPtr();
}

void
PickKnobDialog::onKnobComboIndexChanged(int /*idx*/)
{
    QString selectedNodeName = _imp->nodeSelectionCombo->text();
    NodePtr selectedNode;
    std::string currentNodeName = selectedNodeName.toStdString();

    for (NodesList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        if ( (*it)->getLabel() == currentNodeName ) {
            selectedNode = *it;
            break;
        }
    }
    _imp->selectedKnob.reset();
    if (selectedNode) {
        QString str = _imp->knobSelectionCombo->itemText( _imp->knobSelectionCombo->activeIndex() );
        std::map<QString, KnobIPtr>::const_iterator it = _imp->allKnobs.find(str);
        KnobIPtr selectedKnob;
        if ( it != _imp->allKnobs.end() ) {
            selectedKnob = it->second;
            _imp->selectedKnob = getKnobGuiForKnob(selectedNode, selectedKnob);
        }
    }
    _imp->onSelectedKnobChanged();
}

void
PickKnobDialog::onNodeComboEditingFinished()
{
    QString index = _imp->nodeSelectionCombo->text();

    _imp->knobSelectionCombo->clear();
    _imp->allKnobs.clear();
    _imp->selectedKnob.reset();
    NodePtr selectedNode;
    std::string currentNodeName = index.toStdString();
    for (NodesList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        if ( (*it)->getLabel() == currentNodeName ) {
            selectedNode = *it;
            break;
        }
    }


    if (!selectedNode) {
        return;
    }
    const std::vector<KnobIPtr> & knobs = selectedNode->getKnobs();
    for (U32 j = 0; j < knobs.size(); ++j) {
        if ( !knobs[j]->getIsSecret() ) {
            KnobPage* isPage = dynamic_cast<KnobPage*>( knobs[j].get() );
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knobs[j].get() );
            if (!isPage && !isGroup) {
                QString name = QString::fromUtf8( knobs[j]->getName().c_str() );
                bool canInsertKnob = true;
                for (int k = 0; k < knobs[j]->getDimension(); ++k) {
                    if ( name.isEmpty() ) {
                        canInsertKnob = false;
                    }
                }
                if (canInsertKnob) {
                    if (!_imp->selectedKnob) {
                        _imp->selectedKnob = getKnobGuiForKnob(selectedNode, knobs[j]);
                    }
                    _imp->allKnobs.insert( std::make_pair( name, knobs[j]) );
                    _imp->knobSelectionCombo->addItem(name);
                }
            }
        }
    }

    _imp->onSelectedKnobChanged();
    _imp->knobSelectionCombo->setCurrentIndex_no_emit(0);
}

void
PickKnobDialog::onPageComboIndexChanged(int index)
{
    if ( _imp->pages.empty() ) {
        _imp->groupLabel->hide();
        _imp->groupCombo->hide();
    }
    _imp->groupCombo->clear();
    _imp->groupCombo->addItem( QString::fromUtf8("-") );

    std::string selectedPage = _imp->destPageCombo->itemText(index).toStdString();
    KnobPagePtr parentPage;


    for (std::vector<KnobPagePtr>::iterator it = _imp->pages.begin(); it != _imp->pages.end(); ++it) {
        if ( (*it)->getName() == selectedPage ) {
            parentPage = *it;
            break;
        }
    }


    for (std::vector<KnobGroupPtr>::iterator it = _imp->groups.begin(); it != _imp->groups.end(); ++it) {
        KnobPagePtr page = (*it)->getTopLevelPage();
        assert(page);

        ///add only grps whose parent page is the selected page
        if (page == parentPage) {
            _imp->groupCombo->addItem( QString::fromUtf8( (*it)->getName().c_str() ) );
        }
    }
}

KnobGuiPtr
PickKnobDialog::getSelectedKnob(bool* makeAlias,
                                KnobPagePtr* page,
                                KnobGroupPtr* group) const
{
    int page_i = _imp->destPageCombo->activeIndex();

    if ( (page_i >= 0) && ( page_i < (int)_imp->pages.size() ) ) {
        *page = _imp->pages[page_i];
    }

    *group = _imp->getSelectedGroup();
    *makeAlias = _imp->useAliasCheckBox->isChecked();

    return _imp->selectedKnob;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_PickKnobDialog.cpp"
