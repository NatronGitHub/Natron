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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PyAppInstance.h"

#include <cassert>
#include <stdexcept>

#include <QDebug>

#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/EffectInstance.h"
#include "Engine/Settings.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

App::App(AppInstance* instance)
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
    return _instance->getAppID();
}

AppInstance*
App::getInternalApp() const
{
    return _instance;
}

boost::shared_ptr<NodeCollection>
App::getCollectionFromGroup(Group* group) const
{
    boost::shared_ptr<NodeCollection> collection;

    if (group) {
        App* isApp = dynamic_cast<App*>(group);
        Effect* isEffect = dynamic_cast<Effect*>(group);
        if (isApp) {
            collection = boost::dynamic_pointer_cast<NodeCollection>( isApp->getInternalApp()->getProject() );
        } else if (isEffect) {
            NodePtr node = isEffect->getInternalNode();
            assert(node);
            boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>( node->getEffectInstance()->shared_from_this() );
            if (!isGrp) {
                qDebug() << "The group passed to createNode() is not a group, defaulting to the project root.";
            } else {
                collection = boost::dynamic_pointer_cast<NodeCollection>(isGrp);
                assert(collection);
            }
        }
    }

    if (!collection) {
        collection = boost::dynamic_pointer_cast<NodeCollection>( _instance->getProject() );
    }

    return collection;
}

Effect*
App::createNode(const QString& pluginID,
                int majorVersion,
                Group* group) const
{
    boost::shared_ptr<NodeCollection> collection = getCollectionFromGroup(group);

    assert(collection);

    CreateNodeArgs args(pluginID, eCreateNodeReasonInternal, collection);
    args.majorV = majorVersion;

    NodePtr node = _instance->createNode(args);
    if (node) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

Effect*
App::createReader(const QString& filename,
                  Group* group) const
{
    boost::shared_ptr<NodeCollection> collection = getCollectionFromGroup(group);

    assert(collection);
    NodePtr node = _instance->createReader(filename.toStdString(), eCreateNodeReasonInternal, collection);
    if (node) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

Effect*
App::createWriter(const QString& filename,
                  Group* group) const
{
    boost::shared_ptr<NodeCollection> collection = getCollectionFromGroup(group);

    assert(collection);
    NodePtr node = _instance->createWriter(filename.toStdString(), eCreateNodeReasonInternal, collection);
    if (node) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

int
App::timelineGetTime() const
{
    return _instance->getTimeLine()->currentFrame();
}

int
App::timelineGetLeftBound() const
{
    double left, right;

    _instance->getFrameRange(&left, &right);

    return left;
}

int
App::timelineGetRightBound() const
{
    double left, right;

    _instance->getFrameRange(&left, &right);

    return right;
}

AppSettings::AppSettings(const boost::shared_ptr<Settings>& settings)
    : _settings(settings)
{
}

Param*
AppSettings::getParam(const QString& scriptName) const
{
    KnobPtr knob = _settings->getKnobByName( scriptName.toStdString() );

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
        std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;

        return;
    }
    AppInstance::RenderWork w;
    NodePtr node =  writeNode->getInternalNode();
    if (!node) {
        std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;

        return;
    }
    w.writer = dynamic_cast<OutputEffectInstance*>( node->getEffectInstance().get() );
    if (!w.writer) {
        std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;

        return;
    }

    w.firstFrame = firstFrame;
    w.lastFrame = lastFrame;
    w.frameStep = frameStep;
    w.useRenderStats = false;

    std::list<AppInstance::RenderWork> l;
    l.push_back(w);
    _instance->startWritersRendering(forceBlocking, l);
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
            std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;

            return;
        }
        AppInstance::RenderWork w;
        NodePtr node =  (*itE)->getInternalNode();
        if (!node) {
            std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;

            return;
        }
        w.writer = dynamic_cast<OutputEffectInstance*>( node->getEffectInstance().get() );
        if ( !w.writer || !w.writer->isOutput() ) {
            std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;

            return;
        }

        w.firstFrame = (*itF);
        w.lastFrame = (*itL);
        w.frameStep = (*itS);
        w.useRenderStats = false;

        l.push_back(w);
    }
    _instance->startWritersRendering(forceBlocking, l);
}

Param*
App::getProjectParam(const QString& name) const
{
    KnobPtr knob =  _instance->getProject()->getKnobByName( name.toStdString() );

    if (!knob) {
        return 0;
    }

    return Effect::createParamWrapperForKnob(knob);
}

void
App::writeToScriptEditor(const QString& message)
{
    _instance->appendToScriptEditor( message.toStdString() );
}

void
App::addFormat(const QString& formatSpec)
{
    if ( !_instance->getProject()->addFormat( formatSpec.toStdString() ) ) {
        _instance->appendToScriptEditor( formatSpec.toStdString() );
    }
}

bool
App::saveTempProject(const QString& filename)
{
    return _instance->saveTemp( filename.toStdString() );
}

bool
App::saveProject(const QString& filename)
{
    return _instance->save( filename.toStdString() );
}

bool
App::saveProjectAs(const QString& filename)
{
    return _instance->saveAs( filename.toStdString() );
}

App*
App::loadProject(const QString& filename)
{
    AppInstance* app  = _instance->loadProject( filename.toStdString() );

    if (!app) {
        return 0;
    }

    return new App(app);
}

///Close the current project but keep the window
bool
App::resetProject()
{
    return _instance->resetProject();
}

///Reset + close window, quit if last window
bool
App::closeProject()
{
    return _instance->closeProject();
}

///Opens a new window
App*
App::newProject()
{
    AppInstance* app  = _instance->newProject();

    if (!app) {
        return 0;
    }

    return new App(app);
}

std::list<QString>
App::getViewNames() const
{
    std::list<QString> ret;
    const std::vector<std::string>& v = _instance->getProject()->getProjectViewNames();

    for (std::size_t i = 0; i < v.size(); ++i) {
        ret.push_back( QString::fromUtf8( v[i].c_str() ) );
    }

    return ret;
}

void
App::addProjectLayer(const ImageLayer& layer)
{
    _instance->getProject()->addProjectDefaultLayer( layer.getInternalComps() );
}

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;
