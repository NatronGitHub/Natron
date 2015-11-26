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

#include "Gui.h"
#include "GuiPrivate.h"

// #include <cassert>
// #include <fstream>
// #include <algorithm> // min, max
// #include <stdexcept>

// #include "Global/Macros.h"

#include <QtCore/QUrl>
#include <QtGui/QApplication> // qApp
#include <QtGui/QDrag>
#include <QtGui/QStyle>
#include <QtGui/QMenu>
#include <QtGui/QAction>
// #include <QDesktopWidget>

// #include "Engine/NodeSerialization.h"
// #include "Engine/RotoLayer.h"
#include "Engine/Project.h"

// #include "Gui/CurveEditor.h"
// #include "Gui/DockablePanel.h"
// #include "Gui/DopeSheetEditor.h"
// #include "Gui/FloatingWidget.h"
// #include "Gui/Histogram.h"
// #include "Gui/NodeGraph.h"
// #include "Gui/NodeGui.h"
// #include "Gui/NodeSettingsPanel.h"
// #include "Gui/ProjectGui.h"
// #include "Gui/ProjectGuiSerialization.h" // PaneLayout
// #include "Gui/PropertiesBinWrapper.h"
// #include "Gui/SequenceFileDialog.h"
// #include "Gui/Splitter.h"
// #include "Gui/TabWidget.h"
// #include "Gui/ViewerTab.h"



using namespace Natron;

// see https://doc.qt.io/archives/qq/qq18-macfeatures.html#newevents

bool Gui::event(QEvent *event)
{
    if (!isActiveWindow())
        return QMainWindow::event(event);

    // First, we only really care about processing the IconDrag event if our window is active (if it isn't, we just call our super class's function). We check the type and if it's the IconDrag event, we accept the event and check the current keyboard modifiers:

    switch (event->type()) {
        case QEvent::IconDrag: {
#         ifdef Q_OS_MAC
            event->accept();
            Qt::KeyboardModifiers currentModifiers = qApp->keyboardModifiers();
            boost::shared_ptr<Natron::Project> project= _imp->_appInstance->getProject();
            QString curFile = project->getProjectName();

            if (currentModifiers == Qt::NoModifier) {
                QDrag *drag = new QDrag(this);
                QMimeData *data = new QMimeData();
                data->setUrls(QList<QUrl>() << QUrl::fromLocalFile(curFile));
                drag->setMimeData(data);

                // If we have no modifiers then we want to drag the file. So, we first create a QDrag object and create some QMimeData that contains the QUrl of the file that we are working on.

                QPixmap cursorPixmap = style()->standardPixmap(QStyle::SP_FileIcon, 0, this);
                drag->setPixmap(cursorPixmap);

                QPoint hotspot(cursorPixmap.width() - 5, 5);
                drag->setHotSpot(hotspot);

                drag->start(Qt::LinkAction | Qt::CopyAction);

                // Since we want to create the illusion of dragging an icon, we use the icon itself as the drag's pixmap, set the hotspot of the cursor to be at the top-right corner of the pixmap. We then start the drag, allowing only copy and link actions.

                // When users Command+Click on the icon, we want to show a popup menu showing where the file is located. The Command modifier is represented by the generic Qt::ControlModifier flag:

            } else if (currentModifiers==Qt::ControlModifier) {
                QMenu menu(this);
                connect(&menu, SIGNAL(triggered(QAction *)), this, SLOT(openAt(QAction *)));

                QFileInfo info(curFile);
                QAction *action = menu.addAction(info.fileName());

                QPixmap fileIconPixmap;
#pragma message WARN("TODO: use document icon")
                appPTR->getIcon(Natron::NATRON_PIXMAP_APP_ICON, &fileIconPixmap);
                QIcon fileIcon(fileIconPixmap);
                action->setIcon(fileIcon);

                // We start by creating a menu and connecting its triggered signal to the openAt() slot. We then split up our path with the file name and create an action for each part of the path.

                QStringList folders = info.absolutePath().split('/');
                QStringListIterator it(folders);

                it.toBack();
                while (it.hasPrevious()) {
                    QString string = it.previous();
                    QIcon icon;

                    if (!string.isEmpty()) {
                        icon = style()->standardIcon(QStyle::SP_DirClosedIcon, 0, this);
                    } else { // At the root
                        string = "/";
                        icon = style()->standardIcon(QStyle::SP_DriveHDIcon, 0, this);
                    }
                    action = menu.addAction(string);
                    action->setIcon(icon);
                }

                // We also make sure we pick an appropriate icon for that part of the path.

                QPoint pos(QCursor::pos().x() - 20, frameGeometry().y());
                menu.exec(pos);

                // Finally, we place the menu in a nice place and call exec() on it.
                
                // We ignore icon drags using other combinations of modifiers; even so, we have handled the event so we return true:
                
            } else {
                event->ignore();
            }
            return true;
#         endif // Q_OS_MAC
        }
        default:
            return QMainWindow::event(event);
    }
}

// Now we need to take a look at the openAt() slot:

void Gui::openAt(QAction *action)
{
# ifdef Q_OS_MAC
    boost::shared_ptr<Natron::Project> project= _imp->_appInstance->getProject();
    QString curFile = project->getProjectName();
    QString path = curFile.left(curFile.indexOf(action->text())) + action->text();
    if (path == curFile) {
        return;
    }
    QProcess process;
    process.start("/usr/bin/open", QStringList() << path, QIODevice::ReadOnly);
    process.waitForFinished();
# endif // Q_OS_MAC
}

