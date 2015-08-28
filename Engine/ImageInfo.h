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

#ifndef NATRON_ENGINE_IMAGEINFO_H_
#define NATRON_ENGINE_IMAGEINFO_H_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/Format.h"

class ImageInfo
{
public:
    ImageInfo(Format displayWindow)
        : _rod()
          , _displayWindow(displayWindow)
    {
    }

    ImageInfo()
        : _rod()
          , _displayWindow()
    {
    }

    void setDisplayWindow(const Format & format)
    {
        _displayWindow = format;
    }

    const Format & getDisplayWindow() const
    {
        return _displayWindow;
    }

    void setRoD(const RectD & win)
    {
        _rod = win;
    }

    const RectD & getRoD() const
    {
        return _rod;
    }

    bool operator==(const ImageInfo &other)
    {
        return (_rod == other._rod &&
                _displayWindow == other._displayWindow);
    }

    void operator=(const ImageInfo &other )
    {
        _rod = other._rod;
        _displayWindow = other._displayWindow;
    }

private:

    RectD _rod;  // the image RoD in canonical coordinates (not the same as the OFX::Image rod, which is in pixel coordinates)
    Format _displayWindow; // display window of the data
};

#endif // NATRON_ENGINE_IMAGEINFO_H_
