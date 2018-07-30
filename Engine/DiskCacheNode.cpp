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

#include "DiskCacheNode.h"

#include <cassert>
#include <stdexcept>

#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/InputDescription.h"
#include "Engine/Project.h"
#include "Engine/RenderQueue.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER


#define kDiskCacheNodeFirstFrame "firstFrame"
#define kDiskCacheNodeFirstFrameLabel "First Frame"
#define kDiskCacheNodeFirstFrameHint ""

#define kDiskCacheNodeLastFrame "lastFrame"
#define kDiskCacheNodeLastFrameLabel "Last Frame"
#define kDiskCacheNodeLastFrameHint ""

#define kDiskCacheNodeFrameRange "frameRange"
#define kDiskCacheNodeFrameRangeLabel "Frame Range"
#define kDiskCacheNodeFrameRangeHint ""


struct DiskCacheNodePrivate
{
    KnobChoiceWPtr frameRange;
    KnobIntWPtr firstFrame;
    KnobIntWPtr lastFrame;
    KnobButtonWPtr preRender;

    DiskCacheNodePrivate()
    {
    }
};

PluginPtr
DiskCacheNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_OTHER);
    PluginPtr ret = Plugin::create(DiskCacheNode::create, DiskCacheNode::createRenderClone, PLUGINID_NATRON_DISKCACHE, "DiskCache", 1, 0, grouping);

    QString desc =  tr("This node caches all images of the connected input node onto the disk with full 32bit floating point raw data. "
                       "When an image is found in the cache, %1 will then not request the input branch to render out that image. "
                       "The DiskCache node only caches full images and does not split up the images in chunks.  "
                       "The DiskCache node is useful if working with a large and complex node tree: this allows to break the tree into smaller "
                       "branches and cache any branch that one is no longer working on. The cached images are saved by default in the same directory that is used "
                       "for the viewer cache but you can set its location and size in the preferences. A solid state drive disk is recommended for efficiency of this node. "
                       "By default all images that pass into the node are cached but they depend on the zoom-level of the viewer. For convenience you can cache "
                       "a specific frame range at scale 100% much like a writer node would do.\n"
                       "WARNING: The DiskCache node must be part of the tree when you want to read cached data from it.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) );
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, false);
    effectDesc->setProperty<bool>(kEffectPropSupportsMultiResolution, true);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/diskcache_icon.png");
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));

    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));
    {
        InputDescriptionPtr input = InputDescription::create("Source", "", "", false, false, std::bitset<4>(std::string("1111")));
        ret->addInputDescription(input);
    }

    return ret;
}


DiskCacheNode::DiskCacheNode(const NodePtr& node)
    : EffectInstance(node)
    , _imp( new DiskCacheNodePrivate() )
{
}

DiskCacheNode::DiskCacheNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
: EffectInstance(mainInstance, key)
, _imp(new DiskCacheNodePrivate())
{

}

DiskCacheNode::~DiskCacheNode()
{
}


bool
DiskCacheNode::shouldCacheOutput(bool /*isFrameVaryingOrAnimated*/,
                                 int /*visitsCount*/) const
{
    // The disk cache node always caches.
    return true;
}

void
DiskCacheNode::initializeKnobs()
{
    KnobPagePtr page = createKnob<KnobPage>("controlsPage");
    page->setLabel(tr("Controls") );
    KnobChoicePtr frameRange = createKnob<KnobChoice>(kDiskCacheNodeFrameRange);
    frameRange->setLabel(tr(kDiskCacheNodeFrameRangeLabel) );
    frameRange->setHintToolTip(tr(kDiskCacheNodeFrameRangeHint));
    frameRange->setAnimationEnabled(false);
    {
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption("Input frame range", "", ""));
        choices.push_back(ChoiceOption("Project frame range", "", ""));
        choices.push_back(ChoiceOption("Manual","", ""));
        frameRange->populateChoices(choices);
    }
    frameRange->setEvaluateOnChange(false);
    frameRange->setDefaultValue(0);
    page->addKnob(frameRange);
    _imp->frameRange = frameRange;

    KnobIntPtr firstFrame = createKnob<KnobInt>(kDiskCacheNodeFirstFrame);
    firstFrame->setLabel(tr(kDiskCacheNodeFirstFrameLabel) );
    firstFrame->setHintToolTip(tr(kDiskCacheNodeFirstFrameHint));
    firstFrame->setAnimationEnabled(false);
    firstFrame->disableSlider();
    firstFrame->setEvaluateOnChange(false);
    firstFrame->setAddNewLine(false);
    firstFrame->setDefaultValue(1);
    firstFrame->setSecret(true);
    page->addKnob(firstFrame);
    _imp->firstFrame = firstFrame;

    KnobIntPtr lastFrame = createKnob<KnobInt>(kDiskCacheNodeLastFrame);
    lastFrame->setAnimationEnabled(false);
    lastFrame->setLabel(tr(kDiskCacheNodeLastFrameLabel));
    lastFrame->setHintToolTip(tr(kDiskCacheNodeLastFrameHint));
    lastFrame->disableSlider();
    lastFrame->setEvaluateOnChange(false);
    lastFrame->setDefaultValue(100);
    lastFrame->setSecret(true);
    page->addKnob(lastFrame);
    _imp->lastFrame = lastFrame;

    KnobButtonPtr preRender = createKnob<KnobButton>("preRender");
    preRender->setLabel(tr("Pre-cache"));
    preRender->setEvaluateOnChange(false);
    preRender->setHintToolTip( tr("Cache the frame range specified by rendering images at zoom-level 100% only.") );
    page->addKnob(preRender);
    _imp->preRender = preRender;
}

void
DiskCacheNode::fetchRenderCloneKnobs()
{
    EffectInstance::fetchRenderCloneKnobs();
    _imp->frameRange = toKnobChoice(getKnobByName(kDiskCacheNodeFrameRange));
    _imp->firstFrame = toKnobInt(getKnobByName(kDiskCacheNodeFirstFrame));
    _imp->lastFrame = toKnobInt(getKnobByName(kDiskCacheNodeLastFrame));
}

bool
DiskCacheNode::knobChanged(const KnobIPtr& k,
                           ValueChangedReasonEnum /*reason*/,
                           ViewSetSpec /*view*/,
                           TimeValue /*time*/)
{
    bool ret = true;

    if (_imp->frameRange.lock() == k) {
        int idx = _imp->frameRange.lock()->getValue(DimIdx(0));
        switch (idx) {
        case 0:
        case 1:
            _imp->firstFrame.lock()->setSecret(true);
            _imp->lastFrame.lock()->setSecret(true);
            break;
        case 2:
            _imp->firstFrame.lock()->setSecret(false);
            _imp->lastFrame.lock()->setSecret(false);
            break;
        default:
            break;
        }
    } else if (_imp->preRender.lock() == k) {
        RenderQueue::RenderWork w;
        w.renderLabel = tr("Caching").toStdString();
        w.treeRoot = getNode();
        w.frameStep = TimeValue(1.);
        w.useRenderStats = false;
        std::list<RenderQueue::RenderWork> works;
        works.push_back(w);
        getApp()->getRenderQueue()->renderNonBlocking(works);
    } else {
        ret = false;
    }

    return ret;
}

ActionRetCodeEnum
DiskCacheNode::getFrameRange(double *first,
                             double *last)
{
    int idx = _imp->frameRange.lock()->getValue();

    switch (idx) {
    case 0: {
        EffectInstancePtr input = getInputRenderEffectAtAnyTimeView(0);
        if (input) {

            GetFrameRangeResultsPtr results;
            ActionRetCodeEnum stat = input->getFrameRange_public(&results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            RangeD range;
            results->getFrameRangeResults(&range);
            *first = range.min;
            *last = range.max;

        }
        break;
    }
    case 1: {
        TimeValue left, right;
        getApp()->getProject()->getFrameRange(&left, &right);
        *first = left;
        *last = right;
        break;
    }
    case 2: {
        *first = _imp->firstFrame.lock()->getValue();
        *last = _imp->lastFrame.lock()->getValue();
    };
    default:
        break;
    }
    return eActionStatusOK;
}

ActionRetCodeEnum
DiskCacheNode::render(const RenderActionArgs& args)
{
    // fetch source images and copy them

    for (std::list<std::pair<ImagePlaneDesc, ImagePtr> >::const_iterator it = args.outputPlanes.begin(); it != args.outputPlanes.end(); ++it) {

        GetImageInArgs inArgs(&args.mipMapLevel, &args.proxyScale, &args.roi, &args.backendType);
        inArgs.inputNb = 0;
        inArgs.plane = &it->first;
        GetImageOutArgs outArgs;
        if (!getImagePlane(inArgs, &outArgs)) {
            return eActionStatusInputDisconnected;
        }

        Image::CopyPixelsArgs cpyArgs;
        cpyArgs.roi = args.roi;
        ActionRetCodeEnum stat = it->second->copyPixels(*outArgs.image, cpyArgs);
        if (isFailureRetCode(stat)) {
            return stat;
        }

    }
    return eActionStatusOK;

} // render

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING

#include "moc_DiskCacheNode.cpp"

