/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "OneViewNode.h"

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_ENTER;

struct OneViewNodePrivate
{
    KnobChoiceWPtr viewKnob;

    OneViewNodePrivate()
        : viewKnob()
    {
    }
};

PluginPtr
OneViewNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_MULTIVIEW);
    PluginPtr ret = Plugin::create((void*)OneViewNode::create, PLUGINID_NATRON_ONEVIEW, "OneView", 1, 0, grouping);

    QString desc =  tr("Takes one view from the input");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath, NATRON_IMAGES_PATH "oneViewNode.png");
    return ret;
}

OneViewNode::OneViewNode(const NodePtr& n)
    : EffectInstance(n)
    , _imp( new OneViewNodePrivate() )
{
    if (n) {
        ProjectPtr project = n->getApp()->getProject();
        QObject::connect( project.get(), SIGNAL(projectViewsChanged()), this, SLOT(onProjectViewsChanged()) );
    }
}

OneViewNode::~OneViewNode()
{
}


std::string
OneViewNode::getInputLabel (int /*inputNb*/) const
{
    return "Source";
}

void
OneViewNode::addAcceptedComponents(int /*inputNb*/,
                                   std::bitset<4>* supported)
{
    (*supported)[0] = (*supported)[1] = (*supported)[2] = (*supported)[3] = 1;
}

void
OneViewNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthByte);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthFloat);
}

void
OneViewNode::initializeKnobs()
{
    KnobPagePtr page = AppManager::createKnob<KnobPage>( shared_from_this(), tr("Controls") );

    page->setName("controls");

    KnobChoicePtr viewKnob = AppManager::createKnob<KnobChoice>( shared_from_this(), tr("View") );
    viewKnob->setName("view");
    viewKnob->setHintToolTip( tr("View to take from the input") );
    page->addKnob(viewKnob);

    const std::vector<std::string>& views = getApp()->getProject()->getProjectViewNames();

    std::vector<ChoiceOption> options(views.size());
    for (std::size_t i = 0; i < views.size(); ++i) {
        options[i].id = views[i];
    }
    viewKnob->populateChoices(options);


    _imp->viewKnob = viewKnob;
}

ActionRetCodeEnum
OneViewNode::isIdentity(TimeValue time,
                        const RenderScale & /*scale*/,
                        const RectI & /*roi*/,
                        ViewIdx /*view*/,
                        const TreeRenderNodeArgsPtr& /*render*/,
                        TimeValue* inputTime,
                        ViewIdx* inputView,
                        int* inputNb)
{
    KnobChoicePtr viewKnob = _imp->viewKnob.lock();
    int view_i = viewKnob->getValue();

    *inputView = ViewIdx(view_i);
    *inputNb = 0;
    *inputTime = time;

    return eActionStatusOK;
}

ActionRetCodeEnum
OneViewNode::getFramesNeeded(TimeValue time,
                             ViewIdx /*view*/,
                             const TreeRenderNodeArgsPtr& /*render*/,
                             FramesNeededMap* ret)
{

    FrameRangesMap& rangeMap = (*ret)[0];
    KnobChoicePtr viewKnob = _imp->viewKnob.lock();
    int view_i = viewKnob->getValue();
    std::vector<RangeD>& ranges = rangeMap[ViewIdx(view_i)];

    ranges.resize(1);
    ranges[0].min = ranges[0].max = time;

    return eActionStatusOK;
}

void
OneViewNode::onProjectViewsChanged()
{
    const std::vector<std::string>& views = getApp()->getProject()->getProjectViewNames();
    KnobChoicePtr viewKnob = _imp->viewKnob.lock();

    std::string currentView = viewKnob->getActiveEntryID();

    std::vector<ChoiceOption> options(views.size());
    for (std::size_t i = 0; i < views.size(); ++i) {
        options[i].id = views[i];
    }
    viewKnob->populateChoices(options);

    bool foundView = false;
    for (std::size_t i = 0; i < views.size(); ++i) {
        if (views[i] == currentView) {
            foundView = true;
            break;
        }
    }
    if (!foundView) {
        viewKnob->setValue(0);
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;

#include "moc_OneViewNode.cpp"
