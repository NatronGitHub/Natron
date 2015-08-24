//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_NodeClipBoard_h_
#define _Gui_NodeClipBoard_h_


// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

class NodeSerialization;
class NodeGuiSerialization;


struct NodeClipBoard
{
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
};

#endif // _Gui_NodeClipBoard_h_
