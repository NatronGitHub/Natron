//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "AppInstanceWrapper.h"

#include <QDebug>

#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/EffectInstance.h"
#include "Engine/Settings.h"

App::App(AppInstance* instance)
: Group()
, _instance(instance)
{
    if (instance->getProject()) {
        init(instance->getProject());
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

Effect*
App::createNode(const std::string& pluginID,
                int majorVersion,
                Group* group) const
{
    boost::shared_ptr<NodeCollection> collection;
    if (group) {
        App* isApp = dynamic_cast<App*>(group);
        Effect* isEffect = dynamic_cast<Effect*>(group);
        if (isApp) {
            collection = boost::dynamic_pointer_cast<NodeCollection>(isApp->getInternalApp()->getProject());
        } else if (isEffect) {
            boost::shared_ptr<Natron::Node> node = isEffect->getInternalNode();
            assert(node);
            boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>(node->getLiveInstance()->shared_from_this());
            if (!isGrp) {
                qDebug() << "The group passed to createNode() is not a group, defaulting to the project root.";
            } else {
                collection = boost::dynamic_pointer_cast<NodeCollection>(isGrp);
                assert(collection);
            }

        }
    }
    
    if (!collection) {
        collection = boost::dynamic_pointer_cast<NodeCollection>(_instance->getProject());
    }
    
    assert(collection);
    
    CreateNodeArgs args(pluginID.c_str(),
                        "",
                        majorVersion,
                        -1,
                        false,
                        INT_MIN,
                        INT_MIN,
                        false,
                        true,
                        QString(),
                        CreateNodeArgs::DefaultValuesList(),
                        collection);
    boost::shared_ptr<Natron::Node> node = _instance->createNode(args);
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
    int left,right;
    _instance->getFrameRange(&left, &right);
    return left;
}

int
App::timelineGetRightBound() const
{
    int left,right;
    _instance->getFrameRange(&left, &right);
    return right;
}


AppSettings::AppSettings(const boost::shared_ptr<Settings>& settings)
: _settings(settings)
{
    
}

Param*
AppSettings::getParam(const std::string& scriptName) const
{
    boost::shared_ptr<KnobI> knob = _settings->getKnobByName(scriptName);
    if (!knob) {
        return 0;
    }
    return Effect::createParamWrapperForKnob(knob);
}

std::list<Param*>
AppSettings::getParams() const
{
    std::list<Param*> ret;
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _settings->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
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
    _settings->saveSettings();
}

void AppSettings::restoreDefaultSettings()
{
    _settings->restoreDefault();
}

void
App::render(const RenderTask& task)
{
    if (!task.writeNode) {
        std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;
        return;
    }
    AppInstance::RenderWork w;
    NodePtr node =  task.writeNode->getInternalNode();
    if (!node) {
        std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;
        return;
    }
    w.writer = dynamic_cast<Natron::OutputEffectInstance*>(node->getLiveInstance());
    if (!w.writer) {
        std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;
        return;
    }
    
    w.firstFrame = task.firstFrame;
    w.lastFrame = task.lastFrame;
    
    std::list<AppInstance::RenderWork> l;
    l.push_back(w);
    _instance->startWritersRendering(l);
}

void
App::render(const std::list<RenderTask>& tasks)
{
    std::list<AppInstance::RenderWork> l;

    for (std::list<RenderTask>::const_iterator it = tasks.begin(); it != tasks.end(); ++it) {
        if (!it->writeNode) {
            std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;
            return;
        }
        AppInstance::RenderWork w;
        NodePtr node =  it->writeNode->getInternalNode();
        if (!node) {
            std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;
            return;
        }
        w.writer = dynamic_cast<Natron::OutputEffectInstance*>(node->getLiveInstance());
        if (!w.writer || !w.writer->isOutput()) {
            std::cerr << QObject::tr("Invalid write node").toStdString() << std::endl;
            return;
        }
        
        w.firstFrame = it->firstFrame;
        w.lastFrame = it->lastFrame;
        
        l.push_back(w);

    }
    _instance->startWritersRendering(l);
}

Param*
App::getProjectParam(const std::string& name) const
{
    boost::shared_ptr<KnobI> knob =  _instance->getProject()->getKnobByName(name);
    if (!knob) {
        return 0;
    }
    return Effect::createParamWrapperForKnob(knob);
}
