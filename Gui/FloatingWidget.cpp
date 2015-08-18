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

#include "FloatingWidget.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <algorithm> // min, max

#include <QtCore/QTextStream>
#include <QWaitCondition>
#include <QMutex>
#include <QCoreApplication>
#include <QAction>
#include <QSettings>
#include <QDebug>
#include <QThread>
#include <QCheckBox>
#include <QTimer>
#include <QTextEdit>


#if QT_VERSION >= 0x050000
#include <QScreen>
#endif
#include <QUndoGroup>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QCloseEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QHBoxLayout>
#include <QGraphicsScene>
#include <QApplication>
#include <QMenuBar>
#include <QDesktopWidget>
#include <QToolBar>
#include <QKeySequence>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QMessageBox>
#include <QImage>
#include <QProgressDialog>

#include <cairo/cairo.h>

#include <boost/version.hpp>
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
GCC_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>

#include "Engine/Image.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Lut.h"
#include "Engine/NoOp.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/AboutWindow.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/FromQtEnums.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Histogram.h"
#include "Gui/Menu.h"
#include "Gui/MessageBox.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/ProjectGui.h"
#include "Gui/ProjectGuiSerialization.h"
#include "Gui/PythonPanels.h"
#include "Gui/RenderingProgressDialog.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RotoGui.h"
#include "Gui/ScriptEditor.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ToolButton.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "SequenceParsing.h"

using namespace Natron;

FloatingWidget::FloatingWidget(Gui* gui,
                               QWidget* parent)
    : QWidget(parent, Qt::Tool) // use Qt::Tool instead of Qt::Window to get a minimal titlebar
    , SerializableWindow()
    , _embeddedWidget(0)
    , _scrollArea(0)
    , _layout(0)
    , _gui(gui)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    if (gui) {
        boost::shared_ptr<Project> project = gui->getApp()->getProject();
        QObject::connect(project.get(),SIGNAL(projectNameChanged(QString)), this, SLOT(onProjectNameChanged(QString)));
        QString projectName = project->getProjectName();
        setWindowTitle(projectName);
    }
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    _scrollArea = new QScrollArea(this);
    _layout->addWidget(_scrollArea);
    _scrollArea->setWidgetResizable(true);
}

void
FloatingWidget::onProjectNameChanged(const QString& name)
{
    setWindowTitle(name);
}

static void
closeWidgetRecursively(QWidget* w)
{
    Splitter* isSplitter = dynamic_cast<Splitter*>(w);
    TabWidget* isTab = dynamic_cast<TabWidget*>(w);

    if (!isSplitter && !isTab) {
        return;
    }

    if (isTab) {
        isTab->closePane();
    } else {
        assert(isSplitter);
        for (int i = 0; i < isSplitter->count(); ++i) {
            closeWidgetRecursively( isSplitter->widget(i) );
        }
    }
}

FloatingWidget::~FloatingWidget()
{
    if (_embeddedWidget) {
        closeWidgetRecursively(_embeddedWidget);
    }
}

void
FloatingWidget::setWidget(QWidget* w)
{
    QSize widgetSize = w->size();

    assert(w);
    if (_embeddedWidget) {
        return;
    }
    _embeddedWidget = w;
    _scrollArea->setWidget(w);
    w->setVisible(true);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QDesktopWidget* dw = qApp->desktop();
    assert(dw);
    QRect geom = dw->screenGeometry();
    widgetSize.setWidth( std::min( widgetSize.width(), geom.width() ) );
    widgetSize.setHeight( std::min( widgetSize.height(), geom.height() ) );
    resize(widgetSize);
    show();
}

void
FloatingWidget::removeEmbeddedWidget()
{
    if (!_embeddedWidget) {
        return;
    }
    _scrollArea->takeWidget();
    _embeddedWidget = 0;
    // _embeddedWidget->setVisible(false);
    hide();
}

void
FloatingWidget::moveEvent(QMoveEvent* e)
{
    QWidget::moveEvent(e);
    QPoint p = pos();

    setMtSafePosition( p.x(), p.y() );
}

void
FloatingWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);

    setMtSafeWindowSize( width(), height() );
}

void
FloatingWidget::closeEvent(QCloseEvent* e)
{
    Q_EMIT closed();

    closeWidgetRecursively(_embeddedWidget);
    removeEmbeddedWidget();
    _gui->unregisterFloatingWindow(this);
    QWidget::closeEvent(e);
}
