
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

#include "KnobSerialization.h"

#include "Engine/Knob.h"
#include "Engine/Curve.h"
KnobSerialization::KnobSerialization()
    : _values()
    , _dimension(0)
    , _curves()
{

}

void KnobSerialization::initialize(const Knob* knob){
    _values = knob->getValueForEachDimension();
    _dimension = knob->getDimension();
    const std::map<int,boost::shared_ptr<Curve> >& curves = knob->getCurves();
    for(std::map<int,boost::shared_ptr<Curve> >::const_iterator it = curves.begin(); it!=curves.end();++it){
            if(it->second->isAnimated()){
                _curves.insert(*it);
            }
    }

    const std::map<int,boost::shared_ptr<Knob> >& masters = knob->getMasters();
    for(std::map<int,boost::shared_ptr<Knob> >::const_iterator it = masters.begin();
        it!=masters.end();++it){
        if(it->second){
            _masters.insert(std::make_pair(it->first,it->second->getDescription()));
        }
    }

    const std::multimap<int,Knob*>& slaves = knob->getSlaves();
    for(std::multimap<int,Knob*>::const_iterator it = slaves.begin();it!=slaves.end();++it){
        if(it->second){
            _slaves.insert(std::make_pair(it->first,it->second->getDescription()));
        }
    }

}
