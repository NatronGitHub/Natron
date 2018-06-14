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

#include "TrackerContextPrivate.h"

#include <sstream> // stringstream

#if defined(CERES_USE_OPENMP) && defined(_OPENMP)
#include <omp.h>
#endif

#include <QtCore/QThreadPool>

#include "Engine/AppInstance.h"
#include "Engine/Curve.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/TLSHolder.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerNode.h"
#include "Engine/TrackerContext.h"


#ifdef DEBUG
//#define TRACKER_GENERATE_DATA_SEQUENTIALLY
#endif


NATRON_NAMESPACE_ENTER

/**
 * @brief Creates a duplicate of the knob identified by knobName which is a knob in the internalNode onto the effect and add it to the given page.
 * If otherNode is set, also fetch a knob of the given name on the otherNode and link it to the newly created knob.
 **/

template <typename KNOBTYPE>
boost::shared_ptr<KNOBTYPE>
createDuplicateKnob( const std::string& knobName,
                     const NodePtr& internalNode,
                     const EffectInstancePtr& effect,
                     const KnobPagePtr& page = KnobPagePtr(),
                     const KnobGroupPtr& group = KnobGroupPtr(),
                     const NodePtr& otherNode = NodePtr() )
{
    KnobIPtr internalNodeKnob = internalNode->getKnobByName(knobName);

    if (!internalNodeKnob) {
        return boost::shared_ptr<KNOBTYPE>();
    }
    assert(internalNodeKnob);
    KnobIPtr duplicateKnob = internalNodeKnob->createDuplicateOnHolder(effect.get(), page, group, -1, true, internalNodeKnob->getName(), internalNodeKnob->getLabel(), internalNodeKnob->getHintToolTip(), false, false);

    if (otherNode) {
        KnobIPtr otherNodeKnob = otherNode->getKnobByName(knobName);
        assert(otherNodeKnob);
        for (int i = 0; i < otherNodeKnob->getDimension(); ++i) {
            otherNodeKnob->slaveTo(i, duplicateKnob, i);
        }
    }

    return boost::dynamic_pointer_cast<KNOBTYPE>(duplicateKnob);
}

TrackerContextPrivate::TrackerContextPrivate(TrackerContext* publicInterface,
                                             const NodePtr &node)
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
    , scheduler(_publicInterface, node)
{
    EffectInstancePtr effect = node->getEffectInstance();
    //needs to be blocking, otherwise the progressUpdate() call could be made before startProgress
    QObject::connect( &scheduler, SIGNAL(trackingStarted(int)), _publicInterface, SLOT(onSchedulerTrackingStarted(int)) );
    QObject::connect( &scheduler, SIGNAL(trackingFinished()), _publicInterface, SLOT(onSchedulerTrackingFinished()) );
    QObject::connect( &scheduler, SIGNAL(trackingProgress(double)), _publicInterface, SLOT(onSchedulerTrackingProgress(double)) );
    TrackerNodePtr isTrackerNode = boost::dynamic_pointer_cast<TrackerNode>(effect);
    QString fixedNamePrefix = QString::fromUtf8( node->getScriptName_mt_safe().c_str() );

    fixedNamePrefix.append( QLatin1Char('_') );
    if (isTrackerNode) {
        NodePtr input, output;
        //NodePtr maskInput;

        {
            CreateNodeArgs args(PLUGINID_NATRON_OUTPUT, isTrackerNode);
            args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
            args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);

            output = node->getApp()->createNode(args);
            try {
                output->setScriptName("Output");
            } catch (...) {
            }

            assert(output);
        }
        {
            CreateNodeArgs args(PLUGINID_NATRON_INPUT, isTrackerNode);
            args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
            args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
            args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "Source");
            input = node->getApp()->createNode(args);
            assert(input);
        }


        {
            QString cornerPinName = fixedNamePrefix + QLatin1String("CornerPin");
            CreateNodeArgs args(PLUGINID_OFX_CORNERPIN, isTrackerNode);
            args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
            args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
            args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, cornerPinName.toStdString());
            NodePtr cpNode = node->getApp()->createNode(args);
            if (!cpNode) {
                throw std::runtime_error( tr("The Tracker node requires the Misc.ofx.bundle plug-in to be installed").toStdString() );
            }
            cpNode->setNodeDisabled(true);
            cornerPinNode = cpNode;
        }

        {
            QString transformName = fixedNamePrefix + QLatin1String("Transform");
            CreateNodeArgs args(PLUGINID_OFX_TRANSFORM, isTrackerNode);
            args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
            args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
            args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, transformName.toStdString());
            NodePtr tNode = node->getApp()->createNode(args);
            tNode->setNodeDisabled(true);
            transformNode = tNode;

            output->connectInput(tNode, 0);
            NodePtr cpNode = cornerPinNode.lock();
            tNode->connectInput(cpNode, 0);
            cpNode->connectInput(input, 0);
        }
    }

    KnobPagePtr settingsPage = AppManager::createKnob<KnobPage>(effect.get(), tr("Tracking"), 1, false);
    trackingPageKnob = settingsPage;
    KnobPagePtr transformPage = AppManager::createKnob<KnobPage>(effect.get(), tr("Transform"), 1, false);
    transformPageKnob = transformPage;


#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    KnobBoolPtr enablePatternMatching = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamUsePatternMatchingLabel), 1);
    enablePatternMatching->setName(kTrackerParamUsePatternMatching);
    enablePatternMatching->setHintToolTip( tr(kTrackerParamUsePatternMatchingHint) );
    enablePatternMatching->setDefaultValue(false);
    enablePatternMatching->setAnimationEnabled(false);
    enablePatternMatching->setAddNewLine(false);
    enablePatternMatching->setEvaluateOnChange(false);
    settingsPage->addKnob(enablePatternMatching);
    usePatternMatching = enablePatternMatching;

    KnobChoicePtr patternMatchingScoreKnob = AppManager::createKnob<KnobChoice>(effect.get(), tr(kTrackerParamPatternMatchingScoreTypeLabel), 1, false);
    patternMatchingScoreKnob->setName(kTrackerParamPatternMatchingScoreType);
    patternMatchingScoreKnob->setHintToolTip( tr(kTrackerParamPatternMatchingScoreTypeHint) );
    {
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption(kTrackerParamPatternMatchingScoreOptionSSD, "", tr(kTrackerParamPatternMatchingScoreOptionSSDHint).toStdString()));
        choices.push_back(ChoiceOption(kTrackerParamPatternMatchingScoreOptionSAD, "", tr(kTrackerParamPatternMatchingScoreOptionSADHint).toStdString()));
        choices.push_back(ChoiceOption(kTrackerParamPatternMatchingScoreOptionNCC, "", tr(kTrackerParamPatternMatchingScoreOptionNCCHint).toStdString()));
        choices.push_back(ChoiceOption(kTrackerParamPatternMatchingScoreOptionZNCC, "", tr(kTrackerParamPatternMatchingScoreOptionZNCCHint).toStdString()));
        patternMatchingScoreKnob->populateChoices(choices);
    }
    patternMatchingScoreKnob->setDefaultValue(1); // SAD
    patternMatchingScoreKnob->setAnimationEnabled(false);
    patternMatchingScoreKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(patternMatchingScoreKnob);
    patternMatchingScore = patternMatchingScoreKnob;
#endif

    KnobBoolPtr enableTrackRedKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamTrackRedLabel), 1, false);
    enableTrackRedKnob->setName(kTrackerParamTrackRed);
    enableTrackRedKnob->setHintToolTip( tr(kTrackerParamTrackRedHint) );
    enableTrackRedKnob->setDefaultValue(true);
    enableTrackRedKnob->setAnimationEnabled(false);
    enableTrackRedKnob->setAddNewLine(false);
    enableTrackRedKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(enableTrackRedKnob);
    enableTrackRed = enableTrackRedKnob;

    KnobBoolPtr enableTrackGreenKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamTrackGreenLabel), 1, false);
    enableTrackGreenKnob->setName(kTrackerParamTrackGreen);
    enableTrackGreenKnob->setHintToolTip( tr(kTrackerParamTrackGreenHint) );
    enableTrackGreenKnob->setDefaultValue(true);
    enableTrackGreenKnob->setAnimationEnabled(false);
    enableTrackGreenKnob->setAddNewLine(false);
    enableTrackGreenKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(enableTrackGreenKnob);
    enableTrackGreen = enableTrackGreenKnob;

    KnobBoolPtr enableTrackBlueKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamTrackBlueLabel), 1, false);
    enableTrackBlueKnob->setName(kTrackerParamTrackBlue);
    enableTrackBlueKnob->setHintToolTip( tr(kTrackerParamTrackBlueHint) );
    enableTrackBlueKnob->setDefaultValue(true);
    enableTrackBlueKnob->setAnimationEnabled(false);
    enableTrackBlueKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(enableTrackBlueKnob);
    enableTrackBlue = enableTrackBlueKnob;

    KnobDoublePtr maxErrorKnob = AppManager::createKnob<KnobDouble>(effect.get(), tr(kTrackerParamMaxErrorLabel), 1, false);
    maxErrorKnob->setName(kTrackerParamMaxError);
    maxErrorKnob->setHintToolTip( tr(kTrackerParamMaxErrorHint) );
    maxErrorKnob->setAnimationEnabled(false);
    maxErrorKnob->setMinimum(0.);
    maxErrorKnob->setMaximum(1.);
    maxErrorKnob->setDefaultValue(0.25);
    maxErrorKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(maxErrorKnob);
    maxError = maxErrorKnob;

    KnobIntPtr maxItKnob = AppManager::createKnob<KnobInt>(effect.get(), tr(kTrackerParamMaximumIterationLabel), 1, false);
    maxItKnob->setName(kTrackerParamMaximumIteration);
    maxItKnob->setHintToolTip( tr(kTrackerParamMaximumIterationHint) );
    maxItKnob->setAnimationEnabled(false);
    maxItKnob->setMinimum(0);
    maxItKnob->setMaximum(150);
    maxItKnob->setEvaluateOnChange(false);
    maxItKnob->setDefaultValue(50);
    settingsPage->addKnob(maxItKnob);
    maxIterations = maxItKnob;

    KnobBoolPtr usePretTrackBF = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamBruteForcePreTrackLabel), 1, false);
    usePretTrackBF->setName(kTrackerParamBruteForcePreTrack);
    usePretTrackBF->setHintToolTip( tr(kTrackerParamBruteForcePreTrackHint) );
    usePretTrackBF->setDefaultValue(true);
    usePretTrackBF->setAnimationEnabled(false);
    usePretTrackBF->setEvaluateOnChange(false);
    usePretTrackBF->setAddNewLine(false);
    settingsPage->addKnob(usePretTrackBF);
    bruteForcePreTrack = usePretTrackBF;

    KnobBoolPtr useNormalizedInt = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamNormalizeIntensitiesLabel), 1, false);
    useNormalizedInt->setName(kTrackerParamNormalizeIntensities);
    useNormalizedInt->setHintToolTip( tr(kTrackerParamNormalizeIntensitiesHint) );
    useNormalizedInt->setDefaultValue(false);
    useNormalizedInt->setAnimationEnabled(false);
    useNormalizedInt->setEvaluateOnChange(false);
    settingsPage->addKnob(useNormalizedInt);
    useNormalizedIntensities = useNormalizedInt;

    KnobDoublePtr preBlurSigmaKnob = AppManager::createKnob<KnobDouble>(effect.get(), tr(kTrackerParamPreBlurSigmaLabel), 1, false);
    preBlurSigmaKnob->setName(kTrackerParamPreBlurSigma);
    preBlurSigmaKnob->setHintToolTip( tr(kTrackerParamPreBlurSigmaHint) );
    preBlurSigmaKnob->setAnimationEnabled(false);
    preBlurSigmaKnob->setMinimum(0);
    preBlurSigmaKnob->setMaximum(10.);
    preBlurSigmaKnob->setDefaultValue(0.9);
    preBlurSigmaKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(preBlurSigmaKnob);
    preBlurSigma = preBlurSigmaKnob;

    KnobIntPtr defPatternWinSizeKnob = AppManager::createKnob<KnobInt>(effect.get(), tr(kTrackerParamDefaultMarkerPatternWinSizeLabel), 1, false);
    defPatternWinSizeKnob->setName(kTrackerParamDefaultMarkerPatternWinSize);
    defPatternWinSizeKnob->setInViewerContextLabel(tr(kTrackerParamDefaultMarkerPatternWinSizeLabel));
    defPatternWinSizeKnob->setHintToolTip( tr(kTrackerParamDefaultMarkerPatternWinSizeHint) );
    defPatternWinSizeKnob->setIconLabel(NATRON_IMAGES_PATH "patternSize.png");
    defPatternWinSizeKnob->setAnimationEnabled(false);
    defPatternWinSizeKnob->setMinimum(1);
    defPatternWinSizeKnob->disableSlider();
    defPatternWinSizeKnob->setAddNewLine(false);
    defPatternWinSizeKnob->setEvaluateOnChange(false);
    defPatternWinSizeKnob->setDefaultValue(21);
    defPatternWinSizeKnob->setSecret(true);
    settingsPage->addKnob(defPatternWinSizeKnob);
    defaultPatternWinSize = defPatternWinSizeKnob;

    KnobIntPtr defSearchWinSizeKnob = AppManager::createKnob<KnobInt>(effect.get(), tr(kTrackerParamDefaultMarkerSearchWinSizeLabel), 1, false);
    defSearchWinSizeKnob->setName(kTrackerParamDefaultMarkerSearchWinSize);
    defSearchWinSizeKnob->setInViewerContextLabel(tr(kTrackerParamDefaultMarkerSearchWinSizeLabel));
    defSearchWinSizeKnob->setHintToolTip( tr(kTrackerParamDefaultMarkerSearchWinSizeHint) );
    defSearchWinSizeKnob->setIconLabel(NATRON_IMAGES_PATH "searchSize.png");
    defSearchWinSizeKnob->setAnimationEnabled(false);
    defSearchWinSizeKnob->setMinimum(1);
    defSearchWinSizeKnob->disableSlider();
    defSearchWinSizeKnob->setEvaluateOnChange(false);
    defSearchWinSizeKnob->setSecret(true);
    defSearchWinSizeKnob->setDefaultValue(71);
    settingsPage->addKnob(defSearchWinSizeKnob);
    defaultSearchWinSize = defSearchWinSizeKnob;

    KnobChoicePtr defMotionModelKnob = AppManager::createKnob<KnobChoice>(effect.get(), tr(kTrackerParamDefaultMotionModelLabel), 1, false);
    defMotionModelKnob->setName(kTrackerParamDefaultMotionModel);
    defMotionModelKnob->setAnimationEnabled(false);
    {
        std::vector<ChoiceOption> choices;
        TrackerContext::getMotionModelsAndHelps(false, &choices);
        defMotionModelKnob->populateChoices(choices);

    }
    defMotionModelKnob->setSecret(true);
    defMotionModelKnob->setAnimationEnabled(false);
    defMotionModelKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(defMotionModelKnob);
    defaultMotionModel = defMotionModelKnob;


    KnobSeparatorPtr  perTrackSeparatorKnob = AppManager::createKnob<KnobSeparator>(effect.get(), tr(kTrackerParamPerTrackParamsSeparatorLabel), 3);
    perTrackSeparatorKnob->setName(kTrackerParamPerTrackParamsSeparator);
    settingsPage->addKnob(perTrackSeparatorKnob);
    perTrackParamsSeparator = perTrackSeparatorKnob;

    KnobBoolPtr enableTrackKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamEnabledLabel), 1, false);
    enableTrackKnob->setName(kTrackerParamEnabled);
    enableTrackKnob->setHintToolTip( tr(kTrackerParamEnabledHint) );
    enableTrackKnob->setAnimationEnabled(true);
    enableTrackKnob->setDefaultValue(true);
    enableTrackKnob->setEvaluateOnChange(false);
    enableTrackKnob->setAllDimensionsEnabled(false);
    enableTrackKnob->setAddNewLine(false);
    settingsPage->addKnob(enableTrackKnob);
    activateTrack = enableTrackKnob;
    perTrackKnobs.push_back(enableTrackKnob);
    
    KnobBoolPtr autoKeyEnabledKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamAutoKeyEnabledLabel), 1, false);
    autoKeyEnabledKnob->setName(kTrackerParamAutoKeyEnabled);
    autoKeyEnabledKnob->setHintToolTip( tr(kTrackerParamAutoKeyEnabledHint) );
    autoKeyEnabledKnob->setAnimationEnabled(false);
    autoKeyEnabledKnob->setDefaultValue(false);
    autoKeyEnabledKnob->setEvaluateOnChange(false);
    settingsPage->addKnob(autoKeyEnabledKnob);
    autoKeyEnabled = autoKeyEnabledKnob;

    

    KnobChoicePtr motionModelKnob = AppManager::createKnob<KnobChoice>(effect.get(), tr(kTrackerParamMotionModelLabel), 1, false);
    motionModelKnob->setName(kTrackerParamMotionModel);
    motionModelKnob->setHintToolTip( tr(kTrackerParamMotionModelHint) );
    {
        std::vector<ChoiceOption> choices;
        TrackerContext::getMotionModelsAndHelps(false, &choices);
        motionModelKnob->populateChoices(choices);
    }
    motionModelKnob->setAllDimensionsEnabled(false);
    motionModelKnob->setAnimationEnabled(false);
    motionModelKnob->setEvaluateOnChange(false);
    perTrackKnobs.push_back(motionModelKnob);
    motionModel = motionModelKnob;
    settingsPage->addKnob(motionModelKnob);


    KnobSeparatorPtr  transformGenerationSeparatorKnob = AppManager::createKnob<KnobSeparator>(effect.get(), tr("Transform Generation"), 3);
    transformPage->addKnob(transformGenerationSeparatorKnob);
    transformGenerationSeparator = transformGenerationSeparatorKnob;

    KnobChoicePtr motionTypeKnob = AppManager::createKnob<KnobChoice>(effect.get(), tr(kTrackerParamMotionTypeLabel), 1);
    motionTypeKnob->setName(kTrackerParamMotionType);
    motionTypeKnob->setHintToolTip( tr(kTrackerParamMotionTypeHint) );
    {
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption(kTrackerParamMotionTypeNone, "", tr(kTrackerParamMotionTypeNoneHelp).toStdString()));
        choices.push_back(ChoiceOption(kTrackerParamMotionTypeStabilize, "", tr(kTrackerParamMotionTypeStabilizeHelp).toStdString()));
        choices.push_back(ChoiceOption(kTrackerParamMotionTypeMatchMove, "", tr(kTrackerParamMotionTypeMatchMoveHelp).toStdString()));
        choices.push_back(ChoiceOption(kTrackerParamMotionTypeRemoveJitter, "", tr(kTrackerParamMotionTypeRemoveJitterHelp).toStdString()));
        choices.push_back(ChoiceOption(kTrackerParamMotionTypeAddJitter, "", tr(kTrackerParamMotionTypeAddJitterHelp).toStdString()));

        motionTypeKnob->populateChoices(choices);


    }
    motionTypeKnob->setAddNewLine(false);
    motionType = motionTypeKnob;
    transformPage->addKnob(motionTypeKnob);

    KnobChoicePtr transformTypeKnob = AppManager::createKnob<KnobChoice>(effect.get(), tr(kTrackerParamTransformTypeLabel), 1);
    transformTypeKnob->setName(kTrackerParamTransformType);
    transformTypeKnob->setHintToolTip( tr(kTrackerParamTransformTypeHint) );
    {
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption(kTrackerParamTransformTypeTransform, "", tr(kTrackerParamTransformTypeTransformHelp).toStdString()));
        choices.push_back(ChoiceOption(kTrackerParamTransformTypeCornerPin, "", tr(kTrackerParamTransformTypeTransformHelp).toStdString()));
        transformTypeKnob->populateChoices(choices);
    }
    transformTypeKnob->setDefaultValue(1);
    transformType = transformTypeKnob;
    transformPage->addKnob(transformTypeKnob);

    KnobIntPtr referenceFrameKnob = AppManager::createKnob<KnobInt>(effect.get(), tr(kTrackerParamReferenceFrameLabel), 1);
    referenceFrameKnob->setName(kTrackerParamReferenceFrame);
    referenceFrameKnob->setHintToolTip( tr(kTrackerParamReferenceFrameHint) );
    referenceFrameKnob->setAnimationEnabled(false);
    referenceFrameKnob->setDefaultValue(1);
    referenceFrameKnob->setAddNewLine(false);
    referenceFrameKnob->setEvaluateOnChange(false);
    transformPage->addKnob(referenceFrameKnob);
    referenceFrame = referenceFrameKnob;

    KnobButtonPtr setCurrentFrameKnob = AppManager::createKnob<KnobButton>(effect.get(), tr(kTrackerParamSetReferenceFrameLabel), 1);
    setCurrentFrameKnob->setName(kTrackerParamSetReferenceFrame);
    setCurrentFrameKnob->setHintToolTip( tr(kTrackerParamSetReferenceFrameHint) );
    transformPage->addKnob(setCurrentFrameKnob);
    setCurrentFrameButton = setCurrentFrameKnob;

    KnobIntPtr  jitterPeriodKnob = AppManager::createKnob<KnobInt>(effect.get(), tr(kTrackerParamJitterPeriodLabel), 1);
    jitterPeriodKnob->setName(kTrackerParamJitterPeriod);
    jitterPeriodKnob->setHintToolTip( tr(kTrackerParamJitterPeriodHint) );
    jitterPeriodKnob->setAnimationEnabled(false);
    jitterPeriodKnob->setDefaultValue(10);
    jitterPeriodKnob->setMinimum(0, 0);
    jitterPeriodKnob->setEvaluateOnChange(false);
    transformPage->addKnob(jitterPeriodKnob);
    jitterPeriod = jitterPeriodKnob;

    KnobIntPtr  smoothTransformKnob = AppManager::createKnob<KnobInt>(effect.get(), tr(kTrackerParamSmoothLabel), 3);
    smoothTransformKnob->setName(kTrackerParamSmooth);
    smoothTransformKnob->setHintToolTip( tr(kTrackerParamSmoothHint) );
    smoothTransformKnob->setAnimationEnabled(false);
    smoothTransformKnob->disableSlider();
    smoothTransformKnob->setDimensionName(0, "t");
    smoothTransformKnob->setDimensionName(1, "r");
    smoothTransformKnob->setDimensionName(2, "s");
    for (int i = 0; i < 3; ++i) {
        smoothTransformKnob->setMinimum(0, i);
    }
    smoothTransformKnob->setEvaluateOnChange(false);
    transformPage->addKnob(smoothTransformKnob);
    smoothTransform = smoothTransformKnob;

    KnobIntPtr  smoothCornerPinKnob = AppManager::createKnob<KnobInt>(effect.get(), tr(kTrackerParamSmoothCornerPinLabel), 1);
    smoothCornerPinKnob->setName(kTrackerParamSmoothCornerPin);
    smoothCornerPinKnob->setHintToolTip( tr(kTrackerParamSmoothCornerPinHint) );
    smoothCornerPinKnob->setAnimationEnabled(false);
    smoothCornerPinKnob->disableSlider();
    smoothCornerPinKnob->setMinimum(0, 0);
    smoothCornerPinKnob->setEvaluateOnChange(false);
    smoothCornerPinKnob->setSecret(true);
    transformPage->addKnob(smoothCornerPinKnob);
    smoothCornerPin = smoothCornerPinKnob;

    KnobBoolPtr  autoGenerateTransformKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamAutoGenerateTransformLabel), 1);
    autoGenerateTransformKnob->setName(kTrackerParamAutoGenerateTransform);
    autoGenerateTransformKnob->setHintToolTip( tr(kTrackerParamAutoGenerateTransformHint) );
    autoGenerateTransformKnob->setAnimationEnabled(false);
    autoGenerateTransformKnob->setEvaluateOnChange(false);
    autoGenerateTransformKnob->setDefaultValue(true);
    autoGenerateTransformKnob->setAddNewLine(false);
    transformPage->addKnob(autoGenerateTransformKnob);
    autoGenerateTransform = autoGenerateTransformKnob;


    KnobButtonPtr computeTransformKnob = AppManager::createKnob<KnobButton>(effect.get(), tr(kTrackerParamGenerateTransformLabel), 1);
    computeTransformKnob->setName(kTrackerParamGenerateTransform);
    computeTransformKnob->setHintToolTip( tr(kTrackerParamGenerateTransformHint) );
    computeTransformKnob->setEvaluateOnChange(false);
    //computeTransformKnob->setAddNewLine(false);
    transformPage->addKnob(computeTransformKnob);
    generateTransformButton = computeTransformKnob;

    KnobBoolPtr  robustModelKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamRobustModelLabel), 1);
    robustModelKnob->setName(kTrackerParamRobustModel);
    robustModelKnob->setHintToolTip( tr(kTrackerParamRobustModelHint) );
    robustModelKnob->setAnimationEnabled(false);
    robustModelKnob->setEvaluateOnChange(false);
    robustModelKnob->setDefaultValue(true);
    robustModelKnob->setAddNewLine(false);
    transformPage->addKnob(robustModelKnob);
    robustModel = robustModelKnob;

    KnobStringPtr  fittingErrorWarningKnob = AppManager::createKnob<KnobString>(effect.get(), tr(kTrackerParamFittingErrorWarningLabel), 1);
    fittingErrorWarningKnob->setName(kTrackerParamFittingErrorWarning);
    fittingErrorWarningKnob->setHintToolTip( tr(kTrackerParamFittingErrorWarningHint) );
    fittingErrorWarningKnob->setAnimationEnabled(false);
    fittingErrorWarningKnob->setDefaultValue( tr(kTrackerParamFittingErrorWarningLabel).toStdString() );
    fittingErrorWarningKnob->setIconLabel("dialog-warning");
    fittingErrorWarningKnob->setEvaluateOnChange(false);
    fittingErrorWarningKnob->setSecretByDefault(true);
    fittingErrorWarningKnob->setAsLabel();
    transformPage->addKnob(fittingErrorWarningKnob);
    fittingErrorWarning = fittingErrorWarningKnob;


    KnobDoublePtr  fittingErrorKnob = AppManager::createKnob<KnobDouble>(effect.get(), tr(kTrackerParamFittingErrorLabel), 1);
    fittingErrorKnob->setName(kTrackerParamFittingError);
    fittingErrorKnob->setHintToolTip( tr(kTrackerParamFittingErrorHint) );
    fittingErrorKnob->setEvaluateOnChange(false);
    fittingErrorKnob->setAddNewLine(false);
    fittingErrorKnob->setAllDimensionsEnabled(false);
    transformPage->addKnob(fittingErrorKnob);
    fittingError = fittingErrorKnob;

    KnobDoublePtr  fittingErrorWarnAboveKnob = AppManager::createKnob<KnobDouble>(effect.get(), tr(kTrackerParamFittingErrorWarnValueLabel), 1);
    fittingErrorWarnAboveKnob->setName(kTrackerParamFittingErrorWarnValue);
    fittingErrorWarnAboveKnob->setHintToolTip( tr(kTrackerParamFittingErrorWarnValueHint) );
    fittingErrorWarnAboveKnob->setAnimationEnabled(false);
    fittingErrorWarnAboveKnob->setEvaluateOnChange(false);
    fittingErrorWarnAboveKnob->setDefaultValue(1);
    transformPage->addKnob(fittingErrorWarnAboveKnob);
    fittingErrorWarnIfAbove = fittingErrorWarnAboveKnob;


    KnobStringPtr transformOutOfDateLabelKnob = AppManager::createKnob<KnobString>(effect.get(), std::string(), 1);
    transformOutOfDateLabelKnob->setName(kTrackerParamTransformOutOfDate);
    transformOutOfDateLabelKnob->setHintToolTip( tr(kTrackerParamTransformOutOfDateHint) );
    transformOutOfDateLabelKnob->setIconLabel("dialog-warning");
    transformOutOfDateLabelKnob->setAsLabel();
    transformOutOfDateLabelKnob->setEvaluateOnChange(false);
    transformOutOfDateLabelKnob->setSecret(true);
    transformPage->addKnob(transformOutOfDateLabelKnob);
    transformOutOfDateLabel = transformOutOfDateLabelKnob;


    KnobSeparatorPtr  transformSeparator = AppManager::createKnob<KnobSeparator>(effect.get(), tr("Transform Controls"), 3);
    transformPage->addKnob(transformSeparator);
    transformSeparator->setSecret(true);
    transformControlsSeparator = transformSeparator;

    KnobBoolPtr disableTransformKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamDisableTransformLabel), 1);
    disableTransformKnob->setName(kTrackerParamDisableTransform);
    disableTransformKnob->setHintToolTip( tr(kTrackerParamDisableTransformHint) );
    disableTransformKnob->setEvaluateOnChange(false);
    disableTransformKnob->setSecret(true);
    transformPage->addKnob(disableTransformKnob);
    disableTransform = disableTransformKnob;


    NodePtr tNode = transformNode.lock();
    if (tNode) {
        KnobDoublePtr tKnob = createDuplicateKnob<KnobDouble>(kTransformParamTranslate, tNode, effect, transformPage);
        tKnob->setSecret(true);
        translate = tKnob;
        KnobDoublePtr rotKnob = createDuplicateKnob<KnobDouble>(kTransformParamRotate, tNode, effect, transformPage);
        rotKnob->setSecret(true);
        rotate = rotKnob;
        KnobDoublePtr scaleKnob = createDuplicateKnob<KnobDouble>(kTransformParamScale, tNode, effect, transformPage);
        scaleKnob->setAddNewLine(false);
        scaleKnob->setSecret(true);
        scale = scaleKnob;
        KnobBoolPtr scaleUniKnob = createDuplicateKnob<KnobBool>(kTransformParamUniform, tNode, effect, transformPage);
        scaleUniKnob->setSecret(true);
        scaleUniform = scaleUniKnob;
        KnobDoublePtr skewXKnob = createDuplicateKnob<KnobDouble>(kTransformParamSkewX, tNode, effect, transformPage);
        skewXKnob->setSecret(true);
        skewX = skewXKnob;
        KnobDoublePtr skewYKnob = createDuplicateKnob<KnobDouble>(kTransformParamSkewY, tNode, effect, transformPage);
        skewYKnob->setSecret(true);
        skewY = skewYKnob;
        KnobChoicePtr skewOrderKnob = createDuplicateKnob<KnobChoice>(kTransformParamSkewOrder, tNode, effect, transformPage);
        skewOrderKnob->setSecret(true);
        skewOrder = skewOrderKnob;
        KnobDoublePtr centerKnob = createDuplicateKnob<KnobDouble>(kTransformParamCenter, tNode, effect, transformPage);
        centerKnob->setSecret(true);
        center = centerKnob;
    } // tNode
    NodePtr cNode = cornerPinNode.lock();
    if (cNode) {
        KnobGroupPtr  toGroupKnob = AppManager::createKnob<KnobGroup>(effect.get(), tr(kCornerPinParamTo), 1);
        toGroupKnob->setName(kCornerPinParamTo);
        toGroupKnob->setAsTab();
        toGroupKnob->setSecret(true);
        transformPage->addKnob(toGroupKnob);
        toGroup = toGroupKnob;

        KnobGroupPtr  fromGroupKnob = AppManager::createKnob<KnobGroup>(effect.get(), tr(kCornerPinParamFrom), 1);
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
        KnobButtonPtr setToInputRod = AppManager::createKnob<KnobButton>(effect.get(), tr(kCornerPinParamSetToInputRoDLabel), 1);
        setToInputRod->setName(kCornerPinParamSetToInputRoD);
        setToInputRod->setHintToolTip( tr(kCornerPinParamSetToInputRoDHint) );
        fromGroupKnob->addKnob(setToInputRod);
        setFromPointsToInputRod = setToInputRod;

        KnobBoolPtr cornerPinSet = AppManager::createKnob<KnobBool>( effect.get(), std::string(kTrackerParamCornerPinFromPointsSetOnce) );
        cornerPinSet->setSecretByDefault(true);
        fromGroupKnob->addKnob(cornerPinSet);
        cornerPinFromPointsSetOnceAutomatically = cornerPinSet;

        cornerPinOverlayPoints = createDuplicateKnob<KnobChoice>(kCornerPinParamOverlayPoints, cNode, effect, transformPage);
        cornerPinOverlayPoints.lock()->setSecret(true);

        KnobDoublePtr matrix = createDuplicateKnob<KnobDouble>(kCornerPinParamMatrix, cNode, effect, transformPage);
        if (matrix) {
            cornerPinMatrix = matrix;
            matrix->setSecret(true);
        }
    } // cNode

    // Add filtering & motion blur knobs
    if (tNode) {
        KnobBoolPtr invertTransformKnob = createDuplicateKnob<KnobBool>(kTransformParamInvert, tNode, effect, transformPage, KnobGroupPtr(), cNode);
        invertTransformKnob->setSecret(true);
        invertTransform = invertTransformKnob;
        KnobChoicePtr filterKnob = createDuplicateKnob<KnobChoice>(kTransformParamFilter, tNode, effect, transformPage, KnobGroupPtr(), cNode);
        filterKnob->setSecret(true);
        filterKnob->setAddNewLine(false);
        filter = filterKnob;
        KnobBoolPtr clampKnob = createDuplicateKnob<KnobBool>(kTransformParamClamp, tNode, effect, transformPage, KnobGroupPtr(), cNode);
        clampKnob->setSecret(true);
        clampKnob->setAddNewLine(false);
        clamp = clampKnob;
        KnobBoolPtr blackOutsideKnob = createDuplicateKnob<KnobBool>(kTransformParamBlackOutside, tNode, effect, transformPage, KnobGroupPtr(), cNode);
        blackOutsideKnob->setSecret(true);
        blackOutside = blackOutsideKnob;
        KnobDoublePtr motionBlurKnob = createDuplicateKnob<KnobDouble>(kTransformParamMotionBlur, tNode, effect, transformPage, KnobGroupPtr(), cNode);
        motionBlurKnob->setSecret(true);
        motionBlur = motionBlurKnob;
        KnobDoublePtr shutterKnob = createDuplicateKnob<KnobDouble>(kTransformParamShutter, tNode, effect, transformPage, KnobGroupPtr(), cNode);
        shutterKnob->setSecret(true);
        shutter = shutterKnob;
        KnobChoicePtr shutterOffsetKnob = createDuplicateKnob<KnobChoice>(kTransformParamShutterOffset, tNode, effect, transformPage, KnobGroupPtr(), cNode);
        shutterOffsetKnob->setSecret(true);
        shutterOffset = shutterOffsetKnob;
        KnobDoublePtr customShutterOffsetKnob = createDuplicateKnob<KnobDouble>(kTransformParamCustomShutterOffset, tNode, effect, transformPage, KnobGroupPtr(), cNode);
        customShutterOffsetKnob->setSecret(true);
        customShutterOffset = customShutterOffsetKnob;

        node->addTransformInteract(translate.lock(), scale.lock(), scaleUniform.lock(), rotate.lock(), skewX.lock(), skewY.lock(), skewOrder.lock(), center.lock(), invertTransform.lock(), KnobBoolPtr() /*interactive*/);

        node->addCornerPinInteract(fromPoints[0].lock(), fromPoints[1].lock(), fromPoints[2].lock(), fromPoints[3].lock(),
                                   toPoints[0].lock(), toPoints[1].lock(), toPoints[2].lock(), toPoints[3].lock(),
                                   enableToPoint[0].lock(), enableToPoint[1].lock(), enableToPoint[2].lock(), enableToPoint[3].lock(), cornerPinOverlayPoints.lock(), invertTransform.lock(), KnobBoolPtr() /*interactive*/);
    }

    KnobSeparatorPtr exportDataSepKnob = AppManager::createKnob<KnobSeparator>(effect.get(), tr(kTrackerParamExportDataSeparatorLabel), 1, false);
    exportDataSepKnob->setName(kTrackerParamExportDataSeparator);
    transformPage->addKnob(exportDataSepKnob);
    exportDataSep = exportDataSepKnob;

    KnobBoolPtr exporLinkKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamExportLinkLabel), 1, false);
    exporLinkKnob->setName(kTrackerParamExportLink);
    exporLinkKnob->setHintToolTip( tr(kTrackerParamExportLinkHint) );
    exporLinkKnob->setAnimationEnabled(false);
    exporLinkKnob->setAddNewLine(false);
    exporLinkKnob->setDefaultValue(true);
    transformPage->addKnob(exporLinkKnob);
    exportLink = exporLinkKnob;

    /* KnobBoolPtr exporUseCurFrameKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kTrackerParamExportUseCurrentFrameLabel), 1, false);
       exporUseCurFrameKnob->setName(kTrackerParamExportUseCurrentFrame);
       exporUseCurFrameKnob->setHintToolTip(tr(kTrackerParamExportUseCurrentFrameHint));
       exporUseCurFrameKnob->setAnimationEnabled(false);
       exporUseCurFrameKnob->setAddNewLine(false);
       exporUseCurFrameKnob->setDefaultValue(false);
       transformPage->addKnob(exporUseCurFrameKnob);
       exportUseCurrentFrame = exporUseCurFrameKnob;
       knobs.push_back(exporUseCurFrameKnob);*/

    KnobButtonPtr exportButtonKnob = AppManager::createKnob<KnobButton>(effect.get(), tr(kTrackerParamExportButtonLabel), 1);
    exportButtonKnob->setName(kTrackerParamExportButton);
    exportButtonKnob->setHintToolTip( tr(kTrackerParamExportButtonHint) );
    exportButtonKnob->setAllDimensionsEnabled(false);
    transformPage->addKnob(exportButtonKnob);
    exportButton = exportButtonKnob;
}

/**
 * @brief Set keyframes on knobs from Marker data
 **/
void
TrackerContextPrivate::setKnobKeyframesFromMarker(const mv::Marker& mvMarker,
                                                  int /*formatHeight*/,
                                                  const libmv::TrackRegionResult* result,
                                                  const TrackMarkerPtr& natronMarker)
{
    int time = mvMarker.frame;
    KnobDoublePtr errorKnob = natronMarker->getErrorKnob();

    if (result) {
        double corr = result->correlation;
        if (corr != corr) {
            corr = 1.;
        }
        errorKnob->setValueAtTime(time, 1. - corr, ViewSpec::current(), 0);
    } else {
        errorKnob->setValueAtTime(time, 0., ViewSpec::current(), 0);
    }

    // Blender also adds 0.5 to coordinates
    Point center;
    center.x = (double)mvMarker.center(0);
    center.y = (double)mvMarker.center(1);

    KnobDoublePtr offsetKnob = natronMarker->getOffsetKnob();
    Point offset;
    offset.x = offsetKnob->getValueAtTime(time, 0);
    offset.y = offsetKnob->getValueAtTime(time, 1);

    Point centerPlusOffset;
    centerPlusOffset.x = center.x + offset.x;
    centerPlusOffset.y = center.y + offset.y;

    // Set the center
    KnobDoublePtr centerKnob = natronMarker->getCenterKnob();
    centerKnob->setValuesAtTime(time, center.x + 0.5, center.y + 0.5, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);

    Point topLeftCorner, topRightCorner, btmRightCorner, btmLeftCorner;
    topLeftCorner.x = mvMarker.patch.coordinates(3, 0) - centerPlusOffset.x;
    topLeftCorner.y = mvMarker.patch.coordinates(3, 1) - centerPlusOffset.y;

    topRightCorner.x = mvMarker.patch.coordinates(2, 0) - centerPlusOffset.x;
    topRightCorner.y = mvMarker.patch.coordinates(2, 1) - centerPlusOffset.y;

    btmRightCorner.x = mvMarker.patch.coordinates(1, 0) - centerPlusOffset.x;
    btmRightCorner.y = mvMarker.patch.coordinates(1, 1) - centerPlusOffset.y;

    btmLeftCorner.x = mvMarker.patch.coordinates(0, 0) - centerPlusOffset.x;
    btmLeftCorner.y = mvMarker.patch.coordinates(0, 1) - centerPlusOffset.y;

    KnobDoublePtr pntTopLeftKnob = natronMarker->getPatternTopLeftKnob();
    KnobDoublePtr pntTopRightKnob = natronMarker->getPatternTopRightKnob();
    KnobDoublePtr pntBtmLeftKnob = natronMarker->getPatternBtmLeftKnob();
    KnobDoublePtr pntBtmRightKnob = natronMarker->getPatternBtmRightKnob();

    // Set the pattern Quad
    pntTopLeftKnob->setValuesAtTime(time, topLeftCorner.x, topLeftCorner.y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    pntTopRightKnob->setValuesAtTime(time, topRightCorner.x, topRightCorner.y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    pntBtmLeftKnob->setValuesAtTime(time, btmLeftCorner.x, btmLeftCorner.y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    pntBtmRightKnob->setValuesAtTime(time, btmRightCorner.x, btmRightCorner.y, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
} // TrackerContextPrivate::setKnobKeyframesFromMarker

/// Converts a Natron track marker to the one used in LibMV. This is expensive: many calls to getValue are made
void
TrackerContextPrivate::natronTrackerToLibMVTracker(bool isReferenceMarker,
                                                   bool trackChannels[3],
                                                   const TrackMarker& marker,
                                                   int trackIndex,
                                                   int trackedTime,
                                                   int frameStep,
                                                   double /*formatHeight*/,
                                                   mv::Marker* mvMarker)
{
    KnobDoublePtr searchWindowBtmLeftKnob = marker.getSearchWindowBottomLeftKnob();
    KnobDoublePtr searchWindowTopRightKnob = marker.getSearchWindowTopRightKnob();
    KnobDoublePtr patternTopLeftKnob = marker.getPatternTopLeftKnob();
    KnobDoublePtr patternTopRightKnob = marker.getPatternTopRightKnob();
    KnobDoublePtr patternBtmRightKnob = marker.getPatternBtmRightKnob();
    KnobDoublePtr patternBtmLeftKnob = marker.getPatternBtmLeftKnob();

#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    KnobDoublePtr weightKnob = marker.getWeightKnob();
#endif
    KnobDoublePtr centerKnob = marker.getCenterKnob();
    KnobDoublePtr offsetKnob = marker.getOffsetKnob();

    // We don't use the clip in Natron
    mvMarker->clip = 0;
    mvMarker->reference_clip = 0;

#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    mvMarker->weight = (float)weightKnob->getValue();
#else
    mvMarker->weight = 1.;
#endif

    mvMarker->frame = trackedTime;
    const int referenceTime = marker.getReferenceFrame(trackedTime, frameStep);
    mvMarker->reference_frame = referenceTime;
    if ( marker.isUserKeyframe(trackedTime) ) {
        mvMarker->source = mv::Marker::MANUAL;
    } else {
        mvMarker->source = mv::Marker::TRACKED;
    }

    // This doesn't seem to be used at all by libmv TrackRegion
    mvMarker->model_type = mv::Marker::POINT;
    mvMarker->model_id = 0;
    mvMarker->track = trackIndex;

    mvMarker->disabled_channels =
        (trackChannels[0] ? LIBMV_MARKER_CHANNEL_R : 0) |
        (trackChannels[1] ? LIBMV_MARKER_CHANNEL_G : 0) |
        (trackChannels[2] ? LIBMV_MARKER_CHANNEL_B : 0);


    //int searchWinTime = isReferenceMarker ? trackedTime : mvMarker->reference_frame;

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


    Point tl, tr, br, bl;
    tl.x = patternTopLeftKnob->getValueAtTime(trackedTime, 0);
    tl.y = patternTopLeftKnob->getValueAtTime(trackedTime, 1);

    tr.x = patternTopRightKnob->getValueAtTime(trackedTime, 0);
    tr.y = patternTopRightKnob->getValueAtTime(trackedTime, 1);

    br.x = patternBtmRightKnob->getValueAtTime(trackedTime, 0);
    br.y = patternBtmRightKnob->getValueAtTime(trackedTime, 1);

    bl.x = patternBtmLeftKnob->getValueAtTime(trackedTime, 0);
    bl.y = patternBtmLeftKnob->getValueAtTime(trackedTime, 1);

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

    int searchWinTime = isReferenceMarker ? trackedTime : referenceTime;
    Point searchWndBtmLeft, searchWndTopRight;
    searchWndBtmLeft.x = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, 0);
    searchWndBtmLeft.y = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, 1);

    searchWndTopRight.x = searchWindowTopRightKnob->getValueAtTime(searchWinTime, 0);
    searchWndTopRight.y = searchWindowTopRightKnob->getValueAtTime(searchWinTime, 1);

    /*
       Center and offset are always sampled at the reference time
     */
    Point centerAtTrackedTime;
    centerAtTrackedTime.x = centerKnob->getValueAtTime(trackedTime, 0);
    centerAtTrackedTime.y = centerKnob->getValueAtTime(trackedTime, 1);

    Point offsetAtTrackedTime;
    offsetAtTrackedTime.x = offsetKnob->getValueAtTime(trackedTime, 0);
    offsetAtTrackedTime.y = offsetKnob->getValueAtTime(trackedTime, 1);

    // Blender also substracts 0.5 to coordinates
    centerAtTrackedTime.x -= 0.5;
    centerAtTrackedTime.y -= 0.5;
    mvMarker->center(0) = centerAtTrackedTime.x;
    mvMarker->center(1) = centerAtTrackedTime.y;

    Point centerPlusOffset;
    centerPlusOffset.x = centerAtTrackedTime.x + offsetAtTrackedTime.x;
    centerPlusOffset.y = centerAtTrackedTime.y + offsetAtTrackedTime.y;

    searchWndBtmLeft.x += centerPlusOffset.x;
    searchWndBtmLeft.y += centerPlusOffset.y;

    searchWndTopRight.x += centerPlusOffset.x;
    searchWndTopRight.y += centerPlusOffset.y;

    tl.x += centerPlusOffset.x;
    tl.y += centerPlusOffset.y;

    tr.x += centerPlusOffset.x;
    tr.y += centerPlusOffset.y;

    br.x += centerPlusOffset.x;
    br.y += centerPlusOffset.y;

    bl.x += centerPlusOffset.x;
    bl.y += centerPlusOffset.y;

    mvMarker->search_region.min(0) = searchWndBtmLeft.x;
    mvMarker->search_region.min(1) = searchWndBtmLeft.y;
    mvMarker->search_region.max(0) = searchWndTopRight.x;
    mvMarker->search_region.max(1) = searchWndTopRight.y;


    mvMarker->patch.coordinates(0, 0) = bl.x;
    mvMarker->patch.coordinates(0, 1) = bl.y;

    mvMarker->patch.coordinates(1, 0) = br.x;
    mvMarker->patch.coordinates(1, 1) = br.y;

    mvMarker->patch.coordinates(2, 0) = tr.x;
    mvMarker->patch.coordinates(2, 1) = tr.y;

    mvMarker->patch.coordinates(3, 0) = tl.x;
    mvMarker->patch.coordinates(3, 1) = tl.y;
} // TrackerContextPrivate::natronTrackerToLibMVTracker

void
TrackerContextPrivate::beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const
{
    options->minimum_correlation = 1. - maxError.lock()->getValue();
    assert(options->minimum_correlation >= 0. && options->minimum_correlation <= 1.);
    options->max_iterations = maxIterations.lock()->getValue();
    options->use_brute_initialization = bruteForcePreTrack.lock()->getValue();
    options->use_normalized_intensities = useNormalizedIntensities.lock()->getValue();
    options->sigma = preBlurSigma.lock()->getValue();
    options->use_esm = false;
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
 * @brief This is the internal tracking function that makes use of TrackerPM to do 1 track step
 * @param trackingIndex This is the index of the Marker we should track in the args
 * @param args Multiple arguments global to the whole track, not just this step
 * @param trackTime The search frame time, that is, the frame to track
 */
bool
TrackerContextPrivate::trackStepTrackerPM(TrackMarkerPM* track,
                                          const TrackArgs& args,
                                          int trackTime)
{
    int frameStep = args.getStep();
    int refTime = track->getReferenceFrame(trackTime, frameStep);

    return track->trackMarker(frameStep > 0, refTime, trackTime);
}

/*
 * @brief This is the internal tracking function that makes use of LivMV to do 1 track step
 * @param trackingIndex This is the index of the Marker we should track in the args
 * @param args Multiple arguments global to the whole track, not just this step
 * @param trackTime The search frame time, that is, the frame to track
 */
bool
TrackerContextPrivate::trackStepLibMV(int trackIndex,
                                      const TrackArgs& args,
                                      int trackTime)
{
    assert( trackIndex >= 0 && trackIndex < args.getNumTracks() );

#if defined(CERES_USE_OPENMP) && defined(_OPENMP)
    // Set the number of threads Ceres may use
    QThreadPool* tp = QThreadPool::globalInstance();
    omp_set_num_threads(tp->maxThreadCount() - tp->activeThreadCount() - 1);
#endif

    const std::vector<TrackMarkerAndOptionsPtr>& tracks = args.getTracks();
    const TrackMarkerAndOptionsPtr& track = tracks[trackIndex];
    mv::AutoTrackPtr autoTrack = args.getLibMVAutoTrack();
    QMutex* autoTrackMutex = args.getAutoTrackMutex();
    bool enabledChans[3];
    args.getEnabledChannels(&enabledChans[0], &enabledChans[1], &enabledChans[2]);


    {
        // Add a marker to the auto-track at the tracked time: the mv::Marker struct is filled with the values of the Natron TrackMarker at the trackTime
        QMutexLocker k(autoTrackMutex);
        if ( trackTime == args.getStart() ) {
            bool foundStartMarker = autoTrack->GetMarker(0, trackTime, trackIndex, &track->mvMarker);
            assert(foundStartMarker);
            Q_UNUSED(foundStartMarker);
            track->mvMarker.source = mv::Marker::MANUAL;
        } else {
            natronTrackerToLibMVTracker(false, enabledChans, *track->natronMarker, trackIndex, trackTime, args.getStep(), args.getFormatHeight(), &track->mvMarker);
            autoTrack->AddMarker(track->mvMarker);
        }
    }

    if (track->mvMarker.source == mv::Marker::MANUAL) {
        // This is a user keyframe or the first frame, we do not track it
        assert( trackTime == args.getStart() || track->natronMarker->isUserKeyframe(track->mvMarker.frame) );
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "TrackStep:" << trackTime << "is a keyframe";
#endif
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), 0, track->natronMarker);
    } else {
        // Make sure the reference frame is in the auto-track: the mv::Marker struct is filled with the values of the Natron TrackMarker at the reference_frame
        {
            QMutexLocker k(autoTrackMutex);
            mv::Marker m;
            if ( !autoTrack->GetMarker(0, track->mvMarker.reference_frame, trackIndex, &m) ) {
                natronTrackerToLibMVTracker(true, enabledChans, *track->natronMarker, track->mvMarker.track, track->mvMarker.reference_frame, args.getStep(), args.getFormatHeight(), &m);
                autoTrack->AddMarker(m);
            }
        }


        assert(track->mvMarker.reference_frame != track->mvMarker.frame);


#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << ">>>> Tracking marker" << trackIndex << "at frame" << trackTime <<
            "with reference frame" << track->mvMarker.reference_frame;
#endif

        // Do the actual tracking
        libmv::TrackRegionResult result;
        if ( !autoTrack->TrackMarker(&track->mvMarker, &result,  &track->mvState, &track->mvOptions) || !result.is_usable() ) {
#ifdef TRACE_LIB_MV
            qDebug() << QThread::currentThread() << "Tracking FAILED (" << (int)result.termination <<  ") for track" << trackIndex << "at frame" << trackTime;
#endif

            // Todo: report error to user
            return false;
        }

        //Ok the tracking has succeeded, now the marker is in this state:
        //the source is TRACKED, the search_window has been offset by the center delta,  the center has been offset

#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "Tracking SUCCESS for track" << trackIndex << "at frame" << trackTime;
#endif

        //Extract the marker to the knob keyframes
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), &result, track->natronMarker);

        //Add the marker to the autotrack
        /*{
           QMutexLocker k(autoTrackMutex);
           autoTrack->AddMarker(track->mvMarker);
           }*/
    } // if (track->mvMarker.source == mv::Marker::MANUAL) {


    return true;
} // TrackerContextPrivate::trackStepLibMV

struct PreviouslyComputedTrackFrame
{
    int frame;
    bool isUserKey;

    PreviouslyComputedTrackFrame()
        : frame(0), isUserKey(false) {}

    PreviouslyComputedTrackFrame(int f,
                                 bool b)
        : frame(f), isUserKey(b) {}
};

struct PreviouslyComputedTrackFrameCompareLess
{
    bool operator() (const PreviouslyComputedTrackFrame& lhs,
                     const PreviouslyComputedTrackFrame& rhs) const
    {
        return lhs.frame < rhs.frame;
    }
};

typedef std::set<PreviouslyComputedTrackFrame, PreviouslyComputedTrackFrameCompareLess> PreviouslyTrackedFrameSet;

void
TrackerContext::trackMarkers(const std::list<TrackMarkerPtr>& markers,
                             int start,
                             int end,
                             int frameStep,
                             OverlaySupport* overlayInteract)
{
    if ( markers.empty() ) {
        Q_EMIT trackingFinished();
        return;
    }

    if (!getNode()->getInput(0)) {
        Q_EMIT trackingFinished();
        return;
    }

    ViewerInstance* viewer = 0;
    if (overlayInteract) {
        viewer = overlayInteract->getInternalViewerNode();
    }


    /// The channels we are going to use for tracking
    bool enabledChannels[3];
    enabledChannels[0] = _imp->enableTrackRed.lock()->getValue();
    enabledChannels[1] = _imp->enableTrackGreen.lock()->getValue();
    enabledChannels[2] = _imp->enableTrackBlue.lock()->getValue();

    double formatWidth, formatHeight;
    Format f;
    getNode()->getApp()->getProject()->getProjectDefaultFormat(&f);
    formatWidth = f.width();
    formatHeight = f.height();

    bool autoKeyingOnEnabledParamEnabled = _imp->autoKeyEnabled.lock()->getValue();
    
    /// The accessor and its cache is local to a track operation, it is wiped once the whole sequence track is finished.
    TrackerFrameAccessorPtr accessor( new TrackerFrameAccessor(this, enabledChannels, formatHeight) );
    mv::AutoTrackPtr trackContext( new mv::AutoTrack( accessor.get() ) );
    std::vector<TrackMarkerAndOptionsPtr> trackAndOptions;
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
    for (std::list<TrackMarkerPtr>::const_iterator it = markers.begin(); it != markers.end(); ++it, ++trackIndex) {
        
        if (autoKeyingOnEnabledParamEnabled) {
            (*it)->setEnabledAtTime(start, true);
        }
        
        TrackMarkerAndOptionsPtr t = boost::make_shared<TrackMarkerAndOptions>();
        t->natronMarker = *it;

        // Set a keyframe on the marker to initialize its position
        (*it)->setKeyFrameOnCenterAndPatternAtTime(start);

        std::set<int> userKeys;
        t->natronMarker->getUserKeyframes(&userKeys);

        if ( userKeys.empty() ) {
            // Set a user keyframe on tracking start if the marker does not have any user keys
            t->natronMarker->setUserKeyframe(start);
        }

        PreviouslyTrackedFrameSet previousFramesOrdered;

        // Make sure to create a marker at the start time
        userKeys.insert(start);


        // Add a libmv marker for all keyframes
        for (std::set<int>::iterator it2 = userKeys.begin(); it2 != userKeys.end(); ++it2) {

            // Add the marker to the markers ordered only if it can contribute to predicting its next position
            if ( ( (frameStep > 0) && (*it2 <= start) ) || ( (frameStep < 0) && (*it2 >= start) ) ) {
                previousFramesOrdered.insert( PreviouslyComputedTrackFrame(*it2, true) );
            }
        }


        //For all already tracked frames which are not keyframes, add them to the AutoTrack too
        std::set<double> centerKeys;
        t->natronMarker->getCenterKeyframes(&centerKeys);

        for (std::set<double>::iterator it2 = centerKeys.begin(); it2 != centerKeys.end(); ++it2) {
            if ( userKeys.find(*it2) != userKeys.end() ) {
                continue;
            }

            // Add the marker to the markers ordered only if it can contribute to predicting its next position
            if ( ( ( (frameStep > 0) && (*it2 < start) ) || ( (frameStep < 0) && (*it2 > start) ) ) ) {
                previousFramesOrdered.insert( PreviouslyComputedTrackFrame(*it2, false) );
            }
        }


        // Taken from libmv, only initialize the filter to this amount of frames (max)
        const int max_frames_to_predict_from = 20;
        std::list<mv::Marker> previouslyComputedMarkersOrdered;

        // Find the first keyframe that's not considered to go before start or end
        PreviouslyTrackedFrameSet::iterator prevFramesIt = previousFramesOrdered.lower_bound(PreviouslyComputedTrackFrame(start, false));
        if (frameStep < 0) {
            if (prevFramesIt != previousFramesOrdered.end()) {
                while (prevFramesIt != previousFramesOrdered.end() && (int)previouslyComputedMarkersOrdered.size() != max_frames_to_predict_from) {

                    mv::Marker mvMarker;

                    TrackerContextPrivate::natronTrackerToLibMVTracker(true, enabledChannels, *t->natronMarker, trackIndex, prevFramesIt->frame, frameStep, formatHeight, &mvMarker);
                    trackContext->AddMarker(mvMarker);

                    // insert in the front of the list so that the order is reversed
                    previouslyComputedMarkersOrdered.push_front(mvMarker);
                    ++prevFramesIt;
                }
            }
            // previouslyComputedMarkersOrdered is now ordererd by decreasing order
        } else {

            if (prevFramesIt != previousFramesOrdered.end()) {
                while (prevFramesIt != previousFramesOrdered.begin() && (int)previouslyComputedMarkersOrdered.size() != max_frames_to_predict_from) {

                    mv::Marker mvMarker;

                    TrackerContextPrivate::natronTrackerToLibMVTracker(true, enabledChannels, *t->natronMarker, trackIndex, prevFramesIt->frame, frameStep, formatHeight, &mvMarker);
                    trackContext->AddMarker(mvMarker);

                    // insert in the front of the list so that the order is reversed
                    previouslyComputedMarkersOrdered.push_front(mvMarker);
                    --prevFramesIt;
                }
                if (prevFramesIt == previousFramesOrdered.begin() && (int)previouslyComputedMarkersOrdered.size() != max_frames_to_predict_from) {
                    mv::Marker mvMarker;

                    TrackerContextPrivate::natronTrackerToLibMVTracker(true, enabledChannels, *t->natronMarker, trackIndex, prevFramesIt->frame, frameStep, formatHeight, &mvMarker);
                    trackContext->AddMarker(mvMarker);

                    // insert in the front of the list so that the order is reversed
                    previouslyComputedMarkersOrdered.push_front(mvMarker);

                }
            }
            // previouslyComputedMarkersOrdered is now ordererd by increasing order
        }


        // There must be at least 1 marker at the start time
        assert( !previouslyComputedMarkersOrdered.empty() );

        // Initialise the kalman state with the marker at the position

        std::list<mv::Marker>::iterator mIt = previouslyComputedMarkersOrdered.begin();
        t->mvState.Init(*mIt, frameStep);
        ++mIt;
        for (; mIt != previouslyComputedMarkersOrdered.end(); ++mIt) {
            mv::Marker predictedMarker;
            if ( !t->mvState.PredictForward(mIt->frame, &predictedMarker) ) {
                break;
            } else {
                t->mvState.Update(*mIt);
            }
        }


        t->mvOptions = mvOptions;
        _imp->endLibMVOptionsForTrack(*t->natronMarker, &t->mvOptions);
        trackAndOptions.push_back(t);
    }

    
    /*
       Launch tracking in the scheduler thread.
     */
    TrackArgsPtr args( new TrackArgs(start, end, frameStep, getNode()->getApp()->getTimeLine(), viewer, trackContext, accessor, trackAndOptions, formatWidth, formatHeight, autoKeyingOnEnabledParamEnabled) );
    _imp->scheduler.track(args);
} // TrackerContext::trackMarkers

void
TrackerContextPrivate::linkMarkerKnobsToGuiKnobs(const std::list<TrackMarkerPtr>& markers,
                                                 bool multipleTrackSelected,
                                                 bool slave)
{
    std::list<TrackMarkerPtr>::const_iterator next = markers.begin();

    if ( !markers.empty() ) {
        ++next;
    }
    for (std::list<TrackMarkerPtr>::const_iterator it = markers.begin(); it != markers.end(); ++it) {
        const KnobsVec& trackKnobs = (*it)->getKnobs();
        for (KnobsVec::const_iterator it2 = trackKnobs.begin(); it2 != trackKnobs.end(); ++it2) {
            // Find the knob in the TrackerContext knobs
            KnobIPtr found;
            for (std::list<KnobIWPtr>::iterator it3 = perTrackKnobs.begin(); it3 != perTrackKnobs.end(); ++it3) {
                KnobIPtr k = it3->lock();
                if ( k->getName() == (*it2)->getName() ) {
                    found = k;
                    break;
                }
            }

            if (!found) {
                continue;
            }

            //Clone current state only for the last marker
            found->blockListenersNotification();
            found->cloneAndUpdateGui( it2->get() );
            found->unblockListenersNotification();

            //Slave internal knobs
            assert( (*it2)->getDimension() == found->getDimension() );
            for (int i = 0; i < (*it2)->getDimension(); ++i) {
                if (slave) {
                    (*it2)->slaveTo(i, found, i);
                } else {
                    (*it2)->unSlave(i, !multipleTrackSelected);
                }
            }

            /*if (!slave) {
                QObject::disconnect( (*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,ViewSpec,int,int,bool)),
                                     _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::disconnect( (*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)),
                                     _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::disconnect( (*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(ViewSpec,int,double,double)),
                                     _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::disconnect( (*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(ViewSpec,int)),
                                     _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::disconnect( (*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,ViewSpec,int)),
                                     _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::disconnect( (*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,ViewSpec,int)),
                                     _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
               } else {
                QObject::connect( (*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,ViewSpec,int,int,bool)),
                                  _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::connect( (*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)),
                                  _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::connect( (*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(ViewSpec,int,double,double)),
                                  _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::connect( (*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(ViewSpec,int)),
                                  _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::connect( (*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,ViewSpec,int)),
                                  _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
                QObject::connect( (*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,ViewSpec,int)),
                                  _publicInterface, SLOT(onSelectedKnobCurveChanged()) );
               }*/
        }
        if ( next != markers.end() ) {
            ++next;
        }
    } // for (std::list<TrackMarkerPtr>::const_iterator it = markers() ; it!=markers(); ++it)
} // TrackerContextPrivate::linkMarkerKnobsToGuiKnobs

void
TrackerContextPrivate::refreshVisibilityFromTransformTypeInternal(TrackerTransformNodeEnum transformType)
{
    if ( !transformNode.lock() ) {
        return;
    }

    KnobChoicePtr motionTypeKnob = motionType.lock();
    if (!motionTypeKnob) {
        return;
    }
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum motionType = (TrackerMotionTypeEnum)motionType_i;
    KnobBoolPtr disableTransformKnob = disableTransform.lock();
    bool disableNodes = disableTransformKnob->getValue();

    transformNode.lock()->setNodeDisabled(disableNodes || transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    cornerPinNode.lock()->setNodeDisabled(disableNodes || transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);

    transformControlsSeparator.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    disableTransformKnob->setSecret(motionType == eTrackerMotionTypeNone);
    if (transformType == eTrackerTransformNodeTransform) {
        transformControlsSeparator.lock()->setLabel( tr("Transform Controls") );
        disableTransformKnob->setLabel( tr("Disable Transform") );
    } else if (transformType == eTrackerTransformNodeCornerPin) {
        transformControlsSeparator.lock()->setLabel( tr("CornerPin Controls") );
        disableTransformKnob->setLabel( tr("Disable CornerPin") );
    }


    smoothTransform.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    smoothCornerPin.lock()->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);

    toGroup.lock()->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);
    fromGroup.lock()->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);
    cornerPinOverlayPoints.lock()->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);
    KnobDoublePtr matrix = cornerPinMatrix.lock();
    if (matrix) {
        matrix->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);
    }


    translate.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    scale.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    scaleUniform.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    rotate.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    center.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    skewX.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    skewY.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    skewOrder.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    filter.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    clamp.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    blackOutside.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);

    invertTransform.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    motionBlur.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    shutter.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    shutterOffset.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    customShutterOffset.lock()->setSecret(motionType == eTrackerMotionTypeNone);

    exportLink.lock()->setAllDimensionsEnabled(motionType != eTrackerMotionTypeNone);
    exportButton.lock()->setAllDimensionsEnabled(motionType != eTrackerMotionTypeNone);

#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    bool usePM = usePatternMatching.lock()->getValue();
    enableTrackRed.lock()->setSecret(usePM);
    enableTrackGreen.lock()->setSecret(usePM);
    enableTrackBlue.lock()->setSecret(usePM);
    maxError.lock()->setSecret(usePM);
    maxIterations.lock()->setSecret(usePM);
    bruteForcePreTrack.lock()->setSecret(usePM);
    useNormalizedIntensities.lock()->setSecret(usePM);
    preBlurSigma.lock()->setSecret(usePM);

    patternMatchingScore.lock()->setSecret(!usePM);

#endif
} // TrackerContextPrivate::refreshVisibilityFromTransformTypeInternal

void
TrackerContextPrivate::refreshVisibilityFromTransformType()
{
    KnobChoicePtr transformTypeKnob = transformType.lock();

    assert(transformTypeKnob);
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    refreshVisibilityFromTransformTypeInternal(transformType);
}

RectD
TrackerContextPrivate::getInputRoDAtTime(double time) const
{
    NodePtr thisNode = node.lock();
    NodePtr input = thisNode->getInput(0);
    bool useProjFormat = false;
    RectD ret;

    if (!input) {
        useProjFormat = true;
    } else {
        StatusEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(input->getHashValue(), time, RenderScale(1.), ViewIdx(0), &ret, 0);
        if (stat == eStatusFailed) {
            useProjFormat = true;
        } else {
            return ret;
        }
    }
    if (useProjFormat) {
        Format f;
        thisNode->getApp()->getProject()->getProjectDefaultFormat(&f);
        ret.x1 = f.x1;
        ret.x2 = f.x2;
        ret.y1 = f.y1;
        ret.y2 = f.y2;
    }

    return ret;
}

static Transform::Point3D
euclideanToHomogenous(const Point& p)
{
    Transform::Point3D r;

    r.x = p.x;
    r.y = p.y;
    r.z = 1;

    return r;
}

static Point
applyHomography(const Point& p,
                const Transform::Matrix3x3& h)
{
    Transform::Point3D a = euclideanToHomogenous(p);
    Transform::Point3D r = Transform::matApply(h, a);
    Point ret;

    ret.x = r.x / r.z;
    ret.y = r.y / r.z;

    return ret;
}

using namespace openMVG::robust;
static void
throwProsacError(ProsacReturnCodeEnum c,
                 int nMinSamples)
{
    switch (c) {
    case openMVG::robust::eProsacReturnCodeFoundModel:
        break;
    case openMVG::robust::eProsacReturnCodeInliersIsMinSamples:
        break;
    case openMVG::robust::eProsacReturnCodeNoModelFound:
        throw std::runtime_error("Could not find a model for the given correspondences.");
        break;
    case openMVG::robust::eProsacReturnCodeNotEnoughPoints: {
        std::stringstream ss;
        ss << "This model requires a minimum of ";
        ss << nMinSamples;
        ss << " correspondences.";
        throw std::runtime_error( ss.str() );
        break;
    }
    case openMVG::robust::eProsacReturnCodeMaxIterationsFromProportionParamReached:
        throw std::runtime_error("Maximum iterations computed from outliers proportion reached");
        break;
    case openMVG::robust::eProsacReturnCodeMaxIterationsParamReached:
        throw std::runtime_error("Maximum solver iterations reached");
        break;
    }
}

/*
 * @brief Search for the best model that fits the correspondences x1 to x2.
 * @param dataSetIsManual If true, this indicates that the x1 points were placed manually by the user
 * in which case we expect the vector to have less than 10% outliers
 * @param robustModel When dataSetIsManual is true, if this parameter is true then the solver will run a MEsimator on the data
 * assuming the model searched is the correct model. Otherwise if false, only a least-square pass is done to compute a model that fits
 * all correspondences (but which may be incorrect)
 */
template <typename MODELTYPE>
void
searchForModel(const bool dataSetIsManual,
               const bool robustModel,
               const std::vector<Point>& x1,
               const std::vector<Point>& x2,
               int w1,
               int h1,
               int w2,
               int h2,
               typename MODELTYPE::Model* foundModel,
               double *RMS = 0
#ifdef DEBUG
               ,
               std::vector<bool>* inliers = 0
#endif
               )
{
    typedef ProsacKernelAdaptor<MODELTYPE> KernelType;

    assert( x1.size() == x2.size() );
    openMVG::Mat M1( 2, x1.size() ), M2( 2, x2.size() );
    for (std::size_t i = 0; i < x1.size(); ++i) {
        M1(0, i) = x1[i].x;
        M1(1, i) = x1[i].y;

        M2(0, i) = x2[i].x;
        M2(1, i) = x2[i].y;
    }

    KernelType kernel(M1, w1, h1, M2, w2, h2);

    if (dataSetIsManual) {
        if (robustModel) {
            double sigmaMAD;
            if ( !searchModelWithMEstimator(kernel, 3, foundModel, RMS, &sigmaMAD) ) {
                throw std::runtime_error("MEstimator failed to run a successful iteration");
            }
        } else {
            if ( !searchModelLS(kernel, foundModel, RMS) ) {
                throw std::runtime_error("Least-squares solver failed to find a model");
            }
        }
    } else {
        ProsacReturnCodeEnum ret = prosac(kernel, foundModel
#ifdef DEBUG
                                          , inliers
#else
                                          , 0
#endif
                                          , RMS);
        throwProsacError( ret, KernelType::MinimumSamples() );
    }
}

void
TrackerContextPrivate::computeTranslationFromNPoints(const bool dataSetIsManual,
                                                     const bool robustModel,
                                                     const std::vector<Point>& x1,
                                                     const std::vector<Point>& x2,
                                                     int w1,
                                                     int h1,
                                                     int w2,
                                                     int h2,
                                                     Point* translation,
                                                     double *RMS)
{
    openMVG::Vec2 model = openMVG::Vec2::Zero();

    searchForModel<openMVG::robust::Translation2DSolver>(dataSetIsManual, robustModel, x1, x2, w1, h1, w2, h2, &model, RMS);
    translation->x = model(0);
    translation->y = model(1);
}

void
TrackerContextPrivate::computeSimilarityFromNPoints(const bool dataSetIsManual,
                                                    const bool robustModel,
                                                    const std::vector<Point>& x1,
                                                    const std::vector<Point>& x2,
                                                    int w1,
                                                    int h1,
                                                    int w2,
                                                    int h2,
                                                    Point* translation,
                                                    double* rotate,
                                                    double* scale,
                                                    double *RMS)
{
    openMVG::Vec4 model = openMVG::Vec4::Zero();

    searchForModel<openMVG::robust::Similarity2DSolver>(dataSetIsManual, robustModel, x1, x2, w1, h1, w2, h2, &model, RMS);
    openMVG::robust::Similarity2DSolver::rtsFromVec4(model, &translation->x, &translation->y, scale, rotate);
    *rotate = Transform::toDegrees(*rotate);
}

void
TrackerContextPrivate::computeHomographyFromNPoints(const bool dataSetIsManual,
                                                    const bool robustModel,
                                                    const std::vector<Point>& x1,
                                                    const std::vector<Point>& x2,
                                                    int w1,
                                                    int h1,
                                                    int w2,
                                                    int h2,
                                                    Transform::Matrix3x3* homog,
                                                    double *RMS)
{
    openMVG::Mat3 model = openMVG::Mat3::Zero();

#ifdef DEBUG
    std::vector<bool> inliers;
#endif

    searchForModel<openMVG::robust::Homography2DSolver>(dataSetIsManual, robustModel, x1, x2, w1, h1, w2, h2, &model, RMS
#ifdef DEBUG
                                                        , &inliers
#endif
                                                        );

    *homog = Transform::Matrix3x3( model(0, 0), model(0, 1), model(0, 2),
                                   model(1, 0), model(1, 1), model(1, 2),
                                   model(2, 0), model(2, 1), model(2, 2) );

#ifdef DEBUG
    // Check that the warped x1 points match x2
    assert( x1.size() == x2.size() );
    for (std::size_t i = 0; i < x1.size(); ++i) {
        if (!dataSetIsManual && inliers[i]) {
            Point p2 = applyHomography(x1[i], *homog);
            if ( (std::abs(p2.x - x2[i].x) >  0.02) ||
                 ( std::abs(p2.y - x2[i].y) > 0.02) ) {
                qDebug() << "[BUG]: Inlier for Homography2DSolver failed to fit the found model: X1 (" << x1[i].x << ',' << x1[i].y << ')' << "X2 (" << x2[i].x << ',' << x2[i].y << ')'  << "P2 (" << p2.x << ',' << p2.y << ')';
            }
        }
    }
#endif
}

void
TrackerContextPrivate::computeFundamentalFromNPoints(const bool dataSetIsManual,
                                                     const bool robustModel,
                                                     const std::vector<Point>& x1,
                                                     const std::vector<Point>& x2,
                                                     int w1,
                                                     int h1,
                                                     int w2,
                                                     int h2,
                                                     Transform::Matrix3x3* fundamental,
                                                     double *RMS)
{
    openMVG::Mat3 model = openMVG::Mat3::Zero();

    searchForModel<openMVG::robust::FundamentalSolver>(dataSetIsManual, robustModel, x1, x2, w1, h1, w2, h2, &model, RMS);

    *fundamental = Transform::Matrix3x3( model(0, 0), model(0, 1), model(0, 2),
                                         model(1, 0), model(1, 1), model(1, 2),
                                         model(2, 0), model(2, 1), model(2, 2) );
}

struct PointWithError
{
    Point p1, p2;
    double error;
};

static bool
PointWithErrorCompareLess(const PointWithError& lhs,
                          const PointWithError& rhs)
{
    return lhs.error < rhs.error;
}

void
TrackerContextPrivate::extractSortedPointsFromMarkers(double refTime,
                                                      double time,
                                                      const std::vector<TrackMarkerPtr>& markers,
                                                      int jitterPeriod,
                                                      bool jitterAdd,
                                                      std::vector<Point>* x1,
                                                      std::vector<Point>* x2)
{
    assert( !markers.empty() );

    std::vector<PointWithError> pointsWithErrors;
    bool useJitter = (jitterPeriod > 1);
    int halfJitter = std::max(0, jitterPeriod / 2);
    // Prosac expects the points to be sorted by decreasing correlation score (increasing error)
    int pIndex = 0;
    for (std::size_t i = 0; i < markers.size(); ++i) {
        KnobDoublePtr centerKnob = markers[i]->getCenterKnob();
        KnobDoublePtr errorKnob = markers[i]->getErrorKnob();

        if (centerKnob->getKeyFrameIndex(ViewSpec::current(), 0, time) < 0) {
            continue;
        }
        pointsWithErrors.resize(pointsWithErrors.size() + 1);

        PointWithError& perr = pointsWithErrors[pIndex];

        if (!useJitter) {
            perr.p1.x = centerKnob->getValueAtTime(refTime, 0);
            perr.p1.y = centerKnob->getValueAtTime(refTime, 1);
            perr.p2.x = centerKnob->getValueAtTime(time, 0);
            perr.p2.y = centerKnob->getValueAtTime(time, 1);
        } else {
            // Average halfJitter frames before and after refTime and time together to smooth the center
            std::vector<Point> x2PointJitter;

            for (double t = time - halfJitter; t <= time + halfJitter; t += 1.) {
                Point p;
                p.x = centerKnob->getValueAtTime(t, 0);
                p.y = centerKnob->getValueAtTime(t, 1);
                x2PointJitter.push_back(p);
            }
            Point x2avg = {0, 0};
            for (std::size_t i = 0; i < x2PointJitter.size(); ++i) {
                x2avg.x += x2PointJitter[i].x;
                x2avg.y += x2PointJitter[i].y;
            }
            if ( !x2PointJitter.empty() ) {
                x2avg.x /= x2PointJitter.size();
                x2avg.y /= x2PointJitter.size();
            }
            if (!jitterAdd) {
                perr.p1.x = centerKnob->getValueAtTime(time, 0);
                perr.p1.y = centerKnob->getValueAtTime(time, 1);
                perr.p2.x = x2avg.x;
                perr.p2.y = x2avg.y;
            } else {
                Point highFreqX2;

                Point x2;
                x2.x = centerKnob->getValueAtTime(time, 0);
                x2.y = centerKnob->getValueAtTime(time, 1);
                highFreqX2.x = x2.x - x2avg.x;
                highFreqX2.y = x2.y - x2avg.y;

                perr.p1.x = x2.x;
                perr.p1.y = x2.y;
                perr.p2.x = x2.x + highFreqX2.x;
                perr.p2.y = x2.y + highFreqX2.y;
            }
        }

        perr.error = errorKnob->getValueAtTime(time, 0);
        ++pIndex;
    }

    std::sort(pointsWithErrors.begin(), pointsWithErrors.end(), PointWithErrorCompareLess);

    x1->resize( pointsWithErrors.size() );
    x2->resize( pointsWithErrors.size() );

    for (std::size_t i =  0; i < pointsWithErrors.size(); ++i) {
        assert(i == 0 || pointsWithErrors[i].error >= pointsWithErrors[i - 1].error);
        (*x1)[i] = pointsWithErrors[i].p1;
        (*x2)[i] = pointsWithErrors[i].p2;
    }
} // TrackerContext::extractSortedPointsFromMarkers

TrackerContextPrivate::TransformData
TrackerContextPrivate::computeTransformParamsFromTracksAtTime(double refTime,
                                                              double time,
                                                              int jitterPeriod,
                                                              bool jitterAdd,
                                                              bool robustModel,
                                                              const std::vector<TrackMarkerPtr>& allMarkers)
{

    RectD rodRef = getInputRoDAtTime(refTime);
    RectD rodTime = getInputRoDAtTime(time);
    int w1 = rodRef.width();
    int h1 = rodRef.height();
    int w2 = rodTime.width();
    int h2 = rodTime.height();

    std::vector<TrackMarkerPtr> markers;

    for (std::size_t i = 0; i < allMarkers.size(); ++i) {
        if ( allMarkers[i]->isEnabled(time) ) {
            markers.push_back(allMarkers[i]);
        }
    }
    TrackerContextPrivate::TransformData data;
    data.rms = 0.;
    data.time = time;
    data.valid = true;
    assert( !markers.empty() );
    std::vector<Point> x1, x2;
    extractSortedPointsFromMarkers(refTime, time, markers, jitterPeriod, jitterAdd, &x1, &x2);
    assert( x1.size() == x2.size() );
    if ( x1.empty() ) {
        data.valid = false;

        return data;
    }
    if (refTime == time) {
        data.hasRotationAndScale = x1.size() > 1;
        data.translation.x = data.translation.y = data.rotation = 0;
        data.scale = 1.;

        return data;
    }


    const bool dataSetIsUserManual = true;

    try {
        if (x1.size() == 1) {
            data.hasRotationAndScale = false;
            computeTranslationFromNPoints(dataSetIsUserManual, robustModel, x1, x2, w1, h1, w2, h2, &data.translation);
        } else {
            data.hasRotationAndScale = true;
            computeSimilarityFromNPoints(dataSetIsUserManual, robustModel, x1, x2, w1, h1, w2, h2, &data.translation, &data.rotation, &data.scale, &data.rms);
        }
    } catch (...) {
        data.valid = false;
    }

    return data;
} // TrackerContextPrivate::computeTransformParamsFromTracksAtTime

TrackerContextPrivate::CornerPinData
TrackerContextPrivate::computeCornerPinParamsFromTracksAtTime(double refTime,
                                                              double time,
                                                              int jitterPeriod,
                                                              bool jitterAdd,
                                                              bool robustModel,
                                                              const std::vector<TrackMarkerPtr>& allMarkers)
{
    RectD rodRef = getInputRoDAtTime(refTime);
    RectD rodTime = getInputRoDAtTime(time);
    int w1 = rodRef.width();
    int h1 = rodRef.height();
    int w2 = rodTime.width();
    int h2 = rodTime.height();


    std::vector<TrackMarkerPtr> markers;

    for (std::size_t i = 0; i < allMarkers.size(); ++i) {
        if ( allMarkers[i]->isEnabled(time) ) {
            markers.push_back(allMarkers[i]);
        }
    }
    TrackerContextPrivate::CornerPinData data;
    data.rms = 0.;
    data.time = time;
    data.valid = true;
    assert( !markers.empty() );
    std::vector<Point> x1, x2;
    extractSortedPointsFromMarkers(refTime, time, markers, jitterPeriod, jitterAdd, &x1, &x2);
    assert( x1.size() == x2.size() );
    if ( x1.empty() ) {
        data.valid = false;

        return data;
    }
    if (refTime == time) {
        data.h.setIdentity();
        data.nbEnabledPoints = 4;

        return data;
    }


    if (x1.size() == 1) {
        data.h.setTranslationFromOnePoint( euclideanToHomogenous(x1[0]), euclideanToHomogenous(x2[0]) );
        data.nbEnabledPoints = 1;
    } else if (x1.size() == 2) {
        data.h.setSimilarityFromTwoPoints( euclideanToHomogenous(x1[0]), euclideanToHomogenous(x1[1]), euclideanToHomogenous(x2[0]), euclideanToHomogenous(x2[1]) );
        data.nbEnabledPoints = 2;
    } else if (x1.size() == 3) {
        data.h.setAffineFromThreePoints( euclideanToHomogenous(x1[0]), euclideanToHomogenous(x1[1]), euclideanToHomogenous(x1[2]), euclideanToHomogenous(x2[0]), euclideanToHomogenous(x2[1]), euclideanToHomogenous(x2[2]) );
        data.nbEnabledPoints = 3;
    } else {
        const bool dataSetIsUserManual = true;
        try {
            computeHomographyFromNPoints(dataSetIsUserManual, robustModel, x1, x2, w1, h1, w2, h2, &data.h, &data.rms);
            data.nbEnabledPoints = 4;
        } catch (...) {
            data.valid = false;
        }
    }

    return data;
} // TrackerContextPrivate::computeCornerPinParamsFromTracksAtTime


struct CornerPinPoints
{
    Point pts[4];
};

struct TranslateData
{
    Point p;
};

struct ScaleData
{
    double s;
};

struct RotateData
{
    double r;
};

template <typename DstDataType>
void initializeResults(DstDataType* results);

template <>
void initializeResults(CornerPinPoints* results)
{
    for (int i = 0; i < 4; ++i) {
        results->pts[i].x = results->pts[i].y = 0;
    }
}

template <>
void initializeResults(double* results)
{
    *results = 0;
}

template <>
void initializeResults(TranslateData* results)
{
    results->p.x = results->p.y = 0.;
}

template <>
void initializeResults(ScaleData* results)
{
    results->s = 0.;
}

template <>
void initializeResults(RotateData* results)
{
    results->r = 0.;
}

template <typename SrcIterator, typename UserDataType, typename DstDataType>
bool extractSample(SrcIterator it,
                  const UserDataType* userData,
                  DstDataType* sample);

template <>
bool extractSample(QList<TrackerContextPrivate::CornerPinData>::const_iterator it,
                  const CornerPinPoints* userData,
                  CornerPinPoints* sample)
{
    assert(userData);
    for (int c = 0; c < 4; ++c) {
        sample->pts[c] = applyHomography(userData->pts[c], it->h);
    }
    return true;
}

template <>
bool extractSample(KeyFrameSet::const_iterator it,
                   const void* /*userData*/,
                   double* sample)
{
    *sample = it->getValue();
    return true;
}

template <>
bool extractSample(QList<TrackerContextPrivate::TransformData>::const_iterator it,
                   const void* /*userData*/,
                   TranslateData* sample)
{
    sample->p = it->translation;
    return true;
}

template <>
bool extractSample(QList<TrackerContextPrivate::TransformData>::const_iterator it,
                   const void* /*userData*/,
                   RotateData* sample)
{
    if (!it->hasRotationAndScale) {
        return false;
    }
    sample->r = it->rotation;
    return true;
}

template <>
bool extractSample(QList<TrackerContextPrivate::TransformData>::const_iterator it,
                   const void* /*userData*/,
                   ScaleData* sample)
{
    if (!it->hasRotationAndScale) {
        return false;
    }
    sample->s = it->scale;
    return true;
}



template <typename DstDataType>
void appendSample(const DstDataType& sample, DstDataType* results);


template <>
void appendSample(const CornerPinPoints& sample, CornerPinPoints* results)
{
    for (int c = 0; c < 4; ++c) {
        results->pts[c].x += sample.pts[c].x;
        results->pts[c].y += sample.pts[c].y;
    }
}

template <>
void appendSample(const double& sample, double* results)
{
    *results += sample;
}

template <>
void appendSample(const TranslateData& sample, TranslateData* results)
{
    results->p.x += sample.p.x;
    results->p.y += sample.p.y;
}

template <>
void appendSample(const RotateData& sample, RotateData* results)
{
    results->r += sample.r;
}

template <>
void appendSample(const ScaleData& sample, ScaleData* results)
{
    results->s += sample.s;
}

template <typename DstDataType>
void divideResults(int nSamples, DstDataType* results);

template <>
void divideResults(int nSamples, CornerPinPoints* results)
{
    for (int c = 0; c < 4; ++c) {
        results->pts[c].x /= nSamples;
        results->pts[c].y /= nSamples;
    }
}

template <>
void divideResults(int nSamples, double* results)
{
    *results /= nSamples;
}

template <>
void divideResults(int nSamples, TranslateData* results)
{
    results->p.x /= nSamples;
    results->p.y /= nSamples;
}

template <>
void divideResults(int nSamples, RotateData* results)
{
    results->r /= nSamples;
}

template <>
void divideResults(int nSamples, ScaleData* results)
{
    results->s /= nSamples;
}

template <typename SrcIterator, typename UserDataType, typename DstDataType>
static void averageDataFunctor(SrcIterator srcBegin,
                               SrcIterator srcEnd,
                               SrcIterator itResults,
                               const int halfJitter,
                               const UserDataType* userData,
                               DstDataType *results,
                               DstDataType *dataAtIt)
{
    assert(results);
    initializeResults<DstDataType>(results);

    // Get halfJitter samples before the given time
    int nSamplesBefore = 0;
    {
        SrcIterator prevHalfIt = itResults;
        while (prevHalfIt != srcBegin && nSamplesBefore <= halfJitter) {

            DstDataType sample;
            if (extractSample<SrcIterator, UserDataType, DstDataType>(prevHalfIt, userData, &sample)) {
                appendSample<DstDataType>(sample, results);

                if (dataAtIt && prevHalfIt == itResults) {
                    *dataAtIt = sample;
                }
                ++nSamplesBefore;
            }
            --prevHalfIt;
        }
    }
    
    // Get halfJitter samples after the given time
    int nSamplesAfter = 0;
    {
        SrcIterator nextHalfIt = itResults;
        ++nextHalfIt;
        while (nextHalfIt != srcEnd && nSamplesAfter < halfJitter) {

            DstDataType sample;
            if (extractSample<SrcIterator, UserDataType, DstDataType>(nextHalfIt, userData, &sample)) {
                appendSample<DstDataType>(sample, results);

                ++nSamplesAfter;
            }
            ++nextHalfIt;
        }
    }

    int nSamples = nSamplesAfter + nSamplesBefore;
    assert(nSamples > 0);
    if (nSamples > 0) {
        divideResults<DstDataType>(nSamples, results);
    }

} // averageDataFunctor

void
TrackerContextPrivate::computeCornerParamsFromTracksEnd(double refTime,
                                                        double maxFittingError,
                                                        const QList<CornerPinData>& results)
{
    // Make sure we get only valid results
    QList<CornerPinData> validResults;
    for (QList<CornerPinData>::const_iterator it = results.begin(); it != results.end(); ++it) {
        if (it->valid) {
            validResults.push_back(*it);
        }
    }


    // Get all knobs that we are going to write to and block any value changes on them
    KnobIntPtr smoothCornerPinKnob = smoothCornerPin.lock();
    int smoothJitter = smoothCornerPinKnob->getValue();
    int halfJitter = smoothJitter / 2;
    KnobDoublePtr fittingErrorKnob = fittingError.lock();
    KnobDoublePtr fromPointsKnob[4];
    KnobDoublePtr toPointsKnob[4];
    KnobBoolPtr enabledPointsKnob[4];
    KnobStringPtr fittingWarningKnob = fittingErrorWarning.lock();
    for (int i = 0; i < 4; ++i) {
        fromPointsKnob[i] = fromPoints[i].lock();
        toPointsKnob[i] = toPoints[i].lock();
        enabledPointsKnob[i] = enableToPoint[i].lock();
    }

    std::list<KnobIPtr> animatedKnobsChanged;

    fittingErrorKnob->blockValueChanges();
    animatedKnobsChanged.push_back(fittingErrorKnob);

    for (int i = 0; i < 4; ++i) {
        toPointsKnob[i]->blockValueChanges();
        animatedKnobsChanged.push_back(toPointsKnob[i]);
    }

    // Get reference corner pin
    CornerPinPoints refFrom;
    for (int c = 0; c < 4; ++c) {
        refFrom.pts[c].x = fromPointsKnob[c]->getValueAtTime(refTime, 0);
        refFrom.pts[c].y = fromPointsKnob[c]->getValueAtTime(refTime, 1);
    }

    // Create temporary curves and clone the toPoint internal curves at once because setValueAtTime will be slow since it emits
    // signals to create keyframes in keyframeSet
    Curve tmpToPointsCurveX[4], tmpToPointsCurveY[4];
    Curve tmpFittingErrorCurve;
    bool mustShowFittingWarn = false;
    for (QList<CornerPinData>::const_iterator itResults = validResults.begin(); itResults != validResults.end(); ++itResults) {
        const CornerPinData& dataAtTime = *itResults;

        // Add the error to the curve and check if we need to turn on the RMS warning
        {
            KeyFrame kf(dataAtTime.time, dataAtTime.rms);
            if (dataAtTime.rms >= maxFittingError) {
                mustShowFittingWarn = true;
            }
            tmpFittingErrorCurve.addKeyFrame(kf);
        }


        if (smoothJitter <= 1) {
            for (int c = 0; c < 4; ++c) {
                Point toPoint;
                toPoint = applyHomography(refFrom.pts[c], dataAtTime.h);
                KeyFrame kx(dataAtTime.time, toPoint.x);
                KeyFrame ky(dataAtTime.time, toPoint.y);
                tmpToPointsCurveX[c].addKeyFrame(kx);
                tmpToPointsCurveY[c].addKeyFrame(ky);
                //toPoints[c]->setValuesAtTime(dataAtTime[i].time, toPoint.x, toPoint.y, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
            }
        } else {
            // Average to points before and after if using jitter
            CornerPinPoints avgTos;
            averageDataFunctor<QList<CornerPinData>::const_iterator, CornerPinPoints, CornerPinPoints>(validResults.begin(), validResults.end(), itResults, halfJitter, &refFrom, &avgTos, 0);

            for (int c = 0; c < 4; ++c) {
                KeyFrame kx(dataAtTime.time, avgTos.pts[c].x);
                KeyFrame ky(dataAtTime.time, avgTos.pts[c].y);
                tmpToPointsCurveX[c].addKeyFrame(kx);
                tmpToPointsCurveY[c].addKeyFrame(ky);
            }


        } // use jitter

    } // for each result
    
    
    
    // If user wants a post-smooth, apply it
    if (smoothJitter > 1) {

        int halfSmoothJitter = smoothJitter / 2;


        KeyFrameSet xSet[4], ySet[4];
        KeyFrameSet newXSet[4], newYSet[4];
        for (int c = 0; c < 4; ++c) {
            xSet[c] = tmpToPointsCurveX[c].getKeyFrames_mt_safe();
            ySet[c] = tmpToPointsCurveY[c].getKeyFrames_mt_safe();
        }
        for (int c = 0; c < 4; ++c) {

            for (KeyFrameSet::const_iterator it = xSet[c].begin(); it != xSet[c].end(); ++it) {
                double avg;
                averageDataFunctor<KeyFrameSet::const_iterator, void, double>(xSet[c].begin(), xSet[c].end(), it, halfSmoothJitter, 0, &avg, 0);
                KeyFrame k(*it);
                k.setValue(avg);
                newXSet[c].insert(k);
            }
            for (KeyFrameSet::const_iterator it = ySet[c].begin(); it != ySet[c].end(); ++it) {
                double avg;
                averageDataFunctor<KeyFrameSet::const_iterator, void, double>(ySet[c].begin(), ySet[c].end(), it, halfSmoothJitter, 0, &avg, 0);
                KeyFrame k(*it);
                k.setValue(avg);
                newYSet[c].insert(k);
            }

        }

        for (int c = 0; c < 4; ++c) {
            tmpToPointsCurveX[c].setKeyframes(newXSet[c], true);
            tmpToPointsCurveY[c].setKeyframes(newYSet[c], true);
        }
    }

    fittingWarningKnob->setSecret(!mustShowFittingWarn);
    fittingErrorKnob->cloneCurve( ViewSpec::all(), 0, tmpFittingErrorCurve);
    for (int c = 0; c < 4; ++c) {
        toPointsKnob[c]->cloneCurve(ViewSpec::all(), 0, tmpToPointsCurveX[c]);
        toPointsKnob[c]->cloneCurve(ViewSpec::all(), 1, tmpToPointsCurveY[c]);
    }
    for (std::list<KnobIPtr>::iterator it = animatedKnobsChanged.begin(); it != animatedKnobsChanged.end(); ++it) {
        (*it)->unblockValueChanges();
        int nDims = (*it)->getDimension();
        for (int i = 0; i < nDims; ++i) {
            (*it)->evaluateValueChange(i, refTime, ViewIdx(0), eValueChangedReasonNatronInternalEdited);
        }
    }

    endSolve();
} // TrackerContextPrivate::computeCornerParamsFromTracksEnd

void
TrackerContextPrivate::computeCornerParamsFromTracks()
{
#ifndef TRACKER_GENERATE_DATA_SEQUENTIALLY
    lastSolveRequest.tWatcher.reset();
    lastSolveRequest.cpWatcher.reset( new QFutureWatcher<TrackerContextPrivate::CornerPinData>() );
    QObject::connect( lastSolveRequest.cpWatcher.get(), SIGNAL(finished()), this, SLOT(onCornerPinSolverWatcherFinished()) );
    QObject::connect( lastSolveRequest.cpWatcher.get(), SIGNAL(progressValueChanged(int)), this, SLOT(onCornerPinSolverWatcherProgress(int)) );
    lastSolveRequest.cpWatcher->setFuture( QtConcurrent::mapped( lastSolveRequest.keyframes, boost::bind(&TrackerContextPrivate::computeCornerPinParamsFromTracksAtTime, this, lastSolveRequest.refTime, _1, lastSolveRequest.jitterPeriod, lastSolveRequest.jitterAdd, lastSolveRequest.robustModel, lastSolveRequest.allMarkers) ) );
#else
    NodePtr thisNode = node.lock();
    QList<CornerPinData> validResults;
    {
        int nKeys = (int)lastSolveRequest.keyframes.size();
        int keyIndex = 0;
        for (std::set<double>::const_iterator it = lastSolveRequest.keyframes.begin(); it != lastSolveRequest.keyframes.end(); ++it, ++keyIndex) {
            CornerPinData data = computeCornerPinParamsFromTracksAtTime(lastSolveRequest.refTime, *it, lastSolveRequest.jitterPeriod, lastSolveRequest.jitterAdd, lastSolveRequest.robustModel, lastSolveRequest.allMarkers);
            if (data.valid) {
                validResults.push_back(data);
            }
            double progress = (keyIndex + 1) / (double)nKeys;
            thisNode->getApp()->progressUpdate(thisNode, progress);
        }
    }
    computeCornerParamsFromTracksEnd(lastSolveRequest.refTime, lastSolveRequest.maxFittingError, validResults);
#endif
} // TrackerContext::computeCornerParamsFromTracks

void
TrackerContextPrivate::resetTransformParamsAnimation()
{
    {
        // Revert animation on the corner pin
        KnobDoublePtr toPointsKnob[4];
        KnobBoolPtr enabledPointsKnob[4];
        for (int i = 0; i < 4; ++i) {
            toPointsKnob[i] = toPoints[i].lock();
            enabledPointsKnob[i] = enableToPoint[i].lock();
        }


        for (int i = 0; i < 4; ++i) {
            toPointsKnob[i]->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
            toPointsKnob[i]->resetToDefaultValueWithoutSecretNessAndEnabledNess(1);
            enabledPointsKnob[i]->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
        }
    }
    KnobDoublePtr centerKnob = center.lock();

    centerKnob->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
    centerKnob->resetToDefaultValueWithoutSecretNessAndEnabledNess(1);
    {
        // Revert animation on the transform
        KnobDoublePtr translationKnob = translate.lock();
        KnobDoublePtr scaleKnob = scale.lock();
        KnobDoublePtr rotationKnob = rotate.lock();

        translationKnob->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
        translationKnob->resetToDefaultValueWithoutSecretNessAndEnabledNess(1);

        scaleKnob->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
        scaleKnob->resetToDefaultValueWithoutSecretNessAndEnabledNess(1);

        rotationKnob->resetToDefaultValueWithoutSecretNessAndEnabledNess(0);
    }
}

void
TrackerContextPrivate::computeTransformParamsFromTracksEnd(double refTime,
                                                           double maxFittingError,
                                                           const QList<TransformData>& results)
{
    QList<TransformData> validResults;
    for (QList<TransformData>::const_iterator it = results.begin(); it != results.end(); ++it) {
        if (it->valid) {
            validResults.push_back(*it);
        }
    }


    KnobIntPtr smoothKnob = smoothTransform.lock();
    int smoothTJitter, smoothRJitter, smoothSJitter;

    smoothTJitter = smoothKnob->getValue(0);
    smoothRJitter = smoothKnob->getValue(1);
    smoothSJitter = smoothKnob->getValue(2);

    int halfTJitter = smoothTJitter / 2;
    int halfRJitter = smoothRJitter / 2;
    int halfSJitter = smoothSJitter / 2;


    KnobDoublePtr translationKnob = translate.lock();
    KnobDoublePtr scaleKnob = scale.lock();
    KnobDoublePtr rotationKnob = rotate.lock();
    KnobDoublePtr centerKnob = center.lock();
    KnobDoublePtr fittingErrorKnob = fittingError.lock();
    KnobStringPtr fittingWarningKnob = fittingErrorWarning.lock();
    translationKnob->blockValueChanges();
    scaleKnob->blockValueChanges();
    rotationKnob->blockValueChanges();
    fittingErrorKnob->blockValueChanges();

    std::list<KnobIPtr> animatedKnobsChanged;
    animatedKnobsChanged.push_back(translationKnob);
    animatedKnobsChanged.push_back(scaleKnob);
    animatedKnobsChanged.push_back(rotationKnob);
    animatedKnobsChanged.push_back(fittingErrorKnob);


    Curve tmpTXCurve, tmpTYCurve, tmpRotateCurve, tmpScaleCurve, tmpFittingErrorCurve;
    bool mustShowFittingWarn = false;
    for (QList<TransformData>::const_iterator itResults = validResults.begin(); itResults != validResults.end(); ++itResults) {
        const TransformData& dataAtTime = *itResults;
        {
            KeyFrame kf(dataAtTime.time, dataAtTime.rms);
            if (dataAtTime.rms >= maxFittingError) {
                mustShowFittingWarn = true;
            }
            tmpFittingErrorCurve.addKeyFrame(kf);
        }
        if (!dataAtTime.hasRotationAndScale) {
            // no rotation or scale: simply extract the translation
            if (smoothTJitter > 1) {
                TranslateData avgT;
                averageDataFunctor<QList<TransformData>::const_iterator, void, TranslateData>(validResults.begin(), validResults.end(), itResults, halfTJitter, 0, &avgT, 0);
                KeyFrame kx(dataAtTime.time, avgT.p.x);
                KeyFrame ky(dataAtTime.time, avgT.p.y);
                tmpTXCurve.addKeyFrame(kx);
                tmpTYCurve.addKeyFrame(ky);
            } else {
                KeyFrame kx(dataAtTime.time, dataAtTime.translation.x);
                KeyFrame ky(dataAtTime.time, dataAtTime.translation.y);
                tmpTXCurve.addKeyFrame(kx);
                tmpTYCurve.addKeyFrame(ky);

            }
        } else {
            if (smoothRJitter > 1) {
                RotateData avgR;
                averageDataFunctor<QList<TransformData>::const_iterator, void, RotateData>(validResults.begin(), validResults.end(), itResults, halfRJitter, 0, &avgR, 0);

                KeyFrame k(dataAtTime.time, avgR.r);
                tmpRotateCurve.addKeyFrame(k);

            } else {
                KeyFrame k(dataAtTime.time, dataAtTime.rotation);
                tmpRotateCurve.addKeyFrame(k);
            }
            if (smoothSJitter > 1) {
                ScaleData avgR;
                averageDataFunctor<QList<TransformData>::const_iterator, void, ScaleData>(validResults.begin(), validResults.end(), itResults, halfSJitter, 0, &avgR, 0);
                KeyFrame k(dataAtTime.time, avgR.s);
                tmpScaleCurve.addKeyFrame(k);

            } else {
                KeyFrame k(dataAtTime.time, dataAtTime.scale);
                tmpScaleCurve.addKeyFrame(k);
            }
#pragma message WARN("BUG: actual translation must be computed from centerKnob->getValueAtTime(), tmpRotateCurve, tmpScaleCurve, and validResults[*].translation before smoothing https://github.com/NatronGitHub/Natron/issues/289")
            if (smoothTJitter > 1) {
                TranslateData avgT;
                averageDataFunctor<QList<TransformData>::const_iterator, void, TranslateData>(validResults.begin(), validResults.end(), itResults, halfTJitter, 0, &avgT, 0);
                KeyFrame kx(dataAtTime.time, avgT.p.x);
                KeyFrame ky(dataAtTime.time, avgT.p.y);
                tmpTXCurve.addKeyFrame(kx);
                tmpTYCurve.addKeyFrame(ky);
            } else {
                KeyFrame kx(dataAtTime.time, dataAtTime.translation.x);
                KeyFrame ky(dataAtTime.time, dataAtTime.translation.y);
                tmpTXCurve.addKeyFrame(kx);
                tmpTYCurve.addKeyFrame(ky);

            }
        }
    } // for all samples

    fittingWarningKnob->setSecret(!mustShowFittingWarn);
    fittingErrorKnob->cloneCurve(ViewSpec::all(), 0, tmpFittingErrorCurve);
    translationKnob->cloneCurve(ViewSpec::all(), 0, tmpTXCurve);
    translationKnob->cloneCurve(ViewSpec::all(), 1, tmpTYCurve);
    rotationKnob->cloneCurve(ViewSpec::all(), 0, tmpRotateCurve);
    scaleKnob->cloneCurve(ViewSpec::all(), 0, tmpScaleCurve);
    scaleKnob->cloneCurve(ViewSpec::all(), 1, tmpScaleCurve);

    for (std::list<KnobIPtr>::iterator it = animatedKnobsChanged.begin(); it != animatedKnobsChanged.end(); ++it) {
        (*it)->unblockValueChanges();
        int nDims = (*it)->getDimension();
        for (int i = 0; i < nDims; ++i) {
            (*it)->evaluateValueChange(i, refTime, ViewIdx(0), eValueChangedReasonNatronInternalEdited);
        }
    }
    endSolve();
} // TrackerContextPrivate::computeTransformParamsFromTracksEnd

void
TrackerContextPrivate::computeTransformParamsFromTracks()
{
#ifndef TRACKER_GENERATE_DATA_SEQUENTIALLY
    lastSolveRequest.cpWatcher.reset();
    lastSolveRequest.tWatcher.reset( new QFutureWatcher<TrackerContextPrivate::TransformData>() );
    QObject::connect( lastSolveRequest.tWatcher.get(), SIGNAL(finished()), this, SLOT(onTransformSolverWatcherFinished()) );
    QObject::connect( lastSolveRequest.tWatcher.get(), SIGNAL(progressValueChanged(int)), this, SLOT(onTransformSolverWatcherProgress(int)) );
    lastSolveRequest.tWatcher->setFuture( QtConcurrent::mapped( lastSolveRequest.keyframes, boost::bind(&TrackerContextPrivate::computeTransformParamsFromTracksAtTime, this, lastSolveRequest.refTime, _1, lastSolveRequest.jitterPeriod, lastSolveRequest.jitterAdd, lastSolveRequest.robustModel, lastSolveRequest.allMarkers) ) );
#else
    NodePtr thisNode = node.lock();
    QList<TransformData> validResults;
    {
        int nKeys = lastSolveRequest.keyframes.size();
        int keyIndex = 0;
        for (std::set<double>::const_iterator it = lastSolveRequest.keyframes.begin(); it != lastSolveRequest.keyframes.end(); ++it, ++keyIndex) {
            TransformData data = computeTransformParamsFromTracksAtTime(lastSolveRequest.refTime, *it, lastSolveRequest.jitterPeriod, lastSolveRequest.jitterAdd, lastSolveRequest.robustModel, lastSolveRequest.allMarkers);
            if (data.valid) {
                validResults.push_back(data);
            }
            double progress = (keyIndex + 1) / (double)nKeys;
            thisNode->getApp()->progressUpdate(thisNode, progress);
        }
    }
    computeTransformParamsFromTracksEnd(lastSolveRequest.refTime, lastSolveRequest.maxFittingError, validResults);
#endif
} // TrackerContextPrivate::computeTransformParamsFromTracks

void
TrackerContextPrivate::onCornerPinSolverWatcherFinished()
{
    assert(lastSolveRequest.cpWatcher);
    computeCornerParamsFromTracksEnd( lastSolveRequest.refTime, lastSolveRequest.maxFittingError, lastSolveRequest.cpWatcher->future().results() );
}

void
TrackerContextPrivate::onTransformSolverWatcherFinished()
{
    assert(lastSolveRequest.tWatcher);
    computeTransformParamsFromTracksEnd( lastSolveRequest.refTime, lastSolveRequest.maxFittingError, lastSolveRequest.tWatcher->future().results() );
}

void
TrackerContextPrivate::onCornerPinSolverWatcherProgress(int progress)
{
    assert(lastSolveRequest.cpWatcher);
    NodePtr thisNode = node.lock();
    double min = lastSolveRequest.cpWatcher->progressMinimum();
    double max = lastSolveRequest.cpWatcher->progressMaximum();
    double p = (progress - min) / (max - min);
    thisNode->getApp()->progressUpdate(thisNode, p);
}

void
TrackerContextPrivate::onTransformSolverWatcherProgress(int progress)
{
    assert(lastSolveRequest.tWatcher);
    NodePtr thisNode = node.lock();
    double min = lastSolveRequest.tWatcher->progressMinimum();
    double max = lastSolveRequest.tWatcher->progressMaximum();
    double p = (progress - min) / (max - min);
    thisNode->getApp()->progressUpdate(thisNode, p);
}

void
TrackerContextPrivate::setSolverParamsEnabled(bool enabled)
{
    motionType.lock()->setAllDimensionsEnabled(enabled);
    setCurrentFrameButton.lock()->setAllDimensionsEnabled(enabled);
    robustModel.lock()->setAllDimensionsEnabled(enabled);
    referenceFrame.lock()->setAllDimensionsEnabled(enabled);
    transformType.lock()->setAllDimensionsEnabled(enabled);
    jitterPeriod.lock()->setAllDimensionsEnabled(enabled);
    smoothTransform.lock()->setAllDimensionsEnabled(enabled);
    smoothCornerPin.lock()->setAllDimensionsEnabled(enabled);
}

void
TrackerContextPrivate::endSolve()
{
    lastSolveRequest.cpWatcher.reset();
    lastSolveRequest.tWatcher.reset();
    lastSolveRequest.keyframes.clear();
    lastSolveRequest.allMarkers.clear();
    setSolverParamsEnabled(true);
    NodePtr n = node.lock();
    n->getApp()->progressEnd(n);
    n->getEffectInstance()->endChanges();
}

bool
TrackerContextPrivate::isTransformAutoGenerationEnabled() const
{
    return autoGenerateTransform.lock()->getValue();
}

void
TrackerContextPrivate::setTransformOutOfDate(bool outdated)
{
    transformOutOfDateLabel.lock()->setSecret(!outdated);
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_TrackerContextPrivate.cpp"
