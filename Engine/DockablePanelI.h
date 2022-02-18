/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#ifndef DOCKABLEPANELI_H
#define DOCKABLEPANELI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief Interface used by the Engine Knob class to interact with the gui for dynamic parameter creation
 **/
class DockablePanelI
{
public:

    DockablePanelI() {}

    virtual ~DockablePanelI() {}

    /**
     * @brief Must scan for new knobs and rebuild the panel accordingly
     **/
    virtual void recreateUserKnobs(bool restorePageIndex) = 0;

    /**
     * @brief Same as recreateUserKnobs but for all knobs
     **/
    virtual void refreshGuiForKnobsChanges(bool restorePageIndex) = 0;

    /**
     * @brief Removes a specific knob from the gui
     **/
    virtual void deleteKnobGui(const KnobIPtr& knob) = 0;
};

NATRON_NAMESPACE_EXIT

#endif // DOCKABLEPANELI_H
