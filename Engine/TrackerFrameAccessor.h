/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#endif

GCC_DIAG_OFF(unused-function)
GCC_DIAG_OFF(unused-parameter)
#include <libmv/image/array_nd.h>
#include <libmv/autotrack/frame_accessor.h>
GCC_DIAG_ON(unused-function)
GCC_DIAG_ON(unused-parameter)
#include "Engine/EngineFwd.h"
#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_ENTER


class MvFloatImage
: public libmv::Array3D<float>
{
public:

    MvFloatImage()
    : libmv::Array3D<float>()
    {
    }

    MvFloatImage(int height,
                 int width)
    : libmv::Array3D<float>(height, width)
    {
    }

    MvFloatImage(float* data,
                 int height,
                 int width)
    : libmv::Array3D<float>(data, height, width)
    {
    }

    virtual ~MvFloatImage()
    {
    }
};


struct TrackerFrameAccessorPrivate;

class TrackerFrameAccessor
    : public mv::FrameAccessor
    , public boost::enable_shared_from_this<TrackerFrameAccessor>
{
    struct MakeSharedEnabler;

protected:
    // used by boost::make_shared<TrackerFrameAccessor>
    TrackerFrameAccessor(const NodePtr& sourceImageProvider,
                         const NodePtr& maskImageProvider,
                         const ImagePlaneDesc& maskImagePlane,
                         int maskPlaneIndex,
                         bool enabledChannels[3],
                         int formatHeight);

public:
    static TrackerFrameAccessorPtr create(const NodePtr& sourceImageProvider,
                                          const NodePtr& maskImageProvider,
                                          const ImagePlaneDesc& maskImagePlane,
                                          int maskPlaneIndex,
                                          bool enabledChannels[3],
                                          int formatHeight);

    virtual ~TrackerFrameAccessor();


    void getEnabledChannels(bool* r, bool* g, bool* b) const;


   
    void GetImageInternal(std::list<mv::FrameAccessor::GetImageArgs>& imageRequests);


    // Get a possibly-filtered version of a frame of a video. Downscale will
    // cause the input image to get downscaled by 2^downscale for pyramid access.
    // Region is always in original-image coordinates, and describes the
    // requested area. The transform describes an (optional) transform to apply
    // to the image before it is returned.
    //
    // When done with an image, you must call ReleaseImage with the returned key.
    virtual void GetImage(std::list<mv::FrameAccessor::GetImageArgs>& imageRequests) OVERRIDE FINAL;

    // Releases an image from the frame accessor. Non-caching implementations may
    // free the image immediately; others may hold onto the image.
    virtual void ReleaseImage(Key) OVERRIDE FINAL;



    virtual bool GetClipDimensions(int clip, int* width, int* height) OVERRIDE FINAL;
    virtual int NumClips() OVERRIDE FINAL;
    virtual int NumFrames(int clip) OVERRIDE FINAL;
    static double invertYCoordinate(double yIn, double formatHeight);
    static void convertLibMVRegionToRectI(const mv::Region& region, int formatHeight, RectI* roi);

    static ActionRetCodeEnum natronImageToLibMvFloatImage(bool enabledChannels[3],
                                                          const Image& source,
                                                          const RectI& roi,
                                                          bool takeDstFromAlpha,
                                                          MvFloatImage& mvImg);
    

private:

    boost::scoped_ptr<TrackerFrameAccessorPrivate> _imp;
};

NATRON_NAMESPACE_EXIT
#endif // TRACKERFRAMEACCESSOR_H
