/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include <QApplication>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QScrollArea>
#include <QAction>

#include "Global/FStreamsSupport.h"

#include "Engine/Settings.h"
#include "Engine/ViewIdx.h"

#include "Gui/DockablePanel.h"
#include "Gui/Menu.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/ProjectGui.h"
#include "Gui/SequenceFileDialog.h"

#include "Serialization/WorkspaceSerialization.h"
#include "Serialization/SerializationIO.h"
SERIALIZATION_NAMESPACE_USING

NATRON_NAMESPACE_ENTER

void
Gui::importLayoutInternal(const std::string& filename)
{
    FStreamsSupport::ifstream ifile;
    FStreamsSupport::open(&ifile, filename);
    if (!ifile) {
        Dialogs::errorDialog( tr("Error").toStdString(), tr("Failed to open file: ").toStdString() + filename, false );

        return;
    }
    
    try {
        WorkspaceSerialization s;
        try {
            SERIALIZATION_NAMESPACE::read(NATRON_LAYOUT_FILE_HEADER, ifile, &s);
        } catch (SERIALIZATION_NAMESPACE::InvalidSerializationFileException& e) {
            Dialogs::errorDialog( tr("Error").toStdString(), tr("Failed to open %1: this file does not appear to be a layout file").arg(QString::fromUtf8(filename.c_str())).toStdString());
            return;
        }
        restoreLayout(true, false, s);

    }  catch (const std::exception & e) {
        QString err = QString::fromUtf8("Failed to open file %1: %2").arg( QString::fromUtf8( filename.c_str() ) ).arg( QString::fromUtf8( e.what() ) );
        Dialogs::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

        return;
    }
}

void
Gui::importLayout()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this, filters, false, SequenceFileDialog::eFileDialogModeOpen, _imp->_lastLoadProjectOpenedDir.toStdString(), this, false );
    if ( dialog.exec() ) {
        std::string filename = dialog.selectedFiles();
        importLayoutInternal(filename);
    }
}

void
Gui::createDefaultLayoutInternal(bool wipePrevious)
{
    if (wipePrevious) {
        wipeLayout();
    }

    std::string fileLayout = appPTR->getCurrentSettings()->getDefaultLayoutFile();
    if ( !fileLayout.empty() ) {
        importLayoutInternal(fileLayout);
    } else {
        createDefaultLayout1();
    }
}

void
Gui::restoreDefaultLayout()
{
    createDefaultLayoutInternal(true);
}

void
Gui::initProjectGuiKnobs()
{
    assert(_imp->_projectGui);
    _imp->_projectGui->initializeKnobsGui();
}

QKeySequence
Gui::keySequenceForView(ViewIdx v)
{
    switch ( static_cast<int>(v) ) {
    case 0:

        return QKeySequence(Qt::CTRL + Qt::ALT +  Qt::Key_1);
        break;
    case 1:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_2);
        break;
    case 2:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_3);
        break;
    case 3:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_4);
        break;
    case 4:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_5);
        break;
    case 5:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_6);
        break;
    case 6:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_7);
        break;
    case 7:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_8);
        break;
    case 8:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_9);
        break;
    case 9:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_0);
        break;
    default:

        return QKeySequence();
    }
}

static const char*
slotForView(int view)
{
    switch (view) {
    case 0:

        return SLOT(showView0());
        break;
    case 1:

        return SLOT(showView1());
        break;
    case 2:

        return SLOT(showView2());
        break;
    case 3:

        return SLOT(showView3());
        break;
    case 4:

        return SLOT(showView4());
        break;
    case 5:

        return SLOT(showView5());
        break;
    case 6:

        return SLOT(showView6());
        break;
    case 7:

        return SLOT(showView7());
        break;
    case 8:

        return SLOT(showView7());
        break;
    case 9:

        return SLOT(showView8());
        break;
    default:

        return NULL;
    }
}

void
Gui::updateViewsActions(int viewsCount)
{
    _imp->viewersViewMenu->clear();
    //if viewsCount == 1 we don't add a menu entry
    _imp->viewersMenu->removeAction( _imp->viewersViewMenu->menuAction() );
    if (viewsCount == 2) {
        QAction* left = new QAction(this);
        left->setCheckable(false);
        left->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_1) );
        _imp->viewersViewMenu->addAction(left);
        left->setText( tr("Display Left View") );
        QObject::connect( left, SIGNAL(triggered()), this, SLOT(showView0()) );
        QAction* right = new QAction(this);
        right->setCheckable(false);
        right->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_2) );
        _imp->viewersViewMenu->addAction(right);
        right->setText( tr("Display Right View") );
        QObject::connect( right, SIGNAL(triggered()), this, SLOT(showView1()) );

        _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    } else if (viewsCount > 2) {
        for (int i = 0; i < viewsCount; ++i) {
            if (i > 9) {
                break;
            }
            QAction* viewI = new QAction(this);
            viewI->setCheckable(false);
            QKeySequence seq = keySequenceForView( ViewIdx(i) );
            if ( !seq.isEmpty() ) {
                viewI->setShortcut(seq);
            }
            _imp->viewersViewMenu->addAction(viewI);
            const char* slot = slotForView(i);
            viewI->setText( QString( tr("Display View ") ) + QString::number(i + 1) );
            if (slot) {
                QObject::connect(viewI, SIGNAL(triggered()), this, slot);
            }
        }

        _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    }
}

void
Gui::putSettingsPanelFirst(DockablePanel* panel)
{
    if (!panel) {
        return;
    }
    std::list<DockablePanelI*> panels = getApp()->getOpenedSettingsPanels();
    std::list<DockablePanelI*>::iterator it = std::find(panels.begin(), panels.end(), panel);
    if ( it == panels.end() ) {
        return;
    }
    if (panel->isFloating()) {
        panel->activateWindow();
    } else {
        panels.erase(it);
        panels.push_front(panel);
        getApp()->setOpenedSettingsPanelsInternal(panels);
        panel->setParent( _imp->_layoutPropertiesBin->parentWidget() );
        _imp->_layoutPropertiesBin->removeWidget(panel);
        _imp->_layoutPropertiesBin->insertWidget(0, panel);
        _imp->_propertiesScrollArea->verticalScrollBar()->setValue(0);
        if ( !panel->isVisible() ) {
            panel->setVisible(true);
        }
        buildTabFocusOrderPropertiesBin();
    }

    // Refresh timeline's keyframes
    refreshTimelineGuiKeyframesLater();


}

void
Gui::addVisibleDockablePanel(DockablePanel* panel)
{


    int nbDockedPanels = 0;
    DockablePanel* foundPanel = 0;
    assert(panel);
    int maxPanels = appPTR->getCurrentSettings()->getMaxPanelsOpened();
    do {
        DockablePanel* first = 0;

        {
            std::list<DockablePanelI*> panels = getApp()->getOpenedSettingsPanels();
            for (std::list<DockablePanelI*>::iterator it = panels.begin(); it != panels.end(); ++it) {
                DockablePanel* isPanel = dynamic_cast<DockablePanel*>(*it);
                if (!isPanel) {
                    continue;
                }
                if ( !isPanel->isFloating() ) {
                    if ( !first && (*it != panel) ) {
                        first = isPanel;
                    }
                    ++nbDockedPanels;
                }
                if (panel == isPanel) {
                    foundPanel = isPanel;
                }
            }
        }

        // If the panel is already here, don't remove anything just put it first
        if (foundPanel && (nbDockedPanels <= maxPanels || maxPanels == 0)) {
            break;
        }
        if ((nbDockedPanels >= maxPanels) && (maxPanels != 0)) {
            if (first) {
                first->closePanel();
            } else {
                break;
            }

            nbDockedPanels = 0;

            std::list<DockablePanelI*> panels = getApp()->getOpenedSettingsPanels();
            for (std::list<DockablePanelI*>::iterator it = panels.begin(); it != panels.end(); ++it) {
                DockablePanel* isPanel = dynamic_cast<DockablePanel*>(*it);
                if (!isPanel) {
                    continue;
                }
                if ( !isPanel->isFloating() ) {
                    ++nbDockedPanels;
                }
                
            }
        }

    }
    while ( (nbDockedPanels >= maxPanels) && (maxPanels != 0) );



    if (foundPanel) {
        putSettingsPanelFirst(panel);
    } else {
        if (!panel->isFloating()) {
            panel->setParent( _imp->_layoutPropertiesBin->parentWidget() );
            _imp->_layoutPropertiesBin->insertWidget(0, panel);
            _imp->_propertiesScrollArea->verticalScrollBar()->setValue(0);
        }
        if ( !panel->isVisible() ) {
            panel->setVisible(true);
        }
        getApp()->registerSettingsPanel(panel);

        // Refresh timeline's keyframes, this is also done in putSettingsPanelFirst
        refreshTimelineGuiKeyframesLater();
    }



} // Gui::addVisibleDockablePanel

void
Gui::removeVisibleDockablePanel(DockablePanel* panel)
{

    getApp()->unregisterSettingsPanel(panel);
    // Refresh timeline's keyframes
    refreshTimelineGuiKeyframesLater();
}


void
Gui::buildTabFocusOrderPropertiesBin()
{
    int next = 1;

    for (int i = 0; i < _imp->_layoutPropertiesBin->count(); ++i, ++next) {
        QLayoutItem* item = _imp->_layoutPropertiesBin->itemAt(i);
        QWidget* w = item->widget();
        QWidget* nextWidget = _imp->_layoutPropertiesBin->itemAt(next < _imp->_layoutPropertiesBin->count() ? next : 0)->widget();

        if (w && nextWidget) {
            setTabOrder(w, nextWidget);
        }
    }
}

void
Gui::setVisibleProjectSettingsPanel()
{
    if ( _imp->_projectGui->getPanel()->isClosed() ) {
        _imp->_projectGui->getPanel()->setClosed(false);
    } else {
        addVisibleDockablePanel( _imp->_projectGui->getPanel() );
    }
    if ( !_imp->_projectGui->isVisible() ) {
        _imp->_projectGui->setVisible(true);
    }
}

NATRON_NAMESPACE_EXIT
