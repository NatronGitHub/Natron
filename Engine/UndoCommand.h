/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef UNDOCOMMAND_H
#define UNDOCOMMAND_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

class UndoCommand
{
    std::string _text;

public:

    UndoCommand( const std::string& text = std::string() )
        : _text(text)
    {}

    virtual ~UndoCommand() {}

    std::string getText() const
    {
        return _text;
    }

    // Must be called before pushUndoCommand() otherwise the text does not get updated
    void setText(const std::string& text)
    {
        _text = text;
    }

    /**
     * @brief Called to redo the action
     **/
    virtual void redo() = 0;

    /**
     * @brief Called to undo the action
     **/
    virtual void undo() = 0;

    /**
     * @brief Called to merge the other action in this action (other is the newest action)
     **/
    virtual bool mergeWith(const UndoCommandPtr& /*other*/)
    {
        return false;
    }
};

NATRON_NAMESPACE_EXIT

#endif // UNDOCOMMAND_H
