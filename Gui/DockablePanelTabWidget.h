//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef _Gui_DockablePanelTabWidget_h_
#define _Gui_DockablePanelTabWidget_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
