//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef PowiterOsX_readerInfo_h
#define PowiterOsX_readerInfo_h

#include "Core/displayFormat.h"
#include "Core/channels.h"
#include <QtCore/QString>

class ReaderInfo{
    
    
public:
    ReaderInfo(DisplayFormat dispW,
               IntegerBox dataW,
               QString currentFrameName,
               ChannelMask channels= Mask_RGB,
               int Ydirection = -1,
               bool rgb = true,
               int currentFrame = 0,
               int firstFrame=0,
               int lastFrame=0):
    _channels(channels),_Ydirection(Ydirection),_rgbMode(rgb),
    _currentFrame(currentFrame),_lastFrame(lastFrame),_firstFrame(firstFrame),
    _displayWindow(dispW.x(), dispW.y(), dispW.right(), dispW.top(),""),
    _dataWindow(dataW.x(), dataW.y(), dataW.right(), dataW.top()),_currentFrameName(currentFrameName)
    {
        
        _displayWindow.name(dispW.name());
        _displayWindow.pixel_aspect(dispW.pixel_aspect());
    }
    ReaderInfo():_dataWindow(),_displayWindow(),_channels(Mask_RGB),
    _Ydirection(-1),_rgbMode(true),_currentFrame(0),_lastFrame(0),_firstFrame(0){
    }
    ReaderInfo(const ReaderInfo& other):_dataWindow(other.dataWindow()),
    _displayWindow(other.displayWindow()),_Ydirection(other.Ydirection()),
    _rgbMode(other.rgbMode()),_channels(other.channels()),_currentFrameName(other.currentFrameName()),
    _currentFrame(other.currentFrame()),_lastFrame(other.lastFrame()),_firstFrame(other.firstFrame())
    {
    }
    
    
    ~ReaderInfo(){}
    
    void copy(ReaderInfo* other){
        _dataWindow = other->dataWindow();
        _displayWindow = other->displayWindow();
        _Ydirection = other->Ydirection();
        _rgbMode = other->rgbMode();
        _channels = other->channels();
        _currentFrame = other->currentFrame();
        _lastFrame = other->lastFrame();
        _currentFrameName = other->currentFrameName();
        _firstFrame = other->firstFrame();
    }
    
    const ChannelMask channels() const {return _channels;}
    const DisplayFormat displayWindow() const {return _displayWindow;}
    const IntegerBox dataWindow() const {return _dataWindow;}
    const int Ydirection() const {return _Ydirection;}
    const bool rgbMode() const {return _rgbMode;}
    const int firstFrame() const {return _firstFrame;}
    const int lastFrame() const {return _lastFrame;}
    const int currentFrame() const {return _currentFrame;}
    const QString currentFrameName() const {return _currentFrameName;}
    void currentFrame(int c){_currentFrame=c;}
    void currentFrameName(QString name){_currentFrameName = name;}
    void lastFrame(int l){_lastFrame=l;}
    void firstFrame(int f){_firstFrame=f;}
    void setDisplayWindowName(const char* n){_displayWindow.name(n);}
    void dataWindow(int x,int y,int r,int t){_dataWindow.set(x,y, r, t);}
    void displayWindow(int x,int y,int r,int t){_displayWindow.set(x, y, r, t);}
    void Ydirection(int y){_Ydirection=y;}
    void channels(ChannelMask channels){_channels=channels;}
    void rgbMode(bool b){_rgbMode=b;}
    void pixelAspect(double p){_displayWindow.pixel_aspect(p);}
    
    bool operator==(const ReaderInfo& other){
        return _channels == other.channels() &&
        _displayWindow == other.displayWindow() &&
        _dataWindow == other.dataWindow() &&
        _Ydirection == other.Ydirection() &&
        _rgbMode == other.rgbMode() &&
        _firstFrame == other.firstFrame() &&
        _lastFrame == other.lastFrame();
    }
    
private:
    ChannelMask _channels;  // channels contained in file
    DisplayFormat _displayWindow; // data window contained in file
    IntegerBox _dataWindow;
    int _Ydirection;
    bool _rgbMode;
    int _firstFrame;
    int _lastFrame;
    int _currentFrame;
    QString _currentFrameName;
    
};

#endif
