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

#ifndef _Gui_RightClickableWidget_h_
#define _Gui_RightClickableWidget_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include <QTabWidget>
#include <QDialog>
#include "Global/GlobalDefines.h"

#include "Engine/DockablePanelI.h"

class KnobI;
class KnobGui;
class KnobHolder;
class NodeGui;
class Gui;
class Page_Knob;
class QVBoxLayout;
class Button;
class QUndoStack;
class QUndoCommand;
class QGridLayout;
class RotoPanel;
class MultiInstancePanel;
class QTabWidget;
class Group_Knob;
class DockablePanel;

class RightClickableWidget : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    DockablePanel* panel;
    
public:
    
    
    RightClickableWidget(DockablePanel* panel,QWidget* parent)
    : QWidget(parent)
    , panel(panel)
    {
        setObjectName("SettingsPanel");
    }
    
    virtual ~RightClickableWidget() {}
    
    DockablePanel* getPanel() const { return panel; }
    
Q_SIGNALS:
    
    void rightClicked(const QPoint& p);
    void escapePressed();
    
private:
    
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    
};

#endif // _Gui_RightClickableWidget_h_
