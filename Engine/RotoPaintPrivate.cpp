/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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


#include "RotoPaintPrivate.h"

#include <QtCore/QLineF>
#include <QtConcurrentRun>

#include "Global/GLIncludes.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Color.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoPaint.h"
#include "Engine/MergingEnum.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Format.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/OverlaySupport.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoUndoCommand.h"
#include "Engine/RotoLayer.h"
#include "Engine/Bezier.h"
#include "Engine/Transform.h"
#include "Engine/TrackMarker.h"
#include "Engine/TimeLine.h"
#include "Engine/AppInstance.h"
#include "Engine/ViewerInstance.h"


NATRON_NAMESPACE_ENTER


RotoPaintPrivate::RotoPaintPrivate(RotoPaint* publicInterface,
                                   RotoPaint::RotoPaintTypeEnum type)
#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    : TrackerParamsProvider(),
#else
    :
#endif
    publicInterface(publicInterface)
    , nodeType(type)
    , premultKnob()
    , enabledKnobs()
    , ui()
    , inputNodes()
    , premultNode()
    , knobsTable()
    , globalMergeNodes()
    , treeRefreshBlocked(0)
{
}



NATRON_NAMESPACE_EXIT
