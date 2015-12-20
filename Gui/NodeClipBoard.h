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

#ifndef Gui_NodeClipBoard_h
#define Gui_NodeClipBoard_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Gui/GuiFwd.h"

#include "Engine/NodeSerialization.h"
#include "Gui/NodeGuiSerialization.h"

class NodeClipBoard
{
public:
    std::list<boost::shared_ptr<NodeSerialization> > nodes;
    std::list<boost::shared_ptr<NodeGuiSerialization> > nodesUI;
    
    NodeClipBoard()
    : nodes()
    , nodesUI()
    {
    }
    
    bool isEmpty() const
    {
        return nodes.empty() && nodesUI.empty();
    }
    
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const

    {
        int nNodes = nodes.size();
        assert(nodes.size() == nodesUI.size());
        ar & boost::serialization::make_nvp("NbNodes",nNodes);
        
        std::list<boost::shared_ptr<NodeGuiSerialization> >::const_iterator itUI = nodesUI.begin();
        for (std::list<boost::shared_ptr<NodeSerialization> >::const_iterator it = nodes.begin();
             it != nodes.end(); ++it,++itUI) {
            ar & boost::serialization::make_nvp("Node",**it);
            ar & boost::serialization::make_nvp("NodeUI",**itUI);
        }
        
        
    }
        
    template<class Archive>
    void load(Archive & ar,
                  const unsigned int /*version*/)
    {
        nodes.clear();
        nodesUI.clear();
        int nNodes;
        ar & boost::serialization::make_nvp("NbNodes",nNodes);
        for (int i = 0; i < nNodes; ++i) {
            
            boost::shared_ptr<NodeSerialization> nS(new NodeSerialization);
            ar & boost::serialization::make_nvp("Node",*nS);
            nodes.push_back(nS);
            boost::shared_ptr<NodeGuiSerialization> nGui(new NodeGuiSerialization);
            ar & boost::serialization::make_nvp("NodeUI",*nGui);
            nodesUI.push_back(nGui);
        }
        
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

#endif // Gui_NodeClipBoard_h
