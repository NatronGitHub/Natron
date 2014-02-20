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

#if defined(appPTR)
#undef appPTR
#endif
#define appPTR (static_cast<GuiApplicationManager*>(AppManager::instance()))


class QPixmap;
class QCursor;

class PluginGroupNode;
class DockablePanel;
class Knob;
class KnobGui;
class KnobSerialization;

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
    
    void setKnobClipBoard(const KnobSerialization& s,bool copyAnimation);
    
    bool isClipBoardEmpty() const;
    
    void getKnobClipBoard(KnobSerialization* k,bool* copyAnimation) const;
    
    void updateAllRecentFileMenus();

    virtual void hideSplashScreen() OVERRIDE FINAL;
    
    const QCursor& getColorPickerCursor() const;
    
    virtual void setLoadingStatus(const QString& str) OVERRIDE FINAL;
    
    KnobGui* createGuiForKnob(boost::shared_ptr<Knob> knob, DockablePanel *container) const;
    
public slots:
    
    virtual void addPluginToolButtons(const QStringList& groups,
                                      const QString& pluginID,
                                      const QString& pluginLabel,
                                      const QString& pluginIconPath,
                                      const QString& groupIconPath) OVERRIDE FINAL;
    
private:
    
    virtual void loadBuiltinNodePlugins(std::vector<Natron::Plugin*>* plugins,
                                        std::map<std::string,std::vector<std::string> >* readersMap,
                                        std::map<std::string,std::vector<std::string> >* writersMap);
    
    virtual void initGui() OVERRIDE FINAL;
    
    virtual AppInstance* makeNewInstance(int appID) const OVERRIDE FINAL;
    
    virtual void registerGuiMetaTypes() const OVERRIDE FINAL;

    virtual void initializeQApp(int argc,char* argv[]) const OVERRIDE FINAL;

    boost::scoped_ptr<GuiApplicationManagerPrivate> _imp;

};

#endif // GUIAPPLICATIONMANAGER_H
