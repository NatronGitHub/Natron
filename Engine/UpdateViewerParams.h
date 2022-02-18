/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Natron_Engine_UpdateViewerParams_h
#define Natron_Engine_UpdateViewerParams_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <boost/shared_ptr.hpp>

#include <string>
#include <list>
#include <cstddef>

#include "Global/Enums.h"

#include "Engine/BufferableObject.h"
#include "Engine/RectD.h"
#include "Engine/RectI.h"
#include "Engine/TextureRect.h"


NATRON_NAMESPACE_ENTER

/// parameters send from the scheduler thread to updateViewer() (which runs in the main thread)
class UpdateViewerParams
    : public BufferableObject
{
public:

    struct CachedTile
    {
        TextureRect rect;
        RectI rectRounded;
        // This is a pointer to the data held in the cache. If this is set the ramBuffer is no more than cachedFrame->data()
        // use a shared_ptr here, so that the cache entry is never released before the end of updateViewer()
        FrameEntryPtr cachedData;
        bool isCached;
        unsigned char* ramBuffer; // a pointer to the RAM buffer held either by the cached frame or allocated by malloc()
        std::size_t bytesCount; // number of bytes in the texture


        CachedTile()
            : rect(), rectRounded(), cachedData(), isCached(false), ramBuffer(0), bytesCount(0) {}
    };

    UpdateViewerParams()
        : mustFreeRamBuffer(false)
        , textureIndex(0)
        , time(0)
        , view(0)
        , srcPremult(eImagePremultiplicationOpaque)
        , depth()
        , gain(1.)
        , gamma(1.)
        , offset(0.)
        , mipMapLevel(0)
        , lut(eViewerColorSpaceSRGB)
        , layer()
        , alphaLayer()
        , alphaChannelName()
        , tiles()
        , tileSize(0)
        , nbCachedTile(0)
        , colorImage()
        , rod()
        , pixelAspectRatio(1.)
        , abortInfo()
        , isSequential(false)
        , isPartialRect(false)
        , isViewerPaused(false)
        , recenterViewport(false)
        , viewportCenter()
    {
    }

    virtual ~UpdateViewerParams()
    {
        if (mustFreeRamBuffer) {
            assert(tiles.size() == 1);
            free(tiles.front().ramBuffer);
        }
    }

    virtual std::size_t sizeInRAM() const OVERRIDE FINAL
    {
        std::size_t ret = 0;

        for (std::list<CachedTile>::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            ret += it->bytesCount;
        }

        return ret;
    }

    bool mustFreeRamBuffer; // set to true when !cachedFrame, in this case we have only 1 tile
    int textureIndex; // The texture index (for input A or B)
    int time; // the frame
    ViewIdx view; // the view
    ImagePremultiplicationEnum srcPremult; // the image premult
    ImageBitDepthEnum depth; // bitdepth of the texture
    double gain; // viewer gain
    double gamma; // viewer gamma
    double offset; // viewer offset
    unsigned int mipMapLevel; // viewer mipmaplevel
    ViewerColorSpaceEnum lut; // the viewer colorspace lut
    ImagePlaneDesc layer; // the image layer
    ImagePlaneDesc alphaLayer; // the alpha layer
    std::string alphaChannelName; // the alpha channel name
    std::list<CachedTile> tiles;
    int tileSize;
    int nbCachedTile;

    // The image which was used to make the texture
    ImagePtr colorImage;

    // The RoD of the src image
    RectD rod;

    // The RoI of the viewer, rounded to the tile size
    RectI roi,roiNotRoundedToTileSize;

    // The Par of the input image
    double pixelAspectRatio;

    // Abort data held so that we know if this frame is aborted
    AbortableRenderInfoPtr abortInfo;

    // Is this during playback ?
    bool isSequential;

    // Is this a marker overlay used when tracking ?
    bool isPartialRect;

    // Is the viewer paused ?
    bool isViewerPaused;

    // Should we center the viewer on the viewportCenter
    bool recenterViewport;
    Point viewportCenter;
};


NATRON_NAMESPACE_EXIT


#endif // ifndef Natron_Engine_UpdateViewerParams_h
