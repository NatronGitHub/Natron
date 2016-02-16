/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef Natron_Engine_UpdateViewerParams_h
#define Natron_Engine_UpdateViewerParams_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <boost/shared_ptr.hpp>

#include <string>
#include <list>
#include <cstddef>

#include "Global/Enums.h"
#include "Engine/BufferableObject.h"
#include "Engine/RectD.h"
#include "Engine/RectI.h"
#include "Engine/TextureRect.h"


NATRON_NAMESPACE_ENTER;

/// parameters send from the scheduler thread to updateViewer() (which runs in the main thread)
class UpdateViewerParams : public BufferableObject
{
public:
    UpdateViewerParams()
    : ramBuffer(NULL)
    , mustFreeRamBuffer(false)
    , textureIndex(0)
    , time(0)
    , textureRect()
    , srcPremult(eImagePremultiplicationOpaque)
    , bytesCount(0)
    , depth()
    , gain(1.)
    , gamma(1.)
    , offset(0.)
    , mipMapLevel(0)
    , premult(eImagePremultiplicationOpaque)
    , lut(eViewerColorSpaceSRGB)
    , layer()
    , alphaLayer()
    , alphaChannelName()
    , cachedFrame()
    , tiles()
    , rod()
    , renderAge(0)
    , isSequential(false)
    , roi()
    , updateOnlyRoi(false)
    {
    }
    
    virtual ~UpdateViewerParams() {
        if (mustFreeRamBuffer) {
            free(ramBuffer);
        }
    }
    
    virtual std::size_t sizeInRAM() const OVERRIDE FINAL
    {
        return bytesCount;
    }

    unsigned char* ramBuffer;
    bool mustFreeRamBuffer; //< set to true when !cachedFrame
    int textureIndex;
    int time;
    TextureRect textureRect;
    ImagePremultiplicationEnum srcPremult;
    size_t bytesCount;
    ImageBitDepthEnum depth;
    double gain;
    double gamma;
    double offset;
    unsigned int mipMapLevel;
    ImagePremultiplicationEnum premult;
    ViewerColorSpaceEnum lut;
    ImageComponents layer;
    ImageComponents alphaLayer;
    std::string alphaChannelName;
    
    // put a shared_ptr here, so that the cache entry is never released before the end of updateViewer()
    boost::shared_ptr<FrameEntry> cachedFrame;
    std::list<boost::shared_ptr<Image> > tiles;
    RectD rod;
    U64 renderAge;
    bool isSequential;
    RectI roi;
    bool updateOnlyRoi;
};


NATRON_NAMESPACE_EXIT;


#endif // ifndef Natron_Engine_UpdateViewerParams_h
