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

#ifndef POWITER_ENGINE_VIEWERNODE_H_
#define POWITER_ENGINE_VIEWERNODE_H_

#include <string>
#include <QtCore/QFutureWatcher>

#include "Global/Macros.h"
#include "Engine/Node.h"


class ViewerCache;
class ViewerInfos;
class TabWidget;
class ViewerTab;
class FrameEntry;
class ViewerNode: public OutputNode
{
    
    ViewerInfos* _viewerInfos;
	ViewerTab* _uiContext;
    ViewerCache* _viewerCache;
    int _pboIndex;
    
public:
    
        
    ViewerNode(ViewerCache* cache);
    
    virtual ~ViewerNode();
    
    virtual int maximumInputs() const OVERRIDE { return 1; }
    
    virtual int minimumInputs() const OVERRIDE { return 1; }
    
    virtual bool cacheData() const OVERRIDE { return false; }
    
    /*Add a new viewer tab to the GUI*/
    void initializeViewerTab(TabWidget* where);
    
    /*tell the ViewerGL to use the current viewerInfos*/
    void makeCurrentViewer();
    
    ViewerInfos* getViewerInfos(){return _viewerInfos;}
    
    ViewerTab* getUiContext(){return _uiContext;}
    
    void setUiContext(ViewerTab* ptr){_uiContext = ptr;}
    
    virtual std::string className() OVERRIDE { return "Viewer"; }
    
    virtual std::string description() OVERRIDE;
    
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
    
    virtual bool _validate(bool forReal);
    
    
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


#endif // POWITER_ENGINE_VIEWERNODE_H_
