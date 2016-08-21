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

NATRON_NAMESPACE_ENTER;

NodeSerialization::NodeSerialization(const NodePtr & n,
                                     bool serializeInputs)
    : _version(NODE_SERIALIZATION_CURRENT_VERSION)
    , _hasBeenSerialized(false)
    , _knobsValues()
    , _knobsAge(0)
    , _groupFullyQualifiedScriptName()
    , _nodeLabel()
    , _nodeScriptName()
    , _cacheID()
    , _pluginID()
    , _pluginMajorVersion(-1)
    , _pluginMinorVersion(-1)
    , _masterNodeFullyQualifiedScriptName()
    , _inputs()
    , _oldInputs()
    , _rotoContext()
    , _trackerContext()
    , _multiInstanceParentName()
    , _userPages()
    , _pagesIndexes()
    , _children()
    , _pythonModule()
    , _pythonModuleVersion(0)
    , _userComponents()
    , _nodePositionCoords()
    , _nodeSize()
    , _nodeColor()
    , _overlayColor()
    , _nodeIsSelected(false)
    , _viewerUIKnobsOrder()
{
    _nodePositionCoords[0] = _nodePositionCoords[1] = INT_MIN;
    _nodeSize[0] = _nodeSize[1] = -1;
    _nodeColor[0] = _nodeColor[1] = _nodeColor[2] = -1;
    _overlayColor[0] = _overlayColor[1] = _overlayColor[2] = -1;

    if (n) {
        n->toSerialization(this);
        if (!serializeInputs) {
            _inputs.clear();
        }
    }
}



NATRON_NAMESPACE_EXIT;
