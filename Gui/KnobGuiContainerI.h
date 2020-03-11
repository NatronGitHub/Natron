/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef KNOBGUICONTAINERI_H
#define KNOBGUICONTAINERI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER

class KnobGuiContainerI
{
public:

    KnobGuiContainerI()
        : _containerWidget(0)
    {}

    KnobGuiContainerI(QWidget* w)
        : _containerWidget(w)
    {}

    virtual ~KnobGuiContainerI() {}

    virtual Gui* getGui() const = 0;
    virtual const QUndoCommand* getLastUndoCommand() const = 0;
    virtual void pushUndoCommand(QUndoCommand* cmd) = 0;
    virtual KnobGuiPtr getKnobGui(const KnobIPtr& knob) const = 0;
    virtual int getItemsSpacingOnSameLine() const = 0;
    virtual void refreshTabWidgetMaxHeight() {}

    QWidget* getContainerWidget() const
    {
        return _containerWidget;
    }

    virtual NodeGuiPtr getNodeGui() const = 0;

    /**
     * @brief Refresh whether a page should be made visible or not. A page is considered to be visible
     * when at least one of its children (recursively) is not secret.
     **/
    virtual void refreshPageVisibility(const KnobPagePtr& /*page*/) {}

    virtual QWidget* createKnobHorizontalFieldContainer(QWidget* parent) const = 0;

protected:

    void setContainerWidget(QWidget* widget)
    {
        _containerWidget = widget;
    }

    QWidget* _containerWidget;
};

NATRON_NAMESPACE_EXIT

#endif // KNOBGUICONTAINERI_H
