/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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
#include "StyledKnobWidgetBase.h"

NATRON_NAMESPACE_ENTER

StyledKnobWidgetBase::StyledKnobWidgetBase()
: animation(0)
, multipleSelection(false)
, selected(false)
, modified(false)
{

}

StyledKnobWidgetBase::~StyledKnobWidgetBase()
{

}


void
StyledKnobWidgetBase::setAnimation(int anim)
{
    animation = anim;
    refreshStylesheet();
}

int
StyledKnobWidgetBase::getAnimation() const
{
    return animation;
}

void
StyledKnobWidgetBase::setIsSelectedMultipleTimes(bool b)
{
    multipleSelection = b;
    refreshStylesheet();
}

bool
StyledKnobWidgetBase::getIsSelectedMultipleTimes() const
{
    return multipleSelection;
}

void
StyledKnobWidgetBase::setIsSelected(bool b)
{
    selected = b;
    refreshStylesheet();
}

bool
StyledKnobWidgetBase::getIsSelected() const
{
    return selected;
}

void
StyledKnobWidgetBase::setIsModified(bool b)
{
    modified = b;
    refreshStylesheet();
}

bool
StyledKnobWidgetBase::getIsModified() const
{
    return modified;
}

NATRON_NAMESPACE_EXIT
