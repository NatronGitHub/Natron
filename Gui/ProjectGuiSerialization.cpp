//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ProjectGuiSerialization.h"

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDebug>
#include <QSplitter>
#include <QVBoxLayout>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Gui/NodeGui.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/ProjectGui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"
#include "Gui/Histogram.h"
#include "Gui/Splitter.h"
#include "Gui/DockablePanel.h"

static void createParenting(std::map<std::string,PaneLayout>& layout,std::map<std::string,PaneLayout>::iterator it) {
    QString nameCpy = it->first.c_str();
    
    bool isSplit,horizontalSplit;
    nameCpy = TabWidget::getTabWidgetParentName(nameCpy, &isSplit,&horizontalSplit);
    if (!isSplit) {
        ///not a child
        return;
    }
    
    if (!horizontalSplit) {
        std::map<std::string,PaneLayout>::iterator foundParent = layout.find(nameCpy.toStdString());
        assert(foundParent != layout.end());
        std::list<std::string>::iterator foundSplit = std::find(foundParent->second.splitsNames.begin(),
                                                                foundParent->second.splitsNames.end(),it->first);
        
        if (foundSplit == foundParent->second.splitsNames.end()) {
            foundParent->second.splitsNames.push_back(it->first);
        }
        it->second.parentName = nameCpy.toStdString();
        it->second.parentingCreated = true;
        
        //call this recursively
        createParenting(layout,foundParent);
    } else {
        //we now have the name of the parent
        std::map<std::string,PaneLayout>::iterator foundParent = layout.find(nameCpy.toStdString());
        assert(foundParent != layout.end());
        
        std::list<std::string>::iterator foundSplit = std::find(foundParent->second.splitsNames.begin(),
                                                                foundParent->second.splitsNames.end(),it->first);
        
        if (foundSplit == foundParent->second.splitsNames.end()) {
            foundParent->second.splitsNames.push_back(it->first);
        }
        
        it->second.parentName = nameCpy.toStdString();
        it->second.parentingCreated = true;
        
        //call this recursively
        createParenting(layout,foundParent);
        
    }
}

static void saveLayout(const Gui* gui,std::map<std::string,PaneLayout>& layout,std::map<std::string,std::string>& splittersState)
{
    std::list<TabWidget*> tabWidgets = gui->getPanes_mt_safe();
    for (std::list<TabWidget*>::const_iterator it = tabWidgets.begin(); it!= tabWidgets.end(); ++it) {
        QString widgetName = (*it)->objectName_mt_safe();
        if(widgetName.isEmpty()){
            qDebug() << "Warning: attempting to save the layout of an unnamed TabWidget, discarding.";
            continue;
        }
        
        PaneLayout tabLayout;
        tabLayout.parentingCreated = false;
        std::list<std::pair<TabWidget*,bool> > userSplits = (*it)->getUserSplits();
        for (std::list<std::pair<TabWidget*,bool> >::const_iterator split = userSplits.begin(); split!=userSplits.end(); ++split) {
            tabLayout.splits.push_back(split->second);
        }
        tabLayout.floating = (*it)->isFloating();
        if (tabLayout.floating) {
            QPoint pos = (*it)->getWindowPos_mt_safe();
            tabLayout.posx = pos.x();
            tabLayout.posy = pos.y();
            (*it)->getWindowSize_mt_safe(tabLayout.width, tabLayout.height);
        } else {
            //not releveant since the tab is not floating
            tabLayout.posx = -1;
            tabLayout.posy = -1;
        }
        
        QStringList tabNames = (*it)->getTabNames();
        for (int i = 0; i < tabNames.size(); ++i) {
            if(tabNames[i].isEmpty()){
                qDebug() << "Warning: attempting to save the position of an unnamed tab, discarding.";
                continue;
            }
            tabLayout.tabs.push_back(tabNames[i].toStdString());
        }
        tabLayout.currentIndex = (*it)->activeIndex();
        layout.insert(std::make_pair(widgetName.toStdString(),tabLayout));
    }
    
    ///now we do a second pass to insert for each layout what are its children splits
    for (std::map<std::string,PaneLayout>::iterator it = layout.begin(); it!= layout.end(); ++it) {
        if(!it->second.parentingCreated){
            createParenting(layout,it);
        }
    }
    
    ///save application's splitters states
    std::list<Splitter*> splitters = gui->getSplitters();
    for (std::list<Splitter*>::const_iterator it = splitters.begin(); it!= splitters.end(); ++it) {
        QString ba = (*it)->serializeNatron();
        splittersState.insert(std::make_pair((*it)->objectName_mt_safe().toStdString(),ba.toStdString()));
        
    }

}

void
ProjectGuiSerialization::initialize(const ProjectGui* projectGui)
{
     std::list<boost::shared_ptr<NodeGui> > activeNodes = projectGui->getVisibleNodes();
     _serializedNodes.clear();
     for (std::list<boost::shared_ptr<NodeGui> >::iterator it = activeNodes.begin();it!=activeNodes.end();++it) {
         NodeGuiSerialization state;
         (*it)->serialize(&state);
         _serializedNodes.push_back(state);
         
         if ((*it)->getNode()->getPluginID() == "Viewer") {
             ViewerInstance* viewer = dynamic_cast<ViewerInstance*>((*it)->getNode()->getLiveInstance());
             assert(viewer);
             ViewerTab* tab = projectGui->getGui()->getViewerTabForInstance(viewer);
             assert(tab);
             ViewerData viewerData;
             tab->getViewer()->getProjection(&viewerData.zoomLeft, &viewerData.zoomBottom, &viewerData.zoomFactor, &viewerData.zoomPAR);
             viewerData.userRoI = tab->getViewer()->getUserRegionOfInterest();
             viewerData.userRoIenabled = tab->getViewer()->isUserRegionOfInterestEnabled();
             viewerData.isClippedToProject = tab->isClippedToProject();
             viewerData.autoContrastEnabled = tab->isAutoContrastEnabled();
             viewerData.gain = tab->getGain();
             viewerData.colorSpace = tab->getColorSpace();
             viewerData.channels = tab->getChannelsString();
             viewerData.renderScaleActivated = tab->getRenderScaleActivated();
             viewerData.mipMapLevel = tab->getMipMapLevel();
             viewerData.zoomOrPanSinceLastFit = tab->getZoomOrPannedSinceLastFit();
             viewerData.wipeCompositingOp = (int)tab->getCompositingOperator();
             viewerData.frameRangeLocked = tab->isFrameRangeLocked();
             _viewersData.insert(std::make_pair(viewer->getNode()->getName_mt_safe(),viewerData));
         }
     }
    
    _layoutSerialization.initialize(projectGui->getGui());
         
     ///save histograms
    std::list<Histogram*> histograms = projectGui->getGui()->getHistograms_mt_safe();
     for (std::list<Histogram*>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
         _histograms.push_back((*it)->objectName().toStdString());
     }
    
    std::list<NodeBackDrop*> backdrops = projectGui->getGui()->getNodeGraph()->getActiveBackDrops();
    for (std::list<NodeBackDrop*>::iterator it = backdrops.begin(); it!=backdrops.end();++it) {
        NodeBackDropSerialization s;
        s.initialize(*it);
        _backdrops.push_back(s);
    }
    
    ///save opened panels by order
    QVBoxLayout* propLayout = projectGui->getGui()->getPropertiesLayout();
    for (int i = 0; i < propLayout->count(); ++i) {
        DockablePanel* isPanel = dynamic_cast<DockablePanel*>(propLayout->itemAt(i)->widget());
        if (isPanel && isPanel->isVisible()) {
            KnobHolder* holder = isPanel->getHolder();
            assert(holder);
            NamedKnobHolder* namedHolder = dynamic_cast<NamedKnobHolder*>(holder);
            if (namedHolder) {
                _openedPanelsOrdered.push_back(namedHolder->getName_mt_safe());
            } else if (holder->isProject()) {
                _openedPanelsOrdered.push_back("Natron_Project_Settings_Panel");
            }
        }
    }
    
}

void GuiLayoutSerialization::initialize(const Gui* gui)
{
    saveLayout(gui, _layout, _splittersStates);
}
