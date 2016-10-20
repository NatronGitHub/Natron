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


#include "TrackerNode.h"
#include "TrackerNodePrivate.h"

#include "Engine/Node.h"

NATRON_NAMESPACE_ENTER


static
KnobDoublePtr
getCornerPinPoint(const NodePtr& node,
                  bool isFrom,
                  int index)
{
    assert(0 <= index && index < 4);
    QString name = isFrom ? QString::fromUtf8("from%1").arg(index + 1) : QString::fromUtf8("to%1").arg(index + 1);
    KnobIPtr knob = node->getKnobByName( name.toStdString() );
    assert(knob);
    KnobDoublePtr  ret = toKnobDouble(knob);
    assert(ret);

    return ret;
}

void
TrackerNodePrivate::exportTrackDataFromExportOptions()
{
    //bool transformLink = _imp->exportLink.lock()->getValue();
    KnobChoicePtr transformTypeKnob = transformType.lock();

    assert(transformTypeKnob);
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    KnobChoicePtr motionTypeKnob = motionType.lock();
    if (!motionTypeKnob) {
        return;
    }
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum mt = (TrackerMotionTypeEnum)motionType_i;

    if (mt == eTrackerMotionTypeNone) {
        Dialogs::errorDialog( tr("Tracker Export").toStdString(), tr("Please select the export mode with the Motion Type parameter").toStdString() );

        return;
    }

    bool linked = exportLink.lock()->getValue();
    QString pluginID;
    switch (transformType) {
        case eTrackerTransformNodeCornerPin:
            pluginID = QString::fromUtf8(PLUGINID_OFX_CORNERPIN);
            break;
        case eTrackerTransformNodeTransform:
            pluginID = QString::fromUtf8(PLUGINID_OFX_TRANSFORM);
            break;
    }

    NodePtr thisNode = getNode();
    AppInstancePtr app = thisNode->getApp();
    CreateNodeArgsPtr args(CreateNodeArgs::create( pluginID.toStdString(), thisNode->getGroup() ));
    args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

    NodePtr createdNode = app->createNode(args);
    if (!createdNode) {
        return;
    }

    // Move the new node
    double thisNodePos[2];
    double thisNodeSize[2];
    thisNode->getPosition(&thisNodePos[0], &thisNodePos[1]);
    thisNode->getSize(&thisNodeSize[0], &thisNodeSize[1]);
    createdNode->setPosition(thisNodePos[0] + thisNodeSize[0] * 2., thisNodePos[1]);

    int timeForFromPoints = getTransformReferenceFrame();


    switch (transformType) {
        case eTrackerTransformNodeCornerPin: {
            KnobDoublePtr cornerPinToPoints[4];
            KnobDoublePtr cornerPinFromPoints[4];


            for (unsigned int i = 0; i < 4; ++i) {
                cornerPinFromPoints[i] = getCornerPinPoint(createdNode, true, i);
                assert(cornerPinFromPoints[i]);
                for (int j = 0; j < cornerPinFromPoints[i]->getNDimensions(); ++j) {
                    cornerPinFromPoints[i]->setValue(fromPoints[i].lock()->getValueAtTime(timeForFromPoints, j), ViewSpec(0), j);
                }

                cornerPinToPoints[i] = getCornerPinPoint(createdNode, false, i);
                assert(cornerPinToPoints[i]);
                if (!linked) {
                    cornerPinToPoints[i]->clone( toPoints[i].lock() );
                } else {
                    bool ok = cornerPinToPoints[i]->slaveTo(_imp->toPoints[i].lock());
                    (void)ok;
                    assert(ok);
                }
            }
            {
                KnobIPtr knob = createdNode->getKnobByName(kCornerPinParamMatrix);
                if (knob) {
                    KnobDoublePtr isType = toKnobDouble(knob);
                    if (isType) {
                        isType->clone( _imp->cornerPinMatrix.lock() );
                    }
                }
            }
            break;
        }
        case eTrackerTransformNodeTransform: {
            KnobIPtr translateKnob = createdNode->getKnobByName(kTransformParamTranslate);
            if (translateKnob) {
                KnobDoublePtr isDbl = toKnobDouble(translateKnob);
                if (isDbl) {
                    if (!linked) {
                        isDbl->clone( _imp->translate.lock() );
                    } else {
                        isDbl->slaveTo(0, _imp->translate.lock(), 0);
                        isDbl->slaveTo(1, _imp->translate.lock(), 1);
                    }
                }
            }

            KnobIPtr scaleKnob = createdNode->getKnobByName(kTransformParamScale);
            if (scaleKnob) {
                KnobDoublePtr isDbl = toKnobDouble(scaleKnob);
                if (isDbl) {
                    if (!linked) {
                        isDbl->clone( _imp->scale.lock() );
                    } else {
                        isDbl->slaveTo(0, _imp->scale.lock(), 0);
                        isDbl->slaveTo(1, _imp->scale.lock(), 1);
                    }
                }
            }

            KnobIPtr rotateKnob = createdNode->getKnobByName(kTransformParamRotate);
            if (rotateKnob) {
                KnobDoublePtr isDbl = toKnobDouble(rotateKnob);
                if (isDbl) {
                    if (!linked) {
                        isDbl->clone( _imp->rotate.lock() );
                    } else {
                        isDbl->slaveTo(0, _imp->rotate.lock(), 0);
                    }
                }
            }
            KnobIPtr centerKnob = createdNode->getKnobByName(kTransformParamCenter);
            if (centerKnob) {
                KnobDoublePtr isDbl = toKnobDouble(centerKnob);
                if (isDbl) {
                    isDbl->clone( _imp->center.lock() );
                }
            }
            break;
        }
    } // switch

    KnobIPtr cpInvertKnob = createdNode->getKnobByName(kTransformParamInvert);
    if (cpInvertKnob) {
        KnobBoolPtr isBool = toKnobBool(cpInvertKnob);
        if (isBool) {
            if (!linked) {
                isBool->clone( _imp->invertTransform.lock() );
            } else {
                isBool->slaveTo(0, _imp->invertTransform.lock(), 0);
            }
        }
    }

    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamMotionBlur);
        if (knob) {
            KnobDoublePtr isType = toKnobDouble(knob);
            if (isType) {
                isType->clone( _imp->motionBlur.lock() );
            }
        }
    }
    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamShutter);
        if (knob) {
            KnobDoublePtr isType = toKnobDouble(knob);
            if (isType) {
                isType->clone( _imp->shutter.lock() );
            }
        }
    }
    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamShutterOffset);
        if (knob) {
            KnobChoicePtr isType = toKnobChoice(knob);
            if (isType) {
                isType->clone( _imp->shutterOffset.lock() );
            }
        }
    }
    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamCustomShutterOffset);
        if (knob) {
            KnobDoublePtr isType = toKnobDouble(knob);
            if (isType) {
                isType->clone( _imp->customShutterOffset.lock() );
            }
        }
    }
} // exportTrackDataFromExportOptions



void
TrackerNodePrivate::solveTransformParamsIfAutomatic()
{
    if ( isTransformAutoGenerationEnabled() ) {
        solveTransformParams();
    } else {
        setTransformOutOfDate(true);
    }
}

void
TrackerNodePrivate::solveTransformParams()
{
    _imp->setTransformOutOfDate(false);

    std::vector<TrackMarkerPtr> markers;

    getAllMarkers(&markers);
    if ( markers.empty() ) {
        return;
    }

    _imp->resetTransformParamsAnimation();

    KnobChoicePtr motionTypeKnob = _imp->motionType.lock();
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum type =  (TrackerMotionTypeEnum)motionType_i;
    double refTime = (double)getTransformReferenceFrame();
    int jitterPeriod = 0;
    bool jitterAdd = false;
    switch (type) {
        case eTrackerMotionTypeNone:

            return;
        case eTrackerMotionTypeMatchMove:
        case eTrackerMotionTypeStabilize:
            break;
        case eTrackerMotionTypeAddJitter:
        case eTrackerMotionTypeRemoveJitter: {
            jitterPeriod = _imp->jitterPeriod.lock()->getValue();
            jitterAdd = type == eTrackerMotionTypeAddJitter;
            break;
        }
    }

    _imp->setSolverParamsEnabled(false);

    std::set<double> keyframes;
    {
        for (std::size_t i = 0; i < markers.size(); ++i) {
            std::set<double> keys;
            markers[i]->getCenterKeyframes(&keys);
            for (std::set<double>::iterator it = keys.begin(); it != keys.end(); ++it) {
                keyframes.insert(*it);
            }
        }
    }
    KnobChoicePtr transformTypeKnob = _imp->transformType.lock();
    assert(transformTypeKnob);
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    NodePtr node = getNode();
    node->getEffectInstance()->beginChanges();


    if (type == eTrackerMotionTypeStabilize) {
        _imp->invertTransform.lock()->setValue(true);
    } else {
        _imp->invertTransform.lock()->setValue(false);
    }

    KnobDoublePtr centerKnob = _imp->center.lock();

    // Set the center at the reference frame
    Point centerValue = {0, 0};
    int nSamplesAtRefTime = 0;
    for (std::size_t i = 0; i < markers.size(); ++i) {
        if ( !markers[i]->isEnabled(refTime) ) {
            continue;
        }
        KnobDoublePtr markerCenterKnob = markers[i]->getCenterKnob();

        centerValue.x += markerCenterKnob->getValueAtTime(refTime, 0);
        centerValue.y += markerCenterKnob->getValueAtTime(refTime, 1);
        ++nSamplesAtRefTime;
    }
    if (nSamplesAtRefTime) {
        centerValue.x /= nSamplesAtRefTime;
        centerValue.y /= nSamplesAtRefTime;
        centerKnob->setValues(centerValue.x, centerValue.y, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
    }

    bool robustModel;
    robustModel = _imp->robustModel.lock()->getValue();

    KnobDoublePtr maxFittingErrorKnob = _imp->fittingErrorWarnIfAbove.lock();
    const double maxFittingError = maxFittingErrorKnob->getValue();

    node->getApp()->progressStart( node, tr("Solving for transform parameters...").toStdString(), std::string() );

    _imp->lastSolveRequest.refTime = refTime;
    _imp->lastSolveRequest.jitterPeriod = jitterPeriod;
    _imp->lastSolveRequest.jitterAdd = jitterAdd;
    _imp->lastSolveRequest.allMarkers = markers;
    _imp->lastSolveRequest.keyframes = keyframes;
    _imp->lastSolveRequest.robustModel = robustModel;
    _imp->lastSolveRequest.maxFittingError = maxFittingError;

    switch (transformType) {
        case eTrackerTransformNodeTransform:
            _imp->computeTransformParamsFromTracks();
            break;
        case eTrackerTransformNodeCornerPin:
            _imp->computeCornerParamsFromTracks();
            break;
    }
} // TrackerNodePrivate::solveTransformParams


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
TrackerNodePrivate::computeCornerParamsFromTracksEnd(double refTime,
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
        int nDims = (*it)->getNDimensions();
        for (int i = 0; i < nDims; ++i) {
            (*it)->evaluateValueChange(i, refTime, ViewIdx(0), eValueChangedReasonNatronInternalEdited);
        }
    }

    endSolve();
} // TrackerNodePrivate::computeCornerParamsFromTracksEnd

void
TrackerNodePrivate::computeCornerParamsFromTracks()
{
#ifndef TRACKER_GENERATE_DATA_SEQUENTIALLY
    lastSolveRequest.tWatcher.reset();
    lastSolveRequest.cpWatcher.reset( new QFutureWatcher<TrackerContextPrivate::CornerPinData>() );
    QObject::connect( lastSolveRequest.cpWatcher.get(), SIGNAL(finished()), publicInterface, SLOT(onCornerPinSolverWatcherFinished()) );
    QObject::connect( lastSolveRequest.cpWatcher.get(), SIGNAL(progressValueChanged(int)), publicInterface, SLOT(onCornerPinSolverWatcherProgress(int)) );
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
TrackerNodePrivate::resetTransformParamsAnimation()
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
            toPointsKnob[i]->resetToDefaultValue(0);
            toPointsKnob[i]->resetToDefaultValue(1);
            enabledPointsKnob[i]->resetToDefaultValue(0);
        }
    }
    KnobDoublePtr centerKnob = center.lock();

    centerKnob->resetToDefaultValue(0);
    centerKnob->resetToDefaultValue(1);
    {
        // Revert animation on the transform
        KnobDoublePtr translationKnob = translate.lock();
        KnobDoublePtr scaleKnob = scale.lock();
        KnobDoublePtr rotationKnob = rotate.lock();

        translationKnob->resetToDefaultValue(0);
        translationKnob->resetToDefaultValue(1);

        scaleKnob->resetToDefaultValue(0);
        scaleKnob->resetToDefaultValue(1);

        rotationKnob->resetToDefaultValue(0);
    }
}

void
TrackerNodePrivate::computeTransformParamsFromTracksEnd(double refTime,
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

    KnobDoublePtr translationKnob = translate.lock();
    KnobDoublePtr scaleKnob = scale.lock();
    KnobDoublePtr rotationKnob = rotate.lock();
    KnobDoublePtr fittingErrorKnob = fittingError.lock();
    KnobStringPtr fittingWarningKnob = fittingErrorWarning.lock();
    int halfTJitter = smoothTJitter / 2;
    int halfRJitter = smoothRJitter / 2;
    int halfSJitter = smoothSJitter / 2;



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
        if (dataAtTime.hasRotationAndScale) {
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
        int nDims = (*it)->getNDimensions();
        for (int i = 0; i < nDims; ++i) {
            (*it)->evaluateValueChange(i, refTime, ViewIdx(0), eValueChangedReasonNatronInternalEdited);
        }
    }
    endSolve();
} // TrackerNodePrivate::computeTransformParamsFromTracksEnd

void
TrackerNodePrivate::computeTransformParamsFromTracks()
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
TrackerNode::onCornerPinSolverWatcherFinished()
{
    assert(lastSolveRequest.cpWatcher);
    computeCornerParamsFromTracksEnd( lastSolveRequest.refTime, lastSolveRequest.maxFittingError, lastSolveRequest.cpWatcher->future().results() );
}

void
TrackerNode::onTransformSolverWatcherFinished()
{
    assert(lastSolveRequest.tWatcher);
    computeTransformParamsFromTracksEnd( lastSolveRequest.refTime, lastSolveRequest.maxFittingError, lastSolveRequest.tWatcher->future().results() );
}

void
TrackerNode::onCornerPinSolverWatcherProgress(int progress)
{
    assert(lastSolveRequest.cpWatcher);
    NodePtr thisNode = node.lock();
    double min = lastSolveRequest.cpWatcher->progressMinimum();
    double max = lastSolveRequest.cpWatcher->progressMaximum();
    double p = (progress - min) / (max - min);
    thisNode->getApp()->progressUpdate(thisNode, p);
}

void
TrackerNode::onTransformSolverWatcherProgress(int progress)
{
    assert(lastSolveRequest.tWatcher);
    NodePtr thisNode = node.lock();
    double min = lastSolveRequest.tWatcher->progressMinimum();
    double max = lastSolveRequest.tWatcher->progressMaximum();
    double p = (progress - min) / (max - min);
    thisNode->getApp()->progressUpdate(thisNode, p);
}



void
TrackerNodePrivate::endSolve()
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


NATRON_NAMESPACE_EXIT