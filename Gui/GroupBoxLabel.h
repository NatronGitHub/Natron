/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef NATRON_GUI_GROUPBOXLABEL_H
#define NATRON_GUI_GROUPBOXLABEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Gui/Label.h"
#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER

class GroupBoxLabel
    : public Label
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    GroupBoxLabel(QWidget *parent = 0);

    virtual ~GroupBoxLabel() OVERRIDE
    {
    }

    bool isChecked() const
    {
        return _checked;
    }

public Q_SLOTS:

    void setChecked(bool);

private:
    virtual void mousePressEvent(QMouseEvent* /*e*/) OVERRIDE FINAL
    {
        if ( isEnabled() ) {
            Q_EMIT checked(!_checked);
        }
    }

Q_SIGNALS:
    void checked(bool);

private:
    bool _checked;
};

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_GUI_GROUPBOXLABEL_H
