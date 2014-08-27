
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

#include "Engine/Format.h"
#include "Engine/ChannelSet.h"

class ImageInfo
{
public:
    ImageInfo(Format displayWindow,
              Natron::ChannelSet channels)
        : _rod()
          , _displayWindow(displayWindow)
          , _channels(channels)
    {
    }

    ImageInfo()
        : _rod()
          , _displayWindow()
          , _channels()
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

    void setChannels(const Natron::ChannelSet & mask)
    {
        _channels = mask;
    }

    const Natron::ChannelSet & getChannels() const
    {
        return _channels;
    }

    bool operator==(const ImageInfo &other)
    {
        return _rod == other._rod &&
               _displayWindow == other._displayWindow &&
               _channels == other._channels;
    }

    void operator=(const ImageInfo &other )
    {
        _rod = other._rod;
        _displayWindow = other._displayWindow;
        _channels = other._channels;
    }

private:

    RectD _rod;  // the image RoD in canonical coordinates (not the same as the OFX::Image rod, which is in pixel coordinates)
    Format _displayWindow; // display window of the data
    Natron::ChannelSet _channels; // all channels defined by the current Node ( that are allocated)
};

#endif // NATRON_ENGINE_IMAGEINFO_H_
