/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef PANELWIDGET_H
#define PANELWIDGET_H

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/ScriptObject.h"

class QKeyEvent;
class Gui;
class TabWidget;
class PanelWidget : public ScriptObject
{
    QWidget* _thisWidget;
    Gui* _gui;
public:
    
    PanelWidget(QWidget* thisWidget,Gui* gui);
    
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
    
protected:
    
    virtual void notifyGuiClosing() {}
    
    /**
     * @brief To be called in the enterEvent handler of all derived classes.
     **/
    void enterEventBase();
    
    /**
     * @brief To be called in the leaveEvent handler of all derived classes.
     **/
    void leaveEventBase();
  
 

};

#endif // PANELWIDGET_H
