//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#include "NodeSerialization.h"

#include "Engine/Knob.h"
#include "Engine/Node.h"

void NodeSerialization::initialize(const Natron::Node* n){
    
    _knobsValues.clear();
    _inputs.clear();
    
    const std::vector< boost::shared_ptr<Knob> >& knobs = n->getKnobs();
    
    for (U32 i  = 0; i < knobs.size(); ++i) {
        if(knobs[i]->isPersistent()){
            boost::shared_ptr<KnobSerialization> newKnobSer(new KnobSerialization);
            knobs[i]->save(newKnobSer.get());
            _knobsValues.push_back(newKnobSer);
        }
    }
    
    _pluginLabel = n->getName();
    
    _pluginID = n->pluginID();
    
    _pluginMajorVersion = n->majorVersion();
    
    _pluginMinorVersion = n->minorVersion();
    
    const Natron::Node::InputMap& inputs = n->getInputs();
    for(Natron::Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second){
            _inputs.insert(std::make_pair(it->first, it->second->getName()));
        }
    }
    
    _isNull = false;
}
