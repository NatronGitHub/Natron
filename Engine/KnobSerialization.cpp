
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
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"
KnobSerialization::KnobSerialization()
: _values()
, _dimension(0)
{

}

void KnobSerialization::initialize(const Knob* knob) {
    
    ///this function is MT-safe
    _name = knob->getName();
    _dimension = knob->getDimension();

    const std::vector<Variant>& values = knob->getValueForEachDimension_mt_safe();
    const std::vector<boost::shared_ptr<Curve> >& curves = knob->getCurves();
    std::vector<std::pair<int,boost::shared_ptr<Knob> > > masters = knob->getMasters_mt_safe();
    
    assert(values.size() == curves.size() && values.size() == masters.size() && (int)values.size() == _dimension);
    
    for (U32 i = 0; i < values.size(); ++i) {
        ValueSerialization vs;
        vs.value = values[i];
        vs.hasAnimation = curves[i]->isAnimated();
        vs.curve = Curve(*curves[i]);
        if (masters[i].second) {
            vs.hasMaster = true;
            MasterSerialization master;
            master.masterDimension = masters[i].first;
            Natron::EffectInstance* effect= dynamic_cast<Natron::EffectInstance*>(masters[i].second->getHolder());
            ///Master/slaves only works for knobs that belong to an effect.
            ///That means all the knobs used for the settings and the project should NEVER EVER
            ///be slaved.
            assert(effect);
            
            master.masterNodeName = effect->getNode()->getName_mt_safe();
            master.masterKnobName = masters[i].second->getName();
            vs.master = master;
        } else {
            vs.hasMaster = false;
        }
        _values.push_back(vs);
    }
    _extraData = knob->saveExtraData().toStdString();
}
