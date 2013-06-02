#ifndef VIEWERNODE_H
#define VIEWERNODE_H
#include <cmath>
#include "Gui/viewerTab.h"
//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Superviser/powiterFn.h"
#include "Core/outputnode.h"


class  Viewer: public OutputNode
{
public:
    Viewer(Viewer& ref):OutputNode(ref){}
    Viewer(Node* node,ViewerGL* v);


    virtual ~Viewer(){}
    
	ViewerGL* ui_context(){return _ui_context;}
    virtual std::string className(){return "Viewer";}
    virtual std::string description();
    void engine(int y,int offset,int range,ChannelMask channels,Row* out);
protected:
	
	
private:
    void _validate(bool for_real);
    bool _firstTime;
	ViewerGL* _ui_context;

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
