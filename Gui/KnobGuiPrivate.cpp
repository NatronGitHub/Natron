/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#include "KnobGuiPrivate.h"
#include <stdexcept>

#include "Gui/NodeViewerContext.h"

NATRON_NAMESPACE_ENTER


KnobGuiPrivate::KnobGuiPrivate(KnobGui* publicInterface, const KnobIPtr& knob, KnobGui::KnobLayoutTypeEnum layoutType, KnobGuiContainerI* container)
: _publicInterface(publicInterface)
, knob(knob)
, singleDimensionEnabled(false)
, singleDimension(DimIdx(0))
, isOnNewLine(false)
, widgetCreated(false)
, container(container)
, copyRightClickMenu( new MenuWithToolTips( container->getContainerWidget() ) )
, prevKnob()
, views()
, labelContainer(NULL)
, viewUnfoldArrow(NULL)
, viewsContainer(NULL)
, viewsContainerLayout(NULL)
, mainContainer(NULL)
, mainLayout(NULL)
, endOfLineSpacer(0)
, spacerAdded(false)
, mustAddSpacerByDefault(true)
, firstKnobOnLine()
, otherKnobsInMainLayout()
, descriptionLabel(NULL)
, warningIndicator(NULL)
, customInteract(NULL)
, guiRemoved(false)
, tabGroup(0)
, refreshGuiRequests(0)
, refreshModifStateRequests(0)
, refreshDimensionVisibilityRequests()
, layoutType(layoutType)
{
}



NATRON_NAMESPACE_EXIT

