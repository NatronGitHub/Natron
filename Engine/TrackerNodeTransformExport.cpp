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


#include "TrackerNode.h"
#include "TrackerNodePrivate.h"


CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)



#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Engine/AppInstance.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/TrackerHelper.h"
#include "Engine/TrackMarker.h"


#ifdef DEBUG
// Uncomment to prevent transform computation from using multiple threads
//#define TRACKER_GENERATE_DATA_SEQUENTIALLY
#endif



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

    NodePtr thisNode = publicInterface->getNode();
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

    TimeValue timeForFromPoints(referenceFrame.lock()->getValue());


    switch (transformType) {
        case eTrackerTransformNodeCornerPin: {
            KnobDoublePtr cornerPinToPoints[4];
            KnobDoublePtr cornerPinFromPoints[4];


            for (unsigned int i = 0; i < 4; ++i) {
                cornerPinFromPoints[i] = getCornerPinPoint(createdNode, true, i);
                assert(cornerPinFromPoints[i]);
                for (int j = 0; j < cornerPinFromPoints[i]->getNDimensions(); ++j) {
                    cornerPinFromPoints[i]->setValue(fromPoints[i].lock()->getValueAtTime(timeForFromPoints, DimIdx(j)), ViewSetSpec::all(), DimIdx(j));
                }

                cornerPinToPoints[i] = getCornerPinPoint(createdNode, false, i);
                assert(cornerPinToPoints[i]);
                if (!linked) {
                    cornerPinToPoints[i]->copyKnob( toPoints[i].lock() );
                } else {
                    bool ok = cornerPinToPoints[i]->linkTo(toPoints[i].lock());
                    (void)ok;
                    assert(ok);
                }
            }
            {
                KnobIPtr knob = createdNode->getKnobByName(kCornerPinParamMatrix);
                if (knob) {
                    KnobDoublePtr isType = toKnobDouble(knob);
                    if (isType) {
                        isType->copyKnob(cornerPinMatrix.lock() );
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
                        isDbl->copyKnob(translate.lock() );
                    } else {
                        ignore_result(isDbl->linkTo(translate.lock()));
                    }
                }
            }

            KnobIPtr scaleKnob = createdNode->getKnobByName(kTransformParamScale);
            if (scaleKnob) {
                KnobDoublePtr isDbl = toKnobDouble(scaleKnob);
                if (isDbl) {
                    if (!linked) {
                        isDbl->copyKnob(scale.lock() );
                    } else {
                        ignore_result(isDbl->linkTo(scale.lock()));
                    }
                }
            }

            KnobIPtr rotateKnob = createdNode->getKnobByName(kTransformParamRotate);
            if (rotateKnob) {
                KnobDoublePtr isDbl = toKnobDouble(rotateKnob);
                if (isDbl) {
                    if (!linked) {
                        isDbl->copyKnob(rotate.lock());
                    } else {
                        ignore_result(isDbl->linkTo(rotate.lock()));
                    }
                }
            }
            KnobIPtr centerKnob = createdNode->getKnobByName(kTransformParamCenter);
            if (centerKnob) {
                KnobDoublePtr isDbl = toKnobDouble(centerKnob);
                if (isDbl) {
                    isDbl->copyKnob( center.lock() );
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
                isBool->copyKnob(invertTransform.lock());
            } else {
                ignore_result(isBool->linkTo(invertTransform.lock()));
            }
        }
    }

    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamMotionBlur);
        if (knob) {
            KnobDoublePtr isType = toKnobDouble(knob);
            if (isType) {
                isType->copyKnob(motionBlur.lock());
            }
        }
    }
    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamShutter);
        if (knob) {
            KnobDoublePtr isType = toKnobDouble(knob);
            if (isType) {
                isType->copyKnob(shutter.lock());
            }
        }
    }
    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamShutterOffset);
        if (knob) {
            KnobChoicePtr isType = toKnobChoice(knob);
            if (isType) {
                isType->copyKnob(shutterOffset.lock());
            }
        }
    }
    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamCustomShutterOffset);
        if (knob) {
            KnobDoublePtr isType = toKnobDouble(knob);
            if (isType) {
                isType->copyKnob(customShutterOffset.lock());
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
    setTransformOutOfDate(false);

    std::vector<TrackMarkerPtr> markers;

    knobsTable->getAllMarkers(&markers);
    if ( markers.empty() ) {
        return;
    }

    resetTransformParamsAnimation();

    KnobChoicePtr motionTypeKnob = motionType.lock();
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum type =  (TrackerMotionTypeEnum)motionType_i;
    TimeValue refTime(referenceFrame.lock()->getValue());
    int jitterPer = 0;
    bool jitterAdd = false;
    switch (type) {
        case eTrackerMotionTypeNone:

            return;
        case eTrackerMotionTypeMatchMove:
        case eTrackerMotionTypeStabilize:
            break;
        case eTrackerMotionTypeAddJitter:
        case eTrackerMotionTypeRemoveJitter: {
            jitterPer = jitterPeriod.lock()->getValue();
            jitterAdd = type == eTrackerMotionTypeAddJitter;
            break;
        }
    }

    setSolverParamsEnabled(false);

    std::set<TimeValue> keyframes;
    {
        for (std::size_t i = 0; i < markers.size(); ++i) {
            std::set<double> keys;
            markers[i]->getCenterKeyframes(&keys);
            for (std::set<double>::iterator it = keys.begin(); it != keys.end(); ++it) {
                keyframes.insert(TimeValue(*it));
            }
        }
    }
    KnobChoicePtr transformTypeKnob = transformType.lock();
    assert(transformTypeKnob);
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    NodePtr node = publicInterface->getNode();

    invertTransform.lock()->setValue(type == eTrackerMotionTypeStabilize);

    KnobDoublePtr centerKnob = center.lock();

    // Set the center at the reference frame
    Point centerValue = {0, 0};
    int nSamplesAtRefTime = 0;
    for (std::size_t i = 0; i < markers.size(); ++i) {
        if ( !markers[i]->isEnabled(refTime) ) {
            continue;
        }
        KnobDoublePtr markerCenterKnob = markers[i]->getCenterKnob();

        centerValue.x += markerCenterKnob->getValueAtTime(refTime);
        centerValue.y += markerCenterKnob->getValueAtTime(refTime, DimIdx(1));
        ++nSamplesAtRefTime;
    }
    if (nSamplesAtRefTime) {
        centerValue.x /= nSamplesAtRefTime;
        centerValue.y /= nSamplesAtRefTime;
        {
            std::vector<double> values(2);
            values[0] = centerValue.x;
            values[1] = centerValue.y;
            centerKnob->setValueAcrossDimensions(values);
        }

    }

    bool robust;
    robust = robustModel.lock()->getValue();

    KnobDoublePtr maxFittingErrorKnob = fittingErrorWarnIfAbove.lock();
    const double maxFittingError = maxFittingErrorKnob->getValue();

    node->getApp()->progressStart( node, tr("Solving for transform parameters...").toStdString(), std::string() );

    lastSolveRequest.refTime = refTime;
    lastSolveRequest.jitterPeriod = jitterPer;
    lastSolveRequest.jitterAdd = jitterAdd;
    lastSolveRequest.allMarkers = markers;
    lastSolveRequest.keyframes = keyframes;
    lastSolveRequest.robustModel = robust;
    lastSolveRequest.maxFittingError = maxFittingError;

    switch (transformType) {
        case eTrackerTransformNodeTransform:
            computeTransformParamsFromTracks();
            break;
        case eTrackerTransformNodeCornerPin:
            computeCornerParamsFromTracks();
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
bool extractSample(QList<CornerPinData>::const_iterator it,
                   const CornerPinPoints* userData,
                   CornerPinPoints* sample)
{
    assert(userData);
    for (int c = 0; c < 4; ++c) {
        sample->pts[c] = TrackerHelper::applyHomography(userData->pts[c], it->h);
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
bool extractSample(QList<TransformData>::const_iterator it,
                   const void* /*userData*/,
                   TranslateData* sample)
{
    sample->p = it->translation;
    return true;
}

template <>
bool extractSample(QList<TransformData>::const_iterator it,
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
bool extractSample(QList<TransformData>::const_iterator it,
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
        refFrom.pts[c].x = fromPointsKnob[c]->getValueAtTime(TimeValue(refTime));
        refFrom.pts[c].y = fromPointsKnob[c]->getValueAtTime(TimeValue(refTime), DimIdx(1));
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
            tmpFittingErrorCurve.setOrAddKeyframe(kf);
        }


        if (smoothJitter <= 1) {
            for (int c = 0; c < 4; ++c) {
                Point toPoint;
                toPoint = TrackerHelper::applyHomography(refFrom.pts[c], dataAtTime.h);
                KeyFrame kx(dataAtTime.time, toPoint.x);
                KeyFrame ky(dataAtTime.time, toPoint.y);
                tmpToPointsCurveX[c].setOrAddKeyframe(kx);
                tmpToPointsCurveY[c].setOrAddKeyframe(ky);
                //toPoints[c]->setValuesAtTime(dataAtTime[i].time, toPoint.x, toPoint.y, ViewSpec::all(), eValueChangedReasonUserEdited);
            }
        } else {
            // Average to points before and after if using jitter
            CornerPinPoints avgTos;
            averageDataFunctor<QList<CornerPinData>::const_iterator, CornerPinPoints, CornerPinPoints>(validResults.begin(), validResults.end(), itResults, halfJitter, &refFrom, &avgTos, 0);

            for (int c = 0; c < 4; ++c) {
                KeyFrame kx(dataAtTime.time, avgTos.pts[c].x);
                KeyFrame ky(dataAtTime.time, avgTos.pts[c].y);
                tmpToPointsCurveX[c].setOrAddKeyframe(kx);
                tmpToPointsCurveY[c].setOrAddKeyframe(ky);
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
    fittingErrorKnob->cloneCurve(ViewIdx(0), DimIdx(0), tmpFittingErrorCurve, 0 /*offset*/, 0 /*range*/);
    for (int c = 0; c < 4; ++c) {
        toPointsKnob[c]->cloneCurve(ViewIdx(0), DimIdx(0), tmpToPointsCurveX[c], 0 /*offset*/, 0 /*range*/);
        toPointsKnob[c]->cloneCurve(ViewIdx(0), DimIdx(1), tmpToPointsCurveY[c], 0 /*offset*/, 0 /*range*/);
    }
    for (std::list<KnobIPtr>::iterator it = animatedKnobsChanged.begin(); it != animatedKnobsChanged.end(); ++it) {
        (*it)->unblockValueChanges();
        (*it)->evaluateValueChange(DimSpec::all(), TimeValue(refTime), ViewSetSpec::all(), eValueChangedReasonUserEdited);
    }

    endSolve();
} // TrackerNodePrivate::computeCornerParamsFromTracksEnd

void
TrackerNodePrivate::computeCornerParamsFromTracks()
{
    boost::shared_ptr<TrackerParamsProvider> thisShared = shared_from_this();
#ifndef TRACKER_GENERATE_DATA_SEQUENTIALLY
    lastSolveRequest.tWatcher.reset();
    lastSolveRequest.cpWatcher.reset( new QFutureWatcher<CornerPinData>() );
    QObject::connect( lastSolveRequest.cpWatcher.get(), SIGNAL(finished()), publicInterface, SLOT(onCornerPinSolverWatcherFinished()) );
    QObject::connect( lastSolveRequest.cpWatcher.get(), SIGNAL(progressValueChanged(int)), publicInterface, SLOT(onCornerPinSolverWatcherProgress(int)) );

    lastSolveRequest.cpWatcher->setFuture( QtConcurrent::mapped( lastSolveRequest.keyframes, boost::bind(&TrackerHelper::computeCornerPinParamsFromTracksAtTime,
                                                                                                         lastSolveRequest.refTime,
                                                                                                         _1,
                                                                                                         lastSolveRequest.jitterPeriod,
                                                                                                         lastSolveRequest.jitterAdd,
                                                                                                         lastSolveRequest.robustModel,
                                                                                                         thisShared,
                                                                                                         lastSolveRequest.allMarkers)) );
#else
    NodePtr thisNode = publicInterface->getNode();
    QList<CornerPinData> validResults;
    {
        int nKeys = (int)lastSolveRequest.keyframes.size();
        int keyIndex = 0;
        for (std::set<double>::const_iterator it = lastSolveRequest.keyframes.begin(); it != lastSolveRequest.keyframes.end(); ++it, ++keyIndex) {
            CornerPinData data = tracker->computeCornerPinParamsFromTracksAtTime(lastSolveRequest.refTime, *it, lastSolveRequest.jitterPeriod, lastSolveRequest.jitterAdd, lastSolveRequest.robustModel, thisShared, lastSolveRequest.allMarkers);
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
            toPointsKnob[i]->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
            enabledPointsKnob[i]->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
        }
    }
    KnobDoublePtr centerKnob = center.lock();

    centerKnob->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
    {
        // Revert animation on the transform
        KnobDoublePtr translationKnob = translate.lock();
        KnobDoublePtr scaleKnob = scale.lock();
        KnobDoublePtr rotationKnob = rotate.lock();

        translationKnob->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());

        scaleKnob->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());

        rotationKnob->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
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

    smoothTJitter = smoothKnob->getValue(DimIdx(0));
    smoothRJitter = smoothKnob->getValue(DimIdx(1));
    smoothSJitter = smoothKnob->getValue(DimIdx(2));

    KnobDoublePtr translationKnob = translate.lock();
    KnobDoublePtr scaleKnob = scale.lock();
    KnobDoublePtr rotationKnob = rotate.lock();
    KnobDoublePtr centerKnob = center.lock();
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
            tmpFittingErrorCurve.setOrAddKeyframe(kf);
        }
        if (!dataAtTime.hasRotationAndScale) {
            // no rotation or scale: simply extract the translation
            if (smoothTJitter > 1) {
                TranslateData avgT;
                averageDataFunctor<QList<TransformData>::const_iterator, void, TranslateData>(validResults.begin(), validResults.end(), itResults, halfTJitter, 0, &avgT, 0);
                KeyFrame kx(dataAtTime.time, avgT.p.x);
                KeyFrame ky(dataAtTime.time, avgT.p.y);
                tmpTXCurve.setOrAddKeyframe(kx);
                tmpTYCurve.setOrAddKeyframe(ky);
            } else {
                KeyFrame kx(dataAtTime.time, dataAtTime.translation.x);
                KeyFrame ky(dataAtTime.time, dataAtTime.translation.y);
                tmpTXCurve.setOrAddKeyframe(kx);
                tmpTYCurve.setOrAddKeyframe(ky);
            }
        } else {
            if (smoothRJitter > 1) {
                RotateData avgR;
                averageDataFunctor<QList<TransformData>::const_iterator, void, RotateData>(validResults.begin(), validResults.end(), itResults, halfRJitter, 0, &avgR, 0);

                KeyFrame k(dataAtTime.time, avgR.r);
                tmpRotateCurve.setOrAddKeyframe(k);

            } else {
                KeyFrame k(dataAtTime.time, dataAtTime.rotation);
                tmpRotateCurve.setOrAddKeyframe(k);
            }
            if (smoothSJitter > 1) {
                ScaleData avgR;
                averageDataFunctor<QList<TransformData>::const_iterator, void, ScaleData>(validResults.begin(), validResults.end(), itResults, halfSJitter, 0, &avgR, 0);
                KeyFrame k(dataAtTime.time, avgR.s);
                tmpScaleCurve.setOrAddKeyframe(k);

            } else {
                KeyFrame k(dataAtTime.time, dataAtTime.scale);
                tmpScaleCurve.setOrAddKeyframe(k);
            }
#pragma message WARN("BUG: actual translation must be computed from centerKnob->getValueAtTime(), tmpRotateCurve, tmpScaleCurve, and validResults[*].translation before smoothing https://github.com/NatronGitHub/Natron/issues/289")
            if (smoothTJitter > 1) {
                TranslateData avgT;
                averageDataFunctor<QList<TransformData>::const_iterator, void, TranslateData>(validResults.begin(), validResults.end(), itResults, halfTJitter, 0, &avgT, 0);
                KeyFrame kx(dataAtTime.time, avgT.p.x);
                KeyFrame ky(dataAtTime.time, avgT.p.y);
                tmpTXCurve.setOrAddKeyframe(kx);
                tmpTYCurve.setOrAddKeyframe(ky);
            } else {
                KeyFrame kx(dataAtTime.time, dataAtTime.translation.x);
                KeyFrame ky(dataAtTime.time, dataAtTime.translation.y);
                tmpTXCurve.setOrAddKeyframe(kx);
                tmpTYCurve.setOrAddKeyframe(ky);
            }
       }
    } // for all samples

    fittingWarningKnob->setSecret(!mustShowFittingWarn);
    fittingErrorKnob->cloneCurve(ViewIdx(0), DimIdx(0), tmpFittingErrorCurve, 0 /*offset*/, 0 /*range*/);
    translationKnob->cloneCurve(ViewIdx(0), DimIdx(0), tmpTXCurve, 0 /*offset*/, 0 /*range*/);
    translationKnob->cloneCurve(ViewIdx(0), DimIdx(1), tmpTYCurve, 0 /*offset*/, 0 /*range*/);
    rotationKnob->cloneCurve(ViewIdx(0), DimIdx(0), tmpRotateCurve, 0 /*offset*/, 0 /*range*/);
    scaleKnob->cloneCurve(ViewIdx(0), DimIdx(0), tmpScaleCurve, 0 /*offset*/, 0 /*range*/);
    scaleKnob->cloneCurve(ViewIdx(0), DimIdx(1), tmpScaleCurve, 0 /*offset*/, 0 /*range*/);

    for (std::list<KnobIPtr>::iterator it = animatedKnobsChanged.begin(); it != animatedKnobsChanged.end(); ++it) {
        (*it)->unblockValueChanges();
        (*it)->evaluateValueChange(DimSpec::all(), TimeValue(refTime), ViewSetSpec::all(), eValueChangedReasonUserEdited);
    }
    endSolve();
} // TrackerNodePrivate::computeTransformParamsFromTracksEnd

void
TrackerNodePrivate::computeTransformParamsFromTracks()
{
    boost::shared_ptr<TrackerParamsProvider> thisShared = shared_from_this();

#ifndef TRACKER_GENERATE_DATA_SEQUENTIALLY
    lastSolveRequest.cpWatcher.reset();
    lastSolveRequest.tWatcher.reset( new QFutureWatcher<TransformData>() );
    QObject::connect( lastSolveRequest.tWatcher.get(), SIGNAL(finished()), publicInterface, SLOT(onTransformSolverWatcherFinished()) );
    QObject::connect( lastSolveRequest.tWatcher.get(), SIGNAL(progressValueChanged(int)), publicInterface, SLOT(onTransformSolverWatcherProgress(int)) );

    lastSolveRequest.tWatcher->setFuture( QtConcurrent::mapped( lastSolveRequest.keyframes, boost::bind(&TrackerHelper::computeTransformParamsFromTracksAtTime,
                                                                                                        lastSolveRequest.refTime,
                                                                                                        _1,
                                                                                                        lastSolveRequest.jitterPeriod,
                                                                                                        lastSolveRequest.jitterAdd,
                                                                                                        lastSolveRequest.robustModel,
                                                                                                        thisShared,
                                                                                                        lastSolveRequest.allMarkers)) );
#else
    NodePtr thisNode = publicInterface->getNode();
    QList<TransformData> validResults;
    {
        int nKeys = lastSolveRequest.keyframes.size();
        int keyIndex = 0;
        for (std::set<double>::const_iterator it = lastSolveRequest.keyframes.begin(); it != lastSolveRequest.keyframes.end(); ++it, ++keyIndex) {
            TransformData data = tracker->computeTransformParamsFromTracksAtTime(lastSolveRequest.refTime, *it, lastSolveRequest.jitterPeriod, lastSolveRequest.jitterAdd, lastSolveRequest.robustModel, thisShared, lastSolveRequest.allMarkers);
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
    assert(_imp->lastSolveRequest.cpWatcher);
    _imp->computeCornerParamsFromTracksEnd( _imp->lastSolveRequest.refTime, _imp->lastSolveRequest.maxFittingError, _imp->lastSolveRequest.cpWatcher->future().results() );
}

void
TrackerNode::onTransformSolverWatcherFinished()
{
    assert(_imp->lastSolveRequest.tWatcher);
    _imp->computeTransformParamsFromTracksEnd( _imp->lastSolveRequest.refTime, _imp->lastSolveRequest.maxFittingError, _imp->lastSolveRequest.tWatcher->future().results() );
}

void
TrackerNode::onCornerPinSolverWatcherProgress(int progress)
{
    assert(_imp->lastSolveRequest.cpWatcher);
    NodePtr thisNode = getNode();
    double min = _imp->lastSolveRequest.cpWatcher->progressMinimum();
    double max = _imp->lastSolveRequest.cpWatcher->progressMaximum();
    double p = (progress - min) / (max - min);
    thisNode->getApp()->progressUpdate(thisNode, p);
}

void
TrackerNode::onTransformSolverWatcherProgress(int progress)
{
    assert(_imp->lastSolveRequest.tWatcher);
    NodePtr thisNode = getNode();
    double min = _imp->lastSolveRequest.tWatcher->progressMinimum();
    double max = _imp->lastSolveRequest.tWatcher->progressMaximum();
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
    NodePtr n = publicInterface->getNode();
    n->getApp()->progressEnd(n);
    n->getEffectInstance()->endChanges();
}


NATRON_NAMESPACE_EXIT
