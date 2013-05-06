#include "Core/viewerNode.h"
#include "Gui/GLViewer.h"
using Powiter_Enums::ROW_RANK;

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

void Viewer::engine(int y,int offset,int range,ChannelMask channels,Row* out,ROW_RANK rank){
	
	// single threaded: engine protected with a mutex to avoid corruption on the display

	QMutexLocker lock(_mutex); // mutex
    
    ui_context()->makeCurrent(); // make display widget context the current OpenGL context
    
    
	if(_firstTime) {  // if this is the first engine for this frame we initialize the display widget settings
//        if(!ui_context()->drawing()){
//            ui_context()->initTextures();
//        }
		ui_context()->drawing(true); // activating the drawing of our rows
		_firstTime=false;
	}
	
	ui_context()->setRow(out,rank); // we set the display widget current row to display, and ask for a redraw
	

}

