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

#include "Gui.h"
#include "GuiPrivate.h"

#include <cassert>
#include <fstream>
#include <algorithm> // min, max
#include <stdexcept>

#include "Global/Macros.h"

#include <QApplication> // qApp
#include <QDesktopWidget>

#include "Engine/NodeSerialization.h"
#include "Engine/RotoLayer.h"

#include "Gui/CurveEditor.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/FloatingWidget.h"
#include "Gui/Histogram.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ProjectGui.h"
#include "Gui/ProjectGuiSerialization.h" // PaneLayout
#include "Gui/PropertiesBinWrapper.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h" // removeFileExtension

#ifdef __NATRON_WIN32__
#if _WIN32_WINNT < 0x0500
#define _WIN32_WINNT 0x0500
#endif
#endif

using namespace Natron;

void
Gui::createDefaultLayout1()
{
    ///First tab widget must be created this way
    TabWidget* mainPane = new TabWidget(this, _imp->_leftRightSplitter);
    {
        QMutexLocker l(&_imp->_panesMutex);
        _imp->_panes.push_back(mainPane);
    }

    mainPane->setObjectName_mt_safe("pane1");
    mainPane->setAsAnchor(true);

    _imp->_leftRightSplitter->addWidget(mainPane);

    QList<int> sizes;
    sizes << _imp->_toolBox->sizeHint().width() << width();
    _imp->_leftRightSplitter->setSizes_mt_safe(sizes);


    TabWidget* propertiesPane = mainPane->splitHorizontally(false);
    TabWidget* workshopPane = mainPane->splitVertically(false);
    Splitter* propertiesSplitter = dynamic_cast<Splitter*>( propertiesPane->parentWidget() );
    assert(propertiesSplitter);
    sizes.clear();
    sizes << width() * 0.65 << width() * 0.35;
    propertiesSplitter->setSizes_mt_safe(sizes);

    TabWidget::moveTab(_imp->_nodeGraphArea, _imp->_nodeGraphArea, workshopPane);
    TabWidget::moveTab(_imp->_curveEditor, _imp->_curveEditor, workshopPane);
    TabWidget::moveTab(_imp->_dopeSheetEditor, _imp->_dopeSheetEditor, workshopPane);
    TabWidget::moveTab(_imp->_propertiesBin, _imp->_propertiesBin, propertiesPane);

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it2 = _imp->_viewerTabs.begin(); it2 != _imp->_viewerTabs.end(); ++it2) {
            TabWidget::moveTab(*it2, *it2, mainPane);
        }
    }
    {
        QMutexLocker l(&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it2 = _imp->_histograms.begin(); it2 != _imp->_histograms.end(); ++it2) {
            TabWidget::moveTab(*it2, *it2, mainPane);
        }
    }


    ///Default to NodeGraph displayed
    workshopPane->makeCurrentTab(0);
}

static void
restoreTabWidget(TabWidget* pane,
                 const PaneLayout & serialization)
{
    ///Find out if the name is already used
    QString availableName = pane->getGui()->getAvailablePaneName( serialization.name.c_str() );

    pane->setObjectName_mt_safe(availableName);
    pane->setAsAnchor(serialization.isAnchor);
    const RegisteredTabs & tabs = pane->getGui()->getRegisteredTabs();
    for (std::list<std::string>::const_iterator it = serialization.tabs.begin(); it != serialization.tabs.end(); ++it) {
        RegisteredTabs::const_iterator found = tabs.find(*it);

        ///If the tab exists in the current project, move it
        if ( found != tabs.end() ) {
            TabWidget::moveTab(found->second.first, found->second.second, pane);
        }
    }
    pane->makeCurrentTab(serialization.currentIndex);
}

static void
restoreSplitterRecursive(Gui* gui,
                         Splitter* splitter,
                         const SplitterSerialization & serialization)
{
    Qt::Orientation qO;
    Natron::OrientationEnum nO = (Natron::OrientationEnum)serialization.orientation;

    switch (nO) {
    case Natron::eOrientationHorizontal:
        qO = Qt::Horizontal;
        break;
    case Natron::eOrientationVertical:
        qO = Qt::Vertical;
        break;
    default:
        throw std::runtime_error("Unrecognized splitter orientation");
        break;
    }
    splitter->setOrientation(qO);

    if (serialization.children.size() != 2) {
        throw std::runtime_error("Splitter has a child count that is not 2");
    }

    for (std::vector<SplitterSerialization::Child*>::const_iterator it = serialization.children.begin();
         it != serialization.children.end(); ++it) {
        if ( (*it)->child_asSplitter ) {
            Splitter* child = new Splitter(splitter);
            splitter->addWidget_mt_safe(child);
            restoreSplitterRecursive( gui, child, *( (*it)->child_asSplitter ) );
        } else {
            assert( (*it)->child_asPane );
            TabWidget* pane = new TabWidget(gui, splitter);
            gui->registerPane(pane);
            splitter->addWidget_mt_safe(pane);
            restoreTabWidget( pane, *( (*it)->child_asPane ) );
        }
    }

    splitter->restoreNatron( serialization.sizes.c_str() );
}

void
Gui::restoreLayout(bool wipePrevious,
                   bool enableOldProjectCompatibility,
                   const GuiLayoutSerialization & layoutSerialization)
{
    ///Wipe the current layout
    if (wipePrevious) {
        wipeLayout();
    }

    ///For older projects prior to the layout change, just set default layout.
    if (enableOldProjectCompatibility) {
        createDefaultLayout1();
    } else {
        std::list<ApplicationWindowSerialization*> floatingDockablePanels;

        QDesktopWidget* desktop = QApplication::desktop();
        QRect screen = desktop->screenGeometry();
        
        ///now restore the gui layout
        for (std::list<ApplicationWindowSerialization*>::const_iterator it = layoutSerialization._windows.begin();
             it != layoutSerialization._windows.end(); ++it) {
            QWidget* mainWidget = 0;

            ///The window contains only a pane (for the main window it also contains the toolbar)
            if ( (*it)->child_asPane ) {
                TabWidget* centralWidget = new TabWidget(this);
                registerPane(centralWidget);
                restoreTabWidget(centralWidget, *(*it)->child_asPane);
                mainWidget = centralWidget;
            }
            ///The window contains a splitter as central widget
            else if ( (*it)->child_asSplitter ) {
                Splitter* centralWidget = new Splitter(this);
                restoreSplitterRecursive(this, centralWidget, *(*it)->child_asSplitter);
                mainWidget = centralWidget;
            }
            ///The child is a dockable panel, restore it later
            else if ( !(*it)->child_asDockablePanel.empty() ) {
                assert(!(*it)->isMainWindow);
                floatingDockablePanels.push_back(*it);
                continue;
            }

            assert(mainWidget);
            if (!mainWidget) {
                continue;
            }
            QWidget* window;
            if ( (*it)->isMainWindow ) {
                // mainWidget->setParent(_imp->_leftRightSplitter);
                _imp->_leftRightSplitter->addWidget_mt_safe(mainWidget);
                window = this;
            } else {
                FloatingWidget* floatingWindow = new FloatingWidget(this, this);
                floatingWindow->setWidget(mainWidget);
                registerFloatingWindow(floatingWindow);
                window = floatingWindow;
            }

            ///Restore geometry
            int w = std::min((*it)->w,screen.width());
            int h = std::min((*it)->h,screen.height());
            window->resize(w, h);
            int x = std::min(std::max((*it)->x,screen.x()),screen.right());
            int y = std::min(std::max((*it)->y,screen.y()),screen.bottom());
            window->move(QPoint(x,y));
        }

        for (std::list<ApplicationWindowSerialization*>::iterator it = floatingDockablePanels.begin();
             it != floatingDockablePanels.end(); ++it) {
            ///Find the node associated to the floating panel if any and float it
            assert( !(*it)->child_asDockablePanel.empty() );
            if ( (*it)->child_asDockablePanel == kNatronProjectSettingsPanelSerializationName ) {
                _imp->_projectGui->getPanel()->floatPanel();
            } else {
                ///Find a node with the dockable panel name
                const std::list<boost::shared_ptr<NodeGui> > & nodes = getNodeGraph()->getAllActiveNodes();
                DockablePanel* panel = 0;
                for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
                    if ( (*it2)->getNode()->getScriptName() == (*it)->child_asDockablePanel ) {
                        (*it2)->ensurePanelCreated();
                        NodeSettingsPanel* nodeSettings = (*it2)->getSettingPanel();
                        if (nodeSettings) {
                            nodeSettings->floatPanel();
                            panel = nodeSettings;
                        }
                        break;
                    }
                }
                if (panel) {
                    FloatingWidget* fWindow = 0;
                    QWidget* w = panel->parentWidget();
                    while (!fWindow && w) {
                        fWindow = dynamic_cast<FloatingWidget*>(w);
                        w = w->parentWidget();
                    }
                    
                    assert(fWindow);
                    fWindow->move( QPoint( (*it)->x, (*it)->y ) );
                    fWindow->resize(std::min((*it)->w,screen.width()), std::min((*it)->h,screen.height()));
                }
            }
        }
    }
} // restoreLayout

void
Gui::exportLayout()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this, filters, false, SequenceFileDialog::eFileDialogModeSave, _imp->_lastSaveProjectOpenedDir.toStdString(), this, false );
    if ( dialog.exec() ) {
        std::string filename = dialog.filesToSave();
        QString filenameCpy( filename.c_str() );
        QString ext = Natron::removeFileExtension(filenameCpy);
        if (ext != NATRON_LAYOUT_FILE_EXT) {
            filename.append("." NATRON_LAYOUT_FILE_EXT);
        }

        std::ofstream ofile;
        try {
            ofile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ofile.open(filename.c_str(), std::ofstream::out);
        } catch (const std::ofstream::failure & e) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Exception occured when opening file").toStdString(), false );

            return;
        }

        if ( !ofile.good() ) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Failure to open the file").toStdString(), false );

            return;
        }

        try {
            boost::archive::xml_oarchive oArchive(ofile);
            GuiLayoutSerialization s;
            s.initialize(this);
            oArchive << boost::serialization::make_nvp("Layout", s);
        }catch (...) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Failure when saving the layout").toStdString(), false );
            ofile.close();

            return;
        }

        ofile.close();
    }
}

const QString &
Gui::getLastLoadProjectDirectory() const
{
    return _imp->_lastLoadProjectOpenedDir;
}

const QString &
Gui::getLastSaveProjectDirectory() const
{
    return _imp->_lastSaveProjectOpenedDir;
}

const QString &
Gui::getLastPluginDirectory() const
{
    return _imp->_lastPluginDir;
}

void
Gui::updateLastPluginDirectory(const QString & str)
{
    _imp->_lastPluginDir = str;
}

#ifdef __NATRON_WIN32__

void
Gui::onShowApplicationConsoleActionTriggered()
{
    setApplicationConsoleActionVisible(!_imp->applicationConsoleVisible);
}

void
Gui::setApplicationConsoleActionVisible(bool visible)
{
    if (visible == _imp->applicationConsoleVisible) {
        return;
    }
    _imp->applicationConsoleVisible = visible;
    HWND hWnd = GetConsoleWindow();
    if (hWnd) {
        ShowWindow(hWnd, _imp->applicationConsoleVisible ? SW_SHOW : SW_HIDE );
    }

}
#endif

