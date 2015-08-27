/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#include "DockablePanelTabWidget.h"

#include <QTabBar>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
//// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/Knob.h" // KnobI

#include "Gui/ActionShortcuts.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h" // isKeybind
#include "Gui/NodeGraph.h"
#include "Gui/ViewerTab.h"

//using namespace Natron;


namespace {
class NoWheelTabBar : public QTabBar
{
public:
    
    NoWheelTabBar(QWidget* parent) : QTabBar(parent) {}
    
private:
    
    virtual void wheelEvent(QWheelEvent* event) OVERRIDE FINAL
    {
        //ignore wheel events so it doesn't scroll the tabs
        QWidget::wheelEvent(event);
    }
};
}


DockablePanelTabWidget::DockablePanelTabWidget(Gui* gui,QWidget* parent)
    : QTabWidget(parent)
    , _gui(gui)
{
    setFocusPolicy(Qt::ClickFocus);
    QTabBar* tabbar = new NoWheelTabBar(this);
    tabbar->setObjectName("PanelTabBar");
    tabbar->setFocusPolicy(Qt::ClickFocus);
    setTabBar(tabbar);
    setObjectName("PanelTabBar");
}

void
DockablePanelTabWidget::keyPressEvent(QKeyEvent* event)
{
    Qt::Key key = (Qt::Key)event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();
    
    if (isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key)) {
        if ( _gui->getNodeGraph()->getLastSelectedViewer() ) {
            _gui->getNodeGraph()->getLastSelectedViewer()->previousFrame();
        }
    } else if (isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( _gui->getNodeGraph()->getLastSelectedViewer() ) {
            _gui->getNodeGraph()->getLastSelectedViewer()->nextFrame();
        }
    } else {
        QTabWidget::keyPressEvent(event);
    }
}

QSize
DockablePanelTabWidget::sizeHint() const
{
    return currentWidget() ? currentWidget()->sizeHint() + QSize(0,20) : QSize(300,100);
}

QSize
DockablePanelTabWidget::minimumSizeHint() const
{
    return currentWidget() ? currentWidget()->minimumSizeHint() + QSize(0,20) : QSize(300,100);
}

