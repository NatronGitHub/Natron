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

#ifndef VIEWERTOOLBUTTON_H
#define VIEWERTOOLBUTTON_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Gui/GuiFwd.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QToolButton>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

NATRON_NAMESPACE_ENTER

class ViewerToolButton
    : public QToolButton
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    // properties
    Q_PROPERTY(bool isSelected READ getIsSelected WRITE setIsSelected)

public:

    ViewerToolButton(QWidget* parent);

    virtual ~ViewerToolButton();


    void handleSelection();

    bool getIsSelected() const;
    void setIsSelected(bool s);

public Q_SLOTS:

    void handleLongPress();

private:

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    bool isSelected;
    bool wasMouseReleased;
};

NATRON_NAMESPACE_EXIT

#endif // VIEWERTOOLBUTTON_H
