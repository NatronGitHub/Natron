//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Gui/knob_callback.h"
#include "Gui/knob.h"
#include "Gui/dockableSettings.h"
#include "Core/node.h"
Knob_Callback::Knob_Callback(SettingsPanel *panel, Node *node){
    this->panel=panel;
    this->node=node;

}

Knob_Callback::~Knob_Callback(){
    for (U32 i = 0 ; i< knobs.size(); i++) {
        delete knobs[i];
    }
    knobs.clear();
}
void Knob_Callback::initNodeKnobsVector(){
    for(U32 i=0;i<knobs.size();i++){
        Knob* pair=knobs[i];
        node->addToKnobVector(pair);
    }

}
void Knob_Callback::createKnobDynamically(){
	const std::vector<Knob*>& node_knobs=node->getKnobs();
	foreach(Knob* knob,knobs){
		bool already_exists=false;
		for(U32 i=0;i<node_knobs.size();i++){
			if(node_knobs[i]==knob){
				already_exists=true;
			}
		}
		if(!already_exists){
			node->addToKnobVector(knob);
			panel->addKnobDynamically(knob);
		}
	}
}

void Knob_Callback::removeAndDeleteKnob(Knob* knob){
    node->removeKnob(knob);
    for (U32 i = 0; i< knobs.size(); i++) {
        if (knobs[i] == knob) {
            knobs.erase(knobs.begin()+i);
            break;
        }
    }
    panel->removeAndDeleteKnob(knob);
}