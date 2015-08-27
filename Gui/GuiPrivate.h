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

#ifndef _Gui_GuiPrivate_h_
#define _Gui_GuiPrivate_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <map>
#include <vector>

#include "Global/Macros.h"

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QToolBar>
#include <QMessageBox>
class QAction;
class QEvent;
class QHBoxLayout;
class QVBoxLayout;
class QMenuBar;
class QProgressDialog;
class QScrollArea;
class QUndoGroup;
class QUndoStack;
class QWidget;
class QToolButton;

#include "Global/Enums.h"

#include "Gui/RegisteredTabs.h"

class AboutWindow;
class ActionWithShortcut;
class Button;
class CurveEditor;
class DopeSheetEditor;
class FloatingWidget;
class Histogram;
class Gui;
class GuiAppInstance;
namespace Natron {
    class Menu;
}
class NodeGraph;
class PreferencesPanel;
class ProjectGui;
class PropertiesBinWrapper;
class PyPanel;
class ShortCutEditor;
class SpinBox;
class Splitter;
class TabWidget;
class ToolButton;
class ViewerTab;
class KnobHolder;
class DockablePanel;
class ScriptEditor;
class RenderStatsDialog;

#define kPropertiesBinName "properties"

struct GuiPrivate
{
    Gui* _gui; //< ptr to the public interface
    mutable QMutex _isInDraftModeMutex;
    bool _isInDraftMode; //< true if the user is actively moving the cursor on the timeline or a slider. False on mouse release.
    GuiAppInstance* _appInstance; //< ptr to the appInstance

    ///Dialogs handling members
    QWaitCondition _uiUsingMainThreadCond; //< used with _uiUsingMainThread
    bool _uiUsingMainThread; //< true when the Gui is showing a dialog in the main thread
    mutable QMutex _uiUsingMainThreadMutex; //< protects _uiUsingMainThread
    Natron::StandardButtonEnum _lastQuestionDialogAnswer; //< stores the last question answer
    bool _lastStopAskingAnswer;

    ///ptrs to the undo/redo actions from the active stack.
    QAction* _currentUndoAction;
    QAction* _currentRedoAction;

    ///all the undo stacks of Natron are gathered here
    QUndoGroup* _undoStacksGroup;
    std::map<QUndoStack*, std::pair<QAction*, QAction*> > _undoStacksActions;

    ///all the splitters used to separate the "panes" of the application
    mutable QMutex _splittersMutex;
    std::list<Splitter*> _splitters;
    mutable QMutex _pyPanelsMutex;
    std::map<PyPanel*, std::string> _userPanels;

    bool _isTripleSyncEnabled;
    
    mutable QMutex areRenderStatsEnabledMutex;
    bool areRenderStatsEnabled;

    ///all the menu actions
    ActionWithShortcut *actionNew_project;
    ActionWithShortcut *actionOpen_project;
    ActionWithShortcut *actionClose_project;
    ActionWithShortcut *actionSave_project;
    ActionWithShortcut *actionSaveAs_project;
    ActionWithShortcut *actionExportAsGroup;
    ActionWithShortcut *actionSaveAndIncrementVersion;
    ActionWithShortcut *actionPreferences;
    ActionWithShortcut *actionExit;
    ActionWithShortcut *actionProject_settings;
    ActionWithShortcut *actionShowOfxLog;
    ActionWithShortcut *actionShortcutEditor;
    ActionWithShortcut *actionNewViewer;
    ActionWithShortcut *actionFullScreen;
    ActionWithShortcut *actionClearDiskCache;
    ActionWithShortcut *actionClearPlayBackCache;
    ActionWithShortcut *actionClearNodeCache;
    ActionWithShortcut *actionClearPluginsLoadingCache;
    ActionWithShortcut *actionClearAllCaches;
    ActionWithShortcut *actionShowAboutWindow;
    QAction *actionsOpenRecentFile[NATRON_MAX_RECENT_FILES];
    ActionWithShortcut *renderAllWriters;
    ActionWithShortcut *renderSelectedNode;
    ActionWithShortcut *enableRenderStats;
    ActionWithShortcut* actionConnectInput1;
    ActionWithShortcut* actionConnectInput2;
    ActionWithShortcut* actionConnectInput3;
    ActionWithShortcut* actionConnectInput4;
    ActionWithShortcut* actionConnectInput5;
    ActionWithShortcut* actionConnectInput6;
    ActionWithShortcut* actionConnectInput7;
    ActionWithShortcut* actionConnectInput8;
    ActionWithShortcut* actionConnectInput9;
    ActionWithShortcut* actionConnectInput10;
    ActionWithShortcut* actionImportLayout;
    ActionWithShortcut* actionExportLayout;
    ActionWithShortcut* actionRestoreDefaultLayout;
    ActionWithShortcut* actionNextTab;
    ActionWithShortcut* actionPrevTab;
    ActionWithShortcut* actionCloseTab;

    ///the main "central" widget
    QWidget *_centralWidget;
    QHBoxLayout* _mainLayout; //< its layout

    ///strings that remember for project save/load and writers/reader dialog where
    ///the user was the last time.
    QString _lastLoadSequenceOpenedDir;
    QString _lastLoadProjectOpenedDir;
    QString _lastSaveSequenceOpenedDir;
    QString _lastSaveProjectOpenedDir;
    QString _lastPluginDir;

    // this one is a ptr to others TabWidget.
    //It tells where to put the viewer when making a new one
    // If null it places it on default tab widget
    TabWidget* _nextViewerTabPlace;

    ///the splitter separating the gui and the left toolbar
    Splitter* _leftRightSplitter;

    ///a list of ptrs to all the viewer tabs.
    mutable QMutex _viewerTabsMutex;
    std::list<ViewerTab*> _viewerTabs;

    ///Used when all viewers are synchronized to determine which one triggered the sync
    ViewerTab* _masterSyncViewer;

    ///a list of ptrs to all histograms
    mutable QMutex _histogramsMutex;
    std::list<Histogram*> _histograms;
    int _nextHistogramIndex; //< for giving a unique name to histogram tabs

    ///The node graph (i.e: the view of the scene)
    NodeGraph* _nodeGraphArea;
    NodeGraph* _lastFocusedGraph;
    std::list<NodeGraph*> _groups;

    ///The curve editor.
    CurveEditor *_curveEditor;

    // The dope sheet
    DopeSheetEditor *_dopeSheetEditor;

    ///the left toolbar
    QToolBar* _toolBox;

    ///a vector of all the toolbuttons
    std::vector<ToolButton* > _toolButtons;

    ///holds the properties dock
    PropertiesBinWrapper *_propertiesBin;
    QScrollArea* _propertiesScrollArea;
    QWidget* _propertiesContainer;

    ///the vertical layout for the properties dock container.
    QVBoxLayout *_layoutPropertiesBin;
    Button* _clearAllPanelsButton;
    Button* _minimizeAllPanelsButtons;
    SpinBox* _maxPanelsOpenedSpinBox;
    QMutex _isGUIFrozenMutex;
    bool _isGUIFrozen;

    ///The menu bar and all the menus
    QMenuBar *menubar;
    Natron::Menu *menuFile;
    Natron::Menu *menuRecentFiles;
    Natron::Menu *menuEdit;
    Natron::Menu *menuLayout;
    Natron::Menu *menuDisplay;
    Natron::Menu *menuOptions;
    Natron::Menu *menuRender;
    Natron::Menu *viewersMenu;
    Natron::Menu *viewerInputsMenu;
    Natron::Menu *viewersViewMenu;
    Natron::Menu *cacheMenu;


    ///all TabWidget's : used to know what to hide/show for fullscreen mode
    mutable QMutex _panesMutex;
    std::list<TabWidget*> _panes;
    mutable QMutex _floatingWindowMutex;
    std::list<FloatingWidget*> _floatingWindows;

    ///All the tabs used in the TabWidgets (used for d&d purpose)
    RegisteredTabs _registeredTabs;

    ///The user preferences window
    PreferencesPanel* _settingsGui;

    ///The project Gui (stored in the properties pane)
    ProjectGui* _projectGui;

    ///ptr to the currently dragged tab for d&d purpose.
    QWidget* _currentlyDraggedPanel;

    ///The "About" window.
    AboutWindow* _aboutWindow;
    std::map<KnobHolder*, QProgressDialog*> _progressBars;

    ///list of the currently opened property panels
    std::list<DockablePanel*> openedPanels;
    QString _openGLVersion;
    QString _glewVersion;
    QToolButton* _toolButtonMenuOpened;
    QMutex aboutToCloseMutex;
    bool _aboutToClose;
    ShortCutEditor* shortcutEditor;
    bool leftToolBarDisplayedOnHoverOnly;
    ScriptEditor* _scriptEditor;
    TabWidget* _lastEnteredTabWidget;

    ///Menu entries added by the user
    std::map<ActionWithShortcut*, std::string> pythonCommands;
    
    RenderStatsDialog* statsDialog;
    
    GuiPrivate(GuiAppInstance* app,
               Gui* gui);

    void restoreGuiGeometry();

    void saveGuiGeometry();

    void setUndoRedoActions(QAction* undoAction, QAction* redoAction);

    void addToolButton(ToolButton* tool);

    ///Creates the properties bin and appends it as a tab to the propertiesPane TabWidget
    void createPropertiesBinGui();

    void notifyGuiClosing();

    ///Must be called absolutely before createPropertiesBinGui
    void createNodeGraphGui();

    void createCurveEditorGui();

    void createDopeSheetGui();

    void createScriptEditorGui();

    ///If there's only 1 non-floating pane in the main window, return it, otherwise returns NULL
    TabWidget* getOnly1NonFloatingPane(int & count) const;

    void refreshLeftToolBarVisibility(const QPoint & p);

    QAction* findActionRecursive(int i, QWidget* widget, const QStringList & grouping);
    
    ///True= yes overwrite
    bool checkProjectLockAndWarn(const QString& projectPath,const QString& projectName);
};

#endif // _Gui_GuiPrivate_h_
