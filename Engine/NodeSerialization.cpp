//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "NodeSerialization.h"

#include "Engine/AppInstance.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/RotoSerialization.h"
#include "Engine/RotoContext.h"

NodeSerialization::NodeSerialization(const boost::shared_ptr<Natron::Node> & n)
    : _isNull(true)
      , _hasRotoContext(false)
      , _node()
      , _app(NULL)
{
    if (n) {
        _node = n;
        _app = _node->getApp();

        ///All this code is MT-safe

        _knobsValues.clear();
        _inputs.clear();

        if ( n->isOpenFXNode() ) {
            dynamic_cast<OfxEffectInstance*>( n->getLiveInstance() )->syncPrivateData_other_thread();
        }

        const std::vector< boost::shared_ptr<KnobI> > & knobs = n->getKnobs();

        for (U32 i  = 0; i < knobs.size(); ++i) {
            Group_Knob* isGroup = dynamic_cast<Group_Knob*>( knobs[i].get() );
            Page_Knob* isPage = dynamic_cast<Page_Knob*>( knobs[i].get() );
            Button_Knob* isButton = dynamic_cast<Button_Knob*>( knobs[i].get() );
            if (knobs[i]->getIsPersistant() && !isGroup && !isPage && !isButton) {
                boost::shared_ptr<KnobSerialization> newKnobSer( new KnobSerialization(knobs[i]) );
                _knobsValues.push_back(newKnobSer);
            }
        }
        _nbKnobs = (int)_knobsValues.size();

        _knobsAge = n->getKnobsAge();

        _pluginLabel = n->getName_mt_safe();

        _pluginID = n->getPluginID();

        _pluginMajorVersion = n->getMajorVersion();

        _pluginMinorVersion = n->getMinorVersion();

        n->getInputNames(_inputs);

        boost::shared_ptr<Natron::Node> masterNode = n->getMasterNode();
        if (masterNode) {
            _masterNodeName = masterNode->getName_mt_safe();
        }

        boost::shared_ptr<RotoContext> roto = n->getRotoContext();
        if ( roto && !roto->isEmpty() ) {
            _hasRotoContext = true;
            roto->save(&_rotoContext);
        } else {
            _hasRotoContext = false;
        }

        _multiInstanceParentName = n->getParentMultiInstanceName();

        _isNull = false;
    }
}

