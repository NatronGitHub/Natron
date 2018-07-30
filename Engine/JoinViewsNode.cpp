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

#include "JoinViewsNode.h"

#include <QtCore/QMutex>

#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/NodeMetadata.h"

NATRON_NAMESPACE_ENTER



PluginPtr
JoinViewsNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_MULTIVIEW);
    PluginPtr ret = Plugin::create(JoinViewsNode::create, JoinViewsNode::createRenderClone, PLUGINID_NATRON_JOINVIEWS, "JoinViews", 1, 0, grouping);

    QString desc =  tr("Take in input separate views to make a multiple view stream output. "
                       "The first view from each input is copied to one of the view of the output.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, true);
    ret->setProperty<bool>(kNatronPluginPropViewAware, true);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/joinViewsNode.png");
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));
    return ret;
}

JoinViewsNode::JoinViewsNode(const NodePtr& node)
    : EffectInstance(node)
{

}

JoinViewsNode::JoinViewsNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
: EffectInstance(mainInstance, key)
{

}

JoinViewsNode::~JoinViewsNode()
{
}

void
JoinViewsNode::initializeKnobs()
{
}


ActionRetCodeEnum
JoinViewsNode::isIdentity(TimeValue /*time*/,
                          const RenderScale & /*scale*/,
                          const RectI & /*roi*/,
                          ViewIdx view,
                          const ImagePlaneDesc& /*plane*/,
                          TimeValue* /*inputTime*/,
                          ViewIdx* /*inputView*/,
                          int* inputNb,
                          ImagePlaneDesc* /*inputPlane*/)
{
    *inputNb = getNInputs() - 1 - view.value();
    return eActionStatusOK;
}

void
JoinViewsNode::onMetadataChanged(const NodeMetadata& metadata)
{
    NodePtr node = getNode();

    std::size_t nInputs, oldNInputs;

    const std::vector<std::string>& views = getApp()->getProject()->getProjectViewNames();

    //Reverse names
    oldNInputs = getNInputs();
    nInputs = views.size();
    std::vector<NodePtr> inputs(oldNInputs);
    for (std::size_t i = 0; i < oldNInputs; ++i) {
        inputs[i] = node->getInput(i);
    }
    node->beginInputEdition();
    node->removeAllInputs();
    for (std::size_t i = 0; i < nInputs; ++i) {
        const std::string& inputName = views[nInputs - 1 - i];
        InputDescriptionPtr desc = InputDescription::create(inputName, inputName, "", false, false, std::bitset<4>(std::string("1111")));
        node->addInput(desc);
    }


    //Reconnect the inputs
    int index = oldNInputs - 1;
    if (index >= 0) {


        for (int i = (int)nInputs - 1; i  >= 0; --i, --index) {
            node->disconnectInput(i);
            if (index >= 0) {
                if (inputs[index]) {
                    node->connectInput(inputs[index], i);
                }
            }
        }
    }
    node->endInputEdition(true);

    EffectInstance::onMetadataChanged(metadata);
}


NATRON_NAMESPACE_EXIT

