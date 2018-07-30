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

#ifndef NATRON_GUI_STYLEDKNOBWIDGETBASE_H
#define NATRON_GUI_STYLEDKNOBWIDGETBASE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <QObject>

#include "Global/Macros.h"
#include "Gui/GuiFwd.h"

#define DEFINE_KNOB_GUI_STYLE_PROPERTIES \
Q_PROPERTY(int animation READ getAnimation WRITE setAnimation) \
Q_PROPERTY(bool multipleSelection READ getIsSelectedMultipleTimes WRITE setIsSelectedMultipleTimes) \
Q_PROPERTY(bool selected READ getIsSelected WRITE setIsSelected) \
Q_PROPERTY(bool modified READ getIsModified WRITE setIsModified)

NATRON_NAMESPACE_ENTER

class StyledKnobWidgetBase
{
public:

    
    StyledKnobWidgetBase();

    virtual ~StyledKnobWidgetBase();

    virtual void setAnimation(int animation);

    int getAnimation() const;

    virtual void setIsSelectedMultipleTimes(bool b);

    bool getIsSelectedMultipleTimes() const;

    virtual void setIsSelected(bool b);

    bool getIsSelected() const;

    virtual void setIsModified(bool b);

    bool getIsModified() const;

protected:

    virtual void refreshStylesheet() = 0;

    // The animation level of the corresponding knob.
    // The value is a direct cast of AnimationLevelEnum
    int animation;

    // Useful for nodes using a KnobItemsTable: this indicates for a master
    // knob if interacting with the widget will affect a single or multiple items
    // in the table, depending on the selection.
    bool multipleSelection;

    // Useful for knobs inside a KnobItemsTable: indicates whether the row of the knob
    // is selected or not.
    bool selected;

    // Indicates whether the knob is considered (modified: it was modified since default value)
    // or not (the current value equals the default value).
    bool modified;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_STYLEDKNOBWIDGETBASE_H
