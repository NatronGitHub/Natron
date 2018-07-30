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

#ifndef TRACKERDETECT_H
#define TRACKERDETECT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "Engine/EngineFwd.h"

#include <vector>
NATRON_NAMESPACE_ENTER

namespace TrackerDetect
{

    enum DetectionAlgorithmEnum
    {
        eDetectionAlgorithmLibMVFast,
        eDetectionAlgorithmLibMVMoravec,
        eDetectionAlgorithmLibMVHarris
    };

    struct DetectionArgs
    {
        // Defaults to eDetectionAlgorithmLibMVHarris
        DetectionAlgorithmEnum algo;

        // Margin in pixels from the image boundary.
        // No features will be detected within the margin.
        //
        // Defaults to  16, ranges between 0 and INT_MAX
        int margin;

        // Minimal distance between detected features.
        //
        // Defaults to 120, ranges between 0 and INT_MAX
        int min_distance;

        // Minimum score to add a feature. Only used by FAST detector.
        //
        // Defaults to 128
        int fast_min_trackness;

        // Maximum count to detect. Only used by MORAVEC detector.
        //
        // Defaults to 0
        int moravec_max_count;

        // Find only features similar to this pattern. Only used by MORAVEC detector.
        //
        // This is an image patch denoted in byte array with dimensions of 16px by 16px
        // used to filter features by similarity to this patch.
        //
        // Defaults to NULL
        unsigned char *moravec_pattern;
        
        // Threshold value of the Harris function to add new featrue
        // to the result.
        //
        // Defaults to 1e-5
        double harris_threshold;

        DetectionArgs();
    };

    struct Feature
    {
        // Position of the feature in pixels from top-left corner.
        // Note: Libmv detector might eventually support subpixel precision.
        double x, y;

        // An estimate of how well the feature will be tracked.
        //
        // Absolute values totally depends on particular detector type
        // used for detection. It's only guaranteed that features with
        // higher score from the same Detect() result will be tracked better.
        double score;

        // An approximate feature size in pixels.
        //
        // If the feature is approximately a 5x5 square region, then size will be 5.
        // It can be used as an initial pattern size to track the feature.
        double size;
    };


    void detectFeatures(const ImagePtr& image, const RectI& roi, const DetectionArgs& args, std::vector<Feature>* features);

} // namespace TrackerDetect

NATRON_NAMESPACE_EXIT

#endif // TRACKERDETECT_H
