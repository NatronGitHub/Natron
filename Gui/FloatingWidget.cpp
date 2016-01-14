/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "FloatingWidget.h"

#include <cassert>
#include <algorithm> // min, max

#include <QHBoxLayout>
#include <QApplication> // qApp
#include <QDesktopWidget>
#include <QScrollArea>
#include "Engine/Project.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"

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
        QObject::connect(project.get(),SIGNAL(projectNameChanged(QString, bool)), this, SLOT(onProjectNameChanged(QString, bool)));
        onProjectNameChanged(project->getProjectPath(), false);
    }
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    _scrollArea = new QScrollArea(this);
    _layout->addWidget(_scrollArea);
    _scrollArea->setWidgetResizable(true);
}

void
FloatingWidget::onProjectNameChanged(const QString& filePath, bool modified)
{
   // handles window title and appearance formatting
    // http://doc.qt.io/qt-4.8/qwidget.html#windowModified-prop
    setWindowModified(modified);
    // http://doc.qt.io/qt-4.8/qwidget.html#windowFilePath-prop
    setWindowFilePath(filePath);
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
