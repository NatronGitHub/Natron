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

#include "ProjectGuiSerialization.h"

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDebug>
#include <QSplitter>
#include <QVBoxLayout>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/ParameterWrapper.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"

#include "Gui/DockablePanel.h"
#include "Gui/FloatingWidget.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Histogram.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/ProjectGui.h"
#include "Gui/PythonPanels.h"
#include "Gui/ScriptEditor.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_ENTER;

void
ProjectGuiSerialization::initialize(const ProjectGui* projectGui)
{
    NodesList activeNodes;
    projectGui->getInternalProject()->getActiveNodesExpandGroups(&activeNodes);
    
    _serializedNodes.clear();
    for (NodesList::iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
        boost::shared_ptr<NodeGuiI> nodegui_i = (*it)->getNodeGui();
        if (!nodegui_i) {
            continue;
        }
        NodeGuiPtr nodegui = boost::dynamic_pointer_cast<NodeGui>(nodegui_i);
        
        if (nodegui->isVisible()) {
            
            boost::shared_ptr<NodeCollection> isInCollection = (*it)->getGroup();
            NodeGroup* isCollectionAGroup = dynamic_cast<NodeGroup*>(isInCollection.get());
            if (!isCollectionAGroup) {
                ///Nodes within a group will be serialized recursively in the node group serialization
                NodeGuiSerialization state;
                nodegui->serialize(&state);
                _serializedNodes.push_back(state);
            }
            ViewerInstance* viewer = (*it)->isEffectViewer();
            if (viewer) {
                ViewerTab* tab = projectGui->getGui()->getViewerTabForInstance(viewer);
                assert(tab);
                ViewerData viewerData;
                double zoompar;
                tab->getViewer()->getProjection(&viewerData.zoomLeft, &viewerData.zoomBottom, &viewerData.zoomFactor, &zoompar);
                viewerData.userRoI = tab->getViewer()->getUserRegionOfInterest();
                viewerData.userRoIenabled = tab->getViewer()->isUserRegionOfInterestEnabled();
                viewerData.isClippedToProject = tab->isClippedToProject();
                viewerData.autoContrastEnabled = tab->isAutoContrastEnabled();
                viewerData.gain = tab->getGain();
                viewerData.gamma = tab->getGamma();
                viewerData.colorSpace = tab->getColorSpace();
                viewerData.channels = tab->getChannelsString();
                viewerData.renderScaleActivated = tab->getRenderScaleActivated();
                viewerData.mipMapLevel = tab->getMipMapLevel();
                viewerData.zoomOrPanSinceLastFit = tab->getZoomOrPannedSinceLastFit();
                viewerData.wipeCompositingOp = (int)tab->getCompositingOperator();
                viewerData.leftToolbarVisible = tab->isLeftToolbarVisible();
                viewerData.rightToolbarVisible = tab->isRightToolbarVisible();
                viewerData.topToolbarVisible = tab->isTopToolbarVisible();
                viewerData.infobarVisible = tab->isInfobarVisible();
                viewerData.playerVisible = tab->isPlayerVisible();
                viewerData.timelineVisible = tab->isTimelineVisible();
                viewerData.checkerboardEnabled = tab->isCheckerboardEnabled();
                viewerData.fps = tab->getDesiredFps();
                viewerData.fpsLocked = tab->isFPSLocked();
                tab->getTimelineBounds(&viewerData.leftBound, &viewerData.rightBound);
                tab->getActiveInputs(&viewerData.aChoice, &viewerData.bChoice);
                viewerData.version = VIEWER_DATA_SERIALIZATION_VERSION;
                _viewersData.insert( std::make_pair(viewer->getNode()->getScriptName_mt_safe(),viewerData) );
            }
        }
    }

    ///Init windows
    _layoutSerialization.initialize( projectGui->getGui() );

    ///save histograms
    std::list<Histogram*> histograms = projectGui->getGui()->getHistograms_mt_safe();
    for (std::list<Histogram*>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
        _histograms.push_back( (*it)->objectName().toStdString() );
    }

    ///save opened panels by order
    
    std::list<DockablePanel*> panels = projectGui->getGui()->getVisiblePanels_mt_safe();
    for (std::list<DockablePanel*>::iterator it = panels.begin(); it!=panels.end(); ++it) {
        if ((*it)->isVisible() ) {
            KnobHolder* holder = (*it)->getHolder();
            assert(holder);
            
            EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
            Project* isProject = dynamic_cast<Project*>(holder);

            if (isProject) {
                _openedPanelsOrdered.push_back(kNatronProjectSettingsPanelSerializationName);
            } else if (isEffect) {
                _openedPanelsOrdered.push_back(isEffect->getNode()->getFullyQualifiedName());
            } 
        }
    }
    
    _scriptEditorInput = projectGui->getGui()->getScriptEditor()->getAutoSavedScript().toStdString();
    
    std::map<PyPanel*,std::string> pythonPanels = projectGui->getGui()->getPythonPanels();
    for ( std::map<PyPanel*,std::string>::iterator it = pythonPanels.begin(); it != pythonPanels.end(); ++it) {
        boost::shared_ptr<PythonPanelSerialization> s(new PythonPanelSerialization);
        s->initialize(it->first, it->second);
        _pythonPanels.push_back(s);
    }
} // initialize

void
PaneLayout::initialize(TabWidget* tab)
{
    QStringList children = tab->getTabScriptNames();

    for (int i = 0; i < children.size(); ++i) {
        tabs.push_back( children[i].toStdString() );
    }
    currentIndex = tab->activeIndex();
    name = tab->objectName_mt_safe().toStdString();
    isAnchor = tab->isAnchor();
}

void
SplitterSerialization::initialize(Splitter* splitter)
{
    sizes = splitter->serializeNatron().toStdString();
    OrientationEnum nO = eOrientationHorizontal;
    Qt::Orientation qO = splitter->orientation();
    switch (qO) {
    case Qt::Horizontal:
        nO = eOrientationHorizontal;
        break;
    case Qt::Vertical:
        nO = eOrientationVertical;
        break;
    default:
        assert(false);
        break;
    }
    orientation = (int)nO;
    std::list<QWidget*> ch;
    splitter->getChildren_mt_safe(ch);
    assert(ch.size() == 2);

    for (std::list<QWidget*>::iterator it = ch.begin(); it != ch.end(); ++it) {
        Child *c = new Child;
        Splitter* isSplitter = dynamic_cast<Splitter*>(*it);
        TabWidget* isTabWidget = dynamic_cast<TabWidget*>(*it);
        if (isSplitter) {
            c->child_asSplitter = new SplitterSerialization;
            c->child_asSplitter->initialize(isSplitter);
        } else if (isTabWidget) {
            c->child_asPane = new PaneLayout;
            c->child_asPane->initialize(isTabWidget);
        }
        children.push_back(c);
    }
}

void
ApplicationWindowSerialization::initialize(bool mainWindow,
                                           SerializableWindow* widget)
{
    isMainWindow = mainWindow;
    widget->getMtSafePosition(x, y);
    widget->getMtSafeWindowSize(w, h);

    if (mainWindow) {
        Gui* gui = dynamic_cast<Gui*>(widget);
        assert(gui);
        QWidget* centralWidget = gui->getCentralWidget();
        Splitter* isSplitter = dynamic_cast<Splitter*>(centralWidget);
        TabWidget* isTabWidget = dynamic_cast<TabWidget*>(centralWidget);

        assert(isSplitter || isTabWidget);

        if (isSplitter) {
            child_asSplitter = new SplitterSerialization;
            child_asSplitter->initialize(isSplitter);
        } else {
            child_asPane = new PaneLayout;
            child_asPane->initialize(isTabWidget);
        }
    } else {
        FloatingWidget* isFloating = dynamic_cast<FloatingWidget*>(widget);
        assert(isFloating);

        QWidget* embedded = isFloating->getEmbeddedWidget();
        Splitter* isSplitter = dynamic_cast<Splitter*>(embedded);
        TabWidget* isTabWidget = dynamic_cast<TabWidget*>(embedded);
        DockablePanel* isPanel = dynamic_cast<DockablePanel*>(embedded);
        assert(isSplitter || isTabWidget || isPanel);

        if (isSplitter) {
            child_asSplitter = new SplitterSerialization;
            child_asSplitter->initialize(isSplitter);
        } else if (isTabWidget) {
            child_asPane = new PaneLayout;
            child_asPane->initialize(isTabWidget);
        } else {
            ///A named knob holder is a knob holder which has a unique name.
            NamedKnobHolder* isNamedHolder = dynamic_cast<NamedKnobHolder*>( isPanel->getHolder() );
            if (isNamedHolder) {
                child_asDockablePanel = isNamedHolder->getScriptName_mt_safe();
            } else {
                ///This must be the project settings panel
                child_asDockablePanel = kNatronProjectSettingsPanelSerializationName;
            }
        }
    }
} // initialize

void
GuiLayoutSerialization::initialize(Gui* gui)
{
    ApplicationWindowSerialization* mainWindow = new ApplicationWindowSerialization;

    mainWindow->initialize(true, gui);
    _windows.push_back(mainWindow);

    std::list<FloatingWidget*> floatingWindows = gui->getFloatingWindows();
    for (std::list<FloatingWidget*>::iterator it = floatingWindows.begin(); it != floatingWindows.end(); ++it) {
        ApplicationWindowSerialization* window = new ApplicationWindowSerialization;
        window->initialize(false, *it);
        _windows.push_back(window);
    }
}


void
PythonPanelSerialization::initialize(PyPanel* tab,const std::string& func)
{
    name = tab->getLabel();
    pythonFunction = func;
    std::list<Param*> parameters = tab->getParams();
    for (std::list<Param*>::iterator it = parameters.begin(); it != parameters.end(); ++it) {
        
        KnobPtr knob = (*it)->getInternalKnob();
        KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knob.get() );
        KnobPage* isPage = dynamic_cast<KnobPage*>( knob.get() );
        KnobButton* isButton = dynamic_cast<KnobButton*>( knob.get() );
        //KnobChoice* isChoice = dynamic_cast<KnobChoice*>( knob.get() );
        
        if (!isGroup && !isPage && !isButton) {
            boost::shared_ptr<KnobSerialization> k(new KnobSerialization(knob));
            knobs.push_back(k);
        }
        delete *it;
    }
    
    userData = tab->save_serialization_thread();
}

NATRON_NAMESPACE_EXIT;
