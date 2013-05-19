//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Core/viewerNode.h"
#include "Gui/GLViewer.h"

Viewer::Viewer(Node* node,ViewerGL* v):OutputNode(node)
{
	this->_ui_context=v;
    _firstTime=true;
}

void Viewer::_validate(bool first_time){
    first_time=true;
    _firstTime=true;
    Node::_validate(first_time);
}

const char* Viewer::description(){
    return "OutputNode";
}

void Viewer::engine(int y,int offset,int range,ChannelMask channels,Row* out){
	
	if(_firstTime) {  // if this is the first engine for this frame we initialize the display widget settings
		ui_context()->drawing(true); // activating the drawing of our rows
		_firstTime=false;
	}
	ui_context()->drawRow(out); // we set the display widget current row to display, and ask for a redraw
}

