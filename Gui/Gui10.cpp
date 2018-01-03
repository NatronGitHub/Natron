/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <boost/algorithm/clamp.hpp>

#include <QApplication> // qApp
#include <QDesktopWidget>

#include "Global/FStreamsSupport.h"

#include "Engine/RotoLayer.h"

#include "Gui/DockablePanel.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/FloatingWidget.h"
#include "Gui/Histogram.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ProjectGui.h"
#include "Gui/PropertiesBinWrapper.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/PythonPanels.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h" // removeFileExtension

#include "Serialization/SerializationIO.h"
#include "Serialization/WorkspaceSerialization.h"

#ifdef __NATRON_WIN32__
#if _WIN32_WINNT < 0x0500
#define _WIN32_WINNT 0x0500
#endif
#endif

NATRON_NAMESPACE_ENTER

void
Gui::createDefaultLayout1()
{
    ///First tab widget must be created this way
    TabWidget* mainPane = new TabWidget(this, _imp->_leftRightSplitter);
    getApp()->registerTabWidget(mainPane);
    mainPane->setScriptName("pane1");
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

    workshopPane->moveTab(_imp->_nodeGraphArea, _imp->_nodeGraphArea);
    workshopPane->moveTab(_imp->_animationModule, _imp->_animationModule);
    propertiesPane->moveTab(_imp->_propertiesBin, _imp->_propertiesBin);

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it2 = _imp->_viewerTabs.begin(); it2 != _imp->_viewerTabs.end(); ++it2) {
            mainPane->moveTab(*it2, *it2);
        }
    }
    {
        QMutexLocker l(&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it2 = _imp->_histograms.begin(); it2 != _imp->_histograms.end(); ++it2) {
            mainPane->moveTab(*it2, *it2);
        }
    }


    ///Default to NodeGraph displayed
    workshopPane->setCurrentIndex(0);
}


void
Gui::restoreLayout(bool wipePrevious,
                   bool enableOldProjectCompatibility,
                   const SERIALIZATION_NAMESPACE::WorkspaceSerialization& layoutSerialization)
{
    ///Wipe the current layout
    if (wipePrevious) {
        wipeLayout();
    }

    ///For older projects prior to the layout change, just set default layout.
    if (enableOldProjectCompatibility) {
        createDefaultLayout1();
    } else {

        // Restore histograms
        const std::list<std::string> & histograms = layoutSerialization._histograms;
        for (std::list<std::string>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
            Histogram* h = addNewHistogram();
            h->setObjectName( QString::fromUtf8( (*it).c_str() ) );
            //move it by default to the viewer pane, before restoring the layout anyway which
            ///will relocate it correctly
            appendTabToDefaultViewerPane(h, h);
        }

        // Restore user python panels
        const std::list<SERIALIZATION_NAMESPACE::PythonPanelSerialization >& pythonPanels = layoutSerialization._pythonPanels;
        if ( !pythonPanels.empty() ) {
            std::string appID = getApp()->getAppIDString();
            std::string err;
            std::string script = "app = " + appID + '\n';
            bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
            (void)ok;
        }
        for (std::list<SERIALIZATION_NAMESPACE::PythonPanelSerialization >::const_iterator it = pythonPanels.begin(); it != pythonPanels.end(); ++it) {
            std::string script = it->pythonFunction + "()\n";
            std::string err, output;
            if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
                getApp()->appendToScriptEditor(err);
            } else {
                if ( !output.empty() ) {
                    getApp()->appendToScriptEditor(output);
                }
            }
            const RegisteredTabs& registeredTabs = getRegisteredTabs();
            RegisteredTabs::const_iterator found = registeredTabs.find(it->name );
            if ( found != registeredTabs.end() ) {
                NATRON_PYTHON_NAMESPACE::PyPanel* panel = dynamic_cast<NATRON_PYTHON_NAMESPACE::PyPanel*>(found->second.first);
                if (panel) {
                    panel->fromSerialization(*it);
                }
            }
        }

        fromSerialization(*layoutSerialization._mainWindowSerialization);
        for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::WindowSerialization> >::const_iterator it = layoutSerialization._floatingWindowsSerialization.begin(); it!= layoutSerialization._floatingWindowsSerialization.end(); ++it) {
            FloatingWidget* window = new FloatingWidget(this);
            getApp()->registerFloatingWindow(window);
            window->fromSerialization(**it);
        }


    }
} // restoreLayout

void
Gui::restoreChildFromSerialization(const SERIALIZATION_NAMESPACE::WindowSerialization& serialization)
{
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screen = desktop->screenGeometry(desktop->screenNumber(this));
    if (serialization.childType == kSplitterChildTypeSplitter) {
        assert(serialization.isChildSplitter);
        Qt::Orientation orientation = Qt::Horizontal;
        if (serialization.isChildSplitter->orientation == kSplitterOrientationHorizontal) {
            orientation = Qt::Horizontal;
        } else if (serialization.isChildSplitter->orientation == kSplitterOrientationVertical) {
            orientation = Qt::Vertical;
        }
        Splitter* splitter = new Splitter(orientation, this, this);
        _imp->_leftRightSplitter->addWidget_mt_safe(splitter);
        getApp()->registerSplitter(splitter);
        splitter->fromSerialization(*serialization.isChildSplitter);
    } else if (serialization.childType == kSplitterChildTypeTabWidget) {
        assert(serialization.isChildTabWidget);
        TabWidget* tab = new TabWidget(this, this);
        _imp->_leftRightSplitter->addWidget_mt_safe(tab);
        getApp()->registerTabWidget(tab);
        tab->fromSerialization(*serialization.isChildTabWidget);
    }


    // Restore geometry
    int w = std::min( serialization.windowSize[0], screen.width() );
    int h = std::min( serialization.windowSize[1], screen.height() );
    resize(w, h);

    // If the screen size changed, make sure at least 50x50 pixels of the window are visible
    int x = boost::algorithm::clamp( serialization.windowPosition[0], screen.left(), screen.right() );
    int y = boost::algorithm::clamp( serialization.windowPosition[0], screen.top(), screen.bottom() );
    move( QPoint(x, y) );

}

void
Gui::exportLayout()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this, filters, false, SequenceFileDialog::eFileDialogModeSave, _imp->_lastSaveProjectOpenedDir.toStdString(), this, false );
    if ( dialog.exec() ) {
        std::string filename = dialog.filesToSave();
        QString filenameCpy( QString::fromUtf8( filename.c_str() ) );
        QString ext = QtCompat::removeFileExtension(filenameCpy);
        if ( ext != QString::fromUtf8(NATRON_LAYOUT_FILE_EXT) ) {
            filename.append("." NATRON_LAYOUT_FILE_EXT);
        }


        FStreamsSupport::ofstream ofile;
        FStreamsSupport::open(&ofile, filename);
        if (!ofile) {
            Dialogs::errorDialog( tr("Error").toStdString()
                                  , tr("Failed to open file ").toStdString() + filename, false );

            return;
        }

        try {
            SERIALIZATION_NAMESPACE::WorkspaceSerialization s;
            getApp()->saveApplicationWorkspace(&s);
            SERIALIZATION_NAMESPACE::write(ofile, s, NATRON_LAYOUT_FILE_HEADER);
        } catch (...) {
            Dialogs::errorDialog( tr("Error").toStdString()
                                 , tr("Failed to save workspace").toStdString(), false );
            
            return;
        }
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

void
Gui::onShowApplicationConsoleActionTriggered()
{
    setApplicationConsoleActionVisible(!_imp->applicationConsoleVisible);
}

void
Gui::setApplicationConsoleActionVisible(bool visible)
{
#ifdef __NATRON_WIN32__
    if (visible == _imp->applicationConsoleVisible) {
        return;
    }
    _imp->applicationConsoleVisible = visible;
    HWND hWnd = GetConsoleWindow();
    if (hWnd) {
        if (_imp->applicationConsoleVisible) {
            ShowWindow(hWnd, SW_HIDE);
        } else {
            ShowWindow(hWnd, SW_SHOWNORMAL);
        }
    }
#else
    Q_UNUSED(visible)
#endif
}

NATRON_NAMESPACE_EXIT

