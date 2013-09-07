//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#ifndef KNOBINSTANCE_H
#define KNOBINSTANCE_H

#include <string>
#include <map>

#include "Engine/Singleton.h"

class Knob;
class KnobGui;
class NodeInstance;
class PluginID;
class Node;

typedef unsigned int Knob_Mask;
/******************************KNOB_FACTORY**************************************/


class KnobFactory : public Singleton<KnobFactory>{
    
    std::map<std::string,PluginID*> _loadedKnobs;
    
    void loadKnobPlugins();
    
    void loadBultinKnobs();
    
public:
    
    
    KnobFactory();
    
    virtual ~KnobFactory();
    
    const std::map<std::string,PluginID*>& getLoadedKnobs(){return _loadedKnobs;}
    
    /*Calls the unique instance of the KnobFactory and
     calls the appropriate pointer to function to create a knob.*/
    static Knob* createKnob(const std::string& name, Node* node, const std::string& description,int dimension, Knob_Mask flags);
    
};

/******************************KNOB_INSTANCE**************************************/
/*This class manages the interaction between
a KnobGui object and a Knob object.
*/
class KnobInstance
{
    Knob* _knob;
    KnobGui* _gui;
    NodeInstance* _node;
    
public:
    KnobInstance(Knob* knob,KnobGui* gui,NodeInstance* node):
    _knob(knob),
    _gui(gui),
    _node(node)
    {}
    
    virtual ~KnobInstance();
        
    Knob* getKnob() const {return _knob;}
    
    KnobGui* getKnobGui() const {return _gui;}
    
    NodeInstance* getNode() const {return _node;}
    
    /*Turn off the  new line for the gui's layout
     before inserting the knob in the interface.*/
    void turnOffNewLine();
    
    /*Set the spacing between items in the knob
     gui.*/
    void setSpacingBetweenItems(int spacing);
    
    /*Whether the knob should be visible on the gui.*/
    void setVisible(bool b);

    /*Whether the knob should be enabled. When disabled
     the user will not be able to interact with it and
     the value will not refresh.*/
    void setEnabled(bool b);
};

#endif // KNOBINSTANCE_H
