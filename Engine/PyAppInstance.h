/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#ifndef Engine_AppInstanceWrapper_h
#define Engine_AppInstanceWrapper_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)


#include "Engine/PyNode.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

/**
 * @brief Simple wrap for the AppInstance class that is the API we want to expose to the Python
 * Engine module.
 **/

// Used as a temp variable to retrieve existing objects without creating new object.
#define kPythonTmpCheckerVariable "_tmp_checker_"


class AppSettings
{
public:

    AppSettings(const SettingsPtr& settings);

    Param* getParam(const QString& scriptName) const;
    std::list<Param*> getParams() const;

    void saveSettings();

    void restoreDefaultSettings();

private:

    SettingsPtr _settings;
};


class NodeCreationProperty
{


public:

    NodeCreationProperty()
    {

    }

    virtual ~NodeCreationProperty()
    {

    }

};

typedef std::map<QString, NodeCreationProperty*> NodeCreationPropertyMap;

class IntNodeCreationProperty : public NodeCreationProperty
{
    std::vector<int> values;

public:

    IntNodeCreationProperty(int value)
    : NodeCreationProperty()
    , values()
    {
        values.push_back(value);
    }

    IntNodeCreationProperty(const std::vector<int>& values = std::vector<int>())
    : NodeCreationProperty()
    , values(values)
    {

    }

    void setValue(int value, int index = 0)
    {
        if (index < 0) {
            return;
        }
        if (index >= (int)values.size()) {
            values.resize(index + 1);
        }
        values[index] = value;
    }

    const std::vector<int>& getValues() const
    {
        return values;
    }

    virtual ~IntNodeCreationProperty()
    {
    }
};

class BoolNodeCreationProperty : public NodeCreationProperty
{
    std::vector<bool> values;

public:

    BoolNodeCreationProperty(bool value)
    : NodeCreationProperty()
    , values()
    {
        values.push_back(value);
    }

    BoolNodeCreationProperty(const std::vector<bool>& values = std::vector<bool>())
    : NodeCreationProperty()
    , values(values)
    {

    }

    void setValue(bool value, int index = 0)
    {
        if (index < 0) {
            return;
        }
        if (index >= (int)values.size()) {
            values.resize(index + 1);
        }
        values[index] = value;
    }

    const std::vector<bool>& getValues() const
    {
        return values;
    }


    virtual ~BoolNodeCreationProperty()
    {
    }
};

class FloatNodeCreationProperty : public NodeCreationProperty
{
    std::vector<double> values;

public:

    FloatNodeCreationProperty(double value)
    : NodeCreationProperty()
    , values()
    {
        values.push_back(value);
    }

    FloatNodeCreationProperty(const std::vector<double>& values = std::vector<double>())
    : NodeCreationProperty()
    , values(values)
    {

    }

    void setValue(double value, int index = 0)
    {
        if (index < 0) {
            return;
        }
        if (index >= (int)values.size()) {
            values.resize(index + 1);
        }
        values[index] = value;
    }

    const std::vector<double>& getValues() const
    {
        return values;
    }


    virtual ~FloatNodeCreationProperty()
    {
    }
};

class StringNodeCreationProperty : public NodeCreationProperty
{
    std::vector<std::string> values;

public:

    StringNodeCreationProperty(const std::string& value)
    : NodeCreationProperty()
    , values()
    {
        values.push_back(value);
    }

    StringNodeCreationProperty(const std::vector<std::string>& values = std::vector<std::string>())
    : NodeCreationProperty()
    , values(values)
    {

    }

    void setValue(const std::string& value, int index = 0)
    {
        if (index < 0) {
            return;
        }
        if (index >= (int)values.size()) {
            values.resize(index + 1);
        }
        values[index] = value;
    }

    const std::vector<std::string>& getValues() const
    {
        return values;
    }


    virtual ~StringNodeCreationProperty()
    {
    }
};


class App
    : public Group
{
    Q_DECLARE_TR_FUNCTIONS(App)

private:
    AppInstanceWPtr _instance;

public:


    App(const AppInstancePtr& instance);

    virtual ~App() {}

    int getAppID() const;

    AppInstancePtr getInternalApp() const;

    /**
     * @brief Creates a new instance of the plugin identified by the given pluginID.
     * @param majorVersion If different than -1, it will try to load a specific major version
     * of the plug-in, otherwise it will default to the greatest major version found for that plug-in.
     * @param group If not NULL, this should be a pointer to a group node where-in to insert the newly created effect.
     * If NULL, the newly created node will be inserted in the project's root.
     **/
    Effect* createNode(const QString& pluginID,
                       int majorVersion = -1,
                       Group* group = 0,
                       const NodeCreationPropertyMap& props = NodeCreationPropertyMap()) const;
    Effect* createReader(const QString& filename,
                         Group* group = 0,
                         const NodeCreationPropertyMap& props = NodeCreationPropertyMap()) const;
    Effect* createWriter(const QString& filename,
                         Group* group = 0,
                         const NodeCreationPropertyMap& props = NodeCreationPropertyMap()) const;

    int timelineGetTime() const;

    int timelineGetLeftBound() const;

    int timelineGetRightBound() const;

    void timelineGoTo(int frame);
    
    void addFormat(const QString& formatSpec);

    void render(Effect* writeNode, int firstFrame, int lastFrame, int frameStep = 1);

    void render(const std::list<Effect*>& effects, const std::list<int>& firstFrames, const std::list<int>& lastFrames, const std::list<int>& frameSteps);

    void redrawViewer(Effect* viewerNode);

    void refreshViewer(Effect* viewerNode,bool useCache = true);

    Param* getProjectParam(const QString& name) const;

    void writeToScriptEditor(const QString& message);

    bool saveProject(const QString& filename);
    bool saveProjectAs(const QString& filename);
    bool saveTempProject(const QString& filename);
    App* loadProject(const QString& filename);

    ///Close the current project but keep the window
    bool resetProject();

    ///Reset + close window, quit if last window
    bool closeProject();

    ///Opens a new window
    App* newProject();
    std::list<QString> getViewNames() const;

    int getViewIndex(const QString& viewName) const;

    QString getViewName(int viewIndex) const;

    void addProjectLayer(const ImageLayer& layer);

    static Effect* createEffectFromNodeWrapper(const NodePtr& node);

    static App* createAppFromAppInstance(const AppInstancePtr& app);

protected:

    void renderInternal(bool forceBlocking, Effect* writeNode, int firstFrame, int lastFrame, int frameStep);
    void renderInternal(bool forceBlocking, const std::list<Effect*>& effects, const std::list<int>& firstFrames, const std::list<int>& lastFrames,
                        const std::list<int>& frameSteps);

    NodeCollectionPtr getCollectionFromGroup(Group* group) const;
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // Engine_AppInstanceWrapper_h
