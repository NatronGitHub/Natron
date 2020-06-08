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

#include "RotoPaint.h"

#include <sstream> // stringstream
#include <cassert>
#include <stdexcept>

#include "Engine/AppInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeMetadata.h"
#include "Engine/MergingEnum.h"
#include "Engine/RotoContext.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoUndoCommand.h"
#include "Engine/RotoPaintInteract.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"


#include "Global/GLIncludes.h"
#include "Global/GlobalDefines.h"


#define ROTOPAINT_MASK_INPUT_INDEX 10

#define ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX 5

NATRON_NAMESPACE_ENTER


std::string
RotoPaint::getPluginDescription() const
{
    return "RotoPaint is a vector based free-hand drawing node that helps for tasks such as rotoscoping, matting, etc...";
}

RotoPaint::RotoPaint(NodePtr node,
                     bool isPaintByDefault)
    : EffectInstance(node)
    , _imp( new RotoPaintPrivate(this, isPaintByDefault) )
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

RotoPaint::~RotoPaint()
{
}

bool
RotoPaint::isDefaultBehaviourPaintContext() const
{
    return _imp->isPaintByDefault;
}

std::string
RotoPaint::getPluginID() const
{
    return PLUGINID_NATRON_ROTOPAINT;
}

std::string
RotoPaint::getPluginLabel() const
{
    return "RotoPaint";
}

std::string
RotoNode::getPluginID() const
{
    return PLUGINID_NATRON_ROTO;
}

std::string
RotoNode::getPluginLabel() const
{
    return "Roto";
}

std::string
RotoNode::getPluginDescription() const
{
    return "Create masks and shapes";
}

bool
RotoPaint::isHostChannelSelectorSupported(bool* defaultR,
                                          bool* defaultG,
                                          bool* defaultB,
                                          bool* defaultA) const
{
    //Use our own selectors, we don't want Natron to copy back channels
    *defaultR = true;
    *defaultG = true;
    *defaultB = true;
    *defaultA = true;

    return false;
}

bool
RotoNode::isHostChannelSelectorSupported(bool* defaultR,
                                         bool* defaultG,
                                         bool* defaultB,
                                         bool* defaultA) const
{
    *defaultR = false;
    *defaultG = false;
    *defaultB = false;
    *defaultA = true;

    return false;
}

std::string
RotoPaint::getInputLabel (int inputNb) const
{
    if (inputNb == ROTOPAINT_MASK_INPUT_INDEX) {
        return "Mask";
    } else if (inputNb == 0) {
        return "Bg";
    } else {
        std::stringstream ss;
        ss << "Bg" << inputNb + 1;

        return ss.str();
    }
}

bool
RotoPaint::isInputMask(int inputNb) const
{
    return inputNb == ROTOPAINT_MASK_INPUT_INDEX;
}

void
RotoPaint::addAcceptedComponents(int inputNb,
                                 std::list<ImagePlaneDesc>* comps)
{
    if (inputNb != ROTOPAINT_MASK_INPUT_INDEX) {
        comps->push_back( ImagePlaneDesc::getRGBAComponents() );
        comps->push_back( ImagePlaneDesc::getRGBComponents() );
        comps->push_back( ImagePlaneDesc::getXYComponents() );
    }
    comps->push_back( ImagePlaneDesc::getAlphaComponents() );
}

void
RotoPaint::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
}

void
RotoPaint::initializeKnobs()
{
    //This page is created in the RotoContext, before initializeKnobs() is called.
    KnobPagePtr generalPage = boost::dynamic_pointer_cast<KnobPage>( getKnobByName("General") );

    assert(generalPage);


    KnobSeparatorPtr sep = AppManager::createKnob<KnobSeparator>(this, tr("Output"), 1, false);
    generalPage->addKnob(sep);


    std::string channelNames[4] = {kNatronOfxParamProcessR, kNatronOfxParamProcessG, kNatronOfxParamProcessB, kNatronOfxParamProcessA};
    std::string channelLabels[4] = {"R", "G", "B", "A"};
    bool defaultValues[4];
    bool channelSelectorSupported = isHostChannelSelectorSupported(&defaultValues[0], &defaultValues[1], &defaultValues[2], &defaultValues[3]);
    Q_UNUSED(channelSelectorSupported);

    for (int i = 0; i < 4; ++i) {
        KnobBoolPtr enabled =  AppManager::createKnob<KnobBool>(this, channelLabels[i], 1, false);
        enabled->setName(channelNames[i]);
        enabled->setAnimationEnabled(false);
        enabled->setAddNewLine(i == 3);
        enabled->setDefaultValue(defaultValues[i]);
        enabled->setHintToolTip( tr("Enable drawing onto this channel") );
        generalPage->addKnob(enabled);
        _imp->enabledKnobs[i] = enabled;
    }


    KnobBoolPtr premultKnob = AppManager::createKnob<KnobBool>(this, tr("Premultiply"), 1, false);
    premultKnob->setName("premultiply");
    premultKnob->setHintToolTip( tr("When checked, the red, green and blue channels of the output are premultiplied by the alpha channel.\n"
                                    "This will result in the pixels outside of the shapes and paint strokes being black and transparent.\n"
                                    "This should only be used if all the inputs are Opaque or UnPremultiplied, and only the Alpha channel "
                                    "is selected to be drawn by this node.") );
    premultKnob->setDefaultValue(false);
    premultKnob->setAnimationEnabled(false);
    premultKnob->setIsMetadataSlave(true);
    _imp->premultKnob = premultKnob;
    generalPage->addKnob(premultKnob);

    RotoContextPtr context = getNode()->getRotoContext();
    assert(context);
    QObject::connect( context.get(), SIGNAL(refreshViewerOverlays()), this, SLOT(onRefreshAsked()) );
    QObject::connect( context.get(), SIGNAL(selectionChanged(int)), this, SLOT(onSelectionChanged(int)) );
    QObject::connect( context.get(), SIGNAL(itemLockedChanged(int)), this, SLOT(onCurveLockedChanged(int)) );
    QObject::connect( context.get(), SIGNAL(breakMultiStroke()), this, SLOT(onBreakMultiStrokeTriggered()) );


    /// Initializing the viewer interface
    KnobButtonPtr autoKeyingEnabled = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamAutoKeyingEnabledLabel) );
    autoKeyingEnabled->setName(kRotoUIParamAutoKeyingEnabled);
    autoKeyingEnabled->setHintToolTip( tr(kRotoUIParamAutoKeyingEnabledHint) );
    autoKeyingEnabled->setEvaluateOnChange(false);
    autoKeyingEnabled->setCheckable(true);
    autoKeyingEnabled->setDefaultValue(true);
    autoKeyingEnabled->setSecretByDefault(true);
    autoKeyingEnabled->setInViewerContextCanHaveShortcut(true);
    autoKeyingEnabled->setIconLabel(NATRON_IMAGES_PATH "autoKeyingEnabled.png", true);
    autoKeyingEnabled->setIconLabel(NATRON_IMAGES_PATH "autoKeyingDisabled.png", false);
    generalPage->addKnob(autoKeyingEnabled);
    _imp->ui->autoKeyingEnabledButton = autoKeyingEnabled;

    KnobButtonPtr featherLinkEnabled = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamFeatherLinkEnabledLabel) );
    featherLinkEnabled->setName(kRotoUIParamFeatherLinkEnabled);
    featherLinkEnabled->setCheckable(true);
    featherLinkEnabled->setEvaluateOnChange(false);
    featherLinkEnabled->setDefaultValue(true);
    featherLinkEnabled->setSecretByDefault(true);
    featherLinkEnabled->setInViewerContextCanHaveShortcut(true);
    featherLinkEnabled->setHintToolTip( tr(kRotoUIParamFeatherLinkEnabledHint) );
    featherLinkEnabled->setIconLabel(NATRON_IMAGES_PATH "featherLinkEnabled.png", true);
    featherLinkEnabled->setIconLabel(NATRON_IMAGES_PATH "featherLinkDisabled.png", false);
    generalPage->addKnob(featherLinkEnabled);
    _imp->ui->featherLinkEnabledButton = featherLinkEnabled;

    KnobButtonPtr displayFeatherEnabled = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamDisplayFeatherLabel) );
    displayFeatherEnabled->setName(kRotoUIParamDisplayFeather);
    displayFeatherEnabled->setHintToolTip( tr(kRotoUIParamDisplayFeatherHint) );
    displayFeatherEnabled->setEvaluateOnChange(false);
    displayFeatherEnabled->setCheckable(true);
    displayFeatherEnabled->setDefaultValue(true);
    displayFeatherEnabled->setSecretByDefault(true);
    displayFeatherEnabled->setInViewerContextCanHaveShortcut(true);
    addOverlaySlaveParam(displayFeatherEnabled);
    displayFeatherEnabled->setIconLabel(NATRON_IMAGES_PATH "featherEnabled.png", true);
    displayFeatherEnabled->setIconLabel(NATRON_IMAGES_PATH "featherDisabled.png", false);
    generalPage->addKnob(displayFeatherEnabled);
    _imp->ui->displayFeatherEnabledButton = displayFeatherEnabled;

    KnobButtonPtr stickySelection = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamStickySelectionEnabledLabel) );
    stickySelection->setName(kRotoUIParamStickySelectionEnabled);
    stickySelection->setHintToolTip( tr(kRotoUIParamStickySelectionEnabledHint) );
    stickySelection->setEvaluateOnChange(false);
    stickySelection->setCheckable(true);
    stickySelection->setDefaultValue(false);
    stickySelection->setSecretByDefault(true);
    stickySelection->setInViewerContextCanHaveShortcut(true);
    stickySelection->setIconLabel(NATRON_IMAGES_PATH "stickySelectionEnabled.png", true);
    stickySelection->setIconLabel(NATRON_IMAGES_PATH "stickySelectionDisabled.png", false);
    generalPage->addKnob(stickySelection);
    _imp->ui->stickySelectionEnabledButton = stickySelection;

    KnobButtonPtr bboxClickAnywhere = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamStickyBboxLabel) );
    bboxClickAnywhere->setName(kRotoUIParamStickyBbox);
    bboxClickAnywhere->setHintToolTip( tr(kRotoUIParamStickyBboxHint) );
    bboxClickAnywhere->setEvaluateOnChange(false);
    bboxClickAnywhere->setCheckable(true);
    bboxClickAnywhere->setDefaultValue(false);
    bboxClickAnywhere->setSecretByDefault(true);
    bboxClickAnywhere->setInViewerContextCanHaveShortcut(true);
    bboxClickAnywhere->setIconLabel(NATRON_IMAGES_PATH "viewer_roiEnabled.png", true);
    bboxClickAnywhere->setIconLabel(NATRON_IMAGES_PATH "viewer_roiDisabled.png", false);
    generalPage->addKnob(bboxClickAnywhere);
    _imp->ui->bboxClickAnywhereButton = bboxClickAnywhere;

    KnobButtonPtr rippleEditEnabled = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRippleEditLabel) );
    rippleEditEnabled->setName(kRotoUIParamRippleEdit);
    rippleEditEnabled->setHintToolTip( tr(kRotoUIParamRippleEditLabelHint) );
    rippleEditEnabled->setEvaluateOnChange(false);
    rippleEditEnabled->setCheckable(true);
    rippleEditEnabled->setDefaultValue(false);
    rippleEditEnabled->setSecretByDefault(true);
    rippleEditEnabled->setInViewerContextCanHaveShortcut(true);
    rippleEditEnabled->setIconLabel(NATRON_IMAGES_PATH "rippleEditEnabled.png", true);
    rippleEditEnabled->setIconLabel(NATRON_IMAGES_PATH "rippleEditDisabled.png", false);
    generalPage->addKnob(rippleEditEnabled);
    _imp->ui->rippleEditEnabledButton = rippleEditEnabled;

    KnobButtonPtr addKeyframe = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamAddKeyFrameLabel) );
    addKeyframe->setName(kRotoUIParamAddKeyFrame);
    addKeyframe->setEvaluateOnChange(false);
    addKeyframe->setHintToolTip( tr(kRotoUIParamAddKeyFrameHint) );
    addKeyframe->setSecretByDefault(true);
    addKeyframe->setInViewerContextCanHaveShortcut(true);
    addKeyframe->setIconLabel(NATRON_IMAGES_PATH "addKF.png");
    generalPage->addKnob(addKeyframe);
    _imp->ui->addKeyframeButton = addKeyframe;

    KnobButtonPtr removeKeyframe = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRemoveKeyframeLabel) );
    removeKeyframe->setName(kRotoUIParamRemoveKeyframe);
    removeKeyframe->setHintToolTip( tr(kRotoUIParamRemoveKeyframeHint) );
    removeKeyframe->setEvaluateOnChange(false);
    removeKeyframe->setSecretByDefault(true);
    removeKeyframe->setInViewerContextCanHaveShortcut(true);
    removeKeyframe->setIconLabel(NATRON_IMAGES_PATH "removeKF.png");
    generalPage->addKnob(removeKeyframe);
    _imp->ui->removeKeyframeButton = removeKeyframe;


    KnobButtonPtr showTransform = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamShowTransformLabel) );
    showTransform->setName(kRotoUIParamShowTransform);
    showTransform->setHintToolTip( tr(kRotoUIParamShowTransformHint) );
    showTransform->setEvaluateOnChange(false);
    showTransform->setSecretByDefault(true);
    showTransform->setCheckable(true);
    showTransform->setDefaultValue(true);
    showTransform->setInViewerContextCanHaveShortcut(true);
    showTransform->setInViewerContextNewLineActivated(true);
    showTransform->setIconLabel(NATRON_IMAGES_PATH "rotoShowTransformOverlay.png", true);
    showTransform->setIconLabel(NATRON_IMAGES_PATH "rotoHideTransformOverlay.png", false);
    generalPage->addKnob(showTransform);
    addOverlaySlaveParam(showTransform);
    _imp->ui->showTransformHandle = showTransform;
    // RotoPaint

    KnobBoolPtr multiStroke = AppManager::createKnob<KnobBool>( this, tr(kRotoUIParamMultiStrokeEnabledLabel) );
    multiStroke->setName(kRotoUIParamMultiStrokeEnabled);
    multiStroke->setInViewerContextLabel( tr(kRotoUIParamMultiStrokeEnabledLabel) );
    multiStroke->setHintToolTip( tr(kRotoUIParamMultiStrokeEnabledHint) );
    multiStroke->setEvaluateOnChange(false);
    multiStroke->setDefaultValue(true);
    multiStroke->setSecretByDefault(true);
    generalPage->addKnob(multiStroke);
    _imp->ui->multiStrokeEnabled = multiStroke;

    KnobColorPtr colorWheel = AppManager::createKnob<KnobColor>(this, tr(kRotoUIParamColorWheelLabel), 3);
    colorWheel->setName(kRotoUIParamColorWheel);
    colorWheel->setHintToolTip( tr(kRotoUIParamColorWheelHint) );
    colorWheel->setEvaluateOnChange(false);
    colorWheel->setDefaultValue(1., 0);
    colorWheel->setDefaultValue(1., 1);
    colorWheel->setDefaultValue(1., 2);
    colorWheel->setSecretByDefault(true);
    generalPage->addKnob(colorWheel);
    _imp->ui->colorWheelButton = colorWheel;

    KnobChoicePtr blendingModes = AppManager::createKnob<KnobChoice>( this, tr(kRotoUIParamBlendingOpLabel) );
    blendingModes->setName(kRotoUIParamBlendingOp);
    blendingModes->setHintToolTip( tr(kRotoUIParamBlendingOpHint) );
    blendingModes->setEvaluateOnChange(false);
    blendingModes->setSecretByDefault(true);
    {
        std::vector<ChoiceOption> choices;
        Merge::getOperatorStrings(&choices);
        blendingModes->populateChoices(choices);
    }
    blendingModes->setDefaultValue( (int)eMergeOver );
    generalPage->addKnob(blendingModes);
    _imp->ui->compositingOperatorChoice = blendingModes;


    KnobDoublePtr opacityKnob = AppManager::createKnob<KnobDouble>( this, tr(kRotoUIParamOpacityLabel) );
    opacityKnob->setName(kRotoUIParamOpacity);
    opacityKnob->setInViewerContextLabel( tr(kRotoUIParamOpacityLabel) );
    opacityKnob->setHintToolTip( tr(kRotoUIParamOpacityHint) );
    opacityKnob->setEvaluateOnChange(false);
    opacityKnob->setSecretByDefault(true);
    opacityKnob->setDefaultValue(1.);
    opacityKnob->setMinimum(0.);
    opacityKnob->setMaximum(1.);
    opacityKnob->disableSlider();
    generalPage->addKnob(opacityKnob);
    _imp->ui->opacitySpinbox = opacityKnob;

    KnobButtonPtr pressureOpacity = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamPressureOpacityLabel) );
    pressureOpacity->setName(kRotoUIParamPressureOpacity);
    pressureOpacity->setHintToolTip( tr(kRotoUIParamPressureOpacityHint) );
    pressureOpacity->setEvaluateOnChange(false);
    pressureOpacity->setCheckable(true);
    pressureOpacity->setDefaultValue(true);
    pressureOpacity->setSecretByDefault(true);
    pressureOpacity->setInViewerContextCanHaveShortcut(true);
    pressureOpacity->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_on.png", true);
    pressureOpacity->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_off.png", false);
    generalPage->addKnob(pressureOpacity);
    _imp->ui->pressureOpacityButton = pressureOpacity;

    KnobDoublePtr sizeKnob = AppManager::createKnob<KnobDouble>( this, tr(kRotoUIParamSizeLabel) );
    sizeKnob->setName(kRotoUIParamSize);
    sizeKnob->setInViewerContextLabel( tr(kRotoUIParamSizeLabel) );
    sizeKnob->setHintToolTip( tr(kRotoUIParamSizeHint) );
    sizeKnob->setEvaluateOnChange(false);
    sizeKnob->setSecretByDefault(true);
    sizeKnob->setDefaultValue(25.);
    sizeKnob->setMinimum(0.);
    sizeKnob->setMaximum(1000.);
    sizeKnob->disableSlider();
    generalPage->addKnob(sizeKnob);
    _imp->ui->sizeSpinbox = sizeKnob;

    KnobButtonPtr pressureSize = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamPressureSizeLabel) );
    pressureSize->setName(kRotoUIParamPressureSize);
    pressureSize->setHintToolTip( tr(kRotoUIParamPressureSizeHint) );
    pressureSize->setEvaluateOnChange(false);
    pressureSize->setCheckable(true);
    pressureSize->setDefaultValue(false);
    pressureSize->setSecretByDefault(true);
    pressureSize->setInViewerContextCanHaveShortcut(true);
    pressureSize->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_on.png", true);
    pressureSize->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_off.png", false);
    generalPage->addKnob(pressureSize);
    _imp->ui->pressureSizeButton = pressureSize;

    KnobDoublePtr hardnessKnob = AppManager::createKnob<KnobDouble>( this, tr(kRotoUIParamHardnessLabel) );
    hardnessKnob->setName(kRotoUIParamHardness);
    hardnessKnob->setInViewerContextLabel( tr(kRotoUIParamHardnessLabel) );
    hardnessKnob->setHintToolTip( tr(kRotoUIParamHardnessHint) );
    hardnessKnob->setEvaluateOnChange(false);
    hardnessKnob->setSecretByDefault(true);
    hardnessKnob->setDefaultValue(.2);
    hardnessKnob->setMinimum(0.);
    hardnessKnob->setMaximum(1.);
    hardnessKnob->disableSlider();
    generalPage->addKnob(hardnessKnob);
    _imp->ui->hardnessSpinbox = hardnessKnob;

    KnobButtonPtr pressureHardness = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamPressureHardnessLabel) );
    pressureHardness->setName(kRotoUIParamPressureHardness);
    pressureHardness->setHintToolTip( tr(kRotoUIParamPressureHardnessHint) );
    pressureHardness->setEvaluateOnChange(false);
    pressureHardness->setCheckable(true);
    pressureHardness->setDefaultValue(false);
    pressureHardness->setSecretByDefault(true);
    pressureHardness->setInViewerContextCanHaveShortcut(true);
    pressureHardness->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_on.png", true);
    pressureHardness->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_off.png", false);
    generalPage->addKnob(pressureHardness);
    _imp->ui->pressureHardnessButton = pressureHardness;

    KnobButtonPtr buildUp = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamBuildUpLabel) );
    buildUp->setName(kRotoUIParamBuildUp);
    buildUp->setInViewerContextLabel( tr(kRotoUIParamBuildUpLabel) );
    buildUp->setHintToolTip( tr(kRotoUIParamBuildUpHint) );
    buildUp->setEvaluateOnChange(false);
    buildUp->setCheckable(true);
    buildUp->setDefaultValue(true);
    buildUp->setSecretByDefault(true);
    buildUp->setInViewerContextCanHaveShortcut(true);
    buildUp->setIconLabel(NATRON_IMAGES_PATH "rotopaint_buildup_on.png", true);
    buildUp->setIconLabel(NATRON_IMAGES_PATH "rotopaint_buildup_off.png", false);
    generalPage->addKnob(buildUp);
    _imp->ui->buildUpButton = buildUp;

    KnobDoublePtr effectStrength = AppManager::createKnob<KnobDouble>( this, tr(kRotoUIParamEffectLabel) );
    effectStrength->setName(kRotoUIParamEffect);
    effectStrength->setInViewerContextLabel( tr(kRotoUIParamEffectLabel) );
    effectStrength->setHintToolTip( tr(kRotoUIParamEffectHint) );
    effectStrength->setEvaluateOnChange(false);
    effectStrength->setSecretByDefault(true);
    effectStrength->setDefaultValue(15);
    effectStrength->setMinimum(0.);
    effectStrength->setMaximum(100.);
    effectStrength->disableSlider();
    generalPage->addKnob(effectStrength);
    _imp->ui->effectSpinBox = effectStrength;

    KnobIntPtr timeOffsetSb = AppManager::createKnob<KnobInt>( this, tr(kRotoUIParamTimeOffsetLabel) );
    timeOffsetSb->setName(kRotoUIParamTimeOffset);
    timeOffsetSb->setInViewerContextLabel( tr(kRotoUIParamTimeOffsetLabel) );
    timeOffsetSb->setHintToolTip( tr(kRotoUIParamTimeOffsetHint) );
    timeOffsetSb->setEvaluateOnChange(false);
    timeOffsetSb->setSecretByDefault(true);
    timeOffsetSb->setDefaultValue(0);
    timeOffsetSb->disableSlider();
    generalPage->addKnob(timeOffsetSb);
    _imp->ui->timeOffsetSpinBox = timeOffsetSb;

    KnobChoicePtr timeOffsetMode = AppManager::createKnob<KnobChoice>( this, tr(kRotoUIParamTimeOffsetLabel) );
    timeOffsetMode->setName(kRotoUIParamTimeOffset);
    timeOffsetMode->setHintToolTip( tr(kRotoUIParamTimeOffsetHint) );
    timeOffsetMode->setEvaluateOnChange(false);
    timeOffsetMode->setSecretByDefault(true);
    {
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption("Relative", "", tr("The time offset is a frame number in the source").toStdString()));
        choices.push_back(ChoiceOption("Absolute", "", tr("The time offset is a relative amount of frames relative to the current frame").toStdString()));
        timeOffsetMode->populateChoices(choices);
    }
    timeOffsetMode->setDefaultValue(0);
    generalPage->addKnob(timeOffsetMode);
    _imp->ui->timeOffsetModeChoice = timeOffsetMode;

    KnobChoicePtr sourceType = AppManager::createKnob<KnobChoice>( this, tr(kRotoUIParamSourceTypeLabel) );
    sourceType->setName(kRotoUIParamSourceType);
    sourceType->setHintToolTip( tr(kRotoUIParamSourceTypeHint) );
    sourceType->setEvaluateOnChange(false);
    sourceType->setSecretByDefault(true);
    {
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption("foreground", "", ""));
        choices.push_back(ChoiceOption("background", "", ""));
        for (int i = 1; i < 10; ++i) {
            QString str = QString::fromUtf8("background") + QString::number(i + 1);
            choices.push_back( ChoiceOption(str.toStdString(), "", ""));
        }
        sourceType->populateChoices(choices);
    }
    sourceType->setDefaultValue(1);
    generalPage->addKnob(sourceType);
    _imp->ui->sourceTypeChoice = sourceType;


    KnobButtonPtr resetCloneOffset = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamResetCloneOffsetLabel) );
    resetCloneOffset->setName(kRotoUIParamResetCloneOffset);
    resetCloneOffset->setHintToolTip( tr(kRotoUIParamResetCloneOffsetHint) );
    resetCloneOffset->setEvaluateOnChange(false);
    resetCloneOffset->setSecretByDefault(true);
    resetCloneOffset->setInViewerContextCanHaveShortcut(true);
    resetCloneOffset->setInViewerContextNewLineActivated(true);
    generalPage->addKnob(resetCloneOffset);
    addOverlaySlaveParam(resetCloneOffset);
    _imp->ui->resetCloneOffsetButton = resetCloneOffset;


    // Roto
    addKnobToViewerUI(autoKeyingEnabled);
    addKnobToViewerUI(featherLinkEnabled);
    addKnobToViewerUI(displayFeatherEnabled);
    addKnobToViewerUI(stickySelection);
    addKnobToViewerUI(bboxClickAnywhere);
    addKnobToViewerUI(rippleEditEnabled);
    addKnobToViewerUI(addKeyframe);
    addKnobToViewerUI(removeKeyframe);
    addKnobToViewerUI(showTransform);

    // RotoPaint
    addKnobToViewerUI(multiStroke);
    multiStroke->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(colorWheel);
    colorWheel->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(blendingModes);
    blendingModes->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(opacityKnob);
    opacityKnob->setInViewerContextItemSpacing(1);
    addKnobToViewerUI(pressureOpacity);
    pressureOpacity->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(sizeKnob);
    sizeKnob->setInViewerContextItemSpacing(1);
    addKnobToViewerUI(pressureSize);
    pressureSize->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(hardnessKnob);
    hardnessKnob->setInViewerContextItemSpacing(1);
    addKnobToViewerUI(pressureHardness);
    pressureHardness->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(buildUp);
    buildUp->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(effectStrength);
    effectStrength->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(timeOffsetSb);
    timeOffsetSb->setInViewerContextItemSpacing(1);
    addKnobToViewerUI(timeOffsetMode);
    addKnobToViewerUI(sourceType);
    addKnobToViewerUI(resetCloneOffset);


    KnobPagePtr toolbar = AppManager::createKnob<KnobPage>( this, std::string(kRotoUIParamToolbar) );
    toolbar->setAsToolBar(true);
    toolbar->setEvaluateOnChange(false);
    toolbar->setSecretByDefault(true);
    _imp->ui->toolbarPage = toolbar;
    KnobGroupPtr selectionToolButton = AppManager::createKnob<KnobGroup>( this, tr(kRotoUIParamSelectionToolButtonLabel) );
    selectionToolButton->setName(kRotoUIParamSelectionToolButton);
    selectionToolButton->setAsToolButton(true);
    selectionToolButton->setEvaluateOnChange(false);
    selectionToolButton->setSecretByDefault(true);
    selectionToolButton->setInViewerContextCanHaveShortcut(true);
    selectionToolButton->setIsPersistent(false);
    toolbar->addKnob(selectionToolButton);
    _imp->ui->selectToolGroup = selectionToolButton;
    KnobGroupPtr editPointsToolButton = AppManager::createKnob<KnobGroup>( this, tr(kRotoUIParamEditPointsToolButtonLabel) );
    editPointsToolButton->setName(kRotoUIParamEditPointsToolButton);
    editPointsToolButton->setAsToolButton(true);
    editPointsToolButton->setEvaluateOnChange(false);
    editPointsToolButton->setSecretByDefault(true);
    editPointsToolButton->setInViewerContextCanHaveShortcut(true);
    editPointsToolButton->setIsPersistent(false);
    toolbar->addKnob(editPointsToolButton);
    _imp->ui->pointsEditionToolGroup = editPointsToolButton;
    KnobGroupPtr editBezierToolButton = AppManager::createKnob<KnobGroup>( this, tr(kRotoUIParamBezierEditionToolButtonLabel) );
    editBezierToolButton->setName(kRotoUIParamBezierEditionToolButton);
    editBezierToolButton->setAsToolButton(true);
    editBezierToolButton->setEvaluateOnChange(false);
    editBezierToolButton->setSecretByDefault(true);
    editBezierToolButton->setInViewerContextCanHaveShortcut(true);
    editBezierToolButton->setIsPersistent(false);
    toolbar->addKnob(editBezierToolButton);
    _imp->ui->bezierEditionToolGroup = editBezierToolButton;
    KnobGroupPtr paintToolButton = AppManager::createKnob<KnobGroup>( this, tr(kRotoUIParamPaintBrushToolButtonLabel) );
    paintToolButton->setName(kRotoUIParamPaintBrushToolButton);
    paintToolButton->setAsToolButton(true);
    paintToolButton->setEvaluateOnChange(false);
    paintToolButton->setSecretByDefault(true);
    paintToolButton->setInViewerContextCanHaveShortcut(true);
    paintToolButton->setIsPersistent(false);
    toolbar->addKnob(paintToolButton);
    _imp->ui->paintBrushToolGroup = paintToolButton;

    KnobGroupPtr cloneToolButton, effectToolButton, mergeToolButton;
    if (_imp->isPaintByDefault) {
        cloneToolButton = AppManager::createKnob<KnobGroup>( this, tr(kRotoUIParamCloneBrushToolButtonLabel) );
        cloneToolButton->setName(kRotoUIParamCloneBrushToolButton);
        cloneToolButton->setAsToolButton(true);
        cloneToolButton->setEvaluateOnChange(false);
        cloneToolButton->setSecretByDefault(true);
        cloneToolButton->setInViewerContextCanHaveShortcut(true);
        cloneToolButton->setIsPersistent(false);
        toolbar->addKnob(cloneToolButton);
        _imp->ui->cloneBrushToolGroup = cloneToolButton;
        effectToolButton = AppManager::createKnob<KnobGroup>( this, tr(kRotoUIParamEffectBrushToolButtonLabel) );
        effectToolButton->setName(kRotoUIParamEffectBrushToolButton);
        effectToolButton->setAsToolButton(true);
        effectToolButton->setEvaluateOnChange(false);
        effectToolButton->setSecretByDefault(true);
        effectToolButton->setInViewerContextCanHaveShortcut(true);
        effectToolButton->setIsPersistent(false);
        toolbar->addKnob(effectToolButton);
        _imp->ui->effectBrushToolGroup = effectToolButton;
        mergeToolButton = AppManager::createKnob<KnobGroup>( this, tr(kRotoUIParamMergeBrushToolButtonLabel) );
        mergeToolButton->setName(kRotoUIParamMergeBrushToolButton);
        mergeToolButton->setAsToolButton(true);
        mergeToolButton->setEvaluateOnChange(false);
        mergeToolButton->setSecretByDefault(true);
        mergeToolButton->setInViewerContextCanHaveShortcut(true);
        mergeToolButton->setIsPersistent(false);
        toolbar->addKnob(mergeToolButton);
        _imp->ui->mergeBrushToolGroup = mergeToolButton;
    }


    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamSelectAllToolButtonActionLabel) );
        tool->setName(kRotoUIParamSelectAllToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSelectAllToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(true);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "cursor.png");
        tool->setIsPersistent(false);
        selectionToolButton->addKnob(tool);
        _imp->ui->selectAllAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamSelectPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamSelectPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSelectPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "selectPoints.png");
        tool->setIsPersistent(false);
        selectionToolButton->addKnob(tool);
        _imp->ui->selectPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamSelectShapesToolButtonActionLabel) );
        tool->setName(kRotoUIParamSelectShapesToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSelectShapesToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "selectCurves.png");
        tool->setIsPersistent(false);
        selectionToolButton->addKnob(tool);
        _imp->ui->selectCurvesAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamSelectFeatherPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamSelectFeatherPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSelectFeatherPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "selectFeather.png");
        tool->setIsPersistent(false);
        selectionToolButton->addKnob(tool);
        _imp->ui->selectFeatherPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamAddPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamAddPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamAddPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(true);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "addPoints.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->addPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRemovePointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamRemovePointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamRemovePointsToolButtonAction) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "removePoints.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->removePointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamCuspPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamCuspPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamCuspPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "cuspPoints.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->cuspPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamSmoothPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamSmoothPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSmoothPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "smoothPoints.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->smoothPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamOpenCloseCurveToolButtonActionLabel) );
        tool->setName(kRotoUIParamOpenCloseCurveToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamOpenCloseCurveToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "openCloseCurve.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->openCloseCurveAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRemoveFeatherToolButtonAction) );
        tool->setName(kRotoUIParamRemoveFeatherToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamRemoveFeatherToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "removeFeather.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->removeFeatherAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamDrawBezierToolButtonActionLabel) );
        tool->setName(kRotoUIParamDrawBezierToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamDrawBezierToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(true);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "bezier32.png");
        tool->setIsPersistent(false);
        editBezierToolButton->addKnob(tool);
        _imp->ui->drawBezierAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamDrawEllipseToolButtonActionLabel) );
        tool->setName(kRotoUIParamDrawEllipseToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamDrawEllipseToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "ellipse.png");
        tool->setIsPersistent(false);
        editBezierToolButton->addKnob(tool);
        _imp->ui->drawEllipseAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamDrawRectangleToolButtonActionLabel) );
        tool->setName(kRotoUIParamDrawRectangleToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamDrawRectangleToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "rectangle.png");
        tool->setIsPersistent(false);
        editBezierToolButton->addKnob(tool);
        _imp->ui->drawRectangleAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamDrawBrushToolButtonActionLabel) );
        tool->setName(kRotoUIParamDrawBrushToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamDrawBrushToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(true);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_solid.png");
        tool->setIsPersistent(false);
        paintToolButton->addKnob(tool);
        _imp->ui->brushAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamPencilToolButtonActionLabel) );
        tool->setName(kRotoUIParamPencilToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamPencilToolButtonAction) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "rotoToolPencil.png");
        tool->setIsPersistent(false);
        paintToolButton->addKnob(tool);
        _imp->ui->pencilAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamEraserToolButtonActionLabel) );
        tool->setName(kRotoUIParamEraserToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamEraserToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecretByDefault(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_eraser.png");
        tool->setIsPersistent(false);
        paintToolButton->addKnob(tool);
        _imp->ui->eraserAction = tool;
    }
    if (_imp->isPaintByDefault) {
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamCloneToolButtonActionLabel) );
            tool->setName(kRotoUIParamCloneToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamCloneToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(true);
            tool->setSecretByDefault(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_clone.png");
            tool->setIsPersistent(false);
            cloneToolButton->addKnob(tool);
            _imp->ui->cloneAction = tool;
        }
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRevealToolButtonAction) );
            tool->setName(kRotoUIParamRevealToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamRevealToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(false);
            tool->setSecretByDefault(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_reveal.png");
            tool->setIsPersistent(false);
            cloneToolButton->addKnob(tool);
            _imp->ui->revealAction = tool;
        }


        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamBlurToolButtonActionLabel) );
            tool->setName(kRotoUIParamBlurToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamBlurToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(true);
            tool->setSecretByDefault(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_blur.png");
            tool->setIsPersistent(false);
            effectToolButton->addKnob(tool);
            _imp->ui->blurAction = tool;
        }
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamSmearToolButtonActionLabel) );
            tool->setName(kRotoUIParamSmearToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamSmearToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(false);
            tool->setSecretByDefault(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_smear.png");
            tool->setIsPersistent(false);
            effectToolButton->addKnob(tool);
            _imp->ui->smearAction = tool;
        }
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamDodgeToolButtonActionLabel) );
            tool->setName(kRotoUIParamDodgeToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamDodgeToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(true);
            tool->setSecretByDefault(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_dodge.png");
            tool->setIsPersistent(false);
            mergeToolButton->addKnob(tool);
            _imp->ui->dodgeAction = tool;
        }
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamBurnToolButtonActionLabel) );
            tool->setName(kRotoUIParamBurnToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamBurnToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(false);
            tool->setSecretByDefault(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_burn.png");
            tool->setIsPersistent(false);
            mergeToolButton->addKnob(tool);
            _imp->ui->burnAction = tool;
        }
    } // if (_imp->isPaintByDefault) {

    // Right click menu
    KnobChoicePtr rightClickMenu = AppManager::createKnob<KnobChoice>( this, std::string(kRotoUIParamRightClickMenu) );
    rightClickMenu->setSecretByDefault(true);
    rightClickMenu->setEvaluateOnChange(false);
    generalPage->addKnob(rightClickMenu);
    _imp->ui->rightClickMenuKnob = rightClickMenu;
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionRemoveItemsLabel) );
        action->setName(kRotoUIParamRightClickMenuActionRemoveItems);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->removeItemsMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionCuspItemsLabel) );
        action->setName(kRotoUIParamRightClickMenuActionCuspItems);
        action->setEvaluateOnChange(false);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->cuspItemMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionSmoothItemsLabel) );
        action->setName(kRotoUIParamRightClickMenuActionSmoothItems);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->smoothItemMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionRemoveItemsFeatherLabel) );
        action->setName(kRotoUIParamRightClickMenuActionRemoveItemsFeather);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->removeItemFeatherMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionNudgeBottomLabel) );
        action->setName(kRotoUIParamRightClickMenuActionNudgeBottom);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->nudgeBottomMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionNudgeLeftLabel) );
        action->setName(kRotoUIParamRightClickMenuActionNudgeLeft);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->nudgeLeftMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionNudgeTopLabel) );
        action->setName(kRotoUIParamRightClickMenuActionNudgeTop);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->nudgeTopMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionNudgeRightLabel) );
        action->setName(kRotoUIParamRightClickMenuActionNudgeRight);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->nudgeRightMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionSelectAllLabel) );
        action->setName(kRotoUIParamRightClickMenuActionSelectAll);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        generalPage->addKnob(action);
        _imp->ui->selectAllMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionOpenCloseLabel) );
        action->setName(kRotoUIParamRightClickMenuActionOpenClose);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->openCloseMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kRotoUIParamRightClickMenuActionLockShapesLabel) );
        action->setName(kRotoUIParamRightClickMenuActionLockShapes);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->lockShapeMenuAction = action;
    }

    KnobButtonPtr defaultAction;
    KnobGroupPtr defaultRole;
    if (_imp->isPaintByDefault) {
        defaultAction = _imp->ui->brushAction.lock();
        defaultRole = _imp->ui->paintBrushToolGroup.lock();
    } else {
        defaultAction = _imp->ui->drawBezierAction.lock();
        defaultRole = _imp->ui->bezierEditionToolGroup.lock();
    }
    _imp->ui->setCurrentTool(defaultAction);
    _imp->ui->onRoleChangedInternal(defaultRole);
    _imp->ui->onToolChangedInternal(defaultAction);
} // RotoPaint::initializeKnobs

void
RotoPaint::getPluginShortcuts(std::list<PluginActionShortcut>* shortcuts)
{
    // Viewer buttons
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamAutoKeyingEnabled, kRotoUIParamAutoKeyingEnabledLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamFeatherLinkEnabled, kRotoUIParamFeatherLinkEnabledLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamDisplayFeather, kRotoUIParamDisplayFeatherLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamStickySelectionEnabled, kRotoUIParamStickySelectionEnabledLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamStickyBbox, kRotoUIParamStickyBboxLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRippleEdit, kRotoUIParamRippleEditLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamAddKeyFrame, kRotoUIParamAddKeyFrameLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRemoveKeyframe, kRotoUIParamRemoveKeyframeLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamShowTransform, kRotoUIParamShowTransformLabel, Key_T) );

    shortcuts->push_back( PluginActionShortcut(kRotoUIParamPressureOpacity, kRotoUIParamPressureOpacityLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamPressureSize, kRotoUIParamPressureSizeLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamPressureHardness, kRotoUIParamPressureHardnessLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamBuildUp, kRotoUIParamBuildUpLabel) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamResetCloneOffset, kRotoUIParamResetCloneOffsetLabel) );

    // Toolbuttons
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamSelectionToolButton, kRotoUIParamSelectionToolButtonLabel, Key_Q) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamEditPointsToolButton, kRotoUIParamEditPointsToolButtonLabel, Key_D) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamBezierEditionToolButton, kRotoUIParamBezierEditionToolButtonLabel, Key_V) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamPaintBrushToolButton, kRotoUIParamPaintBrushToolButtonLabel, Key_N) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamCloneBrushToolButton, kRotoUIParamCloneBrushToolButtonLabel, Key_C) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamEffectBrushToolButton, kRotoUIParamEffectBrushToolButtonLabel, Key_X) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamMergeBrushToolButton, kRotoUIParamMergeBrushToolButtonLabel, Key_E) );

    // Right click actions
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionRemoveItems, kRotoUIParamRightClickMenuActionRemoveItemsLabel, Key_BackSpace) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionCuspItems, kRotoUIParamRightClickMenuActionCuspItemsLabel, Key_Z, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionSmoothItems, kRotoUIParamRightClickMenuActionSmoothItemsLabel, Key_Z) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionRemoveItemsFeather, kRotoUIParamRightClickMenuActionRemoveItemsFeatherLabel, Key_E, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionNudgeLeft, kRotoUIParamRightClickMenuActionNudgeLeftLabel, Key_4, eKeyboardModifierKeypad) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionNudgeBottom, kRotoUIParamRightClickMenuActionNudgeBottomLabel, Key_2, eKeyboardModifierKeypad) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionNudgeTop, kRotoUIParamRightClickMenuActionNudgeTopLabel, Key_8, eKeyboardModifierKeypad) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionNudgeRight, kRotoUIParamRightClickMenuActionNudgeRightLabel, Key_6, eKeyboardModifierKeypad) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionSelectAll, kRotoUIParamRightClickMenuActionSelectAllLabel, Key_A, eKeyboardModifierControl) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionOpenClose, kRotoUIParamRightClickMenuActionOpenCloseLabel, Key_Return) );
    shortcuts->push_back( PluginActionShortcut(kRotoUIParamRightClickMenuActionLockShapes, kRotoUIParamRightClickMenuActionLockShapesLabel, Key_L, eKeyboardModifierShift) );
}

bool
RotoPaint::shouldPreferPluginOverlayOverHostOverlay() const
{
    return !_imp->ui->ctrlDown;
}

bool
RotoPaint::shouldDrawHostOverlay() const
{
    KnobButtonPtr b = _imp->ui->showTransformHandle.lock();
    if (!b) {
        return true;
    }
    return b->getValue();
}

void
RotoPaint::onKnobsLoaded()
{
    _imp->ui->selectedItems = getNode()->getRotoContext()->getSelectedCurves();

    // When loading a roto node, always switch to selection mode by default
    KnobButtonPtr defaultAction = _imp->ui->selectAllAction.lock();
    KnobGroupPtr defaultRole = _imp->ui->selectToolGroup.lock();;

    _imp->ui->setCurrentTool(defaultAction);
    _imp->ui->onRoleChangedInternal(defaultRole);
    _imp->ui->onToolChangedInternal(defaultAction);
}

bool
RotoPaint::knobChanged(KnobI* k,
                       ValueChangedReasonEnum reason,
                       ViewSpec view,
                       double time,
                       bool originatedFromMainThread)
{
    if (!k) {
        return false;
    }


    RotoContextPtr ctx = getNode()->getRotoContext();

    if (!ctx) {
        return false;
    }
    bool ret = true;
    KnobIPtr kShared = k->shared_from_this();
    KnobButtonPtr isBtn = boost::dynamic_pointer_cast<KnobButton>(kShared);
    KnobGroupPtr isGrp = boost::dynamic_pointer_cast<KnobGroup>(kShared);
    if ( isBtn && _imp->ui->onToolChangedInternal(isBtn) ) {
        return true;
    } else if ( isGrp && _imp->ui->onRoleChangedInternal(isGrp) ) {
        return true;
    } else if ( k == _imp->ui->featherLinkEnabledButton.lock().get() ) {
        ctx->onFeatherLinkChanged( _imp->ui->featherLinkEnabledButton.lock()->getValue() );
    } else if ( k == _imp->ui->autoKeyingEnabledButton.lock().get() ) {
        ctx->onAutoKeyingChanged( _imp->ui->autoKeyingEnabledButton.lock()->getValue() );
    } else if ( k == _imp->ui->rippleEditEnabledButton.lock().get() ) {
        ctx->onRippleEditChanged( _imp->ui->rippleEditEnabledButton.lock()->getValue() );
    } else if ( k == _imp->ui->colorWheelButton.lock().get() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->pressureOpacityButton.lock().get() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->pressureSizeButton.lock().get() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->hardnessSpinbox.lock().get() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->buildUpButton.lock().get() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->resetCloneOffsetButton.lock().get() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
        _imp->ui->cloneOffset.first = _imp->ui->cloneOffset.second = 0;
    } else if ( k == _imp->ui->addKeyframeButton.lock().get() ) {
        for (SelectedItems::iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
            if (!isBezier) {
                continue;
            }
            isBezier->setKeyframe(time);
        }
        getNode()->getRotoContext()->evaluateChange();
    } else if ( k == _imp->ui->removeKeyframeButton.lock().get() ) {
        for (SelectedItems::iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
            if (!isBezier) {
                continue;
            }
            isBezier->removeKeyframe(time);
        }
        getNode()->getRotoContext()->evaluateChange();
    } else if ( k == _imp->ui->removeItemsMenuAction.lock().get() ) {
        ///if control points are selected, delete them, otherwise delete the selected beziers
        if ( !_imp->ui->selectedCps.empty() ) {
            pushUndoCommand( new RemovePointUndoCommand(_imp->ui, _imp->ui->selectedCps) );
        } else if ( !_imp->ui->selectedItems.empty() ) {
            pushUndoCommand( new RemoveCurveUndoCommand(_imp->ui, _imp->ui->selectedItems) );
        }
    } else if ( k == _imp->ui->smoothItemMenuAction.lock().get() ) {
        if ( !_imp->ui->smoothSelectedCurve() ) {
            return false;
        }
    } else if ( k == _imp->ui->cuspItemMenuAction.lock().get() ) {
        if ( !_imp->ui->cuspSelectedCurve() ) {
            return false;
        }
    } else if ( k == _imp->ui->removeItemFeatherMenuAction.lock().get() ) {
        if ( !_imp->ui->removeFeatherForSelectedCurve() ) {
            return false;
        }
    } else if ( k == _imp->ui->nudgeLeftMenuAction.lock().get() ) {
        if ( !_imp->ui->moveSelectedCpsWithKeyArrows(-1, 0) ) {
            return false;
        }
    } else if ( k == _imp->ui->nudgeRightMenuAction.lock().get() ) {
        if ( !_imp->ui->moveSelectedCpsWithKeyArrows(1, 0) ) {
            return false;
        }
    } else if ( k == _imp->ui->nudgeBottomMenuAction.lock().get() ) {
        if ( !_imp->ui->moveSelectedCpsWithKeyArrows(0, -1) ) {
            return false;
        }
    } else if ( k == _imp->ui->nudgeTopMenuAction.lock().get() ) {
        if ( !_imp->ui->moveSelectedCpsWithKeyArrows(0, 1) ) {
            return false;
        }
    } else if ( k == _imp->ui->selectAllMenuAction.lock().get() ) {
        _imp->ui->iSelectingwithCtrlA = true;
        ///if no bezier are selected, select all beziers
        if ( _imp->ui->selectedItems.empty() ) {
            std::list<RotoDrawableItemPtr> bez = ctx->getCurvesByRenderOrder();
            for (std::list<RotoDrawableItemPtr>::const_iterator it = bez.begin(); it != bez.end(); ++it) {
                ctx->select(*it, RotoItem::eSelectionReasonOverlayInteract);
                _imp->ui->selectedItems.push_back(*it);
            }
        } else {
            ///select all the control points of all selected beziers
            _imp->ui->selectedCps.clear();
            for (SelectedItems::iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
                Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
                if (!isBezier) {
                    continue;
                }
                const std::list<BezierCPPtr> & cps = isBezier->getControlPoints();
                const std::list<BezierCPPtr> & fps = isBezier->getFeatherPoints();
                assert( cps.size() == fps.size() );

                std::list<BezierCPPtr>::const_iterator cpIT = cps.begin();
                for (std::list<BezierCPPtr>::const_iterator fpIT = fps.begin(); fpIT != fps.end(); ++fpIT, ++cpIT) {
                    _imp->ui->selectedCps.push_back( std::make_pair(*cpIT, *fpIT) );
                }
            }
            _imp->ui->computeSelectedCpsBBOX();
        }
    } else if ( k == _imp->ui->openCloseMenuAction.lock().get() ) {
        if ( ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) && _imp->ui->builtBezier && !_imp->ui->builtBezier->isCurveFinished() ) {
            pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, _imp->ui->builtBezier) );

            _imp->ui->builtBezier.reset();
            _imp->ui->selectedCps.clear();
            _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
            _imp->ui->getContext()->evaluateChange();
        }
    } else if ( k == _imp->ui->lockShapeMenuAction.lock().get() ) {
        _imp->ui->lockSelectedCurves();
    } else {
        ret = false;
    }

    if (!ret) {
        ret |= ctx->knobChanged(k, reason, view, time, originatedFromMainThread);
    }

    return ret;
} // RotoPaint::knobChanged

void
RotoPaint::refreshExtraStateAfterTimeChanged(bool isPlayback,
                                             double time)
{
    EffectInstance::refreshExtraStateAfterTimeChanged(isPlayback, time);
    if (time != _imp->ui->strokeBeingPaintedTimelineFrame) {
        if ( (_imp->ui->selectedTool == eRotoToolBlur) ||
            ( _imp->ui->selectedTool == eRotoToolBurn) ||
            ( _imp->ui->selectedTool == eRotoToolDodge) ||
            ( _imp->ui->selectedTool == eRotoToolClone) ||
            ( _imp->ui->selectedTool == eRotoToolEraserBrush) ||
            ( _imp->ui->selectedTool == eRotoToolSolidBrush) ||
            ( _imp->ui->selectedTool == eRotoToolReveal) ||
            ( _imp->ui->selectedTool == eRotoToolSmear) ||
            ( _imp->ui->selectedTool == eRotoToolSharpen) ) {
            _imp->ui->onBreakMultiStrokeTriggered();
        }
    }
    _imp->ui->computeSelectedCpsBBOX();
}

StatusEnum
RotoPaint::getPreferredMetadata(NodeMetadata& metadata)
{
    metadata.setNComps( -1, 4 );
    metadata.setComponentsType(-1, kNatronColorPlaneID);
    /*KnobBoolPtr premultKnob = _imp->premultKnob.lock();
       assert(premultKnob);
       bool premultiply = premultKnob->getValue();
       if (premultiply) {
        metadata.setOutputPremult(eImagePremultiplicationPremultiplied);
       } else {
        ImagePremultiplicationEnum srcPremult = eImagePremultiplicationOpaque;
        EffectInstancePtr input = getInput(0);
        if (input) {
            srcPremult = input->getPremult();
        }
        bool processA = getNode()->getProcessChannel(3);
        if ( (srcPremult == eImagePremultiplicationOpaque) && processA ) {
            metadata.setOutputPremult(eImagePremultiplicationUnPremultiplied);
        } else {
            metadata.setOutputPremult(eImagePremultiplicationPremultiplied);
        }
       }*/
    metadata.setOutputPremult(eImagePremultiplicationPremultiplied);

    return eStatusOK;
}

void
RotoPaint::onInputChanged(int inputNb)
{
    NodePtr inputNode = getNode()->getInput(0);

    getNode()->getRotoContext()->onRotoPaintInputChanged(inputNode);
    EffectInstance::onInputChanged(inputNb);
}

StatusEnum
RotoPaint::getRegionOfDefinition(U64 hash,
                                 double time,
                                 const RenderScale & scale,
                                 ViewIdx view,
                                 RectD* rod)
{
    StatusEnum st = EffectInstance::getRegionOfDefinition(hash, time, scale, view, rod);

    if (st != eStatusOK) {
        rod->x1 = rod->y1 = rod->x2 = rod->y2 = 0.;
    }
    RectD maskRod;
    getNode()->getRotoContext()->getMaskRegionOfDefinition(time, view, &maskRod);
    if ( rod->isNull() ) {
        *rod = maskRod;
    } else {
        rod->merge(maskRod);
    }

    return eStatusOK;
}

FramesNeededMap
RotoPaint::getFramesNeeded(double time,
                           ViewIdx view)
{
    FramesNeededMap ret;
    FrameRangesMap views;
    OfxRangeD range;

    range.min = range.max = time;
    views[view].push_back(range);
    ret.insert( std::make_pair(0, views) );

    return ret;
}

void
RotoPaint::getRegionsOfInterest(double time,
                                const RenderScale & scale,
                                const RectD & outputRoD, //!< the RoD of the effect, in canonical coordinates
                                const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                ViewIdx view,
                                RoIMap* ret)
{
    RotoContextPtr roto = getNode()->getRotoContext();
    NodePtr bottomMerge = roto->getRotoPaintBottomMergeNode();

    if (bottomMerge) {
        ret->insert( std::make_pair(bottomMerge->getEffectInstance(), renderWindow) );
    }
    EffectInstance::getRegionsOfInterest(time, scale, outputRoD, renderWindow, view, ret);
}

bool
RotoPaint::isIdentity(double time,
                      const RenderScale & scale,
                      const RectI & roi,
                      ViewIdx view,
                      double* inputTime,
                      ViewIdx* inputView,
                      int* inputNb)
{
    *inputView = view;
    NodePtr node = getNode();
    EffectInstancePtr maskInput = getInput(ROTOPAINT_MASK_INPUT_INDEX);
    if (maskInput) {
        RectD maskRod;
        bool isProjectFormat;
        StatusEnum s = maskInput->getRegionOfDefinition_public(maskInput->getRenderHash(), time, scale, view, &maskRod, &isProjectFormat);
        Q_UNUSED(s);
        RectI maskPixelRod;
        maskRod.toPixelEnclosing(scale, getAspectRatio(ROTOPAINT_MASK_INPUT_INDEX), &maskPixelRod);
        if ( !maskPixelRod.intersects(roi) ) {
            *inputTime = time;
            *inputNb = 0;

            return true;
        }
    }

    std::list<RotoDrawableItemPtr> items = node->getRotoContext()->getCurvesByRenderOrder();
    if ( items.empty() ) {
        *inputNb = 0;
        *inputTime = time;

        return true;
    }

    return false;
}

StatusEnum
RotoPaint::render(const RenderActionArgs& args)
{
    RotoContextPtr roto = getNode()->getRotoContext();
    std::list<RotoDrawableItemPtr> items = roto->getCurvesByRenderOrder(false /*onlyActiveItems*/);
    ImageBitDepthEnum bgDepth = getBitDepth(0);
    std::list<ImagePlaneDesc> neededComps;

    for (std::list<std::pair<ImagePlaneDesc, ImagePtr> >::const_iterator plane = args.outputPlanes.begin();
         plane != args.outputPlanes.end(); ++plane) {
        neededComps.push_back(plane->first);
    }

    KnobBoolPtr premultKnob = _imp->premultKnob.lock();
    assert(premultKnob);
    bool premultiply = premultKnob->getValueAtTime(args.time);

    if ( items.empty() ) {
        RectI bgImgRoI;
        ImagePtr bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, 0, false /*mapToClipPrefs*/, false /*dontUpscale*/, eStorageModeRAM /*returnOpenGLtexture*/, 0 /*textureDepth*/, &bgImgRoI);

        for (std::list<std::pair<ImagePlaneDesc, ImagePtr> >::const_iterator plane = args.outputPlanes.begin();
             plane != args.outputPlanes.end(); ++plane) {
            if (bgImg) {
                if ( bgImg->getComponents() != plane->second->getComponents() ) {
                    bgImg->convertToFormat( args.roi,
                                            getApp()->getDefaultColorSpaceForBitDepth( bgImg->getBitDepth() ),
                                            getApp()->getDefaultColorSpaceForBitDepth( plane->second->getBitDepth() ), 3
                                            , false, false, plane->second.get() );
                } else {
                    plane->second->pasteFrom(*bgImg, args.roi, false);
                }


                if ( premultiply && ( plane->second->getComponents() == ImagePlaneDesc::getRGBAComponents() ) ) {
                    plane->second->premultImage(args.roi);
                }
            } else {
                plane->second->fillZero(args.roi);
            }
        }
    } else {
        NodesList rotoPaintNodes;
        {
            bool ok = getThreadLocalRotoPaintTreeNodes(&rotoPaintNodes);
            if (!ok) {
                throw std::logic_error("RotoPaint::render(): getThreadLocalRotoPaintTreeNodes() failed");
            }
        }
        NodePtr bottomMerge = roto->getRotoPaintBottomMergeNode();
        RenderingFlagSetter flagIsRendering( bottomMerge );
        std::bitset<4> copyChannels;
        for (int i = 0; i < 4; ++i) {
            copyChannels[i] = _imp->enabledKnobs[i].lock()->getValue();
        }

        unsigned int mipMapLevel = Image::getLevelFromScale(args.mappedScale.x);
        RenderRoIArgs rotoPaintArgs(args.time,
                                    args.mappedScale,
                                    mipMapLevel,
                                    args.view,
                                    args.byPassCache,
                                    args.roi,
                                    RectD(),
                                    neededComps,
                                    bgDepth,
                                    false,
                                    this,
                                    eStorageModeRAM /*returnOpenGLtex*/,
                                    args.time);
        std::map<ImagePlaneDesc, ImagePtr> rotoPaintImages;
        RenderRoIRetCode code = bottomMerge->getEffectInstance()->renderRoI(rotoPaintArgs, &rotoPaintImages);
        if (code == eRenderRoIRetCodeFailed) {
            return eStatusFailed;
        } else if (code == eRenderRoIRetCodeAborted) {
            return eStatusOK;
        } else if ( rotoPaintImages.empty() ) {
            for (std::list<std::pair<ImagePlaneDesc, ImagePtr> >::const_iterator plane = args.outputPlanes.begin();
                 plane != args.outputPlanes.end(); ++plane) {
                plane->second->fillZero(args.roi);
            }

            return eStatusOK;
        }
        assert( rotoPaintImages.size() == args.outputPlanes.size() );

        RectI bgImgRoI;
        ImagePtr bgImg;
        ImagePremultiplicationEnum outputPremult = getPremult();
        bool triedGetImage = false;

        for (std::list<std::pair<ImagePlaneDesc, ImagePtr> >::const_iterator plane = args.outputPlanes.begin();
             plane != args.outputPlanes.end(); ++plane) {
            std::map<ImagePlaneDesc, ImagePtr>::iterator rotoImagesIt = rotoPaintImages.find(plane->first);
            assert( rotoImagesIt != rotoPaintImages.end() );
            if ( rotoImagesIt == rotoPaintImages.end() ) {
                continue;
            }
            if (!bgImg) {
                if (!triedGetImage) {
                    bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, 0, false /*mapToClipPrefs*/, false /*dontUpscale*/, eStorageModeRAM /*returnOpenGLtexture*/, 0 /*textureDepth*/, &bgImgRoI);
                    triedGetImage = true;
                }
            }
            if ( !rotoImagesIt->second->getBounds().contains(args.roi) ) {
                // We first fill with the bg image because the bounds of the image produced by the last merge of the rotopaint tree
                // might not be equal to the bounds of the image produced by the rotopaint. This is because the RoD of the rotopaint is the
                // union of all the mask strokes bounds, whereas all nodes inside the rotopaint tree don't take the mask RoD into account.
                if (bgImg) {
                    RectI bgBounds = bgImg->getBounds();

                    // The bg bounds might not be inside the roi, but yet we need to fill the whole roi, so just fill borders
                    // with black and transparent, e.g:
                    /*
                        AAAAAAAAA
                        DDXXXXXBB
                        DDXXXXXBB
                        DDXXXXXBB
                        CCCCCCCCC
                     */
                    RectI merge = bgBounds;
                    merge.merge(args.roi);
                    RectI aRect;
                    aRect.x1 = merge.x1;
                    aRect.y1 = bgBounds.y2;
                    aRect.y2 = merge.y2;
                    aRect.x2 = merge.x2;

                    RectI bRect;
                    bRect.x1 = bgBounds.x2;
                    bRect.y1 = bgBounds.y1;
                    bRect.x2 = merge.x2;
                    bRect.y2 = bgBounds.y2;

                    RectI cRect;
                    cRect.x1 = merge.x1;
                    cRect.y1 = merge.y1;
                    cRect.x2 = merge.x2;
                    cRect.y2 = bgBounds.y1;

                    RectI dRect;
                    dRect.x1 = merge.x1;
                    dRect.y1 = bgBounds.y1;
                    dRect.x2 = bgBounds.x1;
                    dRect.y2 = bgBounds.y2;

                    plane->second->fillZero(aRect);
                    plane->second->fillZero(bRect);
                    plane->second->fillZero(cRect);
                    plane->second->fillZero(dRect);

                    if ( bgImg->getComponents() != plane->second->getComponents() ) {
                        RectI intersection;
                        if (args.roi.intersect(bgImg->getBounds(), &intersection)) {
                            bgImg->convertToFormat( intersection,
                                                getApp()->getDefaultColorSpaceForBitDepth( rotoImagesIt->second->getBitDepth() ),
                                                getApp()->getDefaultColorSpaceForBitDepth( plane->second->getBitDepth() ), 3
                                                , false, false, plane->second.get() );
                        }
                    } else {
                        plane->second->pasteFrom(*bgImg, args.roi, false);
                    }
                } else {
                    plane->second->fillZero(args.roi);
                }
            }


            if ( rotoImagesIt->second->getComponents() != plane->second->getComponents() ) {
                rotoImagesIt->second->convertToFormat( args.roi,
                                                       getApp()->getDefaultColorSpaceForBitDepth( rotoImagesIt->second->getBitDepth() ),
                                                       getApp()->getDefaultColorSpaceForBitDepth( plane->second->getBitDepth() ), 3
                                                       , false, false, plane->second.get() );
            } else {
                plane->second->pasteFrom(*(rotoImagesIt->second), args.roi, false);
            }
            plane->second->copyUnProcessedChannels(args.roi, outputPremult, bgImg ? bgImg->getPremultiplication() : eImagePremultiplicationOpaque, copyChannels, bgImg, false);
            if ( premultiply && ( plane->second->getComponents() == ImagePlaneDesc::getRGBAComponents() ) ) {
                plane->second->premultImage(args.roi);
            }
        }
    } // RenderingFlagSetter

    return eStatusOK;
} // RotoPaint::render

void
RotoPaint::clearLastRenderedImage()
{
    EffectInstance::clearLastRenderedImage();
    NodesList rotoPaintNodes;
    NodePtr node = getNode();

    if (node) {
        RotoContextPtr roto = node->getRotoContext();
        assert(roto);
        roto->getRotoPaintTreeNodes(&rotoPaintNodes);
        for (NodesList::iterator it = rotoPaintNodes.begin(); it != rotoPaintNodes.end(); ++it) {
            (*it)->clearLastRenderedImage();
        }
    }
}

void
RotoPaint::drawOverlay(double time,
                       const RenderScale & /*renderScale*/,
                       ViewIdx /*view*/)
{
    std::list<RotoDrawableItemPtr> drawables = getNode()->getRotoContext()->getCurvesByRenderOrder();
    std::pair<double, double> pixelScale;
    std::pair<double, double> viewportSize;

    getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);
    getCurrentViewportForOverlays()->getViewportSize(viewportSize.first, viewportSize.second);

    bool featherVisible = _imp->ui->isFeatherVisible();

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        glLineWidth(1.5);


        double cpWidth = kControlPointMidSize * 2;
        glPointSize(cpWidth);
        for (std::list<RotoDrawableItemPtr>::const_iterator it = drawables.begin(); it != drawables.end(); ++it) {
            if ( !(*it)->isGloballyActivated() ) {
                continue;
            }


            Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( it->get() );
            if (isStroke) {
                if (_imp->ui->selectedTool != eRotoToolSelectAll) {
                    continue;
                }

                bool selected = false;
                for (SelectedItems::const_iterator it2 = _imp->ui->selectedItems.begin(); it2 != _imp->ui->selectedItems.end(); ++it2) {
                    if (it2->get() == isStroke) {
                        selected = true;
                        break;
                    }
                }
                if (!selected) {
                    continue;
                }

                std::list<std::list<std::pair<Point, double> > > strokes;
                isStroke->evaluateStroke(0, time, &strokes);
                bool locked = (*it)->isLockedRecursive();
                double curveColor[4];
                if (!locked) {
                    (*it)->getOverlayColor(curveColor);
                } else {
                    curveColor[0] = 0.8; curveColor[1] = 0.8; curveColor[2] = 0.8; curveColor[3] = 1.;
                }
                glColor4dv(curveColor);

                for (std::list<std::list<std::pair<Point, double> > >::iterator itStroke = strokes.begin(); itStroke != strokes.end(); ++itStroke) {
                    glBegin(GL_LINE_STRIP);
                    for (std::list<std::pair<Point, double> >::const_iterator it2 = itStroke->begin(); it2 != itStroke->end(); ++it2) {
                        glVertex2f(it2->first.x, it2->first.y);
                    }
                    glEnd();
                }
            } else if (isBezier) {
                ///draw the bezier
                // check if the bbox is visible
                // if the bbox is visible, compute the polygon and draw it.


                RectD bbox = isBezier->getBoundingBox(time);

                // To decomment you must transform the viewport by the OpenGL transform first
                //if ( !getCurrentViewportForOverlays()->isVisibleInViewport(bbox) ) {
                //                  continue;
                //            }

                std::list<ParametricPoint > points;
                isBezier->evaluateAtTime_DeCasteljau(true, time, 0,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                     100,
#else
                                                     1,
#endif
                                                     &points, NULL);

                bool locked = (*it)->isLockedRecursive();
                double curveColor[4];
                if (!locked) {
                    (*it)->getOverlayColor(curveColor);
                } else {
                    curveColor[0] = 0.8; curveColor[1] = 0.8; curveColor[2] = 0.8; curveColor[3] = 1.;
                }
                glColor4dv(curveColor);

                glBegin(GL_LINE_STRIP);
                for (std::list<ParametricPoint >::const_iterator it2 = points.begin(); it2 != points.end(); ++it2) {
                    glVertex2f(it2->x, it2->y);
                }
                glEnd();

                ///draw the feather points
                std::list<ParametricPoint > featherPoints;
                RectD featherBBox( std::numeric_limits<double>::infinity(),
                                   std::numeric_limits<double>::infinity(),
                                   -std::numeric_limits<double>::infinity(),
                                   -std::numeric_limits<double>::infinity() );
                bool clockWise = isBezier->isFeatherPolygonClockwiseOriented(true, time);


                if (featherVisible) {
                    ///Draw feather only if visible (button is toggled in the user interface)
                    isBezier->evaluateFeatherPointsAtTime_DeCasteljau(true, time, 0,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                                      100,
#else
                                                                      1,
#endif
                                                                      true, &featherPoints, &featherBBox);

                    if ( !featherPoints.empty() ) {
                        glLineStipple(2, 0xAAAA);
                        glEnable(GL_LINE_STIPPLE);
                        glBegin(GL_LINE_STRIP);
                        for (std::list<ParametricPoint >::const_iterator it2 = featherPoints.begin(); it2 != featherPoints.end(); ++it2) {
                            glVertex2f(it2->x, it2->y);
                        }
                        glEnd();
                        glDisable(GL_LINE_STIPPLE);
                    }
                }

                ///draw the control points if the bezier is selected
                bool selected = false;
                for (SelectedItems::const_iterator it2 = _imp->ui->selectedItems.begin(); it2 != _imp->ui->selectedItems.end(); ++it2) {
                    if (it2->get() == isBezier) {
                        selected = true;
                        break;
                    }
                }


                if (selected && !locked) {
                    Transform::Matrix3x3 transform;
                    isBezier->getTransformAtTime(time, &transform);

                    const std::list<BezierCPPtr> & cps = isBezier->getControlPoints();
                    const std::list<BezierCPPtr> & featherPts = isBezier->getFeatherPoints();
                    assert( cps.size() == featherPts.size() );

                    if ( cps.empty() ) {
                        continue;
                    }


                    glColor3d(0.85, 0.67, 0.);

                    std::list<BezierCPPtr>::const_iterator itF = featherPts.begin();
                    int index = 0;
                    std::list<BezierCPPtr>::const_iterator prevCp = cps.end();
                    if ( prevCp != cps.begin() ) {
                        --prevCp;
                    }
                    std::list<BezierCPPtr>::const_iterator nextCp = cps.begin();
                    if ( nextCp != cps.end() ) {
                        ++nextCp;
                    }
                    for (std::list<BezierCPPtr>::const_iterator it2 = cps.begin(); it2 != cps.end();
                         ++it2) {
                        if ( nextCp == cps.end() ) {
                            nextCp = cps.begin();
                        }
                        if ( prevCp == cps.end() ) {
                            prevCp = cps.begin();
                        }
                        assert( itF != featherPts.end() ); // because cps.size() == featherPts.size()
                        if ( itF == featherPts.end() ) {
                            break;
                        }
                        double x, y;
                        Transform::Point3D p, pF;
                        (*it2)->getPositionAtTime(true, time, ViewIdx(0), &p.x, &p.y);
                        p.z = 1.;

                        double xF, yF;
                        (*itF)->getPositionAtTime(true, time,  ViewIdx(0), &pF.x, &pF.y);
                        pF.z = 1.;

                        p = Transform::matApply(transform, p);
                        pF = Transform::matApply(transform, pF);

                        x = p.x;
                        y = p.y;
                        xF = pF.x;
                        yF = pF.y;

                        ///draw the feather point only if it is distinct from the associated point
                        bool drawFeather = featherVisible;
                        if (featherVisible) {
                            drawFeather = !(*it2)->equalsAtTime(true, time, ViewIdx(0), **itF);
                        }


                        ///if the control point is the only control point being dragged, color it to identify it to the user
                        bool colorChanged = false;
                        SelectedCPs::const_iterator firstSelectedCP = _imp->ui->selectedCps.begin();
                        if ( ( firstSelectedCP != _imp->ui->selectedCps.end() ) &&
                             ( ( firstSelectedCP->first == *it2) || ( firstSelectedCP->second == *it2) ) &&
                             ( _imp->ui->selectedCps.size() == 1) &&
                             ( ( _imp->ui->state == eEventStateDraggingSelectedControlPoints) || ( _imp->ui->state == eEventStateDraggingControlPoint) ) ) {
                            glColor3f(0., 1., 1.);
                            colorChanged = true;
                        }

                        for (SelectedCPs::const_iterator cpIt = _imp->ui->selectedCps.begin();
                             cpIt != _imp->ui->selectedCps.end();
                             ++cpIt) {
                            ///if the control point is selected, draw its tangent handles
                            if (cpIt->first == *it2) {
                                _imp->ui->drawSelectedCp(time, cpIt->first, x, y, transform);
                                if (drawFeather) {
                                    _imp->ui->drawSelectedCp(time, cpIt->second, xF, yF, transform);
                                }
                                glColor3f(0.2, 1., 0.);
                                colorChanged = true;
                                break;
                            } else if (cpIt->second == *it2) {
                                _imp->ui->drawSelectedCp(time, cpIt->second, x, y, transform);
                                if (drawFeather) {
                                    _imp->ui->drawSelectedCp(time, cpIt->first, xF, yF, transform);
                                }
                                glColor3f(0.2, 1., 0.);
                                colorChanged = true;
                                break;
                            }
                        } // for(cpIt)

                        glBegin(GL_POINTS);
                        glVertex2f(x,y);
                        glEnd();

                        if (colorChanged) {
                            glColor3d(0.85, 0.67, 0.);
                        }

                        if ( (firstSelectedCP->first == *itF)
                             && ( _imp->ui->selectedCps.size() == 1) &&
                             ( ( _imp->ui->state == eEventStateDraggingSelectedControlPoints) || ( _imp->ui->state == eEventStateDraggingControlPoint) )
                             && !colorChanged ) {
                            glColor3f(0.2, 1., 0.);
                            colorChanged = true;
                        }


                        double distFeatherX = 20. * pixelScale.first;
                        double distFeatherY = 20. * pixelScale.second;
                        bool isHovered = false;
                        if (_imp->ui->featherBarBeingHovered.first) {
                            assert(_imp->ui->featherBarBeingHovered.second);
                            if ( _imp->ui->featherBarBeingHovered.first->isFeatherPoint() ) {
                                isHovered = _imp->ui->featherBarBeingHovered.first == *itF;
                            } else if ( _imp->ui->featherBarBeingHovered.second->isFeatherPoint() ) {
                                isHovered = _imp->ui->featherBarBeingHovered.second == *itF;
                            }
                        }

                        if (drawFeather) {
                            glBegin(GL_POINTS);
                            glVertex2f(xF, yF);
                            glEnd();


                            if ( ( (_imp->ui->state == eEventStateDraggingFeatherBar) &&
                                   ( ( *itF == _imp->ui->featherBarBeingDragged.first) || ( *itF == _imp->ui->featherBarBeingDragged.second) ) ) ||
                                 isHovered ) {
                                glColor3f(0.2, 1., 0.);
                                colorChanged = true;
                            } else {
                                glColor4dv(curveColor);
                            }

                            double beyondX, beyondY;
                            double dx = (xF - x);
                            double dy = (yF - y);
                            double dist = sqrt(dx * dx + dy * dy);
                            beyondX = ( dx * (dist + distFeatherX) ) / dist + x;
                            beyondY = ( dy * (dist + distFeatherY) ) / dist + y;

                            ///draw a link between the feather point and the control point.
                            ///Also extend that link of 20 pixels beyond the feather point.

                            glBegin(GL_LINE_STRIP);
                            glVertex2f(x, y);
                            glVertex2f(xF, yF);
                            glVertex2f(beyondX, beyondY);
                            glEnd();

                            glColor3d(0.85, 0.67, 0.);
                        } else if (featherVisible) {
                            ///if the feather point is identical to the control point
                            ///draw a small hint line that the user can drag to move the feather point
                            if ( !isBezier->isOpenBezier() && ( (_imp->ui->selectedTool == eRotoToolSelectAll) || (_imp->ui->selectedTool == eRotoToolSelectFeatherPoints) ) ) {
                                int cpCount = (*it2)->getBezier()->getControlPointsCount();
                                if (cpCount > 1) {
                                    Point controlPoint;
                                    controlPoint.x = x;
                                    controlPoint.y = y;
                                    Point featherPoint;
                                    featherPoint.x = xF;
                                    featherPoint.y = yF;


                                    Bezier::expandToFeatherDistance(true, controlPoint, &featherPoint, distFeatherX, time, clockWise, transform, prevCp, it2, nextCp);

                                    if ( ( (_imp->ui->state == eEventStateDraggingFeatherBar) &&
                                           ( ( *itF == _imp->ui->featherBarBeingDragged.first) ||
                                             ( *itF == _imp->ui->featherBarBeingDragged.second) ) ) || isHovered ) {
                                        glColor3f(0.2, 1., 0.);
                                        colorChanged = true;
                                    } else {
                                        glColor4dv(curveColor);
                                    }

                                    glBegin(GL_LINES);
                                    glVertex2f(x, y);
                                    glVertex2f(featherPoint.x, featherPoint.y);
                                    glEnd();

                                    glColor3d(0.85, 0.67, 0.);
                                }
                            }
                        } // isFeatherVisible()


                        if (colorChanged) {
                            glColor3d(0.85, 0.67, 0.);
                        }

                        // increment for next iteration
                        if ( itF != featherPts.end() ) {
                            ++itF;
                        }
                        if ( nextCp != cps.end() ) {
                            ++nextCp;
                        }
                        if ( prevCp != cps.end() ) {
                            ++prevCp;
                        }
                        ++index;
                    } // for(it2)
                } // if ( ( selected != _imp->ui->selectedBeziers.end() ) && !locked ) {
            } // if (isBezier)
            glCheckError();
        } // for (std::list<RotoDrawableItemPtr>::const_iterator it = drawables.begin(); it != drawables.end(); ++it) {

        if ( _imp->isPaintByDefault &&
             ( ( _imp->ui->selectedRole == eRotoRoleMergeBrush) ||
               ( _imp->ui->selectedRole == eRotoRolePaintBrush) ||
               ( _imp->ui->selectedRole == eRotoRoleEffectBrush) ||
               ( _imp->ui->selectedRole == eRotoRoleCloneBrush) ) &&
             ( _imp->ui->selectedTool != eRotoToolOpenBezier) ) {
            Point cursorPos;
            getCurrentViewportForOverlays()->getCursorPosition(cursorPos.x, cursorPos.y);
            RectD viewportRect = getCurrentViewportForOverlays()->getViewportRect();

            if ( viewportRect.contains(cursorPos.x, cursorPos.y) ) {
                //Draw a circle  around the cursor
                double brushSize = _imp->ui->sizeSpinbox.lock()->getValue();
                GLdouble projection[16];
                glGetDoublev( GL_PROJECTION_MATRIX, projection);
                Point shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
                shadow.x = 2. / (projection[0] * viewportSize.first);
                shadow.y = 2. / (projection[5] * viewportSize.second);

                double halfBrush = brushSize / 2.;
                QPointF ellipsePos;
                if ( (_imp->ui->state == eEventStateDraggingBrushSize) || (_imp->ui->state == eEventStateDraggingBrushOpacity) ) {
                    ellipsePos = _imp->ui->mouseCenterOnSizeChange;
                } else {
                    ellipsePos = _imp->ui->lastMousePos;
                }
                double opacity = _imp->ui->opacitySpinbox.lock()->getValue();

                for (int l = 0; l < 2; ++l) {
                    glMatrixMode(GL_PROJECTION);
                    int direction = (l == 0) ? 1 : -1;
                    // translate (1,-1) pixels
                    glTranslated(direction * shadow.x, -direction * shadow.y, 0);
                    glMatrixMode(GL_MODELVIEW);
                    _imp->ui->drawEllipse(ellipsePos.x(), ellipsePos.y(), halfBrush, halfBrush, l, 1.f, 1.f, 1.f, opacity);

                    glColor3f(.5f * l * opacity, .5f * l * opacity, .5f * l * opacity);


                    if ( ( (_imp->ui->selectedTool == eRotoToolClone) || (_imp->ui->selectedTool == eRotoToolReveal) ) &&
                         ( ( _imp->ui->cloneOffset.first != 0) || ( _imp->ui->cloneOffset.second != 0) ) ) {
                        glBegin(GL_LINES);

                        if (_imp->ui->state == eEventStateDraggingCloneOffset) {
                            //draw a line between the center of the 2 ellipses
                            glVertex2d( ellipsePos.x(), ellipsePos.y() );
                            glVertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first, ellipsePos.y() + _imp->ui->cloneOffset.second);
                        }
                        //draw a cross in the center of the source ellipse
                        glVertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first, ellipsePos.y()  + _imp->ui->cloneOffset.second - halfBrush);
                        glVertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first, ellipsePos.y() +  _imp->ui->cloneOffset.second + halfBrush);
                        glVertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first - halfBrush, ellipsePos.y()  + _imp->ui->cloneOffset.second);
                        glVertex2d(ellipsePos.x() + _imp->ui->cloneOffset.first + halfBrush, ellipsePos.y()  + _imp->ui->cloneOffset.second);
                        glEnd();


                        //draw the source ellipse
                        _imp->ui->drawEllipse(ellipsePos.x() + _imp->ui->cloneOffset.first, ellipsePos.y() + _imp->ui->cloneOffset.second, halfBrush, halfBrush, l, 1.f, 1.f, 1.f, opacity / 2.);
                    }
                }
            }
        }
    } // GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

    if (_imp->ui->showCpsBbox) {
        _imp->ui->drawSelectedCpsBBOX();
    }
    glCheckError();
} // drawOverlay

void
RotoPaint::onInteractViewportSelectionCleared()
{
    if (!_imp->ui->isStickySelectionEnabled()  && !_imp->ui->shiftDown) {
        _imp->ui->clearSelection();
    }

    if ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) {
        if ( ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) && _imp->ui->builtBezier && !_imp->ui->builtBezier->isCurveFinished() ) {
            pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, _imp->ui->builtBezier) );

            _imp->ui->builtBezier.reset();
            _imp->ui->selectedCps.clear();
            _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
            _imp->ui->getContext()->evaluateChange();
        }
    }
}

void
RotoPaint::onInteractViewportSelectionUpdated(const RectD& rectangle,
                                              bool onRelease)
{
    if ( !onRelease || !getNode()->isSettingsPanelVisible() ) {
        return;
    }

    bool stickySel = _imp->ui->isStickySelectionEnabled();
    if (!stickySel && !_imp->ui->shiftDown) {
        _imp->ui->clearCPSSelection();
        _imp->ui->selectedItems.clear();
    }

    int selectionMode = -1;
    if (_imp->ui->selectedTool == eRotoToolSelectAll) {
        selectionMode = 0;
    } else if (_imp->ui->selectedTool == eRotoToolSelectPoints) {
        selectionMode = 1;
    } else if ( (_imp->ui->selectedTool == eRotoToolSelectFeatherPoints) || (_imp->ui->selectedTool == eRotoToolSelectCurves) ) {
        selectionMode = 2;
    }


    bool featherVisible = _imp->ui->isFeatherVisible();
    RotoContextPtr ctx = getNode()->getRotoContext();
    assert(ctx);
    std::list<RotoDrawableItemPtr> curves = ctx->getCurvesByRenderOrder();
    for (std::list<RotoDrawableItemPtr>::const_iterator it = curves.begin(); it != curves.end(); ++it) {
        BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if ( (*it)->isLockedRecursive() ) {
            continue;
        }

        if (isBezier) {
            SelectedCPs points  = isBezier->controlPointsWithinRect(rectangle.x1, rectangle.x2, rectangle.y1, rectangle.y2, 0, selectionMode);
            if (_imp->ui->selectedTool != eRotoToolSelectCurves) {
                for (SelectedCPs::iterator ptIt = points.begin(); ptIt != points.end(); ++ptIt) {
                    if ( !featherVisible && ptIt->first->isFeatherPoint() ) {
                        continue;
                    }
                    SelectedCPs::iterator foundCP = std::find(_imp->ui->selectedCps.begin(), _imp->ui->selectedCps.end(), *ptIt);
                    if ( foundCP == _imp->ui->selectedCps.end() ) {
                        if (!_imp->ui->shiftDown || !_imp->ui->ctrlDown) {
                            _imp->ui->selectedCps.push_back(*ptIt);
                        }
                    } else {
                        if (_imp->ui->shiftDown && _imp->ui->ctrlDown) {
                            _imp->ui->selectedCps.erase(foundCP);
                        }
                    }
                }
            }
            if ( !points.empty() ) {
                _imp->ui->selectedItems.push_back(isBezier);
            }
        }
    }

    if ( !_imp->ui->selectedItems.empty() ) {
        ctx->select(_imp->ui->selectedItems, RotoItem::eSelectionReasonOverlayInteract);
    } else if (!stickySel && !_imp->ui->shiftDown) {
        ctx->clearSelection(RotoItem::eSelectionReasonOverlayInteract);
    }

    _imp->ui->computeSelectedCpsBBOX();
} // RotoPaint::onInteractViewportSelectionUpdated

bool
RotoPaint::onOverlayPenDoubleClicked(double /*time*/,
                                     const RenderScale & /*renderScale*/,
                                     ViewIdx /*view*/,
                                     const QPointF & /*viewportPos*/,
                                     const QPointF & pos)
{
    bool didSomething = false;
    std::pair<double, double> pixelScale;

    getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    if (_imp->ui->selectedTool == eRotoToolSelectAll) {
        double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
        double nearbyBezierT;
        int nearbyBezierCPIndex;
        bool isFeather;
        BezierPtr nearbyBezier =
            getNode()->getRotoContext()->isNearbyBezier(pos.x(), pos.y(), bezierSelectionTolerance, &nearbyBezierCPIndex, &nearbyBezierT, &isFeather);

        if (nearbyBezier) {
            ///If the bezier is already selected and we re-click on it, change the transform mode
            _imp->ui->handleBezierSelection(nearbyBezier);
            _imp->ui->clearCPSSelection();
            const std::list<BezierCPPtr> & cps = nearbyBezier->getControlPoints();
            const std::list<BezierCPPtr> & fps = nearbyBezier->getFeatherPoints();
            assert( cps.size() == fps.size() );
            std::list<BezierCPPtr>::const_iterator itCp = cps.begin();
            std::list<BezierCPPtr>::const_iterator itFp = fps.begin();
            for (; itCp != cps.end(); ++itCp, ++itFp) {
                _imp->ui->selectedCps.push_back( std::make_pair(*itCp, *itFp) );
            }
            if (_imp->ui->selectedCps.size() > 1) {
                _imp->ui->computeSelectedCpsBBOX();
            }
            didSomething = true;
        }
    }

    return didSomething;
} // onOverlayPenDoubleClicked

bool
RotoPaint::onOverlayPenDown(double time,
                            const RenderScale & /*renderScale*/,
                            ViewIdx /*view*/,
                            const QPointF & /*viewportPos*/,
                            const QPointF & pos,
                            double pressure,
                            double timestamp,
                            PenType pen)
{
    NodePtr node = getNode();

    std::pair<double, double> pixelScale;
    getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    bool didSomething = false;
    double tangentSelectionTol = kTangentHandleSelectionTolerance * pixelScale.first;
    double cpSelectionTolerance = kControlPointSelectionTolerance * pixelScale.first;

    _imp->ui->lastTabletDownTriggeredEraser = false;
    if ( _imp->isPaintByDefault && ( (pen == ePenTypeEraser) || (pen == ePenTypePen) || (pen == ePenTypeCursor) ) ) {
        if ( (pen == ePenTypeEraser) && (_imp->ui->selectedTool != eRotoToolEraserBrush) ) {
            _imp->ui->setCurrentTool( _imp->ui->eraserAction.lock() );
            _imp->ui->lastTabletDownTriggeredEraser = true;
        }
    }

    RotoContextPtr context = node->getRotoContext();
    assert(context);

    const bool featherVisible = _imp->ui->isFeatherVisible();

    //////////////////BEZIER SELECTION
    /////Check if the point is nearby a bezier
    ///tolerance for bezier selection
    double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
    double nearbyBezierT;
    int nearbyBezierCPIndex;
    bool isFeather;
    BezierPtr nearbyBezier =
        context->isNearbyBezier(pos.x(), pos.y(), bezierSelectionTolerance, &nearbyBezierCPIndex, &nearbyBezierT, &isFeather);
    std::pair<BezierCPPtr, BezierCPPtr> nearbyCP;
    int nearbyCpIndex = -1;
    if (nearbyBezier) {
        /////////////////CONTROL POINT SELECTION
        //////Check if the point is nearby a control point of a selected bezier
        ///Find out if the user selected a control point

        Bezier::ControlPointSelectionPrefEnum pref = Bezier::eControlPointSelectionPrefWhateverFirst;
        if ( (_imp->ui->selectedTool == eRotoToolSelectFeatherPoints) && featherVisible ) {
            pref = Bezier::eControlPointSelectionPrefFeatherFirst;
        }

        nearbyCP = nearbyBezier->isNearbyControlPoint(pos.x(), pos.y(), cpSelectionTolerance, pref, &nearbyCpIndex);
    }

    ////////////////// TANGENT SELECTION
    ///in all cases except cusp/smooth if a control point is selected, check if the user clicked on a tangent handle
    ///in which case we go into eEventStateDraggingTangent mode
    if ( !nearbyCP.first &&
         ( _imp->ui->selectedTool != eRotoToolCuspPoints) &&
         ( _imp->ui->selectedTool != eRotoToolSmoothPoints) &&
         ( _imp->ui->selectedTool != eRotoToolSelectCurves) ) {
        for (SelectedCPs::iterator it = _imp->ui->selectedCps.begin(); it != _imp->ui->selectedCps.end(); ++it) {
            if ( (_imp->ui->selectedTool == eRotoToolSelectAll) ||
                 ( _imp->ui->selectedTool == eRotoToolDrawBezier) ) {
                int ret = it->first->isNearbyTangent(true, time, ViewIdx(0), pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->ui->tangentBeingDragged = it->first;
                    _imp->ui->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                } else {
                    ///try with the counter part point
                    if (it->second) {
                        ret = it->second->isNearbyTangent(true, time, ViewIdx(0), pos.x(), pos.y(), tangentSelectionTol);
                    }
                    if (ret >= 0) {
                        _imp->ui->tangentBeingDragged = it->second;
                        _imp->ui->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                        didSomething = true;
                    }
                }
            } else if (_imp->ui->selectedTool == eRotoToolSelectFeatherPoints) {
                const BezierCPPtr & fp = it->first->isFeatherPoint() ? it->first : it->second;
                int ret = fp->isNearbyTangent(true, time, ViewIdx(0), pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->ui->tangentBeingDragged = fp;
                    _imp->ui->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                }
            } else if (_imp->ui->selectedTool == eRotoToolSelectPoints) {
                const BezierCPPtr & cp = it->first->isFeatherPoint() ? it->second : it->first;
                int ret = cp->isNearbyTangent(true, time,  ViewIdx(0), pos.x(), pos.y(), tangentSelectionTol);
                if (ret >= 0) {
                    _imp->ui->tangentBeingDragged = cp;
                    _imp->ui->state = ret == 0 ? eEventStateDraggingLeftTangent : eEventStateDraggingRightTangent;
                    didSomething = true;
                }
            }

            ///check in case this is a feather tangent
            if (_imp->ui->tangentBeingDragged && _imp->ui->tangentBeingDragged->isFeatherPoint() && !featherVisible) {
                _imp->ui->tangentBeingDragged.reset();
                _imp->ui->state = eEventStateNone;
                didSomething = false;
            }

            if (didSomething) {
                return didSomething;
            }
        }
    }


    switch (_imp->ui->selectedTool) {
    case eRotoToolSelectAll:
    case eRotoToolSelectPoints:
    case eRotoToolSelectFeatherPoints: {
        if ( ( _imp->ui->selectedTool == eRotoToolSelectFeatherPoints) && !featherVisible ) {
            ///nothing to do
            break;
        }
        std::pair<BezierCPPtr, BezierCPPtr> featherBarSel;
        if ( ( ( _imp->ui->selectedTool == eRotoToolSelectAll) || ( _imp->ui->selectedTool == eRotoToolSelectFeatherPoints) ) ) {
            featherBarSel = _imp->ui->isNearbyFeatherBar(time, pixelScale, pos);
            if (featherBarSel.first && !featherVisible) {
                featherBarSel.first.reset();
                featherBarSel.second.reset();
            }
        }


        if (nearbyBezier) {
            ///check if the user clicked nearby the cross hair of the selection rectangle in which case
            ///we drag all the control points selected
            if (nearbyCP.first) {
                _imp->ui->handleControlPointSelection(nearbyCP);
                _imp->ui->handleBezierSelection(nearbyBezier);
                if (pen == ePenTypeRMB) {
                    _imp->ui->state = eEventStateNone;
                    _imp->ui->showMenuForControlPoint(nearbyCP.first);
                }
                didSomething = true;
            } else if (featherBarSel.first) {
                _imp->ui->clearCPSSelection();
                _imp->ui->featherBarBeingDragged = featherBarSel;

                ///Also select the point only if the curve is the same!
                if (featherBarSel.first->getBezier() == nearbyBezier) {
                    _imp->ui->handleControlPointSelection(_imp->ui->featherBarBeingDragged);
                    _imp->ui->handleBezierSelection(nearbyBezier);
                }
                _imp->ui->state = eEventStateDraggingFeatherBar;
                didSomething = true;
            } else {
                bool found = false;
                for (SelectedItems::iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
                    if ( it->get() == nearbyBezier.get() ) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    _imp->ui->handleBezierSelection(nearbyBezier);
                }
                if (pen == ePenTypeLMB || pen == ePenTypeMMB) {
                    if (_imp->ui->ctrlDown && _imp->ui->altDown && !_imp->ui->shiftDown) {
                        pushUndoCommand( new AddPointUndoCommand(_imp->ui, nearbyBezier, nearbyBezierCPIndex, nearbyBezierT) );
                        _imp->ui->evaluateOnPenUp = true;
                    } else {
                        _imp->ui->state = eEventStateDraggingSelectedControlPoints;
                        _imp->ui->bezierBeingDragged = nearbyBezier;
                    }
                } else if (pen == ePenTypeRMB) {
                    _imp->ui->showMenuForCurve(nearbyBezier);
                }
                didSomething = true;
            }
        } else {
            if (featherBarSel.first) {
                _imp->ui->clearCPSSelection();
                _imp->ui->featherBarBeingDragged = featherBarSel;
                _imp->ui->handleControlPointSelection(_imp->ui->featherBarBeingDragged);
                _imp->ui->state = eEventStateDraggingFeatherBar;
                didSomething = true;
            }
            if (_imp->ui->state == eEventStateNone) {
                _imp->ui->state = _imp->ui->isMouseInteractingWithCPSBbox(pos, cpSelectionTolerance, pixelScale);
                if (_imp->ui->state != eEventStateNone) {
                    didSomething = true;
                }
            }
        }
        break;
    }
    case eRotoToolSelectCurves:

        if (nearbyBezier) {
            ///If the bezier is already selected and we re-click on it, change the transform mode
            bool found = false;
            for (SelectedItems::iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
                if ( it->get() == nearbyBezier.get() ) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                _imp->ui->handleBezierSelection(nearbyBezier);
            }
            if (pen == ePenTypeRMB) {
                _imp->ui->showMenuForCurve(nearbyBezier);
            } else {
                if (_imp->ui->ctrlDown && _imp->ui->altDown && !_imp->ui->shiftDown) {
                    pushUndoCommand( new AddPointUndoCommand(_imp->ui, nearbyBezier, nearbyBezierCPIndex, nearbyBezierT) );
                    _imp->ui->evaluateOnPenUp = true;
                }
            }
            didSomething = true;
        } else {
            if (_imp->ui->state == eEventStateNone) {
                _imp->ui->state = _imp->ui->isMouseInteractingWithCPSBbox(pos, cpSelectionTolerance, pixelScale);
                if (_imp->ui->state != eEventStateNone) {
                    didSomething = true;
                }
            }
        }
        break;
    case eRotoToolAddPoints:
        ///If the user clicked on a bezier and this bezier is selected add a control point by
        ///splitting up the targeted segment
        if (nearbyBezier) {
            bool found = false;
            for (SelectedItems::iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
                if ( it->get() == nearbyBezier.get() ) {
                    found = true;
                    break;
                }
            }
            if (found) {
                ///check that the point is not too close to an existing point
                if (nearbyCP.first) {
                    _imp->ui->handleControlPointSelection(nearbyCP);
                } else {
                    pushUndoCommand( new AddPointUndoCommand(_imp->ui, nearbyBezier, nearbyBezierCPIndex, nearbyBezierT) );
                    _imp->ui->evaluateOnPenUp = true;
                }
                didSomething = true;
            }
        }
        break;
    case eRotoToolRemovePoints:
        if (nearbyCP.first) {
            assert( nearbyBezier && nearbyBezier == nearbyCP.first->getBezier() );
            if ( nearbyCP.first->isFeatherPoint() ) {
                pushUndoCommand( new RemovePointUndoCommand(_imp->ui, nearbyBezier, nearbyCP.second) );
            } else {
                pushUndoCommand( new RemovePointUndoCommand(_imp->ui, nearbyBezier, nearbyCP.first) );
            }
            didSomething = true;
        }
        break;
    case eRotoToolRemoveFeatherPoints:
        if (nearbyCP.first) {
            assert(nearbyBezier);
            std::list<RemoveFeatherUndoCommand::RemoveFeatherData> datas;
            RemoveFeatherUndoCommand::RemoveFeatherData data;
            data.curve = nearbyBezier;
            data.newPoints.push_back(nearbyCP.first->isFeatherPoint() ? nearbyCP.first : nearbyCP.second);
            datas.push_back(data);
            pushUndoCommand( new RemoveFeatherUndoCommand(_imp->ui, datas) );
            didSomething = true;
        }
        break;
    case eRotoToolOpenCloseCurve:
        if (nearbyBezier) {
            pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, nearbyBezier) );
            didSomething = true;
        }
        break;
    case eRotoToolSmoothPoints:

        if (nearbyCP.first) {
            std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = nearbyBezier;
            data.newPoints.push_back(nearbyCP);
            datas.push_back(data);
            pushUndoCommand( new SmoothCuspUndoCommand(_imp->ui, datas, time, false, pixelScale) );
            didSomething = true;
        }
        break;
    case eRotoToolCuspPoints:
        if ( nearbyCP.first && getNode()->getRotoContext()->isAutoKeyingEnabled() ) {
            std::list<SmoothCuspUndoCommand::SmoothCuspCurveData> datas;
            SmoothCuspUndoCommand::SmoothCuspCurveData data;
            data.curve = nearbyBezier;
            data.newPoints.push_back(nearbyCP);
            datas.push_back(data);
            pushUndoCommand( new SmoothCuspUndoCommand(_imp->ui, datas, time, true, pixelScale) );
            didSomething = true;
        }
        break;
    case eRotoToolDrawBezier:
    case eRotoToolOpenBezier: {
        if ( _imp->ui->builtBezier && _imp->ui->builtBezier->isCurveFinished() ) {
            _imp->ui->builtBezier.reset();
            _imp->ui->clearSelection();
            _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );

            return true;
        }
        if (_imp->ui->builtBezier) {
            ///if the user clicked on a control point of the bezier, select the point instead.
            ///if that point is the starting point of the curve, close the curve
            const std::list<BezierCPPtr> & cps = _imp->ui->builtBezier->getControlPoints();
            int i = 0;
            for (std::list<BezierCPPtr>::const_iterator it = cps.begin(); it != cps.end(); ++it, ++i) {
                double x, y;
                (*it)->getPositionAtTime(true, time, ViewIdx(0), &x, &y);
                if ( ( x >= (pos.x() - cpSelectionTolerance) ) && ( x <= (pos.x() + cpSelectionTolerance) ) &&
                     ( y >= (pos.y() - cpSelectionTolerance) ) && ( y <= (pos.y() + cpSelectionTolerance) ) ) {
                    if ( it == cps.begin() ) {
                        pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, _imp->ui->builtBezier) );

                        _imp->ui->builtBezier.reset();

                        _imp->ui->selectedCps.clear();
                        _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
                    } else {
                        BezierCPPtr fp = _imp->ui->builtBezier->getFeatherPointAtIndex(i);
                        assert(fp);
                        _imp->ui->handleControlPointSelection( std::make_pair(*it, fp) );
                    }

                    return true;
                }
            }
        }

        bool isOpenBezier = _imp->ui->selectedTool == eRotoToolOpenBezier;
        MakeBezierUndoCommand* cmd = new MakeBezierUndoCommand(_imp->ui, _imp->ui->builtBezier, isOpenBezier, true, pos.x(), pos.y(), time);
        pushUndoCommand(cmd);
        _imp->ui->builtBezier = cmd->getCurve();
        assert(_imp->ui->builtBezier);
        _imp->ui->state = eEventStateBuildingBezierControlPointTangent;
        didSomething = true;
        break;
    }
    case eRotoToolDrawBSpline:

        break;
    case eRotoToolDrawEllipse: {
        _imp->ui->click = pos;
        pushUndoCommand( new MakeEllipseUndoCommand(_imp->ui, true, false, false, pos.x(), pos.y(), pos.x(), pos.y(), time) );
        _imp->ui->state = eEventStateBuildingEllipse;
        didSomething = true;
        break;
    }
    case eRotoToolDrawRectangle: {
        _imp->ui->click = pos;
        pushUndoCommand( new MakeRectangleUndoCommand(_imp->ui, true, false, false, pos.x(), pos.y(), pos.x(), pos.y(), time) );
        _imp->ui->evaluateOnPenUp = true;
        _imp->ui->state = eEventStateBuildingRectangle;
        didSomething = true;
        break;
    }
    case eRotoToolSolidBrush:
    case eRotoToolEraserBrush:
    case eRotoToolClone:
    case eRotoToolReveal:
    case eRotoToolBlur:
    case eRotoToolSharpen:
    case eRotoToolSmear:
    case eRotoToolDodge:
    case eRotoToolBurn: {
        if ( ( ( _imp->ui->selectedTool == eRotoToolClone) || ( _imp->ui->selectedTool == eRotoToolReveal) ) && _imp->ui->ctrlDown &&
             !_imp->ui->shiftDown && !_imp->ui->altDown ) {
            _imp->ui->state = eEventStateDraggingCloneOffset;
        } else if (_imp->ui->shiftDown && !_imp->ui->ctrlDown && !_imp->ui->altDown) {
            _imp->ui->state = eEventStateDraggingBrushSize;
            _imp->ui->mouseCenterOnSizeChange = pos;
        } else if (_imp->ui->ctrlDown && _imp->ui->shiftDown) {
            _imp->ui->state = eEventStateDraggingBrushOpacity;
            _imp->ui->mouseCenterOnSizeChange = pos;
        } else {
            /*
               Check that all viewers downstream are connected directly to the RotoPaint to avoid glitches and bugs
             */
            _imp->ui->checkViewersAreDirectlyConnected();
            bool multiStrokeEnabled = _imp->ui->isMultiStrokeEnabled();
            if (_imp->ui->strokeBeingPaint &&
                _imp->ui->strokeBeingPaint->getParentLayer() &&
                multiStrokeEnabled) {
                RotoLayerPtr layer = _imp->ui->strokeBeingPaint->getParentLayer();
                if (!layer) {
                    layer = context->findDeepestSelectedLayer();
                    if (!layer) {
                        layer = context->getOrCreateBaseLayer();
                    }
                    assert(layer);
                    context->addItem(layer, 0, _imp->ui->strokeBeingPaint, RotoItem::eSelectionReasonOther);
                }

                context->getNode()->getApp()->setUserIsPainting(context->getNode(), _imp->ui->strokeBeingPaint, true);

                KnobIntPtr lifeTimeFrameKnob = _imp->ui->strokeBeingPaint->getLifeTimeFrameKnob();
                lifeTimeFrameKnob->setValue( context->getTimelineCurrentTime() );

                _imp->ui->strokeBeingPaint->appendPoint( true, RotoPoint(pos.x(), pos.y(), pressure, timestamp) );
            } else {
                if (_imp->ui->strokeBeingPaint &&
                    !_imp->ui->strokeBeingPaint->getParentLayer() &&
                    multiStrokeEnabled) {
                    _imp->ui->strokeBeingPaint.reset();
                }
                _imp->ui->makeStroke( false, RotoPoint(pos.x(), pos.y(), pressure, timestamp) );
            }
            context->evaluateChange();
            _imp->ui->state = eEventStateBuildingStroke;
            setCurrentCursor(eCursorBlank);
        }
        didSomething = true;
        break;
    }
    default:
        assert(false);
        break;
    } // switch

    _imp->ui->lastClickPos = pos;
    _imp->ui->lastMousePos = pos;

    return didSomething;
} // penDown

bool
RotoPaint::onOverlayPenMotion(double time,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/,
                              const QPointF & /*viewportPos*/,
                              const QPointF & pos,
                              double pressure,
                              double timestamp)
{
    std::pair<double, double> pixelScale;

    getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);

    RotoContextPtr context = getNode()->getRotoContext();
    assert(context);
    if (!context) {
        return false;
    }
    bool didSomething = false;
    HoverStateEnum lastHoverState = _imp->ui->hoverState;
    ///Set the cursor to the appropriate case
    bool cursorSet = false;
    double cpTol = kControlPointSelectionTolerance * pixelScale.first;

    if ( context->isRotoPaint() &&
         ( ( _imp->ui->selectedRole == eRotoRoleMergeBrush) ||
           ( _imp->ui->selectedRole == eRotoRoleCloneBrush) ||
           ( _imp->ui->selectedRole == eRotoRolePaintBrush) ||
           ( _imp->ui->selectedRole == eRotoRoleEffectBrush) ) ) {
        if (_imp->ui->state != eEventStateBuildingStroke) {
            setCurrentCursor(eCursorCross);
        } else {
            setCurrentCursor(eCursorBlank);
        }
        didSomething = true;
        cursorSet = true;
    }

    if ( !cursorSet && _imp->ui->showCpsBbox && (_imp->ui->state != eEventStateDraggingControlPoint) && (_imp->ui->state != eEventStateDraggingSelectedControlPoints)
         && ( _imp->ui->state != eEventStateDraggingLeftTangent) &&
         ( _imp->ui->state != eEventStateDraggingRightTangent) ) {
        double bboxTol = cpTol;
        if ( _imp->ui->isNearbyBBoxBtmLeft(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxBtmLeft;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxBtmRight(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxBtmRight;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxTopRight(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxTopRight;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxTopLeft(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxTopLeft;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxMidTop(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxMidTop;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxMidRight(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxMidRight;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxMidBtm(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxMidBtm;
            didSomething = true;
        } else if ( _imp->ui->isNearbyBBoxMidLeft(pos, bboxTol, pixelScale) ) {
            _imp->ui->hoverState = eHoverStateBboxMidLeft;
            didSomething = true;
        } else {
            _imp->ui->hoverState = eHoverStateNothing;
            didSomething = true;
        }
    }
    const bool featherVisible = _imp->ui->isFeatherVisible();

    if ( (_imp->ui->state == eEventStateNone) && (_imp->ui->hoverState == eHoverStateNothing) ) {
        if ( (_imp->ui->state != eEventStateDraggingControlPoint) && (_imp->ui->state != eEventStateDraggingSelectedControlPoints) ) {
            for (SelectedItems::const_iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
                int index = -1;
                Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
                if (isBezier) {
                    std::pair<BezierCPPtr, BezierCPPtr> nb =
                        isBezier->isNearbyControlPoint(pos.x(), pos.y(), cpTol, Bezier::eControlPointSelectionPrefWhateverFirst, &index);
                    if ( (index != -1) && ( ( !nb.first->isFeatherPoint() && !featherVisible ) || featherVisible ) ) {
                        setCurrentCursor(eCursorCross);
                        cursorSet = true;
                        break;
                    }
                }
            }
        }
        if ( !cursorSet && (_imp->ui->state != eEventStateDraggingLeftTangent) && (_imp->ui->state != eEventStateDraggingRightTangent) ) {
            ///find a nearby tangent
            for (SelectedCPs::const_iterator it = _imp->ui->selectedCps.begin(); it != _imp->ui->selectedCps.end(); ++it) {
                if (it->first->isNearbyTangent(true, time,  ViewIdx(0), pos.x(), pos.y(), cpTol) != -1) {
                    setCurrentCursor(eCursorCross);
                    cursorSet = true;
                    break;
                }
            }
        }
        if ( !cursorSet && (_imp->ui->state != eEventStateDraggingControlPoint) && (_imp->ui->state != eEventStateDraggingSelectedControlPoints) && (_imp->ui->state != eEventStateDraggingLeftTangent) &&
             ( _imp->ui->state != eEventStateDraggingRightTangent) ) {
            double bezierSelectionTolerance = kBezierSelectionTolerance * pixelScale.first;
            double nearbyBezierT;
            int nearbyBezierCPIndex;
            bool isFeather;
            BezierPtr nearbyBezier =
                context->isNearbyBezier(pos.x(), pos.y(), bezierSelectionTolerance, &nearbyBezierCPIndex, &nearbyBezierT, &isFeather);
            if (isFeather && !featherVisible) {
                nearbyBezier.reset();
            }
            if (nearbyBezier) {
                setCurrentCursor(eCursorPointingHand);
                cursorSet = true;
            }
        }

        bool clickAnywhere = _imp->ui->isBboxClickAnywhereEnabled();

        if ( !cursorSet && (_imp->ui->selectedCps.size() > 1) ) {
            if ( ( clickAnywhere && _imp->ui->isWithinSelectedCpsBBox(pos) ) ||
                 ( !clickAnywhere && _imp->ui->isNearbySelectedCpsCrossHair(pos) ) ) {
                setCurrentCursor(eCursorSizeAll);
                cursorSet = true;
            }
        }

        SelectedCP nearbyFeatherBar;
        if (!cursorSet && featherVisible) {
            nearbyFeatherBar = _imp->ui->isNearbyFeatherBar(time, pixelScale, pos);
            if (nearbyFeatherBar.first && nearbyFeatherBar.second) {
                _imp->ui->featherBarBeingHovered = nearbyFeatherBar;
            }
        }
        if (!nearbyFeatherBar.first || !nearbyFeatherBar.second) {
            _imp->ui->featherBarBeingHovered.first.reset();
            _imp->ui->featherBarBeingHovered.second.reset();
        }

        if ( (_imp->ui->state != eEventStateNone) || _imp->ui->featherBarBeingHovered.first || cursorSet || (lastHoverState != eHoverStateNothing) ) {
            didSomething = true;
        }
    }


    if (!cursorSet) {
        setCurrentCursor(eCursorDefault);
    }


    double dx = pos.x() - _imp->ui->lastMousePos.x();
    double dy = pos.y() - _imp->ui->lastMousePos.y();
    switch (_imp->ui->state) {
    case eEventStateDraggingSelectedControlPoints: {
        if (_imp->ui->bezierBeingDragged) {
            SelectedCPs cps;
            const std::list<BezierCPPtr>& c = _imp->ui->bezierBeingDragged->getControlPoints();
            const std::list<BezierCPPtr>& f = _imp->ui->bezierBeingDragged->getFeatherPoints();
            assert( c.size() == f.size() || !_imp->ui->bezierBeingDragged->useFeatherPoints() );
            bool useFeather = _imp->ui->bezierBeingDragged->useFeatherPoints();
            std::list<BezierCPPtr>::const_iterator itFp = f.begin();
            for (std::list<BezierCPPtr>::const_iterator itCp = c.begin(); itCp != c.end(); ++itCp) {
                if (useFeather) {
                    cps.push_back( std::make_pair(*itCp, *itFp) );
                    ++itFp;
                } else {
                    cps.push_back( std::make_pair( *itCp, BezierCPPtr() ) );
                }
            }
            pushUndoCommand( new MoveControlPointsUndoCommand(_imp->ui, cps, dx, dy, time) );
        } else {
            pushUndoCommand( new MoveControlPointsUndoCommand(_imp->ui, _imp->ui->selectedCps, dx, dy, time) );
        }
        _imp->ui->evaluateOnPenUp = true;
        _imp->ui->computeSelectedCpsBBOX();
        didSomething = true;
        break;
    }
    case eEventStateDraggingControlPoint: {
        assert(_imp->ui->cpBeingDragged.first);
        std::list<SelectedCP> toDrag;
        toDrag.push_back(_imp->ui->cpBeingDragged);
        pushUndoCommand( new MoveControlPointsUndoCommand(_imp->ui, toDrag, dx, dy, time) );
        _imp->ui->evaluateOnPenUp = true;
        _imp->ui->computeSelectedCpsBBOX();
        didSomething = true;
    };  break;
    case eEventStateBuildingBezierControlPointTangent: {
        assert(_imp->ui->builtBezier);
        bool isOpenBezier = _imp->ui->selectedTool == eRotoToolOpenBezier;
        pushUndoCommand( new MakeBezierUndoCommand(_imp->ui, _imp->ui->builtBezier, isOpenBezier, false, dx, dy, time) );
        break;
    }
    case eEventStateBuildingEllipse: {
        bool fromCenter = _imp->ui->ctrlDown > 0;
        bool constrained = _imp->ui->shiftDown > 0;
        pushUndoCommand( new MakeEllipseUndoCommand(_imp->ui, false, fromCenter, constrained, _imp->ui->click.x(), _imp->ui->click.y(), pos.x(), pos.y(), time) );

        didSomething = true;
        _imp->ui->evaluateOnPenUp = true;
        break;
    }
    case eEventStateBuildingRectangle: {
        bool fromCenter = _imp->ui->ctrlDown > 0;
        bool constrained = _imp->ui->shiftDown > 0;
        pushUndoCommand( new MakeRectangleUndoCommand(_imp->ui, false, fromCenter, constrained, _imp->ui->click.x(), _imp->ui->click.y(), pos.x(), pos.y(), time) );
        didSomething = true;
        _imp->ui->evaluateOnPenUp = true;
        break;
    }
    case eEventStateDraggingLeftTangent: {
        assert(_imp->ui->tangentBeingDragged);
        pushUndoCommand( new MoveTangentUndoCommand( _imp->ui, dx, dy, time, _imp->ui->tangentBeingDragged, true,
                                                     _imp->ui->ctrlDown && !_imp->ui->shiftDown && !_imp->ui->altDown ) );
        _imp->ui->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingRightTangent: {
        assert(_imp->ui->tangentBeingDragged);
        pushUndoCommand( new MoveTangentUndoCommand( _imp->ui, dx, dy, time, _imp->ui->tangentBeingDragged, false,
                                                     _imp->ui->ctrlDown && !_imp->ui->shiftDown && !_imp->ui->altDown ) );
        _imp->ui->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingFeatherBar: {
        pushUndoCommand( new MoveFeatherBarUndoCommand(_imp->ui, dx, dy, _imp->ui->featherBarBeingDragged, time) );
        _imp->ui->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingBBoxTopLeft:
    case eEventStateDraggingBBoxTopRight:
    case eEventStateDraggingBBoxBtmRight:
    case eEventStateDraggingBBoxBtmLeft: {
        QPointF center = _imp->ui->getSelectedCpsBBOXCenter();
        double rot = 0;
        double sx = 1., sy = 1.;

        if (_imp->ui->transformMode == eSelectedCpsTransformModeRotateAndSkew) {
            double angle = std::atan2( pos.y() - center.y(), pos.x() - center.x() );
            double prevAngle = std::atan2( _imp->ui->lastMousePos.y() - center.y(), _imp->ui->lastMousePos.x() - center.x() );
            rot = angle - prevAngle;
        } else {
            // the scale ratio is the ratio of distances to the center
            double prevDist = ( _imp->ui->lastMousePos.x() - center.x() ) * ( _imp->ui->lastMousePos.x() - center.x() ) +
                              ( _imp->ui->lastMousePos.y() - center.y() ) * ( _imp->ui->lastMousePos.y() - center.y() );
            if (prevDist != 0) {
                double dist = ( pos.x() - center.x() ) * ( pos.x() - center.x() ) + ( pos.y() - center.y() ) * ( pos.y() - center.y() );
                double ratio = std::sqrt(dist / prevDist);
                sx *= ratio;
                sy *= ratio;
            }
        }

        double tx = 0., ty = 0.;
        double skewX = 0., skewY = 0.;
        pushUndoCommand( new TransformUndoCommand(_imp->ui, center.x(), center.y(), rot, skewX, skewY, tx, ty, sx, sy, time) );
        _imp->ui->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateDraggingBBoxMidTop:
    case eEventStateDraggingBBoxMidBtm:
    case eEventStateDraggingBBoxMidLeft:
    case eEventStateDraggingBBoxMidRight: {
        QPointF center = _imp->ui->getSelectedCpsBBOXCenter();
        double rot = 0;
        double sx = 1., sy = 1.;
        double skewX = 0., skewY = 0.;
        double tx = 0., ty = 0.;
        TransformUndoCommand::TransformPointsSelectionEnum type = TransformUndoCommand::eTransformAllPoints;
        if (!_imp->ui->shiftDown) {
            type = TransformUndoCommand::eTransformAllPoints;
        } else {
            if (_imp->ui->state == eEventStateDraggingBBoxMidTop) {
                type = TransformUndoCommand::eTransformMidTop;
            } else if (_imp->ui->state == eEventStateDraggingBBoxMidBtm) {
                type = TransformUndoCommand::eTransformMidBottom;
            } else if (_imp->ui->state == eEventStateDraggingBBoxMidLeft) {
                type = TransformUndoCommand::eTransformMidLeft;
            } else if (_imp->ui->state == eEventStateDraggingBBoxMidRight) {
                type = TransformUndoCommand::eTransformMidRight;
            }
        }

        const QRectF& bbox = _imp->ui->selectedCpsBbox;

        switch (type) {
        case TransformUndoCommand::eTransformMidBottom:
            center.rx() = bbox.center().x();
            center.ry() = bbox.top();
            break;
        case TransformUndoCommand::eTransformMidTop:
            center.rx() = bbox.center().x();
            center.ry() = bbox.bottom();
            break;
        case TransformUndoCommand::eTransformMidRight:
            center.rx() = bbox.left();
            center.ry() = bbox.center().y();
            break;
        case TransformUndoCommand::eTransformMidLeft:
            center.rx() = bbox.right();
            center.ry() = bbox.center().y();
            break;
        default:
            break;
        }

        bool processX = _imp->ui->state == eEventStateDraggingBBoxMidRight || _imp->ui->state == eEventStateDraggingBBoxMidLeft;

        if (_imp->ui->transformMode == eSelectedCpsTransformModeRotateAndSkew) {
            if (!processX) {
                const double addSkew = ( pos.x() - _imp->ui->lastMousePos.x() ) / ( pos.y() - center.y() );
                skewX += addSkew;
            } else {
                const double addSkew = ( pos.y() - _imp->ui->lastMousePos.y() ) / ( pos.x() - center.x() );
                skewY += addSkew;
            }
        } else {
            // the scale ratio is the ratio of distances to the center
            double prevDist = ( _imp->ui->lastMousePos.x() - center.x() ) * ( _imp->ui->lastMousePos.x() - center.x() ) +
                              ( _imp->ui->lastMousePos.y() - center.y() ) * ( _imp->ui->lastMousePos.y() - center.y() );
            if (prevDist != 0) {
                double dist = ( pos.x() - center.x() ) * ( pos.x() - center.x() ) + ( pos.y() - center.y() ) * ( pos.y() - center.y() );
                double ratio = std::sqrt(dist / prevDist);
                if (processX) {
                    sx *= ratio;
                } else {
                    sy *= ratio;
                }
            }
        }


        pushUndoCommand( new TransformUndoCommand(_imp->ui, center.x(), center.y(), rot, skewX, skewY, tx, ty, sx, sy, time) );
        _imp->ui->evaluateOnPenUp = true;
        didSomething = true;
        break;
    }
    case eEventStateBuildingStroke: {
        if (_imp->ui->strokeBeingPaint) {
            // disable draft during painting
            if ( getApp()->isDraftRenderEnabled() ) {
                getApp()->setDraftRenderEnabled(false);
            }

            RotoPoint p(pos.x(), pos.y(), pressure, timestamp);
            if ( _imp->ui->strokeBeingPaint->appendPoint(false, p) ) {
                _imp->ui->lastMousePos = pos;
                context->evaluateChange_noIncrement();

                return true;
            }
        }
        break;
    }
    case eEventStateDraggingCloneOffset: {
        _imp->ui->cloneOffset.first -= dx;
        _imp->ui->cloneOffset.second -= dy;
        _imp->ui->onBreakMultiStrokeTriggered();
        break;
    }
    case eEventStateDraggingBrushSize: {
        KnobDoublePtr sizeSb = _imp->ui->sizeSpinbox.lock();
        double size = 0;
        if (sizeSb) {
            size = sizeSb->getValue();
        }
        size += ( (dx + dy) / 2. );
        const double scale = 0.01;      // i.e. round to nearest one-hundreth
        size = std::floor(size / scale + 0.5) * scale;
        if (sizeSb) {
            sizeSb->setValue( std::max(1., size) );
        }
        _imp->ui->onBreakMultiStrokeTriggered();
        didSomething = true;
        break;
    }
    case eEventStateDraggingBrushOpacity: {
        KnobDoublePtr opaSb = _imp->ui->sizeSpinbox.lock();
        double opa = 0;
        if (opaSb) {
            opa = opaSb->getValue();
        }
        double newOpa = opa + ( (dx + dy) / 2. );
        if (opa != 0) {
            newOpa = std::max( 0., std::min(1., newOpa / opa) );
            newOpa = newOpa > 0 ? std::min(1., opa + 0.05) : std::max(0., opa - 0.05);
        } else {
            newOpa = newOpa < 0 ? .0 : 0.05;
        }
        const double scale = 0.01;      // i.e. round to nearest one-hundreth
        newOpa = std::floor(newOpa / scale + 0.5) * scale;
        if (opaSb) {
            opaSb->setValue(newOpa);
        }
        _imp->ui->onBreakMultiStrokeTriggered();
        didSomething = true;
        break;
    }
    case eEventStateNone:
    default:
        break;
    } // switch
    _imp->ui->lastMousePos = pos;

    return didSomething;
} // onOverlayPenMotion

bool
RotoPaint::onOverlayPenUp(double /*time*/,
                          const RenderScale & /*renderScale*/,
                          ViewIdx /*view*/,
                          const QPointF & /*viewportPos*/,
                          const QPointF & /*pos*/,
                          double /*pressure*/,
                          double /*timestamp*/)
{
    RotoContextPtr context = getNode()->getRotoContext();

    assert(context);
    if (!context) {
        return false;
    }

    if (_imp->ui->evaluateOnPenUp) {
        context->evaluateChange();
        getApp()->triggerAutoSave();

        //sync other viewers linked to this roto
        redrawOverlayInteract();
        _imp->ui->evaluateOnPenUp = false;
    }

    bool ret = false;
    _imp->ui->tangentBeingDragged.reset();
    _imp->ui->bezierBeingDragged.reset();
    _imp->ui->cpBeingDragged.first.reset();
    _imp->ui->cpBeingDragged.second.reset();
    _imp->ui->featherBarBeingDragged.first.reset();
    _imp->ui->featherBarBeingDragged.second.reset();

    if ( (_imp->ui->state == eEventStateDraggingBBoxMidLeft) ||
         ( _imp->ui->state == eEventStateDraggingBBoxMidLeft) ||
         ( _imp->ui->state == eEventStateDraggingBBoxMidTop) ||
         ( _imp->ui->state == eEventStateDraggingBBoxMidBtm) ||
         ( _imp->ui->state == eEventStateDraggingBBoxTopLeft) ||
         ( _imp->ui->state == eEventStateDraggingBBoxTopRight) ||
         ( _imp->ui->state == eEventStateDraggingBBoxBtmRight) ||
         ( _imp->ui->state == eEventStateDraggingBBoxBtmLeft) ) {
        _imp->ui->computeSelectedCpsBBOX();
    }

    if (_imp->ui->state == eEventStateBuildingStroke) {
        assert(_imp->ui->strokeBeingPaint);
        context->getNode()->getApp()->setUserIsPainting(context->getNode(), _imp->ui->strokeBeingPaint, false);
        assert( _imp->ui->strokeBeingPaint->getParentLayer() );

        bool multiStrokeEnabled = _imp->ui->isMultiStrokeEnabled();
        if (!multiStrokeEnabled) {
            pushUndoCommand( new AddStrokeUndoCommand(_imp->ui, _imp->ui->strokeBeingPaint) );
            _imp->ui->makeStroke( true, RotoPoint() );
        } else {
            pushUndoCommand( new AddMultiStrokeUndoCommand(_imp->ui, _imp->ui->strokeBeingPaint) );
        }

        /**
         * Do a neat render for the stroke (better interpolation). This call is blocking otherwise the user might
         * attempt to make a new stroke while the previous stroke is not finished... this would yield artifacts.
         **/
        setCurrentCursor(eCursorBusy);
        context->evaluateNeatStrokeRender();
        setCurrentCursor(eCursorDefault);
        _imp->ui->strokeBeingPaint->setStrokeFinished();
        ret = true;
    }

    _imp->ui->state = eEventStateNone;

    if ( (_imp->ui->selectedTool == eRotoToolDrawEllipse) || (_imp->ui->selectedTool == eRotoToolDrawRectangle) ) {
        _imp->ui->selectedCps.clear();
        _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
        ret = true;
    }

    if (_imp->ui->lastTabletDownTriggeredEraser) {
        _imp->ui->setCurrentTool( _imp->ui->lastPaintToolAction.lock() );
        ret = true;
    }

    return ret;
} // onOverlayPenUp

bool
RotoPaint::onOverlayKeyDown(double /*time*/,
                            const RenderScale & /*renderScale*/,
                            ViewIdx /*view*/,
                            Key key,
                            KeyboardModifiers /*modifiers*/)
{
    bool didSomething = false;

    if ( (key == Key_Shift_L) || (key == Key_Shift_R) ) {
        ++_imp->ui->shiftDown;
    } else if ( (key == Key_Control_L) || (key == Key_Control_R) ) {
        ++_imp->ui->ctrlDown;
    } else if ( (key == Key_Alt_L) || (key == Key_Alt_R) ) {
        ++_imp->ui->altDown;
    }

    if (_imp->ui->ctrlDown && !_imp->ui->shiftDown && !_imp->ui->altDown) {
        if ( !_imp->ui->iSelectingwithCtrlA && _imp->ui->showCpsBbox && ( (key == Key_Control_L) || (key == Key_Control_R) ) ) {
            _imp->ui->transformMode = _imp->ui->transformMode == eSelectedCpsTransformModeTranslateAndScale ?
                                      eSelectedCpsTransformModeRotateAndSkew : eSelectedCpsTransformModeTranslateAndScale;
            didSomething = true;
        }
    }

    if ( ( (key == Key_Escape) && ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) ) ) {
        if ( ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) && _imp->ui->builtBezier && !_imp->ui->builtBezier->isCurveFinished() ) {
            pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, _imp->ui->builtBezier) );

            _imp->ui->builtBezier.reset();
            _imp->ui->selectedCps.clear();
            _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
            getNode()->getRotoContext()->evaluateChange();
            didSomething = true;
        }
    }

    return didSomething;
} //onOverlayKeyDown

bool
RotoPaint::onOverlayKeyUp(double /*time*/,
                          const RenderScale & /*renderScale*/,
                          ViewIdx /*view*/,
                          Key key,
                          KeyboardModifiers /*modifiers*/)
{
    bool didSomething = false;

    if ( (key == Key_Shift_L) || (key == Key_Shift_R) ) {
        if (_imp->ui->shiftDown) {
            --_imp->ui->shiftDown;
        }
    } else if ( (key == Key_Control_L) || (key == Key_Control_R) ) {
        if (_imp->ui->ctrlDown) {
            --_imp->ui->ctrlDown;
        }
    } else if ( (key == Key_Alt_L) || (key == Key_Alt_R) ) {
        if (_imp->ui->altDown) {
            --_imp->ui->altDown;
        }
    }


    if (!_imp->ui->ctrlDown) {
        if ( !_imp->ui->iSelectingwithCtrlA && _imp->ui->showCpsBbox && ( (key == Key_Control_L) || (key == Key_Control_R) ) ) {
            _imp->ui->transformMode = (_imp->ui->transformMode == eSelectedCpsTransformModeTranslateAndScale ?
                                       eSelectedCpsTransformModeRotateAndSkew : eSelectedCpsTransformModeTranslateAndScale);
            didSomething = true;
        }
    }

    if ( ( (key == Key_Control_L) || (key == Key_Control_R) ) && _imp->ui->iSelectingwithCtrlA ) {
        _imp->ui->iSelectingwithCtrlA = false;
    }

    if (_imp->ui->evaluateOnKeyUp) {
        getNode()->getRotoContext()->evaluateChange();
        getNode()->getApp()->triggerAutoSave();
        redrawOverlayInteract();
        _imp->ui->evaluateOnKeyUp = false;
    }

    return didSomething;
} // onOverlayKeyUp

bool
RotoPaint::onOverlayKeyRepeat(double /*time*/,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/,
                              Key /*key*/,
                              KeyboardModifiers /*modifiers*/)
{
    return false;
} // onOverlayKeyRepeat

bool
RotoPaint::onOverlayFocusGained(double /*time*/,
                                const RenderScale & /*renderScale*/,
                                ViewIdx /*view*/)
{
    return false;
} // onOverlayFocusGained

bool
RotoPaint::onOverlayFocusLost(double /*time*/,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/)
{
    _imp->ui->shiftDown = 0;
    _imp->ui->ctrlDown = 0;
    _imp->ui->altDown = 0;

    return true;
} // onOverlayFocusLost

void
RotoPaint::onRefreshAsked()
{
    redrawOverlayInteract();
}

void
RotoPaint::onCurveLockedChanged(int reason)
{
    RotoItemPtr item = getNode()->getRotoContext()->getLastItemLocked();

    if ( item && ( (RotoItem::SelectionReasonEnum)reason != RotoItem::eSelectionReasonOverlayInteract ) ) {
        assert(item);
        bool changed = false;
        if (item) {
            _imp->ui->onCurveLockedChangedRecursive(item, &changed);
        }

        if (changed) {
            redrawOverlayInteract();
        }
    }
}

void
RotoPaint::onBreakMultiStrokeTriggered()
{
    _imp->ui->onBreakMultiStrokeTriggered();
}

void
RotoPaint::onSelectionChanged(int reason)
{
    if ( (RotoItem::SelectionReasonEnum)reason != RotoItem::eSelectionReasonOverlayInteract ) {
        _imp->ui->selectedItems = getNode()->getRotoContext()->getSelectedCurves();
        redrawOverlayInteract();
    }
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_RotoPaint.cpp"

