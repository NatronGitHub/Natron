/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
#include "natronengine_python.h"
#include <shiboken.h> // produces many warnings
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
CLANG_DIAG_ON(mismatched-tags)
GCC_DIAG_ON(unused-parameter)


#include "Engine/AppInstance.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/EffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/RenderQueue.h"
#include "Engine/RenderEngine.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"
#include "Engine/OutputSchedulerThread.h"
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
            collection = boost::dynamic_pointer_cast<NodeCollection>( isApp->getInternalApp()->getProject() );
        } else if (isEffect) {
            NodePtr node = isEffect->getInternalNode();
            assert(node);
            NodeGroupPtr isGrp = toNodeGroup( node->getEffectInstance()->shared_from_this() );
            if (!isGrp) {
                qDebug() << "The group passed to createNode() is not a group, defaulting to the project root.";
            } else {
                collection = boost::dynamic_pointer_cast<NodeCollection>(isGrp);
                assert(collection);
            }
        }
    }

    if (!collection) {
        collection = boost::dynamic_pointer_cast<NodeCollection>( getInternalApp()->getProject() );
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
    args->setProperty<bool>(kCreateNodeArgsPropSubGraphOpened, false);
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
                if (prop == kCreateNodeArgsPropVolatile) {
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
App::createEffectFromNodeWrapper(const NodePtr& node)
{
    assert(node);

    // First, try to re-use an existing Effect object that was created for this node.
    // If not found, create one.
    std::stringstream ss;
    ss << kPythonTmpCheckerVariable << " = ";
    ss << node->getApp()->getAppIDString() << "." << node->getFullyQualifiedName();
    std::string script = ss.str();
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, 0, 0);
    // Clear errors if our call to interpretPythonScript failed, we don't want the
    // calling function to fail aswell.
    NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
    if (ok) {
        PyObject* pyEffect = 0;
        PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
        if ( PyObject_HasAttrString(mainModule, kPythonTmpCheckerVariable) ) {
            pyEffect = PyObject_GetAttrString(mainModule, kPythonTmpCheckerVariable);
            if (pyEffect == Py_None) {
                pyEffect = 0;
            }
        }
        Effect* cppEffect = 0;
        if (pyEffect && Shiboken::Object::isValid(pyEffect)) {
            cppEffect = (Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)pyEffect);
        }

        NATRON_PYTHON_NAMESPACE::interpretPythonScript("del " kPythonTmpCheckerVariable, 0, 0);
        NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
        if (cppEffect) {
            return cppEffect;
        }
    }

    // Ok not found, create one
    return new Effect(node);
}

App*
App::createAppFromAppInstance(const AppInstancePtr& app)
{
    assert(app);

    // First, try to re-use an existing Effect object that was created for this node.
    // If not found, create one.
    std::stringstream ss;
    ss << kPythonTmpCheckerVariable << " = " << app->getAppIDString() ;
    std::string script = ss.str();
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, 0, 0);
    // Clear errors if our call to interpretPythonScript failed, we don't want the
    // calling function to fail aswell.
    NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
    if (ok) {
        PyObject* pyApp = 0;
        PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
        if ( PyObject_HasAttrString(mainModule, kPythonTmpCheckerVariable )) {
            pyApp = PyObject_GetAttrString(mainModule, kPythonTmpCheckerVariable);
            if (pyApp == Py_None) {
                pyApp = 0;
            }
        }
        App* cppApp = 0;
        if (pyApp && Shiboken::Object::isValid(pyApp)) {
            cppApp = (App*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], (SbkObject*)pyApp);
        }
        NATRON_PYTHON_NAMESPACE::interpretPythonScript("del " kPythonTmpCheckerVariable, 0, 0);
        NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
        if (cppApp) {
            return cppApp;
        }
    }

    // Ok not found, create one
    return new App(app);
}

Effect*
App::createNode(const QString& pluginID,
                int majorVersion,
                Group* group,
                const NodeCreationPropertyMap& props) const
{
    NodeCollectionPtr collection = getCollectionFromGroup(group);

    assert(collection);
    CreateNodeArgsPtr args(new CreateNodeArgs);
    makeCreateNodeArgs(getInternalApp(), pluginID, majorVersion, collection, props, args.get());

    NodePtr node = getInternalApp()->createNode(args);
    if (node) {
        return App::createEffectFromNodeWrapper(node);
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

    CreateNodeArgsPtr args(new CreateNodeArgs);
    makeCreateNodeArgs(getInternalApp(),  QString::fromUtf8(PLUGINID_NATRON_READ), -1, collection, props, args.get());

    NodePtr node = getInternalApp()->createReader(filename.toStdString(), args);
    if (node) {
        return App::createEffectFromNodeWrapper(node);
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
    CreateNodeArgsPtr args(new CreateNodeArgs);
    makeCreateNodeArgs(getInternalApp(), QString::fromUtf8(PLUGINID_NATRON_WRITE), -1, collection, props, args.get());
    NodePtr node = getInternalApp()->createWriter(filename.toStdString(), args);
    if (node) {
        return App::createEffectFromNodeWrapper(node);
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
    TimeValue left, right;

    getInternalApp()->getProject()->getFrameRange(&left, &right);

    return left;
}

int
App::timelineGetRightBound() const
{
    TimeValue left, right;

    getInternalApp()->getProject()->getFrameRange(&left, &right);

    return right;
}

void
App::timelineGoTo(int frame)
{
    getInternalApp()->getTimeLine()->seekFrame(frame, false, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);
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
    _settings->saveSettingsToFile();
}

void
AppSettings::restoreDefaultSettings()
{
    _settings->restoreAllSettingsToDefaults();
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
    RenderQueue::RenderWork w;
    NodePtr node =  writeNode->getInternalNode();
    if (!node) {
        std::cerr << tr("Invalid write node").toStdString() << std::endl;

        return;
    }
    w.treeRoot = node;


    w.firstFrame = TimeValue(firstFrame);
    w.lastFrame = TimeValue(lastFrame);
    w.frameStep = TimeValue(frameStep);
    w.useRenderStats = false;

    std::list<RenderQueue::RenderWork> l;
    l.push_back(w);
    if (forceBlocking) {
        getInternalApp()->getRenderQueue()->renderBlocking(l);
    } else {
        getInternalApp()->getRenderQueue()->renderNonBlocking(l);
    }
}

void
App::renderInternal(bool forceBlocking,
                    const std::list<Effect*>& effects,
                    const std::list<int>& firstFrames,
                    const std::list<int>& lastFrames,
                    const std::list<int>& frameSteps)
{
    std::list<RenderQueue::RenderWork> l;

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
        RenderQueue::RenderWork w;
        NodePtr node =  (*itE)->getInternalNode();
        if (!node) {
            std::cerr << tr("Invalid write node").toStdString() << std::endl;

            return;
        }
        w.treeRoot = node;
        if ( !w.treeRoot || !w.treeRoot->getEffectInstance()->isOutput() ) {
            std::cerr << tr("Invalid write node").toStdString() << std::endl;

            return;
        }

        w.firstFrame = TimeValue(*itF);
        w.lastFrame = TimeValue(*itL);
        w.frameStep = TimeValue(*itS);
        w.useRenderStats = false;

        l.push_back(w);
    }
    if (forceBlocking) {
        getInternalApp()->getRenderQueue()->renderBlocking(l);
    } else {
        getInternalApp()->getRenderQueue()->renderNonBlocking(l);
    }
}

void
App::redrawViewer(Effect* viewerNode)
{
    if (!viewerNode) {
        return;
    }
    NodePtr internalNode = viewerNode->getInternalNode();
    if (!internalNode) {
        return;
    }
    ViewerNodePtr viewer = internalNode->isEffectViewerNode();
    if (!viewer) {
        return;
    }
    viewer->redrawViewer();
}

void
App::refreshViewer(Effect* viewerNode, bool useCache)
{
    if (!viewerNode) {
        return;
    }
    NodePtr internalNode = viewerNode->getInternalNode();
    if (!internalNode) {
        return;
    }
    ViewerNodePtr viewer = internalNode->isEffectViewerNode();
    if (!viewer) {
        return;
    }
    if (!useCache) {
        viewer->forceNextRenderWithoutCacheRead();
    }
    viewer->getNode()->getRenderEngine()->renderCurrentFrame();
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
    if ( !getInternalApp()->getProject()->addDefaultFormat( formatSpec.toStdString() ) ) {
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
    return getInternalApp()->save( filename.toStdString() );
}

bool
App::saveProjectAs(const QString& filename)
{
    return getInternalApp()->saveAs( filename.toStdString() );
}

App*
App::loadProject(const QString& filename)
{
    AppInstancePtr app  = getInternalApp()->loadProject( filename.toStdString() );

    if (!app) {
        return 0;
    }

    return App::createAppFromAppInstance(app);
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

    return App::createAppFromAppInstance(app);
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

int
App::getViewIndex(const QString& viewName) const
{
    ViewIdx view_i;
    if (!getInternalApp()->getProject()->getViewIndex(viewName.toStdString(), &view_i)) {
        return false;
    }
    return view_i;
}

QString
App::getViewName(int viewIndex) const
{
    if (viewIndex < 0) {
        return QString();
    }
    return QString::fromUtf8(getInternalApp()->getProject()->getViewName(ViewIdx(viewIndex)).c_str());
}

void
App::addProjectLayer(const ImageLayer& layer)
{
    getInternalApp()->getProject()->addProjectDefaultLayer( layer.getInternalComps() );
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT
