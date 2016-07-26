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

#include "ViewerNode.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/MemoryFile.h"
#include "Engine/Node.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"

#define kViewerNodeParamLayers "layer"
#define kViewerNodeParamLayersLabel "Layer"
#define kViewerNodeParamLayersHint "The layer that the Viewer node will fetch upstream in the tree. " \
"The channels of the layer will be mapped to the RGBA channels of the viewer according to " \
"its number of channels. (e.g: UV would be mapped to RG)"

#define kViewerNodeParamAlphaChannel "alphaChannel"
#define kViewerNodeParamAlphaChannelLabel "Alpha Channel"
#define kViewerNodeParamAlphaChannelHint "Select here a channel of any layer that will be used when displaying the " \
"alpha channel with the Channels choice on the right"

#define kViewerNodeParamDisplayChannels "displayChannels"
#define kViewerNodeParamDisplayChannelsLabel "Display Channels"
#define kViewerNodeParamDisplayChannelsHint "The channels to display on the viewer from the selected layer"

#define kViewerNodeParamZoom "zoom"
#define kViewerNodeParamZoomLabel "Zoom"
#define kViewerNodeParamZoomHint "The zoom applied to the image on the viewer"

#define kViewerNodeParamSyncViewports "syncViewports"
#define kViewerNodeParamSyncViewportsLabel "Sync Viewports"
#define kViewerNodeParamSyncViewportsHint "When enabled, all viewers will be synchronized to the same portion of the image in the viewport"

#define kViewerNodeParamFitViewport "fitViewport"
#define kViewerNodeParamFitViewportLabel "Fit Viewport"
#define kViewerNodeParamFitViewportHint "Scales the image so it doesn't exceed the size of the viewport and centers it"

#define kViewerNodeParamClipToProject "clipToProject"
#define kViewerNodeParamClipToProjectLabel "Clip To Project"
#define kViewerNodeParamClipToProjectHint "Clips the portion of the image displayed " \
"on the viewer to the project format. " \
"When off, everything in the union of all nodes " \
"region of definition is displayed"

#define kViewerNodeParamFullFrame "fullFrame"
#define kViewerNodeParamFullFrameLabel "Full Frame"
#define kViewerNodeParamFullFrameHint "When checked, the viewer will render the image in its entirety (at full resolution) not just the visible portion. This may be useful when panning/zooming during playback"

#define kViewerNodeParamEnableUserRoI "enableRegionOfInterest"
#define kViewerNodeParamEnableUserRoILabel "Region Of Interest"
#define kViewerNodeParamEnableUserRoIHint "When active, enables the region of interest that limits " \
"the portion of the viewer that is kept updated. Press %2 to create and drag a new region."

#define kViewerNodeParamUserRoI "userRoI"

#define kViewerNodeParamEnableProxyMode "proxyMode"
#define kViewerNodeParamEnableProxyModeLabel "Proxy Mode"
#define kViewerNodeParamEnableProxyModeHint "Activates the downscaling by the amount indicated by the value on the right. " \
"The rendered images are degraded and as a result of this the whole rendering pipeline is much faster"

#define kViewerNodeParamProxyLevel "proxyLevel"
#define kViewerNodeParamProxyLevelLabel "Proxy Level"
#define kViewerNodeParamProxyLevelHint "When proxy mode is activated, it scales down the rendered image by this factor " \
"to accelerate the rendering"

#define kViewerNodeParamRefreshViewport "refreshViewport"
#define kViewerNodeParamRefreshViewportLabel "Refresh Viewport"
#define kViewerNodeParamRefreshViewportHint "Forces a new render of the current frame. Press %2 to activate in-depth render statistics useful " \
"for debugging the composition"

#define kViewerNodeParamPauseRender "pauseUpdates"
#define kViewerNodeParamPauseRenderLabel "Pause Updates"
#define kViewerNodeParamPauseRenderHint "When activated the viewer will not update after any change that would modify the image " \
"displayed in the viewport. Use %2 to pause both input A and B"

#define kViewerNodeParamAInput "aInput"
#define kViewerNodeParamAInputLabel "A"
#define kViewerNodeParamAInputHint "What node to display in the viewer input A"

#define kViewerNodeParamBInput "bInput"
#define kViewerNodeParamBInputLabel "B"
#define kViewerNodeParamBInputHint "What node to display in the viewer input B"

#define kViewerNodeParamOperation "operation"
#define kViewerNodeParamOperationLabel "Operation"
#define kViewerNodeParamOperationHint "Operation applied between viewer inputs A and B. a and b are the alpha components of each input. d is the wipe dissolve factor, controlled by the arc handle"

#define kViewerNodeParamOperationWipeUnder "Wipe Under"
#define kViewerNodeParamOperationWipeUnderHint "A(1 - d) + Bd"

#define kViewerNodeParamOperationWipeOver "Wipe Over"
#define kViewerNodeParamOperationWipeOverHint "A + B(1 - a)d"

#define kViewerNodeParamOperationWipeMinus "Wipe Minus"
#define kViewerNodeParamOperationWipeMinusHint "A - B"

#define kViewerNodeParamOperationWipeOnionSkin "Wipe Onion skin"
#define kViewerNodeParamOperationWipeOnionSkinHint "A + B"

#define kViewerNodeParamOperationStackUnder "Stack Under"
#define kViewerNodeParamOperationStackUnderHint "B"

#define kViewerNodeParamOperationStackOver "Stack Over"
#define kViewerNodeParamOperationStackOverHint "A + B(1 - a)"

#define kViewerNodeParamOperationStackMinus "Stack Minus"
#define kViewerNodeParamOperationStackMinusHint "A - B"

#define kViewerNodeParamOperationStackOnionSkin "Stack Onion skin"
#define kViewerNodeParamOperationStackOnionSkinHint "A + B"

#define kViewerNodeParamEnableGain "enableGain"
#define kViewerNodeParamEnableGainLabel "Enable Gain"
#define kViewerNodeParamEnableGainHint "Switch between \"neutral\" 1.0 gain f-stop and the previous setting"

#define kViewerNodeParamGain "gain"
#define kViewerNodeParamGainLabel "Gain"
#define kViewerNodeParamGainHint "Gain is shown as f-stops. The image is multipled by pow(2,value) before display"

#define kViewerNodeParamEnableAutoContrast "autoContrast"
#define kViewerNodeParamEnableAutoContrastLabel "Auto Contrast"
#define kViewerNodeParamEnableAutoContrastHint "Automatically adjusts the gain and the offset applied " \
"to the colors of the visible image portion on the viewer"

#define kViewerNodeParamEnableGamma "enableGamma"
#define kViewerNodeParamEnableGammaLabel "Enable Gamma"
#define kViewerNodeParamEnableGammaHint "Gamma correction: Switch between gamma=1.0 and user setting"

#define kViewerNodeParamGamma "gamma"
#define kViewerNodeParamGammaLabel "Gamma"
#define kViewerNodeParamGammaHint "Viewer gamma correction level (applied after gain and before colorspace correction)"

#define kViewerNodeParamCheckerBoard "enableCheckerBoard"
#define kViewerNodeParamCheckerBoardLabel "Enable Checkerboard"
#define kViewerNodeParamCheckerBoardHint "If checked, the viewer draws a checkerboard under input A instead of black (disabled under the wipe area and in stack modes)"

#define kViewerNodeParamColorspace "deviceColorspace"
#define kViewerNodeParamColorspaceLabel "Device Colorspace"
#define kViewerNodeParamColorspaceHint "The operation applied to the image before it is displayed " \
"on screen. The image is converted to this colorspace before being displayed on the monitor"

#define kViewerNodeParamView "activeView"
#define kViewerNodeParamViewLabel "Active View"
#define kViewerNodeParamViewHint "The view displayed on the viewer"

#define kViewerNodeParamEnableColorPicker "enableInfoBar"
#define kViewerNodeParamEnableColorPickerLabel "Show Info Bar"
#define kViewerNodeParamEnableColorPickerHint "Show/Hide information bar in the bottom of the viewer. If unchecked it also deactivates any active color picker"

#define VIEWER_UI_SECTIONS_SPACING_PX 5

NATRON_NAMESPACE_ENTER;

struct ViewerNodePrivate
{
    ViewerNode* _publicInterface;

    // Pointer to ViewerGL (interface)
    OpenGLViewerI* uiContext;

    boost::weak_ptr<KnobChoice> layersKnob;
    boost::weak_ptr<KnobChoice> alphaChannelKnob;
    boost::weak_ptr<KnobChoice> displayChannelsKnob;
    boost::weak_ptr<KnobChoice> zoomChoiceKnob;
    boost::weak_ptr<KnobButton> syncViewersButtonKnob;
    boost::weak_ptr<KnobButton> centerViewerButtonKnob;
    boost::weak_ptr<KnobButton> clipToProjectButtonKnob;
    boost::weak_ptr<KnobButton> fullFrameButtonKnob;
    boost::weak_ptr<KnobButton> toggleUserRoIButtonKnob;
    boost::weak_ptr<KnobDouble> userRoIKnob;
    boost::weak_ptr<KnobButton> toggleProxyModeButtonKnob;
    boost::weak_ptr<KnobChoice> proxyChoiceKnob;
    boost::weak_ptr<KnobButton> refreshButtonKnob;
    boost::weak_ptr<KnobButton> pauseButtonKnob;
    boost::weak_ptr<KnobChoice> aInputNodeChoiceKnob;
    boost::weak_ptr<KnobChoice> blendingModeChoiceKnob;
    boost::weak_ptr<KnobChoice> bInputNodeChoiceKnob;

    boost::weak_ptr<KnobButton> enableGainButtonKnob;
    boost::weak_ptr<KnobDouble> gainSliderKnob;
    boost::weak_ptr<KnobButton> enableAutoContrastButtonKnob;
    boost::weak_ptr<KnobButton> enableGammaButtonKnob;
    boost::weak_ptr<KnobDouble> gammaSliderKnob;
    boost::weak_ptr<KnobButton> enableCheckerboardButtonKnob;
    boost::weak_ptr<KnobChoice> colorspaceKnob;
    boost::weak_ptr<KnobChoice> activeViewKnob;
    boost::weak_ptr<KnobButton> enableInfoBarButtonKnob;

    ViewerNodePrivate(ViewerNode* publicInterface)
    : _publicInterface(publicInterface)
    , uiContext(0)
    {

    }
};

EffectInstancePtr
ViewerNode::create(const NodePtr& node)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return EffectInstancePtr( new ViewerNode(node) );
}

ViewerNode::ViewerNode(const NodePtr& node)
    : NodeGroup(node)
    , _imp( new ViewerNodePrivate(this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setSupportsRenderScaleMaybe(EffectInstance::eSupportsYes);

    QObject::connect( this, SIGNAL(disconnectTextureRequest(int,bool)), this, SLOT(executeDisconnectTextureRequestOnMainThread(int,bool)) );
    QObject::connect( this, SIGNAL(s_callRedrawOnMainThread()), this, SLOT(redrawViewer()) );
}

ViewerNode::~ViewerNode()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );


    // If _imp->updateViewerRunning is true, that means that the next updateViewer call was
    // not yet processed. Since we're in the main thread and it is processed in the main thread,
    // there is no way to wait for it (locking the mutex would cause a deadlock).
    // We don't care, after all.
    //{
    //    QMutexLocker locker(&_imp->updateViewerMutex);
    //    assert(!_imp->updateViewerRunning);
    //}
    if (_imp->uiContext) {
        _imp->uiContext->removeGUI();
    }
}

void
ViewerNode::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_IMAGE);
}

void
ViewerNode::initializeKnobs()
{

    KnobPagePtr page = AppManager::createKnob<KnobPage>( shared_from_this(), tr("Controls") );

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamLayersLabel) );
        param->setName(kViewerNodeParamLayers);
        param->setHintToolTip(tr(kViewerNodeParamLayersHint));
        param->setSecretByDefault(true);
        {
            std::vector<std::string> entries;
            entries.push_back("-");
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->layersKnob = param;
    }
    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamAlphaChannelLabel) );
        param->setName(kViewerNodeParamAlphaChannel);
        param->setHintToolTip(tr(kViewerNodeParamAlphaChannelHint));
        param->setSecretByDefault(true);
        {
            std::vector<std::string> entries;
            entries.push_back("-");
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->alphaChannelKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamDisplayChannelsLabel) );
        param->setName(kViewerNodeParamDisplayChannels);
        param->setHintToolTip(tr(kViewerNodeParamDisplayChannelsHint));
        param->setSecretByDefault(true);
        {
            std::vector<std::string> entries;
            entries.push_back("Luminance");
            entries.push_back("RGB");
            entries.push_back("Red");
            entries.push_back("Green");
            entries.push_back("Blue");
            entries.push_back("Alpha");
            entries.push_back("Matte");
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->displayChannelsKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamZoomLabel) );
        param->setName(kViewerNodeParamZoom);
        param->setHintToolTip(tr(kViewerNodeParamZoomHint));
        param->setSecretByDefault(true);
        {
            std::vector<std::string> entries;
            entries.push_back("Fit");
            entries.push_back("+");
            entries.push_back("-");
            entries.push_back(KnobChoice::getSeparatorOption());
            entries.push_back("10%");
            entries.push_back("25%");
            entries.push_back("50%");
            entries.push_back("75%");
            entries.push_back("100%");
            entries.push_back("125%");
            entries.push_back("150%");
            entries.push_back("200%");
            entries.push_back("400%");
            entries.push_back("800%");
            entries.push_back("1600%");
            entries.push_back("2400%");
            entries.push_back("3200%");
            entries.push_back("6400%");
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->zoomChoiceKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamSyncViewportsLabel) );
        param->setName(kViewerNodeParamSyncViewports);
        param->setHintToolTip(tr(kViewerNodeParamSyncViewportsHint));
        param->setInViewerContextCanHaveShortcut(true);
        param->setSecretByDefault(true);
        param->setCheckable(true);
        param->setEvaluateOnChange(false);
        param->setIconLabel(NATRON_IMAGES_PATH "locked.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "unlocked.png", false);
        page->addKnob(param);
        _imp->syncViewersButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamFitViewportLabel) );
        param->setName(kViewerNodeParamFitViewport);
        param->setHintToolTip(tr(kViewerNodeParamFitViewportHint));
        param->setInViewerContextCanHaveShortcut(true);
        param->setSecretByDefault(true);
        param->setIconLabel(NATRON_IMAGES_PATH "centerViewer.png", true);
        page->addKnob(param);
        _imp->centerViewerButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamClipToProjectLabel) );
        param->setName(kViewerNodeParamClipToProject);
        param->setHintToolTip(tr(kViewerNodeParamClipToProjectHint));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "cliptoprojectEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "cliptoprojectDisable.png", false);
        page->addKnob(param);
        _imp->clipToProjectButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamFullFrameLabel) );
        param->setName(kViewerNodeParamFullFrame);
        param->setHintToolTip(tr(kViewerNodeParamFullFrameHint));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "fullFrameOn.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "fullFrameOff.png", false);
        page->addKnob(param);
        _imp->fullFrameButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamEnableUserRoILabel) );
        param->setName(kViewerNodeParamEnableUserRoI);
        param->setHintToolTip(tr(kViewerNodeParamEnableUserRoIHint));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "viewer_roiEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "viewer_roiDisabled.png", false);
        page->addKnob(param);
        _imp->toggleUserRoIButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamEnableProxyModeLabel) );
        param->setName(kViewerNodeParamEnableProxyMode);
        param->setHintToolTip(tr(kViewerNodeParamEnableProxyMode));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "renderScale_checked.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "renderScale.png", false);
        page->addKnob(param);
        _imp->toggleProxyModeButtonKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamProxyLevelLabel) );
        param->setName(kViewerNodeParamProxyLevel);
        param->setHintToolTip(tr(kViewerNodeParamProxyLevelHint));
        param->setSecretByDefault(true);
        {
            std::vector<std::string> entries;
            entries.push_back("2");
            entries.push_back("4");
            entries.push_back("8");
            entries.push_back("16");
            entries.push_back("32");
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->proxyChoiceKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamRefreshViewportLabel) );
        param->setName(kViewerNodeParamRefreshViewport);
        param->setHintToolTip(tr(kViewerNodeParamRefreshViewportHint));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "refreshActive.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "refresh.png", false);
        page->addKnob(param);
        _imp->refreshButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamPauseRenderLabel) );
        param->setName(kViewerNodeParamPauseRender);
        param->setHintToolTip(tr(kViewerNodeParamPauseRenderHint));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "pauseEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "pauseDisabled.png", false);
        page->addKnob(param);
        _imp->pauseButtonKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamAInputLabel) );
        param->setName(kViewerNodeParamAInput);
        param->setHintToolTip(tr(kViewerNodeParamAInputHint));
        param->setSecretByDefault(true);
        {
            std::vector<std::string> entries;
            entries.push_back("-");
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->aInputNodeChoiceKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamOperationLabel) );
        param->setName(kViewerNodeParamOperation);
        param->setHintToolTip(tr(kViewerNodeParamOperation));
        param->setSecretByDefault(true);
        param->setIconLabel(NATRON_IMAGES_PATH "roto_merge.png");
        {
            std::vector<std::string> entries, helps;
            entries.push_back("-");
            helps.push_back("");
            entries.push_back(kViewerNodeParamOperationWipeUnder);
            helps.push_back(kViewerNodeParamOperationWipeUnderHint);
            entries.push_back(kViewerNodeParamOperationWipeOver);
            helps.push_back(kViewerNodeParamOperationWipeOverHint);
            entries.push_back(kViewerNodeParamOperationWipeMinus);
            helps.push_back(kViewerNodeParamOperationWipeMinusHint);
            entries.push_back(kViewerNodeParamOperationWipeOnionSkin);
            helps.push_back(kViewerNodeParamOperationWipeOnionSkinHint);

            entries.push_back(kViewerNodeParamOperationStackUnder);
            helps.push_back(kViewerNodeParamOperationStackUnderHint);
            entries.push_back(kViewerNodeParamOperationStackOver);
            helps.push_back(kViewerNodeParamOperationStackOverHint);
            entries.push_back(kViewerNodeParamOperationStackMinus);
            helps.push_back(kViewerNodeParamOperationStackMinusHint);
            entries.push_back(kViewerNodeParamOperationStackOnionSkin);
            helps.push_back(kViewerNodeParamOperationStackOnionSkinHint);
            param->populateChoices(entries, helps);
        }
        page->addKnob(param);
        _imp->blendingModeChoiceKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamBInputLabel) );
        param->setName(kViewerNodeParamBInput);
        param->setHintToolTip(tr(kViewerNodeParamBInputHint));
        param->setSecretByDefault(true);
        {
            std::vector<std::string> entries;
            entries.push_back("-");
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->bInputNodeChoiceKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamEnableGainLabel) );
        param->setName(kViewerNodeParamEnableGain);
        param->setHintToolTip(tr(kViewerNodeParamEnableGainHint));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "expoON.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "expoOFF.png", false);
        page->addKnob(param);
        _imp->enableGainButtonKnob = param;
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>( shared_from_this(), tr(kViewerNodeParamGainLabel), 1 );
        param->setName(kViewerNodeParamGain);
        param->setHintToolTip(tr(kViewerNodeParamGainHint));
        param->setSecretByDefault(true);
        param->setDisplayMinimum(-6.);
        param->setDisplayMaximum(6.);
        page->addKnob(param);
        _imp->enableGainButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamEnableAutoContrastLabel) );
        param->setName(kViewerNodeParamEnableAutoContrast);
        param->setHintToolTip(tr(kViewerNodeParamEnableAutoContrastHint));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "AutoContrastON.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "AutoContrast.png", false);
        page->addKnob(param);
        _imp->enableAutoContrastButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamEnableGammaLabel) );
        param->setName(kViewerNodeParamEnableGamma);
        param->setHintToolTip(tr(kViewerNodeParamEnableGammaHint));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "gammaON.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "gammaOFF.png", false);
        page->addKnob(param);
        _imp->enableGammaButtonKnob = param;
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>( shared_from_this(), tr(kViewerNodeParamGammaLabel), 1 );
        param->setName(kViewerNodeParamGamma);
        param->setHintToolTip(tr(kViewerNodeParamGammaHint));
        param->setSecretByDefault(true);
        param->setDisplayMinimum(0.);
        param->setDisplayMaximum(4.);
        param->setDefaultValue(1.);
        page->addKnob(param);
        _imp->gammaSliderKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamColorspaceLabel) );
        param->setName(kViewerNodeParamColorspace);
        param->setHintToolTip(tr(kViewerNodeParamColorspaceHint));
        param->setSecretByDefault(true);
        {
            std::vector<std::string> entries;
            entries.push_back("Linear(None)");
            entries.push_back("sRGB");
            entries.push_back("Rec.709");
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->colorspaceKnob = param;
    }
    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamCheckerBoardLabel) );
        param->setName(kViewerNodeParamCheckerBoard);
        param->setHintToolTip(tr(kViewerNodeParamCheckerBoardHint));
        param->setSecretByDefault(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "checkerboard_on.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "checkerboard_off.png", false);
        page->addKnob(param);
        _imp->enableCheckerboardButtonKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( shared_from_this(), tr(kViewerNodeParamViewLabel) );
        param->setName(kViewerNodeParamView);
        param->setHintToolTip(tr(kViewerNodeParamViewHint));
        param->setSecretByDefault(true);
        {
            // Views gets populated in getPreferredMetadata
            std::vector<std::string> entries;
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->activeViewKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamEnableColorPickerLabel) );
        param->setName(kViewerNodeParamEnableColorPicker);
        param->setHintToolTip(tr(kViewerNodeParamEnableColorPickerHint));
        param->setSecretByDefault(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "color_picker.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "color_picker.png", false);
        page->addKnob(param);
        _imp->enableInfoBarButtonKnob = param;
    }

    // RotoPaint
    addKnobToViewerUI(_imp->layersKnob.lock());
    addKnobToViewerUI(_imp->alphaChannelKnob.lock());
    addKnobToViewerUI(_imp->displayChannelsKnob.lock());
    _imp->displayChannelsKnob.lock()->setInViewerContextAddSeparator(true);
    addKnobToViewerUI(_imp->zoomChoiceKnob.lock());
    addKnobToViewerUI(_imp->syncViewersButtonKnob.lock());
    addKnobToViewerUI(_imp->centerViewerButtonKnob.lock());
    _imp->centerViewerButtonKnob.lock()->setInViewerContextAddSeparator(true);
    addKnobToViewerUI(_imp->clipToProjectButtonKnob.lock());
    addKnobToViewerUI(_imp->fullFrameButtonKnob.lock());
    addKnobToViewerUI(_imp->toggleUserRoIButtonKnob.lock());
    addKnobToViewerUI(_imp->toggleProxyModeButtonKnob.lock());
    addKnobToViewerUI(_imp->proxyChoiceKnob.lock());
    _imp->proxyChoiceKnob.lock()->setInViewerContextAddSeparator(true);
    addKnobToViewerUI(_imp->refreshButtonKnob.lock());
    addKnobToViewerUI(_imp->pauseButtonKnob.lock());
    _imp->pauseButtonKnob.lock()->setInViewerContextAddSeparator(true);
    addKnobToViewerUI(_imp->aInputNodeChoiceKnob.lock());
    addKnobToViewerUI(_imp->blendingModeChoiceKnob.lock());
    addKnobToViewerUI(_imp->bInputNodeChoiceKnob.lock());
    _imp->bInputNodeChoiceKnob.lock()->setInViewerContextNewLineActivated(true);
    addKnobToViewerUI(_imp->enableGainButtonKnob.lock());
    addKnobToViewerUI(_imp->gainSliderKnob.lock());
    addKnobToViewerUI(_imp->enableAutoContrastButtonKnob.lock());
    addKnobToViewerUI(_imp->enableGammaButtonKnob.lock());
    addKnobToViewerUI(_imp->gammaSliderKnob.lock());
    addKnobToViewerUI(_imp->colorspaceKnob.lock());
    addKnobToViewerUI(_imp->enableCheckerboardButtonKnob.lock());
    addKnobToViewerUI(_imp->activeViewKnob.lock());

    // Add stretch here
    addKnobToViewerUI(_imp->enableInfoBarButtonKnob.lock());


} // initializeKnobs

bool
ViewerNode::knobChanged(const KnobIPtr& k, ValueChangedReasonEnum reason,
                        ViewSpec /*view*/,
                        double /*time*/,
                        bool /*originatedFromMainThread*/)
{

} // knobChanged

OpenGLViewerI*
ViewerNode::getUiContext() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->uiContext;
}

void
ViewerNode::setUiContext(OpenGLViewerI* viewer)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->uiContext = viewer;
}


void
ViewerNode::forceFullComputationOnNextFrame()
{
    // this is called by the GUI when the user presses the "Refresh" button.
    // It set the flag forceRender to true, meaning no cache will be used.

    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

}

void
ViewerNode::clearLastRenderedImage()
{
    NodeGroup::clearLastRenderedImage();

    if (_imp->uiContext) {
        _imp->uiContext->clearLastRenderedImage();
    }

}

void
ViewerNode::invalidateUiContext()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->uiContext = NULL;
}


void
ViewerNode::executeDisconnectTextureRequestOnMainThread(int index,bool clearRoD)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->uiContext) {
        _imp->uiContext->disconnectInputTexture(index, clearRoD);
    }
}
void
ViewerNode::aboutToUpdateTextures()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->uiContext->clearPartialUpdateTextures();
}

bool
ViewerNode::isViewerUIVisible() const
{
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->uiContext ? _imp->uiContext->isViewerUIVisible() : false;
}

void
ViewerInstance::onGammaChanged(double value)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);

        if (_imp->viewerParamsGamma != value) {
            _imp->viewerParamsGamma = value;
            changed = true;
        }
    }
    if (changed) {
        QWriteLocker k(&_imp->gammaLookupMutex);
        _imp->fillGammaLut(1. / value);
    }
    assert(_imp->uiContext);
    if (changed) {
        if ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte)
             && !getApp()->getProject()->isLoadingProject() ) {
            renderCurrentFrame(true);
        } else {
            _imp->uiContext->redraw();
        }
    }
}

double
ViewerInstance::getGamma() const
{
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsGamma;
}

void
ViewerInstance::onGainChanged(double exp)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsGain != exp) {
            _imp->viewerParamsGain = exp;
            changed = true;
        }
    }
    if (changed) {
        assert(_imp->uiContext);
        if ( ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte) )
             && !getApp()->getProject()->isLoadingProject() ) {
            renderCurrentFrame(true);
        } else {
            _imp->uiContext->redraw();
        }
    }
}

unsigned int
ViewerInstance::getViewerMipMapLevel() const
{
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerMipMapLevel;
}

void
ViewerInstance::onMipMapLevelChanged(int level)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerMipMapLevel == (unsigned int)level) {
            return;
        }
        _imp->viewerMipMapLevel = level;
    }
}

void
ViewerInstance::onAutoContrastChanged(bool autoContrast,
                                      bool refresh)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsAutoContrast != autoContrast) {
            _imp->viewerParamsAutoContrast = autoContrast;
            changed = true;
        }
    }
    if ( changed && refresh && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

bool
ViewerInstance::isAutoContrastEnabled() const
{
    // MT-safe
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsAutoContrast;
}

void
ViewerInstance::onColorSpaceChanged(ViewerColorSpaceEnum colorspace)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsLut == colorspace) {
            return;
        }
        _imp->viewerParamsLut = colorspace;
    }
    assert(_imp->uiContext);
    if ( ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte) )
         && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    } else {
        _imp->uiContext->redraw();
    }
}

void
ViewerInstance::setDisplayChannels(DisplayChannelsEnum channels,
                                   bool bothInputs)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (!bothInputs) {
            if (_imp->viewerParamsChannels[0] != channels) {
                _imp->viewerParamsChannels[0] = channels;
                changed = true;
            }
        } else {
            if (_imp->viewerParamsChannels[0] != channels) {
                _imp->viewerParamsChannels[0] = channels;
                changed = true;
            }
            if (_imp->viewerParamsChannels[1] != channels) {
                _imp->viewerParamsChannels[1] = channels;
                changed = true;
            }
        }
    }
    if ( changed && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::setActiveLayer(const ImageComponents& layer,
                               bool doRender)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsLayer != layer) {
            _imp->viewerParamsLayer = layer;
            changed = true;
        }
    }
    if ( doRender && changed && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::setAlphaChannel(const ImageComponents& layer,
                                const std::string& channelName,
                                bool doRender)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsAlphaLayer != layer) {
            _imp->viewerParamsAlphaLayer = layer;
            changed = true;
        }
        if (_imp->viewerParamsAlphaChannelName != channelName) {
            _imp->viewerParamsAlphaChannelName = channelName;
            changed = true;
        }
    }
    if ( changed && doRender && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::disconnectViewer()
{
    // always running in the render thread
    if (_imp->uiContext) {
        Q_EMIT viewerDisconnected();
    }
}

void
ViewerInstance::disconnectTexture(int index,bool clearRod)
{
    if (_imp->uiContext) {
        Q_EMIT disconnectTextureRequest(index, clearRod);
    }
}

void
ViewerInstance::redrawViewer()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (!_imp->uiContext) {
        return;
    }
    _imp->uiContext->redraw();
}

void
ViewerInstance::redrawViewerNow()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(_imp->uiContext);
    _imp->uiContext->redrawNow();
}

int
ViewerInstance::getLutType() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsLut;
}

double
ViewerInstance::getGain() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsGain;
}

int
ViewerInstance::getMipMapLevel() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerMipMapLevel;
}

DisplayChannelsEnum
ViewerInstance::getChannels(int texIndex) const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsChannels[texIndex];
}

void
ViewerInstance::setFullFrameProcessingEnabled(bool fullFrame)
{
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->fullFrameProcessingEnabled == fullFrame) {
            return;
        }
        _imp->fullFrameProcessingEnabled = fullFrame;
    }
    if ( !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

bool
ViewerInstance::isFullFrameProcessingEnabled() const
{
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->fullFrameProcessingEnabled;
}

void
ViewerInstance::addAcceptedComponents(int /*inputNb*/,
                                      std::list<ImageComponents>* comps)
{
    ///Viewer only supports RGBA for now.
    comps->push_back( ImageComponents::getRGBAComponents() );
    comps->push_back( ImageComponents::getRGBComponents() );
    comps->push_back( ImageComponents::getAlphaComponents() );
}

ViewIdx
ViewerInstance::getViewerCurrentView() const
{
    return _imp->uiContext ? _imp->uiContext->getCurrentView() : ViewIdx(0);
}

void
ViewerInstance::setActivateInputChangeRequestedFromViewer(bool fromViewer)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->activateInputChangedFromViewer = fromViewer;
}

bool
ViewerInstance::isInputChangeRequestedFromViewer() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->activateInputChangedFromViewer;
}

void
ViewerInstance::setViewerPaused(bool paused,
                                bool allInputs)
{
    QMutexLocker k(&_imp->isViewerPausedMutex);

    _imp->isViewerPaused[0] = paused;
    if (allInputs) {
        _imp->isViewerPaused[1] = paused;
    }
}

bool
ViewerInstance::isViewerPaused(int texIndex) const
{
    QMutexLocker k(&_imp->isViewerPausedMutex);

    return _imp->isViewerPaused[texIndex];
}

void
ViewerInstance::onInputChanged(int /*inputNb*/)
{
    Q_EMIT clipPreferencesChanged();
}

void
ViewerInstance::onMetaDatasRefreshed(const NodeMetadata& /*metadata*/)
{
    Q_EMIT clipPreferencesChanged();
}

void
ViewerInstance::onChannelsSelectorRefreshed()
{
    Q_EMIT availableComponentsChanged();
}

void
ViewerInstance::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthByte);
}

void
ViewerInstance::getActiveInputs(int & a,
                                int &b) const
{
    NodePtr n = getNode();
    InspectorNodePtr isInspector = toInspectorNode(n);

    assert(isInspector);
    if (isInspector) {
        isInspector->getActiveInputs(a, b);
    }
}

void
ViewerInstance::setInputA(int inputNb)
{
    NodePtr n = getNode();
    InspectorNodePtr isInspector = toInspectorNode(n);

    assert(isInspector);
    if (isInspector) {
        isInspector->setInputA(inputNb);
    }

    Q_EMIT availableComponentsChanged();
}

void
ViewerInstance::setInputB(int inputNb)
{
    NodePtr n = getNode();
    InspectorNodePtr isInspector = toInspectorNode(n);

    assert(isInspector);
    if (isInspector) {
        isInspector->setInputB(inputNb);
    }

    Q_EMIT availableComponentsChanged();
}

int
ViewerInstance::getLastRenderedTime() const
{
    return _imp->uiContext ? _imp->uiContext->getCurrentlyDisplayedTime() : getApp()->getTimeLine()->currentFrame();
}

TimeLinePtr
ViewerInstance::getTimeline() const
{
    return _imp->uiContext ? _imp->uiContext->getTimeline() : getApp()->getTimeLine();
}

void
ViewerInstance::getTimelineBounds(int* first,
                                  int* last) const
{
    if (_imp->uiContext) {
        _imp->uiContext->getViewerFrameRange(first, last);
    } else {
        *first = 0;
        *last = 0;
    }
}

int
ViewerInstance::getMipMapLevelFromZoomFactor() const
{
    double zoomFactor = _imp->uiContext->getZoomFactor();
    double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) );

    return std::log(closestPowerOf2) / M_LN2;
}

double
ViewerInstance::getCurrentTime() const
{
    return getFrameRenderArgsCurrentTime();
}

ViewIdx
ViewerInstance::getCurrentView() const
{
    return getFrameRenderArgsCurrentView();
}

bool
ViewerInstance::isLatestRender(int textureIndex,
                               U64 renderAge) const
{
    return _imp->isLatestRender(textureIndex, renderAge);
}

void
ViewerInstance::setPartialUpdateParams(const std::list<RectD>& rois,
                                       bool recenterViewer)
{
    double viewerCenterX = 0;
    double viewerCenterY = 0;

    if (recenterViewer) {
        RectD bbox;
        bool bboxSet = false;
        for (std::list<RectD>::const_iterator it = rois.begin(); it != rois.end(); ++it) {
            if (!bboxSet) {
                bboxSet = true;
                bbox = *it;
            } else {
                bbox.merge(*it);
            }
        }
        viewerCenterX = (bbox.x1 + bbox.x2) / 2.;
        viewerCenterY = (bbox.y1 + bbox.y2) / 2.;
    }
    QMutexLocker k(&_imp->viewerParamsMutex);
    _imp->partialUpdateRects = rois;
    _imp->viewportCenterSet = recenterViewer;
    _imp->viewportCenter.x = viewerCenterX;
    _imp->viewportCenter.y = viewerCenterY;
}

void
ViewerInstance::clearPartialUpdateParams()
{
    QMutexLocker k(&_imp->viewerParamsMutex);

    _imp->partialUpdateRects.clear();
    _imp->viewportCenterSet = false;
}

void
ViewerInstance::setDoingPartialUpdates(bool doing)
{
    QMutexLocker k(&_imp->viewerParamsMutex);

    _imp->isDoingPartialUpdates = doing;
}

bool
ViewerInstance::isDoingPartialUpdates() const
{
    QMutexLocker k(&_imp->viewerParamsMutex);

    return _imp->isDoingPartialUpdates;
}

void
ViewerInstance::reportStats(int time,
                            ViewIdx view,
                            double wallTime,
                            const RenderStatsMap& stats)
{
    Q_EMIT renderStatsAvailable(time, view, wallTime, stats);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ViewerInstance.cpp"
#include "moc_ViewerInstancePrivate.cpp"
