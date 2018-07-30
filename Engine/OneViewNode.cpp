/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include "Engine/InputDescription.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"


NATRON_NAMESPACE_ENTER


#define kOneViewViewParam "view"
#define kOneViewViewParamLabel "View"
#define kOneViewViewParamHint "View to take from the input"


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
    PluginPtr ret = Plugin::create(OneViewNode::create, OneViewNode::createRenderClone, PLUGINID_NATRON_ONEVIEW, "OneView", 1, 0, grouping);

    QString desc =  tr("Takes one view from the input");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, true);

    ret->setProperty<std::string>(kNatronPluginPropIconFilePath, NATRON_IMAGES_PATH "oneViewNode.png");
    ret->setProperty<bool>(kNatronPluginPropViewAware, true);
    ret->setProperty<ViewInvarianceLevel>(kNatronPluginPropViewInvariant, eViewInvarianceAllViewsVariant);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));
    {
        InputDescriptionPtr input = InputDescription::create("Source", "Source", "", false, false, std::bitset<4>(std::string("1111")));
        ret->addInputDescription(input);
    }

    return ret;
}

OneViewNode::OneViewNode(const NodePtr& n)
    : EffectInstance(n)
    , _imp( new OneViewNodePrivate() )
{

}

OneViewNode::OneViewNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
: EffectInstance(mainInstance, key)
, _imp( new OneViewNodePrivate() )
{

}

OneViewNode::~OneViewNode()
{
}


void
OneViewNode::initializeKnobs()
{
    KnobPagePtr page = createKnob<KnobPage>("controlsPage");
    page->setLabel(tr("Controls"));

    KnobChoicePtr viewKnob = createKnob<KnobChoice>(kOneViewViewParam);
    viewKnob->setLabel(tr(kOneViewViewParamLabel));
    viewKnob->setHintToolTip(tr(kOneViewViewParamHint));
    page->addKnob(viewKnob);

    const std::vector<std::string>& views = getApp()->getProject()->getProjectViewNames();

    std::vector<ChoiceOption> options(views.size());
    for (std::size_t i = 0; i < views.size(); ++i) {
        options[i].id = views[i];
    }
    viewKnob->populateChoices(options);


    _imp->viewKnob = viewKnob;
}

void
OneViewNode::fetchRenderCloneKnobs()
{
    EffectInstance::fetchRenderCloneKnobs();
    _imp->viewKnob = toKnobChoice(getKnobByName(kOneViewViewParam));
}

ActionRetCodeEnum
OneViewNode::isIdentity(TimeValue /*time*/,
                        const RenderScale & /*scale*/,
                        const RectI & /*roi*/,
                        ViewIdx /*view*/,
                        const ImagePlaneDesc& /*plane*/,
                        TimeValue* /*inputTime*/,
                        ViewIdx* inputView,
                        int* inputNb,
                        ImagePlaneDesc* /*inputPlane*/)
{
    KnobChoicePtr viewKnob = _imp->viewKnob.lock();
    int view_i = viewKnob->getValue();

    *inputView = ViewIdx(view_i);
    *inputNb = 0;

    return eActionStatusOK;
}

ActionRetCodeEnum
OneViewNode::getFramesNeeded(TimeValue time,
                             ViewIdx /*view*/,
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
OneViewNode::onMetadataChanged(const NodeMetadata& metadata)
{
    const std::vector<std::string>& views = getApp()->getProject()->getProjectViewNames();
    KnobChoicePtr viewKnob = _imp->viewKnob.lock();

    ChoiceOption currentView = viewKnob->getCurrentEntry();

    std::vector<ChoiceOption> options(views.size());
    for (std::size_t i = 0; i < views.size(); ++i) {
        options[i].id = views[i];
    }
    viewKnob->populateChoices(options);

    bool foundView = false;
    for (std::size_t i = 0; i < views.size(); ++i) {
        if (views[i] == currentView.id) {
            foundView = true;
            break;
        }
    }
    if (!foundView) {
        viewKnob->setValue(0);
    }
    EffectInstance::onMetadataChanged(metadata);
} // onMetadataChanged


NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_OneViewNode.cpp"
