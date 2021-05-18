/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <cassert>
#include <stdexcept>

#include "Engine/AppInstance.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/RotoLayer.h"
#include "Engine/NodeGroupSerialization.h"
#include "Engine/RotoContext.h"

NATRON_NAMESPACE_ENTER

NodeSerialization::NodeSerialization(const NodePtr & n,
                                     bool serializeInputs)
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
    , _hasTrackerContext(false)
    , _node()
    , _pythonModuleVersion(0)
{
    if (n) {
        _node = n;

        ///All this code is MT-safe

        _knobsValues.clear();
        _inputs.clear();

        if ( n->isOpenFXNode() ) {
            OfxEffectInstance* effect = dynamic_cast<OfxEffectInstance*>( n->getEffectInstance().get() );
            assert(effect);
            if (effect) {
                effect->syncPrivateData_other_thread();
            }
        }

        std::vector<KnobIPtr>  knobs = n->getEffectInstance()->getKnobs_mt_safe();
        std::list<KnobIPtr> userPages;
        for (U32 i  = 0; i < knobs.size(); ++i) {
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knobs[i].get() );
            KnobPage* isPage = dynamic_cast<KnobPage*>( knobs[i].get() );

            if (isPage) {
                _pagesIndexes.push_back( knobs[i]->getName() );
                if ( knobs[i]->isUserKnob() ) {
                    userPages.push_back(knobs[i]);
                }
            }
            if (knobs[i]->isUserKnob()) {
                continue;
            }
            if (isGroup || isPage) {
                continue;
            }
            bool hasExpr = false;
            for (int d = 0; d < knobs[i]->getDimension(); ++d) {
                if (!knobs[i]->getExpression(d).empty()) {
                    hasExpr = true;
                    break;
                }
                std::pair<int, KnobIPtr> master = knobs[i]->getMaster(d);
                if (master.second) {
                    hasExpr = true;
                    break;
                }
            }
            // Always serialize nodes that have an expression or a link.
            // See https://github.com/NatronGitHub/Natron/issues/585
            // Already fixed in RB-3.0 by https://github.com/NatronGitHub/Natron/commit/7ef38849fe
            if (!hasExpr) {
                if ( !knobs[i]->getIsPersistent() ) {
                    continue;
                }

                if ( !knobs[i]->hasModificationsForSerialization() ) {
                    continue;
                }
            }

            KnobSerializationPtr newKnobSer = boost::make_shared<KnobSerialization>(knobs[i]);
            _knobsValues.push_back(newKnobSer);
        }

        _nbKnobs = (int)_knobsValues.size();

        for (std::list<KnobIPtr>::const_iterator it = userPages.begin(); it != userPages.end(); ++it) {
            GroupKnobSerializationPtr s = boost::make_shared<GroupKnobSerialization>(*it);
            _userPages.push_back(s);
        }

        _knobsAge = n->getKnobsAge();

        _nodeLabel = n->getLabel_mt_safe();

        _nodeScriptName = n->getScriptName_mt_safe();

        _cacheID = n->getCacheID();

        _pluginID = n->getPluginID();

        if ( !n->hasPyPlugBeenEdited() ) {
            _pythonModule = n->getPluginPythonModule();
            _pythonModuleVersion = n->getMajorVersion();
        }

        _pluginMajorVersion = n->getMajorVersion();

        _pluginMinorVersion = n->getMinorVersion();

        if (serializeInputs) {
            n->getInputNames(_inputs);
        }

        NodePtr masterNode = n->getMasterNode();
        if (masterNode) {
            _masterNodeName = masterNode->getFullyQualifiedName();
        }

        RotoContextPtr roto = n->getRotoContext();
        if ( roto && !roto->isEmpty() ) {
            _hasRotoContext = true;
            roto->save(&_rotoContext);
        } else {
            _hasRotoContext = false;
        }

        TrackerContextPtr tracker = n->getTrackerContext();
        if (tracker) {
            _hasTrackerContext = true;
            tracker->save(&_trackerContext);
        } else {
            _hasTrackerContext = false;
        }


        NodeGroup* isGrp = n->isEffectGroup();
        if (isGrp) {
            NodesList nodes;
            isGrp->getActiveNodes(&nodes);

            _children.clear();

            for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                if ( (*it)->isPartOfProject() ) {
                    NodeSerializationPtr state = boost::make_shared<NodeSerialization>(*it);
                    _children.push_back(state);
                }
            }
        }

        _multiInstanceParentName = n->getParentMultiInstanceName();

        NodesList childrenMultiInstance;
        _node->getChildrenMultiInstance(&childrenMultiInstance);
        if ( !childrenMultiInstance.empty() ) {
            assert(!isGrp);
            for (NodesList::iterator it = childrenMultiInstance.begin(); it != childrenMultiInstance.end(); ++it) {
                assert( (*it)->getParentMultiInstance() );
                if ( (*it)->isActivated() ) {
                    NodeSerializationPtr state = boost::make_shared<NodeSerialization>(*it);
                    _children.push_back(state);
                }
            }
        }

        n->getUserCreatedComponents(&_userComponents);

        _isNull = false;
    }
}

NATRON_NAMESPACE_EXIT
