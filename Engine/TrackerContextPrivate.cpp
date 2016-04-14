/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "TrackerContextPrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/TLSHolder.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerFrameAccessor.h"
#include "Engine/TrackerNode.h"
#include "Engine/TrackerContext.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief Creates a duplicate of the knob identified by knobName which is a knob in the internalNode onto the effect and add it to the given page.
 * If otherNode is set, also fetch a knob of the given name on the otherNode and link it to the newly created knob.
 **/

template <typename KNOBTYPE>
boost::shared_ptr<KNOBTYPE> createDuplicateKnob(const std::string& knobName, const NodePtr& internalNode, const EffectInstPtr& effect, const boost::shared_ptr<KnobPage>& page = boost::shared_ptr<KnobPage>(), const boost::shared_ptr<KnobGroup>& group = boost::shared_ptr<KnobGroup>(), const NodePtr& otherNode = NodePtr()) {
    KnobPtr internalNodeKnob = internalNode->getKnobByName(knobName);
    assert(internalNodeKnob);
    KnobPtr duplicateKnob = internalNodeKnob->createDuplicateOnNode(effect.get(), page, group, -1, true, internalNodeKnob->getName(), internalNodeKnob->getLabel(), internalNodeKnob->getHintToolTip(), false, false);
    
    if (otherNode) {
        KnobPtr otherNodeKnob = otherNode->getKnobByName(knobName);
        assert(otherNodeKnob);
        for (int i = 0; i < otherNodeKnob->getDimension(); ++i) {
            otherNodeKnob->slaveTo(i, duplicateKnob, i);
        }
    }
    
    return boost::dynamic_pointer_cast<KNOBTYPE>(duplicateKnob);

}

TrackerContextPrivate::TrackerContextPrivate(TrackerContext* publicInterface, const boost::shared_ptr<Node> &node)
: _publicInterface(publicInterface)
, node(node)
, perTrackKnobs()
, enableTrackRed()
, enableTrackGreen()
, enableTrackBlue()
, maxError()
, maxIterations()
, bruteForcePreTrack()
, useNormalizedIntensities()
, preBlurSigma()
, exportDataSep()
, exportButton()
, referenceFrame()
, trackerContextMutex()
, markers()
, selectedMarkers()
, markersToSlave()
, markersToUnslave()
, beginSelectionCounter(0)
, selectionRecursion(0)
, scheduler(_publicInterface ,node, &TrackerContextPrivate::trackStepLibMV)
{
    EffectInstPtr effect = node->getEffectInstance();
    QObject::connect(&scheduler, SIGNAL(trackingStarted(int)), _publicInterface, SIGNAL(trackingStarted(int)));
    QObject::connect(&scheduler, SIGNAL(trackingFinished()), _publicInterface, SIGNAL(trackingFinished()));
    
    boost::shared_ptr<TrackerNode> isTrackerNode = boost::dynamic_pointer_cast<TrackerNode>(effect);
    
    QString fixedNamePrefix = QString::fromUtf8(node->getScriptName_mt_safe().c_str());
    fixedNamePrefix.append(QLatin1Char('_'));
    if (isTrackerNode) {
        NodePtr input,output;
        //NodePtr maskInput;
        
        {
            CreateNodeArgs args(QString::fromUtf8(PLUGINID_NATRON_OUTPUT), eCreateNodeReasonInternal, isTrackerNode);
            args.createGui = false;
            args.addToProject = false;
            output = node->getApp()->createNode(args);
            try {
                output->setScriptName("Output");
            } catch (...) {
                
            }
            assert(output);
        }
        {
            CreateNodeArgs args(QString::fromUtf8(PLUGINID_NATRON_INPUT), eCreateNodeReasonInternal, isTrackerNode);
            args.fixedName = QLatin1String("Source");
            args.createGui = false;
            args.addToProject = false;
            input = node->getApp()->createNode(args);
            assert(input);
        }
        
       /* {
            CreateNodeArgs args(QString::fromUtf8(PLUGINID_NATRON_INPUT), eCreateNodeReasonInternal, isTrackerNode);
            args.fixedName = QLatin1String("Mask");
            args.createGui = false;
            args.addToProject = false;
            maskInput = node->getApp()->createNode(args);
            assert(maskInput);
            KnobPtr isMaskInputKnob = maskInput->getKnobByName(kNatronGroupInputIsMaskParamName);
            if (isMaskInputKnob) {
                KnobBool* maskInputKnob = dynamic_cast<KnobBool*>(isMaskInputKnob.get());
                assert(maskInputKnob);
                if (maskInputKnob) {
                    maskInputKnob->setValue(true);
                }
            }
            KnobPtr isOptionalInputKnob = maskInput->getKnobByName(kNatronGroupInputIsOptionalParamName);
            if (isOptionalInputKnob) {
                KnobBool* optionalInputKnob = dynamic_cast<KnobBool*>(isOptionalInputKnob.get());
                assert(optionalInputKnob);
                if (optionalInputKnob) {
                    optionalInputKnob->setValue(true);
                }
            }

        }*/

        {
            QString cornerPinName = fixedNamePrefix + QLatin1String("CornerPin");
            
            CreateNodeArgs args(QString::fromUtf8(PLUGINID_OFX_CORNERPIN), eCreateNodeReasonInternal, isTrackerNode);
            args.fixedName = cornerPinName;
            args.createGui = false;
            args.addToProject = false;
            NodePtr cpNode = node->getApp()->createNode(args);
            if (!cpNode) {
                throw std::runtime_error(QObject::tr("The Tracker node requires the Misc.ofx.bundle plug-in to be installed").toStdString());
            }
            cpNode->setNodeDisabled(true);
            cornerPinNode = cpNode;
        }
        
        {
            QString transformName = fixedNamePrefix + QLatin1String("Transform");
            
            CreateNodeArgs args(QString::fromUtf8(PLUGINID_OFX_TRANSFORM), eCreateNodeReasonInternal, isTrackerNode);
            args.fixedName = transformName;
            args.createGui = false;
            args.addToProject = false;
            NodePtr tNode = node->getApp()->createNode(args);
            tNode->setNodeDisabled(true);
            transformNode = tNode;
            
            output->connectInput(tNode, 0);
            NodePtr cpNode = cornerPinNode.lock();
            tNode->connectInput(cpNode, 0);
            cpNode->connectInput(input, 0);

        }
    }
    
    boost::shared_ptr<KnobPage> settingsPage = AppManager::createKnob<KnobPage>(effect.get(), "Tracking", 1 , false);
    boost::shared_ptr<KnobPage> transformPage = AppManager::createKnob<KnobPage>(effect.get(), "Transform", 1 , false);
    transformPageKnob = transformPage;
    
    boost::shared_ptr<KnobBool> enableTrackRedKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamTrackRedLabel, 1, false);
    enableTrackRedKnob->setName(kTrackerParamTrackRed);
    enableTrackRedKnob->setHintToolTip(kTrackerParamTrackRedHint);
    enableTrackRedKnob->setDefaultValue(true);
    enableTrackRedKnob->setAnimationEnabled(false);
    enableTrackRedKnob->setAddNewLine(false);
    enableTrackRedKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(enableTrackRedKnob);
    enableTrackRed = enableTrackRedKnob;
    
    boost::shared_ptr<KnobBool> enableTrackGreenKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamTrackGreenLabel, 1, false);
    enableTrackGreenKnob->setName(kTrackerParamTrackGreen);
    enableTrackGreenKnob->setHintToolTip(kTrackerParamTrackGreenHint);
    enableTrackGreenKnob->setDefaultValue(true);
    enableTrackGreenKnob->setAnimationEnabled(false);
    enableTrackGreenKnob->setAddNewLine(false);
    enableTrackGreenKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(enableTrackGreenKnob);
    enableTrackGreen = enableTrackGreenKnob;
    
    boost::shared_ptr<KnobBool> enableTrackBlueKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamTrackBlueLabel, 1, false);
    enableTrackBlueKnob->setName(kTrackerParamTrackBlue);
    enableTrackBlueKnob->setHintToolTip(kTrackerParamTrackBlueHint);
    enableTrackBlueKnob->setDefaultValue(true);
    enableTrackBlueKnob->setAnimationEnabled(false);
    enableTrackBlueKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(enableTrackBlueKnob);
    enableTrackBlue = enableTrackBlueKnob;
    
    boost::shared_ptr<KnobDouble> maxErrorKnob = AppManager::createKnob<KnobDouble>(effect.get(), kTrackerParamMaxErrorLabel, 1, false);
    maxErrorKnob->setName(kTrackerParamMaxError);
    maxErrorKnob->setHintToolTip(kTrackerParamMaxErrorHint);
    maxErrorKnob->setAnimationEnabled(false);
    maxErrorKnob->setMinimum(0.);
    maxErrorKnob->setMaximum(1.);
    maxErrorKnob->setDefaultValue(0.2);
    maxErrorKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(maxErrorKnob);
    maxError = maxErrorKnob;
    
    boost::shared_ptr<KnobInt> maxItKnob = AppManager::createKnob<KnobInt>(effect.get(), kTrackerParamMaximumIterationLabel, 1, false);
    maxItKnob->setName(kTrackerParamMaximumIteration);
    maxItKnob->setHintToolTip(kTrackerParamMaximumIterationHint);
    maxItKnob->setAnimationEnabled(false);
    maxItKnob->setMinimum(0);
    maxItKnob->setMaximum(150);
    maxItKnob->setEvaluateOnChange(false);
    maxItKnob->setDefaultValue(50);
    settingsPage->addKnob(maxItKnob);
    maxIterations = maxItKnob;
    
    boost::shared_ptr<KnobBool> usePretTrackBF = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamBruteForcePreTrackLabel, 1, false);
    usePretTrackBF->setName(kTrackerParamBruteForcePreTrack);
    usePretTrackBF->setHintToolTip(kTrackerParamBruteForcePreTrackHint);
    usePretTrackBF->setDefaultValue(true);
    usePretTrackBF->setAnimationEnabled(false);
    usePretTrackBF->setEvaluateOnChange(false);
    usePretTrackBF->setAddNewLine(false);
    settingsPage->addKnob(usePretTrackBF);
    bruteForcePreTrack = usePretTrackBF;
    
    boost::shared_ptr<KnobBool> useNormalizedInt = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamNormalizeIntensitiesLabel, 1, false);
    useNormalizedInt->setName(kTrackerParamNormalizeIntensities);
    useNormalizedInt->setHintToolTip(kTrackerParamNormalizeIntensitiesHint);
    useNormalizedInt->setDefaultValue(false);
    useNormalizedInt->setAnimationEnabled(false);
    useNormalizedInt->setEvaluateOnChange(false);
    settingsPage->addKnob(useNormalizedInt);
    useNormalizedIntensities = useNormalizedInt;
    
    boost::shared_ptr<KnobDouble> preBlurSigmaKnob = AppManager::createKnob<KnobDouble>(effect.get(), kTrackerParamPreBlurSigmaLabel, 1, false);
    preBlurSigmaKnob->setName(kTrackerParamPreBlurSigma);
    preBlurSigmaKnob->setHintToolTip(kTrackerParamPreBlurSigmaHint);
    preBlurSigmaKnob->setAnimationEnabled(false);
    preBlurSigmaKnob->setMinimum(0);
    preBlurSigmaKnob->setMaximum(10.);
    preBlurSigmaKnob->setDefaultValue(0.9);
    preBlurSigmaKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(preBlurSigmaKnob);
    preBlurSigma = preBlurSigmaKnob;
    
    boost::shared_ptr<KnobBool> enableTrackKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamEnabledLabel, 1, false);
    enableTrackKnob->setName(kTrackerParamEnabled);
    enableTrackKnob->setHintToolTip(kTrackerParamEnabledHint);
    enableTrackKnob->setAnimationEnabled(true);
    enableTrackKnob->setDefaultValue(true);
    enableTrackKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(enableTrackKnob);
    activateTrack = enableTrackKnob;
    perTrackKnobs.push_back(enableTrackKnob);
    
    
    boost::shared_ptr<KnobSeparator>  transformGenerationSeparatorKnob = AppManager::createKnob<KnobSeparator>(effect.get(), "Transform Generation", 3);
    transformPage->addKnob(transformGenerationSeparatorKnob);
    transformGenerationSeparator = transformGenerationSeparatorKnob;
    
    boost::shared_ptr<KnobChoice> motionTypeKnob = AppManager::createKnob<KnobChoice>(effect.get(), kTrackerParamMotionTypeLabel, 1);
    motionTypeKnob->setName(kTrackerParamMotionType);
    motionTypeKnob->setHintToolTip(kTrackerParamMotionTypeHint);
    {
        std::vector<std::string> choices,helps;
        choices.push_back(kTrackerParamMotionTypeNone);
        helps.push_back(kTrackerParamMotionTypeNoneHelp);
        choices.push_back(kTrackerParamMotionTypeStabilize);
        helps.push_back(kTrackerParamMotionTypeStabilizeHelp);
        choices.push_back(kTrackerParamMotionTypeMatchMove);
        helps.push_back(kTrackerParamMotionTypeMatchMoveHelp);
        choices.push_back(kTrackerParamMotionTypeRemoveJitter);
        helps.push_back(kTrackerParamMotionTypeRemoveJitterHelp);
        choices.push_back(kTrackerParamMotionTypeAddJitter);
        helps.push_back(kTrackerParamMotionTypeAddJitterHelp);
        
        motionTypeKnob->populateChoices(choices,helps);
    }
    motionTypeKnob->setAddNewLine(false);
    motionType = motionTypeKnob;
    transformPage->addKnob(motionTypeKnob);

    boost::shared_ptr<KnobChoice> transformTypeKnob = AppManager::createKnob<KnobChoice>(effect.get(), kTrackerParamTransformTypeLabel, 1);
    transformTypeKnob->setName(kTrackerParamTransformType);
    transformTypeKnob->setHintToolTip(kTrackerParamTransformTypeHint);
    {
        std::vector<std::string> choices,helps;
        choices.push_back(kTrackerParamTransformTypeTransform);
        helps.push_back(kTrackerParamTransformTypeTransformHelp);
        choices.push_back(kTrackerParamTransformTypeCornerPin);
        helps.push_back(kTrackerParamTransformTypeCornerPinHelp);
        
        transformTypeKnob->populateChoices(choices,helps);
    }
    transformType = transformTypeKnob;
    transformPage->addKnob(transformTypeKnob);
    
    boost::shared_ptr<KnobInt> referenceFrameKnob = AppManager::createKnob<KnobInt>(effect.get(), kTrackerParamReferenceFrameLabel, 1);
    referenceFrameKnob->setName(kTrackerParamReferenceFrame);
    referenceFrameKnob->setHintToolTip(kTrackerParamReferenceFrameHint);
    referenceFrameKnob->setAnimationEnabled(false);
    referenceFrameKnob->setDefaultValue(0);
    referenceFrameKnob->setAddNewLine(false);
    referenceFrameKnob->setEvaluateOnChange(false);
    transformPage->addKnob(referenceFrameKnob);
    referenceFrame = referenceFrameKnob;
    
    boost::shared_ptr<KnobButton> setCurrentFrameKnob = AppManager::createKnob<KnobButton>(effect.get(), kTrackerParamSetReferenceFrameLabel, 1);
    setCurrentFrameKnob->setName(kTrackerParamSetReferenceFrame);
    setCurrentFrameKnob->setHintToolTip(kTrackerParamSetReferenceFrameHint);
    transformPage->addKnob(setCurrentFrameKnob);
    setCurrentFrameButton = setCurrentFrameKnob;
    
    boost::shared_ptr<KnobInt>  jitterPeriodKnob = AppManager::createKnob<KnobInt>(effect.get(), kTrackerParamJitterPeriodLabel, 1);
    jitterPeriodKnob->setName(kTrackerParamJitterPeriod);
    jitterPeriodKnob->setHintToolTip(kTrackerParamJitterPeriodHint);
    jitterPeriodKnob->setAnimationEnabled(false);
    jitterPeriodKnob->setDefaultValue(10);
    jitterPeriodKnob->setMinimum(0, 0);
    jitterPeriodKnob->setEvaluateOnChange(false);
    transformPage->addKnob(jitterPeriodKnob);
    jitterPeriod = jitterPeriodKnob;
    
    boost::shared_ptr<KnobInt>  smoothTransformKnob = AppManager::createKnob<KnobInt>(effect.get(), kTrackerParamSmoothLabel, 3);
    smoothTransformKnob->setName(kTrackerParamSmooth);
    smoothTransformKnob->setHintToolTip(kTrackerParamSmoothHint);
    smoothTransformKnob->setAnimationEnabled(false);
    smoothTransformKnob->disableSlider();
    smoothTransformKnob->setDimensionName(0, "t");
    smoothTransformKnob->setDimensionName(1, "r");
    smoothTransformKnob->setDimensionName(2, "s");
    for (int i = 0;i < 3; ++i) {
        smoothTransformKnob->setMinimum(0, i);
    }
    smoothTransformKnob->setEvaluateOnChange(false);
    transformPage->addKnob(smoothTransformKnob);
    smoothTransform = smoothTransformKnob;
    
    boost::shared_ptr<KnobSeparator>  transformSeparator = AppManager::createKnob<KnobSeparator>(effect.get(), "Transform Controls", 3);
    transformPage->addKnob(transformSeparator);
    transformControlsSeparator = transformSeparator;
    
    NodePtr tNode = transformNode.lock();
    if (tNode) {

        translate = createDuplicateKnob<KnobDouble>(kTransformParamTranslate, tNode, effect, transformPage);
        rotate = createDuplicateKnob<KnobDouble>(kTransformParamRotate, tNode, effect, transformPage);
        scale = createDuplicateKnob<KnobDouble>(kTransformParamScale, tNode, effect, transformPage);
        scale.lock()->setAddNewLine(false);
        scaleUniform = createDuplicateKnob<KnobBool>(kTransformParamUniform, tNode, effect, transformPage);
        skewX = createDuplicateKnob<KnobDouble>(kTransformParamSkewX, tNode, effect, transformPage);
        skewY = createDuplicateKnob<KnobDouble>(kTransformParamSkewY, tNode, effect, transformPage);
        skewOrder = createDuplicateKnob<KnobChoice>(kTransformParamSkewOrder, tNode, effect, transformPage);
        center = createDuplicateKnob<KnobDouble>(kTransformParamCenter, tNode, effect, transformPage);
        node->addTransformInteract(translate.lock(), scale.lock(), scaleUniform.lock(), rotate.lock(), skewX.lock(), skewY.lock(), skewOrder.lock(), center.lock());
    } // tNode
    NodePtr cNode = cornerPinNode.lock();
    if (cNode) {
        boost::shared_ptr<KnobGroup>  toGroupKnob = AppManager::createKnob<KnobGroup>(effect.get(), kCornerPinParamTo, 1);
        toGroupKnob->setName(kCornerPinParamTo);
        toGroupKnob->setAsTab();
        toGroupKnob->setSecret(true);
        transformPage->addKnob(toGroupKnob);
        toGroup = toGroupKnob;
        
        boost::shared_ptr<KnobGroup>  fromGroupKnob = AppManager::createKnob<KnobGroup>(effect.get(), kCornerPinParamFrom, 1);
        fromGroupKnob->setName(kCornerPinParamFrom);
        fromGroupKnob->setAsTab();
        fromGroupKnob->setSecret(true);
        transformPage->addKnob(fromGroupKnob);
        fromGroup = fromGroupKnob;
        
        const char* fromPointNames[4] = {kCornerPinParamFrom1, kCornerPinParamFrom2, kCornerPinParamFrom3, kCornerPinParamFrom4};
        const char* toPointNames[4] = {kCornerPinParamTo1, kCornerPinParamTo2, kCornerPinParamTo3, kCornerPinParamTo4};
        const char* enablePointNames[4] = {kCornerPinParamEnable1, kCornerPinParamEnable2, kCornerPinParamEnable3, kCornerPinParamEnable4};
        
        for (int i = 0; i < 4; ++i) {
            
            fromPoints[i] = createDuplicateKnob<KnobDouble>(fromPointNames[i], cNode, effect, transformPage, fromGroupKnob);
            
            toPoints[i] = createDuplicateKnob<KnobDouble>(toPointNames[i], cNode, effect, transformPage, toGroupKnob);
            toPoints[i].lock()->setAddNewLine(false);
            enableToPoint[i] = createDuplicateKnob<KnobBool>(enablePointNames[i], cNode, effect, transformPage, toGroupKnob);
            
        }
        
        cornerPinOverlayPoints = createDuplicateKnob<KnobChoice>(kCornerPinParamOverlayPoints, cNode, effect, transformPage);
        cornerPinOverlayPoints.lock()->setSecret(true);

        cornerPinMatrix = createDuplicateKnob<KnobDouble>(kCornerPinParamMatrix, cNode, effect, transformPage);
        cornerPinMatrix.lock()->setSecret(true);
        
    } // cNode
    
    // Add filtering & motion blur knobs
    if (tNode) {
        
        invertTransform = createDuplicateKnob<KnobBool>(kTransformParamInvert, tNode, effect, transformPage, boost::shared_ptr<KnobGroup>(), cNode);
        filter = createDuplicateKnob<KnobChoice>(kTransformParamFilter, tNode, effect, transformPage, boost::shared_ptr<KnobGroup>(), cNode);
        filter.lock()->setAddNewLine(false);
        clamp = createDuplicateKnob<KnobBool>(kTransformParamClamp, tNode, effect, transformPage, boost::shared_ptr<KnobGroup>(), cNode);
        clamp.lock()->setAddNewLine(false);
        blackOutside = createDuplicateKnob<KnobBool>(kTransformParamBlackOutside, tNode, effect, transformPage, boost::shared_ptr<KnobGroup>(), cNode);
        motionBlur = createDuplicateKnob<KnobDouble>(kTransformParamMotionBlur, tNode, effect, transformPage, boost::shared_ptr<KnobGroup>(), cNode);
        shutter = createDuplicateKnob<KnobDouble>(kTransformParamShutter, tNode, effect, transformPage, boost::shared_ptr<KnobGroup>(), cNode);
        shutterOffset = createDuplicateKnob<KnobChoice>(kTransformParamShutterOffset, tNode, effect, transformPage, boost::shared_ptr<KnobGroup>(), cNode);
        customShutterOffset = createDuplicateKnob<KnobDouble>(kTransformParamCustomShutterOffset, tNode, effect, transformPage, boost::shared_ptr<KnobGroup>(), cNode);
       
    }
    
    boost::shared_ptr<KnobSeparator> exportDataSepKnob = AppManager::createKnob<KnobSeparator>(effect.get(), kTrackerParamExportDataSeparatorLabel, 1, false);
    exportDataSepKnob->setName(kTrackerParamExportDataSeparator);
    transformPage->addKnob(exportDataSepKnob);
    exportDataSep = exportDataSepKnob;
    
    boost::shared_ptr<KnobBool> exporLinkKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamExportLinkLabel, 1, false);
    exporLinkKnob->setName(kTrackerParamExportLink);
    exporLinkKnob->setHintToolTip(kTrackerParamExportLinkHint);
    exporLinkKnob->setAnimationEnabled(false);
    exporLinkKnob->setAddNewLine(false);
    exporLinkKnob->setDefaultValue(true);
    transformPage->addKnob(exporLinkKnob);
    exportLink = exporLinkKnob;
    
   /* boost::shared_ptr<KnobBool> exporUseCurFrameKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamExportUseCurrentFrameLabel, 1, false);
    exporUseCurFrameKnob->setName(kTrackerParamExportUseCurrentFrame);
    exporUseCurFrameKnob->setHintToolTip(kTrackerParamExportUseCurrentFrameHint);
    exporUseCurFrameKnob->setAnimationEnabled(false);
    exporUseCurFrameKnob->setAddNewLine(false);
    exporUseCurFrameKnob->setDefaultValue(false);
    transformPage->addKnob(exporUseCurFrameKnob);
    exportUseCurrentFrame = exporUseCurFrameKnob;
    knobs.push_back(exporUseCurFrameKnob);*/
    
    boost::shared_ptr<KnobButton> exportButtonKnob = AppManager::createKnob<KnobButton>(effect.get(), kTrackerParamExportButtonLabel, 1);
    exportButtonKnob->setName(kTrackerParamExportButton);
    exportButtonKnob->setHintToolTip(kTrackerParamExportButtonHint);
    transformPage->addKnob(exportButtonKnob);
    exportButton = exportButtonKnob;
}


void
TrackArgsLibMV::getEnabledChannels(bool* r, bool* g, bool* b) const
{
    _fa->getEnabledChannels(r,g,b);
}

void
TrackArgsLibMV::getRedrawAreasNeeded(int time, std::list<RectD>* canonicalRects) const
{
    for (std::vector<boost::shared_ptr<TrackMarkerAndOptions> >::const_iterator it = _tracks.begin(); it!=_tracks.end(); ++it) {
        boost::shared_ptr<KnobDouble> searchBtmLeft = (*it)->natronMarker->getSearchWindowBottomLeftKnob();
        boost::shared_ptr<KnobDouble> searchTopRight = (*it)->natronMarker->getSearchWindowTopRightKnob();
        boost::shared_ptr<KnobDouble> centerKnob = (*it)->natronMarker->getCenterKnob();
        boost::shared_ptr<KnobDouble> offsetKnob = (*it)->natronMarker->getOffsetKnob();
        
        Point offset,center,btmLeft,topRight;
        offset.x = offsetKnob->getValueAtTime(time, 0);
        offset.y = offsetKnob->getValueAtTime(time, 1);
        
        center.x = centerKnob->getValueAtTime(time, 0);
        center.y = centerKnob->getValueAtTime(time, 1);
        
        btmLeft.x = searchBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
        btmLeft.y = searchBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
        
        topRight.x = searchTopRight->getValueAtTime(time, 0) + center.x + offset.x;
        topRight.y = searchTopRight->getValueAtTime(time, 1) + center.y + offset.y;
        
        RectD rect;
        rect.x1 = btmLeft.x;
        rect.y1 = btmLeft.y;
        rect.x2 = topRight.x;
        rect.y2 = topRight.y;
        canonicalRects->push_back(rect);
    }
}


/**
 * @brief Set keyframes on knobs from Marker data
 **/
void
TrackerContextPrivate::setKnobKeyframesFromMarker(const mv::Marker& mvMarker,
                                       int formatHeight,
                                       const libmv::TrackRegionResult* result,
                                       const TrackMarkerPtr& natronMarker)
{
    int time = mvMarker.frame;
    boost::shared_ptr<KnobDouble> errorKnob = natronMarker->getErrorKnob();
    if (result) {
        double corr = result->correlation;
        if (corr != corr) {
            corr = 1.;
        }
        errorKnob->setValueAtTime(time, 1. - corr, ViewSpec::current(), 0);
    } else {
        errorKnob->setValueAtTime(time, 0., ViewSpec::current(), 0);
    }
    
    Point center;
    center.x = (double)mvMarker.center(0);
    center.y = (double)TrackerFrameAccessor::invertYCoordinate(mvMarker.center(1), formatHeight);
    
    boost::shared_ptr<KnobDouble> offsetKnob = natronMarker->getOffsetKnob();
    Point offset;
    offset.x = offsetKnob->getValueAtTime(time,0);
    offset.y = offsetKnob->getValueAtTime(time,1);
    
    // Set the center
    boost::shared_ptr<KnobDouble> centerKnob = natronMarker->getCenterKnob();
    centerKnob->setValuesAtTime(time, center.x, center.y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    
    Point topLeftCorner,topRightCorner,btmRightCorner,btmLeftCorner;
    topLeftCorner.x = mvMarker.patch.coordinates(0,0) - offset.x - center.x;
    topLeftCorner.y = TrackerFrameAccessor::invertYCoordinate(mvMarker.patch.coordinates(0,1),formatHeight) - offset.y - center.y;
    
    topRightCorner.x = mvMarker.patch.coordinates(1,0) - offset.x - center.x;
    topRightCorner.y = TrackerFrameAccessor::invertYCoordinate(mvMarker.patch.coordinates(1,1),formatHeight) - offset.y - center.y;
    
    btmRightCorner.x = mvMarker.patch.coordinates(2,0) - offset.x - center.x;
    btmRightCorner.y = TrackerFrameAccessor::invertYCoordinate(mvMarker.patch.coordinates(2,1),formatHeight) - offset.y - center.y;
    
    btmLeftCorner.x = mvMarker.patch.coordinates(3,0) - offset.x - center.x;
    btmLeftCorner.y = TrackerFrameAccessor::invertYCoordinate(mvMarker.patch.coordinates(3,1),formatHeight) - offset.y - center.y;
    
    boost::shared_ptr<KnobDouble> pntTopLeftKnob = natronMarker->getPatternTopLeftKnob();
    boost::shared_ptr<KnobDouble> pntTopRightKnob = natronMarker->getPatternTopRightKnob();
    boost::shared_ptr<KnobDouble> pntBtmLeftKnob = natronMarker->getPatternBtmLeftKnob();
    boost::shared_ptr<KnobDouble> pntBtmRightKnob = natronMarker->getPatternBtmRightKnob();
    
    // Set the pattern Quad
    pntTopLeftKnob->setValuesAtTime(time, topLeftCorner.x, topLeftCorner.y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    pntTopRightKnob->setValuesAtTime(time, topRightCorner.x, topRightCorner.y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    pntBtmLeftKnob->setValuesAtTime(time, btmLeftCorner.x, btmLeftCorner.y,ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    pntBtmRightKnob->setValuesAtTime(time, btmRightCorner.x, btmRightCorner.y, ViewSpec::current(),eValueChangedReasonNatronInternalEdited);
}


/// Converts a Natron track marker to the one used in LibMV. This is expensive: many calls to getValue are made
void
TrackerContextPrivate::natronTrackerToLibMVTracker(bool useRefFrameForSearchWindow,
                                        bool trackChannels[3],
                                        const TrackMarker& marker,
                                        int trackIndex,
                                        int time,
                                        bool forward,
                                        double formatHeight,
                                        mv::Marker* mvMarker)
{
    boost::shared_ptr<KnobDouble> searchWindowBtmLeftKnob = marker.getSearchWindowBottomLeftKnob();
    boost::shared_ptr<KnobDouble> searchWindowTopRightKnob = marker.getSearchWindowTopRightKnob();
    boost::shared_ptr<KnobDouble> patternTopLeftKnob = marker.getPatternTopLeftKnob();
    boost::shared_ptr<KnobDouble> patternTopRightKnob = marker.getPatternTopRightKnob();
    boost::shared_ptr<KnobDouble> patternBtmRightKnob = marker.getPatternBtmRightKnob();
    boost::shared_ptr<KnobDouble> patternBtmLeftKnob = marker.getPatternBtmLeftKnob();
    
#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    boost::shared_ptr<KnobDouble> weightKnob = marker.getWeightKnob();
#endif
    boost::shared_ptr<KnobDouble> centerKnob = marker.getCenterKnob();
    boost::shared_ptr<KnobDouble> offsetKnob = marker.getOffsetKnob();
    
    // We don't use the clip in Natron
    mvMarker->clip = 0;
    mvMarker->reference_clip = 0;
    
#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    mvMarker->weight = (float)weightKnob->getValue();
#else
    mvMarker->weight = 1.;
#endif
    
    mvMarker->frame = time;
    mvMarker->reference_frame = marker.getReferenceFrame(time, forward);
    if (marker.isUserKeyframe(time)) {
        mvMarker->source = mv::Marker::MANUAL;
    } else {
        mvMarker->source = mv::Marker::TRACKED;
    }
    Point nCenter;
    nCenter.x = centerKnob->getValueAtTime(time, 0);
    nCenter.y = centerKnob->getValueAtTime(time, 1);
    mvMarker->center(0) = nCenter.x;
    mvMarker->center(1) = TrackerFrameAccessor::invertYCoordinate(nCenter.y, formatHeight);
    mvMarker->model_type = mv::Marker::POINT;
    mvMarker->model_id = 0;
    mvMarker->track = trackIndex;
    
    mvMarker->disabled_channels =
    (trackChannels[0] ? LIBMV_MARKER_CHANNEL_R : 0) |
    (trackChannels[1] ? LIBMV_MARKER_CHANNEL_G : 0) |
    (trackChannels[2] ? LIBMV_MARKER_CHANNEL_B : 0);
    
    Point searchWndBtmLeft,searchWndTopRight;
    int searchWinTime = useRefFrameForSearchWindow ? mvMarker->reference_frame : time;
    searchWndBtmLeft.x = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, 0);
    searchWndBtmLeft.y = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, 1);
    
    searchWndTopRight.x = searchWindowTopRightKnob->getValueAtTime(searchWinTime, 0);
    searchWndTopRight.y = searchWindowTopRightKnob->getValueAtTime(searchWinTime, 1);
    
    Point offset;
    offset.x = offsetKnob->getValueAtTime(time,0);
    offset.y = offsetKnob->getValueAtTime(time,1);
    
    
    Point tl,tr,br,bl;
    tl.x = patternTopLeftKnob->getValueAtTime(time, 0);
    tl.y = patternTopLeftKnob->getValueAtTime(time, 1);
    
    tr.x = patternTopRightKnob->getValueAtTime(time, 0);
    tr.y = patternTopRightKnob->getValueAtTime(time, 1);
    
    br.x = patternBtmRightKnob->getValueAtTime(time, 0);
    br.y = patternBtmRightKnob->getValueAtTime(time, 1);
    
    bl.x = patternBtmLeftKnob->getValueAtTime(time, 0);
    bl.y = patternBtmLeftKnob->getValueAtTime(time, 1);
    
    /*RectD patternBbox;
     patternBbox.setupInfinity();
     updateBbox(tl, &patternBbox);
     updateBbox(tr, &patternBbox);
     updateBbox(br, &patternBbox);
     updateBbox(bl, &patternBbox);*/
    
    // The search-region is laid out as such:
    //
    //    +----------> x
    //    |
    //    |   (min.x, min.y)           (max.x, min.y)
    //    |        +-------------------------+
    //    |        |                         |
    //    |        |                         |
    //    |        |                         |
    //    |        +-------------------------+
    //    v   (min.x, max.y)           (max.x, max.y)
    //
    mvMarker->search_region.min(0) = searchWndBtmLeft.x + nCenter.x + offset.x;
    mvMarker->search_region.min(1) = TrackerFrameAccessor::invertYCoordinate(searchWndTopRight.y + nCenter.y + offset.y,formatHeight);
    mvMarker->search_region.max(0) = searchWndTopRight.x + nCenter.x + offset.x;
    mvMarker->search_region.max(1) = TrackerFrameAccessor::invertYCoordinate(searchWndBtmLeft.y + nCenter.y + offset.y,formatHeight);
    
    
    // The patch is a quad (4 points); generally in 2D or 3D (here 2D)
    //
    //    +----------> x
    //    |\.
    //    | \.
    //    |  z (z goes into screen)
    //    |
    //    |     r0----->r1
    //    |      ^       |
    //    |      |   .   |
    //    |      |       V
    //    |     r3<-----r2
    //    |              \.
    //    |               \.
    //    v                normal goes away (right handed).
    //    y
    //
    // Each row is one of the corners coordinates; either (x, y) or (x, y, z).
    // TrackMarker extracts the patch coordinates as such:
    /*
     Quad2Df offset_quad = marker.patch;
     Vec2f origin = marker.search_region.Rounded().min;
     offset_quad.coordinates.rowwise() -= origin.transpose();
     QuadToArrays(offset_quad, x, y);
     x[4] = marker.center.x() - origin(0);
     y[4] = marker.center.y() - origin(1);
     */
    // The patch coordinates should be canonical
    
    mvMarker->patch.coordinates(0,0) = tl.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(0,1) = TrackerFrameAccessor::invertYCoordinate(tl.y + nCenter.y + offset.y,formatHeight);
    
    mvMarker->patch.coordinates(1,0) = tr.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(1,1) = TrackerFrameAccessor::invertYCoordinate(tr.y + nCenter.y + offset.y,formatHeight);
    
    mvMarker->patch.coordinates(2,0) = br.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(2,1) = TrackerFrameAccessor::invertYCoordinate(br.y + nCenter.y + offset.y,formatHeight);
    
    mvMarker->patch.coordinates(3,0) = bl.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(3,1) = TrackerFrameAccessor::invertYCoordinate(bl.y + nCenter.y + offset.y,formatHeight);
}



void
TrackerContextPrivate::beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const
{
    options->minimum_correlation = 1. - maxError.lock()->getValue();
    assert(options->minimum_correlation >= 0. && options->minimum_correlation <= 1.);
    options->max_iterations = maxIterations.lock()->getValue();
    options->use_brute_initialization = bruteForcePreTrack.lock()->getValue();
    options->use_normalized_intensities = useNormalizedIntensities.lock()->getValue();
    options->sigma = preBlurSigma.lock()->getValue();
    
}

void
TrackerContextPrivate::endLibMVOptionsForTrack(const TrackMarker& marker,
                                               mv::TrackRegionOptions* options) const
{
    int mode_i = marker.getMotionModelKnob()->getValue();
    switch (mode_i) {
        case 0:
            options->mode = mv::TrackRegionOptions::TRANSLATION;
            break;
        case 1:
            options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION;
            break;
        case 2:
            options->mode = mv::TrackRegionOptions::TRANSLATION_SCALE;
            break;
        case 3:
            options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION_SCALE;
            break;
        case 4:
            options->mode = mv::TrackRegionOptions::AFFINE;
            break;
        case 5:
            options->mode = mv::TrackRegionOptions::HOMOGRAPHY;
            break;
        default:
            options->mode = mv::TrackRegionOptions::AFFINE;
            break;
    }
}


/*
 * @brief This is the internal tracking function that makes use of LivMV to do 1 track step
 * @param trackingIndex This is the index of the Marker we should track in the args
 * @param args Multiple arguments global to the whole track, not just this step
 * @param time The search frame time, that is, the frame to track
 */
bool
TrackerContextPrivate::trackStepLibMV(int trackIndex, const TrackArgsLibMV& args, int time)
{
    assert(trackIndex >= 0 && trackIndex < args.getNumTracks());
    
    const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& tracks = args.getTracks();
    const boost::shared_ptr<TrackMarkerAndOptions>& track = tracks[trackIndex];
    
    boost::shared_ptr<mv::AutoTrack> autoTrack = args.getLibMVAutoTrack();
    QMutex* autoTrackMutex = args.getAutoTrackMutex();
    
    bool enabledChans[3];
    args.getEnabledChannels(&enabledChans[0], &enabledChans[1], &enabledChans[2]);
    
    
    {
        //Add the tracked marker
        QMutexLocker k(autoTrackMutex);
        natronTrackerToLibMVTracker(true, enabledChans, *track->natronMarker, trackIndex, time, args.getForward(), args.getFormatHeight(), &track->mvMarker);
        autoTrack->AddMarker(track->mvMarker);
    }
    
    
    //The frame on the mvMarker should have been set accordingly
    assert(track->mvMarker.frame == time);
    
    if (track->mvMarker.source == mv::Marker::MANUAL) {
        // This is a user keyframe or the first frame, we do not track it
        assert(time == args.getStart() || track->natronMarker->isUserKeyframe(track->mvMarker.frame));
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "TrackStep:" << time << "is a keyframe";
#endif
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), 0, track->natronMarker);
        
    } else {
        
        // Set the reference rame
        track->mvMarker.reference_frame = track->natronMarker->getReferenceFrame(time, args.getForward());
        assert(track->mvMarker.reference_frame != track->mvMarker.frame);
        
        //Make sure the reference frame as the same search window as the tracked frame and exists in the AutoTrack
        {
            QMutexLocker k(autoTrackMutex);
            mv::Marker m;
            natronTrackerToLibMVTracker(false, enabledChans, *track->natronMarker, track->mvMarker.track, track->mvMarker.reference_frame, args.getForward(), args.getFormatHeight(), &m);
            autoTrack->AddMarker(m);
        }
        
        
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << ">>>> Tracking marker" << trackIndex << "at frame" << time <<
        "with reference frame" << track->mvMarker.reference_frame;
#endif
        
        // Do the actual tracking
        libmv::TrackRegionResult result;
        if (!autoTrack->TrackMarker(&track->mvMarker, &result, &track->mvOptions) || !result.is_usable()) {
#ifdef TRACE_LIB_MV
            qDebug() << QThread::currentThread() << "Tracking FAILED (" << (int)result.termination <<  ") for track" << trackIndex << "at frame" << time;
#endif
            // Todo: report error to user
            return false;
        }
        
        //Ok the tracking has succeeded, now the marker is in this state:
        //the source is TRACKED, the search_window has been offset by the center delta,  the center has been offset
        
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "Tracking SUCCESS for track" << trackIndex << "at frame" << time;
#endif
        
        //Extract the marker to the knob keyframes
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), &result, track->natronMarker);
        
        //Add the marker to the autotrack
        /*{
         QMutexLocker k(autoTrackMutex);
         autoTrack->AddMarker(track->mvMarker);
         }*/
    } // if (track->mvMarker.source == mv::Marker::MANUAL) {
    
    appPTR->getAppTLS()->cleanupTLSForThread();
    
    return true;
}

void
TrackerContext::trackMarkers(const std::list<TrackMarkerPtr >& markers,
                             int start,
                             int end,
                             bool forward,
                             ViewerInstance* viewer)
{
    
    if (markers.empty()) {
        return;
    }
    
    /// The channels we are going to use for tracking
    bool enabledChannels[3];
    enabledChannels[0] = _imp->enableTrackRed.lock()->getValue();
    enabledChannels[1] = _imp->enableTrackGreen.lock()->getValue();
    enabledChannels[2] = _imp->enableTrackBlue.lock()->getValue();
    
    double formatWidth,formatHeight;
    Format f;
    getNode()->getApp()->getProject()->getProjectDefaultFormat(&f);
    formatWidth = f.width();
    formatHeight = f.height();
    
    /// The accessor and its cache is local to a track operation, it is wiped once the whole sequence track is finished.
    boost::shared_ptr<TrackerFrameAccessor> accessor(new TrackerFrameAccessor(this, enabledChannels, formatHeight));
    boost::shared_ptr<mv::AutoTrack> trackContext(new mv::AutoTrack(accessor.get()));
    
    
    
    std::vector<boost::shared_ptr<TrackMarkerAndOptions> > trackAndOptions;
    
    mv::TrackRegionOptions mvOptions;
    /*
     Get the global parameters for the LivMV track: pre-blur sigma, No iterations, normalized intensities, etc...
     */
    _imp->beginLibMVOptionsForTrack(&mvOptions);
    
    /*
     For the given markers, do the following:
     - Get the "User" keyframes that have been set and create a LibMV marker for each keyframe as well as for the "start" time
     - Set the "per-track" parameters that were given by the user, that is the mv::TrackRegionOptions
     - t->mvMarker will contain the marker that evolves throughout the tracking
     */
    int trackIndex = 0;
    for (std::list<TrackMarkerPtr >::const_iterator it = markers.begin(); it!= markers.end(); ++it, ++trackIndex) {
        boost::shared_ptr<TrackMarkerAndOptions> t(new TrackMarkerAndOptions);
        t->natronMarker = *it;
        
        std::set<int> userKeys;
        t->natronMarker->getUserKeyframes(&userKeys);
        
        // Add a libmv marker for all keyframes
        for (std::set<int>::iterator it2 = userKeys.begin(); it2 != userKeys.end(); ++it2) {
            
            if (*it2 == start) {
                //Will be added in the track step
                continue;
            } else {
                mv::Marker marker;
                TrackerContextPrivate::natronTrackerToLibMVTracker(false, enabledChannels, *t->natronMarker, trackIndex, *it2, forward, formatHeight, &marker);
                assert(marker.source == mv::Marker::MANUAL);
                trackContext->AddMarker(marker);
            }
            
            
        }
        
        
        //For all already tracked frames which are not keyframes, add them to the AutoTrack too
        std::set<double> centerKeys;
        t->natronMarker->getCenterKeyframes(&centerKeys);
        for (std::set<double>::iterator it2 = centerKeys.begin(); it2 != centerKeys.end(); ++it2) {
            if (userKeys.find(*it2) != userKeys.end()) {
                continue;
            }
            if (*it2 == start) {
                //Will be added in the track step
                continue;
            } else {
                mv::Marker marker;
                TrackerContextPrivate::natronTrackerToLibMVTracker(false, enabledChannels, *t->natronMarker, trackIndex, *it2, forward, formatHeight, &marker);
                assert(marker.source == mv::Marker::TRACKED);
                trackContext->AddMarker(marker);
            }
        }
        
        
        
        
        t->mvOptions = mvOptions;
        _imp->endLibMVOptionsForTrack(*t->natronMarker, &t->mvOptions);
        trackAndOptions.push_back(t);
    }
    
    
    /*
     Launch tracking in the scheduler thread.
     */
    TrackArgsLibMV args(start, end, forward, getNode()->getApp()->getTimeLine(), viewer, trackContext, accessor, trackAndOptions,formatWidth,formatHeight);
    _imp->scheduler.track(args);
}

void
TrackerContextPrivate::linkMarkerKnobsToGuiKnobs(const std::list<TrackMarkerPtr >& markers,
                                                 bool multipleTrackSelected,
                                                 bool slave)
{
    std::list<TrackMarkerPtr >::const_iterator next = markers.begin();
    if (!markers.empty()) {
        ++next;
    }
    for (std::list<TrackMarkerPtr >::const_iterator it = markers.begin() ; it!= markers.end(); ++it) {
        const KnobsVec& trackKnobs = (*it)->getKnobs();
        for (KnobsVec::const_iterator it2 = trackKnobs.begin(); it2 != trackKnobs.end(); ++it2) {
            
            // Find the knob in the TrackerContext knobs
            boost::shared_ptr<KnobI> found;
            for (std::list<boost::weak_ptr<KnobI> >::iterator it3 = perTrackKnobs.begin(); it3 != perTrackKnobs.end(); ++it3) {
                boost::shared_ptr<KnobI> k = it3->lock();
                if (k->getName() == (*it2)->getName()) {
                    found = k;
                    break;
                }
            }
            
            if (!found) {
                continue;
            }
            
            //Clone current state only for the last marker
            if (next == markers.end()) {
                found->cloneAndUpdateGui(it2->get());
            }
            
            //Slave internal knobs
            assert((*it2)->getDimension() == found->getDimension());
            for (int i = 0; i < (*it2)->getDimension(); ++i) {
                if (slave) {
                    (*it2)->slaveTo(i, found, i);
                } else {
                    (*it2)->unSlave(i, !multipleTrackSelected);
                }
            }
            
            if (!slave) {
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,ViewSpec,int,bool)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(ViewSpec,int, double,double)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(ViewSpec,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,ViewSpec,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,ViewSpec,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                
            } else {
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,ViewSpec,int,bool)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(ViewSpec, int,double,double)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(ViewSpec, int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,ViewSpec, int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,ViewSpec,int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
            }
            
        }
        if (next != markers.end()) {
            ++next;
        }
    } // for (std::list<TrackMarkerPtr >::const_iterator it = markers() ; it!=markers(); ++it)
}





void
TrackerContextPrivate::createCornerPinFromSelection(const std::list<TrackMarkerPtr > & selection,
                                                    bool linked)
{
        if (selection.size() > 4 || selection.empty()) {
        Dialogs::errorDialog(QObject::tr("Export").toStdString(),
                             QObject::tr("Export to corner pin needs between 1 and 4 selected tracks.").toStdString());
        
        return;
    }
    
    boost::shared_ptr<KnobDouble> centers[4];
    int i = 0;
    for (std::list<TrackMarkerPtr >::const_iterator it = selection.begin(); it != selection.end(); ++it, ++i) {
        centers[i] = (*it)->getCenterKnob();
        assert(centers[i]);
    }
    
    NodePtr thisNode = node.lock();
    
    AppInstance* app = thisNode->getApp();
    CreateNodeArgs args(QString::fromUtf8(PLUGINID_OFX_CORNERPIN), eCreateNodeReasonInternal, thisNode->getGroup());
    NodePtr cornerPin = app->createNode(args);
    if (!cornerPin) {
        return;
    }
    
    // Move the Corner Pin node
    double thisNodePos[2];
    double thisNodeSize[2];
    thisNode->getPosition(&thisNodePos[0], &thisNodePos[1]);
    thisNode->getSize(&thisNodeSize[0], &thisNodeSize[1]);
    
    cornerPin->setPosition(thisNodePos[0] + thisNodeSize[0] * 2., thisNodePos[1]);
    
    
    /*boost::shared_ptr<KnobDouble> toPoints[4];
    boost::shared_ptr<KnobDouble> fromPoints[4];
    
    int timeForFromPoints = useTransformRefFrame ? _publicInterface->getTransformReferenceFrame() : app->getTimeLine()->currentFrame();
    
    for (unsigned int i = 0; i < selection.size(); ++i) {
        fromPoints[i] = getCornerPinPoint(cornerPin.get(), true, i);
        assert(fromPoints[i] && centers[i]);
        for (int j = 0; j < fromPoints[i]->getDimension(); ++j) {
            fromPoints[i]->setValue(centers[i]->getValueAtTime(timeForFromPoints,j), ViewSpec(0), j);
        }
        
        toPoints[i] = getCornerPinPoint(cornerPin.get(), false, i);
        assert(toPoints[i]);
        if (!linked) {
            toPoints[i]->cloneAndUpdateGui(centers[i].get());
        } else {
            bool ok = false;
            for (int d = 0; d < toPoints[i]->getDimension() ; ++d) {
                ok = dynamic_cast<KnobI*>(toPoints[i].get())->slaveTo(d, centers[i], d);
            }
            (void)ok;
            assert(ok);
        }
    }
    
    ///Disable all non used points
    for (unsigned int i = selection.size(); i < 4; ++i) {
        QString enableName = QString::fromUtf8("enable%1").arg(i + 1);
        KnobPtr knob = cornerPin->getKnobByName( enableName.toStdString() );
        assert(knob);
        KnobBool* enableKnob = dynamic_cast<KnobBool*>( knob.get() );
        assert(enableKnob);
        enableKnob->setValue(false, ViewSpec(0), 0);
    }
    
    if (motionType == eTrackerMotionTypeStabilize) {
        KnobPtr invertKnob = cornerPin->getKnobByName(kCornerPinInvertParamName);
        assert(invertKnob);
        KnobBool* isBool = dynamic_cast<KnobBool*>(invertKnob.get());
        assert(isBool);
        isBool->setValue(true, ViewSpec(0), 0);
    }
    */
}

void
TrackerContextPrivate::createTransformFromSelection(const std::list<TrackMarkerPtr > & selection,
                                                    bool linked)
{
    boost::shared_ptr<KnobChoice> motionTypeKnob = motionType.lock();
    if (!motionTypeKnob) {
        return;
    }
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum mt = (TrackerMotionTypeEnum)motionType_i;

    if (mt == eTrackerMotionTypeNone) {
        Dialogs::errorDialog(QObject::tr("Tracker Export").toStdString(), QObject::tr("Please select the export mode with the Transform Type parameter").toStdString());
        return;
    }

    NodePtr thisNode = node.lock();
    
    AppInstance* app = thisNode->getApp();
    CreateNodeArgs args(QString::fromUtf8(PLUGINID_OFX_TRANSFORM), eCreateNodeReasonInternal, thisNode->getGroup());
    NodePtr transformNode = app->createNode(args);
    if (!transformNode) {
        return;
    }
    
    // Move the Corner Pin node
    double thisNodePos[2];
    double thisNodeSize[2];
    thisNode->getPosition(&thisNodePos[0], &thisNodePos[1]);
    thisNode->getSize(&thisNodeSize[0], &thisNodeSize[1]);
    
    transformNode->setPosition(thisNodePos[0] + thisNodeSize[0] * 2., thisNodePos[1]);
    
#pragma message WARN("TODO")
}

void
TrackerContextPrivate::refreshTransformKnobs()
{
    boost::shared_ptr<KnobChoice> motionTypeKnob = motionType.lock();
    if (!motionTypeKnob) {
        return;
    }
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum motionType = (TrackerMotionTypeEnum)motionType_i;
    
    boost::shared_ptr<KnobChoice> transformTypeKnob = transformType.lock();
    assert(transformTypeKnob);
    
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    
    switch (motionType) {
        case eTrackerMotionTypeNone:
        {
        }   break;
        case eTrackerMotionTypeMatchMove:
        {
        }   break;
        case eTrackerMotionTypeStabilize:
        {
        }   break;
        case eTrackerMotionTypeAddJitter:
        {
        }   break;
        case eTrackerMotionTypeRemoveJitter:
        {
        }   break;
        default:
            break;
    }
}

void
TrackerContextPrivate::refreshVisibilityFromTransformTypeInternal(TrackerTransformNodeEnum transformType)
{
    if (!transformNode.lock()) {
        return;
    }
    
    boost::shared_ptr<KnobChoice> motionTypeKnob = motionType.lock();
    if (!motionTypeKnob) {
        return;
    }
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum motionType = (TrackerMotionTypeEnum)motionType_i;
    
    transformNode.lock()->setNodeDisabled(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    cornerPinNode.lock()->setNodeDisabled(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);
    
    if (transformType == eTrackerTransformNodeTransform) {
        transformControlsSeparator.lock()->setLabel("Transform Controls");
    } else if (transformType == eTrackerTransformNodeCornerPin) {
        transformControlsSeparator.lock()->setLabel("CornerPin Controls");
    }
    
    toGroup.lock()->setSecret(transformType == eTrackerTransformNodeTransform);
    fromGroup.lock()->setSecret(transformType == eTrackerTransformNodeTransform);
    cornerPinOverlayPoints.lock()->setSecret(transformType == eTrackerTransformNodeTransform);
    cornerPinMatrix.lock()->setSecret(transformType == eTrackerTransformNodeTransform);
    
    translate.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    scale.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    scaleUniform.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    rotate.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    center.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    skewX.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    skewY.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    skewOrder.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    filter.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    clamp.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
    blackOutside.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin);
}

void
TrackerContextPrivate::refreshVisibilityFromTransformType()
{
   
    boost::shared_ptr<KnobChoice> transformTypeKnob = transformType.lock();
    assert(transformTypeKnob);
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    refreshVisibilityFromTransformTypeInternal(transformType);
    
}


NATRON_NAMESPACE_EXIT;
