/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

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

DiskCacheNode::DiskCacheNode(NodePtr node)
    : OutputEffectInstance(node)
    , _imp( new DiskCacheNodePrivate() )
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

DiskCacheNode::~DiskCacheNode()
{
}

void
DiskCacheNode::addAcceptedComponents(int /*inputNb*/,
                                     std::list<ImagePlaneDesc>* comps)
{
    comps->push_back( ImagePlaneDesc::getRGBAComponents() );
    comps->push_back( ImagePlaneDesc::getRGBComponents() );
    comps->push_back( ImagePlaneDesc::getAlphaComponents() );
}

void
DiskCacheNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
}

bool
DiskCacheNode::shouldCacheOutput(bool /*isFrameVaryingOrAnimated*/,
                                 double /*time*/,
                                 ViewIdx /*view*/,
                                 int /*visitsCount*/) const
{
    return true;
}

void
DiskCacheNode::initializeKnobs()
{
    boost::shared_ptr<KnobPage> page = AppManager::createKnob<KnobPage>( this, tr("Controls") );
    boost::shared_ptr<KnobChoice> frameRange = AppManager::createKnob<KnobChoice>( this, tr("Frame range") );

    frameRange->setName("frameRange");
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

    boost::shared_ptr<KnobInt> firstFrame = AppManager::createKnob<KnobInt>( this, tr("First Frame") );
    firstFrame->setAnimationEnabled(false);
    firstFrame->setName("firstFrame");
    firstFrame->disableSlider();
    firstFrame->setEvaluateOnChange(false);
    firstFrame->setAddNewLine(false);
    firstFrame->setDefaultValue(1);
    firstFrame->setSecretByDefault(true);
    page->addKnob(firstFrame);
    _imp->firstFrame = firstFrame;

    boost::shared_ptr<KnobInt> lastFrame = AppManager::createKnob<KnobInt>( this, tr("Last Frame") );
    lastFrame->setAnimationEnabled(false);
    lastFrame->setName("LastFrame");
    lastFrame->disableSlider();
    lastFrame->setEvaluateOnChange(false);
    lastFrame->setDefaultValue(100);
    lastFrame->setSecretByDefault(true);
    page->addKnob(lastFrame);
    _imp->lastFrame = lastFrame;

    boost::shared_ptr<KnobButton> preRender = AppManager::createKnob<KnobButton>( this, tr("Pre-cache") );
    preRender->setName("preRender");
    preRender->setEvaluateOnChange(false);
    preRender->setHintToolTip( tr("Cache the frame range specified by rendering images at zoom-level 100% only.") );
    page->addKnob(preRender);
    _imp->preRender = preRender;
}

bool
DiskCacheNode::knobChanged(KnobI* k,
                           ValueChangedReasonEnum /*reason*/,
                           ViewSpec /*view*/,
                           double /*time*/,
                           bool /*originatedFromMainThread*/)
{
    bool ret = true;

    if (_imp->frameRange.lock().get() == k) {
        int idx = _imp->frameRange.lock()->getValue(0);
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
    } else if (_imp->preRender.lock().get() == k) {
        AppInstance::RenderWork w;
        w.writer = this;
        w.firstFrame = INT_MIN;
        w.lastFrame = INT_MAX;
        w.frameStep = 1;
        w.useRenderStats = false;
        std::list<AppInstance::RenderWork> works;
        works.push_back(w);
        getApp()->startWritersRendering(false, works);
    } else {
        ret = false;
    }

    return ret;
}

void
DiskCacheNode::getFrameRange(double *first,
                             double *last)
{
    int idx = _imp->frameRange.lock()->getValue();

    switch (idx) {
    case 0: {
        EffectInstancePtr input = getInput(0);
        if (input) {
            input->getFrameRange_public(input->getHash(), first, last);
        }
        break;
    }
    case 1: {
        getApp()->getFrameRange(first, last);
        break;
    }
    case 2: {
        *first = _imp->firstFrame.lock()->getValue();
        *last = _imp->lastFrame.lock()->getValue();
    };
    default:
        break;
    }
}

StatusEnum
DiskCacheNode::render(const RenderActionArgs& args)
{
    assert(args.outputPlanes.size() == 1);

    EffectInstancePtr input = getInput(0);
    if (!input) {
        return eStatusFailed;
    }


    const std::pair<ImagePlaneDesc, ImagePtr>& output = args.outputPlanes.front();

    for (std::list<std::pair<ImagePlaneDesc, boost::shared_ptr<Image> > >::const_iterator it = args.outputPlanes.begin(); it != args.outputPlanes.end(); ++it) {
        RectI roiPixel;
        ImagePtr srcImg = getImage(0, args.time, args.originalScale, args.view, NULL, &it->first, false /*mapToClipPrefs*/, true /*dontUpscale*/, eStorageModeRAM /*useOpenGL*/, 0 /*textureDepth*/,  &roiPixel);
        if (!srcImg) {
            return eStatusFailed;
        }
        if ( srcImg->getMipMapLevel() != output.second->getMipMapLevel() ) {
            throw std::runtime_error("Host gave image with wrong scale");
        }
        if ( ( srcImg->getComponents() != output.second->getComponents() ) || ( srcImg->getBitDepth() != output.second->getBitDepth() ) ) {
            srcImg->convertToFormat( args.roi, getApp()->getDefaultColorSpaceForBitDepth( srcImg->getBitDepth() ),
                                     getApp()->getDefaultColorSpaceForBitDepth( output.second->getBitDepth() ), 3, true, false, output.second.get() );
        } else {
            output.second->pasteFrom( *srcImg, args.roi, output.second->usesBitMap() && srcImg->usesBitMap() );
        }
    }

    return eStatusOK;
}

bool
DiskCacheNode::isHostChannelSelectorSupported(bool* /*defaultR*/,
                                              bool* /*defaultG*/,
                                              bool* /*defaultB*/,
                                              bool* /*defaultA*/) const
{
    return false;
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING

#include "moc_DiskCacheNode.cpp"

