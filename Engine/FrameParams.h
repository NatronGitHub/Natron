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

#ifndef FRAMEPARAMS_H
#define FRAMEPARAMS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/RectI.h"
#include "Engine/NonKeyParams.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

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
                int texW,
                int texH,
                const ImagePtr& originalImage)
        : NonKeyParams(1, bitDepth != 0 ? texW * texH * 16 : texW * texH * 4)
        , _image(originalImage)
        , _rod(rod)
    {
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
    boost::weak_ptr<Image>  _image;

    // The RoD of the image used to make this frame entry
    RectI _rod;
};

NATRON_NAMESPACE_EXIT;

#endif // FRAMEPARAMS_H

