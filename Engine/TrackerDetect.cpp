/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "TrackerDetect.h"

#include "Engine/RectI.h"
#include "Engine/Image.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/TrackerHelperPrivate.h"
#include "Engine/TrackerFrameAccessor.h"

GCC_DIAG_OFF(unused-function)
GCC_DIAG_OFF(unused-parameter)
#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 408
GCC_DIAG_OFF(maybe-uninitialized)
#endif
#include <libmv/simple_pipeline/detect.h>
GCC_DIAG_ON(unused-function)
GCC_DIAG_ON(unused-parameter)

NATRON_NAMESPACE_ENTER

TrackerDetect::DetectionArgs::DetectionArgs()
: algo(TrackerDetect::eDetectionAlgorithmLibMVHarris)
, margin(16)
, min_distance(120)
, fast_min_trackness(128)
, moravec_max_count(0)
, moravec_pattern(0)
, harris_threshold(1e-5)
{
    
}

static void natronDetectionArgsToLibMvOptions(const TrackerDetect::DetectionArgs& inArgs, libmv::DetectOptions* outArgs)
{
    switch (inArgs.algo) {
        case TrackerDetect::eDetectionAlgorithmLibMVHarris:
            outArgs->type = libmv::DetectOptions::HARRIS;
            break;
        case TrackerDetect::eDetectionAlgorithmLibMVFast:
            outArgs->type = libmv::DetectOptions::FAST;
            break;
        case TrackerDetect::eDetectionAlgorithmLibMVMoravec:
            outArgs->type = libmv::DetectOptions::MORAVEC;
            break;
    }
    outArgs->margin = inArgs.margin;
    outArgs->min_distance = inArgs.min_distance;
    outArgs->fast_min_trackness = inArgs.fast_min_trackness;
    outArgs->moravec_pattern = inArgs.moravec_pattern;
    outArgs->moravec_max_count = inArgs.moravec_max_count;
    outArgs->harris_threshold = inArgs.harris_threshold;
}

static void libmvFeatureToNatronFeature(const libmv::Feature& mvFeature, TrackerDetect::Feature* feature)
{
    /* In Libmv integer coordinate points to pixel center, in blender
     * it's not. Need to add 0.5px offset to center.
     */
    feature->x = mvFeature.x + 0.5;
    feature->y = mvFeature.y + 0.5;
    feature->score = mvFeature.score;
    feature->size = mvFeature.size;
}

void TrackerDetect::detectFeatures(const ImagePtr& image, const RectI& roi, const DetectionArgs& args, std::vector<Feature>* features)
{
    if (!image) {
        return;
    }
    bool enabledChannels[3] = {true,true,true};

    MvFloatImage libmvImage;
    ActionRetCodeEnum stat = TrackerFrameAccessor::natronImageToLibMvFloatImage(enabledChannels, *image, roi, false /*dstFromAlpha*/, libmvImage);
    if (isFailureRetCode(stat)) {
        return;
    }

    libmv::DetectOptions options;
    natronDetectionArgsToLibMvOptions(args, &options);

    libmv::vector<libmv::Feature> detected_features;
    libmv::Detect(libmvImage, options, &detected_features);

    features->resize(detected_features.size());
    for (int i = 0; i < detected_features.size(); ++i) {
        const libmv::Feature& mvFeature = detected_features[i];
        libmvFeatureToNatronFeature(mvFeature,&(*features)[i]);
    }
}

NATRON_NAMESPACE_EXIT
