/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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
#include "Engine/KnobTypes.h" // Button_Knob...
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodeCollection

#include "Gui/ComboBox.h"
#include "Gui/Label.h"
#include "Gui/NodeCreationDialog.h" // CompleterLineEdit
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/Utils.h" // convertFromPlainText

struct PickKnobDialogPrivate
{
    
    QGridLayout* mainLayout;
    Natron::Label* selectNodeLabel;
    CompleterLineEdit* nodeSelectionCombo;
    ComboBox* knobSelectionCombo;
    Natron::Label* useExpressionLabel;
    QCheckBox* useExpressionCheckBox;
    QDialogButtonBox* buttons;
    NodeList allNodes;
    std::map<QString,boost::shared_ptr<KnobI > > allKnobs;
    
    PickKnobDialogPrivate()
    : mainLayout(0)
    , selectNodeLabel(0)
    , nodeSelectionCombo(0)
    , knobSelectionCombo(0)
    , useExpressionLabel(0)
    , useExpressionCheckBox(0)
    , buttons(0)
    , allNodes()
    , allKnobs()
    {
        
    }
};

PickKnobDialog::PickKnobDialog(DockablePanel* panel, QWidget* parent)
: QDialog(parent)
, _imp(new PickKnobDialogPrivate())
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
    _imp->selectNodeLabel = new Natron::Label(tr("Node:"));
    _imp->nodeSelectionCombo = new CompleterLineEdit(nodeNames,nodeNames,false,this);
    _imp->nodeSelectionCombo->setToolTip(Natron::convertFromPlainText(tr("Input the name of a node in the current project."), Qt::WhiteSpaceNormal));
    _imp->nodeSelectionCombo->setFocus(Qt::PopupFocusReason);
    
    _imp->knobSelectionCombo = new ComboBox(this);
    
    _imp->useExpressionLabel = new Natron::Label(tr("Also create an expression to control the parameter:"),this);
    _imp->useExpressionCheckBox = new QCheckBox(this);
    _imp->useExpressionCheckBox->setChecked(true);
    
    QObject::connect( _imp->nodeSelectionCombo,SIGNAL( itemCompletionChosen() ),this,SLOT( onNodeComboEditingFinished() ) );
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal,this);
    QObject::connect( _imp->buttons, SIGNAL( accepted() ), this, SLOT( accept() ) );
    QObject::connect( _imp->buttons, SIGNAL( rejected() ), this, SLOT( reject() ) );
    
    _imp->mainLayout->addWidget(_imp->selectNodeLabel, 0, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->nodeSelectionCombo, 0, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->knobSelectionCombo, 0, 2, 1, 1);
    _imp->mainLayout->addWidget(_imp->useExpressionLabel, 1, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->useExpressionCheckBox, 1, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->buttons, 2, 0, 1, 3);
    
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
    boost::shared_ptr<Natron::Node> selectedNode;
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
            Button_Knob* isButton = dynamic_cast<Button_Knob*>( knobs[j].get() );
            Page_Knob* isPage = dynamic_cast<Page_Knob*>( knobs[j].get() );
            Group_Knob* isGroup = dynamic_cast<Group_Knob*>( knobs[j].get() );
            if (!isButton && !isPage && !isGroup) {
                QString name( knobs[j]->getDescription().c_str() );
                
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

KnobGui*
PickKnobDialog::getSelectedKnob(bool* useExpressionLink) const
{
    QString index = _imp->nodeSelectionCombo->text();
    boost::shared_ptr<Natron::Node> selectedNode;
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
    }
    if (!selectedPanel) {
        return 0;
    }
    if (!hadPanelVisible && selectedPanel) {
        selectedPanel->setClosed(true);
    }
    const std::map<boost::weak_ptr<KnobI>,KnobGui*>& knobsMap = selectedPanel->getKnobs();
    std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator found = knobsMap.find(selectedKnob);
    if (found != knobsMap.end()) {
        *useExpressionLink = _imp->useExpressionCheckBox->isChecked();
        return found->second;
    }
    return 0;
}
