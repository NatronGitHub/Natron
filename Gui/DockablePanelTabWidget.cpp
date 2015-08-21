//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "DockablePanelTabWidget.h"

#include <cfloat>
#include <iostream>
#include <fstream>
#include <QLayout>
#include <QAction>
#include <QApplication>
#include <QTabWidget>
#include <QStyle>
#include <QUndoStack>
#include <QGridLayout>
#include <QUndoCommand>
#include <QFormLayout>
#include <QDebug>
#include <QToolTip>
#include <QHeaderView>
#include <QMutex>
#include <QTreeWidget>
#include <QCheckBox>
#include <QHeaderView>
#include <QColorDialog>
#include <QTimer>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QPaintEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QPainter>
#include <QImage>
#include <QToolButton>
#include <QDialogButtonBox>

#include <ofxNatron.h>

GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/utility.hpp>
GCC_DIAG_ON(unused-parameter)

#include "Engine/BackDrop.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/NoOp.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveEditorUndoRedo.h"
#include "Gui/CurveGui.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/KnobGuiTypes.h"
#include "Gui/KnobGuiTypes.h" // for Group_KnobGui
#include "Gui/KnobUndoCommand.h"
#include "Gui/LineEdit.h"
#include "Gui/Menu.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RotoPanel.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/TabWidget.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#define NATRON_FORM_LAYOUT_LINES_SPACING 0
#define NATRON_SETTINGS_VERTICAL_SPACING_PIXELS 3

#define NATRON_VERTICAL_BAR_WIDTH 4
using std::make_pair;
using namespace Natron;


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

