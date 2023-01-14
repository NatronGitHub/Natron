/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef FRAMEPARAMS_H
#define FRAMEPARAMS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/RectI.h"
#include "Engine/NonKeyParams.h"
#include "Engine/EngineFwd.h"
#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER

class FrameParams
    : public NonKeyParams
{
public:

    FrameParams()
        : NonKeyParams()
        , _image()
        , _rod()
    {
    }

    FrameParams(const FrameParams & other)
        : NonKeyParams(other)
        , _image(other._image)
        , _rod(other._rod)
    {
    }

    FrameParams(const RectI & rod,
                int bitDepth,
                const RectI& bounds,
                const ImagePtr& originalImage)
        : NonKeyParams()
        , _image(originalImage)
        , _rod(rod)
    {
        CacheEntryStorageInfo& info = getStorageInfo();

        info.mode = eStorageModeDisk;
        info.numComponents = 4; // always RGBA
        info.dataTypeSize = getSizeOfForBitDepth( (ImageBitDepthEnum)bitDepth );
        info.bounds = bounds;
        info.textureTarget = GL_TEXTURE_2D;
    }

    virtual ~FrameParams()
    {
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    bool operator==(const FrameParams & other) const
    {
        return NonKeyParams::operator==(other) && _rod == other._rod;
    }

    bool operator!=(const FrameParams & other) const
    {
        return !(*this == other);
    }

    ImagePtr getInternalImage() const
    {
        return _image.lock();
    }

    void setInternalImage(const ImagePtr& image)
    {
        _image = image;
    }

private:

    // The image used to make this frame entry
    ImageWPtr  _image;

    // The RoD of the image used to make this frame entry
    RectI _rod;
};

NATRON_NAMESPACE_EXIT

#endif // FRAMEPARAMS_H

