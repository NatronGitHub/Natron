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

/**
* @brief Simple wrap for the AppInstance class that is the API we want to expose to the Python
* Engine module.
**/

#ifndef APPINSTANCEWRAPPER_H
#define APPINSTANCEWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/NodeWrapper.h"

class Settings;
class AppInstance;


class AppSettings
{
    
public:
    
    AppSettings(const boost::shared_ptr<Settings>& settings);
    
    Param* getParam(const std::string& scriptName) const;
    
    std::list<Param*> getParams() const;
    
    void saveSettings();
    
    void restoreDefaultSettings();
    
private:
    
    boost::shared_ptr<Settings> _settings;
};


class App : public Group
{
    AppInstance* _instance;
    
public:
    
    
    App(AppInstance* instance);
    
    virtual ~App() {}
    
    int getAppID() const;
    
    AppInstance* getInternalApp() const;
    
    /**
     * @brief Creates a new instance of the plugin identified by the given pluginID.
     * @param majorVersion If different than -1, it will try to load a specific major version
     * of the plug-in, otherwise it will default to the greatest major version found for that plug-in.
     * @param group If not NULL, this should be a pointer to a group node where-in to insert the newly created effect.
     * If NULL, the newly created node will be inserted in the project's root.
     **/
    Effect* createNode(const std::string& pluginID,
                       int majorVersion = -1,
                       Group* group = 0) const;
    
    int timelineGetTime() const;
    
    int timelineGetLeftBound() const;
    
    int timelineGetRightBound() const;
    
    void addFormat(const std::string& formatSpec);
    
    void render(Effect* writeNode,int firstFrame, int lastFrame);
    void render(const std::list<Effect*>& effects,const std::list<int>& firstFrames,const std::list<int>& lastFrames);
    
    Param* getProjectParam(const std::string& name) const;
    
    void writeToScriptEditor(const std::string& message);
    
    bool saveProject(const std::string& filename);
    bool saveProjectAs(const std::string& filename);
    bool saveTempProject(const std::string& filename);
    App* loadProject(const std::string& filename);
    
    ///Close the current project but keep the window
    bool resetProject();
    
    ///Reset + close window, quit if last window
    bool closeProject();
    
    ///Opens a new window
    App* newProject();
};


#endif // APPINSTANCEWRAPPER_H
