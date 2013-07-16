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

 

 



#ifndef KNOB_CALLBACK_H
#define KNOB_CALLBACK_H
#include <vector>

class Knob;
class SettingsPanel;
class Node;

class Knob_Callback
{
    SettingsPanel* getSettingsPanel(){return panel;}
public:
    Knob_Callback(SettingsPanel* panel,Node* node);
    ~Knob_Callback();

    
    void addKnob(Knob* knob){knobs.push_back(knob);}
    
    void initNodeKnobsVector();
    
	Node* getNode(){return node;}
    
	void createKnobDynamically();
    
    void removeAndDeleteKnob(Knob* knob);

private:
    Node* node;
    std::vector<Knob*> knobs;
    SettingsPanel* panel;
};

#endif // KNOB_CALLBACK_H
