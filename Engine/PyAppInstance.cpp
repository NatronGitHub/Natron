/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PyAppInstance.h"

#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QDebug>

#include "Engine/AppInstance.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/EffectInstance.h"
#include "Engine/Settings.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

App::App(const AppInstancePtr& instance)
    : Group()
    , _instance(instance)
{
    if ( instance->getProject() ) {
        init( instance->getProject() );
    }
}

int
App::getAppID() const
{
    return getInternalApp()->getAppID();
}

AppInstancePtr
App::getInternalApp() const
{
    return _instance.lock();
}

NodeCollectionPtr
App::getCollectionFromGroup(Group* group) const
{
    NodeCollectionPtr collection;

    if (group) {
        App* isApp = dynamic_cast<App*>(group);
        Effect* isEffect = dynamic_cast<Effect*>(group);
        if (isApp) {
            collection = std::dynamic_pointer_cast<NodeCollection>( isApp->getInternalApp()->getProject() );
        } else if (isEffect) {
            NodePtr node = isEffect->getInternalNode();
            assert(node);
            NodeGroupPtr isGrp = std::dynamic_pointer_cast<NodeGroup>( node->getEffectInstance()->shared_from_this() );
            if (!isGrp) {
                qDebug() << "The group passed to createNode() is not a group, defaulting to the project root.";
            } else {
                collection = std::dynamic_pointer_cast<NodeCollection>(isGrp);
                assert(collection);
            }
        }
    }

    if (!collection) {
        collection = std::dynamic_pointer_cast<NodeCollection>( getInternalApp()->getProject() );
    }

    return collection;
}

static void makeCreateNodeArgs(const AppInstancePtr& app,
                               const QString& pluginID,
                               int majorVersion,
                               const NodeCollectionPtr& collection,
                               const NodeCreationPropertyMap& props,
                               CreateNodeArgs* args)
{

    args->setProperty<std::string>(kCreateNodeArgsPropPluginID, pluginID.toStdString());
    args->setProperty<NodeCollectionPtr>(kCreateNodeArgsPropGroupContainer, collection);
    args->setProperty<int>(kCreateNodeArgsPropPluginVersion, majorVersion, 0);

    args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
    args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args->setProperty<bool>(kCreateNodeArgsPropSilent, true);
    args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true); // also load deprecated plugins

    bool skipNoNodeGuiProp = false;
    for (NodeCreationPropertyMap::const_iterator it = props.begin(); it!=props.end(); ++it) {
        IntNodeCreationProperty* isInt = dynamic_cast<IntNodeCreationProperty*>(it->second);
        BoolNodeCreationProperty* isBool = dynamic_cast<BoolNodeCreationProperty*>(it->second);
        FloatNodeCreationProperty* isDouble = dynamic_cast<FloatNodeCreationProperty*>(it->second);
        StringNodeCreationProperty* isString = dynamic_cast<StringNodeCreationProperty*>(it->second);

        bool fail = true;
        if (it->first.startsWith(QString::fromUtf8(kCreateNodeArgsPropParamValue))) {
            fail = false;
        }
        std::string prop = it->first.toStdString();
        try {
            if (isInt) {
                args->setPropertyN<int>(prop, isInt->getValues(), fail);
            } else if (isBool) {
                if (prop == kCreateNodeArgsPropOutOfProject) {
                    args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
                    skipNoNodeGuiProp = true;
                } else if (skipNoNodeGuiProp && prop == kCreateNodeArgsPropNoNodeGUI) {
                    continue;
                }
                args->setPropertyN<bool>(prop, isBool->getValues(), fail);
            } else if (isDouble) {
                args->setPropertyN<double>(prop, isDouble->getValues(), fail);
            } else if (isString) {
                args->setPropertyN<std::string>(prop, isString->getValues(), fail);
            }
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << ("Error while setting property ");
            ss << it->first.toStdString();
            ss << ": " << e.what();
            app->appendToScriptEditor(ss.str());
        }
    }


}


Effect*
App::createNode(const QString& pluginID,
                int majorVersion,
                Group* group,
                const NodeCreationPropertyMap& props) const
{
    NodeCollectionPtr collection = getCollectionFromGroup(group);

    assert(collection);
    CreateNodeArgs args;
    makeCreateNodeArgs(getInternalApp(), pluginID, majorVersion, collection, props, &args);

    NodePtr node = getInternalApp()->createNode(args);
    if (node) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

Effect*
App::createReader(const QString& filename,
                  Group* group,
                  const NodeCreationPropertyMap& props) const
{
    NodeCollectionPtr collection = getCollectionFromGroup(group);

    assert(collection);

    CreateNodeArgs args;
    makeCreateNodeArgs(getInternalApp(),  QString::fromUtf8(PLUGINID_NATRON_READ), -1, collection, props, &args);

    NodePtr node = getInternalApp()->createReader(filename.toStdString(), args);
    if (node) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

Effect*
App::createWriter(const QString& filename,
                  Group* group,
                  const NodeCreationPropertyMap& props) const
{
    NodeCollectionPtr collection = getCollectionFromGroup(group);

    assert(collection);
    CreateNodeArgs args;
    makeCreateNodeArgs(getInternalApp(), QString::fromUtf8(PLUGINID_NATRON_WRITE), -1, collection, props, &args);
    NodePtr node = getInternalApp()->createWriter(filename.toStdString(), args);
    if (node) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

int
App::timelineGetTime() const
{
    return getInternalApp()->getTimeLine()->currentFrame();
}

int
App::timelineGetLeftBound() const
{
    double left, right;

    getInternalApp()->getFrameRange(&left, &right);

    return left;
}

int
App::timelineGetRightBound() const
{
    double left, right;

    getInternalApp()->getFrameRange(&left, &right);

    return right;
}

AppSettings::AppSettings(const SettingsPtr& settings)
    : _settings(settings)
{
}

Param*
AppSettings::getParam(const QString& scriptName) const
{
    KnobIPtr knob = _settings->getKnobByName( scriptName.toStdString() );

    if (!knob) {
        return 0;
    }

    return Effect::createParamWrapperForKnob(knob);
}

std::list<Param*>
AppSettings::getParams() const
{
    std::list<Param*> ret;
    const KnobsVec& knobs = _settings->getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        Param* p = Effect::createParamWrapperForKnob(*it);
        if (p) {
            ret.push_back(p);
        }
    }

    return ret;
}

void
AppSettings::saveSettings()
{
    _settings->saveAllSettings();
}

void
AppSettings::restoreDefaultSettings()
{
    _settings->restoreDefault();
}

void
App::render(Effect* writeNode,
            int firstFrame,
            int lastFrame,
            int frameStep)
{
    renderInternal(false, writeNode, firstFrame, lastFrame, frameStep);
}

void
App::render(const std::list<Effect*>& effects,
            const std::list<int>& firstFrames,
            const std::list<int>& lastFrames,
            const std::list<int>& frameSteps)
{
    renderInternal(false, effects, firstFrames, lastFrames, frameSteps);
}

void
App::renderInternal(bool forceBlocking,
                    Effect* writeNode,
                    int firstFrame,
                    int lastFrame,
                    int frameStep)
{
    if (!writeNode) {
        std::cerr << tr("Invalid write node").toStdString() << std::endl;

        return;
    }
    AppInstance::RenderWork w;
    NodePtr node =  writeNode->getInternalNode();
    if (!node) {
        std::cerr << tr("Invalid write node").toStdString() << std::endl;

        return;
    }
    w.writer = dynamic_cast<OutputEffectInstance*>( node->getEffectInstance().get() );
    if (!w.writer) {
        std::cerr << tr("Invalid write node").toStdString() << std::endl;

        return;
    }

    w.firstFrame = firstFrame;
    w.lastFrame = lastFrame;
    w.frameStep = frameStep;
    w.useRenderStats = false;

    std::list<AppInstance::RenderWork> l;
    l.push_back(w);

    PythonGILUnlocker pgl;

    getInternalApp()->startWritersRendering(forceBlocking, l);
}

void
App::renderInternal(bool forceBlocking,
                    const std::list<Effect*>& effects,
                    const std::list<int>& firstFrames,
                    const std::list<int>& lastFrames,
                    const std::list<int>& frameSteps)
{
    std::list<AppInstance::RenderWork> l;

    assert( effects.size() == firstFrames.size() && effects.size() == lastFrames.size() && frameSteps.size() == effects.size() );
    std::list<Effect*>::const_iterator itE = effects.begin();
    std::list<int>::const_iterator itF = firstFrames.begin();
    std::list<int>::const_iterator itL = lastFrames.begin();
    std::list<int>::const_iterator itS = frameSteps.begin();
    for (; itE != effects.end(); ++itE, ++itF, ++itL, ++itS) {
        if (!*itE) {
            std::cerr << tr("Invalid write node").toStdString() << std::endl;

            return;
        }
        AppInstance::RenderWork w;
        NodePtr node =  (*itE)->getInternalNode();
        if (!node) {
            std::cerr << tr("Invalid write node").toStdString() << std::endl;

            return;
        }
        w.writer = dynamic_cast<OutputEffectInstance*>( node->getEffectInstance().get() );
        if ( !w.writer || !w.writer->isOutput() ) {
            std::cerr << tr("Invalid write node").toStdString() << std::endl;

            return;
        }

        w.firstFrame = (*itF);
        w.lastFrame = (*itL);
        w.frameStep = (*itS);
        w.useRenderStats = false;

        l.push_back(w);
    }

    PythonGILUnlocker pgl;

    getInternalApp()->startWritersRendering(forceBlocking, l);
}

Param*
App::getProjectParam(const QString& name) const
{
    KnobIPtr knob =  getInternalApp()->getProject()->getKnobByName( name.toStdString() );

    if (!knob) {
        return 0;
    }

    return Effect::createParamWrapperForKnob(knob);
}

void
App::writeToScriptEditor(const QString& message)
{
    getInternalApp()->appendToScriptEditor( message.toStdString() );
}

void
App::addFormat(const QString& formatSpec)
{
    if ( !getInternalApp()->getProject()->addFormat( formatSpec.toStdString() ) ) {
        getInternalApp()->appendToScriptEditor( formatSpec.toStdString() );
    }
}

bool
App::saveTempProject(const QString& filename)
{
    return getInternalApp()->saveTemp( filename.toStdString() );
}

bool
App::saveProject(const QString& filename)
{
    std::string outFile = filename.toStdString();
    std::map<std::string, std::string> env;
    getInternalApp()->getProject()->getEnvironmentVariables(env);
    Project::expandVariable(env, outFile);

    return getInternalApp()->save(outFile);
}

bool
App::saveProjectAs(const QString& filename)
{
    std::string outFile = filename.toStdString();
    std::map<std::string, std::string> env;
    getInternalApp()->getProject()->getEnvironmentVariables(env);
    Project::expandVariable(env, outFile);

    return getInternalApp()->saveAs(outFile);
}

App*
App::loadProject(const QString& filename)
{
    AppInstancePtr app  = getInternalApp()->loadProject( filename.toStdString() );

    if (!app) {
        return 0;
    }

    return new App(app);
}

///Close the current project but keep the window
bool
App::resetProject()
{
    return getInternalApp()->resetProject();
}

///Reset + close window, quit if last window
bool
App::closeProject()
{
    return getInternalApp()->closeProject();
}

///Opens a new window
App*
App::newProject()
{
    AppInstancePtr app  = getInternalApp()->newProject();

    if (!app) {
        return 0;
    }

    return new App(app);
}

std::list<QString>
App::getViewNames() const
{
    std::list<QString> ret;
    const std::vector<std::string>& v = getInternalApp()->getProject()->getProjectViewNames();

    for (std::size_t i = 0; i < v.size(); ++i) {
        ret.push_back( QString::fromUtf8( v[i].c_str() ) );
    }

    return ret;
}

void
App::addProjectLayer(const ImageLayer& layer)
{
    getInternalApp()->getProject()->addProjectDefaultLayer( layer.getInternalComps() );
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT
