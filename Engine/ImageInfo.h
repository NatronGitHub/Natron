
//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_IMAGEINFO_H_
#define NATRON_ENGINE_IMAGEINFO_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
