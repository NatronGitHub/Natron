#ifndef VIEWERNODE_H
#define VIEWERNODE_H

//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <cmath>
#include "Superviser/powiterFn.h"
#include "Core/outputnode.h"

/*class deriving Node::Info and used by the current viewer.
 This is essentially the same but it is left this way instead of
 a typedef if we'd like to pass more infos in the future.*/
class ViewerInfos : public Node::Info{
public:
    ViewerInfos():Node::Info(){}
    virtual ~ViewerInfos(){}
};

class ViewerTab;
class  Viewer: public OutputNode
{
public:
    
    
    Viewer(Viewer& ref):OutputNode(ref){}
    
    Viewer(Node* node);

    virtual ~Viewer();
    
    /*tell the ViewerGL to use the current viewerInfos*/
    void makeCurrentViewer();
    ViewerInfos* getViewerInfos(){return _viewerInfos;}
    
    ViewerTab* getUiContext(){return _uiContext;}
    
    virtual std::string className(){return "Viewer";}
    
    virtual std::string description();
    void engine(int y,int offset,int range,ChannelMask channels,Row* out);
protected:
	
	
private:
    virtual void _validate(bool forReal);
    ViewerInfos* _viewerInfos;
	ViewerTab* _uiContext;
    

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
