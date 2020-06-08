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

#ifndef TRACKERFRAMEACCESSOR_H
#define TRACKERFRAMEACCESSOR_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

#include <libmv/autotrack/frame_accessor.h>


NATRON_NAMESPACE_ENTER

struct TrackerFrameAccessorPrivate;
class TrackerFrameAccessor
    : public mv::FrameAccessor
{
public:

    TrackerFrameAccessor(const TrackerContext* context,
                         bool enabledChannels[3],
                         int formatHeight);

    virtual ~TrackerFrameAccessor();


    void getEnabledChannels(bool* r, bool* g, bool* b) const;


    // Get a possibly-filtered version of a frame of a video. Downscale will
    // cause the input image to get downscaled by 2^downscale for pyramid access.
    // Region is always in original-image coordinates, and describes the
    // requested area. The transform describes an (optional) transform to apply
    // to the image before it is returned.
    //
    // When done with an image, you must call ReleaseImage with the returned key.
    virtual mv::FrameAccessor::Key GetImage(int clip,
                                            int frame,
                                            mv::FrameAccessor::InputMode input_mode,
                                            int downscale,               // Downscale by 2^downscale.
                                            const mv::Region* region,        // Get full image if NULL.
                                            const mv::FrameAccessor::Transform* transform,  // May be NULL.
                                            mv::FloatImage** destination) OVERRIDE FINAL;

    // Releases an image from the frame accessor. Non-caching implementations may
    // free the image immediately; others may hold onto the image.
    virtual void ReleaseImage(Key) OVERRIDE FINAL;

    // Get mask image for the given track.
    //
    // Implementation of this method should sample mask associated with the track
    // within given region to the given destination.
    //
    // Result is supposed to be a single channel image.
    //
    // If region is NULL, it it assumed to be full-frame.
    virtual mv::FrameAccessor::Key GetMaskForTrack(int clip,
                                                   int frame,
                                                   int track,
                                                   const mv::Region* region,
                                                   mv::FloatImage* destination) OVERRIDE FINAL;

    // Release a specified mask.
    //
    // Non-caching implementation may free used memory immediately.
    virtual void ReleaseMask(mv::FrameAccessor::Key key) OVERRIDE FINAL;

    virtual bool GetClipDimensions(int clip, int* width, int* height) OVERRIDE FINAL;
    virtual int NumClips() OVERRIDE FINAL;
    virtual int NumFrames(int clip) OVERRIDE FINAL;
    static double invertYCoordinate(double yIn, double formatHeight);
    static void convertLibMVRegionToRectI(const mv::Region& region, int formatHeight, RectI* roi);

private:

    boost::scoped_ptr<TrackerFrameAccessorPrivate> _imp;
};

NATRON_NAMESPACE_EXIT
#endif // TRACKERFRAMEACCESSOR_H
