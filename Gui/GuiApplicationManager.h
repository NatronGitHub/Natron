/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Gui_GuiApplicationManager_h
#define Gui_GuiApplicationManager_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
#include "Global/Enums.h"

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <QtCore/QtGlobal> // for Q_OS_*

#include "Engine/AppManager.h"
#include "Engine/Variant.h"
#include "Engine/EngineFwd.h"

#include "Gui/GuiFwd.h"

#if defined(appPTR)
#undef appPTR
#endif
#define appPTR ( static_cast<GuiApplicationManager*>( AppManager::instance() ) )

#define appFont ( appPTR->getAppFont() )
#define appFontSize ( appPTR->getAppFontSize() )

/**
 * @brief Returns true if the given modifiers and symbol should trigger the given action of the given group.
 **/
#define isKeybind(group, action, modifiers, symbol) ( appPTR->matchesKeybind(group, action, modifiers, symbol) )

/**
 * @brief Returns true if the given modifiers and button should trigger the given action of the given group.
 **/
#define isMouseShortcut(group, action, modifiers, button) ( appPTR->matchesMouseShortcut(group, action, modifiers, button) )

/**
 * @brief Returns the QKeySequence object for the given action of the given group.
 **/
#define getKeybind(group, action) ( appPTR->getKeySequenceForAction(group, action) )


NATRON_NAMESPACE_ENTER

struct PythonUserCommand
{
    QString grouping;
    Qt::Key key;
    Qt::KeyboardModifiers modifiers;
    std::string pythonFunction;
};

struct GuiApplicationManagerPrivate;
class GuiApplicationManager
    : public AppManager
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    GuiApplicationManager();

    virtual ~GuiApplicationManager();

    const std::list<PluginGroupNodePtr> & getTopLevelPluginsToolButtons() const;
    PluginGroupNodePtr  findPluginToolButtonOrCreate(const QStringList & grouping,
                                                                     const QString & name,
                                                                     const QStringList& groupIconPath,
                                                                     const QString & iconPath,
                                                                     int major,
                                                                     int minor,
                                                                     bool isUserCreatable);
    virtual bool isBackground() const OVERRIDE FINAL
    {
        return false;
    }

    void getIcon(NATRON_ENUM::PixmapEnum e, QPixmap* pix) const;
    void getIcon(NATRON_ENUM::PixmapEnum e, int size, QPixmap* pix) const;

    void setKnobClipBoard(NATRON_ENUM::KnobClipBoardType type,
                          const KnobIPtr& serialization,
                          int dimension);


    void getKnobClipBoard(NATRON_ENUM::KnobClipBoardType *type,
                          KnobIPtr* serialization,
                          int *dimension) const;


    void updateAllRecentFileMenus();

    bool isSplashcreenVisible() const;

    virtual void hideSplashScreen() OVERRIDE FINAL;
    const QCursor & getColorPickerCursor() const;
    const QCursor & getLinkToCursor() const;
    const QCursor & getLinkToMultCursor() const;
    virtual void setLoadingStatus(const QString & str) OVERRIDE FINAL;
    KnobGui* createGuiForKnob(KnobIPtr knob, KnobGuiContainerI *container) const;
    virtual void setUndoRedoStackLimit(int limit) OVERRIDE FINAL;
    virtual void debugImage( const Image* image, const RectI& roi, const QString & filename = QString() ) const OVERRIDE FINAL;

    void setFileToOpen(const QString & str);

    /**
     * @brief Returns true if the given keyboard symbol and modifiers match the given action.
     * The symbol parameter is to be casted to the Qt::Key enum
     **/
    bool matchesKeybind(const std::string & group,
                        const std::string & actionID,
                        const Qt::KeyboardModifiers & modifiers,
                        int symbol) const;

    /**
     * @brief Returns true if the given keyboard modifiers and the given mouse button match the given action.
     * The button parameter is to be casted to the Qt::MouseButton enum
     **/
    bool matchesMouseShortcut(const std::string & group, const std::string & actionID, const Qt::KeyboardModifiers & modifiers, int button) const;

    std::list<QKeySequence> getKeySequenceForAction(const QString & group, const QString & actionID) const;

    /**
     * @brief Save shortcuts to QSettings
     **/
    void saveShortcuts() const;

    void restoreDefaultShortcuts();

    const std::map<QString, std::map<QString, BoundAction*> > & getAllShortcuts() const;

    /**
     * @brief Register an action to the shortcut manager indicating it is using a shortcut.
     * This is used to update the action's shortcut when it gets modified by the user.
     **/
    void addShortcutAction(const QString & group, const QString & actionID, ActionWithShortcut* action);
    void removeShortcutAction(const QString & group, const QString & actionID, QAction* action);

    void notifyShortcutChanged(KeyBoundAction* action);

    bool isShorcutVersionUpToDate() const;

    virtual void showErrorLog() OVERRIDE FINAL;
    virtual QString getAppFont() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getAppFontSize() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    ///Closes the application, asking the user to save each opened project that has unsaved changes
    virtual void exitApp(bool warnUserForSave) OVERRIDE FINAL;

    bool isNodeClipBoardEmpty() const;

    NodeClipBoard& getNodeClipBoard();
    virtual void reloadStylesheets() OVERRIDE FINAL;
    virtual void reloadScriptEditorFonts() OVERRIDE FINAL;

    void clearNodeClipBoard();

    virtual void addCommand(const QString& grouping, const std::string& pythonFunction, Qt::Key key, const Qt::KeyboardModifiers& modifiers) OVERRIDE;
    const std::list<PythonUserCommand>& getUserPythonCommands() const;

    bool handleImageFileOpenRequest(const std::string& imageFile);

    void appendTaskToPreviewThread(const NodeGuiPtr& node, double time);

    int getDocumentationServerPort();

#ifdef Q_OS_MAC
    void onClickOnDock();
#endif

    virtual void setCurrentLogicalDPI(double dpiX, double dpiY) OVERRIDE FINAL;
    virtual double getLogicalDPIXRATIO() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double getLogicalDPIYRATIO() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void updateAboutWindowLibrariesVersion() OVERRIDE FINAL;

public Q_SLOTS:

    void onFontconfigCacheUpdateFinished();
    void onFontconfigTimerTriggered();

#ifdef Q_OS_MAC
Q_SIGNALS:
    void dockClicked();
#endif
    
private:

    virtual void initBuiltinPythonModules() OVERRIDE FINAL;

    void onPluginLoaded(Plugin* plugin) OVERRIDE;
    virtual void ignorePlugin(Plugin* plugin) OVERRIDE FINAL;
    virtual void onAllPluginsLoaded() OVERRIDE FINAL;
    virtual void loadBuiltinNodePlugins(IOPluginsMap* readersMap,
                                        IOPluginsMap* writersMap) OVERRIDE;
    virtual bool initGui(const CLArgs& args) OVERRIDE FINAL;
    virtual AppInstancePtr makeNewInstance(int appID) const OVERRIDE FINAL;
    virtual void registerGuiMetaTypes() const OVERRIDE FINAL;
    virtual void initializeQApp(int &argc, char **argv)  OVERRIDE FINAL;

    void handleOpenFileRequest();

    virtual void onLoadCompleted() OVERRIDE FINAL;
    virtual void clearLastRenderedTextures() OVERRIDE FINAL;
    /**
     * @brief Load shortcuts from QSettings
     **/
    void loadShortcuts();

    void populateShortcuts();

    boost::scoped_ptr<GuiApplicationManagerPrivate> _imp;
};

NATRON_NAMESPACE_EXIT


#endif // Gui_GuiApplicationManager_h
