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

#ifndef Engine_ChoiceOption_h
#define Engine_ChoiceOption_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct ChoiceOption
{
    // Must necessarily be set, this uniquely identifies the option
    std::string id;

    // Can be set to something different than the ID. This is what will be displayed in the option in the GUI
    // If empty this will be set to the ID.
    std::string label;

    // Can be set to display a tooltip when the user hovers the entry in the choice menu
    std::string tooltip;
    
    ChoiceOption()
    : id()
    , label()
    , tooltip()
    {
        
    }

    explicit ChoiceOption(const std::string& id_)
    : id(id_)
    , label()
    , tooltip()
    {

    }

    ChoiceOption(const std::string& id_, const std::string& label_, const std::string& tooltip_)
    : id(id_)
    , label(label_)
    , tooltip(tooltip_)
    {
        
    }

    bool operator==(const ChoiceOption& other) const
    {
        return id == other.id;
    }

    bool operator!=(const ChoiceOption& other) const
    {
        return id != other.id;
    }
    
};

NATRON_NAMESPACE_EXIT

#endif // Engine_ChoiceOption_h
