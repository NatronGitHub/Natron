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

#include "Gui.h"
#include "GuiPrivate.h"

#include <cassert>
#include <fstream>
#include <algorithm> // min, max
#include <stdexcept>

#include "Global/Macros.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QScrollArea>
#include <QAction>

#include "Engine/Settings.h"
#include "Engine/FStreamsSupport.h"

#include "Gui/DockablePanel.h"
#include "Gui/Menu.h"
#include "Gui/ProjectGui.h"
#include "Gui/ProjectGuiSerialization.h" // PaneLayout, GuiLayoutSerialization
#include "Gui/SequenceFileDialog.h"


NATRON_NAMESPACE_ENTER;


void
Gui::importLayout()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this, filters, false, SequenceFileDialog::eFileDialogModeOpen, _imp->_lastLoadProjectOpenedDir.toStdString(), this, false );
    if ( dialog.exec() ) {
        std::string filename = dialog.selectedFiles();
        
        boost::shared_ptr<std::istream> ifile = FStreamsSupport::open_ifstream(filename);
        if (!ifile) {
            Dialogs::errorDialog( tr("Error").toStdString(), tr("Failed to open file: ").toStdString() + filename, false );
            return;
        }

        try {
            boost::archive::xml_iarchive iArchive(*ifile);
            GuiLayoutSerialization s;
            iArchive >> boost::serialization::make_nvp("Layout", s);
            restoreLayout(true, false, s);
        } catch (const boost::archive::archive_exception & e) {
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Dialogs::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

            return;
        } catch (const std::exception & e) {
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Dialogs::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

            return;
        }

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
        std::ifstream ifile;
        ifile.open( fileLayout.c_str() );
        if ( !ifile.is_open() ) {
            createDefaultLayout1();
        } else {
            try {
                boost::archive::xml_iarchive iArchive(ifile);
                GuiLayoutSerialization s;
                iArchive >> boost::serialization::make_nvp("Layout", s);
                restoreLayout(false, false, s);
            } catch (const boost::archive::archive_exception & e) {
                ifile.close();
                QString err = QString("Exception occured when opening file %1: %2").arg( fileLayout.c_str() ).arg( e.what() );
                Dialogs::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

                return;
            } catch (const std::exception & e) {
                ifile.close();
                QString err = QString("Exception occured when opening file %1: %2").arg( fileLayout.c_str() ).arg( e.what() );
                Dialogs::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

                return;
            }

            ifile.close();
        }
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
    switch (static_cast<int>(v)) {
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

        return SLOT( showView0() );
        break;
    case 1:

        return SLOT( showView1() );
        break;
    case 2:

        return SLOT( showView2() );
        break;
    case 3:

        return SLOT( showView3() );
        break;
    case 4:

        return SLOT( showView4() );
        break;
    case 5:

        return SLOT( showView5() );
        break;
    case 6:

        return SLOT( showView6() );
        break;
    case 7:

        return SLOT( showView7() );
        break;
    case 8:

        return SLOT( showView7() );
        break;
    case 9:

        return SLOT( showView8() );
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
        QObject::connect( left, SIGNAL( triggered() ), this, SLOT( showView0() ) );
        QAction* right = new QAction(this);
        right->setCheckable(false);
        right->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_2) );
        _imp->viewersViewMenu->addAction(right);
        right->setText( tr("Display Right View") );
        QObject::connect( right, SIGNAL( triggered() ), this, SLOT( showView1() ) );

        _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    } else if (viewsCount > 2) {
        for (int i = 0; i < viewsCount; ++i) {
            if (i > 9) {
                break;
            }
            QAction* viewI = new QAction(this);
            viewI->setCheckable(false);
            QKeySequence seq = keySequenceForView(ViewIdx(i));
            if ( !seq.isEmpty() ) {
                viewI->setShortcut(seq);
            }
            _imp->viewersViewMenu->addAction(viewI);
            const char* slot = slotForView(i);
            viewI->setText( QString( tr("Display View ") ) + QString::number(i + 1) );
            if (slot) {
                QObject::connect(viewI, SIGNAL( triggered() ), this, slot);
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
    {
        QMutexLocker k(&_imp->openedPanelsMutex);
        std::list<DockablePanel*>::iterator it = std::find(_imp->openedPanels.begin(), _imp->openedPanels.end(), panel);
        if ( it != _imp->openedPanels.end()) {
            if (!(*it)->isFloating()) {
                _imp->openedPanels.erase(it);
                _imp->openedPanels.push_front(panel);
                
                panel->setParent(_imp->_layoutPropertiesBin->parentWidget());
                _imp->_layoutPropertiesBin->removeWidget(panel);
                _imp->_layoutPropertiesBin->insertWidget(0, panel);
                _imp->_propertiesScrollArea->verticalScrollBar()->setValue(0);
                if (!panel->isVisible()) {
                    panel->setVisible(true);
                }
                buildTabFocusOrderPropertiesBin();

            } else {
                (*it)->activateWindow();
            }
        } else {
            return;
        }
    }
}


void
Gui::addVisibleDockablePanel(DockablePanel* panel)
{
    DockablePanel* found = 0;
    int nbDockedPanels = 0;
    {
        QMutexLocker k(&_imp->openedPanelsMutex);
        
        for (std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin(); it!= _imp->openedPanels.end(); ++it) {
            if (*it == panel) {
                found = *it;
            }
            if (!(*it)->isFloating()) {
                ++nbDockedPanels;
            }
        }
    }
    
    assert(panel);
    int maxPanels = appPTR->getCurrentSettings()->getMaxPanelsOpened();
    while ((nbDockedPanels >= maxPanels) && (maxPanels != 0)) {
        
        DockablePanel* first = 0;
        {
            QMutexLocker k(&_imp->openedPanelsMutex);
            for (std::list<DockablePanel*>::reverse_iterator it = _imp->openedPanels.rbegin(); it != _imp->openedPanels.rend();++it) {
                if (!(*it)->isFloating()) {
                    if (!first && *it != panel) {
                        first = *it;
                        break;
                    }
                }
            }
            
        }
        if (first) {
            first->closePanel();
        } else {
            break;
        }
        
        nbDockedPanels = 0;

        {
            QMutexLocker k(&_imp->openedPanelsMutex);
            for (std::list<DockablePanel*>::reverse_iterator it = _imp->openedPanels.rbegin(); it != _imp->openedPanels.rend();++it) {
                if (!(*it)->isFloating()) {
                    ++nbDockedPanels;
                }
            }
        }
    }

    
    if (found) {
        putSettingsPanelFirst(panel);
    } else {
        
        panel->setParent(_imp->_layoutPropertiesBin->parentWidget());
        _imp->_layoutPropertiesBin->insertWidget(0, panel);
        _imp->_propertiesScrollArea->verticalScrollBar()->setValue(0);
        if (!panel->isVisible()) {
            panel->setVisible(true);
        }
        
        QMutexLocker k(&_imp->openedPanelsMutex);
        _imp->openedPanels.push_front(panel);
    }
}

void
Gui::removeVisibleDockablePanel(DockablePanel* panel)
{
    QMutexLocker k(&_imp->openedPanelsMutex);
    std::list<DockablePanel*>::iterator it = std::find(_imp->openedPanels.begin(), _imp->openedPanels.end(), panel);
    
    if ( it != _imp->openedPanels.end() ) {
        _imp->openedPanels.erase(it);
    }
}

const std::list<DockablePanel*>&
Gui::getVisiblePanels() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->openedPanels;
}

std::list<DockablePanel*>
Gui::getVisiblePanels_mt_safe() const
{
    QMutexLocker k(&_imp->openedPanelsMutex);
    return _imp->openedPanels;
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
    if (_imp->_projectGui->getPanel()->isClosed()) {
        _imp->_projectGui->getPanel()->setClosed(false);
    } else {
        addVisibleDockablePanel( _imp->_projectGui->getPanel() );
    }
    if ( !_imp->_projectGui->isVisible() ) {
        _imp->_projectGui->setVisible(true);
    }
}

NATRON_NAMESPACE_EXIT;
