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

#ifndef Gui_DockablePanelPrivate_h
#define Gui_DockablePanelPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
#include <QFrame>
#include <QTabWidget>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/DockablePanelI.h"

#include "Gui/DockablePanel.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

struct Page
{
    QWidget* tab;
    int currentRow;
    TabGroup* groupAsTab; //< to gather group knobs that are set as a tab
    boost::weak_ptr<KnobPage> pageKnob;
    
    Page()
    : tab(0), currentRow(0),groupAsTab(0), pageKnob()
    {
    }

    Page(const Page & other)
    : tab(other.tab), currentRow(other.currentRow), groupAsTab(other.groupAsTab), pageKnob(other.pageKnob)
    {
    }
};

typedef std::map<QString,Page> PageMap;

struct DockablePanelPrivate
{
    DockablePanel* _publicInterface;
    Gui* _gui;
    QVBoxLayout* _container; /*!< ptr to the layout containing this DockablePanel*/

    /*global layout*/
    QVBoxLayout* _mainLayout;

    /*Header related*/
    QFrame* _headerWidget;
    QHBoxLayout *_headerLayout;
    LineEdit* _nameLineEdit; /*!< if the name is editable*/
    Natron::Label* _nameLabel; /*!< if the name is read-only*/

    QHBoxLayout* _horizLayout;
    QWidget* _horizContainer;
    VerticalColorBar* _verticalColorBar;

    /*Tab related*/
    QWidget* _rightContainer;
    QVBoxLayout* _rightContainerLayout;
    
    QTabWidget* _tabWidget;
    Button* _centerNodeButton;
    Button* _enterInGroupButton;
    Button* _helpButton;
    Button* _minimize;
    Button* _hideUnmodifiedButton;
    Button* _floatButton;
    Button* _cross;
    mutable QMutex _currentColorMutex;
    QColor _overlayColor;
    bool _hasOverlayColor;
    Button* _colorButton;
    Button* _overlayButton;
    Button* _undoButton;
    Button* _redoButton;
    Button* _restoreDefaultsButton;
    bool _minimized; /*!< true if the panel is minimized*/
    boost::shared_ptr<QUndoStack> _undoStack; /*!< undo/redo stack*/
    bool _floating; /*!< true if the panel is floating*/
    FloatingWidget* _floatingWidget;

    /*a map storing for each knob a pointer to their GUI.*/
    std::map<boost::weak_ptr<KnobI>,KnobGui*> _knobs;

    ///THe visibility of the knobs before the hide/show unmodified button is clicked
    ///to show only the knobs that need to afterwards
    std::map<KnobGui*,bool> _knobsVisibilityBeforeHideModif;
    KnobHolder* _holder;

    /* map<tab name, pair<tab , row count> >*/
    PageMap _pages;
    QString _defaultPageName;
    bool _useScrollAreasForTabs;
    DockablePanel::HeaderModeEnum _mode;
    mutable QMutex _isClosedMutex;
    bool _isClosed; //< accessed by serialization thread too

    QString _helpToolTip;
    QString _pluginID;
    unsigned _pluginVersionMajor,_pluginVersionMinor;


    bool _pagesEnabled;

    Natron::Label* _iconLabel;

    DockablePanelPrivate(DockablePanel* publicI,
                         Gui* gui,
                         KnobHolder* holder,
                         QVBoxLayout* container,
                         DockablePanel::HeaderModeEnum headerMode,
                         bool useScrollAreasForTabs,
                         const QString & defaultPageName,
                         const QString& helpToolTip,
                         const boost::shared_ptr<QUndoStack>& stack);
    
    /*inserts a new page to the dockable panel.*/
    PageMap::iterator getOrCreatePage(const boost::shared_ptr<KnobPage>& page);

    boost::shared_ptr<KnobPage> ensureDefaultPageKnobCreated() ;


    void initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> > & knobs,
                              QWidget* lastRowWidget);

    KnobGui* createKnobGui(const boost::shared_ptr<KnobI> &knob);

    /*Search an existing knob GUI in the map, otherwise creates
     the gui for the knob.*/
    KnobGui* findKnobGuiOrCreate( const boost::shared_ptr<KnobI> &knob,
                                 bool makeNewLine,
                                 QWidget* lastRowWidget,
                                 const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine = std::vector< boost::shared_ptr< KnobI > >() );

    PageMap::iterator getDefaultPage(const boost::shared_ptr<KnobI> &knob);
    
    void refreshPagesSecretness();
};

NATRON_NAMESPACE_EXIT;

#endif // Gui_DockablePanelPrivate_h
