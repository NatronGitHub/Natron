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

#ifndef _Gui_DockablePanelTabWidget_h_
#define _Gui_DockablePanelTabWidget_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QTabWidget>
class QKeyEvent;

class Gui;

/**
 * @class This is to overcome an issue in QTabWidget : switching tab does not resize the QTabWidget.
 * This class resizes the QTabWidget to the current tab size.
 **/
class DockablePanelTabWidget
    : public QTabWidget
{
    Gui* _gui;
public:

    DockablePanelTabWidget(Gui* gui, QWidget* parent = 0);

    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    
private:
    
    virtual void keyPressEvent(QKeyEvent* event) OVERRIDE FINAL;
};


#endif // _Gui_DockablePanelTabWidget_h_
