
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

#include "NodeSettingsPanel.h"

#include <vector>
#include <list>
#include <string>
#include <exception>
#include <fstream>
#include <stdexcept>

#include <QtGui/QStyle>

#include "Engine/EffectInstance.h"
#include "Engine/Knob.h" // KnobHolder
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/RotoLayer.h"
#include "Engine/FStreamsSupport.h"

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
NATRON_NAMESPACE_ENTER;


NodeSettingsPanel::NodeSettingsPanel(const boost::shared_ptr<MultiInstancePanel> & multiPanel,
                                     Gui* gui,
                                     const NodeGuiPtr &NodeUi,
                                     QVBoxLayout* container,
                                     QWidget *parent)
    : DockablePanel(gui,
                    multiPanel.get() != NULL ? dynamic_cast<KnobHolder*>( multiPanel.get() ) : NodeUi->getNode()->getEffectInstance().get(),
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

    
    QObject::connect( this,SIGNAL(closeChanged(bool)),NodeUi.get(),SLOT(onSettingsPanelClosedChanged(bool)) );
    
    const QSize mediumBSize(TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE));
    const QSize mediumIconSize(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE));

    QPixmap pixSettings;
    appPTR->getIcon(NATRON_PIXMAP_SETTINGS, TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), &pixSettings);
    _settingsButton = new Button( QIcon(pixSettings),"",getHeaderWidget() );
    _settingsButton->setFixedSize(mediumBSize);
    _settingsButton->setIconSize(mediumIconSize);
    _settingsButton->setToolTip(GuiUtils::convertFromPlainText(tr("Settings and presets."), Qt::WhiteSpaceNormal));
    _settingsButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _settingsButton,SIGNAL(clicked()),this,SLOT(onSettingsButtonClicked()) );
    insertHeaderWidget(1, _settingsButton);
}

NodeSettingsPanel::~NodeSettingsPanel()
{
    NodeGuiPtr node = getNode();
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
    Menu menu(this);
    //menu.setFont(QFont(appFont,appFontSize));
    
    NodeGuiPtr node = getNode();
    NodePtr master = node->getNode()->getMasterNode();
    
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
    

    
    FStreamsSupport::IStreamWrapper ifile;
    FStreamsSupport::open(&ifile, filename);
    if (!ifile) {
        Dialogs::errorDialog( tr("Presets").toStdString(), tr("Failed to open file: ").toStdString() + filename, false );
        return;
    }
    
    std::list<boost::shared_ptr<NodeSerialization> > nodeSerialization;
    try {

        int nNodes;
        boost::archive::xml_iarchive iArchive(*ifile);
        iArchive >> boost::serialization::make_nvp("NodesCount",nNodes);
        for (int i = 0; i < nNodes ; ++i) {
            boost::shared_ptr<NodeSerialization> node(new NodeSerialization());
            iArchive >> boost::serialization::make_nvp("Node",*node);
            nodeSerialization.push_back(node);
        }
        
        
    } catch (const std::exception & e) {
        Dialogs::errorDialog("Presets",e.what());
        return;
    }
    NodeGuiPtr node = getNode();
    if (nodeSerialization.front()->getPluginID() != node->getNode()->getPluginID()) {
        QString err = QString(tr("You cannot load ") + filename.c_str()  + tr(" which are presets for the plug-in ") +
                              nodeSerialization.front()->getPluginID().c_str() + tr(" on the plug-in ") +
                              node->getNode()->getPluginID().c_str());
        Dialogs::errorDialog(tr("Presets").toStdString(),err.toStdString());
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
    

    FStreamsSupport::OStreamWrapper ofile;
    FStreamsSupport::open(&ofile, filename);
    if (!ofile) {
        Dialogs::errorDialog( tr("Presets").toStdString()
                             , tr("Failed to open file ").toStdString() + filename, false );
        return;
    }

    NodeGuiPtr node = getNode();
    std::list<boost::shared_ptr<NodeSerialization> > nodeSerialization;
    node->serializeInternal(nodeSerialization);
    try {
         
        int nNodes = nodeSerialization.size();
        boost::archive::xml_oarchive oArchive(*ofile);
        oArchive << boost::serialization::make_nvp("NodesCount",nNodes);
        for (std::list<boost::shared_ptr<NodeSerialization> >::iterator it = nodeSerialization.begin();
             it != nodeSerialization.end(); ++it) {
            oArchive << boost::serialization::make_nvp("Node",**it);
        }
        
        
    }  catch (const std::exception & e) {
        Dialogs::errorDialog("Presets",e.what());
        return;
    }
 
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_NodeSettingsPanel.cpp"
