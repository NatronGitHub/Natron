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

#include "RightClickableWidget.h"

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
#include "Gui/RotoPanel.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/TabWidget.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "DockablePanelPrivate.h"

#define NATRON_FORM_LAYOUT_LINES_SPACING 0
#define NATRON_SETTINGS_VERTICAL_SPACING_PIXELS 3


#define NATRON_VERTICAL_BAR_WIDTH 4
using std::make_pair;
using namespace Natron;


void
RightClickableWidget::mousePressEvent(QMouseEvent* e)
{
    if (buttonDownIsRight(e)) {
        QWidget* underMouse = qApp->widgetAt(e->globalPos());
        if (underMouse == this) {
            Q_EMIT rightClicked(e->pos());
            e->accept();
        }
    } else {
        QWidget::mousePressEvent(e);
    }
}

void
RightClickableWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        Q_EMIT escapePressed();
    }
    QWidget::keyPressEvent(e);
}

void
RightClickableWidget::enterEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    QWidget* currentFocus = qApp->focusWidget();
    
    bool canSetFocus = !currentFocus ||
    dynamic_cast<ViewerGL*>(currentFocus) ||
    dynamic_cast<CurveWidget*>(currentFocus) ||
    dynamic_cast<Histogram*>(currentFocus) ||
    dynamic_cast<NodeGraph*>(currentFocus) ||
    dynamic_cast<QToolButton*>(currentFocus) ||
    currentFocus->objectName() == "Properties";
    
    if (canSetFocus) {
        setFocus();
    }
    QWidget::enterEvent(e);
}

