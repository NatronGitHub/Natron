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

#include "NodeSettingsPanel.h"

#include <vector>
#include <list>
#include <string>
#include <exception>
#include <fstream>

#include <QtGui/QStyle>

#include "Engine/EffectInstance.h"
#include "Engine/Knob.h" // KnobHolder
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/RotoLayer.h"

#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiDefines.h"
#include "Gui/Menu.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/RotoPanel.h"
#include "Gui/Utils.h" // convertFromPlainText



using std::make_pair;
using namespace Natron;



NodeSettingsPanel::NodeSettingsPanel(const boost::shared_ptr<MultiInstancePanel> & multiPanel,
                                     Gui* gui,
                                     const boost::shared_ptr<NodeGui> &NodeUi,
                                     QVBoxLayout* container,
                                     QWidget *parent)
    : DockablePanel(gui,
                    multiPanel.get() != NULL ? dynamic_cast<KnobHolder*>( multiPanel.get() ) : NodeUi->getNode()->getLiveInstance(),
                    container,
                    DockablePanel::eHeaderModeFullyFeatured,
                    false,
                    NodeUi->getUndoStack(),
                    NodeUi->getNode()->getLabel().c_str(),
                    NodeUi->getNode()->getPluginDescription().c_str(),
                    false,
                    "Settings",
                    parent)
      , _nodeGUI(NodeUi)
      , _selected(false)
      , _settingsButton(0)
      , _multiPanel(multiPanel)
{
    if (multiPanel) {
        multiPanel->initializeKnobsPublic();
    }

    
    QObject::connect( this,SIGNAL( closeChanged(bool) ),NodeUi.get(),SLOT( onSettingsPanelClosedChanged(bool) ) );
    
    QPixmap pixSettings;
    appPTR->getIcon(NATRON_PIXMAP_SETTINGS, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixSettings);
    _settingsButton = new Button( QIcon(pixSettings),"",getHeaderWidget() );
    _settingsButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _settingsButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _settingsButton->setToolTip(Natron::convertFromPlainText(tr("Settings and presets."), Qt::WhiteSpaceNormal));
    _settingsButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _settingsButton,SIGNAL( clicked() ),this,SLOT( onSettingsButtonClicked() ) );
    insertHeaderWidget(1, _settingsButton);
}

NodeSettingsPanel::~NodeSettingsPanel()
{
    boost::shared_ptr<NodeGui> node = getNode();
    if (node) {
        node->removeSettingsPanel();
    }
}

void
NodeSettingsPanel::setSelected(bool s)
{
    _selected = s;
    style()->unpolish(this);
    style()->polish(this);
}

void
NodeSettingsPanel::centerOnItem()
{
    getNode()->centerGraphOnIt();
}

RotoPanel*
NodeSettingsPanel::initializeRotoPanel()
{
    if (getNode()->getNode()->isRotoPaintingNode()) {
        return new RotoPanel(_nodeGUI.lock(),this);
    } else {
        return NULL;
    }
}

QColor
NodeSettingsPanel::getCurrentColor() const
{
    return getNode()->getCurrentColor();
}

void
NodeSettingsPanel::initializeExtraGui(QVBoxLayout* layout)
{
    if ( _multiPanel && !_multiPanel->isGuiCreated() ) {
        _multiPanel->createMultiInstanceGui(layout);
    }
}

void
NodeSettingsPanel::onSettingsButtonClicked()
{
    Natron::Menu menu(this);
    //menu.setFont(QFont(appFont,appFontSize));
    
    boost::shared_ptr<NodeGui> node = getNode();
    boost::shared_ptr<Natron::Node> master = node->getNode()->getMasterNode();
    
    QAction* importPresets = new QAction(tr("Import presets"),&menu);
    QObject::connect(importPresets,SIGNAL(triggered()),this,SLOT(onImportPresetsActionTriggered()));
    QAction* exportAsPresets = new QAction(tr("Export as presets"),&menu);
    QObject::connect(exportAsPresets,SIGNAL(triggered()),this,SLOT(onExportPresetsActionTriggered()));
    
    menu.addAction(importPresets);
    menu.addAction(exportAsPresets);
    menu.addSeparator();
    
    QAction* manageUserParams = new QAction(tr("Manage user parameters..."),&menu);
    QObject::connect(manageUserParams,SIGNAL(triggered()),this,SLOT(onManageUserParametersActionTriggered()));
    menu.addAction(manageUserParams);
   
    menu.addSeparator();

    
    QAction* setKeyOnAll = new QAction(tr("Set key on all parameters"),&menu);
    QObject::connect(setKeyOnAll,SIGNAL(triggered()),this,SLOT(setKeyOnAllParameters()));
    QAction* removeAnimationOnAll = new QAction(tr("Remove animation on all parameters"),&menu);
    QObject::connect(removeAnimationOnAll,SIGNAL(triggered()),this,SLOT(removeAnimationOnAllParameters()));
    menu.addAction(setKeyOnAll);
    menu.addAction(removeAnimationOnAll);
    
    if (master || !node->getDagGui() || !node->getDagGui()->getGui() || node->getDagGui()->getGui()->isGUIFrozen()) {
        importPresets->setEnabled(false);
        exportAsPresets->setEnabled(false);
        setKeyOnAll->setEnabled(false);
        removeAnimationOnAll->setEnabled(false);
    }
    
    menu.exec(_settingsButton->mapToGlobal(_settingsButton->pos()));
}

void
NodeSettingsPanel::onImportPresetsActionTriggered()
{
    std::vector<std::string> filters;
    filters.push_back(NATRON_PRESETS_FILE_EXT);
    std::string filename = getGui()->popOpenFileDialog(false, filters, getGui()->getLastLoadProjectDirectory().toStdString(), false);
    if (filename.empty()) {
        return;
    }
    
    std::ifstream ifile;
    try {
        ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifile.open(filename.c_str(),std::ifstream::in);
    } catch (const std::ifstream::failure & e) {
        Natron::errorDialog("Presets",e.what());
        return;
    }
    
    std::list<boost::shared_ptr<NodeSerialization> > nodeSerialization;
    try {

        int nNodes;
        boost::archive::xml_iarchive iArchive(ifile);
        iArchive >> boost::serialization::make_nvp("NodesCount",nNodes);
        for (int i = 0; i < nNodes ; ++i) {
            boost::shared_ptr<NodeSerialization> node(new NodeSerialization());
            iArchive >> boost::serialization::make_nvp("Node",*node);
            nodeSerialization.push_back(node);
        }
        
        
    } catch (const std::exception & e) {
        ifile.close();
        Natron::errorDialog("Presets",e.what());
        return;
    }
    boost::shared_ptr<NodeGui> node = getNode();
    if (nodeSerialization.front()->getPluginID() != node->getNode()->getPluginID()) {
        QString err = QString(tr("You cannot load ") + filename.c_str()  + tr(" which are presets for the plug-in ") +
                              nodeSerialization.front()->getPluginID().c_str() + tr(" on the plug-in ") +
                              node->getNode()->getPluginID().c_str());
        Natron::errorDialog(tr("Presets").toStdString(),err.toStdString());
        return;
    }
    
    node->restoreInternal(node,nodeSerialization);
}


static bool endsWith(const std::string &str, const std::string &suffix)
{
    return ((str.size() >= suffix.size()) &&
            (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0));
}

void
NodeSettingsPanel::onExportPresetsActionTriggered()
{
    std::vector<std::string> filters;
    filters.push_back(NATRON_PRESETS_FILE_EXT);
    std::string filename = getGui()->popSaveFileDialog(false, filters, getGui()->getLastSaveProjectDirectory().toStdString(), false);
    if (filename.empty()) {
        return;
    }
    
    if (!endsWith(filename, "." NATRON_PRESETS_FILE_EXT)) {
        filename.append("." NATRON_PRESETS_FILE_EXT);
    }
    
    std::ofstream ofile;
    try {
        ofile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        ofile.open(filename.c_str(),std::ofstream::out);
    } catch (const std::ofstream::failure & e) {
        Natron::errorDialog("Presets",e.what());
        return;
    }

    boost::shared_ptr<NodeGui> node = getNode();
    std::list<boost::shared_ptr<NodeSerialization> > nodeSerialization;
    node->serializeInternal(nodeSerialization);
    try {
         
        int nNodes = nodeSerialization.size();
        boost::archive::xml_oarchive oArchive(ofile);
        oArchive << boost::serialization::make_nvp("NodesCount",nNodes);
        for (std::list<boost::shared_ptr<NodeSerialization> >::iterator it = nodeSerialization.begin();
             it != nodeSerialization.end(); ++it) {
            oArchive << boost::serialization::make_nvp("Node",**it);
        }
        
        
    }  catch (const std::exception & e) {
        ofile.close();
        Natron::errorDialog("Presets",e.what());
        return;
    }
 
}

