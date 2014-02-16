
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
    : _hasAnimation(false)
    , _values()
    , _dimension(0)
    , _curves()
{

}

void KnobSerialization::initialize(const Knob* knob) {
    _hasAnimation = knob->hasAnimation();
    _label = knob->getDescription();
    _values = knob->getValueForEachDimension();
    _dimension = knob->getDimension();
    const std::vector<boost::shared_ptr<Curve> >& curves = knob->getCurves();
    for(U32 i = 0; i < curves.size();++i){
            if(curves[i]){
                _curves.push_back(curves[i]);
            }
    }

    const std::vector<boost::shared_ptr<Knob> >& masters = knob->getMasters();
    for(U32 i = 0; i < masters.size();++i){
        if(masters[i]){
            _masters.push_back(masters[i]->getDescription());
        }
    }

    _extraData = knob->saveExtraData().toStdString();
}
