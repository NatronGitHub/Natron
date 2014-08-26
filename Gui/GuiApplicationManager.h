//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/


#ifndef GUIAPPLICATIONMANAGER_H
#define GUIAPPLICATIONMANAGER_H

#include "Engine/AppManager.h"
#include "Engine/Variant.h"

#if defined(appPTR)
#undef appPTR
#endif
#define appPTR (static_cast<GuiApplicationManager*>(AppManager::instance()))

/**
 * @brief Returns true if the given modifiers and symbol should trigger the given action of the given group.
 **/
#define isKeybind(group,action,modifiers,symbol) (appPTR->matchesKeybind(group,action, modifiers, symbol))

/**
 * @brief Returns true if the given modifiers and button should trigger the given action of the given group.
 **/
#define isMouseShortcut(group,action,modifiers,button) (appPTR->matchesMouseShortcut(group,action, modifiers, button))

/**
 * @brief Returns the QKeySequence object for the given action of the given group.
 **/
#define getKeybind(group,action) (appPTR->getKeySequenceForAction(group,action))

class QPixmap;
class QCursor;

class PluginGroupNode;
class DockablePanel;
class KnobI;
class KnobGui;
class KnobSerialization;
class Curve;

struct GuiApplicationManagerPrivate;
class GuiApplicationManager : public AppManager
{
    
public:
    
    GuiApplicationManager();
    
    virtual ~GuiApplicationManager();
    
    const std::vector<PluginGroupNode*>& getPluginsToolButtons() const;
    
    PluginGroupNode* findPluginToolButtonOrCreate(const QString& pluginID,const QString& name,const QString& iconPath);
    
    virtual bool isBackground() const OVERRIDE FINAL { return false; }
    
    void getIcon(Natron::PixmapEnum e,QPixmap* pix) const;
    
    void setKnobClipBoard(bool copyAnimation,int dimension,
                          const std::list<Variant>& values,
                          const std::list<boost::shared_ptr<Curve> >& animation,
                          const std::map<int,std::string>& stringAnimation,
                          const std::list<boost::shared_ptr<Curve> >& parametricCurves);
    
    
    void getKnobClipBoard(bool* copyAnimation,int* dimension,
                          std::list<Variant>* values,
                          std::list<boost::shared_ptr<Curve> >* animation,
                          std::map<int,std::string>* stringAnimation,
                          std::list<boost::shared_ptr<Curve> >* parametricCurves) const;
    
    bool isClipBoardEmpty() const;

    
    void updateAllRecentFileMenus();

    virtual void hideSplashScreen() OVERRIDE FINAL;
    
    const QCursor& getColorPickerCursor() const;
    
    virtual void setLoadingStatus(const QString& str) OVERRIDE FINAL;
    
    KnobGui* createGuiForKnob(boost::shared_ptr<KnobI> knob, DockablePanel *container) const;
    
    virtual void setUndoRedoStackLimit(int limit) OVERRIDE FINAL;

    virtual void debugImage(const Natron::Image* image,const QString& filename = QString()) const OVERRIDE FINAL;
        
    void setFileToOpen(const QString& str);
    
    /**
     * @brief Returns true if the given keyboard symbol and modifiers match the given action.
     * The symbol parameter is to be casted to the Qt::Key enum
     **/
    bool matchesKeybind(const QString& group,const QString& actionID,const Qt::KeyboardModifiers& modifiers,int symbol) const;
    
    /**
     * @brief Returns true if the given keyboard modifiers and the given mouse button match the given action.
     * The button parameter is to be casted to the Qt::MouseButton enum
     **/
    bool matchesMouseShortcut(const QString& group,const QString& actionID,const Qt::KeyboardModifiers& modifiers,int button) const;
    
    QKeySequence getKeySequenceForAction(const QString& group,const QString& actionID) const;
    bool getModifiersAndKeyForAction(const QString& group,const QString& actionID,Qt::KeyboardModifiers& modifiers,int& symbol) const;
    
    /**
     * @brief Save shortcuts to QSettings
     **/
    void saveShortcuts() const;
    
    void restoreDefaultShortcuts();
public slots:
    
    
    ///Closes the application, asking the user to save each opened project that has unsaved changes
    virtual void exitApp() OVERRIDE FINAL;
    
    
private:
    
    virtual void onPluginLoaded(const QStringList& groups,
                                const QString& pluginID,
                                const QString& pluginLabel,
                                const QString& pluginIconPath,
                                const QString& groupIconPath) OVERRIDE FINAL;

    
    
    virtual void loadBuiltinNodePlugins(std::vector<Natron::Plugin*>* plugins,
                                        std::map<std::string,std::vector<std::string> >* readersMap,
                                        std::map<std::string,std::vector<std::string> >* writersMap);
    
    virtual void initGui() OVERRIDE FINAL;
    
    virtual AppInstance* makeNewInstance(int appID) const OVERRIDE FINAL;
    
    virtual void registerGuiMetaTypes() const OVERRIDE FINAL;

    virtual void initializeQApp(int &argc, char **argv)  OVERRIDE FINAL;
    
    void handleOpenFileRequest();
    
    virtual void onLoadCompleted() OVERRIDE FINAL;
    
    /**
     * @brief Load shortcuts from QSettings
     **/
    void loadShortcuts();
    
    void populateShortcuts();

    boost::scoped_ptr<GuiApplicationManagerPrivate> _imp;

};

#endif // GUIAPPLICATIONMANAGER_H
