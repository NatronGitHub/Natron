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


#ifndef _Gui_GuiApplicationManager_h_
#define _Gui_GuiApplicationManager_h_


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>

#include "Engine/AppManager.h"
#include "Engine/Variant.h"

#if defined(appPTR)
#undef appPTR
#endif
#define appPTR ( static_cast<GuiApplicationManager*>( AppManager::instance() ) )

#define appFont ( appPTR->getAppFont() )
#define appFontSize ( appPTR->getAppFontSize() )

/**
 * @brief Returns true if the given modifiers and symbol should trigger the given action of the given group.
 **/
#define isKeybind(group,action, modifiers,symbol) ( appPTR->matchesKeybind(group,action, modifiers, symbol) )

/**
 * @brief Returns true if the given modifiers and button should trigger the given action of the given group.
 **/
#define isMouseShortcut(group,action, modifiers,button) ( appPTR->matchesMouseShortcut(group,action, modifiers, button) )

/**
 * @brief Returns the QKeySequence object for the given action of the given group.
 **/
#define getKeybind(group,action) ( appPTR->getKeySequenceForAction(group,action) )

class QPixmap;
class QCursor;

class ActionWithShortcut;
class PluginGroupNode;
class DockablePanel;
class KnobI;
class KnobGui;
class KnobSerialization;
class Curve;
class BoundAction;
class KeyBoundAction;
class QAction;
class NodeSerialization;
class NodeGuiSerialization;
struct NodeClipBoard;

struct PythonUserCommand {
    QString grouping;
    Qt::Key key;
    Qt::KeyboardModifiers modifiers;
    std::string pythonFunction;
};

struct GuiApplicationManagerPrivate;
class GuiApplicationManager
    : public AppManager
{
public:

    GuiApplicationManager();

    virtual ~GuiApplicationManager();

    const std::list<boost::shared_ptr<PluginGroupNode> > & getTopLevelPluginsToolButtons() const;
    boost::shared_ptr<PluginGroupNode>  findPluginToolButtonOrCreate(const QStringList & grouping,
                                                                     const QString & name,
                                                                     const QString& groupIconPath,
                                                                     const QString & iconPath,
                                                                     int major,
                                                                     int minor,
                                                                     bool isUserCreatable);
    virtual bool isBackground() const OVERRIDE FINAL
    {
        return false;
    }

    void getIcon(Natron::PixmapEnum e, QPixmap* pix) const;
    void getIcon(Natron::PixmapEnum e, int size, QPixmap* pix) const;

    void setKnobClipBoard(bool copyAnimation,
                          const std::list<Variant> & values,
                          const std::list<boost::shared_ptr<Curve> > & animation,
                          const std::map<int,std::string> & stringAnimation,
                          const std::list<boost::shared_ptr<Curve> > & parametricCurves,
                          const std::string& appID,
                          const std::string& nodeFullyQualifiedName,
                          const std::string& paramName);


    void getKnobClipBoard(bool* copyAnimation,
                          std::list<Variant>* values,
                          std::list<boost::shared_ptr<Curve> >* animation,
                          std::map<int,std::string>* stringAnimation,
                          std::list<boost::shared_ptr<Curve> >* parametricCurves,
                          std::string* appID,
                          std::string* nodeFullyQualifiedName,
                          std::string* paramName) const;

    bool isClipBoardEmpty() const;


    void updateAllRecentFileMenus();

    bool isSplashcreenVisible() const;
    
    virtual void hideSplashScreen() OVERRIDE FINAL;
    const QCursor & getColorPickerCursor() const;
    virtual void setLoadingStatus(const QString & str) OVERRIDE FINAL;
    KnobGui* createGuiForKnob(boost::shared_ptr<KnobI> knob, DockablePanel *container) const;
    virtual void setUndoRedoStackLimit(int limit) OVERRIDE FINAL;
    virtual void debugImage( const Natron::Image* image, const RectI& roi, const QString & filename = QString() ) const OVERRIDE FINAL;

    void setFileToOpen(const QString & str);

    /**
     * @brief Returns true if the given keyboard symbol and modifiers match the given action.
     * The symbol parameter is to be casted to the Qt::Key enum
     **/
    bool matchesKeybind(const QString & group,const QString & actionID,const Qt::KeyboardModifiers & modifiers,int symbol) const;

    /**
     * @brief Returns true if the given keyboard modifiers and the given mouse button match the given action.
     * The button parameter is to be casted to the Qt::MouseButton enum
     **/
    bool matchesMouseShortcut(const QString & group,const QString & actionID,const Qt::KeyboardModifiers & modifiers,int button) const;

    QKeySequence getKeySequenceForAction(const QString & group,const QString & actionID) const;
    bool getModifiersAndKeyForAction(const QString & group,const QString & actionID,Qt::KeyboardModifiers & modifiers,int & symbol) const;

    /**
     * @brief Save shortcuts to QSettings
     **/
    void saveShortcuts() const;

    void restoreDefaultShortcuts();

    const std::map<QString,std::map<QString,BoundAction*> > & getAllShortcuts() const;

    /**
     * @brief Register an action to the shortcut manager indicating it is using a shortcut.
     * This is used to update the action's shortcut when it gets modified by the user.
     **/
    void addShortcutAction(const QString & group,const QString & actionID,ActionWithShortcut* action);
    void removeShortcutAction(const QString & group,const QString & actionID,QAction* action);

    void notifyShortcutChanged(KeyBoundAction* action);
    
    bool isShorcutVersionUpToDate() const;
    
    virtual void showOfxLog() OVERRIDE FINAL;
    
    virtual QString getAppFont() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual int getAppFontSize() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    bool isNodeClipBoardEmpty() const;
    
    NodeClipBoard& getNodeClipBoard();
    
    virtual void reloadStylesheets() OVERRIDE FINAL;
    
    void clearNodeClipBoard();
    
    virtual void addCommand(const QString& grouping,const std::string& pythonFunction, Qt::Key key,const Qt::KeyboardModifiers& modifiers) OVERRIDE;
    
    const std::list<PythonUserCommand>& getUserPythonCommands() const;
    
public Q_SLOTS:


    ///Closes the application, asking the user to save each opened project that has unsaved changes
    virtual void exitApp() OVERRIDE FINAL;

private:

    virtual void initBuiltinPythonModules() OVERRIDE FINAL;

    void onPluginLoaded(Natron::Plugin* plugin) OVERRIDE;
    virtual void ignorePlugin(Natron::Plugin* plugin) OVERRIDE FINAL;
    virtual void onAllPluginsLoaded() OVERRIDE FINAL;
    virtual void loadBuiltinNodePlugins(std::map<std::string,std::vector< std::pair<std::string,double> > >* readersMap,
                                        std::map<std::string,std::vector< std::pair<std::string,double> > >* writersMap) OVERRIDE;
    virtual void initGui() OVERRIDE FINAL;
    virtual AppInstance* makeNewInstance(int appID) const OVERRIDE FINAL;
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

#endif // _Gui_GuiApplicationManager_h_
