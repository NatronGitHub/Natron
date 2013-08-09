
#ifndef VIEWERNODE_H
#define VIEWERNODE_H

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

 

 



#include <cmath>
#include "Superviser/powiterFn.h"
#include "Core/node.h"
#include <QtCore/QFuture>
#include "Gui/texture.h"
#include <QtCore/QFutureWatcher>
class ViewerCache;
class ViewerInfos;
class TabWidget;
class ViewerTab;
class FrameEntry;
class Viewer: public Node
{
    
    ViewerInfos* _viewerInfos;
	ViewerTab* _uiContext;
    ViewerCache* _viewerCache;
    int _pboIndex;
    QFutureWatcher<void> *_cacheWatcher;
    
public:
    
        
    Viewer(ViewerCache* cache);
    
    virtual ~Viewer();
    
    virtual bool isOutputNode(){return true;}
    
    virtual int maximumInputs(){return 1;}
    
    virtual int minimumInputs(){return 1;}
    
    virtual bool cacheData(){return false;}
    
    /*Add a new viewer tab to the GUI*/
    void initializeViewerTab(TabWidget* where);
    
    /*tell the ViewerGL to use the current viewerInfos*/
    void makeCurrentViewer();
    
    ViewerInfos* getViewerInfos(){return _viewerInfos;}
    
    ViewerTab* getUiContext(){return _uiContext;}
    
    virtual const std::string className(){return "Viewer";}
    
    virtual const std::string description();
    
    void engine(int y,int offset,int range,ChannelSet channels,Row* out);
    
    /*Convenience functions. They query the values from the timeline.*/
    int firstFrame() const;
    
    int lastFrame() const;
    
    int currentFrame() const;
    
    FrameEntry* get(U64 key);
        
    /*This function MUST be called in the main thread.*/
    void cachedFrameEngine(FrameEntry* frame);
    
protected:
    
    virtual ChannelSet supportedComponents(){return Powiter::Mask_All;}
	
private:
    
    void retrieveCachedFrame(const char* cachedFrame,void* dst,size_t dataSize);
    virtual void _validate(bool forReal);
    
    
};

/*#ifdef __cplusplus
 extern "C" {
 #endif
 #ifdef _WIN32
 VIEWER_EXPORT Viewer* BuildViewer(Node *node){return new Viewer(node);}
 #elif defined(unix) || defined(__unix__) || defined(__unix)
 Viewer* BuildViewer(Node *node){return new Viewer(node);}
 #endif
 #ifdef __cplusplus
 }
 #endif*/


#endif // VIEWERNODE_H
