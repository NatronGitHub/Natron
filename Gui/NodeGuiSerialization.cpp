/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "NodeGuiSerialization.h"

#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QColor>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/NodeGui.h"

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"

NATRON_NAMESPACE_ENTER;

void
NodeGuiSerialization::initialize(const NodeGui*  n)
{
    ////All this code is MT-safe
    NodePtr node = n->getNode();
    
    
    _nodeName = node->getFullyQualifiedName();
    QPointF pos = n->getPos_mt_safe();
    _posX = pos.x();
    _posY = pos.y();
    n->getSize(&_width, &_height);
    _previewEnabled = node->isPreviewEnabled();
    QColor color = n->getCurrentColor();
    _r = color.redF();
    _g = color.greenF();
    _b = color.blueF();
    _selected = n->isSelected();
    
    _hasOverlayColor = n->getOverlayColor(&_overlayR, &_overlayG, &_overlayB);
    
    NodeGroup* isGrp = node->isEffectGroup();
    if (isGrp) {
        NodesList nodes;
        isGrp->getActiveNodes(&nodes);
        
        _children.clear();
        
        for (NodesList::iterator it = nodes.begin(); it != nodes.end() ; ++it) {
            boost::shared_ptr<NodeGuiI> gui_i = (*it)->getNodeGui();
            if (!gui_i) {
                continue;
            }
            NodeGui* childGui = dynamic_cast<NodeGui*>(gui_i.get());
            if (!childGui) {
                continue;
            }
            boost::shared_ptr<NodeGuiSerialization> state(new NodeGuiSerialization());
            state->initialize(childGui);
            _children.push_back(state);
        }
        
    }
    
}

NATRON_NAMESPACE_EXIT;
