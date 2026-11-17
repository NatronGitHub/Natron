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

#include "OneViewNode.h"

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_ENTER

struct OneViewNodePrivate
{
    KnobChoiceWPtr viewKnob;

    OneViewNodePrivate()
        : viewKnob()
    {
    }
};

OneViewNode::OneViewNode(NodePtr n)
    : EffectInstance(n)
    , _imp( new OneViewNodePrivate() )
{
    setSupportsRenderScaleMaybe(eSupportsYes);
    if (n) {
        ProjectPtr project = n->getApp()->getProject();
        QObject::connect( project.get(), SIGNAL(projectViewsChanged()), this, SLOT(onProjectViewsChanged()) );
    }
}

OneViewNode::~OneViewNode()
{
}

std::string
OneViewNode::getPluginID() const
{
    return PLUGINID_NATRON_ONEVIEW;
}

std::string
OneViewNode::getPluginLabel() const
{
    return "OneView";
}

std::string
OneViewNode::getPluginDescription() const
{
    return "Takes one view from the input.";
}

void
OneViewNode::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_MULTIVIEW);
}

std::string
OneViewNode::getInputLabel (int /*inputNb*/) const
{
    return "Source";
}

void
OneViewNode::addAcceptedComponents(int /*inputNb*/,
                                   std::list<ImagePlaneDesc>* comps)
{
    comps->push_back( ImagePlaneDesc::getRGBAComponents() );
    comps->push_back( ImagePlaneDesc::getAlphaComponents() );
    comps->push_back( ImagePlaneDesc::getRGBComponents() );
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
    KnobPagePtr page = AppManager::createKnob<KnobPage>( this, tr("Controls") );

    page->setName("controls");

    KnobChoicePtr viewKnob = AppManager::createKnob<KnobChoice>( this, tr("View") );
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

bool
OneViewNode::isIdentity(double time,
                        const RenderScale & /*scale*/,
                        const RectI & /*roi*/,
                        ViewIdx /*view*/,
                        double* inputTime,
                        ViewIdx* inputView,
                        int* inputNb)
{
    KnobChoicePtr viewKnob = _imp->viewKnob.lock();
    int view_i = viewKnob->getValue();

    *inputView = ViewIdx(view_i);
    *inputNb = 0;
    *inputTime = time;

    return true;
}

FramesNeededMap
OneViewNode::getFramesNeeded(double time,
                             ViewIdx /*view*/)
{
    FramesNeededMap ret;
    FrameRangesMap& rangeMap = ret[0];
    KnobChoicePtr viewKnob = _imp->viewKnob.lock();
    int view_i = viewKnob->getValue();
    std::vector<RangeD>& ranges = rangeMap[ViewIdx(view_i)];

    ranges.resize(1);
    ranges[0].min = ranges[0].max = time;

    return ret;
}

void
OneViewNode::onProjectViewsChanged()
{
    const std::vector<std::string>& views = getApp()->getProject()->getProjectViewNames();
    KnobChoicePtr viewKnob = _imp->viewKnob.lock();
    ChoiceOption currentView = viewKnob->getActiveEntry();

    std::vector<ChoiceOption> options(views.size());
    for (std::size_t i = 0; i < views.size(); ++i) {
        options[i].id = views[i];
    }
    viewKnob->populateChoices(options);

    bool foundView = false;
    for (std::size_t i = 0; i < views.size(); ++i) {
        if (views[i] == currentView.id) {
            foundView = true;
            viewKnob->setValue(i);
            break;
        }
    }
    if (!foundView) {
        viewKnob->setValue(0);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING

#include "moc_OneViewNode.cpp"
