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

#include "ViewerNodePrivate.h"

#include "Engine/RenderEngine.h"


#define VIEWER_UI_SECTIONS_SPACING_PX 5


NATRON_NAMESPACE_ENTER


/**
 * @brief Creates a duplicate of the knob identified by knobName which is a knob in the internalNode onto the effect and add it to the given page.
 **/
template <typename KNOBTYPE>
boost::shared_ptr<KNOBTYPE>
createDuplicateKnob( const std::string& knobName,
                    const NodePtr& internalNode,
                    const EffectInstancePtr& effect,
                    const KnobPagePtr& page = KnobPagePtr(),
                    const KnobGroupPtr& group = KnobGroupPtr() )
{
    KnobIPtr internalNodeKnob = internalNode->getKnobByName(knobName);
    KnobIPtr duplicateKnob = internalNodeKnob->createDuplicateOnHolder(effect, page, group, -1, KnobI::eDuplicateKnobTypeAlias, internalNodeKnob->getName(), internalNodeKnob->getLabel(), internalNodeKnob->getHintToolTip(), false /*refreshParamsGui*/, KnobI::eKnobDeclarationTypePlugin);
    return boost::dynamic_pointer_cast<KNOBTYPE>(duplicateKnob);
}


void
ViewerNode::abortAllViewersRendering()
{
    return _imp->abortAllViewersRendering();
}


void
ViewerNode::initializeKnobs()
{

    ViewerNodePtr thisShared = shared_from_this();



    KnobPagePtr page = createKnob<KnobPage>("viewerUIControls");
    page->setLabel(tr("UIControls"));
    page->setSecret(true);

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamLayers);
        param->setLabel(tr(kViewerNodeParamLayersLabel));
        param->setHintToolTip(tr(kViewerNodeParamLayersHint));
        param->setMissingEntryWarningEnabled(false);
        page->addKnob(param);
        param->setSecret(true);
        param->setTextToFitHorizontally("Color.Toto.RGBA");
        _imp->layersKnob = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamAlphaChannel);
        param->setLabel(tr(kViewerNodeParamAlphaChannelLabel));
        param->setHintToolTip(tr(kViewerNodeParamAlphaChannelHint));
        param->setMissingEntryWarningEnabled(false);
        page->addKnob(param);
        param->setSecret(true);
        param->setTextToFitHorizontally("Color.alpha");
        _imp->alphaChannelKnob = param;
    }

    std::vector<ChoiceOption> displayChannelEntries;

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamDisplayChannels);
        param->setLabel(tr(kViewerNodeParamDisplayChannelsLabel));
        param->setHintToolTip(tr(kViewerNodeParamDisplayChannelsHint));
        {

            displayChannelEntries.push_back(ChoiceOption("Luminance", "", ""));
            displayChannelEntries.push_back(ChoiceOption("RGB", "", ""));
            displayChannelEntries.push_back(ChoiceOption("Red", "", ""));
            displayChannelEntries.push_back(ChoiceOption("Green", "", ""));
            displayChannelEntries.push_back(ChoiceOption("Blue", "", ""));
            displayChannelEntries.push_back(ChoiceOption("Alpha", "", ""));
            displayChannelEntries.push_back(ChoiceOption("Matte", "", ""));
            param->populateChoices(displayChannelEntries);
        }
        {
            std::map<int, std::string> shortcuts;
            shortcuts[0] = kViewerNodeParamActionLuminance;
            shortcuts[2] = kViewerNodeParamActionRed;
            shortcuts[3] = kViewerNodeParamActionGreen;
            shortcuts[4] = kViewerNodeParamActionBlue;
            shortcuts[5] = kViewerNodeParamActionAlpha;
            shortcuts[6] = kViewerNodeParamActionMatte;
            param->setShortcuts(shortcuts);
        }
        param->setTextToFitHorizontally("Luminance");
        page->addKnob(param);
        param->setDefaultValue(1);
        {
            // Luminance
            RGBAColourD color = {0.398979, 0.398979, 0.398979, 1.};
            param->setColorForIndex(0, color);
        }
        {
            // Red
            RGBAColourD color = {0.851643, 0.196936, 0.196936, 1.};
            param->setColorForIndex(2, color);
        }
        {
            // Green
            RGBAColourD color = {0., 0.654707, 0., 1.};
            param->setColorForIndex(3, color);
        }
        {
            // Blue
            RGBAColourD color = {0.345293, 0.345293, 1., 1.};
            param->setColorForIndex(4, color);
        }
        {
            // Alpha
            RGBAColourD color = {1., 1., 1., 1.};
            param->setColorForIndex(5, color);
        }
        param->setSecret(true);
        _imp->displayChannelsKnob[0] = param;
    }
    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamDisplayChannelsB);
        param->setLabel(tr(kViewerNodeParamDisplayChannelsLabel));
        param->populateChoices(displayChannelEntries);

        param->setDefaultValue(1);
        page->addKnob(param);
        param->setSecret(true);
        _imp->displayChannelsKnob[1] = param;
    }


    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamZoom);
        param->setLabel(tr(kViewerNodeParamZoomLabel));
        param->setHintToolTip(tr(kViewerNodeParamZoomHint));
        param->setSecret(true);
        param->setMissingEntryWarningEnabled(false);
        param->setIsPersistent(false);
        {
            std::vector<ChoiceOption> entries;
            entries.push_back(ChoiceOption("Fit", "", ""));
            entries.push_back(ChoiceOption("+", "", ""));
            entries.push_back(ChoiceOption("-", "", ""));
            entries.push_back(ChoiceOption("10%", "", ""));
            entries.push_back(ChoiceOption("25%", "", ""));
            entries.push_back(ChoiceOption("50%", "", ""));
            entries.push_back(ChoiceOption("75%", "", ""));
            entries.push_back(ChoiceOption("100%", "", ""));
            entries.push_back(ChoiceOption("125%", "", ""));
            entries.push_back(ChoiceOption("150%", "", ""));
            entries.push_back(ChoiceOption("200%", "", ""));
            entries.push_back(ChoiceOption("400%", "", ""));
            entries.push_back(ChoiceOption("800%", "", ""));
            entries.push_back(ChoiceOption("1600%", "", ""));
            entries.push_back(ChoiceOption("2400%", "", ""));
            entries.push_back(ChoiceOption("3200%", "", ""));
            entries.push_back(ChoiceOption("6400%", "", ""));
            param->populateChoices(entries);
        }
        {
            std::map<int, std::string> shortcuts;
            shortcuts[0] = kViewerNodeParamFitViewport;
            shortcuts[1] = kViewerNodeParamActionZoomIn;
            shortcuts[2] = kViewerNodeParamActionZoomOut;
            shortcuts[7] = kViewerNodeParamActionScaleOne;
            param->setShortcuts(shortcuts);
        }
        {
            std::vector<int> separators;
            separators.push_back(2);
            param->setSeparators(separators);
        }
        param->setTextToFitHorizontally("100000%");
        page->addKnob(param);
        _imp->zoomChoiceKnob = param;
    }


    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerInstanceParamClipToFormat);
        param->setLabel(tr(kViewerInstanceParamClipToFormatLabel));
        param->setHintToolTip(tr(kViewerInstanceParamClipToFormatHint));
        page->addKnob(param);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setDefaultValue(true);
        param->setIconLabel(NATRON_IMAGES_PATH "cliptoprojectEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "cliptoprojectDisable.png", false);
        _imp->clipToFormatButtonKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamFullFrame);
        param->setLabel(tr(kViewerNodeParamFullFrameLabel));
        param->setHintToolTip(tr(kViewerNodeParamFullFrameHint));
        param->setCheckable(true);
        page->addKnob(param);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "fullFrameOn.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "fullFrameOff.png", false);
        _imp->fullFrameButtonKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerInstanceParamEnableUserRoI);
        param->setLabel(tr(kViewerInstanceParamEnableUserRoILabel));
        param->setHintToolTip(tr(kViewerInstanceParamEnableUserRoIHint));
        param->addInViewerContextShortcutsReference(kViewerNodeParamActionCreateNewRoI);
        param->setCheckable(true);
        page->addKnob(param);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "viewer_roiEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "viewer_roiDisabled.png", false);
        addOverlaySlaveParam(param);
        _imp->toggleUserRoIButtonKnob = param;
    }

    {
        KnobDoublePtr param = createKnob<KnobDouble>(std::string(kViewerInstanceParamUserRoIBottomLeft), 2 );
        param->setDefaultValuesAreNormalized(true);
        param->setSecret(true);
        param->setDefaultValue(0.2, DimIdx(0));
        param->setDefaultValue(0.2, DimIdx(1));
        page->addKnob(param);
        _imp->userRoIBtmLeftKnob = param;
    }
    {
        KnobDoublePtr param = createKnob<KnobDouble>(std::string(kViewerInstanceParamUserRoISize), 2 );
        param->setDefaultValuesAreNormalized(true);
        param->setDefaultValue(.6, DimIdx(0));
        param->setDefaultValue(.6, DimIdx(1));
        param->setSecret(true);
        page->addKnob(param);
        _imp->userRoISizeKnob = param;
    }


    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamProxyLevel);
        param->setLabel(tr(kViewerNodeParamProxyLevelLabel));
        param->setHintToolTip(tr(kViewerNodeParamProxyLevelHint));
        {
            std::vector<ChoiceOption> entries;
            entries.push_back(ChoiceOption("Auto", "", ""));
            entries.push_back(ChoiceOption("2", "", ""));
            entries.push_back(ChoiceOption("4", "", ""));
            entries.push_back(ChoiceOption("8", "", ""));
            entries.push_back(ChoiceOption("16", "", ""));
            entries.push_back(ChoiceOption("32", "", ""));
            param->populateChoices(entries);
        }
        {
            std::map<int, std::string> shortcuts;
            shortcuts[1] = kViewerNodeParamActionProxy2;
            shortcuts[2] = kViewerNodeParamActionProxy4;
            shortcuts[3] = kViewerNodeParamActionProxy8;
            shortcuts[4] = kViewerNodeParamActionProxy16;
            shortcuts[5] = kViewerNodeParamActionProxy32;
            param->setShortcuts(shortcuts);
        }
        page->addKnob(param);
        param->setSecret(true);
        _imp->downscaleChoiceKnob = param;
    }


    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamPauseRender);
        param->setLabel(tr(kViewerNodeParamPauseRenderLabel));
        param->setHintToolTip(tr(kViewerNodeParamPauseRenderHint));
        param->addInViewerContextShortcutsReference(kViewerNodeParamActionPauseAB);
        page->addKnob(param);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setDefaultValue(false);
        param->setIconLabel(NATRON_IMAGES_PATH "pauseEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "pauseDisabled.png", false);
        _imp->pauseButtonKnob[0] = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamPauseRenderB);
        param->setLabel(tr(kViewerNodeParamPauseRenderLabel));
        param->setHintToolTip(tr(kViewerNodeParamPauseRenderHint));
        page->addKnob(param);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setDefaultValue(false);
        param->setIconLabel(NATRON_IMAGES_PATH "pauseEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "pauseDisabled.png", false);
        _imp->pauseButtonKnob[1] = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamAInput);
        param->setLabel(tr(kViewerNodeParamAInputLabel));
        param->setHintToolTip(tr(kViewerNodeParamAInputHint));
        param->setInViewerContextLabel(tr(kViewerNodeParamAInputLabel));
        page->addKnob(param);
        param->setTextToFitHorizontally("ColorCorrect1");
        param->setSecret(true);
        _imp->aInputNodeChoiceKnob = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamOperation);
        param->setLabel(tr(kViewerNodeParamOperationLabel));
        param->setHintToolTip(tr(kViewerNodeParamOperation));
        {
            std::vector<ChoiceOption> entries;
            entries.push_back(ChoiceOption("-", "", ""));
            entries.push_back(ChoiceOption(kViewerNodeParamOperationWipeUnder, "", tr(kViewerNodeParamOperationWipeUnderHint).toStdString()));
            entries.push_back(ChoiceOption(kViewerNodeParamOperationWipeOver, "", tr(kViewerNodeParamOperationWipeOverHint).toStdString()));
            entries.push_back(ChoiceOption(kViewerNodeParamOperationWipeMinus, "", tr(kViewerNodeParamOperationWipeMinusHint).toStdString()));
            entries.push_back(ChoiceOption(kViewerNodeParamOperationWipeOnionSkin, "", tr(kViewerNodeParamOperationWipeOnionSkinHint).toStdString()));
            entries.push_back(ChoiceOption(kViewerNodeParamOperationStackUnder, "", tr(kViewerNodeParamOperationStackUnderHint).toStdString()));
            entries.push_back(ChoiceOption(kViewerNodeParamOperationStackOver, "", tr(kViewerNodeParamOperationStackOverHint).toStdString()));
            entries.push_back(ChoiceOption(kViewerNodeParamOperationStackMinus, "", tr(kViewerNodeParamOperationStackMinusHint).toStdString()));
            entries.push_back(ChoiceOption(kViewerNodeParamOperationStackOnionSkin, "", tr(kViewerNodeParamOperationStackOnionSkinHint).toStdString()));

            param->populateChoices(entries);
        }
        {
            std::map<int, std::string> shortcuts;
            shortcuts[1] = kViewerNodeParamRightClickMenuToggleWipe;
            param->setShortcuts(shortcuts);
        }
        {
            std::vector<int> separators;
            separators.push_back(4);
            param->setSeparators(separators);
        }
        page->addKnob(param);
        param->setSecret(true);
        param->setTextToFitHorizontally("Wipe OnionSkin");
        param->setInViewerContextIconFilePath(NATRON_IMAGES_PATH "GroupingIcons/Set3/merge_grouping_3.png");
        _imp->blendingModeChoiceKnob = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamBInput);
        param->setLabel(tr(kViewerNodeParamBInputLabel));
        param->setHintToolTip(tr(kViewerNodeParamBInputHint));
        param->setInViewerContextLabel(tr(kViewerNodeParamBInputLabel));
        param->setTextToFitHorizontally("ColorCorrect1");
        page->addKnob(param);
        param->setSecret(true);
        _imp->bInputNodeChoiceKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamEnableGain);
        param->setLabel(tr(kViewerNodeParamEnableGainLabel));
        param->setHintToolTip(tr(kViewerNodeParamEnableGainHint));
        page->addKnob(param);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "expoON.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "expoOFF.png", false);
        _imp->enableGainButtonKnob = param;
    }

    {
        KnobDoublePtr param = createKnob<KnobDouble>(kViewerNodeParamGain);
        param->setLabel(tr(kViewerNodeParamGainLabel));
        param->setHintToolTip(tr(kViewerNodeParamGainHint));
        page->addKnob(param);
        param->setSecret(true);
        param->setDisplayRange(-6., 6.);
        _imp->gainSliderKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamEnableAutoContrast);
        param->setLabel(tr(kViewerNodeParamEnableAutoContrastLabel));
        param->setHintToolTip(tr(kViewerNodeParamEnableAutoContrastHint));
        page->addKnob(param);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "AutoContrastON.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "AutoContrast.png", false);
        _imp->enableAutoContrastButtonKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamEnableGamma);
        param->setLabel(tr(kViewerNodeParamEnableGammaLabel));
        param->setHintToolTip(tr(kViewerNodeParamEnableGammaHint));
        page->addKnob(param);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "gammaON.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "gammaOFF.png", false);
        _imp->enableGammaButtonKnob = param;
    }

    {
        KnobDoublePtr param = createKnob<KnobDouble>(kViewerNodeParamGamma);
        param->setLabel(tr(kViewerNodeParamGammaLabel));
        param->setHintToolTip(tr(kViewerNodeParamGammaHint));
        param->setDefaultValue(1.);
        page->addKnob(param);
        param->setSecret(true);
        param->setDisplayRange(0., 5.);
        _imp->gammaSliderKnob = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamColorspace);
        param->setLabel(tr(kViewerNodeParamColorspaceLabel));
        param->setHintToolTip(tr(kViewerNodeParamColorspaceHint));
        {
            std::vector<ChoiceOption> entries;
            entries.push_back(ChoiceOption("Linear(None)", "", ""));
            entries.push_back(ChoiceOption("sRGB", "", ""));
            entries.push_back(ChoiceOption("Rec.709", "", ""));
            param->populateChoices(entries);
        }
        param->setDefaultValue(1);
        page->addKnob(param);
        param->setSecret(true);
        _imp->colorspaceKnob = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamView);
        param->setLabel(tr(kViewerNodeParamViewLabel));
        param->setHintToolTip(tr(kViewerNodeParamViewHint));
        {
            // Views gets populated in getPreferredMetadata
            std::vector<ChoiceOption> entries;
            param->populateChoices(entries);
        }
        page->addKnob(param);

        param->setSecret(true);
        _imp->activeViewKnob = param;
        refreshViewsKnobVisibility();
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamRefreshViewport);
        param->setLabel(tr(kViewerNodeParamRefreshViewportLabel));
        param->setHintToolTip(tr(kViewerNodeParamRefreshViewportHint));
        param->addInViewerContextShortcutsReference(kViewerNodeParamActionRefreshWithStats);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        // Do not set evaluate on change, trigger the render ourselves in ViewerNode::knobChanged
        // We do this so that we can set down/up the button during render to give feedback to the user without triggering a new render
        param->setEvaluateOnChange(false);
        param->setIsPersistent(false);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "refreshActive.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "refresh.png", false);
        page->addKnob(param);
        _imp->refreshButtonKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamSyncViewports);
        param->setLabel(tr(kViewerNodeParamSyncViewportsLabel));
        param->setHintToolTip(tr(kViewerNodeParamSyncViewportsHint));
        param->setInViewerContextCanHaveShortcut(true);
        param->setSecret(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "locked.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "unlocked.png", false);
        page->addKnob(param);
        _imp->syncViewersButtonKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamFitViewport);
        param->setLabel(tr(kViewerNodeParamFitViewportLabel));
        param->setHintToolTip(tr(kViewerNodeParamFitViewportHint));
        param->setInViewerContextCanHaveShortcut(true);
        param->setSecret(true);
        param->setIconLabel(NATRON_IMAGES_PATH "centerViewer.png", true);
        page->addKnob(param);
        _imp->centerViewerButtonKnob = param;
    }


    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamCheckerBoard);
        param->setLabel(tr(kViewerNodeParamCheckerBoardLabel));
        param->setHintToolTip(tr(kViewerNodeParamCheckerBoardHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "checkerboard_on.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "checkerboard_off.png", false);
        page->addKnob(param);
        _imp->enableCheckerboardButtonKnob = param;
    }


    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamEnableColorPicker);
        param->setLabel(tr(kViewerNodeParamEnableColorPickerLabel));
        param->setHintToolTip(tr(kViewerNodeParamEnableColorPickerHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setDefaultValue(true);
        param->setIconLabel(NATRON_IMAGES_PATH "color_picker.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "color_picker.png", false);
        page->addKnob(param);
        _imp->enableInfoBarButtonKnob = param;
    }

    // Player toolbar
    TimeValue projectFirst, projectLast;
    getApp()->getProject()->getFrameRange(&projectFirst, &projectLast);
    double projectFps = getApp()->getProject()->getProjectFrameRate();
    TimeValue currentFrame(getApp()->getTimeLine()->currentFrame());



    KnobPagePtr playerToolbarPage = createKnob<KnobPage>(kViewerNodeParamPlayerToolBarPage);
    playerToolbarPage->setSecret(true);
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamSetInPoint);
        param->setLabel(tr(kViewerNodeParamSetInPointLabel));
        param->setHintToolTip(tr(kViewerNodeParamSetInPointHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "timelineIn.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "timelineIn.png", false);
        _imp->setInPointButtonKnob = param;
    }
    {
        KnobIntPtr param = createKnob<KnobInt>(kViewerNodeParamInPoint);
        param->setLabel(tr(kViewerNodeParamInPointLabel));
        param->setHintToolTip(tr(kViewerNodeParamInPointHint));
        param->setSecret(true);
        param->setDefaultValue(projectFirst);
        param->setValueCenteredInSpinBox(true);
        param->setEvaluateOnChange(false);
        _imp->inPointKnob = param;
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(kViewerNodeParamEnableFps);
        param->setLabel(tr(kViewerNodeParamEnableFpsLabel));
        param->setHintToolTip(tr(kViewerNodeParamEnableFpsHint));
        param->setInViewerContextLabel(tr("FPS"));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        _imp->enableFpsKnob = param;
    }

    {
        KnobDoublePtr param = createKnob<KnobDouble>(kViewerNodeParamFps);
        param->setLabel(tr(kViewerNodeParamFpsLabel));
        param->setHintToolTip(tr(kViewerNodeParamFpsHint));
        param->setSecret(true);
        param->setDefaultValue(projectFps);
        param->setEvaluateOnChange(false);
        param->setEnabled(false);
        param->setRange(0., std::numeric_limits<double>::infinity());
        param->disableSlider();
        _imp->fpsKnob = param;
    }
    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamTimeFormat);
        param->setLabel(tr(kViewerNodeParamTimeFormatLabel));
        param->setHintToolTip(tr(kViewerNodeParamTimeFormatHint));
        {
            std::vector<ChoiceOption> entries;
            entries.push_back(ChoiceOption("TC", tr("TC").toStdString(), tr("Timecode").toStdString()));
            entries.push_back(ChoiceOption("TF", tr("TF").toStdString(), tr("Timeline Frames").toStdString()));
            param->populateChoices(entries);
        }
        param->setDefaultValueFromID("TF");
        _imp->timeFormatKnob = param;
    }


    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamEnableTurboMode);
        param->setLabel(tr(kViewerNodeParamEnableTurboModeLabel));
        param->setHintToolTip(tr(kViewerNodeParamEnableTurboModeHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "turbo_on.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "turbo_off.png", false);
        _imp->enableTurboModeButtonKnob = param;
    }
    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerNodeParamPlaybackMode);
        param->setLabel(tr(kViewerNodeParamPlaybackModeLabel));
        param->setHintToolTip(tr(kViewerNodeParamPlaybackModeHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        {
            std::vector<ChoiceOption> entries;
            std::map<int ,std::string> icons;
            entries.push_back(ChoiceOption("Repeat", "", tr("Playback will loop over the timeline in/out points").toStdString()));
            icons[0] = NATRON_IMAGES_PATH "loopmode.png";
            entries.push_back(ChoiceOption("Bounce", "", tr("Playback will bounce between the timeline in/out points").toStdString()));
            icons[1] = NATRON_IMAGES_PATH "bounce.png";
            entries.push_back(ChoiceOption("Stop", "", tr("Playback will play once until reaches either the timeline's in or out point").toStdString()));
            icons[2] = NATRON_IMAGES_PATH "playOnce.png";
            param->populateChoices(entries);
            param->setIcons(icons);
        }
        _imp->playbackModeKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamSyncTimelines);
        param->setLabel(tr(kViewerNodeParamSyncTimelinesLabel));
        param->setHintToolTip(tr(kViewerNodeParamSyncTimelinesHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "locked.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "unlocked.png", false);
        _imp->syncTimelinesButtonKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamFirstFrame);
        param->setLabel(tr(kViewerNodeParamFirstFrameLabel));
        param->setHintToolTip(tr(kViewerNodeParamFirstFrameHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "firstFrame.png", true);
        _imp->firstFrameButtonKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamPlayBackward);
        param->setLabel(tr(kViewerNodeParamPlayBackwardLabel));
        param->setHintToolTip(tr(kViewerNodeParamPlayBackwardHint));
        param->addInViewerContextShortcutsReference(kViewerNodeParamActionAbortRender);
        param->setSecret(true);
        param->setCheckable(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "rewind_enabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "rewind.png", false);
        _imp->playBackwardButtonKnob = param;
    }
    {
        KnobIntPtr param = createKnob<KnobInt>(kViewerNodeParamCurrentFrame);
        param->setLabel(tr(kViewerNodeParamCurrentFrameLabel));
        param->setHintToolTip(tr(kViewerNodeParamCurrentFrameHint));
        param->setSecret(true);
        param->setDefaultValue(currentFrame);
        param->setEvaluateOnChange(false);
        param->setIsPersistent(false);
        param->setValueCenteredInSpinBox(true);
        param->disableSlider();
        _imp->curFrameKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamPlayForward);
        param->setLabel(tr(kViewerNodeParamPlayForwardLabel));
        param->setHintToolTip(tr(kViewerNodeParamPlayForwardHint));
        param->addInViewerContextShortcutsReference(kViewerNodeParamActionAbortRender);
        param->setSecret(true);
        param->setCheckable(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "play_enabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "play.png", false);
        _imp->playForwardButtonKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamLastFrame);
        param->setLabel(tr(kViewerNodeParamLastFrameLabel));
        param->setHintToolTip(tr(kViewerNodeParamLastFrameHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "lastFrame.png", true);
        _imp->lastFrameButtonKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamPreviousFrame);
        param->setLabel(tr(kViewerNodeParamPreviousFrameLabel));
        param->setHintToolTip(tr(kViewerNodeParamPreviousFrameHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "back1.png", true);
        _imp->prevFrameButtonKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamNextFrame);
        param->setLabel(tr(kViewerNodeParamNextFrameLabel));
        param->setHintToolTip(tr(kViewerNodeParamNextFrameHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "forward1.png", true);
        _imp->nextFrameButtonKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamPreviousKeyFrame);
        param->setLabel(tr(kViewerNodeParamPreviousKeyFrameLabel));
        param->setHintToolTip(tr(kViewerNodeParamPreviousKeyFrameHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "prevKF.png", true);
        _imp->prevKeyFrameButtonKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamNextKeyFrame);
        param->setLabel(tr(kViewerNodeParamNextKeyFrameLabel));
        param->setHintToolTip(tr(kViewerNodeParamNextKeyFrameHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "nextKF.png", true);
        _imp->nextKeyFrameButtonKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamPreviousIncr);
        param->setLabel(tr(kViewerNodeParamPreviousIncrLabel));
        param->setHintToolTip(tr(kViewerNodeParamPreviousIncrHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "previousIncr.png", true);
        _imp->prevIncrButtonKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamNextIncr);
        param->setLabel(tr(kViewerNodeParamNextIncrLabel));
        param->setHintToolTip(tr(kViewerNodeParamNextIncrHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "nextIncr.png", true);
        _imp->nextIncrButtonKnob = param;
    }
    {
        KnobIntPtr param = createKnob<KnobInt>(kViewerNodeParamFrameIncrement);
        param->setLabel(tr(kViewerNodeParamFrameIncrementLabel));
        param->setHintToolTip(tr(kViewerNodeParamFrameIncrementHint));
        param->setSecret(true);
        param->setDefaultValue(10);
        param->setValueCenteredInSpinBox(true);
        param->setEvaluateOnChange(false);
        param->disableSlider();
        _imp->incrFrameKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerNodeParamSetOutPoint);
        param->setLabel(tr(kViewerNodeParamSetOutPointLabel));
        param->setHintToolTip(tr(kViewerNodeParamSetOutPointHint));
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel(NATRON_IMAGES_PATH "timelineOut.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "timelineOut.png", false);
        _imp->setOutPointButtonKnob = param;
    }
    {
        KnobIntPtr param = createKnob<KnobInt>(kViewerNodeParamOutPoint);
        param->setLabel(tr(kViewerNodeParamOutPointLabel));
        param->setHintToolTip(tr(kViewerNodeParamOutPointHint));
        param->setSecret(true);
        param->setValueCenteredInSpinBox(true);
        param->setDefaultValue(projectLast);
        param->setEvaluateOnChange(false);
        _imp->outPointKnob = param;
    }

    addKnobToViewerUI(_imp->layersKnob.lock());
    addKnobToViewerUI(_imp->alphaChannelKnob.lock());
    addKnobToViewerUI(_imp->displayChannelsKnob[0].lock());
    _imp->displayChannelsKnob[0].lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeStretchAfter);
    addKnobToViewerUI(_imp->aInputNodeChoiceKnob.lock());
    addKnobToViewerUI(_imp->blendingModeChoiceKnob.lock());
    addKnobToViewerUI(_imp->bInputNodeChoiceKnob.lock());
    _imp->bInputNodeChoiceKnob.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeStretchAfter);

    addKnobToViewerUI(_imp->clipToFormatButtonKnob.lock());
    _imp->clipToFormatButtonKnob.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->downscaleChoiceKnob.lock());
    _imp->downscaleChoiceKnob.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->fullFrameButtonKnob.lock());
    _imp->fullFrameButtonKnob.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->toggleUserRoIButtonKnob.lock());
    _imp->toggleUserRoIButtonKnob.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeSeparator);
    addKnobToViewerUI(_imp->refreshButtonKnob.lock());
    _imp->refreshButtonKnob.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->pauseButtonKnob[0].lock());
    _imp->pauseButtonKnob[0].lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeSeparator);

    addKnobToViewerUI(_imp->centerViewerButtonKnob.lock());
    _imp->centerViewerButtonKnob.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->syncViewersButtonKnob.lock());
    _imp->syncViewersButtonKnob.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->zoomChoiceKnob.lock());
    _imp->zoomChoiceKnob.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeAddNewLine);


    addKnobToViewerUI(_imp->enableGainButtonKnob.lock());
    addKnobToViewerUI(_imp->gainSliderKnob.lock());
    addKnobToViewerUI(_imp->enableAutoContrastButtonKnob.lock());
    addKnobToViewerUI(_imp->enableGammaButtonKnob.lock());
    addKnobToViewerUI(_imp->gammaSliderKnob.lock());
    addKnobToViewerUI(_imp->colorspaceKnob.lock());
    addKnobToViewerUI(_imp->enableCheckerboardButtonKnob.lock());
    addKnobToViewerUI(_imp->activeViewKnob.lock());
    _imp->activeViewKnob.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeStretchAfter);
    addKnobToViewerUI(_imp->enableInfoBarButtonKnob.lock());
    _imp->enableInfoBarButtonKnob.lock()->setInViewerContextItemSpacing(0);


    // Setup player buttons layout but don't add them to the viewer UI, since it's kind of a specific case, we want the toolbar below
    // the viewer
    playerToolbarPage->addKnob(_imp->setInPointButtonKnob.lock());
    _imp->setInPointButtonKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->inPointKnob.lock());
    _imp->inPointKnob.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeStretchAfter);
    playerToolbarPage->addKnob(_imp->enableFpsKnob.lock());
    _imp->enableFpsKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->fpsKnob.lock());
    _imp->fpsKnob.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeSeparator);
    playerToolbarPage->addKnob(_imp->timeFormatKnob.lock());
    _imp->timeFormatKnob.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeSeparator);
    playerToolbarPage->addKnob(_imp->enableTurboModeButtonKnob.lock());
    _imp->enableTurboModeButtonKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->playbackModeKnob.lock());
    _imp->playbackModeKnob.lock()->setInViewerContextItemSpacing(VIEWER_UI_SECTIONS_SPACING_PX);
    playerToolbarPage->addKnob(_imp->syncTimelinesButtonKnob.lock());
    _imp->syncTimelinesButtonKnob.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeStretchAfter);
    playerToolbarPage->addKnob(_imp->firstFrameButtonKnob.lock());
    _imp->firstFrameButtonKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->playBackwardButtonKnob.lock());
    _imp->playBackwardButtonKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->curFrameKnob.lock());
    _imp->curFrameKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->playForwardButtonKnob.lock());
    _imp->playForwardButtonKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->lastFrameButtonKnob.lock());
    _imp->lastFrameButtonKnob.lock()->setInViewerContextItemSpacing(VIEWER_UI_SECTIONS_SPACING_PX);
    playerToolbarPage->addKnob(_imp->prevFrameButtonKnob.lock());
    _imp->prevFrameButtonKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->nextFrameButtonKnob.lock());
    _imp->nextFrameButtonKnob.lock()->setInViewerContextItemSpacing(VIEWER_UI_SECTIONS_SPACING_PX);
    playerToolbarPage->addKnob(_imp->prevKeyFrameButtonKnob.lock());
    _imp->prevKeyFrameButtonKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->nextKeyFrameButtonKnob.lock());
    _imp->nextKeyFrameButtonKnob.lock()->setInViewerContextItemSpacing(VIEWER_UI_SECTIONS_SPACING_PX);
    playerToolbarPage->addKnob(_imp->prevIncrButtonKnob.lock());
    _imp->prevIncrButtonKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->incrFrameKnob.lock());
    _imp->incrFrameKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->nextIncrButtonKnob.lock());
    _imp->nextIncrButtonKnob.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeStretchAfter);
    playerToolbarPage->addKnob(_imp->outPointKnob.lock());
    _imp->outPointKnob.lock()->setInViewerContextItemSpacing(0);
    playerToolbarPage->addKnob(_imp->setOutPointButtonKnob.lock());

    // Right click menu
    KnobChoicePtr rightClickMenu = createKnob<KnobChoice>( std::string(kViewerNodeParamRightClickMenu) );
    rightClickMenu->setSecret(true);
    rightClickMenu->setEvaluateOnChange(false);
    page->addKnob(rightClickMenu);
    _imp->rightClickMenu = rightClickMenu;
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuToggleWipe);
        action->setLabel(tr(kViewerNodeParamRightClickMenuToggleWipeLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickToggleWipe = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuCenterWipe);
        action->setLabel(tr(kViewerNodeParamRightClickMenuCenterWipeLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickCenterWipe = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuPreviousLayer);
        action->setLabel(tr(kViewerNodeParamRightClickMenuPreviousLayerLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickPreviousLayer = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuNextLayer);
        action->setLabel(tr(kViewerNodeParamRightClickMenuNextLayerLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickNextLayer = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuPreviousView);
        action->setLabel(tr(kViewerNodeParamRightClickMenuPreviousViewLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickPreviousView = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuNextView);
        action->setLabel(tr(kViewerNodeParamRightClickMenuNextViewLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickNextView = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuSwitchAB);
        action->setLabel(tr(kViewerNodeParamRightClickMenuSwitchABLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickSwitchAB = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuShowHideOverlays);
        action->setLabel(tr(kViewerNodeParamRightClickMenuShowHideOverlaysLabel));
        action->setSecret(true);
        action->setCheckable(true);
        action->setDefaultValue(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setEvaluateOnChange(false);
        addOverlaySlaveParam(action);
        page->addKnob(action);
        _imp->rightClickShowHideOverlays = action;
    }

    KnobChoicePtr showHideSubMenu = createKnob<KnobChoice>(kViewerNodeParamRightClickMenuShowHideSubMenu);
    showHideSubMenu->setLabel(tr(kViewerNodeParamRightClickMenuShowHideSubMenuLabel));
    showHideSubMenu->setSecret(true);
    showHideSubMenu->setEvaluateOnChange(false);
    page->addKnob(showHideSubMenu);
    _imp->rightClickShowHideSubMenu = showHideSubMenu;

    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuHideAll);
        action->setLabel(tr(kViewerNodeParamRightClickMenuHideAllLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        page->addKnob(action);
        _imp->rightClickHideAll = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuHideAllTop);
        action->setLabel(tr(kViewerNodeParamRightClickMenuHideAllTopLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        page->addKnob(action);
        _imp->rightClickHideAllTop = action;
    }


    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuHideAllBottom);
        action->setLabel(tr(kViewerNodeParamRightClickMenuHideAllBottomLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        page->addKnob(action);
        _imp->rightClickHideAllBottom = action;
    }


    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuShowHidePlayer);
        action->setLabel(tr(kViewerNodeParamRightClickMenuShowHidePlayerLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        action->setDefaultValue(true);
        page->addKnob(action);
        _imp->rightClickShowHidePlayer = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuShowHideTimeline);
        action->setLabel(tr(kViewerNodeParamRightClickMenuShowHideTimelineLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        action->setDefaultValue(true);
        page->addKnob(action);
        _imp->rightClickShowHideTimeline = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuShowHideLeftToolbar);
        action->setLabel(tr(kViewerNodeParamRightClickMenuShowHideLeftToolbarLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        action->setDefaultValue(true);
        page->addKnob(action);
        _imp->rightClickShowHideLeftToolbar = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuShowHideTopToolbar);
        action->setLabel(tr(kViewerNodeParamRightClickMenuShowHideTopToolbarLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        action->setDefaultValue(true);
        page->addKnob(action);
        _imp->rightClickShowHideTopToolbar = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamRightClickMenuShowHideTabHeader);
        action->setLabel(tr(kViewerNodeParamRightClickMenuShowHideTabHeaderLabel));
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        action->setDefaultValue(true);
        page->addKnob(action);
        _imp->rightClickShowHideTabHeader = action;
    }


    // Viewer actions
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionLuminance);
        action->setLabel(tr(kViewerNodeParamActionLuminanceLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayLuminanceAction[0] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionLuminanceA);
        action->setLabel(tr(kViewerNodeParamActionLuminanceALabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayLuminanceAction[1] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionRed);
        action->setLabel(tr(kViewerNodeParamActionRedLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayRedAction[0] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionRedA);
        action->setLabel(tr(kViewerNodeParamActionRedALabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayRedAction[1] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionGreen);
        action->setLabel(tr(kViewerNodeParamActionGreenLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayGreenAction[0] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionGreenA);
        action->setLabel(tr(kViewerNodeParamActionGreenALabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayGreenAction[1] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionBlue);
        action->setLabel(tr(kViewerNodeParamActionBlueLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayBlueAction[0] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionBlueA);
        action->setLabel(tr(kViewerNodeParamActionBlueALabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayBlueAction[1] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionAlpha);
        action->setLabel(tr(kViewerNodeParamActionAlphaLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayAlphaAction[0] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionAlphaA);
        action->setLabel(tr(kViewerNodeParamActionAlphaALabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayAlphaAction[1] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionMatte);
        action->setLabel(tr(kViewerNodeParamActionMatteLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayMatteAction[0] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionMatteA);
        action->setLabel(tr(kViewerNodeParamActionMatteALabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayMatteAction[1] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionZoomIn);
        action->setLabel(tr(kViewerNodeParamActionZoomInLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->zoomInAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionZoomOut);
        action->setLabel(tr(kViewerNodeParamActionZoomOutLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->zoomOutAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionScaleOne);
        action->setLabel(tr(kViewerNodeParamActionScaleOneLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->zoomScaleOneAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionProxy2);
        action->setLabel(tr(kViewerNodeParamActionProxy2Label));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[0] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionProxy4);
        action->setLabel(tr(kViewerNodeParamActionProxy4Label));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[1] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionProxy8);
        action->setLabel(tr(kViewerNodeParamActionProxy8Label));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[2] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionProxy16);
        action->setLabel(tr(kViewerNodeParamActionProxy16Label));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[3] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionProxy32);
        action->setLabel(tr(kViewerNodeParamActionProxy32Label));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[4] = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionLeftView);
        action->setLabel(tr(kViewerNodeParamActionLeftViewLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->leftViewAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionRightView);
        action->setLabel(tr(kViewerNodeParamActionRightViewLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightViewAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionPauseAB);
        action->setLabel(tr(kViewerNodeParamActionPauseABLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->pauseABAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionCreateNewRoI);
        action->setLabel(tr(kViewerNodeParamActionCreateNewRoILabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->createUserRoIAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionRefreshWithStats);
        action->setLabel(tr(kViewerNodeParamActionRefreshWithStatsLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->enableStatsAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kViewerNodeParamActionAbortRender);
        action->setLabel(tr(kViewerNodeParamActionAbortRenderLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->abortRenderingAction = action;
    }

    // Viewer overlays
    {
        KnobDoublePtr param = createKnob<KnobDouble>( std::string(kViewerNodeParamWipeCenter), 2 );
        param->setLabel(tr(kViewerNodeParamWipeCenter));
        param->setSecret(true);
        param->setDefaultValuesAreNormalized(true);
        param->setDefaultValue(0.5, DimIdx(0));
        param->setDefaultValue(0.5, DimIdx(1));
        page->addKnob(param);
        addOverlaySlaveParam(param);
        _imp->wipeCenter = param;
    }
    {
        KnobDoublePtr param = createKnob<KnobDouble>( std::string(kViewerNodeParamWipeAmount), 1 );
        param->setLabel(tr(kViewerNodeParamWipeAmount));
        param->setSecret(true);
        param->setDefaultValue(1.);
        page->addKnob(param);
        addOverlaySlaveParam(param);
        _imp->wipeAmount = param;
    }
    {
        KnobDoublePtr param = createKnob<KnobDouble>( std::string(kViewerNodeParamWipeAngle), 1 );
        param->setLabel(tr(kViewerNodeParamWipeAngle));
        param->setSecret(true);
        page->addKnob(param);
        addOverlaySlaveParam(param);
        _imp->wipeAngle = param;
    }


    RenderEnginePtr engine = getNode()->getRenderEngine();
    QObject::connect( engine.get(), SIGNAL(renderFinished(int)), this, SLOT(onEngineStopped()) );
    QObject::connect( engine.get(), SIGNAL(renderStarted(bool)), this, SLOT(onEngineStarted(bool)) );

    _imp->mustSetUpPlaybackButtonsTimer.setSingleShot(true);
    QObject::connect( &_imp->mustSetUpPlaybackButtonsTimer, SIGNAL(timeout()), this, SLOT(onSetDownPlaybackButtonsTimeout()) );

    // Refresh visibility & enabledness
    _imp->fpsKnob.lock()->setEnabled(_imp->enableFpsKnob.lock()->getValue());

    // Refresh playback mode
    PlaybackModeEnum mode = (PlaybackModeEnum)_imp->playbackModeKnob.lock()->getValue();
    engine->setPlaybackMode(mode);

    // Refresh fps
    engine->setDesiredFPS(_imp->fpsKnob.lock()->getValue());


} // initializeKnobs

void
ViewerNode::onKnobsLoaded()
{
    _imp->lastGammaValue = _imp->gammaSliderKnob.lock()->getValue();
    _imp->lastFstopValue = _imp->gainSliderKnob.lock()->getValue();

    // Refresh visibility & enabledness
    _imp->fpsKnob.lock()->setEnabled(_imp->enableFpsKnob.lock()->getValue());

    bool fullFrameOn = _imp->fullFrameButtonKnob.lock()->getValue();
    _imp->downscaleChoiceKnob.lock()->setEnabled(!fullFrameOn);

    RenderEnginePtr engine = getNode()->getRenderEngine();
    // Refresh playback mode
    PlaybackModeEnum mode = (PlaybackModeEnum)_imp->playbackModeKnob.lock()->getValue();
    engine->setPlaybackMode(mode);

    // Refresh fps
    engine->setDesiredFPS(_imp->fpsKnob.lock()->getValue());

}


bool
ViewerNode::knobChanged(const KnobIPtr& k, ValueChangedReasonEnum reason,
                        ViewSetSpec view,
                        TimeValue /*time*/)
{
    if (!k || reason == eValueChangedReasonRestoreDefault) {
        return false;
    }

    bool caught = true;
    if (k == _imp->layersKnob.lock() && reason != eValueChangedReasonPluginEdited) {
        _imp->setAlphaChannelFromLayerIfRGBA();
    } else if (k == _imp->aInputNodeChoiceKnob.lock()) {
        if (reason != eValueChangedReasonPluginEdited) {
            refreshInputFromChoiceMenu(0);
        }
    } else if (k == _imp->bInputNodeChoiceKnob.lock()) {
        if (reason != eValueChangedReasonPluginEdited) {
            refreshInputFromChoiceMenu(1);
        }
    } else if (k == _imp->blendingModeChoiceKnob.lock()) {
        ViewerCompositingOperatorEnum op = (ViewerCompositingOperatorEnum)_imp->blendingModeChoiceKnob.lock()->getValue();
        _imp->uiContext->setInfoBarVisible(1, op != eViewerCompositingOperatorNone);
        _imp->bInputNodeChoiceKnob.lock()->setEnabled(op != eViewerCompositingOperatorNone);
    } else if (k == _imp->zoomChoiceKnob.lock() && reason != eValueChangedReasonPluginEdited) {
        ChoiceOption zoomChoice = _imp->zoomChoiceKnob.lock()->getCurrentEntry();
        if (zoomChoice.id == "Fit") {
            _imp->uiContext->fitImageToFormat();
        } else if (zoomChoice.id == "+") {
            _imp->scaleZoomFactor(1.1);
        } else if (zoomChoice.id == "-") {
            _imp->scaleZoomFactor(0.9);
        } else {
            QString str = QString::fromUtf8(zoomChoice.id.c_str());
            str = str.trimmed();
            str = str.mid(0, str.size() - 1);
            int zoomInteger = str.toInt();
            _imp->uiContext->zoomViewport(zoomInteger / 100.);
        }
    } else if (k == _imp->enableGainButtonKnob.lock() && reason == eValueChangedReasonUserEdited) {
        double value;
        bool down = _imp->enableGainButtonKnob.lock()->getValue();
        if (down) {
            value = _imp->lastFstopValue;
        } else {
            value = 0;
        }
        _imp->gainSliderKnob.lock()->setValue(value, ViewIdx(0), DimIdx(0), eValueChangedReasonPluginEdited);
    } else if (k == _imp->gainSliderKnob.lock()) {
        KnobButtonPtr enableKnob = _imp->enableGainButtonKnob.lock();
        if (reason == eValueChangedReasonUserEdited) {
            enableKnob->setValue(true, view, DimIdx(0), eValueChangedReasonPluginEdited);
            _imp->lastFstopValue =  _imp->gainSliderKnob.lock()->getValue();
        }
    } else if (k == _imp->enableGammaButtonKnob.lock() && reason == eValueChangedReasonUserEdited) {
        double value;
        bool down = _imp->enableGammaButtonKnob.lock()->getValue();
        if (down) {
            value = _imp->lastGammaValue;
        } else {
            value = 1;
        }
        _imp->gammaSliderKnob.lock()->setValue(value, view, DimIdx(0), eValueChangedReasonPluginEdited);

    } else if (k == _imp->gammaSliderKnob.lock()) {
        KnobButtonPtr enableKnob = _imp->enableGammaButtonKnob.lock();
        if (reason == eValueChangedReasonUserEdited) {
            enableKnob->setValue(true, view, DimIdx(0), eValueChangedReasonPluginEdited);
            _imp->lastGammaValue =  _imp->gammaSliderKnob.lock()->getValue();
        }

    } else if (k == _imp->enableAutoContrastButtonKnob.lock()) {
        bool enable = _imp->enableAutoContrastButtonKnob.lock()->getValue();
        _imp->enableGammaButtonKnob.lock()->setEnabled(!enable);
        _imp->gammaSliderKnob.lock()->setEnabled(!enable);
        _imp->enableGainButtonKnob.lock()->setEnabled(!enable);
        _imp->gainSliderKnob.lock()->setEnabled(!enable);

    } else if (k == _imp->refreshButtonKnob.lock() && reason == eValueChangedReasonUserEdited) {
        getApp()->checkAllReadersModificationDate(false);
        getNode()->clearAllPersistentMessages(true); // user can explicitely clear persistent messages, which may be re-set by the next render
        forceNextRenderWithoutCacheRead();
        getNode()->getRenderEngine()->renderCurrentFrame();
    } else if (k == _imp->syncViewersButtonKnob.lock()) {

        getApp()->setMasterSyncViewer(getNode());
        NodesList allNodes;
        getApp()->getProject()->getNodes_recursive(allNodes);
        double left, bottom, factor, par;
        _imp->uiContext->getProjection(&left, &bottom, &factor, &par);
        for (NodesList::iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
            ViewerNodePtr instance = toViewerNode((*it)->getEffectInstance());
            if (instance && instance.get() != this) {
                instance->getUiContext()->setProjection(left, bottom, factor, par);
                instance->getNode()->getRenderEngine()->renderCurrentFrame();
            }
        }
    } else if (k == _imp->centerViewerButtonKnob.lock()) {
        if (!getApp()->getActiveRotoDrawingStroke()) {
            _imp->uiContext->fitImageToFormat();
        }
    } else if (k == _imp->enableInfoBarButtonKnob.lock()) {
        bool infoBarVisible = _imp->enableInfoBarButtonKnob.lock()->getValue();
        if (reason == eValueChangedReasonUserEdited) {
            NodesList allNodes;
            getApp()->getProject()->getNodes_recursive(allNodes);
            ViewerNodePtr thisInstance = shared_from_this();
            for (NodesList::iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
                ViewerNodePtr instance = toViewerNode((*it)->getEffectInstance());
                if (instance) {
                    if (instance != thisInstance) {
                        instance->_imp->enableInfoBarButtonKnob.lock()->setValue(infoBarVisible);
                    }
                    instance->_imp->uiContext->setInfoBarVisible(infoBarVisible);
                }
            }
        } else {
            _imp->uiContext->setInfoBarVisible(infoBarVisible);
        }
    } else if (k == _imp->rightClickToggleWipe.lock()) {
        KnobChoicePtr wipeChoiceKnob = _imp->blendingModeChoiceKnob.lock();
        int value = wipeChoiceKnob->getValue();
        if (value != 0) {
            wipeChoiceKnob->setValue(0, view, DimIdx(0), eValueChangedReasonPluginEdited);
        } else {
            if (_imp->lastWipeIndex == 0) {
                _imp->lastWipeIndex = 1;
            }
            wipeChoiceKnob->setValue(_imp->lastWipeIndex, view, DimIdx(0), eValueChangedReasonPluginEdited);
        }
    } else if (k == _imp->blendingModeChoiceKnob.lock() && reason == eValueChangedReasonUserEdited) {
        KnobChoicePtr wipeChoice = _imp->blendingModeChoiceKnob.lock();
        int value = wipeChoice->getValue();
        if (value != 0) {
            _imp->lastWipeIndex = value;
        }
    } else if (k == _imp->rightClickCenterWipe.lock()) {
        KnobDoublePtr knob = _imp->wipeCenter.lock();
        std::vector<double> values(2);
        values[0] = _imp->ui->lastMousePos.x();
        values[1] = _imp->ui->lastMousePos.y();
        knob->setValueAcrossDimensions(values, DimIdx(0), view, eValueChangedReasonPluginEdited);
    } else if (k == _imp->rightClickNextLayer.lock()) {

    } else if (k == _imp->rightClickPreviousLayer.lock()) {

    } else if (k == _imp->rightClickSwitchAB.lock()) {
        _imp->swapViewerProcessInputs();
    } else if (k == _imp->rightClickHideAll.lock()) {
        bool allHidden = _imp->rightClickHideAll.lock()->getValue();
        _imp->rightClickHideAllTop.lock()->setValue(!allHidden, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        _imp->rightClickHideAllBottom.lock()->setValue(!allHidden, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        if (reason != eValueChangedReasonPluginEdited) {
            if (!getApp()->getActiveRotoDrawingStroke()) {
                _imp->uiContext->fitImageToFormat();
            }
        }
    } else if (k == _imp->rightClickHideAllTop.lock()) {
        bool allHidden = _imp->rightClickHideAllTop.lock()->getValue();
        _imp->rightClickShowHideTopToolbar.lock()->setValue(!allHidden, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        _imp->rightClickShowHideLeftToolbar.lock()->setValue(!allHidden, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        _imp->rightClickShowHideTabHeader.lock()->setValue(!allHidden, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        if (reason != eValueChangedReasonPluginEdited) {
            if (!getApp()->getActiveRotoDrawingStroke()) {
                _imp->uiContext->fitImageToFormat();
            }
        }
    } else if (k == _imp->rightClickHideAllBottom.lock()) {
        bool allHidden = _imp->rightClickHideAllBottom.lock()->getValue();
        _imp->rightClickShowHidePlayer.lock()->setValue(!allHidden, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        _imp->rightClickShowHideTimeline.lock()->setValue(!allHidden, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        _imp->enableInfoBarButtonKnob.lock()->setValue(!allHidden, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        if (reason != eValueChangedReasonPluginEdited) {
            if (!getApp()->getActiveRotoDrawingStroke()) {
                _imp->uiContext->fitImageToFormat();
            }
        }
    } else if (k == _imp->rightClickShowHideTopToolbar.lock()) {
        bool topToolbarVisible = _imp->rightClickShowHideTopToolbar.lock()->getValue();
        _imp->uiContext->setTopToolBarVisible(topToolbarVisible);
    } else if (k == _imp->rightClickShowHideLeftToolbar.lock()) {
        bool visible = _imp->rightClickShowHideLeftToolbar.lock()->getValue();
        _imp->uiContext->setLeftToolBarVisible(visible);
    } else if (k == _imp->rightClickShowHidePlayer.lock()) {
        bool visible = _imp->rightClickShowHidePlayer.lock()->getValue();
        _imp->uiContext->setPlayerVisible(visible);
    } else if (k == _imp->rightClickShowHideTimeline.lock()) {
        bool visible = _imp->rightClickShowHideTimeline.lock()->getValue();
        _imp->uiContext->setTimelineVisible(visible);
    } else if (k == _imp->rightClickShowHideTabHeader.lock()) {
        bool visible = _imp->rightClickShowHideTabHeader.lock()->getValue();
        _imp->uiContext->setTabHeaderVisible(visible);
    } else if (k == _imp->displayRedAction[0].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[0].lock()->getValue() != eDisplayChannelsR) {
            _imp->setDisplayChannels((int)eDisplayChannelsR, true);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, true);
        }
    } else if (k == _imp->displayRedAction[1].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[1].lock()->getValue() != eDisplayChannelsR) {
            _imp->setDisplayChannels((int)eDisplayChannelsR, false);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, false);
        }
    } else if (k == _imp->displayGreenAction[0].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[0].lock()->getValue() != eDisplayChannelsG) {
            _imp->setDisplayChannels((int)eDisplayChannelsG, true);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, true);
        }
    } else if (k == _imp->displayGreenAction[1].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[1].lock()->getValue() != eDisplayChannelsG) {
            _imp->setDisplayChannels((int)eDisplayChannelsG, false);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, false);
        }
    } else if (k == _imp->displayBlueAction[0].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[0].lock()->getValue() != eDisplayChannelsB) {
            _imp->setDisplayChannels((int)eDisplayChannelsB, true);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, true);
        }
    } else if (k == _imp->displayBlueAction[1].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[1].lock()->getValue() != eDisplayChannelsB) {
            _imp->setDisplayChannels((int)eDisplayChannelsB, false);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, false);
        }
    } else if (k == _imp->displayAlphaAction[0].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[0].lock()->getValue() != eDisplayChannelsA) {
            _imp->setDisplayChannels((int)eDisplayChannelsA, true);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, true);
        }
    } else if (k == _imp->displayAlphaAction[1].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[1].lock()->getValue() != eDisplayChannelsA) {
            _imp->setDisplayChannels((int)eDisplayChannelsA, false);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, false);
        }
    } else if (k == _imp->displayMatteAction[0].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[0].lock()->getValue() != eDisplayChannelsMatte) {
            _imp->setDisplayChannels((int)eDisplayChannelsMatte, true);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, true);
        }
    } else if (k == _imp->displayMatteAction[1].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[1].lock()->getValue() != eDisplayChannelsMatte) {
            _imp->setDisplayChannels((int)eDisplayChannelsMatte, false);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, false);
        }
    } else if (k == _imp->displayLuminanceAction[0].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[0].lock()->getValue() != eDisplayChannelsY) {
            _imp->setDisplayChannels((int)eDisplayChannelsY, true);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, true);
        }
    } else if (k == _imp->displayLuminanceAction[1].lock()) {
        if ((DisplayChannelsEnum)_imp->displayChannelsKnob[1].lock()->getValue() != eDisplayChannelsY) {
            _imp->setDisplayChannels((int)eDisplayChannelsY, false);
        } else {
            _imp->setDisplayChannels((int)eDisplayChannelsRGB, false);
        }
    } else if (k == _imp->zoomInAction.lock()) {
        _imp->scaleZoomFactor(1.1);
    } else if (k == _imp->zoomOutAction.lock()) {
        _imp->scaleZoomFactor(0.9);
    } else if (k == _imp->zoomScaleOneAction.lock()) {
        _imp->uiContext->zoomViewport(1.);
    } else if (k == _imp->proxyLevelAction[0].lock()) {
        _imp->toggleDownscaleLevel(1);
    } else if (k == _imp->proxyLevelAction[1].lock()) {
        _imp->toggleDownscaleLevel(2);
    } else if (k == _imp->proxyLevelAction[2].lock()) {
        _imp->toggleDownscaleLevel(3);
    } else if (k == _imp->proxyLevelAction[3].lock()) {
        _imp->toggleDownscaleLevel(4);
    } else if (k == _imp->proxyLevelAction[4].lock()) {
        _imp->toggleDownscaleLevel(5);
    } else if (k == _imp->leftViewAction.lock()) {
        _imp->activeViewKnob.lock()->setValue(0);
    } else if (k == _imp->rightViewAction.lock()) {
        const std::vector<std::string>& views = getApp()->getProject()->getProjectViewNames();
        if (views.size() > 1) {
            _imp->activeViewKnob.lock()->setValue(1);
        }
    } else if (k == _imp->pauseABAction.lock()) {
        bool curValue = _imp->pauseButtonKnob[0].lock()->getValue();
        _imp->pauseButtonKnob[0].lock()->setValue(!curValue);
        _imp->pauseButtonKnob[1].lock()->setValue(!curValue);
    } else if (k == _imp->createUserRoIAction.lock()) {
        _imp->ui->buildUserRoIOnNextPress = true;
        _imp->toggleUserRoIButtonKnob.lock()->setValue(true);
        _imp->ui->draggedUserRoI = getUserRoI();
    } else if (k == _imp->toggleUserRoIButtonKnob.lock()) {
        bool enabled = _imp->toggleUserRoIButtonKnob.lock()->getValue();
        if (!enabled) {
            _imp->ui->buildUserRoIOnNextPress = false;
        }
    } else if (k == _imp->enableStatsAction.lock() && reason == eValueChangedReasonUserEdited) {
        getApp()->checkAllReadersModificationDate(false);
        forceNextRenderWithoutCacheRead();
        getApp()->showRenderStatsWindow();
        getNode()->getRenderEngine()->renderCurrentFrameWithRenderStats();
    } else if (k == _imp->rightClickPreviousLayer.lock()) {
        KnobChoicePtr knob = _imp->layersKnob.lock();
        int currentIndex = knob->getValue();
        int nChoices = knob->getNumEntries();

        if (currentIndex <= 1) {
            currentIndex = nChoices - 1;
        } else {
            --currentIndex;
        }
        if (currentIndex >= 0) {
            // User edited so the handler is executed
            knob->setValue(currentIndex, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited);
        }

    } else if (k == _imp->rightClickNextLayer.lock()) {

        KnobChoicePtr knob = _imp->layersKnob.lock();
        int currentIndex = knob->getValue();
        int nChoices = knob->getNumEntries();

        currentIndex = (currentIndex + 1) % nChoices;
        if ( (currentIndex == 0) && (nChoices > 1) ) {
            currentIndex = 1;
        }
        knob->setValue(currentIndex, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited);
    } else if (k == _imp->rightClickPreviousView.lock()) {
        KnobChoicePtr knob = _imp->activeViewKnob.lock();
        int currentIndex = knob->getValue();
        int nChoices = knob->getNumEntries();

        if (currentIndex == 0) {
            currentIndex = nChoices - 1;
        } else {
            --currentIndex;
        }
        if (currentIndex >= 0) {
            // User edited so the handler is executed
            knob->setValue(currentIndex, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited);
        }
    } else if (k == _imp->rightClickNextView.lock()) {
        KnobChoicePtr knob = _imp->activeViewKnob.lock();
        int currentIndex = knob->getValue();
        int nChoices = knob->getNumEntries();

        if (currentIndex == nChoices - 1) {
            currentIndex = 0;
        } else {
            currentIndex = currentIndex + 1;
        }

        knob->setValue(currentIndex, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited);
    } else if (k == _imp->enableFpsKnob.lock()) {
        _imp->fpsKnob.lock()->setEnabled(_imp->enableFpsKnob.lock()->getValue());
        refreshFps();
    } else if (k == _imp->fpsKnob.lock() && reason != eValueChangedReasonPluginEdited) {
        refreshFps();
    } else if (k == _imp->timeFormatKnob.lock()) {
        _imp->uiContext->setTimelineFormatFrames(_imp->timeFormatKnob.lock()->getValue() == 1);
    } else if (k == _imp->playForwardButtonKnob.lock()) {
        if (reason != eValueChangedReasonPluginEdited) {
            if (_imp->playForwardButtonKnob.lock()->getValue()) {
                _imp->startPlayback(eRenderDirectionForward);
            } else {
                abortAllViewersRendering();
            }
        }
    } else if (k == _imp->playBackwardButtonKnob.lock()) {
        if (reason != eValueChangedReasonPluginEdited) {
            if (_imp->playBackwardButtonKnob.lock()->getValue()) {
                _imp->startPlayback(eRenderDirectionBackward);
            } else {
                abortAllViewersRendering();
            }
        }
    } else if (k == _imp->curFrameKnob.lock()) {
        if (reason != eValueChangedReasonPluginEdited) {
            _imp->timelineGoTo(TimeValue(_imp->curFrameKnob.lock()->getValue()));
        }
    } else if (k == _imp->prevFrameButtonKnob.lock()) {
        int prevFrame = getTimeline()->currentFrame() - 1;

        if ( prevFrame  < _imp->inPointKnob.lock()->getValue() ) {
            prevFrame = _imp->outPointKnob.lock()->getValue();
        }
        _imp->timelineGoTo(TimeValue(prevFrame));
    } else if (k == _imp->nextFrameButtonKnob.lock()) {
        int nextFrame = getTimeline()->currentFrame() + 1;

        if ( nextFrame > _imp->outPointKnob.lock()->getValue()) {
            nextFrame = _imp->inPointKnob.lock()->getValue();
        }
        _imp->timelineGoTo(TimeValue(nextFrame));
    } else if (k == _imp->prevKeyFrameButtonKnob.lock()) {
        getApp()->goToPreviousKeyframe();
    } else if (k == _imp->nextKeyFrameButtonKnob.lock()) {
        getApp()->goToNextKeyframe();
    } else if (k == _imp->prevIncrButtonKnob.lock()) {
        int time = getTimeline()->currentFrame();
        time -= _imp->incrFrameKnob.lock()->getValue();
        _imp->timelineGoTo(TimeValue(time));
    } else if (k == _imp->nextIncrButtonKnob.lock()) {
        int time = getTimeline()->currentFrame();
        time += _imp->incrFrameKnob.lock()->getValue();
        _imp->timelineGoTo(TimeValue(time));
    } else if (k == _imp->firstFrameButtonKnob.lock()) {
        int time = _imp->inPointKnob.lock()->getValue();
        _imp->timelineGoTo(TimeValue(time));
    } else if (k == _imp->lastFrameButtonKnob.lock()) {
        int time = _imp->outPointKnob.lock()->getValue();
        _imp->timelineGoTo(TimeValue(time));
    } else if (k == _imp->playbackModeKnob.lock()) {
        PlaybackModeEnum mode = (PlaybackModeEnum)_imp->playbackModeKnob.lock()->getValue();
        getNode()->getRenderEngine()->setPlaybackMode(mode);

    } else if (k == _imp->syncTimelinesButtonKnob.lock()) {
        if (reason != eValueChangedReasonPluginEdited) {
            getUiContext()->setTripleSyncEnabled(_imp->syncTimelinesButtonKnob.lock()->getValue());
        }
    } else if (k == _imp->enableTurboModeButtonKnob.lock()) {
        if (reason != eValueChangedReasonPluginEdited) {
            getApp()->setGuiFrozen(_imp->enableTurboModeButtonKnob.lock()->getValue());
        }
    } else if (k == _imp->abortRenderingAction.lock()) {
        abortAllViewersRendering();
    } else if (k == _imp->setInPointButtonKnob.lock()) {
        _imp->inPointKnob.lock()->setValue(getTimelineCurrentTime(), ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
    } else if (k == _imp->setOutPointButtonKnob.lock()) {
        _imp->outPointKnob.lock()->setValue(getTimelineCurrentTime(), ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);

    } else if (k == _imp->inPointKnob.lock()) {
        _imp->uiContext->setTimelineBounds(_imp->inPointKnob.lock()->getValue(), _imp->outPointKnob.lock()->getValue());
    } else if (k == _imp->outPointKnob.lock()) {
        _imp->uiContext->setTimelineBounds(_imp->inPointKnob.lock()->getValue(), _imp->outPointKnob.lock()->getValue());
    } else if (k == _imp->fullFrameButtonKnob.lock()) {
        bool fullFrameOn = _imp->fullFrameButtonKnob.lock()->getValue();
        _imp->downscaleChoiceKnob.lock()->setEnabled(!fullFrameOn);
    } else {
        caught = false;
    }
    return caught;
    
} // knobChanged


NATRON_NAMESPACE_EXIT
