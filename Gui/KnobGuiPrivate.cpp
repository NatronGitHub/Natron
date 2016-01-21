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

#include "Gui/KnobGuiPrivate.h"

NATRON_NAMESPACE_ENTER;


KnobGuiPrivate::KnobGuiPrivate(DockablePanel* container)
: triggerNewLine(true)
, spacingBetweenItems(0)
, widgetCreated(false)
, container(container)
, animationMenu(NULL)
, animationButton(NULL)
, copyRightClickMenu( new MenuWithToolTips(container) )
, fieldLayout(NULL)
, knobsOnSameLine()
, containerLayout(NULL)
, field(NULL)
, descriptionLabel(NULL)
, isOnNewLine(false)
, customInteract(NULL)
, guiCurves()
, guiRemoved(false)
{
    //copyRightClickMenu->setFont( QFont(appFont,appFontSize) );
}

void KnobGuiPrivate::removeFromKnobsOnSameLineVector(const boost::shared_ptr<KnobI>& knob)
{
    for (std::vector< boost::weak_ptr< KnobI > >::iterator it = knobsOnSameLine.begin(); it != knobsOnSameLine.end(); ++it) {
        if (it->lock() == knob) {
            knobsOnSameLine.erase(it);
            break;
        }
    }
}

NATRON_NAMESPACE_EXIT;

