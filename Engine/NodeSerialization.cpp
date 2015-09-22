/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeSerialization.h"

#include "Engine/AppInstance.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/RotoLayer.h"
#include "Engine/NodeGroupSerialization.h"
#include "Engine/RotoContext.h"

NodeSerialization::NodeSerialization(const boost::shared_ptr<Natron::Node> & n,bool serializeInputs)
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
    , _pythonModuleVersion(0)
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
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knobs[i].get() );
            KnobPage* isPage = dynamic_cast<KnobPage*>( knobs[i].get() );
            KnobButton* isButton = dynamic_cast<KnobButton*>( knobs[i].get() );
            //KnobChoice* isChoice = dynamic_cast<KnobChoice*>( knobs[i].get() );
            
            if (isPage && knobs[i]->isUserKnob()) {
                userPages.push_back(knobs[i]);
                continue;
            }
            
            if (!knobs[i]->isUserKnob() &&
                knobs[i]->getIsPersistant() &&
                !isGroup && !isPage && !isButton
                && knobs[i]->hasModificationsForSerialization()) {
                
                ///For choice do a deepclone because we need entries
                //bool doCopyKnobs = isChoice ? true : copyKnobs;
                
                boost::shared_ptr<KnobSerialization> newKnobSer( new KnobSerialization(knobs[i]) );
                _knobsValues.push_back(newKnobSer);
            }
        }
        
        _nbKnobs = (int)_knobsValues.size();
        
        for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = userPages.begin(); it != userPages.end(); ++it) {
            boost::shared_ptr<GroupKnobSerialization> s(new GroupKnobSerialization(*it));
            _userPages.push_back(s);
        }

        _knobsAge = n->getKnobsAge();

        _nodeLabel = n->getLabel_mt_safe();
        
        _nodeScriptName = n->getScriptName_mt_safe();
        
        _cacheID = n->getCacheID();

        _pluginID = n->getPluginID();
        
        _pythonModule = n->getPluginPythonModule();
        
        _pythonModuleVersion = n->getPluginPythonModuleVersion();

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
                assert((*it)->getParentMultiInstance());
                if ((*it)->isActivated()) {
                    boost::shared_ptr<NodeSerialization> state(new NodeSerialization(*it));
                    _children.push_back(state);
                }
            }
        }
        
        n->getUserComponents(&_userComponents);
        
        _isNull = false;
    }
}
