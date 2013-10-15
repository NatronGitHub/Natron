
//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef IMAGEINFO_H
#define IMAGEINFO_H

#include "Engine/Format.h"
#include "Engine/ChannelSet.h"

class ImageInfo{
public:
    ImageInfo(Format displayWindow,ChannelSet channels)
    : _dataWindow()
    , _displayWindow(displayWindow)
    , _channels(channels)
    {}
    ImageInfo()
    : _dataWindow()
    , _displayWindow()
    , _channels()
    {}
    
   
    
    void setDisplayWindow(const Format& format) { _displayWindow = format; }
    const Format& getDisplayWindow() const { return _displayWindow; }
    
    void setDataWindow(const Box2D& win) { _dataWindow = win; }
    const Box2D& getDataWindow() const { return _dataWindow; }
    
    void setChannels(const ChannelSet& mask) { _channels = mask; }
    const ChannelSet& getChannels() const { return _channels; }
    
    bool operator==(const ImageInfo &other){
        return _dataWindow == other._dataWindow &&
        _displayWindow == other._displayWindow &&
        _channels == other._channels;
    }
    void operator=(const ImageInfo &other){
        _dataWindow = other._dataWindow;
        _displayWindow = other._displayWindow;
        _channels = other._channels;
    }
    
private:
    
    Box2D _dataWindow;
    Format _displayWindow; // display window of the data, for the data window see x,y,range,offset parameters
    ChannelSet _channels; // all channels defined by the current Node ( that are allocated)
};

#endif // IMAGEINFO_H
