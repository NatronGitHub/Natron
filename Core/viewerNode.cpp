//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Core/viewerNode.h"
#include "Gui/GLViewer.h"
#include "Gui/mainGui.h"
#include "Superviser/controler.h"
#include "Gui/viewerTab.h"
#include "Gui/GLViewer.h"
Viewer::Viewer(Node* node):OutputNode(node),_viewerInfos(0)
{
    ctrlPTR->getGui()->addViewerTab();
    _uiContext = currentViewer;
}
Viewer::~Viewer(){
    ctrlPTR->getGui()->removeViewerTab(_uiContext);
    if(_viewerInfos)
        delete _viewerInfos;
}

void Viewer::_validate(bool forReal){
    if(_parents.size()==1){
		copy_info(_parents[0],forReal);
	}
    set_output_channels(Mask_All);
    
    if(_viewerInfos){
        delete _viewerInfos;
    }
    makeCurrentViewer();
    
}

std::string Viewer::description(){
    return "OutputNode";
}

void Viewer::engine(int y,int offset,int range,ChannelMask channels,Row* out){
    _uiContext->viewer->drawRow(out); // we  ask for the GLViewer to draw this row
}

void Viewer::makeCurrentViewer(){
    _viewerInfos = new ViewerInfos;
    dynamic_cast<Node::Info&>(*_viewerInfos) = dynamic_cast<const Node::Info&>(*_info);
    currentViewer->viewer->setCurrentViewerInfos(_viewerInfos,false,false);
}