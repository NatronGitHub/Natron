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

#ifndef PANELWIDGET_H
#define PANELWIDGET_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/ScriptObject.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class PanelWidget
    : public ScriptObject
{
    QWidget* _thisWidget;
    Gui* _gui;

public:

    PanelWidget(QWidget* thisWidget,
                Gui* gui);

    Gui* getGui() const;

    void notifyGuiClosingPublic();

    virtual ~PanelWidget();

    QWidget* getWidget() const
    {
        return _thisWidget;
    }

    TabWidget* getParentPane() const;

    void removeClickFocus();

    void takeClickFocus();

    bool isClickFocusPanel() const;


    /*
     * @brief To be called when a keypress event is not accepted
     */
    void handleUnCaughtKeyPressEvent(QKeyEvent* e);

    /*
     * @brief To be called when a keyrelease event is not accepted
     */
    void handleUnCaughtKeyUpEvent(QKeyEvent* e);

    virtual void pushUndoCommand(QUndoCommand* command);

    /*
     * @brief Called whenever this panel is made the current tab in the parent tab widget
     */
    virtual void onPanelMadeCurrent() {}

protected:

    virtual QUndoStack* getUndoStack() const { return 0; }

    virtual void notifyGuiClosing() {}

    /**
     * @brief To be called in the enterEvent handler of all derived classes.
     **/
    bool enterEventBase();

    /**
     * @brief To be called in the leaveEvent handler of all derived classes.
     **/
    void leaveEventBase();
};

NATRON_NAMESPACE_EXIT


#endif // PANELWIDGET_H
