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

#include "NodeGraphPrivate.h"
#include "NodeGraph.h"

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"

#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/NodeClipBoard.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGuiSerialization.h"


using namespace Natron;



void
NodeGraphPrivate::pasteNodesInternal(const NodeClipBoard & clipboard,const QPointF& scenePos,
                                     bool useUndoCommand,
                                     std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > *newNodes)
{
    if ( !clipboard.isEmpty() ) {

        double xmax = INT_MIN;
        double xmin = INT_MAX;
        double ymin = INT_MAX;
        double ymax = INT_MIN;

        for (std::list<boost::shared_ptr<NodeGuiSerialization> >::const_iterator it = clipboard.nodesUI.begin();
             it != clipboard.nodesUI.end(); ++it) {
            double x = (*it)->getX();
            double y = (*it)->getY();
            double w,h;
            (*it)->getSize(&w,&h);
            if ((x + w) > xmax) {
                xmax = x;
            }
            if (x < xmin) {
                xmin = x;
            }
            if ((y + h)> ymax) {
                ymax = y;
            }
            if (y < ymin) {
                ymin = y;
            }
        }


        QPointF offset(scenePos.x() - ((xmin + xmax) / 2.), scenePos.y() - ((ymin + ymax) / 2.));

        assert( clipboard.nodes.size() == clipboard.nodesUI.size() );

        std::list<NodeGuiPtr> newNodeList;
        std::list<boost::shared_ptr<NodeSerialization> > internalNodesClipBoard = clipboard.nodes;
        std::list<boost::shared_ptr<NodeSerialization> >::iterator itOther = internalNodesClipBoard.begin();
        for (std::list<boost::shared_ptr<NodeGuiSerialization> >::const_iterator it = clipboard.nodesUI.begin();
             it != clipboard.nodesUI.end(); ++it, ++itOther) {
            boost::shared_ptr<NodeGui> node = pasteNode( **itOther,**it,offset,group.lock(),std::string(), false);
            newNodes->push_back(std::make_pair((*itOther)->getNodeScriptName(),node));
            newNodeList.push_back(node);
            ///The script-name of the copy node is different than the one of the original one, update all input connections in the serialization
            for (std::list<boost::shared_ptr<NodeSerialization> >::iterator it2 = internalNodesClipBoard.begin(); it2!=internalNodesClipBoard.end(); ++it2) {
                (*it2)->switchInput((*itOther)->getNodeScriptName(), node->getNode()->getScriptName());
            }
        }
        assert( clipboard.nodes.size() == newNodes->size() );

        ///Now that all nodes have been duplicated, try to restore nodes connections
        restoreConnections(clipboard.nodes, *newNodes);

        if (useUndoCommand) {
            _publicInterface->pushUndoCommand( new AddMultipleNodesCommand(_publicInterface,newNodeList) );
        }
    }
} // pasteNodesInternal

boost::shared_ptr<NodeGui>
NodeGraphPrivate::pasteNode(const NodeSerialization & internalSerialization,
                            const NodeGuiSerialization & guiSerialization,
                            const QPointF & offset,
                            const boost::shared_ptr<NodeCollection>& grp,
                            const std::string& parentName,
                            bool clone)
{
    boost::shared_ptr<Natron::Node> n = _publicInterface->getGui()->getApp()->loadNode( LoadNodeArgs(internalSerialization.getPluginID().c_str(),
                                                                               parentName,
                                                                               internalSerialization.getPluginMajorVersion(),
                                                                               internalSerialization.getPluginMinorVersion(),
                                                                               &internalSerialization,
                                                                               true,
                                                                               grp) );

    assert(n);
    boost::shared_ptr<NodeGuiI> gui_i = n->getNodeGui();
    boost::shared_ptr<NodeGui> gui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
    assert(gui);

    std::string name;
    if (internalSerialization.getNode()->getGroup() != group.lock() || grp != group.lock()) {
        name = internalSerialization.getNodeScriptName();
        n->setScriptName( name);
        n->setLabel(internalSerialization.getNodeLabel());
    } else {
        int no = 1;
        std::stringstream ss;
        ss << internalSerialization.getNodeScriptName();
        ss << '_';
        ss << no;
        name = ss.str();
        while ( grp->checkIfNodeNameExists( name,n.get() ) ) {
            ++no;
            ss.str( std::string() );
            ss.clear();
            ss << internalSerialization.getNodeScriptName();
            ss << '_';
            ss << no;
            name = ss.str();
        }
        n->setScriptName( name);
    }

    const std::string & masterNodeName = internalSerialization.getMasterNodeName();
    if ( !masterNodeName.empty() ) {

        boost::shared_ptr<Natron::Node> masterNode = _publicInterface->getGui()->getApp()->getProject()->getNodeByName(masterNodeName);

        ///the node could not exist any longer if the user deleted it in the meantime
        if ( masterNode && masterNode->isActivated() ) {
            n->getLiveInstance()->slaveAllKnobs( masterNode->getLiveInstance(), true );
        }
    }
    std::list<boost::shared_ptr<Natron::Node> > allNodes;
    _publicInterface->getGui()->getApp()->getProject()->getActiveNodes(&allNodes);
    n->restoreKnobsLinks(internalSerialization,allNodes);

    //We don't want the clone to have the same hash as the original
    n->incrementKnobsAge();

    gui->copyFrom(guiSerialization);
    QPointF newPos = gui->getPos_mt_safe() + offset;
    gui->setPosition( newPos.x(), newPos.y() );
    gui->forceComputePreview( _publicInterface->getGui()->getApp()->getProject()->currentFrame() );

    if (clone) {
        DotGui* isDot = dynamic_cast<DotGui*>( gui.get() );
        ///Dots cannot be cloned, just copy them
        if (!isDot) {
            n->getLiveInstance()->slaveAllKnobs( internalSerialization.getNode()->getLiveInstance(), false );
        }
    }

    ///Recurse if this is a group or multi-instance
    NodePtr serializedNode = internalSerialization.getNode();
    assert(serializedNode);
    boost::shared_ptr<NodeGroup> isGrp =
    boost::dynamic_pointer_cast<NodeGroup>(n->getLiveInstance()->shared_from_this());

    const std::list<boost::shared_ptr<NodeSerialization> >& nodes = internalSerialization.getNodesCollection();

    if (!nodes.empty()) {

        std::string parentName;
        boost::shared_ptr<NodeCollection> collection;
        if (isGrp) {
            collection = isGrp;
        } else {
            assert(n->isMultiInstance());
            collection = n->getGroup();
            parentName = n->getScriptName_mt_safe();
        }
        std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > newNodes;
        for (std::list<boost::shared_ptr<NodeSerialization> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            NodePtr child = (*it)->getNode();
            assert(child);
            boost::shared_ptr<NodeGuiI> child_gui_i = child->getNodeGui();
            NodeGui* child_gui = dynamic_cast<NodeGui*>(child_gui_i.get());
            assert(child_gui);
            if (child_gui) {
                NodeGuiSerialization gS;
                gS.initialize(child_gui);
                NodeGuiPtr newChild = pasteNode(**it, gS, QPointF(0,0),collection,parentName,clone);
                if (newChild) {
                    newNodes.push_back(std::make_pair((*it)->getNodeScriptName(),newChild));
                }
            }
        }
        restoreConnections(nodes, newNodes);
    }
    return gui;
}

void
NodeGraphPrivate::restoreConnections(const std::list<boost::shared_ptr<NodeSerialization> > & serializations,
                                     const std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > & newNodes)
{
    ///For all nodes restore its connections
    std::list<boost::shared_ptr<NodeSerialization> >::const_iterator itSer = serializations.begin();
    assert(serializations.size() == newNodes.size());
    for (std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > >::const_iterator it = newNodes.begin();
         it != newNodes.end(); ++it, ++itSer) {
        const std::map<std::string,std::string> & inputNames = (*itSer)->getInputs();

        ///Restore each input
        for (std::map<std::string,std::string>::const_iterator it2 = inputNames.begin(); it2 != inputNames.end(); ++it2) {
            if ( it2->second.empty() ) {
                continue;
            }

            int index = it->second->getNode()->getInputNumberFromLabel(it2->first);
            if (index == -1) {
                qDebug() << "Could not find an input named " << it2->first.c_str();
                continue;
            }


            ///find a node  containing the same name. It should not match exactly because there's already
            /// the "-copy" that was added to its name
            for (std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > >::const_iterator it3 = newNodes.begin();
                 it3 != newNodes.end(); ++it3) {
                if ( it3->second->getNode()->getScriptName() == it2->second ) {
                    NodeCollection::connectNodes( index,it3->second->getNode(),it->second->getNode().get() );
                    break;
                }
            }
        }
    }
}

void
NodeGraphPrivate::toggleSelectedNodesEnabled()
{
    std::list<boost::shared_ptr<NodeGui> > toProcess;

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _selection.begin(); it != _selection.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            toProcess.push_back(*it);
        }
    }
    ///if some nodes are disabled , enable them before

    if ( toProcess.size() == _selection.size() ) {
        _publicInterface->pushUndoCommand( new EnableNodesCommand(_selection) );
    } else if (toProcess.size() > 0) {
        _publicInterface->pushUndoCommand( new EnableNodesCommand(toProcess) );
    } else {
        _publicInterface->pushUndoCommand( new DisableNodesCommand(_selection) );
    }
}
