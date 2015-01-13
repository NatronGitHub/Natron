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

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "NodeSerialization.h"

#include "Engine/AppInstance.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/RotoSerialization.h"
#include "Engine/NodeGroupSerialization.h"
#include "Engine/RotoContext.h"

NodeSerialization::NodeSerialization(const boost::shared_ptr<Natron::Node> & n,bool serializeInputs,bool copyKnobs)
    : _isNull(true)
    , _nbKnobs(0)
    , _knobsValues()
    , _knobsAge(0)
    , _nodeLabel()
    , _nodeScriptName()
    , _pluginID()
    , _pluginMajorVersion(-1)
    , _pluginMinorVersion(-1)
    , _hasRotoContext(false)
    , _node()
{
    if (n) {
        _node = n;

        ///All this code is MT-safe

        _knobsValues.clear();
        _inputs.clear();

        if ( n->isOpenFXNode() ) {
            dynamic_cast<OfxEffectInstance*>( n->getLiveInstance() )->syncPrivateData_other_thread();
        }

        std::vector< boost::shared_ptr<KnobI> >  knobs = n->getLiveInstance()->getKnobs_mt_safe();

        std::list<boost::shared_ptr<KnobI> > userPages;
        for (U32 i  = 0; i < knobs.size(); ++i) {
            Group_Knob* isGroup = dynamic_cast<Group_Knob*>( knobs[i].get() );
            Page_Knob* isPage = dynamic_cast<Page_Knob*>( knobs[i].get() );
            Button_Knob* isButton = dynamic_cast<Button_Knob*>( knobs[i].get() );
            Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>( knobs[i].get() );
            
            if (isPage && knobs[i]->isUserKnob()) {
                userPages.push_back(knobs[i]);
                continue;
            }
            
            if (!knobs[i]->isUserKnob() && knobs[i]->getIsPersistant() && !isGroup && !isPage && !isButton) {
                
                ///For choice do a deepclone because we need entries
                bool doCopyKnobs = isChoice ? true : copyKnobs;
                
                boost::shared_ptr<KnobSerialization> newKnobSer( new KnobSerialization(knobs[i],doCopyKnobs) );
                _knobsValues.push_back(newKnobSer);
            }
        }
        
        _nbKnobs = (int)_knobsValues.size();
        
        for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = userPages.begin(); it!=userPages.end(); ++it) {
            boost::shared_ptr<GroupKnobSerialization> s(new GroupKnobSerialization(*it));
            _userPages.push_back(s);
        }

        _knobsAge = n->getKnobsAge();

        _nodeLabel = n->getLabel_mt_safe();
        
        _nodeScriptName = n->getScriptName_mt_safe();

        _pluginID = n->getPluginID();

        _pluginMajorVersion = n->getMajorVersion();

        _pluginMinorVersion = n->getMinorVersion();
        
        if (serializeInputs) {
            n->getInputNames(_inputs);
        }

        boost::shared_ptr<Natron::Node> masterNode = n->getMasterNode();
        if (masterNode) {
            _masterNodeName = masterNode->getFullyQualifiedName();
        }

        boost::shared_ptr<RotoContext> roto = n->getRotoContext();
        if ( roto && !roto->isEmpty() ) {
            _hasRotoContext = true;
            roto->save(&_rotoContext);
        } else {
            _hasRotoContext = false;
        }

        
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>(n->getLiveInstance());
        if (isGrp) {
            NodeList nodes;
            isGrp->getActiveNodes(&nodes);
            
            _children.clear();
            
            for (NodeList::iterator it = nodes.begin(); it != nodes.end() ; ++it) {
                boost::shared_ptr<NodeSerialization> state(new NodeSerialization(*it));
                _children.push_back(state);
            }

        }

         _multiInstanceParentName = n->getParentMultiInstanceName();
        
        std::list<NodePtr> childrenMultiInstance;
        _node->getChildrenMultiInstance(&childrenMultiInstance);
        if (!childrenMultiInstance.empty()) {
            assert(!isGrp);
            for (NodeList::iterator it = childrenMultiInstance.begin(); it != childrenMultiInstance.end() ; ++it) {
                boost::shared_ptr<NodeSerialization> state(new NodeSerialization(*it));
                _children.push_back(state);
            }
        }
        
        _isNull = false;
    }
}
