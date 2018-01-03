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
#include <QIcon>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif


#include "Engine/ScriptObject.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class PanelWidget
    : public ScriptObject
{
    QWidget* _thisWidget;
    Gui* _gui;

public:

    PanelWidget(const std::string& scriptName,
                QWidget* thisWidget,
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

    void pushUndoCommand(const UndoCommandPtr& command);

    // Takes ownership, command is deleted when returning call
    void pushUndoCommand(UndoCommand* command);

    /*
     * @brief Called whenever this panel is made the current tab in the parent tab widget
     */
    virtual void onPanelMadeCurrent() {}

    virtual bool saveProjection(SERIALIZATION_NAMESPACE::ViewportData* data) { Q_UNUSED(data); return false; }

    virtual bool loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& data) { Q_UNUSED(data); return false; }

    virtual QIcon getIcon() const
    {
        return QIcon();
    }

protected:

    virtual void onScriptNameChanged() OVERRIDE;

    virtual void onLabelChanged() OVERRIDE;

    virtual boost::shared_ptr<QUndoStack> getUndoStack() const { return boost::shared_ptr<QUndoStack>(); }

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
