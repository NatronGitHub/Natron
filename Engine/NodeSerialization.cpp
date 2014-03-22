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
#include "Engine/OfxEffectInstance.h"
void NodeSerialization::initialize(Natron::Node* n){
    
    
    ///All this code is MT-safe
    
    _knobsValues.clear();
    _inputs.clear();
    
    if (n->isOpenFXNode()) {
        dynamic_cast<OfxEffectInstance*>(n->getLiveInstance())->syncPrivateData_other_thread();
    }
    
    const std::vector< boost::shared_ptr<Knob> >& knobs = n->getKnobs();
    
    for (U32 i  = 0; i < knobs.size(); ++i) {
        if(knobs[i]->getIsPersistant()){
            boost::shared_ptr<KnobSerialization> newKnobSer(new KnobSerialization);
            knobs[i]->save(newKnobSer.get());
            _knobsValues.push_back(newKnobSer);
        }
    }
    
    _knobsAge = n->getKnobsAge();
    
    _pluginLabel = n->getName_mt_safe();
    
    _pluginID = n->pluginID();
    
    _pluginMajorVersion = n->majorVersion();
    
    _pluginMinorVersion = n->minorVersion();
    
    _inputs = n->getInputNames();
    
    Natron::Node* masterNode = n->getMasterNode();
    if (masterNode) {
        _masterNodeName = masterNode->getName_mt_safe();
    }
    
    _isNull = false;
}
